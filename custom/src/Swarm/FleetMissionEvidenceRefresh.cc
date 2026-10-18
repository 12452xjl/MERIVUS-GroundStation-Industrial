#include "FleetMissionEvidenceRefresh.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
bool outboundSources(const QObject* owner, const FleetOperatorSession* session)
{
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, session);
}
constexpr qint64 LifetimeMs = 120000;
constexpr qint64 StageTimeoutMs = 4000;
constexpr int RequestIntervalMs = 100;

struct FlagGuard {
    QPointer<QObject> object;
    bool* flag;
    ~FlagGuard() { if (object) *flag = false; }
};

const QStringList& names()
{
    static const QStringList fixed = []() {
        QStringList result = {QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("MPC_LAND_SPEED"),
            QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
            QStringLiteral("MPC_YAWRAUTO_MAX"), QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
            QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
            QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("NAV_ACC_RAD")};
        result.append(ParameterManager::executionRateGuardParameterNames());
        result.append(ParameterManager::executionActionPhaseParameterNames());
        result.append(ParameterManager::executionArrivalParameterNames());
        result.sort();
        return result;
    }();
    return fixed;
}

bool trueValue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool falseValue(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }

bool text(const QVariant& value, int maximum = 128)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool integer(const QVariant& value, double minimum, double maximum, double* result)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < minimum || number > maximum || std::floor(number) != number) return false;
    *result = number;
    return true;
}

bool sequence(const QVariant& value, quint64* result)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    if ((string.size() > 1 && string.front() == QLatin1Char('0'))) return false;
    for (QChar character : string) if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = string.toULongLong(&ok);
    if (!ok || QString::number(number) != string) return false;
    *result = number;
    return true;
}

bool laterSequence(const QVariant& value, const QString& baseline)
{
    quint64 observed = 0, before = 0;
    return sequence(value, &observed) && sequence(baseline, &before) && observed > before;
}

bool notEarlierSequence(const QVariant& value, const QVariant& captured)
{
    quint64 observed = 0, before = 0;
    return sequence(value, &observed) && sequence(captured, &before) && observed >= before && before > 0;
}

Vehicle* actualVehicle(int id)
{
    if (!qgcApp() || !qgcApp()->toolbox() || !qgcApp()->toolbox()->multiVehicleManager()) return nullptr;
    QmlObjectListModel* model = qgcApp()->toolbox()->multiVehicleManager()->vehicles();
    Vehicle* found = nullptr;
    for (int index = 0; model && index < model->count(); ++index) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(index));
        if (vehicle && vehicle->id() == id) {
            if (found) return nullptr;
            found = vehicle;
        }
    }
    return found;
}

bool validRole(const QVariantMap& identity, const QVariantMap& role)
{
    return text(role.value(QStringLiteral("actorId"))) && text(role.value(QStringLiteral("role")))
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.evidence.refresh")
        && trueValue(role.value(QStringLiteral("allowed")))
        && trueValue(identity.value(QStringLiteral("authenticated")))
        && falseValue(role.value(QStringLiteral("actorAuthenticated")))
        && falseValue(role.value(QStringLiteral("executionPermitted")));
}

bool usableVersion(const QVariantMap& observation, int id, int component)
{
    double system = 0, comp = 0, age = 0;
    quint64 receipt = 0;
    return integer(observation.value(QStringLiteral("vehicleId")), 1, 255, &system) && system == id
        && integer(observation.value(QStringLiteral("componentId")), 1, 255, &comp) && comp == component
        && trueValue(observation.value(QStringLiteral("observed")))
        && trueValue(observation.value(QStringLiteral("available"))) && trueValue(observation.value(QStringLiteral("fresh")))
        && observation.value(QStringLiteral("source")).type() == QVariant::String
        && observation.value(QStringLiteral("source")).toString() == QStringLiteral("live-autopilot-version")
        && text(observation.value(QStringLiteral("sourceEpoch")))
        && text(observation.value(QStringLiteral("contentHash")), 64)
        && observation.value(QStringLiteral("contentHash")).toString().size() == 64
        && sequence(observation.value(QStringLiteral("receiptSequence")), &receipt) && receipt > 0
        && integer(observation.value(QStringLiteral("ageMs")), 0, LifetimeMs, &age);
}

bool usableParameter(const QVariantMap& observation, const QString& name, int component, const QString& epoch)
{
    double comp = 0, age = 0;
    quint64 receipt = 0;
    const bool actionPhase = ParameterManager::executionActionPhaseParameterNames().contains(name);
    const bool arrival = ParameterManager::executionArrivalParameterNames().contains(name);
    if (ParameterManager::executionRateGuardParameterNames().contains(name) || actionPhase || arrival) {
        const QVariant raw = observation.value(QStringLiteral("value"));
        const bool actualNumber = raw.type() == QVariant::Int || raw.type() == QVariant::UInt
            || raw.type() == QVariant::LongLong || raw.type() == QVariant::ULongLong || raw.type() == QVariant::Double;
        const double value = raw.toDouble();
        const bool booleanPhase = actionPhase
            && (name == QStringLiteral("COM_OBS_AVOID") || name == QStringLiteral("MPC_LAND_RC_HELP"));
        double type = 0;
        if (!actualNumber || raw.isNull() || !std::isfinite(value)
            || ((actionPhase || arrival) && std::abs(value) > 9007199254740991.0)
            || (arrival && value < 0.0)) return false;
        if (booleanPhase) {
            if ((value != 0.0 && value != 1.0)
                || !integer(observation.value(QStringLiteral("mavParamType")), MAV_PARAM_TYPE_INT32, MAV_PARAM_TYPE_INT32, &type)) return false;
        } else if (std::abs(value) > static_cast<double>(std::numeric_limits<float>::max())
            || static_cast<double>(static_cast<float>(value)) != value
            || !integer(observation.value(QStringLiteral("mavParamType")), MAV_PARAM_TYPE_REAL32, MAV_PARAM_TYPE_REAL32, &type)) return false;
        // Negative disables the aggregate override; zero is an observed active
        // setting, not a legal replacement for the legacy positive rates.
        // Phase values here prove only wire type/numeric domain. Their range
        // and action-branch semantics belong to the independent pure review.
    }
    return observation.value(QStringLiteral("name")).type() == QVariant::String
        && observation.value(QStringLiteral("name")).toString() == name
        && integer(observation.value(QStringLiteral("componentId")), 1, 255, &comp) && comp == component
        && trueValue(observation.value(QStringLiteral("observed")))
        && trueValue(observation.value(QStringLiteral("available"))) && trueValue(observation.value(QStringLiteral("fresh")))
        && trueValue(observation.value(QStringLiteral("cachedValueMatches")))
        && falseValue(observation.value(QStringLiteral("pendingWrite")))
        && observation.value(QStringLiteral("source")).type() == QVariant::String
        && observation.value(QStringLiteral("source")).toString() == QStringLiteral("live-param-value")
        && observation.value(QStringLiteral("managerEpoch")).type() == QVariant::String
        && observation.value(QStringLiteral("managerEpoch")).toString() == epoch
        && sequence(observation.value(QStringLiteral("receiptSequence")), &receipt) && receipt > 0
        && integer(observation.value(QStringLiteral("ageMs")), 0, LifetimeMs, &age);
}

QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("responseAttributionVerified"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

FleetMissionEvidenceRefresh::FleetMissionEvidenceRefresh(QObject* parent) : QObject(parent)
{
    _clock.start();
    _timer.setInterval(RequestIntervalMs);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionEvidenceRefresh::_tick);
}

FleetMissionEvidenceRefresh::~FleetMissionEvidenceRefresh() { _disconnectSources(); }

QVariantMap FleetMissionEvidenceRefresh::current() const
{
    QVariantList vehicles;
    int completed = 0;
    for (const Target& target : _targets) {
        if (target.state == QStringLiteral("observations-renewed")) ++completed;
        vehicles.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
            {QStringLiteral("componentId"), target.componentId}, {QStringLiteral("state"), target.state},
            {QStringLiteral("versionSourceEpoch"), target.versionEpoch},
            {QStringLiteral("versionReceiptSequence"), target.versionReceiptSequence},
            {QStringLiteral("managerEpoch"), target.managerEpoch},
            {QStringLiteral("parameterReceipts"), target.parameterReceipts}});
    }
    return {{QStringLiteral("ok"), _state == QStringLiteral("observations-renewed")},
        {QStringLiteral("requestId"), _requestId}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason}, {QStringLiteral("active"), _active},
        {QStringLiteral("actorId"), _identityReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("vehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), vehicles},
        {QStringLiteral("completedVehicleCount"), completed}, {QStringLiteral("totalVehicleCount"), _targets.size()},
        {QStringLiteral("currentVehicleId"), _active && _targetIndex < _targets.size()
            ? _targets.at(_targetIndex).vehicleId : 0},
        {QStringLiteral("currentParameter"), _active && (_stage == Stage::NeedParameter || _stage == Stage::AwaitParameter)
            && _parameterIndex < names().size() ? names().at(_parameterIndex) : QString()},
        {QStringLiteral("remainingMs"), _active ? static_cast<double>(qMax<qint64>(0, LifetimeMs - (_clock.elapsed() - _createdAtMs))) : 0.0},
        {QStringLiteral("requestLifetimeMs"), static_cast<double>(LifetimeMs)},
        {QStringLiteral("stageTimeoutMs"), static_cast<double>(StageTimeoutMs)},
        {QStringLiteral("requestIntervalMs"), RequestIntervalMs},
        {QStringLiteral("readOnlyRequestsMayRemainInflight"), _issuedAnyRequest},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("responseAttributionVerified"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("reviewRestored"), false},
        {QStringLiteral("uploadApprovalRestored"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetMissionEvidenceRefresh::_same(quint64 generation) const
{
    return _active && generation == _generation && generation != 0 && _createdAtMs >= 0
        && _clock.elapsed() - _createdAtMs < LifetimeMs;
}

bool FleetMissionEvidenceRefresh::_targetReady(const Target& suppliedTarget) const
{
    const Target target = suppliedTarget;
    const QPointer<FleetMissionEvidenceRefresh> self(const_cast<FleetMissionEvidenceRefresh*>(this));
    if (!target.vehicle || !target.parameters || !target.mission || !target.links || !target.primary
        || actualVehicle(target.vehicleId) != target.vehicle.data()) return false;
    const SharedLinkInterfacePtr primary = target.links->primaryLink().lock();
    const auto alive = [&]() { return self && target.vehicle && target.parameters && target.mission && target.links && target.primary; };
    if (!primary || primary.get() != target.primary.data() || !target.primary->isConnected() || !alive()) return false;
    if (target.primary->isLogReplay() || !alive()) return false;
    if (target.primary->isMockLink() || !alive()) return false;
    return alive()
        && target.vehicle->thread() == thread() && target.parameters->thread() == thread()
        && target.mission->thread() == thread() && target.links->thread() == thread()
        && target.vehicle->vehicleLinkManager() == target.links.data()
        && target.vehicle->parameterManager() == target.parameters.data() && target.parameters->vehicle() == target.vehicle.data()
        && target.vehicle->missionManager() == target.mission.data() && target.vehicle->defaultComponentId() == target.componentId
        && !target.vehicle->isOfflineEditingVehicle() && target.vehicle->isInitialConnectComplete()
        && target.vehicle->px4Firmware() && target.vehicle->multiRotor()
        && !target.vehicle->armed() && !target.vehicle->flying() && !target.links->communicationLost()
        && target.parameters->parametersReady() && !target.parameters->missingParameters() && !target.parameters->pendingWrites()
        && !target.mission->inProgress() && !target.mission->transportCallInProgress()
        && actualVehicle(target.vehicleId) == target.vehicle.data();
}

bool FleetMissionEvidenceRefresh::_readVersion(const Target& suppliedTarget, quint64 generation,
    QVariantMap* observation, bool* sourceMatches) const
{
    const Target target = suppliedTarget;
    const QPointer<FleetMissionEvidenceRefresh> self(const_cast<FleetMissionEvidenceRefresh*>(this));
    const auto alive = [&]() { return self && outboundSources(this, _session.data()) && _same(generation) && target.vehicle && target.primary && target.links
        && target.parameters && target.mission; };
    if (!alive()) return false;
    const bool before = target.vehicle->autopilotVersionObservationSourceMatches(target.primary.data());
    if (!alive()) return false;
    const QVariantMap captured = target.vehicle->autopilotVersionObservation(LifetimeMs);
    if (!alive()) return false;
    const bool after = target.vehicle->autopilotVersionObservationSourceMatches(target.primary.data());
    if (!alive()) return false;
    *observation = captured; *sourceMatches = before && after;
    return true;
}

bool FleetMissionEvidenceRefresh::_readParameter(const Target& suppliedTarget, quint64 generation,
    const QString& suppliedName, QVariantMap* observation, bool* sourceMatches) const
{
    const Target target = suppliedTarget;
    const QString name = suppliedName;
    const QPointer<FleetMissionEvidenceRefresh> self(const_cast<FleetMissionEvidenceRefresh*>(this));
    const auto alive = [&]() { return self && outboundSources(this, _session.data()) && _same(generation) && target.vehicle && target.primary && target.links
        && target.parameters && target.mission; };
    if (!alive()) return false;
    const bool before = target.parameters->parameterObservationSourceMatches(target.componentId, name, target.primary.data());
    if (!alive()) return false;
    const QVariantMap captured = target.parameters->parameterObservation(target.componentId, name, LifetimeMs);
    if (!alive()) return false;
    const bool after = target.parameters->parameterObservationSourceMatches(target.componentId, name, target.primary.data());
    if (!alive()) return false;
    *observation = captured; *sourceMatches = before && after;
    return true;
}

bool FleetMissionEvidenceRefresh::_evidenceCurrent(const Target& suppliedTarget) const
{
    const Target target = suppliedTarget;
    const quint64 generation = _generation;
    if (target.versionReceiptSequence.isEmpty()) return true;
    QVariantMap version;
    bool sourceMatches = false;
    if (!_readVersion(target, generation, &version, &sourceMatches)) return false;
    if (!sourceMatches
        || !usableVersion(version, target.vehicleId, target.componentId)
        || version.value(QStringLiteral("sourceEpoch")).toString() != target.versionEpoch
        || version.value(QStringLiteral("contentHash")).toString() != target.versionHash
        || !notEarlierSequence(version.value(QStringLiteral("receiptSequence")), target.versionReceiptSequence)) return false;
    for (const QVariant& value : target.parameterReceipts) {
        const QVariantMap row = value.toMap();
        const QString name = row.value(QStringLiteral("name")).toString();
        QVariantMap parameter;
        if (!_readParameter(target, generation, name, &parameter, &sourceMatches)) return false;
        if (!sourceMatches
            || !usableParameter(parameter, name, target.componentId, target.managerEpoch)
            || !notEarlierSequence(parameter.value(QStringLiteral("receiptSequence")), row.value(QStringLiteral("receiptSequence")))) return false;
    }
    return true;
}

bool FleetMissionEvidenceRefresh::_gate(quint64 generation)
{
    if (_checking || thread() != QThread::currentThread()) { _reentered = true; return false; }
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionEvidenceRefresh> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap identity = _identityReceipt;
    const QVariantMap role = _roleReceipt;
    const QVariantList ids = _vehicleIds;
    const QList<Target> targets = _targets;
    const int targetIndex = _targetIndex, parameterIndex = _parameterIndex;
    const Stage stage = _stage;
    const auto unchanged = [&]() {
        return self && outboundSources(this, session.data()) && !_reentered && _same(generation) && session && roles && owner
            && _session == session && _roles == roles && _authorization == owner
            && _identityReceipt == identity && _roleReceipt == role && _vehicleIds == ids
            && _targetIndex == targetIndex && _parameterIndex == parameterIndex && _stage == stage
            && session->thread() == thread() && roles->thread() == thread() && owner->thread() == thread();
    };
    const auto nativeCurrent = [&]() {
        if (!unchanged() || !roles->actionSnapshotCurrent(role) || !validRole(identity, role)) return false;
        if (!session->identitySnapshotCurrent(identity) || !unchanged()) return false;
        for (const Target& target : targets)
            if (!_targetReady(target) || !_evidenceCurrent(target) || !unchanged() || !_targetReady(target) || !unchanged()) return false;
        return true;
    };
    if (!nativeCurrent()) return false;
    QVariant sources;
    const bool sourceInvoked = QMetaObject::invokeMethod(owner.data(), "fleetEvidenceRefreshSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, sources), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))));
    if (!unchanged() || !sourceInvoked || !trueValue(sources) || !nativeCurrent()) return false;
    QVariant allowed;
    const bool allowedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetEvidenceRefreshAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(ids)));
    return unchanged() && allowedInvoked && trueValue(allowed) && nativeCurrent();
}

void FleetMissionEvidenceRefresh::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}

void FleetMissionEvidenceRefresh::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _finish(generation, QStringLiteral("stale"), QStringLiteral("native-source-changed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), _authorization.data()})
        _connections.append(connect(object, &QObject::destroyed, this, stale));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        const QPointer<FleetMissionEvidenceRefresh> self(this);
        const QPointer<FleetOperatorSession> session(_session);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QVariantMap identity = _identityReceipt;
        const QVariantMap role = _roleReceipt;
        const bool currentIdentity = session && session->identitySnapshotCurrent(identity);
        if (!self) return;
        // Expiring an unrelated, unused approver receipt does not replace the
        // frozen primary identity. Actual logout/lock/new identity still fails.
        if (currentIdentity && _same(generation) && session && roles && _session == session && _roles == roles
            && _identityReceipt == identity && _roleReceipt == role && roles->actionSnapshotCurrent(role)) return;
        _finish(generation, QStringLiteral("stale"), QStringLiteral("operator-identity-changed"));
    }));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index);
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.parameters.data()),
                static_cast<QObject*>(target.mission.data()), static_cast<QObject*>(target.links.data()),
                static_cast<QObject*>(target.primary.data())}) _connections.append(connect(object, &QObject::destroyed, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, [stale](bool armed) { if (armed) stale(); }));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, [stale](bool flying) { if (flying) stale(); }));
        _connections.append(connect(target.vehicle.data(), &Vehicle::firmwareTypeChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::vehicleTypeChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this,
            [stale](bool pending) { if (pending) stale(); }));
        _connections.append(connect(target.parameters.data(), &ParameterManager::parametersReadyChanged, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::missingParametersChanged, this, stale));
        _connections.append(connect(target.mission.data(), &MissionManager::inProgressChanged, this,
            [stale](bool busy) { if (busy) stale(); }));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this,
            [this, generation, index]() {
                if (!_active || _generation != generation) return;
                const bool notReadYet = _targets.at(index).versionReceiptSequence.isEmpty();
                if (notReadYet && (index > _targetIndex || (_stage == Stage::NeedVersion || _stage == Stage::AwaitVersion))) return;
                _finish(generation, QStringLiteral("stale"), QStringLiteral("version-source-changed-after-refresh"));
            }));
    }
}

void FleetMissionEvidenceRefresh::_finish(quint64 generation, const QString& state, const QString& reason)
{
    if (!_active || _generation != generation) return;
    _active = false; _state = state; _reason = reason;
    _timer.stop(); _disconnectSources();
    if (_targetIndex < _targets.size() && state != QStringLiteral("observations-renewed"))
        _targets[_targetIndex].state = state;
    emit changed();
}

bool FleetMissionEvidenceRefresh::_publish(quint64 generation)
{
    const QPointer<FleetMissionEvidenceRefresh> self(this);
    emit changed();
    return self && _same(generation);
}

QVariantMap FleetMissionEvidenceRefresh::requestRefresh(QObject* sessionObject, QObject* roleObject,
    QObject* authorizationOwner, const QVariantList& requestedIds, bool operatorConfirmed)
{
    const QVariantList suppliedIds = requestedIds;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _active || _busy || _checking || !operatorConfirmed
        || suppliedIds.isEmpty() || suppliedIds.size() > 32)
        return rejection(QStringLiteral("confirmation-idle-native-owner-and-bounded-targets-required"));
    _busy = true;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    QPointer<FleetMissionEvidenceRefresh> self(this);
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(sessionObject);
    QPointer<FleetRolePolicy> roles = qobject_cast<FleetRolePolicy*>(roleObject);
    QPointer<QObject> owner(authorizationOwner);
    const auto sourcesAlive = [&]() { return self && session && roles && owner && outboundSources(this, session.data()); };
    if (!sourcesAlive() || session->thread() != thread() || roles->thread() != thread() || owner->thread() != thread())
        return rejection(QStringLiteral("actual-native-identity-role-owner-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!sourcesAlive()) return rejection(QStringLiteral("identity-source-destroyed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.evidence.refresh"));
    if (!session->identitySnapshotCurrent(identity) || !sourcesAlive()
        || !roles->actionSnapshotCurrent(role) || !validRole(identity, role))
        return rejection(QStringLiteral("authenticated-refresh-role-required"));
    if (_generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("refresh-generation-exhausted"));
    QList<Target> targets;
    QVariantList ids;
    QSet<int> seen;
    for (const QVariant& value : suppliedIds) {
        double number = 0;
        if (!integer(value, 1, 255, &number) || seen.contains(static_cast<int>(number))) return rejection(QStringLiteral("target-identity-invalid"));
        Target target;
        target.vehicleId = static_cast<int>(number);
        target.vehicle = actualVehicle(target.vehicleId);
        if (!target.vehicle) return rejection(QStringLiteral("actual-vehicle-unavailable-or-ambiguous"));
        target.componentId = target.vehicle->defaultComponentId();
        target.parameters = target.vehicle->parameterManager(); target.mission = target.vehicle->missionManager();
        target.links = target.vehicle->vehicleLinkManager();
        const SharedLinkInterfacePtr primary = target.links ? target.links->primaryLink().lock() : SharedLinkInterfacePtr();
        target.primary = primary.get();
        if (target.componentId < 1 || target.componentId > 255 || !_targetReady(target) || !sourcesAlive())
            return rejection(QStringLiteral("grounded-settled-live-px4-multirotor-required"));
        seen.insert(target.vehicleId); ids.append(number); targets.append(target);
    }
    if (!sourcesAlive()) return rejection(QStringLiteral("native-episode-changed-before-refresh-admission"));
    _disconnectSources();
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _authorization = owner; _identityReceipt = identity; _roleReceipt = role;
    _targets = targets; _vehicleIds = ids; _requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _state = QStringLiteral("refreshing"); _reason.clear(); _active = true; _createdAtMs = _clock.elapsed();
    _targetIndex = 0; _parameterIndex = 0; _stage = Stage::NeedVersion;
    _stageDeadlineMs = _createdAtMs + StageTimeoutMs; _lastRequestAtMs = -1;
    _baselineSequence.clear(); _nativeRequestId.clear(); _issuedAnyRequest = false;
    _connectSources(generation);
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("initial-refresh-context-denied"));
        return rejection(QStringLiteral("initial-refresh-context-denied"));
    }
    _timer.start();
    if (!_publish(generation) || !self) return rejection(QStringLiteral("refresh-revoked-at-publication"));
    if (!outboundSources(this, session.data())) {
        _finish(generation, QStringLiteral("stale"), QStringLiteral("native-episode-changed-at-refresh-publication"));
        return rejection(QStringLiteral("refresh-revoked-at-publication"));
    }
    QVariantMap result = current();
    result.insert(QStringLiteral("ok"), true); // Accepted acquisition, not fresh evidence yet.
    result.insert(QStringLiteral("requestAccepted"), true);
    return result;
}

void FleetMissionEvidenceRefresh::_nextParameter(qint64 now)
{
    ++_parameterIndex;
    if (_parameterIndex < names().size()) {
        _stage = Stage::NeedParameter; _stageDeadlineMs = now + StageTimeoutMs;
        return;
    }
    _targets[_targetIndex].state = QStringLiteral("observations-renewed");
    ++_targetIndex;
    _parameterIndex = 0; _stage = Stage::NeedVersion; _stageDeadlineMs = now + StageTimeoutMs;
}

void FleetMissionEvidenceRefresh::_tick()
{
    if (!_active || _busy || _checking) return;
    _busy = true;
    FlagGuard working{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionEvidenceRefresh> self(this);
    const quint64 generation = _generation;
    qint64 now = _clock.elapsed();
    if (!_same(generation)) { _finish(generation, QStringLiteral("failed"), QStringLiteral("refresh-total-deadline-expired")); return; }
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("refresh-context-invalidated"));
        return;
    }
    now = _clock.elapsed(); // WTS/controller callbacks may have consumed time.
    if (_targetIndex >= _targets.size()) {
        _finish(generation, QStringLiteral("observations-renewed"), QString());
        return;
    }
    if (now >= _stageDeadlineMs) { _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return; }
    const Target target = _targets.at(_targetIndex);
    if (_stage == Stage::NeedVersion || _stage == Stage::NeedParameter) {
        if (_lastRequestAtMs >= 0 && now - _lastRequestAtMs < RequestIntervalMs) return;
        const Stage requestedStage = _stage;
        const int requestedTarget = _targetIndex, requestedParameter = _parameterIndex;
        const QString name = requestedStage == Stage::NeedParameter ? names().at(_parameterIndex) : QString();
        const QPointer<FleetOperatorSession> sendingSession(_session);
        const QPointer<FleetRolePolicy> sendingRoles(_roles);
        const QPointer<QObject> sendingOwner(_authorization);
        const QVariantMap sendingIdentity = _identityReceipt;
        const QVariantMap sendingRole = _roleReceipt;
        const QVariantList sendingIds = _vehicleIds;
        const qint64 sendingDeadline = _stageDeadlineMs;
        const auto authorizationFence = [self, generation, requestedStage, requestedTarget, requestedParameter,
            target, sendingSession, sendingRoles, sendingOwner, sendingIdentity, sendingRole, sendingIds, sendingDeadline]() {
            const auto same = [&]() {
                if (!self || !outboundSources(self.data(), sendingSession.data()) || !self->_same(generation) || self->_clock.elapsed() >= sendingDeadline
                    || self->_stageDeadlineMs != sendingDeadline || self->_stage != requestedStage
                    || self->_targetIndex != requestedTarget || self->_parameterIndex != requestedParameter
                    || requestedTarget < 0 || requestedTarget >= self->_targets.size()
                    || !sendingSession || !sendingRoles || !sendingOwner
                    || self->_session != sendingSession || self->_roles != sendingRoles || self->_authorization != sendingOwner
                    || self->_identityReceipt != sendingIdentity || self->_roleReceipt != sendingRole
                    || self->_vehicleIds != sendingIds) return false;
                const Target currentTarget = self->_targets.at(requestedTarget);
                return target.vehicle && target.parameters && target.mission && target.links && target.primary
                    && currentTarget.vehicle == target.vehicle && currentTarget.parameters == target.parameters
                    && currentTarget.mission == target.mission && currentTarget.links == target.links
                    && currentTarget.primary == target.primary && currentTarget.componentId == target.componentId
                    && currentTarget.vehicleId == target.vehicleId && currentTarget.managerEpoch == target.managerEpoch
                    && currentTarget.versionEpoch == target.versionEpoch && currentTarget.versionHash == target.versionHash;
            };
            // The native sender calls this AFTER outgoing plugin callbacks and
            // immediately before queuing. A post-send check cannot substitute
            // for it: cancellation/logout may happen before a packet is queued.
            return same() && self->_gate(generation) && same();
        };
        _lastRequestAtMs = now;
        // Conservatively retain this warning even if a synchronous callback
        // cancels/deletes the request before the native result is returned.
        _issuedAnyRequest = true;
        const QVariantMap issued = requestedStage == Stage::NeedVersion
            ? target.vehicle->requestFleetAutopilotVersion(target.primary.data(), authorizationFence)
            : target.parameters->requestFleetParameterObservation(target.componentId, name, target.primary.data(), authorizationFence);
        if (!self || !_same(generation)) return;
        if (_stage != requestedStage || _targetIndex != requestedTarget || _parameterIndex != requestedParameter
            || !_gate(generation) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("refresh-context-changed-during-native-request"));
            return;
        }
        if (_clock.elapsed() >= _stageDeadlineMs) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return;
        }
        if (!trueValue(issued.value(QStringLiteral("ok"))) || !trueValue(issued.value(QStringLiteral("requestIssued")))) {
            const QVariant reason = issued.value(QStringLiteral("reason"));
            if (falseValue(issued.value(QStringLiteral("requestIssued"))) && reason.type() == QVariant::String
                && (reason.toString() == QStringLiteral("fleet-evidence-request-busy")
                    || reason.toString() == QStringLiteral("fleet-evidence-rate-limited")
                    || reason.toString() == QStringLiteral("fleet-evidence-status-wait"))) return;
            _finish(generation, QStringLiteral("failed"), QStringLiteral("native-read-only-request-not-issued"));
            return;
        }
        quint64 baseline = 0;
        double vehicle = 0, component = 0;
        const QString epochKey = requestedStage == Stage::NeedVersion ? QStringLiteral("sourceEpoch") : QStringLiteral("managerEpoch");
        if (!sequence(issued.value(QStringLiteral("baselineReceiptSequence")), &baseline)
            || !text(issued.value(epochKey)) || !text(issued.value(QStringLiteral("requestId")), 64)
            || !integer(issued.value(QStringLiteral("vehicleId")), 1, 255, &vehicle) || vehicle != target.vehicleId
            || !integer(issued.value(QStringLiteral("componentId")), 1, 255, &component) || component != target.componentId
            || (requestedStage == Stage::NeedParameter && (issued.value(QStringLiteral("name")).type() != QVariant::String
                || issued.value(QStringLiteral("name")).toString() != name
                || issued.value(epochKey).toString() != target.managerEpoch))) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("native-request-receipt-shape-or-epoch-invalid")); return;
        }
        _lastRequestAtMs = _clock.elapsed();
        _baselineSequence = issued.value(QStringLiteral("baselineReceiptSequence")).toString();
        _nativeRequestId = issued.value(QStringLiteral("requestId")).toString();
        if (requestedStage == Stage::NeedVersion) {
            _targets[_targetIndex].versionEpoch = issued.value(epochKey).toString();
            _targets[_targetIndex].state = QStringLiteral("awaiting-version-receipt"); _stage = Stage::AwaitVersion;
        } else {
            _targets[_targetIndex].state = QStringLiteral("awaiting-parameter-receipt"); _stage = Stage::AwaitParameter;
        }
        _publish(generation);
        return;
    }
    if (_stage == Stage::AwaitVersion) {
        QVariantMap version;
        bool sourceMatches = false;
        if (!_readVersion(target, generation, &version, &sourceMatches)) return;
        if (_clock.elapsed() >= _stageDeadlineMs) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return;
        }
        if (version.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
            || version.value(QStringLiteral("sourceEpoch")).toString() != target.versionEpoch) {
            _finish(generation, QStringLiteral("stale"), QStringLiteral("version-source-epoch-changed")); return;
        }
        if (!sourceMatches || !usableVersion(version, target.vehicleId, target.componentId)
            || !laterSequence(version.value(QStringLiteral("receiptSequence")), _baselineSequence)) return;
        QVariantMap parameter;
        if (!_readParameter(target, generation, names().first(), &parameter, &sourceMatches)) return;
        if (!text(parameter.value(QStringLiteral("managerEpoch"))) || !_gate(generation) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("post-version-parameter-epoch-unavailable")); return;
        }
        QVariantMap versionAfterGate, parameterAfterGate;
        bool versionSourceMatches = false;
        if (!_readVersion(target, generation, &versionAfterGate, &versionSourceMatches)
            || !_readParameter(target, generation, names().first(), &parameterAfterGate, &sourceMatches)) return;
        if (_clock.elapsed() >= _stageDeadlineMs) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return;
        }
        if (!versionSourceMatches || !usableVersion(versionAfterGate, target.vehicleId, target.componentId)
            || versionAfterGate.value(QStringLiteral("sourceEpoch")) != version.value(QStringLiteral("sourceEpoch"))
            || versionAfterGate.value(QStringLiteral("contentHash")) != version.value(QStringLiteral("contentHash"))
            || parameterAfterGate.value(QStringLiteral("managerEpoch")) != parameter.value(QStringLiteral("managerEpoch"))) {
            _finish(generation, QStringLiteral("stale"), QStringLiteral("version-or-parameter-source-changed-at-capture")); return;
        }
        _targets[_targetIndex].versionHash = version.value(QStringLiteral("contentHash")).toString();
        _targets[_targetIndex].versionReceiptSequence = version.value(QStringLiteral("receiptSequence")).toString();
        _targets[_targetIndex].managerEpoch = parameter.value(QStringLiteral("managerEpoch")).toString();
        _targets[_targetIndex].state = QStringLiteral("refreshing-parameters");
        _parameterIndex = 0; _stage = Stage::NeedParameter; _stageDeadlineMs = _clock.elapsed() + StageTimeoutMs;
        _publish(generation);
        return;
    }
    const QString name = names().at(_parameterIndex);
    QVariantMap parameter;
    bool sourceMatches = false;
    if (!_readParameter(target, generation, name, &parameter, &sourceMatches)) return;
    if (_clock.elapsed() >= _stageDeadlineMs) {
        _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return;
    }
    if (parameter.value(QStringLiteral("managerEpoch")).type() != QVariant::String
        || parameter.value(QStringLiteral("managerEpoch")).toString() != target.managerEpoch) {
        _finish(generation, QStringLiteral("stale"), QStringLiteral("parameter-source-epoch-changed")); return;
    }
    if (!sourceMatches || !usableParameter(parameter, name, target.componentId, target.managerEpoch)
        || !laterSequence(parameter.value(QStringLiteral("receiptSequence")), _baselineSequence)) return;
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("parameter-context-changed-at-observation")); return;
    }
    if (_clock.elapsed() >= _stageDeadlineMs) {
        _finish(generation, QStringLiteral("failed"), QStringLiteral("new-raw-receipt-stage-timeout")); return;
    }
    _targets[_targetIndex].parameterReceipts.append(QVariantMap{{QStringLiteral("name"), name},
        {QStringLiteral("receiptSequence"), parameter.value(QStringLiteral("receiptSequence"))},
        {QStringLiteral("managerEpoch"), target.managerEpoch}, {QStringLiteral("source"), QStringLiteral("live-param-value")},
        {QStringLiteral("observedAtRefreshMonotonicMs"), static_cast<double>(_clock.elapsed())},
        {QStringLiteral("responseAttributionVerified"), false}});
    _nextParameter(_clock.elapsed());
    _publish(generation);
}

bool FleetMissionEvidenceRefresh::cancel(const QString& requestId)
{
    if (thread() != QThread::currentThread() || !_active || requestId != _requestId) return false;
    _finish(_generation, QStringLiteral("canceled"), QStringLiteral("operator-canceled-read-only-refresh"));
    return true;
}

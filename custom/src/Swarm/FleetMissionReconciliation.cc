#include "FleetMissionReconciliation.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionPlanStore.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QSet>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
constexpr qint64 LifetimeMs = 120000;
constexpr int MaximumItems = 2048;
constexpr int MaximumTotalItems = 8192;
struct FlagGuard { QPointer<QObject> owner; bool* flag; ~FlagGuard() { if (owner) *flag = false; } };
bool truth(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool falsity(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool uuid(const QString& text) { return text.size() == 36 && !QUuid(text).isNull() && QUuid(text).toString(QUuid::WithoutBraces) == text; }
bool digest(const QString& text)
{
    if (text.size() != 64) return false;
    for (QChar ch : text) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
bool decimal(const QVariant& value, quint64* output)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > 20) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok);
    if (!ok || QString::number(number) != value.toString()) return false;
    *output = number; return true;
}
bool number(const QVariant& value, double low, double high, double* output)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (value.isNull() || !std::isfinite(candidate) || candidate < low || candidate > high || std::floor(candidate) != candidate) return false;
    *output = candidate; return true;
}
bool float32(const QVariant& value, double* output)
{
    if (value.type() != QVariant::Double || value.isNull()) return false;
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || std::abs(candidate) > std::numeric_limits<float>::max()
        || static_cast<double>(static_cast<float>(candidate)) != candidate) return false;
    *output = candidate; return true;
}
QVariantMap reject(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    return role.size() == 9 && truth(identity.value(QStringLiteral("authenticated")))
        && role.value(QStringLiteral("action")) == QStringLiteral("mission.reconcile.read")
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && truth(role.value(QStringLiteral("allowed"))) && falsity(role.value(QStringLiteral("actorAuthenticated")))
        && falsity(role.value(QStringLiteral("requiresSecondApproval")))
        && falsity(role.value(QStringLiteral("executionPermitted")));
}
// A diagnostic RAW read has its own representation. Never feed RAW evidence
// into the ordinary decoded-item helper or relabel its result as RAW proof.
QVariantMap canonicalRaw(const QVariantList& items)
{
    if (items.isEmpty() || items.size() > MaximumItems) return {};
    QVariantList normalized;
    for (int index = 0; index < items.size(); ++index) {
        if (items.at(index).type() != QVariant::Map) return {};
        const QVariantMap raw = items.at(index).toMap();
        double seq = 0, command = 0, frame = 0, x = 0, y = 0, current = 0, automatic = 0, missionType = 0, system = 0, component = 0;
        if (raw.size() != 16 || !number(raw.value(QStringLiteral("sequenceNumber")), index, index, &seq)
            || !number(raw.value(QStringLiteral("command")), 0, 65535, &command)
            || !number(raw.value(QStringLiteral("frame")), 0, 255, &frame)
            || !number(raw.value(QStringLiteral("x")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &x)
            || !number(raw.value(QStringLiteral("y")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &y)
            || !number(raw.value(QStringLiteral("current")), 0, 1, &current)
            || !number(raw.value(QStringLiteral("autocontinue")), 0, 1, &automatic)
            || !number(raw.value(QStringLiteral("missionType")), MAV_MISSION_TYPE_MISSION, MAV_MISSION_TYPE_MISSION, &missionType)
            || !number(raw.value(QStringLiteral("targetSystem")), 0, 255, &system)
            || !number(raw.value(QStringLiteral("targetComponent")), 0, 255, &component)
            || !digest(raw.value(QStringLiteral("rawPayloadHash")).toString())) return {};
        const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
        const bool geographic = command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_TAKEOFF
            || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
        if ((!speed && !geographic) || frame != (speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL_INT)) return {};
        double p1 = 0, p2 = 0, p3 = 0, p4 = 0, z = 0;
        if (!float32(raw.value(QStringLiteral("param1")), &p1) || !float32(raw.value(QStringLiteral("param2")), &p2)
            || !float32(raw.value(QStringLiteral("param3")), &p3) || !float32(raw.value(QStringLiteral("z")), &z)
            || !raw.contains(QStringLiteral("param4"))
            || (raw.value(QStringLiteral("param4")).isValid() && !float32(raw.value(QStringLiteral("param4")), &p4))
            || (speed && (x != 0 || y != 0))) return {};
        QVariant parameter4;
        QString yawMode, headingError;
        if (!FleetMissionHeadingEncoding::decodeParam4(static_cast<int>(command),
                raw.value(QStringLiteral("param4")).isValid() ? float(p4) : std::numeric_limits<float>::quiet_NaN(),
                parameter4, yawMode, headingError)) return {};
        normalized.append(QVariantMap{{QStringLiteral("sequenceNumber"), index}, {QStringLiteral("command"), static_cast<int>(command)},
            {QStringLiteral("frame"), static_cast<int>(speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL)},
            {QStringLiteral("param1"), p1}, {QStringLiteral("param2"), p2}, {QStringLiteral("param3"), p3},
            {QStringLiteral("param4"), parameter4}, {QStringLiteral("param5"), speed ? 0.0 : x / 1e7},
            {QStringLiteral("param6"), speed ? 0.0 : y / 1e7}, {QStringLiteral("param7"), z},
            {QStringLiteral("autoContinue"), automatic == 1}, {QStringLiteral("isCurrentItem"), current == 1},
            {QStringLiteral("yawMode"), yawMode}});
    }
    return FleetMissionPayloadBuilder::canonicalWireItems(normalized, true);
}
}

FleetMissionReconciliation::FleetMissionReconciliation(FleetMissionDeployment* owner)
    : QObject(owner), _owner(owner), _runEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionReconciliation::_tick);
}
FleetMissionReconciliation::~FleetMissionReconciliation()
{
    _active = false; _disconnectSources(); _detachDrain();
}
bool FleetMissionReconciliation::_owned() const
{
    return _owner && parent() == _owner.data() && _owner->_reconciliation.data() == this && _owner->thread() == thread();
}
bool FleetMissionReconciliation::busy() const { return _active || _busy || _checking || _dispatching || _reservationHeld || _transportDraining; }
bool FleetMissionReconciliation::_same(quint64 generation) const { return generation != 0 && generation == _generation; }
QVariantMap FleetMissionReconciliation::current() const
{
    if (_requestId.isEmpty()) return {};
    QVariantList vehicles;
    int matched = 0, different = 0, unknown = 0, pending = 0;
    for (const Target& target : _targets) {
        if (target.state == QStringLiteral("matched")) ++matched;
        else if (target.state == QStringLiteral("different")) ++different;
        else if (target.state == QStringLiteral("unknown")) ++unknown;
        else ++pending;
        vehicles.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("state"), target.state},
            {QStringLiteral("reason"), target.reason}, {QStringLiteral("readScopeId"), target.scopeId},
            {QStringLiteral("operationId"), QString::number(target.operationId)},
            {QStringLiteral("expectedItemCount"), target.payload.value(QStringLiteral("items")).toList().size()},
            {QStringLiteral("observedItemCount"), target.observedCount},
            {QStringLiteral("expectedContentHash"), target.payload.value(QStringLiteral("contentHash"))},
            {QStringLiteral("observedContentHash"), target.observedHash}, {QStringLiteral("rawItemsFingerprint"), target.rawHash},
            {QStringLiteral("countObserved"), target.observedCount >= 0}, {QStringLiteral("completeReadObserved"), target.observedCount >= 0},
            {QStringLiteral("comparisonBasis"), target.comparisonBasis},
            {QStringLiteral("observedAtMonotonicMs"), target.observedAt < 0 ? QStringLiteral("") : QString::number(target.observedAt)},
            {QStringLiteral("historicalOnly"), true}, {QStringLiteral("currentOnboardContentsProven"), false}});
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), _requestId}, {QStringLiteral("runEpoch"), _runEpoch},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("deploymentId"), _deploymentId},
        {QStringLiteral("deploymentGeneration"), QString::number(_deploymentGeneration)}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason}, {QStringLiteral("active"), _active},
        {QStringLiteral("reservationHeld"), _reservationHeld}, {QStringLiteral("transportDraining"), _transportDraining},
        {QStringLiteral("vehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), vehicles}, {QStringLiteral("totalVehicleCount"), _targets.size()},
        {QStringLiteral("matchedVehicleCount"), matched}, {QStringLiteral("differentVehicleCount"), different},
        {QStringLiteral("unknownVehicleCount"), unknown}, {QStringLiteral("pendingVehicleCount"), pending},
        {QStringLiteral("expectedTotalItemCount"), _expectedTotal}, {QStringLiteral("observedTotalItemCount"), _observedTotal},
        {QStringLiteral("remainingMs"), static_cast<double>(_active ? qMax<qint64>(0, _startedAt + LifetimeMs - _clock.elapsed()) : 0)},
        {QStringLiteral("sourceRepresentation"), QStringLiteral("independent-diagnostic-raw-int-history")},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

bool FleetMissionReconciliation::_globalNativeCurrent() const
{
    if (!_owned() || !_active || _startedAt < 0 || _clock.elapsed() >= _startedAt + LifetimeMs
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(_owner.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(_owner.data(), _session.data())
        || !_session || !_roles || !_store || !_swarm || !_authorization
        || _session->thread() != thread() || _roles->thread() != thread() || _store->thread() != thread()
        || _swarm->thread() != thread() || _authorization->thread() != thread()
        || _owner->_operatorSession != _session || _owner->_rolePolicy != _roles || _owner->_store != _store
        || _owner->_swarm != _swarm || _owner->_authorization != _authorization
        || _owner->_deploymentId != _deploymentId || _owner->_generation != _deploymentGeneration
        || _owner->_state != _deploymentState
        || (_owner->_state != QStringLiteral("failed") && _owner->_state != QStringLiteral("cancelled"))
        || _owner->_active() || _owner->_reservationHeld || _owner->_transportDraining
        || _owner->_beginInProgress || _owner->_gateInProgress || _owner->_inspectionInProgress || _owner->_dispatching
        || _owner->_binding != _binding || _owner->_targets.size() != _targets.size()
        || !roleMatches(_identity, _role) || !_roles->actionSnapshotCurrent(_role)
        || (_reservationHeld && !_swarm->missionDeploymentOwns(_requestId, _vehicleIds))
        || _store->record(_binding.value(QStringLiteral("sourceTaskId")).toString(), _binding.value(QStringLiteral("recordRevision"))) != _record) return false;
    for (int index = 0; index < _targets.size(); ++index) {
        const Target& target = _targets.at(index);
        const FleetMissionDeployment::Target& original = _owner->_targets.at(index);
        if (original.vehicleId != target.vehicleId || original.vehicle != target.vehicle || original.manager != target.manager
            || original.payload != target.payload || (target.vehicle && (target.vehicle->armed() || target.vehicle->flying()))) return false;
    }
    return true;
}
bool FleetMissionReconciliation::_targetSourceCurrent(const Target& target) const
{
    if (!target.vehicle || !target.manager || !target.parameters || !target.links || !target.primary
        || target.vehicle->thread() != thread() || target.manager->thread() != thread()
        || target.parameters->thread() != thread() || target.links->thread() != thread()
        || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
        || target.vehicle->missionManager() != target.manager.data() || target.vehicle->parameterManager() != target.parameters.data()
        || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->vehicleLinkManager() != target.links.data()) return false;
    const WeakLinkInterfacePtr current = target.links->primaryLink();
    return !target.primaryOwner.expired() && !current.expired()
        && !target.primaryOwner.owner_before(current) && !current.owner_before(target.primaryOwner);
}
bool FleetMissionReconciliation::_targetReadable(const Target& target, bool ownRead) const
{
    if (!_targetSourceCurrent(target) || target.vehicle->isOfflineEditingVehicle() || !target.vehicle->isInitialConnectComplete()
        || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || target.vehicle->armed() || target.vehicle->flying()
        || target.links->communicationLost() || !target.parameters->parametersReady() || target.parameters->missingParameters()
        || target.parameters->pendingWrites()) return false;
    const QPointer<FleetMissionReconciliation> self(const_cast<FleetMissionReconciliation*>(this));
    const QVariantMap version = target.vehicle->autopilotVersionObservation(LifetimeMs);
    if (!self || !_targetSourceCurrent(target) || !uuid(target.versionEpoch)
        || version.value(QStringLiteral("sourceEpoch")) != target.versionEpoch) return false;
    const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MPC_XY_CRUISE"), LifetimeMs);
    if (!self || !_targetSourceCurrent(target) || !uuid(target.parameterEpoch)
        || parameter.value(QStringLiteral("managerEpoch")) != target.parameterEpoch) return false;
    if (target.manager->inProgress() || target.manager->transportCallInProgress())
        return ownRead && target.state == QStringLiteral("reading") && !target.scopeId.isEmpty()
            && (target.operationId == 0 ? _dispatching : target.manager->activeOperationId() == target.operationId || target.completionSeen);
    return true;
}
bool FleetMissionReconciliation::nativeReadContextCurrent(PlanManager* manager, Vehicle* vehicle,
    LinkInterface* primary, const QString& scopeId, quint64 operationId) const
{
    if (QThread::currentThread() != thread() || !_globalNativeCurrent() || !_reservationHeld
        || _targetIndex < 0 || _targetIndex >= _targets.size()) return false;
    const Target& target = _targets.at(_targetIndex);
    if (!_targetSourceCurrent(target) || target.manager.data() != manager || target.vehicle.data() != vehicle
        || target.primary.data() != primary || target.scopeId != scopeId || target.state != QStringLiteral("reading")
        || target.deadlineMs < 0 || _clock.elapsed() >= target.deadlineMs) return false;
    if (operationId == 0) return _dispatching && target.operationId == 0 && manager->activeOperationId() == 0;
    return target.operationId == operationId || (_dispatching && target.operationId == 0 && manager->activeOperationId() == operationId);
}

bool FleetMissionReconciliation::_gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (thread() != QThread::currentThread() || !_same(generation) || !_globalNativeCurrent()) return false;
    _checking = true; _reentered = false;
    FlagGuard flag{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionReconciliation> self(this);
    const QPointer<FleetMissionDeployment> owner(_owner);
    const QPointer<FleetOperatorSession> session(_session);
    const QVariantMap identity = _identity;
    const auto same = [&]() { return self && owner && session && _same(generation) && !_reentered
        && _owner == owner && _session == session && _identity == identity && _globalNativeCurrent(); };
    const bool authenticated = session->identitySnapshotCurrent(identity);
    if (!same() || !authenticated) return false;
    const bool sources = owner->_reconciliationSourcesMatch(_session.data(), _roles.data(), _store.data(), _swarm.data(), _authorization.data());
    if (!same() || !sources) return false;
    const bool allowed = owner->_reconciliationContextAllowed(_authorization.data(), _requestId, _binding, _vehicleIds);
    if (!same() || !allowed) return false;
    const bool stillAuthenticated = session->identitySnapshotCurrent(identity);
    return same() && stillAuthenticated;
}

bool FleetMissionReconciliation::_publish(quint64 generation)
{
    const QPointer<FleetMissionReconciliation> self(this); emit changed(); return self && _same(generation);
}

QVariantMap FleetMissionReconciliation::request(QObject* sessionObject, QObject* roleObject, QObject* storeObject,
    QObject* swarmObject, QObject* authorizationOwner, const QString& requestedDeploymentId, bool confirmed)
{
    const QString deploymentId = requestedDeploymentId;
    if (QThread::currentThread() != thread() || !_owned() || busy() || !confirmed || !uuid(deploymentId)
        || _owner->_deploymentId != deploymentId || !_owner->_generation
        || (_owner->_state != QStringLiteral("failed") && _owner->_state != QStringLiteral("cancelled"))
        || _owner->_reservationHeld || _owner->_transportDraining || _owner->_stackBusy()
        || _owner->_beginInProgress || _owner->_gateInProgress || _owner->_inspectionInProgress)
        return reject(QStringLiteral("confirmed-drained-failed-or-cancelled-deployment-required"));
    _busy = true;
    FlagGuard working{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionReconciliation> self(this);
    const QPointer<FleetMissionDeployment> owner(_owner);
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(sessionObject));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(roleObject));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(storeObject));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(swarmObject));
    const QPointer<QObject> authorization(authorizationOwner);
    const quint64 deploymentGeneration = owner->_generation;
    const QString deploymentState = owner->_state;
    const QVariantMap binding = owner->_binding;
    const auto sources = [&]() { return self && owner && session && roles && store && swarm && authorization && _owned()
        && owner->_deploymentId == deploymentId && owner->_generation == deploymentGeneration && owner->_binding == binding
        && owner->_state == deploymentState
        && owner->_operatorSession == session && owner->_rolePolicy == roles && owner->_store == store
        && owner->_swarm == swarm && owner->_authorization == authorization
        && session->thread() == thread() && roles->thread() == thread() && store->thread() == thread()
        && swarm->thread() == thread() && authorization->thread() == thread()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner.data())
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner.data(), session.data()); };
    if (!sources()) return reject(QStringLiteral("original-native-reconciliation-sources-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!sources()) return reject(QStringLiteral("identity-source-changed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.reconcile.read"));
    if (!roleMatches(identity, role) || !roles->actionSnapshotCurrent(role)) return reject(QStringLiteral("reconciliation-role-denied"));
    const bool authenticated = session->identitySnapshotCurrent(identity);
    if (!sources() || !authenticated) return reject(QStringLiteral("current-native-identity-required"));
    const bool sourceMatched = owner->_reconciliationSourcesMatch(session.data(), roles.data(), store.data(), swarm.data(), authorization.data());
    if (!sources() || !sourceMatched) return reject(QStringLiteral("actual-reconciliation-owner-required"));
    const QVariantMap record = store->record(binding.value(QStringLiteral("sourceTaskId")).toString(), binding.value(QStringLiteral("recordRevision")));
    if (record.isEmpty() || !falsity(record.value(QStringLiteral("restoredFromArchive")))) return reject(QStringLiteral("original-runtime-record-required"));
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (record.value(key) != binding.value(key)) return reject(QStringLiteral("original-record-binding-mismatch"));
    // liveReview can legitimately have been revoked before this NEW action.
    // Compare immutable provenance, then freeze the post-revocation record.
    for (const QString& key : {QStringLiteral("plan"), QStringLiteral("reviewEvidence"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (record.value(key) != owner->_recordSnapshot.value(key)) return reject(QStringLiteral("original-record-content-mismatch"));
    if (owner->_targets.isEmpty() || owner->_targets.size() > 32) return reject(QStringLiteral("reconciliation-target-budget"));
    QList<Target> targets;
    QVariantList ids;
    QSet<int> seen;
    int expectedTotal = 0;
    for (const FleetMissionDeployment::Target& original : owner->_targets) {
        const int count = original.payload.value(QStringLiteral("items")).toList().size();
        if (original.vehicleId < 1 || original.vehicleId > 255 || seen.contains(original.vehicleId)
            || count < 1 || count > MaximumItems || original.payload.value(QStringLiteral("canonicalWireItems")).toList().size() != count
            || !FleetMissionHeadingEncoding::isSupportedWireSchema(original.payload.value(QStringLiteral("wireSchema")))
            || !digest(original.payload.value(QStringLiteral("contentHash")).toString())) return reject(QStringLiteral("bounded-original-payload-required"));
        expectedTotal += count;
        if (expectedTotal > MaximumTotalItems) return reject(QStringLiteral("reconciliation-total-item-budget"));
        Target target;
        target.vehicleId = original.vehicleId; target.vehicle = original.vehicle; target.manager = original.manager; target.payload = original.payload;
        if (target.vehicle) {
            target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager();
            target.componentId = target.vehicle->defaultComponentId();
            target.primaryOwner = target.links ? target.links->primaryLink() : WeakLinkInterfacePtr();
            { const SharedLinkInterfacePtr primary = target.primaryOwner.lock(); target.primary = primary.get(); }
            if (!sources()) return reject(QStringLiteral("target-source-changed-at-capture"));
            if (original.transportPrimaryCaptured && (!original.transportPrimary
                || target.primary != original.transportPrimary || original.transportPrimaryOwner.expired()
                || target.primaryOwner.owner_before(original.transportPrimaryOwner)
                || original.transportPrimaryOwner.owner_before(target.primaryOwner))) {
                target.primary = original.transportPrimary; target.primaryOwner = original.transportPrimaryOwner;
                target.state = QStringLiteral("unknown"); target.reason = QStringLiteral("original-deployment-primary-changed");
            }
            if (target.vehicle->armed() || target.vehicle->flying()) return reject(QStringLiteral("whole-group-grounded-disarmed-required"));
            const QVariantMap version = target.vehicle->autopilotVersionObservation(LifetimeMs);
            if (!sources() || !target.vehicle) return reject(QStringLiteral("target-source-changed-at-capture"));
            target.versionEpoch = version.value(QStringLiteral("sourceEpoch")).toString();
            if (target.parameters) {
                const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MPC_XY_CRUISE"), LifetimeMs);
                if (!sources()) return reject(QStringLiteral("parameter-source-changed-at-capture"));
                target.parameterEpoch = parameter.value(QStringLiteral("managerEpoch")).toString();
            }
        }
        seen.insert(target.vehicleId); ids.append(target.vehicleId); targets.append(target);
    }
    if (!sources() || _generation == std::numeric_limits<quint64>::max()) return reject(QStringLiteral("reconciliation-generation-exhausted-or-changed"));
    _disconnectSources(); ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _swarm = swarm; _authorization = authorization;
    _identity = identity; _role = role; _binding = binding; _record = record; _targets = targets; _vehicleIds = ids;
    _requestId = QUuid::createUuid().toString(QUuid::WithoutBraces); _deploymentId = deploymentId; _deploymentGeneration = deploymentGeneration;
    _deploymentState = deploymentState;
    _targetIndex = 0; _expectedTotal = expectedTotal; _observedTotal = 0; _startedAt = _clock.elapsed();
    _active = true; _state = QStringLiteral("reading"); _reason.clear(); _reservationHeld = false; _transportDraining = false;
    _connectSources(generation);
    if (!_gate(generation) || !self) {
        if (self) _finish(QStringLiteral("stopped"), QStringLiteral("reconciliation-context-denied"));
        return reject(QStringLiteral("reconciliation-context-denied"));
    }
    const QString requestId = _requestId;
    const bool reserved = swarm->reserveMissionDeployment(requestId, ids);
    if (!self) { if (swarm && reserved) swarm->releaseMissionDeployment(requestId); return reject(QStringLiteral("reconciliation-owner-destroyed")); }
    if (!_same(generation) || !sources() || !reserved || !_active) {
        if (swarm && reserved) swarm->releaseMissionDeployment(requestId);
        _finish(QStringLiteral("stopped"), QStringLiteral("whole-group-read-reservation-denied"));
        return reject(QStringLiteral("whole-group-read-reservation-denied"));
    }
    _reservationHeld = true;
    if (!_gate(generation) || !self) {
        if (self) _finish(QStringLiteral("stopped"), QStringLiteral("reserved-reconciliation-context-denied"));
        return reject(QStringLiteral("reserved-reconciliation-context-denied"));
    }
    _timer.start();
    if (!_publish(generation) || !_active) return reject(QStringLiteral("reconciliation-revoked-at-publication"));
    QVariantMap result = current(); result.insert(QStringLiteral("ok"), true); result.insert(QStringLiteral("requestAccepted"), true);
    return result;
}

void FleetMissionReconciliation::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionReconciliation::_connectSources(quint64 generation)
{
    const auto stop = [this, generation]() { if (_same(generation) && _active) _finish(QStringLiteral("stopped"), QStringLiteral("reconciliation-global-source-changed")); };
    for (QObject* source : {static_cast<QObject*>(_owner.data()), static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()),
        static_cast<QObject*>(_store.data()), static_cast<QObject*>(_swarm.data()), _authorization.data()})
        _connections.append(connect(source, &QObject::destroyed, this, stop));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stop));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stop));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stop));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, stop));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stop));
    _connections.append(connect(_owner.data(), &FleetMissionDeployment::changed, this, [this, generation, stop]() {
        if (_same(generation) && _active && !_globalNativeCurrent()) stop();
    }));
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index);
        if (target.vehicle) {
            _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, [stop](bool armed) { if (armed) stop(); }));
            _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, [stop](bool flying) { if (flying) stop(); }));
        }
        if (!target.manager) continue;
        _connections.append(connect(target.manager.data(), &MissionManager::operationStarted, this,
            [this, generation, index](quint64 operation, int transaction) {
                if (!_same(generation) || !_active || index >= _targets.size()) return;
                Target& current = _targets[index];
                const bool own = index == _targetIndex && _dispatching && current.state == QStringLiteral("reading")
                    && transaction == PlanManager::TransactionRead && current.operationId == 0 && !current.scopeId.isEmpty();
                if (own) current.operationId = operation;
                else _finish(QStringLiteral("stopped"), QStringLiteral("foreign-mission-operation-during-reconciliation"));
            }));
        _connections.append(connect(target.manager.data(), &PlanManager::fleetMissionDiagnosticReadComplete, this,
            [this, generation, index](quint64 operation, const QString& scope, bool success) {
                if (!_same(generation) || !_active || index != _targetIndex || index >= _targets.size()) return;
                Target& current = _targets[index];
                if (current.state != QStringLiteral("reading") || current.scopeId != scope || current.operationId != operation) return;
                current.completionSeen = true; current.completionSuccess = success;
                // The timer also handles nested event loops: a queued tick on
                // the finish stack must wait, never take an unfinished slot.
                QTimer::singleShot(0, this, [this, generation]() { if (_same(generation)) _tick(); });
            }));
    }
}

bool FleetMissionReconciliation::_captureResult(const QVariantMap& result, Target* target)
{
    if (!target || result.size() != 35 || result.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || result.value(QStringLiteral("schemaVersion")).toInt() != 1
        || result.value(QStringLiteral("kind")) != QStringLiteral("diagnostic-raw-int-read")) return false;
    for (const QString& key : {QStringLiteral("observed"), QStringLiteral("requestIssued"), QStringLiteral("countObserved"),
        QStringLiteral("allItemsInt"), QStringLiteral("readEndAckQueued"), QStringLiteral("diagnosticOnly"),
        QStringLiteral("historicalOnly"), QStringLiteral("snapshotOnly")}) if (!truth(result.value(key))) return false;
    for (const QString& key : {QStringLiteral("requestCausalityVerified"), QStringLiteral("atomicSnapshotVerified"),
        QStringLiteral("currentOnboardContentsProven"), QStringLiteral("authenticated"), QStringLiteral("signatureVerified"),
        QStringLiteral("startupPermitGranted"), QStringLiteral("executionVerified"), QStringLiteral("flightCommandReleased")})
        if (!falsity(result.value(key))) return false;
    quint64 operation = 0, sequence = 0;
    if (result.value(QStringLiteral("sourceEpoch")) != target->managerEpoch || !uuid(target->managerEpoch)
        || result.value(QStringLiteral("vehicleSourceEpoch")) != target->versionEpoch
        || result.value(QStringLiteral("readScopeId")) != target->scopeId
        || !decimal(result.value(QStringLiteral("operationId")), &operation) || operation == 0 || operation != target->operationId
        || !decimal(result.value(QStringLiteral("receiptSequence")), &sequence) || sequence == 0
        || result.value(QStringLiteral("vehicleId")).type() != QVariant::Int || result.value(QStringLiteral("vehicleId")).toInt() != target->vehicleId
        || result.value(QStringLiteral("componentId")).type() != QVariant::Int || result.value(QStringLiteral("componentId")).toInt() != target->componentId
        || result.value(QStringLiteral("itemCount")).type() != QVariant::Int
        || result.value(QStringLiteral("rawItems")).type() != QVariant::List
        || !digest(result.value(QStringLiteral("countPayloadHash")).toString())
        || !digest(result.value(QStringLiteral("rawItemsFingerprint")).toString())) return false;
    const int count = result.value(QStringLiteral("itemCount")).toInt();
    const QVariantList raw = result.value(QStringLiteral("rawItems")).toList();
    if (count < 0 || count > target->maximumItemCount || count > MaximumItems || raw.size() != count
        || count > MaximumTotalItems - _observedTotal) return false;
    double started = 0, counted = 0, first = 0, last = 0, completed = 0, duration = 0;
    const auto time = [&](const QString& key, double low, double* value) {
        return result.value(key).type() == QVariant::Double && number(result.value(key), low, 9007199254740991.0, value);
    };
    if (!time(QStringLiteral("readStartedAtMonotonicMs"), 0, &started)
        || !time(QStringLiteral("countReceivedAtMonotonicMs"), started, &counted)
        || !time(QStringLiteral("completedAtMonotonicMs"), counted, &completed)
        || !time(QStringLiteral("durationMs"), 0, &duration) || duration != completed - started || duration > LifetimeMs
        || !time(QStringLiteral("firstItemReceivedAtMonotonicMs"), count == 0 ? -1 : counted, &first)
        || !time(QStringLiteral("lastItemReceivedAtMonotonicMs"), count == 0 ? -1 : first, &last)
        || (count == 0 ? first != -1 || last != -1 : last > completed)) return false;
    target->observedCount = count; target->observedAt = _clock.elapsed();
    target->rawHash = result.value(QStringLiteral("rawItemsFingerprint")).toString();
    _observedTotal += count;
    const int expected = target->payload.value(QStringLiteral("items")).toList().size();
    if (count == 0) {
        target->state = QStringLiteral("different"); target->reason = QStringLiteral("complete-raw-count-zero");
        target->comparisonBasis = QStringLiteral("count-only"); return true;
    }
    const QVariantMap canonical = canonicalRaw(raw);
    if (!truth(canonical.value(QStringLiteral("ok")))) {
        target->state = count != expected ? QStringLiteral("different") : QStringLiteral("unknown");
        target->reason = count != expected ? QStringLiteral("count-different-content-not-canonical") : QStringLiteral("complete-raw-content-not-canonical");
        target->comparisonBasis = count != expected ? QStringLiteral("count-only") : QStringLiteral("none"); return true;
    }
    target->observedHash = canonical.value(QStringLiteral("contentHash")).toString();
    target->comparisonBasis = FleetMissionHeadingEncoding::comparisonBasis(
        target->payload.value(QStringLiteral("wireSchema")) == FleetMissionHeadingEncoding::wireSchema(true)
        || canonical.value(QStringLiteral("wireSchema")) == FleetMissionHeadingEncoding::wireSchema(true));
    const bool matched = count == expected && canonical.value(QStringLiteral("items")) == target->payload.value(QStringLiteral("canonicalWireItems"))
        && canonical.value(QStringLiteral("contentHash")) == target->payload.value(QStringLiteral("contentHash"));
    target->state = matched ? QStringLiteral("matched") : QStringLiteral("different");
    target->reason = matched ? QStringLiteral("complete-raw-canonical-match") : QStringLiteral("complete-raw-canonical-different");
    return true;
}

void FleetMissionReconciliation::_advanceUnknown(const QString& reason)
{
    if (_targetIndex < 0 || _targetIndex >= _targets.size()) return;
    const QPointer<FleetMissionReconciliation> self(this);
    const quint64 generation = _generation;
    const Target target = _targets.at(_targetIndex);
    _targets[_targetIndex].state = QStringLiteral("unknown"); _targets[_targetIndex].reason = reason;
    // Revoke native admission BEFORE cancellation; a reentrant timeout/retry
    // cannot reuse this target while it is waiting for local stack drain.
    if (target.manager && !target.scopeId.isEmpty()) target.manager->cancelFleetMissionDiagnosticRead(this, target.scopeId);
    if (self && _same(generation)) _publish(generation);
}
void FleetMissionReconciliation::_tick()
{
    if (_busy || _checking || _dispatching) return;
    _busy = true;
    FlagGuard working{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionReconciliation> self(this);
    const quint64 generation = _generation;
    if (!_active) { _drain(); return; }
    if (!_gate(generation) || !self) {
        if (self) _finish(QStringLiteral("stopped"), QStringLiteral("reconciliation-global-context-expired-or-changed"));
        return;
    }
    if (_targetIndex >= _targets.size()) { _finish(QStringLiteral("completed"), QStringLiteral("whole-group-attempt-finished")); return; }
    const int index = _targetIndex;
    const Target target = _targets.at(index);
    if (target.state == QStringLiteral("unknown")) {
        if ((target.manager && (target.manager->inProgress() || target.manager->transportCallInProgress()))
            || (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())) return;
        ++_targetIndex; _publish(generation); return;
    }
    if (!_targetReadable(target, target.state == QStringLiteral("reading")) || !self) {
        if (self) _advanceUnknown(QStringLiteral("original-target-unavailable-or-source-changed"));
        return;
    }
    if (target.state == QStringLiteral("queued")) {
        if (_observedTotal >= MaximumTotalItems) { _advanceUnknown(QStringLiteral("observed-group-item-budget-exhausted")); return; }
        const QString scope = QUuid::createUuid().toString(QUuid::WithoutBraces);
        _targets[index].scopeId = scope; _targets[index].state = QStringLiteral("reading");
        _targets[index].maximumItemCount = qMin(MaximumItems, MaximumTotalItems - _observedTotal);
        _targets[index].deadlineMs = qMin(_startedAt + LifetimeMs,
            _clock.elapsed() + 4000 + 250LL * target.payload.value(QStringLiteral("items")).toList().size());
        const int maximum = _targets[index].maximumItemCount;
        const auto fence = [self, generation, index, scope]() {
            const auto same = [&]() { return self && self->_same(generation) && self->_active
                && self->_targetIndex == index && index >= 0 && index < self->_targets.size()
                && self->_targets.at(index).scopeId == scope && self->_targets.at(index).state == QStringLiteral("reading"); };
            return same() && self->_gate(generation) && same() && self->_targetReadable(self->_targets.at(index), true);
        };
        _dispatching = true;
        const QVariantMap issued = target.manager->loadFleetMissionForDiagnostic(this, target.primary.data(), scope, maximum, fence);
        if (!self) return;
        _dispatching = false;
        if (!_same(generation) || !_active) return;
        if (_targetIndex != index || _targets.at(index).scopeId != scope || !_gate(generation) || !self) {
            if (self) _finish(QStringLiteral("stopped"), QStringLiteral("read-dispatch-context-changed")); return;
        }
        quint64 operation = 0, baseline = 0;
        if (issued.size() != 9 || !truth(issued.value(QStringLiteral("ok"))) || !truth(issued.value(QStringLiteral("requestIssued")))
            || !decimal(issued.value(QStringLiteral("operationId")), &operation) || operation == 0
            || !decimal(issued.value(QStringLiteral("baselineReceiptSequence")), &baseline)
            || issued.value(QStringLiteral("readScopeId")) != scope || issued.value(QStringLiteral("vehicleSourceEpoch")) != target.versionEpoch
            || !uuid(issued.value(QStringLiteral("sourceEpoch")).toString())
            || issued.value(QStringLiteral("vehicleId")).toInt() != target.vehicleId || issued.value(QStringLiteral("componentId")).toInt() != target.componentId
            || (_targets.at(index).operationId != 0 && _targets.at(index).operationId != operation)) {
            _advanceUnknown(QStringLiteral("diagnostic-read-not-issued-or-unconfirmed")); return;
        }
        _targets[index].operationId = operation; _targets[index].managerEpoch = issued.value(QStringLiteral("sourceEpoch")).toString();
        _publish(generation); return;
    }
    if (target.deadlineMs < 0 || _clock.elapsed() >= target.deadlineMs) { _advanceUnknown(QStringLiteral("diagnostic-read-deadline-expired")); return; }
    if (target.manager->inProgress() || target.manager->transportCallInProgress()) return;
    if (!target.completionSeen || !target.completionSuccess) { _advanceUnknown(QStringLiteral("complete-diagnostic-read-not-observed")); return; }
    const QVariantMap result = target.manager->takeFleetMissionDiagnosticResult(this, target.primary.data(), target.scopeId, target.operationId);
    if (!self || !_same(generation) || !_active) return;
    if (!_gate(generation) || !self || !_targetReadable(target, false)) {
        if (self) _finish(QStringLiteral("stopped"), QStringLiteral("diagnostic-capture-context-changed")); return;
    }
    if (!_captureResult(result, &_targets[index])) { _advanceUnknown(QStringLiteral("complete-diagnostic-result-unavailable-or-invalid")); return; }
    ++_targetIndex;
    _publish(generation);
}

void FleetMissionReconciliation::_finish(const QString& state, const QString& reason)
{
    if (!_active) return;
    const QPointer<FleetMissionReconciliation> self(this);
    const quint64 generation = _generation;
    const QList<Target> targets = _targets;
    _active = false; _state = state; _reason = reason;
    for (Target& target : _targets) if (target.state == QStringLiteral("queued") || target.state == QStringLiteral("reading")) {
        target.state = QStringLiteral("unknown"); target.reason = reason;
    }
    _disconnectSources();
    for (const Target& target : targets) {
        if (target.manager && !target.scopeId.isEmpty()) target.manager->cancelFleetMissionDiagnosticRead(this, target.scopeId);
        if (!self || !_same(generation)) return;
    }
    _transportDraining = _reservationHeld;
    _drain();
    if (self && _same(generation)) _publish(generation);
}
bool FleetMissionReconciliation::cancel(const QString& requestId)
{
    if (QThread::currentThread() != thread() || !_owned() || requestId.isEmpty() || requestId != _requestId || !_active) return false;
    _finish(QStringLiteral("cancelled"), QStringLiteral("operator-cancelled-diagnostic-read")); return true;
}
void FleetMissionReconciliation::_drain()
{
    if (_active) return;
    if (!_reservationHeld) { _transportDraining = false; _timer.stop(); return; }
    if (_dispatching) { _transportDraining = true; _timer.start(); return; }
    for (const Target& target : _targets) if ((target.manager && (target.manager->inProgress() || target.manager->transportCallInProgress()))
        || (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())) {
        _transportDraining = true; _timer.start(); return;
    }
    const QPointer<FleetMissionReconciliation> self(this);
    const QPointer<SwarmController> swarm(_swarm);
    const QString requestId = _requestId;
    _reservationHeld = false; _transportDraining = false; _timer.stop();
    if (swarm) swarm->releaseMissionDeployment(requestId);
    if (self) emit changed();
}
void FleetMissionReconciliation::_detachDrain()
{
    const QPointer<SwarmController> swarm(_swarm);
    const QList<Target> targets = _targets;
    const QString requestId = _requestId;
    const bool held = _reservationHeld;
    struct DrainSource { QPointer<Vehicle> vehicle; QPointer<MissionManager> manager; };
    QList<DrainSource> drainSources;
    for (const Target& target : targets) drainSources.append({target.vehicle, target.manager});
    for (const Target& target : targets)
        if (target.manager && !target.scopeId.isEmpty()) target.manager->cancelFleetMissionDiagnosticRead(this, target.scopeId);
    if (!held || !swarm) return;
    QTimer* cleanup = new QTimer(swarm.data());
    cleanup->setInterval(100);
    connect(cleanup, &QTimer::timeout, cleanup, [cleanup, swarm, drainSources, requestId]() {
        if (!swarm) { cleanup->deleteLater(); return; }
        for (const DrainSource& source : drainSources) if ((source.manager && (source.manager->inProgress() || source.manager->transportCallInProgress()))
            || (source.vehicle && !source.vehicle->fleetStartupTransportQuiescent())) return;
        const QPointer<QTimer> guard(cleanup);
        cleanup->stop(); swarm->releaseMissionDeployment(requestId);
        if (guard) cleanup->deleteLater();
    });
    cleanup->start();
}

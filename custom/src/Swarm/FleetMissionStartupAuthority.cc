#include "FleetMissionStartupAuthority.h"
#include "FaultToleranceManager.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionStartApproval.h"
#include "FleetMissionStartContentCheck.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionStartupClearanceWitness.h"
#include "FleetMissionStartupClearanceWorker.h"
#include "FleetMissionStartReview.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <initializer_list>
#include <limits>

namespace {
constexpr qint64 MaximumLifetimeMs = 30000;
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr double ExactMaximum = 9007199254740991.0;
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool outboundSourcesCurrent(const QObject* authority, std::initializer_list<const QObject*> sources)
{
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(authority)) return false;
    for (const QObject* source : sources)
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(authority, source)) return false;
    return true;
}
bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool integer(const QVariant& value, double minimum, double maximum, double* number)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum || std::floor(candidate) != candidate) return false;
    *number = candidate; return true;
}
bool text(const QVariant& value, int maximum = 128)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed() || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}
bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar character : value.toString()) if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
        || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}
bool clearanceBindingMatches(bool required, const FleetMissionStartupClearanceWitness::Ptr& witness,
    const QVariantMap& artifact)
{
    if (!required) return !witness;
    const QVariant value = artifact.value(QStringLiteral("startupClearanceReview"));
    if (!witness || value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap result = value.toMap();
    return result.size() == 8 && isTrue(result.value(QStringLiteral("ok")))
        && result.value(QStringLiteral("startupDomainReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("startupDomainReview")).isNull()
        && !result.value(QStringLiteral("startupDomainReview")).toMap().isEmpty()
        && result.value(QStringLiteral("waypointSwitchReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("waypointSwitchReview")).isNull()
        && !result.value(QStringLiteral("waypointSwitchReview")).toMap().isEmpty()
        && result.value(QStringLiteral("interVehicleEnvelopeReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("interVehicleEnvelopeReview")).isNull()
        && !result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap().isEmpty()
        // This conclusion check does not replace the original native Witness
        // or re-run geometry. Incomplete/overlapping envelopes cannot bind.
        && FleetMissionStartupClearanceWorker::startupModelSatisfied(result)
        && digest(result.value(QStringLiteral("inputFingerprint")))
        && witness->inputFingerprint() == result.value(QStringLiteral("inputFingerprint")).toString();
}
bool tree(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 20 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map:
        {
            const QVariantMap map = value.toMap();
            for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
                *amount += static_cast<qint64>(it.key().size()) * 2;
                if (*amount > MaximumBytes || !tree(it.value(), depth + 1, nodes, amount)) return false;
            }
        }
        return true;
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!tree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList()) if (!tree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::String:
        *amount += static_cast<qint64>(value.toString().size()) * 2;
        return *amount <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool encode(const QVariantMap& map, QByteArray* result)
{
    int nodes = 0; qint64 amount = 0;
    if (map.isEmpty() || !tree(map, 0, &nodes, &amount)) return false;
    *result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !result->isEmpty() && result->size() <= MaximumBytes;
}
QString hash(const QVariantMap& map)
{
    QByteArray encoded;
    return encode(map, &encoded) ? QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex()) : QString();
}
QVariantMap stable(QVariantMap map) { map.remove(QStringLiteral("remainingLifetimeMs")); return map; }
QVariantMap stablePhaseReadiness(QVariantMap map)
{
    // The phase fingerprint still binds the native stable coordinate risk.
    // Preserve the original full RAW supplement as provenance, but do not
    // confuse age/same-value receipt advancement with a physical transition.
    map.remove(QStringLiteral("startupCoordinateObservation"));
    // This is a captured display-only ALTITUDE sample, not a periodic
    // contribution to the reviewed physical risk/permission identity.
    map.remove(QStringLiteral("startupAltitudeDiagnostic"));
    map.remove(QStringLiteral("startupAltitudeReferenceSample"));
    return map;
}
bool altitudeMatches(const QVariantMap& current, const QVariantMap& original, const QVariantMap& policy)
{
    if (policy.value(QStringLiteral("schemaVersion")).toInt() != 4) return true;
    const QVariantMap reference = original.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
    return isTrue(current.value(QStringLiteral("startupAltitudeReferenceRequired"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceRequired")))
        && isTrue(current.value(QStringLiteral("startupAltitudeReferenceCaptured"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceCaptured")))
        && isTrue(current.value(QStringLiteral("startupAltitudeReferenceMatches"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceMatches")))
        && reference.size() == 20 && reference == current.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
        && digest(original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
        && hash(reference) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
        && current.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint"))
        && FleetMissionStartupAltitude::sampleWithinBaseline(current.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
            original.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result)
{
    const QString task = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(task), record = store->record(task, binding.value(QStringLiteral("recordRevision")));
    if (!isFalse(latest.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (!binding.contains(key) || binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return encode(record, result);
}
bool validRole(const QVariantMap& identity, const QVariantMap& role)
{
    const QString primary = identity.value(QStringLiteral("role")).toString();
    return (primary == QStringLiteral("operator") || primary == QStringLiteral("commander") || primary == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && isTrue(role.value(QStringLiteral("allowed"))) && isTrue(role.value(QStringLiteral("requiresSecondApproval")))
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")))
        && isTrue(identity.value(QStringLiteral("authenticated")));
}
QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("authorityAccepted"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QObject* actualObject(const QVariant& value)
{
    return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr;
}
QVariantMap select(const QVariantMap& input, const QStringList& keys)
{
    QVariantMap result;
    for (const QString& key : keys) result.insert(key, input.value(key));
    return result;
}
} // namespace

FleetMissionStartupAuthority::FleetMissionStartupAuthority(QObject* parent)
    : QObject(parent), _authorityEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (_busy || _checking || _state != QStringLiteral("accepted")) return;
        const QPointer<FleetMissionStartupAuthority> self(this);
        const quint64 generation = _generation;
        if (!_live(generation) || !_gate(generation, true) || !self) {
            if (self) _invalidate(generation, QStringLiteral("startup-authority-expired-or-source-changed"));
        }
    });
}
FleetMissionStartupAuthority::~FleetMissionStartupAuthority()
{
    _state = QStringLiteral("destroyed");
    _disconnectSources();
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetMissionStartApproval> approval(_approval);
    const QString oldApprovalId = _conditionalGrant.value(QStringLiteral("approvalReceipt")).toMap().value(QStringLiteral("approvalId")).toString();
    const QString startupId = _startupId, epoch = _authorityEpoch;
    const bool held = _reservationHeld;
    if (session) session->revokeStartupAuthentication(this, startupId, epoch);
    if (!_reservationTransferred && approval && uuid(oldApprovalId)) approval->revokeCurrent(oldApprovalId);
    if (!_reservationTransferred && held && swarm) swarm->releaseMissionDeployment(startupId);
}
bool FleetMissionStartupAuthority::_live(quint64 generation) const
{
    return generation > 0 && generation == _generation && _clock.elapsed() < _expiresAtMs
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("accepted"));
}
QVariantMap FleetMissionStartupAuthority::_bundle() const
{
    const auto object = [](QObject* source) { return QVariant::fromValue(source); };
    return {{QStringLiteral("session"), object(_session.data())}, {QStringLiteral("roles"), object(_roles.data())},
        {QStringLiteral("store"), object(_store.data())}, {QStringLiteral("review"), object(_review.data())},
        {QStringLiteral("approval"), object(_approval.data())}, {QStringLiteral("contentCheck"), object(_content.data())},
        {QStringLiteral("deployment"), object(_deployment.data())}, {QStringLiteral("swarm"), object(_swarm.data())},
        {QStringLiteral("health"), object(_health.data())}, {QStringLiteral("authorizationOwner"), object(_authorization.data())}};
}
QVariantMap FleetMissionStartupAuthority::current() const
{
    const bool accepted = _state == QStringLiteral("accepted") && _live(_generation) && _reservationHeld;
    QVariantList targets;
    for (const Target& target : _targets) targets.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("startupPhase"), QStringLiteral("before-normal-arm")}, {QStringLiteral("readinessFingerprint"), target.readinessFingerprint},
        {QStringLiteral("readScopeId"), target.readReceipt.value(QStringLiteral("readScopeId"))},
        {QStringLiteral("readOperationId"), target.readReceipt.value(QStringLiteral("operationId"))}});
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("authorityAccepted"), accepted}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("deploymentId"), _reviewContext.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("authorityEpoch"), _authorityEpoch}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason}, {QStringLiteral("sourceHash"), _sourceHash},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), targets},
        {QStringLiteral("reservationHeld"), _reservationHeld}, {QStringLiteral("remainingLifetimeMs"), static_cast<double>(qMax<qint64>(0, _expiresAtMs - _clock.elapsed()))},
        {QStringLiteral("reservationTransferred"), _reservationTransferred}, {QStringLiteral("transactionEpoch"), _transferredTransactionEpoch},
        {QStringLiteral("allowedFutureActionScope"), QStringLiteral("seq0-normal-arm-param2-zero-mode-only-auto-mission")},
        {QStringLiteral("preARMOnly"), true}, {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("remoteAtomicSnapshotVerified"), false},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true}};
}
void FleetMissionStartupAuthority::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionStartupAuthority::_invalidate(quint64 generation, const QString& reason, const QString& state)
{
    if (generation == 0 || generation != _generation || (_state != QStringLiteral("preparing") && _state != QStringLiteral("accepted"))) return;
    const QPointer<FleetMissionStartupAuthority> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetMissionStartApproval> approval(_approval);
    const QString oldApprovalId = _conditionalGrant.value(QStringLiteral("approvalReceipt")).toMap().value(QStringLiteral("approvalId")).toString();
    const QString startupId = _startupId, epoch = _authorityEpoch;
    const bool held = _reservationHeld;
    _state = state; _reason = reason.left(256); _reservationHeld = false;
    _timer.stop(); _disconnectSources();
    if (session) session->revokeStartupAuthentication(this, startupId, epoch);
    if (!self) { if (held && swarm) swarm->releaseMissionDeployment(startupId); return; }
    if (approval && uuid(oldApprovalId)) approval->revokeCurrent(oldApprovalId);
    if (!self) { if (held && swarm) swarm->releaseMissionDeployment(startupId); return; }
    if (held && swarm) swarm->releaseMissionDeployment(startupId);
    if (self && _generation == generation && _startupId == startupId && _authorityEpoch == epoch && _state == state) emit changed();
}
void FleetMissionStartupAuthority::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _invalidate(generation, QStringLiteral("startup-authority-native-source-changed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_review.data()), static_cast<QObject*>(_approval.data()), static_cast<QObject*>(_content.data()),
        static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()), static_cast<QObject*>(_health.data()), _authorization.data(),
        static_cast<QObject*>(_coordinator.data()), static_cast<QObject*>(_catalog.data()), static_cast<QObject*>(_registry.data()), static_cast<QObject*>(_faultPolicy.data())})
        if (object) _connections.append(connect(object, &QObject::destroyed, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    _connections.append(connect(_coordinator.data(), &FleetMissionReviewCoordinator::firmwareAdapterCatalogChanged, this, stale));
    _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, stale));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, stale));
    _connections.append(connect(_swarm.data(), &SwarmController::formationActiveChanged, this, stale));
    _connections.append(connect(_swarm.data(), &SwarmController::formationBusyChanged, this, stale));
    // Old Review/Content/Approval state changes are intentionally NOT an
    // authorization source after native handoff. Actual frozen facts remain.
    for (const Target& target : _targets) {
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(object, &QObject::destroyed, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, [stale](bool pending) { if (pending) stale(); }));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this, stale));
    }
}
bool FleetMissionStartupAuthority::_targetsCurrent(quint64 generation) const
{
    const QPointer<FleetMissionStartupAuthority> self(const_cast<FleetMissionStartupAuthority*>(this));
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QVariantMap constraints = _constraints;
    const QString deploymentId = _deploymentContext.value(QStringLiteral("deploymentId")).toString();
    const QList<Target> targets = _targets;
    const auto live = [&]() { return self && _live(generation) && deployment && catalog && _deployment == deployment
        && _catalog == catalog && _constraints == constraints; };
    if (!live() || targets.isEmpty() || targets.size() > 32) return false;
    for (const Target& target : targets) {
        const auto alive = [&]() { return live() && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
        if (!alive()) return false;
        const QPointer<Vehicle> vehicle(deployment->startReviewVehicle(deploymentId, target.vehicleId));
        if (!alive()) return false;
        const QPointer<MissionManager> mission(deployment->startReviewManager(deploymentId, target.vehicleId));
        if (!alive()) return false;
        const SharedLinkInterfacePtr primary = target.links->primaryLink().lock();
        if (!alive() || vehicle != target.vehicle || mission != target.mission || !primary || primary.get() != target.primary.data()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.componentId < 1 || target.componentId > 255 || target.vehicle->parameterManager() != target.parameters.data()
            || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->missionManager() != target.mission.data()
            || target.vehicle->vehicleLinkManager() != target.links.data() || target.vehicle->thread() != thread()
            || target.mission->thread() != thread() || target.parameters->thread() != thread() || target.links->thread() != thread()
            || target.vehicle->armed() || target.vehicle->flying() || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor()
            || !target.vehicle->isInitialConnectComplete() || target.vehicle->isOfflineEditingVehicle()
            || target.links->communicationLost() || target.mission->inProgress() || target.mission->transportCallInProgress()
            || !target.parameters->parametersReady() || target.parameters->missingParameters() || target.parameters->pendingWrites()) return false;
        if (!target.primary->isConnected() || !alive() || target.primary->isMockLink() || !alive() || target.primary->isLogReplay() || !alive()) return false;
        if (!target.vehicle->startupCoordinateObservationSourceMatches(target.primary.data()) || !alive()) return false;
        const bool readBefore = target.mission->fleetMissionReadReceiptRetainedSourceMatches(target.primary.data(), target.readReceipt);
        if (!alive() || !readBefore) return false;
        const QVariantMap captured = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member,
            constraints, target.payload, FleetMissionStartReadiness::Phase::BeforeArm, catalog.data());
        if (!alive() || !isTrue(captured.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !isTrue(captured.value(QStringLiteral("controlledArmPreconditionsMatch")))
            || !isTrue(captured.value(QStringLiteral("sourceEvidenceComplete"))) || !isTrue(captured.value(QStringLiteral("sourcesStillCurrent")))
            || captured.value(QStringLiteral("launchType")).toString() != QStringLiteral("takeoff")
            || captured.value(QStringLiteral("startupPhaseFingerprint")) != target.readinessFingerprint
            || stablePhaseReadiness(captured) != stablePhaseReadiness(target.readiness)
            || !altitudeMatches(captured, target.readiness, target.readiness.value(QStringLiteral("startupTransitionPolicy")).toMap())) return false;
        if (!target.vehicle->startupCoordinateObservationSourceMatches(target.primary.data()) || !alive()) return false;
        const bool readAfter = target.mission->fleetMissionReadReceiptRetainedSourceMatches(target.primary.data(), target.readReceipt);
        if (!alive() || !readAfter) return false;
    }
    return live();
}
bool FleetMissionStartupAuthority::_gate(quint64 generation, bool requireAuthentication)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    if (!outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})) return false;
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionStartupAuthority> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionStartReview> review(_review);
    const QPointer<FleetMissionStartApproval> approval(_approval);
    const QPointer<FleetMissionStartContentCheck> content(_content);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<QObject> owner(_authorization);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FaultToleranceManager> faults(_faultPolicy);
    const bool hadRegistry = bool(registry), hadFaults = bool(faults), held = _reservationHeld;
    const QVariantMap identity = _identityReceipt, role = _roleReceipt, binding = _binding, authentication = _authenticationReceipt;
    const QVariantMap healthGate = _healthGate, catalogSnapshot = _catalogSnapshot, frozenDeployment = _deploymentContext;
    const QVariantMap reviewContext = _reviewContext;
    const auto clearance = _clearanceWitness;
    const bool clearanceRequired = _clearanceRequired;
    const QByteArray recordBytes = _recordBytes, deploymentBytes = _deploymentBytes;
    const QVariantList ids = _vehicleIds;
    const QString startupId = _startupId, sourceHash = _sourceHash, epoch = _authorityEpoch;
    const QString state = _state;
    const auto unchanged = [&]() {
        return self && !_reentered && _live(generation) && _state == state && _reservationHeld == held
            && session && roles && store && review && approval && content && deployment && swarm && health && owner && coordinator && catalog
            && outboundSourcesCurrent(self.data(), {session.data(), review.data(), approval.data(), content.data(), deployment.data()})
            && _session == session && _roles == roles && _store == store && _review == review && _approval == approval
            && _content == content && _deployment == deployment && _swarm == swarm && _health == health && _authorization == owner
            && _coordinator == coordinator && _catalog == catalog && _registry == registry && _faultPolicy == faults
            && (!hadRegistry || registry) && (!hadFaults || faults) && _identityReceipt == identity && _roleReceipt == role
            && _binding == binding && _authenticationReceipt == authentication && _healthGate == healthGate && _catalogSnapshot == catalogSnapshot
            && _reviewContext == reviewContext && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired
            && _recordBytes == recordBytes && _deploymentBytes == deploymentBytes && _vehicleIds == ids
            && _startupId == startupId && _sourceHash == sourceHash && _authorityEpoch == epoch && _deploymentContext == frozenDeployment;
    };
    const auto nativeCurrent = [&]() {
        if (!unchanged() || !clearanceBindingMatches(clearanceRequired, clearance,
                reviewContext.value(QStringLiteral("reviewArtifact")).toMap())) return false;
        if (clearanceRequired && (!clearance->sourceCurrent(unchanged) || !unchanged())) return false;
        if (!unchanged() || !validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return false;
        // All WTS/group validators may dispatch sent messages: refreeze AFTER.
        const bool identityCurrent = session->identitySnapshotCurrent(identity);
        if (!unchanged() || !identityCurrent) return false;
        if (requireAuthentication) {
            const bool authenticationCurrent = session->startupAuthenticationCurrent(authentication, self.data(), startupId, epoch, sourceHash);
            if (!unchanged() || !authenticationCurrent) return false;
        }
        if (held && !swarm->missionDeploymentOwns(startupId, ids)) return false;
        if (swarm->formationActive() || swarm->formationBusy() || swarm->hasActiveTemporaryMission(ids)) return false;
        if (health->fleetRegistry() != registry.data() || health->faultToleranceManager() != faults.data()
            || health->evaluateTaskGate(ids, _requireLeader) != healthGate || !isTrue(healthGate.value(QStringLiteral("allowed")))
            || (faults && faults->leaderVehicleId() != _leaderId)
            || (registry && _leaderId > 0 && registry->containsVehicle(_leaderId) != _leaderRegistered)) return false;
        if (coordinator->firmwareAdapterCatalogBackend() != catalog.data() || catalog->snapshot() != catalogSnapshot) return false;
        QByteArray record;
        if (!currentRecord(store.data(), binding, &record) || !unchanged() || record != recordBytes) return false;
        const QVariantMap actualDeployment = deployment->startReviewContext(frozenDeployment.value(QStringLiteral("deploymentId")).toString());
        QByteArray actualDeploymentBytes;
        if (!unchanged() || !encode(actualDeployment, &actualDeploymentBytes) || actualDeploymentBytes != deploymentBytes) return false;
        if (!_targetsCurrent(generation) || !unchanged()
            || !clearanceBindingMatches(clearanceRequired, clearance, reviewContext.value(QStringLiteral("reviewArtifact")).toMap())) return false;
        if (clearanceRequired && !clearance->memoryCurrent()) return false;
        return unchanged();
    };
    if (!nativeCurrent()) return false;
    QVariant matched;
    const QVariantMap bundle = _bundle();
    const bool sourcesInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartupAuthoritySourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!unchanged() || !sourcesInvoked || !isTrue(matched) || !nativeCurrent()) return false;
    QVariant allowed;
    const bool allowedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartupAuthorityContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(startupId)), Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(ids)));
    return unchanged() && allowedInvoked && isTrue(allowed) && nativeCurrent() && unchanged()
        && (!clearanceRequired || clearance->memoryCurrent());
}
QVariantMap FleetMissionStartupAuthority::requestAuthority(const QVariantMap& suppliedSources, const QString& suppliedReviewId, bool operatorConfirmed)
{
    const QVariantMap sources = suppliedSources;
    const QString reviewId = suppliedReviewId;
    if (thread() != QThread::currentThread() || _busy || _checking || !operatorConfirmed || !uuid(reviewId)
        || _state == QStringLiteral("transferred")
        || (_state == QStringLiteral("accepted") && _live(_generation))) return rejection(QStringLiteral("confirmed-idle-native-authority-required"));
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return rejection(QStringLiteral("current-native-deployment-episode-required"));
    _busy = true;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (_state == QStringLiteral("accepted") || _state == QStringLiteral("preparing")) {
        _invalidate(_generation, QStringLiteral("replaced-expired-authority"));
        if (!self) return rejection(QStringLiteral("authority-destroyed-at-replacement"));
    }
    const QSet<QString> names = {QStringLiteral("session"), QStringLiteral("roles"), QStringLiteral("store"), QStringLiteral("review"),
        QStringLiteral("approval"), QStringLiteral("contentCheck"), QStringLiteral("deployment"), QStringLiteral("swarm"),
        QStringLiteral("health"), QStringLiteral("authorizationOwner")};
    if (sources.size() != 10) return rejection(QStringLiteral("exact-ten-actual-native-sources-required"));
    for (auto it = sources.constBegin(); it != sources.constEnd(); ++it)
        if (!names.contains(it.key()) || !actualObject(it.value())) return rejection(QStringLiteral("actual-qobject-source-required-not-json"));
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(actualObject(sources.value(QStringLiteral("session"))));
    QPointer<FleetRolePolicy> roles = qobject_cast<FleetRolePolicy*>(actualObject(sources.value(QStringLiteral("roles"))));
    QPointer<FleetMissionPlanStore> store = qobject_cast<FleetMissionPlanStore*>(actualObject(sources.value(QStringLiteral("store"))));
    QPointer<FleetMissionStartReview> review = qobject_cast<FleetMissionStartReview*>(actualObject(sources.value(QStringLiteral("review"))));
    QPointer<FleetMissionStartApproval> approval = qobject_cast<FleetMissionStartApproval*>(actualObject(sources.value(QStringLiteral("approval"))));
    QPointer<FleetMissionStartContentCheck> content = qobject_cast<FleetMissionStartContentCheck*>(actualObject(sources.value(QStringLiteral("contentCheck"))));
    QPointer<FleetMissionDeployment> deployment = qobject_cast<FleetMissionDeployment*>(actualObject(sources.value(QStringLiteral("deployment"))));
    QPointer<SwarmController> swarm = qobject_cast<SwarmController*>(actualObject(sources.value(QStringLiteral("swarm"))));
    QPointer<FleetHealthCenter> health = qobject_cast<FleetHealthCenter*>(actualObject(sources.value(QStringLiteral("health"))));
    QPointer<QObject> owner(actualObject(sources.value(QStringLiteral("authorizationOwner"))));
    const auto alive = [&]() { return self && session && roles && store && review && approval && content && deployment && swarm && health && owner
        && outboundSourcesCurrent(self.data(), {session.data(), review.data(), approval.data(), content.data(), deployment.data()}); };
    if (!alive()) return rejection(QStringLiteral("actual-startup-backend-types-required"));
    for (const QVariant& source : sources) if (actualObject(source)->thread() != thread()) return rejection(QStringLiteral("actual-sources-owner-thread-required"));
    QVariant matched;
    const bool matchedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartupAuthoritySourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(sources)));
    if (!alive() || !matchedInvoked || !isTrue(matched)) return rejection(QStringLiteral("private-startup-authority-source-owner-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!alive()) return rejection(QStringLiteral("identity-source-destroyed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start"));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!alive() || !identityCurrent || !validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return rejection(QStringLiteral("actual-start-role-and-primary-required"));
    const qint64 beforeReview = _clock.elapsed();
    const QVariantMap reviewContext = review->approvalContext(reviewId);
    double reviewRemaining = 0;
    if (!alive() || !isTrue(reviewContext.value(QStringLiteral("ok"))) || !isTrue(reviewContext.value(QStringLiteral("reviewAcknowledged")))
        || reviewContext.value(QStringLiteral("reviewId")) != reviewId || !uuid(reviewContext.value(QStringLiteral("deploymentId")))
        || !integer(reviewContext.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumLifetimeMs, &reviewRemaining)
        || reviewContext.value(QStringLiteral("reviewArtifact")).type() != QVariant::Map) return rejection(QStringLiteral("actual-fresh-acknowledged-review-required"));
    const QVariantMap artifact = reviewContext.value(QStringLiteral("reviewArtifact")).toMap();
    const QVariantMap reviewRole = reviewContext.value(QStringLiteral("roleSnapshot")).toMap();
    if (!isTrue(artifact.value(QStringLiteral("allControlledArmPreconditionsMatch")))
        || !isTrue(artifact.value(QStringLiteral("allFreshOnboardReadsMatched"))) || !isTrue(artifact.value(QStringLiteral("allCurrentNativeCachesMatch")))
        || !isTrue(artifact.value(QStringLiteral("allSourceEvidenceComplete")))
        || reviewRole.value(QStringLiteral("actorId")) != identity.value(QStringLiteral("actorId"))
        || reviewRole.value(QStringLiteral("role")) != identity.value(QStringLiteral("role")) || !roles->actionSnapshotCurrent(reviewRole))
        return rejection(QStringLiteral("typed-controlled-arm-facts-and-same-risk-actor-required"));
    const QString deploymentId = reviewContext.value(QStringLiteral("deploymentId")).toString();
    const QString checkId = artifact.value(QStringLiteral("freshContentCheckId")).toString();
    const qint64 beforeContent = _clock.elapsed();
    const QVariantMap contentContext = content->reviewContext(checkId, deploymentId);
    double contentRemaining = 0;
    if (!alive() || !isTrue(contentContext.value(QStringLiteral("ok"))) || contentContext.value(QStringLiteral("deploymentId")) != deploymentId
        || !isTrue(contentContext.value(QStringLiteral("allFreshOnboardReadsMatched")))
        || !integer(contentContext.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumLifetimeMs, &contentRemaining)
        || contentContext.value(QStringLiteral("binding")) != reviewContext.value(QStringLiteral("binding"))
        || contentContext.value(QStringLiteral("targetVehicleIds")) != reviewContext.value(QStringLiteral("targetVehicleIds")))
        return rejection(QStringLiteral("actual-whole-new-read-content-basis-required"));
    QPointer<FleetFirmwareAdapterCatalog> catalog(review->nativeFirmwareCatalog());
    if (!alive() || !catalog) return rejection(QStringLiteral("actual-review-catalog-required"));
    QPointer<FleetMissionReviewCoordinator> coordinator(review->nativeReviewCoordinator());
    if (!alive() || !catalog || !coordinator || coordinator->firmwareAdapterCatalogBackend() != catalog.data())
        return rejection(QStringLiteral("actual-catalog-provider-required"));
    const QVariantMap deploymentContext = deployment->startReviewContext(deploymentId);
    if (!alive() || !catalog || !coordinator || !isTrue(deploymentContext.value(QStringLiteral("ok")))
        || deploymentContext.value(QStringLiteral("binding")) != contentContext.value(QStringLiteral("binding"))
        || deploymentContext.value(QStringLiteral("generation")) != contentContext.value(QStringLiteral("deploymentGeneration"))
        || deploymentContext.value(QStringLiteral("plan")) != contentContext.value(QStringLiteral("plan"))
        || deploymentContext.value(QStringLiteral("payloadBuild")) != contentContext.value(QStringLiteral("payloadBuild")))
        return rejection(QStringLiteral("same-actual-deployment-and-complete-payload-required"));
    const QVariantMap binding = contentContext.value(QStringLiteral("binding")).toMap();
    QByteArray recordBytes, deploymentBytes;
    if (!currentRecord(store.data(), binding, &recordBytes) || !encode(deploymentContext, &deploymentBytes)) return rejection(QStringLiteral("latest-full-nonrestored-record-required"));
    const QVariantMap plan = contentContext.value(QStringLiteral("plan")).toMap();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    const bool clearanceRequired = constraints.contains(QStringLiteral("clearanceInput"));
    const auto clearance = clearanceRequired ? review->startupClearanceWitness(reviewId) : FleetMissionStartupClearanceWitness::Ptr();
    if (!alive() || !clearanceBindingMatches(clearanceRequired, clearance, artifact)
        || (clearanceRequired && (!clearance->sourceCurrent(alive) || !alive())))
        return rejection(QStringLiteral("original-native-startup-clearance-witness-required"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    const QVariantList payloads = contentContext.value(QStringLiteral("payloadBuild")).toMap().value(QStringLiteral("payloads")).toList();
    const QVariantList readbacks = contentContext.value(QStringLiteral("readbacks")).toList();
    const QVariantList rows = artifact.value(QStringLiteral("vehicles")).toList();
    const QVariantList ids = contentContext.value(QStringLiteral("targetVehicleIds")).toList();
    if (ids.isEmpty() || ids.size() > 32 || members.size() != ids.size() || payloads.size() != ids.size()
        || readbacks.size() != ids.size() || rows.size() != ids.size()) return rejection(QStringLiteral("whole-bounded-native-target-set-required"));
    const auto byId = [](const QVariantList& list, int id) {
        QVariantMap found;
        for (const QVariant& entry : list) {
            double number = 0;
            if (entry.type() != QVariant::Map || !integer(entry.toMap().value(QStringLiteral("vehicleId")), 1, 255, &number)) return QVariantMap();
            if (number == id) { if (!found.isEmpty()) return QVariantMap(); found = entry.toMap(); }
        }
        return found;
    };
    QList<Target> targets;
    QSet<int> seen;
    int itemBudget = 0;
    for (const QVariant& value : ids) {
        double number = 0;
        if (!integer(value, 1, 255, &number) || seen.contains(static_cast<int>(number))) return rejection(QStringLiteral("native-target-identity-invalid"));
        Target target;
        target.vehicleId = static_cast<int>(number); seen.insert(target.vehicleId);
        target.member = byId(members, target.vehicleId); target.payload = byId(payloads, target.vehicleId);
        const QVariantMap readback = byId(readbacks, target.vehicleId), row = byId(rows, target.vehicleId);
        const int itemCount = target.payload.value(QStringLiteral("items")).toList().size();
        itemBudget += itemCount;
        if (target.member.isEmpty() || target.payload.isEmpty() || readback.isEmpty() || row.isEmpty()
            || itemCount < 1 || itemCount > 2048 || itemBudget > 8192 || !isTrue(row.value(QStringLiteral("controlledArmPreconditionsMatch"))))
            return rejection(QStringLiteral("same-complete-native-members-payloads-readbacks-and-phase-facts-required"));
        target.vehicle = content->reviewVehicle(checkId, deploymentId, target.vehicleId);
        if (!alive() || !target.vehicle) return rejection(QStringLiteral("actual-new-read-vehicle-required"));
        target.mission = content->reviewManager(checkId, deploymentId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission) return rejection(QStringLiteral("actual-new-read-manager-required"));
        target.primary = content->reviewPrimaryLink(checkId, deploymentId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission || !target.primary) return rejection(QStringLiteral("actual-new-read-primary-required"));
        target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager(); target.componentId = target.vehicle->defaultComponentId();
        target.readReceipt = target.mission->fleetMissionReadReceipt(MaximumLifetimeMs);
        if (!alive() || !target.vehicle || !target.mission || !target.primary || !target.parameters || !target.links
            || !isTrue(target.readReceipt.value(QStringLiteral("available"))) || !isTrue(target.readReceipt.value(QStringLiteral("fresh")))
            || target.readReceipt.size() != 34) return rejection(QStringLiteral("complete-current-native-raw-read-receipt-required"));
        QVariantMap receiptStable = target.readReceipt; receiptStable.remove(QStringLiteral("ageMs"));
        if (receiptStable != readback.value(QStringLiteral("nativeReceipt")).toMap()) return rejection(QStringLiteral("same-frozen-complete-raw-scope-required"));
        target.readiness = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, constraints,
            target.payload, FleetMissionStartReadiness::Phase::BeforeArm, catalog.data());
        if (!alive() || !target.vehicle || !target.mission || !target.primary || !target.parameters || !target.links || !catalog || !coordinator
            || !isTrue(target.readiness.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !isTrue(target.readiness.value(QStringLiteral("controlledArmPreconditionsMatch")))
            || target.readiness.value(QStringLiteral("sourceFingerprint")) != row.value(QStringLiteral("readinessSummary")).toMap().value(QStringLiteral("sourceFingerprint"))
            || !digest(target.readiness.value(QStringLiteral("startupPhaseFingerprint")))
            || !altitudeMatches(target.readiness, row.value(QStringLiteral("readinessSummary")).toMap(),
                target.readiness.value(QStringLiteral("startupTransitionPolicy")).toMap())) return rejection(QStringLiteral("unchanged-full-native-before-arm-facts-required"));
        if (target.readiness.value(QStringLiteral("startupTransitionPolicy")).toMap().value(QStringLiteral("schemaVersion")).toInt() == 4)
            target.readiness.insert(QStringLiteral("startupAltitudeReferenceSample"),
                row.value(QStringLiteral("readinessSummary")).toMap().value(QStringLiteral("startupAltitudeReferenceSample")));
        target.readinessFingerprint = target.readiness.value(QStringLiteral("startupPhaseFingerprint")).toString();
        targets.append(target);
    }
    const QVariantMap leader = artifact.value(QStringLiteral("leaderBinding")).toMap();
    if (leader.value(QStringLiteral("requiredByFrozenRequest")).type() != QVariant::Bool) return rejection(QStringLiteral("actual-leader-requirement-required"));
    const bool requireLeader = leader.value(QStringLiteral("requiredByFrozenRequest")).toBool();
    double plannedLeader = 0;
    if (!integer(leader.value(QStringLiteral("plannedLeaderVehicleId")), 0, 255, &plannedLeader)
        || ((requireLeader || plannedLeader > 0) && !isTrue(leader.value(QStringLiteral("currentLeaderBindingMatches")))))
        return rejection(QStringLiteral("actual-required-leader-binding-not-current"));
    QPointer<FleetRegistry> registry(health->fleetRegistry());
    QPointer<FaultToleranceManager> faults(health->faultToleranceManager());
    const QVariantMap healthGate = health->evaluateTaskGate(ids, requireLeader);
    const QVariantMap catalogSnapshot = catalog->snapshot();
    if (!alive() || !catalog || !coordinator || !isTrue(healthGate.value(QStringLiteral("allowed")))
        || select(healthGate, {QStringLiteral("allowed"), QStringLiteral("state"), QStringLiteral("selectedVehicleIds"), QStringLiteral("blockingVehicleIds"),
            QStringLiteral("warningVehicleIds"), QStringLiteral("requireLeader")}) != artifact.value(QStringLiteral("healthGate")).toMap()
        || select(catalogSnapshot, {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogFingerprint"), QStringLiteral("entryCount")})
            != artifact.value(QStringLiteral("firmwareAdapterCatalogBinding")).toMap()) return rejection(QStringLiteral("unchanged-actual-fleet-health-and-catalog-required"));
    if (!review->approvalContextCurrent(reviewId, reviewContext) || !alive()) return rejection(QStringLiteral("review-changed-before-conditional-consumption"));
    if (!content->reviewContextCurrent(checkId, deploymentId, contentContext) || !alive()) return rejection(QStringLiteral("new-read-basis-changed-before-consumption"));
    if (clearanceRequired) {
        const auto currentWitness = review->startupClearanceWitness(reviewId);
        if (!alive() || currentWitness != clearance || !clearance->memoryCurrent())
            return rejection(QStringLiteral("original-native-startup-clearance-witness-changed"));
    }
    if (_generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("authority-generation-exhausted"));
    _disconnectSources(); ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _review = review; _approval = approval; _content = content;
    _deployment = deployment; _swarm = swarm; _health = health; _authorization = owner; _coordinator = coordinator; _catalog = catalog;
    _registry = registry; _faultPolicy = faults; _identityReceipt = identity; _roleReceipt = role; _reviewContext = reviewContext;
    _contentContext = contentContext; _deploymentContext = deploymentContext; _binding = binding; _constraints = constraints;
    _clearanceRequired = clearanceRequired; _clearanceWitness = clearance;
    _recordBytes = recordBytes; _deploymentBytes = deploymentBytes; _targets = targets; _vehicleIds = ids;
    _healthGate = healthGate; _catalogSnapshot = catalogSnapshot; _requireLeader = requireLeader;
    _leaderId = faults ? faults->leaderVehicleId() : 0; _leaderRegistered = registry && _leaderId > 0 && registry->containsVehicle(_leaderId);
    _conditionalGrant.clear(); _authenticationReceipt.clear(); _sourceHash.clear(); _reservationHeld = false;
    _reservationTransferred = false; _transferredTransaction.clear(); _transferredTransactionEpoch.clear();
    _startupId = QUuid::createUuid().toString(QUuid::WithoutBraces); _state = QStringLiteral("preparing"); _reason.clear();
    _expiresAtMs = qMin(beforeReview + static_cast<qint64>(reviewRemaining), beforeContent + static_cast<qint64>(contentRemaining));
    _connectSources(generation);
    if (!_gate(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("authority-before-transfer-source-invalid"));
        return rejection(QStringLiteral("authority-before-transfer-source-invalid"));
    }
    const QVariantMap approvalCurrent = approval->current();
    const QVariantMap receipt = approvalCurrent.value(QStringLiteral("approvalReceipt")).toMap();
    if (!alive() || receipt.value(QStringLiteral("reviewId")) != reviewId) {
        if (self) _invalidate(generation, QStringLiteral("actual-conditional-approval-for-same-review-required"));
        return rejection(QStringLiteral("actual-conditional-approval-for-same-review-required"));
    }
    const QString startupId = _startupId, epoch = _authorityEpoch;
    const QString consumedApprovalId = receipt.value(QStringLiteral("approvalId")).toString();
    if (!alive() || !_live(generation)) {
        _invalidate(generation, QStringLiteral("authority-episode-changed-before-conditional-consumption"), QStringLiteral("failed"));
        return rejection(QStringLiteral("authority-episode-changed-before-conditional-consumption"));
    }
    const QVariantMap grant = approval->consumeStartApproval(session.data(), roles.data(), store.data(), review.data(), deployment.data(), owner.data(), receipt, startupId);
    if (!self) {
        if (approval && uuid(consumedApprovalId)) approval->revokeCurrent(consumedApprovalId);
        return rejection(QStringLiteral("authority-destroyed-at-consumption"));
    }
    if (!alive() || !_live(generation)) {
        if (approval && uuid(consumedApprovalId)) approval->revokeCurrent(consumedApprovalId);
        _invalidate(generation, QStringLiteral("conditional-consumption-episode-changed"), QStringLiteral("failed"));
        return rejection(QStringLiteral("conditional-consumption-episode-changed"));
    }
    _conditionalGrant = grant;
    if (grant.isEmpty() || !_live(generation) || !approval || !approval->grantCurrent(grant, startupId) || !self || !alive()) {
        if (self) _invalidate(generation, QStringLiteral("conditional-approval-not-consumed-current"));
        return rejection(QStringLiteral("conditional-approval-not-consumed-current"));
    }
    QVariantList physicalBasis;
    for (const Target& target : _targets) physicalBasis.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("member"), target.member}, {QStringLiteral("payload"), target.payload},
        {QStringLiteral("readReceipt"), target.readReceipt}, {QStringLiteral("beforeArmReadiness"), target.readiness}});
    _sourceHash = hash(QVariantMap{{QStringLiteral("reviewContext"), stable(reviewContext)}, {QStringLiteral("contentContext"), stable(contentContext)},
        {QStringLiteral("conditionalGrant"), grant}, {QStringLiteral("identity"), identity}, {QStringLiteral("role"), role},
        {QStringLiteral("recordHash"), QString::fromLatin1(QCryptographicHash::hash(recordBytes, QCryptographicHash::Sha256).toHex())},
        {QStringLiteral("healthGate"), healthGate}, {QStringLiteral("catalogSnapshot"), catalogSnapshot},
        {QStringLiteral("physicalBasis"), physicalBasis},
        {QStringLiteral("startupId"), startupId}, {QStringLiteral("authorityEpoch"), epoch}});
    if (!digest(_sourceHash) || !_gate(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("bounded-authority-source-hash-invalid"));
        return rejection(QStringLiteral("bounded-authority-source-hash-invalid"));
    }
    const qint64 beforeTransfer = _clock.elapsed();
    const qint64 maximumRemaining = qMin<qint64>(MaximumLifetimeMs, _expiresAtMs - beforeTransfer);
    if (!alive() || !_live(generation)) {
        _invalidate(generation, QStringLiteral("authority-episode-changed-before-authentication-handoff"), QStringLiteral("failed"));
        return rejection(QStringLiteral("authority-episode-changed-before-authentication-handoff"));
    }
    const QVariantMap transferred = approval->transferToStartup(self.data(), grant, startupId, _sourceHash, maximumRemaining);
    if (!self) return rejection(QStringLiteral("authority-destroyed-at-handoff")); // Session watches this exact owner's destruction.
    const QVariantMap authentication = transferred.value(QStringLiteral("authenticationReceipt")).toMap();
    double remaining = 0;
    if (!_live(generation) || !alive() || !isTrue(transferred.value(QStringLiteral("ok")))
        || transferred.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map || authentication.size() != 22
        || !integer(transferred.value(QStringLiteral("remainingMs")), 1, MaximumLifetimeMs, &remaining)) {
        _invalidate(generation, QStringLiteral("actual-authentication-handoff-not-current"), QStringLiteral("failed"));
        return rejection(QStringLiteral("actual-authentication-handoff-not-current"));
    }
    _authenticationReceipt = authentication;
    _expiresAtMs = qMin(_expiresAtMs, beforeTransfer + static_cast<qint64>(remaining));
    if (!_gate(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("handoff-actual-native-sources-invalid"));
        return rejection(QStringLiteral("handoff-actual-native-sources-invalid"));
    }
    const bool reserved = swarm->reserveMissionDeployment(startupId, ids);
    if (!self) { if (swarm && reserved) swarm->releaseMissionDeployment(startupId); return rejection(QStringLiteral("authority-destroyed-at-reservation")); }
    if (!reserved || !_live(generation) || !alive()) {
        if (swarm && reserved) swarm->releaseMissionDeployment(startupId);
        _invalidate(generation, QStringLiteral("whole-group-authority-reservation-denied"), QStringLiteral("failed"));
        return rejection(QStringLiteral("whole-group-authority-reservation-denied"));
    }
    _reservationHeld = true;
    if (!_gate(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("reserved-authority-context-invalid"));
        return rejection(QStringLiteral("reserved-authority-context-invalid"));
    }
    _state = QStringLiteral("accepted"); _timer.start();
    emit changed();
    if (!self || !_live(generation) || !_gate(generation, true)) return rejection(QStringLiteral("authority-revoked-at-publication"));
    return current();
}
bool FleetMissionStartupAuthority::authenticationTransferContextCurrent(const QString& suppliedStartupId, const QString& suppliedSourceHash,
    QObject* actualStartApproval, const QVariantMap& suppliedGrant)
{
    const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QVariantMap grant = suppliedGrant;
    if (thread() != QThread::currentThread() || _state != QStringLiteral("preparing") || !_busy || _checking || _reentered || !_live(_generation)
        || startupId != _startupId || sourceHash != _sourceHash || !digest(sourceHash)
        || actualStartApproval != _approval.data() || grant.isEmpty() || grant != _conditionalGrant || !_authenticationReceipt.isEmpty()
        || _reservationHeld || _reservationTransferred
        || !outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})) return false;
    // Passive ownership admission ONLY. The caller brackets this with full
    // source/account gates; no Session/WTS, QML or recursive _gate here.
    for (QObject* source : {static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()), static_cast<QObject*>(_swarm.data()),
        static_cast<QObject*>(_health.data()), _authorization.data(), static_cast<QObject*>(_coordinator.data()), static_cast<QObject*>(_catalog.data())})
        if (!source || source->thread() != thread()) return false;
    const qint64 remaining = _expiresAtMs - _clock.elapsed();
    return remaining > 0 && remaining <= MaximumLifetimeMs;
}
QVariantMap FleetMissionStartupAuthority::authorityContext(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _busy || _checking || startupId != _startupId
        || _state != QStringLiteral("accepted") || !_reservationHeld || !_live(_generation)) return rejection(QStringLiteral("current-native-startup-authority-required"));
    if (!outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()}))
        return rejection(QStringLiteral("current-native-authority-episode-required"));
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupAuthority> self(this);
    const quint64 generation = _generation;
    if (!_gate(generation, true) || !self) return rejection(QStringLiteral("actual-startup-authority-source-check-failed"));
    QVariantList physical;
    for (const Target& target : _targets) physical.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("member"), target.member}, {QStringLiteral("payload"), target.payload},
        {QStringLiteral("readReceipt"), target.readReceipt}, {QStringLiteral("beforeArmReadiness"), target.readiness}});
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true}, {QStringLiteral("authorityAccepted"), true},
        {QStringLiteral("startupId"), _startupId}, {QStringLiteral("authorityEpoch"), _authorityEpoch}, {QStringLiteral("authorityGeneration"), QString::number(generation)},
        {QStringLiteral("sourceHash"), _sourceHash}, {QStringLiteral("reviewId"), _reviewContext.value(QStringLiteral("reviewId"))},
        {QStringLiteral("deploymentId"), _reviewContext.value(QStringLiteral("deploymentId"))}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("reviewContext"), stable(_reviewContext)},
        {QStringLiteral("contentContext"), stable(_contentContext)}, {QStringLiteral("deploymentContext"), _deploymentContext},
        {QStringLiteral("conditionalGrant"), _conditionalGrant}, {QStringLiteral("authenticationReceipt"), _authenticationReceipt},
        {QStringLiteral("physicalTargets"), physical}, {QStringLiteral("healthGate"), _healthGate}, {QStringLiteral("catalogSnapshot"), _catalogSnapshot},
        {QStringLiteral("allowedFutureActionScope"), QStringLiteral("seq0-normal-arm-param2-zero-mode-only-auto-mission")},
        {QStringLiteral("preARMOnly"), true}, {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("remainingLifetimeMs"), static_cast<double>(_expiresAtMs - _clock.elapsed())}};
}
bool FleetMissionStartupAuthority::authorityContextCurrent(const QString& suppliedStartupId, const QVariantMap& supplied)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || !outboundSourcesCurrent(this,
        {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})) return false;
    const QVariantMap expected = supplied;
    double schema = 0, remaining = 0;
    if (expected.size() != 25 || !integer(expected.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || !isTrue(expected.value(QStringLiteral("ok"))) || !isTrue(expected.value(QStringLiteral("authorityAccepted")))
        || !isTrue(expected.value(QStringLiteral("preARMOnly"))) || !isFalse(expected.value(QStringLiteral("startupPermitGranted")))
        || !isFalse(expected.value(QStringLiteral("executable"))) || !isFalse(expected.value(QStringLiteral("flightCommandReleased")))
        || !uuid(expected.value(QStringLiteral("startupId"))) || !uuid(expected.value(QStringLiteral("authorityEpoch")))
        || !digest(expected.value(QStringLiteral("sourceHash"))) || !integer(expected.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumLifetimeMs, &remaining)) return false;
    const QPointer<FleetMissionStartupAuthority> self(this);
    const QVariantMap actual = authorityContext(startupId);
    QByteArray expectedBytes, actualBytes;
    return self && isTrue(actual.value(QStringLiteral("ok"))) && encode(stable(expected), &expectedBytes)
        && encode(stable(actual), &actualBytes) && expectedBytes == actualBytes;
}
QVariantMap FleetMissionStartupAuthority::actualSourcesForStartupTransaction(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (thread() != QThread::currentThread() || _busy || _checking || _state != QStringLiteral("accepted")
        || startupId != _startupId || !_reservationHeld || !_live(_generation)) return {};
    if (!outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})) return {};
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    const quint64 generation = _generation;
    if (!_gate(generation, true) || !self) return {};
    const auto object = [](QObject* source) { return QVariant::fromValue(source); };
    return {{QStringLiteral("session"), object(_session.data())}, {QStringLiteral("roles"), object(_roles.data())},
        {QStringLiteral("store"), object(_store.data())}, {QStringLiteral("deployment"), object(_deployment.data())},
        {QStringLiteral("swarm"), object(_swarm.data())}, {QStringLiteral("health"), object(_health.data())},
        {QStringLiteral("authorizationOwner"), object(_authorization.data())}, {QStringLiteral("authority"), object(self.data())}};
}

FleetFirmwareAdapterCatalog* FleetMissionStartupAuthority::nativeFirmwareCatalog(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (thread() != QThread::currentThread() || _busy || _checking || _state != QStringLiteral("accepted")
        || startupId != _startupId || !_reservationHeld || !_live(_generation)) return nullptr;
    if (!outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})) return nullptr;
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    if (!_gate(_generation, true) || !self) return nullptr;
    return _catalog.data();
}

FleetMissionReviewCoordinator* FleetMissionStartupAuthority::nativeReviewCoordinator(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (!nativeFirmwareCatalog(startupId) || !self) return nullptr;
    return _coordinator.data();
}

FleetRegistry* FleetMissionStartupAuthority::nativeRegistry(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (!nativeFirmwareCatalog(startupId) || !self) return nullptr;
    return _registry.data();
}

FaultToleranceManager* FleetMissionStartupAuthority::nativeFaultPolicy(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupAuthority> self(this);
    if (!nativeFirmwareCatalog(startupId) || !self) return nullptr;
    return _faultPolicy.data();
}

std::shared_ptr<FleetMissionStartupClearanceWitness>
FleetMissionStartupAuthority::startupClearanceWitness(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _busy || _checking || _state != QStringLiteral("accepted")
        || startupId != _startupId || !_reservationHeld || _reservationTransferred || !_live(_generation)) return {};
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupAuthority> self(this);
    const quint64 generation = _generation;
    const auto witness = _clearanceWitness;
    const bool required = _clearanceRequired;
    if (!_gate(generation, true) || !self || !_live(generation) || _state != QStringLiteral("accepted")
        || startupId != _startupId || !_reservationHeld || _reservationTransferred
        || _clearanceRequired != required || _clearanceWitness != witness
        || !clearanceBindingMatches(required, witness, _reviewContext.value(QStringLiteral("reviewArtifact")).toMap())
        || (required && !witness->memoryCurrent())) return {};
    return witness;
}

QVariantMap FleetMissionStartupAuthority::transferToStartupTransaction(QObject* transactionObject,
    const QVariantMap& suppliedContext, const QString& suppliedStartupId, const QString& suppliedSourceHash,
    qint64 maximumRemainingMs)
{
    const QVariantMap context = suppliedContext;
    const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QPointer<FleetMissionStartupAuthority> self(this);
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (thread() != QThread::currentThread() || _busy || _checking || _state != QStringLiteral("accepted")
        || !_reservationHeld || _reservationTransferred || !transaction || transaction->thread() != thread()
        || !outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data(), transaction.data()})
        || startupId != _startupId || sourceHash != _sourceHash || !uuid(startupId) || !digest(sourceHash)
        || maximumRemainingMs < 1 || maximumRemainingMs > MaximumLifetimeMs) return rejection(QStringLiteral("actual-accepted-startup-authority-required"));
    const QString transactionEpoch = transaction->transactionEpoch(), epoch = _authorityEpoch;
    if (!uuid(transactionEpoch)) return rejection(QStringLiteral("actual-startup-transaction-epoch-required"));
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetMissionStartReview> review(_review);
    const QPointer<FleetMissionStartApproval> approval(_approval);
    const QPointer<FleetMissionStartContentCheck> content(_content);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QVariantMap authentication = _authenticationReceipt;
    const auto clearance = _clearanceWitness;
    const bool clearanceRequired = _clearanceRequired;
    const QVariantList ids = _vehicleIds;
    const qint64 entered = _clock.elapsed(), deadline = qMin(_expiresAtMs, entered + maximumRemainingMs);
    if (!authorityContextCurrent(startupId, context) || !self || !session || !swarm || !transaction)
        return rejection(QStringLiteral("startup-authority-context-not-current-before-transaction"));
    _busy = true;
    FlagGuard transferring{QPointer<QObject>(this), &_busy};
    const auto same = [&]() {
        return self && session && swarm && transaction && _generation == generation && _state == QStringLiteral("accepted")
            && _review == review && _approval == approval && _content == content && _deployment == deployment
            && outboundSourcesCurrent(self.data(), {session.data(), review.data(), approval.data(), content.data(), deployment.data(), transaction.data()})
            && _session == session && _swarm == swarm && _startupId == startupId && _sourceHash == sourceHash && _authorityEpoch == epoch
            && _authenticationReceipt == authentication && _vehicleIds == ids && _reservationHeld && !_reservationTransferred
            && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired
            && transaction->thread() == thread() && transaction->transactionEpoch() == transactionEpoch && _clock.elapsed() < deadline;
    };
    if (!same() || !_gate(generation, true) || !same()
        || !transaction->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), context) || !same()
        || !_gate(generation, true) || !same()) return rejection(QStringLiteral("actual-startup-transaction-pre-handoff-gate-invalid"));
    const qint64 remainingBefore = deadline - _clock.elapsed();
    if (remainingBefore < 1 || !same() || (clearanceRequired && (!clearance || !clearance->memoryCurrent())))
        return rejection(QStringLiteral("original-startup-authority-budget-expired"));
    const QVariantMap movedResult = session->transferStartupAuthenticationToTransaction(authentication, self.data(), transaction.data(),
        startupId, epoch, transactionEpoch, sourceHash, int(remainingBefore));
    const bool moved = isTrue(movedResult.value(QStringLiteral("ok")));
    const QVariantMap receipt = movedResult.value(QStringLiteral("authenticationReceipt")).toMap();
    const auto revokeMoved = [&]() {
        if (moved && session && transaction) session->revokeStartupTransactionAuthentication(transaction.data(), startupId, transactionEpoch);
    };
    double duration = 0;
    if (!same() || !moved || movedResult.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map
        || !integer(receipt.value(QStringLiteral("durationMs")), 1, MaximumLifetimeMs, &duration)
        || (clearanceRequired && (!clearance || !clearance->sourceCurrent(same) || !same() || !clearance->memoryCurrent()))) {
        revokeMoved();
        if (self && moved) _invalidate(generation, QStringLiteral("startup-transaction-handoff-not-committed"), QStringLiteral("failed"));
        return rejection(QStringLiteral("startup-transaction-handoff-not-committed"));
    }
    // Same UUID/reservation; only the actual native OWNER changes. Old preARM
    // watchers/timer must not revoke an expected, owned later ARM transition.
    _state = QStringLiteral("transferred"); _reason.clear(); _timer.stop(); _disconnectSources();
    _authenticationReceipt.clear(); _reservationHeld = false; _reservationTransferred = true;
    _transferredTransaction = transaction; _transferredTransactionEpoch = transactionEpoch;
    const auto transferred = [&]() {
        return self && session && swarm && transaction && _generation == generation && _state == QStringLiteral("transferred")
            && _review == review && _approval == approval && _content == content && _deployment == deployment
            && outboundSourcesCurrent(self.data(), {session.data(), review.data(), approval.data(), content.data(), deployment.data(), transaction.data()})
            && _session == session && _swarm == swarm && _startupId == startupId && _sourceHash == sourceHash && _authorityEpoch == epoch
            && _transferredTransaction == transaction && _transferredTransactionEpoch == transactionEpoch && _vehicleIds == ids
            && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired
            && !_reservationHeld && _reservationTransferred && _authenticationReceipt.isEmpty() && _clock.elapsed() < deadline;
    };
    const bool clearanceCurrent = transferred() && (!clearanceRequired
        || (clearance && clearance->sourceCurrent(transferred) && transferred()));
    const bool nativeCurrent = clearanceCurrent && session->startupTransactionAuthenticationCurrent(receipt, transaction.data(), startupId, transactionEpoch, sourceHash);
    const bool ownerCurrent = transferred() && nativeCurrent && swarm->missionDeploymentOwns(startupId, ids)
        && transaction->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), context) && transferred()
        && (!clearanceRequired || clearance->memoryCurrent());
    const qint64 now = self ? _clock.elapsed() : deadline;
    const qint64 remaining = qMin(deadline - now, qint64(duration) - (now - entered));
    if (!ownerCurrent || !transferred() || remaining < 1) {
        revokeMoved();
        // A new owner can disappear before its return wrapper commits. Only
        // this exact never-reused UUID is released, not a replacement scope.
        if (swarm && swarm->missionDeploymentOwns(startupId, ids)) swarm->releaseMissionDeployment(startupId);
        if (self && _generation == generation && _startupId == startupId && _state == QStringLiteral("transferred")) {
            _reservationTransferred = false; _state = QStringLiteral("failed"); _reason = QStringLiteral("startup-transaction-post-handoff-gate-invalid");
            QTimer::singleShot(0, this, [self, generation, startupId]() {
                if (self && self->_generation == generation && self->_startupId == startupId && self->_state == QStringLiteral("failed")) emit self->changed();
            });
        }
        QVariantMap denied = rejection(QStringLiteral("startup-transaction-post-handoff-gate-invalid"));
        denied[QStringLiteral("reservationTransferred")] = false;
        return denied;
    }
    QTimer::singleShot(0, this, [self, generation, startupId, transactionEpoch]() {
        if (self && self->_generation == generation && self->_startupId == startupId && self->_state == QStringLiteral("transferred")
            && self->_transferredTransactionEpoch == transactionEpoch) emit self->changed();
    });
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), receipt}, {QStringLiteral("startupId"), startupId},
        {QStringLiteral("transactionEpoch"), transactionEpoch}, {QStringLiteral("sourceHash"), sourceHash},
        {QStringLiteral("remainingMs"), double(remaining)}, {QStringLiteral("reservationTransferred"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

Vehicle* FleetMissionStartupAuthority::authorityVehicle(const QString& suppliedStartupId, int vehicleId) const
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _state != QStringLiteral("accepted") || startupId != _startupId
        || !outboundSourcesCurrent(this, {_session.data(), _review.data(), _approval.data(), _content.data(), _deployment.data()})
        || !_reservationHeld || !_live(_generation) || !_targetsCurrent(_generation)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.vehicle.data();
    return nullptr;
}
MissionManager* FleetMissionStartupAuthority::authorityManager(const QString& suppliedStartupId, int vehicleId) const
{
    const QString startupId = suppliedStartupId;
    if (!authorityVehicle(startupId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.mission.data();
    return nullptr;
}
LinkInterface* FleetMissionStartupAuthority::authorityPrimaryLink(const QString& suppliedStartupId, int vehicleId) const
{
    const QString startupId = suppliedStartupId;
    if (!authorityVehicle(startupId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.primary.data();
    return nullptr;
}
bool FleetMissionStartupAuthority::revokeAuthority(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || startupId != _startupId) return false;
    if (_state == QStringLiteral("transferred")) {
        if (_busy || _checking) return false;
        _busy = true;
        FlagGuard forwarding{QPointer<QObject>(this), &_busy};
        const QPointer<FleetMissionStartupAuthority> self(this);
        const QPointer<FleetMissionStartupTransaction> transaction(_transferredTransaction);
        const quint64 generation = _generation;
        const QString epoch = _transferredTransactionEpoch, sourceHash = _sourceHash;
        const auto same = [&]() {
            return self && _generation == generation && _state == QStringLiteral("transferred") && _startupId == startupId
                && _sourceHash == sourceHash && _transferredTransactionEpoch == epoch && _transferredTransaction == transaction
                && _reservationTransferred && !_reservationHeld && _authenticationReceipt.isEmpty();
        };
        const auto exact = [&](const QVariantMap& status) {
            return same() && transaction && transaction->thread() == thread() && transaction->transactionEpoch() == epoch
                && status.value(QStringLiteral("startupId")).type() == QVariant::String && status.value(QStringLiteral("startupId")).toString() == startupId
                && status.value(QStringLiteral("transactionEpoch")).type() == QVariant::String && status.value(QStringLiteral("transactionEpoch")).toString() == epoch
                && status.value(QStringLiteral("sourceHash")).type() == QVariant::String && status.value(QStringLiteral("sourceHash")).toString() == sourceHash;
        };
        const auto terminal = [&](const QVariantMap& status) {
            const QString state = status.value(QStringLiteral("state")).toString();
            return exact(status) && status.value(QStringLiteral("state")).type() == QVariant::String
                && (state == QStringLiteral("cancelled") || state == QStringLiteral("failed") || state == QStringLiteral("expired")
                    || state == QStringLiteral("stale") || state == QStringLiteral("started-observed"))
                && isFalse(status.value(QStringLiteral("active"))) && isFalse(status.value(QStringLiteral("reservationHeld")))
                && isFalse(status.value(QStringLiteral("transportDraining")));
        };
        bool retired = !transaction;
        if (transaction) {
            const QVariantMap before = transaction->current();
            if (!same()) return false;
            if (exact(before)) {
                if (terminal(before)) retired = true;
                else {
                    transaction->revokeStartup(startupId); // Exact explicit cancellation, NOT old Review invalidation.
                    if (!same()) return false;
                    retired = !transaction || terminal(transaction->current());
                }
            }
        }
        if (!same()) return false;
        if (!retired) { _reason = QStringLiteral("startup-transaction-cancellation-or-drain-required"); emit changed(); return false; }
        _state = QStringLiteral("revoked"); _reason = QStringLiteral("transferred-startup-transaction-retired");
        _reservationTransferred = false;
        // The actual Transaction owns/revokes/releases its exact basis. This
        // old Authority never clears a replacement proof or frees its UUID.
        emit changed();
        return true;
    }
    if (_state != QStringLiteral("preparing") && _state != QStringLiteral("accepted")) return false;
    _invalidate(_generation, QStringLiteral("operator-revoked-startup-authority"), QStringLiteral("revoked"));
    return true;
}

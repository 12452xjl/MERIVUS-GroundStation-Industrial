#include "FleetMissionGroundStartApproval.h"
#include "FaultToleranceManager.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
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
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>
#include <initializer_list>

namespace {
bool outboundGroup(const QObject* owner, std::initializer_list<const QObject*> peers)
{
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)) return false;
    for (const QObject* peer : peers)
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, peer)) return false;
    return true;
}
constexpr qint64 MaximumMs = 30000, MaximumBytes = 32LL * 1024 * 1024;
constexpr double ExactMaximum = 9007199254740991.0;
const QString Action = QStringLiteral("mission.start.ground.reassess.approve");
const QString Purpose = QStringLiteral("new-ground-reassessment-conditional-intent-not-dispatch");
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    switch (value.type()) { case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break; default: return false; }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < minimum || number > maximum || std::floor(number) != number) return false;
    if (output) *output = number; return true;
}
bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar c : value.toString()) if (!(c >= QLatin1Char('0') && c <= QLatin1Char('9')) && !(c >= QLatin1Char('a') && c <= QLatin1Char('f'))) return false;
    return true;
}
bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > 20) return false;
    bool ok = false; const quint64 number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}
bool bounded(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 24 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += qint64(it.key().size()) * 2;
            if (*amount > MaximumBytes || !bounded(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List: for (const QVariant& item : value.toList()) if (!bounded(item, depth + 1, nodes, amount)) return false; return true;
    case QVariant::StringList: for (const QString& item : value.toStringList()) if (!bounded(item, depth + 1, nodes, amount)) return false; return true;
    case QVariant::String: *amount += qint64(value.toString().size()) * 2; return *amount <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool bytes(const QVariantMap& map, QByteArray* output)
{
    int nodes = 0; qint64 amount = 0;
    if (map.isEmpty() || !bounded(map, 0, &nodes, &amount)) return false;
    *output = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !output->isEmpty() && output->size() <= MaximumBytes;
}
bool sameMap(const QVariantMap& left, const QVariantMap& right)
{
    QByteArray first, second;
    return bytes(left, &first) && bytes(right, &second) && first == second;
}
QString hash(const QVariantMap& map)
{
    QByteArray encoded;
    return bytes(map, &encoded) ? QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex()) : QString();
}
QVariantMap stable(QVariantMap map) { map.remove(QStringLiteral("remainingLifetimeMs")); return map; }
QObject* object(const QVariant& value) { return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr; }
bool ids(const QVariant& value)
{
    if (value.type() != QVariant::List) return false;
    const QVariantList list = value.toList(); if (list.isEmpty() || list.size() > 32) return false;
    QSet<int> seen;
    for (const QVariant& id : list) { double n = 0; if (!integer(id, 1, 255, &n) || seen.contains(int(n))) return false; seen.insert(int(n)); }
    return true;
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result)
{
    const QVariantMap latest = store->current(binding.value(QStringLiteral("sourceTaskId")).toString());
    const QVariantMap record = store->record(binding.value(QStringLiteral("sourceTaskId")).toString(), binding.value(QStringLiteral("recordRevision")));
    if (!no(latest.value(QStringLiteral("restoredFromArchive"))) || !no(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (!binding.contains(key) || binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return bytes(record, result);
}
bool contextShape(const QVariantMap& map)
{
    if (map.size() != 20 || !integer(map.value(QStringLiteral("schemaVersion")), 1, 1) || !yes(map.value(QStringLiteral("ok")))
        || !uuid(map.value(QStringLiteral("reviewId"))) || !uuid(map.value(QStringLiteral("reviewEpoch"))) || !decimal(map.value(QStringLiteral("generation")))
        || !uuid(map.value(QStringLiteral("previousStartupId"))) || !uuid(map.value(QStringLiteral("previousTransactionEpoch")))
        || !uuid(map.value(QStringLiteral("deploymentId"))) || map.value(QStringLiteral("binding")).type() != QVariant::Map
        || !ids(map.value(QStringLiteral("targetVehicleIds"))) || !digest(map.value(QStringLiteral("sourceFingerprint")))
        || map.value(QStringLiteral("reviewArtifact")).type() != QVariant::Map || !yes(map.value(QStringLiteral("reviewAcknowledged")))
        || !yes(map.value(QStringLiteral("allNewRawReadsMatched"))) || !integer(map.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumMs)
        || !no(map.value(QStringLiteral("previousAuthorizationRestored"))) || !no(map.value(QStringLiteral("startupPermitGranted")))
        || !no(map.value(QStringLiteral("executable"))) || !no(map.value(QStringLiteral("flightCommandReleased")))
        || map.value(QStringLiteral("scope")).type() != QVariant::String
        || map.value(QStringLiteral("scope")).toString() != QStringLiteral("new-risk-review-only-not-new-startup-authority")) return false;
    const QVariantMap artifact = map.value(QStringLiteral("reviewArtifact")).toMap();
    return hash(artifact) == map.value(QStringLiteral("sourceFingerprint")).toString() && artifact.value(QStringLiteral("targetVehicleIds")) == map.value(QStringLiteral("targetVehicleIds"))
        && artifact.value(QStringLiteral("binding")) == map.value(QStringLiteral("binding")) && no(artifact.value(QStringLiteral("previousAuthorizationRestored")))
        && no(artifact.value(QStringLiteral("startupPermitGranted"))) && no(artifact.value(QStringLiteral("executable"))) && no(artifact.value(QStringLiteral("flightCommandReleased")));
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    const QString principal = identity.value(QStringLiteral("role")).toString();
    return identity.size() == 11 && yes(identity.value(QStringLiteral("authenticated")))
        && (principal == QStringLiteral("operator") || principal == QStringLiteral("commander") || principal == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role")) && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start") && yes(role.value(QStringLiteral("allowed")))
        && yes(role.value(QStringLiteral("requiresSecondApproval"))) && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
bool receiptShape(const QVariantMap& map)
{
    if (map.size() != 27 || !integer(map.value(QStringLiteral("schemaVersion")), 1, 1)) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approvalEpoch"), QStringLiteral("reviewId"), QStringLiteral("reviewEpoch"),
        QStringLiteral("deploymentId"), QStringLiteral("previousStartupId")}) if (!uuid(map.value(key))) return false;
    for (const QString& key : {QStringLiteral("generation"), QStringLiteral("reviewGeneration")}) if (!decimal(map.value(key))) return false;
    for (const QString& key : {QStringLiteral("bindingHash"), QStringLiteral("reviewContextHash"), QStringLiteral("sourceFingerprint")}) if (!digest(map.value(key))) return false;
    double approved = 0, expires = 0;
    return map.value(QStringLiteral("binding")).type() == QVariant::Map && ids(map.value(QStringLiteral("targetVehicleIds")))
        && integer(map.value(QStringLiteral("approvedAtMonotonicMs")), 0, ExactMaximum, &approved)
        && integer(map.value(QStringLiteral("expiresAtMonotonicMs")), 0, ExactMaximum, &expires) && expires > approved
        && yes(map.value(QStringLiteral("conditionalIntentApproved"))) && yes(map.value(QStringLiteral("oneUse")))
        && yes(map.value(QStringLiteral("technicalRisksPreserved"))) && yes(map.value(QStringLiteral("requiresFreshSourceRevalidation")))
        && map.value(QStringLiteral("roleAction")).type() == QVariant::String && map.value(QStringLiteral("roleAction")).toString() == QStringLiteral("mission.start")
        && map.value(QStringLiteral("authenticationAction")).type() == QVariant::String && map.value(QStringLiteral("authenticationAction")).toString() == Action
        && map.value(QStringLiteral("purpose")).type() == QVariant::String && map.value(QStringLiteral("purpose")).toString() == Purpose
        && no(map.value(QStringLiteral("startupPermitGranted"))) && no(map.value(QStringLiteral("executable")))
        && no(map.value(QStringLiteral("flightCommandReleased"))) && no(map.value(QStringLiteral("previousAuthorizationRestored")));
}
QVariantMap denied(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason.left(256)}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("previousAuthorizationRestored"), false}};
}
} // namespace

FleetMissionGroundStartApproval::FleetMissionGroundStartApproval(QObject* parent)
    : QObject(parent), _approvalEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(250);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        const quint64 generation = _generation;
        if (_expired()) { _invalidate(generation, QStringLiteral("original-ground-review-or-authentication-budget-expired"), QStringLiteral("expired")); return; }
        if (_busy || _checking || _prompt) return;
        const QPointer<FleetMissionGroundStartApproval> self(this);
        const bool consumed = _state == QStringLiteral("consumed");
        if (!_sourcesCurrent(generation, !consumed && _state == QStringLiteral("approved"), consumed) && self)
            _invalidate(generation, QStringLiteral("ground-approval-native-sources-no-longer-current"));
    });
}
FleetMissionGroundStartApproval::~FleetMissionGroundStartApproval()
{
    _disconnectSources();
    if (_session && !_approvalId.isEmpty()) _session->revokeGroundStartApprovalAuthentication(this, _approvalId, _approvalEpoch);
}
bool FleetMissionGroundStartApproval::_expired() const { return _expiresAtMs < 0 || _clock.elapsed() >= _expiresAtMs; }
bool FleetMissionGroundStartApproval::_same(quint64 generation, const QString& state) const
{
    return generation != 0 && _generation == generation && _state == state && !_expired()
        && outboundGroup(this, {_session.data(), _previous.data(), _deployment.data(), _review.data()})
        && (state == QStringLiteral("pending") || state == QStringLiteral("approved") || state == QStringLiteral("consumed"))
        && _session && _roles && _store && _previous && _deployment && _swarm && _health && _authorization && _review && _catalog && _coordinator && _registry && _faults;
}
QVariantMap FleetMissionGroundStartApproval::_bundle() const
{
    const auto actual = [](QObject* pointer) { return QVariant::fromValue(pointer); };
    return {{QStringLiteral("session"), actual(_session.data())}, {QStringLiteral("roles"), actual(_roles.data())}, {QStringLiteral("store"), actual(_store.data())},
        {QStringLiteral("previousStartup"), actual(_previous.data())}, {QStringLiteral("deployment"), actual(_deployment.data())},
        {QStringLiteral("swarm"), actual(_swarm.data())}, {QStringLiteral("health"), actual(_health.data())},
        {QStringLiteral("authorizationOwner"), actual(_authorization.data())}, {QStringLiteral("groundReview"), actual(_review.data())}};
}
QVariantMap FleetMissionGroundStartApproval::current() const
{
    const bool current = !_expired() && (_state == QStringLiteral("approved") || _state == QStringLiteral("consumed"));
    return {{QStringLiteral("ok"), current}, {QStringLiteral("approvalId"), _approvalId}, {QStringLiteral("approvalEpoch"), _approvalEpoch},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason}, {QStringLiteral("reviewId"), _context.value(QStringLiteral("reviewId"))},
        {QStringLiteral("reviewEpoch"), _context.value(QStringLiteral("reviewEpoch"))}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("targetVehicleIds"), _context.value(QStringLiteral("targetVehicleIds"))}, {QStringLiteral("approvalReceipt"), _receipt},
        {QStringLiteral("remainingMs"), double(qMax<qint64>(0, _expiresAtMs - _clock.elapsed()))}, {QStringLiteral("busy"), _busy},
        {QStringLiteral("conditionalIntentApproved"), current}, {QStringLiteral("technicalRisksPreserved"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("previousAuthorizationRestored"), false}, {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true}};
}
void FleetMissionGroundStartApproval::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionGroundStartApproval::_invalidate(quint64 generation, const QString& suppliedReason, const QString& suppliedState, bool defer)
{
    const QString reason = suppliedReason.left(256), state = suppliedState;
    if (!generation || generation != _generation || (_state != QStringLiteral("pending") && _state != QStringLiteral("approved") && _state != QStringLiteral("consumed"))) return;
    const QPointer<FleetMissionGroundStartApproval> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QString approvalId = _approvalId, epoch = _approvalEpoch;
    _state = state; _reason = reason; _authentication.clear(); _grant.clear(); _timer.stop(); _disconnectSources();
    if (session) session->revokeGroundStartApprovalAuthentication(self.data(), approvalId, epoch);
    if (!self || _generation != generation || _approvalId != approvalId || _state != state) return;
    const auto publish = [self, generation, approvalId, state]() {
        if (self && self->_generation == generation && self->_approvalId == approvalId && self->_state == state) emit self->changed();
    };
    if (defer) QTimer::singleShot(0, this, publish); else publish();
}
void FleetMissionGroundStartApproval::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _invalidate(generation, QStringLiteral("actual-ground-approval-source-changed")); };
    for (QObject* actual : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_previous.data()), static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()),
        static_cast<QObject*>(_health.data()), _authorization.data(), static_cast<QObject*>(_review.data()), static_cast<QObject*>(_catalog.data()),
        static_cast<QObject*>(_coordinator.data()), static_cast<QObject*>(_registry.data()), static_cast<QObject*>(_faults.data())})
        if (actual) _connections.append(connect(actual, &QObject::destroyed, this, stale));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        // This ledger's own secondary credential window is not a change of
        // primary identity. The exact native snapshot is recaptured after it.
        if (_generation != generation || (_prompt && _state == QStringLiteral("pending"))) return;
        if (!_busy && !_checking) {
            const QPointer<FleetMissionGroundStartApproval> self(this);
            const bool valid = _sourcesCurrent(generation, _state == QStringLiteral("approved"), _state == QStringLiteral("consumed"));
            if (self && !valid) _invalidate(generation, QStringLiteral("ground-approval-native-session-changed"));
        }
    }));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_review.data(), &FleetMissionArmedGroundReview::changed, this, stale));
    _connections.append(connect(_previous.data(), &FleetMissionStartupTransaction::changed, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    _connections.append(connect(_swarm.data(), &SwarmController::formationActiveChanged, this, stale));
    _connections.append(connect(_swarm.data(), &SwarmController::formationBusyChanged, this, stale));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, stale));
    _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, stale));
    _connections.append(connect(_coordinator.data(), &FleetMissionReviewCoordinator::firmwareAdapterCatalogChanged, this, stale));
    for (const Target& target : _targets) {
        for (QObject* actual : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(actual, &QObject::destroyed, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, [stale](bool value) { if (value) stale(); }));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this, stale));
    }
}
bool FleetMissionGroundStartApproval::_targetsCurrent(quint64 generation) const
{
    const QPointer<FleetMissionGroundStartApproval> self(const_cast<FleetMissionGroundStartApproval*>(this));
    const QPointer<FleetMissionArmedGroundReview> review(_review);
    const QString state = _state, reviewId = _context.value(QStringLiteral("reviewId")).toString();
    const QList<Target> targets = _targets;
    const auto same = [&]() { return self && _same(generation, state) && review && _review == review; };
    if (!same() || targets.isEmpty() || targets.size() > 32) return false;
    for (const Target& target : targets) {
        const auto alive = [&]() { return same() && target.vehicle && target.mission && target.parameters && target.links && target.primary
            && target.vehicle->thread() == thread() && target.mission->thread() == thread() && target.parameters->thread() == thread()
            && target.links->thread() == thread(); };
        if (!alive()) return false;
        const QPointer<Vehicle> vehicle(review->reviewVehicle(reviewId, target.vehicleId));
        if (!alive() || vehicle != target.vehicle) return false;
        const QPointer<MissionManager> mission(review->reviewManager(reviewId, target.vehicleId));
        if (!alive() || mission != target.mission) return false;
        const QPointer<LinkInterface> primary(review->reviewPrimaryLink(reviewId, target.vehicleId));
        if (!alive() || primary != target.primary) return false;
        const auto shared = target.links->primaryLink().lock();
        if (!alive() || !shared || shared.get() != target.primary.data() || target.vehicle->id() != target.vehicleId
            || target.vehicle->defaultComponentId() != target.componentId || target.componentId < 1 || target.componentId > 255
            || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.vehicle->armed() != target.armed || target.vehicle->flying() || target.links->communicationLost()
            || target.mission->inProgress() || target.mission->transportCallInProgress() || !target.parameters->parametersReady()
            || target.parameters->missingParameters()) return false;
        const bool writes = target.parameters->pendingWrites();
        if (!alive() || writes) return false;
        const bool ground = target.vehicle->fleetArmedGroundReviewSourceMatches(target.primary.data());
        if (!alive() || !ground) return false;
    }
    return same();
}
bool FleetMissionGroundStartApproval::_sourcesCurrent(quint64 generation, bool requireApprover, bool requireProof)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    if (!outboundGroup(this, {_session.data(), _previous.data(), _deployment.data(), _review.data()})) return false;
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionGroundStartApproval> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionArmedGroundReview> review(_review);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FaultToleranceManager> faults(_faults);
    const bool hadRegistry = bool(registry), hadFaults = bool(faults);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap bundle = _bundle(), context = _context, identity = _identity, role = _role, approver = _approver, authentication = _authentication;
    const QVariantMap binding = _binding;
    const QByteArray record = _recordBytes;
    const QString state = _state, approvalId = _approvalId, epoch = _approvalEpoch, bindingHash = _bindingHash, contextHash = _contextHash;
    const QString reviewId = context.value(QStringLiteral("reviewId")).toString(), reviewEpoch = context.value(QStringLiteral("reviewEpoch")).toString();
    const QVariantList vehicleIds = context.value(QStringLiteral("targetVehicleIds")).toList();
    const auto same = [&]() { return self && !_reentered && _same(generation, state) && _bundle() == bundle && session && roles && store && review && owner
        && catalog && coordinator && _session == session && _roles == roles && _store == store && _review == review && _authorization == owner
        && _catalog == catalog && _coordinator == coordinator && _registry == registry && _faults == faults && (!hadRegistry || registry) && (!hadFaults || faults)
        && _context == context && _identity == identity && _role == role && _approver == approver && _authentication == authentication && _binding == binding
        && _recordBytes == record && _approvalId == approvalId && _approvalEpoch == epoch && _bindingHash == bindingHash && _contextHash == contextHash
        && session->thread() == thread() && roles->thread() == thread() && store->thread() == thread() && review->thread() == thread()
        && owner->thread() == thread() && catalog->thread() == thread() && coordinator->thread() == thread()
        && registry && faults && registry->thread() == thread() && faults->thread() == thread(); };
    const auto native = [&]() {
        if (!same() || !roleMatches(identity, role) || !roles->actionSnapshotCurrent(role)) return false;
        const QVariantMap actualIdentity = session->identitySnapshot();
        if (!same() || !sameMap(actualIdentity, identity)) return false;
        if (requireApprover) {
            const bool fresh = session->approverSnapshotCurrent(approver);
            if (!same() || !fresh || approver.value(QStringLiteral("action")).toString() != Action
                || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash
                || approver.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))) return false;
        }
        if (requireProof) {
            const bool actual = session->groundStartApprovalAuthenticationCurrent(authentication, self.data(), approvalId, epoch, bindingHash, reviewId, reviewEpoch);
            if (!same() || !actual) return false;
        }
        QByteArray current;
        if (!currentRecord(store.data(), binding, &current) || !same() || current != record) return false;
        const bool exact = review->reviewContextCurrent(reviewId, context);
        if (!same() || !exact) return false;
        const QVariantMap actualSources = review->actualSourcesForStartApproval(reviewId);
        if (!same() || actualSources.size() != 9) return false;
        for (auto it = bundle.constBegin(); it != bundle.constEnd(); ++it) if (object(actualSources.value(it.key())) != object(it.value())) return false;
        const QPointer<FleetFirmwareAdapterCatalog> actualCatalog(review->nativeReviewFirmwareCatalog(reviewId));
        if (!same() || actualCatalog != catalog) return false;
        const QPointer<FleetMissionReviewCoordinator> actualCoordinator(review->nativeReviewCoordinator(reviewId));
        if (!same() || actualCoordinator != coordinator) return false;
        const QPointer<FleetRegistry> actualRegistry(review->nativeReviewRegistry(reviewId));
        if (!same() || actualRegistry != registry) return false;
        const QPointer<FaultToleranceManager> actualFaults(review->nativeReviewFaultPolicy(reviewId));
        return same() && actualFaults == faults && _targetsCurrent(generation) && same();
    };
    if (!native()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(owner.data(), "fleetGroundStartApprovalSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!same() || !invoked || !yes(matched) || !native()) return false;
    QVariant allowed;
    const bool gate = QMetaObject::invokeMethod(owner.data(), "fleetGroundStartApprovalContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(approvalId)), Q_ARG(QVariant, QVariant(reviewId)),
        Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(vehicleIds)));
    return same() && gate && yes(allowed) && native() && same();
}
QVariantMap FleetMissionGroundStartApproval::requestApproval(const QVariantMap& suppliedSources, const QString& suppliedReviewId, bool operatorConfirmed)
{
    const QVariantMap sources = suppliedSources; const QString reviewId = suppliedReviewId;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _busy || _checking || _consumeInProgress || !operatorConfirmed || !uuid(reviewId)
        || _state == QStringLiteral("transferred")
        || (_state == QStringLiteral("consumed") && !_expired())) return denied(QStringLiteral("confirmed-idle-new-ground-approval-required"));
    _busy = true; FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionGroundStartApproval> self(this);
    if (_state == QStringLiteral("pending") || _state == QStringLiteral("approved") || _state == QStringLiteral("consumed")) {
        _invalidate(_generation, QStringLiteral("explicitly-replaced-ground-approval"), QStringLiteral("revoked"));
        if (!self) return denied(QStringLiteral("ground-approval-destroyed-at-replacement"));
    }
    const QSet<QString> keys{QStringLiteral("session"), QStringLiteral("roles"), QStringLiteral("store"), QStringLiteral("previousStartup"),
        QStringLiteral("deployment"), QStringLiteral("swarm"), QStringLiteral("health"), QStringLiteral("authorizationOwner"), QStringLiteral("groundReview")};
    if (sources.size() != 9) return denied(QStringLiteral("exact-nine-actual-ground-review-sources-required"));
    for (auto it = sources.constBegin(); it != sources.constEnd(); ++it)
        if (!keys.contains(it.key()) || !object(it.value())) return denied(QStringLiteral("actual-qobject-not-json-required"));
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(object(sources.value(QStringLiteral("session")))));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(object(sources.value(QStringLiteral("roles")))));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(object(sources.value(QStringLiteral("store")))));
    const QPointer<FleetMissionStartupTransaction> previous(qobject_cast<FleetMissionStartupTransaction*>(object(sources.value(QStringLiteral("previousStartup")))));
    const QPointer<FleetMissionDeployment> deployment(qobject_cast<FleetMissionDeployment*>(object(sources.value(QStringLiteral("deployment")))));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(object(sources.value(QStringLiteral("swarm")))));
    const QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(object(sources.value(QStringLiteral("health")))));
    const QPointer<QObject> owner(object(sources.value(QStringLiteral("authorizationOwner"))));
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(object(sources.value(QStringLiteral("groundReview")))));
    const auto alive = [&]() { return self && session && roles && store && previous && deployment && swarm && health && owner && review
        && outboundGroup(self.data(), {session.data(), previous.data(), deployment.data(), review.data()}); };
    if (!alive()) return denied(QStringLiteral("typed-ground-review-sources-required"));
    for (QObject* actual : {static_cast<QObject*>(session.data()), static_cast<QObject*>(roles.data()), static_cast<QObject*>(store.data()),
        static_cast<QObject*>(previous.data()), static_cast<QObject*>(deployment.data()), static_cast<QObject*>(swarm.data()),
        static_cast<QObject*>(health.data()), owner.data(), static_cast<QObject*>(review.data())})
        if (actual->thread() != thread()) return denied(QStringLiteral("same-owner-thread-required"));
    const qint64 beforeContext = _clock.elapsed();
    const QVariantMap context = review->reviewContext(reviewId);
    if (!alive() || !contextShape(context) || context.value(QStringLiteral("reviewId")).toString() != reviewId)
        return denied(QStringLiteral("new-complete-acknowledged-ground-review-required"));
    const QVariantMap actualSources = review->actualSourcesForStartApproval(reviewId);
    if (!alive() || actualSources.size() != 9) return denied(QStringLiteral("actual-ground-review-source-bundle-required"));
    for (const QString& key : keys) if (object(actualSources.value(key)) != object(sources.value(key))) return denied(QStringLiteral("ground-review-source-substitution-denied"));
    const QPointer<FleetFirmwareAdapterCatalog> catalog(review->nativeReviewFirmwareCatalog(reviewId));
    if (!alive() || !catalog) return denied(QStringLiteral("actual-ground-review-catalog-required"));
    const QPointer<FleetMissionReviewCoordinator> coordinator(review->nativeReviewCoordinator(reviewId));
    if (!alive() || !catalog || !coordinator) return denied(QStringLiteral("actual-ground-review-coordinator-required"));
    const QPointer<FleetRegistry> registry(review->nativeReviewRegistry(reviewId));
    if (!alive() || !catalog || !coordinator || !registry) return denied(QStringLiteral("actual-ground-review-registry-required"));
    const QPointer<FaultToleranceManager> faults(review->nativeReviewFaultPolicy(reviewId));
    if (!alive() || !catalog || !coordinator || !registry || !faults) return denied(QStringLiteral("actual-ground-review-fault-policy-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!alive() || !catalog || !coordinator || !registry || !faults) return denied(QStringLiteral("identity-source-destroyed"));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!alive() || !identityCurrent) return denied(QStringLiteral("actual-fresh-primary-operator-required"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start"));
    const QVariantMap artifact = context.value(QStringLiteral("reviewArtifact")).toMap();
    if (!alive() || !roleMatches(identity, role) || !roles->actionSnapshotCurrent(role)
        || !sameMap(identity, artifact.value(QStringLiteral("primaryIdentity")).toMap()))
        return denied(QStringLiteral("same-actual-reviewed-primary-and-start-role-required"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    QByteArray record;
    if (!currentRecord(store.data(), binding, &record) || !alive()) return denied(QStringLiteral("same-latest-full-nonrestored-record-required"));
    const QVariantList vehicleIds = context.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList rows = artifact.value(QStringLiteral("physicalTargets")).toList();
    if (rows.size() != vehicleIds.size()) return denied(QStringLiteral("whole-new-read-risk-artifact-required"));
    QList<Target> targets;
    for (int index = 0; index < vehicleIds.size(); ++index) {
        const QVariantMap row = rows[index].toMap(); double id = 0, component = 0;
        if (!integer(vehicleIds[index], 1, 255, &id) || !integer(row.value(QStringLiteral("vehicleId")), id, id)
            || !integer(row.value(QStringLiteral("componentId")), 1, 255, &component)
            || row.value(QStringLiteral("frozenArmed")).type() != QVariant::Bool) return denied(QStringLiteral("actual-ordered-frozen-armed-phases-required"));
        Target target; target.vehicleId = int(id); target.componentId = int(component); target.armed = row.value(QStringLiteral("frozenArmed")).toBool();
        target.vehicle = review->reviewVehicle(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle) return denied(QStringLiteral("actual-new-reviewed-vehicle-required"));
        target.mission = review->reviewManager(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission) return denied(QStringLiteral("actual-new-reviewed-manager-required"));
        target.primary = review->reviewPrimaryLink(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission || !target.primary) return denied(QStringLiteral("actual-new-reviewed-primary-required"));
        target.parameters = target.vehicle->parameterManager();
        if (!alive() || !target.vehicle || !target.parameters) return denied(QStringLiteral("actual-reviewed-parameter-manager-required"));
        target.links = target.vehicle->vehicleLinkManager();
        if (!alive() || !target.vehicle || !target.links) return denied(QStringLiteral("actual-reviewed-link-manager-required"));
        targets.append(target);
    }
    if (!alive() || !review->reviewContextCurrent(reviewId, context) || !alive() || _generation == std::numeric_limits<quint64>::max())
        return denied(QStringLiteral("new-review-changed-before-approval"));
    _disconnectSources(); ++_generation; const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _previous = previous; _deployment = deployment; _swarm = swarm; _health = health;
    _authorization = owner; _review = review; _catalog = catalog; _coordinator = coordinator; _registry = registry; _faults = faults;
    _context = context; _identity = identity; _role = role; _binding = binding; _recordBytes = record; _targets = targets;
    _contextHash = hash(stable(context)); _approvalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _bindingHash = hash(QVariantMap{{QStringLiteral("action"), Action}, {QStringLiteral("approvalId"), _approvalId}, {QStringLiteral("approvalEpoch"), _approvalEpoch},
        {QStringLiteral("reviewContextHash"), _contextHash}, {QStringLiteral("primary"), identity}, {QStringLiteral("role"), role},
        {QStringLiteral("recordHash"), QString::fromLatin1(QCryptographicHash::hash(record, QCryptographicHash::Sha256).toHex())}});
    _state = QStringLiteral("pending"); _reason.clear(); _approver.clear(); _authentication.clear(); _receipt.clear(); _grant.clear(); _consumeMaximumMs = 0;
    _transferredAuthority.clear(); _transferredStartupId.clear(); _transferredAuthorityEpoch.clear(); _transferredSourceHash.clear();
    _approvedAtMs = -1; _expiresAtMs = beforeContext + qint64(context.value(QStringLiteral("remainingLifetimeMs")).toDouble());
    _connectSources(generation); _timer.start();
    if (!digest(_contextHash) || !digest(_bindingHash) || !_sourcesCurrent(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("ground-approval-before-prompt-source-denied"));
        return denied(QStringLiteral("ground-approval-before-prompt-source-denied"));
    }
    int armedCount = 0; for (const Target& target : targets) if (target.armed) ++armedCount;
    const QString summary = QStringLiteral("NEW ground reassessment intent\nReview %1\nVehicles %2; already armed %3\nNew complete onboard mission read matched. HOME/local reference branches, neutral pilot input, clearance, hardware identity and remote atomic snapshot remain UNVERIFIED. This is a NEW conditional intent only: no old approval restored, no ARM, mode, upload or flight command.")
        .arg(reviewId).arg(targets.size()).arg(armedCount);
    _prompt = true;
    const QVariantMap authenticated = session->authenticateApprover(Action, _bindingHash, identity, summary);
    if (!self) return denied(QStringLiteral("ground-approval-destroyed-during-prompt"));
    _prompt = false;
    if (!_same(generation, QStringLiteral("pending")) || !yes(authenticated.value(QStringLiteral("ok")))
        || authenticated.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) {
        _invalidate(generation, QStringLiteral("new-independent-account-prompt-denied"), QStringLiteral("failed"));
        return denied(QStringLiteral("new-independent-account-prompt-denied"));
    }
    _approver = authenticated.value(QStringLiteral("identityReceipt")).toMap();
    if (!_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("new-review-or-account-changed-during-prompt"));
        return denied(QStringLiteral("new-review-or-account-changed-during-prompt"));
    }
    _approvedAtMs = _clock.elapsed();
    _receipt = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("approvalId"), _approvalId}, {QStringLiteral("approvalEpoch"), _approvalEpoch},
        {QStringLiteral("generation"), QString::number(generation)}, {QStringLiteral("reviewId"), reviewId}, {QStringLiteral("reviewEpoch"), context.value(QStringLiteral("reviewEpoch"))},
        {QStringLiteral("reviewGeneration"), context.value(QStringLiteral("generation"))}, {QStringLiteral("deploymentId"), context.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("previousStartupId"), context.value(QStringLiteral("previousStartupId"))}, {QStringLiteral("binding"), binding},
        {QStringLiteral("targetVehicleIds"), vehicleIds}, {QStringLiteral("bindingHash"), _bindingHash}, {QStringLiteral("reviewContextHash"), _contextHash},
        {QStringLiteral("sourceFingerprint"), context.value(QStringLiteral("sourceFingerprint"))}, {QStringLiteral("approvedAtMonotonicMs"), double(_approvedAtMs)},
        {QStringLiteral("expiresAtMonotonicMs"), double(_expiresAtMs)}, {QStringLiteral("conditionalIntentApproved"), true}, {QStringLiteral("oneUse"), true},
        {QStringLiteral("technicalRisksPreserved"), true}, {QStringLiteral("requiresFreshSourceRevalidation"), true}, {QStringLiteral("roleAction"), QStringLiteral("mission.start")},
        {QStringLiteral("authenticationAction"), Action}, {QStringLiteral("purpose"), Purpose}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("previousAuthorizationRestored"), false}};
    if (!receiptShape(_receipt)) { _invalidate(generation, QStringLiteral("new-approval-receipt-shape-invalid"), QStringLiteral("failed")); return denied(QStringLiteral("new-approval-receipt-shape-invalid")); }
    _state = QStringLiteral("approved"); emit changed();
    if (!self) return denied(QStringLiteral("ground-approval-destroyed-at-publication"));
    if (!_same(generation, QStringLiteral("approved")) || !_sourcesCurrent(generation, true)) {
        if (self) _invalidate(generation, QStringLiteral("ground-approval-revoked-at-publication"));
        return denied(QStringLiteral("ground-approval-revoked-at-publication"));
    }
    const QString approvedId = _approvalId;
    QTimer::singleShot(0, this, [self, generation, approvedId]() {
        if (self && self->_generation == generation && self->_approvalId == approvedId && self->_state == QStringLiteral("approved")
            && !self->_busy && !self->_checking && !self->_prompt && !self->_expired()) emit self->changed();
    });
    return current();
}

bool FleetMissionGroundStartApproval::authenticationConsumeContextCurrent(const QString& suppliedId, const QString& suppliedEpoch,
    const QString& suppliedHash, const QString& suppliedReviewId, const QString& suppliedReviewEpoch,
    const QVariantMap& suppliedPrimary, const QVariantMap& suppliedApprover, int maximumRemainingMs)
{
    // PURE memory/actual-owner context. Session itself performs native SAM/WTS
    // checks; the ledger performs full source gates before and AFTER consume.
    const QString id = suppliedId, epoch = suppliedEpoch, bindingHash = suppliedHash, reviewId = suppliedReviewId, reviewEpoch = suppliedReviewEpoch;
    const QVariantMap primary = suppliedPrimary, approver = suppliedApprover;
    return thread() == QThread::currentThread() && _consumeInProgress && _busy && !_checking && !_prompt
        && _same(_generation, QStringLiteral("approved")) && id == _approvalId && epoch == _approvalEpoch && bindingHash == _bindingHash
        && reviewId == _context.value(QStringLiteral("reviewId")).toString() && reviewEpoch == _context.value(QStringLiteral("reviewEpoch")).toString()
        && maximumRemainingMs >= 1 && maximumRemainingMs <= MaximumMs && maximumRemainingMs == _consumeMaximumMs
        && sameMap(primary, _identity) && sameMap(approver, _approver) && contextShape(_context)
        && hash(stable(_context)) == _contextHash && receiptShape(_receipt);
}
QVariantMap FleetMissionGroundStartApproval::consumeApproval(const QVariantMap& suppliedSources, const QVariantMap& suppliedReceipt, const QString& suppliedConsumerId)
{
    const QVariantMap sources = suppliedSources, receipt = suppliedReceipt; const QString consumerId = suppliedConsumerId;
    if (thread() != QThread::currentThread() || _busy || _checking || _prompt || _consumeInProgress || !uuid(consumerId)
        || !_same(_generation, QStringLiteral("approved")) || !receiptShape(receipt) || !sameMap(receipt, _receipt) || sources != _bundle()) return {};
    _busy = true; FlagGuard consuming{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionGroundStartApproval> self(this);
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QString id = _approvalId, epoch = _approvalEpoch, bindingHash = _bindingHash;
    const QString reviewId = _context.value(QStringLiteral("reviewId")).toString(), reviewEpoch = _context.value(QStringLiteral("reviewEpoch")).toString();
    if (!_sourcesCurrent(generation, true) || !self) return {};
    const qint64 entered = _clock.elapsed();
    _consumeMaximumMs = int(qMin<qint64>(MaximumMs, _expiresAtMs - entered));
    if (_consumeMaximumMs < 1) return {};
    _consumeInProgress = true; FlagGuard nativeConsume{QPointer<QObject>(this), &_consumeInProgress};
    const QVariantMap consumed = session->consumeGroundStartApprovalAuthentication(_identity, _approver, self.data(), id, epoch,
        bindingHash, reviewId, reviewEpoch, _consumeMaximumMs);
    if (!self) return {}; // Session watches destruction of this exact native owner.
    const QVariantMap authentication = consumed.value(QStringLiteral("authenticationReceipt")).toMap();
    double duration = 0;
    const auto revoke = [&]() { if (session && self) session->revokeGroundStartApprovalAuthentication(self.data(), id, epoch); };
    if (!_same(generation, QStringLiteral("approved")) || !yes(consumed.value(QStringLiteral("ok")))
        || consumed.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map || authentication.size() != 22
        || !no(consumed.value(QStringLiteral("startupPermitGranted"))) || !no(consumed.value(QStringLiteral("executable")))
        || !no(consumed.value(QStringLiteral("flightCommandReleased"))) || !integer(authentication.value(QStringLiteral("durationMs")), 1, MaximumMs, &duration)) {
        revoke(); if (self) _invalidate(generation, QStringLiteral("new-ground-authentication-not-consumed"), QStringLiteral("failed"), true); return {};
    }
    _authentication = authentication;
    const qint64 now = _clock.elapsed();
    _expiresAtMs = qMin(_expiresAtMs, entered + qint64(duration) - (now - entered));
    if (!_sourcesCurrent(generation, false, true) || !self) {
        revoke(); if (self) _invalidate(generation, QStringLiteral("new-ground-source-changed-after-native-consume"), QStringLiteral("stale"), true); return {};
    }
    // Commit one-use state before any publication. No synchronous changed
    // during valid consumption or native proof/source revalidation.
    _state = QStringLiteral("consumed");
    _grant = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("grantId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("consumerId"), consumerId}, {QStringLiteral("approvalReceipt"), _receipt}, {QStringLiteral("authenticationReceipt"), authentication},
        {QStringLiteral("reviewContextHash"), _contextHash}, {QStringLiteral("consumedAtMonotonicMs"), double(_clock.elapsed())},
        {QStringLiteral("expiresAtMonotonicMs"), double(_expiresAtMs)}, {QStringLiteral("purpose"), Purpose}, {QStringLiteral("conditionalIntentApproved"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("previousAuthorizationRestored"), false}};
    const QVariantMap grant = _grant;
    const QString grantId = grant.value(QStringLiteral("grantId")).toString();
    QTimer::singleShot(0, this, [self, generation, id, grantId]() {
        if (self && self->_generation == generation && self->_approvalId == id && self->_state == QStringLiteral("consumed")
            && self->_grant.value(QStringLiteral("grantId")).toString() == grantId) emit self->changed();
    });
    return grant;
}
bool FleetMissionGroundStartApproval::grantCurrent(const QVariantMap& suppliedGrant, const QString& suppliedConsumerId)
{
    const QVariantMap grant = suppliedGrant; const QString consumerId = suppliedConsumerId;
    if (thread() != QThread::currentThread() || _busy || _checking || _prompt || _consumeInProgress || !uuid(consumerId)
        || !_same(_generation, QStringLiteral("consumed")) || grant.size() != 14 || !sameMap(grant, _grant)
        || grant.value(QStringLiteral("consumerId")).type() != QVariant::String || grant.value(QStringLiteral("consumerId")).toString() != consumerId
        || !uuid(grant.value(QStringLiteral("grantId"))) || !receiptShape(grant.value(QStringLiteral("approvalReceipt")).toMap())
        || grant.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map || grant.value(QStringLiteral("authenticationReceipt")).toMap().size() != 22
        || !no(grant.value(QStringLiteral("startupPermitGranted"))) || !no(grant.value(QStringLiteral("executable")))
        || !no(grant.value(QStringLiteral("flightCommandReleased"))) || !no(grant.value(QStringLiteral("previousAuthorizationRestored")))) return false;
    _busy = true; FlagGuard validating{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionGroundStartApproval> self(this); const quint64 generation = _generation;
    const bool fresh = _sourcesCurrent(generation, false, true);
    return self && fresh && _same(generation, QStringLiteral("consumed")) && sameMap(grant, _grant)
        && _grant.value(QStringLiteral("consumerId")).toString() == consumerId;
}
QVariantMap FleetMissionGroundStartApproval::transferToGroundStartup(QObject* actualGroundAuthority, const QVariantMap& suppliedGrant,
    const QString& suppliedStartupId, const QString& suppliedSourceHash, qint64 maximumRemainingMs)
{
    const QVariantMap grant = suppliedGrant;
    const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QPointer<FleetMissionGroundStartApproval> self(this);
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(actualGroundAuthority));
    if (thread() != QThread::currentThread() || _busy || _checking || _prompt || _consumeInProgress
        || !authority || authority->thread() != thread() || !outboundGroup(this, {authority.data()})
        || !uuid(startupId) || !digest(sourceHash)
        || maximumRemainingMs < 1 || maximumRemainingMs > MaximumMs || !_same(_generation, QStringLiteral("consumed"))
        || grant.size() != 14 || !sameMap(grant, _grant) || grant.value(QStringLiteral("consumerId")).type() != QVariant::String
        || grant.value(QStringLiteral("consumerId")).toString() != startupId) return denied(QStringLiteral("actual-consumed-ground-grant-and-authority-required"));
    const QString epoch = authority->authorityEpoch();
    if (!uuid(epoch)) return denied(QStringLiteral("actual-ground-startup-authority-epoch-required"));
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QVariantMap primary = _identity, basis = _authentication;
    const QString approvalId = _approvalId;
    // This entry budget includes grant/current/owner/SAM/WTS work. The old
    // shortened ledger deadline, not the public receipt expiry, is final.
    const qint64 entered = _clock.elapsed(), deadline = qMin(_expiresAtMs, entered + maximumRemainingMs);
    if (!grantCurrent(grant, startupId) || !self || !session || !authority)
        return denied(QStringLiteral("ground-grant-not-current-before-handoff"));
    _busy = true; FlagGuard transferring{QPointer<QObject>(this), &_busy};
    const auto same = [&]() {
        return self && session && authority && _same(generation, QStringLiteral("consumed"))
            && outboundGroup(self.data(), {session.data(), authority.data()}) && _session == session && _approvalId == approvalId
            && sameMap(_grant, grant) && _identity == primary && _authentication == basis && authority->thread() == thread()
            && authority->authorityEpoch() == epoch && _clock.elapsed() < deadline;
    };
    if (!same() || !_sourcesCurrent(generation, false, true) || !same()
        || !authority->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), basis) || !same()
        || !_sourcesCurrent(generation, false, true) || !same()) return denied(QStringLiteral("ground-handoff-actual-sources-or-owner-context-denied"));
    const qint64 remainingBefore = deadline - _clock.elapsed();
    if (remainingBefore < 1 || remainingBefore > MaximumMs) return denied(QStringLiteral("original-ground-handoff-budget-expired"));
    const QVariantMap movedResult = session->transferGroundStartApprovalAuthentication(primary, basis, self.data(), authority.data(),
        startupId, epoch, sourceHash, int(remainingBefore));
    const bool moved = yes(movedResult.value(QStringLiteral("ok")));
    const QVariantMap authentication = movedResult.value(QStringLiteral("authenticationReceipt")).toMap();
    const auto revokeMoved = [&]() {
        if (moved && session && authority) session->revokeGroundStartupAuthentication(authority.data(), startupId, epoch);
    };
    double reportedRemaining = 0;
    if (!same() || !moved || movedResult.size() != 6 || movedResult.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map
        || authentication.size() != 28 || !integer(movedResult.value(QStringLiteral("remainingMs")), 1, MaximumMs, &reportedRemaining)
        || !no(movedResult.value(QStringLiteral("startupPermitGranted"))) || !no(movedResult.value(QStringLiteral("executable")))
        || !no(movedResult.value(QStringLiteral("flightCommandReleased")))) {
        revokeMoved();
        if (self && moved) _invalidate(generation, QStringLiteral("ground-authentication-handoff-not-committed"), QStringLiteral("failed"), true);
        return denied(QStringLiteral("ground-authentication-handoff-not-committed"));
    }
    // Actual Session has retired the old ground22. Stop old Review observers
    // BEFORE new-owner checks, so later old-review expiry cannot kill a new
    // independent owner whose unchanged original budget is separately held.
    _state = QStringLiteral("transferred"); _reason.clear(); _timer.stop(); _disconnectSources();
    _authentication.clear(); _grant.clear();
    _transferredAuthority = authority; _transferredStartupId = startupId;
    _transferredAuthorityEpoch = epoch; _transferredSourceHash = sourceHash;
    const auto transferred = [&]() {
        return self && session && authority && outboundGroup(self.data(), {session.data(), authority.data()})
            && _generation == generation && _state == QStringLiteral("transferred") && _approvalId == approvalId
            && _session == session && _transferredAuthority == authority && _transferredStartupId == startupId
            && _transferredAuthorityEpoch == epoch && _transferredSourceHash == sourceHash && _authentication.isEmpty() && _grant.isEmpty()
            && authority->thread() == thread() && authority->authorityEpoch() == epoch && _clock.elapsed() < deadline;
    };
    const bool proofCurrent = session->groundStartupAuthenticationCurrent(authentication, authority.data(), startupId, epoch, sourceHash);
    const bool ownerCurrent = transferred() && proofCurrent
        && authority->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), basis) && transferred();
    const qint64 now = self ? _clock.elapsed() : deadline;
    // Native remaining is measured AFTER its checks in its own clock domain;
    // subtract complete entry overhead conservatively, never reset the clock.
    const qint64 remaining = qMin(deadline - now, qint64(reportedRemaining) - (now - entered));
    if (!ownerCurrent || !transferred() || remaining < 1) {
        revokeMoved();
        if (self && _generation == generation && _state == QStringLiteral("transferred") && _approvalId == approvalId
            && _transferredAuthority == authority && _transferredStartupId == startupId && _transferredAuthorityEpoch == epoch) {
            _state = QStringLiteral("failed"); _reason = QStringLiteral("ground-owner-post-handoff-context-denied");
            QTimer::singleShot(0, this, [self, generation, approvalId]() {
                if (self && self->_generation == generation && self->_approvalId == approvalId && self->_state == QStringLiteral("failed")) emit self->changed();
            });
        }
        return denied(QStringLiteral("ground-owner-post-handoff-context-denied"));
    }
    QTimer::singleShot(0, this, [self, generation, approvalId, startupId, epoch, sourceHash]() {
        if (self && self->_generation == generation && self->_approvalId == approvalId && self->_state == QStringLiteral("transferred")
            && self->_transferredStartupId == startupId && self->_transferredAuthorityEpoch == epoch && self->_transferredSourceHash == sourceHash) emit self->changed();
    });
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), authentication}, {QStringLiteral("startupId"), startupId},
        {QStringLiteral("authorityEpoch"), epoch}, {QStringLiteral("sourceHash"), sourceHash}, {QStringLiteral("remainingMs"), double(remaining)},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("previousAuthorizationRestored"), false}};
}
bool FleetMissionGroundStartApproval::revokeApproval(const QString& suppliedApprovalId)
{
    const QString id = suppliedApprovalId;
    if (thread() != QThread::currentThread() || !uuid(id) || id != _approvalId) return false;
    if (_state == QStringLiteral("transferred")) {
        if (_busy || _checking) return false;
        _busy = true; FlagGuard forwarding{QPointer<QObject>(this), &_busy};
        const QPointer<FleetMissionGroundStartApproval> self(this);
        const QPointer<FleetMissionGroundStartupAuthority> authority(_transferredAuthority);
        const quint64 generation = _generation;
        const QString startupId = _transferredStartupId, epoch = _transferredAuthorityEpoch, sourceHash = _transferredSourceHash;
        const auto same = [&]() {
            return self && _generation == generation && _state == QStringLiteral("transferred") && _approvalId == id
                && _transferredAuthority == authority && _transferredStartupId == startupId && _transferredAuthorityEpoch == epoch
                && _transferredSourceHash == sourceHash && _grant.isEmpty() && _authentication.isEmpty();
        };
        bool retired = !authority;
        if (authority) {
            const auto exact = [&](const QVariantMap& status) {
                return same() && authority && authority->thread() == thread() && authority->authorityEpoch() == epoch
                    && status.value(QStringLiteral("startupId")).type() == QVariant::String && status.value(QStringLiteral("startupId")).toString() == startupId
                    && status.value(QStringLiteral("authorityEpoch")).type() == QVariant::String && status.value(QStringLiteral("authorityEpoch")).toString() == epoch
                    && status.value(QStringLiteral("sourceHash")).type() == QVariant::String && status.value(QStringLiteral("sourceHash")).toString() == sourceHash;
            };
            const auto terminal = [&](const QVariantMap& status) {
                const QString state = status.value(QStringLiteral("state")).toString();
                return exact(status) && status.value(QStringLiteral("state")).type() == QVariant::String
                    && (state == QStringLiteral("revoked") || state == QStringLiteral("expired") || state == QStringLiteral("stale") || state == QStringLiteral("failed"))
                    && no(status.value(QStringLiteral("active"))) && no(status.value(QStringLiteral("reservationHeld"))) && no(status.value(QStringLiteral("transportDraining")));
            };
            const QVariantMap before = authority->current();
            if (!same()) return false;
            if (exact(before)) {
                if (terminal(before)) retired = true;
                else {
                    authority->revokeAuthority(startupId);
                    if (!same()) return false;
                    if (!authority) retired = true;
                    else retired = terminal(authority->current());
                }
            }
        }
        if (!same()) return false;
        if (!retired) { _reason = QStringLiteral("transferred-ground-owner-cancellation-or-drain-required"); emit changed(); return false; }
        _state = QStringLiteral("revoked"); _reason = QStringLiteral("transferred-ground-owner-retired");
        // No old grant can be revived and no replacement owner's proof is touched.
        emit changed(); return true;
    }
    if (_state != QStringLiteral("pending") && _state != QStringLiteral("approved") && _state != QStringLiteral("consumed")) return false;
    _invalidate(_generation, QStringLiteral("explicitly-revoked-new-ground-approval"), QStringLiteral("revoked"));
    return true;
}

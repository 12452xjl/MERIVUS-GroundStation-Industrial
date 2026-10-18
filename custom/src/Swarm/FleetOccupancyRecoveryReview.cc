#include "FleetOccupancyRecoveryReview.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <QThread>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
const QString Action = QStringLiteral("fleet.occupancy.recover");
const QString ApprovalAction = QStringLiteral("fleet.occupancy.recover.approve");
constexpr qint64 ReviewLifetimeMs = 30000;
struct BusyGuard {
    QPointer<QObject> owner;
    bool* busy;
    ~BusyGuard() { if (owner) *busy = false; }
};
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool referenceValid(const QString& value)
{
    if (value.isEmpty() || value != value.trimmed() || value.toUtf8().size() > 256
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar character : value)
        if (character.isNull() || character.category() == QChar::Other_Control
            || character.category() == QChar::Other_Format) return false;
    return true;
}
bool normalizeIds(const QVariantList& input, QVariantList* output)
{
    if (!output || input.isEmpty() || input.size() > 32) return false;
    QList<int> ids;
    QSet<int> seen;
    for (const QVariant& value : input) {
        switch (value.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
        case QVariant::ULongLong: case QVariant::Double: break;
        default: return false;
        }
        const double number = value.toDouble();
        if (!std::isfinite(number) || number < 1.0 || number > 255.0
            || std::floor(number) != number || seen.contains(static_cast<int>(number))) return false;
        seen.insert(static_cast<int>(number)); ids.append(static_cast<int>(number));
    }
    std::sort(ids.begin(), ids.end());
    output->clear();
    for (int id : ids) output->append(id);
    return true;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    return identity.size() == 11 && yes(identity.value(QStringLiteral("authenticated")))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == Action
        && yes(role.value(QStringLiteral("allowed"))) && yes(role.value(QStringLiteral("requiresSecondApproval")))
        && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
} // namespace

FleetOccupancyRecoveryReview::FleetOccupancyRecoveryReview(SwarmController* actualSwarm)
    : QObject(actualSwarm), _swarm(actualSwarm),
      _workflowEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        const QPointer<FleetOccupancyRecoveryReview> self(this);
        refreshResolvedBindings();
        if (!self || (_state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return;
        const quint64 generation = _generation;
        if (_clock.elapsed() >= _deadlineMs) invalidate(generation, QStringLiteral("original-recovery-review-expired"));
        else if (!sourcesCurrent(generation, false, false) && self)
            invalidate(generation, QStringLiteral("recovery-source-changed"));
    });
    _timer.start();
}

FleetOccupancyRecoveryReview::~FleetOccupancyRecoveryReview()
{
    _state = QStringLiteral("retiring"); _busy = true; _consuming = false;
    _timer.stop();
    disconnectReviewSources();
    const auto connections = _resolvedConnections;
    _resolvedConnections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

QVariantMap FleetOccupancyRecoveryReview::current() const
{
    if (thread() != QThread::currentThread()) return {};
    const QPointer<FleetOccupancyRecoveryReview> self(const_cast<FleetOccupancyRecoveryReview*>(this));
    const_cast<FleetOccupancyRecoveryReview*>(this)->refreshResolvedBindings();
    if (!self) return {};
    QVariantList selectedReviewRequired;
    for (const QVariant& id : _vehicleIds)
        if (!_swarm || _swarm->_continuityRestartReviewVehicleIds.contains(id.toInt())
            || _swarm->_continuityRestoredHistoricalReviewVehicleIds.contains(id.toInt())
            || _swarm->_continuityUncertainVehicleIds.contains(id.toInt())) selectedReviewRequired.append(id);
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("reviewGeneration"), QString::number(_generation)},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("dispositionReference"), _dispositionReference},
        {QStringLiteral("context"), _context}, {QStringLiteral("targetIdentities"), _targetIdentities},
        {QStringLiteral("bindingHash"), _bindingHash},
        {QStringLiteral("remainingReviewMs"), qMax<qint64>(0, _deadlineMs - _clock.elapsed())},
        {QStringLiteral("applicantActorId"), _identity.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverActorId"), _approver.value(QStringLiteral("actorId"))},
        {QStringLiteral("groundInspectionDeclared"), _groundInspectionConfirmed},
        {QStringLiteral("localReuseOnlyConfirmed"), _localReuseOnlyConfirmed},
        {QStringLiteral("localHoldsReleased"), _localHoldsReleased},
        {QStringLiteral("releaseFactScope"), QStringLiteral("historical-local-mutation-not-current-availability")},
        {QStringLiteral("selectedReviewRequiredVehicleIds"), selectedReviewRequired},
        {QStringLiteral("controllerManaged"), _controllerManaged},
        {QStringLiteral("authorizationOwnerRetained"), _controllerManaged && _authorization && _swarm
            && _authorization->thread() == thread() && _swarm->parent() == _authorization.data()},
        {QStringLiteral("currentUncertainOutcomesCleared"), false},
        {QStringLiteral("physicalDispositionVerified"), false},
        {QStringLiteral("remoteTaskAbsenceVerified"), false}, {QStringLiteral("remoteControlLeaseGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("recoveryUiIntegrated"), false}};
}

QVariantMap FleetOccupancyRecoveryReview::result(bool ok, const QString& reason) const
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reviewId"), _reviewId},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), reason},
        {QStringLiteral("localHoldsReleased"), _localHoldsReleased},
        {QStringLiteral("controllerManaged"), _controllerManaged},
        {QStringLiteral("authorizationOwnerRetained"), _controllerManaged && _authorization && _swarm
            && _authorization->thread() == thread() && _swarm->parent() == _authorization.data()},
        {QStringLiteral("currentUncertainOutcomesCleared"), false},
        {QStringLiteral("physicalDispositionVerified"), false},
        {QStringLiteral("remoteTaskAbsenceVerified"), false}, {QStringLiteral("remoteControlLeaseGranted"), false},
        {QStringLiteral("persistenceConfirmed"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOccupancyRecoveryReview::memoryCurrent(quint64 generation) const
{
    return thread() == QThread::currentThread() && generation != 0 && _generation == generation
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("review") || _state == QStringLiteral("approving"))
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _swarm
        && _swarm->thread() == thread() && parent() == _swarm.data()
        && _swarm->_continuityRecoveryReview.data() == this && !_swarm->_continuityJournalRetiring
        && _session && _session->thread() == thread() && _roles && _roles->thread() == thread()
        && (_controllerManaged ? (_authorization && _authorization->thread() == thread()
            && _swarm->parent() == _authorization.data()) : _authorization.isNull());
}

bool FleetOccupancyRecoveryReview::authorizationSourcesCurrent(quint64 generation) const
{
    if (!memoryCurrent(generation)) return false;
    if (!_controllerManaged) return true; // The old native entry remains explicitly native-only.
    if (_authorizationChecking || _context.isEmpty() || _reviewId.isEmpty()) return false;
    const QPointer<FleetOccupancyRecoveryReview> self(const_cast<FleetOccupancyRecoveryReview*>(this));
    const QPointer<QObject> authorization(_authorization);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<SwarmController> swarm(_swarm);
    const QString state = _state, reviewId = _reviewId, bindingHash = _bindingHash;
    const QVariantMap context = _context, identity = _identity, role = _role, approver = _approver;
    const QVariantList ids = _vehicleIds, targetIdentities = _targetIdentities;
    _authorizationChecking = true;
    const BusyGuard checkingGuard{self, &_authorizationChecking};
    const auto same = [&]() {
        return self && memoryCurrent(generation) && _controllerManaged
            && authorization && authorization == _authorization && session && session == _session
            && roles && roles == _roles && swarm && swarm == _swarm
            && _state == state && _reviewId == reviewId && _bindingHash == bindingHash
            && _context == context && _identity == identity && _role == role && _approver == approver
            && _vehicleIds == ids && _targetIdentities == targetIdentities
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
            && roles->actionSnapshotCurrent(role) && roleMatches(identity, role);
    };
    if (!same()) return false;
    // The actual owner is frozen before this generation starts. No same-ID
    // lookup, public source replacement, or controller-to-native fallback.
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(authorization.data(), "fleetOccupancyRecoverySourcesMatch",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, matched),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant(context)), Q_ARG(QVariant, QVariant(reviewId)));
    return same() && invoked && matched.type() == QVariant::Bool && !matched.isNull() && matched.toBool()
        && same();
}

bool FleetOccupancyRecoveryReview::dispositionSourcesCurrent() const
{
    const QPointer<FleetOccupancyRecoveryReview> self(const_cast<FleetOccupancyRecoveryReview*>(this));
    const quint64 generation = _generation;
    if (!memoryCurrent(generation) || _targets.size() != _vehicleIds.size()
        || _targets.size() != _targetIdentities.size() || _targets.isEmpty()) return false;
    // Own copies: a native source getter may deliver a callback that retires
    // this workflow while the original Session/Swarm remains alive.
    const auto targets = _targets;
    const QVariantList identities = _targetIdentities;
    for (int i = 0; i < targets.size(); ++i) {
        const QVariantMap evidence = FleetOccupancyRecoveryEvidence::capture(targets.at(i));
        if (!self || !memoryCurrent(generation)
            || FleetOccupancyRecoveryEvidence::stableIdentity(evidence) != identities.at(i).toMap()) return false;
    }
    // This callback is before Swarm's separate callback-free whole-set ground,
    // ownership and consumed-identity tail fences, never inside those fences.
    return self && memoryCurrent(generation) && authorizationSourcesCurrent(generation)
        && self && memoryCurrent(generation);
}

bool FleetOccupancyRecoveryReview::sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover)
{
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<SwarmController> swarm(_swarm);
    if (!memoryCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !roleMatches(_identity, _role)) return false;
    if (!authorizationSourcesCurrent(generation) || !self || !memoryCurrent(generation)) return false;
    if (checkIdentity && (!session->identitySnapshotCurrent(_identity) || !self || !memoryCurrent(generation))) return false;
    if (!self || !session || !roles || !swarm || !memoryCurrent(generation)
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !authorizationSourcesCurrent(generation)
        || !self || !memoryCurrent(generation)) return false;
    const QVariantMap context = swarm->_continuityRecoveryContext(_vehicleIds);
    if (!self || !memoryCurrent(generation) || context.isEmpty() || context != _context
        || !dispositionSourcesCurrent() || !self || !memoryCurrent(generation)) return false;
    if (requireApprover && (!session->approverSnapshotCurrent(_approver) || !self || !memoryCurrent(generation))) return false;
    return self && memoryCurrent(generation) && session && roles && swarm
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        && roles->actionSnapshotCurrent(_role) && authorizationSourcesCurrent(generation)
        && self && memoryCurrent(generation);
}

bool FleetOccupancyRecoveryReview::dispositionFinalSourcesCurrent() const
{
    if (!memoryCurrent(_generation) || _targets.isEmpty() || _targets.size() != _targetIdentities.size()) return false;
    // No source virtual, OS gate or outward callback in this entire loop.
    // Later targets cannot silently invalidate an earlier target's ground row.
    for (int i = 0; i < _targets.size(); ++i)
        if (!FleetOccupancyRecoveryEvidence::finalCurrent(_targets.at(i), _targetIdentities.at(i).toMap())) return false;
    return memoryCurrent(_generation);
}

QVariantMap FleetOccupancyRecoveryReview::requestReview(QObject* actualSession, QObject* actualRoles,
    const QVariantList& selectedVehicleIds, const QString& dispositionReference, bool operatorConfirmed)
{
    return requestReviewImpl(actualSession, actualRoles, nullptr, selectedVehicleIds,
        dispositionReference, operatorConfirmed, false);
}

QVariantMap FleetOccupancyRecoveryReview::requestReviewForController(QObject* actualSession, QObject* actualRoles,
    QObject* actualAuthorizationOwner, const QVariantList& selectedVehicleIds,
    const QString& dispositionReference, bool operatorConfirmed)
{
    return requestReviewImpl(actualSession, actualRoles, actualAuthorizationOwner, selectedVehicleIds,
        dispositionReference, operatorConfirmed, true);
}

QVariantMap FleetOccupancyRecoveryReview::requestReviewImpl(QObject* actualSession, QObject* actualRoles,
    QObject* actualAuthorizationOwner, const QVariantList& selectedVehicleIds,
    const QString& dispositionReference, bool operatorConfirmed, bool controllerManaged)
{
    const QVariantList requested = selectedVehicleIds;
    const QString reference = dispositionReference;
    const QPointer<QObject> requestedSession(actualSession), requestedRoles(actualRoles);
    const QPointer<QObject> requestedAuthorization(actualAuthorizationOwner);
    const QPointer<SwarmController> requestedSwarm(_swarm);
    QVariantList ids;
    if (thread() != QThread::currentThread() || _busy || _authorizationChecking || _state == QStringLiteral("review")
        || _state == QStringLiteral("approving") || FleetOperatorSession::authenticationPromptInProgress()
        || !operatorConfirmed || !normalizeIds(requested, &ids) || !referenceValid(reference)
        || _generation == std::numeric_limits<quint64>::max()) return result(false, QStringLiteral("recovery-request-invalid-or-busy"));
    if (!requestedSwarm || requestedSwarm->thread() != thread() || parent() != requestedSwarm.data()
        || (controllerManaged ? (!requestedAuthorization || requestedAuthorization->thread() != thread()
            || requestedSwarm->parent() != requestedAuthorization.data()) : !requestedAuthorization.isNull()))
        return result(false, QStringLiteral("actual-recovery-authorization-owner-invalid"));
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    _busy = true; const BusyGuard guard{self, &_busy};
    disconnectReviewSources();
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
    _session = qobject_cast<FleetOperatorSession*>(requestedSession.data());
    _roles = qobject_cast<FleetRolePolicy*>(requestedRoles.data());
    _authorization = requestedAuthorization;
    _controllerManaged = controllerManaged;
    _vehicleIds = ids; _targets.clear(); _targetIdentities.clear(); _context.clear();
    _identity.clear(); _role.clear(); _approver.clear(); _bindingHash.clear();
    _groundInspectionConfirmed = false; _localReuseOnlyConfirmed = false; _localHoldsReleased = false;
    _authenticationConsumed = false; _consuming = false; _prompt = false;
    _dispositionReference = reference;
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const quint64 generation = ++_generation;
    _deadlineMs = _clock.elapsed() + ReviewLifetimeMs;
    _state = QStringLiteral("preparing"); _reason.clear();
    const auto deny = [&](const QString& reason) {
        if (!self) return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
        _state = QStringLiteral("stale"); _reason = reason;
        const QVariantMap response = result(false, reason);
        emit changed(); return response;
    };
    if (!memoryCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data()))
        return deny(QStringLiteral("actual-recovery-sources-invalid"));
    const QPointer<FleetOperatorSession> session(_session);
    const QVariantMap identity = session->identitySnapshot();
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
    if (!memoryCurrent(generation) || !session || session != _session) return deny(QStringLiteral("recovery-account-changed"));
    _identity = identity;
    _role = _roles->actionSnapshot(Action);
    if (!roleMatches(_identity, _role) || !session->identitySnapshotCurrent(_identity))
        return deny(QStringLiteral("recovery-account-or-role-unavailable"));
    if (!self || !memoryCurrent(generation)) return deny(QStringLiteral("recovery-account-changed"));
    const QVariantMap context = _swarm->_continuityRecoveryContext(ids);
    if (!self || !memoryCurrent(generation) || context.isEmpty()) return deny(QStringLiteral("recovery-occupancy-not-reviewable"));
    _context = context;
    if (!authorizationSourcesCurrent(generation) || !self || !memoryCurrent(generation))
        return deny(QStringLiteral("recovery-controller-context-rejected"));
    const auto vehicles = _swarm->_selectedVehicles(ids);
    if (vehicles.size() != ids.size()) return deny(QStringLiteral("recovery-vehicle-source-missing-or-ambiguous"));
    QList<QPointer<Vehicle>> originalVehicles;
    for (Vehicle* vehicle : vehicles) originalVehicles.append(QPointer<Vehicle>(vehicle));
    for (const QVariant& id : ids) {
        QPointer<Vehicle> selected;
        for (const QPointer<Vehicle>& vehicle : originalVehicles) if (vehicle && vehicle->id() == id.toInt()) {
            if (selected) return deny(QStringLiteral("recovery-vehicle-source-ambiguous"));
            selected = vehicle;
        }
        const auto target = FleetOccupancyRecoveryEvidence::bind(selected.data());
        const QVariantMap evidence = FleetOccupancyRecoveryEvidence::capture(target);
        if (!self || !memoryCurrent(generation)) return deny(QStringLiteral("recovery-source-changed"));
        const QVariantMap sourceIdentity = FleetOccupancyRecoveryEvidence::stableIdentity(evidence);
        if (sourceIdentity.isEmpty()) return deny(QStringLiteral("recovery-grounded-evidence-unavailable"));
        _targets.append(target); _targetIdentities.append(sourceIdentity);
    }
    QVariantMap binding{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("generation"), QString::number(generation)},
        {QStringLiteral("action"), ApprovalAction}, {QStringLiteral("context"), _context},
        {QStringLiteral("targets"), _targetIdentities}, {QStringLiteral("dispositionReference"), reference},
        {QStringLiteral("primaryIdentity"), _identity}, {QStringLiteral("role"), _role}};
    if (_controllerManaged) binding.insert(QStringLiteral("controllerManaged"), true);
    _bindingHash = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(binding).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    _state = QStringLiteral("review");
    watchReviewSources(generation);
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
    if (!sourcesCurrent(generation, true, false) || !self) return deny(QStringLiteral("recovery-review-source-changed"));
    const QVariantMap response = result(true, QString());
    emit changed(); return response;
}

bool FleetOccupancyRecoveryReview::authenticationBindingCurrent(FleetOperatorSession* actualSession,
    const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const
{
    // Session calls this around OS validation and after burning its pending
    // identity. Only owned frozen memory is read here: no OS/source getters.
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && !_authenticationConsumed
        && actualSession == _session.data() && primary == _identity && approver == _approver
        && !_bindingHash.isEmpty() && bindingHash == _bindingHash
        && _groundInspectionConfirmed && _localReuseOnlyConfirmed
        && approver.value(QStringLiteral("action")).toString() == ApprovalAction
        && approver.value(QStringLiteral("bindingHash")).toString() == _bindingHash;
}

bool FleetOccupancyRecoveryReview::mutationBindingCurrent(SwarmController* actualSwarm, const QVariantMap& context) const
{
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && _authenticationConsumed
        && actualSwarm == _swarm.data() && context == _context && _groundInspectionConfirmed && _localReuseOnlyConfirmed
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role)
        && _session->occupancyRecoveryConsumedIdentityCurrent(_identity, _approver);
}

QVariantMap FleetOccupancyRecoveryReview::approveAndResolve(const QString& requestedReviewId,
    bool groundInspectionConfirmed, bool localReuseOnlyConfirmed)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _busy || _state != QStringLiteral("review")
        || reviewId.isEmpty() || reviewId != _reviewId || !groundInspectionConfirmed || !localReuseOnlyConfirmed
        || FleetOperatorSession::authenticationPromptInProgress()) return result(false, QStringLiteral("recovery-confirmation-invalid-or-busy"));
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    _busy = true; const BusyGuard guard{self, &_busy};
    const quint64 generation = _generation;
    const auto deny = [&](const QString& reason) {
        if (!self) return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
        _state = QStringLiteral("stale"); _reason = reason; _prompt = false; _consuming = false;
        disconnectReviewSources();
        if (!self) return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
        const QVariantMap response = result(false, reason);
        emit changed(); return response;
    };
    if (!sourcesCurrent(generation, true, false) || !self) return deny(QStringLiteral("recovery-review-no-longer-current"));
    _groundInspectionConfirmed = groundInspectionConfirmed; _localReuseOnlyConfirmed = localReuseOnlyConfirmed;
    _state = QStringLiteral("approving"); _prompt = true;
    const QPointer<FleetOperatorSession> session(_session);
    const QString summary = QStringLiteral("IDs:%1; disposition:%2. Ground inspection DECLARED by applicant, not attested. "
        "LOCAL restart/history holds only; current unknown/owners untouched. NO remote task/lease/flight permission.")
        .arg(QString::fromUtf8(QJsonDocument::fromVariant(_vehicleIds).toJson(QJsonDocument::Compact)), _dispositionReference);
    const QVariantMap approval = session->authenticateApprover(ApprovalAction, _bindingHash, _identity, summary);
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("recovery-owner-destroyed")}};
    _prompt = false;
    if (!memoryCurrent(generation) || !session || session != _session || !yes(approval.value(QStringLiteral("ok")))
        || approval.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) return deny(QStringLiteral("independent-recovery-approval-unavailable"));
    _approver = approval.value(QStringLiteral("identityReceipt")).toMap();
    if (!sourcesCurrent(generation, true, true) || !self) return deny(QStringLiteral("recovery-post-approval-source-changed"));
    _consuming = true;
    if (!session->consumeOccupancyRecoveryAuthentication(_identity, _approver, this, _bindingHash) || !self)
        return deny(QStringLiteral("recovery-authentication-consumption-failed"));
    if (!memoryCurrent(generation)) return deny(QStringLiteral("recovery-owner-changed-after-consumption"));
    _authenticationConsumed = true;
    const QPointer<SwarmController> swarm(_swarm);
    if (!sourcesCurrent(generation, true, false) || !self || !swarm
        || !swarm->_resolveContinuityRecovery(this, _context, _vehicleIds) || !self)
        return deny(QStringLiteral("recovery-final-local-disposition-rejected"));
    _localHoldsReleased = true; _consuming = false;
    _state = QStringLiteral("resolved-awaiting-current-journal"); _reason.clear();
    const QVariantMap response = result(true, QStringLiteral("local-holds-retired-new-native-snapshot-must-commit"));
    disconnectReviewSources();
    if (!self) return response;
    retainResolvedTargets();
    if (!self) return response; // Preserve the already committed local fact.
    // No optimistic mark-as-committed. Swarm's existing getter/CAS remains
    // closed until the actual original worker commits its NEW current identity.
    emit changed(); return response;
}

void FleetOccupancyRecoveryReview::disconnectReviewSources()
{
    const auto connections = _reviewConnections;
    _reviewConnections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

void FleetOccupancyRecoveryReview::watchReviewSources(quint64 generation)
{
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    const auto stale = [this, self, generation]() {
        if (self) invalidate(generation, QStringLiteral("recovery-original-source-mutated"));
    };
    const auto add = [this, self, generation](auto* sender, auto signal, auto handler) {
        if (!sender) return false;
        const QMetaObject::Connection connection = QObject::connect(sender, signal, this, handler);
        if (!self || !memoryCurrent(generation)) { QObject::disconnect(connection); return false; }
        _reviewConnections.append(connection); return true;
    };
    if (!add(_session.data(), &QObject::destroyed, stale)) return;
    if (!add(_session.data(), &FleetOperatorSession::changed, [this, self, generation]() {
        if (self && !_prompt) invalidate(generation, QStringLiteral("recovery-account-mutated"));
    })) return;
    if (!add(_roles.data(), &QObject::destroyed, stale)
        || !add(_roles.data(), &FleetRolePolicy::policyChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentUserChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentRoleChanged, stale)) return;
    if (_controllerManaged && !add(_authorization.data(), &QObject::destroyed, stale)) return;
    const auto targets = _targets;
    for (const auto& target : targets) {
        if (!add(target.vehicle.data(), &QObject::destroyed, stale)
            || !add(target.vehicle.data(), &Vehicle::armedChanged, stale)
            || !add(target.vehicle.data(), &Vehicle::flyingChanged, stale)
            || !add(target.manager.data(), &QObject::destroyed, stale)
            || !add(target.manager.data(), &PlanManager::operationStarted, stale)
            || !add(target.parameters.data(), &QObject::destroyed, stale)
            || !add(target.parameters.data(), &ParameterManager::pendingWritesChanged, stale)
            || !add(target.parameters.data(), &ParameterManager::parametersReadyChanged, stale)
            || !add(target.links.data(), &QObject::destroyed, stale)
            || !add(target.links.data(), &VehicleLinkManager::primaryLinkChanged, stale)
            || !add(target.links.data(), &VehicleLinkManager::communicationLostChanged, stale)
            || !add(target.vehicles.data(), &QObject::destroyed, stale)
            || !add(target.model.data(), &QObject::destroyed, stale)) return;
    }
}

void FleetOccupancyRecoveryReview::invalidate(quint64 generation, const QString& reason)
{
    if (thread() != QThread::currentThread() || generation != _generation
        || (_state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return;
    _state = QStringLiteral("stale"); _reason = reason;
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    disconnectReviewSources();
    if (self) emit changed(); // Last operation; observers may destroy us.
}

bool FleetOccupancyRecoveryReview::cancelReview(const QString& requestedReviewId)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || reviewId.isEmpty() || reviewId != _reviewId
        || (_state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return false;
    _state = QStringLiteral("cancelled"); _reason = QStringLiteral("operator-cancelled-recovery-review");
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    disconnectReviewSources();
    if (self) emit changed(); return true;
}

void FleetOccupancyRecoveryReview::retainResolvedTargets()
{
    for (int i = 0; i < _targets.size(); ++i) {
        const auto& target = _targets.at(i);
        for (int old = _resolvedTargets.size() - 1; old >= 0; --old)
            if (_resolvedTargets.at(old).source.vehicleId == target.vehicleId) _resolvedTargets.removeAt(old);
        _resolvedTargets.append(ResolvedTarget{target, _targetIdentities.at(i).toMap()});
    }
    const auto oldConnections = _resolvedConnections;
    _resolvedConnections.clear();
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    for (const auto& connection : oldConnections) { QObject::disconnect(connection); if (!self) return; }
    const auto recheck = [this, self]() { if (self) refreshResolvedBindings(); };
    const auto add = [this, self, recheck](auto* sender, auto signal) {
        if (!sender) return true; // Final getter will rehold a lost source.
        const QMetaObject::Connection connection = QObject::connect(sender, signal, this, recheck);
        if (!self) { QObject::disconnect(connection); return false; }
        _resolvedConnections.append(connection); return true;
    };
    const auto retained = _resolvedTargets;
    for (const auto& target : retained) {
        if (!add(target.source.vehicle.data(), &QObject::destroyed)
            || !add(target.source.manager.data(), &QObject::destroyed)
            || !add(target.source.parameters.data(), &QObject::destroyed)
            || !add(target.source.links.data(), &QObject::destroyed)
            || !add(target.source.links.data(), &VehicleLinkManager::primaryLinkChanged)
            || !add(target.source.links.data(), &VehicleLinkManager::communicationLostChanged)
            || !add(target.source.vehicles.data(), &QObject::destroyed)
            || !add(target.source.model.data(), &QObject::destroyed)) return;
    }
}

void FleetOccupancyRecoveryReview::refreshResolvedBindings()
{
    if (thread() != QThread::currentThread() || !_swarm || _swarm->_continuityJournalRetiring) return;
    const QPointer<FleetOccupancyRecoveryReview> self(this);
    const QPointer<SwarmController> swarm(_swarm);
    // Owned copy and pure native source predicates: normal RAW updates and
    // subsequent flight do not become permanent source-replacement holds.
    const auto retained = _resolvedTargets;
    for (const auto& target : retained) {
        const bool current = FleetOccupancyRecoveryEvidence::bindingCurrent(target.source, target.identity);
        if (!self || !swarm) return;
        if (current) continue;
        bool removed = false;
        for (int i = _resolvedTargets.size() - 1; i >= 0; --i)
            if (_resolvedTargets.at(i).source.vehicleId == target.source.vehicleId
                && _resolvedTargets.at(i).identity == target.identity) { _resolvedTargets.removeAt(i); removed = true; }
        if (removed) swarm->_reinstateContinuityRecoveryHold(target.source.vehicleId);
    }
    // No outward notify or I/O: final native occupancy/CAS calls this directly,
    // not only the timer, closing the pre-next-tick same-ID replacement window.
}

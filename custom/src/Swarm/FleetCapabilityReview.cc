#include "FleetCapabilityReview.h"

#include "FleetCapabilityEvidence.h"
#include "FleetConfigurationDiff.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QThread>
#include <QUuid>

#include <limits>
#include <cmath>

namespace {
bool baselineOperation(const QString& operation)
{
    return operation == QStringLiteral("approve-baseline") || operation == QStringLiteral("revoke-baseline");
}
QString policyAction(const QString& operation)
{
    if (baselineOperation(operation)) return QStringLiteral("fleet.configuration.review");
    if (operation == QStringLiteral("approve") || operation == QStringLiteral("revoke")
        || operation == QStringLiteral("approve-performance"))
        return QStringLiteral("fleet.capability.review");
    return {};
}
QString approvalPurpose(const QString& operation)
{
    if (baselineOperation(operation)) return QStringLiteral("fleet.configuration.review.approve");
    if (operation == QStringLiteral("approve-performance")) return QStringLiteral("fleet.performance.declaration.approve");
    if (operation == QStringLiteral("approve") || operation == QStringLiteral("revoke"))
        return QStringLiteral("fleet.capability.review.approve");
    return {};
}
constexpr qint64 LifetimeMs = 30000;
struct BusyGuard {
    QPointer<FleetCapabilityReview> owner;
    bool* busy;
    bool notifyOnExit = false;
    ~BusyGuard() {
        if (!owner) return;
        *busy = false;
        if (notifyOnExit) QMetaObject::invokeMethod(owner.data(), "changed", Qt::QueuedConnection);
    }
};
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool text(const QString& value, int maximum)
{
    if (value.isEmpty() || value.size() > maximum || value != value.trimmed()
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar ch : value) if (ch.isNull() || ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format) return false;
    return true;
}
bool digest(const QString& value)
{
    if (value.size() != 64) return false;
    for (QChar ch : value) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool inputsValid(const QString& operation, const QStringList& capabilities, const QString& reason)
{
    if (policyAction(operation).isEmpty()
        || !text(reason, 1024) || capabilities.size() > 128) return false;
    for (const QString& value : capabilities) if (!text(value, 128)) return false;
    QStringList canonical;
    return FleetCapabilityEvidence::canonicalCapabilities(capabilities, &canonical) && canonical == capabilities
        && ((operation == QStringLiteral("approve") || operation == QStringLiteral("approve-performance"))
            ? !capabilities.isEmpty() : capabilities.isEmpty());
}
QString ownedText(const QString& value) { return QString(value.constData(), value.size()); }
QVariantMap ownedContext(const QVariantMap& value)
{
    // Only the validated, fixed context/baseline DTOs use this copy. No
    // arbitrary configuration is traversed or converted through JSON.
    QVariantMap result;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) {
        const QVariant original = it.value();
        result.insert(ownedText(it.key()), original.type() == QVariant::String
            ? QVariant(ownedText(original.toString())) : original.type() == QVariant::Map
                ? QVariant(ownedContext(original.toMap())) : original);
    }
    return result;
}
bool performanceProposal(const QVariantMap& value, QVariantMap* result)
{
    if (!result) return false;
    QVariantMap canonical;
    if (value.size() != 2 || !value.contains(QStringLiteral("maxSpeedMps"))
        || !value.contains(QStringLiteral("enduranceSeconds"))) { result->clear(); return false; }
    bool known = false;
    for (const QString& key : {QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds")}) {
        const QVariant source = value.value(key);
        if (source.isNull() || (source.type() != QVariant::Int && source.type() != QVariant::UInt
            && source.type() != QVariant::LongLong && source.type() != QVariant::ULongLong
            && source.type() != QVariant::Double)) { result->clear(); return false; }
        bool ok = false;
        const double number = source.toDouble(&ok);
        if (!ok || !std::isfinite(number) || std::abs(number) > 9007199254740991.0
            || (number < 0.0 && number != -1.0)
            || (key == QStringLiteral("enduranceSeconds") && number == 0.0)) { result->clear(); return false; }
        canonical.insert(key, number);
        known = known || number >= 0.0;
    }
    result->clear();
    if (!known) return false;
    *result = canonical;
    return true;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role, const QString& operation)
{
    const QString applicantRole = identity.value(QStringLiteral("role")).toString();
    return identity.size() == 11 && yes(identity.value(QStringLiteral("authenticated")))
        && (applicantRole == QStringLiteral("maintenance") || applicantRole == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && !policyAction(operation).isEmpty() && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == policyAction(operation)
        && yes(role.value(QStringLiteral("allowed"))) && yes(role.value(QStringLiteral("requiresSecondApproval")))
        && role.value(QStringLiteral("actorAuthenticated")).type() == QVariant::Bool
        && !role.value(QStringLiteral("actorAuthenticated")).isNull() && !role.value(QStringLiteral("actorAuthenticated")).toBool()
        && role.value(QStringLiteral("executionPermitted")).type() == QVariant::Bool
        && !role.value(QStringLiteral("executionPermitted")).isNull() && !role.value(QStringLiteral("executionPermitted")).toBool();
}
bool localBaselineApproval(const QVariantMap& approval, const QVariantMap& snapshot, int vehicleId)
{
    if (approval.size() < 2 || approval.size() > 5 || !FleetConfigurationDiff::valueValid(approval)
        || snapshot.value(QStringLiteral("baselineSource")).type() != QVariant::String
        || snapshot.value(QStringLiteral("baselineSource")).toString() != QStringLiteral("local_approval")
        || approval.value(QStringLiteral("source")).type() != QVariant::String
        || approval.value(QStringLiteral("source")).toString().trimmed() != QStringLiteral("local_approval")
        || approval.value(QStringLiteral("fingerprint")).type() != QVariant::String) return false;
    const QString fingerprint = approval.value(QStringLiteral("fingerprint")).toString().trimmed().toLower();
    if (!digest(fingerprint) || fingerprint != snapshot.value(QStringLiteral("baselineConfigurationFingerprint")).toString()) return false;
    for (auto it = approval.constBegin(); it != approval.constEnd(); ++it)
        if (it.key() != QStringLiteral("systemId") && it.key() != QStringLiteral("fingerprint")
            && it.key() != QStringLiteral("source") && it.key() != QStringLiteral("reason")
            && it.key() != QStringLiteral("approvedAtUtc")) return false;
    if (approval.contains(QStringLiteral("systemId"))) {
        const QVariant id = approval.value(QStringLiteral("systemId"));
        if (id.isNull() || !(id.type() == QVariant::Int || id.type() == QVariant::UInt
            || id.type() == QVariant::LongLong || id.type() == QVariant::ULongLong || id.type() == QVariant::Double)) return false;
        const double number = id.toDouble();
        if (!std::isfinite(number) || number != vehicleId) return false;
    }
    for (const QString& key : {QStringLiteral("reason"), QStringLiteral("approvedAtUtc")})
        if (approval.contains(key) && approval.value(key).type() != QVariant::String) return false;
    return true; // Missing optional imported fields remain missing, not synthesized.
}
QVariantMap descriptorForReview(const QVariantMap& actualSnapshot, const QVariantMap& comparison)
{
    // Exclude health/heartbeat/age/position and arbitrary metadata. This is a
    // selected historical display basis, not independent source provenance.
    QVariantMap descriptor;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleType"), QStringLiteral("autopilotType"),
        QStringLiteral("firmwareVersion"), QStringLiteral("configurationFingerprint"),
        QStringLiteral("baselineConfigurationFingerprint"), QStringLiteral("baselineSource"), QStringLiteral("configurationState"),
        QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"), QStringLiteral("canUseGnss"),
        QStringLiteral("canUseRtk"), QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"), QStringLiteral("canRelay"),
        QStringLiteral("configurationEvidenceSources"), QStringLiteral("discoveryState"), QStringLiteral("discoveryErrors")})
        if (actualSnapshot.contains(key)) descriptor.insert(key, actualSnapshot.value(key));
    const QVariantMap configuration = actualSnapshot.value(QStringLiteral("configuration")).toMap();
    const bool fullKnown = yes(comparison.value(QStringLiteral("currentFullKnown")));
    // A partial/hash-only observation must not present a previously merged
    // map as a newly observed complete configuration.
    descriptor.insert(QStringLiteral("configurationJson"), fullKnown ? QString::fromUtf8(
        QJsonDocument::fromVariant(configuration).toJson(QJsonDocument::Indented)) : QString());
    descriptor.insert(QStringLiteral("configurationRepresentation"), QStringLiteral("qt-json-display-not-typed-export"));
    descriptor.insert(QStringLiteral("configurationKnown"), fullKnown);
    const QVariantMap metadata = actualSnapshot.value(QStringLiteral("metadata")).toMap();
    QVariantMap uid;
    for (const QString& key : {QStringLiteral("vehicleUID"), QStringLiteral("uidDecimal"), QStringLiteral("uid2Hex"),
        QStringLiteral("uidKnown"), QStringLiteral("uid2Known")}) {
        if (actualSnapshot.contains(key)) uid.insert(key, actualSnapshot.value(key));
        else if (metadata.contains(key)) uid.insert(key, metadata.value(key));
    }
    descriptor.insert(QStringLiteral("providedUidEvidence"), uid);
    return descriptor;
}
} // namespace

FleetCapabilityReview::FleetCapabilityReview(SwarmController* actualSwarm, FleetRegistry* actualRegistry)
    : QObject(actualSwarm), _swarm(actualSwarm), _registry(actualRegistry),
      _workflowEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (_state != QStringLiteral("review") && _state != QStringLiteral("approving")) return;
        const QPointer<FleetCapabilityReview> self(this);
        const quint64 generation = _generation;
        if (_clock.elapsed() >= _deadlineMs) invalidate(generation, QStringLiteral("original-capability-review-expired"));
        else if (!sourcesCurrent(generation, false, false) && self)
            invalidate(generation, QStringLiteral("capability-review-original-source-changed"));
    });
    _timer.start();
}

FleetCapabilityReview::~FleetCapabilityReview()
{
    _state = QStringLiteral("retiring"); _busy = true; _consuming = false;
    _timer.stop(); disconnectSources();
}

QVariantMap FleetCapabilityReview::current() const
{
    if (thread() != QThread::currentThread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("reviewGeneration"), QString::number(_generation)},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason},
        {QStringLiteral("scopeToken"), _scopeToken}, {QStringLiteral("bindingHash"), _bindingHash},
        {QStringLiteral("remainingReviewMs"), qMax<qint64>(0, _deadlineMs - _clock.elapsed())},
        {QStringLiteral("busy"), _busy}, {QStringLiteral("promptInProgress"), _prompt},
        {QStringLiteral("vehicleId"), _vehicleId}, {QStringLiteral("operation"), _operation},
        {QStringLiteral("capabilities"), _capabilities}, {QStringLiteral("engineeringReason"), _engineeringReason},
        {QStringLiteral("registryContext"), _registryContext}, {QStringLiteral("preview"), _preview},
        {QStringLiteral("applicantActorId"), _identity.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("approverActorId"), _approver.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("operationConfirmed"), _operationConfirmed}, {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("mutationCommitted"), _mutationCommitted}, {QStringLiteral("commitReceipt"), _commitReceipt},
        {QStringLiteral("authorizationOwnerRetained"), _root && _swarm && _swarm->parent() == _root.data()},
        {QStringLiteral("source"), QStringLiteral("local-engineering-attestation")}, {QStringLiteral("reviewUiIntegrated"), false},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("executionPermitted"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetCapabilityReview::result(bool ok, const QString& reason) const
{
    const QVariantMap approvalEvidence{{QStringLiteral("source"), QStringLiteral("local-account-review")},
        {QStringLiteral("applicantActorId"), _identity.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("applicantRole"), _identity.value(QStringLiteral("role"), QString())},
        {QStringLiteral("approverActorId"), _approver.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("approverRole"), _approver.value(QStringLiteral("role"), QString())},
        {QStringLiteral("action"), authenticationPurpose()}, {QStringLiteral("bindingHash"), _bindingHash},
        {QStringLiteral("authenticationConsumed"), _authenticationConsumed}};
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reviewId"), _reviewId},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), reason},
        {QStringLiteral("vehicleId"), _vehicleId}, {QStringLiteral("operation"), _operation},
        {QStringLiteral("operationConfirmed"), _operationConfirmed}, {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("mutationCommitted"), _mutationCommitted}, {QStringLiteral("commitReceipt"), _commitReceipt},
        {QStringLiteral("approvalEvidence"), approvalEvidence},
        {QStringLiteral("reviewUiIntegrated"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("executionPermitted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetCapabilityReview::ownersCurrent(quint64 generation) const
{
    return thread() == QThread::currentThread() && generation != 0 && _generation == generation
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("review") || _state == QStringLiteral("approving"))
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _swarm && _registry && _root && _session && _roles
        && _swarm->thread() == thread() && _registry->thread() == thread() && _root->thread() == thread()
        && _session->thread() == thread() && _roles->thread() == thread()
        && parent() == _swarm.data() && _registry->parent() == _swarm.data() && _swarm->parent() == _root.data()
        && _swarm->ownsCapabilityReview(const_cast<FleetCapabilityReview*>(this), _registry.data());
}

bool FleetCapabilityReview::recordCurrent() const
{
    if (!_registry || _registry->thread() != thread() || _vehicleId < 1 || _vehicleId > 255
        || !digest(_recordIdentityFingerprint)
        || (!FleetCapabilityEvidence::contextValid(_registryContext)
            && !(_operation == QStringLiteral("revoke-baseline") && _registryContext.isEmpty()))) return false;
    // These getters only copy this actual Registry's owned memory. Do not
    // compare the whole telemetry snapshot, whose health/age changes normally.
    const QVariantMap context = _registry->capabilityReviewContext(_vehicleId);
    const QVariantMap snapshot = _registry->vehicle(_vehicleId);
    const QVariantMap approval = snapshot.value(QStringLiteral("capabilityApproval")).toMap();
    const QVariantMap comparison = _registry->configurationComparison(_vehicleId);
    const bool originalRecordCurrent = context == _registryContext && snapshot.value(QStringLiteral("systemId")).type() == QVariant::Int
        && snapshot.value(QStringLiteral("systemId")).toInt() == _vehicleId
        && FleetCapabilityEvidence::identityFingerprint(snapshot) == _recordIdentityFingerprint
        && (_registryContext.isEmpty()
            || _recordIdentityFingerprint == _registryContext.value(QStringLiteral("identityFingerprint")).toString())
        && FleetCapabilityEvidence::stableReviewIdentity(snapshot) == _previousReviewIdentity && approval == _previousApproval
        && snapshot.value(QStringLiteral("configuration")).type() == QVariant::Map
        && FleetConfigurationDiff::exactEqual(snapshot.value(QStringLiteral("configuration")), _configuration)
        && !comparison.isEmpty() && comparison == _configurationComparison
        && comparison.value(QStringLiteral("registryContext")).toMap() == _registryContext
        && _preview.value(QStringLiteral("descriptor")).type() == QVariant::Map
        && descriptorForReview(snapshot, comparison) == _preview.value(QStringLiteral("descriptor")).toMap()
        && (baselineOperation(_operation)
            ? FleetConfigurationDiff::exactEqual(_registry->configurationApprovalRecord(_vehicleId), _configurationApproval)
            : _registry->configurationApproval(_vehicleId) == _configurationApproval)
        && (_operation != QStringLiteral("approve-baseline")
            || (yes(comparison.value(QStringLiteral("currentFullKnown"))) && !_configuration.isEmpty()))
        && (_operation != QStringLiteral("revoke-baseline") || localBaselineApproval(_configurationApproval, snapshot, _vehicleId));
    if (!originalRecordCurrent) return false;
    if (_operation != QStringLiteral("approve-performance"))
        return _proposedPerformance.isEmpty() && _proposedContext.isEmpty() && _performanceBaselineIdentity.isEmpty();
    QVariantMap canonical, baselineIdentity;
    if (!performanceProposal(_proposedPerformance, &canonical) || canonical != _proposedPerformance
        || !FleetCapabilityEvidence::currentConfigurationBaseline(snapshot, &baselineIdentity)
        || baselineIdentity != _performanceBaselineIdentity
        || baselineIdentity.value(QStringLiteral("registryContext")).toMap() != _registryContext
        || !FleetCapabilityEvidence::contextValid(_proposedContext)
        || _preview.value(QStringLiteral("proposedPerformance")).type() != QVariant::Map
        || _preview.value(QStringLiteral("proposedPerformance")).toMap() != _proposedPerformance
        || _preview.value(QStringLiteral("proposedContext")).type() != QVariant::Map
        || _preview.value(QStringLiteral("proposedContext")).toMap() != _proposedContext) return false;
    // Registry recomputes the proposed identity from THIS original record and
    // canonical proposal. The private expected context is never caller truth.
    const QVariantMap proposal = _registry->previewPerformanceDeclaration(_vehicleId, _registryContext, _proposedPerformance);
    return proposal.size() == 4 && yes(proposal.value(QStringLiteral("ok")))
        && proposal.value(QStringLiteral("reason")).type() == QVariant::String
        && proposal.value(QStringLiteral("reason")).toString().isEmpty()
        && proposal.value(QStringLiteral("proposedPerformance")).type() == QVariant::Map
        && proposal.value(QStringLiteral("proposedPerformance")).toMap() == _proposedPerformance
        && proposal.value(QStringLiteral("proposedContext")).type() == QVariant::Map
        && proposal.value(QStringLiteral("proposedContext")).toMap() == _proposedContext;
}

bool FleetCapabilityReview::memoryCurrent(quint64 generation) const
{
    return ownersCurrent(generation) && inputsValid(_operation, _capabilities, _engineeringReason)
        && recordCurrent() && _clock.elapsed() < _deadlineMs;
}

bool FleetCapabilityReview::contextMatches(QObject* actualSession, QObject* actualRoles,
    const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const
{
    if (!memoryCurrent(_generation) || actualSession != _session.data() || actualRoles != _roles.data()
        || expectedPrimary.size() != 5 || expectedRole.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("actorId"), QStringLiteral("role")}) {
        const QVariant value = expectedPrimary.value(key);
        if (value.type() != QVariant::String || value.isNull() || value != _identity.value(key)) return false;
    }
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
        QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")})
        if (expectedRole.value(key).type() != QVariant::String || expectedRole.value(key).isNull()) return false;
    for (const QString& key : {QStringLiteral("allowed"), QStringLiteral("requiresSecondApproval"),
        QStringLiteral("actorAuthenticated"), QStringLiteral("executionPermitted")})
        if (expectedRole.value(key).type() != QVariant::Bool || expectedRole.value(key).isNull()) return false;
    return expectedRole == _role && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role, _operation)
        && _clock.elapsed() < _deadlineMs;
}

bool FleetCapabilityReview::rootSourcesCurrent(quint64 generation) const
{
    if (!memoryCurrent(generation) || _rootChecking) return false;
    const QPointer<FleetCapabilityReview> self(const_cast<FleetCapabilityReview*>(this));
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root);
    const QString state = _state, id = _reviewId, scope = _scopeToken, binding = _bindingHash;
    const QString operation = _operation, reason = _engineeringReason;
    const int vehicleId = _vehicleId;
    const QStringList capabilities = _capabilities;
    const QVariantMap identity = _identity, role = _role, approver = _approver, preview = _preview;
    const QVariantMap proposedPerformance = _proposedPerformance, proposedContext = _proposedContext;
    const QVariantMap performanceBaseline = _performanceBaselineIdentity;
    _rootChecking = true; const BusyGuard checking{self, &_rootChecking};
    const auto same = [&]() {
        return self && memoryCurrent(generation) && root && root == _root && swarm && swarm == _swarm
            && registry && registry == _registry && session && session == _session && roles && roles == _roles
            && _state == state && _reviewId == id && _scopeToken == scope && _bindingHash == binding
            && _vehicleId == vehicleId && _operation == operation && _engineeringReason == reason && _capabilities == capabilities
            && _identity == identity && _role == role && _approver == approver && _preview == preview
            && _proposedPerformance == proposedPerformance && _proposedContext == proposedContext
            && _performanceBaselineIdentity == performanceBaseline
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
            && roles->actionSnapshotCurrent(role) && roleMatches(identity, role, operation);
    };
    if (!same()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetCapabilityReviewSourcesCurrent",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, matched),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(registry.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && same();
}

bool FleetCapabilityReview::sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover)
{
    const QPointer<FleetCapabilityReview> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    if (!memoryCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !roleMatches(_identity, _role, _operation)
        || !rootSourcesCurrent(generation) || !self || !memoryCurrent(generation)) return false;
    const QVariantMap primary = _identity, approver = _approver;
    if (checkIdentity && (!session->identitySnapshotCurrent(primary) || !self || !memoryCurrent(generation))) return false;
    if (!self || !session || !roles || !memoryCurrent(generation) || primary != _identity || approver != _approver
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !rootSourcesCurrent(generation)
        || !self || !memoryCurrent(generation)) return false;
    if (requireApprover && (!session->approverSnapshotCurrent(approver) || !self || !memoryCurrent(generation))) return false;
    return self && session && roles && memoryCurrent(generation) && primary == _identity && approver == _approver
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        && roles->actionSnapshotCurrent(_role) && rootSourcesCurrent(generation) && self && memoryCurrent(generation);
}

bool FleetCapabilityReview::authenticationBindingCurrent(FleetOperatorSession* actualSession,
    const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const
{
    // Session calls this around OS checks and after burning its pending
    // approval. This fence uses only frozen native values and original clocks.
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && !_authenticationConsumed
        && actualSession == _session.data() && primary == _identity && approver == _approver
        && digest(_bindingHash) && bindingHash == _bindingHash && _operationConfirmed
        && approver.value(QStringLiteral("action")).type() == QVariant::String
        && !authenticationPurpose().isEmpty() && approver.value(QStringLiteral("action")).toString() == authenticationPurpose()
        && approver.value(QStringLiteral("bindingHash")) == _bindingHash
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role, _operation) && _clock.elapsed() < _deadlineMs;
}

QString FleetCapabilityReview::authenticationPurpose() const
{
    return approvalPurpose(_operation);
}

bool FleetCapabilityReview::commitBindingCurrent(quint64 generation) const
{
    // No Root callback/OS check/Vehicle request in the final Registry fence.
    return memoryCurrent(generation) && _busy && _consuming && _applying && !_prompt && _authenticationConsumed
        && _operationConfirmed && !_preview.isEmpty() && digest(_bindingHash)
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role, _operation)
        && _session->capabilityReviewConsumedIdentityCurrent(_identity, _approver, this) && _clock.elapsed() < _deadlineMs;
}

void FleetCapabilityReview::disconnectSources()
{
    const auto connections = _connections; _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

void FleetCapabilityReview::watchSources(quint64 generation)
{
    const QPointer<FleetCapabilityReview> self(this);
    const auto stale = [this, self, generation]() {
        if (self) invalidate(generation, QStringLiteral("capability-review-original-source-mutated"));
    };
    const auto add = [this, self, generation](auto* sender, auto signal, auto handler) {
        if (!sender) return false;
        const auto connection = QObject::connect(sender, signal, this, handler);
        if (!self || !memoryCurrent(generation)) { QObject::disconnect(connection); return false; }
        _connections.append(connection); return true;
    };
    if (!add(_swarm.data(), &QObject::destroyed, stale) || !add(_registry.data(), &QObject::destroyed, stale)
        || !add(_root.data(), &QObject::destroyed, stale) || !add(_session.data(), &QObject::destroyed, stale)
        || !add(_roles.data(), &QObject::destroyed, stale)) return;
    // Ordinary telemetry can notify Registry every second. Compare only this
    // native record's static context/approval/configuration, never all health.
    if (!add(_registry.data(), &FleetRegistry::registryChanged, [this, self, generation]() {
        if (self && !_applying && !memoryCurrent(generation))
            invalidate(generation, QStringLiteral("capability-review-record-changed"));
    })) return;
    if (!add(_session.data(), &FleetOperatorSession::changed, [this, self, generation]() {
        if (self && !_prompt) invalidate(generation, QStringLiteral("capability-review-account-mutated"));
    })) return;
    if (!add(_roles.data(), &FleetRolePolicy::policyChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentUserChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentRoleChanged, stale)) return;
}

void FleetCapabilityReview::invalidate(quint64 generation, const QString& reason)
{
    if (thread() != QThread::currentThread() || generation != _generation || _applying
        || (_state != QStringLiteral("preparing") && _state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return;
    _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale"); _reason = reason;
    const QPointer<FleetCapabilityReview> self(this);
    disconnectSources();
    if (self) emit changed();
}

QVariantMap FleetCapabilityReview::requestReview(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
    const QString& requestedScopeToken, int requestedVehicleId, const QString& requestedOperation,
    const QStringList& requestedCapabilities, const QString& requestedReason, bool confirmed)
{
    // Preserve the original four-operation public contract. The new purpose
    // cannot be requested by relabelling an old capability/baseline request.
    if (requestedOperation != QStringLiteral("approve") && requestedOperation != QStringLiteral("revoke")
        && requestedOperation != QStringLiteral("approve-baseline") && requestedOperation != QStringLiteral("revoke-baseline"))
        return result(false, QStringLiteral("capability-review-request-invalid-or-busy"));
    return request(actualSession, actualRoles, actualRoot, requestedScopeToken, requestedVehicleId,
        requestedOperation, requestedCapabilities, QVariantMap(), requestedReason, confirmed);
}

QVariantMap FleetCapabilityReview::requestPerformanceReview(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
    const QString& scopeToken, int vehicleId, const QStringList& capabilities,
    const QVariantMap& proposedPerformance, const QString& reason, bool confirmed)
{
    return request(actualSession, actualRoles, actualRoot, scopeToken, vehicleId,
        QStringLiteral("approve-performance"), capabilities, proposedPerformance, reason, confirmed);
}

QVariantMap FleetCapabilityReview::request(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
    const QString& requestedScopeToken, int requestedVehicleId, const QString& requestedOperation,
    const QStringList& requestedCapabilities, const QVariantMap& requestedPerformance,
    const QString& requestedReason, bool confirmed)
{
    // Bound and validate before independently allocating caller text. No
    // external callback occurs between this check and the private freeze.
    if (thread() != QThread::currentThread() || _busy || _rootChecking || _applying
        || _state == QStringLiteral("preparing") || _state == QStringLiteral("review") || _state == QStringLiteral("approving")
        || !confirmed || !text(requestedScopeToken, 32768) || requestedVehicleId < 1 || requestedVehicleId > 255
        || !inputsValid(requestedOperation, requestedCapabilities, requestedReason)
        || FleetOperatorSession::authenticationPromptInProgress() || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("capability-review-request-invalid-or-busy"));
    const QPointer<QObject> requestedSession(actualSession), requestedRoles(actualRoles), requestedRoot(actualRoot);
    const QString scope = ownedText(requestedScopeToken), operation = ownedText(requestedOperation), reason = ownedText(requestedReason);
    QStringList capabilities;
    for (const QString& value : requestedCapabilities) capabilities.append(ownedText(value));
    const bool performance = operation == QStringLiteral("approve-performance");
    QVariantMap proposedPerformance;
    const bool proposalValid = performance ? performanceProposal(requestedPerformance, &proposedPerformance) : requestedPerformance.isEmpty();
    const int vehicleId = requestedVehicleId;
    if (!proposalValid)
        return result(false, QStringLiteral("capability-review-request-invalid-or-busy"));
    const QPointer<FleetCapabilityReview> self(this);
    QVariantMap fallback = result(false, QStringLiteral("capability-review-owner-destroyed"));
    _busy = true; const BusyGuard guard{self, &_busy, true};
    disconnectSources();
    if (!self) return fallback;
    _session = qobject_cast<FleetOperatorSession*>(requestedSession.data());
    _roles = qobject_cast<FleetRolePolicy*>(requestedRoles.data());
    _root = requestedRoot; _scopeToken = scope;
    _vehicleId = vehicleId; _operation = operation; _capabilities = capabilities; _engineeringReason = reason;
    _identity.clear(); _role.clear(); _approver.clear(); _registryContext.clear(); _previousApproval.clear();
    _previousReviewIdentity.clear(); _configurationApproval.clear(); _configuration.clear(); _configurationComparison.clear();
    _proposedPerformance = proposedPerformance; _proposedContext.clear(); _performanceBaselineIdentity.clear();
    _preview.clear(); _bindingHash.clear(); _commitReceipt.clear(); _recordIdentityFingerprint.clear();
    _authenticationConsumed = false; _operationConfirmed = false; _mutationCommitted = false;
    _consuming = false; _prompt = false; _applying = false;
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const quint64 generation = ++_generation;
    _deadlineMs = _clock.elapsed() + LifetimeMs;
    _state = QStringLiteral("preparing"); _reason.clear();
    fallback = result(false, QStringLiteral("capability-review-owner-destroyed"));
    const auto deny = [&](const QString& problem) {
        if (!self) return fallback;
        if (_generation == generation && (_state == QStringLiteral("preparing") || _state == QStringLiteral("review")
            || _state == QStringLiteral("approving"))) {
            _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale"); _reason = problem;
        }
        const QVariantMap response = result(false, _reason.isEmpty() ? problem : _reason);
        disconnectSources();
        if (self) emit changed();
        return response;
    };
    if (!ownersCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data()))
        return deny(QStringLiteral("actual-capability-review-sources-invalid"));
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QVariantMap snapshot = registry->vehicle(vehicleId);
    _registryContext = registry->capabilityReviewContext(vehicleId);
    _recordIdentityFingerprint = FleetCapabilityEvidence::identityFingerprint(snapshot);
    const QVariant rawApproval = snapshot.value(QStringLiteral("capabilityApproval"));
    if ((!FleetCapabilityEvidence::contextValid(_registryContext)
            && !(operation == QStringLiteral("revoke-baseline") && _registryContext.isEmpty()))
        || !digest(_recordIdentityFingerprint)
        || snapshot.value(QStringLiteral("systemId")).type() != QVariant::Int || snapshot.value(QStringLiteral("systemId")).toInt() != vehicleId
        || rawApproval.type() != QVariant::Map || rawApproval.isNull()
        || (!rawApproval.toMap().isEmpty() && !FleetCapabilityEvidence::approvalValid(rawApproval.toMap()))
        || (!_registryContext.isEmpty() && _recordIdentityFingerprint != _registryContext.value(QStringLiteral("identityFingerprint")).toString()))
        return deny(QStringLiteral("capability-review-current-native-record-unavailable"));
    _previousApproval = rawApproval.toMap();
    _previousReviewIdentity = FleetCapabilityEvidence::stableReviewIdentity(snapshot);
    _configurationApproval = baselineOperation(operation) ? registry->configurationApprovalRecord(vehicleId)
                                                        : registry->configurationApproval(vehicleId);
    if (snapshot.value(QStringLiteral("configuration")).type() != QVariant::Map)
        return deny(QStringLiteral("capability-review-configuration-shape-invalid"));
    _configuration = snapshot.value(QStringLiteral("configuration")).toMap();
    _configurationComparison = registry->configurationComparison(vehicleId);
    if (_configurationComparison.size() != 16
        || _configurationComparison.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || _configurationComparison.value(QStringLiteral("schemaVersion")).toInt() != 1
        || _configurationComparison.value(QStringLiteral("systemId")) != snapshot.value(QStringLiteral("systemId"))
        || _configurationComparison.value(QStringLiteral("registryContext")).toMap() != _registryContext
        || _configurationComparison.value(QStringLiteral("observationEpoch")).toString().isEmpty()
        || _configurationComparison.value(QStringLiteral("comparisonRevision")).toString().isEmpty()
        || _configurationComparison.value(QStringLiteral("comparison")).type() != QVariant::Map
        || _configurationComparison.value(QStringLiteral("comparison")).toMap().size() != 13)
        return deny(QStringLiteral("capability-review-configuration-comparison-unavailable"));
    if (_previousReviewIdentity.value(QStringLiteral("capabilityReviewContext")).toMap() != _registryContext
        || (operation == QStringLiteral("revoke") && (_previousApproval.isEmpty()
            || _previousApproval.value(QStringLiteral("state")).toString() == QStringLiteral("revoked"))))
        return deny(QStringLiteral("capability-review-operation-not-applicable"));
    if ((operation == QStringLiteral("approve-baseline")
            && (!yes(_configurationComparison.value(QStringLiteral("currentFullKnown"))) || _configuration.isEmpty()))
        || (operation == QStringLiteral("revoke-baseline") && !localBaselineApproval(_configurationApproval, snapshot, vehicleId)))
        return deny(QStringLiteral("configuration-baseline-operation-not-applicable"));
    if (performance) {
        QVariantMap baselineIdentity;
        if (!FleetCapabilityEvidence::currentConfigurationBaseline(snapshot, &baselineIdentity)
            || baselineIdentity.value(QStringLiteral("registryContext")).toMap() != _registryContext)
            return deny(QStringLiteral("performance-declaration-current-complete-baseline-required"));
        const QVariantMap proposal = registry->previewPerformanceDeclaration(vehicleId, _registryContext, _proposedPerformance);
        if (proposal.size() != 4 || !yes(proposal.value(QStringLiteral("ok")))
            || proposal.value(QStringLiteral("reason")).type() != QVariant::String
            || !proposal.value(QStringLiteral("reason")).toString().isEmpty()
            || proposal.value(QStringLiteral("proposedPerformance")).type() != QVariant::Map
            || proposal.value(QStringLiteral("proposedPerformance")).toMap() != _proposedPerformance
            || proposal.value(QStringLiteral("proposedContext")).type() != QVariant::Map
            || !FleetCapabilityEvidence::contextValid(proposal.value(QStringLiteral("proposedContext")).toMap()))
            return deny(QStringLiteral("performance-declaration-native-proposal-unavailable"));
        _proposedContext = ownedContext(proposal.value(QStringLiteral("proposedContext")).toMap());
        _performanceBaselineIdentity = ownedContext(baselineIdentity);
    }
    QStringList observed, declared, effective;
    if (!FleetCapabilityEvidence::canonicalCapabilities(snapshot.value(QStringLiteral("capabilities"), QStringList()), &observed)
        || !FleetCapabilityEvidence::canonicalCapabilities(snapshot.value(QStringLiteral("declaredVerifiedCapabilities"), QStringList()), &declared))
        return deny(QStringLiteral("capability-review-native-declarations-invalid"));
    const bool hasCurrentApproval = FleetCapabilityEvidence::currentCapabilities(snapshot, &effective);
    if (!baselineOperation(operation) && !_previousApproval.isEmpty()
        && _previousApproval.value(QStringLiteral("state")).toString() == QStringLiteral("current") && !hasCurrentApproval)
        return deny(QStringLiteral("capability-review-native-approval-inconsistent"));
    _preview = {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("source"), QStringLiteral("actual-native-registry-record")},
        {QStringLiteral("registryContext"), _registryContext}, {QStringLiteral("previousApproval"), _previousApproval},
        {QStringLiteral("previousReviewIdentity"), _previousReviewIdentity}, {QStringLiteral("configurationApproval"), _configurationApproval},
        {QStringLiteral("descriptor"), descriptorForReview(snapshot, _configurationComparison)},
        {QStringLiteral("configurationComparison"), _configurationComparison}, {QStringLiteral("observedCapabilities"), observed},
        {QStringLiteral("declaredCapabilities"), declared}, {QStringLiteral("effectiveCapabilities"), effective},
        {QStringLiteral("proposedOperation"), operation}, {QStringLiteral("proposedCapabilities"), capabilities},
        {QStringLiteral("engineeringReason"), reason}, {QStringLiteral("localDeclarationOnly"), true},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (performance) {
        _preview.insert(QStringLiteral("schemaVersion"), 3);
        _preview.insert(QStringLiteral("proposedPerformance"), _proposedPerformance);
        _preview.insert(QStringLiteral("proposedContext"), _proposedContext);
    }
    if (!memoryCurrent(generation)) return deny(QStringLiteral("capability-review-record-source-changed"));
    const QVariantMap identity = session->identitySnapshot();
    if (!self) return fallback;
    if (!memoryCurrent(generation) || !session || session != _session || !roles || roles != _roles || !registry || registry != _registry)
        return deny(QStringLiteral("capability-review-account-source-changed"));
    _identity = identity; _role = roles->actionSnapshot(policyAction(operation));
    if (!roleMatches(_identity, _role, operation) || !sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("capability-review-account-or-role-unavailable"));
    // Qt's JSON rendering cannot restore QVariant bytes/dates/typed nulls.
    // Exclude that DISPLAY string from authentication material. Authorization
    // instead compares the original private typed map, the actual Registry's
    // independent observation epoch/revision and the typed before/after hashes.
    // The bounded field display is never the sole identity of the comparison.
    QVariantMap bindingPreview = _preview;
    QVariantMap bindingDescriptor = bindingPreview.value(QStringLiteral("descriptor")).toMap();
    bindingDescriptor.remove(QStringLiteral("configurationJson"));
    bindingPreview.insert(QStringLiteral("descriptor"), bindingDescriptor);
    QVariantMap binding{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("generation"), QString::number(generation)},
        {QStringLiteral("action"), authenticationPurpose()}, {QStringLiteral("scopeToken"), _scopeToken},
        {QStringLiteral("recordIdentityFingerprint"), _recordIdentityFingerprint},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("operation"), operation},
        {QStringLiteral("capabilities"), capabilities}, {QStringLiteral("engineeringReason"), reason},
        {QStringLiteral("registryContext"), _registryContext}, {QStringLiteral("previousReviewIdentity"), _previousReviewIdentity},
        {QStringLiteral("preview"), bindingPreview}, {QStringLiteral("primaryIdentity"), _identity}, {QStringLiteral("role"), _role}};
    if (performance) binding.insert(QStringLiteral("performanceBaselineIdentity"), _performanceBaselineIdentity);
    _bindingHash = hash(binding);
    _state = QStringLiteral("review");
    watchSources(generation);
    if (!self) return fallback;
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("capability-review-preview-no-longer-current"));
    const QVariantMap response = result(true, QString());
    emit changed();
    return response;
}

QVariantMap FleetCapabilityReview::approveAndApply(const QString& requestedReviewId, bool operationConfirmed)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _busy || _rootChecking || _applying || _state != QStringLiteral("review")
        || reviewId.isEmpty() || reviewId != _reviewId || !operationConfirmed || FleetOperatorSession::authenticationPromptInProgress())
        return result(false, QStringLiteral("capability-review-confirmation-invalid-or-busy"));
    const QPointer<FleetCapabilityReview> self(this);
    QVariantMap fallback = result(false, QStringLiteral("capability-review-owner-destroyed"));
    _busy = true; const BusyGuard guard{self, &_busy, true};
    const quint64 generation = _generation;
    const auto deny = [&](const QString& problem) {
        if (!self) return fallback;
        _prompt = false; _consuming = false; _applying = false;
        if (_generation == generation && (_state == QStringLiteral("review") || _state == QStringLiteral("approving"))) {
            _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale"); _reason = problem;
        }
        const QVariantMap response = result(false, _reason.isEmpty() ? problem : _reason);
        disconnectSources();
        if (self) emit changed();
        return response;
    };
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("capability-review-no-longer-current"));
    _operationConfirmed = true;
    _state = QStringLiteral("approving"); _prompt = true;
    // Idle REVIEW is not an approval controller call. Verify the new phase
    // against the actual Root's native call lease BEFORE any OS prompt.
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("capability-review-approval-controller-context-rejected"));
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRegistry> registry(_registry);
    const QString purpose = authenticationPurpose();
    const QString summary = baselineOperation(_operation)
        ? QStringLiteral("LOCAL configuration baseline: %1; vehicle ID:%2; observation revision:%3; identity SHA256:%4. "
            "Approve/revoke only the frozen ground-station baseline reference and original record. "
            "NO capability adoption, onboard/parameter write, upload, ARM, START or flight permission. "
            "NOT physical-modification detection, hardware authentication or performance certification.")
            .arg(_operation, QString::number(_vehicleId), _configurationComparison.value(QStringLiteral("comparisonRevision")).toString(),
                _recordIdentityFingerprint)
        : _operation == QStringLiteral("approve-performance")
        ? QStringLiteral("LOCAL engineering performance; id:%1; maxSpeedMps:%2; enduranceSeconds:%3 (-1=UNKNOWN); "
            "caps:%4; revision:%5; proposed SHA256:%6. Frozen values/caps/reason, current full matching baseline only. "
            "NOT measured-performance/hardware certification. NO baseline/parameter write, upload, ARM, START or flight permission.")
            .arg(QString::number(_vehicleId), QString::number(_proposedPerformance.value(QStringLiteral("maxSpeedMps")).toDouble(), 'g', 17),
                QString::number(_proposedPerformance.value(QStringLiteral("enduranceSeconds")).toDouble(), 'g', 17),
                QString::number(_capabilities.size()), _registryContext.value(QStringLiteral("capabilityRevision")).toString(),
                _proposedContext.value(QStringLiteral("identityFingerprint")).toString())
        : QStringLiteral("LOCAL capability engineering declaration: %1; vehicle ID:%2; capability count:%3. "
            "Record revision:%4; identity SHA256:%5. The reviewed capabilities/reason/configuration are bound by the approval hash. "
            "No automatic modification detection, hardware authentication, performance certification, baseline change or flight command.")
            .arg(_operation, QString::number(_vehicleId), QString::number(_capabilities.size()),
                _registryContext.value(QStringLiteral("capabilityRevision")).toString(), _recordIdentityFingerprint);
    const QVariantMap approval = session->authenticateApprover(purpose, _bindingHash, _identity, summary);
    if (!self) return fallback;
    _prompt = false;
    if (!memoryCurrent(generation) || !session || session != _session || !registry || registry != _registry
        || !yes(approval.value(QStringLiteral("ok"))) || approval.value(QStringLiteral("identityReceipt")).type() != QVariant::Map)
        return deny(QStringLiteral("independent-capability-review-approval-unavailable"));
    _approver = approval.value(QStringLiteral("identityReceipt")).toMap();
    if (!sourcesCurrent(generation, true, true) || !self)
        return deny(QStringLiteral("capability-review-post-approval-source-changed"));
    _consuming = true;
    if (!session->consumeCapabilityReviewAuthentication(_identity, _approver, this, _bindingHash) || !self)
        return deny(QStringLiteral("capability-review-authentication-consumption-failed"));
    if (!memoryCurrent(generation)) return deny(QStringLiteral("capability-review-source-changed-after-consumption"));
    _authenticationConsumed = true;
    fallback = result(false, QStringLiteral("capability-review-owner-destroyed-after-consumption"));
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("capability-review-consumed-source-no-longer-current"));
    _applying = true;
    if (!commitBindingCurrent(generation)) return deny(QStringLiteral("capability-review-final-binding-rejected"));
    // Own the arguments and fence independently of this QObject. The actual
    // Registry's receipt is frozen BEFORE public notifications can retire us.
    const int vehicleId = _vehicleId;
    const QString operation = _operation, reason = _engineeringReason;
    const QStringList capabilities = _capabilities;
    const QVariantMap context = _registryContext;
    const QVariantMap comparison = _configurationComparison;
    const QVariantMap proposedPerformance = _proposedPerformance, proposedContext = _proposedContext;
    const auto fence = [self, generation]() { return self && self->commitBindingCurrent(generation); };
    QVariantMap receipt;
    if (operation == QStringLiteral("approve-baseline"))
        receipt = registry->approveCurrentConfiguration(vehicleId, comparison, reason, fence);
    else if (operation == QStringLiteral("revoke-baseline"))
        receipt = registry->rollbackConfigurationBaseline(vehicleId, comparison, reason, fence);
    else if (operation == QStringLiteral("approve-performance"))
        receipt = registry->approveDeclaredPerformance(vehicleId, context, proposedPerformance, proposedContext, capabilities, reason, fence);
    else if (operation == QStringLiteral("approve"))
        receipt = registry->approveCurrentCapabilities(vehicleId, context, capabilities, reason, fence);
    else
        receipt = registry->revokeCapabilityApproval(vehicleId, context, reason, fence);
    const bool committed = yes(receipt.value(QStringLiteral("mutationCommitted")));
    const bool accepted = yes(receipt.value(QStringLiteral("ok")));
    if (!self) {
        fallback.insert(QStringLiteral("ok"), committed && accepted);
        fallback.insert(QStringLiteral("state"), committed ? QStringLiteral("applied") : QStringLiteral("stale"));
        fallback.insert(QStringLiteral("reason"), committed ? QStringLiteral("local-engineering-mutation-recorded-owner-no-longer-retained")
            : QStringLiteral("capability-review-owner-destroyed-before-confirmed-mutation"));
        fallback.insert(QStringLiteral("mutationCommitted"), committed);
        fallback.insert(QStringLiteral("commitReceipt"), receipt);
        return fallback;
    }
    _commitReceipt = receipt; _mutationCommitted = committed; _applying = false; _consuming = false;
    if (!committed) return deny(QStringLiteral("capability-review-native-commit-rejected"));
    // An exhaustion-induced conservative invalidation can be a true mutation
    // with ok=false. APPLIED records a local mutation, not successful approval.
    _state = QStringLiteral("applied");
    _reason = accepted ? (baselineOperation(operation)
        ? QStringLiteral("local-configuration-baseline-mutated-not-hardware-or-performance-certified")
        : operation == QStringLiteral("approve-performance")
            ? QStringLiteral("local-performance-declaration-mutated-not-hardware-or-performance-certified")
            : QStringLiteral("local-capability-declaration-mutated-not-hardware-or-performance-certified"))
        : receipt.value(QStringLiteral("reason")).toString();
    const QVariantMap response = result(accepted, _reason);
    disconnectSources();
    if (self) emit changed();
    return response;
}

bool FleetCapabilityReview::cancel(const QString& requestedReviewId)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _applying || reviewId.isEmpty() || reviewId != _reviewId
        || (_state != QStringLiteral("preparing") && _state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return false;
    _state = QStringLiteral("cancelled"); _reason = QStringLiteral("operator-cancelled-capability-review");
    const QPointer<FleetCapabilityReview> self(this);
    disconnectSources();
    if (self) emit changed();
    return true;
}

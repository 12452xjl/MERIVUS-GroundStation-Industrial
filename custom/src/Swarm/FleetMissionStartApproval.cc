#include "FleetMissionStartApproval.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionStartReview.h"
#include "FleetMissionStartupAuthority.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 ApprovalLifetimeMs = 60000;
constexpr qint64 ReviewMaximumMs = 30000;
constexpr double MaxExactNumber = 9007199254740991.0;
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
const QString Purpose = QStringLiteral("conditional-start-intent-approval-not-dispatch");

struct FlagGuard {
    QPointer<QObject> object;
    bool* flag;
    ~FlagGuard() { if (object) *flag = false; }
};
bool outboundSourcesCurrent(const QObject* approval, const QObject* session, const QObject* review, const QObject* deployment)
{
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(approval, session)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(approval, review)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(approval, deployment);
}
bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
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
bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool decimal(const QVariant& value)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    bool valid = false;
    const quint64 number = string.toULongLong(&valid);
    return valid && number > 0 && QString::number(number) == string;
}
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
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
bool bindingShape(const QVariantMap& binding)
{
    const QSet<QString> texts = {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    const QSet<QString> numbers = {QStringLiteral("recordRevision"), QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")};
    if ((binding.size() != 7 && binding.size() != 9)
        || binding.contains(QStringLiteral("reviewGeneration")) != binding.contains(QStringLiteral("inputGeneration"))) return false;
    for (auto it = binding.constBegin(); it != binding.constEnd(); ++it) {
        if (texts.contains(it.key())) { if (!text(it.value())) return false; }
        else if (numbers.contains(it.key())) {
            double number = 0;
            if (!integer(it.value(), it.key() == QStringLiteral("recordRevision") ? 1 : 0, MaxExactNumber, &number)) return false;
        } else return false;
    }
    for (const QString& key : texts) if (!binding.contains(key)) return false;
    return binding.contains(QStringLiteral("recordRevision")) && digest(binding.value(QStringLiteral("planId")))
        && digest(binding.value(QStringLiteral("contentHash"))) && digest(binding.value(QStringLiteral("reviewEvidenceHash")));
}
bool idsShape(const QVariant& value)
{
    if (value.type() != QVariant::List) return false;
    const QVariantList ids = value.toList();
    if (ids.isEmpty() || ids.size() > 32) return false;
    QSet<int> seen;
    for (const QVariant& id : ids) {
        double number = 0;
        if (!integer(id, 1, 255, &number) || seen.contains(static_cast<int>(number))) return false;
        seen.insert(static_cast<int>(number));
    }
    return true;
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 18 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += static_cast<qint64>(it.key().size()) * 2;
            if (*bytes > MaximumBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!boundedTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList()) if (!boundedTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::String:
        *bytes += static_cast<qint64>(value.toString().size()) * 2;
        return *bytes <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= MaxExactNumber;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool bytes(const QVariantMap& map, QByteArray* encoded, qint64 maximum = MaximumBytes)
{
    int nodes = 0;
    qint64 amount = 0;
    if (map.isEmpty() || !boundedTree(map, 0, &nodes, &amount)) return false;
    *encoded = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !encoded->isEmpty() && encoded->size() <= maximum;
}
QString hash(const QVariantMap& map)
{
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}
QVariantMap stableContext(QVariantMap context)
{
    context.remove(QStringLiteral("remainingLifetimeMs"));
    return context;
}
bool contextShape(const QVariantMap& context)
{
    double schema = 0, remaining = 0;
    if (context.size() != 17 || !integer(context.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || !isTrue(context.value(QStringLiteral("ok"))) || !uuid(context.value(QStringLiteral("reviewId")))
        || !uuid(context.value(QStringLiteral("reviewEpoch"))) || !decimal(context.value(QStringLiteral("reviewGeneration")))
        || !uuid(context.value(QStringLiteral("deploymentId"))) || !decimal(context.value(QStringLiteral("deploymentGeneration")))
        || context.value(QStringLiteral("binding")).type() != QVariant::Map || !bindingShape(context.value(QStringLiteral("binding")).toMap())
        || context.value(QStringLiteral("roleSnapshot")).type() != QVariant::Map
        || !idsShape(context.value(QStringLiteral("targetVehicleIds")))
        || context.value(QStringLiteral("reviewArtifact")).type() != QVariant::Map
        || !digest(context.value(QStringLiteral("sourceFingerprint"))) || !isTrue(context.value(QStringLiteral("reviewAcknowledged")))
        || !isFalse(context.value(QStringLiteral("startupPermitGranted"))) || !isFalse(context.value(QStringLiteral("executable")))
        || !isFalse(context.value(QStringLiteral("flightCommandReleased")))
        || !integer(context.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewMaximumMs, &remaining)) return false;
    QByteArray encoded;
    const QVariantMap artifact = context.value(QStringLiteral("reviewArtifact")).toMap();
    return digest(artifact.value(QStringLiteral("artifactFingerprint")))
        && bytes(context, &encoded, 3LL * 1024 * 1024);
}
bool primaryRole(const QVariantMap& identity, const QVariantMap& role)
{
    const QString principalRole = identity.value(QStringLiteral("role")).toString();
    return (principalRole == QStringLiteral("operator") || principalRole == QStringLiteral("commander") || principalRole == QStringLiteral("administrator"))
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && isTrue(role.value(QStringLiteral("allowed"))) && isTrue(role.value(QStringLiteral("requiresSecondApproval")))
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")))
        && isTrue(identity.value(QStringLiteral("authenticated")));
}
bool approvalShape(const QVariantMap& receipt)
{
    double schema = 0, approved = 0, expires = 0;
    if (receipt.size() != 34 || !integer(receipt.value(QStringLiteral("schemaVersion")), 1, 1, &schema)) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approvalEpoch"), QStringLiteral("reviewId"),
        QStringLiteral("reviewEpoch"), QStringLiteral("deploymentId"), QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionEpoch"),
        QStringLiteral("rolePolicyEpoch")}) if (!uuid(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("approvalGeneration"), QStringLiteral("reviewGeneration"), QStringLiteral("deploymentGeneration"),
        QStringLiteral("applicantSessionRevision"), QStringLiteral("rolePolicyRevision")}) if (!decimal(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("reviewContextHash"), QStringLiteral("reviewSourceFingerprint"), QStringLiteral("reviewArtifactFingerprint")})
        if (!digest(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("primaryRole"), QStringLiteral("approverId"), QStringLiteral("approverRole")})
        if (!text(receipt.value(key))) return false;
    return receipt.value(QStringLiteral("action")).type() == QVariant::String && receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && receipt.value(QStringLiteral("purpose")).type() == QVariant::String && receipt.value(QStringLiteral("purpose")).toString() == Purpose
        && receipt.value(QStringLiteral("binding")).type() == QVariant::Map && bindingShape(receipt.value(QStringLiteral("binding")).toMap())
        && idsShape(receipt.value(QStringLiteral("targetVehicleIds")))
        && integer(receipt.value(QStringLiteral("approvedAtMonotonicMs")), 0, MaxExactNumber, &approved)
        && integer(receipt.value(QStringLiteral("expiresAtMonotonicMs")), 0, MaxExactNumber, &expires) && expires > approved
        && isTrue(receipt.value(QStringLiteral("conditionalIntentApproved"))) && isTrue(receipt.value(QStringLiteral("oneUse")))
        && isTrue(receipt.value(QStringLiteral("requiresFreshSourceRevalidation"))) && isTrue(receipt.value(QStringLiteral("technicalRisksPreserved")))
        && isFalse(receipt.value(QStringLiteral("startupPermitGranted"))) && isFalse(receipt.value(QStringLiteral("executable")))
        && isFalse(receipt.value(QStringLiteral("flightCommandReleased")));
}
bool grantShape(const QVariantMap& grant)
{
    double schema = 0, consumed = 0, expires = 0;
    return grant.size() == 13 && integer(grant.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        && uuid(grant.value(QStringLiteral("grantId"))) && uuid(grant.value(QStringLiteral("startupId")))
        && grant.value(QStringLiteral("approvalReceipt")).type() == QVariant::Map && approvalShape(grant.value(QStringLiteral("approvalReceipt")).toMap())
        && grant.value(QStringLiteral("authenticationReceipt")).type() == QVariant::Map
        && integer(grant.value(QStringLiteral("consumedAtMonotonicMs")), 0, MaxExactNumber, &consumed)
        && integer(grant.value(QStringLiteral("expiresAtMonotonicMs")), 0, MaxExactNumber, &expires) && expires > consumed
        && digest(grant.value(QStringLiteral("reviewContextHash"))) && grant.value(QStringLiteral("purpose")).type() == QVariant::String
        && grant.value(QStringLiteral("purpose")).toString() == Purpose && isTrue(grant.value(QStringLiteral("conditionalIntentApproved")))
        && isFalse(grant.value(QStringLiteral("startupPermitGranted"))) && isFalse(grant.value(QStringLiteral("executable")))
        && isFalse(grant.value(QStringLiteral("flightCommandReleased")));
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* encoded)
{
    const QString task = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(task), record = store->record(task, binding.value(QStringLiteral("recordRevision")));
    if (!isFalse(latest.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return bytes(record, encoded);
}
QVariantList codes(const QVariant& value)
{
    QVariantList result;
    if (value.type() != QVariant::List) return result;
    for (const QVariant& entry : value.toList()) {
        if (entry.type() == QVariant::String) result.append(entry.toString().left(256));
        if (result.size() == 32) break;
    }
    return result;
}
QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("purpose"), Purpose},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

FleetMissionStartApproval::FleetMissionStartApproval(QObject* parent)
    : QObject(parent), _approvalEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _timer.setInterval(1000);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (_expired()) _invalidate(_generation, QStringLiteral("original-review-or-approval-expired"), QStringLiteral("expired"));
    });
}
FleetMissionStartApproval::~FleetMissionStartApproval()
{
    _disconnectSources();
    if (_session && !_authenticationReceipt.isEmpty()) _session->revokeStartApprovalAuthentication(_approvalId);
}
bool FleetMissionStartApproval::_expired() const { return _expiresAtMs < 0 || !_clock.isValid() || _clock.elapsed() >= _expiresAtMs; }
bool FleetMissionStartApproval::_same(quint64 generation, const QString& state) const
{
    return generation != 0 && _generation == generation && _state == state && !_expired();
}
QVariantMap FleetMissionStartApproval::current() const
{
    const bool active = _state == QStringLiteral("pending") || _state == QStringLiteral("approved") || _state == QStringLiteral("consumed");
    const bool expired = active && _expired();
    const QVariantMap artifact = _context.value(QStringLiteral("reviewArtifact")).toMap();
    return {{QStringLiteral("ok"), _state == QStringLiteral("approved") && !expired},
        {QStringLiteral("approvalId"), _approvalId}, {QStringLiteral("state"), expired ? QStringLiteral("expired") : _state},
        {QStringLiteral("reason"), expired ? QStringLiteral("original-review-or-approval-expired") : _reason},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("purpose"), Purpose},
        {QStringLiteral("approvalReceipt"), _approvalReceipt}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("reviewId"), _context.value(QStringLiteral("reviewId"))},
        {QStringLiteral("deploymentId"), _context.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("targetVehicleIds"), _context.value(QStringLiteral("targetVehicleIds"))},
        {QStringLiteral("actorId"), _identityReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverId"), _approverReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("remainingMs"), active && !expired ? static_cast<double>(_expiresAtMs - _clock.elapsed()) : 0.0},
        {QStringLiteral("blockedReasons"), codes(artifact.value(QStringLiteral("blockedReasons")))},
        {QStringLiteral("unverifiedRequirements"), codes(artifact.value(QStringLiteral("unverifiedRequirements")))},
        {QStringLiteral("conditionalIntentApproved"), !expired && (_state == QStringLiteral("approved") || _state == QStringLiteral("consumed"))},
        {QStringLiteral("oneUseConsumed"), _state == QStringLiteral("consumed") || _state == QStringLiteral("transferred")},
        {QStringLiteral("authenticationTransferred"), _state == QStringLiteral("transferred")},
        {QStringLiteral("transferredStartupId"), _transferredStartupId},
        {QStringLiteral("technicalRisksPreserved"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("revalidationRequired"), true}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
void FleetMissionStartApproval::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionStartApproval::_invalidate(quint64 generation, const QString& reason, const QString& state, bool deferNotification)
{
    if (_generation != generation || (_state != QStringLiteral("pending") && _state != QStringLiteral("approved") && _state != QStringLiteral("consumed"))) return;
    if (_session && !_authenticationReceipt.isEmpty()) _session->revokeStartApprovalAuthentication(_approvalId);
    _state = state; _reason = reason; _timer.stop(); _disconnectSources();
    if (deferNotification) {
        const QPointer<FleetMissionStartApproval> self(this);
        QTimer::singleShot(0, this, [self, generation, state]() {
            if (self && self->_generation == generation && self->_state == state) emit self->changed();
        });
    } else emit changed();
}
void FleetMissionStartApproval::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _invalidate(generation, QStringLiteral("start-intent-source-changed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_review.data()), static_cast<QObject*>(_deployment.data()), _authorization.data()})
        _connections.append(connect(object, &QObject::destroyed, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_review.data(), &FleetMissionStartReview::changed, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    for (const Target& target : _targets) {
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            if (object) _connections.append(connect(object, &QObject::destroyed, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        if (target.primary) {
            _connections.append(connect(target.primary.data(), &LinkInterface::connected, this, stale));
            _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        }
    }
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        if (_busy && _state == QStringLiteral("pending")) return; // Native prompt busy; full post-modal recapture is mandatory.
        const QPointer<FleetMissionStartApproval> self(this);
        const QPointer<FleetOperatorSession> session(_session);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QVariantMap identity = _identityReceipt, role = _roleReceipt;
        const QString state = _state;
        const bool identityCurrent = session && session->identitySnapshotCurrent(identity);
        if (!self) return;
        if (identityCurrent && _same(generation, state) && session && roles && _session == session && _roles == roles
            && _identityReceipt == identity && _roleReceipt == role && roles->actionSnapshotCurrent(role)) return;
        _invalidate(generation, QStringLiteral("start-intent-authenticated-source-changed"));
    }));
}
bool FleetMissionStartApproval::_targetsCurrent(quint64 generation, const QList<Target>& suppliedTargets) const
{
    const QList<Target> targets = suppliedTargets;
    const QPointer<FleetMissionStartApproval> self(const_cast<FleetMissionStartApproval*>(this));
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QString deploymentId = _context.value(QStringLiteral("deploymentId")).toString();
    const QString state = _state;
    const auto live = [&]() { return self && _same(generation, state) && deployment && _deployment == deployment; };
    if (!live() || targets.isEmpty() || targets.size() > 32 || targets.size() != _targets.size()) return false;
    for (int index = 0; index < targets.size(); ++index) {
        const Target target = targets.at(index);
        if (!target.vehicle || !target.mission || !target.parameters || !target.links
            || (target.hadPrimary && !target.primary) || !live()) return false;
        const QPointer<Vehicle> vehicle(deployment->startReviewVehicle(deploymentId, target.vehicleId));
        if (!live() || !target.vehicle || !target.mission || !target.parameters || !target.links) return false;
        const QPointer<MissionManager> mission(deployment->startReviewManager(deploymentId, target.vehicleId));
        if (!live() || !target.vehicle || !target.mission || !target.parameters || !target.links) return false;
        const SharedLinkInterfacePtr primary = target.links->primaryLink().lock();
        if (!live() || vehicle != target.vehicle || mission != target.mission || primary.get() != target.primary.data()
            || bool(primary) != target.hadPrimary || target.vehicle->id() != target.vehicleId
            || target.componentId < 1 || target.componentId > 255 || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.vehicle->thread() != thread() || target.mission->thread() != thread()
            || target.parameters->thread() != thread() || target.links->thread() != thread()
            || target.mission->inProgress() || target.mission->transportCallInProgress()) return false;
        const Target currentTarget = _targets.at(index);
        if (target.vehicleId != currentTarget.vehicleId || target.componentId != currentTarget.componentId
            || target.vehicle != currentTarget.vehicle || target.mission != currentTarget.mission
            || target.parameters != currentTarget.parameters || target.links != currentTarget.links || target.primary != currentTarget.primary
            || target.hadPrimary != currentTarget.hadPrimary) return false;
    }
    return live();
}
bool FleetMissionStartApproval::_sourcesCurrent(quint64 generation, bool requireApprover)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    if (!outboundSourcesCurrent(this, _session.data(), _review.data(), _deployment.data())) return false;
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionStartApproval> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionStartReview> review(_review);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap context = _context, identity = _identityReceipt, role = _roleReceipt,
        approver = _approverReceipt, authentication = _authenticationReceipt;
    const QByteArray record = _recordBytes;
    const QList<Target> targets = _targets;
    const QString state = _state, bindingHash = _bindingHash;
    const auto unchanged = [&]() {
        return self && !_reentered && _same(generation, state) && session && roles && store && review && deployment && owner
            && outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())
            && _session == session && _roles == roles && _store == store && _review == review && _deployment == deployment && _authorization == owner
            && _context == context && _identityReceipt == identity && _roleReceipt == role && _approverReceipt == approver
            && _authenticationReceipt == authentication && _recordBytes == record && _bindingHash == bindingHash
            && session->thread() == thread() && roles->thread() == thread() && store->thread() == thread()
            && review->thread() == thread() && deployment->thread() == thread() && owner->thread() == thread();
    };
    const auto nativeCurrent = [&]() {
        if (!unchanged() || !roles->actionSnapshotCurrent(role) || !primaryRole(identity, role)) return false;
        if (!session->identitySnapshotCurrent(identity) || !unchanged()) return false;
        if (!_targetsCurrent(generation, targets) || !unchanged()) return false;
        const QVariantMap riskRole = context.value(QStringLiteral("roleSnapshot")).toMap();
        if (!roles->actionSnapshotCurrent(riskRole) || riskRole.value(QStringLiteral("action")).toString() != QStringLiteral("mission.start.review")
            || riskRole.value(QStringLiteral("actorId")) != identity.value(QStringLiteral("actorId"))
            || riskRole.value(QStringLiteral("role")) != identity.value(QStringLiteral("role"))) return false;
        if (!review->approvalContextCurrent(context.value(QStringLiteral("reviewId")).toString(), context) || !unchanged()) return false;
        if (!_targetsCurrent(generation, targets) || !unchanged()) return false;
        QByteArray actualRecord;
        if (!currentRecord(store.data(), context.value(QStringLiteral("binding")).toMap(), &actualRecord) || actualRecord != record) return false;
        const QVariantMap deploymentContext = deployment->startReviewContext(context.value(QStringLiteral("deploymentId")).toString());
        if (!unchanged() || !isTrue(deploymentContext.value(QStringLiteral("ok")))
            || deploymentContext.value(QStringLiteral("generation")) != context.value(QStringLiteral("deploymentGeneration"))) return false;
        if (!requireApprover) return true;
        if (state == QStringLiteral("consumed")) {
            if (!session->startApprovalAuthenticationCurrent(authentication) || !unchanged()) return false;
            return authentication.value(QStringLiteral("approvalId")) == _approvalId
                && authentication.value(QStringLiteral("bindingHash")).toString() == bindingHash
                && authentication.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start");
        }
        if (!session->approverSnapshotCurrent(approver) || !unchanged()) return false;
        return approver.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
            && approver.value(QStringLiteral("bindingHash")).toString() == bindingHash
            && approver.value(QStringLiteral("actorId")) != identity.value(QStringLiteral("actorId"));
    };
    if (!nativeCurrent()) return false;
    QVariant sources;
    const bool sourcesInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartApprovalSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, sources), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(store.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(review.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(deployment.data()))));
    if (!unchanged() || !sourcesInvoked || !isTrue(sources) || !nativeCurrent()) return false;
    QVariant allowed;
    const bool allowedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartApprovalContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(context)));
    return unchanged() && allowedInvoked && isTrue(allowed) && nativeCurrent();
}

QVariantMap FleetMissionStartApproval::requestStartApproval(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* reviewObject, QObject* deploymentObject, QObject* authorizationOwner,
    const QString& suppliedReviewId)
{
    const QString reviewId = suppliedReviewId;
    if (thread() != QThread::currentThread() || _busy || _checking || !uuid(reviewId)) {
        _reentered = true;
        return rejection(QStringLiteral("native-owner-idle-and-current-review-id-required"));
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return rejection(QStringLiteral("current-native-deployment-episode-required"));
    if (_state == QStringLiteral("consumed") && !_expired()) return rejection(QStringLiteral("consumed-start-intent-busy-revoke-first"));
    if (_state == QStringLiteral("transferred")) return rejection(QStringLiteral("conditional-authentication-transferred-use-startup-cancel"));
    if (_generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("start-approval-generation-exhausted"));
    _busy = true;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartApproval> self(this);
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(sessionObject);
    QPointer<FleetRolePolicy> roles = qobject_cast<FleetRolePolicy*>(roleObject);
    QPointer<FleetMissionPlanStore> store = qobject_cast<FleetMissionPlanStore*>(storeObject);
    QPointer<FleetMissionStartReview> review = qobject_cast<FleetMissionStartReview*>(reviewObject);
    QPointer<FleetMissionDeployment> deployment = qobject_cast<FleetMissionDeployment*>(deploymentObject);
    QPointer<QObject> owner(authorizationOwner);
    const auto sourceObjects = [&]() { return self && session && roles && store && review && deployment && owner
        && outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data()); };
    if (!sourceObjects() || session->thread() != thread() || roles->thread() != thread() || store->thread() != thread()
        || review->thread() != thread() || deployment->thread() != thread() || owner->thread() != thread())
        return rejection(QStringLiteral("actual-start-approval-native-sources-required"));
    const qint64 startedAt = _clock.elapsed();
    const QVariantMap identity = session->identitySnapshot();
    if (!sourceObjects()) return rejection(QStringLiteral("start-approval-source-destroyed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start"));
    if (!session->identitySnapshotCurrent(identity) || !sourceObjects() || !roles->actionSnapshotCurrent(role) || !primaryRole(identity, role))
        return rejection(QStringLiteral("authenticated-start-role-and-extra-approval-required"));
    const qint64 captureBefore = _clock.elapsed();
    const QVariantMap context = review->approvalContext(reviewId);
    if (!sourceObjects() || !contextShape(context) || context.value(QStringLiteral("reviewId")).toString() != reviewId)
        return rejection(QStringLiteral("actual-unexpired-acknowledged-review-context-required"));
    const QVariantMap riskRole = context.value(QStringLiteral("roleSnapshot")).toMap();
    if (!roles->actionSnapshotCurrent(riskRole) || riskRole.value(QStringLiteral("actorId")) != identity.value(QStringLiteral("actorId"))
        || riskRole.value(QStringLiteral("role")) != identity.value(QStringLiteral("role")))
        return rejection(QStringLiteral("risk-acknowledgement-must-belong-to-current-authenticated-actor"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    QList<Target> targets;
    for (const QVariant& id : context.value(QStringLiteral("targetVehicleIds")).toList()) {
        Target target;
        target.vehicleId = id.toInt();
        target.vehicle = deployment->startReviewVehicle(context.value(QStringLiteral("deploymentId")).toString(), target.vehicleId);
        if (!sourceObjects() || !target.vehicle) return rejection(QStringLiteral("actual-conditional-start-target-required"));
        target.mission = deployment->startReviewManager(context.value(QStringLiteral("deploymentId")).toString(), target.vehicleId);
        if (!sourceObjects() || !target.vehicle || !target.mission) return rejection(QStringLiteral("actual-conditional-start-manager-required"));
        target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager();
        target.componentId = target.vehicle->defaultComponentId();
        if (target.componentId < 1 || target.componentId > 255) return rejection(QStringLiteral("actual-default-start-component-required"));
        if (!target.parameters || !target.links) return rejection(QStringLiteral("actual-conditional-start-parameter-link-owner-required"));
        const SharedLinkInterfacePtr primary = target.links->primaryLink().lock();
        target.primary = primary.get(); target.hadPrimary = bool(primary);
        targets.append(target);
    }
    QByteArray record;
    if (!currentRecord(store.data(), binding, &record)) return rejection(QStringLiteral("latest-nonrestored-full-start-record-required"));
    double remaining = 0;
    if (!integer(context.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewMaximumMs, &remaining)
        || captureBefore > static_cast<qint64>(MaxExactNumber) - static_cast<qint64>(remaining)
        || _clock.elapsed() >= captureBefore + static_cast<qint64>(remaining))
        return rejection(QStringLiteral("original-risk-review-deadline-already-expired"));
    if (!sourceObjects()) return rejection(QStringLiteral("start-approval-episode-changed-before-state-replacement"));
    // A consumed native account proof can outlive our shorter original-review
    // cap. Do not orphan it when replacing an expired local intent.
    if (_session && !_authenticationReceipt.isEmpty()) _session->revokeStartApprovalAuthentication(_approvalId);
    _disconnectSources(); _timer.stop();
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _review = review; _deployment = deployment; _authorization = owner;
    _identityReceipt = identity; _roleReceipt = role; _context = context; _binding = binding; _recordBytes = record;
    _targets = targets;
    _approverReceipt.clear(); _authenticationReceipt.clear(); _approvalReceipt.clear(); _grant.clear();
    _transferredAuthority.clear(); _transferredStartupId.clear(); _transferredAuthorityEpoch.clear(); _transferredSourceHash.clear();
    _createdAtMs = startedAt;
    _expiresAtMs = qMin(startedAt + ApprovalLifetimeMs, captureBefore + static_cast<qint64>(remaining));
    _contextHash = hash(stableContext(context));
    _bindingHash = hash(QVariantMap{{QStringLiteral("reviewContext"), stableContext(context)},
        {QStringLiteral("fullRecordHash"), QString::fromLatin1(QCryptographicHash::hash(record, QCryptographicHash::Sha256).toHex())},
        {QStringLiteral("purpose"), Purpose}, {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}});
    _approvalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _state = QStringLiteral("pending"); _reason.clear();
    _connectSources(generation); _timer.start();
    if (!_sourcesCurrent(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("start-approval-context-invalid-before-prompt"));
        return rejection(QStringLiteral("start-approval-context-invalid-before-prompt"));
    }
    emit changed();
    if (!self || !_same(generation, QStringLiteral("pending")) || !_sourcesCurrent(generation, false))
        return rejection(QStringLiteral("start-approval-revoked-before-prompt"));
    QStringList targetText;
    for (const QVariant& id : context.value(QStringLiteral("targetVehicleIds")).toList()) targetText.append(QString::number(id.toInt()));
    const QVariantMap riskArtifact = context.value(QStringLiteral("reviewArtifact")).toMap();
    const QString summary = QStringLiteral(
        "CONDITIONAL intent; NO ARM/START/dispatch.\nReview: %1\nDeployment: %2\nTargets: %3\nPlan SHA-256: %4\nUnresolved: %5 blocked / %6 unverified; see ORIGINAL risk review.\nOriginal 30-second review/account deadlines; NO renewal.")
        .arg(reviewId, context.value(QStringLiteral("deploymentId")).toString(), targetText.join(QStringLiteral(", ")),
            binding.value(QStringLiteral("planId")).toString())
        .arg(codes(riskArtifact.value(QStringLiteral("blockedReasons"))).size())
        .arg(codes(riskArtifact.value(QStringLiteral("unverifiedRequirements"))).size());
    const QVariantMap authentication = session->authenticateApprover(QStringLiteral("mission.start"), _bindingHash, identity, summary);
    if (!self) return rejection(QStringLiteral("start-approval-owner-destroyed-during-prompt"));
    if (!_same(generation, QStringLiteral("pending")) || !sourceObjects())
        return rejection(QStringLiteral("start-approval-context-revoked-during-prompt"));
    if (!isTrue(authentication.value(QStringLiteral("ok"))) || authentication.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) {
        _invalidate(generation, QStringLiteral("independent-start-approver-not-authenticated"));
        return rejection(QStringLiteral("independent-start-approver-not-authenticated"));
    }
    _approverReceipt = authentication.value(QStringLiteral("identityReceipt")).toMap();
    if (!_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("start-approval-post-modal-context-invalid"));
        return rejection(QStringLiteral("start-approval-post-modal-context-invalid"));
    }
    _approvalReceipt = {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("approvalId"), _approvalId},
        {QStringLiteral("approvalEpoch"), _approvalEpoch}, {QStringLiteral("approvalGeneration"), QString::number(generation)},
        {QStringLiteral("action"), QStringLiteral("mission.start")}, {QStringLiteral("purpose"), Purpose},
        {QStringLiteral("reviewId"), reviewId}, {QStringLiteral("reviewEpoch"), context.value(QStringLiteral("reviewEpoch"))},
        {QStringLiteral("reviewGeneration"), context.value(QStringLiteral("reviewGeneration"))},
        {QStringLiteral("deploymentId"), context.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("deploymentGeneration"), context.value(QStringLiteral("deploymentGeneration"))},
        {QStringLiteral("binding"), binding}, {QStringLiteral("reviewContextHash"), _contextHash},
        {QStringLiteral("reviewSourceFingerprint"), context.value(QStringLiteral("sourceFingerprint"))},
        {QStringLiteral("reviewArtifactFingerprint"), context.value(QStringLiteral("reviewArtifact")).toMap().value(QStringLiteral("artifactFingerprint"))},
        {QStringLiteral("actorId"), identity.value(QStringLiteral("actorId"))},
        {QStringLiteral("applicantSessionId"), identity.value(QStringLiteral("sessionId"))},
        {QStringLiteral("applicantSessionEpoch"), identity.value(QStringLiteral("sessionEpoch"))},
        {QStringLiteral("applicantSessionRevision"), identity.value(QStringLiteral("sessionRevision"))},
        {QStringLiteral("primaryRole"), identity.value(QStringLiteral("role"))},
        {QStringLiteral("approverId"), _approverReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverRole"), _approverReceipt.value(QStringLiteral("role"))},
        {QStringLiteral("rolePolicyEpoch"), role.value(QStringLiteral("policyEpoch"))},
        {QStringLiteral("rolePolicyRevision"), role.value(QStringLiteral("policyRevision"))},
        {QStringLiteral("approvedAtMonotonicMs"), static_cast<double>(_clock.elapsed())},
        {QStringLiteral("expiresAtMonotonicMs"), static_cast<double>(_expiresAtMs)},
        {QStringLiteral("targetVehicleIds"), context.value(QStringLiteral("targetVehicleIds"))},
        {QStringLiteral("conditionalIntentApproved"), true}, {QStringLiteral("oneUse"), true},
        {QStringLiteral("requiresFreshSourceRevalidation"), true}, {QStringLiteral("technicalRisksPreserved"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
    _state = QStringLiteral("approved");
    if (!approvalShape(_approvalReceipt) || !_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("conditional-start-approval-final-capture-invalid"));
        return rejection(QStringLiteral("conditional-start-approval-final-capture-invalid"));
    }
    emit changed();
    if (!self || !_same(generation, QStringLiteral("approved")) || !_sourcesCurrent(generation, true))
        return rejection(QStringLiteral("conditional-start-approval-revoked-at-publication"));
    return current();
}

QVariantMap FleetMissionStartApproval::consumeStartApproval(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* reviewObject, QObject* deploymentObject, QObject* authorizationOwner,
    const QVariantMap& suppliedReceipt, const QString& suppliedStartupId)
{
    const QVariantMap receipt = suppliedReceipt;
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _busy || _checking || !uuid(startupId)
        || !outboundSourcesCurrent(this, _session.data(), _review.data(), _deployment.data())
        || !_same(_generation, QStringLiteral("approved")) || !approvalShape(receipt) || receipt != _approvalReceipt
        || sessionObject != _session.data() || roleObject != _roles.data() || storeObject != _store.data()
        || reviewObject != _review.data() || deploymentObject != _deployment.data() || authorizationOwner != _authorization.data()) return QVariantMap();
    _busy = true;
    FlagGuard consuming{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartApproval> self(this);
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetMissionStartReview> review(_review);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    if (!_sourcesCurrent(generation, true) || !self || !session) return QVariantMap();
    const QVariantMap identity = _identityReceipt, approver = _approverReceipt;
    const QString approvalId = _approvalId, bindingHash = _bindingHash;
    const qint64 consumedAt = _clock.elapsed();
    if (!outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())) return QVariantMap();
    const QVariantMap result = session->consumeStartApprovalAuthentication(identity, approver, bindingHash, approvalId);
    if (!self) {
        if (session) session->revokeStartApprovalAuthentication(approvalId);
        return QVariantMap();
    }
    const QVariantMap authentication = result.value(QStringLiteral("authenticationReceipt")).toMap();
    double duration = 0;
    if (!_same(generation, QStringLiteral("approved")) || !session || _session != session || _approvalId != approvalId
        || _review != review || _deployment != deployment
        || !outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())
        || _bindingHash != bindingHash || _identityReceipt != identity || _approverReceipt != approver
        || !isTrue(result.value(QStringLiteral("ok"))) || result.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map
        || !integer(authentication.value(QStringLiteral("durationMs")), 1, ApprovalLifetimeMs, &duration)) {
        if (session) session->revokeStartApprovalAuthentication(approvalId);
        _invalidate(generation, QStringLiteral("conditional-start-authentication-not-consumed"), QStringLiteral("failed"), true);
        return QVariantMap();
    }
    const bool authenticationCurrent = session->startApprovalAuthenticationCurrent(authentication);
    if (!self) {
        if (session) session->revokeStartApprovalAuthentication(approvalId);
        return QVariantMap();
    }
    if (!authenticationCurrent || !_same(generation, QStringLiteral("approved")) || !session || _session != session
        || _review != review || _deployment != deployment
        || !outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())
        || _approvalId != approvalId || _identityReceipt != identity || _approverReceipt != approver) {
        if (session) session->revokeStartApprovalAuthentication(approvalId);
        _invalidate(generation, QStringLiteral("conditional-start-authentication-changed-before-commit"), QStringLiteral("failed"), true);
        return QVariantMap();
    }
    // This only SHORTENS the frozen original risk-review deadline. It cannot
    // renew either actor's identity or create an upload/execution lease.
    _expiresAtMs = qMin(_expiresAtMs, consumedAt + static_cast<qint64>(duration));
    _authenticationReceipt = authentication; _state = QStringLiteral("consumed");
    _grant = {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("grantId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("startupId"), startupId}, {QStringLiteral("approvalReceipt"), _approvalReceipt},
        {QStringLiteral("authenticationReceipt"), authentication}, {QStringLiteral("consumedAtMonotonicMs"), static_cast<double>(consumedAt)},
        {QStringLiteral("expiresAtMonotonicMs"), static_cast<double>(_expiresAtMs)}, {QStringLiteral("reviewContextHash"), _contextHash},
        {QStringLiteral("purpose"), Purpose}, {QStringLiteral("conditionalIntentApproved"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (!grantShape(_grant) || !_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("conditional-start-grant-context-invalid"), QStringLiteral("failed"), true);
        return QVariantMap();
    }
    const QVariantMap grant = _grant;
    QTimer::singleShot(0, this, [self, generation, grant]() {
        if (self && self->_same(generation, QStringLiteral("consumed")) && self->_grant == grant) emit self->changed();
    });
    return grant;
}

bool FleetMissionStartApproval::grantCurrent(const QVariantMap& suppliedGrant, const QString& suppliedStartupId)
{
    const QVariantMap grant = suppliedGrant;
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _busy || _checking || !uuid(startupId)
        || !outboundSourcesCurrent(this, _session.data(), _review.data(), _deployment.data())
        || !_same(_generation, QStringLiteral("consumed")) || !grantShape(grant) || grant != _grant
        || grant.value(QStringLiteral("startupId")).toString() != startupId) return false;
    _busy = true;
    FlagGuard checkingGrant{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartApproval> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const quint64 generation = _generation;
    const bool authenticationCurrent = session && session->startApprovalAuthenticationCurrent(grant.value(QStringLiteral("authenticationReceipt")).toMap());
    return self && authenticationCurrent && session && _session == session && _same(generation, QStringLiteral("consumed"))
        && grant == _grant && _sourcesCurrent(generation, true) && self && _same(generation, QStringLiteral("consumed"));
}

QVariantMap FleetMissionStartApproval::transferToStartup(QObject* authorityObject, const QVariantMap& suppliedGrant,
    const QString& suppliedStartupId, const QString& suppliedSourceHash, qint64 maximumRemainingMs)
{
    // Borrowed QObject member arguments can disappear in WTS/owner callbacks.
    const QVariantMap grant = suppliedGrant;
    const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QPointer<FleetMissionStartApproval> self(this);
    const QPointer<FleetMissionStartupAuthority> authority(qobject_cast<FleetMissionStartupAuthority*>(authorityObject));
    if (thread() != QThread::currentThread() || _busy || _checking || !authority || authority->thread() != thread()
        || !outboundSourcesCurrent(this, _session.data(), _review.data(), _deployment.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !uuid(startupId) || !digest(sourceHash) || maximumRemainingMs < 1 || maximumRemainingMs > ReviewMaximumMs
        || !_same(_generation, QStringLiteral("consumed")) || !grantShape(grant) || grant != _grant
        || grant.value(QStringLiteral("startupId")).toString() != startupId)
        return rejection(QStringLiteral("actual-consumed-start-grant-and-authority-required"));
    const QString authorityEpoch = authority->authorityEpoch();
    if (!uuid(authorityEpoch)) return rejection(QStringLiteral("actual-startup-authority-epoch-required"));
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetMissionStartReview> review(_review);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QVariantMap identity = _identityReceipt, authentication = _authenticationReceipt;
    const QString approvalId = _approvalId;
    // ENTRY budget includes all grant, owner, SAM and WTS checks below. It can
    // only shorten the actual old Review deadline, never create a new window.
    const qint64 entered = _clock.elapsed();
    const qint64 deadline = qMin(_expiresAtMs, entered + maximumRemainingMs);
    if (!grantCurrent(grant, startupId) || !self || !session || !authority)
        return rejection(QStringLiteral("conditional-start-grant-not-current-before-handoff"));
    _busy = true;
    FlagGuard transferring{QPointer<QObject>(this), &_busy};
    const auto unchanged = [&]() {
        return self && session && authority && _session == session && _same(generation, QStringLiteral("consumed"))
            && _review == review && _deployment == deployment
            && outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), authority.data())
            && _approvalId == approvalId && _grant == grant && _identityReceipt == identity && _authenticationReceipt == authentication
            && authority->thread() == thread() && authority->authorityEpoch() == authorityEpoch && _clock.elapsed() < deadline;
    };
    if (!unchanged() || !_sourcesCurrent(generation, true) || !unchanged()
        || !authority->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), grant) || !unchanged()
        || !_sourcesCurrent(generation, true) || !unchanged())
        return rejection(QStringLiteral("actual-startup-handoff-source-context-not-current"));
    const qint64 beforeTransfer = _clock.elapsed();
    const qint64 remainingBefore = deadline - beforeTransfer;
    if (remainingBefore < 1 || remainingBefore > ReviewMaximumMs || !unchanged())
        return rejection(QStringLiteral("original-start-review-budget-expired"));
    const QVariantMap result = session->transferStartApprovalAuthentication(identity, authentication, authority.data(),
        startupId, authorityEpoch, sourceHash, int(remainingBefore));
    const bool moved = isTrue(result.value(QStringLiteral("ok")));
    const QVariantMap receipt = result.value(QStringLiteral("authenticationReceipt")).toMap();
    const auto revokeMoved = [&]() {
        if (moved && session && authority) session->revokeStartupAuthentication(authority.data(), startupId, authorityEpoch);
    };
    double duration = 0;
    if (!unchanged() || !moved || result.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map
        || !integer(receipt.value(QStringLiteral("durationMs")), 1, ReviewMaximumMs, &duration)
        || !isFalse(result.value(QStringLiteral("startupPermitGranted"))) || !isFalse(result.value(QStringLiteral("executable")))
        || !isFalse(result.value(QStringLiteral("flightCommandReleased")))) {
        revokeMoved();
        if (self && moved) _invalidate(generation, QStringLiteral("startup-authentication-handoff-not-committed"), QStringLiteral("failed"), true);
        return rejection(QStringLiteral("startup-authentication-handoff-not-committed"));
    }
    // The actual Session has retired the old account proof. Detach its old
    // Review/Content watchers BEFORE new-owner callbacks; expected phase changes
    // can now only be adjudicated by the Authority's independent actual gate.
    _state = QStringLiteral("transferred"); _reason.clear(); _timer.stop(); _disconnectSources();
    _authenticationReceipt.clear(); _grant.clear();
    _transferredAuthority = authority; _transferredStartupId = startupId;
    _transferredAuthorityEpoch = authorityEpoch; _transferredSourceHash = sourceHash;
    const auto transferred = [&]() {
        return self && session && authority && _generation == generation && _state == QStringLiteral("transferred")
            && _review == review && _deployment == deployment
            && outboundSourcesCurrent(self.data(), session.data(), review.data(), deployment.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), authority.data())
            && _session == session && _transferredAuthority == authority && _transferredStartupId == startupId
            && _transferredAuthorityEpoch == authorityEpoch && _transferredSourceHash == sourceHash
            && _authenticationReceipt.isEmpty() && _grant.isEmpty() && _clock.elapsed() < deadline;
    };
    const bool nativeCurrent = session->startupAuthenticationCurrent(receipt, authority.data(), startupId, authorityEpoch, sourceHash);
    const bool ownerCurrent = transferred() && nativeCurrent
        && authority->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), grant) && transferred();
    // Do not compare unrelated clock origins. Subtract the COMPLETE entry-call
    // overhead from the Session duration and cap at our unchanged old deadline.
    const qint64 now = self ? _clock.elapsed() : deadline;
    const qint64 remaining = qMin(deadline - now, qint64(duration) - (now - entered));
    if (!ownerCurrent || !transferred() || remaining < 1) {
        revokeMoved();
        if (self && _generation == generation && _state == QStringLiteral("transferred")
            && _transferredStartupId == startupId && _transferredAuthority == authority) {
            _state = QStringLiteral("failed"); _reason = QStringLiteral("startup-owner-post-handoff-gate-invalid");
            QTimer::singleShot(0, this, [self, generation]() {
                if (self && self->_generation == generation && self->_state == QStringLiteral("failed")) emit self->changed();
            });
        }
        return rejection(QStringLiteral("startup-owner-post-handoff-gate-invalid"));
    }
    QTimer::singleShot(0, this, [self, generation, startupId, authorityEpoch]() {
        if (self && self->_generation == generation && self->_state == QStringLiteral("transferred")
            && self->_transferredStartupId == startupId && self->_transferredAuthorityEpoch == authorityEpoch) emit self->changed();
    });
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), receipt},
        {QStringLiteral("startupId"), startupId}, {QStringLiteral("authorityEpoch"), authorityEpoch},
        {QStringLiteral("sourceHash"), sourceHash}, {QStringLiteral("remainingMs"), double(remaining)},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetMissionStartApproval::revokeCurrent(const QString& suppliedApprovalId)
{
    const QString approvalId = suppliedApprovalId;
    if (thread() != QThread::currentThread() || !uuid(approvalId) || approvalId != _approvalId) return false;
    if (_state == QStringLiteral("transferred")) {
        if (_busy || _checking) return false;
        _busy = true;
        FlagGuard forwarding{QPointer<QObject>(this), &_busy};
        const QPointer<FleetMissionStartApproval> self(this);
        const QPointer<FleetMissionStartupAuthority> authority(_transferredAuthority);
        const quint64 generation = _generation;
        const QString startupId = _transferredStartupId, epoch = _transferredAuthorityEpoch, sourceHash = _transferredSourceHash;
        const auto same = [&]() {
            return self && _generation == generation && _state == QStringLiteral("transferred") && _approvalId == approvalId
                && _transferredStartupId == startupId && _transferredAuthorityEpoch == epoch && _transferredSourceHash == sourceHash
                && _transferredAuthority == authority && _grant.isEmpty() && _authenticationReceipt.isEmpty();
        };
        bool retired = !authority; // Destroyed actual owner cannot retain its Session proof.
        if (authority) {
            const auto exact = [&](const QVariantMap& status) {
                return same() && authority && authority->thread() == thread() && authority->authorityEpoch() == epoch
                    && status.value(QStringLiteral("startupId")).type() == QVariant::String
                    && status.value(QStringLiteral("startupId")).toString() == startupId
                    && status.value(QStringLiteral("authorityEpoch")).type() == QVariant::String
                    && status.value(QStringLiteral("authorityEpoch")).toString() == epoch
                    && status.value(QStringLiteral("sourceHash")).type() == QVariant::String
                    && status.value(QStringLiteral("sourceHash")).toString() == sourceHash;
            };
            const auto terminal = [&](const QVariantMap& status) {
                const QString state = status.value(QStringLiteral("state")).toString();
                return exact(status) && status.value(QStringLiteral("state")).type() == QVariant::String
                    && (state == QStringLiteral("revoked") || state == QStringLiteral("expired")
                        || state == QStringLiteral("stale") || state == QStringLiteral("failed"))
                    && isFalse(status.value(QStringLiteral("reservationHeld")));
            };
            const QVariantMap before = authority->current();
            if (!same()) return false;
            if (exact(before)) {
                if (terminal(before)) retired = true;
                else {
                    // Explicit exact forwarding only; no automatic old Review
                    // event can cancel a transferred authentication basis.
                    authority->revokeAuthority(startupId);
                    if (!same()) return false;
                    if (!authority) retired = true;
                    else retired = terminal(authority->current());
                }
            }
        }
        if (!same()) return false;
        if (!retired) {
            _reason = QStringLiteral("conditional-authentication-transferred-use-startup-cancel");
            emit changed();
            return false;
        }
        _state = QStringLiteral("revoked"); _reason = QStringLiteral("transferred-startup-authority-retired");
        // Never revive/clear another owner's proof or manufacture a new grant.
        emit changed();
        return true;
    }
    if (_state != QStringLiteral("pending") && _state != QStringLiteral("approved") && _state != QStringLiteral("consumed")) return false;
    _invalidate(_generation, QStringLiteral("operator-revoked-conditional-start-intent"), QStringLiteral("revoked"));
    return true;
}

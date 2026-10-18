#include "FleetOperationApproval.h"
#include "FleetMissionPlanStore.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "FleetMissionDeploymentEpisode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 ApprovalLifetimeMs = 60000;
constexpr qint64 UploadLeaseMaximumMs = 240000;
constexpr double MaxExactNumber = 9007199254740991.0;
constexpr qint64 MaxRecordBytes = 32LL * 1024 * 1024;
constexpr int MaxRecordNodes = 2000000;

struct FlagGuard {
    QPointer<QObject> object;
    bool* flag;
    ~FlagGuard() { if (object) *flag = false; }
};

bool strictTrue(const QVariant& value)
{
    return value.type() == QVariant::Bool && value.toBool();
}

bool strictFalse(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.toBool();
}

bool text(const QVariant& value, int limit)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > limit || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool deploymentIdentifier(const QString& value)
{
    if (value.size() != 36) return false;
    const QUuid uuid(value);
    return !uuid.isNull() && uuid.toString(QUuid::WithoutBraces) == value;
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
    if (!std::isfinite(number) || number < minimum || number > maximum
        || std::floor(number) != number) return false;
    *result = number;
    return true;
}

bool canonicalBinding(const QVariantMap& input, QVariantMap* output)
{
    const QSet<QString> textKeys = {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"),
        QStringLiteral("storeEpoch")};
    const QSet<QString> hashes = {QStringLiteral("planId"), QStringLiteral("contentHash"),
        QStringLiteral("reviewEvidenceHash")};
    const QSet<QString> numberKeys = {QStringLiteral("recordRevision"),
        QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")};
    if ((input.size() != 7 && input.size() != 9)
        || input.contains(QStringLiteral("reviewGeneration")) != input.contains(QStringLiteral("inputGeneration"))) return false;
    QVariantMap canonical;
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (textKeys.contains(it.key())) {
            if (!text(it.value(), 128) || (hashes.contains(it.key()) && !digest(it.value()))) return false;
            canonical.insert(it.key(), it.value());
        } else if (numberKeys.contains(it.key())) {
            double number = 0;
            if (!integer(it.value(), it.key() == QStringLiteral("recordRevision") ? 1.0 : 0.0,
                MaxExactNumber, &number)) return false;
            canonical.insert(it.key(), number); // Stable across the QML numeric boundary.
        } else return false;
    }
    for (const QString& key : textKeys) if (!canonical.contains(key)) return false;
    if (!canonical.contains(QStringLiteral("recordRevision"))) return false;
    *output = canonical;
    return true;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 18 || ++(*nodes) > MaxRecordNodes) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += static_cast<qint64>(it.key().size()) * 2;
            if (*bytes > MaxRecordBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList())
            if (!boundedTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList())
            if (!boundedTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::String:
        *bytes += static_cast<qint64>(value.toString().size()) * 2;
        return *bytes <= MaxRecordBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong:
        return std::abs(value.toDouble()) <= MaxExactNumber;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}

bool recordBytes(const QVariantMap& record, QByteArray* encoded)
{
    int nodes = 0;
    qint64 bytes = 0;
    if (record.isEmpty() || !boundedTree(record, 0, &nodes, &bytes)) return false;
    *encoded = QJsonDocument::fromVariant(record).toJson(QJsonDocument::Compact);
    return !encoded->isEmpty() && encoded->size() <= MaxRecordBytes;
}

bool uploadRole(const QVariantMap& identity, const QVariantMap& role)
{
    const QString principalRole = identity.value(QStringLiteral("role")).toString();
    return (principalRole == QStringLiteral("operator") || principalRole == QStringLiteral("commander")
            || principalRole == QStringLiteral("administrator"))
        && role.value(QStringLiteral("actorId")).type() == QVariant::String
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")).type() == QVariant::String
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload")
        && strictTrue(role.value(QStringLiteral("allowed")))
        && strictTrue(role.value(QStringLiteral("requiresSecondApproval")))
        && strictFalse(role.value(QStringLiteral("actorAuthenticated")))
        && strictFalse(role.value(QStringLiteral("executionPermitted")))
        && strictTrue(identity.value(QStringLiteral("authenticated")));
}

bool receiptShape(const QVariantMap& receipt)
{
    double schema = 0;
    if (receipt.size() != 22 || !integer(receipt.value(QStringLiteral("schemaVersion")), 2.0, 2.0, &schema))
        return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approvalEpoch"),
            QStringLiteral("approvalGeneration"), QStringLiteral("action"), QStringLiteral("bindingHash"),
            QStringLiteral("actorId"), QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionEpoch"),
            QStringLiteral("applicantSessionRevision"), QStringLiteral("approverId"), QStringLiteral("approverRole"),
            QStringLiteral("rolePolicyEpoch"), QStringLiteral("rolePolicyRevision")})
        if (!text(receipt.value(key), 128)) return false;
    for (const QString& key : {QStringLiteral("approvedAtMonotonicMs"), QStringLiteral("expiresAtMonotonicMs")}) {
        double value = 0;
        if (!integer(receipt.value(key), 0.0, MaxExactNumber, &value)) return false;
    }
    QVariantMap binding;
    double maximum = 0;
    return receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload")
        && digest(receipt.value(QStringLiteral("bindingHash")))
        && receipt.value(QStringLiteral("binding")).type() == QVariant::Map
        && canonicalBinding(receipt.value(QStringLiteral("binding")).toMap(), &binding)
        && integer(receipt.value(QStringLiteral("uploadLeaseMaximumMs")),
            UploadLeaseMaximumMs, UploadLeaseMaximumMs, &maximum)
        && strictTrue(receipt.value(QStringLiteral("uploadLeaseRequiresCurrentOperator")))
        && strictTrue(receipt.value(QStringLiteral("oneUse")))
        && strictFalse(receipt.value(QStringLiteral("startupPermitGranted")))
        && strictFalse(receipt.value(QStringLiteral("flightCommandReleased")));
}

bool grantShape(const QVariantMap& grant)
{
    double schema = 0;
    double consumed = 0;
    double expires = 0;
    double maximum = 0;
    double duration = 0;
    return grant.size() == 13 && integer(grant.value(QStringLiteral("schemaVersion")), 2.0, 2.0, &schema)
        && text(grant.value(QStringLiteral("grantId")), 64)
        && grant.value(QStringLiteral("approvalReceipt")).type() == QVariant::Map
        && receiptShape(grant.value(QStringLiteral("approvalReceipt")).toMap())
        && integer(grant.value(QStringLiteral("consumedAtMonotonicMs")), 0, MaxExactNumber, &consumed)
        && grant.value(QStringLiteral("deploymentId")).type() == QVariant::String
        && deploymentIdentifier(grant.value(QStringLiteral("deploymentId")).toString())
        && grant.value(QStringLiteral("identityLease")).type() == QVariant::Map
        && integer(grant.value(QStringLiteral("leaseExpiresAtMonotonicMs")), 0, MaxExactNumber, &expires)
        && integer(grant.value(QStringLiteral("leaseMaximumMs")), UploadLeaseMaximumMs, UploadLeaseMaximumMs, &maximum)
        && integer(grant.value(QStringLiteral("leaseDurationMs")), 1, UploadLeaseMaximumMs, &duration)
        && expires - consumed == duration
        && grant.value(QStringLiteral("purpose")).type() == QVariant::String
        && grant.value(QStringLiteral("purpose")).toString() == QStringLiteral("grounded-mission-upload-and-readback")
        && strictTrue(grant.value(QStringLiteral("oneUseConsumed")))
        && strictFalse(grant.value(QStringLiteral("startupPermitGranted")))
        && strictFalse(grant.value(QStringLiteral("flightCommandReleased")));
}

QVariantMap rejected(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("approvalReceipt"), QVariantMap()}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

FleetOperationApproval::FleetOperationApproval(QObject* parent)
    : QObject(parent), _approvalEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _expiryTimer.setInterval(1000);
    connect(&_expiryTimer, &QTimer::timeout, this, [this]() {
        if (_expired()) _invalidate(_generation, _state == QStringLiteral("consumed")
            ? QStringLiteral("upload-lease-expired") : QStringLiteral("approval-expired"), QStringLiteral("expired"));
    });
}

FleetOperationApproval::~FleetOperationApproval() { _disconnectSources(); }

bool FleetOperationApproval::_expired() const
{
    if (_state == QStringLiteral("consumed"))
        return _leaseConsumedAtMs < 0 || _leaseExpiresAtMs <= _leaseConsumedAtMs
            || !_clock.isValid() || _clock.elapsed() >= _leaseExpiresAtMs;
    return _createdAtMs < 0 || !_clock.isValid() || _clock.elapsed() - _createdAtMs >= ApprovalLifetimeMs;
}

QVariantMap FleetOperationApproval::current() const
{
    const bool active = _state == QStringLiteral("pending") || _state == QStringLiteral("approved")
        || _state == QStringLiteral("consumed");
    const bool expired = active && _expired();
    const QVariantMap uploadLease = _grant.isEmpty() ? QVariantMap() : QVariantMap{
        {QStringLiteral("deploymentId"), _grant.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("grantId"), _grant.value(QStringLiteral("grantId"))},
        {QStringLiteral("maximumMs"), static_cast<double>(UploadLeaseMaximumMs)},
        {QStringLiteral("durationMs"), _grant.value(QStringLiteral("leaseDurationMs"))},
        {QStringLiteral("expiresAtMonotonicMs"), static_cast<double>(_leaseExpiresAtMs)},
        {QStringLiteral("remainingMs"), _state == QStringLiteral("consumed") && !expired
            ? static_cast<double>(_leaseExpiresAtMs - _clock.elapsed()) : 0.0},
        {QStringLiteral("scope"), QStringLiteral("grounded-upload-readback")},
        {QStringLiteral("renewed"), false}};
    return {{QStringLiteral("ok"), _state == QStringLiteral("approved") && !expired},
        {QStringLiteral("approvalId"), _approvalId}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("state"), expired ? QStringLiteral("expired") : _state},
        {QStringLiteral("reason"), expired ? (_state == QStringLiteral("consumed")
            ? QStringLiteral("upload-lease-expired") : QStringLiteral("approval-expired")) : _reason},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("approvalReceipt"), _approvalReceipt},
        {QStringLiteral("actorId"), _identityReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverId"), _approverReceipt.value(QStringLiteral("actorId"))},
        {QStringLiteral("targetVehicleIds"), _targetVehicleIds},
        {QStringLiteral("remainingMs"), active && !expired ? static_cast<double>(
            _state == QStringLiteral("consumed") ? _leaseExpiresAtMs - _clock.elapsed()
                : ApprovalLifetimeMs - (_clock.elapsed() - _createdAtMs)) : 0.0},
        {QStringLiteral("approvalRequestRemainingMs"), _createdAtMs >= 0 && _clock.isValid()
            ? static_cast<double>(qMax<qint64>(0, ApprovalLifetimeMs - (_clock.elapsed() - _createdAtMs))) : 0.0},
        {QStringLiteral("uploadLease"), uploadLease},
        {QStringLiteral("uploadLeaseMaximumMs"), static_cast<double>(UploadLeaseMaximumMs)},
        {QStringLiteral("uploadLeaseRemainingMs"), _state == QStringLiteral("consumed") && !expired
            ? static_cast<double>(_leaseExpiresAtMs - _clock.elapsed()) : 0.0},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("oneUseConsumed"), _state == QStringLiteral("consumed")},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperationApproval::_same(quint64 generation, const QString& state) const
{
    return generation != 0 && generation == _generation && _state == state && !_expired();
}

void FleetOperationApproval::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}

void FleetOperationApproval::_invalidate(quint64 generation, const QString& reason,
    const QString& state, bool deferNotification)
{
    if (generation != _generation || (_state != QStringLiteral("pending")
        && _state != QStringLiteral("approved") && _state != QStringLiteral("consumed"))) return;
    if (_session && !_identityLease.isEmpty())
        _session->revokeUploadIdentityLease(_identityLease.value(QStringLiteral("leaseId")).toString());
    _state = state;
    _reason = reason;
    _expiryTimer.stop();
    _disconnectSources();
    if (deferNotification) {
        const QPointer<FleetOperationApproval> self(this);
        const QVariantMap invalidatedGrant = _grant;
        QTimer::singleShot(0, this, [self, generation, state, invalidatedGrant]() {
            if (self && self->_generation == generation && self->_state == state
                && self->_grant == invalidatedGrant) emit self->changed();
        });
        return;
    }
    emit changed(); // No member access after a callback which may destroy this object.
}

void FleetOperationApproval::_connectSources(quint64 generation)
{
    const auto invalidate = [this, generation]() {
        _invalidate(generation, QStringLiteral("approval-source-changed-or-destroyed"));
    };
    for (QObject* source : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()),
            static_cast<QObject*>(_store.data()), _authorization.data()})
        _connections.append(connect(source, &QObject::destroyed, this, invalidate));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        const QPointer<FleetOperationApproval> self(this);
        // Authentication's native modal announces busy before/after prompting.
        // Its primary receipt is recaptured after it returns. Logout/replacement
        // inside that modal cannot survive the mandatory post-modal check.
        if (_requestInProgress && _state == QStringLiteral("pending")) return;
        // Clearing the historical 60-second approver receipt is not revocation
        // of an already consumed finite upload lease. Query the actual native
        // lease and local role; logout/new identity/role loss still fails closed.
        if (_state == QStringLiteral("consumed") && !_expired() && _session && _roles) {
            const QPointer<FleetOperatorSession> session(_session);
            const QPointer<FleetRolePolicy> roles(_roles);
            const QVariantMap identityLease = _identityLease;
            const QVariantMap role = _roleReceipt;
            const bool leaseCurrent = session->uploadIdentityLeaseCurrent(identityLease);
            // Windows freshness queries can dispatch sent native messages,
            // including lock/logout notifications which revoke or delete us.
            if (!self) return;
            if (leaseCurrent && _same(generation, QStringLiteral("consumed")) && session && roles
                && _session == session && _roles == roles && _identityLease == identityLease
                && _roleReceipt == role && roles->actionSnapshotCurrent(role)) return;
        }
        if (self) _invalidate(generation, QStringLiteral("authenticated-session-changed"));
    }));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, invalidate));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, invalidate));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, invalidate));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, invalidate));
}

bool FleetOperationApproval::_sourcesCurrent(quint64 generation, bool requireApprover)
{
    if (thread() != QThread::currentThread() || _checking) {
        _reentered = true;
        return false;
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, _session.data())) return false;
    _checking = true;
    _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    QPointer<FleetOperationApproval> self(this);
    const QString state = _state;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap binding = _binding;
    const QVariantMap role = _roleReceipt;
    const QVariantMap identity = _identityReceipt;
    const QVariantMap approver = _approverReceipt;
    const QVariantMap identityLease = _identityLease;
    const QVariantMap grant = _grant;
    const qint64 leaseConsumedAt = _leaseConsumedAtMs;
    const qint64 leaseExpiresAt = _leaseExpiresAtMs;
    const QByteArray record = _recordBytes;
    const QString bindingHash = _bindingHash;
    const auto unchanged = [&]() {
        return self && !_reentered && _same(generation, state) && session && roles && store && owner
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data())
            && _session == session && _roles == roles && _store == store && _authorization == owner
            && _binding == binding && _roleReceipt == role && _identityReceipt == identity
            && _approverReceipt == approver && _recordBytes == record && _bindingHash == bindingHash
            && _identityLease == identityLease && _grant == grant
            && _leaseConsumedAtMs == leaseConsumedAt && _leaseExpiresAtMs == leaseExpiresAt
            && session->thread() == thread() && roles->thread() == thread()
            && store->thread() == thread() && owner->thread() == thread();
    };
    const auto nativeCurrent = [&]() {
        if (!unchanged() || !roles->actionSnapshotCurrent(role) || !uploadRole(identity, role)) return false;
        if (state == QStringLiteral("consumed")) {
            if (!session->uploadIdentityLeaseCurrent(identityLease) || !unchanged()) return false;
        } else if (!session->identitySnapshotCurrent(identity) || !unchanged()) return false;
        if (!store->matchesBinding(binding)) return false;
        QByteArray actual;
        if (!recordBytes(store->record(binding.value(QStringLiteral("sourceTaskId")).toString(),
                binding.value(QStringLiteral("recordRevision"))), &actual) || actual != record || !unchanged()) return false;
        if (!requireApprover) return true;
        if (state == QStringLiteral("consumed")) {
            return grantShape(grant) && identityLease == grant.value(QStringLiteral("identityLease")).toMap()
                && identityLease.value(QStringLiteral("leaseId")) == grant.value(QStringLiteral("grantId"))
                && identityLease.value(QStringLiteral("sessionEpoch")) == identity.value(QStringLiteral("sessionEpoch"))
                && identityLease.value(QStringLiteral("sessionRevision")) == identity.value(QStringLiteral("sessionRevision"))
                && identityLease.value(QStringLiteral("primarySessionId")) == identity.value(QStringLiteral("sessionId"))
                && identityLease.value(QStringLiteral("primaryActorId")) == identity.value(QStringLiteral("actorId"))
                && identityLease.value(QStringLiteral("primaryRole")) == identity.value(QStringLiteral("role"))
                && identityLease.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
                && identityLease.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
                && identityLease.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
                && identityLease.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload")
                && identityLease.value(QStringLiteral("bindingHash")).toString() == bindingHash;
        }
        if (!session->approverSnapshotCurrent(approver) || !unchanged()) return false;
        const QString approverRole = approver.value(QStringLiteral("role")).toString();
        return strictTrue(approver.value(QStringLiteral("authenticated")))
            && approver.value(QStringLiteral("actorId")).type() == QVariant::String
            && approver.value(QStringLiteral("actorId")) != identity.value(QStringLiteral("actorId"))
            && (approverRole == QStringLiteral("safety_officer") || approverRole == QStringLiteral("commander")
                || approverRole == QStringLiteral("administrator"))
            && approver.value(QStringLiteral("action")).type() == QVariant::String
            && approver.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload")
            && approver.value(QStringLiteral("bindingHash")).type() == QVariant::String
            && approver.value(QStringLiteral("bindingHash")).toString() == bindingHash
            && approver.value(QStringLiteral("applicantActorId")) == identity.value(QStringLiteral("actorId"))
            && approver.value(QStringLiteral("applicantSessionId")) == identity.value(QStringLiteral("sessionId"))
            && approver.value(QStringLiteral("applicantSessionRevision")) == identity.value(QStringLiteral("sessionRevision"));
    };
    if (!nativeCurrent()) return false;
    QVariant sourceMatches;
    const bool sourceInvoked = QMetaObject::invokeMethod(owner.data(), "fleetOperationApprovalSourcesMatch",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, sourceMatches),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(store.data()))));
    if (!unchanged() || !sourceInvoked || !strictTrue(sourceMatches) || !nativeCurrent()) return false;
    QVariant bindingAllowed;
    const bool bindingInvoked = QMetaObject::invokeMethod(owner.data(), "fleetDeploymentBindingAllowed",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, bindingAllowed),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(binding)));
    return unchanged() && bindingInvoked && strictTrue(bindingAllowed) && nativeCurrent();
}

QVariantMap FleetOperationApproval::requestUploadApproval(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding)
{
    if (thread() != QThread::currentThread() || _requestInProgress || _checking) {
        _reentered = true;
        return rejected(QStringLiteral("approval-owner-thread-or-reentrancy-blocked"));
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return rejected(QStringLiteral("current-native-deployment-episode-required"));
    if (_state == QStringLiteral("consumed") && !_expired())
        return rejected(QStringLiteral("consumed-upload-approval-busy-revoke-first"));
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(sessionObject);
    QPointer<FleetRolePolicy> roles = qobject_cast<FleetRolePolicy*>(roleObject);
    QPointer<FleetMissionPlanStore> store = qobject_cast<FleetMissionPlanStore*>(storeObject);
    QPointer<QObject> owner(authorizationOwner);
    QVariantMap canonical;
    if (!session || !roles || !store || !owner || session->thread() != thread() || roles->thread() != thread()
        || store->thread() != thread() || owner->thread() != thread() || !canonicalBinding(binding, &canonical))
        return rejected(QStringLiteral("actual-native-sources-and-bounded-binding-required"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, session.data()))
        return rejected(QStringLiteral("same-current-native-session-episode-required"));
    if (_generation == std::numeric_limits<quint64>::max()) return rejected(QStringLiteral("approval-generation-exhausted"));
    _requestInProgress = true;
    FlagGuard requesting{QPointer<QObject>(this), &_requestInProgress};
    QPointer<FleetOperationApproval> self(this);
    const QVariantMap identity = session->identitySnapshot();
    if (!self || !session || !roles || !store || !owner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data()))
        return rejected(QStringLiteral("approval-source-destroyed-or-episode-changed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.upload"));
    if (!session->identitySnapshotCurrent(identity) || !self || !session || !roles || !store || !owner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data())
        || !roles->actionSnapshotCurrent(role) || !uploadRole(identity, role))
        return rejected(QStringLiteral("authenticated-upload-actor-and-two-person-policy-required"));
    QByteArray encoded;
    const QVariantMap record = store->record(canonical.value(QStringLiteral("sourceTaskId")).toString(),
        canonical.value(QStringLiteral("recordRevision")));
    if (!store->matchesBinding(canonical) || !recordBytes(record, &encoded))
        return rejected(QStringLiteral("current-unrestored-approved-record-required"));
    QVariantList targets;
    QSet<int> seen;
    if (record.value(QStringLiteral("plan")).type() != QVariant::Map) return rejected(QStringLiteral("native-plan-missing"));
    const QVariantMap plan = record.value(QStringLiteral("plan")).toMap();
    if (plan.value(QStringLiteral("members")).type() != QVariant::List) return rejected(QStringLiteral("native-plan-members-missing"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.isEmpty() || members.size() > 32) return rejected(QStringLiteral("native-target-count-invalid"));
    for (const QVariant& member : members) {
        double number = 0;
        if (member.type() != QVariant::Map || !integer(member.toMap().value(QStringLiteral("vehicleId")), 1, 255, &number)
            || seen.contains(static_cast<int>(number))) return rejected(QStringLiteral("native-target-identity-invalid"));
        seen.insert(static_cast<int>(number));
        targets.append(number);
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data()))
        return rejected(QStringLiteral("approval-episode-changed-before-state-replacement"));
    _disconnectSources();
    _expiryTimer.stop();
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _authorization = owner;
    _binding = canonical; _roleReceipt = role; _identityReceipt = identity;
    _recordBytes = encoded; _targetVehicleIds = targets;
    _approverReceipt.clear(); _identityLease.clear(); _approvalReceipt.clear(); _grant.clear();
    _leaseConsumedAtMs = -1; _leaseExpiresAtMs = -1;
    _approvalId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _bindingHash = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(QVariantMap{{QStringLiteral("binding"), canonical},
            {QStringLiteral("record"), record}, {QStringLiteral("approvalSchemaVersion"), 2.0},
            {QStringLiteral("uploadLeaseMaximumMs"), static_cast<double>(UploadLeaseMaximumMs)},
            {QStringLiteral("uploadLeaseRequiresCurrentOperator"), true},
            {QStringLiteral("purpose"), QStringLiteral("grounded-mission-upload-and-readback")}})
            .toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    _state = QStringLiteral("pending"); _reason.clear(); _createdAtMs = _clock.elapsed();
    _connectSources(generation);
    _expiryTimer.start();
    if (!_sourcesCurrent(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("approval-context-invalid-before-authentication"));
        return rejected(QStringLiteral("approval-context-invalid-before-authentication"));
    }
    emit changed();
    if (!self || !_same(generation, QStringLiteral("pending")) || !_sourcesCurrent(generation, false))
        return rejected(QStringLiteral("approval-request-revoked-before-authentication"));
    // Only this actual native session may acquire another independently
    // authenticated principal. No caller-supplied signed/authenticated flag.
    QStringList targetText;
    for (const QVariant& target : targets) targetText.append(QString::number(target.toInt()));
    const QString operationSummary = QStringLiteral(
        "Record: %1\nRevision: %2\nPlan SHA-256: %3\nTarget vehicle IDs: %4\nScope: grounded overwrite + readback; NO ARM/START.\nLease: <=4 min; ORIGINAL operator deadline cap; current operator required; never renewed.")
        .arg(canonical.value(QStringLiteral("recordId")).toString())
        .arg(QString::number(canonical.value(QStringLiteral("recordRevision")).toDouble(), 'f', 0))
        .arg(canonical.value(QStringLiteral("planId")).toString())
        .arg(targetText.join(QStringLiteral(", ")));
    const QVariantMap authentication = session->authenticateApprover(QStringLiteral("mission.upload"),
        _bindingHash, identity, operationSummary);
    if (!self) return rejected(QStringLiteral("approval-owner-destroyed-during-authentication"));
    if (!_same(generation, QStringLiteral("pending")) || !session || !roles || !store || !owner
        || _session != session || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data()))
        return rejected(QStringLiteral("approval-request-revoked-or-expired-during-authentication"));
    if (!strictTrue(authentication.value(QStringLiteral("ok")))
        || authentication.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) {
        _invalidate(generation, QStringLiteral("independent-approver-authentication-not-granted"));
        return rejected(QStringLiteral("independent-approver-authentication-not-granted"));
    }
    const QVariantMap approver = authentication.value(QStringLiteral("identityReceipt")).toMap();
    _approverReceipt = approver;
    if (!_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("independent-approver-or-post-modal-context-invalid"));
        return rejected(QStringLiteral("independent-approver-or-post-modal-context-invalid"));
    }
    _approvalReceipt = {{QStringLiteral("schemaVersion"), 2.0}, {QStringLiteral("approvalId"), _approvalId},
        {QStringLiteral("approvalEpoch"), _approvalEpoch}, {QStringLiteral("approvalGeneration"), QString::number(generation)},
        {QStringLiteral("action"), QStringLiteral("mission.upload")}, {QStringLiteral("bindingHash"), _bindingHash},
        {QStringLiteral("actorId"), identity.value(QStringLiteral("actorId"))},
        {QStringLiteral("applicantSessionId"), identity.value(QStringLiteral("sessionId"))},
        {QStringLiteral("applicantSessionEpoch"), identity.value(QStringLiteral("sessionEpoch"))},
        {QStringLiteral("applicantSessionRevision"), identity.value(QStringLiteral("sessionRevision"))},
        {QStringLiteral("approverId"), approver.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverRole"), approver.value(QStringLiteral("role"))},
        {QStringLiteral("rolePolicyEpoch"), role.value(QStringLiteral("policyEpoch"))},
        {QStringLiteral("rolePolicyRevision"), role.value(QStringLiteral("policyRevision"))},
        {QStringLiteral("approvedAtMonotonicMs"), static_cast<double>(_clock.elapsed())},
        {QStringLiteral("expiresAtMonotonicMs"), static_cast<double>(_createdAtMs + ApprovalLifetimeMs)},
        {QStringLiteral("binding"), canonical}, {QStringLiteral("oneUse"), true},
        {QStringLiteral("uploadLeaseMaximumMs"), static_cast<double>(UploadLeaseMaximumMs)},
        {QStringLiteral("uploadLeaseRequiresCurrentOperator"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    _state = QStringLiteral("approved");
    if (!receiptShape(_approvalReceipt) || !_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("approval-final-capture-invalid"));
        return rejected(QStringLiteral("approval-final-capture-invalid"));
    }
    emit changed();
    if (!self || !_same(generation, QStringLiteral("approved")) || !_sourcesCurrent(generation, true))
        return rejected(QStringLiteral("approval-revoked-at-publication"));
    return current();
}

bool FleetOperationApproval::_argumentsMatch(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding,
    const QVariantMap& roleReceipt, const QVariantMap& identityReceipt, const QVariantMap& approvalReceipt) const
{
    const QVariantMap candidateRole = roleReceipt;
    const QVariantMap candidateIdentity = identityReceipt;
    const QVariantMap candidateApproval = approvalReceipt;
    const QPointer<FleetOperationApproval> self(const_cast<FleetOperationApproval*>(this));
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<QObject> owner(_authorization);
    const quint64 generation = _generation;
    const QString state = _state;
    const QVariantMap frozenBinding = _binding;
    const QVariantMap frozenRole = _roleReceipt;
    const QVariantMap frozenIdentity = _identityReceipt;
    const QVariantMap frozenApproval = _approvalReceipt;
    QVariantMap canonical;
    if (!session || !roles || !store || !owner || sessionObject != session.data()
        || roleObject != roles.data() || storeObject != store.data() || authorizationOwner != owner.data()
        || !canonicalBinding(binding, &canonical) || canonical != frozenBinding
        || !roles->actionSnapshotCurrent(candidateRole) || candidateRole != frozenRole
        || !receiptShape(candidateApproval) || candidateApproval != frozenApproval) return false;
    const bool identityCurrent = session->identitySnapshotCurrent(candidateIdentity);
    return self && identityCurrent && _same(generation, state) && session && roles && store && owner
        && _session == session && _roles == roles && _store == store && _authorization == owner
        && _binding == frozenBinding && _roleReceipt == frozenRole && _identityReceipt == frozenIdentity
        && _approvalReceipt == frozenApproval && candidateIdentity == frozenIdentity
        && roles->actionSnapshotCurrent(frozenRole);
}

bool FleetOperationApproval::approvalReceiptCurrent(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding,
    const QVariantMap& roleReceipt, const QVariantMap& identityReceipt, const QVariantMap& approvalReceipt)
{
    if (thread() != QThread::currentThread() || _requestInProgress || _checking) {
        _reentered = true;
        return false;
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, _session.data())) return false;
    QPointer<FleetOperationApproval> self(this);
    const quint64 generation = _generation;
    return _same(generation, QStringLiteral("approved"))
        && _argumentsMatch(sessionObject, roleObject, storeObject, authorizationOwner,
            binding, roleReceipt, identityReceipt, approvalReceipt)
        && self && _sourcesCurrent(generation, true) && self
        && _same(generation, QStringLiteral("approved"))
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), _session.data());
}

QVariantMap FleetOperationApproval::consumeUploadApproval(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding,
    const QVariantMap& roleReceipt, const QVariantMap& identityReceipt, const QVariantMap& approvalReceipt,
    const QString& deploymentId)
{
    const QString frozenDeploymentId = deploymentId;
    QPointer<FleetOperationApproval> self(this);
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, _session.data())
        || !deploymentIdentifier(frozenDeploymentId)) return QVariantMap();
    if (!approvalReceiptCurrent(sessionObject, roleObject, storeObject, authorizationOwner,
        binding, roleReceipt, identityReceipt, approvalReceipt) || !self) return QVariantMap();
    _requestInProgress = true;
    FlagGuard consuming{QPointer<QObject>(this), &_requestInProgress};
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap frozenBinding = _binding;
    const QVariantMap frozenRole = _roleReceipt;
    const QVariantMap frozenIdentity = _identityReceipt;
    const QVariantMap frozenApprover = _approverReceipt;
    const QVariantMap frozenApproval = _approvalReceipt;
    const QByteArray frozenRecord = _recordBytes;
    const auto unchanged = [&]() {
        return self && _same(generation, QStringLiteral("approved")) && session && roles && store && owner
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data())
            && _session == session && _roles == roles && _store == store && _authorization == owner
            && _binding == frozenBinding && _roleReceipt == frozenRole && _identityReceipt == frozenIdentity
            && _approverReceipt == frozenApprover && _approvalReceipt == frozenApproval && _recordBytes == frozenRecord;
    };
    const QString grantId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto refuseLease = [&]() {
        if (session) session->revokeUploadIdentityLease(grantId);
        if (self) _invalidate(generation, QStringLiteral("native-upload-identity-lease-not-committed"), QStringLiteral("failed"), true);
        return QVariantMap();
    };
    // The native session caps duration at its ORIGINAL primary-session expiry.
    // Use our timestamp BEFORE that call plus its returned duration: elapsed
    // native-call time is deducted conservatively, with no cross-clock compare.
    const qint64 consumedAt = _clock.elapsed();
    if (!unchanged()) return QVariantMap();
    const QVariantMap result = session->consumeUploadIdentityLease(frozenIdentity, frozenApprover,
        grantId, static_cast<int>(UploadLeaseMaximumMs));
    if (!self) return QVariantMap();
    double duration = 0;
    const QVariantMap identityLease = result.value(QStringLiteral("identityLease")).toMap();
    if (!unchanged() || !strictTrue(result.value(QStringLiteral("ok")))
        || result.value(QStringLiteral("identityLease")).type() != QVariant::Map
        || !integer(identityLease.value(QStringLiteral("durationMs")), 1, UploadLeaseMaximumMs, &duration)
        || identityLease.value(QStringLiteral("leaseId")).type() != QVariant::String
        || identityLease.value(QStringLiteral("leaseId")).toString() != grantId
        || consumedAt > static_cast<qint64>(MaxExactNumber) - static_cast<qint64>(duration)
        || _clock.elapsed() >= consumedAt + static_cast<qint64>(duration)) return refuseLease();
    const bool identityLeaseCurrent = session->uploadIdentityLeaseCurrent(identityLease);
    if (!self) return QVariantMap();
    if (!identityLeaseCurrent || !unchanged() || _clock.elapsed() >= consumedAt + static_cast<qint64>(duration))
        return refuseLease();
    // Burn the request once and freeze this deployment-scoped finite grant
    // BEFORE any owner callback or transport. No synchronous publication.
    if (!unchanged()) return refuseLease();
    _identityLease = identityLease;
    _leaseConsumedAtMs = consumedAt;
    _leaseExpiresAtMs = consumedAt + static_cast<qint64>(duration);
    _state = QStringLiteral("consumed");
    _grant = {{QStringLiteral("schemaVersion"), 2.0},
        {QStringLiteral("grantId"), grantId},
        {QStringLiteral("approvalReceipt"), _approvalReceipt},
        {QStringLiteral("consumedAtMonotonicMs"), static_cast<double>(consumedAt)},
        {QStringLiteral("deploymentId"), frozenDeploymentId}, {QStringLiteral("identityLease"), identityLease},
        {QStringLiteral("leaseExpiresAtMonotonicMs"), static_cast<double>(_leaseExpiresAtMs)},
        {QStringLiteral("leaseMaximumMs"), static_cast<double>(UploadLeaseMaximumMs)},
        {QStringLiteral("leaseDurationMs"), duration},
        {QStringLiteral("purpose"), QStringLiteral("grounded-mission-upload-and-readback")},
        {QStringLiteral("oneUseConsumed"), true}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (!grantShape(_grant) || !_sourcesCurrent(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("consumed-upload-lease-context-invalid"), QStringLiteral("failed"), true);
        return QVariantMap();
    }
    const quint64 consumedGeneration = _generation;
    const QVariantMap consumedGrant = _grant;
    QTimer::singleShot(0, this, [self, consumedGeneration, consumedGrant]() {
        // Publish consumption on a later event turn, not on the successful
        // consume/gate call stack. Replacement, revocation or expiry must not
        // publish an obsolete consumed grant or refresh its authority.
        if (!self || !self->_same(consumedGeneration, QStringLiteral("consumed"))
            || self->_grant != consumedGrant) return;
        emit self->changed();
    });
    return _grant;
}

bool FleetOperationApproval::grantCurrent(const QVariantMap& grant, const QString& deploymentId)
{
    const QVariantMap candidateGrant = grant;
    const QString expectedDeploymentId = deploymentId;
    if (thread() != QThread::currentThread() || _requestInProgress || _checking) {
        _reentered = true;
        return false;
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, _session.data())) return false;
    QPointer<FleetOperationApproval> self(this);
    const quint64 generation = _generation;
    if (!deploymentIdentifier(expectedDeploymentId) || !_same(generation, QStringLiteral("consumed"))
        || !grantShape(candidateGrant) || candidateGrant != _grant
        || candidateGrant.value(QStringLiteral("deploymentId")).toString() != expectedDeploymentId) return false;
    const QPointer<FleetOperatorSession> session(_session);
    // Validate the caller's nested lease, not just our frozen native copy:
    // QVariant map equality alone could coerce a numeric schema/time to Bool.
    const bool candidateLeaseCurrent = session
        && session->uploadIdentityLeaseCurrent(candidateGrant.value(QStringLiteral("identityLease")).toMap());
    return self && candidateLeaseCurrent && session && _session == session
        && _same(generation, QStringLiteral("consumed")) && candidateGrant == _grant
        && _sourcesCurrent(generation, true) && self && _same(generation, QStringLiteral("consumed"))
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(self.data(), session.data());
}

bool FleetOperationApproval::revokeCurrent(const QString& approvalId)
{
    if (thread() != QThread::currentThread() || (!approvalId.isEmpty() && approvalId != _approvalId)) return false;
    if (_state != QStringLiteral("pending") && _state != QStringLiteral("approved")
        && _state != QStringLiteral("consumed")) return false;
    _invalidate(_generation, QStringLiteral("operator-revoked-approval"));
    return true;
}

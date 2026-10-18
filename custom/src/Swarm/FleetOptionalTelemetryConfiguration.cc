#include "FleetOptionalTelemetryConfiguration.h"

#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetOptionalTelemetryCapture.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "Vehicle.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
const QString Action = QStringLiteral("fleet.telemetry.interval.configure");
const QString Purpose = QStringLiteral("fleet.telemetry.interval.configure.approve");
const int MessageIds[] = {62, 74, 83};
struct BusyGuard {
    QPointer<FleetOptionalTelemetryConfiguration> owner;
    bool* flag;
    bool previous = false;
    ~BusyGuard() { if (owner) *flag = previous; }
};
bool yes(const QVariant& v) { return v.type() == QVariant::Bool && !v.isNull() && v.toBool(); }
bool no(const QVariant& v) { return v.type() == QVariant::Bool && !v.isNull() && !v.toBool(); }
QString ownText(const QString& v) { return v.isEmpty() ? QStringLiteral("") : QString(v.constData(), v.size()); }
bool text(const QString& v, int maximum, bool empty = false)
{
    if ((!empty && v.isEmpty()) || v.size() > maximum || v != v.trimmed() || QString::fromUtf8(v.toUtf8()) != v) return false;
    for (QChar c : v) if (c.isNull() || c.category() == QChar::Other_Control || c.category() == QChar::Other_Format) return false;
    return true;
}
bool str(const QVariant& v, int maximum, bool empty = false)
{ return v.type() == QVariant::String && !v.isNull() && text(v.toString(), maximum, empty); }
bool integer(const QVariant& v, qint64 minimum, qint64 maximum)
{
    return !v.isNull() && (v.type() == QVariant::Int || v.type() == QVariant::LongLong)
        && v.toLongLong() >= minimum && v.toLongLong() <= maximum;
}
bool counter(const QVariant& v)
{
    if (!str(v, 20)) return false;
    bool ok = false; const quint64 n = v.toString().toULongLong(&ok);
    return ok && QString::number(n) == v.toString();
}
bool digest(const QString& v) { return QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(v).hasMatch(); }
QVariant owned(const QVariant& v)
{
    if (v.type() == QVariant::String) return ownText(v.toString());
    if (v.type() == QVariant::Map) {
        QVariantMap out; const QVariantMap m = v.toMap();
        for (auto it = m.constBegin(); it != m.constEnd(); ++it) out.insert(ownText(it.key()), owned(it.value()));
        return out;
    }
    if (v.type() == QVariant::List) { QVariantList out; for (const QVariant& row : v.toList()) out.append(owned(row)); return out; }
    return v;
}
QVariantMap ownMap(const QVariantMap& v) { return owned(v).toMap(); }
bool primaryShape(const QVariantMap& v)
{
    if (v.size() != 11 || !yes(v.value(QStringLiteral("authenticated")))) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("actorId"), QStringLiteral("displayName"), QStringLiteral("role"), QStringLiteral("authenticationSource")})
        if (!str(v.value(key), 256)) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"), QStringLiteral("expiresAtMonotonicMs")}) {
        const QVariant n = v.value(key);
        if (n.isNull() || (n.type() != QVariant::Double && n.type() != QVariant::Int && n.type() != QVariant::LongLong)
            || !std::isfinite(n.toDouble())) return false;
    }
    return v.value(QStringLiteral("schemaVersion")).toDouble() == 1.0;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    if (role.size() != 9) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
        QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")}) if (!str(role.value(key), 128)) return false;
    return QStringList{QStringLiteral("operator"), QStringLiteral("maintenance"), QStringLiteral("commander"), QStringLiteral("administrator")}
            .contains(identity.value(QStringLiteral("role")).toString())
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")) == QVariant(Action) && yes(role.value(QStringLiteral("allowed")))
        && yes(role.value(QStringLiteral("requiresSecondApproval"))) && no(role.value(QStringLiteral("actorAuthenticated")))
        && no(role.value(QStringLiteral("executionPermitted")));
}
bool exactInterval(int v) { return v > 0 && double(float(v)) == double(v); }
bool basisShape(const QVariantMap& v, int interval)
{
    if (v.size() != 15 || v.value(QStringLiteral("schemaVersion")) != QVariant(1)
        || v.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-optional-telemetry-configuration-basis"))
        || !str(v.value(QStringLiteral("basisId")), 64) || !integer(v.value(QStringLiteral("vehicleId")), 1, 255)
        || !integer(v.value(QStringLiteral("componentId")), 1, 255)
        || !yes(v.value(QStringLiteral("sourceCurrent"))) || !no(v.value(QStringLiteral("consumed")))
        || !integer(v.value(QStringLiteral("remainingMs")), 1, 30000)
        || !counter(v.value(QStringLiteral("originalMutationGeneration"))) || !str(v.value(QStringLiteral("ledgerEpoch")), 128)
        || !yes(v.value(QStringLiteral("snapshotOnly"))) || !no(v.value(QStringLiteral("configurationPermitted")))
        || !no(v.value(QStringLiteral("executionPermit"))) || !no(v.value(QStringLiteral("flightCommandReleased")))
        || v.value(QStringLiteral("rows")).type() != QVariant::List || v.value(QStringLiteral("rows")).toList().size() != 3) return false;
    const QVariantList rows = v.value(QStringLiteral("rows")).toList();
    for (int i = 0; i < 3; ++i) {
        if (rows.at(i).type() != QVariant::Map) return false;
        const QVariantMap row = rows.at(i).toMap();
        if (row.size() != 3 || row.value(QStringLiteral("messageId")) != QVariant(MessageIds[i])
            || row.value(QStringLiteral("intervalUs")).type() != QVariant::Int || row.value(QStringLiteral("intervalUs")).isNull()
            || row.value(QStringLiteral("intervalUs")).toInt() <= 0 || interval < row.value(QStringLiteral("intervalUs")).toInt()
            || !counter(row.value(QStringLiteral("contentRevision")))) return false;
    }
    return true;
}
bool queryShape(const QVariantMap& v, int vehicleId, int messageId)
{
    if (v.isEmpty()) return true;
    if (v.size() != 32 || v.value(QStringLiteral("schemaVersion")) != QVariant(1)
        || v.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-message-interval-query"))
        || !str(v.value(QStringLiteral("requestId")), 64) || v.value(QStringLiteral("vehicleId")) != QVariant(vehicleId)
        || v.value(QStringLiteral("messageId")) != QVariant(messageId)) return false;
    for (const QString& key : {QStringLiteral("requestAccepted"), QStringLiteral("active"), QStringLiteral("requestIssued"),
        QStringLiteral("ackObserved"), QStringLiteral("reportObserved"), QStringLiteral("sourceCurrent")})
        if (v.value(key).type() != QVariant::Bool || v.value(key).isNull()) return false;
    for (const QString& key : {QStringLiteral("receiptSequence"), QStringLiteral("baselineReceiptSequence"),
        QStringLiteral("contentRevision"), QStringLiteral("mutationGeneration")}) if (!counter(v.value(key))) return false;
    for (const QString& key : {QStringLiteral("ackResult"), QStringLiteral("intervalUs")})
        if (!v.value(key).isNull() && v.value(key).type() != QVariant::Int) return false;
    if (!yes(v.value(QStringLiteral("readOnly")))) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("responseAttributionVerified"),
        QStringLiteral("actualTransmitRateVerified"), QStringLiteral("availableBandwidthVerified"), QStringLiteral("configurationPermitted"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")}) if (!no(v.value(key))) return false;
    return true;
}
bool runShape(const QVariantMap& v, const QVariantMap& basis, int interval)
{
    if (v.size() != 29 || v.value(QStringLiteral("schemaVersion")) != QVariant(1)
        || v.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-optional-telemetry-configuration-run"))
        || !str(v.value(QStringLiteral("runId")), 64) || v.value(QStringLiteral("basisId")) != basis.value(QStringLiteral("basisId"))
        || v.value(QStringLiteral("vehicleId")) != basis.value(QStringLiteral("vehicleId"))
        || v.value(QStringLiteral("componentId")) != basis.value(QStringLiteral("componentId"))
        || v.value(QStringLiteral("requestedIntervalUs")) != QVariant(interval)
        || !integer(v.value(QStringLiteral("currentStageIndex")), 0, 9)
        || !integer(v.value(QStringLiteral("currentMessageId")), 0, 16777215)
        || !integer(v.value(QStringLiteral("remainingMs")), 0, 30000)
        || !str(v.value(QStringLiteral("state")), 64) || !str(v.value(QStringLiteral("reason")), 256, true)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-set-ack"), QStringLiteral("waiting-readback"),
            QStringLiteral("waiting-final-readback"), QStringLiteral("reported-values-match"), QStringLiteral("failed"), QStringLiteral("cancelled")}
                .contains(v.value(QStringLiteral("state")).toString())
        || v.value(QStringLiteral("originalMutationGeneration")) != basis.value(QStringLiteral("originalMutationGeneration"))
        || !counter(v.value(QStringLiteral("expectedMutationGeneration"))) || !str(v.value(QStringLiteral("ledgerEpoch")), 128)) return false;
    for (const QString& key : {QStringLiteral("active"), QStringLiteral("requestIssued"), QStringLiteral("sourceCurrent"), QStringLiteral("reportedValuesMatch")})
        if (v.value(key).type() != QVariant::Bool || v.value(key).isNull()) return false;
    if (!yes(v.value(QStringLiteral("snapshotOnly")))) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("responseAttributionVerified"),
        QStringLiteral("actualTransmitRateVerified"), QStringLiteral("availableBandwidthVerified"), QStringLiteral("configurationPermitted"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")}) if (!no(v.value(key))) return false;
    if (v.value(QStringLiteral("rows")).type() != QVariant::List || v.value(QStringLiteral("rows")).toList().size() != 3) return false;
    const QVariantList rows = v.value(QStringLiteral("rows")).toList();
    const QVariantList originals = basis.value(QStringLiteral("rows")).toList();
    for (int i = 0; i < 3; ++i) {
        if (rows.at(i).type() != QVariant::Map) return false;
        const QVariantMap row = rows.at(i).toMap();
        if (row.size() != 9 || row.value(QStringLiteral("messageId")) != QVariant(MessageIds[i])
            || row.value(QStringLiteral("originalIntervalUs")) != originals.at(i).toMap().value(QStringLiteral("intervalUs"))
            || !str(row.value(QStringLiteral("setRequestId")), 64, true)
            || (!row.value(QStringLiteral("setAckResult")).isNull() && row.value(QStringLiteral("setAckResult")).type() != QVariant::Int)) return false;
        for (const QString& key : {QStringLiteral("setRequestIssued"), QStringLiteral("setAckObserved"), QStringLiteral("reportedValueMatches")})
            if (row.value(key).type() != QVariant::Bool || row.value(key).isNull()) return false;
        for (const QString& key : {QStringLiteral("readbackQuery"), QStringLiteral("finalQuery")})
            if (row.value(key).type() != QVariant::Map || !queryShape(row.value(key).toMap(), v.value(QStringLiteral("vehicleId")).toInt(), MessageIds[i])) return false;
    }
    return true;
}
bool completedRun(const QVariantMap& v, int interval)
{
    if (v.value(QStringLiteral("state")) != QVariant(QStringLiteral("reported-values-match"))
        || !no(v.value(QStringLiteral("active"))) || !yes(v.value(QStringLiteral("sourceCurrent")))
        || !yes(v.value(QStringLiteral("reportedValuesMatch"))) || v.value(QStringLiteral("currentStageIndex")).toInt() != 9) return false;
    for (const QVariant& value : v.value(QStringLiteral("rows")).toList()) {
        const QVariantMap row = value.toMap();
        if (!yes(row.value(QStringLiteral("setRequestIssued"))) || !yes(row.value(QStringLiteral("setAckObserved")))
            || row.value(QStringLiteral("setAckResult")) != QVariant(int(MAV_RESULT_ACCEPTED))
            || !yes(row.value(QStringLiteral("reportedValueMatches")))) return false;
        for (const QString& key : {QStringLiteral("readbackQuery"), QStringLiteral("finalQuery")}) {
            const QVariantMap query = row.value(key).toMap();
            bool baselineOk = false, receiptOk = false;
            const quint64 baseline = query.value(QStringLiteral("baselineReceiptSequence")).toString().toULongLong(&baselineOk);
            const quint64 receipt = query.value(QStringLiteral("receiptSequence")).toString().toULongLong(&receiptOk);
            if (query.value(QStringLiteral("state")) != QVariant(QStringLiteral("complete"))
                || !yes(query.value(QStringLiteral("requestAccepted"))) || !no(query.value(QStringLiteral("active")))
                || !yes(query.value(QStringLiteral("requestIssued"))) || !yes(query.value(QStringLiteral("ackObserved")))
                || query.value(QStringLiteral("ackResult")) != QVariant(int(MAV_RESULT_ACCEPTED))
                || !yes(query.value(QStringLiteral("reportObserved"))) || query.value(QStringLiteral("intervalUs")) != QVariant(interval)
                || !baselineOk || !receiptOk || receipt <= baseline
                || (key == QStringLiteral("finalQuery") && !yes(query.value(QStringLiteral("sourceCurrent"))))) return false;
        }
    }
    return true;
}
}

FleetOptionalTelemetryConfiguration::FleetOptionalTelemetryConfiguration(SwarmController* swarm, FleetRegistry* registry,
    FleetOptionalTelemetryCapture* capture) : QObject(swarm), _swarm(swarm), _registry(registry), _capture(capture)
{
    _clock.start(); _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, &FleetOptionalTelemetryConfiguration::tick);
}

FleetOptionalTelemetryConfiguration::~FleetOptionalTelemetryConfiguration()
{
    _destroying = true; _active = false; _timer.stop();
    const QPointer<FleetOperatorSession> session(_session);
    const QString hash = _bindingHash;
    if (session) session->revokeTelemetryConfigurationAuthentication(this, hash);
    disconnectSources();
    // Opaque native callbacks retain QPointer fences; destruction cannot
    // authorize another write. A possibly sent request is never rolled back.
}

QVariantMap FleetOptionalTelemetryConfiguration::result(bool accepted, const QString& reason, const QString& reviewId)
{
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("requestAccepted"), accepted},
        {QStringLiteral("reviewId"), ownText(reviewId)}, {QStringLiteral("reason"), ownText(reason)},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetOptionalTelemetryConfiguration::current() const
{
    if (_destroying || QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reviewId"), ownText(_reviewId)},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("state"), ownText(_state)},
        {QStringLiteral("reason"), ownText(_reason)}, {QStringLiteral("active"), _active},
        {QStringLiteral("scopeToken"), ownText(_scopeToken)}, {QStringLiteral("captureId"), ownText(_captureId)},
        {QStringLiteral("vehicleId"), _vehicleId}, {QStringLiteral("requestedIntervalUs"), _intervalUs},
        {QStringLiteral("remainingMs"), _active ? qMax<qint64>(0, _deadlineMs - _clock.elapsed()) : qint64(0)},
        {QStringLiteral("bindingHash"), ownText(_bindingHash)}, {QStringLiteral("basisSummary"), ownMap(_basisSummary)},
        {QStringLiteral("run"), ownMap(_runSnapshot)}, {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("configurationRequestsMayRemainInflight"), _requestsMayRemainInflight},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("responseAttributionVerified"), false},
        {QStringLiteral("actualTransmitRateVerified"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
}

bool FleetOptionalTelemetryConfiguration::memoryCurrent(quint64 generation) const
{
    return !_destroying && _active && generation != 0 && generation == _generation && QThread::currentThread() == thread()
        && _clock.elapsed() >= 0 && _clock.elapsed() < _deadlineMs && _swarm && _registry && _capture && _session && _roles && _root && _vehicle
        && _swarm->thread() == thread() && _registry->thread() == thread() && _capture->thread() == thread()
        && _session->thread() == thread() && _roles->thread() == thread() && _root->thread() == thread() && _vehicle->thread() == thread()
        && parent() == _swarm.data() && _swarm->parent() == _root.data() && _registry->parent() == _swarm.data()
        && _swarm->ownsOptionalTelemetryConfiguration(const_cast<FleetOptionalTelemetryConfiguration*>(this), _registry.data(), _capture.data())
        && _capture->_requestId == _captureId && _capture->_generation == _captureGeneration && _capture->_vehicle == _vehicle
        && _capture->_session == _session && _capture->_root == _root && _capture->_scopeToken == _scopeToken
        && _capture->configurationBasisSourceCurrent(_captureGeneration, _captureId)
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data());
}

bool FleetOptionalTelemetryConfiguration::contextMatches(QObject* session, QObject* roles,
    const QVariantMap& primary, const QVariantMap& role) const
{
    if (!memoryCurrent(_generation) || session != _session.data() || roles != _roles.data() || primary.size() != 5 || role.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("actorId"), QStringLiteral("role")}) if (!str(primary.value(key), 128) || primary.value(key) != _identity.value(key)) return false;
    return role == _role && roleMatches(_identity, role) && _roles->actionSnapshotCurrent(role);
}

bool FleetOptionalTelemetryConfiguration::gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (!memoryCurrent(generation) || !_vehicle->fleetMessageIntervalQueryTransportQuiescent()) return false;
    const QPointer<FleetOptionalTelemetryConfiguration> self(this);
    const QPointer<FleetOperatorSession> session(_session); const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root); const QPointer<Vehicle> vehicle(_vehicle);
    const auto basis = _basis; const auto run = _run;
    const QString id = _reviewId, scope = _scopeToken, hash = _bindingHash, state = _state;
    const QVariantMap identity = _identity, role = _role, approver = _approver;
    const bool consumed = _authenticationConsumed, nativeCall = _nativeCall;
    _checking = true; _reentered = false; const BusyGuard checking{self, &_checking};
    const auto same = [&]() {
        return self && !_reentered && memoryCurrent(generation) && session && session == _session && roles && roles == _roles
            && root && root == _root && vehicle && vehicle == _vehicle && _basis == basis && _run == run
            && _reviewId == id && _scopeToken == scope && _bindingHash == hash && _state == state
            && _identity == identity && _role == role && _approver == approver && _authenticationConsumed == consumed && _nativeCall == nativeCall
            && roles->actionSnapshotCurrent(role) && roleMatches(identity, role);
    };
    if (!same()) return false;
    // During the native operation's final fence that operation itself checks
    // the opaque source and its exact owned mutation. No stale Capture row or
    // transient write-depth snapshot is allowed to rebase that native scope.
    if (!nativeCall) {
        if (!run && !vehicle->fleetOptionalTelemetryConfigurationBasisCurrent(basis)) return false;
        if (run) {
            const QVariantMap snapshot = vehicle->fleetOptionalTelemetryConfigurationSnapshot(self.data(), run);
            if (!same() || !runShape(snapshot, _basisSummary, _intervalUs) || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) return false;
        }
    }
    if (!same() || !session->identitySnapshotCurrent(identity) || !same()) return false;
    if (consumed && (!session->telemetryConfigurationAuthenticationCurrent(self.data()) || !same())) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetOptionalTelemetryConfigurationSourcesCurrent", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(_registry.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(_swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && same()
        && (!consumed || session->telemetryConfigurationConsumedIdentityCurrent(self.data()));
}

bool FleetOptionalTelemetryConfiguration::authenticationBindingCurrent(FleetOperatorSession* session,
    const QVariantMap& primary, const QVariantMap& approver, const QString& hash) const
{
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && !_authenticationConsumed
        && _operationConfirmed && session == _session.data() && primary == _identity && approver == _approver
        && digest(hash) && hash == _bindingHash && approver.value(QStringLiteral("action")) == QVariant(Purpose)
        && approver.value(QStringLiteral("bindingHash")) == QVariant(hash) && _roles->actionSnapshotCurrent(_role)
        && roleMatches(_identity, _role);
}

void FleetOptionalTelemetryConfiguration::disconnectSources()
{
    const auto connections = _connections; _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

bool FleetOptionalTelemetryConfiguration::watchSources(quint64 generation)
{
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); QList<QMetaObject::Connection> staged;
    const auto stale = [self, generation]() { if (self) self->finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-original-source-changed")); };
    const auto add = [&](const QMetaObject::Connection& c) { staged.append(c); return bool(c) && self && memoryCurrent(generation); };
    if (!add(connect(_swarm.data(), &QObject::destroyed, this, stale)) || !add(connect(_registry.data(), &QObject::destroyed, this, stale))
        || !add(connect(_capture.data(), &QObject::destroyed, this, stale)) || !add(connect(_session.data(), &QObject::destroyed, this, stale))
        || !add(connect(_roles.data(), &QObject::destroyed, this, stale)) || !add(connect(_root.data(), &QObject::destroyed, this, stale))
        || !add(connect(_vehicle.data(), &QObject::destroyed, this, stale))
        || !add(connect(_session.data(), &FleetOperatorSession::changed, this, [self, generation]() { if (self && !self->_prompt) self->finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-account-mutated")); }))
        || !add(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale))
        || !add(connect(_capture.data(), &FleetOptionalTelemetryCapture::changed, this, [self, generation]() { if (self && !self->memoryCurrent(generation)) self->finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-capture-source-retired")); }))
        || !add(connect(_registry.data(), &FleetRegistry::registryChanged, this, [self, generation]() { if (self && !self->memoryCurrent(generation)) self->finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-registry-binding-changed")); }))
        || !add(connect(_vehicle.data(), &Vehicle::armedChanged, this, [stale](bool armed) { if (armed) stale(); }))
        || !add(connect(_vehicle.data(), &Vehicle::flyingChanged, this, [stale](bool flying) { if (flying) stale(); }))) {
        for (const auto& c : staged) QObject::disconnect(c); return false;
    }
    if (!self || !memoryCurrent(generation)) { for (const auto& c : staged) QObject::disconnect(c); return false; }
    _connections = staged; return true;
}

bool FleetOptionalTelemetryConfiguration::publish(quint64 generation)
{
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); emit changed();
    return self && memoryCurrent(generation) && gate(generation);
}

QVariantMap FleetOptionalTelemetryConfiguration::requestReview(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
    const QString& requestedScope, const QString& requestedCaptureId, int intervalUs, bool confirmed)
{
    const QString scope = ownText(requestedScope), captureId = ownText(requestedCaptureId);
    if (_destroying || QThread::currentThread() != thread() || _active || _busy || _checking || !confirmed || !exactInterval(intervalUs)
        || !text(scope, 32768) || !text(captureId, 64) || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("confirmed-idle-positive-exact-interval-review-required"), QStringLiteral(""));
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualSession));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualRoles)); const QPointer<QObject> root(actualRoot);
    const QPointer<FleetOptionalTelemetryCapture> capture(_capture); const quint64 generationBefore = _generation;
    const auto original = [&]() {
        return self && !_destroying && !_active && _generation == generationBefore && session && roles && root && capture
            && capture == _capture && _swarm && _registry && session->thread() == thread() && roles->thread() == thread() && root->thread() == thread()
            && parent() == _swarm.data() && _swarm->parent() == root.data()
            && _swarm->ownsOptionalTelemetryConfiguration(self.data(), _registry.data(), capture.data())
            && capture->_requestId == captureId && capture->_session == session && capture->_root == root && capture->_scopeToken == scope
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data());
    };
    if (!original()) return result(false, QStringLiteral("actual-current-capture-session-scope-required"), QStringLiteral(""));
    const QPointer<Vehicle> vehicle(capture->_vehicle); const quint64 captureGeneration = capture->_generation;
    if (!vehicle || !vehicle->fleetMessageIntervalQueryTransportQuiescent()) return result(false, QStringLiteral("configuration-raw-stack-busy"), QStringLiteral(""));
    const QVariantMap identity = session->identitySnapshot();
    if (!original() || !vehicle || capture->_vehicle != vehicle || capture->_generation != captureGeneration || !primaryShape(identity))
        return result(false, QStringLiteral("configuration-primary-identity-unavailable"), QStringLiteral(""));
    const QVariantMap role = roles->actionSnapshot(Action);
    if (!original() || !vehicle->fleetMessageIntervalQueryTransportQuiescent() || !roleMatches(identity, role)
        || !roles->actionSnapshotCurrent(role) || !session->identitySnapshotCurrent(identity) || !original())
        return result(false, QStringLiteral("configuration-authenticated-two-account-role-required"), QStringLiteral(""));
    const auto basis = capture->takeConfigurationBasis(self.data(), captureId);
    if (!original() || !vehicle || capture->_vehicle != vehicle || capture->_generation != captureGeneration || !basis)
        return result(false, QStringLiteral("configuration-one-use-native-basis-unavailable"), QStringLiteral(""));
    const qint64 summaryAt = _clock.elapsed();
    const QVariantMap summary = vehicle->fleetOptionalTelemetryConfigurationBasisSummary(basis);
    if (!original() || !basisShape(summary, intervalUs) || !vehicle->fleetOptionalTelemetryConfigurationBasisCurrent(basis))
        return result(false, QStringLiteral("configuration-positive-current-non-increasing-baseline-required"), QStringLiteral(""));
    const qint64 remaining = summary.value(QStringLiteral("remainingMs")).toLongLong();
    if (summaryAt < 0 || summaryAt > std::numeric_limits<qint64>::max() - remaining)
        return result(false, QStringLiteral("configuration-original-clock-exhausted"), QStringLiteral(""));
    disconnectSources();
    if (!original()) return result(false, QStringLiteral("configuration-source-changed-during-cleanup"), QStringLiteral(""));
    ++_generation; const quint64 generation = _generation;
    _session = session; _roles = roles; _root = root; _vehicle = vehicle; _basis = basis; _run.reset();
    _captureGeneration = captureGeneration; _captureId = captureId; _scopeToken = scope;
    _identity = ownMap(identity); _role = ownMap(role); _approver.clear(); _basisSummary = ownMap(summary); _runSnapshot.clear();
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces); _vehicleId = summary.value(QStringLiteral("vehicleId")).toInt();
    _intervalUs = intervalUs; _deadlineMs = summaryAt + remaining;
    _authenticationConsumed = false; _operationConfirmed = false; _requestsMayRemainInflight = false;
    _state = QStringLiteral("preparing"); _reason = QStringLiteral(""); _active = true;
    QVariantMap stable = summary; stable.remove(QStringLiteral("remainingMs")); stable.remove(QStringLiteral("sourceCurrent")); stable.remove(QStringLiteral("consumed"));
    const QVariantMap body{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), Action}, {QStringLiteral("purpose"), Purpose},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("captureId"), captureId}, {QStringLiteral("captureGeneration"), QString::number(captureGeneration)},
        {QStringLiteral("scopeToken"), scope}, {QStringLiteral("basis"), stable}, {QStringLiteral("requestedIntervalUs"), intervalUs},
        {QStringLiteral("primary"), _identity}, {QStringLiteral("role"), _role}, {QStringLiteral("originalRemainingMs"), remaining}};
    _bindingHash = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    const QString id = _reviewId;
    if (!watchSources(generation) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-initial-source-denied"));
        return result(false, QStringLiteral("configuration-initial-source-denied"), id);
    }
    _state = QStringLiteral("review"); _timer.start();
    if (!publish(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-review-publication-changed")); return result(false, QStringLiteral("configuration-review-publication-changed"), id); }
    return result(true, QStringLiteral("configuration-review-ready-no-write"), id);
}

QVariantMap FleetOptionalTelemetryConfiguration::approveAndApply(const QString& requestedId, bool confirmed)
{
    const QString id = ownText(requestedId);
    if (_destroying || QThread::currentThread() != thread() || _busy || _checking || !confirmed || !_active
        || _state != QStringLiteral("review") || id != _reviewId || !text(id, 64))
        return result(false, QStringLiteral("separate-current-review-approval-required"), id);
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const quint64 generation = _generation; const QPointer<FleetOperatorSession> session(_session);
    if (!gate(generation) || !self) return result(false, QStringLiteral("configuration-review-source-not-current"), id);
    _state = QStringLiteral("approving"); _operationConfirmed = true; _prompt = true;
    if (!publish(generation) || !self) { if (self) { _prompt = false; finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-before-prompt-source-changed")); } return result(false, QStringLiteral("configuration-before-prompt-source-changed"), id); }
    const QVariantList rows = _basisSummary.value(QStringLiteral("rows")).toList();
    const QString summary = QStringLiteral("Ground vehicle %1. Baselines 62/74/83: %2/%3/%4 us; target each %5 us. Original capture deadline is NOT renewed. This may write three optional intervals and perform readbacks; no retry/rollback, no flight permission.")
        .arg(_vehicleId).arg(rows.at(0).toMap().value(QStringLiteral("intervalUs")).toInt())
        .arg(rows.at(1).toMap().value(QStringLiteral("intervalUs")).toInt())
        .arg(rows.at(2).toMap().value(QStringLiteral("intervalUs")).toInt()).arg(_intervalUs);
    const QVariantMap approval = session->authenticateApprover(Purpose, _bindingHash, _identity, summary);
    if (!self) return result(false, QStringLiteral("configuration-destroyed-during-approval"), id);
    _prompt = false;
    if (!memoryCurrent(generation) || _state != QStringLiteral("approving") || !yes(approval.value(QStringLiteral("ok")))
        || approval.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("configuration-independent-approval-failed"));
        return result(false, QStringLiteral("configuration-independent-approval-failed"), id);
    }
    _approver = ownMap(approval.value(QStringLiteral("identityReceipt")).toMap());
    if (!gate(generation) || !self || !session || !session->approverSnapshotCurrent(_approver) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-approval-source-changed"));
        return result(false, QStringLiteral("configuration-approval-source-changed"), id);
    }
    _consuming = true;
    const int remaining = int(qMax<qint64>(0, qMin<qint64>(30000, _deadlineMs - _clock.elapsed())));
    const bool consumed = session->consumeTelemetryConfigurationAuthentication(_identity, _approver, self.data(), _bindingHash, remaining);
    if (!self) return result(false, QStringLiteral("configuration-destroyed-during-consume"), id);
    _consuming = false; _authenticationConsumed = consumed;
    if (!consumed || !gate(generation) || !self) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-authentication-consume-failed"));
        return result(false, QStringLiteral("configuration-authentication-consume-failed"), id);
    }
    _state = QStringLiteral("applying");
    const QPointer<Vehicle> vehicle(_vehicle); const auto basis = _basis;
    const auto fence = [self, generation, id]() { return self && self->_reviewId == id && self->_authenticationConsumed && self->gate(generation); };
    _nativeCall = true;
    const auto run = vehicle->startFleetOptionalTelemetryConfiguration(self.data(), basis, _intervalUs, fence);
    if (self) _nativeCall = false;
    if (!self) return result(false, QStringLiteral("configuration-destroyed-during-native-start"), id);
    const QVariantMap snapshot = vehicle ? vehicle->fleetOptionalTelemetryConfigurationSnapshot(self.data(), run)
        : Vehicle::fleetOptionalTelemetryConfigurationRetiredSnapshot(self.data(), run);
    if (!self) return result(false, QStringLiteral("configuration-destroyed-during-native-start-snapshot"), id);
    if (_generation != generation || _reviewId != id || !_active) {
        if (_generation == generation && _reviewId == id && run && runShape(snapshot, _basisSummary, _intervalUs)) {
            _run = run; _runSnapshot = ownMap(snapshot);
            _requestsMayRemainInflight |= yes(snapshot.value(QStringLiteral("requestIssued")));
        }
        if (run && vehicle) vehicle->cancelFleetOptionalTelemetryConfiguration(self.data(), run);
        if (self && _generation == generation && _reviewId == id) emit changed();
        return result(false, QStringLiteral("configuration-retired-during-native-start"), id);
    }
    if (!run || !runShape(snapshot, _basisSummary, _intervalUs)) {
        if (run && vehicle) vehicle->cancelFleetOptionalTelemetryConfiguration(self.data(), run);
        if (self) finish(generation, QStringLiteral("failed"), QStringLiteral("configuration-native-run-not-created"));
        return result(false, QStringLiteral("configuration-native-run-not-created"), id);
    }
    _run = run; _runSnapshot = ownMap(snapshot);
    if (!yes(snapshot.value(QStringLiteral("active"))) || snapshot.value(QStringLiteral("state")) != QVariant(QStringLiteral("ready"))
        || !no(snapshot.value(QStringLiteral("requestIssued")))) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("configuration-native-ready-denied"));
        return result(false, QStringLiteral("configuration-native-ready-denied"), id);
    }
    _timer.start(); // No advance/write until Root has adopted the accepted decision.
    if (!publish(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-native-ready-publication-changed")); return result(false, QStringLiteral("configuration-native-ready-publication-changed"), id); }
    return result(true, QStringLiteral("configuration-accepted-writes-not-yet-issued"), id);
}

void FleetOptionalTelemetryConfiguration::tick()
{
    if (_destroying || !_active || _busy || _checking || QThread::currentThread() != thread()) return;
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const quint64 generation = _generation;
    if (!memoryCurrent(generation)) { finish(generation, QStringLiteral("expired"), QStringLiteral("configuration-original-deadline-or-source-expired")); return; }
    if (_state != QStringLiteral("applying") || !_run) {
        if (!_vehicle->fleetMessageIntervalQueryTransportQuiescent()) return;
        if (!gate(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-authority-or-source-changed"));
        return;
    }
    const QPointer<Vehicle> vehicle(_vehicle); const auto run = _run; const QString id = _reviewId;
    // Native advance checks the original wire deadline before RAW quiescence.
    // Busy RAW only waits; no OS/fence/wire occurs there. In a quiet turn its
    // actual write gate invokes our independent two-account final fence.
    _nativeCall = true;
    vehicle->advanceFleetOptionalTelemetryConfiguration(self.data(), run);
    if (self) _nativeCall = false;
    if (!self) return;
    const QVariantMap snapshot = vehicle ? vehicle->fleetOptionalTelemetryConfigurationSnapshot(self.data(), run)
        : Vehicle::fleetOptionalTelemetryConfigurationRetiredSnapshot(self.data(), run);
    if (!self) return;
    // An exact retired generation may still learn the actual write/ACK facts.
    // A new review/run can never inherit that late return or its cleanup.
    if (_generation != generation || _reviewId != id || _run != run) { if (vehicle) vehicle->cancelFleetOptionalTelemetryConfiguration(self.data(), run); return; }
    if (!runShape(snapshot, _basisSummary, _intervalUs)) { finish(generation, QStringLiteral("failed"), QStringLiteral("configuration-native-run-shape-invalid")); return; }
    _runSnapshot = ownMap(snapshot); _requestsMayRemainInflight = _requestsMayRemainInflight || yes(snapshot.value(QStringLiteral("requestIssued")));
    if (!_active) { if (vehicle) vehicle->cancelFleetOptionalTelemetryConfiguration(self.data(), run); if (self && _generation == generation && _reviewId == id) emit changed(); return; }
    if (!yes(snapshot.value(QStringLiteral("active")))) {
        const bool matched = completedRun(snapshot, _intervalUs);
        finish(generation, matched ? QStringLiteral("reported-values-match") : QStringLiteral("failed"),
            matched ? QStringLiteral("reported-values-only-not-rate-or-attribution-proof") : QStringLiteral("configuration-native-run-stopped"));
        return;
    }
    if (!vehicle || !vehicle->fleetMessageIntervalQueryTransportQuiescent()) return;
    if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("configuration-progress-publication-changed"));
}

void FleetOptionalTelemetryConfiguration::finish(quint64 generation, const QString& state, const QString& reason)
{
    if (_destroying || !_active || _generation != generation) return;
    const QPointer<FleetOptionalTelemetryConfiguration> self(this); const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<FleetOperatorSession> session(_session); const auto run = _run; const QString id = _reviewId, hash = _bindingHash;
    const QString finalState = ownText(state), finalReason = ownText(reason);
    if (run) {
        const QVariantMap partial = vehicle ? vehicle->fleetOptionalTelemetryConfigurationSnapshot(self.data(), run)
            : Vehicle::fleetOptionalTelemetryConfigurationRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _reviewId != id || _run != run) return;
        if (runShape(partial, _basisSummary, _intervalUs)) { _runSnapshot = ownMap(partial); _requestsMayRemainInflight |= yes(partial.value(QStringLiteral("requestIssued"))); }
    }
    _active = false; _state = finalState; _reason = finalReason; _timer.stop();
    if (session) session->revokeTelemetryConfigurationAuthentication(self.data(), hash);
    if (run && vehicle) vehicle->cancelFleetOptionalTelemetryConfiguration(self.data(), run);
    if (!self || _generation != generation || _reviewId != id) return;
    disconnectSources();
    if (self && _generation == generation && _reviewId == id) emit changed();
}

bool FleetOptionalTelemetryConfiguration::cancel(const QString& requestedId)
{
    const QString id = ownText(requestedId);
    if (_destroying || QThread::currentThread() != thread() || !_active || id != _reviewId || !text(id, 64)) return false;
    finish(_generation, QStringLiteral("cancelled"), QStringLiteral("configuration-cancelled-no-rollback")); return true;
}

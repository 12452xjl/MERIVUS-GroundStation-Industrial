#include "FleetOptionalTelemetryCapture.h"

#include "FleetMissionDeploymentEpisode.h"
#include "FleetOptionalTelemetryConfiguration.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QSet>
#include <QStringList>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 LifetimeMs = 30000;
constexpr qint64 ItemTimeoutMs = 4000;
const int MessageIds[] = {62, 74, 83};
const QString Action = QStringLiteral("fleet.telemetry.interval.capture");

struct BusyGuard {
    QPointer<FleetOptionalTelemetryCapture> owner;
    bool* flag;
    bool previous = false;
    ~BusyGuard() { if (owner) *flag = previous; }
};

bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && !value.toBool(); }
QString ownedText(const QString& value) { return value.isEmpty() ? QStringLiteral("") : QString(value.constData(), value.size()); }
bool text(const QString& value, int maximum, bool empty = false)
{
    if ((!empty && value.isEmpty()) || value.size() > maximum || value != value.trimmed()
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar ch : value)
        if (ch.isNull() || ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format) return false;
    return true;
}
bool stringValue(const QVariant& value, int maximum, bool empty = false)
{
    return value.type() == QVariant::String && !value.isNull() && text(value.toString(), maximum, empty);
}
bool sequence(const QVariant& value, quint64* result = nullptr)
{
    if (!stringValue(value, 20)) return false;
    const QString valueText = value.toString();
    if (valueText.size() > 1 && valueText.front() == QLatin1Char('0')) return false;
    for (QChar ch : valueText) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = valueText.toULongLong(&ok);
    if (!ok || QString::number(number) != valueText) return false;
    if (result) *result = number;
    return true;
}
// Only validated, small fixed native DTOs enter this copier. In particular it
// does not traverse caller-supplied arbitrary telemetry/configuration objects.
QVariant ownedValue(const QVariant& value)
{
    if (value.type() == QVariant::String) return ownedText(value.toString());
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            result.insert(ownedText(it.key()), ownedValue(it.value()));
        return result;
    }
    if (value.type() == QVariant::List) {
        QVariantList result;
        for (const QVariant& row : value.toList()) result.append(ownedValue(row));
        return result;
    }
    return value;
}
QVariantMap ownMap(const QVariantMap& value) { return ownedValue(value).toMap(); }
bool primaryShape(const QVariantMap& identity)
{
    if (identity.size() != 11 || !yes(identity.value(QStringLiteral("authenticated")))) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("actorId"), QStringLiteral("displayName"),
        QStringLiteral("role"), QStringLiteral("authenticationSource")})
        if (!stringValue(identity.value(key), 256)) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs")}) {
        const QVariant value = identity.value(key);
        if (value.isNull() || (value.type() != QVariant::Double && value.type() != QVariant::Int
            && value.type() != QVariant::LongLong) || !std::isfinite(value.toDouble())) return false;
    }
    return identity.value(QStringLiteral("schemaVersion")).toDouble() == 1.0;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    if (role.size() != 9) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
        QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")})
        if (!stringValue(role.value(key), 128)) return false;
    return role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).toString() == Action
        && yes(role.value(QStringLiteral("allowed"))) && no(role.value(QStringLiteral("requiresSecondApproval")))
        && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
bool queryShape(const QVariantMap& value, int vehicleId, int messageId, const QString& expectedId = QString())
{
    if (value.size() != 32 || value.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || value.value(QStringLiteral("schemaVersion")).toInt() != 1
        || value.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-message-interval-query"))
        || !yes(value.value(QStringLiteral("requestAccepted"))) || !stringValue(value.value(QStringLiteral("requestId")), 64)
        || (!expectedId.isEmpty() && value.value(QStringLiteral("requestId")).toString() != expectedId)) return false;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("messageId")})
        if (value.value(key).type() != QVariant::Int || value.value(key).isNull()) return false;
    if (value.value(QStringLiteral("vehicleId")).toInt() != vehicleId
        || value.value(QStringLiteral("messageId")).toInt() != messageId
        || value.value(QStringLiteral("componentId")).toInt() < 1 || value.value(QStringLiteral("componentId")).toInt() > 255) return false;
    for (const QString& key : {QStringLiteral("active"), QStringLiteral("requestIssued"), QStringLiteral("ackObserved"),
        QStringLiteral("reportObserved"), QStringLiteral("sourceCurrent")})
        if (value.value(key).type() != QVariant::Bool || value.value(key).isNull()) return false;
    if (!stringValue(value.value(QStringLiteral("state")), 64) || !stringValue(value.value(QStringLiteral("reason")), 256, true)
        || !QStringList{QStringLiteral("waiting-ack-and-report"), QStringLiteral("waiting-ack"), QStringLiteral("waiting-report"),
            QStringLiteral("complete"), QStringLiteral("failed"), QStringLiteral("cancelled"), QStringLiteral("unavailable")}
                .contains(value.value(QStringLiteral("state")).toString())) return false;
    for (const QString& key : {QStringLiteral("receiptSequence"), QStringLiteral("baselineReceiptSequence"),
        QStringLiteral("contentRevision"), QStringLiteral("mutationGeneration")}) if (!sequence(value.value(key))) return false;
    for (const QString& key : {QStringLiteral("versionEpoch"), QStringLiteral("transportEpoch")})
        if (!stringValue(value.value(key), 128)) return false;
    if (!stringValue(value.value(QStringLiteral("ledgerEpoch")), 128, true)) return false;
    const QVariant remaining = value.value(QStringLiteral("remainingMs"));
    if (remaining.isNull() || (remaining.type() != QVariant::LongLong && remaining.type() != QVariant::Int)
        || remaining.toLongLong() < 0 || remaining.toLongLong() > ItemTimeoutMs) return false;
    for (const QString& key : {QStringLiteral("ackResult"), QStringLiteral("intervalUs")}) {
        const QVariant item = value.value(key);
        if (!item.isNull() && item.type() != QVariant::Int) return false;
    }
    if (!yes(value.value(QStringLiteral("readOnly")))) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("responseAttributionVerified"),
        QStringLiteral("actualTransmitRateVerified"), QStringLiteral("availableBandwidthVerified"),
        QStringLiteral("configurationPermitted"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!no(value.value(key))) return false;
    return true;
}
bool completeQuery(const QVariantMap& value)
{
    quint64 receipt = 0, baseline = 0;
    return value.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete"))
        && no(value.value(QStringLiteral("active"))) && yes(value.value(QStringLiteral("sourceCurrent")))
        && yes(value.value(QStringLiteral("requestIssued"))) && yes(value.value(QStringLiteral("ackObserved")))
        && value.value(QStringLiteral("ackResult")).type() == QVariant::Int
        && value.value(QStringLiteral("ackResult")).toInt() == MAV_RESULT_ACCEPTED
        && yes(value.value(QStringLiteral("reportObserved")))
        && value.value(QStringLiteral("intervalUs")).type() == QVariant::Int
        && !value.value(QStringLiteral("intervalUs")).isNull() && value.value(QStringLiteral("intervalUs")).toInt() >= -1
        && sequence(value.value(QStringLiteral("receiptSequence")), &receipt)
        && sequence(value.value(QStringLiteral("baselineReceiptSequence")), &baseline) && receipt > baseline
        && stringValue(value.value(QStringLiteral("ledgerEpoch")), 128);
}
QVariantMap batchSource(const QVariantMap& query, bool includeLedger)
{
    QVariantMap result;
    for (const QString& key : {QStringLiteral("versionEpoch"), QStringLiteral("transportEpoch"), QStringLiteral("mutationGeneration")})
        result.insert(key, ownedValue(query.value(key)));
    if (includeLedger) result.insert(QStringLiteral("ledgerEpoch"), ownedValue(query.value(QStringLiteral("ledgerEpoch"))));
    return result;
}
QVariantMap stableCompletedQuery(QVariantMap query)
{
    query.remove(QStringLiteral("remainingMs"));
    return query;
}
QVariantMap row(int messageId, const QString& state, const QString& reason, const QVariantMap& query)
{
    return {{QStringLiteral("messageId"), messageId}, {QStringLiteral("state"), state},
        {QStringLiteral("reason"), reason}, {QStringLiteral("query"), ownMap(query)}};
}
}

FleetOptionalTelemetryCapture::FleetOptionalTelemetryCapture(SwarmController* actualSwarm, FleetRegistry* actualRegistry)
    : QObject(actualSwarm), _swarm(actualSwarm), _registry(actualRegistry)
{
    _clock.start();
    for (int messageId : MessageIds) _rows.append(row(messageId, QStringLiteral("queued"), QStringLiteral(""), {}));
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, &FleetOptionalTelemetryCapture::tick);
}

FleetOptionalTelemetryCapture::~FleetOptionalTelemetryCapture()
{
    _destroying = true; _active = false; _timer.stop();
    disconnectSources();
    // Native records retain their own original finite timeout. The frozen
    // fence/owner QPointer prevents any later write after this owner is gone.
}

QVariantMap FleetOptionalTelemetryCapture::result(bool accepted, const QString& reason, const QString& requestId)
{
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("requestAccepted"), accepted},
        {QStringLiteral("requestId"), ownedText(requestId)}, {QStringLiteral("reason"), ownedText(reason)},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetOptionalTelemetryCapture::current() const
{
    if (QThread::currentThread() != thread() || _destroying) return {};
    QVariantList ids, rows;
    int completed = 0;
    for (int messageId : MessageIds) ids.append(messageId);
    for (const QVariant& value : _rows) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("state")) == QVariant(QStringLiteral("observed"))) ++completed;
        rows.append(ownedValue(item));
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), ownedText(_requestId)},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("state"), ownedText(_state)},
        {QStringLiteral("reason"), ownedText(_reason)}, {QStringLiteral("active"), _active},
        {QStringLiteral("scopeToken"), ownedText(_scopeToken)}, {QStringLiteral("vehicleId"), _vehicleId},
        {QStringLiteral("currentMessageId"), _active && _messageIndex < 3 ? MessageIds[_messageIndex] : 0},
        {QStringLiteral("messageIds"), ids}, {QStringLiteral("rows"), rows},
        {QStringLiteral("completedMessageCount"), completed},
        {QStringLiteral("remainingMs"), _active ? qMax<qint64>(0, _deadlineMs - _clock.elapsed()) : qint64(0)},
        {QStringLiteral("totalLifetimeMs"), LifetimeMs}, {QStringLiteral("itemTimeoutMs"), ItemTimeoutMs},
        {QStringLiteral("readOnlyRequestsMayRemainInflight"), _queryMayRemainInflight},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("responseAttributionVerified"), false}, {QStringLiteral("actualTransmitRateVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
}

bool FleetOptionalTelemetryCapture::recordCurrent() const
{
    if (!_swarm || !_registry || !_vehicle || !_links || !_primary || _primaryOwner.expired()
        || !_swarm->ownsOptionalTelemetryCapture(const_cast<FleetOptionalTelemetryCapture*>(this), _registry.data())
        || _vehicle->thread() != thread() || _links->thread() != thread() || _primary->thread() != thread()
        || _vehicle->vehicleLinkManager() != _links.data() || _vehicle->id() != _vehicleId) return false;
    QString recordEpoch, bindingEpoch;
    if (_swarm->optionalTelemetryCaptureVehicle(const_cast<FleetOptionalTelemetryCapture*>(this), _vehicleId,
        &recordEpoch, &bindingEpoch) != _vehicle.data() || recordEpoch != _recordEpoch || bindingEpoch != _bindingEpoch) return false;
    const WeakLinkInterfacePtr current = _links->primaryLink();
    // Keep the original pointer and control block without acquiring a new
    // strong owner in this callback-free fence. The native query sender also
    // validates that pointer against its original retained physical member.
    return !current.expired() && !current.owner_before(_primaryOwner) && !_primaryOwner.owner_before(current);
}

bool FleetOptionalTelemetryCapture::configurationBasisSourceCurrent(quint64 generation, const QString& requestId) const
{
    // This is the source/lifetime part, NOT a read/capture or configure grant.
    // Native owns the report revisions and the precisely expected mutations.
    return !_destroying && QThread::currentThread() == thread() && generation != 0 && _generation == generation
        && _requestId == requestId && !_configurationBasisRetired && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs
        && ((_active && _state == QStringLiteral("capturing") && _messageIndex == 3)
            || (!_active && _state == QStringLiteral("observations-captured")))
        && _swarm && _registry && _root && _session && _roles && _vehicle && _links && _primary
        && parent() == _swarm.data() && _swarm->parent() == _root.data() && _registry->parent() == _swarm.data()
        && _root->thread() == thread() && _session->thread() == thread() && _roles->thread() == thread()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role) && recordCurrent();
}

std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> FleetOptionalTelemetryCapture::takeConfigurationBasis(
    FleetOptionalTelemetryConfiguration* actualOwner, const QString& captureId)
{
    if (!actualOwner || !_swarm || !_registry || _busy || _checking || _active
        || _configurationBasisTransferred || !_configurationBasis
        || captureId.isEmpty() || captureId != _requestId || actualOwner->thread() != thread()
        || !_swarm->ownsOptionalTelemetryConfiguration(actualOwner, _registry.data(), this)
        || !configurationBasisSourceCurrent(_generation, captureId)
        || !_vehicle->fleetOptionalTelemetryConfigurationBasisCurrent(_configurationBasis)) return {};
    // Burn the handoff before any consumer can enter an OS/plugin callback.
    // A rejected/cancelled review does not make the old basis reusable.
    _configurationBasisTransferred = true;
    auto basis = _configurationBasis;
    _configurationBasis.reset();
    return basis;
}

bool FleetOptionalTelemetryCapture::memoryCurrent(quint64 generation) const
{
    return !_destroying && QThread::currentThread() == thread() && _active && generation != 0 && _generation == generation
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _swarm && _registry && _session && _roles && _root
        && _swarm->thread() == thread() && _registry->thread() == thread() && _session->thread() == thread()
        && _roles->thread() == thread() && _root->thread() == thread()
        && parent() == _swarm.data() && _swarm->parent() == _root.data() && _registry->parent() == _swarm.data()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data()) && recordCurrent();
}

bool FleetOptionalTelemetryCapture::contextMatches(QObject* actualSession, QObject* actualRoles,
    const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const
{
    if (!memoryCurrent(_generation) || actualSession != _session.data() || actualRoles != _roles.data()
        || expectedPrimary.size() != 5 || expectedRole.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("actorId"), QStringLiteral("role")})
        if (!stringValue(expectedPrimary.value(key), 128) || expectedPrimary.value(key) != _identity.value(key)) return false;
    return roleMatches(_identity, expectedRole) && expectedRole == _role && _roles->actionSnapshotCurrent(_role);
}

bool FleetOptionalTelemetryCapture::gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (!memoryCurrent(generation) || !_vehicle->fleetMessageIntervalQueryTransportQuiescent()) return false;
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const QString requestId = _requestId, scope = _scopeToken, state = _state, nativeId = _nativeQueryId, firstId = _batchQueryId;
    const QVariantMap identity = _identity, role = _role, source = _batchSource;
    const QVariantList rows = _rows;
    const int index = _messageIndex;
    const qint64 itemDeadline = _itemDeadlineMs, deadline = _deadlineMs;
    _checking = true; _reentered = false;
    const BusyGuard checking{self, &_checking, false};
    const auto same = [&]() {
        return self && !_reentered && memoryCurrent(generation) && session && session == _session && roles && roles == _roles
            && root && root == _root && swarm && swarm == _swarm && registry && registry == _registry
            && _requestId == requestId && _scopeToken == scope && _state == state && _nativeQueryId == nativeId
            && _batchQueryId == firstId && _identity == identity && _role == role && _batchSource == source && _rows == rows
            && _messageIndex == index && _itemDeadlineMs == itemDeadline && _deadlineMs == deadline;
    };
    const auto valuesCurrent = [&]() {
        if (!same() || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role)) return false;
        if (!firstId.isEmpty()) {
            const QVariantMap first = _vehicle->fleetMessageIntervalQuerySnapshot(self.data(), firstId);
            if (!same() || !queryShape(first, _vehicleId, MessageIds[0], firstId) || !yes(first.value(QStringLiteral("sourceCurrent")))
                || batchSource(first, source.contains(QStringLiteral("ledgerEpoch"))) != source) return false;
        }
        for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
            const QVariantMap retained = rows.at(rowIndex).toMap();
            if (retained.value(QStringLiteral("state")) != QVariant(QStringLiteral("observed"))) continue;
            const QVariantMap before = retained.value(QStringLiteral("query")).toMap();
            const QString id = before.value(QStringLiteral("requestId")).toString();
            const QVariantMap now = _vehicle->fleetMessageIntervalQuerySnapshot(self.data(), id);
            if (!same() || !queryShape(now, _vehicleId, MessageIds[rowIndex], id) || !completeQuery(now)
                || stableCompletedQuery(now) != stableCompletedQuery(before)) return false;
        }
        return same();
    };
    if (!valuesCurrent()) return false;
    if (!_vehicle->fleetMessageIntervalQueryTransportQuiescent()) return false;
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!same() || !identityCurrent || !valuesCurrent()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetOptionalTelemetryCaptureSourcesCurrent",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, matched),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(registry.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && valuesCurrent();
}

void FleetOptionalTelemetryCapture::disconnectSources()
{
    const auto connections = _connections;
    _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

bool FleetOptionalTelemetryCapture::watchSources(quint64 generation)
{
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    QList<QMetaObject::Connection> staged;
    const auto stale = [self, generation]() {
        if (self && self->_generation == generation) {
            self->_configurationBasisRetired = true;
            self->finish(generation, QStringLiteral("stale"), QStringLiteral("capture-native-source-changed"));
        }
    };
    const auto add = [&](const QMetaObject::Connection& connection) {
        staged.append(connection);
        return bool(connection) && self && memoryCurrent(generation);
    };
    const QList<QPointer<QObject>> owners{_swarm.data(), _registry.data(), _session.data(), _roles.data(), _root.data(),
        _vehicle.data(), _links.data(), _primary.data()};
    for (const auto& owner : owners)
        if (!owner || !add(connect(owner.data(), &QObject::destroyed, this, stale))) {
            for (const auto& connection : staged) QObject::disconnect(connection);
            return false;
        }
    if (!add(connect(_session.data(), &FleetOperatorSession::changed, this, [self, generation, stale]() {
            // A new independent approver normally changes this Session. Once
            // handed off, the configure workflow owns its frozen identity and
            // realtime two-account checks; never treat its own prompt as a
            // physical source replacement. Before handoff, retire conservatively.
            if (self && self->_generation == generation && (!self->_configurationBasisTransferred || self->_active)) stale();
        }))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale))
        || !add(connect(_vehicle.data(), &Vehicle::armedChanged, this, [stale](bool armed) { if (armed) stale(); }))
        || !add(connect(_vehicle.data(), &Vehicle::flyingChanged, this, [stale](bool flying) { if (flying) stale(); }))
        || !add(connect(_vehicle.data(), &Vehicle::firmwareTypeChanged, this, stale))
        || !add(connect(_vehicle.data(), &Vehicle::vehicleTypeChanged, this, stale))
        || !add(connect(_links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale))
        || !add(connect(_links.data(), &VehicleLinkManager::communicationLostChanged, this, stale))
        || !add(connect(_primary.data(), &LinkInterface::disconnected, this, stale))
        || !add(connect(_registry.data(), &FleetRegistry::registryChanged, this, [self, generation]() {
            if (self && self->_generation == generation && !self->recordCurrent()) {
                self->_configurationBasisRetired = true;
                self->finish(generation, QStringLiteral("stale"), QStringLiteral("capture-registry-binding-changed"));
            }
        }))) {
        for (const auto& connection : staged) QObject::disconnect(connection);
        return false;
    }
    if (!self || !memoryCurrent(generation)) {
        for (const auto& connection : staged) QObject::disconnect(connection);
        return false;
    }
    _connections = staged;
    return true;
}

bool FleetOptionalTelemetryCapture::publish(quint64 generation)
{
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    emit changed();
    return self && memoryCurrent(generation) && gate(generation);
}

QVariantMap FleetOptionalTelemetryCapture::requestCapture(QObject* actualSession, QObject* actualRoles,
    QObject* actualRoot, const QString& requestedScope, int vehicleId, bool confirmed)
{
    const QString scope = ownedText(requestedScope);
    if (QThread::currentThread() != thread() || _destroying || _active || _busy || _checking || !confirmed
        || vehicleId < 1 || vehicleId > 255 || !text(scope, 32768)
        || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("confirmed-idle-native-capture-required"), QStringLiteral(""));
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - LifetimeMs)
        return result(false, QStringLiteral("capture-original-clock-exhausted"), QStringLiteral(""));
    const qint64 originalDeadline = enteredAt + LifetimeMs;
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    _busy = true; const BusyGuard requesting{self, &_busy, false};
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualSession));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualRoles));
    const QPointer<QObject> root(actualRoot);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const quint64 oldGeneration = _generation;
    const auto original = [&]() {
        return self && !_destroying && _generation == oldGeneration && !_active && _clock.elapsed() < originalDeadline
            && session && roles && root && swarm && registry
            && swarm == _swarm && registry == _registry && session->thread() == thread() && roles->thread() == thread()
            && root->thread() == thread() && parent() == swarm.data() && swarm->parent() == root.data()
            && swarm->ownsOptionalTelemetryCapture(self.data(), registry.data())
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data());
    };
    if (!original()) return result(false, QStringLiteral("actual-capture-session-role-root-required"), QStringLiteral(""));
    QString recordEpoch, bindingEpoch;
    const QPointer<Vehicle> vehicle(swarm->optionalTelemetryCaptureVehicle(self.data(), vehicleId, &recordEpoch, &bindingEpoch));
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const WeakLinkInterfacePtr primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    const auto retainedPrimary = primaryOwner.lock();
    const QPointer<LinkInterface> primary(retainedPrimary.get());
    if (!original() || !vehicle || !links || !primary || vehicle->thread() != thread() || links->thread() != thread()
        || primary->thread() != thread() || recordEpoch.isEmpty() || bindingEpoch.isEmpty())
        return result(false, QStringLiteral("actual-bound-vehicle-primary-required"), QStringLiteral(""));
    const auto originalSource = [&]() {
        if (!original() || !vehicle || !links || !primary || vehicle->vehicleLinkManager() != links.data()
            || vehicle->id() != vehicleId) return false;
        QString actualRecord, actualBinding;
        if (swarm->optionalTelemetryCaptureVehicle(self.data(), vehicleId, &actualRecord, &actualBinding) != vehicle.data()
            || actualRecord != recordEpoch || actualBinding != bindingEpoch) return false;
        const WeakLinkInterfacePtr current = links->primaryLink();
        return !current.expired() && !current.owner_before(primaryOwner) && !primaryOwner.owner_before(current);
    };
    // Resolve the original actual source before entering Session/OS code. A
    // timer nested by a RAW/plugin event loop is not an idle acquisition turn.
    if (!originalSource() || !vehicle->fleetMessageIntervalQueryTransportQuiescent())
        return result(false, QStringLiteral("capture-raw-receive-stack-busy"), QStringLiteral(""));
    const QVariantMap identity = session->identitySnapshot();
    if (!originalSource() || !primaryShape(identity))
        return result(false, QStringLiteral("capture-primary-identity-unavailable"), QStringLiteral(""));
    const QVariantMap role = roles->actionSnapshot(Action);
    if (!originalSource() || !vehicle->fleetMessageIntervalQueryTransportQuiescent())
        return result(false, QStringLiteral("capture-raw-receive-stack-busy"), QStringLiteral(""));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!originalSource() || !identityCurrent || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role))
        return result(false, QStringLiteral("capture-authenticated-independent-read-role-required"), QStringLiteral(""));
    disconnectSources();
    if (!originalSource()) return result(false, QStringLiteral("capture-source-changed-during-cleanup"), QStringLiteral(""));
    ++_generation;
    const quint64 generation = _generation;
    _configurationBasis.reset();
    _configurationBasisTransferred = false; _configurationBasisRetired = false;
    _session = session; _roles = roles; _root = root; _vehicle = vehicle; _links = links; _primary = primary; _primaryOwner = primaryOwner;
    _vehicleId = vehicleId;
    _identity = ownMap(identity); _role = ownMap(role); _recordEpoch = ownedText(recordEpoch); _bindingEpoch = ownedText(bindingEpoch);
    _requestId = QUuid::createUuid().toString(QUuid::WithoutBraces); _scopeToken = scope;
    const QString requestId = _requestId;
    _state = QStringLiteral("preparing"); _reason = QStringLiteral(""); _active = true;
    _deadlineMs = originalDeadline; _itemDeadlineMs = qMin(_deadlineMs, _clock.elapsed() + ItemTimeoutMs);
    _messageIndex = 0; _nativeQueryId.clear(); _batchQueryId.clear(); _batchSource.clear(); _rows.clear();
    _queryMayRemainInflight = false;
    for (int messageId : MessageIds) _rows.append(row(messageId, QStringLiteral("queued"), QStringLiteral(""), {}));
    if (!watchSources(generation) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("capture-initial-source-denied"));
        return result(false, QStringLiteral("capture-initial-source-denied"), requestId);
    }
    _state = QStringLiteral("capturing");
    _timer.start(); // The first actual query runs later, after Root owns this receipt.
    if (!publish(generation) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("capture-publication-source-changed"));
        return result(false, QStringLiteral("capture-publication-source-changed"), requestId);
    }
    return result(true, QStringLiteral("capture-request-accepted-not-evidence"), requestId);
}

void FleetOptionalTelemetryCapture::tick()
{
    if (_destroying || !_active || _busy || _checking || QThread::currentThread() != thread()) return;
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    _busy = true; const BusyGuard ticking{self, &_busy, false};
    const quint64 generation = _generation;
    if (!memoryCurrent(generation)) { finish(generation, QStringLiteral("expired"), QStringLiteral("capture-total-deadline-or-source-expired")); return; }
    if (_messageIndex < 3 && _clock.elapsed() >= _itemDeadlineMs) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("capture-item-deadline-expired")); return;
    }
    // RAW depth/probe is a temporary busy state, not evidence of a broken
    // source. Keep both original deadlines and retry only the timer admission,
    // without reading a transient-false snapshot or entering an OS check.
    if (!_vehicle->fleetMessageIntervalQueryTransportQuiescent()) return;
    if (!gate(generation) || !self) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("capture-authority-or-source-changed"));
        return;
    }
    if (_messageIndex >= 3) { finish(generation, QStringLiteral("observations-captured"), QStringLiteral("historical-query-snapshots-not-rate-certification")); return; }
    if (_clock.elapsed() >= _itemDeadlineMs) { finish(generation, QStringLiteral("failed"), QStringLiteral("capture-item-deadline-expired")); return; }
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<LinkInterface> primary(_primary);
    const int index = _messageIndex, messageId = MessageIds[index];
    if (_nativeQueryId.isEmpty()) {
        const QString requestId = _requestId;
        const auto fence = [self, generation, index, requestId, vehicle, primary]() {
            const auto same = [&]() {
                return self && vehicle && primary && self->memoryCurrent(generation) && self->_requestId == requestId
                    && self->_messageIndex == index && self->_nativeQueryId.isEmpty() && self->_vehicle == vehicle
                    && self->_primary == primary && self->_clock.elapsed() < self->_itemDeadlineMs;
            };
            return same() && self->gate(generation) && same();
        };
        // Conservative warning survives a cancellation/deletion inside native
        // callbacks; it is not a claim that write or remote application occurred.
        _queryMayRemainInflight = true;
        const QVariantMap started = vehicle->beginFleetMessageIntervalQuery(self.data(), primary.data(), messageId, fence);
        const QString startedId = started.value(QStringLiteral("requestId")).toString();
        if (!self) return;
        if (!memoryCurrent(generation) || _messageIndex != index || _requestId != requestId || _vehicle != vehicle) {
            // A notification inside the actual write may have cancelled this
            // batch before begin() could return its native ID. Retain that
            // exact old batch's partial facts, never revive it or touch a new
            // generation. The native result, not an inferred rollback, wins.
            const bool originalBatch = _generation == generation && _messageIndex == index
                && _requestId == requestId && _vehicle == vehicle;
            const bool retain = originalBatch && queryShape(started, _vehicleId, messageId);
            if (retain) {
                _rows[index] = row(messageId, _active ? QStringLiteral("querying") : _state,
                    QStringLiteral("native-query-returned-after-batch-retirement"), started);
                _queryMayRemainInflight = _queryMayRemainInflight || yes(started.value(QStringLiteral("requestIssued")));
                if (_active) {
                    _nativeQueryId = ownedText(startedId);
                    finish(generation, QStringLiteral("stale"), QStringLiteral("capture-retired-during-native-query"));
                    return;
                }
            }
            if (vehicle && !startedId.isEmpty()) vehicle->cancelFleetMessageIntervalQuery(self.data(), startedId);
            if (self && retain && _generation == generation && _requestId == requestId && _vehicle == vehicle) emit changed();
            return;
        }
        const bool knownStarted = queryShape(started, _vehicleId, messageId);
        if (!knownStarted || !yes(started.value(QStringLiteral("sourceCurrent")))) {
            if (knownStarted) {
                _nativeQueryId = ownedText(startedId);
                _rows[index] = row(messageId, QStringLiteral("failed"), QStringLiteral("native-query-source-retired"), started);
                finish(generation, QStringLiteral("failed"), QStringLiteral("native-interval-query-not-accepted-or-current"));
                return;
            }
            if (vehicle && !startedId.isEmpty()) vehicle->cancelFleetMessageIntervalQuery(self.data(), startedId);
            if (self) finish(generation, QStringLiteral("failed"), QStringLiteral("native-interval-query-not-accepted"));
            return;
        }
        _nativeQueryId = ownedText(startedId);
        _rows[index] = row(messageId, QStringLiteral("querying"), QStringLiteral(""), started);
        if (_batchQueryId.isEmpty()) {
            _batchQueryId = _nativeQueryId;
            _batchSource = batchSource(started, false);
        } else if (batchSource(started, true) != _batchSource) {
            finish(generation, QStringLiteral("stale"), QStringLiteral("interval-query-batch-source-changed")); return;
        }
        if (!gate(generation) || !self) {
            if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("capture-source-changed-after-query-start"));
            return;
        }
        if (!publish(generation) && self)
            finish(generation, QStringLiteral("stale"), QStringLiteral("capture-query-publication-source-changed"));
        return;
    }
    const QString nativeId = _nativeQueryId;
    const QVariantMap query = vehicle->fleetMessageIntervalQuerySnapshot(self.data(), nativeId);
    if (!queryShape(query, _vehicleId, messageId, nativeId) || !yes(query.value(QStringLiteral("sourceCurrent")))) {
        finish(generation, QStringLiteral("stale"), QStringLiteral("native-query-source-or-shape-changed")); return;
    }
    if (yes(query.value(QStringLiteral("active")))) return;
    if (!completeQuery(query)) {
        _rows[index] = row(messageId, QStringLiteral("failed"), QStringLiteral("native-query-not-complete"), query);
        finish(generation, QStringLiteral("failed"), QStringLiteral("native-query-not-complete")); return;
    }
    if (index == 0) _batchSource = batchSource(query, true);
    else if (batchSource(query, true) != _batchSource) {
        finish(generation, QStringLiteral("stale"), QStringLiteral("interval-query-complete-source-changed")); return;
    }
    _rows[index] = row(messageId, QStringLiteral("observed"), QStringLiteral(""), query);
    _nativeQueryId.clear(); ++_messageIndex;
    _itemDeadlineMs = qMin(_deadlineMs, _clock.elapsed() + ItemTimeoutMs);
    if (!gate(generation) || !self) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("completed-interval-query-no-longer-current"));
        return;
    }
    if (_messageIndex >= 3) {
        finish(generation, QStringLiteral("observations-captured"), QStringLiteral("historical-query-snapshots-not-rate-certification")); return;
    }
    if (!publish(generation) && self)
        finish(generation, QStringLiteral("stale"), QStringLiteral("capture-observation-publication-source-changed"));
    // Next item is deferred to a new timer turn outside RAW.
}

void FleetOptionalTelemetryCapture::finish(quint64 generation, const QString& state, const QString& reason)
{
    if (!_active || _generation != generation || _destroying) return;
    const QPointer<FleetOptionalTelemetryCapture> self(this);
    const bool previousBusy = _busy;
    _busy = true; const BusyGuard finishing{self, &_busy, previousBusy};
    const QPointer<Vehicle> vehicle(_vehicle);
    const QString requestId = _requestId;
    QSet<QString> queries;
    if (!_nativeQueryId.isEmpty()) queries.insert(_nativeQueryId);
    for (int index = 0; index < _rows.size(); ++index) {
        const QVariantMap retained = _rows.at(index).toMap();
        const QString id = retained.value(QStringLiteral("query")).toMap().value(QStringLiteral("requestId")).toString();
        if (id.isEmpty()) continue;
        queries.insert(id);
        // This getter is deliberately callback-free. Preserve the newest local
        // ACK/report/write facts before releasing this original native record,
        // even when its source has already become stale or the user cancelled.
        if (vehicle) {
            const QVariantMap latest = vehicle->fleetMessageIntervalQuerySnapshot(self.data(), id);
            if (queryShape(latest, _vehicleId, MessageIds[index], id)) {
                _rows[index] = row(MessageIds[index], retained.value(QStringLiteral("state")).toString(),
                    retained.value(QStringLiteral("reason")).toString(), latest);
                _queryMayRemainInflight = _queryMayRemainInflight || yes(latest.value(QStringLiteral("requestIssued")));
            }
        }
    }
    if (state == QStringLiteral("observations-captured") && _messageIndex == 3 && vehicle) {
        QStringList ids;
        for (const QVariant& value : _rows)
            ids.append(value.toMap().value(QStringLiteral("query")).toMap().value(QStringLiteral("requestId")).toString());
        const qint64 remaining = qMax<qint64>(0, _deadlineMs - _clock.elapsed());
        const auto sourceFence = [self, generation, requestId]() {
            return self && self->configurationBasisSourceCurrent(generation, requestId);
        };
        _configurationBasis = vehicle->sealFleetOptionalTelemetryCapture(this, ids, remaining, sourceFence);
        if (!self || _generation != generation || _requestId != requestId || _vehicle != vehicle) return;
    }
    _active = false; _state = state; _reason = reason; _timer.stop();
    _nativeQueryId.clear();
    if (!_configurationBasis) disconnectSources();
    if (!self || _generation != generation || _requestId != requestId) return;
    // Release only this actual owner's recorded IDs. A possibly written read
    // retains native drain/quarantine; terminal release never clears a new lane.
    for (const QString& id : queries) {
        if (vehicle) vehicle->cancelFleetMessageIntervalQuery(self.data(), id);
        if (!self || _generation != generation || _requestId != requestId || _vehicle != vehicle) return;
    }
    emit changed();
}

bool FleetOptionalTelemetryCapture::cancel(const QString& requestedId)
{
    const QString id = ownedText(requestedId);
    if (QThread::currentThread() != thread() || _destroying || !_active || id.isEmpty() || id != _requestId) return false;
    finish(_generation, QStringLiteral("cancelled"), QStringLiteral("operator-cancelled-read-only-interval-capture"));
    return true;
}

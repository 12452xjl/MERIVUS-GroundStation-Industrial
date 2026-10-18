#include "FleetLinkLatencyCapture.h"

#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <initializer_list>
#include <limits>

namespace {
constexpr qint64 LifetimeMs = 4000;
const QString Action = QStringLiteral("fleet.telemetry.rtt.capture");

struct BusyGuard {
    QPointer<FleetLinkLatencyCapture> owner;
    bool* flag;
    bool previous = false;
    ~BusyGuard() { if (owner) *flag = previous; }
};

QString ownText(const QString& value) { return value.isEmpty() ? QStringLiteral("") : QString(value.constData(), value.size()); }
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && !value.toBool(); }
bool text(const QString& value, int limit, bool empty = false)
{
    if ((!empty && value.isEmpty()) || value.size() > limit || value != value.trimmed()
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar character : value)
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format) return false;
    return true;
}
bool stringValue(const QVariant& value, int limit, bool empty = false)
{
    return value.type() == QVariant::String && !value.isNull() && text(value.toString(), limit, empty);
}
bool integer(const QVariant& value, int minimum, int maximum)
{
    return value.type() == QVariant::Int && !value.isNull() && value.toInt() >= minimum && value.toInt() <= maximum;
}
bool longInteger(const QVariant& value, qint64 minimum, qint64 maximum)
{
    return value.type() == QVariant::LongLong && !value.isNull()
        && value.toLongLong() >= minimum && value.toLongLong() <= maximum;
}
bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}
QVariant ownValue(const QVariant& value)
{
    // Only small, fixed native DTOs reach this copier; arbitrary caller maps
    // are never traversed or turned into a source or an account receipt.
    if (value.type() == QVariant::String) return ownText(value.toString());
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
            result.insert(ownText(it.key()), ownValue(it.value()));
        return result;
    }
    return value;
}
QVariantMap ownMap(const QVariantMap& value) { return ownValue(value).toMap(); }
bool primaryShape(const QVariantMap& identity)
{
    if (!exact(identity, {"schemaVersion", "sessionId", "sessionEpoch", "sessionRevision", "actorId", "displayName", "role",
            "authenticationSource", "authenticated", "issuedAtMonotonicMs", "expiresAtMonotonicMs"})
        || !yes(identity.value(QStringLiteral("authenticated")))) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
            QStringLiteral("actorId"), QStringLiteral("displayName"), QStringLiteral("role"), QStringLiteral("authenticationSource")})
        if (!stringValue(identity.value(key), 256)) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"), QStringLiteral("expiresAtMonotonicMs")}) {
        const QVariant value = identity.value(key);
        if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::LongLong && value.type() != QVariant::Double)
            || !std::isfinite(value.toDouble())) return false;
    }
    return identity.value(QStringLiteral("schemaVersion")).toDouble() == 1.0;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    if (!exact(role, {"actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision",
            "actorAuthenticated", "executionPermitted"})) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
            QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")})
        if (!stringValue(role.value(key), 128)) return false;
    return role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")) == QVariant(Action) && yes(role.value(QStringLiteral("allowed")))
        && no(role.value(QStringLiteral("requiresSecondApproval"))) && no(role.value(QStringLiteral("actorAuthenticated")))
        && no(role.value(QStringLiteral("executionPermitted")));
}
bool nonceValue(const QVariant& value)
{
    if (!stringValue(value, 19, true)) return false;
    const QString nonce = value.toString();
    if (nonce.isEmpty()) return true;
    if (nonce.startsWith(QLatin1Char('0'))) return false;
    for (QChar character : nonce) if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 number = nonce.toLongLong(&ok);
    return ok && number > 0 && QString::number(number) == nonce;
}
bool observationShape(const QVariantMap& value, int vehicleId)
{
    if (!exact(value, {"schemaVersion", "reportType", "requestId", "vehicleId", "componentId", "active", "state", "reason",
            "requestIssued", "responseObserved", "sourceCurrent", "remainingMs", "roundTripMs", "nonce", "versionEpoch",
            "transportEpoch", "snapshotOnly", "authenticated", "wireCausalityProven", "availableBandwidthVerified",
            "packetLossVerified", "remoteClockOffsetVerified", "configurationPermitted", "executionPermit", "flightCommandReleased", "ageMs"})
        || !integer(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || value.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-link-rtt"))
        || !stringValue(value.value(QStringLiteral("requestId")), 64)
        || QUuid(value.value(QStringLiteral("requestId")).toString()).isNull()
        || !integer(value.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(value.value(QStringLiteral("componentId")), 1, 255)
        || !stringValue(value.value(QStringLiteral("state")), 32)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-response"), QStringLiteral("complete"),
            QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(value.value(QStringLiteral("state")).toString())
        || !stringValue(value.value(QStringLiteral("reason")), 256, true)
        || !longInteger(value.value(QStringLiteral("remainingMs")), 0, LifetimeMs)
        || !longInteger(value.value(QStringLiteral("ageMs")), -1, std::numeric_limits<qint64>::max())
        || value.value(QStringLiteral("roundTripMs")).type() != QVariant::Double
        || value.value(QStringLiteral("roundTripMs")).isNull()
        || !std::isfinite(value.value(QStringLiteral("roundTripMs")).toDouble())
        || !nonceValue(value.value(QStringLiteral("nonce")))
        || !stringValue(value.value(QStringLiteral("versionEpoch")), 128, true)
        || !stringValue(value.value(QStringLiteral("transportEpoch")), 128, true)
        || !yes(value.value(QStringLiteral("snapshotOnly")))) return false;
    for (const char* key : {"active", "requestIssued", "responseObserved", "sourceCurrent"}) {
        const QVariant item = value.value(QString::fromLatin1(key));
        if (item.type() != QVariant::Bool || item.isNull()) return false;
    }
    for (const char* key : {"authenticated", "wireCausalityProven", "availableBandwidthVerified", "packetLossVerified",
            "remoteClockOffsetVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!no(value.value(QString::fromLatin1(key)))) return false;
    const bool response = yes(value.value(QStringLiteral("responseObserved")));
    const double elapsed = value.value(QStringLiteral("roundTripMs")).toDouble();
    if ((!response && (elapsed != -1.0 || value.value(QStringLiteral("ageMs")).toLongLong() != -1))
        || (response && (!yes(value.value(QStringLiteral("requestIssued"))) || elapsed < 0 || elapsed > double(LifetimeMs)))
        || (yes(value.value(QStringLiteral("requestIssued"))) && value.value(QStringLiteral("nonce")).toString().isEmpty())) return false;
    const QString state = value.value(QStringLiteral("state")).toString();
    return ((state == QStringLiteral("ready") || state == QStringLiteral("waiting-response")) == yes(value.value(QStringLiteral("active"))))
        && (state != QStringLiteral("complete") || response);
}
bool completeObservation(const QVariantMap& value)
{
    return value.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete"))
        && no(value.value(QStringLiteral("active"))) && yes(value.value(QStringLiteral("requestIssued")))
        && yes(value.value(QStringLiteral("responseObserved"))) && yes(value.value(QStringLiteral("sourceCurrent")))
        && value.value(QStringLiteral("ageMs")).toLongLong() >= 0 && value.value(QStringLiteral("ageMs")).toLongLong() <= 30000;
}
}

FleetLinkLatencyCapture::FleetLinkLatencyCapture(SwarmController* actualSwarm, FleetRegistry* actualRegistry)
    : QObject(actualSwarm), _swarm(actualSwarm), _registry(actualRegistry)
{
    _clock.start();
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, &FleetLinkLatencyCapture::tick);
}

FleetLinkLatencyCapture::~FleetLinkLatencyCapture()
{
    _destroying = true; _active = false; _timer.stop();
    disconnectSources();
    // Native keeps its own original finite deadline and owner QPointer. A
    // destroyed service cannot authorize another packet or revive a Run.
}

QVariantMap FleetLinkLatencyCapture::result(bool accepted, const QString& reason, const QString& requestId)
{
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("requestAccepted"), accepted},
        {QStringLiteral("requestId"), ownText(requestId)}, {QStringLiteral("reason"), ownText(reason)},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetLinkLatencyCapture::current() const
{
    if (QThread::currentThread() != thread() || _destroying) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), ownText(_requestId)},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("scopeToken"), ownText(_scopeToken)},
        {QStringLiteral("vehicleId"), _vehicleId}, {QStringLiteral("state"), ownText(_state)}, {QStringLiteral("active"), _active},
        {QStringLiteral("reason"), ownText(_reason)},
        {QStringLiteral("remainingMs"), _active ? qMax<qint64>(0, _deadlineMs - _clock.elapsed()) : qint64(0)},
        {QStringLiteral("observation"), ownMap(_observation)}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("protocol"), QStringLiteral("targeted-timesync-v2")}, {QStringLiteral("maximumAgeMs"), 30000},
        {QStringLiteral("requestAccepted"), !_requestId.isEmpty()}};
}

bool FleetLinkLatencyCapture::nativeVehicleMatches(const Vehicle* actualVehicle) const
{
    // Source pairing only; no Session/OS calls, signals, Link virtual probes
    // or temporary strong owner. Terminal diagnostics may outlive authority.
    if (_destroying || QThread::currentThread() != thread() || !_swarm || !_registry || !_vehicle || !_links || !_primary
        || actualVehicle != _vehicle.data() || _primaryOwner.expired() || parent() != _swarm.data()
        || !_swarm->ownsLinkLatencyCapture(const_cast<FleetLinkLatencyCapture*>(this), _registry.data())
        || _vehicle->thread() != thread() || _links->thread() != thread() || _primary->thread() != thread()
        || _vehicle->vehicleLinkManager() != _links.data() || _vehicle->id() != _vehicleId) return false;
    QString record, binding;
    if (_swarm->linkLatencyCaptureVehicle(const_cast<FleetLinkLatencyCapture*>(this), _vehicleId, &record, &binding) != actualVehicle
        || record != _recordEpoch || binding != _bindingEpoch) return false;
    const WeakLinkInterfacePtr current = _links->primaryLink();
    return !current.expired() && !current.owner_before(_primaryOwner) && !_primaryOwner.owner_before(current);
}

bool FleetLinkLatencyCapture::memoryCurrent(quint64 generation) const
{
    return !_destroying && QThread::currentThread() == thread() && _active && generation != 0 && _generation == generation
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _swarm && _registry && _session && _roles && _root
        && _swarm->thread() == thread() && _registry->thread() == thread() && _session->thread() == thread()
        && _roles->thread() == thread() && _root->thread() == thread() && _swarm->parent() == _root.data()
        && _registry->parent() == _swarm.data() && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && nativeVehicleMatches(_vehicle.data()) && !_vehicle->armed() && !_vehicle->flying();
}

bool FleetLinkLatencyCapture::nativeResponseCurrent(const std::shared_ptr<FleetLinkLatencyRun>& run) const
{
    // Response admission alone is bounded by the original service entry
    // deadline. No OS/probe/quiescence check belongs in the RAW receiver, and
    // this must not become a source gate for already complete diagnostics.
    return !_destroying && run && _run == run && _active && memoryCurrent(_generation)
        && roleMatches(_identity, _role) && _roles->actionSnapshotCurrent(_role);
}

bool FleetLinkLatencyCapture::contextMatches(QObject* actualSession, QObject* actualRoles,
    const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const
{
    if (!memoryCurrent(_generation) || actualSession != _session.data() || actualRoles != _roles.data()
        || expectedPrimary.size() != 5 || expectedRole.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
            QStringLiteral("actorId"), QStringLiteral("role")})
        if (!stringValue(expectedPrimary.value(key), 128) || expectedPrimary.value(key) != _identity.value(key)) return false;
    return roleMatches(_identity, expectedRole) && expectedRole == _role && _roles->actionSnapshotCurrent(_role);
}

bool FleetLinkLatencyCapture::gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (!memoryCurrent(generation) || !_vehicle->fleetLinkLatencyCaptureTransportQuiescent()) return false;
    const QPointer<FleetLinkLatencyCapture> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<Vehicle> vehicle(_vehicle);
    const QString requestId = _requestId, scope = _scopeToken, state = _state;
    const QVariantMap identity = _identity, role = _role;
    const auto run = _run;
    const qint64 deadline = _deadlineMs;
    _checking = true; _reentered = false;
    const BusyGuard checking{self, &_checking, false};
    const auto same = [&]() {
        return self && !_reentered && memoryCurrent(generation) && session && session == _session && roles && roles == _roles
            && root && root == _root && swarm && swarm == _swarm && registry && registry == _registry && vehicle && vehicle == _vehicle
            && _requestId == requestId && _scopeToken == scope && _state == state && _identity == identity && _role == role
            && _run == run && _deadlineMs == deadline;
    };
    if (!same() || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role)
        || !vehicle->fleetLinkLatencyCaptureTransportQuiescent()) return false;
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!same() || !identityCurrent || !roles->actionSnapshotCurrent(role)) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetLinkLatencyCaptureSourcesCurrent", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(registry.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && roles->actionSnapshotCurrent(role);
}

void FleetLinkLatencyCapture::disconnectSources()
{
    const auto connections = _connections;
    _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

bool FleetLinkLatencyCapture::watchSources(quint64 generation)
{
    const QPointer<FleetLinkLatencyCapture> self(this);
    QList<QMetaObject::Connection> staged;
    const auto stale = [self, generation]() {
        if (self && self->_generation == generation)
            self->finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-capture-source-changed"));
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
    if (!add(connect(_session.data(), &FleetOperatorSession::changed, this, stale))
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
            if (self && self->_generation == generation && !self->nativeVehicleMatches(self->_vehicle.data()))
                self->finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-capture-binding-changed"));
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

bool FleetLinkLatencyCapture::publish(quint64 generation)
{
    const QPointer<FleetLinkLatencyCapture> self(this);
    emit changed();
    return self && memoryCurrent(generation) && gate(generation);
}

QVariantMap FleetLinkLatencyCapture::requestCapture(QObject* actualSession, QObject* actualRoles,
    QObject* actualRoot, const QString& requestedScope, int vehicleId, bool confirmed)
{
    const QString scope = ownText(requestedScope);
    if (QThread::currentThread() != thread() || _destroying || _active || _busy || _checking || !confirmed
        || vehicleId < 1 || vehicleId > 255 || !text(scope, 32768) || !_clock.isValid() || !QElapsedTimer::isMonotonic()
        || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("confirmed-idle-rtt-capture-required"), QStringLiteral(""));
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - LifetimeMs)
        return result(false, QStringLiteral("rtt-capture-clock-exhausted"), QStringLiteral(""));
    const qint64 deadline = enteredAt + LifetimeMs;
    const QPointer<FleetLinkLatencyCapture> self(this);
    _busy = true; const BusyGuard requesting{self, &_busy, false};
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualSession));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualRoles));
    const QPointer<QObject> root(actualRoot);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const quint64 oldGeneration = _generation;
    const auto original = [&]() {
        return self && !_destroying && !_active && _generation == oldGeneration && _clock.elapsed() < deadline
            && session && roles && root && swarm && registry && swarm == _swarm && registry == _registry
            && session->thread() == thread() && roles->thread() == thread() && root->thread() == thread()
            && parent() == swarm.data() && swarm->parent() == root.data()
            && swarm->ownsLinkLatencyCapture(self.data(), registry.data())
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data());
    };
    if (!original()) return result(false, QStringLiteral("actual-rtt-session-role-root-required"), QStringLiteral(""));
    QString recordEpoch, bindingEpoch;
    const QPointer<Vehicle> vehicle(swarm->linkLatencyCaptureVehicle(self.data(), vehicleId, &recordEpoch, &bindingEpoch));
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const WeakLinkInterfacePtr primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    QPointer<LinkInterface> primary;
    { const auto retained = primaryOwner.lock(); primary = retained.get(); }
    const auto originalSource = [&]() {
        if (!original() || !vehicle || !links || !primary || vehicle->thread() != thread() || links->thread() != thread()
            || primary->thread() != thread() || vehicle->vehicleLinkManager() != links.data() || vehicle->id() != vehicleId
            || vehicle->armed() || vehicle->flying()) return false;
        QString record, binding;
        if (swarm->linkLatencyCaptureVehicle(self.data(), vehicleId, &record, &binding) != vehicle.data()
            || record.isEmpty() || record != recordEpoch || binding.isEmpty() || binding != bindingEpoch) return false;
        const WeakLinkInterfacePtr current = links->primaryLink();
        return !current.expired() && !current.owner_before(primaryOwner) && !primaryOwner.owner_before(current);
    };
    if (!originalSource() || !vehicle->fleetLinkLatencyCaptureTransportQuiescent())
        return result(false, QStringLiteral("rtt-native-source-or-transport-busy"), QStringLiteral(""));
    const QVariantMap identity = session->identitySnapshot();
    if (!originalSource() || !primaryShape(identity))
        return result(false, QStringLiteral("rtt-primary-identity-unavailable"), QStringLiteral(""));
    const QVariantMap role = roles->actionSnapshot(Action);
    if (!originalSource() || !vehicle->fleetLinkLatencyCaptureTransportQuiescent())
        return result(false, QStringLiteral("rtt-native-source-or-transport-busy"), QStringLiteral(""));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!originalSource() || !identityCurrent || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role))
        return result(false, QStringLiteral("rtt-independent-authenticated-read-scope-required"), QStringLiteral(""));
    disconnectSources();
    if (!originalSource()) return result(false, QStringLiteral("rtt-source-changed-during-cleanup"), QStringLiteral(""));
    const QPointer<Vehicle> oldVehicle(_vehicle);
    const auto oldRun = _run;
    if (oldVehicle && oldRun) oldVehicle->cancelFleetLinkLatencyCapture(self.data(), oldRun);
    if (!originalSource()) return result(false, QStringLiteral("rtt-source-changed-during-release"), QStringLiteral(""));
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _root = root; _vehicle = vehicle; _links = links; _primary = primary; _primaryOwner = primaryOwner;
    _vehicleId = vehicleId; _recordEpoch = ownText(recordEpoch); _bindingEpoch = ownText(bindingEpoch);
    _identity = ownMap(identity); _role = ownMap(role); _scopeToken = scope;
    _requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString requestId = _requestId;
    _run.reset(); _observation.clear(); _deadlineMs = deadline; _state = QStringLiteral("preparing"); _reason = QStringLiteral(""); _active = true;
    if (!watchSources(generation) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-initial-source-denied"));
        return result(false, QStringLiteral("rtt-initial-source-denied"), requestId);
    }
    _state = QStringLiteral("capturing");
    _timer.start(); // No wire until a later turn, after Root adopts the receipt.
    if (!publish(generation) || !self || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-publication-source-changed"));
        return result(false, QStringLiteral("rtt-publication-source-changed"), requestId);
    }
    return result(true, QStringLiteral("rtt-capture-accepted-not-a-measurement"), requestId);
}

void FleetLinkLatencyCapture::tick()
{
    if (_destroying || !_active || _busy || _checking || QThread::currentThread() != thread()) return;
    const QPointer<FleetLinkLatencyCapture> self(this);
    _busy = true; const BusyGuard ticking{self, &_busy, false};
    const quint64 generation = _generation;
    const QString requestId = _requestId;
    if (!memoryCurrent(generation)) { finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-original-deadline-or-source-expired")); return; }
    // Busy RAW is only a bounded wait. Check the original total deadline first
    // and do not enter Session/OS or inspect a transient-false native source.
    if (!_vehicle->fleetLinkLatencyCaptureTransportQuiescent()) return;
    if (!gate(generation) || !self) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-authority-or-source-changed"));
        return;
    }
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<LinkInterface> primary(_primary);
    if (!_run) {
        const auto fence = [self, generation, requestId, vehicle, primary]() {
            const auto same = [&]() {
                return self && vehicle && primary && self->memoryCurrent(generation) && self->_requestId == requestId
                    && self->_vehicle == vehicle && self->_primary == primary;
            };
            return same() && self->gate(generation) && same();
        };
        const auto started = vehicle->prepareFleetLinkLatencyCapture(self.data(), primary.data(),
            qMax<qint64>(0, _deadlineMs - _clock.elapsed()), fence);
        if (!self) return;
        const bool sameBatch = _generation == generation && _requestId == requestId && _vehicle == vehicle;
        if (!sameBatch) { if (vehicle && started) vehicle->cancelFleetLinkLatencyCapture(self.data(), started); return; }
        if (started) _run = started;
        const QVariantMap snapshot = started ? (vehicle ? vehicle->fleetLinkLatencyCaptureSnapshot(self.data(), started)
            : Vehicle::fleetLinkLatencyCaptureRetiredSnapshot(self.data(), started)) : QVariantMap();
        if (!self || _generation != generation || _requestId != requestId || _run != started) return;
        if (observationShape(snapshot, _vehicleId)) _observation = ownMap(snapshot);
        if (!_active) {
            if (vehicle && started) vehicle->cancelFleetLinkLatencyCapture(self.data(), started);
            if (self && _generation == generation && _requestId == requestId && _run == started) emit changed();
            return;
        }
        if (!started || !observationShape(snapshot, _vehicleId) || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) {
            finish(generation, QStringLiteral("failed"), QStringLiteral("rtt-native-prepare-failed")); return;
        }
        if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-prepare-publication-changed"));
        return;
    }
    const auto run = _run;
    const bool advanced = vehicle->advanceFleetLinkLatencyCapture(self.data(), run);
    if (!self) return;
    const QVariantMap snapshot = vehicle ? vehicle->fleetLinkLatencyCaptureSnapshot(self.data(), run)
        : Vehicle::fleetLinkLatencyCaptureRetiredSnapshot(self.data(), run);
    if (!self || _generation != generation || _requestId != requestId || _run != run) return;
    const bool valid = observationShape(snapshot, _vehicleId);
    if (valid) _observation = ownMap(snapshot);
    if (!_active) { emit changed(); return; } // Preserve late facts, never revive.
    if (!advanced || !valid) { finish(generation, QStringLiteral("failed"), QStringLiteral("rtt-native-run-unavailable")); return; }
    if (completeObservation(snapshot)) {
        if (!gate(generation) || !self) {
            if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-final-authority-changed"));
            return;
        }
        finish(generation, QStringLiteral("observed"), QStringLiteral("local-handoff-to-response-elapsed-not-pure-network-rtt"));
        return;
    }
    if (!yes(snapshot.value(QStringLiteral("active"))) || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("rtt-native-sample-not-complete-or-current")); return;
    }
    if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("rtt-progress-publication-changed"));
}

void FleetLinkLatencyCapture::finish(quint64 generation, const QString& requestedState, const QString& requestedReason)
{
    const QString state = ownText(requestedState), reason = ownText(requestedReason);
    if (!_active || _generation != generation || _destroying) return;
    const QPointer<FleetLinkLatencyCapture> self(this);
    const bool previousBusy = _busy;
    _busy = true; const BusyGuard finishing{self, &_busy, previousBusy};
    const QPointer<Vehicle> vehicle(_vehicle);
    const auto run = _run;
    const QString requestId = _requestId;
    if (run) {
        const QVariantMap latest = vehicle ? vehicle->fleetLinkLatencyCaptureSnapshot(self.data(), run)
            : Vehicle::fleetLinkLatencyCaptureRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        if (observationShape(latest, _vehicleId)) _observation = ownMap(latest);
    }
    _active = false; _state = state; _reason = reason; _timer.stop();
    disconnectSources();
    if (!self || _generation != generation || _requestId != requestId || _run != run) return;
    // A complete Run remains solely for a <=30s read-only native diagnostic.
    // Cancel/failure stops exactly this Run; an already possible write remains
    // recorded and is not claimed to have been withdrawn or rolled back.
    if (state != QStringLiteral("observed") && vehicle && run) {
        vehicle->cancelFleetLinkLatencyCapture(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        const QVariantMap latest = vehicle ? vehicle->fleetLinkLatencyCaptureSnapshot(self.data(), run)
            : Vehicle::fleetLinkLatencyCaptureRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        if (observationShape(latest, _vehicleId)) _observation = ownMap(latest);
    }
    emit changed();
}

bool FleetLinkLatencyCapture::cancel(const QString& requestedId)
{
    const QString id = ownText(requestedId);
    if (QThread::currentThread() != thread() || _destroying || !_active || id.isEmpty() || id != _requestId) return false;
    finish(_generation, QStringLiteral("cancelled"), QStringLiteral("operator-cancelled-targeted-rtt-capture"));
    return true;
}

QVariantMap FleetLinkLatencyCapture::latencyReview(int vehicleId) const
{
    const auto failure = [vehicleId](const QString& error) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("error"), ownText(error)},
            {QStringLiteral("vehicleId"), vehicleId >= 1 && vehicleId <= 255 ? vehicleId : 0},
            {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")},
            {QStringLiteral("observation"), QVariantMap()}, {QStringLiteral("snapshotOnly"), true}};
    };
    if (QThread::currentThread() != thread() || _destroying || vehicleId < 1 || vehicleId > 255
        || vehicleId != _vehicleId || !_run || !nativeVehicleMatches(_vehicle.data()))
        return failure(QStringLiteral("actual-rtt-vehicle-binding-required"));
    const QPointer<FleetLinkLatencyCapture> self(const_cast<FleetLinkLatencyCapture*>(this));
    const QPointer<Vehicle> vehicle(_vehicle);
    const auto run = _run;
    const quint64 generation = _generation;
    const QString requestId = _requestId, record = _recordEpoch, binding = _bindingEpoch;
    const auto same = [&]() {
        return self && vehicle && _generation == generation && _requestId == requestId && _run == run && _vehicle == vehicle
            && _vehicleId == vehicleId && _recordEpoch == record && _bindingEpoch == binding && nativeVehicleMatches(vehicle.data());
    };
    const QVariantMap observed = vehicle->fleetLinkLatencyCaptureSnapshot(self.data(), run);
    if (!same() || !observationShape(observed, vehicleId)) return failure(QStringLiteral("rtt-native-observation-invalid-or-replaced"));
    // A valid stale/failed diagnostic is still a successful native read. Only
    // complete + sourceCurrent + age <=30s may be treated as a known sample.
    const QVariantMap owned = ownMap(observed);
    if (!same()) return failure(QStringLiteral("rtt-native-source-changed-during-copy"));
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("vehicleRecordEpoch"), ownText(record)}, {QStringLiteral("bindingEpoch"), ownText(binding)},
        {QStringLiteral("observation"), owned}, {QStringLiteral("snapshotOnly"), true}};
}

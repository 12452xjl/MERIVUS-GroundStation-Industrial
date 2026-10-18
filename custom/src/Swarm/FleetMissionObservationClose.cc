#include "FleetMissionObservationClose.h"
#include "FleetMissionStartupSeries.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumMs = 30000;
constexpr int MaximumBytes = 256 * 1024;
constexpr double ExactMaximum = 9007199254740991.0;
const QString Action = QStringLiteral("task.observation.close");
const QString ApprovalAction = QStringLiteral("task.observation.close.approve");
const QString Disposition = QStringLiteral("end-local-observation-and-release-only-owned-local-reservations");
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool exactKeys(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}
bool text(const QVariant& value, int maximum)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > maximum
        || value.toString() != value.toString().trimmed() || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
    for (QChar character : value.toString()) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
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
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}
bool integer(const QVariant& value, double minimum, double maximum = ExactMaximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) { case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break; default: return false; }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= minimum && number <= maximum && std::floor(number) == number;
}
bool bounded(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 12 || ++(*nodes) > 16384) return false;
    if (!value.isValid()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumBytes || !bounded(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List: for (const QVariant& item : value.toList()) if (!bounded(item, depth + 1, nodes, bytes)) return false; return true;
    case QVariant::String: *bytes += value.toString().toUtf8().size(); return *bytes <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
QByteArray encoded(const QVariantMap& map)
{
    int nodes = 0; qint64 bytes = 0;
    if (map.isEmpty() || !bounded(map, 0, &nodes, &bytes)) return {};
    const QByteArray result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return result.size() <= MaximumBytes ? result : QByteArray();
}
bool sameMap(const QVariantMap& left, const QVariantMap& right)
{
    const QByteArray first = encoded(left), second = encoded(right);
    return !first.isEmpty() && !second.isEmpty() && first == second;
}
bool bindingShape(const QVariant& value)
{
    if (value.type() != QVariant::Map) return false;
    const QVariantMap map = value.toMap();
    QStringList keys{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (map.size() == 9) keys << QStringLiteral("inputGeneration") << QStringLiteral("reviewGeneration");
    return exactKeys(map, keys) && text(map.value(QStringLiteral("sourceTaskId")), 128) && uuid(map.value(QStringLiteral("recordId")))
        && integer(map.value(QStringLiteral("recordRevision")), 1) && digest(map.value(QStringLiteral("planId")))
        && digest(map.value(QStringLiteral("contentHash"))) && digest(map.value(QStringLiteral("reviewEvidenceHash")))
        && uuid(map.value(QStringLiteral("storeEpoch"))) && (map.size() == 7 || (integer(map.value(QStringLiteral("inputGeneration")), 0)
            && integer(map.value(QStringLiteral("reviewGeneration")), 0)));
}
bool reservationStateShape(const QVariant& value, const QVariant& owner, const QSet<int>& targetIds,
    bool currentlyHeld, QSet<int>* allOccupied)
{
    if (value.type() != QVariant::Map || !allOccupied) return false;
    const QVariantMap state = value.toMap();
    if (!exactKeys(state, {QStringLiteral("owner"), QStringLiteral("occupiedVehicleIds"), QStringLiteral("authorityRevoked")})
        || !uuid(state.value(QStringLiteral("owner"))) || state.value(QStringLiteral("owner")) != owner
        || state.value(QStringLiteral("authorityRevoked")).type() != QVariant::Bool
        || state.value(QStringLiteral("occupiedVehicleIds")).type() != QVariant::List) return false;
    const QVariantList occupied = state.value(QStringLiteral("occupiedVehicleIds")).toList();
    if (occupied.size() > 32 || currentlyHeld != !occupied.isEmpty()) return false;
    int previous = 0;
    for (const QVariant& id : occupied) {
        if (id.type() != QVariant::Int || id.isNull() || id.toInt() < 1 || id.toInt() > 255 || id.toInt() <= previous
            || !targetIds.contains(id.toInt()) || allOccupied->contains(id.toInt())) return false;
        previous = id.toInt(); allOccupied->insert(previous);
    }
    return true;
}
bool contextShape(const QVariantMap& map)
{
    if (!exactKeys(map, {QStringLiteral("schemaVersion"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
        QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("runId"), QStringLiteral("executionObservationTaskId"),
        QStringLiteral("binding"), QStringLiteral("targetVehicleIds"), QStringLiteral("startupState"), QStringLiteral("runReservationCurrentlyHeld"),
        QStringLiteral("startupReservationCurrentlyHeld"), QStringLiteral("reservationDisposition"), QStringLiteral("flightMayContinue"),
        QStringLiteral("missionResultVerified"), QStringLiteral("sourcesMayBeUnavailable"), QStringLiteral("targets"), QStringLiteral("localReservationState")})
        || !integer(map.value(QStringLiteral("schemaVersion")), 1, 1) || !uuid(map.value(QStringLiteral("startupId")))
        || !uuid(map.value(QStringLiteral("transactionEpoch"))) || !decimal(map.value(QStringLiteral("transactionGeneration")))
        || !uuid(map.value(QStringLiteral("deploymentId"))) || !uuid(map.value(QStringLiteral("runId")))
        || !uuid(map.value(QStringLiteral("executionObservationTaskId"))) || !bindingShape(map.value(QStringLiteral("binding")))
        || !text(map.value(QStringLiteral("startupState")), 64) || map.value(QStringLiteral("runReservationCurrentlyHeld")).type() != QVariant::Bool
        || map.value(QStringLiteral("startupReservationCurrentlyHeld")).type() != QVariant::Bool
        || map.value(QStringLiteral("reservationDisposition")).type() != QVariant::String
        || map.value(QStringLiteral("reservationDisposition")).toString() != Disposition || !yes(map.value(QStringLiteral("flightMayContinue")))
        || !no(map.value(QStringLiteral("missionResultVerified"))) || !yes(map.value(QStringLiteral("sourcesMayBeUnavailable")))
        || map.value(QStringLiteral("targetVehicleIds")).type() != QVariant::List || map.value(QStringLiteral("targets")).type() != QVariant::List
        || map.value(QStringLiteral("localReservationState")).type() != QVariant::Map) return false;
    const QVariantList ids = map.value(QStringLiteral("targetVehicleIds")).toList(), rows = map.value(QStringLiteral("targets")).toList();
    if (ids.isEmpty() || ids.size() > 32 || rows.size() != ids.size()) return false;
    QSet<int> seen;
    for (int index = 0; index < ids.size(); ++index) {
        const QVariant id = ids.at(index), rowValue = rows.at(index);
        if (id.type() != QVariant::Int || id.isNull() || id.toInt() < 1 || id.toInt() > 255 || seen.contains(id.toInt()) || rowValue.type() != QVariant::Map) return false;
        seen.insert(id.toInt());
        const QVariantMap row = rowValue.toMap();
        if (!exactKeys(row, {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("modeStepId"), QStringLiteral("modeQueueBegan"),
            QStringLiteral("modeQueueCommitted"), QStringLiteral("modeIssuedOrUncertain"), QStringLiteral("continuityUnknown"),
            QStringLiteral("sourceBindingActiveAtLastObservation"), QStringLiteral("trackingAtLastObservation"),
            QStringLiteral("vehicleObjectRetained"), QStringLiteral("missionObjectRetained")})
            || row.value(QStringLiteral("vehicleId")).type() != QVariant::Int || row.value(QStringLiteral("vehicleId")) != id
            || row.value(QStringLiteral("componentId")).type() != QVariant::Int || !integer(row.value(QStringLiteral("componentId")), 1, 255)
            || !uuid(row.value(QStringLiteral("modeStepId")))) return false;
        for (const QString& key : {QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"), QStringLiteral("modeIssuedOrUncertain"),
            QStringLiteral("continuityUnknown"), QStringLiteral("sourceBindingActiveAtLastObservation"), QStringLiteral("trackingAtLastObservation"),
            QStringLiteral("vehicleObjectRetained"), QStringLiteral("missionObjectRetained")}) if (row.value(key).type() != QVariant::Bool) return false;
    }
    const QVariantMap reservation = map.value(QStringLiteral("localReservationState")).toMap();
    QSet<int> allOccupied;
    return exactKeys(reservation, {QStringLiteral("run"), QStringLiteral("startup")})
        && reservationStateShape(reservation.value(QStringLiteral("run")), map.value(QStringLiteral("runId")), seen,
            map.value(QStringLiteral("runReservationCurrentlyHeld")).toBool(), &allOccupied)
        && reservationStateShape(reservation.value(QStringLiteral("startup")), map.value(QStringLiteral("startupId")), seen,
            map.value(QStringLiteral("startupReservationCurrentlyHeld")).toBool(), &allOccupied)
        && !encoded(map).isEmpty();
}
bool identityShape(const QVariantMap& map)
{
    return exactKeys(map, {QStringLiteral("schemaVersion"), QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("actorId"), QStringLiteral("displayName"), QStringLiteral("role"), QStringLiteral("authenticationSource"), QStringLiteral("authenticated"),
        QStringLiteral("issuedAtMonotonicMs"), QStringLiteral("expiresAtMonotonicMs")}) && integer(map.value(QStringLiteral("schemaVersion")), 1, 1)
        && uuid(map.value(QStringLiteral("sessionId"))) && uuid(map.value(QStringLiteral("sessionEpoch"))) && decimal(map.value(QStringLiteral("sessionRevision")))
        && text(map.value(QStringLiteral("actorId")), 128) && text(map.value(QStringLiteral("displayName")), 512)
        && text(map.value(QStringLiteral("role")), 64) && text(map.value(QStringLiteral("authenticationSource")), 128)
        && yes(map.value(QStringLiteral("authenticated"))) && integer(map.value(QStringLiteral("issuedAtMonotonicMs")), 0)
        && integer(map.value(QStringLiteral("expiresAtMonotonicMs")), 1)
        && map.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() > map.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    return identityShape(identity) && exactKeys(role, {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"), QStringLiteral("allowed"),
        QStringLiteral("requiresSecondApproval"), QStringLiteral("policyEpoch"), QStringLiteral("policyRevision"), QStringLiteral("actorAuthenticated"), QStringLiteral("executionPermitted")})
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId")) && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String && role.value(QStringLiteral("action")).toString() == Action
        && uuid(role.value(QStringLiteral("policyEpoch"))) && decimal(role.value(QStringLiteral("policyRevision")))
        && yes(role.value(QStringLiteral("allowed"))) && yes(role.value(QStringLiteral("requiresSecondApproval")))
        && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
bool approverShape(const QVariantMap& map, const QVariantMap& primary, const QString& hash)
{
    if (map.size() != 16) return false;
    QVariantMap identity = map;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"), QStringLiteral("applicantSessionId"),
        QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) {
        if (map.value(key).type() != QVariant::String) return false;
        identity.remove(key);
    }
    return identityShape(identity) && map.value(QStringLiteral("action")).toString() == ApprovalAction
        && map.value(QStringLiteral("bindingHash")).toString() == hash && digest(hash)
        && map.value(QStringLiteral("actorId")) != primary.value(QStringLiteral("actorId"))
        && map.value(QStringLiteral("sessionEpoch")) == primary.value(QStringLiteral("sessionEpoch"))
        && map.value(QStringLiteral("sessionRevision")) == primary.value(QStringLiteral("sessionRevision"))
        && map.value(QStringLiteral("applicantSessionId")) == primary.value(QStringLiteral("sessionId"))
        && map.value(QStringLiteral("applicantSessionRevision")) == primary.value(QStringLiteral("sessionRevision"))
        && map.value(QStringLiteral("applicantActorId")) == primary.value(QStringLiteral("actorId"));
}
bool dispositionShape(const QVariantMap& map)
{
    if (!exactKeys(map, {QStringLiteral("monitoringRetired"), QStringLiteral("runReservationCurrentlyHeld"),
        QStringLiteral("startupReservationCurrentlyHeld"), QStringLiteral("localReservationsReleasedVerified")})) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (it.value().type() != QVariant::Bool) return false;
    return map.value(QStringLiteral("localReservationsReleasedVerified")).toBool()
        == (!map.value(QStringLiteral("runReservationCurrentlyHeld")).toBool() && !map.value(QStringLiteral("startupReservationCurrentlyHeld")).toBool());
}
QObject* object(const QVariant& value) { return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr; }
QVariantMap outcome(const QString& state, bool retired)
{
    return {{QStringLiteral("ok"), retired}, {QStringLiteral("state"), state}, {QStringLiteral("monitoringRetired"), retired ? QVariant(true) : QVariant()},
        {QStringLiteral("monitoringRetiredKnown"), retired}, {QStringLiteral("localReservationsReleaseKnown"), false}, {QStringLiteral("localReservationsReleased"), QVariant()},
        {QStringLiteral("flightMayContinue"), true}, {QStringLiteral("physicalStateVerified"), false}, {QStringLiteral("sourceFreshnessVerified"), false},
        {QStringLiteral("sourceStateVerified"), false}, {QStringLiteral("remoteControlReleasedVerified"), false},
        {QStringLiteral("missionResultVerified"), false}, {QStringLiteral("missionCompleted"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

FleetMissionObservationClose::FleetMissionObservationClose(QObject* parent)
    : QObject(parent), _workflowEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(250);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (active() && _clock.elapsed() >= _deadlineMs) invalidate(_generation, QStringLiteral("original-close-review-budget-expired"), QStringLiteral("expired"));
    });
}
FleetMissionObservationClose::~FleetMissionObservationClose() { _timer.stop(); disconnectSources(); }
bool FleetMissionObservationClose::active() const
{
    return _state == QStringLiteral("preparing") || _state == QStringLiteral("awaiting-review")
        || _state == QStringLiteral("authenticating") || _state == QStringLiteral("consuming");
}
bool FleetMissionObservationClose::memoryCurrent(quint64 generation, const QString& state) const
{
    return QThread::currentThread() == thread() && generation != 0 && _generation == generation && _state == state
        && active() && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && uuid(_workflowEpoch) && uuid(_reviewId)
        && uuid(_startupId) && uuid(_runId) && _session && _roles && _series && _transaction && _authorization
        && _session->thread() == thread() && _roles->thread() == thread() && _series->thread() == thread()
        && _transaction->thread() == thread() && _authorization->thread() == thread()
        && ((state == QStringLiteral("preparing") && !_nativeOwnersFrozen) || (_nativeOwnersFrozen && _runOwner && _swarmOwner && _orchestratorOwner
            && _runOwner->thread() == thread() && _swarmOwner->thread() == thread() && _orchestratorOwner->thread() == thread()));
}
QString FleetMissionObservationClose::bindingHash() const
{
    const QByteArray bytes = encoded({{QStringLiteral("reviewUUID"), _reviewId}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("context"), _context},
        {QStringLiteral("identity"), _identity}, {QStringLiteral("role"), _role}});
    return bytes.isEmpty() ? QString() : QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}
QVariantMap FleetMissionObservationClose::sourceBundle() const
{
    const auto actual = [](QObject* value) { return QVariant::fromValue(value); };
    return {{QStringLiteral("session"), actual(_session.data())}, {QStringLiteral("roles"), actual(_roles.data())},
        {QStringLiteral("startupSeries"), actual(_series.data())}, {QStringLiteral("authorizationOwner"), actual(_authorization.data())}};
}
bool FleetMissionObservationClose::nativeCurrent(quint64 generation, const QString& state) const
{
    if (!memoryCurrent(generation, state) || !_nativeOwnersFrozen || !contextShape(_context) || !roleMatches(_identity, _role) || _bindingHash != bindingHash()
        || _context.value(QStringLiteral("startupId")).toString() != _startupId || _context.value(QStringLiteral("runId")).toString() != _runId) return false;
    const QPointer<FleetMissionObservationClose> self(const_cast<FleetMissionObservationClose*>(this));
    const QPointer<FleetMissionStartupSeries> series(_series);
    const QPointer<FleetMissionStartupTransaction> transaction(_transaction);
    if (series->previousStartup() != transaction.data() || !self || !memoryCurrent(generation, state)) return false;
    const bool contextCurrent = transaction->observationCloseContextCurrent(_startupId, _context);
    if (!self || !contextCurrent || !memoryCurrent(generation, state)) return false;
    const QVariantMap owners = transaction->observationCloseSourceObjects(_startupId, _context);
    if (!self || !memoryCurrent(generation, state) || !exactKeys(owners, {QStringLiteral("run"), QStringLiteral("swarm"), QStringLiteral("orchestrator")})
        || object(owners.value(QStringLiteral("run"))) != _runOwner.data() || object(owners.value(QStringLiteral("swarm"))) != _swarmOwner.data()
        || object(owners.value(QStringLiteral("orchestrator"))) != _orchestratorOwner.data()) return false;
    return _roles->actionSnapshotCurrent(_role) && memoryCurrent(generation, state);
}
bool FleetMissionObservationClose::sourcesCurrent(quint64 generation, const QString& state, bool requireApprover)
{
    if (_checking || !nativeCurrent(generation, state)) return false;
    _checking = true; FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionObservationClose> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<QObject> authorization(_authorization);
    const QVariantMap bundle = sourceBundle(), context = _context, identity = _identity, role = _role, approver = _approver;
    const QString hash = _bindingHash;
    const auto same = [&]() { return self && nativeCurrent(generation, state) && _session == session && _authorization == authorization
        && sourceBundle() == bundle && sameMap(_context, context) && sameMap(_identity, identity) && sameMap(_role, role)
        && _approver == approver && _bindingHash == hash; };
    const auto accounts = [&]() {
        if (!same()) return false;
        const bool primary = session->identitySnapshotCurrent(identity);
        if (!same() || !primary) return false;
        if (requireApprover) {
            if (!approverShape(approver, identity, hash)) return false;
            const bool secondary = session->approverSnapshotCurrent(approver);
            if (!same() || !secondary) return false;
        }
        return same();
    };
    if (!accounts()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(authorization.data(), "fleetObservationCloseSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!same() || !invoked || !yes(matched)) return false;
    QVariant allowed;
    const bool gate = QMetaObject::invokeMethod(authorization.data(), "fleetObservationCloseContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(context)));
    return same() && gate && yes(allowed) && accounts() && same();
}
void FleetMissionObservationClose::notify()
{
    if (_notificationPending) return;
    _notificationPending = true;
    QTimer::singleShot(0, this, [this]() { _notificationPending = false; emit changed(); });
}
void FleetMissionObservationClose::disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionObservationClose::invalidate(quint64 generation, const QString& reason, const QString& state)
{
    if (generation == 0 || generation != _generation || !active()) return;
    _state = state; _reason = reason; _timer.stop(); disconnectSources();
    _consumptionNonce.clear(); _consuming = false; _approver.clear(); notify();
}
void FleetMissionObservationClose::connectSources(quint64 generation)
{
    const auto lost = [this, generation]() { invalidate(generation, QStringLiteral("actual-close-review-owner-destroyed")); };
    for (QObject* source : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_series.data()),
        static_cast<QObject*>(_transaction.data()), _authorization.data(), _runOwner.data(), _swarmOwner.data(), _orchestratorOwner.data()})
        _connections.append(connect(source, &QObject::destroyed, this, lost));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        // Our own credential window emits Session.changed on entry and exit.
        // Actual account/WTS/identity is checked again after that entire call.
        if (!_prompt) invalidate(generation, QStringLiteral("actual-close-review-session-changed"));
    }));
    const auto roleChanged = [this, generation]() { invalidate(generation, QStringLiteral("actual-close-review-role-changed")); };
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, roleChanged));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, roleChanged));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, roleChanged));
    const auto nativeChanged = [this, generation]() {
        if (_generation != generation || !active()) return;
        const QString state = _state;
        if (!nativeCurrent(generation, state)) invalidate(generation, QStringLiteral("actual-close-review-context-changed"));
    };
    _connections.append(connect(_series.data(), &FleetMissionStartupSeries::changed, this, nativeChanged));
    _connections.append(connect(_transaction.data(), &FleetMissionStartupTransaction::changed, this, nativeChanged));
}
QVariantMap FleetMissionObservationClose::current() const
{
    if (QThread::currentThread() != thread()) return outcome(QStringLiteral("owner-thread-required"), false);
    QVariantMap result = outcome(_state, _monitoringRetiredKnown && _monitoringRetired);
    result.insert(QStringLiteral("ok"), (_state == QStringLiteral("awaiting-review") && memoryCurrent(_generation, _state))
        || (_monitoringRetiredKnown && _monitoringRetired));
    result.insert(QStringLiteral("reviewId"), _reviewId);
    result.insert(QStringLiteral("workflowEpoch"), _workflowEpoch);
    result.insert(QStringLiteral("generation"), QString::number(_generation));
    result.insert(QStringLiteral("reason"), _reason);
    result.insert(QStringLiteral("remainingMs"), double(active() ? qMax<qint64>(0, _deadlineMs - _clock.elapsed()) : 0));
    result.insert(QStringLiteral("reviewActive"), active() && _clock.elapsed() < _deadlineMs);
    result.insert(QStringLiteral("busy"), _busy);
    result.insert(QStringLiteral("context"), _context);
    result.insert(QStringLiteral("roleLabel"), _role.value(QStringLiteral("role")));
    result.insert(QStringLiteral("monitorStopConfirmed"), _monitorStopConfirmed);
    result.insert(QStringLiteral("localControlReleaseConfirmed"), _localControlReleaseConfirmed);
    result.insert(QStringLiteral("aircraftMayContinueConfirmed"), _aircraftMayContinueConfirmed);
    result.insert(QStringLiteral("monitoringRetiredKnown"), _monitoringRetiredKnown);
    result.insert(QStringLiteral("monitoringRetired"), _monitoringRetiredKnown ? QVariant(_monitoringRetired) : QVariant());
    const bool releaseKnown = dispositionShape(_disposition);
    result.insert(QStringLiteral("localReservationsReleaseKnown"), releaseKnown);
    result.insert(QStringLiteral("localReservationsReleased"), releaseKnown ? _disposition.value(QStringLiteral("localReservationsReleasedVerified")) : QVariant());
    result.insert(QStringLiteral("runReservationCurrentlyHeld"), releaseKnown ? _disposition.value(QStringLiteral("runReservationCurrentlyHeld")) : QVariant());
    result.insert(QStringLiteral("startupReservationCurrentlyHeld"), releaseKnown ? _disposition.value(QStringLiteral("startupReservationCurrentlyHeld")) : QVariant());
    result.insert(QStringLiteral("riskWarning"), QStringLiteral("仅结束本地监视并释放本次所有者的本地占用；无人机可能继续飞行。不会停止、降落、上锁或宣布任务完成。"));
    return result;
}
QVariantMap FleetMissionObservationClose::requestReview(const QVariantMap& suppliedSources, const QString& suppliedStartupId,
    const QString& suppliedRunId, bool operatorConfirmed)
{
    const QVariantMap sources = suppliedSources;
    const QString startupId = suppliedStartupId, runId = suppliedRunId;
    if (QThread::currentThread() != thread() || _busy || _checking || _prompt || _consuming || active() || _state == QStringLiteral("committing")
        || !operatorConfirmed || !uuid(startupId) || !uuid(runId) || _generation == std::numeric_limits<quint64>::max()
        || !exactKeys(sources, {QStringLiteral("session"), QStringLiteral("roles"), QStringLiteral("startupSeries"), QStringLiteral("authorizationOwner")}))
        return outcome(QStringLiteral("confirmed-idle-exact-close-review-required"), false);
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(object(sources.value(QStringLiteral("session")))));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(object(sources.value(QStringLiteral("roles")))));
    const QPointer<FleetMissionStartupSeries> series(qobject_cast<FleetMissionStartupSeries*>(object(sources.value(QStringLiteral("startupSeries")))));
    const QPointer<QObject> authorization(object(sources.value(QStringLiteral("authorizationOwner"))));
    if (!session || !roles || !series || !authorization || session->thread() != thread() || roles->thread() != thread()
        || series->thread() != thread() || authorization->thread() != thread()) return outcome(QStringLiteral("actual-native-close-review-sources-required"), false);
    _busy = true; FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionObservationClose> self(this);
    disconnectSources(); ++_generation;
    const quint64 generation = _generation;
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupId = startupId; _runId = runId; _state = QStringLiteral("preparing"); _reason.clear();
    _deadlineMs = _clock.elapsed() + MaximumMs; // Includes every check, human delay and credential modal.
    _session = session; _roles = roles; _series = series; _authorization = authorization;
    _context.clear(); _identity.clear(); _role.clear(); _approver.clear(); _disposition.clear(); _bindingHash.clear(); _consumptionNonce.clear();
    _runOwner.clear(); _swarmOwner.clear(); _orchestratorOwner.clear(); _nativeOwnersFrozen = false;
    _authenticationConsumed = false; _consumingGeneration = 0;
    _monitorStopConfirmed = false; _localControlReleaseConfirmed = false; _aircraftMayContinueConfirmed = false;
    _monitoringRetiredKnown = false; _monitoringRetired = false;
    _transaction = qobject_cast<FleetMissionStartupTransaction*>(series->previousStartup());
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    if (!memoryCurrent(generation, QStringLiteral("preparing"))) { invalidate(generation, QStringLiteral("unique-actual-current-transaction-required")); return current(); }
    const QVariantMap context = _transaction->observationCloseContext(startupId);
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    if (!memoryCurrent(generation, QStringLiteral("preparing")) || !contextShape(context)
        || context.value(QStringLiteral("startupId")).toString() != startupId || context.value(QStringLiteral("runId")).toString() != runId) {
        invalidate(generation, QStringLiteral("actual-quiet-native-close-context-required")); return current();
    }
    _context = context;
    const QVariantMap owners = _transaction->observationCloseSourceObjects(startupId, context);
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    if (!memoryCurrent(generation, QStringLiteral("preparing"))
        || !exactKeys(owners, {QStringLiteral("run"), QStringLiteral("swarm"), QStringLiteral("orchestrator")})) {
        invalidate(generation, QStringLiteral("actual-close-native-owner-objects-required")); return current();
    }
    _runOwner = object(owners.value(QStringLiteral("run"))); _swarmOwner = object(owners.value(QStringLiteral("swarm")));
    _orchestratorOwner = object(owners.value(QStringLiteral("orchestrator"))); _nativeOwnersFrozen = true;
    if (!memoryCurrent(generation, QStringLiteral("preparing"))) {
        invalidate(generation, QStringLiteral("same-thread-close-native-owner-objects-required")); return current();
    }
    const QVariantMap identity = session->identitySnapshot();
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    if (!memoryCurrent(generation, QStringLiteral("preparing")) || !identityShape(identity)) { invalidate(generation, QStringLiteral("actual-current-operator-identity-required")); return current(); }
    _identity = identity; _role = roles->actionSnapshot(Action); _bindingHash = bindingHash();
    if (!sourcesCurrent(generation, QStringLiteral("preparing"), false)) {
        if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
        invalidate(generation, QStringLiteral("native-close-review-sources-or-authorization-denied")); return current();
    }
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    _state = QStringLiteral("awaiting-review"); connectSources(generation); _timer.start(); notify();
    return current(); // No approval prompt, consumption, local release or retirement here.
}
bool FleetMissionObservationClose::authenticationBindingCurrent(FleetOperatorSession* session, const QVariantMap& primary,
    const QVariantMap& approver, const QString& hash) const
{
    // Do not call nativeCurrent/sourcesCurrent/Session here: Session calls this
    // again AFTER burning its approver storage. Only local frozen memory.
    return session && memoryCurrent(_consumingGeneration, QStringLiteral("consuming")) && _busy && !_checking && !_prompt && _consuming
        && !_authenticationConsumed && session == _session.data() && session->thread() == thread() && uuid(_consumptionNonce)
        && _consumingGeneration == _generation && _monitorStopConfirmed && _localControlReleaseConfirmed && _aircraftMayContinueConfirmed
        && sameMap(primary, _identity) && sameMap(approver, _approver) && approverShape(approver, primary, hash)
        && hash == _bindingHash && digest(hash) && hash == bindingHash();
}
QVariantMap FleetMissionObservationClose::approveAndClose(const QString& suppliedReviewId, bool monitorStopConfirmed,
    bool localControlReleaseConfirmed, bool aircraftMayContinueConfirmed)
{
    const QString reviewId = suppliedReviewId;
    if (QThread::currentThread() != thread() || _busy || _checking || _prompt || _consuming || _authenticationConsumed
        || !uuid(reviewId) || reviewId != _reviewId || _state != QStringLiteral("awaiting-review")
        || !monitorStopConfirmed || !localControlReleaseConfirmed || !aircraftMayContinueConfirmed)
        return outcome(QStringLiteral("three-confirmations-and-current-review-required"), false);
    _busy = true; FlagGuard approving{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionObservationClose> self(this);
    const quint64 generation = _generation;
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetMissionStartupTransaction> transaction(_transaction);
    const QString startupId = _startupId;
    const QVariantMap context = _context, identity = _identity;
    const QString hash = _bindingHash;
    _monitorStopConfirmed = true; _localControlReleaseConfirmed = true; _aircraftMayContinueConfirmed = true;
    _state = QStringLiteral("authenticating");
    if (!sourcesCurrent(generation, QStringLiteral("authenticating"), false)) {
        if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
        invalidate(generation, QStringLiteral("close-review-stale-before-approval")); return current();
    }
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    _prompt = true;
    const QVariantMap authenticated = session->authenticateApprover(ApprovalAction, hash, identity,
        QStringLiteral("End only this Run's LOCAL monitoring and release only its owned LOCAL reservations. Aircraft MAY CONTINUE FLYING. NO STOP, LAND, DISARM or Completed."));
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    _prompt = false;
    const QVariantMap approver = authenticated.value(QStringLiteral("identityReceipt")).toMap();
    if (!nativeCurrent(generation, QStringLiteral("authenticating")) || !yes(authenticated.value(QStringLiteral("ok")))
        || !no(authenticated.value(QStringLiteral("flightCommandReleased"))) || authenticated.value(QStringLiteral("identityReceipt")).type() != QVariant::Map
        || !approverShape(approver, identity, hash)) {
        invalidate(generation, QStringLiteral("independent-close-approval-failed-or-original-deadline-expired"), QStringLiteral("failed")); return current();
    }
    _approver = approver;
    if (!sourcesCurrent(generation, QStringLiteral("authenticating"), true)) {
        if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
        invalidate(generation, QStringLiteral("close-review-source-changed-after-approval")); return current();
    }
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed"), false);
    _state = QStringLiteral("consuming"); _consuming = true; _consumingGeneration = generation;
    _consumptionNonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool consumed = session->consumeObservationCloseAuthentication(identity, approver, self.data(), hash);
    if (!self) return outcome(QStringLiteral("close-review-owner-destroyed-before-commit"), false);
    // No callbacks or OS checks after this one-use native consume. Only the
    // actual native owner/context/role fence, then one irreversible commit.
    if (!consumed || !nativeCurrent(generation, QStringLiteral("consuming"))) {
        _authenticationConsumed = consumed;
        invalidate(generation, QStringLiteral("close-authentication-consume-or-last-native-fence-failed"), QStringLiteral("failed")); return current();
    }
    _authenticationConsumed = true; _consuming = false; _consumptionNonce.clear();
    _state = QStringLiteral("committing"); _timer.stop(); disconnectSources();
    const bool retired = transaction->commitObservationClose(startupId, context);
    // commit may synchronously publish/drain and destroy this service/Txn.
    // The local returned fact must survive; never read a destroyed this or
    // pretend an actual retirement was rolled back by a later UI lifetime.
    if (!self) return outcome(retired ? QStringLiteral("closed-release-unknown-owner-destroyed") : QStringLiteral("close-unverified-owner-destroyed"), retired);
    _monitoringRetiredKnown = retired; _monitoringRetired = retired;
    QVariantMap disposition;
    if (transaction && transaction->thread() == thread()) disposition = transaction->observationCloseDisposition(startupId, context);
    if (!self) return outcome(retired ? QStringLiteral("closed-release-unknown-owner-destroyed") : QStringLiteral("close-unverified-owner-destroyed"), retired);
    if (dispositionShape(disposition)) {
        _disposition = disposition;
        _monitoringRetiredKnown = true;
        _monitoringRetired = retired || disposition.value(QStringLiteral("monitoringRetired")).toBool();
    }
    const bool releaseKnown = dispositionShape(_disposition);
    _state = _monitoringRetired ? (releaseKnown
        ? (yes(_disposition.value(QStringLiteral("localReservationsReleasedVerified"))) ? QStringLiteral("closed")
            : QStringLiteral("closed-local-reservations-held"))
        : QStringLiteral("closed-release-unknown")) : QStringLiteral("close-unverified");
    if (_monitoringRetired && releaseKnown && !yes(_disposition.value(QStringLiteral("localReservationsReleasedVerified"))))
        _reason = QStringLiteral("actual-local-reservation-still-present-not-restored-authority");
    else if (!_monitoringRetired) _reason = QStringLiteral("native-close-effect-not-verified-credentials-not-restored");
    notify(); return current();
}
bool FleetMissionObservationClose::cancelReview(const QString& suppliedReviewId)
{
    const QString reviewId = suppliedReviewId;
    if (QThread::currentThread() != thread() || !uuid(reviewId) || reviewId != _reviewId || !active()) return false;
    invalidate(_generation, QStringLiteral("explicitly-cancelled-this-close-review-only"), QStringLiteral("cancelled"));
    return true;
}

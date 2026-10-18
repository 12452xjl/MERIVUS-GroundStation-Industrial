#include "FleetMissionStartContentCheck.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionPlanStore.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "QGCMAVLink.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
bool outboundSources(const QObject* owner, const FleetOperatorSession* session, const FleetMissionDeployment* deployment)
{
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, session)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, deployment);
}
constexpr qint64 BatchLifetimeMs = 120000;
constexpr qint64 ReviewLifetimeMs = 30000;
constexpr int PollMs = 100;
constexpr double ExactMaximum = 9007199254740991.0;
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
struct FlagGuard {
    QPointer<QObject> object;
    bool* flag;
    ~FlagGuard() { if (object) *flag = false; }
};
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
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}
bool integer(const QVariant& value, double minimum, double maximum, double* number)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum || std::floor(candidate) != candidate) return false;
    *number = candidate;
    return true;
}
bool finiteFloat(const QVariant& value, double* number)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || std::abs(candidate) > std::numeric_limits<float>::max()
        || static_cast<double>(static_cast<float>(candidate)) != candidate) return false;
    *number = candidate;
    return true;
}
bool decimal(const QVariant& value, quint64* number)
{
    if (!text(value, 20)) return false;
    bool valid = false;
    const quint64 candidate = value.toString().toULongLong(&valid);
    if (!valid || QString::number(candidate) != value.toString()) return false;
    *number = candidate;
    return true;
}
bool positiveDecimal(const QVariant& value)
{
    quint64 number = 0;
    return decimal(value, &number) && number > 0;
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 20 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += static_cast<qint64>(it.key().size()) * 2;
            if (*amount > MaximumBytes || !boundedTree(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!boundedTree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList()) if (!boundedTree(entry, depth + 1, nodes, amount)) return false;
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
    int nodes = 0;
    qint64 amount = 0;
    if (map.isEmpty() || !boundedTree(map, 0, &nodes, &amount)) return false;
    *result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !result->isEmpty() && result->size() <= MaximumBytes;
}
QString hash(const QVariantMap& value)
{
    QByteArray encoded;
    if (!encode(value, &encoded)) return {};
    return QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex());
}
bool bindingShape(const QVariantMap& binding)
{
    const QSet<QString> strings = {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if ((binding.size() != 7 && binding.size() != 9)
        || binding.contains(QStringLiteral("reviewGeneration")) != binding.contains(QStringLiteral("inputGeneration"))) return false;
    for (auto it = binding.constBegin(); it != binding.constEnd(); ++it) {
        if (strings.contains(it.key())) { if (!text(it.value())) return false; }
        else if (it.key() == QStringLiteral("recordRevision") || it.key() == QStringLiteral("reviewGeneration")
            || it.key() == QStringLiteral("inputGeneration")) {
            double number = 0;
            if (!integer(it.value(), it.key() == QStringLiteral("recordRevision") ? 1 : 0, ExactMaximum, &number)) return false;
        } else return false;
    }
    for (const QString& key : strings) if (!binding.contains(key)) return false;
    return binding.contains(QStringLiteral("recordRevision")) && digest(binding.value(QStringLiteral("planId")))
        && digest(binding.value(QStringLiteral("contentHash"))) && digest(binding.value(QStringLiteral("reviewEvidenceHash")));
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result)
{
    const QString task = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(task);
    const QVariantMap record = store->record(task, binding.value(QStringLiteral("recordRevision")));
    if (!isFalse(latest.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return encode(record, result);
}
bool validRole(const QVariantMap& identity, const QVariantMap& role)
{
    return role.size() == 9 && text(role.value(QStringLiteral("actorId"))) && text(role.value(QStringLiteral("role")))
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start.content.read")
        && isTrue(role.value(QStringLiteral("allowed"))) && isTrue(identity.value(QStringLiteral("authenticated")))
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")));
}
QVariantMap stableReceipt(QVariantMap value) { value.remove(QStringLiteral("ageMs")); return value; }
QVariantMap stableContext(QVariantMap value) { value.remove(QStringLiteral("remainingLifetimeMs")); return value; }
QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

// Decode the retained ORIGINAL MISSION_ITEM_INT evidence, never MissionItem
// display/cache values. The sole frame translation is AMSL GLOBAL_INT ->
// canonical AMSL GLOBAL. Current is retained but not immutable plan identity.
QVariantMap canonicalRaw(const QVariantMap& receipt, int expectedCount)
{
    if (receipt.value(QStringLiteral("rawItems")).type() != QVariant::List) return {};
    const QVariantList items = receipt.value(QStringLiteral("rawItems")).toList();
    if (items.size() != expectedCount || expectedCount < 1 || expectedCount > 2048) return {};
    QVariantList normalized;
    for (int index = 0; index < items.size(); ++index) {
        if (items.at(index).type() != QVariant::Map) return {};
        const QVariantMap raw = items.at(index).toMap();
        double seq = 0, command = 0, frame = 0, x = 0, y = 0, current = 0, automatic = 0, missionType = 0, system = 0, component = 0;
        if (raw.size() != 16 || !integer(raw.value(QStringLiteral("sequenceNumber")), index, index, &seq)
            || !integer(raw.value(QStringLiteral("command")), 0, 65535, &command)
            || !integer(raw.value(QStringLiteral("frame")), 0, 255, &frame)
            || !integer(raw.value(QStringLiteral("x")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &x)
            || !integer(raw.value(QStringLiteral("y")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &y)
            || !integer(raw.value(QStringLiteral("current")), 0, 1, &current)
            || !integer(raw.value(QStringLiteral("autocontinue")), 0, 1, &automatic)
            || !integer(raw.value(QStringLiteral("missionType")), MAV_MISSION_TYPE_MISSION, MAV_MISSION_TYPE_MISSION, &missionType)
            || !integer(raw.value(QStringLiteral("targetSystem")), 0, 255, &system)
            || !integer(raw.value(QStringLiteral("targetComponent")), 0, 255, &component)
            || !digest(raw.value(QStringLiteral("rawPayloadHash")))) return {};
        const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
        const bool geographic = command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_TAKEOFF
            || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
        if ((!speed && !geographic) || frame != (speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL_INT)) return {};
        double p1 = 0, p2 = 0, p3 = 0, z = 0, p4 = 0;
        if (!finiteFloat(raw.value(QStringLiteral("param1")), &p1) || !finiteFloat(raw.value(QStringLiteral("param2")), &p2)
            || !finiteFloat(raw.value(QStringLiteral("param3")), &p3) || !finiteFloat(raw.value(QStringLiteral("z")), &z)
            || !raw.contains(QStringLiteral("param4"))
            || (raw.value(QStringLiteral("param4")).isValid() && !finiteFloat(raw.value(QStringLiteral("param4")), &p4))
            || (speed && (x != 0 || y != 0))) return {};
        QVariant parameter4;
        QString yawMode, headingError;
        if (!FleetMissionHeadingEncoding::decodeParam4(static_cast<int>(command),
                raw.value(QStringLiteral("param4")).isValid() ? float(p4) : std::numeric_limits<float>::quiet_NaN(),
                parameter4, yawMode, headingError)) return {};
        normalized.append(QVariantMap{{QStringLiteral("sequenceNumber"), index}, {QStringLiteral("command"), static_cast<int>(command)},
            {QStringLiteral("frame"), static_cast<int>(speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL)},
            {QStringLiteral("param1"), p1}, {QStringLiteral("param2"), p2}, {QStringLiteral("param3"), p3},
            {QStringLiteral("param4"), parameter4}, {QStringLiteral("param5"), speed ? 0.0 : x / 1e7},
            {QStringLiteral("param6"), speed ? 0.0 : y / 1e7}, {QStringLiteral("param7"), z},
            {QStringLiteral("autoContinue"), automatic == 1}, {QStringLiteral("isCurrentItem"), current == 1},
            {QStringLiteral("yawMode"), yawMode}});
    }
    return FleetMissionPayloadBuilder::canonicalWireItems(normalized, true);
}
} // namespace

FleetMissionStartContentCheck::FleetMissionStartContentCheck(QObject* parent)
    : QObject(parent), _checkEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _timer.setInterval(PollMs);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionStartContentCheck::_tick);
}
FleetMissionStartContentCheck::~FleetMissionStartContentCheck()
{
    _active = false;
    _disconnectSources();
    _detachDrain();
}
QVariantMap FleetMissionStartContentCheck::current() const
{
    QVariantList vehicles;
    int matched = 0;
    for (const Target& target : _targets) {
        if (target.state == QStringLiteral("content-matched")) ++matched;
        vehicles.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("state"), target.state},
            {QStringLiteral("reason"), target.reason}, {QStringLiteral("itemCount"), target.payload.value(QStringLiteral("itemCount"))},
            {QStringLiteral("readScopeId"), target.readScopeId}, {QStringLiteral("operationId"), QString::number(target.operationId)},
            {QStringLiteral("receiptSequence"), target.receipt.value(QStringLiteral("receiptSequence"))},
            {QStringLiteral("contentHash"), target.canonical.value(QStringLiteral("contentHash"))}});
    }
    const qint64 now = _clock.elapsed();
    return {{QStringLiteral("ok"), _state == QStringLiteral("content-matched") && now < _verifiedUntilMs
            && !_active && !_reservationHeld && !_transportDraining},
        {QStringLiteral("requestId"), _checkId}, {QStringLiteral("checkId"), _checkId}, {QStringLiteral("checkEpoch"), _checkEpoch},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("deploymentId"), _deploymentId},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("active"), _active}, {QStringLiteral("reservationHeld"), _reservationHeld},
        {QStringLiteral("transportDraining"), _transportDraining}, {QStringLiteral("vehicleIds"), _vehicleIds},
        {QStringLiteral("vehicles"), vehicles}, {QStringLiteral("matchedVehicleCount"), matched},
        {QStringLiteral("totalVehicleCount"), _targets.size()}, {QStringLiteral("sourceFingerprint"), _sourceFingerprint},
        {QStringLiteral("remainingMs"), static_cast<double>(qMax<qint64>(0, _active ? _createdAtMs + BatchLifetimeMs - now : _verifiedUntilMs - now))},
        {QStringLiteral("context"), QVariantMap{{QStringLiteral("deploymentGeneration"), _deploymentContext.value(QStringLiteral("generation"))},
            {QStringLiteral("historicalStagingOnly"), true}, {QStringLiteral("revalidationRequired"), true}}},
        {QStringLiteral("readOnlyRequestsMayRemainInflight"), _issuedAnyRequest}, {QStringLiteral("remoteAtomicSnapshotVerified"), false},
        {QStringLiteral("requestAttributionVerified"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
bool FleetMissionStartContentCheck::_live(quint64 generation) const
{
    return generation != 0 && generation == _generation && _createdAtMs >= 0
        && ((_active && _clock.elapsed() < _createdAtMs + BatchLifetimeMs)
            || (!_active && _state == QStringLiteral("content-matched") && _clock.elapsed() < _verifiedUntilMs));
}
bool FleetMissionStartContentCheck::_targetCurrent(const Target& supplied, quint64 generation, bool allowOwnRead) const
{
    const Target target = supplied;
    const QPointer<FleetMissionStartContentCheck> self(const_cast<FleetMissionStartContentCheck*>(this));
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QString deploymentId = _deploymentId;
    const auto alive = [&]() { return self && outboundSources(this, _session.data(), deployment.data()) && _live(generation) && deployment && _deployment == deployment
        && _deploymentId == deploymentId && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
    if (!alive()) return false;
    const QPointer<Vehicle> vehicle(deployment->startReviewVehicle(deploymentId, target.vehicleId));
    if (!alive()) return false;
    const QPointer<MissionManager> mission(deployment->startReviewManager(deploymentId, target.vehicleId));
    if (!alive()) return false;
    const SharedLinkInterfacePtr primary = target.links->primaryLink().lock();
    if (!alive() || vehicle != target.vehicle || mission != target.mission || !primary || primary.get() != target.primary.data()) return false;
    if (!target.primary->isConnected() || !alive() || target.primary->isMockLink() || !alive()
        || target.primary->isLogReplay() || !alive()) return false;
    if (target.vehicle->thread() != thread() || target.mission->thread() != thread() || target.parameters->thread() != thread()
        || target.links->thread() != thread() || target.vehicle->id() != target.vehicleId
        || target.vehicle->defaultComponentId() != target.componentId || target.componentId < 1 || target.componentId > 255
        || target.vehicle->missionManager() != target.mission.data() || target.vehicle->parameterManager() != target.parameters.data()
        || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->vehicleLinkManager() != target.links.data()
        || target.vehicle->isOfflineEditingVehicle() || !target.vehicle->isInitialConnectComplete()
        || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || target.vehicle->armed() || target.vehicle->flying()
        || target.links->communicationLost() || !target.parameters->parametersReady()
        || target.parameters->missingParameters() || target.parameters->pendingWrites()) return false;
    const QVariantMap version = target.vehicle->autopilotVersionObservation(BatchLifetimeMs);
    if (!alive() || version.value(QStringLiteral("sourceEpoch")) != target.versionEpoch) return false;
    const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MPC_XY_CRUISE"), BatchLifetimeMs);
    if (!alive() || parameter.value(QStringLiteral("managerEpoch")) != target.parameterEpoch) return false;
    if (target.mission->inProgress() || target.mission->transportCallInProgress()) {
        const bool own = allowOwnRead && !target.readScopeId.isEmpty()
            && target.mission->ownsFleetMissionRead(target.readScopeId, target.primary.data(), target.operationId);
        if (!alive() || !own) return false;
    }
    return alive();
}
bool FleetMissionStartContentCheck::_deploymentCurrent(quint64 generation, bool requireIdle) const
{
    const QPointer<FleetMissionStartContentCheck> self(const_cast<FleetMissionStartContentCheck*>(this));
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QVariantMap frozen = _deploymentContext;
    const QByteArray frozenBytes = _deploymentBytes;
    if (!self || !outboundSources(this, _session.data(), deployment.data()) || !_live(generation) || !deployment) return false;
    const QVariantMap current = deployment->current();
    if (!self || !outboundSources(this, _session.data(), deployment.data()) || !_live(generation) || !deployment || _deployment != deployment || _deploymentContext != frozen || _deploymentBytes != frozenBytes
        || current.value(QStringLiteral("deploymentId")) != frozen.value(QStringLiteral("deploymentId"))
        || current.value(QStringLiteral("binding")) != frozen.value(QStringLiteral("binding"))
        || current.value(QStringLiteral("payload")) != frozen.value(QStringLiteral("payloadBuild"))
        || !isFalse(current.value(QStringLiteral("transportDraining"))) || !isFalse(current.value(QStringLiteral("reservationHeld")))) return false;
    if (!requireIdle) return true;
    const QVariantMap actual = deployment->startReviewContext(_deploymentId);
    QByteArray encoded;
    return self && outboundSources(this, _session.data(), deployment.data()) && _live(generation) && deployment && _deployment == deployment && _deploymentContext == frozen
        && encode(actual, &encoded) && encoded == frozenBytes;
}
bool FleetMissionStartContentCheck::_receiptCurrent(const Target& supplied, quint64 generation, QVariantMap* output) const
{
    const Target target = supplied;
    const QPointer<FleetMissionStartContentCheck> self(const_cast<FleetMissionStartContentCheck*>(this));
    const auto alive = [&]() { return self && outboundSources(this, _session.data(), _deployment.data())
        && _live(generation) && target.mission && target.primary && target.vehicle; };
    if (!alive() || !target.operationId || target.readScopeId.isEmpty() || !_targetCurrent(target, generation, false)) return false;
    const bool before = target.mission->fleetMissionReadReceiptSourceMatches(target.primary.data(), QString::number(target.operationId), target.readScopeId);
    if (!alive()) return false;
    const QVariantMap receipt = target.mission->fleetMissionReadReceipt(_active ? BatchLifetimeMs : ReviewLifetimeMs);
    if (!alive()) return false;
    const bool after = target.mission->fleetMissionReadReceiptSourceMatches(target.primary.data(), QString::number(target.operationId), target.readScopeId);
    if (!alive() || !before || !after) return false;
    double schema = 0, age = 0, vehicle = 0, component = 0, count = 0, started = 0, countAt = 0, firstAt = 0, lastAt = 0, complete = 0, duration = 0;
    quint64 sequence = 0, baseline = 0;
    const int expectedCount = target.payload.value(QStringLiteral("items")).toList().size();
    if (receipt.size() != 34 || !integer(receipt.value(QStringLiteral("schemaVersion")), 1, 1, &schema) || !isTrue(receipt.value(QStringLiteral("observed")))
        || !isTrue(receipt.value(QStringLiteral("available"))) || !isTrue(receipt.value(QStringLiteral("fresh")))
        || !isTrue(receipt.value(QStringLiteral("requestIssued"))) || !isTrue(receipt.value(QStringLiteral("countObserved")))
        || !isTrue(receipt.value(QStringLiteral("allItemsInt"))) || !isTrue(receipt.value(QStringLiteral("readEndAckQueued")))
        || !integer(receipt.value(QStringLiteral("ageMs")), 0, _active ? BatchLifetimeMs : ReviewLifetimeMs, &age)
        || !integer(receipt.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId, &vehicle)
        || !integer(receipt.value(QStringLiteral("componentId")), target.componentId, target.componentId, &component)
        || !integer(receipt.value(QStringLiteral("itemCount")), expectedCount, expectedCount, &count)
        || receipt.value(QStringLiteral("operationId")) != QString::number(target.operationId)
        || receipt.value(QStringLiteral("readScopeId")) != target.readScopeId
        || receipt.value(QStringLiteral("sourceEpoch")) != target.managerEpoch
        || receipt.value(QStringLiteral("vehicleSourceEpoch")) != target.versionEpoch
        || !decimal(receipt.value(QStringLiteral("receiptSequence")), &sequence)
        || !decimal(target.baselineSequence, &baseline) || sequence <= baseline
        || !digest(receipt.value(QStringLiteral("cacheFingerprint"))) || !digest(receipt.value(QStringLiteral("countPayloadHash")))
        || !isFalse(receipt.value(QStringLiteral("requestCausalityVerified"))) || !isFalse(receipt.value(QStringLiteral("authenticated")))
        || !isFalse(receipt.value(QStringLiteral("atomicSnapshotVerified"))) || !isFalse(receipt.value(QStringLiteral("currentOnboardContentsProven")))
        || !isFalse(receipt.value(QStringLiteral("signatureVerified"))) || !isFalse(receipt.value(QStringLiteral("startupPermitGranted")))
        || !isFalse(receipt.value(QStringLiteral("executionVerified"))) || !isFalse(receipt.value(QStringLiteral("flightCommandReleased")))
        || !integer(receipt.value(QStringLiteral("readStartedAtMonotonicMs")), 0, ExactMaximum, &started)
        || !integer(receipt.value(QStringLiteral("countReceivedAtMonotonicMs")), started, ExactMaximum, &countAt)
        || !integer(receipt.value(QStringLiteral("firstItemReceivedAtMonotonicMs")), countAt, ExactMaximum, &firstAt)
        || !integer(receipt.value(QStringLiteral("lastItemReceivedAtMonotonicMs")), firstAt, ExactMaximum, &lastAt)
        || !integer(receipt.value(QStringLiteral("completedAtMonotonicMs")), lastAt, ExactMaximum, &complete)
        || !integer(receipt.value(QStringLiteral("durationMs")), 0, BatchLifetimeMs, &duration) || duration != complete - started) return false;
    const QVariantMap canonical = canonicalRaw(receipt, expectedCount);
    if (!isTrue(canonical.value(QStringLiteral("ok"))) || canonical.value(QStringLiteral("wireSchema")) != target.payload.value(QStringLiteral("wireSchema"))
        || canonical.value(QStringLiteral("contentHash")) != target.payload.value(QStringLiteral("contentHash"))
        || canonical.value(QStringLiteral("items")) != target.payload.value(QStringLiteral("canonicalWireItems"))) return false;
    if (!target.receipt.isEmpty() && stableReceipt(receipt) != stableReceipt(target.receipt)) return false;
    *output = receipt;
    return alive();
}
bool FleetMissionStartContentCheck::_gate(quint64 generation, bool requireComplete)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionStartContentCheck> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<QObject> owner(_authorization);
    const QVariantMap identity = _identityReceipt, role = _roleReceipt, binding = _binding, context = _deploymentContext;
    const QVariantList ids = _vehicleIds;
    const QByteArray record = _recordBytes;
    const QString requestId = _checkId, deploymentId = _deploymentId;
    const bool active = _active, held = _reservationHeld;
    const QList<Target> targets = _targets;
    const auto unchanged = [&]() {
        return self && outboundSources(this, session.data(), deployment.data()) && !_reentered && _live(generation) && _active == active && _reservationHeld == held
            && session && roles && store && deployment && swarm && owner
            && _session == session && _roles == roles && _store == store && _deployment == deployment && _swarm == swarm && _authorization == owner
            && _identityReceipt == identity && _roleReceipt == role && _binding == binding && _deploymentContext == context
            && _vehicleIds == ids && _recordBytes == record && _checkId == requestId && _deploymentId == deploymentId
            && session->thread() == thread() && roles->thread() == thread() && store->thread() == thread()
            && deployment->thread() == thread() && swarm->thread() == thread() && owner->thread() == thread();
    };
    const auto nativeCurrent = [&]() {
        if (!unchanged() || !validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return false;
        // WTS/local-group validation can dispatch sent window messages.
        const bool identityCurrent = active ? session->identitySnapshotCurrent(identity)
            : session->identitySnapshot() == identity;
        if (!identityCurrent || !unchanged()) return false;
        if (held && !swarm->missionDeploymentOwns(requestId, ids)) return false;
        if (!active && held) return false;
        QByteArray actualRecord;
        if (!currentRecord(store.data(), binding, &actualRecord) || !unchanged() || actualRecord != record) return false;
        bool ownRead = false;
        for (const Target& target : targets) {
            if (!_targetCurrent(target, generation, active) || !unchanged()) return false;
            if (target.mission->inProgress() || target.mission->transportCallInProgress()) ownRead = true;
            if (requireComplete || !target.receipt.isEmpty()) {
                QVariantMap receipt;
                if (!_receiptCurrent(target, generation, &receipt) || !unchanged()) return false;
            }
        }
        return _deploymentCurrent(generation, !ownRead) && unchanged();
    };
    if (!nativeCurrent()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(owner.data(), "fleetStartContentSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(store.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(deployment.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))));
    if (!unchanged() || !invoked || !isTrue(matched) || !nativeCurrent()) return false;
    QVariant allowed;
    const bool allowedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartContentContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(requestId)), Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(ids)));
    return unchanged() && allowedInvoked && isTrue(allowed) && nativeCurrent();
}
void FleetMissionStartContentCheck::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionStartContentCheck::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _finish(generation, QStringLiteral("stale"), QStringLiteral("content-read-source-changed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()), _authorization.data()})
        _connections.append(connect(object, &QObject::destroyed, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        const QPointer<FleetMissionStartContentCheck> self(this);
        const QPointer<FleetOperatorSession> session(_session);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QVariantMap identity = _identityReceipt, role = _roleReceipt;
        const bool current = session && (_active ? session->identitySnapshotCurrent(identity) : session->identitySnapshot() == identity);
        if (!self) return;
        if (current && _live(generation) && session && roles && _session == session && _roles == roles
            && _identityReceipt == identity && _roleReceipt == role && roles->actionSnapshotCurrent(role)) return;
        _finish(generation, QStringLiteral("stale"), QStringLiteral("content-read-authenticated-source-changed"));
    }));
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index);
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(object, &QObject::destroyed, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, [stale](bool pending) { if (pending) stale(); }));
        _connections.append(connect(target.parameters.data(), &ParameterManager::parametersReadyChanged, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::missingParametersChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, stale));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this,
            [this, generation, index](quint64 operationId, int transaction) {
                if (!_live(generation) || index >= _targets.size()) return;
                const Target target = _targets.at(index);
                const QPointer<FleetMissionStartContentCheck> self(this);
                const bool own = _active && index == _targetIndex && transaction == PlanManager::TransactionRead
                    && target.mission && target.primary && !target.readScopeId.isEmpty()
                    && target.mission->ownsFleetMissionRead(target.readScopeId, target.primary.data(), operationId);
                if (!self || !_live(generation)) return;
                if (!own) { _finish(generation, QStringLiteral("stale"), QStringLiteral("foreign-mission-operation-invalidated-content")); return; }
                _targets[index].operationId = operationId;
            }));
        // Completion's boolean is REMOVE_ALL, not success. Poll the retained
        // native successful RAW receipt only after the finish stack unwinds.
    }
}
bool FleetMissionStartContentCheck::_publish(quint64 generation)
{
    const QPointer<FleetMissionStartContentCheck> self(this);
    emit changed();
    return self && _live(generation);
}
QVariantMap FleetMissionStartContentCheck::requestCheck(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* deploymentObject, QObject* swarmObject, QObject* authorizationOwner,
    const QString& suppliedDeploymentId, bool operatorConfirmed)
{
    const QString deploymentId = suppliedDeploymentId;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _active || _reservationHeld || _transportDraining || _busy || _checking
        || !operatorConfirmed || !uuid(deploymentId)) return rejection(QStringLiteral("confirmed-idle-native-content-check-required"));
    _busy = true;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartContentCheck> self(this);
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(sessionObject);
    QPointer<FleetRolePolicy> roles = qobject_cast<FleetRolePolicy*>(roleObject);
    QPointer<FleetMissionPlanStore> store = qobject_cast<FleetMissionPlanStore*>(storeObject);
    QPointer<FleetMissionDeployment> deployment = qobject_cast<FleetMissionDeployment*>(deploymentObject);
    QPointer<SwarmController> swarm = qobject_cast<SwarmController*>(swarmObject);
    QPointer<QObject> owner(authorizationOwner);
    const auto sourcesAlive = [&]() { return self && session && roles && store && deployment && swarm && owner
        && outboundSources(this, session.data(), deployment.data()); };
    if (!sourcesAlive() || session->thread() != thread() || roles->thread() != thread() || store->thread() != thread()
        || deployment->thread() != thread() || swarm->thread() != thread() || owner->thread() != thread())
        return rejection(QStringLiteral("actual-native-content-sources-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!sourcesAlive()) return rejection(QStringLiteral("identity-source-destroyed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start.content.read"));
    if (!validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return rejection(QStringLiteral("content-read-role-denied"));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!sourcesAlive() || !identityCurrent) return rejection(QStringLiteral("native-current-operator-required"));
    QVariant matched;
    const bool ownerInvoked = QMetaObject::invokeMethod(owner.data(), "fleetStartContentSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(store.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(deployment.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))));
    if (!sourcesAlive() || !ownerInvoked || !isTrue(matched)) return rejection(QStringLiteral("private-content-source-owner-required"));
    // Revoke the OLD verified/live staging before freezing our new context.
    // This does not write/clear the aircraft mission or restore old approval.
    const bool prepared = deployment->prepareStartContentCheck(deploymentId);
    if (!sourcesAlive() || !prepared) return rejection(QStringLiteral("historical-staging-not-ready-for-separate-read"));
    const QVariantMap context = deployment->startReviewContext(deploymentId);
    if (!sourcesAlive() || !isTrue(context.value(QStringLiteral("ok"))) || context.value(QStringLiteral("deploymentId")) != deploymentId
        || !positiveDecimal(context.value(QStringLiteral("generation"))) || context.value(QStringLiteral("binding")).type() != QVariant::Map
        || !bindingShape(context.value(QStringLiteral("binding")).toMap()) || context.value(QStringLiteral("payloadBuild")).type() != QVariant::Map
        || context.value(QStringLiteral("plan")).type() != QVariant::Map) return rejection(QStringLiteral("bounded-current-runtime-staging-required"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    QByteArray recordBytes, deploymentBytes;
    if (!currentRecord(store.data(), binding, &recordBytes) || !sourcesAlive() || !encode(context, &deploymentBytes))
        return rejection(QStringLiteral("latest-nonrestored-full-store-record-required"));
    const QVariantMap payloadBuild = context.value(QStringLiteral("payloadBuild")).toMap();
    if (!isTrue(payloadBuild.value(QStringLiteral("ok"))) || payloadBuild.value(QStringLiteral("payloads")).type() != QVariant::List)
        return rejection(QStringLiteral("actual-bounded-payload-required"));
    const QVariantList members = payloadBuild.value(QStringLiteral("payloads")).toList();
    if (members.isEmpty() || members.size() > 32) return rejection(QStringLiteral("target-count-outside-read-budget"));
    QList<Target> targets;
    QVariantList ids;
    QSet<int> seen;
    int itemBudget = 0;
    for (const QVariant& member : members) {
        if (member.type() != QVariant::Map) return rejection(QStringLiteral("actual-payload-member-required"));
        const QVariantMap payload = member.toMap();
        double id = 0, count = 0;
        if (!integer(payload.value(QStringLiteral("vehicleId")), 1, 255, &id) || seen.contains(static_cast<int>(id))
            || payload.value(QStringLiteral("items")).type() != QVariant::List
            || !integer(payload.value(QStringLiteral("itemCount")), 1, 2048, &count)
            || payload.value(QStringLiteral("items")).toList().size() != count
            || payload.value(QStringLiteral("canonicalWireItems")).type() != QVariant::List
            || payload.value(QStringLiteral("canonicalWireItems")).toList().size() != count
            || !FleetMissionHeadingEncoding::isSupportedWireSchema(payload.value(QStringLiteral("wireSchema")))
            || !digest(payload.value(QStringLiteral("contentHash")))) return rejection(QStringLiteral("bounded-exact-v2-payload-required"));
        itemBudget += static_cast<int>(count);
        if (itemBudget > 8192) return rejection(QStringLiteral("batch-item-budget-exceeded"));
        Target target;
        target.vehicleId = static_cast<int>(id); target.payload = payload;
        target.vehicle = deployment->startReviewVehicle(deploymentId, target.vehicleId);
        if (!sourcesAlive() || !target.vehicle) return rejection(QStringLiteral("actual-runtime-target-unavailable"));
        target.mission = deployment->startReviewManager(deploymentId, target.vehicleId);
        if (!sourcesAlive() || !target.vehicle || !target.mission) return rejection(QStringLiteral("actual-runtime-manager-unavailable"));
        target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager();
        target.componentId = target.vehicle->defaultComponentId();
        const SharedLinkInterfacePtr primary = target.links ? target.links->primaryLink().lock() : SharedLinkInterfacePtr();
        target.primary = primary.get();
        if (!target.parameters || !target.links || !target.primary) return rejection(QStringLiteral("actual-primary-parameter-source-required"));
        const QVariantMap version = target.vehicle->autopilotVersionObservation(BatchLifetimeMs);
        if (!sourcesAlive() || !target.vehicle || !target.mission || !target.parameters || !target.links || !target.primary)
            return rejection(QStringLiteral("target-source-destroyed-during-capture"));
        const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MPC_XY_CRUISE"), BatchLifetimeMs);
        if (!sourcesAlive() || !target.vehicle || !target.mission || !target.parameters || !target.links || !target.primary
            || !uuid(version.value(QStringLiteral("sourceEpoch"))) || !uuid(parameter.value(QStringLiteral("managerEpoch"))))
            return rejection(QStringLiteral("native-source-epochs-required"));
        target.versionEpoch = version.value(QStringLiteral("sourceEpoch")).toString();
        target.parameterEpoch = parameter.value(QStringLiteral("managerEpoch")).toString();
        seen.insert(target.vehicleId); ids.append(id); targets.append(target);
    }
    if (!sourcesAlive() || _generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("content-check-generation-exhausted"));
    _disconnectSources();
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _deployment = deployment; _swarm = swarm; _authorization = owner;
    _identityReceipt = identity; _roleReceipt = role; _deploymentContext = context; _binding = binding;
    _recordBytes = recordBytes; _deploymentBytes = deploymentBytes; _targets = targets; _vehicleIds = ids;
    _checkId = QUuid::createUuid().toString(QUuid::WithoutBraces); _deploymentId = deploymentId;
    _sourceFingerprint.clear(); _state = QStringLiteral("reading-content"); _reason.clear(); _active = true;
    _createdAtMs = _clock.elapsed(); _targetIndex = 0; _lastRequestAtMs = -1; _verifiedUntilMs = -1;
    _targetDeadlineMs = _createdAtMs + qMin<qint64>(BatchLifetimeMs, 4000 + targets.first().payload.value(QStringLiteral("itemCount")).toInt() * 250LL);
    _issuedAnyRequest = false;
    _connectSources(generation);
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("initial-content-context-denied"));
        return rejection(QStringLiteral("initial-content-context-denied"));
    }
    const QString requestId = _checkId;
    const bool reserved = swarm->reserveMissionDeployment(requestId, ids);
    if (!self) { if (swarm && reserved) swarm->releaseMissionDeployment(requestId); return rejection(QStringLiteral("content-reader-destroyed-at-reservation")); }
    if (!_live(generation) || !sourcesAlive() || _swarm != swarm || !reserved) {
        if (swarm && reserved) swarm->releaseMissionDeployment(requestId);
        _finish(generation, QStringLiteral("failed"), QStringLiteral("whole-group-read-reservation-denied"));
        return rejection(QStringLiteral("whole-group-read-reservation-denied"));
    }
    _reservationHeld = true;
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("reserved-content-context-denied"));
        return rejection(QStringLiteral("reserved-content-context-denied"));
    }
    _timer.start();
    if (!_publish(generation) || !self) return rejection(QStringLiteral("content-request-revoked-at-publication"));
    if (!outboundSources(this, session.data(), deployment.data())) {
        _finish(generation, QStringLiteral("stale"), QStringLiteral("native-episode-changed-at-content-publication"));
        return rejection(QStringLiteral("content-request-revoked-at-publication"));
    }
    QVariantMap result = current();
    result.insert(QStringLiteral("ok"), true); // Accepted read, NOT matched yet.
    result.insert(QStringLiteral("requestAccepted"), true);
    return result;
}
void FleetMissionStartContentCheck::_finish(quint64 generation, const QString& state, const QString& reason)
{
    if (_generation != generation || generation == 0 || (!_active && _state != QStringLiteral("content-matched"))) return;
    const QPointer<FleetMissionStartContentCheck> self(this);
    const QList<Target> targets = _targets;
    _active = false; _state = state; _reason = reason;
    if (state != QStringLiteral("content-matched")) {
        _verifiedUntilMs = -1;
        _disconnectSources();
        if (_targetIndex < _targets.size() && _targets[_targetIndex].state != QStringLiteral("content-matched")) {
            _targets[_targetIndex].state = state; _targets[_targetIndex].reason = reason;
        }
        // Flip the service generation's send gate BEFORE native cancellation.
        // Cancellation only stops future requests. Already queued readonly
        // packets/late replies cannot be retracted and imply no rollback.
        for (const Target& target : targets) {
            if (target.mission && !target.readScopeId.isEmpty()) target.mission->cancelFleetMissionRead(target.readScopeId);
            if (!self || _generation != generation) return;
        }
    }
    _transportDraining = _reservationHeld;
    _drain();
    if (!self || _generation != generation) return;
    if (_transportDraining || state == QStringLiteral("content-matched")) _timer.start();
    else _timer.stop();
    emit changed();
}
void FleetMissionStartContentCheck::_drain()
{
    if (!_reservationHeld || _active) return;
    const QPointer<FleetMissionStartContentCheck> self(this);
    const QPointer<SwarmController> swarm(_swarm);
    const quint64 generation = _generation;
    const QString requestId = _checkId;
    for (const Target& target : _targets)
        if (target.mission && (target.mission->inProgress() || target.mission->transportCallInProgress())) {
            _transportDraining = true;
            return;
        }
    if (!self || _generation != generation || _checkId != requestId) return;
    _reservationHeld = false; _transportDraining = false;
    if (swarm) swarm->releaseMissionDeployment(requestId);
}
void FleetMissionStartContentCheck::_detachDrain()
{
    const QPointer<SwarmController> swarm(_swarm);
    const QList<Target> targets = _targets;
    const QString requestId = _checkId;
    const bool held = _reservationHeld;
    for (const Target& target : targets)
        if (target.mission && !target.readScopeId.isEmpty()) target.mission->cancelFleetMissionRead(target.readScopeId);
    if (!held || !swarm) return;
    // The service can disappear from a plugin/finish callback. Keep a tiny
    // native cleanup owner until actual protocol stacks unwind; never release
    // the group's reservation while the old manager still uses that stack.
    QTimer* cleanup = new QTimer(swarm.data());
    cleanup->setInterval(PollMs);
    connect(cleanup, &QTimer::timeout, cleanup, [cleanup, swarm, targets, requestId]() {
        if (!swarm) { cleanup->deleteLater(); return; }
        for (const Target& target : targets)
            if (target.mission && (target.mission->inProgress() || target.mission->transportCallInProgress())) return;
        const QPointer<QTimer> guarded(cleanup);
        cleanup->stop();
        swarm->releaseMissionDeployment(requestId);
        if (guarded) cleanup->deleteLater();
    });
    cleanup->start();
}
void FleetMissionStartContentCheck::_tick()
{
    if (_busy || _checking) return;
    _busy = true;
    FlagGuard working{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartContentCheck> self(this);
    const quint64 generation = _generation;
    if (!_active) {
        const bool draining = _transportDraining;
        _drain();
        if (!self || _generation != generation) return;
        if (_state == QStringLiteral("content-matched")) {
            if (!_live(generation) || !_gate(generation, true) || !self) {
                if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("fresh-start-content-context-expired-or-changed"));
                return;
            }
        } else if (!_transportDraining) _timer.stop();
        if (draining != _transportDraining) emit changed();
        return;
    }
    if (!_live(generation)) { _finish(generation, QStringLiteral("failed"), QStringLiteral("content-check-total-deadline-expired")); return; }
    if (!_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("content-check-context-invalidated"));
        return;
    }
    if (_targetIndex >= _targets.size()) {
        qint64 expires = _clock.elapsed() + ReviewLifetimeMs;
        QVariantList readbacks;
        for (const Target& target : _targets) {
            const qint64 beforeRead = _clock.elapsed();
            QVariantMap receipt;
            if (!_receiptCurrent(target, generation, &receipt) || !self) {
                if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("whole-new-raw-read-revalidation-failed"));
                return;
            }
            double age = 0;
            if (!integer(receipt.value(QStringLiteral("ageMs")), 0, BatchLifetimeMs, &age)) {
                _finish(generation, QStringLiteral("failed"), QStringLiteral("raw-read-age-invalid")); return;
            }
            // Native age is measured from original READ START, not completion.
            // Clock domains are not compared; only its bounded elapsed age is.
            expires = qMin(expires, beforeRead + ReviewLifetimeMs - static_cast<qint64>(age));
            readbacks.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
                {QStringLiteral("nativeReceipt"), stableReceipt(receipt)}, {QStringLiteral("canonicalWireItems"), target.canonical.value(QStringLiteral("items"))},
                {QStringLiteral("contentHash"), target.canonical.value(QStringLiteral("contentHash"))},
                {QStringLiteral("wireSchema"), target.canonical.value(QStringLiteral("wireSchema"))},
                {QStringLiteral("matched"), true}, {QStringLiteral("requestAttributionVerified"), false},
                {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("remoteAtomicSnapshotVerified"), false}});
        }
        if (!_gate(generation, true) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("whole-group-final-context-changed"));
            return;
        }
        if (_clock.elapsed() >= expires) {
            _finish(generation, QStringLiteral("matched-stale"), QStringLiteral("matched-read-start-evidence-too-old-for-new-review")); return;
        }
        _verifiedUntilMs = expires;
        _sourceFingerprint = hash(QVariantMap{{QStringLiteral("checkId"), _checkId}, {QStringLiteral("checkEpoch"), _checkEpoch},
            {QStringLiteral("generation"), QString::number(generation)}, {QStringLiteral("deploymentContext"), _deploymentContext},
            {QStringLiteral("recordHash"), QString::fromLatin1(QCryptographicHash::hash(_recordBytes, QCryptographicHash::Sha256).toHex())},
            {QStringLiteral("roleSnapshot"), _roleReceipt}, {QStringLiteral("primaryIdentity"), _identityReceipt}, {QStringLiteral("readbacks"), readbacks}});
        if (!digest(_sourceFingerprint)) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("whole-content-source-capture-budget-exceeded")); return;
        }
        _finish(generation, QStringLiteral("content-matched"), QString());
        return;
    }
    if (_clock.elapsed() >= _targetDeadlineMs) { _finish(generation, QStringLiteral("failed"), QStringLiteral("bounded-complete-mission-read-timeout")); return; }
    const Target target = _targets.at(_targetIndex);
    if (target.state == QStringLiteral("queued")) {
        const qint64 now = _clock.elapsed();
        if (_lastRequestAtMs >= 0 && now - _lastRequestAtMs < PollMs) return;
        const QString scope = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const int index = _targetIndex;
        const qint64 deadline = _targetDeadlineMs;
        _targets[index].readScopeId = scope; _targets[index].state = QStringLiteral("reading-content");
        const QPointer<FleetOperatorSession> session(_session);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QPointer<FleetMissionPlanStore> store(_store);
        const QPointer<FleetMissionDeployment> deployment(_deployment);
        const QPointer<SwarmController> swarm(_swarm);
        const QPointer<QObject> owner(_authorization);
        const QVariantMap identity = _identityReceipt, role = _roleReceipt, binding = _binding;
        const QString requestId = _checkId, deploymentId = _deploymentId;
        const auto fence = [self, generation, index, deadline, scope, target, session, roles, store, deployment, swarm, owner,
            identity, role, binding, requestId, deploymentId]() {
            const auto same = [&]() {
                if (!self || !outboundSources(self.data(), session.data(), deployment.data())
                    || !self->_active || !self->_live(generation) || self->_clock.elapsed() >= deadline
                    || self->_targetDeadlineMs != deadline || self->_targetIndex != index || index < 0 || index >= self->_targets.size()
                    || !session || !roles || !store || !deployment || !swarm || !owner
                    || self->_session != session || self->_roles != roles || self->_store != store || self->_deployment != deployment
                    || self->_swarm != swarm || self->_authorization != owner || self->_identityReceipt != identity || self->_roleReceipt != role
                    || self->_binding != binding || self->_checkId != requestId || self->_deploymentId != deploymentId) return false;
                const Target current = self->_targets.at(index);
                return target.vehicle && target.mission && target.parameters && target.links && target.primary
                    && current.vehicle == target.vehicle && current.mission == target.mission && current.parameters == target.parameters
                    && current.links == target.links && current.primary == target.primary && current.payload == target.payload
                    && current.versionEpoch == target.versionEpoch && current.parameterEpoch == target.parameterEpoch
                    && current.readScopeId == scope && current.state == QStringLiteral("reading-content");
            };
            // Mandatory native callback runs after plugins and immediately
            // prequeue, including retries/end-ACK. Own scope is fixed before
            // native operation entry, not trusted from a returned JSON nonce.
            return same() && self->_gate(generation) && same();
        };
        _lastRequestAtMs = now; _issuedAnyRequest = true;
        const QVariantMap issued = target.mission->loadFleetMissionForStart(target.primary.data(), scope, fence);
        if (!self || !_live(generation) || !_active) return;
        if (_targetIndex != index || _targets.at(index).readScopeId != scope || !_gate(generation) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("native-read-context-changed-before-return"));
            return;
        }
        quint64 operationId = 0, baseline = 0;
        double vehicle = 0, component = 0;
        if (_clock.elapsed() >= deadline || !isTrue(issued.value(QStringLiteral("ok"))) || !isTrue(issued.value(QStringLiteral("requestIssued")))
            || !decimal(issued.value(QStringLiteral("operationId")), &operationId) || operationId == 0
            || !decimal(issued.value(QStringLiteral("baselineReceiptSequence")), &baseline)
            || issued.value(QStringLiteral("readScopeId")) != scope || !uuid(issued.value(QStringLiteral("sourceEpoch")))
            || issued.value(QStringLiteral("vehicleSourceEpoch")) != target.versionEpoch
            || !integer(issued.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId, &vehicle)
            || !integer(issued.value(QStringLiteral("componentId")), target.componentId, target.componentId, &component)
            || (_targets.at(index).operationId != 0 && _targets.at(index).operationId != operationId)) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("new-scoped-native-read-not-issued-or-invalid")); return;
        }
        _targets[index].operationId = operationId;
        _targets[index].baselineSequence = issued.value(QStringLiteral("baselineReceiptSequence")).toString();
        _targets[index].managerEpoch = issued.value(QStringLiteral("sourceEpoch")).toString();
        _publish(generation);
        return;
    }
    // Never inspect or advance from a finish signal's synchronous stack.
    if (target.mission->inProgress() || target.mission->transportCallInProgress()) return;
    QVariantMap receipt;
    if (!_receiptCurrent(target, generation, &receipt) || !self) {
        if (self) _finish(generation, QStringLiteral("failed"), QStringLiteral("complete-new-raw-int-read-not-matched"));
        return;
    }
    if (!_gate(generation) || !self || _clock.elapsed() >= _targetDeadlineMs) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("read-context-changed-at-complete-capture"));
        return;
    }
    _targets[_targetIndex].receipt = receipt;
    _targets[_targetIndex].canonical = canonicalRaw(receipt, target.payload.value(QStringLiteral("itemCount")).toInt());
    _targets[_targetIndex].state = QStringLiteral("content-matched");
    ++_targetIndex;
    if (_targetIndex < _targets.size()) _targetDeadlineMs = qMin(_createdAtMs + BatchLifetimeMs,
        _clock.elapsed() + qMin<qint64>(BatchLifetimeMs, 4000 + _targets.at(_targetIndex).payload.value(QStringLiteral("itemCount")).toInt() * 250LL));
    _publish(generation);
}
bool FleetMissionStartContentCheck::cancel(const QString& requestId)
{
    if (thread() != QThread::currentThread() || requestId != _checkId || (!_active && _state != QStringLiteral("content-matched"))) return false;
    _finish(_generation, QStringLiteral("canceled"), QStringLiteral("operator-canceled-read-only-content-check"));
    return true;
}
QVariantMap FleetMissionStartContentCheck::reviewContext(const QString& checkId, const QString& deploymentId)
{
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _deployment.data())
        || _busy || _checking || _active || _reservationHeld || _transportDraining
        || checkId != _checkId || deploymentId != _deploymentId || _state != QStringLiteral("content-matched") || !_live(_generation))
        return rejection(QStringLiteral("fresh-complete-content-context-unavailable"));
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartContentCheck> self(this);
    const quint64 generation = _generation;
    if (!_gate(generation, true) || !self) return rejection(QStringLiteral("actual-content-context-revalidation-failed"));
    QVariantList readbacks;
    for (const Target& target : _targets)
        readbacks.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("nativeReceipt"), stableReceipt(target.receipt)},
            {QStringLiteral("canonicalWireItems"), target.canonical.value(QStringLiteral("items"))}, {QStringLiteral("contentHash"), target.canonical.value(QStringLiteral("contentHash"))},
            {QStringLiteral("wireSchema"), target.canonical.value(QStringLiteral("wireSchema"))}, {QStringLiteral("matched"), true},
            {QStringLiteral("requestAttributionVerified"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
            {QStringLiteral("remoteAtomicSnapshotVerified"), false}});
    if (!outboundSources(this, _session.data(), _deployment.data()) || !_live(generation))
        return rejection(QStringLiteral("original-read-start-deadline-expired"));
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true}, {QStringLiteral("checkId"), _checkId},
        {QStringLiteral("checkEpoch"), _checkEpoch}, {QStringLiteral("checkGeneration"), QString::number(generation)},
        {QStringLiteral("deploymentId"), _deploymentId}, {QStringLiteral("deploymentGeneration"), _deploymentContext.value(QStringLiteral("generation"))},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("plan"), _deploymentContext.value(QStringLiteral("plan"))},
        {QStringLiteral("payloadBuild"), _deploymentContext.value(QStringLiteral("payloadBuild"))}, {QStringLiteral("sourceFingerprint"), _sourceFingerprint},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("readbacks"), readbacks}, {QStringLiteral("allFreshOnboardReadsMatched"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("remainingLifetimeMs"), static_cast<double>(_verifiedUntilMs - _clock.elapsed())}};
}
bool FleetMissionStartContentCheck::reviewContextCurrent(const QString& checkId, const QString& deploymentId, const QVariantMap& supplied)
{
    const QVariantMap expected = supplied;
    double schema = 0, remaining = 0;
    if (expected.size() != 18 || !integer(expected.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || !isTrue(expected.value(QStringLiteral("ok"))) || !isTrue(expected.value(QStringLiteral("allFreshOnboardReadsMatched")))
        || !uuid(expected.value(QStringLiteral("checkId"))) || !uuid(expected.value(QStringLiteral("checkEpoch")))
        || !positiveDecimal(expected.value(QStringLiteral("checkGeneration"))) || !uuid(expected.value(QStringLiteral("deploymentId")))
        || !positiveDecimal(expected.value(QStringLiteral("deploymentGeneration"))) || !digest(expected.value(QStringLiteral("sourceFingerprint")))
        || !isFalse(expected.value(QStringLiteral("startupPermitGranted"))) || !isFalse(expected.value(QStringLiteral("executable")))
        || !isFalse(expected.value(QStringLiteral("flightCommandReleased")))
        || !integer(expected.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewLifetimeMs, &remaining)) return false;
    const QPointer<FleetMissionStartContentCheck> self(this);
    const QVariantMap actual = reviewContext(checkId, deploymentId);
    if (!self || !isTrue(actual.value(QStringLiteral("ok")))) return false;
    QByteArray expectedBytes, actualBytes;
    return encode(stableContext(expected), &expectedBytes) && encode(stableContext(actual), &actualBytes) && expectedBytes == actualBytes;
}
Vehicle* FleetMissionStartContentCheck::reviewVehicle(const QString& checkId, const QString& deploymentId, int vehicleId) const
{
    if (thread() != QThread::currentThread() || _active || _reservationHeld || _transportDraining || !_live(_generation)
        || _state != QStringLiteral("content-matched") || checkId != _checkId || deploymentId != _deploymentId) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId)
        return _targetCurrent(target, _generation, false) ? target.vehicle.data() : nullptr;
    return nullptr;
}
MissionManager* FleetMissionStartContentCheck::reviewManager(const QString& checkId, const QString& deploymentId, int vehicleId) const
{
    if (!reviewVehicle(checkId, deploymentId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.mission.data();
    return nullptr;
}
LinkInterface* FleetMissionStartContentCheck::reviewPrimaryLink(const QString& checkId, const QString& deploymentId, int vehicleId) const
{
    if (!reviewVehicle(checkId, deploymentId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.primary.data();
    return nullptr;
}

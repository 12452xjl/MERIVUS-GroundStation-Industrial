#include "FleetOccupancyRecoveryEvidence.h"

#include "LinkInterface.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCMAVLink.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>

FleetOccupancyRecoveryEvidence::Target::Target() = default;
FleetOccupancyRecoveryEvidence::Target::~Target() = default;
FleetOccupancyRecoveryEvidence::Target::Target(const Target&) = default;
FleetOccupancyRecoveryEvidence::Target& FleetOccupancyRecoveryEvidence::Target::operator=(const Target&) = default;

namespace {
using Target = FleetOccupancyRecoveryEvidence::Target;
constexpr qint64 MaximumAgeMs = 2000;

bool fields(const QVariantMap& value, const QStringList& expected)
{
    if (value.size() != expected.size()) return false;
    for (const QString& key : expected) if (!value.contains(key)) return false;
    return true;
}

bool integer(const QVariant& value, double minimum, double maximum)
{
    if (value.isNull() || !(value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
        || value.type() == QVariant::Double)) return false;
    bool ok = false;
    const double actual = value.toDouble(&ok);
    return ok && std::isfinite(actual) && std::floor(actual) == actual
        && actual >= minimum && actual <= maximum;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool uuid(const QVariant& value)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString().size() == 36
        && !QUuid(value.toString()).isNull()
        && QUuid(value.toString()).toString(QUuid::WithoutBraces) == value.toString();
}

bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (const QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == text;
}

bool sha256(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

QString valueHash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}

bool rawValueValid(const QString& channel, const QVariantMap& value, bool requireGround)
{
    if (channel == QStringLiteral("heartbeat")) {
        if (!fields(value, {QStringLiteral("armed"), QStringLiteral("baseMode"), QStringLiteral("customMode"),
                QStringLiteral("systemStatus"), QStringLiteral("autopilot"), QStringLiteral("vehicleType")})
            || value.value(QStringLiteral("armed")).type() != QVariant::Bool
            || value.value(QStringLiteral("armed")).isNull()
            || !integer(value.value(QStringLiteral("baseMode")), 0, 255)
            || !integer(value.value(QStringLiteral("customMode")), 0, 4294967295.0)
            || !integer(value.value(QStringLiteral("systemStatus")), 0, MAV_STATE_ENUM_END - 1)
            || !integer(value.value(QStringLiteral("autopilot")), 0, MAV_AUTOPILOT_ENUM_END - 1)
            || !integer(value.value(QStringLiteral("vehicleType")), 0, MAV_TYPE_ENUM_END - 1)) return false;
        const bool armed = (value.value(QStringLiteral("baseMode")).toUInt() & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
        return value.value(QStringLiteral("armed")).toBool() == armed && (!requireGround || !armed);
    }
    return channel == QStringLiteral("landedState")
        && fields(value, {QStringLiteral("landedState"), QStringLiteral("vtolState"),
            QStringLiteral("landedStateKnown"), QStringLiteral("vtolStateKnown")})
        && integer(value.value(QStringLiteral("landedState")), 0, MAV_LANDED_STATE_ENUM_END - 1)
        && integer(value.value(QStringLiteral("vtolState")), 0, MAV_VTOL_STATE_ENUM_END - 1)
        && flag(value.value(QStringLiteral("landedStateKnown")), value.value(QStringLiteral("landedState")).toUInt() != MAV_LANDED_STATE_UNDEFINED)
        && flag(value.value(QStringLiteral("vtolStateKnown")), value.value(QStringLiteral("vtolState")).toUInt() != MAV_VTOL_STATE_UNDEFINED)
        && (!requireGround || (flag(value.value(QStringLiteral("landedStateKnown")), true)
            && value.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND));
}

bool semanticRowValid(const QString& channel, const QVariantMap& row)
{
    const QVariant raw = row.value(QStringLiteral("value"));
    return fields(row, {QStringLiteral("contentRevision"), QStringLiteral("contentHash"), QStringLiteral("value")})
        && decimal(row.value(QStringLiteral("contentRevision"))) && sha256(row.value(QStringLiteral("contentHash")))
        && raw.type() == QVariant::Map && !raw.isNull() && rawValueValid(channel, raw.toMap(), true)
        && valueHash(raw.toMap()) == row.value(QStringLiteral("contentHash")).toString();
}

bool identityValid(const QVariantMap& identity)
{
    const QVariant heartbeat = identity.value(QStringLiteral("heartbeat"));
    const QVariant landed = identity.value(QStringLiteral("landedState"));
    return fields(identity, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("versionSourceEpoch"), QStringLiteral("heartbeat"), QStringLiteral("landedState")})
        && integer(identity.value(QStringLiteral("schemaVersion")), 1, 1)
        && integer(identity.value(QStringLiteral("vehicleId")), 1, 255)
        && integer(identity.value(QStringLiteral("componentId")), 1, 255)
        && uuid(identity.value(QStringLiteral("sourceEpoch"))) && uuid(identity.value(QStringLiteral("versionSourceEpoch")))
        && heartbeat.type() == QVariant::Map && !heartbeat.isNull() && semanticRowValid(QStringLiteral("heartbeat"), heartbeat.toMap())
        && landed.type() == QVariant::Map && !landed.isNull() && semanticRowValid(QStringLiteral("landedState"), landed.toMap());
}

bool topologyCurrent(const Target& target)
{
    if (!qgcApp() || !qgcApp()->toolbox() || QThread::currentThread() != qgcApp()->thread()
        || target.vehicleId < 1 || target.vehicleId > 255 || target.componentId < 1 || target.componentId > 255
        || !target.vehicle || !target.manager || !target.parameters || !target.links || !target.vehicles || !target.model
        || !target.primary || !target.primaryObject || target.primary.get() != target.primaryObject.data()) return false;
    QThread* const gui = QThread::currentThread();
    if (target.vehicle->thread() != gui || target.manager->thread() != gui || target.parameters->thread() != gui
        || target.links->thread() != gui || target.vehicles->thread() != gui || target.model->thread() != gui
        || qgcApp()->toolbox()->multiVehicleManager() != target.vehicles.data()
        || target.vehicles->vehicles() != target.model.data()
        || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
        || target.vehicle->missionManager() != target.manager.data()
        || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
        || target.vehicle->vehicleLinkManager() != target.links.data() || target.vehicle->isOfflineEditingVehicle()
        || !target.vehicle->isInitialConnectComplete() || target.links->communicationLost()) return false;
    const WeakLinkInterfacePtr expected(target.primary);
    const WeakLinkInterfacePtr current = target.links->primaryLink();
    if (current.expired() || current.owner_before(expected) || expected.owner_before(current)) return false;
    const int count = target.model->count();
    if (count < 1 || count > 255) return false;
    int sameId = 0;
    for (int index = 0; index < count; ++index) {
        const QPointer<Vehicle> actual(qobject_cast<Vehicle*>(target.model->get(index)));
        if (!actual || actual->thread() != gui) return false;
        if (actual->id() == target.vehicleId) {
            ++sameId;
            if (actual != target.vehicle) return false;
        }
    }
    return sameId == 1;
}

bool quietGroundCurrent(const Target& target)
{
    return topologyCurrent(target) && !target.vehicle->armed() && !target.vehicle->flying()
        && target.vehicle->fleetStartupTransportQuiescent()
        && !target.manager->inProgress() && !target.manager->transportCallInProgress()
        && target.manager->activeOperationId() == 0 && !target.parameters->pendingWrites();
}

bool fullGroundCurrent(const Target& target)
{
    if (!quietGroundCurrent(target)) return false;
    const bool allowed = target.vehicle->fleetOccupancyRecoveryTransportQuiescent(target.primaryObject.data());
    return quietGroundCurrent(target) && allowed;
}

bool connectedSource(const Target& target)
{
    if (!topologyCurrent(target)) return false;
    const bool connected = target.primaryObject->isConnected();
    if (!topologyCurrent(target) || !connected) return false;
    const bool replay = target.primaryObject->isLogReplay();
    if (!topologyCurrent(target) || replay) return false;
    const bool mock = target.primaryObject->isMockLink();
    if (!topologyCurrent(target) || mock) return false;
    const bool versionSource = target.vehicle->autopilotVersionObservationSourceMatches(target.primaryObject.data());
    return topologyCurrent(target) && versionSource;
}

QString versionEpoch(const Target& target)
{
    if (!connectedSource(target)) return {};
    // Availability is a native source predicate. Version age itself is not a
    // ground/health requirement and is not promoted to aircraft authentication.
    const QVariantMap version = target.vehicle->autopilotVersionObservation(120000);
    if (!topologyCurrent(target) || !integer(version.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(version.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId)
        || !integer(version.value(QStringLiteral("componentId")), target.componentId, target.componentId)
        || !flag(version.value(QStringLiteral("observed")), true) || !flag(version.value(QStringLiteral("available")), true)
        || !uuid(version.value(QStringLiteral("sourceEpoch"))) || !decimal(version.value(QStringLiteral("contentRevision")))
        || !sha256(version.value(QStringLiteral("contentHash")))
        || !flag(version.value(QStringLiteral("authenticated")), false)
        || !flag(version.value(QStringLiteral("executionPermit")), false)
        || !flag(version.value(QStringLiteral("flightCommandReleased")), false)) return {};
    return version.value(QStringLiteral("sourceEpoch")).toString();
}

bool flightShape(const QVariantMap& observation, const Target& target)
{
    const QVariant channels = observation.value(QStringLiteral("channels"));
    return fields(observation, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("channels"), QStringLiteral("signatureVerified"),
            QStringLiteral("authenticated"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        && integer(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        && integer(observation.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId)
        && integer(observation.value(QStringLiteral("componentId")), target.componentId, target.componentId)
        && uuid(observation.value(QStringLiteral("sourceEpoch")))
        && flag(observation.value(QStringLiteral("signatureVerified")), false)
        && flag(observation.value(QStringLiteral("authenticated")), false)
        && flag(observation.value(QStringLiteral("executionPermit")), false)
        && flag(observation.value(QStringLiteral("flightCommandReleased")), false)
        && channels.type() == QVariant::Map && !channels.isNull()
        && fields(channels.toMap(), {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
            QStringLiteral("landedState"), QStringLiteral("missionCurrent"), QStringLiteral("missionReached")});
}

bool freshRow(const QString& channel, const QVariantMap& row)
{
    const QVariant value = row.value(QStringLiteral("value"));
    return fields(row, {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"), QStringLiteral("ageMs"),
            QStringLiteral("source"), QStringLiteral("contentRevision"), QStringLiteral("receiptSequence"),
            QStringLiteral("contentHash"), QStringLiteral("value")})
        && flag(row.value(QStringLiteral("observed")), true) && flag(row.value(QStringLiteral("available")), true)
        && flag(row.value(QStringLiteral("fresh")), true) && integer(row.value(QStringLiteral("ageMs")), 0, MaximumAgeMs)
        && row.value(QStringLiteral("source")).type() == QVariant::String
        && row.value(QStringLiteral("source")).toString() == (channel == QStringLiteral("heartbeat")
            ? QStringLiteral("live-heartbeat") : QStringLiteral("live-extended-sys-state"))
        && decimal(row.value(QStringLiteral("contentRevision"))) && decimal(row.value(QStringLiteral("receiptSequence")))
        && sha256(row.value(QStringLiteral("contentHash"))) && value.type() == QVariant::Map && !value.isNull()
        && rawValueValid(channel, value.toMap(), true) && valueHash(value.toMap()) == row.value(QStringLiteral("contentHash")).toString();
}

QVariantMap capturedValue(const Target& target, const QString& version, const QVariantMap& flight)
{
    if (!quietGroundCurrent(target) || !uuid(version) || !flightShape(flight, target)) return {};
    const QVariantMap channels = flight.value(QStringLiteral("channels")).toMap();
    QVariantMap selected;
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
        const QVariant raw = channels.value(channel);
        if (raw.type() != QVariant::Map || raw.isNull() || !freshRow(channel, raw.toMap())) return {};
        selected.insert(channel, raw);
    }
    const QVariantMap heartbeat = selected.value(QStringLiteral("heartbeat")).toMap().value(QStringLiteral("value")).toMap();
    if (heartbeat.value(QStringLiteral("autopilot")).toUInt() != static_cast<uint>(target.vehicle->firmwareType())
        || heartbeat.value(QStringLiteral("vehicleType")).toUInt() != static_cast<uint>(target.vehicle->vehicleType())) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("componentId"), target.componentId}, {QStringLiteral("sourceEpoch"), flight.value(QStringLiteral("sourceEpoch"))},
        {QStringLiteral("versionSourceEpoch"), version}, {QStringLiteral("channels"), selected},
        {QStringLiteral("localTransportQuiescent"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("remoteTaskAbsenceVerified"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

} // namespace

FleetOccupancyRecoveryEvidence::Target FleetOccupancyRecoveryEvidence::bind(Vehicle* actualVehicle)
{
    Target target;
    target.vehicle = actualVehicle;
    if (!target.vehicle || !qgcApp() || !qgcApp()->toolbox()
        || QThread::currentThread() != qgcApp()->thread() || target.vehicle->thread() != QThread::currentThread()) return {};
    target.vehicleId = target.vehicle->id();
    target.componentId = target.vehicle->defaultComponentId();
    target.manager = target.vehicle->missionManager();
    target.parameters = target.vehicle->parameterManager();
    target.links = target.vehicle->vehicleLinkManager();
    target.vehicles = qgcApp()->toolbox()->multiVehicleManager();
    if (!target.links || !target.vehicles || target.links->thread() != QThread::currentThread()
        || target.vehicles->thread() != QThread::currentThread()) return {};
    target.model = target.vehicles->vehicles();
    target.primary = target.links->primaryLink().lock();
    target.primaryObject = target.primary.get();
    if (!topologyCurrent(target)) return {};
    return target;
}

QVariantMap FleetOccupancyRecoveryEvidence::capture(const Target& actualTarget)
{
    // Freeze the actual source values BEFORE any native getter may invoke a
    // link virtual. The caller/its member Target may be destroyed by reentry.
    const Target target(actualTarget);
    if (!fullGroundCurrent(target)) return {};
    const QString firstVersion = versionEpoch(target);
    if (!fullGroundCurrent(target) || firstVersion.isEmpty()) return {};
    const QVariantMap firstFlight = target.vehicle->flightStateObservation(MaximumAgeMs, MaximumAgeMs, MaximumAgeMs, MaximumAgeMs);
    const QVariantMap first = capturedValue(target, firstVersion, firstFlight);
    const QVariantMap firstIdentity = stableIdentity(first);
    if (firstIdentity.isEmpty()) return {};
    const QString finalVersion = versionEpoch(target);
    if (!fullGroundCurrent(target) || finalVersion != firstVersion) return {};
    const QVariantMap finalFlight = target.vehicle->flightStateObservation(MaximumAgeMs, MaximumAgeMs, MaximumAgeMs, MaximumAgeMs);
    // No link virtual/source callback follows this final RAW copy. Native
    // topology/quiet and value checks below cannot attach an older source.
    const QVariantMap final = capturedValue(target, finalVersion, finalFlight);
    return stableIdentity(final) == firstIdentity ? final : QVariantMap();
}

QVariantMap FleetOccupancyRecoveryEvidence::stableIdentity(const QVariantMap& captured)
{
    if (!fields(captured, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("versionSourceEpoch"), QStringLiteral("channels"),
            QStringLiteral("localTransportQuiescent"), QStringLiteral("authenticated"), QStringLiteral("remoteTaskAbsenceVerified"),
            QStringLiteral("flightCommandReleased")})
        || !integer(captured.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(captured.value(QStringLiteral("vehicleId")), 1, 255)
        || !integer(captured.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(captured.value(QStringLiteral("sourceEpoch"))) || !uuid(captured.value(QStringLiteral("versionSourceEpoch")))
        || !flag(captured.value(QStringLiteral("localTransportQuiescent")), true)
        || !flag(captured.value(QStringLiteral("authenticated")), false)
        || !flag(captured.value(QStringLiteral("remoteTaskAbsenceVerified")), false)
        || !flag(captured.value(QStringLiteral("flightCommandReleased")), false)
        || captured.value(QStringLiteral("channels")).type() != QVariant::Map
        || captured.value(QStringLiteral("channels")).isNull()) return {};
    const QVariantMap channels = captured.value(QStringLiteral("channels")).toMap();
    if (!fields(channels, {QStringLiteral("heartbeat"), QStringLiteral("landedState")})) return {};
    QVariantMap identity{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("vehicleId"), captured.value(QStringLiteral("vehicleId")).toInt()},
        {QStringLiteral("componentId"), captured.value(QStringLiteral("componentId")).toInt()},
        {QStringLiteral("sourceEpoch"), captured.value(QStringLiteral("sourceEpoch"))},
        {QStringLiteral("versionSourceEpoch"), captured.value(QStringLiteral("versionSourceEpoch"))}};
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
        const QVariant raw = channels.value(channel);
        if (raw.type() != QVariant::Map || raw.isNull() || !freshRow(channel, raw.toMap())) return {};
        const QVariantMap row = raw.toMap();
        identity.insert(channel, QVariantMap{{QStringLiteral("contentRevision"), row.value(QStringLiteral("contentRevision"))},
            {QStringLiteral("contentHash"), row.value(QStringLiteral("contentHash"))}, {QStringLiteral("value"), row.value(QStringLiteral("value"))}});
    }
    return identityValid(identity) ? identity : QVariantMap();
}

bool FleetOccupancyRecoveryEvidence::bindingCurrent(const Target& actualTarget, const QVariantMap& frozenStableIdentity)
{
    const Target target(actualTarget);
    const QVariantMap identity = frozenStableIdentity;
    if (!identityValid(identity) || !topologyCurrent(target)
        || identity.value(QStringLiteral("vehicleId")).toInt() != target.vehicleId
        || identity.value(QStringLiteral("componentId")).toInt() != target.componentId) return false;
    // This dedicated core predicate reads owned source memory only. A normal
    // receive stack, an updating row, new ARM/landed semantics or age growth
    // must not turn an unchanged retained source into a permanent restart hold.
    const bool source = target.vehicle->fleetOccupancyRecoverySourceMatches(target.primaryObject.data(),
        identity.value(QStringLiteral("sourceEpoch")).toString(), identity.value(QStringLiteral("versionSourceEpoch")).toString());
    return topologyCurrent(target) && source;
}

bool FleetOccupancyRecoveryEvidence::finalCurrent(const Target& actualTarget, const QVariantMap& frozenStableIdentity)
{
    const Target target(actualTarget);
    const QVariantMap identity = frozenStableIdentity;
    if (!identityValid(identity) || !topologyCurrent(target)
        || identity.value(QStringLiteral("vehicleId")).toInt() != target.vehicleId
        || identity.value(QStringLiteral("componentId")).toInt() != target.componentId) return false;
    const bool current = target.vehicle->fleetOccupancyRecoveryGroundMatches(target.primaryObject.data(), identity);
    return topologyCurrent(target) && current;
}

#include "FleetReserveEnergyPlan.h"
#include "FleetCapabilityEvidence.h"

#include <QCryptographicHash>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <cmath>

namespace {
constexpr int MaximumPathPoints = 4096;
constexpr int MaximumPrimaryMembers = 255;
constexpr int MaximumTargetJsonBytes = 2 * 1024 * 1024;

bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double: break;
    default: return false;
    }
    const double parsed = value.toDouble();
    if (!qIsFinite(parsed) || parsed < minimum || parsed > maximum) return false;
    if (output) *output = parsed;
    return true;
}

bool identifier(const QVariant& value, int* output)
{
    double parsed = 0.0;
    if (!number(value, 1.0, 255.0, &parsed) || std::floor(parsed) != parsed) return false;
    *output = int(parsed);
    return true;
}

bool fields(const QVariantMap& map, const QSet<QString>& allowed, bool exact = false)
{
    if (map.size() > allowed.size() || (exact && map.size() != allowed.size())) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!allowed.contains(it.key())) return false;
    return true;
}

QVariantMap policyResult(bool ok, const QVariantMap& policy, const QString& reason)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("policy"), policy}, {QStringLiteral("reason"), reason}};
}

QVariantMap emptyPlan()
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("complete"), false},
        {QStringLiteral("reason"), QStringLiteral("reserve_descriptor_invalid")},
        {QStringLiteral("vehicleId"), -1}, {QStringLiteral("replacementVehicleId"), -1}, {QStringLiteral("policy"), QVariantMap()},
        {QStringLiteral("standbyDurationSeconds"), -1.0}, {QStringLiteral("standbyEquivalentFlightSeconds"), -1.0},
        {QStringLiteral("estimatedIngressSeconds"), -1.0}, {QStringLiteral("estimatedTaskSeconds"), -1.0},
        {QStringLiteral("targetHoldSeconds"), -1.0}, {QStringLiteral("missionEquivalentFlightSeconds"), -1.0},
        {QStringLiteral("estimatedReturnSeconds"), -1.0}, {QStringLiteral("targetRouteHash"), QStringLiteral("")},
        {QStringLiteral("outboundPath"), QVariantList()}, {QStringLiteral("returnPath"), QVariantList()},
        {QStringLiteral("outboundDistanceMeters"), -1.0}, {QStringLiteral("returnDistanceMeters"), -1.0},
        {QStringLiteral("targetWaypointCount"), -1}, {QStringLiteral("speedCapabilityKnown"), false},
        {QStringLiteral("declaredMaxSpeedMetersPerSecond"), -1.0},
        {QStringLiteral("timingBasis"), QStringLiteral("sequential-horizontal-climb-descent-estimate")},
        {QStringLiteral("loadBasis"), QStringLiteral("declared-standby-to-nominal-flight-ratio")},
        {QStringLiteral("targetRouteMode"), QStringLiteral("full-primary-route-redo")},
        {QStringLiteral("routeClearanceVerified"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("nominalLoadAssumptionVerified"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool point(const QVariant& value, bool routePoint, int sequence, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    QSet<QString> allowed{QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")};
    if (routePoint) {
        allowed.unite({QStringLiteral("sequence"), QStringLiteral("heading"), QStringLiteral("holdSeconds"),
            QStringLiteral("timeSeconds"), QStringLiteral("estimatedElapsedSeconds"),
            QStringLiteral("timeReference"), QStringLiteral("timingEvidence")});
    }
    if (!fields(map, allowed, !routePoint)
        || map.value(QStringLiteral("coordinateSystem")).type() != QVariant::String
        || map.value(QStringLiteral("coordinateSystem")).toString() != QStringLiteral("geodetic")
        || map.value(QStringLiteral("altitudeReference")).type() != QVariant::String
        || map.value(QStringLiteral("altitudeReference")).toString() != QStringLiteral("amsl")) return false;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0, hold = 0.0;
    if (!number(map.value(QStringLiteral("latitude")), -90.0, 90.0, &latitude)
        || !number(map.value(QStringLiteral("longitude")), -180.0, 180.0, &longitude)
        || !number(map.value(QStringLiteral("altitude")), -1000.0, 20000.0, &altitude)
        || (map.contains(QStringLiteral("holdSeconds"))
            && !number(map.value(QStringLiteral("holdSeconds")), 0.0, 86400.0, &hold))) return false;
    if (routePoint) {
        double suppliedSequence = 0.0;
        if (!number(map.value(QStringLiteral("sequence")), 0.0, MaximumPathPoints - 1, &suppliedSequence)
            || suppliedSequence != sequence) return false;
        // Validate bounded canonical metadata before hashing the original
        // route. None of these original timing/heading fields sets our speed,
        // arrival time, dwell, or duration for a different aircraft.
        for (const QString& key : {QStringLiteral("heading"), QStringLiteral("timeSeconds"), QStringLiteral("estimatedElapsedSeconds")}) {
            if (!map.contains(key)) continue;
            const double lower = key == QStringLiteral("estimatedElapsedSeconds") ? 0.0 : -1e15;
            if (!number(map.value(key), lower, 1e15)) return false;
        }
        for (const QString& key : {QStringLiteral("timeReference"), QStringLiteral("timingEvidence")}) {
            if (map.contains(key) && (map.value(key).type() != QVariant::String || map.value(key).isNull()
                || map.value(key).toString().isEmpty() || map.value(key).toString().size() > 128)) return false;
        }
    }
    *output = {{QStringLiteral("sequence"), sequence}, {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")}, {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude}, {QStringLiteral("holdSeconds"), hold}};
    return true;
}

bool leg(const QVariantMap& first, const QVariantMap& second, const QVariantMap& policy,
         double* duration, double* distance)
{
    const QGeoCoordinate origin(first.value(QStringLiteral("latitude")).toDouble(), first.value(QStringLiteral("longitude")).toDouble());
    const QGeoCoordinate destination(second.value(QStringLiteral("latitude")).toDouble(), second.value(QStringLiteral("longitude")).toDouble());
    const double horizontal = origin.distanceTo(destination);
    const double vertical = second.value(QStringLiteral("altitude")).toDouble() - first.value(QStringLiteral("altitude")).toDouble();
    const double verticalSpeed = policy.value(vertical >= 0.0 ? QStringLiteral("climbSpeedMetersPerSecond")
                                                           : QStringLiteral("descentSpeedMetersPerSecond")).toDouble();
    // A conservative sequential time allowance, not a generated vertical
    // corridor or a prediction that the aircraft will follow this schedule.
    *duration = horizontal / policy.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble()
        + std::abs(vertical) / verticalSpeed;
    *distance = std::hypot(horizontal, vertical);
    return qIsFinite(horizontal) && horizontal >= 0.0 && qIsFinite(*duration) && *duration >= 0.0
        && qIsFinite(*distance) && *distance >= 0.0;
}
} // namespace

QVariantMap FleetReserveEnergyPlan::normalize(const QVariantMap& source)
{
    const auto reject = [](const QString& reason) { return policyResult(false, {}, reason); };
    if (!fields(source, {QStringLiteral("schemaVersion"), QStringLiteral("replacementVehicleId"), QStringLiteral("standbyMode"),
            QStringLiteral("standbyDurationSeconds"), QStringLiteral("standbyConsumptionFactor"),
            QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("climbSpeedMetersPerSecond"),
            QStringLiteral("descentSpeedMetersPerSecond")}, true)) return reject(QStringLiteral("reserve_policy_invalid_fields"));
    if (!number(source.value(QStringLiteral("schemaVersion")), 1.0, 1.0)) return reject(QStringLiteral("reserve_policy_invalid_schema"));
    int replacementId = -1;
    if (!identifier(source.value(QStringLiteral("replacementVehicleId")), &replacementId)) return reject(QStringLiteral("reserve_policy_invalid_replacement"));
    if (source.value(QStringLiteral("standbyMode")).type() != QVariant::String) return reject(QStringLiteral("reserve_policy_invalid_standby_mode"));
    const QString mode = source.value(QStringLiteral("standbyMode")).toString();
    if (mode != QStringLiteral("ground-disarmed") && mode != QStringLiteral("airborne-hold")) return reject(QStringLiteral("reserve_policy_invalid_standby_mode"));
    double standby = 0.0, factor = 0.0;
    if (!number(source.value(QStringLiteral("standbyDurationSeconds")), 0.0, 86400.0, &standby)) return reject(QStringLiteral("reserve_policy_invalid_standby_duration"));
    if (!number(source.value(QStringLiteral("standbyConsumptionFactor")), 0.0, 10.0, &factor) || factor <= 0.0)
        return reject(QStringLiteral("reserve_policy_invalid_standby_factor"));
    QVariantMap policy{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("replacementVehicleId"), replacementId},
        {QStringLiteral("standbyMode"), mode}, {QStringLiteral("standbyDurationSeconds"), standby}, {QStringLiteral("standbyConsumptionFactor"), factor}};
    for (const QString& key : {QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("climbSpeedMetersPerSecond"),
            QStringLiteral("descentSpeedMetersPerSecond")}) {
        double speed = 0.0;
        if (!number(source.value(key), 0.0, 1000.0, &speed) || speed <= 0.0) return reject(QStringLiteral("reserve_policy_invalid_speed"));
        policy.insert(key, speed);
    }
    return policyResult(true, policy, QStringLiteral(""));
}

QVariantMap FleetReserveEnergyPlan::build(const QVariantMap& descriptor, const QVariantList& primaryMembers,
                                         const QVariantMap& constraints, const QVariantMap& reserveVehicle,
                                         const QVariant& performancePolicy)
{
    QVariantMap result = emptyPlan();
    const auto reject = [&result](const QString& reason) { result[QStringLiteral("reason")] = reason; return result; };
    if (performancePolicy.type() != QVariant::Bool || performancePolicy.isNull())
        return reject(QStringLiteral("reserve_performance_policy_invalid"));
    const bool requirePerformanceEvidence = performancePolicy.toBool();
    QVariantMap declaredPerformance;
    if (requirePerformanceEvidence) {
        result.insert(QStringLiteral("requirePerformanceEvidence"), true);
        result.insert(QStringLiteral("declaredPerformanceEvidence"), QVariantMap());
        result.insert(QStringLiteral("declaredPerformanceEvidenceUsable"), false);
    }
    if (!fields(descriptor, {QStringLiteral("vehicleId"), QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("reservePlan")}))
        return reject(QStringLiteral("reserve_descriptor_invalid"));
    int vehicleId = -1;
    if (!identifier(descriptor.value(QStringLiteral("vehicleId")), &vehicleId)) return reject(QStringLiteral("reserve_vehicle_id_invalid"));
    result[QStringLiteral("vehicleId")] = vehicleId;
    if (requirePerformanceEvidence) {
        if (!FleetCapabilityEvidence::currentDeclaredPerformance(reserveVehicle, &declaredPerformance)
            || declaredPerformance.value(QStringLiteral("vehicleId")).toInt() != vehicleId)
            return reject(QStringLiteral("reserve_performance_declaration_not_current"));
        result.insert(QStringLiteral("declaredPerformanceEvidence"), declaredPerformance);
        result.insert(QStringLiteral("declaredPerformanceEvidenceUsable"), true);
    }
    if (!descriptor.contains(QStringLiteral("reservePlan"))) return reject(QStringLiteral("reserve_policy_missing"));
    if (descriptor.value(QStringLiteral("reservePlan")).type() != QVariant::Map || descriptor.value(QStringLiteral("reservePlan")).isNull())
        return reject(QStringLiteral("reserve_policy_invalid_fields"));
    const QVariantMap normalized = normalize(descriptor.value(QStringLiteral("reservePlan")).toMap());
    if (!normalized.value(QStringLiteral("ok")).toBool()) return reject(normalized.value(QStringLiteral("reason")).toString());
    const QVariantMap policy = normalized.value(QStringLiteral("policy")).toMap();
    const int replacementId = policy.value(QStringLiteral("replacementVehicleId")).toInt();
    result[QStringLiteral("policy")] = policy; result[QStringLiteral("replacementVehicleId")] = replacementId;
    if (replacementId == vehicleId) return reject(QStringLiteral("reserve_replacement_is_self"));
    if (!descriptor.contains(QStringLiteral("start")) || !descriptor.contains(QStringLiteral("returnTo"))) return reject(QStringLiteral("reserve_endpoints_missing"));
    QVariantMap start, returnTo;
    if (!point(descriptor.value(QStringLiteral("start")), false, 0, &start)
        || !point(descriptor.value(QStringLiteral("returnTo")), false, 1, &returnTo)) return reject(QStringLiteral("reserve_endpoints_invalid"));
    if (primaryMembers.isEmpty() || primaryMembers.size() > MaximumPrimaryMembers) return reject(QStringLiteral("reserve_primary_members_invalid"));
    QVariantMap target;
    int matches = 0;
    QSet<int> primaryIds;
    for (const QVariant& value : primaryMembers) {
        if (value.type() != QVariant::Map || value.isNull()) return reject(QStringLiteral("reserve_primary_members_invalid"));
        const QVariantMap member = value.toMap();
        int id = -1;
        if (!identifier(member.value(QStringLiteral("vehicleId")), &id) || id == vehicleId || primaryIds.contains(id))
            return reject(QStringLiteral("reserve_primary_members_invalid"));
        primaryIds.insert(id);
        if (id == replacementId) { target = member; ++matches; }
    }
    if (matches != 1) return reject(QStringLiteral("reserve_replacement_not_found"));
    const QVariant routeValue = target.value(QStringLiteral("waypoints"));
    if (routeValue.type() != QVariant::List || routeValue.isNull() || routeValue.toList().isEmpty())
        return reject(QStringLiteral("reserve_target_route_missing"));
    const QVariantList originalRoute = routeValue.toList();
    if (originalRoute.size() > MaximumPathPoints || originalRoute.size() + 1 > MaximumPathPoints)
        return reject(QStringLiteral("reserve_generated_path_limit_exceeded"));
    bool ceilingKnown = constraints.contains(QStringLiteral("maxAltitudeMeters"));
    double ceiling = 0.0;
    if ((ceilingKnown || constraints.contains(QStringLiteral("maxAltitudeReference")))
        && (!ceilingKnown || !number(constraints.value(QStringLiteral("maxAltitudeMeters")), -1000.0, 20000.0, &ceiling)
            || constraints.value(QStringLiteral("maxAltitudeReference")).type() != QVariant::String
            || constraints.value(QStringLiteral("maxAltitudeReference")).toString() != QStringLiteral("amsl")))
        return reject(QStringLiteral("reserve_altitude_constraint_invalid"));
    if (ceilingKnown && (start.value(QStringLiteral("altitude")).toDouble() > ceiling
        || returnTo.value(QStringLiteral("altitude")).toDouble() > ceiling)) return reject(QStringLiteral("reserve_altitude_exceeded"));
    bool speedKnown = false;
    double maximumSpeed = -1.0;
    if (!reserveVehicle.isEmpty()) {
        bool identityPresent = false;
        for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id")}) {
            if (!reserveVehicle.contains(key)) continue;
            int suppliedId = -1;
            if (!identifier(reserveVehicle.value(key), &suppliedId) || suppliedId != vehicleId) return reject(QStringLiteral("reserve_vehicle_identity_mismatch"));
            identityPresent = true;
        }
        if (!identityPresent) return reject(QStringLiteral("reserve_vehicle_identity_missing"));
        if (requirePerformanceEvidence) {
            maximumSpeed = declaredPerformance.value(QStringLiteral("maxSpeedMps")).toDouble();
            if (maximumSpeed < 0.0) return reject(QStringLiteral("reserve_declared_speed_unknown"));
            speedKnown = true;
            if (policy.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble() > maximumSpeed)
                return reject(QStringLiteral("reserve_speed_exceeds_declared_capability"));
        } else if (reserveVehicle.contains(QStringLiteral("maxSpeedMps"))) {
            if (!number(reserveVehicle.value(QStringLiteral("maxSpeedMps")), -1.0, 1000.0, &maximumSpeed)
                || (maximumSpeed < 0.0 && maximumSpeed != -1.0)) return reject(QStringLiteral("reserve_speed_capability_invalid"));
            speedKnown = maximumSpeed >= 0.0;
            if (speedKnown && policy.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble() > maximumSpeed)
                return reject(QStringLiteral("reserve_speed_exceeds_declared_capability"));
        }
    }
    QVariantList targetPath;
    double holds = 0.0;
    for (int index = 0; index < originalRoute.size(); ++index) {
        QVariantMap normalizedPoint;
        if (!point(originalRoute.at(index), true, index, &normalizedPoint)) return reject(QStringLiteral("reserve_target_route_invalid"));
        if (ceilingKnown && normalizedPoint.value(QStringLiteral("altitude")).toDouble() > ceiling) return reject(QStringLiteral("reserve_altitude_exceeded"));
        holds += normalizedPoint.value(QStringLiteral("holdSeconds")).toDouble();
        targetPath.append(normalizedPoint);
    }
    const QByteArray routeJson = QJsonDocument(QJsonArray::fromVariantList(originalRoute)).toJson(QJsonDocument::Compact);
    if (routeJson.isEmpty() || routeJson.size() > MaximumTargetJsonBytes) return reject(QStringLiteral("reserve_target_route_size_exceeded"));
    double ingressSeconds = 0.0, ingressDistance = 0.0, taskSeconds = 0.0, taskDistance = 0.0;
    double returnSeconds = 0.0, returnDistance = 0.0;
    if (!leg(start, targetPath.first().toMap(), policy, &ingressSeconds, &ingressDistance)
        || !leg(targetPath.last().toMap(), returnTo, policy, &returnSeconds, &returnDistance)) return reject(QStringLiteral("reserve_budget_nonfinite"));
    for (int index = 1; index < targetPath.size(); ++index) {
        double seconds = 0.0, distance = 0.0;
        if (!leg(targetPath.at(index - 1).toMap(), targetPath.at(index).toMap(), policy, &seconds, &distance))
            return reject(QStringLiteral("reserve_budget_nonfinite"));
        taskSeconds += seconds; taskDistance += distance;
    }
    const double standby = policy.value(QStringLiteral("standbyDurationSeconds")).toDouble();
    const double standbyEquivalent = standby * policy.value(QStringLiteral("standbyConsumptionFactor")).toDouble();
    const double missionEquivalent = standbyEquivalent + ingressSeconds + taskSeconds + holds;
    const double outboundDistance = ingressDistance + taskDistance;
    if (!qIsFinite(missionEquivalent) || !qIsFinite(missionEquivalent + returnSeconds) || !qIsFinite(outboundDistance)
        || !qIsFinite(holds) || !qIsFinite(taskSeconds)) return reject(QStringLiteral("reserve_budget_nonfinite"));
    QVariantList outbound{start};
    for (int index = 0; index < targetPath.size(); ++index) {
        QVariantMap item = targetPath.at(index).toMap();
        item[QStringLiteral("sequence")] = index + 1;
        outbound.append(item);
    }
    QVariantMap returnStart = targetPath.last().toMap();
    returnStart[QStringLiteral("sequence")] = 0;
    returnStart[QStringLiteral("holdSeconds")] = 0.0; // Already counted once at the outbound target's final point.
    result[QStringLiteral("complete")] = true; result[QStringLiteral("reason")] = QStringLiteral("");
    result[QStringLiteral("standbyDurationSeconds")] = standby;
    result[QStringLiteral("standbyEquivalentFlightSeconds")] = standbyEquivalent;
    result[QStringLiteral("estimatedIngressSeconds")] = ingressSeconds;
    result[QStringLiteral("estimatedTaskSeconds")] = taskSeconds;
    result[QStringLiteral("targetHoldSeconds")] = holds;
    result[QStringLiteral("missionEquivalentFlightSeconds")] = missionEquivalent;
    result[QStringLiteral("estimatedReturnSeconds")] = returnSeconds;
    result[QStringLiteral("targetRouteHash")] = QString::fromLatin1(QCryptographicHash::hash(routeJson, QCryptographicHash::Sha256).toHex());
    result[QStringLiteral("outboundPath")] = outbound;
    result[QStringLiteral("returnPath")] = QVariantList{returnStart, returnTo};
    result[QStringLiteral("outboundDistanceMeters")] = outboundDistance;
    result[QStringLiteral("returnDistanceMeters")] = returnDistance;
    result[QStringLiteral("targetWaypointCount")] = targetPath.size();
    result[QStringLiteral("speedCapabilityKnown")] = speedKnown;
    result[QStringLiteral("declaredMaxSpeedMetersPerSecond")] = maximumSpeed;
    return result;
}

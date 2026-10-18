#include "FleetMissionPayloadBuilder.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetMissionDraft.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionPreviewTimeline.h"
#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetReserveDispatchPlanner.h"

#include "QGCMAVLink.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QtMath>

#include <cmath>
#include <limits>

namespace {

constexpr int MaxMembers = 32;
constexpr int MaxMemberPoints = 2048;
constexpr int MaxTotalPoints = 8192;
constexpr int MaxPointFields = 24;
constexpr int MaxMetadataFields = 64;

QVariantMap envelope(bool ok)
{
    return {{QStringLiteral("ok"), ok},
            {QStringLiteral("builderVersion"), QStringLiteral("2.1")},
            {QStringLiteral("payloads"), QVariantList()},
            {QStringLiteral("executable"), false},
            {QStringLiteral("temporalSemanticsEncoded"), false},
            {QStringLiteral("endActionEncoded"), false},
            {QStringLiteral("relativeDwellEncoded"), false},
            {QStringLiteral("manualConfirmationRequired"), true},
            {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap failure(const QString& code, const QString& detail, int vehicleId = 0,
                    const QString& route = QString(), int pointIndex = -1)
{
    QVariantMap result = envelope(false);
    result.insert(QStringLiteral("errorCode"), code);
    result.insert(QStringLiteral("errorDetail"), detail);
    if (vehicleId) result.insert(QStringLiteral("vehicleId"), vehicleId);
    if (!route.isEmpty()) result.insert(QStringLiteral("route"), route);
    if (pointIndex >= 0) result.insert(QStringLiteral("pointIndex"), pointIndex);
    return result;
}

bool number(const QVariant& value, double* output)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return false;
    }
    bool ok = false;
    const double candidate = value.toDouble(&ok);
    if (!ok || !qIsFinite(candidate)) return false;
    if (output) *output = candidate;
    return true;
}

bool boolean(const QVariantMap& map, const QString& key, bool expected)
{
    return map.value(key).type() == QVariant::Bool && map.value(key).toBool() == expected;
}

bool textEquals(const QVariantMap& map, const QString& key, const QString& expected)
{
    return map.value(key).type() == QVariant::String && map.value(key).toString() == expected;
}

bool vehicleId(const QVariant& value, int* output)
{
    double candidate = 0.0;
    if (!number(value, &candidate) || candidate < 1.0 || candidate > 255.0
        || std::floor(candidate) != candidate) return false;
    if (output) *output = static_cast<int>(candidate);
    return true;
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QString candidate = value.toString();
    if (candidate.size() != 64) return false;
    for (const QChar character : candidate) {
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    }
    return true;
}

bool references(const QVariantMap& map)
{
    // geodetic means WGS84 in the compiler contract. No local-origin or
    // relative-to-home conversion can be inferred from these declarations.
    return textEquals(map, QStringLiteral("coordinateSystem"), QStringLiteral("geodetic"))
        && textEquals(map, QStringLiteral("altitudeReference"), QStringLiteral("amsl"))
        && !map.contains(QStringLiteral("localFrameId"));
}

bool evidenceLabel(const QVariantMap& map, const QString& key)
{
    if (!map.contains(key)) return true;
    const QVariant value = map.value(key);
    if (value.type() != QVariant::String) return false;
    const QString label = value.toString();
    return label.size() <= 128 && !label.trimmed().isEmpty() && label == label.trimmed();
}

bool aliases(const QVariantMap& map, const QStringList& keys, double* output)
{
    bool found = false;
    double resolved = 0.0;
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        double candidate = 0.0;
        if (!number(map.value(key), &candidate) || (found && candidate != resolved)) return false;
        found = true;
        resolved = candidate;
    }
    if (!found) return false;
    if (output) *output = resolved;
    return true;
}

bool optionalAliases(const QVariantMap& map, const QStringList& keys,
                     double* output, bool* present)
{
    *present = false;
    for (const QString& key : keys) *present = *present || map.contains(key);
    return !*present || aliases(map, keys, output);
}

// Retain validated source requirements without claiming that waypoint-only
// transport enforces them. Null, numeric strings, conflicting aliases, and
// malformed scheduling values are errors, not absent evidence.
bool schedule(const QVariantMap& map, QVariantMap* output, bool deadlineText = false)
{
    output->clear();
    if (map.contains(QStringLiteral("schedule"))) return false;
    const QStringList numericKeys = {QStringLiteral("timeSeconds"), QStringLiteral("time"),
        QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
        QStringLiteral("etaSeconds"), QStringLiteral("startTime"), QStringLiteral("startAt"),
        QStringLiteral("endTime"), QStringLiteral("deadline"), QStringLiteral("durationSeconds"),
        QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"),
        QStringLiteral("returnDurationSeconds"), QStringLiteral("timeLimitSeconds"),
        QStringLiteral("returnReserveSeconds")};
    const QSet<QString> positiveKeys = {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
        QStringLiteral("taskDurationSeconds"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnReserveSeconds")};
    for (const QString& key : numericKeys) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        if (key == QStringLiteral("deadline") && deadlineText && value.type() == QVariant::String) {
            const QString text = value.toString();
            if (text.size() > 128 || text.isEmpty() || text != text.trimmed()
                || (!QDateTime::fromString(text, Qt::ISODateWithMs).isValid()
                    && !QDateTime::fromString(text, Qt::ISODate).isValid())) return false;
            output->insert(key, text);
            continue;
        }
        double candidate = 0.0;
        if (!number(value, &candidate) || (positiveKeys.contains(key) && candidate <= 0.0)) return false;
        output->insert(key, candidate);
    }
    for (const QString& key : {QStringLiteral("timeReference"), QStringLiteral("returnTimeReference"),
                              QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")}) {
        if (!map.contains(key)) continue;
        if (!evidenceLabel(map, key)) return false;
        output->insert(key, map.value(key));
    }
    double ignored = 0.0;
    bool present = false;
    if (!optionalAliases(map, {QStringLiteral("timeSeconds"), QStringLiteral("time"),
            QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
            QStringLiteral("etaSeconds")}, &ignored, &present)
        || !optionalAliases(map, {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
            QStringLiteral("taskDurationSeconds")}, &ignored, &present)) return false;
    double start = 0.0;
    double end = 0.0;
    bool hasStart = false;
    bool hasEnd = false;
    QVariantMap numericMap = map;
    if (deadlineText && numericMap.value(QStringLiteral("deadline")).type() == QVariant::String) {
        numericMap.remove(QStringLiteral("deadline"));
    }
    if (!optionalAliases(numericMap, {QStringLiteral("startTime"), QStringLiteral("startAt")}, &start, &hasStart)
        || !optionalAliases(numericMap, {QStringLiteral("endTime"), QStringLiteral("deadline")}, &end, &hasEnd)
        || (hasStart && hasEnd && end < start)) return false;
    if (map.contains(QStringLiteral("returnStartTime")) && map.contains(QStringLiteral("returnEndTime"))
        && map.value(QStringLiteral("returnEndTime")).toDouble() < map.value(QStringLiteral("returnStartTime")).toDouble()) return false;
    return true;
}

bool knownMemberFields(const QVariantMap& member)
{
    const QSet<QString> allowed = {QStringLiteral("memberIndex"), QStringLiteral("vehicleId"),
        QStringLiteral("role"), QStringLiteral("waypoints"), QStringLiteral("returnPath"),
        QStringLiteral("waypointCount"), QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("returnRequired"), QStringLiteral("manualConfirmationRequired"), QStringLiteral("flightCommandReleased"),
        QStringLiteral("timeSeconds"), QStringLiteral("time"), QStringLiteral("timestampSeconds"),
        QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds"),
        QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"),
        QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnReserveSeconds"),
        QStringLiteral("timeReference"), QStringLiteral("returnTimeReference"),
        QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence"),
        QStringLiteral("estimatedDurationSeconds"), QStringLiteral("estimatedIngressDurationSeconds"),
        QStringLiteral("estimatedSurveyDurationSeconds"), QStringLiteral("estimatedReturnDurationSeconds"),
        QStringLiteral("pathLengthMeters"), QStringLiteral("ingressPathLengthMeters"),
        QStringLiteral("surveyPathLengthMeters"), QStringLiteral("returnPathLengthMeters"),
        QStringLiteral("routeSource"), QStringLiteral("returnRouteSource"),
        QStringLiteral("ingressWaypoints"), QStringLiteral("surveyWaypoints"), QStringLiteral("routeSections"),
        QStringLiteral("routeSectionIndexConvention"), QStringLiteral("ingressGenerated"), QStringLiteral("returnGenerated"),
        QStringLiteral("ingressOutsideArea"), QStringLiteral("returnOutsideArea"), QStringLiteral("endpointPathsClearanceVerified"),
        QStringLiteral("launchLandingInferred"), QStringLiteral("verticalSequencing"),
        QStringLiteral("coverageLegStart"), QStringLiteral("coverageLegCount"),
        QStringLiteral("executionProfile"), QStringLiteral("nominalRouteTiming")};
    for (auto it = member.constBegin(); it != member.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return false;
    }
    return true;
}

bool unwrapRoute(const QVariant& source, QVariantList* points, QVariantMap* wrapper)
{
    QVariant raw = source;
    wrapper->clear();
    if (source.type() == QVariant::Map) {
        *wrapper = source.toMap();
        if (wrapper->size() > 16 || !wrapper->contains(QStringLiteral("waypoints"))) return false;
        raw = wrapper->value(QStringLiteral("waypoints"));
    }
    if (raw.type() != QVariant::List) return false;
    *points = raw.toList();
    return points->size() >= 2;
}

bool nominalTimingMatches(const QVariant& value, const QVariantMap& member)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap supplied = value.toMap();
    const QVariantMap derived = FleetMissionPreviewTimeline::nominalRouteTiming(member);
    if (supplied.size() != derived.size()) return false;
    for (auto it = derived.constBegin(); it != derived.constEnd(); ++it) {
        if (!supplied.contains(it.key())) return false;
        const QVariant actual = supplied.value(it.key());
        if (it.value().isNull()) {
            if (actual.isValid() && actual.userType() != QMetaType::Nullptr) return false;
        } else if (it.value().type() == QVariant::Bool || it.value().type() == QVariant::String) {
            if (actual.type() != it.value().type() || actual != it.value()) return false;
        } else {
            double numberValue = 0.0;
            if (!number(actual, &numberValue) || numberValue != it.value().toDouble()) return false;
        }
    }
    return true;
}

QString wrapperError(const QVariantMap& wrapper)
{
    if (wrapper.isEmpty()) return QString();
    QVariantMap sourceSchedule;
    if (!schedule(wrapper, &sourceSchedule)) return QStringLiteral("invalid_temporal_semantics");
    if (!references(wrapper)) return QStringLiteral("unsupported_coordinate_reference");
    const QSet<QString> allowed = {QStringLiteral("waypoints"), QStringLiteral("coordinateSystem"),
        QStringLiteral("altitudeReference"), QStringLiteral("timingEvidence"),
        QStringLiteral("timeReference"), QStringLiteral("startTime"), QStringLiteral("startAt"),
        QStringLiteral("endTime"), QStringLiteral("deadline"), QStringLiteral("durationSeconds"),
        QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds")};
    for (auto it = wrapper.constBegin(); it != wrapper.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return QStringLiteral("unsupported_route_field");
    }
    return QString();
}

struct Point {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    QVariant elapsedEstimate;
    QVariantMap sourceSchedule;
    double holdSeconds = 0.0;
    bool holdProvided = false;
    FleetMissionHeadingEncoding::Request heading;
};

QString parsePoint(const QVariant& source, double ceiling, Point* point)
{
    if (source.type() != QVariant::Map) return QStringLiteral("invalid_waypoint");
    const QVariantMap map = source.toMap();
    if (map.size() > MaxPointFields) return QStringLiteral("point_resource_limit");
    QString headingError;
    if (!FleetMissionHeadingEncoding::fromPoint(map, point->heading, headingError)) return headingError;
    if (!schedule(map, &point->sourceSchedule)) return QStringLiteral("invalid_temporal_semantics");
    const QSet<QString> allowed = {QStringLiteral("sequence"), QStringLiteral("latitude"),
        QStringLiteral("lat"), QStringLiteral("longitude"), QStringLiteral("lon"),
        QStringLiteral("lng"), QStringLiteral("altitude"), QStringLiteral("alt"),
        QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("estimatedElapsedSeconds"), QStringLiteral("timingEvidence"),
        QStringLiteral("timeReference"), QStringLiteral("timeSeconds"), QStringLiteral("time"),
        QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
        QStringLiteral("etaSeconds"), QStringLiteral("holdSeconds"), QStringLiteral("heading"), QStringLiteral("yaw")};
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return QStringLiteral("unsupported_waypoint_field");
    }
    if (!references(map)) return QStringLiteral("unsupported_coordinate_reference");
    if (!aliases(map, {QStringLiteral("latitude"), QStringLiteral("lat")}, &point->latitude)
        || !aliases(map, {QStringLiteral("longitude"), QStringLiteral("lon"), QStringLiteral("lng")}, &point->longitude)
        || !aliases(map, {QStringLiteral("altitude"), QStringLiteral("alt")}, &point->altitude)
        || std::abs(point->latitude) > 80.0 || std::abs(point->longitude) > 180.0
        || point->altitude < -1000.0 || point->altitude > 20000.0) {
        return QStringLiteral("invalid_waypoint");
    }
    if (point->altitude > ceiling) return QStringLiteral("max_altitude_exceeded");
    if (map.contains(QStringLiteral("holdSeconds"))) {
        FleetMissionDwellEncoding::Value hold;
        if (!FleetMissionDwellEncoding::encode(map.value(QStringLiteral("holdSeconds")), hold))
            return QStringLiteral("invalid_relative_dwell");
        point->holdSeconds = hold.requestedSeconds;
        point->holdProvided = true;
    }
    if (map.contains(QStringLiteral("sequence"))) {
        double sequence = 0.0;
        if (!number(map.value(QStringLiteral("sequence")), &sequence)
            || sequence < 0.0 || sequence > MaxTotalPoints || std::floor(sequence) != sequence) {
            return QStringLiteral("invalid_waypoint_sequence");
        }
    }
    if (map.contains(QStringLiteral("estimatedElapsedSeconds"))) {
        double elapsed = 0.0;
        if (!number(map.value(QStringLiteral("estimatedElapsedSeconds")), &elapsed) || elapsed < 0.0) {
            return QStringLiteral("invalid_route_estimate");
        }
        point->elapsedEstimate = elapsed;
    }
    return QString();
}

QVariantMap item(const Point& point, int sequence, MAV_CMD command = MAV_CMD_NAV_WAYPOINT)
{
    // The bundled MAVLink definition specifies NaN (not 0) for system yaw.
    // Null is the explicit JSON-safe representation; the transport adapter
    // must convert ONLY this declared system-default yaw to NaN and normalize
    // readback NaN to the same canonical null. No NaN enters the JSON hash.
    return {{QStringLiteral("sequenceNumber"), sequence},
            {QStringLiteral("command"), static_cast<int>(command)},
            {QStringLiteral("frame"), static_cast<int>(MAV_FRAME_GLOBAL)},
            {QStringLiteral("param1"), command == MAV_CMD_NAV_WAYPOINT ? point.holdSeconds : 0.0},
            {QStringLiteral("param2"), 0.0},
            {QStringLiteral("param3"), 0.0},
            {QStringLiteral("param4"), command == MAV_CMD_NAV_WAYPOINT && point.heading.provided
                ? QVariant(point.heading.uploadDegrees) : QVariant()},
            {QStringLiteral("param5"), point.latitude}, {QStringLiteral("param6"), point.longitude},
            {QStringLiteral("param7"), point.altitude}, {QStringLiteral("autoContinue"), true},
            {QStringLiteral("isCurrentItem"), false},
            {QStringLiteral("yawMode"), command == MAV_CMD_NAV_WAYPOINT && point.heading.provided
                ? QStringLiteral("absolute-degrees") : QStringLiteral("system-default")}};
}

QVariantMap speedItem(double speed, int sequence)
{
    return {{QStringLiteral("sequenceNumber"), sequence},
        {QStringLiteral("command"), static_cast<int>(MAV_CMD_DO_CHANGE_SPEED)},
        {QStringLiteral("frame"), static_cast<int>(MAV_FRAME_MISSION)},
        {QStringLiteral("param1"), 1.0}, {QStringLiteral("param2"), speed},
        {QStringLiteral("param3"), -1.0}, {QStringLiteral("param4"), 0.0},
        {QStringLiteral("param5"), 0.0}, {QStringLiteral("param6"), 0.0},
        {QStringLiteral("param7"), 0.0}, {QStringLiteral("autoContinue"), true},
        {QStringLiteral("isCurrentItem"), false}, {QStringLiteral("yawMode"), QStringLiteral("not-applicable")}};
}

QVariantMap sourceMap(int sequence, const QString& source, int pointIndex = -1)
{
    QVariantMap result{{QStringLiteral("sequenceNumber"), sequence}, {QStringLiteral("sourcePath"), source}};
    if (pointIndex >= 0) result.insert(QStringLiteral("sourcePointIndex"), pointIndex);
    return result;
}

QVariantMap semantic(const QString& name, const QString& source, int sequence)
{
    return {{QStringLiteral("semantic"), name}, {QStringLiteral("sourcePath"), source},
        {QStringLiteral("sequenceNumber"), sequence}, {QStringLiteral("encoded"), true},
        {QStringLiteral("executionCertified"), false}};
}

bool samePoint(const Point& left, const Point& right)
{
    return left.latitude == right.latitude && left.longitude == right.longitude && left.altitude == right.altitude;
}

bool sameJson(const QVariant& left, const QVariant& right)
{
    return QJsonDocument::fromVariant(left).toJson(QJsonDocument::Compact)
        == QJsonDocument::fromVariant(right).toJson(QJsonDocument::Compact);
}

// Source containers have their own bounded, whole-value validators. Never
// accept a self-reported inactive policy in place of a retained source area,
// or mix editing and remainder provenance from different planning origins.
QString boundarySourceProblem(const QVariantMap& plan, const QVariantMap& policy,
                              const QVariantList& members)
{
    for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
        if (!plan.contains(key)) continue;
        double floor = 0.0;
        if (plan.value(key).isNull() || !number(plan.value(key), &floor)
            || floor < 0.0 || floor > 9007199254740991.0)
            return QStringLiteral("planning_performance_source_invalid");
    }
    if (int(plan.contains(QStringLiteral("planningSource")))
        + int(plan.contains(QStringLiteral("remainderPlanningSource")))
        + int(plan.contains(QStringLiteral("groupRemainderPlanningSource")))
        + int(plan.contains(QStringLiteral("reserveDispatchPlanningSource"))) > 1)
        return QStringLiteral("route_boundary_source_conflict");
    if (plan.contains(QStringLiteral("planningSource"))) {
        const QVariant source = plan.value(QStringLiteral("planningSource"));
        if (source.type() != QVariant::Map || source.isNull())
            return QStringLiteral("route_boundary_source_invalid");
        const QVariantMap checked = FleetMissionDraft::validatePlanningSource(source.toMap());
        if (!boolean(checked, QStringLiteral("ok"), true)
            || !sameJson(source, checked.value(QStringLiteral("source"))))
            return QStringLiteral("route_boundary_source_invalid");
        const QVariantMap sourceTask = checked.value(QStringLiteral("task")).toMap();
        const QString strictKey = QStringLiteral("requirePerformanceEvidence");
        if (sourceTask.contains(strictKey) != plan.contains(strictKey))
            return QStringLiteral("planning_performance_source_mismatch");
        if (sourceTask.contains(strictKey)) {
            const QVariant sourcePolicy = sourceTask.value(strictKey), planPolicy = plan.value(strictKey);
            if (sourcePolicy.type() != QVariant::Bool || sourcePolicy.isNull()
                || planPolicy.type() != QVariant::Bool || planPolicy.isNull() || sourcePolicy != planPolicy)
                return QStringLiteral("planning_performance_source_mismatch");
        }
        for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
            if (sourceTask.contains(key) != plan.contains(key))
                return QStringLiteral("planning_performance_source_mismatch");
            if (!sourceTask.contains(key)) continue;
            double sourceFloor = 0.0, planFloor = 0.0;
            if (sourceTask.value(key).isNull() || plan.value(key).isNull()
                || !number(sourceTask.value(key), &sourceFloor) || !number(plan.value(key), &planFloor)
                || sourceFloor < 0.0 || sourceFloor > 9007199254740991.0
                || planFloor < 0.0 || planFloor > 9007199254740991.0 || sourceFloor != planFloor)
                return QStringLiteral("planning_performance_source_mismatch");
        }
        const QVariantMap rebuilt = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
            sourceTask.value(QStringLiteral("area")), members);
        if (!boolean(rebuilt, QStringLiteral("ok"), true)
            || !sameJson(policy, rebuilt.value(QStringLiteral("policy"))))
            return QStringLiteral("route_boundary_source_mismatch");
    }
    if (plan.contains(QStringLiteral("remainderPlanningSource"))) {
        const QVariant source = plan.value(QStringLiteral("remainderPlanningSource"));
        if (source.type() != QVariant::Map || source.isNull())
            return QStringLiteral("route_boundary_source_invalid");
        const QVariantMap checked = FleetMissionRemainderPlanner::validateProvenance(source.toMap());
        if (!boolean(checked, QStringLiteral("ok"), true)
            || !sameJson(source, checked.value(QStringLiteral("provenance")))
            || !FleetMissionRemainderPlanner::matchesPlan(source.toMap(), plan))
            return QStringLiteral("route_boundary_source_mismatch");
    }
    if (plan.contains(QStringLiteral("groupRemainderPlanningSource"))) {
        const QVariant source = plan.value(QStringLiteral("groupRemainderPlanningSource"));
        if (source.type() != QVariant::Map || source.isNull())
            return QStringLiteral("route_boundary_source_invalid");
        const QVariantMap checked = FleetMissionGroupRemainderPlanner::validateProvenance(source.toMap());
        if (!boolean(checked, QStringLiteral("ok"), true)
            || !sameJson(source, checked.value(QStringLiteral("provenance")))
            || !FleetMissionGroupRemainderPlanner::matchesPlan(source.toMap(), plan))
            return QStringLiteral("route_boundary_source_mismatch");
    }
    if (plan.contains(QStringLiteral("reserveDispatchPlanningSource"))) {
        const QVariant source = plan.value(QStringLiteral("reserveDispatchPlanningSource"));
        if (source.type() != QVariant::Map || source.isNull())
            return QStringLiteral("route_boundary_source_invalid");
        const QVariantMap checked = FleetReserveDispatchPlanner::validateProvenance(source.toMap());
        if (!boolean(checked, QStringLiteral("ok"), true)
            || !sameJson(source, checked.value(QStringLiteral("provenance")))
            || !FleetReserveDispatchPlanner::matchesPlan(source.toMap(), plan))
            return QStringLiteral("route_boundary_source_mismatch");
    }
    if (plan.contains(QStringLiteral("coveragePlan"))) {
        const QVariant source = plan.value(QStringLiteral("coveragePlan"));
        if (source.type() != QVariant::Map || source.isNull())
            return QStringLiteral("route_boundary_coverage_mismatch");
        const QVariantMap coverage = source.toMap();
        const bool anyBoundary = coverage.contains(QStringLiteral("holes"))
            || coverage.contains(QStringLiteral("holeCount"))
            || coverage.contains(QStringLiteral("boundarySetbackMeters"));
        const bool active = boolean(policy, QStringLiteral("active"), true);
        // Every generated active coverage has all three fields. Deleting the
        // complete summary must not silently turn it into a no-policy plan.
        if (active || anyBoundary) {
            double holeCount = 0.0;
            const QVariant holes = coverage.value(QStringLiteral("holes"));
            if (!active || holes.type() != QVariant::List || holes.isNull() || holes.toList().size() > 8
                || coverage.value(QStringLiteral("holeCount")).isNull()
                || !number(coverage.value(QStringLiteral("holeCount")), &holeCount)
                || holeCount != holes.toList().size()
                || !coverage.contains(QStringLiteral("boundarySetbackMeters")))
                return QStringLiteral("route_boundary_coverage_mismatch");
            QVariantMap area = policy.value(QStringLiteral("area")).toMap();
            area.insert(QStringLiteral("holes"), holes);
            area.insert(QStringLiteral("boundarySetbackMeters"), coverage.value(QStringLiteral("boundarySetbackMeters")));
            const QVariantMap rebuilt = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(area, members);
            if (!boolean(rebuilt, QStringLiteral("ok"), true)
                || !sameJson(policy, rebuilt.value(QStringLiteral("policy"))))
                return QStringLiteral("route_boundary_coverage_mismatch");
        }
    }
    return QString();
}

} // namespace

QVariantMap FleetMissionPayloadBuilder::canonicalWireItems(const QVariantList& items, bool fromReadback)
{
    if (items.isEmpty() || items.size() > MaxMemberPoints) {
        return failure(QStringLiteral("wire_resource_limit"), QStringLiteral("Wire identity requires one bounded per-vehicle item list."));
    }
    QVariantList canonical;
    bool explicitHeading = false;
    for (int index = 0; index < items.size(); ++index) {
        if (items.at(index).type() != QVariant::Map || items.at(index).toMap().size() > 16) {
            return failure(QStringLiteral("invalid_wire_item"), QStringLiteral("Wire item must be a bounded actual map."), 0, QStringLiteral("items"), index);
        }
        const QVariantMap raw = items.at(index).toMap();
        const QSet<QString> allowed = {QStringLiteral("sequenceNumber"), QStringLiteral("command"),
            QStringLiteral("frame"), QStringLiteral("autoContinue"), QStringLiteral("isCurrentItem"),
            QStringLiteral("param1"), QStringLiteral("param2"), QStringLiteral("param3"),
            QStringLiteral("param4"), QStringLiteral("param5"), QStringLiteral("param6"),
            QStringLiteral("param7"), QStringLiteral("yawMode")};
        for (auto it = raw.constBegin(); it != raw.constEnd(); ++it) {
            if (!allowed.contains(it.key())) {
                return failure(QStringLiteral("invalid_wire_item"), QStringLiteral("Unknown wire fields cannot be discarded from identity."), 0, QStringLiteral("items"), index);
            }
        }
        double sequence = 0.0;
        double command = 0.0;
        double frame = 0.0;
        if (!number(raw.value(QStringLiteral("sequenceNumber")), &sequence) || sequence != index
            || !number(raw.value(QStringLiteral("command")), &command) || std::floor(command) != command
            || !number(raw.value(QStringLiteral("frame")), &frame)
            || raw.value(QStringLiteral("autoContinue")).type() != QVariant::Bool
            || (raw.contains(QStringLiteral("isCurrentItem")) && raw.value(QStringLiteral("isCurrentItem")).type() != QVariant::Bool)) {
            return failure(QStringLiteral("invalid_wire_item"), QStringLiteral("Sequence, command, frame, or flag type is invalid."), 0, QStringLiteral("items"), index);
        }
        const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
        const bool waypoint = command == MAV_CMD_NAV_WAYPOINT;
        const bool geographic = waypoint || command == MAV_CMD_NAV_TAKEOFF
            || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
        if ((!speed && !geographic) || frame != (speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL))
            return failure(QStringLiteral("unsupported_wire_command"), QStringLiteral("Only declared ground-speed and geographic waypoint/takeoff/hold/land commands are supported."), 0, QStringLiteral("items"), index);
        QVariant canonicalYaw;
        QString headingError;
        if (!raw.contains(QStringLiteral("param4"))
            || !FleetMissionHeadingEncoding::canonicalParam4(static_cast<int>(command),
                raw.value(QStringLiteral("param4")), raw.value(QStringLiteral("yawMode")),
                fromReadback, canonicalYaw, headingError))
            return failure(QStringLiteral("invalid_wire_parameter"),
                headingError.isEmpty() ? QStringLiteral("Command yaw/reserved parameter is invalid.") : headingError,
                0, QStringLiteral("items"), index);
        explicitHeading = explicitHeading || (waypoint && canonicalYaw.isValid());
        QVariantMap normalized = {{QStringLiteral("sequenceNumber"), index},
            {QStringLiteral("command"), static_cast<int>(command)},
            {QStringLiteral("frame"), static_cast<int>(frame)},
            {QStringLiteral("autoContinue"), raw.value(QStringLiteral("autoContinue"))},
            {QStringLiteral("param4"), canonicalYaw},
            {QStringLiteral("yawMode"), raw.value(QStringLiteral("yawMode"))}};
        for (const QString& key : {QStringLiteral("param1"), QStringLiteral("param2"),
                                  QStringLiteral("param3"), QStringLiteral("param7")}) {
            double candidate = 0.0;
            if (!number(raw.value(key), &candidate) || std::abs(candidate) > std::numeric_limits<float>::max()) {
                return failure(QStringLiteral("invalid_wire_parameter"), QStringLiteral("Wire float parameters must be finite and float32-representable."), 0, QStringLiteral("items"), index);
            }
            normalized.insert(key, static_cast<double>(static_cast<float>(candidate)));
        }
        const double p1 = raw.value(QStringLiteral("param1")).toDouble();
        const double p2 = raw.value(QStringLiteral("param2")).toDouble();
        const double p3 = raw.value(QStringLiteral("param3")).toDouble();
        const double p7 = raw.value(QStringLiteral("param7")).toDouble();
        if (waypoint) {
            FleetMissionDwellEncoding::Value hold;
            if (!FleetMissionDwellEncoding::encode(raw.value(QStringLiteral("param1")), hold))
                return failure(QStringLiteral("unsupported_wire_parameter"), QStringLiteral("NAV dwell must survive the declared float32/microsecond wait encoding."), 0, QStringLiteral("items"), index);
            normalized.insert(QStringLiteral("param1"), hold.uploadSeconds);
        }
        if ((speed && (p1 != 1.0 || p2 < 0.1 || p2 > 300.0 || p3 != -1.0 || p7 != 0.0))
            || (geographic && (p2 != 0.0 || p3 != 0.0 || p7 < -1000.0 || p7 > 20000.0
                || (waypoint ? (p1 < 0.0 || p1 > 86400.0) : p1 != 0.0))))
            return failure(QStringLiteral("unsupported_wire_parameter"), QStringLiteral("Requested command parameters fall outside this exact supported encoding."), 0, QStringLiteral("items"), index);
        if (speed) {
            double p5 = 0.0, p6 = 0.0;
            if (!number(raw.value(QStringLiteral("param5")), &p5) || p5 != 0.0
                || !number(raw.value(QStringLiteral("param6")), &p6) || p6 != 0.0)
                return failure(QStringLiteral("invalid_wire_parameter"), QStringLiteral("Non-coordinate speed parameters 5/6 must be explicit numeric zero."), 0, QStringLiteral("items"), index);
            normalized.insert(QStringLiteral("param5Integer"), 0);
            normalized.insert(QStringLiteral("param6Integer"), 0);
            canonical.append(normalized);
            continue;
        }
        double latitude = 0.0;
        double longitude = 0.0;
        if (!number(raw.value(QStringLiteral("param5")), &latitude) || std::abs(latitude) > 80.0
            || !number(raw.value(QStringLiteral("param6")), &longitude) || std::abs(longitude) > 180.0) {
            return failure(QStringLiteral("invalid_wire_coordinate"), QStringLiteral("Wire coordinates must be valid bounded WGS84 degrees."), 0, QStringLiteral("items"), index);
        }
        // PlanManager::_handleMissionRequest converts the ORIGINAL upload
        // double to int32 via truncation. Rewriting raw items to E7/1e7 first
        // would risk a second rounding/truncation. Decoded readback degrees
        // originated as integer E7 and are reconstructed with nearest integer.
        const double latitudeScaled = latitude * 1e7;
        const double longitudeScaled = longitude * 1e7;
        normalized.insert(QStringLiteral("latitudeE7"), static_cast<qint32>(fromReadback ? std::round(latitudeScaled) : std::trunc(latitudeScaled)));
        normalized.insert(QStringLiteral("longitudeE7"), static_cast<qint32>(fromReadback ? std::round(longitudeScaled) : std::trunc(longitudeScaled)));
        canonical.append(normalized);
    }
    const QByteArray bytes = QJsonDocument(QJsonArray::fromVariantList(canonical)).toJson(QJsonDocument::Compact);
    return {{QStringLiteral("ok"), true}, {QStringLiteral("items"), canonical},
        {QStringLiteral("wireSchema"), FleetMissionHeadingEncoding::wireSchema(explicitHeading)},
        {QStringLiteral("contentHash"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}};
}

QVariantMap FleetMissionPayloadBuilder::build(const QVariantMap& plan)
{
    return buildImpl(plan, false);
}

QVariantMap FleetMissionPayloadBuilder::reproduceHistoricalPayloads(const QVariantMap& plan)
{
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) return build(plan);
    if (textEquals(plan, QStringLiteral("compilerVersion"), QStringLiteral("0.7.0"))
        || textEquals(plan, QStringLiteral("planVersion"), QStringLiteral("1.1")))
        return failure(QStringLiteral("route_boundary_policy_missing"),
            QStringLiteral("A current compiled plan cannot omit its complete original boundary policy, including in inert history."));
    QVariantMap result = buildImpl(plan, true);
    result.insert(QStringLiteral("builderVersion"), QStringLiteral("2.0"));
    return result;
}

QVariantMap FleetMissionPayloadBuilder::buildImpl(const QVariantMap& plan, bool legacyHistory)
{
    double schema = 0.0;
    if (plan.size() > MaxMetadataFields
        || !number(plan.value(QStringLiteral("schemaVersion")), &schema) || schema != 1.0
        || !digest(plan.value(QStringLiteral("planId")))
        || !boolean(plan, QStringLiteral("manualConfirmationRequired"), true)
        || !boolean(plan, QStringLiteral("flightCommandReleased"), false)
        || (plan.contains(QStringLiteral("feasible")) && plan.value(QStringLiteral("feasible")).type() != QVariant::Bool)
        || (plan.contains(QStringLiteral("requiresOperatorReview")) && !boolean(plan, QStringLiteral("requiresOperatorReview"), true))
        || (plan.contains(QStringLiteral("dispatch")) && !textEquals(plan, QStringLiteral("dispatch"), QStringLiteral("not-released")))) {
        return failure(QStringLiteral("invalid_plan_contract"), QStringLiteral("Schema, plan identity, or manual/non-release declarations are invalid."));
    }
    QVariantMap boundaryPolicy;
    if (!legacyHistory) {
        if (!plan.contains(QStringLiteral("routeBoundaryPolicy")))
            return failure(QStringLiteral("route_boundary_policy_missing"),
                QStringLiteral("Complete original boundary policy is required; inert historical plans must be recompiled from their full source and reviewed again."));
        const QVariant policy = plan.value(QStringLiteral("routeBoundaryPolicy"));
        if (policy.type() != QVariant::Map || policy.isNull())
            return failure(QStringLiteral("route_boundary_policy_invalid"),
                QStringLiteral("Boundary policy must be a complete actual canonical map, never inferred from an area summary."));
        const QVariantMap checked = FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(policy.toMap());
        if (!boolean(checked, QStringLiteral("ok"), true) || !sameJson(policy, checked.value(QStringLiteral("policy"))))
            return failure(QStringLiteral("route_boundary_policy_invalid"), checked.value(QStringLiteral("reason")).toString());
        boundaryPolicy = checked.value(QStringLiteral("policy")).toMap();
    }
    for (const QString& field : {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference")}) {
        if (plan.contains(field) && !textEquals(plan, field, field == QStringLiteral("coordinateSystem")
                ? QStringLiteral("geodetic") : QStringLiteral("amsl"))) {
            return failure(QStringLiteral("unsupported_coordinate_reference"), QStringLiteral("Plan declarations cannot contradict WGS84/AMSL geometry."));
        }
    }
    if (plan.contains(QStringLiteral("localFrameId"))) {
        return failure(QStringLiteral("unsupported_plan_semantics"), QStringLiteral("A local frame cannot be converted by this geometry adapter."));
    }
    QVariantMap planSchedule;
    if (!schedule(plan, &planSchedule)) {
        return failure(QStringLiteral("invalid_temporal_semantics"), QStringLiteral("Source plan schedule is malformed or conflicting."));
    }
    if (plan.contains(QStringLiteral("routeTiming"))) {
        QVariantMap routeSchedule;
        if (plan.value(QStringLiteral("routeTiming")).type() != QVariant::Map
            || plan.value(QStringLiteral("routeTiming")).toMap().size() > 16
            || !schedule(plan.value(QStringLiteral("routeTiming")).toMap(), &routeSchedule)) {
            return failure(QStringLiteral("invalid_temporal_semantics"), QStringLiteral("Global route schedule is malformed or conflicting."));
        }
        if (routeSchedule.size() != plan.value(QStringLiteral("routeTiming")).toMap().size()) {
            return failure(QStringLiteral("unsupported_temporal_semantics"), QStringLiteral("Unknown global timing fields cannot be discarded."));
        }
        planSchedule.insert(QStringLiteral("routeTiming"), routeSchedule);
    }
    if (plan.value(QStringLiteral("constraints")).type() != QVariant::Map) {
        return failure(QStringLiteral("invalid_altitude_ceiling"), QStringLiteral("An explicit AMSL altitude ceiling is required."));
    }
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    double ceiling = 0.0;
    if (constraints.size() > MaxMetadataFields
        || !number(constraints.value(QStringLiteral("maxAltitudeMeters")), &ceiling)
        || ceiling < -1000.0 || ceiling > 20000.0
        || !textEquals(constraints, QStringLiteral("maxAltitudeReference"), QStringLiteral("amsl"))) {
        return failure(QStringLiteral("invalid_altitude_ceiling"), QStringLiteral("Altitude ceiling and route heights require the same explicit AMSL datum."));
    }
    QVariantMap constraintSchedule;
    if (!schedule(constraints, &constraintSchedule, true)) {
        return failure(QStringLiteral("invalid_temporal_semantics"), QStringLiteral("Constraint schedule/budgets must be valid and are retained as unencoded requirements."));
    }
    planSchedule.insert(QStringLiteral("constraints"), constraintSchedule);
    // Constraints are retained as an explicit unencoded limitation below, not
    // an on-board deadline, speed, fence, or synchronized execution guarantee.
    if (plan.value(QStringLiteral("members")).type() != QVariant::List) {
        return failure(QStringLiteral("invalid_members"), QStringLiteral("members must be an actual list."));
    }
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.isEmpty() || members.size() > MaxMembers) {
        return failure(QStringLiteral("member_resource_limit"), QStringLiteral("One to 32 execution members are supported."));
    }
    QVariantMap defaultExecutionProfile;
    if (plan.contains(QStringLiteral("executionProfile"))) {
        if (plan.value(QStringLiteral("executionProfile")).type() != QVariant::Map)
            return failure(QStringLiteral("invalid_execution_profile"), QStringLiteral("Plan execution-profile default must be an actual normalized map."));
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(
            plan.value(QStringLiteral("executionProfile")).toMap());
        if (!boolean(normalized, QStringLiteral("ok"), true))
            return failure(QStringLiteral("invalid_execution_profile"), normalized.value(QStringLiteral("errorDetail")).toString());
        defaultExecutionProfile = normalized.value(QStringLiteral("profile")).toMap();
    }
    QSet<int> reserves;
    if (plan.contains(QStringLiteral("reserveVehicleIds"))) {
        if (plan.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List
            || plan.value(QStringLiteral("reserveVehicleIds")).toList().size() > 255) {
            return failure(QStringLiteral("invalid_reserves"), QStringLiteral("Reserve identities must be a bounded actual list."));
        }
        for (const QVariant& value : plan.value(QStringLiteral("reserveVehicleIds")).toList()) {
            int id = 0;
            if (!vehicleId(value, &id) || reserves.contains(id)) {
                return failure(QStringLiteral("invalid_reserves"), QStringLiteral("Reserve identities must be unique real numeric system IDs."));
            }
            reserves.insert(id);
        }
    }

    // Count all raw points before allocating output or parsing any geometry.
    // A malformed or oversized route never produces an accepted prefix.
    int totalPoints = 0;
    int plannedItems = 0;
    int leaders = 0;
    QSet<int> identities;
    QMap<int, QVariantMap> executionProfiles;
    for (const QVariant& value : members) {
        if (value.type() != QVariant::Map || value.toMap().size() > MaxMetadataFields) {
            return failure(QStringLiteral("invalid_member"), QStringLiteral("Each bounded member must be an actual map."));
        }
        const QVariantMap member = value.toMap();
        int id = 0;
        if (!vehicleId(member.value(QStringLiteral("vehicleId")), &id)
            || identities.contains(id) || reserves.contains(id)) {
            return failure(QStringLiteral("invalid_member_identity"), QStringLiteral("Members need unique numeric IDs, distinct from reserves."));
        }
        identities.insert(id);
        const QVariant roleValue = member.value(QStringLiteral("role"));
        const QString role = roleValue.toString();
        if (roleValue.type() != QVariant::String
            || (role != QStringLiteral("scout") && role != QStringLiteral("leader")
                && role != QStringLiteral("relay") && role != QStringLiteral("responder"))) {
            return failure(QStringLiteral("invalid_member_role"), QStringLiteral("Only canonical scout, leader, relay, or responder roles are supported."), id);
        }
        if (role == QStringLiteral("leader") && ++leaders > 1) {
            return failure(QStringLiteral("multiple_leaders"), QStringLiteral("At most one leader may be serialized."));
        }
        if (!references(member) || !boolean(member, QStringLiteral("returnRequired"), true)) {
            return failure(QStringLiteral("invalid_member_route_contract"), QStringLiteral("Member must explicitly declare WGS84/AMSL and a required return path."), id);
        }
        if (member.contains(QStringLiteral("heading")) || member.contains(QStringLiteral("yaw"))) {
            return failure(QStringLiteral("unsupported_heading"), QStringLiteral("Requested member heading cannot be dropped by the geometry adapter."), id);
        }
        if (!knownMemberFields(member)
            || (member.contains(QStringLiteral("manualConfirmationRequired")) && !boolean(member, QStringLiteral("manualConfirmationRequired"), true))
            || (member.contains(QStringLiteral("flightCommandReleased")) && !boolean(member, QStringLiteral("flightCommandReleased"), false))) {
            return failure(QStringLiteral("unsupported_member_semantics"), QStringLiteral("Unknown member requirements or contradictory release flags cannot be discarded."), id);
        }
        QVariantMap sourceSchedule;
        if (!schedule(member, &sourceSchedule)) {
            return failure(QStringLiteral("invalid_temporal_semantics"), QStringLiteral("Member schedule cannot be malformed or conflict with its aliases."), id);
        }
        int actionItems = 0;
        if (!defaultExecutionProfile.isEmpty() && !member.contains(QStringLiteral("executionProfile")))
            return failure(QStringLiteral("unresolved_execution_profile"), QStringLiteral("A plan default requires an explicit compiler-resolved profile on every member."), id);
        if (member.contains(QStringLiteral("executionProfile"))) {
            if (member.value(QStringLiteral("executionProfile")).type() != QVariant::Map)
                return failure(QStringLiteral("invalid_execution_profile"), QStringLiteral("Execution profile must be an actual map, never coerced."), id);
            const QVariantMap normalized = FleetMissionExecutionProfile::normalize(
                member.value(QStringLiteral("executionProfile")).toMap());
            if (!boolean(normalized, QStringLiteral("ok"), true))
                return failure(QStringLiteral("invalid_execution_profile"), normalized.value(QStringLiteral("errorDetail")).toString(), id);
            const QVariantMap profile = normalized.value(QStringLiteral("profile")).toMap();
            executionProfiles.insert(id, profile);
            actionItems = 1; // Explicit outbound ground-speed setpoint.
            if (profile.contains(QStringLiteral("returnSpeedMetersPerSecond"))) ++actionItems;
            if (profile.value(QStringLiteral("launchAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("takeoff")) ++actionItems;
            const QString end = profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString();
            if (end == QStringLiteral("hold") || end == QStringLiteral("land")) ++actionItems;
        }
        int memberPoints = 0;
        QVariantMap nominalMember = member;
        for (const QString& route : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            QVariantList points;
            QVariantMap wrapper;
            if (!unwrapRoute(member.value(route), &points, &wrapper)) {
                return failure(QStringLiteral("invalid_route"), QStringLiteral("Both routes must contain at least two explicit points."), id, route);
            }
            if (points.size() > MaxMemberPoints - memberPoints || points.size() > MaxTotalPoints - totalPoints) {
                return failure(QStringLiteral("point_resource_limit"), QStringLiteral("Routes exceed 2048 points per member or 8192 points for the entire batch."), id, route);
            }
            memberPoints += points.size();
            totalPoints += points.size();
            nominalMember.insert(route, points);
            const QString wrapperProblem = wrapperError(wrapper);
            if (!wrapperProblem.isEmpty()) {
                return failure(wrapperProblem, QStringLiteral("Wrapped route contains unencoded requirements or missing explicit references."), id, route);
            }
        }
        if (member.contains(QStringLiteral("nominalRouteTiming"))
            && !nominalTimingMatches(member.value(QStringLiteral("nominalRouteTiming")), nominalMember))
            return failure(QStringLiteral("nominal_route_timing_mismatch"),
                QStringLiteral("Nominal timing must match this member's complete routes and profile; it is not an executable schedule."), id);
        if (memberPoints + actionItems > MaxMemberPoints
            || memberPoints + actionItems > MaxTotalPoints - plannedItems)
            return failure(QStringLiteral("item_resource_limit"), QStringLiteral("Routes plus all speed/takeoff/terminal items exceed the complete member/batch budget."), id);
        plannedItems += memberPoints + actionItems;
    }
    if (!legacyHistory) {
        const QString sourceProblem = boundarySourceProblem(plan, boundaryPolicy, members);
        if (!sourceProblem.isEmpty())
            return failure(sourceProblem, QStringLiteral("The complete boundary or performance admission policy disagrees with its retained planning origin or coverage declaration; no summary or legacy-source downgrade is accepted."));
    }

    QVariantMap coverageRequirements = {{QStringLiteral("available"), false},
        {QStringLiteral("encoded"), false}};
    QSet<int> coverageParticipants;
    if (plan.contains(QStringLiteral("coveragePlan"))) {
        if (plan.value(QStringLiteral("coveragePlan")).type() != QVariant::Map
            || plan.value(QStringLiteral("coveragePlan")).toMap().size() > MaxMetadataFields) {
            return failure(QStringLiteral("invalid_coverage_requirements"), QStringLiteral("Coverage requirements must come from a bounded actual plan map."));
        }
        const QVariantMap coverage = plan.value(QStringLiteral("coveragePlan")).toMap();
        double cruiseSpeed = 0.0;
        double surveyAltitude = 0.0;
        double climbSpeed = 0.0;
        if (!references(coverage)
            || !number(coverage.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &cruiseSpeed)
            || cruiseSpeed < 0.1 || cruiseSpeed > 300.0
            || !number(coverage.value(QStringLiteral("altitudeMeters")), &surveyAltitude)
            || surveyAltitude < -1000.0 || surveyAltitude > 20000.0 || surveyAltitude > ceiling
            || (coverage.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                && (!number(coverage.value(QStringLiteral("climbSpeedMetersPerSecond")), &climbSpeed)
                    || climbSpeed < 0.1 || climbSpeed > 20.0))
            || coverage.value(QStringLiteral("participantVehicleIds")).type() != QVariant::List) {
            return failure(QStringLiteral("invalid_coverage_requirements"), QStringLiteral("Coverage cruise/climb speed, altitude, participant identity, and datum must remain explicit and valid."));
        }
        const QVariantList participantIds = coverage.value(QStringLiteral("participantVehicleIds")).toList();
        if (participantIds.isEmpty() || participantIds.size() > members.size()) {
            return failure(QStringLiteral("invalid_coverage_requirements"), QStringLiteral("Coverage participants must be a nonempty subset of execution members."));
        }
        for (const QVariant& participant : participantIds) {
            int id = 0;
            if (!vehicleId(participant, &id) || !identities.contains(id) || coverageParticipants.contains(id)) {
                return failure(QStringLiteral("invalid_coverage_requirements"), QStringLiteral("Coverage participants need unique real assigned vehicle identities."));
            }
            coverageParticipants.insert(id);
        }
        for (const QVariant& value : members) {
            const QVariantMap member = value.toMap();
            const int id = member.value(QStringLiteral("vehicleId")).toInt();
            if (coverageParticipants.contains(id) && member.value(QStringLiteral("role")).toString() != QStringLiteral("scout")
                && member.value(QStringLiteral("role")).toString() != QStringLiteral("leader")) {
                return failure(QStringLiteral("invalid_coverage_requirements"), QStringLiteral("Only scout/leader members may receive coverage-specific requirements."), id);
            }
        }
        coverageRequirements.insert(QStringLiteral("available"), true);
        coverageRequirements.insert(QStringLiteral("coordinateSystem"), QStringLiteral("geodetic"));
        coverageRequirements.insert(QStringLiteral("altitudeReference"), QStringLiteral("amsl"));
        coverageRequirements.insert(QStringLiteral("participantVehicleIds"), participantIds);
        coverageRequirements.insert(QStringLiteral("altitudeMeters"), surveyAltitude);
        coverageRequirements.insert(QStringLiteral("cruiseSpeedMetersPerSecond"), cruiseSpeed);
        if (coverage.contains(QStringLiteral("climbSpeedMetersPerSecond"))) {
            coverageRequirements.insert(QStringLiteral("climbSpeedMetersPerSecond"), climbSpeed);
        }
    }

    QVariantList payloads;
    QVariantList encodedMembers;
    int totalItems = 0;
    int totalCoordinateItems = 0;
    bool allEndActionsEncoded = true;
    bool anyRelativeDwellEncoded = false;
    bool anyExplicitHeading = false;
    for (const QVariant& value : members) {
        const QVariantMap member = value.toMap();
        const int id = member.value(QStringLiteral("vehicleId")).toInt();
        QVariantMap memberCoverageRequirements = coverageRequirements;
        memberCoverageRequirements.insert(QStringLiteral("appliesToMember"), coverageParticipants.contains(id));
        const QVariantMap profile = executionProfiles.value(id);
        const bool hasProfile = !profile.isEmpty();
        const QVariantMap verticalPolicy = profile.value(QStringLiteral("verticalPolicy")).toMap();
        if (hasProfile && coverageParticipants.contains(id)
            && (profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")) != coverageRequirements.value(QStringLiteral("cruiseSpeedMetersPerSecond"))
                || (coverageRequirements.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                    && verticalPolicy.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                    && verticalPolicy.value(QStringLiteral("climbSpeedMetersPerSecond")) != coverageRequirements.value(QStringLiteral("climbSpeedMetersPerSecond")))))
            return failure(QStringLiteral("execution_coverage_speed_conflict"), QStringLiteral("An explicit member execution speed conflicts with its coverage cruise/climb requirement."), id);
        memberCoverageRequirements.insert(QStringLiteral("cruiseSpeedEncoded"), hasProfile && coverageParticipants.contains(id));
        memberCoverageRequirements.insert(QStringLiteral("climbSpeedEncoded"), false);
        QVariantList items;
        QVariantList sourceItemMap;
        QVariantList encodedSemantics;
        QMap<int, QVariantMap> headingDiagnostics;
        QStringList unencodedRequirements;
        unencodedRequirements << QStringLiteral("runtime-flight-authorization")
            << QStringLiteral("airspace-terrain-and-dynamics-clearance")
            << QStringLiteral("role-and-operational-constraints")
            << QStringLiteral("authoritative-schedule-and-deadline-enforcement")
            << QStringLiteral("vehicle-parameter-profile-unverified");
        bool relativeDwellEncoded = false;
        bool endActionEncoded = false;
        bool launchActionEncoded = false;
        if (hasProfile) {
            items.append(speedItem(profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble(), items.size()));
            sourceItemMap.append(sourceMap(items.size() - 1, QStringLiteral("executionProfile.cruiseSpeedMetersPerSecond")));
            encodedSemantics.append(semantic(QStringLiteral("outbound-ground-speed-setpoint"),
                QStringLiteral("executionProfile.cruiseSpeedMetersPerSecond"), items.size() - 1));
            if (profile.value(QStringLiteral("timingPolicy")).toString() == QStringLiteral("absolute-required"))
                unencodedRequirements << QStringLiteral("absolute-time-and-synchronized-start-not-encoded");
            if (verticalPolicy.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                || verticalPolicy.contains(QStringLiteral("descentSpeedMetersPerSecond")))
                unencodedRequirements << QStringLiteral("independent-vertical-rate-not-encoded");
            if (verticalPolicy.contains(QStringLiteral("requiredProfileHash")))
                unencodedRequirements << QStringLiteral("required-parameter-profile-hash-not-verified");
        } else {
            unencodedRequirements << QStringLiteral("execution-profile-missing")
                << QStringLiteral("cruise-speed-not-declared") << QStringLiteral("launch-action-not-declared")
                << QStringLiteral("end-action-not-declared");
        }
        if (coverageParticipants.contains(id) && coverageRequirements.contains(QStringLiteral("climbSpeedMetersPerSecond"))
            && !unencodedRequirements.contains(QStringLiteral("independent-vertical-rate-not-encoded")))
            unencodedRequirements << QStringLiteral("independent-vertical-rate-not-encoded");
        QVariantList routeEstimates;
        QVariantList routeSchedules;
        QVariantMap memberSchedule;
        schedule(member, &memberSchedule);
        Point outboundEnd;
        Point returnEnd;
        int outboundCount = 0;
        int returnCount = 0;
        for (const QString& route : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            QVariantList points;
            QVariantMap wrapper;
            unwrapRoute(member.value(route), &points, &wrapper);
            const bool returning = route == QStringLiteral("returnPath");
            if (returning && hasProfile && profile.contains(QStringLiteral("returnSpeedMetersPerSecond"))) {
                items.append(speedItem(profile.value(QStringLiteral("returnSpeedMetersPerSecond")).toDouble(), items.size()));
                sourceItemMap.append(sourceMap(items.size() - 1, QStringLiteral("executionProfile.returnSpeedMetersPerSecond")));
                encodedSemantics.append(semantic(QStringLiteral("return-ground-speed-setpoint"),
                    QStringLiteral("executionProfile.returnSpeedMetersPerSecond"), items.size() - 1));
            }
            double previousEstimate = -1.0;
            double previousTime = 0.0;
            bool hasPreviousTime = false;
            QVariantList elapsedEstimates;
            QVariantList pointSchedules;
            int routeFirstSequence = -1;
            for (int index = 0; index < points.size(); ++index) {
                Point point;
                const QString pointProblem = parsePoint(points.at(index), ceiling, &point);
                if (!pointProblem.isEmpty()) {
                    return failure(pointProblem, QStringLiteral("Waypoint is invalid, exceeds the ceiling, or carries an unencoded requirement."), id, route, index);
                }
                if (returning && index == 0 && !samePoint(outboundEnd, point)) {
                    return failure(QStringLiteral("discontinuous_return"), QStringLiteral("Return must begin at the exact outbound endpoint; no connector is invented."), id, route, index);
                }
                if (!returning && index == 0 && hasProfile) {
                    const QVariantMap launch = profile.value(QStringLiteral("launchAction")).toMap();
                    if (launch.value(QStringLiteral("type")).toString() == QStringLiteral("takeoff")) {
                        Point launchPoint;
                        const QString launchProblem = parsePoint(launch.value(QStringLiteral("coordinate")), ceiling, &launchPoint);
                        if (!launchProblem.isEmpty() || !samePoint(launchPoint, point))
                            return failure(QStringLiteral("execution_launch_endpoint_mismatch"), QStringLiteral("Explicit takeoff coordinate must equal the exact first outbound point and satisfy its altitude ceiling."), id, route, index);
                        items.append(item(launchPoint, items.size(), MAV_CMD_NAV_TAKEOFF));
                        sourceItemMap.append(sourceMap(items.size() - 1, QStringLiteral("executionProfile.launchAction")));
                        encodedSemantics.append(semantic(QStringLiteral("takeoff-request"), QStringLiteral("executionProfile.launchAction"), items.size() - 1));
                        launchActionEncoded = true;
                        unencodedRequirements << QStringLiteral("takeoff-actual-path-and-MIS_TAKEOFF_ALT-ceiling-evidence-missing");
                    } else {
                        unencodedRequirements << QStringLiteral("launch-state-and-takeoff-not-encoded");
                    }
                }
                if (point.elapsedEstimate.isValid()) {
                    const double estimate = point.elapsedEstimate.toDouble();
                    if (estimate < previousEstimate) {
                        return failure(QStringLiteral("invalid_route_estimate"), QStringLiteral("Elapsed estimates must not decrease within a route."), id, route, index);
                    }
                    previousEstimate = estimate;
                }
                elapsedEstimates.append(point.elapsedEstimate);
                double time = 0.0;
                bool hasTime = false;
                optionalAliases(point.sourceSchedule, {QStringLiteral("timeSeconds"), QStringLiteral("time"),
                    QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
                    QStringLiteral("etaSeconds")}, &time, &hasTime);
                if (hasTime && hasPreviousTime && time < previousTime) {
                    return failure(QStringLiteral("invalid_temporal_semantics"), QStringLiteral("Source point times must not decrease within a route."), id, route, index);
                }
                if (hasTime) { previousTime = time; hasPreviousTime = true; }
                pointSchedules.append(point.sourceSchedule);
                items.append(item(point, items.size()));
                if (point.heading.provided) {
                    headingDiagnostics.insert(items.size() - 1, FleetMissionHeadingEncoding::diagnostics(point.heading));
                    anyExplicitHeading = true;
                    const QString remaining = QStringLiteral("waypoint-heading-attainment-and-turn-dynamics-not-certified");
                    if (!unencodedRequirements.contains(remaining)) unencodedRequirements.append(remaining);
                    const QString headingKey = points.at(index).toMap().contains(QStringLiteral("heading"))
                        ? QStringLiteral("heading") : QStringLiteral("yaw");
                    encodedSemantics.append(semantic(QStringLiteral("absolute-waypoint-heading-tick256-request"),
                        route + QStringLiteral("[%1].%2").arg(index).arg(headingKey), items.size() - 1));
                }
                if (routeFirstSequence < 0) routeFirstSequence = items.size() - 1;
                sourceItemMap.append(sourceMap(items.size() - 1, route, index));
                if (point.holdProvided) {
                    relativeDwellEncoded = true;
                    encodedSemantics.append(semantic(QStringLiteral("relative-after-arrival-dwell"),
                        route + QStringLiteral("[%1].holdSeconds").arg(index), items.size() - 1));
                }
                if (!returning) outboundEnd = point;
                else returnEnd = point;
            }
            if (returning) returnCount = points.size();
            else outboundCount = points.size();
            QVariantMap geometry = semantic(returning ? QStringLiteral("return-geodetic-amsl-route")
                : QStringLiteral("outbound-geodetic-amsl-route"), route, routeFirstSequence);
            geometry.insert(QStringLiteral("lastSequenceNumber"), items.size() - 1);
            geometry.insert(QStringLiteral("sourcePointCount"), points.size());
            encodedSemantics.append(geometry);
            routeEstimates.append(QVariantMap{{QStringLiteral("route"), route},
                {QStringLiteral("estimatedElapsedSeconds"), elapsedEstimates},
                {QStringLiteral("authoritative"), false}});
            QVariantMap wrapperSchedule;
            schedule(wrapper, &wrapperSchedule);
            routeSchedules.append(QVariantMap{{QStringLiteral("route"), route},
                {QStringLiteral("wrapper"), wrapperSchedule}, {QStringLiteral("points"), pointSchedules}});
        }
        if (hasProfile) {
            const QVariantMap end = profile.value(QStringLiteral("endAction")).toMap();
            const QString type = end.value(QStringLiteral("type")).toString();
            if (type == QStringLiteral("hold") || type == QStringLiteral("land")) {
                Point terminal;
                const QString terminalProblem = parsePoint(end.value(QStringLiteral("coordinate")), ceiling, &terminal);
                if (!terminalProblem.isEmpty())
                    return failure(terminalProblem, QStringLiteral("Terminal coordinate must be an explicit valid WGS84/AMSL point within the altitude ceiling."), id, QStringLiteral("executionProfile.endAction.coordinate"));
                if (type == QStringLiteral("hold") && !samePoint(terminal, returnEnd))
                    return failure(QStringLiteral("execution_terminal_endpoint_mismatch"), QStringLiteral("Terminal hold must exactly match the final return point in latitude, longitude and AMSL altitude."), id, QStringLiteral("executionProfile.endAction.coordinate"));
                if (type == QStringLiteral("land")) {
                    if (terminal.latitude != returnEnd.latitude || terminal.longitude != returnEnd.longitude)
                        return failure(QStringLiteral("execution_landing_endpoint_mismatch"), QStringLiteral("Explicit landing ground location must exactly match the final return approach latitude/longitude; no connector is invented."), id, QStringLiteral("executionProfile.endAction.coordinate"));
                    if (terminal.altitude >= returnEnd.altitude)
                        return failure(QStringLiteral("execution_landing_approach_required"), QStringLiteral("Explicit ground AMSL must be strictly below the final return approach altitude; the approach waypoint must not target the ground."), id, QStringLiteral("executionProfile.endAction.coordinate"));
                    if (static_cast<float>(terminal.altitude) >= static_cast<float>(returnEnd.altitude))
                        return failure(QStringLiteral("wire_landing_approach_collapsed"), QStringLiteral("Float32 encoding collapses the required ground-below-approach altitude ordering; no clamping or invented clearance is allowed."), id, QStringLiteral("executionProfile.endAction.coordinate"));
                }
                items.append(item(terminal, items.size(), type == QStringLiteral("hold") ? MAV_CMD_NAV_LOITER_UNLIM : MAV_CMD_NAV_LAND));
                sourceItemMap.append(sourceMap(items.size() - 1, QStringLiteral("executionProfile.endAction")));
                encodedSemantics.append(semantic(type == QStringLiteral("hold") ? QStringLiteral("terminal-hold-request") : QStringLiteral("terminal-land-request"),
                    QStringLiteral("executionProfile.endAction"), items.size() - 1));
                endActionEncoded = true;
                unencodedRequirements << (type == QStringLiteral("hold") ? QStringLiteral("unlimited-hold-never-proves-mission-complete")
                    : QStringLiteral("landing-ground-datum-clearance-and-land-detected-evidence-missing"));
                if (type == QStringLiteral("land"))
                    unencodedRequirements << QStringLiteral("landing-approach-and-ground-altitude-clearance-evidence-missing");
            } else {
                unencodedRequirements << QStringLiteral("rtl-action-and-current-home-not-encoded");
            }
        }
        if (items.size() > MaxMemberPoints || items.size() > MaxTotalPoints - totalItems)
            return failure(QStringLiteral("item_resource_limit"), QStringLiteral("Complete transport list exceeds the preflight item budget."), id);
        totalItems += items.size();
        allEndActionsEncoded = allEndActionsEncoded && endActionEncoded;
        anyRelativeDwellEncoded = anyRelativeDwellEncoded || relativeDwellEncoded;
        QVariantMap summary;
        for (const QString& key : {QStringLiteral("estimatedDurationSeconds"), QStringLiteral("estimatedIngressDurationSeconds"),
                                  QStringLiteral("estimatedSurveyDurationSeconds"), QStringLiteral("estimatedReturnDurationSeconds"),
                                  QStringLiteral("pathLengthMeters"), QStringLiteral("ingressPathLengthMeters"),
                                  QStringLiteral("surveyPathLengthMeters"), QStringLiteral("returnPathLengthMeters")}) {
            if (!member.contains(key)) continue;
            double estimate = 0.0;
            if (!number(member.value(key), &estimate) || estimate < 0.0) {
                return failure(QStringLiteral("invalid_route_estimate"), QStringLiteral("Member estimates must be finite actual nonnegative numbers."), id);
            }
            summary.insert(key, estimate);
        }
        const QVariantMap wire = canonicalWireItems(items);
        if (!wire.value(QStringLiteral("ok")).toBool()) {
            return failure(QStringLiteral("wire_encoding_failed"), QStringLiteral("Valid source geometry could not be represented by the wire contract."), id);
        }
        const QVariantList wireItems = wire.value(QStringLiteral("items")).toList();
        if (hasProfile && profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("land")) {
            const int landIndex = wireItems.size() - 1;
            if (landIndex < 1)
                return failure(QStringLiteral("wire_landing_approach_contract"), QStringLiteral("Landing requires an immediately preceding encoded return approach waypoint."), id, QStringLiteral("items"), landIndex);
            const QVariantMap approach = wireItems.at(landIndex - 1).toMap();
            const QVariantMap landing = wireItems.at(landIndex).toMap();
            if (approach.value(QStringLiteral("command")).toInt() != MAV_CMD_NAV_WAYPOINT
                || landing.value(QStringLiteral("command")).toInt() != MAV_CMD_NAV_LAND
                || approach.value(QStringLiteral("frame")).toInt() != MAV_FRAME_GLOBAL
                || landing.value(QStringLiteral("frame")).toInt() != MAV_FRAME_GLOBAL
                || approach.value(QStringLiteral("latitudeE7")) != landing.value(QStringLiteral("latitudeE7"))
                || approach.value(QStringLiteral("longitudeE7")) != landing.value(QStringLiteral("longitudeE7"))
                || approach.value(QStringLiteral("param7")).toDouble() <= landing.value(QStringLiteral("param7")).toDouble())
                return failure(QStringLiteral("wire_landing_approach_contract"), QStringLiteral("Encoded LAND must immediately follow the return NAV_WAYPOINT at identical E7 location and a strictly lower float32 ground altitude."), id, QStringLiteral("items"), landIndex);
        }
        QVariantList quantizationDeltas;
        QVariantList floatParameterDeltas;
        QVariantList encodedCoordinates;
        double maximumLatitudeDelta = 0.0;
        double maximumLongitudeDelta = 0.0;
        double maximumAltitudeDelta = 0.0;
        int changedPoints = 0;
        int coordinateItems = 0;
        for (int index = 0; index < items.size(); ++index) {
            const QVariantMap original = items.at(index).toMap();
            const QVariantMap encoded = wireItems.at(index).toMap();
            QVariantMap parameterDeltas;
            for (const QString& key : {QStringLiteral("param1"), QStringLiteral("param2"),
                                      QStringLiteral("param3"), QStringLiteral("param4"), QStringLiteral("param7")}) {
                if (!original.value(key).isValid()) continue; // Explicit default yaw is a sentinel, not a number.
                parameterDeltas.insert(key, encoded.value(key).toDouble() - original.value(key).toDouble());
            }
            QVariantMap parameterRow{{QStringLiteral("sequenceNumber"), index}, {QStringLiteral("deltas"), parameterDeltas}};
            if (headingDiagnostics.contains(index))
                parameterRow.insert(QStringLiteral("headingEncoding"), headingDiagnostics.value(index));
            floatParameterDeltas.append(parameterRow);
            if (original.value(QStringLiteral("frame")).toInt() == MAV_FRAME_MISSION) {
                quantizationDeltas.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
                    {QStringLiteral("coordinateItem"), false}});
                continue; // Reserved p7=0 on DO_CHANGE_SPEED is not an AMSL altitude.
            }
            const double encodedAltitude = encoded.value(QStringLiteral("param7")).toDouble();
            ++coordinateItems;
            if (encodedAltitude > ceiling) {
                return failure(QStringLiteral("wire_altitude_ceiling_exceeded"), QStringLiteral("Float32 wire altitude exceeds the declared AMSL ceiling after quantization."), id, QStringLiteral("items"), index);
            }
            const double latitudeDelta = encoded.value(QStringLiteral("latitudeE7")).toDouble() / 1e7
                - original.value(QStringLiteral("param5")).toDouble();
            const double longitudeDelta = encoded.value(QStringLiteral("longitudeE7")).toDouble() / 1e7
                - original.value(QStringLiteral("param6")).toDouble();
            const double altitudeDelta = encodedAltitude - original.value(QStringLiteral("param7")).toDouble();
            if (!legacyHistory) {
                // Check the one actual INT encoding, without replacing raw
                // upload doubles and causing a second truncation downstream.
                // Filtering only the non-coordinate speed commands retains
                // every join, launch/terminal coordinate and vertical segment.
                encodedCoordinates.append(QVariantMap{
                    {QStringLiteral("latitude"), encoded.value(QStringLiteral("latitudeE7")).toDouble() / 1e7},
                    {QStringLiteral("longitude"), encoded.value(QStringLiteral("longitudeE7")).toDouble() / 1e7},
                    {QStringLiteral("altitude"), encodedAltitude},
                    {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
                    {QStringLiteral("altitudeReference"), QStringLiteral("amsl")}});
            }
            maximumLatitudeDelta = qMax(maximumLatitudeDelta, std::abs(latitudeDelta));
            maximumLongitudeDelta = qMax(maximumLongitudeDelta, std::abs(longitudeDelta));
            maximumAltitudeDelta = qMax(maximumAltitudeDelta, std::abs(altitudeDelta));
            if (latitudeDelta != 0.0 || longitudeDelta != 0.0 || altitudeDelta != 0.0) ++changedPoints;
            quantizationDeltas.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
                {QStringLiteral("coordinateItem"), true},
                {QStringLiteral("latitudeDeltaDegrees"), latitudeDelta},
                {QStringLiteral("longitudeDeltaDegrees"), longitudeDelta},
                {QStringLiteral("altitudeDeltaMeters"), altitudeDelta}});
        }
        if (!legacyHistory) {
            if (coordinateItems != encodedCoordinates.size()
                || coordinateItems > MaxTotalPoints - totalCoordinateItems)
                return failure(QStringLiteral("wire_boundary_item_budget_mismatch"),
                    QStringLiteral("Complete batch coordinate-chain budget does not match its encoded items; no prefix is released."), id);
            totalCoordinateItems += coordinateItems;
            encodedMembers.append(QVariantMap{{QStringLiteral("vehicleId"), id},
                {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
                {QStringLiteral("waypoints"), encodedCoordinates}});
        }
        QVariantMap payload{{QStringLiteral("vehicleId"), id}, {QStringLiteral("role"), member.value(QStringLiteral("role"))},
            {QStringLiteral("items"), items},
            {QStringLiteral("canonicalWireItems"), wire.value(QStringLiteral("items"))},
            {QStringLiteral("wireSchema"), wire.value(QStringLiteral("wireSchema"))},
            {QStringLiteral("contentHash"), wire.value(QStringLiteral("contentHash"))},
            {QStringLiteral("outboundPointCount"), outboundCount}, {QStringLiteral("returnPointCount"), returnCount},
            {QStringLiteral("itemCount"), items.size()}, {QStringLiteral("routeEstimates"), routeEstimates},
            {QStringLiteral("estimateSummary"), summary},
            {QStringLiteral("sourceItemMap"), sourceItemMap},
            {QStringLiteral("executionProfileAvailable"), hasProfile},
            {QStringLiteral("executionProfile"), profile},
            {QStringLiteral("encodedSemantics"), encodedSemantics},
            {QStringLiteral("unencodedRequirements"), unencodedRequirements},
            {QStringLiteral("coverageRequirements"), memberCoverageRequirements},
            {QStringLiteral("wireQuantization"), QVariantMap{{QStringLiteral("deltas"), quantizationDeltas},
                {QStringLiteral("floatParameterDeltas"), floatParameterDeltas},
                {QStringLiteral("coordinateItemCount"), coordinateItems},
                {QStringLiteral("changedPointCount"), changedPoints},
                {QStringLiteral("maxAbsLatitudeDeltaDegrees"), maximumLatitudeDelta},
                {QStringLiteral("maxAbsLongitudeDeltaDegrees"), maximumLongitudeDelta},
                {QStringLiteral("maxAbsAltitudeDeltaMeters"), maximumAltitudeDelta},
                {QStringLiteral("exactGeometryPreserved"), changedPoints == 0},
                {QStringLiteral("clearanceCertified"), false}}},
            {QStringLiteral("sourceSchedule"), QVariantMap{{QStringLiteral("member"), memberSchedule},
                {QStringLiteral("routes"), routeSchedules}, {QStringLiteral("encoded"), false}}},
            {QStringLiteral("temporalSemanticsEncoded"), false},
            {QStringLiteral("relativeDwellEncoded"), relativeDwellEncoded},
            {QStringLiteral("cruiseSpeedEncoded"), hasProfile},
            {QStringLiteral("returnSpeedEncoded"), hasProfile && profile.contains(QStringLiteral("returnSpeedMetersPerSecond"))},
            {QStringLiteral("launchActionEncoded"), launchActionEncoded},
            {QStringLiteral("endActionEncoded"), endActionEncoded},
            {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
        if (!legacyHistory) payload.insert(QStringLiteral("routeBoundaryPolicy"), boundaryPolicy);
        payloads.append(payload);
    }
    if (totalItems != plannedItems)
        return failure(QStringLiteral("item_budget_contract_mismatch"), QStringLiteral("Preflight and complete serialization item counts disagree; no prefix is released."));
    if (!legacyHistory) {
        // A single whole-batch call shares the planner's 8192-point and
        // visibility budgets. Never reset that budget for each vehicle.
        const QVariantMap checked = FleetAreaCoveragePlanner::validateBoundaryPolicyRoutes(boundaryPolicy, encodedMembers);
        if (!boolean(checked, QStringLiteral("ok"), true))
            return failure(QStringLiteral("wire_route_boundary_conflict"), checked.value(QStringLiteral("reason")).toString());
    }
    QVariantMap result = envelope(true);
    result.insert(QStringLiteral("planId"), plan.value(QStringLiteral("planId")));
    result.insert(QStringLiteral("payloads"), payloads);
    result.insert(QStringLiteral("totalItemCount"), totalItems);
    result.insert(QStringLiteral("totalPointCount"), totalPoints);
    result.insert(QStringLiteral("endActionEncoded"), allEndActionsEncoded);
    result.insert(QStringLiteral("relativeDwellEncoded"), anyRelativeDwellEncoded);
    if (!legacyHistory) {
        result.insert(QStringLiteral("routeBoundaryPolicy"), boundaryPolicy);
        result.insert(QStringLiteral("wireBoundaryCheck"), QVariantMap{
            {QStringLiteral("active"), boundaryPolicy.value(QStringLiteral("active"))},
            {QStringLiteral("nominal2DPolicyChecked"), boundaryPolicy.value(QStringLiteral("active"))},
            {QStringLiteral("wireSchema"), FleetMissionHeadingEncoding::wireSchema(anyExplicitHeading)},
            {QStringLiteral("memberCount"), encodedMembers.size()},
            {QStringLiteral("coordinateItemCount"), totalCoordinateItems},
            {QStringLiteral("numericalErrorCertified"), false},
            {QStringLiteral("physicalClearanceVerified"), false},
            {QStringLiteral("clearanceCertified"), false},
            {QStringLiteral("flightCommandReleased"), false}});
    }
    if (!defaultExecutionProfile.isEmpty())
        result.insert(QStringLiteral("sourceExecutionProfileDefault"), defaultExecutionProfile);
    result.insert(QStringLiteral("sourceSchedule"), planSchedule);
    result.insert(QStringLiteral("sourceScheduleEncoded"), false);
    result.insert(QStringLiteral("blockingLimitations"), QStringList()
        << QStringLiteral("Only explicit supported requests are encoded; authoritative timing, vertical-parameter profiles, deadlines and synchronized start are not enforced.")
        << QStringLiteral("Takeoff/hold/land items are requests only, not evidence of actual flight paths, safe ground datum, landing completion or clearance; RTL is not encoded.")
        << QStringLiteral("Explicit waypoint holdSeconds is encoded; acceptance/pass parameters remain zero defaults and real vehicle dynamics are not certified.")
        << QStringLiteral("Role assignments and planning constraints are metadata, not onboard role logic or enforced operational limits.")
        << QStringLiteral("An upload/readback receipt proves neither executable mission semantics nor authorization to arm or start."));
    return result;
}

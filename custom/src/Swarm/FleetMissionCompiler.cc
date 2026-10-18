#include "FleetMissionCompiler.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetCapabilityEvidence.h"
#include "FleetMissionClearanceInput.h"
#include "FleetMissionClearanceReview.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionPreviewTimeline.h"
#include "FleetMissionJointSpatialReview.h"
#include "FleetMissionDraft.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetReserveDispatchPlanner.h"
#include "FleetReserveEnergyPlan.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <cmath>
#include <limits>

namespace {

QVariant firstValue(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        // Presence is evidence, even when the value is explicitly null. Do
        // not replace a malformed authoritative field with its fallback.
        if (map.contains(key)) {
            return map.value(key);
        }
    }
    return QVariant();
}

QString firstString(const QVariantMap& map, const QStringList& keys)
{
    return firstValue(map, keys).toString().trimmed();
}

QVariantList listValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) result.append(item);
        return result;
    }
    if (value.type() == QVariant::Map) {
        QVariantList result;
        result.append(value);
        return result;
    }
    return QVariantList() << value;
}

bool finiteNumber(const QVariant& value, double* result)
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
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) return false;
    if (result) *result = number;
    return true;
}

// The expected tree comes from the bounded Draft validator. Walk exactly its
// shape instead of serializing an arbitrary task subtree. Numeric JSON
// round-trips may change Int to Double; strings/bools/null are never numbers.
bool samePlanningValue(const QVariant& actual, const QVariant& expected, int depth = 0)
{
    if (depth > 16 || !actual.isValid() || actual.isNull()
        || !expected.isValid() || expected.isNull()) return false;
    switch (expected.type()) {
    case QVariant::Map: {
        if (actual.type() != QVariant::Map) return false;
        const QVariantMap left = actual.toMap(), right = expected.toMap();
        if (left.size() != right.size()) return false;
        for (auto it = right.constBegin(); it != right.constEnd(); ++it)
            if (!left.contains(it.key()) || !samePlanningValue(left.value(it.key()), it.value(), depth + 1)) return false;
        return true;
    }
    case QVariant::List: {
        if (actual.type() != QVariant::List) return false;
        const QVariantList left = actual.toList(), right = expected.toList();
        if (left.size() != right.size()) return false;
        for (int index = 0; index < right.size(); ++index)
            if (!samePlanningValue(left.at(index), right.at(index), depth + 1)) return false;
        return true;
    }
    case QVariant::String:
        return actual.type() == QVariant::String && actual.toString() == expected.toString();
    case QVariant::Bool:
        return actual.type() == QVariant::Bool && actual.toBool() == expected.toBool();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: {
        double left = 0.0, right = 0.0;
        return finiteNumber(actual, &left) && finiteNumber(expected, &right)
            && std::abs(left) <= 9007199254740992.0 && left == right;
    }
    default: return false;
    }
}

bool numericAliases(const QVariantMap& source, const QStringList& keys,
                    double* output, bool* provided)
{
    bool found = false;
    double resolved = 0.0;
    for (const QString& key : keys) {
        if (!source.contains(key)) continue;
        double candidate = 0.0;
        if (!finiteNumber(source.value(key), &candidate)
            || (found && candidate != resolved)) {
            if (provided) *provided = true;
            return false;
        }
        found = true;
        resolved = candidate;
    }
    if (provided) *provided = found;
    if (found && output) *output = resolved;
    return true;
}

bool validGeodeticBounds(const QVariantMap& map)
{
    double west = 0.0;
    double east = 0.0;
    double south = 0.0;
    double north = 0.0;
    if (!map.contains(QStringLiteral("west"))
            || !map.contains(QStringLiteral("east"))
            || !map.contains(QStringLiteral("south"))
            || !map.contains(QStringLiteral("north"))
            || !finiteNumber(map.value(QStringLiteral("west")), &west)
            || !finiteNumber(map.value(QStringLiteral("east")), &east)
            || !finiteNumber(map.value(QStringLiteral("south")), &south)
            || !finiteNumber(map.value(QStringLiteral("north")), &north)) {
        return false;
    }
    return west >= -180.0 && west <= 180.0
        && east >= -180.0 && east <= 180.0
        && south >= -90.0 && south <= 90.0
        && north >= -90.0 && north <= 90.0
        && west < east && south < north;
}

bool validLocalBounds(const QVariantMap& map)
{
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
    if (!map.contains(QStringLiteral("minX"))
            || !map.contains(QStringLiteral("maxX"))
            || !map.contains(QStringLiteral("minY"))
            || !map.contains(QStringLiteral("maxY"))
            || !finiteNumber(map.value(QStringLiteral("minX")), &minX)
            || !finiteNumber(map.value(QStringLiteral("maxX")), &maxX)
            || !finiteNumber(map.value(QStringLiteral("minY")), &minY)
            || !finiteNumber(map.value(QStringLiteral("maxY")), &maxY)) {
        return false;
    }
    return minX < maxX && minY < maxY;
}

bool validGeofenceGeometry(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return false;
    if (value.type() == QVariant::List || value.type() == QVariant::StringList) {
        const QVariantList points = listValue(value);
        if (points.size() < 3) return false;
        for (const QVariant& point : points) {
            if (point.type() == QVariant::Map) {
                const QVariantMap pointMap = point.toMap();
                const bool hasLatitude = pointMap.contains(QStringLiteral("latitude"))
                    || pointMap.contains(QStringLiteral("lat"));
                const bool hasLongitude = pointMap.contains(QStringLiteral("longitude"))
                    || pointMap.contains(QStringLiteral("lon"))
                    || pointMap.contains(QStringLiteral("lng"));
                double first = 0.0;
                double second = 0.0;
                if (hasLatitude || hasLongitude) {
                    if (!hasLatitude || !hasLongitude
                        || !finiteNumber(firstValue(pointMap,
                            QStringList() << QStringLiteral("latitude") << QStringLiteral("lat")), &first)
                        || !finiteNumber(firstValue(pointMap,
                            QStringList() << QStringLiteral("longitude") << QStringLiteral("lon")
                                           << QStringLiteral("lng")), &second)
                        || first < -90.0 || first > 90.0
                        || second < -180.0 || second > 180.0) {
                        return false;
                    }
                } else if (!finiteNumber(firstValue(pointMap,
                                  QStringList() << QStringLiteral("x") << QStringLiteral("east")), &first)
                           || !finiteNumber(firstValue(pointMap,
                                  QStringList() << QStringLiteral("y") << QStringLiteral("north")), &second)) {
                    return false;
                }
            } else if (point.type() == QVariant::List) {
                const QVariantList pair = point.toList();
                double first = 0.0;
                double second = 0.0;
                if (pair.size() < 2 || !finiteNumber(pair.at(0), &first)
                    || !finiteNumber(pair.at(1), &second)) {
                    return false;
                }
            } else {
                return false;
            }
        }
        return true;
    }
    if (value.type() != QVariant::Map) return false;
    const QVariantMap map = value.toMap();
    if (map.isEmpty()) return false;
    for (const QString& key : {QStringLiteral("vertices"), QStringLiteral("points"),
                               QStringLiteral("boundary"), QStringLiteral("coordinates"),
                               QStringLiteral("polygon")}) {
        if (map.contains(key) && listValue(map.value(key)).size() >= 3) {
            return validGeofenceGeometry(map.value(key));
        }
    }
    const QVariant bounds = map.value(QStringLiteral("bounds"));
    if (bounds.type() == QVariant::Map) {
        return validGeofenceGeometry(bounds);
    }
    // Keep bounds semantics identical to FleetAirspaceConflictChecker:
    // geodetic west/east/south/north values are range-checked and must be
    // positively ordered; local minX/maxX/minY/maxY values must be finite and
    // positively ordered.  A four-number list is intentionally rejected since
    // it has no coordinate-frame or axis-order declaration.
    if (validGeodeticBounds(map)) return true;
    if (validLocalBounds(map)) return true;
    return false;
}

// A safety-relevant constraint must be a real bool at the ingress boundary.
// QVariant::toBool() treats any non-empty string (including "false") as true,
// which could turn a malformed task into a different geofence decision.
bool strictBool(const QVariantMap& map, const QString& key, bool fallback, bool* valid)
{
    if (!map.contains(key)) {
        if (valid) *valid = true;
        return fallback;
    }
    const QVariant value = map.value(key);
    if (!value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        if (valid) *valid = false;
        return fallback;
    }
    if (valid) *valid = true;
    return value.toBool();
}

QVariant normalizedVehicleId(const QVariantMap& map)
{
    bool found = false;
    int resolved = 0;
    for (const QString& key : {QStringLiteral("vehicleId"),
                               QStringLiteral("systemId"),
                               QStringLiteral("id")}) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        // Vehicle IDs are MAVLink system IDs in this ground-station
        // contract. Do not accept numeric strings such as "01" or "1":
        // downstream Registry/orchestrator paths intentionally reject them
        // so a transport adapter cannot silently alias a textual identity.
        double floating = 0.0;
        if (!finiteNumber(value, &floating)
            || floating < 1.0 || floating > 255.0
            || std::floor(floating) != floating) {
            return QVariant();
        }
        const int candidate = static_cast<int>(floating);
        if (found && candidate != resolved) {
            return QVariant();
        }
        found = true;
        resolved = candidate;
    }
    return found ? QVariant(resolved) : QVariant();
}

QString vehicleKey(const QVariant& id)
{
    return id.isValid() ? id.toString().trimmed() : QString();
}

void appendUnique(QStringList* values, const QString& value)
{
    if (values && !value.isEmpty() && !values->contains(value)) values->append(value);
}

void appendRisk(QVariantList* risks, const QString& code, const QString& severity,
                const QString& detail)
{
    QVariantMap risk;
    risk.insert(QStringLiteral("code"), code);
    risk.insert(QStringLiteral("severity"), severity);
    risk.insert(QStringLiteral("detail"), detail);
    risks->append(risk);
}

bool supportedAssignmentRole(const QString& role)
{
    return role == QStringLiteral("leader")
        || role == QStringLiteral("scout")
        || role == QStringLiteral("relay")
        || role == QStringLiteral("responder");
}

bool planningCapabilityEvidence(const QVariantMap& vehicle, bool requireReviewed,
                                QStringList* available, bool* reviewCurrent)
{
    if (!available || !reviewCurrent) return false;
    available->clear();
    QStringList reviewed;
    *reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewed);
    if (requireReviewed) {
        if (!*reviewCurrent) return false;
        *available = reviewed;
        return true;
    }
    if (vehicle.contains(QStringLiteral("capabilities"))
        && !FleetCapabilityEvidence::canonicalCapabilities(vehicle.value(QStringLiteral("capabilities")), available)) return false;
    if (*reviewCurrent) *available += reviewed;
    const QStringList fields{QStringLiteral("canUseGnss"), QStringLiteral("canUseRtk"),
        QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"), QStringLiteral("canRelay")};
    const QStringList tokens{QStringLiteral("gnss"), QStringLiteral("rtk"), QStringLiteral("vision"),
        QStringLiteral("range"), QStringLiteral("relay")};
    for (int index = 0; index < fields.size(); ++index) {
        bool valid = true;
        const bool enabled = strictBool(vehicle, fields.at(index), false, &valid);
        if (!valid) return false;
        if (enabled) available->append(tokens.at(index));
    }
    available->removeDuplicates();
    return true; // Planning data only, not native/hardware authentication.
}

QVariantMap normalizeWaypoint(const QVariantMap& source, int sequence, bool* valid)
{
    QVariantMap waypoint;
    waypoint.insert(QStringLiteral("sequence"), sequence);
    double first = 0.0;
    double second = 0.0;
    bool latitudeProvided = false;
    bool longitudeProvided = false;
    const bool hasLatitude = numericAliases(source,
        QStringList() << QStringLiteral("latitude") << QStringLiteral("lat"),
        &first, &latitudeProvided);
    const bool hasLongitude = numericAliases(source,
        QStringList() << QStringLiteral("longitude") << QStringLiteral("lon")
                       << QStringLiteral("lng"), &second, &longitudeProvided);
    if (latitudeProvided || longitudeProvided) {
        if (!latitudeProvided || !longitudeProvided || !hasLatitude || !hasLongitude
            || first < -90.0 || first > 90.0 || second < -180.0 || second > 180.0
            || source.contains(QStringLiteral("x")) || source.contains(QStringLiteral("east"))
            || source.contains(QStringLiteral("y")) || source.contains(QStringLiteral("north"))) {
            if (valid) *valid = false;
            return waypoint;
        }
        waypoint.insert(QStringLiteral("coordinateSystem"), QStringLiteral("geodetic"));
        waypoint.insert(QStringLiteral("latitude"), first);
        waypoint.insert(QStringLiteral("longitude"), second);
    } else {
        bool xProvided = false;
        bool yProvided = false;
        const bool hasX = numericAliases(source,
            QStringList() << QStringLiteral("x") << QStringLiteral("east"), &first, &xProvided);
        const bool hasY = numericAliases(source,
            QStringList() << QStringLiteral("y") << QStringLiteral("north"), &second, &yProvided);
        if (!hasX || !hasY || !xProvided || !yProvided) {
            if (valid) *valid = false;
            return waypoint;
        }
        waypoint.insert(QStringLiteral("coordinateSystem"), QStringLiteral("local"));
        waypoint.insert(QStringLiteral("x"), first);
        waypoint.insert(QStringLiteral("y"), second);
    }

    double altitude = 0.0;
    bool altitudeProvided = false;
    if (!numericAliases(source,
        QStringList() << QStringLiteral("altitude") << QStringLiteral("alt")
                       << QStringLiteral("z") << QStringLiteral("height"),
        &altitude, &altitudeProvided)) {
        if (valid) *valid = false;
        return waypoint;
    }
    if (altitudeProvided) waypoint.insert(QStringLiteral("altitude"), altitude);
    FleetMissionHeadingEncoding::Request heading;
    QString headingError;
    if (!FleetMissionHeadingEncoding::fromPoint(source, heading, headingError)) {
        if (valid) *valid = false;
        return waypoint;
    }
    // Keep the operator's original request in the plan/source identity. The
    // explicit tick quantization belongs to encoding, not normalization.
    if (heading.provided) waypoint.insert(QStringLiteral("heading"), heading.requestedDegrees);
    if (source.contains(QStringLiteral("holdSeconds"))) {
        FleetMissionDwellEncoding::Value hold;
        if (!FleetMissionDwellEncoding::encode(source.value(QStringLiteral("holdSeconds")), hold)) {
            if (valid) *valid = false;
            return waypoint;
        }
        // Dwell is explicit, never derived from ETA or estimated duration.
        // A supplied zero remains present in the approved plan identity.
        waypoint.insert(QStringLiteral("holdSeconds"), hold.requestedSeconds);
    }

    // A generated travel-time estimate must never become authoritative
    // waypoint timing. Only the explicit, mutually consistent aliases are
    // canonicalized into timeSeconds for the airspace reviewer.
    double time = 0.0;
    bool timeProvided = false;
    if (!numericAliases(source, QStringList() << QStringLiteral("timeSeconds")
            << QStringLiteral("timestamp") << QStringLiteral("time")
            << QStringLiteral("t") << QStringLiteral("etaSeconds"), &time, &timeProvided)) {
        if (valid) *valid = false;
        return waypoint;
    }
    if (timeProvided) waypoint.insert(QStringLiteral("timeSeconds"), time);
    if (source.contains(QStringLiteral("estimatedElapsedSeconds"))) {
        double elapsed = 0.0;
        if (!finiteNumber(source.value(QStringLiteral("estimatedElapsedSeconds")), &elapsed)
            || elapsed < 0.0) {
            if (valid) *valid = false;
            return waypoint;
        }
        waypoint.insert(QStringLiteral("estimatedElapsedSeconds"), elapsed);
    }
    for (const QString& key : {QStringLiteral("altitudeReference"), QStringLiteral("localFrameId"),
                              QStringLiteral("timeReference"), QStringLiteral("timingEvidence")}) {
        if (!source.contains(key)) continue;
        const QVariant value = source.value(key);
        if (value.type() != QVariant::String || value.toString().size() > 128 || value.toString().trimmed().isEmpty()) {
            if (valid) *valid = false;
            return waypoint;
        }
        waypoint.insert(key, value.toString().trimmed());
    }
    if (source.contains(QStringLiteral("coordinateSystem"))) {
        const QVariant value = source.value(QStringLiteral("coordinateSystem"));
        const QString declared = value.toString();
        const QString resolved = waypoint.value(QStringLiteral("coordinateSystem")).toString();
        if (value.type() != QVariant::String
            || (declared != resolved
                && !(declared == QStringLiteral("localENU") && resolved == QStringLiteral("local")))) {
            if (valid) *valid = false;
            return waypoint;
        }
    }
    if (valid) *valid = true;
    return waypoint;
}

QVariantList normalizeWaypoints(const QVariant& source, QVariantList* invalidEntries)
{
    QVariantList result;
    QVariant unwrapped = source;
    QVariantMap wrapper;
    if (source.type() == QVariant::Map && source.toMap().contains(QStringLiteral("waypoints"))) {
        wrapper = source.toMap();
        unwrapped = wrapper.value(QStringLiteral("waypoints"));
    }
    if (!unwrapped.isValid() || unwrapped.isNull()) {
        if (invalidEntries) invalidEntries->append(QStringLiteral("invalid_route"));
        return result;
    }
    const QVariantList entries = listValue(unwrapped);
    int sequence = 0;
    bool routeValid = true;
    for (const QVariant& entry : entries) {
        QVariantMap sourceMap = entry.toMap();
        bool wrapperMetadataValid = true;
        for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("timeReference"),
                                  QStringLiteral("localFrameId"), QStringLiteral("altitudeReference"),
                                  QStringLiteral("coordinateSystem")}) {
            if (!wrapper.contains(key)) continue;
            const QVariant wrapperValue = wrapper.value(key);
            if (wrapperValue.type() != QVariant::String || wrapperValue.toString().size() > 128
                || wrapperValue.toString().trimmed().isEmpty()) {
                wrapperMetadataValid = false;
            } else if (sourceMap.contains(key)
                       && (sourceMap.value(key).type() != QVariant::String
                           || sourceMap.value(key).toString().size() > 128
                           || sourceMap.value(key).toString() != wrapperValue.toString())) {
                wrapperMetadataValid = false;
            } else {
                sourceMap.insert(key, wrapperValue);
            }
        }
        bool valid = false;
        const QVariantMap waypoint = normalizeWaypoint(sourceMap, sequence, &valid);
        if (valid && wrapperMetadataValid) {
            result.append(waypoint);
            ++sequence;
        } else {
            routeValid = false;
            // Diagnostics need a count, not a retained unbounded payload.
            if (invalidEntries) invalidEntries->append(QStringLiteral("invalid_waypoint"));
        }
    }
    // A malformed entry invalidates the route, not merely that entry.
    // Returning a valid prefix would misrepresent the supplied mission.
    return routeValid ? result : QVariantList();
}

// Narrow admission for the newly materialized global-route execution path.
// The older normalizer is intentionally shared, but any requirement it would
// discard must not silently become executable just because fallback expands.
bool inheritedRouteRequirementsRetained(const QVariant& source)
{
    static const QSet<QString> wrapperFields = {QStringLiteral("waypoints"),
        QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("localFrameId"),
        QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"),
        QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("timingEvidence"), QStringLiteral("timeReference")};
    static const QSet<QString> pointFields = {QStringLiteral("sequence"),
        QStringLiteral("latitude"), QStringLiteral("lat"), QStringLiteral("longitude"), QStringLiteral("lon"), QStringLiteral("lng"),
        QStringLiteral("x"), QStringLiteral("east"), QStringLiteral("y"), QStringLiteral("north"),
        QStringLiteral("altitude"), QStringLiteral("alt"), QStringLiteral("z"), QStringLiteral("height"),
        QStringLiteral("heading"), QStringLiteral("yaw"), QStringLiteral("holdSeconds"),
        QStringLiteral("timeSeconds"), QStringLiteral("timestamp"), QStringLiteral("time"), QStringLiteral("t"), QStringLiteral("etaSeconds"),
        QStringLiteral("estimatedElapsedSeconds"), QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("localFrameId"), QStringLiteral("timeReference"), QStringLiteral("timingEvidence")};
    QVariant points = source;
    if (source.type() == QVariant::Map && !source.isNull()) {
        const QVariantMap wrapper = source.toMap();
        if (!wrapper.contains(QStringLiteral("waypoints"))) return false;
        for (auto it = wrapper.constBegin(); it != wrapper.constEnd(); ++it) {
            if (!wrapperFields.contains(it.key())) return false;
            if (it.key() == QStringLiteral("waypoints") || it.key() == QStringLiteral("coordinateSystem")
                || it.key() == QStringLiteral("altitudeReference") || it.key() == QStringLiteral("localFrameId")
                || it.key() == QStringLiteral("timingEvidence") || it.key() == QStringLiteral("timeReference")) continue;
            if (it.value().isNull() || !finiteNumber(it.value(), nullptr)) return false;
        }
        points = wrapper.value(QStringLiteral("waypoints"));
    }
    if (points.type() != QVariant::List || points.isNull() || points.toList().isEmpty()
        || points.toList().size() > 4096) return false;
    const QVariantList rows = points.toList();
    for (int index = 0; index < rows.size(); ++index) {
        if (rows.at(index).type() != QVariant::Map || rows.at(index).isNull()) return false;
        const QVariantMap row = rows.at(index).toMap();
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (!pointFields.contains(it.key())) return false;
            if (it.key() == QStringLiteral("coordinateSystem") || it.key() == QStringLiteral("altitudeReference")
                || it.key() == QStringLiteral("localFrameId") || it.key() == QStringLiteral("timeReference")
                || it.key() == QStringLiteral("timingEvidence")) continue;
            if (it.value().isNull() || !finiteNumber(it.value(), nullptr)) return false;
        }
        if (row.contains(QStringLiteral("sequence"))) {
            double sequence = 0.0;
            if (!finiteNumber(row.value(QStringLiteral("sequence")), &sequence) || sequence != index) return false;
        }
    }
    return true;
}

bool inheritedMemberRequirementsRetained(const QVariantMap& source)
{
    static const QSet<QString> fields = {QStringLiteral("vehicleId"), QStringLiteral("systemId"), QStringLiteral("id"),
        QStringLiteral("role"), QStringLiteral("assignmentRole"), QStringLiteral("executionProfile"),
        QStringLiteral("waypoints"), QStringLiteral("route"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"),
        QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"),
        QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"),
        QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("estimatedDurationSeconds"), QStringLiteral("pathLengthMeters"),
        QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("returnPathLengthMeters"),
        QStringLiteral("minimumSpeedMps"), QStringLiteral("returnReserveSeconds"),
        QStringLiteral("timingEvidence"), QStringLiteral("timeReference"), QStringLiteral("returnTimingEvidence"),
        QStringLiteral("returnTimeReference"), QStringLiteral("routeSource"), QStringLiteral("returnRouteSource"),
        QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("localFrameId"),
        QStringLiteral("returnRequired")};
    static const QSet<QString> numbers = {QStringLiteral("startTime"), QStringLiteral("startAt"),
        QStringLiteral("endTime"), QStringLiteral("deadline"), QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"),
        QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("estimatedDurationSeconds"), QStringLiteral("pathLengthMeters"),
        QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("returnPathLengthMeters"),
        QStringLiteral("minimumSpeedMps"), QStringLiteral("returnReserveSeconds")};
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        if (!fields.contains(it.key())) return false;
        if (numbers.contains(it.key()) && (it.value().isNull() || !finiteNumber(it.value(), nullptr))) return false;
        if (it.key() == QStringLiteral("returnRequired")
            && (it.value().type() != QVariant::Bool || it.value().isNull())) return false;
    }
    return true;
}

const QStringList& inheritedTaskFields()
{
    // Keep only existing downstream schedule/reference semantics. This is
    // not a new open-ended task-to-plan metadata passthrough.
    static const QStringList fields = {QStringLiteral("timeSeconds"), QStringLiteral("time"),
        QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds"),
        QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"),
        QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnReserveSeconds"),
        QStringLiteral("timeReference"), QStringLiteral("returnTimeReference"), QStringLiteral("timingEvidence"),
        QStringLiteral("returnTimingEvidence"), QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("localFrameId")};
    return fields;
}

bool inheritedTaskScheduleValid(const QVariantMap& task)
{
    const QStringList& fields = inheritedTaskFields();
    const QSet<QString> positive = {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
        QStringLiteral("taskDurationSeconds"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnReserveSeconds")};
    for (int index = 0; index < 22; ++index) {
        const QString& key = fields.at(index);
        if (!task.contains(key)) continue;
        const QVariant value = task.value(key);
        if (index < 18) {
            double number = 0.0;
            if (value.isNull() || !finiteNumber(value, &number)
                || (positive.contains(key) && number <= 0.0)) return false;
        } else if (value.type() != QVariant::String || value.isNull()
            || value.toString().size() > 128 || value.toString().trimmed().isEmpty()
            || value.toString() != value.toString().trimmed()) return false;
    }
    double start = 0.0, end = 0.0, ignored = 0.0;
    bool hasStart = false, hasEnd = false;
    if (!numericAliases(task, {QStringLiteral("timeSeconds"), QStringLiteral("time"),
            QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
            QStringLiteral("etaSeconds")}, &ignored, nullptr)
        || !numericAliases(task, {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
            QStringLiteral("taskDurationSeconds")}, &ignored, nullptr)
        || !numericAliases(task, {QStringLiteral("startTime"), QStringLiteral("startAt")}, &start, &hasStart)
        || !numericAliases(task, {QStringLiteral("endTime"), QStringLiteral("deadline")}, &end, &hasEnd)
        || (hasStart && hasEnd && end < start)) return false;
    return !task.contains(QStringLiteral("returnStartTime")) || !task.contains(QStringLiteral("returnEndTime"))
        || task.value(QStringLiteral("returnEndTime")).toDouble() >= task.value(QStringLiteral("returnStartTime")).toDouble();
}

QVariantMap routeScheduleMetadata(const QVariant& route, bool* valid)
{
    QVariantMap result;
    if (valid) *valid = true;
    if (route.type() != QVariant::Map || !route.toMap().contains(QStringLiteral("waypoints"))) {
        return result;
    }
    const QVariantMap wrapper = route.toMap();
    bool accepted = true;
    double start = 0.0;
    double end = 0.0;
    double duration = 0.0;
    bool hasStart = false;
    bool hasEnd = false;
    bool hasDuration = false;
    accepted = numericAliases(wrapper,
        QStringList() << QStringLiteral("startTime") << QStringLiteral("startAt"), &start, &hasStart) && accepted;
    accepted = numericAliases(wrapper,
        QStringList() << QStringLiteral("endTime") << QStringLiteral("deadline"), &end, &hasEnd) && accepted;
    accepted = numericAliases(wrapper,
        QStringList() << QStringLiteral("durationSeconds") << QStringLiteral("missionDurationSeconds")
                       << QStringLiteral("taskDurationSeconds"), &duration, &hasDuration) && accepted;
    if ((hasStart && hasEnd && end < start) || (hasDuration && duration <= 0.0)) accepted = false;
    if (hasStart) result.insert(QStringLiteral("startTime"), start);
    if (hasEnd) result.insert(QStringLiteral("endTime"), end);
    if (hasDuration) result.insert(QStringLiteral("durationSeconds"), duration);
    for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("timeReference")}) {
        if (!wrapper.contains(key)) continue;
        const QVariant value = wrapper.value(key);
        if (value.type() != QVariant::String || value.toString().trimmed().isEmpty()) accepted = false;
        result.insert(key, value.toString().trimmed());
    }
    if (valid) *valid = accepted;
    return accepted ? result : QVariantMap();
}

QVariantList normalizeRouteAliases(const QVariantMap& source, const QStringList& keys,
                                  bool* provided, bool* valid, QVariantMap* routeMetadata = nullptr)
{
    QVariantList resolved;
    QVariantMap resolvedMetadata;
    bool found = false;
    bool accepted = true;
    for (const QString& key : keys) {
        if (!source.contains(key)) continue;
        QVariantList invalid;
        const QVariantList candidate = normalizeWaypoints(source.value(key), &invalid);
        bool metadataValid = true;
        const QVariantMap candidateMetadata = routeScheduleMetadata(source.value(key), &metadataValid);
        if (!invalid.isEmpty() || candidate.isEmpty() || !metadataValid
            || (found && (candidate != resolved || candidateMetadata != resolvedMetadata))) {
            accepted = false;
        }
        if (!found) {
            resolved = candidate;
            resolvedMetadata = candidateMetadata;
        }
        found = true;
    }
    if (provided) *provided = found;
    if (valid) *valid = accepted;
    if (routeMetadata) *routeMetadata = accepted ? resolvedMetadata : QVariantMap();
    return accepted ? resolved : QVariantList();
}

QVariantMap areaSummary(const QVariant& area, bool* present)
{
    QVariantMap summary;
    const QVariantMap map = area.toMap();
    if (area.type() == QVariant::Map && !map.isEmpty()) {
        if (present) *present = true;
        // Validate the original geometry, including direct bounds and
        // polygon aliases, instead of requiring a particular summary form.
        summary.insert(QStringLiteral("boundaryValid"), validGeofenceGeometry(area));
        const QString shape = firstString(map,
            QStringList() << QStringLiteral("shape") << QStringLiteral("type")
                           << QStringLiteral("kind"));
        if (!shape.isEmpty()) summary.insert(QStringLiteral("shape"), shape);
        const QString name = firstString(map,
            QStringList() << QStringLiteral("name") << QStringLiteral("label"));
        if (!name.isEmpty()) summary.insert(QStringLiteral("name"), name);
        const QVariant vertices = firstValue(map,
            QStringList() << QStringLiteral("vertices") << QStringLiteral("points")
                           << QStringLiteral("boundary") << QStringLiteral("coordinates")
                           << QStringLiteral("polygon"));
        if (vertices.isValid()) {
            summary.insert(QStringLiteral("vertexCount"), listValue(vertices).size());
            summary.insert(QStringLiteral("boundaryValid"), validGeofenceGeometry(vertices));
        }
        const QVariant bounds = firstValue(map, QStringList() << QStringLiteral("bounds"));
        if (bounds.isValid()) summary.insert(QStringLiteral("bounds"), bounds);
        return summary;
    }
    if (area.type() == QVariant::List && !area.toList().isEmpty()) {
        if (present) *present = true;
        summary.insert(QStringLiteral("shape"), QStringLiteral("polygon"));
        summary.insert(QStringLiteral("vertexCount"), area.toList().size());
        summary.insert(QStringLiteral("boundaryValid"), validGeofenceGeometry(area));
        return summary;
    }
    const QString label = area.toString().trimmed();
    if (!label.isEmpty()) {
        if (present) *present = true;
        summary.insert(QStringLiteral("label"), label);
    }
    return summary;
}

QVariantList idList(const QVariant& value, int* invalidEntries = nullptr)
{
    QVariantList result;
    if (invalidEntries) *invalidEntries = 0;
    for (const QVariant& item : listValue(value)) {
        const QVariantMap map = item.toMap();
        QVariant id;
        if (map.isEmpty()) {
            QVariantMap scalar;
            scalar.insert(QStringLiteral("id"), item);
            id = normalizedVehicleId(scalar);
        } else {
            id = normalizedVehicleId(map);
        }
        if (id.isValid()) {
            result.append(id);
        } else if (invalidEntries) {
            ++(*invalidEntries);
        }
    }
    return result;
}

bool sameCanonicalGeodeticPoint(const QVariant& routeValue, const QVariantMap& required,
                               bool compareAltitude = true)
{
    if (routeValue.type() != QVariant::Map) return false;
    const QVariantMap point = routeValue.toMap();
    if (point.value(QStringLiteral("coordinateSystem")).type() != QVariant::String
        || point.value(QStringLiteral("coordinateSystem")).toString() != QStringLiteral("geodetic")
        || point.value(QStringLiteral("altitudeReference")).type() != QVariant::String
        || point.value(QStringLiteral("altitudeReference")).toString() != QStringLiteral("amsl")) return false;
    for (const QString& key : {QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")}) {
        double value = 0.0;
        if (!finiteNumber(point.value(key), &value)
            || ((compareAltitude || key != QStringLiteral("altitude")) && value != required.value(key).toDouble())) return false;
    }
    return true;
}

}

FleetMissionCompiler::FleetMissionCompiler(QObject* parent)
    : QObject(parent)
{
}

QVariantMap FleetMissionCompiler::compile(const QVariantMap& task,
                                          const QVariantList& fleet)
{
    const QVariantMap plan = compileCancelable(task, fleet, {});
    _lastPlan = plan;
    emit planChanged();
    return plan;
}

QVariantMap FleetMissionCompiler::compileCancelable(const QVariantMap& task,
    const QVariantList& fleet, const FleetMissionClearanceGeometry::CancellationToken& cancellation)
{
    const auto cancelled = [&cancellation]() {
        return cancellation && cancellation->load(std::memory_order_acquire);
    };
    const auto cancelledResult = []() {
        return QVariantMap{{QStringLiteral("cancelled"), true},
            {QStringLiteral("reason"), QStringLiteral("mission-compile-cancelled")}};
    };
    if (cancelled()) return cancelledResult();
    QVariantList risks;
    QStringList unmet;
    const int sourceKinds = int(task.contains(QStringLiteral("planningSource")))
        + int(task.contains(QStringLiteral("remainderPlanningSource")))
        + int(task.contains(QStringLiteral("groupRemainderPlanningSource")))
        + int(task.contains(QStringLiteral("reserveDispatchPlanningSource")));
    if (sourceKinds > 1) {
        appendUnique(&unmet, QStringLiteral("conflicting_planning_sources"));
        appendRisk(&risks, QStringLiteral("conflicting_planning_sources"), QStringLiteral("blocking"),
            QStringLiteral("Area, single-member remainder, group remainder and reserve-dispatch sources are mutually exclusive; no source may be discarded to select another scope."));
    }
    QVariantMap remainderPlanningSource;
    if (task.contains(QStringLiteral("remainderPlanningSource"))) {
        const QVariant source = task.value(QStringLiteral("remainderPlanningSource"));
        const QVariantMap checked = source.type() == QVariant::Map && !source.isNull()
            ? FleetMissionRemainderPlanner::validateProvenance(source.toMap()) : QVariantMap();
        if (sourceKinds == 1
            && checked.value(QStringLiteral("ok")).type() == QVariant::Bool
            && checked.value(QStringLiteral("ok")).toBool()
            && samePlanningValue(source, checked.value(QStringLiteral("provenance")))
            && FleetMissionRemainderPlanner::matchesTask(checked.value(QStringLiteral("provenance")).toMap(), task)) {
            remainderPlanningSource = checked.value(QStringLiteral("provenance")).toMap();
        } else {
            appendUnique(&unmet, QStringLiteral("invalid_remainder_planning_source"));
            appendRisk(&risks, QStringLiteral("invalid_remainder_planning_source"), QStringLiteral("blocking"),
                QStringLiteral("A remaining-route proposal must retain its complete derived task policies and canonical historical provenance; area-editor sources cannot be combined with this independent proposal."));
        }
    }
    QVariantMap groupRemainderPlanningSource;
    if (task.contains(QStringLiteral("groupRemainderPlanningSource"))) {
        const QVariant source = task.value(QStringLiteral("groupRemainderPlanningSource"));
        const QVariantMap checked = source.type() == QVariant::Map && !source.isNull()
            ? FleetMissionGroupRemainderPlanner::validateProvenance(source.toMap()) : QVariantMap();
        if (sourceKinds == 1 && checked.value(QStringLiteral("ok")).type() == QVariant::Bool
            && checked.value(QStringLiteral("ok")).toBool()
            && samePlanningValue(source, checked.value(QStringLiteral("provenance")))
            && FleetMissionGroupRemainderPlanner::matchesTask(checked.value(QStringLiteral("provenance")).toMap(), task)) {
            groupRemainderPlanningSource = checked.value(QStringLiteral("provenance")).toMap();
        } else {
            appendUnique(&unmet, QStringLiteral("invalid_group_remainder_planning_source"));
            appendRisk(&risks, QStringLiteral("invalid_group_remainder_planning_source"), QStringLiteral("blocking"),
                QStringLiteral("A group remainder proposal must retain its complete confirmed member mapping, policies and canonical group provenance; no single-member or area source can substitute for it."));
        }
    }
    QVariantMap reserveDispatchPlanningSource;
    if (task.contains(QStringLiteral("reserveDispatchPlanningSource"))) {
        const QVariant source = task.value(QStringLiteral("reserveDispatchPlanningSource"));
        const QVariantMap checked = source.type() == QVariant::Map && !source.isNull()
            ? FleetReserveDispatchPlanner::validateProvenance(source.toMap()) : QVariantMap();
        const QVariant accepted = checked.value(QStringLiteral("ok"));
        const QVariant canonical = checked.value(QStringLiteral("provenance"));
        if (sourceKinds == 1 && accepted.type() == QVariant::Bool && !accepted.isNull() && accepted.toBool()
            && canonical.type() == QVariant::Map && !canonical.isNull()
            && samePlanningValue(source, canonical)
            && FleetReserveDispatchPlanner::matchesTask(canonical.toMap(), task)) {
            reserveDispatchPlanningSource = canonical.toMap();
        } else {
            appendUnique(&unmet, QStringLiteral("invalid_reserve_dispatch_planning_source"));
            appendRisk(&risks, QStringLiteral("invalid_reserve_dispatch_planning_source"), QStringLiteral("blocking"),
                QStringLiteral("A reserve-dispatch proposal must retain its complete confirmed new task, route, profile, safety and performance policies and canonical source; no area or remainder source can substitute for it."));
        }
    }
    if (cancelled()) return cancelledResult();
    QVariantMap planningSource;
    if (task.contains(QStringLiteral("planningSource"))) {
        const QVariant sourceValue = task.value(QStringLiteral("planningSource"));
        const QVariantMap checked = sourceValue.type() == QVariant::Map && !sourceValue.isNull()
            ? FleetMissionDraft::validatePlanningSource(sourceValue.toMap()) : QVariantMap();
        bool matches = sourceKinds == 1 && checked.value(QStringLiteral("ok")).type() == QVariant::Bool
            && checked.value(QStringLiteral("ok")).toBool();
        const QVariantMap originalTask = checked.value(QStringLiteral("task")).toMap();
        const int expectedFields = 8 + int(originalTask.contains(QStringLiteral("reservePlans")))
            + int(originalTask.contains(QStringLiteral("requirePerformanceEvidence")))
            + int(originalTask.contains(QStringLiteral("minimumSpeedMps")))
            + int(originalTask.contains(QStringLiteral("minimumEnduranceSeconds")));
        matches = matches && originalTask.size() == expectedFields;
        // This source format represents the area editor only. Extra mission
        // policies cannot silently disappear on repair. Ordinary tasks with
        // no planningSource retain the broader compiler contract unchanged.
        QSet<QString> allowed = QSet<QString>() << QStringLiteral("planningSource")
            << QStringLiteral("fleet") << QStringLiteral("intent") << QStringLiteral("taskId");
        for (auto it = originalTask.constBegin(); it != originalTask.constEnd(); ++it) {
            if (cancelled()) return cancelledResult();
            allowed.insert(it.key());
            if (!task.contains(it.key()) || !samePlanningValue(task.value(it.key()), it.value())) matches = false;
        }
        for (auto it = task.constBegin(); it != task.constEnd(); ++it) {
            if (cancelled()) return cancelledResult();
            if (!allowed.contains(it.key())) matches = false;
        }
        if (matches) {
            planningSource = checked.value(QStringLiteral("source")).toMap();
        } else {
            appendUnique(&unmet, QStringLiteral("invalid_planning_source"));
            appendRisk(&risks, QStringLiteral("invalid_planning_source"), QStringLiteral("blocking"),
                QStringLiteral("The complete area-editor source must reproduce its eight legacy planning fields and, only when retained by that source, reservePlans, the explicit performance-evidence policy and speed/endurance floors; unsupported, missing or mismatched input cannot become a repairable plan."));
        }
    }
    if (cancelled()) return cancelledResult();
    const bool taskProfileProvided = task.contains(QStringLiteral("executionProfile"));
    QVariantMap taskExecutionProfile;
    if (taskProfileProvided) {
        const QVariant value = task.value(QStringLiteral("executionProfile"));
        QVariantMap normalized;
        if (value.type() == QVariant::Map && !value.isNull()) {
            normalized = FleetMissionExecutionProfile::normalize(value.toMap());
        }
        const bool accepted = normalized.value(QStringLiteral("ok")).type() == QVariant::Bool
            && normalized.value(QStringLiteral("ok")).toBool();
        if (accepted) {
            taskExecutionProfile = normalized.value(QStringLiteral("profile")).toMap();
        } else {
            appendUnique(&unmet, QStringLiteral("invalid_task_execution_profile"));
            appendRisk(&risks, QStringLiteral("invalid_task_execution_profile"), QStringLiteral("blocking"),
                       QStringLiteral("Task executionProfile must satisfy the complete explicit contract; no default or partial profile is invented."));
        }
    }
    if (cancelled()) return cancelledResult();
    bool areaPresent = false;
    bool targetPresent = false;

    const QVariantMap area = areaSummary(task.value(QStringLiteral("area")), &areaPresent);
    const QVariant target = task.value(QStringLiteral("target"));
    if (target.isValid() && !target.isNull()) {
        if (target.type() == QVariant::Map) {
            targetPresent = !target.toMap().isEmpty();
        } else if (target.type() == QVariant::List
                   || target.type() == QVariant::StringList) {
            targetPresent = !listValue(target).isEmpty();
        } else {
            targetPresent = !target.toString().trimmed().isEmpty();
        }
    }
    if (!areaPresent && !targetPresent) {
        appendUnique(&unmet, QStringLiteral("area_or_target_missing"));
    }

    bool globalRouteProvided = false;
    bool globalRouteValid = true;
    QVariantMap globalRouteMetadata;
    const QVariantList waypoints = normalizeRouteAliases(task,
        QStringList() << QStringLiteral("waypoints") << QStringLiteral("route"),
        &globalRouteProvided, &globalRouteValid, &globalRouteMetadata);
    if (cancelled()) return cancelledResult();
    if (!globalRouteValid) {
        appendUnique(&unmet, QStringLiteral("invalid_waypoint"));
        appendRisk(&risks, QStringLiteral("invalid_waypoint"), QStringLiteral("blocking"),
                   QStringLiteral("A supplied route is empty, malformed, or contains conflicting coordinate/timing aliases."));
    }
    const bool coverageRequested = task.contains(QStringLiteral("coverage"));
    const QVariant coverageValue = task.value(QStringLiteral("coverage"));
    QVariantMap coveragePlan;
    if (coverageRequested && (coverageValue.type() != QVariant::Map || coverageValue.isNull())) {
        appendUnique(&unmet, QStringLiteral("invalid_coverage_options"));
        appendRisk(&risks, QStringLiteral("invalid_coverage_options"), QStringLiteral("blocking"),
                   QStringLiteral("coverage must be an actual options map, not null or a converted value."));
    }
    if (coverageRequested && globalRouteProvided) {
        appendUnique(&unmet, QStringLiteral("coverage_global_route_conflict"));
        appendRisk(&risks, QStringLiteral("coverage_global_route_conflict"), QStringLiteral("blocking"),
                   QStringLiteral("Coverage generation cannot overwrite or share a supplied global route."));
    }

    const QVariant originalConstraints = task.value(QStringLiteral("constraints"));
    const bool invalidConstraints = task.contains(QStringLiteral("constraints"))
        && (originalConstraints.type() != QVariant::Map || originalConstraints.isNull());
    QVariantMap constraints = originalConstraints.toMap();
    const QStringList constraintKeys = QStringList()
        << QStringLiteral("timeLimitSeconds") << QStringLiteral("deadline")
        << QStringLiteral("startAt") << QStringLiteral("minimumSeparationMeters")
        << QStringLiteral("maxAltitudeMeters") << QStringLiteral("maxAltitudeReference")
        << QStringLiteral("geofenceRequired")
        << QStringLiteral("returnReserveSeconds");
    for (const QString& key : constraintKeys) {
        if (!constraints.contains(key) && task.contains(key)) constraints.insert(key, task.value(key));
    }
    const QVariant timeLimit = constraints.value(QStringLiteral("timeLimitSeconds"));
    const QVariant deadline = constraints.value(QStringLiteral("deadline"));
    bool deadlineUsable = false;
    bool deadlineInvalid = false;
    if (constraints.contains(QStringLiteral("deadline"))) {
        if (deadline.type() == QVariant::String) {
            const QString text = deadline.toString().trimmed();
            double numericDeadline = 0.0;
            const bool numeric = finiteNumber(deadline, &numericDeadline);
            if (numeric) {
                deadlineUsable = numericDeadline > 0.0;
            } else {
                const QDateTime parsedWithMs = QDateTime::fromString(text, Qt::ISODateWithMs);
                const QDateTime parsed = parsedWithMs.isValid()
                    ? parsedWithMs : QDateTime::fromString(text, Qt::ISODate);
                deadlineUsable = !text.isEmpty() && parsed.isValid();
            }
        } else {
            double deadlineNumber = 0.0;
            deadlineUsable = finiteNumber(deadline, &deadlineNumber) && deadlineNumber > 0.0;
        }
        if (!deadlineUsable) {
            deadlineInvalid = true;
            appendUnique(&unmet, QStringLiteral("invalid_deadline"));
            appendRisk(&risks, QStringLiteral("invalid_deadline"), QStringLiteral("blocking"),
                       QStringLiteral("Deadline must be a positive finite timestamp or a valid ISO-8601 date-time."));
        }
    }
    if (timeLimit.isValid()) {
        double seconds = 0.0;
        if (!finiteNumber(timeLimit, &seconds) || seconds <= 0.0) {
            appendUnique(&unmet, QStringLiteral("invalid_time_limit"));
        }
    } else if (!deadlineUsable && !deadlineInvalid) {
        appendUnique(&unmet, QStringLiteral("time_limit_missing"));
        appendRisk(&risks, QStringLiteral("time_limit_missing"), QStringLiteral("blocking"),
                   QStringLiteral("No positive execution time limit was supplied."));
    }
    const QVariant minSeparation = constraints.value(QStringLiteral("minimumSeparationMeters"));
    if (minSeparation.isValid()) {
        double meters = 0.0;
        if (!finiteNumber(minSeparation, &meters) || meters <= 0.0) {
            appendUnique(&unmet, QStringLiteral("invalid_minimum_separation"));
        }
    }
    bool geofenceRequiredValid = true;
    const bool geofenceRequired = strictBool(constraints,
                                             QStringLiteral("geofenceRequired"),
                                             false,
                                             &geofenceRequiredValid);
    if (!geofenceRequiredValid) {
        appendUnique(&unmet, QStringLiteral("invalid_geofence_required"));
        appendRisk(&risks, QStringLiteral("invalid_geofence_required"),
                   QStringLiteral("blocking"),
                   QStringLiteral("geofenceRequired must be a boolean value."));
    } else if (geofenceRequired) {
        bool geofenceEvidence = false;
        const QVariant boundaryValid = area.value(QStringLiteral("boundaryValid"));
        geofenceEvidence = boundaryValid.type() == QVariant::Bool
            && boundaryValid.toBool();
        if (!geofenceEvidence && area.contains(QStringLiteral("bounds"))) {
            geofenceEvidence = validGeofenceGeometry(area.value(QStringLiteral("bounds")));
        }
        if (!geofenceEvidence && task.contains(QStringLiteral("geofence"))) {
            geofenceEvidence = validGeofenceGeometry(task.value(QStringLiteral("geofence")));
        }
        if (!geofenceEvidence) {
            appendUnique(&unmet, QStringLiteral("geofence_missing"));
            appendRisk(&risks, QStringLiteral("geofence_missing"), QStringLiteral("blocking"),
                       QStringLiteral("geofenceRequired is enabled but no non-empty polygon or bounds were supplied."));
        }
    }

    const QVariant assignmentsValue = firstValue(task,
        QStringList() << QStringLiteral("assignments") << QStringLiteral("vehicleAssignments")
                       << QStringLiteral("members"));
    const QVariantList assignmentEntries = listValue(assignmentsValue);
    QVariantList members;
    bool globalRouteMaterialized = false;
    QSet<QString> assignmentIds;
    QSet<QString> fleetIds;
    QVariantMap fleetById;
    const bool fleetProvided = !fleet.isEmpty();
    bool invalidFleetEntries = false;
    bool duplicateFleetEntries = false;
    for (const QVariant& fleetEntry : fleet) {
        if (cancelled()) return cancelledResult();
        const QString id = vehicleKey(normalizedVehicleId(fleetEntry.toMap()));
        if (id.isEmpty()) {
            invalidFleetEntries = true;
        } else if (fleetIds.contains(id)) {
            duplicateFleetEntries = true;
            fleetById.remove(id); // Ambiguous IDs never select a positive record.
        } else {
            fleetIds.insert(id);
            fleetById.insert(id, fleetEntry.toMap());
        }
    }
    if (invalidFleetEntries) {
        appendUnique(&unmet, QStringLiteral("invalid_fleet_vehicle"));
        appendRisk(&risks, QStringLiteral("invalid_fleet_vehicle"), QStringLiteral("blocking"),
                   QStringLiteral("The supplied fleet snapshot contains an invalid or conflicting vehicle ID."));
    }
    if (duplicateFleetEntries) {
        appendUnique(&unmet, QStringLiteral("duplicate_fleet_vehicle"));
        appendRisk(&risks, QStringLiteral("duplicate_fleet_vehicle"), QStringLiteral("blocking"),
                   QStringLiteral("The supplied fleet snapshot contains duplicate vehicle IDs."));
    }
    bool commonReturnProvided = false;
    bool commonReturnValid = true;
    QVariantMap commonReturnMetadata;
    const QVariantList commonReturnPath = normalizeRouteAliases(task,
        QStringList() << QStringLiteral("returnPath") << QStringLiteral("returnRoute"),
        &commonReturnProvided, &commonReturnValid, &commonReturnMetadata);
    if (cancelled()) return cancelledResult();
    if (!commonReturnValid) {
        appendUnique(&unmet, QStringLiteral("invalid_task_return_path"));
        appendRisk(&risks, QStringLiteral("invalid_task_return_path"), QStringLiteral("blocking"),
                   QStringLiteral("The supplied common return route is empty, malformed, or conflicting."));
    }
    QVariantMap taskReturnPaths;
    QVariantMap taskReturnMetadata;
    if (task.contains(QStringLiteral("returnPaths"))) {
        const QVariant value = task.value(QStringLiteral("returnPaths"));
        if (value.type() != QVariant::Map || value.isNull()) {
            appendUnique(&unmet, QStringLiteral("invalid_task_return_paths"));
            appendRisk(&risks, QStringLiteral("invalid_task_return_paths"), QStringLiteral("blocking"),
                       QStringLiteral("returnPaths must be an actual map keyed by assigned vehicle IDs."));
        } else {
            const QVariantMap supplied = value.toMap();
            for (auto it = supplied.constBegin(); it != supplied.constEnd(); ++it) {
                if (cancelled()) return cancelledResult();
                QVariantList invalid;
                QVariantList normalized = normalizeWaypoints(it.value(), &invalid);
                bool metadataValid = true;
                const QVariantMap metadata = routeScheduleMetadata(it.value(), &metadataValid);
                if (!metadataValid) normalized.clear();
                // Preserve the key and even an empty result: malformed
                // per-vehicle evidence cannot fall back to a common route.
                taskReturnPaths.insert(it.key(), normalized);
                taskReturnMetadata.insert(it.key(), metadata);
                if (!invalid.isEmpty() || normalized.isEmpty() || !metadataValid) {
                    appendUnique(&unmet, QStringLiteral("invalid_task_return_paths"));
                    appendRisk(&risks, QStringLiteral("invalid_task_return_paths"), QStringLiteral("blocking"),
                               QStringLiteral("Return route for task vehicle key %1 is malformed or empty.").arg(it.key()));
                }
            }
        }
    }
    int leaderAssignmentCount = 0;
    QVariant leaderAssignmentId;
    for (int index = 0; index < assignmentEntries.size(); ++index) {
        if (cancelled()) return cancelledResult();
        const QVariantMap source = assignmentEntries.at(index).toMap();
        const QVariant id = normalizedVehicleId(source);
        const QString idKey = vehicleKey(id);
        const bool hasRole = source.contains(QStringLiteral("role"));
        const bool hasAssignmentRole = source.contains(QStringLiteral("assignmentRole"));
        const QVariant roleValue = source.value(QStringLiteral("role"));
        const QVariant assignmentRoleValue = source.value(QStringLiteral("assignmentRole"));
        const bool roleTypeValid = (!hasRole || roleValue.type() == QVariant::String)
            && (!hasAssignmentRole || assignmentRoleValue.type() == QVariant::String);
        const QString role = hasRole
            ? (roleTypeValid ? roleValue.toString().trimmed().toLower() : QString())
            : (hasAssignmentRole
               ? (roleTypeValid ? assignmentRoleValue.toString().trimmed().toLower() : QString())
               : QString());
        const bool roleAliasesAgree = !hasRole || !hasAssignmentRole
            || (roleTypeValid
                && roleValue.toString().trimmed().compare(
                    assignmentRoleValue.toString().trimmed(), Qt::CaseInsensitive) == 0);
        if (!id.isValid()) {
            appendUnique(&unmet, QStringLiteral("assignment_vehicle_missing"));
            continue;
        }
        if (assignmentIds.contains(idKey)) {
            appendUnique(&unmet, QStringLiteral("duplicate_vehicle_assignment"));
        }
        assignmentIds.insert(idKey);
        if (!roleTypeValid) {
            appendUnique(&unmet, QStringLiteral("invalid_assignment_role"));
            appendRisk(&risks, QStringLiteral("invalid_assignment_role"), QStringLiteral("blocking"),
                       QStringLiteral("Assignment role must be a supported string role."));
        } else if (!roleAliasesAgree) {
            appendUnique(&unmet, QStringLiteral("conflicting_assignment_role"));
            appendRisk(&risks, QStringLiteral("conflicting_assignment_role"), QStringLiteral("blocking"),
                       QStringLiteral("role and assignmentRole must agree when both are supplied."));
        } else if (role.isEmpty()) {
            appendUnique(&unmet, QStringLiteral("assignment_role_missing"));
        } else if (!supportedAssignmentRole(role)) {
            appendUnique(&unmet, QStringLiteral("unsupported_assignment_role"));
            appendRisk(&risks, QStringLiteral("unsupported_assignment_role"), QStringLiteral("blocking"),
                       QStringLiteral("Assignment role %1 is not supported by the compiler.").arg(role));
        }
        if (role == QStringLiteral("leader")) {
            ++leaderAssignmentCount;
            leaderAssignmentId = id;
        }
        if (fleetProvided && !fleetIds.contains(idKey)) {
            appendUnique(&unmet, QStringLiteral("assignment_vehicle_not_in_fleet"));
            appendRisk(&risks, QStringLiteral("assignment_vehicle_not_in_fleet"), QStringLiteral("blocking"),
                       QStringLiteral("Vehicle %1 is not present in the supplied fleet snapshot.").arg(idKey));
        }
        QVariantMap member;
        member.insert(QStringLiteral("memberIndex"), index);
        member.insert(QStringLiteral("vehicleId"), id);
        member.insert(QStringLiteral("role"), role.toLower());
        // Member presence means whole-profile replacement, including an
        // explicitly malformed/null replacement. Never merge or fall back.
        if (source.contains(QStringLiteral("executionProfile"))) {
            const QVariant value = source.value(QStringLiteral("executionProfile"));
            QVariantMap normalized;
            if (value.type() == QVariant::Map && !value.isNull()) {
                normalized = FleetMissionExecutionProfile::normalize(value.toMap());
            }
            const bool accepted = normalized.value(QStringLiteral("ok")).type() == QVariant::Bool
                && normalized.value(QStringLiteral("ok")).toBool();
            member.insert(QStringLiteral("executionProfile"), accepted
                ? normalized.value(QStringLiteral("profile")) : QVariant(QVariantMap()));
            if (!accepted) {
                appendUnique(&unmet, QStringLiteral("invalid_member_execution_profile"));
                appendRisk(&risks, QStringLiteral("invalid_member_execution_profile"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 supplied an invalid whole execution profile; the task default cannot replace it.").arg(idKey));
            }
        } else if (taskProfileProvided) {
            member.insert(QStringLiteral("executionProfile"), taskExecutionProfile);
        }
        bool memberRouteProvided = false;
        bool memberRouteValid = true;
        QVariantMap memberRouteMetadata;
        const QVariantList memberWaypoints = normalizeRouteAliases(source,
            QStringList() << QStringLiteral("waypoints") << QStringLiteral("route"),
            &memberRouteProvided, &memberRouteValid, &memberRouteMetadata);
        if (memberRouteProvided) {
            member.insert(QStringLiteral("waypointCount"), memberWaypoints.size());
            // Keep even the empty normalized route present: neither a
            // shared route nor the coverage generator may reinterpret
            // malformed explicit input as an absent member route.
            member.insert(QStringLiteral("waypoints"), memberWaypoints);
            if (!memberRouteValid) {
                appendUnique(&unmet, QStringLiteral("invalid_member_waypoint"));
                appendRisk(&risks, QStringLiteral("invalid_member_waypoint"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 has an empty, malformed, or conflicting member route.").arg(idKey));
            }
            for (auto it = memberRouteMetadata.constBegin(); it != memberRouteMetadata.constEnd(); ++it) {
                QStringList aliases;
                if (it.key() == QStringLiteral("startTime")) {
                    aliases << QStringLiteral("startTime") << QStringLiteral("startAt");
                } else if (it.key() == QStringLiteral("endTime")) {
                    aliases << QStringLiteral("endTime") << QStringLiteral("deadline");
                } else if (it.key() == QStringLiteral("durationSeconds")) {
                    aliases << QStringLiteral("durationSeconds") << QStringLiteral("missionDurationSeconds")
                            << QStringLiteral("taskDurationSeconds");
                } else {
                    aliases << it.key();
                }
                for (const QString& alias : aliases) {
                    if (source.contains(alias) && source.value(alias) != it.value()) {
                        appendUnique(&unmet, QStringLiteral("conflicting_member_route_schedule"));
                        appendRisk(&risks, QStringLiteral("conflicting_member_route_schedule"), QStringLiteral("blocking"),
                                   QStringLiteral("Vehicle %1 field %2 conflicts with its wrapped outbound route.").arg(idKey, alias));
                    }
                }
                member.insert(it.key(), it.value());
            }
        }
        bool returnProvided = false;
        bool returnValid = true;
        QVariantMap returnMetadata;
        QVariantList returnWaypoints = normalizeRouteAliases(source,
            QStringList() << QStringLiteral("returnPath") << QStringLiteral("returnRoute"),
            &returnProvided, &returnValid, &returnMetadata);
        if (!returnProvided && taskReturnPaths.contains(idKey)) {
            returnProvided = true;
            returnWaypoints = taskReturnPaths.value(idKey).toList();
            returnMetadata = taskReturnMetadata.value(idKey).toMap();
            returnValid = !returnWaypoints.isEmpty();
        } else if (!returnProvided && commonReturnProvided) {
            returnProvided = true;
            returnWaypoints = commonReturnPath;
            returnMetadata = commonReturnMetadata;
            returnValid = commonReturnValid;
        }
        if (returnProvided) {
            member.insert(QStringLiteral("returnPath"), returnWaypoints);
            if (!returnValid) {
                appendUnique(&unmet, QStringLiteral("invalid_member_return_path"));
                appendRisk(&risks, QStringLiteral("invalid_member_return_path"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 has an empty, malformed, or conflicting return path.").arg(idKey));
            }
            const QVariantMap returnFieldNames = {
                {QStringLiteral("durationSeconds"), QStringLiteral("returnDurationSeconds")},
                {QStringLiteral("startTime"), QStringLiteral("returnStartTime")},
                {QStringLiteral("endTime"), QStringLiteral("returnEndTime")},
                {QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")},
                {QStringLiteral("timeReference"), QStringLiteral("returnTimeReference")}
            };
            for (auto it = returnMetadata.constBegin(); it != returnMetadata.constEnd(); ++it) {
                const QString field = returnFieldNames.value(it.key()).toString();
                if (source.contains(field) && source.value(field) != it.value()) {
                    appendUnique(&unmet, QStringLiteral("conflicting_member_return_schedule"));
                    appendRisk(&risks, QStringLiteral("conflicting_member_return_schedule"), QStringLiteral("blocking"),
                               QStringLiteral("Vehicle %1 field %2 conflicts with its wrapped return route.").arg(idKey, field));
                }
                member.insert(field, it.value());
            }
        }
        // Validate supplied scheduling fields but keep malformed values
        // visible to the downstream reviewer as well as blocking here.
        for (const QString& key : {QStringLiteral("durationSeconds"),
                                  QStringLiteral("missionDurationSeconds"),
                                  QStringLiteral("taskDurationSeconds"),
                                  QStringLiteral("timeLimitSeconds"),
                                  QStringLiteral("returnDurationSeconds")}) {
            if (!source.contains(key)) continue;
            member.insert(key, source.value(key));
            double seconds = 0.0;
            if (!finiteNumber(source.value(key), &seconds) || seconds <= 0.0) {
                appendUnique(&unmet, QStringLiteral("invalid_member_duration"));
                appendRisk(&risks, QStringLiteral("invalid_member_duration"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 field %2 must be finite and positive.").arg(idKey, key));
            }
        }
        // These are assignment-specific planning requirements. Preserve
        // them on the real compiled member, rather than promoting the
        // fastest aircraft or longest return reserve to a group-wide floor.
        for (const QString& key : {QStringLiteral("minimumSpeedMps"),
                                  QStringLiteral("returnReserveSeconds")}) {
            if (!source.contains(key)) continue;
            member.insert(key, source.value(key));
            double requirement = 0.0;
            if (!source.value(key).isValid() || source.value(key).isNull()
                || !finiteNumber(source.value(key), &requirement) || requirement < 0.0) {
                appendUnique(&unmet, QStringLiteral("invalid_member_performance_requirement"));
                appendRisk(&risks, QStringLiteral("invalid_member_performance_requirement"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 field %2 must be finite and non-negative; malformed original requirements are not dropped.").arg(idKey, key));
            }
        }
        double duration = 0.0;
        if (!numericAliases(source,
            QStringList() << QStringLiteral("durationSeconds") << QStringLiteral("missionDurationSeconds")
                           << QStringLiteral("taskDurationSeconds"), &duration, nullptr)) {
            appendUnique(&unmet, QStringLiteral("invalid_member_duration"));
            appendRisk(&risks, QStringLiteral("invalid_member_duration"), QStringLiteral("blocking"),
                       QStringLiteral("Vehicle %1 has contradictory or malformed duration aliases.").arg(idKey));
        }
        for (const QString& key : {QStringLiteral("startTime"), QStringLiteral("startAt"),
                                  QStringLiteral("endTime"), QStringLiteral("deadline"),
                                  QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime")}) {
            if (!source.contains(key)) continue;
            member.insert(key, source.value(key));
            if (!finiteNumber(source.value(key), nullptr)) {
                appendUnique(&unmet, QStringLiteral("invalid_member_schedule"));
                appendRisk(&risks, QStringLiteral("invalid_member_schedule"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 field %2 must be a finite numeric time.").arg(idKey, key));
            }
        }
        double start = 0.0;
        double end = 0.0;
        bool startProvided = false;
        bool endProvided = false;
        const bool startValid = numericAliases(source,
            QStringList() << QStringLiteral("startTime") << QStringLiteral("startAt"),
            &start, &startProvided);
        const bool endValid = numericAliases(source,
            QStringList() << QStringLiteral("endTime") << QStringLiteral("deadline"),
            &end, &endProvided);
        double returnStart = 0.0;
        double returnEnd = 0.0;
        const bool returnStartValid = finiteNumber(source.value(QStringLiteral("returnStartTime")), &returnStart);
        const bool returnEndValid = finiteNumber(source.value(QStringLiteral("returnEndTime")), &returnEnd);
        if (!startValid || !endValid || (startProvided && endProvided && end < start)
            || (returnStartValid && returnEndValid && returnEnd < returnStart)) {
            appendUnique(&unmet, QStringLiteral("invalid_member_schedule"));
            appendRisk(&risks, QStringLiteral("invalid_member_schedule"), QStringLiteral("blocking"),
                       QStringLiteral("Vehicle %1 has conflicting time aliases or reversed schedule endpoints.").arg(idKey));
        }
        for (const QString& key : {QStringLiteral("estimatedDurationSeconds"), QStringLiteral("pathLengthMeters"),
                                  QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("returnPathLengthMeters")}) {
            if (!source.contains(key)) continue;
            double number = 0.0;
            member.insert(key, source.value(key));
            if (!finiteNumber(source.value(key), &number) || number < 0.0) {
                appendUnique(&unmet, QStringLiteral("invalid_member_route_estimate"));
            }
        }
        for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("timeReference"),
                                  QStringLiteral("returnTimingEvidence"), QStringLiteral("returnTimeReference"),
                                  QStringLiteral("routeSource"), QStringLiteral("returnRouteSource"),
                                  QStringLiteral("altitudeReference"), QStringLiteral("localFrameId"),
                                  QStringLiteral("coordinateSystem")}) {
            if (!source.contains(key)) continue;
            const QVariant value = source.value(key);
            member.insert(key, value);
            if (value.type() != QVariant::String || value.toString().size() > 128 || value.toString().trimmed().isEmpty()) {
                appendUnique(&unmet, QStringLiteral("invalid_member_route_metadata"));
            }
            if (key == QStringLiteral("coordinateSystem")
                && value.toString() != QStringLiteral("geodetic")
                && value.toString() != QStringLiteral("local")
                && value.toString() != QStringLiteral("localENU")) {
                appendUnique(&unmet, QStringLiteral("invalid_member_coordinate_system"));
            }
        }
        if (source.contains(QStringLiteral("returnRequired"))) {
            bool valid = true;
            strictBool(source, QStringLiteral("returnRequired"), true, &valid);
            member.insert(QStringLiteral("returnRequired"), source.value(QStringLiteral("returnRequired")));
            if (!valid) appendUnique(&unmet, QStringLiteral("invalid_member_return_required"));
        }
        // Resolve the compiler's already permitted ordinary global fallback
        // ON THE ACTUAL member, before all geometry, budget and upload views.
        // Presence of even an invalid explicit member route prevents fallback.
        if (!coverageRequested && globalRouteProvided && globalRouteValid
            && !member.contains(QStringLiteral("waypoints"))) {
            bool retained = inheritedMemberRequirementsRetained(source) && !task.contains(QStringLiteral("schedule"));
            for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("route")})
                if (task.contains(key) && !inheritedRouteRequirementsRetained(task.value(key))) retained = false;
            // This route can now reach serialization: inspect the actual raw
            // selected return source too, not a lossy normalized substitute.
            bool memberReturnPresent = false;
            for (const QString& key : {QStringLiteral("returnPath"), QStringLiteral("returnRoute")}) {
                if (!source.contains(key)) continue;
                memberReturnPresent = true;
                if (!inheritedRouteRequirementsRetained(source.value(key))) retained = false;
            }
            if (!memberReturnPresent) {
                if (taskReturnPaths.contains(idKey)) {
                    if (!inheritedRouteRequirementsRetained(task.value(QStringLiteral("returnPaths")).toMap().value(idKey)))
                        retained = false;
                } else {
                    for (const QString& key : {QStringLiteral("returnPath"), QStringLiteral("returnRoute")})
                        if (task.contains(key) && !inheritedRouteRequirementsRetained(task.value(key))) retained = false;
                }
            }
            if (!retained) {
                appendUnique(&unmet, QStringLiteral("global_member_route_requirements_unsupported"));
                appendRisk(&risks, QStringLiteral("global_member_route_requirements_unsupported"), QStringLiteral("blocking"),
                    QStringLiteral("Vehicle %1 global fallback or selected return contains unsupported, dropped, oversized or noncanonical route requirements; no executable member fallback is materialized.").arg(idKey));
            } else {
                globalRouteMaterialized = true;
                member.insert(QStringLiteral("waypoints"), waypoints);
                member.insert(QStringLiteral("waypointCount"), waypoints.size());
                for (auto it = globalRouteMetadata.constBegin(); it != globalRouteMetadata.constEnd(); ++it) {
                    QStringList family{it.key()};
                    if (it.key() == QStringLiteral("startTime")) family = QStringList{QStringLiteral("startTime"), QStringLiteral("startAt")};
                    else if (it.key() == QStringLiteral("endTime")) family = QStringList{QStringLiteral("endTime"), QStringLiteral("deadline")};
                    else if (it.key() == QStringLiteral("durationSeconds"))
                        family = QStringList{QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds")};
                    bool memberProvided = false;
                    for (const QString& key : family) memberProvided = memberProvided || member.contains(key);
                    if (!memberProvided) member.insert(it.key(), it.value());
                }
                double resolvedStart = 0.0, resolvedEnd = 0.0, resolvedDuration = 0.0;
                bool resolvedHasStart = false, resolvedHasEnd = false;
                const bool resolvedScheduleValid = numericAliases(member,
                    {QStringLiteral("startTime"), QStringLiteral("startAt")}, &resolvedStart, &resolvedHasStart)
                    && numericAliases(member, {QStringLiteral("endTime"), QStringLiteral("deadline")}, &resolvedEnd, &resolvedHasEnd)
                    && numericAliases(member, {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                        QStringLiteral("taskDurationSeconds")}, &resolvedDuration, nullptr)
                    && (!resolvedHasStart || !resolvedHasEnd || resolvedEnd >= resolvedStart);
                if (!resolvedScheduleValid) {
                    appendUnique(&unmet, QStringLiteral("invalid_resolved_global_member_schedule"));
                    appendRisk(&risks, QStringLiteral("invalid_resolved_global_member_schedule"), QStringLiteral("blocking"),
                        QStringLiteral("Vehicle %1 final member/global schedule has conflicting aliases or reversed endpoints; original member requirements are retained, not corrected.").arg(idKey));
                }
                // Summarize only a complete explicit common reference. The
                // actual points retain their own declarations unchanged.
                const QVariantList returning = member.value(QStringLiteral("returnPath")).toList();
                for (const QString& key : {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference")}) {
                    QString common;
                    bool complete = !waypoints.isEmpty() && !returning.isEmpty();
                    for (const QVariantList& route : {waypoints, returning}) {
                        for (const QVariant& point : route) {
                            const QVariant declaration = point.toMap().value(key);
                            if (declaration.type() != QVariant::String || declaration.isNull() || declaration.toString().isEmpty())
                                complete = false;
                            else if (common.isEmpty()) common = declaration.toString();
                            else if (common != declaration.toString()) complete = false;
                        }
                    }
                    if (!complete) continue;
                    if (!member.contains(key)) member.insert(key, common);
                    else {
                        const QVariant original = member.value(key);
                        const bool alias = key == QStringLiteral("coordinateSystem") && common == QStringLiteral("local")
                            && original.type() == QVariant::String && original.toString() == QStringLiteral("localENU");
                        if (!alias && (original.type() != QVariant::String || original.isNull() || original.toString() != common)) {
                            appendUnique(&unmet, QStringLiteral("global_member_route_reference_conflict"));
                            appendRisk(&risks, QStringLiteral("global_member_route_reference_conflict"), QStringLiteral("blocking"),
                                QStringLiteral("Vehicle %1 member %2 contradicts its complete inherited outbound/return declarations; the original value is not overwritten.").arg(idKey, key));
                        }
                    }
                }
                // Root references assert the already declared route basis;
                // they never supply missing point datums or local origins.
                for (const QString& key : {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
                        QStringLiteral("localFrameId")}) {
                    if (!task.contains(key)) continue;
                    const QVariant declaration = task.value(key);
                    const auto matches = [&declaration, &key](const QVariant& value) {
                        if (value.type() != QVariant::String || value.isNull()) return false;
                        return value.toString() == declaration.toString()
                            || (key == QStringLiteral("coordinateSystem")
                                && value.toString() == QStringLiteral("local")
                                && declaration.toString() == QStringLiteral("localENU"));
                    };
                    bool agrees = declaration.type() == QVariant::String && !declaration.isNull()
                        && !declaration.toString().isEmpty() && declaration.toString().size() <= 128
                        && declaration.toString() == declaration.toString().trimmed()
                        && !declaration.toString().contains(QChar(0)) && !returning.isEmpty();
                    if (member.contains(key) && !matches(member.value(key))) agrees = false;
                    for (const QVariantList& route : {waypoints, returning})
                        for (const QVariant& point : route)
                            if (!point.toMap().contains(key) || !matches(point.toMap().value(key))) agrees = false;
                    if (!agrees) {
                        appendUnique(&unmet, QStringLiteral("global_task_route_reference_conflict"));
                        appendRisk(&risks, QStringLiteral("global_task_route_reference_conflict"), QStringLiteral("blocking"),
                            QStringLiteral("Vehicle %1 task %2 must agree with complete explicit outbound/return point declarations; no missing datum or origin is inferred, and the original task declaration is retained.").arg(idKey, key));
                    }
                }
                if (!member.contains(QStringLiteral("returnRequired")) && task.contains(QStringLiteral("returnRequired"))) {
                    const QVariant required = task.value(QStringLiteral("returnRequired"));
                    if (required.type() == QVariant::Bool && !required.isNull()) member.insert(QStringLiteral("returnRequired"), required);
                    else {
                        appendUnique(&unmet, QStringLiteral("invalid_task_return_required"));
                        appendRisk(&risks, QStringLiteral("invalid_task_return_required"), QStringLiteral("blocking"),
                            QStringLiteral("Global fallback requires any supplied task returnRequired to remain an actual Bool; no default true is inferred."));
                    }
                }
            }
        }
        member.insert(QStringLiteral("manualConfirmationRequired"), true);
        members.append(member);
    }
    if (globalRouteMaterialized && !inheritedTaskScheduleValid(task)) {
        appendUnique(&unmet, QStringLiteral("invalid_global_task_schedule"));
        appendRisk(&risks, QStringLiteral("invalid_global_task_schedule"), QStringLiteral("blocking"),
            QStringLiteral("The retained task-root schedule has a malformed, null, contradictory or reversed value; lower-priority task requirements are not repaired or promoted over member/global/constraint values."));
    }
    if (members.isEmpty()) appendUnique(&unmet, QStringLiteral("vehicle_assignments_missing"));
    for (auto it = taskReturnPaths.constBegin(); it != taskReturnPaths.constEnd(); ++it) {
        if (cancelled()) return cancelledResult();
        if (!assignmentIds.contains(it.key())) {
            appendUnique(&unmet, QStringLiteral("task_return_vehicle_not_assigned"));
            appendRisk(&risks, QStringLiteral("task_return_vehicle_not_assigned"), QStringLiteral("blocking"),
                       QStringLiteral("Return path key %1 is not an exact assigned vehicle ID.").arg(it.key()));
        }
    }
    if (leaderAssignmentCount > 1) {
        appendUnique(&unmet, QStringLiteral("multiple_leaders"));
        appendRisk(&risks, QStringLiteral("multiple_leaders"), QStringLiteral("blocking"),
                   QStringLiteral("A compiled task may contain at most one leader assignment."));
    }
    if (task.contains(QStringLiteral("leaderVehicleId"))
        || task.contains(QStringLiteral("leaderId"))) {
        QVariantMap leaderMap;
        if (task.contains(QStringLiteral("leaderVehicleId"))) {
            leaderMap.insert(QStringLiteral("vehicleId"), task.value(QStringLiteral("leaderVehicleId")));
        }
        if (task.contains(QStringLiteral("leaderId"))) {
            leaderMap.insert(QStringLiteral("id"), task.value(QStringLiteral("leaderId")));
        }
        const QVariant expectedLeader = normalizedVehicleId(leaderMap);
        if (!expectedLeader.isValid()) {
            appendUnique(&unmet, QStringLiteral("invalid_leader_vehicle_id"));
            appendRisk(&risks, QStringLiteral("invalid_leader_vehicle_id"), QStringLiteral("blocking"),
                       QStringLiteral("leaderVehicleId/leaderId must be one consistent 1-255 integer ID."));
        } else if (leaderAssignmentCount != 1
                   || leaderAssignmentId.toString() != expectedLeader.toString()) {
            appendUnique(&unmet, QStringLiteral("leader_assignment_mismatch"));
            appendRisk(&risks, QStringLiteral("leader_assignment_mismatch"), QStringLiteral("blocking"),
                       QStringLiteral("The declared leader must match the single leader assignment."));
        }
    }

    if (coverageRequested) {
        if (cancelled()) return cancelledResult();
        if (coverageValue.type() == QVariant::Map && !coverageValue.isNull() && !globalRouteProvided) {
            coveragePlan = FleetAreaCoveragePlanner::generate(task.value(QStringLiteral("area")),
                                                              coverageValue.toMap(), members);
            if (cancelled()) return cancelledResult();
            const QStringList errors = coveragePlan.value(QStringLiteral("errors")).toStringList();
            for (const QString& error : errors) {
                if (cancelled()) return cancelledResult();
                appendUnique(&unmet, error);
                appendRisk(&risks, error, QStringLiteral("blocking"),
                           QStringLiteral("Area coverage generation rejected the supplied input: %1.").arg(error));
            }
            const QVariant generated = coveragePlan.value(QStringLiteral("generated"));
            const QVariant inputValid = coveragePlan.value(QStringLiteral("inputValid"));
            if (generated.type() == QVariant::Bool && generated.toBool()
                && inputValid.type() == QVariant::Bool && inputValid.toBool() && errors.isEmpty()) {
                members = coveragePlan.value(QStringLiteral("assignments")).toList();
                if (constraints.contains(QStringLiteral("maxAltitudeMeters"))) {
                    double ceiling = 0.0;
                    double requestedAltitude = 0.0;
                    const QVariant ceilingReference = constraints.value(QStringLiteral("maxAltitudeReference"));
                    if (ceilingReference.type() != QVariant::String
                        || ceilingReference.toString() != coverageValue.toMap().value(QStringLiteral("altitudeReference")).toString()) {
                        appendUnique(&unmet, QStringLiteral("coverage_max_altitude_reference_mismatch"));
                        appendRisk(&risks, QStringLiteral("coverage_max_altitude_reference_mismatch"), QStringLiteral("blocking"),
                                   QStringLiteral("The altitude ceiling requires the same explicit altitude reference as the generated route."));
                    } else if (!finiteNumber(constraints.value(QStringLiteral("maxAltitudeMeters")), &ceiling)) {
                        appendUnique(&unmet, QStringLiteral("invalid_max_altitude"));
                    } else {
                        bool altitudeUnknown = !finiteNumber(coverageValue.toMap().value(QStringLiteral("altitudeMeters")),
                                                             &requestedAltitude);
                        bool altitudeExceeded = !altitudeUnknown && requestedAltitude > ceiling;
                        // The ceiling covers the full adopted route, not just
                        // the survey altitude. Endpoint and vertical portions
                        // can be higher, and support routes may lack height.
                        for (const QVariant& value : members) {
                            if (cancelled()) return cancelledResult();
                            const QVariantMap member = value.toMap();
                            for (const QString& routeKey : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
                                if (!member.contains(routeKey)) continue;
                                for (const QVariant& pointValue : member.value(routeKey).toList()) {
                                    if (cancelled()) return cancelledResult();
                                    const QVariantMap point = pointValue.toMap();
                                    const QVariant reference = point.contains(QStringLiteral("altitudeReference"))
                                        ? point.value(QStringLiteral("altitudeReference"))
                                        : member.value(QStringLiteral("altitudeReference"));
                                    double pointAltitude = 0.0;
                                    if (pointValue.type() != QVariant::Map
                                        || reference.type() != QVariant::String
                                        || reference.toString() != ceilingReference.toString()
                                        || !finiteNumber(point.value(QStringLiteral("altitude")), &pointAltitude)) {
                                        altitudeUnknown = true;
                                    } else if (pointAltitude > ceiling) {
                                        altitudeExceeded = true;
                                    }
                                }
                            }
                        }
                        if (altitudeUnknown) {
                            appendUnique(&unmet, QStringLiteral("coverage_max_altitude_unknown"));
                            appendRisk(&risks, QStringLiteral("coverage_max_altitude_unknown"), QStringLiteral("blocking"),
                                       QStringLiteral("The full coverage, ingress, support and return routes require finite altitude in the declared ceiling reference."));
                        }
                        if (altitudeExceeded) {
                            appendUnique(&unmet, QStringLiteral("coverage_max_altitude_exceeded"));
                            appendRisk(&risks, QStringLiteral("coverage_max_altitude_exceeded"), QStringLiteral("blocking"),
                                       QStringLiteral("Coverage, ingress, support or return altitude exceeds the explicitly supplied altitude ceiling."));
                        }
                    }
                }
            } else if (errors.isEmpty()) {
                appendUnique(&unmet, QStringLiteral("coverage_generation_failed"));
            }
        } else {
            coveragePlan.insert(QStringLiteral("inputValid"), false);
            coveragePlan.insert(QStringLiteral("generated"), false);
            coveragePlan.insert(QStringLiteral("errors"), QStringList()
                << (globalRouteProvided ? QStringLiteral("coverage_global_route_conflict")
                                        : QStringLiteral("invalid_coverage_options")));
        }
        coveragePlan.insert(QStringLiteral("manualConfirmationRequired"), true);
        coveragePlan.insert(QStringLiteral("flightCommandReleased"), false);
    }
    // Every actual member must now carry its resolved route. Eligible global
    // inheritance was materialized above; no later planning-only fallback
    // may hide an invalid or unsupported source requirement.
    if (!members.isEmpty()) {
        for (const QVariant& memberValue : members) {
            if (cancelled()) return cancelledResult();
            const QVariantMap member = memberValue.toMap();
            const bool ownRoute = member.contains(QStringLiteral("waypoints"))
                && !member.value(QStringLiteral("waypoints")).toList().isEmpty();
            if (!ownRoute) {
                appendUnique(&unmet, QStringLiteral("waypoint_route_missing"));
                appendRisk(&risks, QStringLiteral("waypoint_route_missing"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 has no usable resolved member route.")
                               .arg(member.value(QStringLiteral("vehicleId")).toString()));
            }
        }
    } else if (waypoints.isEmpty()) {
        appendUnique(&unmet, QStringLiteral("waypoint_route_missing"));
    }

    // Coverage preserves copied assignments; check the resolved profiles
    // after generated ingress/return geometry has reached its final form.
    // Auxiliary roles do not inherit survey speed from coverage metadata.
    for (const QVariant& memberValue : members) {
        if (cancelled()) return cancelledResult();
        const QVariantMap member = memberValue.toMap();
        if (!member.contains(QStringLiteral("executionProfile"))) continue;
        const QVariantMap profile = member.value(QStringLiteral("executionProfile")).toMap();
        if (profile.isEmpty()) continue; // Earlier contract failure is sticky.
        const QString id = member.value(QStringLiteral("vehicleId")).toString();
        const bool coverageParticipant = coveragePlan.value(QStringLiteral("generated")).type() == QVariant::Bool
            && coveragePlan.value(QStringLiteral("generated")).toBool()
            && coveragePlan.value(QStringLiteral("participantVehicleIds")).toList().contains(member.value(QStringLiteral("vehicleId")));
        const QVariantMap vertical = profile.value(QStringLiteral("verticalPolicy")).toMap();
        if (coverageParticipant
            && (profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")) != coveragePlan.value(QStringLiteral("cruiseSpeedMetersPerSecond"))
                || (coveragePlan.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                    && vertical.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                    && vertical.value(QStringLiteral("climbSpeedMetersPerSecond")) != coveragePlan.value(QStringLiteral("climbSpeedMetersPerSecond"))))) {
            appendUnique(&unmet, QStringLiteral("execution_coverage_speed_conflict"));
            appendRisk(&risks, QStringLiteral("execution_coverage_speed_conflict"), QStringLiteral("blocking"),
                       QStringLiteral("Vehicle %1 explicit execution cruise/climb speed conflicts with its coverage planning requirement.").arg(id));
        }
        const QVariantList outbound = member.value(QStringLiteral("waypoints")).toList();
        const QVariantList returning = member.value(QStringLiteral("returnPath")).toList();
        for (const QString& actionKey : {QStringLiteral("launchAction"), QStringLiteral("endAction")}) {
            const QVariantMap action = profile.value(actionKey).toMap();
            if (!action.contains(QStringLiteral("coordinate"))) continue;
            const QVariantMap point = action.value(QStringLiteral("coordinate")).toMap();
            double ceiling = 0.0;
            const bool ceilingValid = finiteNumber(constraints.value(QStringLiteral("maxAltitudeMeters")), &ceiling)
                && ceiling >= -1000.0 && ceiling <= 20000.0
                && constraints.value(QStringLiteral("maxAltitudeReference")).type() == QVariant::String
                && constraints.value(QStringLiteral("maxAltitudeReference")).toString() == QStringLiteral("amsl");
            if (!ceilingValid) {
                appendUnique(&unmet, QStringLiteral("execution_profile_altitude_ceiling_missing"));
                appendRisk(&risks, QStringLiteral("execution_profile_altitude_ceiling_missing"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 action coordinates require an explicit valid AMSL altitude ceiling.").arg(id));
            } else if (point.value(QStringLiteral("altitude")).toDouble() > ceiling) {
                appendUnique(&unmet, QStringLiteral("execution_profile_altitude_exceeded"));
                appendRisk(&risks, QStringLiteral("execution_profile_altitude_exceeded"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 action coordinate exceeds its declared AMSL ceiling.").arg(id));
            }
            const bool launching = actionKey == QStringLiteral("launchAction");
            const bool landing = action.value(QStringLiteral("type")).toString() == QStringLiteral("land");
            double approachAltitude = 0.0;
            const QVariantMap approach = returning.isEmpty() ? QVariantMap() : returning.last().toMap();
            const bool hasApproachAltitude = !returning.isEmpty()
                && returning.last().type() == QVariant::Map
                && approach.value(QStringLiteral("coordinateSystem")).type() == QVariant::String
                && approach.value(QStringLiteral("coordinateSystem")).toString() == QStringLiteral("geodetic")
                && approach.value(QStringLiteral("altitudeReference")).type() == QVariant::String
                && approach.value(QStringLiteral("altitudeReference")).toString() == QStringLiteral("amsl")
                && finiteNumber(approach.value(QStringLiteral("altitude")), &approachAltitude);
            const bool matches = launching
                ? !outbound.isEmpty() && sameCanonicalGeodeticPoint(outbound.first(), point)
                : landing
                    ? !returning.isEmpty() && sameCanonicalGeodeticPoint(returning.last(), point, false)
                        && hasApproachAltitude && point.value(QStringLiteral("altitude")).toDouble() < approachAltitude
                    : !returning.isEmpty() && sameCanonicalGeodeticPoint(returning.last(), point);
            // LAND explicitly describes the descent from a distinct approach
            // height. Do not force the preceding waypoint onto the ground.
            if (landing && ceilingValid && hasApproachAltitude && approachAltitude > ceiling) {
                appendUnique(&unmet, QStringLiteral("execution_profile_altitude_exceeded"));
                appendRisk(&risks, QStringLiteral("execution_profile_altitude_exceeded"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 final landing approach exceeds its declared AMSL ceiling.").arg(id));
            }
            if (!matches) {
                appendUnique(&unmet, QStringLiteral("execution_profile_endpoint_mismatch"));
                appendRisk(&risks, QStringLiteral("execution_profile_endpoint_mismatch"), QStringLiteral("blocking"),
                           QStringLiteral("Vehicle %1 %2 requires exact takeoff/HOLD endpoint coordinates, or LAND at the same latitude/longitude with explicit ground AMSL strictly below the return approach; no connector or ground height is inferred.").arg(id, actionKey));
            }
        }
        const QVariantMap launch = profile.value(QStringLiteral("launchAction")).toMap();
        const QVariantMap end = profile.value(QStringLiteral("endAction")).toMap();
        if (launch.value(QStringLiteral("type")).toString() == QStringLiteral("none")
            || end.value(QStringLiteral("type")).toString() == QStringLiteral("rtl")
            || profile.value(QStringLiteral("timingPolicy")).toString() == QStringLiteral("absolute-required")) {
            appendRisk(&risks, QStringLiteral("execution_profile_requires_adapter"), QStringLiteral("warning"),
                       QStringLiteral("Vehicle %1 retains none/RTL/absolute timing requirements; this compiler does not encode or authorize those policies.").arg(id));
        }
    }

    QVariant reserveValue = task.value(QStringLiteral("reserveVehicleIds"));
    if (!reserveValue.isValid()) reserveValue = task.value(QStringLiteral("reserve"));
    int invalidReserveIds = 0;
    const QVariantList reserveIds = idList(reserveValue, &invalidReserveIds);
    if (invalidReserveIds > 0) {
        appendUnique(&unmet, QStringLiteral("invalid_reserve_vehicle_id"));
        appendRisk(&risks, QStringLiteral("invalid_reserve_vehicle_id"), QStringLiteral("blocking"),
                   QStringLiteral("One or more reserve vehicle IDs are not valid 1-255 integer IDs."));
    }
    // These descriptors are independent reserve budget intent. They never
    // enter coverage generation, active members, assignments or upload.
    QSet<QString> declaredReserveIds;
    for (const QVariant& id : reserveIds) declaredReserveIds.insert(vehicleKey(id));
    QVariantMap reserveDescriptors;
    bool reservePlansValid = true;
    if (task.contains(QStringLiteral("reservePlans"))) {
        const QVariant supplied = task.value(QStringLiteral("reservePlans"));
        reservePlansValid = supplied.type() == QVariant::List && !supplied.isNull()
            && supplied.toList().size() <= 255;
        const QSet<QString> fields{QStringLiteral("vehicleId"), QStringLiteral("start"),
            QStringLiteral("returnTo"), QStringLiteral("reservePlan")};
        if (reservePlansValid) {
            for (const QVariant& value : supplied.toList()) {
                if (cancelled()) return cancelledResult();
                if (value.type() != QVariant::Map || value.isNull()) { reservePlansValid = false; break; }
                const QVariantMap descriptor = value.toMap();
                bool fieldsValid = descriptor.size() == fields.size();
                for (auto it = descriptor.constBegin(); it != descriptor.constEnd(); ++it)
                    fieldsValid = fieldsValid && fields.contains(it.key());
                const QVariant id = normalizedVehicleId(descriptor);
                const QString idKey = vehicleKey(id);
                if (!fieldsValid || !id.isValid() || !declaredReserveIds.contains(idKey)
                    || assignmentIds.contains(idKey) || reserveDescriptors.contains(idKey)) {
                    reservePlansValid = false;
                    break;
                }
                reserveDescriptors.insert(idKey, descriptor);
            }
        }
        if (!reservePlansValid) {
            // Never retain a valid-looking prefix of a malformed list.
            reserveDescriptors.clear();
            appendUnique(&unmet, QStringLiteral("invalid_reserve_plans"));
            appendRisk(&risks, QStringLiteral("invalid_reserve_plans"), QStringLiteral("blocking"),
                       QStringLiteral("reservePlans requires bounded exact vehicleId/start/returnTo/reservePlan maps with unique IDs belonging only to the actual reserve set."));
        }
    }
    // All consumers now see the same actual resolved member routes.
    const QVariantList reserveBudgetMembers = members;
    QSet<QString> reserveSeen;
    QVariantList reserveMembers;
    int reserveGeneratedPoints = 0;
    bool reserveGeneratedBudgetExceeded = false;
    for (const QVariant& id : reserveIds) {
        if (cancelled()) return cancelledResult();
        const QString idKey = vehicleKey(id);
        if (reserveSeen.contains(idKey)) {
            appendUnique(&unmet, QStringLiteral("duplicate_reserve_vehicle"));
            continue;
        }
        reserveSeen.insert(idKey);
        if (assignmentIds.contains(idKey)) appendUnique(&unmet, QStringLiteral("reserve_vehicle_already_assigned"));
        if (fleetProvided && !fleetIds.contains(idKey)) {
            appendUnique(&unmet, QStringLiteral("reserve_vehicle_not_in_fleet"));
            appendRisk(&risks, QStringLiteral("reserve_vehicle_not_in_fleet"), QStringLiteral("blocking"),
                       QStringLiteral("Reserve vehicle %1 is not present in the supplied fleet snapshot.").arg(idKey));
        }
        QVariantMap reserve;
        reserve.insert(QStringLiteral("vehicleId"), id);
        reserve.insert(QStringLiteral("role"), QStringLiteral("reserve"));
        reserve.insert(QStringLiteral("manualConfirmationRequired"), true);
        const QVariantMap missingDescriptor{{QStringLiteral("vehicleId"), id}};
        const QVariantMap descriptor = !reserveGeneratedBudgetExceeded && reserveDescriptors.contains(idKey)
            ? reserveDescriptors.value(idKey).toMap() : missingDescriptor;
        QVariantMap energyPlan = FleetReserveEnergyPlan::build(descriptor, reserveBudgetMembers, constraints,
            fleetById.value(idKey).toMap(), task.value(QStringLiteral("requirePerformanceEvidence"), false));
        if (cancelled()) return cancelledResult();
        if (!reservePlansValid) energyPlan.insert(QStringLiteral("reason"), QStringLiteral("invalid_reserve_plans"));
        const QVariantList outbound = energyPlan.value(QStringLiteral("outboundPath")).toList();
        const QVariantList returning = energyPlan.value(QStringLiteral("returnPath")).toList();
        if (energyPlan.value(QStringLiteral("reason")).toString() == QStringLiteral("reserve_generated_path_limit_exceeded")
            || outbound.size() > 4096 || returning.size() > 4096
            || reserveGeneratedPoints > 32768 - outbound.size() - returning.size()) {
            reserveGeneratedBudgetExceeded = true;
        } else {
            reserveGeneratedPoints += outbound.size() + returning.size();
        }
        if (energyPlan.value(QStringLiteral("complete")).type() != QVariant::Bool
            || !energyPlan.value(QStringLiteral("complete")).toBool()) {
            appendUnique(&unmet, QStringLiteral("reserve_energy_plan_incomplete"));
            appendRisk(&risks, QStringLiteral("reserve_energy_plan_incomplete"), QStringLiteral("blocking"),
                       QStringLiteral("Reserve vehicle %1 has no complete independent standby/replacement budget: %2.")
                           .arg(idKey, energyPlan.value(QStringLiteral("reason")).toString().left(128)));
        }
        reserve.insert(QStringLiteral("energyPlan"), energyPlan);
        reserveMembers.append(reserve);
    }
    if (reserveGeneratedBudgetExceeded) {
        // All-or-nothing geometry budget: invalidate earlier successful rows
        // too, rather than presenting a bounded prefix as a complete group.
        for (int index = 0; index < reserveMembers.size(); ++index) {
            if (cancelled()) return cancelledResult();
            QVariantMap reserve = reserveMembers.at(index).toMap();
            const QVariant id = reserve.value(QStringLiteral("vehicleId"));
            QVariantMap energyPlan = FleetReserveEnergyPlan::build(
                QVariantMap{{QStringLiteral("vehicleId"), id}}, reserveBudgetMembers, constraints,
                fleetById.value(vehicleKey(id)).toMap(), task.value(QStringLiteral("requirePerformanceEvidence"), false));
            if (cancelled()) return cancelledResult();
            energyPlan.insert(QStringLiteral("reason"), QStringLiteral("reserve_generated_paths_budget_exceeded"));
            reserve.insert(QStringLiteral("energyPlan"), energyPlan);
            reserveMembers[index] = reserve;
        }
        appendUnique(&unmet, QStringLiteral("reserve_generated_paths_budget_exceeded"));
        appendRisk(&risks, QStringLiteral("reserve_generated_paths_budget_exceeded"), QStringLiteral("blocking"),
                   QStringLiteral("Reserve geometry exceeds 4096 points per path or 32768 generated points across all reserves; no partial group budget is accepted."));
    }
    bool reserveCountOk = true;
    const QVariant requestedReserve = task.value(QStringLiteral("reserveCount"));
    if (requestedReserve.isValid()) {
        double countValue = 0.0;
        reserveCountOk = finiteNumber(requestedReserve, &countValue)
            && countValue >= 0.0
            && std::floor(countValue) == countValue
            && countValue <= static_cast<double>(std::numeric_limits<int>::max());
        if (!reserveCountOk) {
            appendUnique(&unmet, QStringLiteral("invalid_reserve_count"));
            appendRisk(&risks, QStringLiteral("invalid_reserve_count"), QStringLiteral("blocking"),
                       QStringLiteral("reserveCount must be a non-negative integer."));
        } else if (reserveIds.size() < static_cast<int>(countValue)) {
            appendUnique(&unmet, QStringLiteral("reserve_requirement_unmet"));
        }
    }

    // Explicit assignments and coverage-resolved members must obey the same
    // local capability policy as allocator-selected members. Never repair a
    // mismatch by selecting another vehicle or silently dropping a member.
    bool verifiedPolicyValid = true, baselinePolicyValid = true, performancePolicyValid = true;
    const bool requireVerifiedCapabilities = strictBool(task, QStringLiteral("requireVerifiedCapabilities"), false, &verifiedPolicyValid);
    const bool requireConfigurationBaseline = strictBool(task, QStringLiteral("requireConfigurationBaseline"), false, &baselinePolicyValid);
    const bool requirePerformanceEvidence = strictBool(task, QStringLiteral("requirePerformanceEvidence"), false, &performancePolicyValid);
    if (!verifiedPolicyValid || !baselinePolicyValid || !performancePolicyValid) {
        appendUnique(&unmet, QStringLiteral("invalid_capability_policy_boolean"));
        appendRisk(&risks, QStringLiteral("invalid_capability_policy_boolean"), QStringLiteral("blocking"),
                   QStringLiteral("Capability/baseline/performance policy fields must be actual non-null booleans; missing fields keep the planning defaults."));
    }
    double minimumDeclaredSpeed = 0.0, minimumDeclaredEndurance = 0.0;
    for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
        if (!task.contains(key)) continue;
        double value = 0.0;
        if (task.value(key).isNull() || !finiteNumber(task.value(key), &value)
            || value < 0.0 || value > 9007199254740991.0) {
            appendUnique(&unmet, QStringLiteral("invalid_declared_performance_threshold"));
            appendRisk(&risks, QStringLiteral("invalid_declared_performance_threshold"), QStringLiteral("blocking"),
                QStringLiteral("Performance threshold %1 must be an actual finite number in 0..9007199254740991; unknown or malformed values are not zero requirements.").arg(key));
        } else if (key == QStringLiteral("minimumSpeedMps")) minimumDeclaredSpeed = value;
        else minimumDeclaredEndurance = value;
    }
    QVariantList declaredPerformanceSources;
    QMap<QString, QStringList> capabilityRequirements;
    for (const QString& key : {QStringLiteral("requiredCapabilities"), QStringLiteral("capabilities"),
            QStringLiteral("leaderCapabilities"), QStringLiteral("scoutCapabilities"),
            QStringLiteral("relayCapabilities"), QStringLiteral("responderCapabilities")}) {
        if (!task.contains(key)) continue;
        QStringList capabilities;
        if (!FleetCapabilityEvidence::canonicalCapabilities(task.value(key), &capabilities)) {
            appendUnique(&unmet, QStringLiteral("invalid_task_capability_requirements"));
            appendRisk(&risks, QStringLiteral("invalid_task_capability_requirements"), QStringLiteral("blocking"),
                       QStringLiteral("Task field %1 must be a bounded, non-null string capability list; malformed values cannot become empty requirements.").arg(key));
        } else {
            capabilityRequirements.insert(key, capabilities);
        }
    }
    // Preserve the existing authoritative-field precedence, including an
    // explicitly empty requiredCapabilities list. The compatibility alias is
    // independently validated above; it cannot hide malformed input.
    const QStringList globalCapabilities = capabilityRequirements.value(task.contains(QStringLiteral("requiredCapabilities"))
        ? QStringLiteral("requiredCapabilities") : QStringLiteral("capabilities"));
    const auto enforceCapabilities = [&](const QVariantMap& member, bool reserve) {
        const QString id = vehicleKey(normalizedVehicleId(member));
        if (id.isEmpty()) return; // Existing assignment/coverage ID errors remain blocking.
        QStringList required = globalCapabilities;
        if (!reserve) {
            for (const QString& token : capabilityRequirements.value(member.value(QStringLiteral("role")).toString() + QStringLiteral("Capabilities")))
                if (!required.contains(token)) required.append(token);
        }
        const bool capabilityEvidenceRequired = requireVerifiedCapabilities || requireConfigurationBaseline || !required.isEmpty();
        const bool speedRequirementPresent = member.contains(QStringLiteral("minimumSpeedMps"));
        double memberMinimumSpeed = 0.0;
        const bool speedEvidenceRequired = speedRequirementPresent
            && !member.value(QStringLiteral("minimumSpeedMps")).isNull()
            && finiteNumber(member.value(QStringLiteral("minimumSpeedMps")), &memberMinimumSpeed)
            && memberMinimumSpeed > 0.0;
        const bool taskPerformanceRequired = minimumDeclaredSpeed > 0.0 || minimumDeclaredEndurance > 0.0;
        if (!capabilityEvidenceRequired && !speedEvidenceRequired && !requirePerformanceEvidence && !taskPerformanceRequired) return;
        const QString label = reserve ? QStringLiteral("Reserve vehicle") : QStringLiteral("Vehicle");
        if (!fleetProvided || !fleetById.contains(id)) {
            appendUnique(&unmet, QStringLiteral("capability_policy_fleet_evidence_missing"));
            appendRisk(&risks, QStringLiteral("capability_policy_fleet_evidence_missing"), QStringLiteral("blocking"),
                       QStringLiteral("%1 %2 has capability, baseline or explicit performance requirements but no unique matching original fleet snapshot; planning-only input cannot prove those requirements.").arg(label, id));
            return;
        }
        const QVariantMap vehicle = fleetById.value(id).toMap();
        QVariantMap declaredPerformance;
        const bool declaredPerformanceUsable = requirePerformanceEvidence
            && FleetCapabilityEvidence::currentDeclaredPerformance(vehicle, &declaredPerformance)
            && vehicleKey(declaredPerformance.value(QStringLiteral("vehicleId"))) == id;
        if (!declaredPerformanceUsable) declaredPerformance.clear();
        if (requirePerformanceEvidence) {
            declaredPerformanceSources.append(QVariantMap{{QStringLiteral("vehicleId"), normalizedVehicleId(member)},
                {QStringLiteral("group"), reserve ? QStringLiteral("reserve") : QStringLiteral("primary")},
                {QStringLiteral("evidence"), declaredPerformance}});
            if (!declaredPerformanceUsable) {
                appendUnique(&unmet, QStringLiteral("declared_performance_not_current"));
                appendRisk(&risks, QStringLiteral("declared_performance_not_current"), QStringLiteral("blocking"),
                    QStringLiteral("%1 %2 needs numerical declarations bound to its current local engineering review and complete matching configuration baseline; raw metadata is not that source.").arg(label, id));
            } else {
                const double maximum = declaredPerformance.value(QStringLiteral("maxSpeedMps")).toDouble();
                // These are the actual member encoding requests, not another
                // aircraft's rating or a role allocator's suggested speed.
                const QVariantMap profile = member.value(QStringLiteral("executionProfile")).toMap();
                for (const QString& key : {QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("returnSpeedMetersPerSecond")}) {
                    if (!profile.contains(key)) continue; // Profile validity remains an independent gate.
                    double requested = 0.0;
                    if (profile.value(key).isNull() || !finiteNumber(profile.value(key), &requested) || requested <= 0.0
                        || maximum < requested) {
                        appendUnique(&unmet, QStringLiteral("execution_speed_exceeds_declared_performance"));
                        appendRisk(&risks, QStringLiteral("execution_speed_exceeds_declared_performance"), QStringLiteral("blocking"),
                            QStringLiteral("%1 %2 %3 is unknown/invalid or exceeds this member's current declared maximum speed; this is not performance certification.").arg(label, id, key));
                    }
                }
                const bool coverageParticipant = !reserve
                    && coveragePlan.value(QStringLiteral("generated")).type() == QVariant::Bool
                    && coveragePlan.value(QStringLiteral("generated")).toBool()
                    && coveragePlan.value(QStringLiteral("participantVehicleIds")).toList().contains(member.value(QStringLiteral("vehicleId")));
                if (coverageParticipant) {
                    double requested = 0.0;
                    if (coveragePlan.value(QStringLiteral("cruiseSpeedMetersPerSecond")).isNull()
                        || !finiteNumber(coveragePlan.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &requested)
                        || requested <= 0.0 || maximum < requested) {
                        appendUnique(&unmet, QStringLiteral("coverage_speed_exceeds_declared_performance"));
                        appendRisk(&risks, QStringLiteral("coverage_speed_exceeds_declared_performance"), QStringLiteral("blocking"),
                            QStringLiteral("%1 %2's generated coverage speed exceeds or lacks its own current declared maximum speed.").arg(label, id));
                    }
                }
            }
        }
        // Positive task floors apply independently of the strict-source switch.
        // In legacy planning mode only this member's own descriptor is used;
        // first-present alias precedence matches Matcher/Allocator. A missing,
        // malformed or exact -1 value is never evidence of meeting a floor.
        const auto enforceTaskFloor = [&](const QStringList& aliases, double minimum) {
            if (minimum <= 0.0 || (requirePerformanceEvidence && !declaredPerformanceUsable)) return;
            const QVariantMap& evidence = requirePerformanceEvidence ? declaredPerformance : vehicle;
            QVariant value;
            for (const QString& alias : aliases) {
                if (evidence.contains(alias)) { value = evidence.value(alias); break; }
            }
            double available = 0.0;
            if (!value.isValid() || value.isNull() || !finiteNumber(value, &available) || available < 0.0) {
                appendUnique(&unmet, QStringLiteral("task_performance_evidence_missing"));
                appendRisk(&risks, QStringLiteral("task_performance_evidence_missing"), QStringLiteral("blocking"),
                    QStringLiteral("%1 %2 needs its own known non-negative %3 to meet the positive task floor; an unknown planning descriptor is not a pass or certification.").arg(label, id, aliases.first()));
            } else if (available < minimum) {
                const QString code = requirePerformanceEvidence ? QStringLiteral("declared_performance_threshold_unmet")
                    : QStringLiteral("task_performance_threshold_unmet");
                appendUnique(&unmet, code);
                appendRisk(&risks, code, QStringLiteral("blocking"),
                    QStringLiteral("%1 %2's own %3 is below the task floor; another member cannot satisfy it.").arg(label, id, aliases.first()));
            }
        };
        enforceTaskFloor(QStringList() << QStringLiteral("maxSpeedMps"), minimumDeclaredSpeed);
        enforceTaskFloor(requirePerformanceEvidence ? QStringList() << QStringLiteral("enduranceSeconds")
            : QStringList() << QStringLiteral("enduranceSeconds") << QStringLiteral("enduranceSec"), minimumDeclaredEndurance);
        if (speedRequirementPresent) {
            double minimum = 0.0, maximum = 0.0;
            const bool requirementValid = !member.value(QStringLiteral("minimumSpeedMps")).isNull()
                && finiteNumber(member.value(QStringLiteral("minimumSpeedMps")), &minimum)
                && minimum >= 0.0;
            // Registry's exact -1 sentinel and an absent field are unknown,
            // not measured zero. Explicit positive member requirements need
            // this member's own current planning descriptor, not another
            // vehicle's speed or a role allocator's selected substitute.
            if (requirementValid && minimum > 0.0
                && ((requirePerformanceEvidence && !declaredPerformanceUsable)
                    || (requirePerformanceEvidence ? declaredPerformance.value(QStringLiteral("maxSpeedMps"))
                        : vehicle.value(QStringLiteral("maxSpeedMps"))).isNull()
                    || !finiteNumber(requirePerformanceEvidence ? declaredPerformance.value(QStringLiteral("maxSpeedMps"))
                        : vehicle.value(QStringLiteral("maxSpeedMps")), &maximum) || maximum < 0.0)) {
                appendUnique(&unmet, QStringLiteral("member_speed_evidence_missing"));
                appendRisk(&risks, QStringLiteral("member_speed_evidence_missing"), QStringLiteral("blocking"),
                           QStringLiteral("%1 %2 needs its own finite non-negative maxSpeedMps evidence for the explicit member speed floor.").arg(label, id));
            } else if (requirementValid && minimum > 0.0 && maximum < minimum) {
                appendUnique(&unmet, QStringLiteral("member_speed_requirement_unmet"));
                appendRisk(&risks, QStringLiteral("member_speed_requirement_unmet"), QStringLiteral("blocking"),
                           QStringLiteral("%1 %2 maxSpeedMps is below this member's minimumSpeedMps; no other group member can satisfy it.").arg(label, id));
            }
        }
        if (!capabilityEvidenceRequired) return;
        if (requireConfigurationBaseline
            && !FleetCapabilityEvidence::currentConfigurationBaseline(vehicle)) {
            appendUnique(&unmet, QStringLiteral("configuration_not_approved"));
            appendRisk(&risks, QStringLiteral("configuration_not_approved"), QStringLiteral("blocking"),
                       QStringLiteral("%1 %2 lacks a current complete typed configuration matching its native local approved baseline; a legacy hash label or imported approval cannot satisfy this requirement.").arg(label, id));
        }
        QStringList available;
        bool reviewCurrent = false;
        const bool capabilitiesUsable = planningCapabilityEvidence(vehicle, requireVerifiedCapabilities, &available, &reviewCurrent);
        if (!capabilitiesUsable) {
            const QString code = requireVerifiedCapabilities && !reviewCurrent
                ? QStringLiteral("capability_review_not_current") : QStringLiteral("invalid_vehicle_capability_evidence");
            appendUnique(&unmet, code);
            appendRisk(&risks, code, QStringLiteral("blocking"),
                       QStringLiteral("%1 %2 lacks current consistent local capability review/evidence; old verified labels cannot satisfy the task.").arg(label, id));
            return;
        }
        for (const QString& token : required) {
            if (available.contains(token)) continue;
            appendUnique(&unmet, QStringLiteral("required_capability_missing"));
            appendRisk(&risks, QStringLiteral("required_capability_missing"), QStringLiteral("blocking"),
                       QStringLiteral("%1 %2 does not satisfy required capability %3 for its resolved assignment.").arg(label, id, token));
        }
    };
    for (const QVariant& member : members) {
        if (cancelled()) return cancelledResult();
        enforceCapabilities(member.toMap(), false);
    }
    for (const QVariant& reserve : reserveMembers) {
        if (cancelled()) return cancelledResult();
        enforceCapabilities(reserve.toMap(), true);
    }

    if (cancelled()) return cancelledResult();
    // Check the actual final nominal paths, not only generated survey legs.
    // Remainder proposals retain area but no longer request coverage, and
    // auxiliary/global-fallback/reserve-budget paths can cross its holes too.
    // These value-only copies neither add assignments nor authorize reserves.
    QVariantList exclusionMembers = reserveBudgetMembers;
    for (const QVariant& value : reserveMembers) {
        if (cancelled()) return cancelledResult();
        const QVariantMap reserve = value.toMap();
        const QVariantMap budget = reserve.value(QStringLiteral("energyPlan")).toMap();
        if (budget.value(QStringLiteral("complete")).type() != QVariant::Bool
            || !budget.value(QStringLiteral("complete")).toBool()) continue; // already blocking above
        exclusionMembers.append(QVariantMap{
            {QStringLiteral("vehicleId"), reserve.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("waypoints"), budget.value(QStringLiteral("outboundPath"))},
            {QStringLiteral("returnPath"), budget.value(QStringLiteral("returnPath"))}});
    }
    const QVariantMap exclusionCheck = FleetAreaCoveragePlanner::validateExclusionRoutes(
        task.value(QStringLiteral("area")), exclusionMembers);
    if (exclusionCheck.value(QStringLiteral("ok")).type() != QVariant::Bool
        || !exclusionCheck.value(QStringLiteral("ok")).toBool()) {
        appendUnique(&unmet, QStringLiteral("route_exclusion_check_failed"));
        appendRisk(&risks, QStringLiteral("route_exclusion_check_failed"), QStringLiteral("blocking"),
                   QStringLiteral("The complete nominal outbound/return routes and their join must respect the retained area's exclusion interiors and any positive boundary setback, including original outer-boundary containment: %1. This is not obstacle clearance or execution permission.")
                       .arg(exclusionCheck.value(QStringLiteral("reason")).toString().left(160)));
    }
    const QVariantMap boundaryResult = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
        task.value(QStringLiteral("area")), exclusionMembers);
    const bool boundaryPolicyValid = boundaryResult.value(QStringLiteral("ok")).type() == QVariant::Bool
        && boundaryResult.value(QStringLiteral("ok")).toBool();
    const QVariantMap routeBoundaryPolicy = boundaryResult.value(QStringLiteral("policy")).toMap();
    if (!boundaryPolicyValid) {
        appendUnique(&unmet, QStringLiteral("route_boundary_policy_invalid"));
        appendRisk(&risks, QStringLiteral("route_boundary_policy_invalid"), QStringLiteral("blocking"),
            QStringLiteral("The complete original boundary policy must survive compilation; malformed or missing geometry cannot be replaced by an inactive declaration: %1.")
                .arg(boundaryResult.value(QStringLiteral("reason")).toString().left(160)));
    }
    // Derive from the FINAL actual member geometry and explicit normalized
    // profile. This key is never copied from a caller's assignment. Global
    // inheritance, when allowed, has already resolved the actual member.
    for (int index = 0; index < members.size(); ++index) {
        if (cancelled()) return cancelledResult();
        QVariantMap member = members.at(index).toMap();
        member.insert(QStringLiteral("nominalRouteTiming"), FleetMissionPreviewTimeline::nominalRouteTiming(member));
        if (cancelled()) return cancelledResult();
        members[index] = member;
    }
    if (cancelled()) return cancelledResult();
    QVariantMap identity;
    identity.insert(QStringLiteral("schemaVersion"), 1);
    identity.insert(QStringLiteral("task"), task);
    identity.insert(QStringLiteral("fleet"), fleet);
    identity.insert(QStringLiteral("waypoints"), waypoints);
    if (!globalRouteMetadata.isEmpty()) identity.insert(QStringLiteral("routeTiming"), globalRouteMetadata);
    identity.insert(QStringLiteral("members"), members);
    if (taskProfileProvided) identity.insert(QStringLiteral("executionProfile"), taskExecutionProfile);
    if (coverageRequested) identity.insert(QStringLiteral("coveragePlan"), coveragePlan);
    if (boundaryPolicyValid) identity.insert(QStringLiteral("routeBoundaryPolicy"), routeBoundaryPolicy);
    identity.insert(QStringLiteral("reserveVehicleIds"), reserveIds);
    identity.insert(QStringLiteral("reserveMembers"), reserveMembers);
    if (requirePerformanceEvidence) {
        identity.insert(QStringLiteral("performanceEvidencePolicy"), QStringLiteral("current-local-engineering-declaration-v1"));
        identity.insert(QStringLiteral("declaredPerformanceSources"), declaredPerformanceSources);
    }
    const QByteArray digest = QCryptographicHash::hash(
        QJsonDocument::fromVariant(identity).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex();
    if (cancelled()) return cancelledResult();

    QVariantMap plan;
    plan.insert(QStringLiteral("schemaVersion"), 1);
    plan.insert(QStringLiteral("planVersion"), QStringLiteral("1.1"));
    plan.insert(QStringLiteral("compilerVersion"), requirePerformanceEvidence ? QStringLiteral("0.8.0") : QStringLiteral("0.7.0"));
    if (requirePerformanceEvidence) plan.insert(QStringLiteral("requirePerformanceEvidence"), true);
    else if (!planningSource.isEmpty()
        && planningSource.value(QStringLiteral("settings")).toMap().contains(QStringLiteral("requirePerformanceEvidence"))) {
        // A schema-5 area's explicit false is retained as an editing policy,
        // not defaulted into legacy or source-less task outputs. The complete
        // task/source comparison above already required this actual Bool.
        plan.insert(QStringLiteral("requirePerformanceEvidence"), planningSource.value(QStringLiteral("settings"))
            .toMap().value(QStringLiteral("requirePerformanceEvidence")));
    }
    // Preserve explicit floors, including zero, in the actual archived plan.
    // The complete original task already participates in planId above.
    for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")})
        if (task.contains(key)) plan.insert(key, task.value(key));
    plan.insert(QStringLiteral("planId"), QString::fromLatin1(digest));
    plan.insert(QStringLiteral("sourceTaskId"), firstString(task, QStringList() << QStringLiteral("taskId") << QStringLiteral("id")));
    if (!planningSource.isEmpty()) plan.insert(QStringLiteral("planningSource"), planningSource);
    plan.insert(QStringLiteral("intent"), task.value(QStringLiteral("intent")));
    plan.insert(QStringLiteral("areaSummary"), area);
    if (boundaryPolicyValid) plan.insert(QStringLiteral("routeBoundaryPolicy"), routeBoundaryPolicy);
    plan.insert(QStringLiteral("target"), target);
    plan.insert(QStringLiteral("waypoints"), waypoints);
    plan.insert(QStringLiteral("waypointCount"), waypoints.size());
    if (!globalRouteMetadata.isEmpty()) plan.insert(QStringLiteral("routeTiming"), globalRouteMetadata);
    if (globalRouteMaterialized)
        for (const QString& key : inheritedTaskFields())
            if (task.contains(key)) plan.insert(key, task.value(key));
    plan.insert(QStringLiteral("members"), members);
    plan.insert(QStringLiteral("assignments"), members);
    if (taskProfileProvided) plan.insert(QStringLiteral("executionProfile"), taskExecutionProfile);
    if (coverageRequested) plan.insert(QStringLiteral("coveragePlan"), coveragePlan);
    plan.insert(QStringLiteral("fleetProvided"), fleetProvided);
    plan.insert(QStringLiteral("planningOnly"), !fleetProvided);
    plan.insert(QStringLiteral("reserveVehicleIds"), reserveIds);
    plan.insert(QStringLiteral("reserveMembers"), reserveMembers);
    // Keep explicitly malformed input visible. A conversion to an empty map
    // must not turn an unknown constraints source into an unconstrained plan.
    plan.insert(QStringLiteral("constraints"), invalidConstraints ? originalConstraints : QVariant(constraints));
    if (!remainderPlanningSource.isEmpty()) {
        if (FleetMissionRemainderPlanner::matchesPlan(remainderPlanningSource, plan)) {
            plan.insert(QStringLiteral("remainderPlanningSource"), remainderPlanningSource);
        } else {
            appendUnique(&unmet, QStringLiteral("remainder_plan_projection_mismatch"));
            appendRisk(&risks, QStringLiteral("remainder_plan_projection_mismatch"), QStringLiteral("blocking"),
                QStringLiteral("The compiled single-member route, dwell, profile, estimates and constraints must match the confirmed remaining-route proposal; normalization cannot silently change its scope."));
        }
    }
    if (!groupRemainderPlanningSource.isEmpty()) {
        if (FleetMissionGroupRemainderPlanner::matchesPlan(groupRemainderPlanningSource, plan)) {
            plan.insert(QStringLiteral("groupRemainderPlanningSource"), groupRemainderPlanningSource);
        } else {
            appendUnique(&unmet, QStringLiteral("group_remainder_plan_projection_mismatch"));
            appendRisk(&risks, QStringLiteral("group_remainder_plan_projection_mismatch"), QStringLiteral("blocking"),
                QStringLiteral("Every compiled group member, route, dwell, profile and shared constraint must match the complete confirmed group proposal; no successful prefix or changed member scope is accepted."));
        }
    }
    if (!reserveDispatchPlanningSource.isEmpty()) {
        if (FleetReserveDispatchPlanner::matchesPlan(reserveDispatchPlanningSource, plan)) {
            plan.insert(QStringLiteral("reserveDispatchPlanningSource"), reserveDispatchPlanningSource);
        } else {
            appendUnique(&unmet, QStringLiteral("reserve_dispatch_plan_projection_mismatch"));
            appendRisk(&risks, QStringLiteral("reserve_dispatch_plan_projection_mismatch"), QStringLiteral("blocking"),
                QStringLiteral("The actual compiled reserve-dispatch member, complete outbound and return routes, dwell, execution profile, constraints and performance policy must match the confirmed new task; no changed or partial projection is accepted."));
        }
    }
    const QVariantMap clearance = FleetMissionClearanceInput::inspectForPlan(plan);
    if (cancelled()) return cancelledResult();
    if (invalidConstraints) {
        appendUnique(&unmet, QStringLiteral("invalid_constraints"));
        appendRisk(&risks, QStringLiteral("invalid_constraints"), QStringLiteral("blocking"),
            QStringLiteral("Explicit constraints must be a non-null map; the original malformed value is retained for diagnosis."));
    } else if (constraints.contains(QStringLiteral("clearanceInput"))) {
        const auto affirmative = [&clearance](const QString& key) {
            const QVariant value = clearance.value(key);
            return value.type() == QVariant::Bool && !value.isNull() && value.toBool();
        };
        if (!affirmative(QStringLiteral("provided")) || !affirmative(QStringLiteral("inputValid"))
            || !affirmative(QStringLiteral("memberIdsMatch"))) {
            const QString reason = clearance.value(QStringLiteral("reason")).toString();
            const QString code = reason.isEmpty() ? QStringLiteral("clearance_input_invalid") : reason;
            appendUnique(&unmet, code);
            appendRisk(&risks, code, QStringLiteral("blocking"),
                QStringLiteral("The complete engineering clearance declaration must be valid and match every active member in order; it is not repaired, reassigned or accepted as a partial prefix."));
        } else {
            appendUnique(&unmet, QStringLiteral("clearance_geometry_review_pending"));
            appendRisk(&risks, QStringLiteral("clearance_geometry_review_pending"), QStringLiteral("blocking"),
                QStringLiteral("Engineering clearance requires a complete declared-model planning result and separately bound native startup/contact evidence; neither input validation nor a planning-only result authorizes startup."));
        }
    }
    plan.insert(QStringLiteral("risks"), risks);
    plan.insert(QStringLiteral("riskSignals"), risks);
    plan.insert(QStringLiteral("unmetRequirements"), QVariant::fromValue(unmet));
    plan.insert(QStringLiteral("feasible"), unmet.isEmpty());
    plan.insert(QStringLiteral("reviewStatus"), unmet.isEmpty()
        ? QStringLiteral("ready-for-review") : QStringLiteral("blocked-review"));
    plan.insert(QStringLiteral("manualConfirmationRequired"), true);
    plan.insert(QStringLiteral("requiresOperatorReview"), true);
    plan.insert(QStringLiteral("flightCommandReleased"), false);
    plan.insert(QStringLiteral("dispatch"), QStringLiteral("not-released"));
    plan.insert(QStringLiteral("compilerLimitations"), QStringList()
        << QStringLiteral("Opt-in area coverage generates review-only geometric routes; it proves neither sensor coverage nor flyability.")
        << QStringLiteral("No terrain, obstacle avoidance, dynamics optimization, or collision clearance is certified by this compiler.")
        << QStringLiteral("Explicit execution profiles and waypoint dwell remain requirements, not proof of firmware encoding, compatible live parameters, synchronized timing, or execution permission.")
        << QStringLiteral("No Vehicle, MAVLink, mission upload, or command dispatch is invoked."));

    if (clearance.value(QStringLiteral("provided")).type() == QVariant::Bool
        && !clearance.value(QStringLiteral("provided")).isNull() && clearance.value(QStringLiteral("provided")).toBool()
        && clearance.value(QStringLiteral("inputValid")).type() == QVariant::Bool
        && !clearance.value(QStringLiteral("inputValid")).isNull() && clearance.value(QStringLiteral("inputValid")).toBool()
        && clearance.value(QStringLiteral("memberIdsMatch")).type() == QVariant::Bool
        && !clearance.value(QStringLiteral("memberIdsMatch")).isNull() && clearance.value(QStringLiteral("memberIdsMatch")).toBool()) {
        // Use the complete unchanged plan, after its non-release envelope is
        // formed. This source has NO fabricated current position or startup
        // view. Archived no-fly-zone review is a separate Coordinator gate.
        if (cancelled()) return cancelledResult();
        const QVariantMap geometry = FleetMissionJointSpatialReview::buildPlanning(plan, {});
        if (cancelled()) return cancelledResult();
        QVariantMap planningReview{{QStringLiteral("ok"), false},
            {QStringLiteral("error"), QStringLiteral("clearance-planning-geometry-source-unavailable")},
            {QStringLiteral("report"), QVariantMap()}};
        const QVariant geometryOk = geometry.value(QStringLiteral("ok"));
        if (geometryOk.type() == QVariant::Bool && !geometryOk.isNull() && geometryOk.toBool()
            && geometry.value(QStringLiteral("view")).type() == QVariant::Map
            && !geometry.value(QStringLiteral("view")).isNull()) {
            planningReview = FleetMissionClearanceReview::buildForGeometryBasisCancelable(
                clearance.value(QStringLiteral("input")), geometry.value(QStringLiteral("view")).toMap(), cancellation);
        }
        if (cancelled() || planningReview.value(QStringLiteral("error")).toString() == QStringLiteral("clearance-review-cancelled"))
            return cancelledResult();
        plan.insert(QStringLiteral("planningClearanceReview"), planningReview);
        const QVariant resultOk = planningReview.value(QStringLiteral("ok"));
        const QVariantMap report = planningReview.value(QStringLiteral("report")).toMap();
        const QVariant modelSatisfied = report.value(QStringLiteral("declaredModelSatisfied"));
        if (resultOk.type() != QVariant::Bool || resultOk.isNull() || !resultOk.toBool()) {
            appendUnique(&unmet, QStringLiteral("clearance_planning_review_unavailable"));
            appendRisk(&risks, QStringLiteral("clearance_planning_review_unavailable"), QStringLiteral("blocking"),
                QStringLiteral("The entire encoded planning source or its declared environment calculation is unavailable; no successful route prefix is substituted."));
        } else if (modelSatisfied.type() != QVariant::Bool || modelSatisfied.isNull() || !modelSatisfied.toBool()) {
            appendUnique(&unmet, QStringLiteral("clearance_planning_model_incomplete_or_violated"));
            appendRisk(&risks, QStringLiteral("clearance_planning_model_incomplete_or_violated"), QStringLiteral("blocking"),
                QStringLiteral("The complete encoded planning scope has a declared-envelope violation, an unknown comparison or unresolved terminal contact; see the frozen planning clearance report."));
        }
        // The existing overall pending is intentionally NOT cleared here:
        // native startup binding, contact semantics, datum/configuration and
        // the final write fence are separate unfinished source requirements.
        plan.insert(QStringLiteral("risks"), risks);
        plan.insert(QStringLiteral("riskSignals"), risks);
        plan.insert(QStringLiteral("unmetRequirements"), QVariant::fromValue(unmet));
        plan.insert(QStringLiteral("feasible"), unmet.isEmpty());
        plan.insert(QStringLiteral("reviewStatus"), unmet.isEmpty()
            ? QStringLiteral("ready-for-review") : QStringLiteral("blocked-review"));
    }

    if (cancelled()) return cancelledResult();
    return plan;
}

void FleetMissionCompiler::clear()
{
    if (_lastPlan.isEmpty()) return;
    _lastPlan.clear();
    emit planChanged();
}

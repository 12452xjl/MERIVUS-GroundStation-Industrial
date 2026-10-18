#include "FleetMissionStartupSpatialReview.h"

#include "FleetAirspaceConflictChecker.h"

#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVariantList>

#include <cmath>

namespace {
constexpr int MaximumVehicles = 32;
constexpr int MaximumNodes = 100000;
constexpr int MaximumDepth = 12;
constexpr qint64 MaximumBytes = 2 * 1024 * 1024;

bool nullValue(const QVariant& value)
{
    return !value.isValid() || value.userType() == QMetaType::Nullptr;
}

bool number(const QVariant& value, double* result = nullptr)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double actual = value.toDouble();
    if (!std::isfinite(actual)) return false;
    if (result) *result = actual;
    return true;
}

bool integer(const QVariant& value, double low, double high)
{
    double actual = 0;
    return number(value, &actual) && std::floor(actual) == actual && actual >= low && actual <= high;
}

bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool boolValue(const QVariantMap& map, const QString& key)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull();
}

bool textEquals(const QVariant& value, const QString& expected)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString() == expected;
}

bool exact(const QVariantMap& map, const QStringList& fields)
{
    if (map.size() != fields.size()) return false;
    for (const QString& field : fields) if (!map.contains(field)) return false;
    return true;
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}

bool tree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > MaximumDepth || ++*nodes > MaximumNodes) return false;
    if (nullValue(value)) return true;
    // Native diagnostic QString() values are ordinary empty JSON strings,
    // not opaque nulls. Early/blocked readiness rows legitimately contain them.
    if (value.isNull() && value.type() != QVariant::String) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > MaximumNodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 256) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumBytes || !tree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes) return false;
        for (const QVariant& child : list) if (!tree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        const QStringList list = value.toStringList();
        if (list.size() > MaximumNodes) return false;
        for (const QString& child : list) if (!tree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        *bytes += value.toString().toUtf8().size();
        return *bytes <= MaximumBytes;
    }
    return value.type() == QVariant::Bool || number(value);
}

bool bounded(const QVariantMap& value)
{
    int nodes = 0; qint64 bytes = 0;
    return tree(value, 0, &nodes, &bytes)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= MaximumBytes;
}

QVariantMap failure(const QString& code)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), code}, {QStringLiteral("view"), QVariantMap()}};
}

bool optionalNumber(const QVariant& value) { return nullValue(value) || number(value); }

bool sameNumber(const QVariant& first, const QVariant& second)
{
    return number(first) && number(second) && first.toDouble() == second.toDouble();
}

bool sameOptionalNumber(const QVariant& first, const QVariant& second)
{
    return (nullValue(first) && nullValue(second)) || sameNumber(first, second);
}

bool pointE7(const QVariant& latitude, const QVariant& longitude, const QVariant& altitude, QVariantMap* output)
{
    double height = 0;
    if (!integer(latitude, -900000000, 900000000) || !integer(longitude, -1800000000, 1800000000)
        || !number(altitude, &height)) return false;
    *output = {{QStringLiteral("latitude"), latitude.toDouble() / 1e7},
        {QStringLiteral("longitude"), longitude.toDouble() / 1e7}, {QStringLiteral("altitude"), height}};
    return true;
}

bool currentAltitudeValid(const QVariantMap& map)
{
    if (!exact(map, {QStringLiteral("reportedAltitudeAmslMeters"), QStringLiteral("telemetryAltitudeResolutionMeters"),
            QStringLiteral("nominalEstimateOnly"), QStringLiteral("encodingUpperAmslMeters"),
            QStringLiteral("withinDeclaredBoundary"), QStringLiteral("clearanceCertified")})
        || !flag(map, QStringLiteral("nominalEstimateOnly"), true) || !flag(map, QStringLiteral("clearanceCertified"), false)
        || !boolValue(map, QStringLiteral("withinDeclaredBoundary"))
        || !number(map.value(QStringLiteral("telemetryAltitudeResolutionMeters")))
        || map.value(QStringLiteral("telemetryAltitudeResolutionMeters")).toDouble() != 0.001
        || !optionalNumber(map.value(QStringLiteral("reportedAltitudeAmslMeters")))
        || !optionalNumber(map.value(QStringLiteral("encodingUpperAmslMeters")))) return false;
    const bool bothKnown = number(map.value(QStringLiteral("reportedAltitudeAmslMeters")))
        && number(map.value(QStringLiteral("encodingUpperAmslMeters")));
    return (!flag(map, QStringLiteral("withinDeclaredBoundary"), true) || bothKnown)
        && (!bothKnown || map.value(QStringLiteral("encodingUpperAmslMeters")).toDouble()
            >= map.value(QStringLiteral("reportedAltitudeAmslMeters")).toDouble());
}

bool profilePointValid(const QVariant& value)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    double latitude = 0, longitude = 0;
    return exact(map, {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("latitude"),
            QStringLiteral("longitude"), QStringLiteral("altitude")})
        && textEquals(map.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic"))
        && textEquals(map.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl"))
        && number(map.value(QStringLiteral("latitude")), &latitude) && std::abs(latitude) <= 90
        && number(map.value(QStringLiteral("longitude")), &longitude) && std::abs(longitude) <= 180
        && number(map.value(QStringLiteral("altitude")));
}

bool takeoffValid(const QVariantMap& map)
{
    if (exact(map, {QStringLiteral("derived"), QStringLiteral("branch"), QStringLiteral("clearanceCertified"), QStringLiteral("actualPathCertified")}))
        return flag(map, QStringLiteral("derived"), false) && flag(map, QStringLiteral("clearanceCertified"), false)
            && flag(map, QStringLiteral("actualPathCertified"), false)
            && textEquals(map.value(QStringLiteral("branch")), QStringLiteral("not-computed"));
    if (!exact(map, {QStringLiteral("derived"), QStringLiteral("branch"), QStringLiteral("nominalEstimateOnly"),
            QStringLiteral("telemetryAltitudeResolutionMeters"), QStringLiteral("telemetryEncodingBoundsAvailable"),
            QStringLiteral("ceilingCheckUsesConservativeUpperBound"), QStringLiteral("positionSourceResolution"),
            QStringLiteral("requestedProfileCoordinate"), QStringLiteral("requestedMissionSequence"),
            QStringLiteral("requestedAltitudeAmslMeters"), QStringLiteral("observedAltitudeAmslMeters"),
            QStringLiteral("observedAltitudeFloat32Meters"), QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("parameterFloat32Meters"),
            QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters"), QStringLiteral("observedAltitudeEncodingLowerMeters"),
            QStringLiteral("observedAltitudeEncodingUpperMeters"), QStringLiteral("predictedTakeoffLowerAmslMeters"),
            QStringLiteral("predictedTakeoffUpperAmslMeters"), QStringLiteral("maxAltitudeAmslMeters"),
            QStringLiteral("withinDeclaredCeiling"), QStringLiteral("currentXyDiffersFromRequestedXy"),
            QStringLiteral("currentXyDiffersFromProfileXy"), QStringLiteral("clearanceCertified"),
            QStringLiteral("actualPathCertified"), QStringLiteral("verticalSegment"), QStringLiteral("subsequentConnection")})) return false;
    for (const QString& key : {QStringLiteral("derived"), QStringLiteral("telemetryEncodingBoundsAvailable"),
            QStringLiteral("withinDeclaredCeiling"), QStringLiteral("currentXyDiffersFromRequestedXy"), QStringLiteral("currentXyDiffersFromProfileXy")})
        if (!boolValue(map, key)) return false;
    if (!flag(map, QStringLiteral("nominalEstimateOnly"), true) || !flag(map, QStringLiteral("clearanceCertified"), false)
        || !flag(map, QStringLiteral("actualPathCertified"), false) || !flag(map, QStringLiteral("ceilingCheckUsesConservativeUpperBound"), true)
        || !textEquals(map.value(QStringLiteral("branch")), QStringLiteral("landed-current-global-alt-plus-MIS_TAKEOFF_ALT"))
        || !textEquals(map.value(QStringLiteral("positionSourceResolution")), QStringLiteral("reported-global-position-mm-and-E7"))
        || !number(map.value(QStringLiteral("telemetryAltitudeResolutionMeters")))
        || map.value(QStringLiteral("telemetryAltitudeResolutionMeters")).toDouble() != 0.001
        || !integer(map.value(QStringLiteral("requestedMissionSequence")), 0, 2047)
        || !profilePointValid(map.value(QStringLiteral("requestedProfileCoordinate")))) return false;
    for (const QString& key : {QStringLiteral("requestedAltitudeAmslMeters"), QStringLiteral("observedAltitudeAmslMeters"),
            QStringLiteral("observedAltitudeFloat32Meters"), QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("parameterFloat32Meters"),
            QStringLiteral("maxAltitudeAmslMeters")}) if (!number(map.value(key))) return false;
    if (map.value(QStringLiteral("MIS_TAKEOFF_ALT")).toDouble() < 0 || map.value(QStringLiteral("parameterFloat32Meters")).toDouble() < 0) return false;
    bool boundsKnown = true;
    for (const QString& key : {QStringLiteral("observedAltitudeEncodingLowerMeters"), QStringLiteral("observedAltitudeEncodingUpperMeters"),
            QStringLiteral("predictedTakeoffLowerAmslMeters"), QStringLiteral("predictedTakeoffUpperAmslMeters")}) {
        if (!optionalNumber(map.value(key))) return false;
        boundsKnown = boundsKnown && number(map.value(key));
    }
    if (!optionalNumber(map.value(QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters")))
        || (flag(map, QStringLiteral("telemetryEncodingBoundsAvailable"), true) != boundsKnown)
        || (flag(map, QStringLiteral("derived"), true)
            && (!boundsKnown || !number(map.value(QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters")))))) return false;
    if (boundsKnown && (map.value(QStringLiteral("observedAltitudeEncodingLowerMeters")).toDouble()
            > map.value(QStringLiteral("observedAltitudeEncodingUpperMeters")).toDouble()
        || map.value(QStringLiteral("predictedTakeoffLowerAmslMeters")).toDouble()
            > map.value(QStringLiteral("predictedTakeoffUpperAmslMeters")).toDouble())) return false;
    if (flag(map, QStringLiteral("withinDeclaredCeiling"), true) && (!flag(map, QStringLiteral("derived"), true)
        || map.value(QStringLiteral("predictedTakeoffUpperAmslMeters")).toDouble() > map.value(QStringLiteral("maxAltitudeAmslMeters")).toDouble())) return false;
    const QVariant verticalValue = map.value(QStringLiteral("verticalSegment")), connectionValue = map.value(QStringLiteral("subsequentConnection"));
    if (verticalValue.type() != QVariant::Map || connectionValue.type() != QVariant::Map) return false;
    const QVariantMap vertical = verticalValue.toMap(), connection = connectionValue.toMap();
    if (!exact(vertical, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("fromAltitudeAmslMeters"),
            QStringLiteral("nominalToAltitudeAmslMeters"), QStringLiteral("nominalEstimateOnly")})
        || !exact(connection, {QStringLiteral("fromLatitudeE7"), QStringLiteral("fromLongitudeE7"), QStringLiteral("nominalFromAltitudeAmslMeters"),
            QStringLiteral("toLatitudeE7"), QStringLiteral("toLongitudeE7"), QStringLiteral("toAltitudeAmslMeters"),
            QStringLiteral("nominalEstimateOnly"), QStringLiteral("clearanceCertified")})
        || !flag(vertical, QStringLiteral("nominalEstimateOnly"), true)
        || !flag(connection, QStringLiteral("nominalEstimateOnly"), true) || !flag(connection, QStringLiteral("clearanceCertified"), false)) return false;
    QVariantMap ignored;
    if (!pointE7(vertical.value(QStringLiteral("latitudeE7")), vertical.value(QStringLiteral("longitudeE7")),
            vertical.value(QStringLiteral("fromAltitudeAmslMeters")), &ignored)
        || !integer(connection.value(QStringLiteral("fromLatitudeE7")), -900000000, 900000000)
        || !integer(connection.value(QStringLiteral("fromLongitudeE7")), -1800000000, 1800000000)
        || !pointE7(connection.value(QStringLiteral("toLatitudeE7")), connection.value(QStringLiteral("toLongitudeE7")),
            connection.value(QStringLiteral("toAltitudeAmslMeters")), &ignored)
        || !optionalNumber(vertical.value(QStringLiteral("nominalToAltitudeAmslMeters")))
        || !optionalNumber(connection.value(QStringLiteral("nominalFromAltitudeAmslMeters")))) return false;
    return sameNumber(vertical.value(QStringLiteral("latitudeE7")), connection.value(QStringLiteral("fromLatitudeE7")))
        && sameNumber(vertical.value(QStringLiteral("longitudeE7")), connection.value(QStringLiteral("fromLongitudeE7")))
        && sameNumber(vertical.value(QStringLiteral("fromAltitudeAmslMeters")), map.value(QStringLiteral("observedAltitudeAmslMeters")))
        && sameOptionalNumber(vertical.value(QStringLiteral("nominalToAltitudeAmslMeters")), map.value(QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters")))
        && sameOptionalNumber(connection.value(QStringLiteral("nominalFromAltitudeAmslMeters")), map.value(QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters")))
        && sameNumber(connection.value(QStringLiteral("toAltitudeAmslMeters")), map.value(QStringLiteral("requestedAltitudeAmslMeters")));
}

bool firstWaypointValid(const QVariantMap& map)
{
    QVariantMap ignored;
    return exact(map, {QStringLiteral("fromLatitudeE7"), QStringLiteral("fromLongitudeE7"), QStringLiteral("reportedFromAltitudeAmslMeters"),
            QStringLiteral("toLatitudeE7"), QStringLiteral("toLongitudeE7"), QStringLiteral("toAltitudeAmslMeters"),
            QStringLiteral("missionSequence"), QStringLiteral("clearanceCertified"), QStringLiteral("nominalTelemetryGeometryOnly")})
        && flag(map, QStringLiteral("clearanceCertified"), false) && flag(map, QStringLiteral("nominalTelemetryGeometryOnly"), true)
        && integer(map.value(QStringLiteral("missionSequence")), 0, 2047)
        && pointE7(map.value(QStringLiteral("fromLatitudeE7")), map.value(QStringLiteral("fromLongitudeE7")), map.value(QStringLiteral("reportedFromAltitudeAmslMeters")), &ignored)
        && pointE7(map.value(QStringLiteral("toLatitudeE7")), map.value(QStringLiteral("toLongitudeE7")), map.value(QStringLiteral("toAltitudeAmslMeters")), &ignored);
}

bool selectedMap(const QVariantMap& source, const QString& key, QVariantMap* output)
{
    const QVariant value = source.value(key);
    if (nullValue(value)) return true;
    if (value.type() != QVariant::Map || value.isNull()) return false;
    *output = value.toMap();
    return true;
}

QVariantMap segment(int vehicleId, const QString& kind, const QVariantMap& from, const QVariantMap& to)
{
    return {{QStringLiteral("segmentId"), QStringLiteral("uav-%1/%2").arg(vehicleId).arg(kind)},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("kind"), kind}, {QStringLiteral("from"), from}, {QStringLiteral("to"), to},
        {QStringLiteral("nominalOnly"), true}, {QStringLiteral("actualPathCertified"), false}, {QStringLiteral("timingKnown"), false}};
}

QVariantMap buildValue(const QVariantMap& source, bool geometryBasis = false)
{
    if (!bounded(source)) return failure(QStringLiteral("startup-spatial-input-resource-or-type-limit"));
    const bool identityValid = geometryBasis
        ? exact(source, {QStringLiteral("reviewKind"), QStringLiteral("geometryBasis"),
              QStringLiteral("vehicles"), QStringLiteral("minimumSeparationMeters")})
            && digest(source.value(QStringLiteral("geometryBasis")))
        : exact(source, {QStringLiteral("reviewKind"), QStringLiteral("reviewId"), QStringLiteral("reviewFingerprint"),
              QStringLiteral("vehicles"), QStringLiteral("minimumSeparationMeters")})
            && uuid(source.value(QStringLiteral("reviewId"))) && digest(source.value(QStringLiteral("reviewFingerprint")));
    if (!identityValid
        || (!textEquals(source.value(QStringLiteral("reviewKind")), QStringLiteral("normal"))
            && !textEquals(source.value(QStringLiteral("reviewKind")), QStringLiteral("ground")))
        || source.value(QStringLiteral("vehicles")).type() != QVariant::List)
        return failure(QStringLiteral("startup-spatial-source-envelope-invalid"));
    const QVariantList inputRows = source.value(QStringLiteral("vehicles")).toList();
    if (inputRows.isEmpty() || inputRows.size() > MaximumVehicles) return failure(QStringLiteral("startup-spatial-whole-group-required"));
    QSet<int> seen;
    QVariantList rows, segments;
    int availableCount = 0, partialCount = 0, unavailableCount = 0;
    for (const QVariant& value : inputRows) {
        const QVariantMap input = value.toMap();
        if (value.type() != QVariant::Map || !exact(input, {QStringLiteral("vehicleId"), QStringLiteral("readinessSummary")})
            || !integer(input.value(QStringLiteral("vehicleId")), 1, 255)
            || input.value(QStringLiteral("readinessSummary")).type() != QVariant::Map)
            return failure(QStringLiteral("startup-spatial-target-shape-invalid"));
        const int id = input.value(QStringLiteral("vehicleId")).toInt();
        if (seen.contains(id)) return failure(QStringLiteral("startup-spatial-duplicate-target"));
        seen.insert(id);
        const QVariantMap readiness = input.value(QStringLiteral("readinessSummary")).toMap();
        const QString launch = textEquals(readiness.value(QStringLiteral("launchType")), QStringLiteral("takeoff")) ? QStringLiteral("takeoff")
            : textEquals(readiness.value(QStringLiteral("launchType")), QStringLiteral("none")) ? QStringLiteral("none") : QStringLiteral("unknown");
        QVariantMap altitude, takeoff, first;
        QVariantList reasons, localSegments, ids;
        bool valid = selectedMap(readiness, QStringLiteral("currentPositionAltitudeSummary"), &altitude)
            && selectedMap(readiness, QStringLiteral("takeoffDerivation"), &takeoff)
            && selectedMap(readiness, QStringLiteral("currentPositionToFirstWaypoint"), &first);
        valid = valid && (altitude.isEmpty() || currentAltitudeValid(altitude))
            && (takeoff.isEmpty() || takeoffValid(takeoff)) && (first.isEmpty() || firstWaypointValid(first));
        if (launch == QStringLiteral("none") && takeoff.size() > 4) valid = false;
        if (valid && !altitude.isEmpty() && !first.isEmpty()
            && number(altitude.value(QStringLiteral("reportedAltitudeAmslMeters")))
            && altitude.value(QStringLiteral("reportedAltitudeAmslMeters")) != first.value(QStringLiteral("reportedFromAltitudeAmslMeters"))) valid = false;
        if (valid && takeoff.size() == 27 && !first.isEmpty()) {
            const QVariantMap vertical = takeoff.value(QStringLiteral("verticalSegment")).toMap();
            valid = vertical.value(QStringLiteral("latitudeE7")) == first.value(QStringLiteral("fromLatitudeE7"))
                && vertical.value(QStringLiteral("longitudeE7")) == first.value(QStringLiteral("fromLongitudeE7"))
                && vertical.value(QStringLiteral("fromAltitudeAmslMeters")) == first.value(QStringLiteral("reportedFromAltitudeAmslMeters"));
        }
        const bool altitudeKnown = !altitude.isEmpty() && number(altitude.value(QStringLiteral("reportedAltitudeAmslMeters")));
        const bool derived = takeoff.size() == 27 && flag(takeoff, QStringLiteral("derived"), true);
        if (!valid) reasons.append(QStringLiteral("readiness-spatial-shape-or-consistency-invalid"));
        if (launch == QStringLiteral("unknown")) reasons.append(QStringLiteral("launch-type-unavailable"));
        if (!altitudeKnown) reasons.append(QStringLiteral("current-amsl-altitude-unavailable"));
        if (first.isEmpty()) reasons.append(QStringLiteral("current-first-waypoint-geometry-unavailable"));
        if (launch == QStringLiteral("takeoff") && !derived) reasons.append(QStringLiteral("nominal-takeoff-derivation-unavailable"));
        if (valid && launch != QStringLiteral("unknown")) {
            QVariantMap from, to;
            if (launch == QStringLiteral("takeoff") && derived) {
                const QVariantMap vertical = takeoff.value(QStringLiteral("verticalSegment")).toMap();
                pointE7(vertical.value(QStringLiteral("latitudeE7")), vertical.value(QStringLiteral("longitudeE7")), vertical.value(QStringLiteral("fromAltitudeAmslMeters")), &from);
                pointE7(vertical.value(QStringLiteral("latitudeE7")), vertical.value(QStringLiteral("longitudeE7")), vertical.value(QStringLiteral("nominalToAltitudeAmslMeters")), &to);
                localSegments.append(segment(id, QStringLiteral("vertical-takeoff"), from, to));
                const QVariantMap connection = takeoff.value(QStringLiteral("subsequentConnection")).toMap();
                pointE7(connection.value(QStringLiteral("fromLatitudeE7")), connection.value(QStringLiteral("fromLongitudeE7")), connection.value(QStringLiteral("nominalFromAltitudeAmslMeters")), &from);
                pointE7(connection.value(QStringLiteral("toLatitudeE7")), connection.value(QStringLiteral("toLongitudeE7")), connection.value(QStringLiteral("toAltitudeAmslMeters")), &to);
                localSegments.append(segment(id, QStringLiteral("takeoff-request-connection"), from, to));
            }
            if (!first.isEmpty()) {
                pointE7(first.value(QStringLiteral("fromLatitudeE7")), first.value(QStringLiteral("fromLongitudeE7")), first.value(QStringLiteral("reportedFromAltitudeAmslMeters")), &from);
                pointE7(first.value(QStringLiteral("toLatitudeE7")), first.value(QStringLiteral("toLongitudeE7")), first.value(QStringLiteral("toAltitudeAmslMeters")), &to);
                localSegments.append(segment(id, QStringLiteral("current-first-waypoint"), from, to));
            }
        }
        const bool all = valid && launch != QStringLiteral("unknown") && altitudeKnown && !first.isEmpty()
            && (launch == QStringLiteral("none") || derived);
        const QString availability = all ? QStringLiteral("available") : !localSegments.isEmpty() ? QStringLiteral("partial") : QStringLiteral("unavailable");
        if (all) ++availableCount; else if (!localSegments.isEmpty()) ++partialCount; else ++unavailableCount;
        for (const QVariant& entry : localSegments) { segments.append(entry); ids.append(entry.toMap().value(QStringLiteral("segmentId"))); }
        // Only recognized, valid original diagnostic shapes are published. A
        // malformed row keeps its identity and reasons, never a geometry prefix.
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("launchType"), launch},
            {QStringLiteral("availability"), availability}, {QStringLiteral("reasonCodes"), reasons},
            {QStringLiteral("currentAltitudeSummary"), valid ? altitude : QVariantMap()},
            {QStringLiteral("takeoffDerivation"), valid ? takeoff : QVariantMap()}, {QStringLiteral("segmentIds"), ids}});
    }
    const QVariant minimum = source.value(QStringLiteral("minimumSeparationMeters"));
    const QVariantMap separation = geometryBasis
        ? QVariantMap{{QStringLiteral("available"), false},
            {QStringLiteral("reason"), QStringLiteral("not-run-in-geometry-scope-extraction")}}
        : FleetAirspaceConflictChecker::checkNominalStartupSegments(segments, minimum);
    QVariantMap view{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reviewKind"), source.value(QStringLiteral("reviewKind"))},
        {QStringLiteral("vehicles"), rows}, {QStringLiteral("segments"), segments},
        {QStringLiteral("summary"), QVariantMap{{QStringLiteral("vehicleCount"), rows.size()}, {QStringLiteral("availableVehicleCount"), availableCount},
            {QStringLiteral("partialVehicleCount"), partialCount}, {QStringLiteral("unavailableVehicleCount"), unavailableCount},
            {QStringLiteral("segmentCount"), segments.size()}, {QStringLiteral("allTargetGeometryAvailable"), availableCount == rows.size()}}},
        {QStringLiteral("limitations"), QStringList{
            QStringLiteral("caller-must-bind-the-original-native-review-and-target-group-values-are-not-authentication"),
            QStringLiteral("frozen-nominal-geometry-not-live-position-or-actual-flight-path"),
            QStringLiteral("three-segment-kinds-are-independent-hypotheses-not-one-stitched-flight-route"),
            QStringLiteral("takeoff-connection-ends-at-requested-takeoff-point-not-the-first-waypoint-observation"),
            QStringLiteral("amsl-is-not-terrain-clearance-or-relative-height"),
            QStringLiteral("encoding-altitude-bounds-exclude-estimator-bias-wind-latency-and-dynamics"),
            QStringLiteral("no-full-mission-return-rtl-home-path-no-fly-zones-terrain-or-obstacle-review"),
            QStringLiteral("no-timing-no-synchronized-flight-separation-or-start-permit-proof")}},
        {QStringLiteral("minimumSeparationMeters"), number(minimum) ? minimum : QVariant()}, {QStringLiteral("separation"), separation},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("clearanceCertified"), false}, {QStringLiteral("timingKnown"), false}};
    if (geometryBasis) {
        view.insert(QStringLiteral("geometryBasis"), source.value(QStringLiteral("geometryBasis")));
        QStringList limitations = view.value(QStringLiteral("limitations")).toStringList();
        limitations.append(QStringLiteral("pre-artifact-geometry-basis-is-not-a-final-review-fingerprint-or-review-acknowledgement"));
        view.insert(QStringLiteral("limitations"), limitations);
    } else {
        view.insert(QStringLiteral("reviewId"), source.value(QStringLiteral("reviewId")));
        view.insert(QStringLiteral("reviewFingerprint"), source.value(QStringLiteral("reviewFingerprint")));
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("view"), view}};
}
} // namespace

QVariantMap FleetMissionStartupSpatialReview::build(const QVariantMap& source)
{
    const QVariantMap result = buildValue(source);
    if (!bounded(result)) return failure(QStringLiteral("startup-spatial-output-resource-or-type-limit"));
    // Qt 5.15 invalid QVariant would become JS undefined and lose its key in
    // JSON.stringify. Preserve explicit unknowns as JSON null instead.
    return QJsonDocument::fromVariant(result).toVariant().toMap();
}

QVariantMap FleetMissionStartupSpatialReview::buildForGeometryBasis(const QVariantMap& source)
{
    const QVariantMap result = buildValue(source, true);
    if (!bounded(result)) return failure(QStringLiteral("startup-spatial-output-resource-or-type-limit"));
    return QJsonDocument::fromVariant(result).toVariant().toMap();
}

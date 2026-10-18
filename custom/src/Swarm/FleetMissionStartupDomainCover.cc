#include "FleetMissionStartupDomainCover.h"

#include "FleetMissionContactModel.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionStartupIntegerDomain.h"
#include "QGCMAVLink.h"

#include <QMetaType>

#include <cmath>
#include <initializer_list>
#include <limits>
#include <utility>

namespace {
using Cover = FleetMissionStartupDomainCover;
using Geometry = Cover::Geometry;
using CancellationToken = Cover::CancellationToken;
constexpr qint64 LatitudeLimit = 800000000;
constexpr qint64 LongitudeLimit = 1800000000;
constexpr qint64 FullLongitude = Q_INT64_C(3600000000);
constexpr qint64 CoordinateLimit = 50000000;
constexpr int MaximumItems = 2048;
constexpr int MaximumCells = 8192;

bool cancelled(const CancellationToken& token)
{
    return token && token->load(std::memory_order_acquire);
}

Cover::Result failure(const QString& error, int segmentCount = 0, int cellCount = 0)
{
    Cover::Result result;
    result.error = error;
    result.segmentCount = segmentCount;
    result.cellCount = cellCount;
    return result;
}

bool exact(const QVariantMap& value, std::initializer_list<const char*> names)
{
    if (value.size() != int(names.size())) return false;
    for (const char* name : names) if (!value.contains(QString::fromLatin1(name))) return false;
    return true;
}

bool mapValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool number(const QVariant& value, double minimum, double maximum, double* result = nullptr)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum) return false;
    if (result) *result = candidate;
    return true;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* result = nullptr)
{
    double candidate = 0.0;
    if (!number(value, double(minimum), double(maximum), &candidate) || std::floor(candidate) != candidate) return false;
    if (result) *result = qint64(candidate);
    return true;
}

bool flag(const QVariantMap& value, const char* name, bool expected)
{
    const QVariant candidate = value.value(QString::fromLatin1(name));
    return candidate.userType() == QMetaType::Bool && !candidate.isNull() && candidate.toBool() == expected;
}

bool constant(const QVariant& value, const char* expected)
{
    return value.userType() == QMetaType::QString && !value.isNull()
        && value.toString() == QString::fromLatin1(expected);
}

bool digest(const QVariant& value)
{
    if (value.userType() != QMetaType::QString || value.isNull() || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

qint64 absolute(qint64 value) { return value < 0 ? -value : value; }
qint64 ceilingDivide(qint64 value, qint64 denominator)
{
    return value / denominator + (value % denominator != 0 ? 1 : 0);
}

qint64 longitudeMillimetresPerE7(qint64 minimumLatitude, qint64 maximumLatitude)
{
    // Both cap centre and query lie inside this COMPLETE latitude interval.
    // For |latitude| <= M, spherical haversine gives distanceMm >=
    // Rmm*cos(M)*wrappedLongitudeE7/900000000. The following integer lower
    // bounds use cos30 > 6/7, cos45 > 7/10, cos60 = 1/2,
    // cos65 > 2/5 and cos80 > 1/7; see the accompanying paper derivation.
    // Choosing a band from the centre alone would not bound the whole cap.
    const qint64 maximumMagnitude = qMax(absolute(minimumLatitude), absolute(maximumLatitude));
    if (maximumMagnitude <= 300000000) return 6;
    if (maximumMagnitude <= 450000000) return 4;
    if (maximumMagnitude <= 600000000) return 3;
    if (maximumMagnitude <= 650000000) return 2;
    return 1; // The caller intersects only with the original +/-80 degree domain.
}

struct Coordinate {
    qint64 latitude = 0;
    qint64 longitude = 0;
};

bool baselineKnown(const QVariantMap& value, Coordinate* coordinate)
{
    return coordinate && exact(value, {"latitudeE7", "longitudeE7", "altitudeAmslMm", "relativeAltitudeMm",
            "velocityNorthCmPerSecond", "velocityEastCmPerSecond", "velocityDownCmPerSecond", "headingCdeg", "headingKnown"})
        && integer(value.value(QStringLiteral("latitudeE7")), -LatitudeLimit, LatitudeLimit, &coordinate->latitude)
        && integer(value.value(QStringLiteral("longitudeE7")), -LongitudeLimit, LongitudeLimit, &coordinate->longitude)
        && integer(value.value(QStringLiteral("altitudeAmslMm")), -1000000, 20000000)
        && integer(value.value(QStringLiteral("relativeAltitudeMm")), -Q_INT64_C(2147483648), Q_INT64_C(2147483647))
        && integer(value.value(QStringLiteral("velocityNorthCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("velocityEastCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("velocityDownCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("headingCdeg")), 0, 35999) && flag(value, "headingKnown", true);
}

bool frameKnown(const QVariantMap& input, QVariantMap* frame)
{
    qint64 schema = 0;
    if (!integer(input.value(QStringLiteral("schemaVersion")), 1, 2, &schema)
        || !(schema == 1 ? exact(input, {"schemaVersion", "scene", "vehicles", "policy"})
                        : exact(input, {"schemaVersion", "scene", "vehicles", "policy", "contacts"}))
        || !mapValue(input.value(QStringLiteral("scene")))) return false;
    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap();
    if (!exact(scene, {"sceneId", "revision", "sourceLabel", "sourceDocumentHash", "acquiredAtUtc", "validUntilUtc",
            "horizontalDatum", "verticalDatumId", "localFrame", "coverageVolumes", "obstacleVolumes", "terrainCells", "corridorVolumes"})
        || !constant(scene.value(QStringLiteral("horizontalDatum")), "wgs84")
        || !mapValue(scene.value(QStringLiteral("localFrame")))) return false;
    const QVariantMap candidate = scene.value(QStringLiteral("localFrame")).toMap();
    qint64 latitude = 0, longitude = 0, latitudeDelta = 0, longitudeDelta = 0;
    if (!exact(candidate, {"modelName", "originLatitudeE7", "originLongitudeE7", "northNanometresPerLatitudeE7",
            "eastNanometresPerLongitudeE7", "maximumLatitudeDeltaE7", "maximumLongitudeDeltaE7"})
        || !constant(candidate.value(QStringLiteral("modelName")), "engineer-declared-local-affine-geodetic-chart")
        || !integer(candidate.value(QStringLiteral("originLatitudeE7")), -LatitudeLimit, LatitudeLimit, &latitude)
        || !integer(candidate.value(QStringLiteral("originLongitudeE7")), -LongitudeLimit, LongitudeLimit, &longitude)
        || !integer(candidate.value(QStringLiteral("maximumLatitudeDeltaE7")), 1, 2000000, &latitudeDelta)
        || !integer(candidate.value(QStringLiteral("maximumLongitudeDeltaE7")), 1, 2000000, &longitudeDelta)
        || !integer(candidate.value(QStringLiteral("northNanometresPerLatitudeE7")), 1, 200000000)
        || !integer(candidate.value(QStringLiteral("eastNanometresPerLongitudeE7")), 1, 200000000)
        || latitude - latitudeDelta < -LatitudeLimit || latitude + latitudeDelta > LatitudeLimit
        || longitude - longitudeDelta < -LongitudeLimit || longitude + longitudeDelta > LongitudeLimit) return false;
    *frame = candidate;
    return true;
}

// The native immutable model already checked the complete payload hash and
// source size. Do not re-prepare it (which would call Qt trig), invent a model
// from a map or interpret this structural recheck as source authentication.
bool sourceKnown(const QVariantMap& source, Coordinate* baseline, QVariantList* wire,
                 QVariantMap* envelope, double* ceiling)
{
    if (!exact(source, {"schemaVersion", "modelName", "sphereRadiusMeters", "baselineGlobalPosition",
            "canonicalWireItems", "startupTransitionPolicy", "maximumAltitudeAmslMeters", "payloadContentHash",
            "snapshotOnly", "numericContainmentProven", "clearanceCertified", "flightCommandReleased"})
        || !integer(source.value(QStringLiteral("schemaVersion")), 1, 1)
        || !constant(source.value(QStringLiteral("modelName")), "qt-qgeocoordinate-startup-corridor-v1")
        || !number(source.value(QStringLiteral("sphereRadiusMeters")), 6371000.0, 6371000.0)
        || !mapValue(source.value(QStringLiteral("baselineGlobalPosition")))
        || !baselineKnown(source.value(QStringLiteral("baselineGlobalPosition")).toMap(), baseline)
        || !mapValue(source.value(QStringLiteral("startupTransitionPolicy")))
        || source.value(QStringLiteral("canonicalWireItems")).userType() != QMetaType::QVariantList
        || source.value(QStringLiteral("canonicalWireItems")).isNull()
        || !number(source.value(QStringLiteral("maximumAltitudeAmslMeters")), -1000.0, 20000.0, ceiling)
        || !digest(source.value(QStringLiteral("payloadContentHash"))) || !flag(source, "snapshotOnly", true)
        || !flag(source, "numericContainmentProven", false) || !flag(source, "clearanceCertified", false)
        || !flag(source, "flightCommandReleased", false)) return false;
    const QVariantMap policy = source.value(QStringLiteral("startupTransitionPolicy")).toMap();
    QVariantMap normalizedPolicy;
    if (!FleetMissionExecutionProfile::normalizeStartupPolicy(policy, &normalizedPolicy) || policy != normalizedPolicy) return false;
    *wire = source.value(QStringLiteral("canonicalWireItems")).toList();
    if (wire->isEmpty() || wire->size() > MaximumItems) return false;
    *envelope = policy.value(QStringLiteral("postModeEnvelope")).toMap();
    return true;
}

bool wireCoordinate(const QVariant& value, int index, Coordinate* coordinate, bool* geographic)
{
    if (!mapValue(value)) return false;
    const QVariantMap row = value.toMap();
    qint64 command = 0;
    if (row.size() != 12 || !integer(row.value(QStringLiteral("sequenceNumber")), index, index)
        || !integer(row.value(QStringLiteral("command")), 0, 65535, &command)
        || row.value(QStringLiteral("autoContinue")).userType() != QMetaType::Bool
        || row.value(QStringLiteral("autoContinue")).isNull()) return false;
    const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
    *geographic = command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_TAKEOFF
        || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
    if ((!speed && !*geographic) || !integer(row.value(QStringLiteral("frame")),
            speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL, speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL)) return false;
    for (const QString& key : {QStringLiteral("param1"), QStringLiteral("param2"), QStringLiteral("param3"), QStringLiteral("param7")}) {
        double candidate = 0.0;
        if (!number(row.value(key), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), &candidate)
            || double(float(candidate)) != candidate) return false;
    }
    QVariant canonicalYaw;
    QString headingError;
    if (!row.contains(QStringLiteral("param4"))
        || !FleetMissionHeadingEncoding::canonicalParam4(int(command), row.value(QStringLiteral("param4")),
            row.value(QStringLiteral("yawMode")), false, canonicalYaw, headingError)) return false;
    const double p1 = row.value(QStringLiteral("param1")).toDouble();
    const double p2 = row.value(QStringLiteral("param2")).toDouble();
    const double p3 = row.value(QStringLiteral("param3")).toDouble();
    const double p7 = row.value(QStringLiteral("param7")).toDouble();
    FleetMissionDwellEncoding::Value dwell;
    if (speed) {
        return p1 == 1.0 && p2 >= 0.1 && p2 <= 300.0 && p3 == -1.0 && p7 == 0.0
            && integer(row.value(QStringLiteral("param5Integer")), 0, 0)
            && integer(row.value(QStringLiteral("param6Integer")), 0, 0);
    }
    return p2 == 0.0 && p3 == 0.0 && p7 >= -1000.0 && p7 <= 20000.0
        && (command == MAV_CMD_NAV_WAYPOINT
            ? FleetMissionDwellEncoding::encode(row.value(QStringLiteral("param1")), dwell) : p1 == 0.0)
        && integer(row.value(QStringLiteral("latitudeE7")), -LatitudeLimit, LatitudeLimit, &coordinate->latitude)
        && integer(row.value(QStringLiteral("longitudeE7")), -LongitudeLimit, LongitudeLimit, &coordinate->longitude);
}

bool appendCell(QVector<Cover::Cell>* cells, qint64 minimumLatitude, qint64 maximumLatitude,
                qint64 minimumLongitude, qint64 maximumLongitude, int segmentIndex, int endpointIndex)
{
    if (cells->size() >= MaximumCells) return false;
    Cover::Cell cell;
    cell.segmentIndex = segmentIndex;
    cell.endpointIndex = endpointIndex;
    cell.minLatE7 = minimumLatitude; cell.maxLatE7 = maximumLatitude;
    cell.minLonE7 = minimumLongitude; cell.maxLonE7 = maximumLongitude;
    cells->append(cell);
    return true;
}

bool appendCap(QVector<Cover::Cell>* cells, const Coordinate& centre, qint64 radiusMm,
               int segmentIndex, int endpointIndex)
{
    const qint64 latitudeRadius = ceilingDivide(radiusMm, 10);
    const qint64 minimumLatitude = qMax(-LatitudeLimit, centre.latitude - latitudeRadius);
    const qint64 maximumLatitude = qMin(LatitudeLimit, centre.latitude + latitudeRadius);
    const qint64 longitudeRadius = ceilingDivide(radiusMm,
        longitudeMillimetresPerE7(minimumLatitude, maximumLatitude));
    // The ideal tube is intersected only with the original legal latitude
    // range, never with the smaller engineering frame. Longitude is circular;
    // retain both encodings at +/-180 degrees when the cap crosses the seam.
    if (longitudeRadius >= LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, LongitudeLimit, segmentIndex, endpointIndex);
    const qint64 lower = centre.longitude - longitudeRadius, upper = centre.longitude + longitudeRadius;
    if (lower <= -LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, upper, segmentIndex, endpointIndex)
            && appendCell(cells, minimumLatitude, maximumLatitude, lower + FullLongitude, LongitudeLimit, segmentIndex, endpointIndex);
    if (upper >= LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, lower, LongitudeLimit, segmentIndex, endpointIndex)
            && appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, upper - FullLongitude, segmentIndex, endpointIndex);
    return appendCell(cells, minimumLatitude, maximumLatitude, lower, upper, segmentIndex, endpointIndex);
}

bool altitudeBounds(double value, const QVariantMap& frame, qint64* lower, qint64* upper)
{
    const QVariantMap point{{QStringLiteral("latitude"), frame.value(QStringLiteral("originLatitudeE7")).toDouble() / 1e7},
        {QStringLiteral("longitude"), frame.value(QStringLiteral("originLongitudeE7")).toDouble() / 1e7},
        {QStringLiteral("altitude"), value}};
    qint64 low[3]{}, high[3]{};
    if (!FleetMissionContactModel::pointMillimetres(point, frame, low, high)) return false;
    *lower = low[2]; *upper = high[2];
    return true;
}

bool project(Cover::Cell* cell, const QVariantMap& frame, double minimumAltitude, double maximumAltitude)
{
    // Positive declared affine scales make the lower and upper raw corners
    // bound every intermediate E7 coordinate. The existing helper verifies
    // the WHOLE raw box fits its chart and uses exact integer mm floor/ceil.
    const QVariantMap first{{QStringLiteral("latitude"), double(cell->minLatE7) / 1e7},
        {QStringLiteral("longitude"), double(cell->minLonE7) / 1e7}, {QStringLiteral("altitude"), minimumAltitude}};
    const QVariantMap last{{QStringLiteral("latitude"), double(cell->maxLatE7) / 1e7},
        {QStringLiteral("longitude"), double(cell->maxLonE7) / 1e7}, {QStringLiteral("altitude"), maximumAltitude}};
    qint64 lower[3]{}, unusedUpper[3]{}, unusedLower[3]{}, upper[3]{};
    if (!FleetMissionContactModel::pointMillimetres(first, frame, lower, unusedUpper)
        || !FleetMissionContactModel::pointMillimetres(last, frame, unusedLower, upper)) return false;
    for (int axis = 0; axis < 3; ++axis)
        if (lower[axis] < -CoordinateLimit || upper[axis] > CoordinateLimit || lower[axis] > upper[axis]) return false;
    Geometry::Polytope reference;
    reference.bounds.minimumEast = lower[0]; reference.bounds.maximumEast = upper[0];
    reference.bounds.minimumNorth = lower[1]; reference.bounds.maximumNorth = upper[1];
    reference.bounds.minimumAltitude = lower[2]; reference.bounds.maximumAltitude = upper[2];
    cell->reference = reference;
    return true;
}
bool projectInteger(Cover::Cell* cell, const QVariantMap& frame, qint64 minimumAltitudeMm, qint64 maximumAltitudeMm)
{
    qint64 lower[3]{}, unusedUpper[3]{}, unusedLower[3]{}, upper[3]{};
    if (!FleetMissionContactModel::integerPointMillimetres(cell->minLatE7, cell->minLonE7,
            minimumAltitudeMm, frame, lower, unusedUpper)
        || !FleetMissionContactModel::integerPointMillimetres(cell->maxLatE7, cell->maxLonE7,
            maximumAltitudeMm, frame, unusedLower, upper)) return false;
    for (int axis = 0; axis < 3; ++axis)
        if (lower[axis] < -CoordinateLimit || upper[axis] > CoordinateLimit || lower[axis] > upper[axis]) return false;
    Geometry::Polytope reference;
    reference.bounds.minimumEast = lower[0]; reference.bounds.maximumEast = upper[0];
    reference.bounds.minimumNorth = lower[1]; reference.bounds.maximumNorth = upper[1];
    reference.bounds.minimumAltitude = lower[2]; reference.bounds.maximumAltitude = upper[2];
    cell->reference = reference;
    return true;
}
} // namespace

FleetMissionStartupDomainCover::Result FleetMissionStartupDomainCover::prepare(
    const FleetMissionStartupGeographicMotion::Ptr& originalModel,
    const QVariantMap& normalizedClearanceInput, const CancellationToken& cancellation)
{
    const QString cancelledError = QStringLiteral("startup-domain-cover-cancelled");
    if (cancelled(cancellation)) return failure(cancelledError);
    if (!originalModel) return failure(QStringLiteral("startup-domain-cover-model-required"));
    QVariantMap frame;
    if (!frameKnown(normalizedClearanceInput, &frame)) return failure(QStringLiteral("startup-domain-cover-input-frame-invalid"));
    const auto integerDomain = originalModel->integerDomain();
    if (integerDomain) {
        // Runtime and cover retain this SAME object, not a descriptor rebuild.
        const QVariantMap source = originalModel->source();
        if (!integer(source.value(QStringLiteral("schemaVersion")), 2, 2)
            || !constant(source.value(QStringLiteral("modelName")), "qt-corridor-with-native-integer-domain-v2")
            || !constant(source.value(QStringLiteral("startupSpatialGuard")), "integer-e7-amsl-mm-v1")
            || !mapValue(source.value(QStringLiteral("integerDomain")))
            || source.value(QStringLiteral("integerDomain")).toMap() != integerDomain->source()
            || integerDomain->segmentCount() < 1 || integerDomain->segmentCount() > MaximumItems
            || integerDomain->cells().isEmpty() || integerDomain->cells().size() > MaximumCells)
            return failure(QStringLiteral("startup-domain-cover-original-integer-domain-invalid"));
        const int segments = integerDomain->segmentCount(), count = integerDomain->cells().size();
        QVector<Cell> cells;
        cells.reserve(count);
        for (const auto& raw : integerDomain->cells()) {
            if (cancelled(cancellation)) return failure(cancelledError, segments, count);
            Cell cell;
            cell.minLatE7 = raw.minLatE7; cell.maxLatE7 = raw.maxLatE7;
            cell.minLonE7 = raw.minLonE7; cell.maxLonE7 = raw.maxLonE7;
            cell.segmentIndex = raw.segmentIndex; cell.endpointIndex = raw.endpointIndex;
            if (cell.segmentIndex < 0 || cell.segmentIndex >= segments || cell.endpointIndex < 0 || cell.endpointIndex > 1
                || !projectInteger(&cell, frame, integerDomain->minimumAltitudeMm(), integerDomain->maximumAltitudeMm()))
                return failure(QStringLiteral("startup-domain-cover-affine-domain-unsupported"), segments, count);
            cells.append(cell);
        }
        if (cancelled(cancellation)) return failure(cancelledError, segments, count);
        Result result;
        result.ok = true; result.segmentCount = segments; result.cellCount = count;
        result.nativeIntegerDomainCovered = true;
        result.cells = std::move(cells);
        return result;
    }
    Coordinate previous;
    QVariantList wire;
    QVariantMap envelope;
    double ceiling = 0.0;
    if (!sourceKnown(originalModel->source(), &previous, &wire, &envelope, &ceiling))
        return failure(QStringLiteral("startup-domain-cover-model-invalid"));
    double minimumAltitude = 0.0, maximumAltitude = 0.0, corridor = 0.0;
    if (!number(envelope.value(QStringLiteral("minAltitudeAmslMeters")), -1000.0, 20000.0, &minimumAltitude)
        || !number(envelope.value(QStringLiteral("maxAltitudeAmslMeters")), -1000.0, 20000.0, &maximumAltitude)
        || !number(envelope.value(QStringLiteral("horizontalCorridorMeters")), 0.01, 1000.0, &corridor))
        return failure(QStringLiteral("startup-domain-cover-model-invalid"));
    maximumAltitude = qMin(maximumAltitude, ceiling);
    if (minimumAltitude > maximumAltitude) return failure(QStringLiteral("startup-domain-cover-slab-invalid"));
    qint64 unusedLower = 0, corridorMm = 0;
    // Extract the exact binary64 * 1000 upper integer without first rounding
    // a floating product. The original contact helper decomposes double bits.
    if (!altitudeBounds(corridor, frame, &unusedLower, &corridorMm) || corridorMm < 1 || corridorMm > 1000000)
        return failure(QStringLiteral("startup-domain-cover-altitude-domain-unsupported"));

    QVector<Cell> rawCells;
    rawCells.reserve(qMin(MaximumCells, wire.size() * 4));
    int segmentCount = 0;
    for (int index = 0; index < wire.size(); ++index) {
        if (cancelled(cancellation)) return failure(cancelledError);
        Coordinate next;
        bool geographic = false;
        if (!wireCoordinate(wire.at(index), index, &next, &geographic))
            return failure(QStringLiteral("startup-domain-cover-model-invalid"));
        if (!geographic) continue; // Speed items do not break the chain.
        const qint64 deltaLatitude = absolute(next.latitude - previous.latitude);
        const qint64 rawLongitude = absolute(next.longitude - previous.longitude);
        const qint64 deltaLongitude = qMin(rawLongitude, FullLongitude - rawLongitude);
        // A meridian + latitude-parallel path is an upper bound for the ideal
        // shortest spherical arc. Each E7 step is strictly below 12 mm.
        // Sum <=3.4e9, product <=40.8e9: all arithmetic fits signed 64 bits.
        const qint64 lengthUpperMm = 12 * (deltaLatitude + deltaLongitude);
        const qint64 radiusMm = ceilingDivide(lengthUpperMm, 2) + corridorMm;
        // Every shortest-arc point lies within half the arc length of an
        // endpoint. Triangle inequality then covers the complete ideal tube
        // by these two enlarged caps; this is not an endpoint-only test.
        if (!appendCap(&rawCells, previous, radiusMm, segmentCount, 0)
            || !appendCap(&rawCells, next, radiusMm, segmentCount, 1))
            return failure(QStringLiteral("startup-domain-cover-cell-resource-limit"));
        ++segmentCount;
        previous = next;
    }
    if (segmentCount < 1 || segmentCount > MaximumItems || rawCells.isEmpty() || rawCells.size() > MaximumCells)
        return failure(QStringLiteral("startup-domain-cover-model-invalid"));
    const int cellCount = rawCells.size();
    if (cancelled(cancellation)) return failure(cancelledError, segmentCount, cellCount);
    for (Cell& cell : rawCells) {
        if (cancelled(cancellation)) return failure(cancelledError, segmentCount, cellCount);
        if (!project(&cell, frame, minimumAltitude, maximumAltitude))
            return failure(QStringLiteral("startup-domain-cover-affine-domain-unsupported"), segmentCount, cellCount);
    }
    if (cancelled(cancellation)) return failure(cancelledError, segmentCount, cellCount);
    Result result;
    result.ok = true;
    result.error = QStringLiteral("");
    result.segmentCount = segmentCount;
    result.cellCount = cellCount;
    result.cells = std::move(rawCells);
    return result;
}

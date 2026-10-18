#include "FleetMissionStartupIntegerDomain.h"

#include "FleetMissionContactModel.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionDwellEncoding.h"
#include "QGCMAVLink.h"

#include <QJsonDocument>
#include <QMetaType>

#include <cmath>
#include <initializer_list>
#include <limits>
#include <utility>

namespace {
using Domain = FleetMissionStartupIntegerDomain;
constexpr qint64 LatitudeLimit = 800000000;
constexpr qint64 LongitudeLimit = 1800000000;
constexpr qint64 FullLongitude = Q_INT64_C(3600000000);
constexpr int MaximumItems = 2048;
constexpr int MaximumCells = 8192;
constexpr int MaximumSourceBytes = 4 * 1024 * 1024;

bool exact(const QVariantMap& value, std::initializer_list<const char*> names)
{
    if (value.size() != int(names.size())) return false;
    for (const char* name : names) if (!value.contains(QString::fromLatin1(name))) return false;
    return true;
}

bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum) return false;
    if (output) *output = candidate;
    return true;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    double candidate = 0.0;
    if (!number(value, double(minimum), double(maximum), &candidate) || std::floor(candidate) != candidate) return false;
    if (output) *output = qint64(candidate);
    return true;
}

struct Coordinate { qint64 latitude = 0, longitude = 0; };

bool baseline(const QVariantMap& value, Coordinate* output)
{
    return output && exact(value, {"latitudeE7", "longitudeE7", "altitudeAmslMm", "relativeAltitudeMm",
            "velocityNorthCmPerSecond", "velocityEastCmPerSecond", "velocityDownCmPerSecond", "headingCdeg", "headingKnown"})
        && integer(value.value(QStringLiteral("latitudeE7")), -LatitudeLimit, LatitudeLimit, &output->latitude)
        && integer(value.value(QStringLiteral("longitudeE7")), -LongitudeLimit, LongitudeLimit, &output->longitude)
        && integer(value.value(QStringLiteral("altitudeAmslMm")), -1000000, 20000000)
        && integer(value.value(QStringLiteral("relativeAltitudeMm")), -Q_INT64_C(2147483648), Q_INT64_C(2147483647))
        && integer(value.value(QStringLiteral("velocityNorthCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("velocityEastCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("velocityDownCmPerSecond")), -32768, 32767)
        && integer(value.value(QStringLiteral("headingCdeg")), 0, 35999)
        && value.value(QStringLiteral("headingKnown")).type() == QVariant::Bool
        && !value.value(QStringLiteral("headingKnown")).isNull() && value.value(QStringLiteral("headingKnown")).toBool();
}

bool envelope(const QVariantMap& value, double* lower, double* upper, double* corridor)
{
    return exact(value, {"minAltitudeAmslMeters", "maxAltitudeAmslMeters", "horizontalCorridorMeters",
            "maxHorizontalSpeedMetersPerSecond", "maxClimbSpeedMetersPerSecond", "maxDescentSpeedMetersPerSecond",
            "maxRelativeAltitudeMagnitudeMeters"})
        && number(value.value(QStringLiteral("minAltitudeAmslMeters")), -1000.0, 20000.0, lower)
        && number(value.value(QStringLiteral("maxAltitudeAmslMeters")), -1000.0, 20000.0, upper) && *lower < *upper
        && number(value.value(QStringLiteral("horizontalCorridorMeters")), 0.01, 1000.0, corridor)
        && number(value.value(QStringLiteral("maxHorizontalSpeedMetersPerSecond")), 0.01, 300.0)
        && number(value.value(QStringLiteral("maxClimbSpeedMetersPerSecond")), 0.01, 20.0)
        && number(value.value(QStringLiteral("maxDescentSpeedMetersPerSecond")), 0.01, 20.0)
        && number(value.value(QStringLiteral("maxRelativeAltitudeMagnitudeMeters")), 0.01, 21000.0);
}

bool wireCoordinate(const QVariant& value, int index, Coordinate* coordinate, bool* geographic)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    qint64 command = 0;
    if (row.size() != 12 || !integer(row.value(QStringLiteral("sequenceNumber")), index, index)
        || !integer(row.value(QStringLiteral("command")), 0, 65535, &command)
        || row.value(QStringLiteral("autoContinue")).type() != QVariant::Bool
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

qint64 absolute(qint64 value) { return value < 0 ? -value : value; }
qint64 ceilingDivide(qint64 value, qint64 denominator)
{
    return value / denominator + (value % denominator != 0 ? 1 : 0);
}

qint64 longitudeMillimetresPerE7(qint64 minimumLatitude, qint64 maximumLatitude)
{
    const qint64 maximumMagnitude = qMax(absolute(minimumLatitude), absolute(maximumLatitude));
    if (maximumMagnitude <= 300000000) return 6;
    if (maximumMagnitude <= 450000000) return 4;
    if (maximumMagnitude <= 600000000) return 3;
    if (maximumMagnitude <= 650000000) return 2;
    return 1;
}

bool appendCell(QVector<Domain::Cell>* cells, qint64 minimumLatitude, qint64 maximumLatitude,
                qint64 minimumLongitude, qint64 maximumLongitude, int segmentIndex, int endpointIndex)
{
    if (cells->size() >= MaximumCells) return false;
    Domain::Cell cell;
    cell.minLatE7 = minimumLatitude; cell.maxLatE7 = maximumLatitude;
    cell.minLonE7 = minimumLongitude; cell.maxLonE7 = maximumLongitude;
    cell.segmentIndex = segmentIndex; cell.endpointIndex = endpointIndex;
    cells->append(cell);
    return true;
}

bool appendCap(QVector<Domain::Cell>* cells, const Coordinate& centre, qint64 radiusMm,
               int segmentIndex, int endpointIndex)
{
    const qint64 latitudeRadius = ceilingDivide(radiusMm, 10);
    const qint64 minimumLatitude = qMax(-LatitudeLimit, centre.latitude - latitudeRadius);
    const qint64 maximumLatitude = qMin(LatitudeLimit, centre.latitude + latitudeRadius);
    const qint64 longitudeRadius = ceilingDivide(radiusMm, longitudeMillimetresPerE7(minimumLatitude, maximumLatitude));
    if (longitudeRadius >= LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, LongitudeLimit, segmentIndex, endpointIndex);
    const qint64 lower = centre.longitude - longitudeRadius, upper = centre.longitude + longitudeRadius;
    // Both encodings of a seam-touch point remain in the complete union.
    if (lower <= -LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, upper, segmentIndex, endpointIndex)
            && appendCell(cells, minimumLatitude, maximumLatitude, lower + FullLongitude, LongitudeLimit, segmentIndex, endpointIndex);
    if (upper >= LongitudeLimit)
        return appendCell(cells, minimumLatitude, maximumLatitude, lower, LongitudeLimit, segmentIndex, endpointIndex)
            && appendCell(cells, minimumLatitude, maximumLatitude, -LongitudeLimit, upper - FullLongitude, segmentIndex, endpointIndex);
    return appendCell(cells, minimumLatitude, maximumLatitude, lower, upper, segmentIndex, endpointIndex);
}
} // namespace

struct FleetMissionStartupIntegerDomain::State {
    QVector<Cell> cells;
    int segmentCount = 0;
    qint64 minimumAltitude = 0, maximumAltitude = 0;
    QVariantMap source;
};

FleetMissionStartupIntegerDomain::FleetMissionStartupIntegerDomain(std::unique_ptr<State> state)
    : _state(std::move(state))
{
}

FleetMissionStartupIntegerDomain::~FleetMissionStartupIntegerDomain() = default;

FleetMissionStartupIntegerDomain::Ptr FleetMissionStartupIntegerDomain::prepare(
    const QVariantMap& originalGlobalPosition9, const QVariantList& canonicalWireItems,
    const QVariantMap& postModeEnvelope, double ceiling)
{
    Coordinate previous;
    double minimumAltitude = 0.0, maximumAltitude = 0.0, corridor = 0.0;
    if (!baseline(originalGlobalPosition9, &previous) || canonicalWireItems.isEmpty() || canonicalWireItems.size() > MaximumItems
        || !envelope(postModeEnvelope, &minimumAltitude, &maximumAltitude, &corridor)
        || !std::isfinite(ceiling) || ceiling < -1000.0 || ceiling > 20000.0) return {};
    maximumAltitude = qMin(maximumAltitude, ceiling);
    if (minimumAltitude > maximumAltitude) return {};
    qint64 minimumFloor = 0, minimumCeiling = 0, maximumFloor = 0, maximumCeiling = 0;
    qint64 corridorFloor = 0, corridorCeiling = 0;
    if (!FleetMissionContactModel::altitudeMillimetreBounds(minimumAltitude, &minimumFloor, &minimumCeiling)
        || !FleetMissionContactModel::altitudeMillimetreBounds(maximumAltitude, &maximumFloor, &maximumCeiling)
        || !FleetMissionContactModel::altitudeMillimetreBounds(corridor, &corridorFloor, &corridorCeiling)
        || minimumCeiling > maximumFloor - 2 || corridorCeiling < 1 || corridorCeiling > 1000000) return {};
    std::unique_ptr<State> state(new State);
    state->minimumAltitude = minimumCeiling;
    state->maximumAltitude = maximumFloor;
    state->cells.reserve(qMin(MaximumCells, canonicalWireItems.size() * 4));
    for (int index = 0; index < canonicalWireItems.size(); ++index) {
        Coordinate next;
        bool geographic = false;
        if (!wireCoordinate(canonicalWireItems.at(index), index, &next, &geographic)) return {};
        if (!geographic) continue;
        const qint64 deltaLatitude = absolute(next.latitude - previous.latitude);
        const qint64 rawLongitude = absolute(next.longitude - previous.longitude);
        const qint64 deltaLongitude = qMin(rawLongitude, FullLongitude - rawLongitude);
        // Full original route tube, not only endpoints or a successful prefix.
        // 12*(dLat+dLon)<=40.8e9 mm; radius<=20.401e9 mm fits qint64.
        const qint64 lengthUpperMm = 12 * (deltaLatitude + deltaLongitude);
        const qint64 radiusMm = ceilingDivide(lengthUpperMm, 2) + corridorCeiling;
        if (!appendCap(&state->cells, previous, radiusMm, state->segmentCount, 0)
            || !appendCap(&state->cells, next, radiusMm, state->segmentCount, 1)) return {};
        ++state->segmentCount;
        previous = next; // Non-geographic speed commands never break the chain.
    }
    if (state->segmentCount < 1 || state->segmentCount > MaximumItems || state->cells.isEmpty()
        || state->cells.size() > MaximumCells) return {};
    QVariantList cells;
    cells.reserve(state->cells.size());
    for (const Cell& cell : state->cells) cells.append(QVariantMap{
        {QStringLiteral("segmentIndex"), cell.segmentIndex}, {QStringLiteral("endpointIndex"), cell.endpointIndex},
        {QStringLiteral("minLatE7"), cell.minLatE7}, {QStringLiteral("maxLatE7"), cell.maxLatE7},
        {QStringLiteral("minLonE7"), cell.minLonE7}, {QStringLiteral("maxLonE7"), cell.maxLonE7}});
    state->source = {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("modelName"), QStringLiteral("native-integer-e7-amsl-mm-domain-v1")},
        {QStringLiteral("coverAlgorithm"), QStringLiteral("ideal-sphere-E7-two-cap-raw-boxes-v2")},
        {QStringLiteral("sphereRadiusMeters"), 6371000}, {QStringLiteral("segmentCount"), state->segmentCount},
        {QStringLiteral("cellCount"), state->cells.size()}, {QStringLiteral("minimumAltitudeMm"), state->minimumAltitude},
        {QStringLiteral("maximumAltitudeMm"), state->maximumAltitude}, {QStringLiteral("altitudeEncodingPaddingMm"), 1},
        {QStringLiteral("cells"), cells}};
    if (QJsonDocument::fromVariant(state->source).toJson(QJsonDocument::Compact).size() > MaximumSourceBytes) return {};
    return Ptr(new FleetMissionStartupIntegerDomain(std::move(state)));
}

const QVector<FleetMissionStartupIntegerDomain::Cell>& FleetMissionStartupIntegerDomain::cells() const { return _state->cells; }
int FleetMissionStartupIntegerDomain::segmentCount() const { return _state->segmentCount; }
qint64 FleetMissionStartupIntegerDomain::minimumAltitudeMm() const { return _state->minimumAltitude; }
qint64 FleetMissionStartupIntegerDomain::maximumAltitudeMm() const { return _state->maximumAltitude; }

bool FleetMissionStartupIntegerDomain::contains(qint64 latitudeE7, qint64 longitudeE7, qint64 altitudeMm) const
{
    if (latitudeE7 < -LatitudeLimit || latitudeE7 > LatitudeLimit || longitudeE7 < -LongitudeLimit || longitudeE7 > LongitudeLimit
        || altitudeMm < -1000000 || altitudeMm > 20000000
        || altitudeMm - 1 < _state->minimumAltitude || altitudeMm + 1 > _state->maximumAltitude) return false;
    for (const Cell& cell : _state->cells)
        if (latitudeE7 >= cell.minLatE7 && latitudeE7 <= cell.maxLatE7
            && longitudeE7 >= cell.minLonE7 && longitudeE7 <= cell.maxLonE7) return true;
    return false;
}

QVariantMap FleetMissionStartupIntegerDomain::source() const { return _state->source; }

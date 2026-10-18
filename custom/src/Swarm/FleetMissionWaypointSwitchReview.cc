#include "FleetMissionWaypointSwitchReview.h"

#include "FleetMissionClearanceReview.h"
#include "FleetMissionActionPhaseReview.h"
#include "FleetMissionContactModel.h"
#include "FleetMissionPlanningWorker.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "ParameterManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <utility>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Polytope = Geometry::Polytope;
using Plane = Geometry::Plane;
using Verdict = Geometry::Verdict;
using CancellationToken = Geometry::CancellationToken;
constexpr int MaximumModels = 32;
constexpr int MaximumItems = 8192;
constexpr int MaximumRegions = 32768;
constexpr int MaximumComponents = 65536;
constexpr int MaximumActions = 8192;
constexpr int MaximumDetails = 512;
constexpr int MaximumWork = 2000000;
constexpr int MaximumFragments = 2048;
constexpr int MaximumOutputBytes = 2 * 1024 * 1024;
constexpr qint64 LatitudeLimit = 800000000;
constexpr qint64 LongitudeLimit = 1800000000;
constexpr qint64 FullLongitude = Q_INT64_C(3600000000);
constexpr qint64 CoordinateBound = 51000000;
constexpr qint64 NormalBound = 100000000;
constexpr qint64 LimitBound = Q_INT64_C(20000000000000000);
const QString ModelName = QStringLiteral("ordinary-nav-land-takeoff-vertical-reference-xy-hull-prism-v5");
const QString ResourceReason = QStringLiteral("waypoint-switch-geometry-resource-limit");
const QString UnknownReason = QStringLiteral("waypoint-switch-geometry-unknown");

bool cancelled(const CancellationToken& token) { return token && token->load(std::memory_order_acquire); }
bool mapValue(const QVariant& value) { return value.type() == QVariant::Map && !value.isNull(); }
bool listValue(const QVariant& value) { return value.type() == QVariant::List && !value.isNull(); }
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
    const double numeric = value.toDouble();
    if (!std::isfinite(numeric) || numeric < minimum || numeric > maximum) return false;
    if (output) *output = numeric;
    return true;
}
bool integer(const QVariant& value, int minimum, int maximum)
{
    double numeric = 0.0;
    return number(value, minimum, maximum, &numeric) && std::floor(numeric) == numeric;
}
bool flag(const QVariantMap& value, const char* name, bool expected)
{
    const QVariant item = value.value(QString::fromLatin1(name));
    return item.type() == QVariant::Bool && !item.isNull() && item.toBool() == expected;
}
bool boolean(const QVariantMap& value, const char* name)
{
    const QVariant item = value.value(QString::fromLatin1(name));
    return item.type() == QVariant::Bool && !item.isNull();
}
bool text(const QVariant& value, int maximum = 256, bool empty = false)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString string = value.toString();
    if ((!empty && string.isEmpty()) || string.size() > maximum || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar character : value.toString()) if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
        && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    return true;
}
QByteArray json(const QVariant& value) { return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact); }
QString fingerprint(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}
QString motionFingerprint(const QVariantList& rows)
{
    QVariantList common;
    common.reserve(rows.size());
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        common.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("geographicMotionModel"), row.value(QStringLiteral("geographicMotionModel"))}});
    }
    return fingerprint(common);
}
QVariantMap failure(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reason}, {QStringLiteral("report"), QVariantMap()}};
}
void addReason(QStringList* reasons, const QString& reason) { if (!reasons->contains(reason)) reasons->append(reason); }
QVariantMap limits()
{
    return {{QStringLiteral("maximumModels"), MaximumModels}, {QStringLiteral("maximumCanonicalItems"), MaximumItems},
        {QStringLiteral("maximumReferenceRegions"), MaximumRegions}, {QStringLiteral("maximumComponents"), MaximumComponents},
        {QStringLiteral("maximumActions"), MaximumActions}, {QStringLiteral("maximumWorkUnits"), MaximumWork},
        {QStringLiteral("maximumFragments"), MaximumFragments}, {QStringLiteral("maximumViolationDetails"), MaximumDetails},
        {QStringLiteral("maximumOutputBytes"), MaximumOutputBytes}};
}

struct Component {
    int vehicleId = 0, fromSequence = 0, toSequence = 0;
    QString regionKind, componentKind;
    Polytope envelope;
    QString contactId;
    QSet<QString> allowedTerrainCellIds;
    QVector<Plane> contactVolume;
};
struct Prepared {
    bool ok = false, modelComplete = false;
    QString error, inputFingerprint, modelsFingerprint;
    QVariantMap input;
    QVariantList memberIds, actions;
    int waypointCount = 0, connectionCount = 0, regionCount = 0;
    QVector<Component> components;
    QStringList reasons{QStringLiteral("dynamic-altitude-acceptance-not-observed"),
        QStringLiteral("px4-smoothing-reachable-set-not-proven"), QStringLiteral("independent-actions-not-certified")};
};
struct Model {
    int vehicleId = 0;
    QVariantList wire;
    QVariantMap declaration;
    qint64 horizontalRadiusMm = 0, verticalRadiusMm = 0;
    bool staticBranch = false;
    int takeoffSequence = -1, firstNavSequence = -1;
    QVariantMap baseline, takeoffItem, firstNavItem;
    bool takeoffParameterSupported = false, takeoffPhaseCompatible = false;
    double minimumTakeoff = 0.0;
    Polytope takeoffStart, takeoffTop;
    FleetMissionContactModel::TakeoffReference takeoffReference;
    int landSequence = -1, landFromSequence = -1;
    FleetMissionContactModel::LandingReference landingReference;
};
struct RawBox { qint64 minLatitude = 0, maxLatitude = 0, minLongitude = 0, maxLongitude = 0; };
qint64 absolute(qint64 value) { return value < 0 ? -value : value; }
qint64 ceilingDivide(qint64 value, qint64 denominator) { return value / denominator + (value % denominator != 0 ? 1 : 0); }
qint64 longitudeScale(qint64 minimum, qint64 maximum)
{
    const qint64 magnitude = qMax(absolute(minimum), absolute(maximum));
    if (magnitude <= 300000000) return 6;
    if (magnitude <= 450000000) return 4;
    if (magnitude <= 600000000) return 3;
    if (magnitude <= 650000000) return 2;
    return 1;
}
QVector<RawBox> cap(qint64 latitude, qint64 longitude, qint64 radiusMm)
{
    const qint64 latitudeRadius = ceilingDivide(radiusMm, 10);
    const qint64 low = latitude - latitudeRadius, high = latitude + latitudeRadius;
    // Unlike the explicitly controlled startup domain, onboard NAV acceptance
    // is not clipped by our +/-80-degree predicate. Reject an unrepresentable
    // COMPLETE cap instead of silently deleting its out-of-domain portion.
    if (low < -LatitudeLimit || high > LatitudeLimit) return {};
    const qint64 longitudeRadius = ceilingDivide(radiusMm, longitudeScale(low, high));
    const qint64 lower = longitude - longitudeRadius, upper = longitude + longitudeRadius;
    if (longitudeRadius >= LongitudeLimit) return {{low, high, -LongitudeLimit, LongitudeLimit}};
    if (lower <= -LongitudeLimit) return {{low, high, -LongitudeLimit, upper}, {low, high, lower + FullLongitude, LongitudeLimit}};
    if (upper >= LongitudeLimit) return {{low, high, lower, LongitudeLimit}, {low, high, -LongitudeLimit, upper - FullLongitude}};
    return {{low, high, lower, upper}};
}
bool project(const RawBox& box, qint64 minimumAltitudeMm, qint64 maximumAltitudeMm,
             const QVariantMap& frame, Polytope* output)
{
    qint64 lower[3]{}, unusedHigh[3]{}, unusedLow[3]{}, upper[3]{};
    if (!FleetMissionContactModel::integerPointMillimetres(box.minLatitude, box.minLongitude, minimumAltitudeMm, frame, lower, unusedHigh)
        || !FleetMissionContactModel::integerPointMillimetres(box.maxLatitude, box.maxLongitude, maximumAltitudeMm, frame, unusedLow, upper)) return false;
    Polytope result;
    result.bounds.minimumEast = lower[0]; result.bounds.maximumEast = upper[0];
    result.bounds.minimumNorth = lower[1]; result.bounds.maximumNorth = upper[1];
    result.bounds.minimumAltitude = lower[2]; result.bounds.maximumAltitude = upper[2];
    *output = result;
    return true;
}
bool boundsValid(const Geometry::Bounds& bounds)
{
    return bounds.minimumEast >= -CoordinateBound && bounds.maximumEast <= CoordinateBound
        && bounds.minimumNorth >= -CoordinateBound && bounds.maximumNorth <= CoordinateBound
        && bounds.minimumAltitude >= -CoordinateBound && bounds.maximumAltitude <= CoordinateBound
        && bounds.minimumEast <= bounds.maximumEast && bounds.minimumNorth <= bounds.maximumNorth
        && bounds.minimumAltitude <= bounds.maximumAltitude;
}
qint64 positiveGcd(qint64 first, qint64 second)
{
    while (second != 0) { const qint64 remainder = first % second; first = second; second = remainder; }
    return first;
}
qint64 orientation(const Geometry::Point2& first, const Geometry::Point2& second, const Geometry::Point2& third)
{
    // All inputs were checked in +/-51,000,000 mm. Each difference is at
    // most 102,000,000; the absolute cross product is <=2*102,000,000^2,
    // strictly inside signed int64. No floating hull or rounded normal.
    return (second.east - first.east) * (third.north - first.north)
        - (second.north - first.north) * (third.east - first.east);
}
bool hullPlane(const Geometry::Point2& first, const Geometry::Point2& second, Plane* output)
{
    const qint64 dx = second.east - first.east, dy = second.north - first.north;
    const qint64 divisor = positiveGcd(absolute(dx), absolute(dy));
    if (!output || divisor == 0) return false;
    Plane result;
    // Exact positive scaling preserves the closed supporting halfspace.
    result.east = dy / divisor; result.north = -dx / divisor;
    if (absolute(result.east) > NormalBound || absolute(result.north) > NormalBound) return false;
    // Two products, each <=100,000,000*51,000,000, cannot overflow. Forming
    // this limit from the divided normal is exactly rawLimit/divisor; the
    // integer vertex makes both terms divisible, with no separate rounding.
    result.limit = result.east * first.east + result.north * first.north;
    if (absolute(result.limit) > LimitBound) return false;
    *output = result;
    return true;
}
bool connectionHull(const Polytope& first, const Polytope& second, Polytope* output)
{
    if (!output) return false;
    *output = Polytope();
    // These are the complete projected endpoint rectangles, not arbitrary
    // polytopes whose additional faces could be silently discarded.
    if (!first.planes.isEmpty() || !second.planes.isEmpty()
        || !boundsValid(first.bounds) || !boundsValid(second.bounds)) return false;
    const auto& a = first.bounds; const auto& b = second.bounds;
    Polytope result;
    result.bounds.minimumEast = std::min(a.minimumEast, b.minimumEast); result.bounds.maximumEast = std::max(a.maximumEast, b.maximumEast);
    result.bounds.minimumNorth = std::min(a.minimumNorth, b.minimumNorth); result.bounds.maximumNorth = std::max(a.maximumNorth, b.maximumNorth);
    result.bounds.minimumAltitude = std::min(a.minimumAltitude, b.minimumAltitude); result.bounds.maximumAltitude = std::max(a.maximumAltitude, b.maximumAltitude);
    QVector<Geometry::Point2> points;
    points.reserve(8);
    for (const auto* bounds : {&a, &b}) {
        for (const qint64 east : {bounds->minimumEast, bounds->maximumEast}) {
            for (const qint64 north : {bounds->minimumNorth, bounds->maximumNorth}) {
                Geometry::Point2 point; point.east = east; point.north = north; points.append(point);
            }
        }
    }
    std::sort(points.begin(), points.end(), [](const Geometry::Point2& left, const Geometry::Point2& right) {
        return left.east < right.east || (left.east == right.east && left.north < right.north);
    });
    points.erase(std::unique(points.begin(), points.end(), [](const Geometry::Point2& left, const Geometry::Point2& right) {
        return left.east == right.east && left.north == right.north;
    }), points.end());
    QVector<Geometry::Point2> hull;
    hull.reserve(16);
    for (const auto& point : points) {
        while (hull.size() >= 2 && orientation(hull.at(hull.size() - 2), hull.last(), point) <= 0) hull.removeLast();
        hull.append(point);
    }
    const int lowerSize = hull.size();
    for (int index = points.size() - 2; index >= 0; --index) {
        const auto& point = points.at(index);
        while (hull.size() > lowerSize && orientation(hull.at(hull.size() - 2), hull.last(), point) <= 0) hull.removeLast();
        hull.append(point);
    }
    if (hull.size() > 1) hull.removeLast();
    if (hull.isEmpty() || hull.size() > 8) return false;
    // One point is already exactly constrained by XY bounds. For a segment,
    // both opposing closed faces preserve its affine line; bounds retain the
    // endpoints. A polygon's CCW edges give its complete horizontal hull.
    if (hull.size() > 1) {
        for (int index = 0; index < hull.size(); ++index) {
            Plane plane;
            if (!hullPlane(hull.at(index), hull.at((index + 1) % hull.size()), &plane)) return false;
            // A closed axis-aligned support edge is already represented by
            // the identical implicit XY bound, including a degenerate line.
            // Bounds receive the same margin below. Keep every diagonal
            // face, but do not turn an ordinary rectangle into a slow query.
            if (plane.east == 0 || plane.north == 0) continue;
            result.planes.append(plane);
        }
    }
    // The full closed altitude slab is retained independently. This prism
    // contains every affine connection between BOTH endpoint 3-D boxes;
    // it does not assert that onboard smoothing follows those connections.
    *output = result;
    return true;
}
bool expanded(const Polytope& reference, const QVariantMap& vehicle, const QVariantMap& policy, bool support, Polytope* output)
{
    if (!output || !boundsValid(reference.bounds) || reference.planes.size() > 8) return false;
    const qint64 horizontal = qint64(vehicle.value(support ? QStringLiteral("supportHorizontalRadiusMm") : QStringLiteral("bodyHorizontalRadiusMm")).toDouble())
        + qint64(vehicle.value(QStringLiteral("horizontalTrackingErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalSceneErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalProjectionErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalSafetyMarginMm")).toDouble());
    const qint64 verticalError = qint64(vehicle.value(QStringLiteral("verticalTrackingErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalSceneErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalDatumAlignmentErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalSafetyMarginMm")).toDouble());
    const qint64 below = verticalError + qint64(vehicle.value(support ? QStringLiteral("supportBelowReferenceMm") : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
    const qint64 above = verticalError + qint64(vehicle.value(support ? QStringLiteral("supportAboveReferenceMm") : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
    // Normalized declarations contain bounded nonnegative integers. Reject
    // any unsupported total before subtracting or forming support products.
    if (horizontal < 0 || horizontal > 2 * CoordinateBound || below < 0 || below > 2 * CoordinateBound
        || above < 0 || above > 2 * CoordinateBound) return false;
    Polytope result = reference;
    result.bounds.minimumEast -= horizontal; result.bounds.maximumEast += horizontal;
    result.bounds.minimumNorth -= horizontal; result.bounds.maximumNorth += horizontal;
    result.bounds.minimumAltitude -= below; result.bounds.maximumAltitude += above;
    if (!boundsValid(result.bounds)) return false;
    for (Plane& plane : result.planes) {
        if (plane.altitude != 0 || plane.strict || (plane.east == 0 && plane.north == 0)
            || absolute(plane.east) > NormalBound || absolute(plane.north) > NormalBound
            || absolute(plane.limit) > LimitBound) return false;
        // Support function of the same translated horizontal axis box used
        // by the bounds. Leaving the old face limits unchanged would clip
        // body/support margins along diagonal edges. Products and sum are
        // <4.1e16 under the checks above; never round or silently drop a face.
        plane.limit += (absolute(plane.east) + absolute(plane.north)) * horizontal;
        if (absolute(plane.limit) > LimitBound) return false;
    }
    *output = result;
    return true;
}
bool parameter(const QVariantList& rows, const QString& name, int type, double minimum, double maximum, double* output)
{
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        if (row.value(QStringLiteral("name")) != QVariant(name)) continue;
        if (!integer(row.value(QStringLiteral("mavParamType")), type, type)
            || !number(row.value(QStringLiteral("value")), minimum, maximum, output)) return false;
        return type != 9 || double(float(*output)) == *output;
    }
    return false;
}
bool radiusMillimetreCeiling(double value, qint64* output)
{
    // Complete nonnegative evidence domain, not the parameter-metadata 200m
    // display range. max(value)*1000 < 9.008e18 < qint64 maximum; the exact
    // binary64 significand*1000 also fits uint64 before any shift.
    if (!output || !std::numeric_limits<double>::is_iec559 || sizeof(double) != sizeof(quint64)
        || std::numeric_limits<double>::radix != 2 || std::numeric_limits<double>::digits != 53
        || std::numeric_limits<double>::max_exponent != 1024 || std::numeric_limits<double>::min_exponent != -1021
        || !std::isfinite(value) || value < 0.0 || value > 9007199254740991.0) return false;
    quint64 bits = 0; std::memcpy(&bits, &value, sizeof(bits));
    const int exponent = int((bits >> 52) & 0x7ff);
    const quint64 fraction = bits & ((quint64(1) << 52) - 1);
    const quint64 significand = exponent ? fraction | (quint64(1) << 52) : fraction;
    const quint64 scaled = significand * quint64(1000);
    const int shift = exponent ? exponent - 1075 : -1074;
    quint64 whole = 0; bool remainder = false;
    if (shift >= 0) {
        if (shift >= 63 || scaled > (quint64(std::numeric_limits<qint64>::max()) >> shift)) return false;
        whole = scaled << shift;
    } else {
        const int right = -shift;
        whole = right >= 64 ? 0 : scaled >> right;
        remainder = right >= 64 ? scaled != 0 : (scaled & ((quint64(1) << right) - 1)) != 0;
    }
    if (whole > quint64(std::numeric_limits<qint64>::max()) - (remainder ? 1 : 0)) return false;
    *output = qint64(whole + (remainder ? 1 : 0));
    return true;
}

struct TakeoffHeights {
    double nominalTop = 0.0;
    qint64 currentLowerMm = 0, currentUpperMm = 0;
    qint64 lowerMm = 0, upperMm = 0;
};
bool takeoffHeights(const Model& model, TakeoffHeights* output)
{
    if (!output || !model.takeoffParameterSupported
        || !std::numeric_limits<float>::is_iec559 || sizeof(float) != sizeof(quint32)
        || std::numeric_limits<float>::radix != 2 || std::numeric_limits<float>::digits != 24
        || std::numeric_limits<float>::max_exponent != 128 || std::numeric_limits<float>::min_exponent != -125) return false;
    double altitudeMm = 0.0, requested = 0.0;
    if (!number(model.baseline.value(QStringLiteral("altitudeAmslMm")), -1000000.0, 20000000.0, &altitudeMm)
        || std::floor(altitudeMm) != altitudeMm
        || !number(model.takeoffItem.value(QStringLiteral("param7")), -1000.0, 20000.0, &requested)
        || double(float(requested)) != requested) return false;
    // Recompute the existing Readiness ground-branch encoding reference from
    // ORIGINAL operands. This does not observe landed state or reconstruct
    // the navigator's exact current altitude, dynamics or completion radius.
    const double altitude = altitudeMm / 1000.0;
    const float current = static_cast<float>(altitude);
    const float minimum = static_cast<float>(model.minimumTakeoff);
    const float request = static_cast<float>(requested);
    const float infinity = std::numeric_limits<float>::infinity();
    const float minimumTarget = static_cast<float>(current + minimum);
    const float nominal = std::fmax(request, minimumTarget);
    const float currentLower = std::nextafterf(static_cast<float>(altitude - 0.001), -infinity);
    const float currentUpper = std::nextafterf(static_cast<float>(altitude + 0.001), infinity);
    const float lowerSum = std::nextafterf(static_cast<float>(currentLower + minimum), -infinity);
    const float upperSum = std::nextafterf(static_cast<float>(currentUpper + minimum), infinity);
    const float predictedLower = std::nextafterf(std::fmax(request, lowerSum), -infinity);
    const float predictedUpper = std::nextafterf(std::fmax(request, upperSum), infinity);
    if (!std::isfinite(current) || !std::isfinite(minimum) || !std::isfinite(request)
        || !std::isfinite(minimumTarget) || !std::isfinite(nominal)
        || !std::isfinite(currentLower) || !std::isfinite(currentUpper)
        || !std::isfinite(lowerSum) || !std::isfinite(upperSum)
        || !std::isfinite(predictedLower) || !std::isfinite(predictedUpper)
        || predictedLower > nominal || nominal > predictedUpper) return false;
    qint64 currentLowerMm = 0, currentUpperMm = 0, lower = 0, upper = 0, unused = 0;
    if (!FleetMissionContactModel::altitudeMillimetreBounds(double(currentLower), &currentLowerMm, &unused)
        || !FleetMissionContactModel::altitudeMillimetreBounds(double(currentUpper), &unused, &currentUpperMm)
        || !FleetMissionContactModel::altitudeMillimetreBounds(double(predictedLower), &lower, &unused)
        || !FleetMissionContactModel::altitudeMillimetreBounds(double(predictedUpper), &unused, &upper)
        || currentLowerMm > currentUpperMm || lower > upper) return false;
    output->currentLowerMm = currentLowerMm; output->currentUpperMm = currentUpperMm;
    output->nominalTop = double(nominal); output->lowerMm = lower; output->upperMm = upper;
    return true;
}
bool unknownSequence(const QVariant& value)
{
    return !value.isValid() || value.type() == QVariant::Invalid || value.userType() == QMetaType::Nullptr;
}
bool sourceMatches(const QVariantMap& source, const QString& kind, int from, int to,
                   const QString& fromPath, const QString& toPath)
{
    return source.value(QStringLiteral("sourceKind")) == QVariant(kind)
        && (from < 0 ? unknownSequence(source.value(QStringLiteral("fromSequenceNumber")))
            : integer(source.value(QStringLiteral("fromSequenceNumber")), from, from))
        && (to < 0 ? unknownSequence(source.value(QStringLiteral("toSequenceNumber")))
            : integer(source.value(QStringLiteral("toSequenceNumber")), to, to))
        && source.value(QStringLiteral("fromSourcePath")) == QVariant(fromPath)
        && source.value(QStringLiteral("toSourcePath")) == QVariant(toPath);
}
bool pointMatches(const QVariant& value, double latitudeE7, double longitudeE7, double altitude)
{
    if (!mapValue(value)) return false;
    const QVariantMap point = value.toMap();
    double latitude = 0.0, longitude = 0.0, height = 0.0;
    return number(point.value(QStringLiteral("latitude")), -80.0, 80.0, &latitude)
        && number(point.value(QStringLiteral("longitude")), -180.0, 180.0, &longitude)
        && number(point.value(QStringLiteral("altitude")), -1000.0, 20000.0, &height)
        && latitude == latitudeE7 / 1e7 && longitude == longitudeE7 / 1e7 && height == altitude;
}
bool takeoffJointMatches(const Model& model, const QVariantMap& member, const QVariantMap& joint,
                         const TakeoffHeights* heights, const CancellationToken& token)
{
    if (!flag(member, "executionProfileAvailable", true) || !flag(member, "takeoffSequenceVerified", true)) return false;
    const QVariantList segments = joint.value(QStringLiteral("segments")).toList();
    const QVariantList sources = joint.value(QStringLiteral("segmentSources")).toList();
    if (segments.size() != sources.size()) return false;
    const QStringList kinds{QStringLiteral("mission-anchor"), QStringLiteral("mission-launch-connection"),
        QStringLiteral("vertical-takeoff"), QStringLiteral("takeoff-request-connection")};
    QMap<QString, QVariantMap> byKind, sourceByKind;
    for (int index = 0; index < segments.size(); ++index) {
        if (cancelled(token)) return false;
        const QVariantMap segment = segments.at(index).toMap(), source = sources.at(index).toMap();
        if (segment.value(QStringLiteral("vehicleId")).toInt() != model.vehicleId) continue;
        const QString kind = segment.value(QStringLiteral("kind")).toString();
        if (!kinds.contains(kind)) continue;
        if (byKind.contains(kind) || source.value(QStringLiteral("segmentId")) != segment.value(QStringLiteral("segmentId"))) return false;
        byKind.insert(kind, segment); sourceByKind.insert(kind, source);
    }
    if (byKind.size() != kinds.size()) return false;
    const QString launch = QStringLiteral("executionProfile.launchAction");
    const QString top = QStringLiteral("takeoffDerivation.nominalVerticalTop");
    if (!sourceMatches(sourceByKind.value(kinds.at(0)), QStringLiteral("encoded-plan"), model.takeoffSequence, model.takeoffSequence, launch, launch)
        || !sourceMatches(sourceByKind.value(kinds.at(1)), QStringLiteral("encoded-plan"), model.takeoffSequence, model.firstNavSequence, launch, QStringLiteral("waypoints"))
        || !sourceMatches(sourceByKind.value(kinds.at(2)), QStringLiteral("startup-hypothesis"), -1, -1, QStringLiteral("currentPosition"), top)
        || !sourceMatches(sourceByKind.value(kinds.at(3)), QStringLiteral("startup-hypothesis"), -1, model.takeoffSequence, top, launch)) return false;
    const double latitude = model.takeoffItem.value(QStringLiteral("latitudeE7")).toDouble();
    const double longitude = model.takeoffItem.value(QStringLiteral("longitudeE7")).toDouble();
    const double altitude = model.takeoffItem.value(QStringLiteral("param7")).toDouble();
    const double baselineLatitude = model.baseline.value(QStringLiteral("latitudeE7")).toDouble();
    const double baselineLongitude = model.baseline.value(QStringLiteral("longitudeE7")).toDouble();
    const auto anchor = byKind.value(kinds.at(0)), connection = byKind.value(kinds.at(1));
    const auto vertical = byKind.value(kinds.at(2)), request = byKind.value(kinds.at(3));
    if (!pointMatches(anchor.value(QStringLiteral("from")), latitude, longitude, altitude)
        || !pointMatches(anchor.value(QStringLiteral("to")), latitude, longitude, altitude)
        || !pointMatches(connection.value(QStringLiteral("from")), latitude, longitude, altitude)
        || !pointMatches(connection.value(QStringLiteral("to")), model.firstNavItem.value(QStringLiteral("latitudeE7")).toDouble(),
            model.firstNavItem.value(QStringLiteral("longitudeE7")).toDouble(), model.firstNavItem.value(QStringLiteral("param7")).toDouble())
        || !pointMatches(vertical.value(QStringLiteral("from")), baselineLatitude, baselineLongitude,
            model.baseline.value(QStringLiteral("altitudeAmslMm")).toDouble() / 1000.0)
        || !pointMatches(request.value(QStringLiteral("to")), latitude, longitude, altitude)) return false;
    const QVariantMap verticalTop = vertical.value(QStringLiteral("to")).toMap();
    double topAltitude = 0.0;
    if (!number(verticalTop.value(QStringLiteral("altitude")), -1000.0, 20000.0, &topAltitude)
        || !pointMatches(vertical.value(QStringLiteral("to")), baselineLatitude, baselineLongitude, topAltitude)
        || !pointMatches(request.value(QStringLiteral("from")), baselineLatitude, baselineLongitude, topAltitude)
        || (heights && topAltitude != heights->nominalTop)) return false;
    return true;
}

Prepared prepare(const QVariant& clearanceInput, const QVariantList& sourceModels,
                 const QVariantMap& frozenJointView, const QString& geometryBasis, const CancellationToken& token)
{
    Prepared result;
    const auto fail = [&](const QString& reason) {
        result.ok = false; result.error = cancelled(token) ? QStringLiteral("waypoint-switch-review-cancelled") : reason;
        result.components.clear(); result.modelComplete = false;
        return result;
    };
    const auto incomplete = [&](const QString& reason) {
        if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
        result.components.clear(); result.regionCount = 0; result.modelComplete = false;
        addReason(&result.reasons, reason); result.ok = true;
        return result;
    };
    if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot({{QStringLiteral("clearanceInput"), clearanceInput}, {QStringLiteral("models"), sourceModels},
            {QStringLiteral("joint"), frozenJointView}, {QStringLiteral("geometryBasis"), geometryBasis}}, &detached))
        return fail(QStringLiteral("waypoint-switch-source-resource-or-value-invalid"));
    const QVariantMap joint = detached.value(QStringLiteral("joint")).toMap();
    const QVariantList rows = detached.value(QStringLiteral("models")).toList();
    if (!digest(geometryBasis) || joint.value(QStringLiteral("geometryBasis")) != QVariant(geometryBasis)
        || !FleetMissionClearanceReview::geometrySourceValid(detached.value(QStringLiteral("clearanceInput")), joint, &result.input, token))
        return fail(QStringLiteral("waypoint-switch-joint-source-invalid"));
    const QVariantList declarations = result.input.value(QStringLiteral("vehicles")).toList();
    const QVariantList coverage = joint.value(QStringLiteral("memberCoverage")).toList();
    if (rows.isEmpty() || rows.size() > MaximumModels || rows.size() != declarations.size() || rows.size() != coverage.size())
        return fail(QStringLiteral("waypoint-switch-member-count-invalid"));
    result.inputFingerprint = fingerprint(result.input); result.modelsFingerprint = fingerprint(rows);
    QVector<Model> models;
    QSet<int> seen;
    int itemCount = 0;
    bool allStatic = true, allLandCompatible = true, anyLand = false;
    for (int index = 0; index < rows.size(); ++index) {
        if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
        if (!mapValue(rows.at(index))) return fail(QStringLiteral("waypoint-switch-model-row-invalid"));
        const QVariantMap row = rows.at(index).toMap(), member = coverage.at(index).toMap(), declaration = declarations.at(index).toMap();
        if (!exact(row, {"vehicleId", "geographicMotionModel", "parameterSnapshot"}) || !integer(row.value(QStringLiteral("vehicleId")), 1, 255)
            || !mapValue(row.value(QStringLiteral("geographicMotionModel"))) || !mapValue(row.value(QStringLiteral("parameterSnapshot"))))
            return fail(QStringLiteral("waypoint-switch-model-row-invalid"));
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        if (seen.contains(id) || !integer(member.value(QStringLiteral("vehicleId")), id, id)
            || !integer(declaration.value(QStringLiteral("vehicleId")), id, id) || !flag(member, "encodingAvailable", true))
            return fail(QStringLiteral("waypoint-switch-member-order-or-encoding-invalid"));
        seen.insert(id); result.memberIds.append(id);
        const QVariantMap source = row.value(QStringLiteral("geographicMotionModel")).toMap();
        if (!mapValue(source.value(QStringLiteral("baselineGlobalPosition"))) || !mapValue(source.value(QStringLiteral("startupTransitionPolicy")))
            || !listValue(source.value(QStringLiteral("canonicalWireItems"))) || source.value(QStringLiteral("payloadContentHash")) != member.value(QStringLiteral("payloadHash")))
            return fail(QStringLiteral("waypoint-switch-motion-source-invalid"));
        double ceiling = 0.0;
        if (!number(source.value(QStringLiteral("maximumAltitudeAmslMeters")), -1000.0, 20000.0, &ceiling))
            return fail(QStringLiteral("waypoint-switch-motion-source-invalid"));
        const QVariantList wire = source.value(QStringLiteral("canonicalWireItems")).toList();
        if (wire.isEmpty() || wire.size() > 2048 || itemCount > MaximumItems - wire.size())
            return fail(QStringLiteral("waypoint-switch-canonical-item-budget-exceeded"));
        itemCount += wire.size();
        QVariantMap payload{{QStringLiteral("canonicalWireItems"), wire}, {QStringLiteral("contentHash"), source.value(QStringLiteral("payloadContentHash"))}};
        if (source.contains(QStringLiteral("startupSpatialGuard"))) payload.insert(QStringLiteral("executionProfile"), QVariantMap{
            {QStringLiteral("startupSpatialGuard"), source.value(QStringLiteral("startupSpatialGuard"))},
            {QStringLiteral("startupTransitionPolicy"), source.value(QStringLiteral("startupTransitionPolicy"))}});
        const auto motion = FleetMissionStartupGeographicMotion::prepare(source.value(QStringLiteral("baselineGlobalPosition")).toMap(), payload,
            source.value(QStringLiteral("startupTransitionPolicy")).toMap(), ceiling);
        if (!motion || json(motion->source()) != json(source)) return fail(QStringLiteral("waypoint-switch-complete-motion-source-mismatch"));
        QVariantList legacy, rates, phases, arrival;
        if (!ParameterManager::executionParameterSnapshotRows(row.value(QStringLiteral("parameterSnapshot")).toMap(), &legacy, &rates, &phases, &arrival))
            return fail(QStringLiteral("waypoint-switch-complete-parameter-snapshot-invalid"));
        double horizontal = 0.0, vertical = 0.0, avoidance = 0.0;
        if (!parameter(legacy, QStringLiteral("NAV_ACC_RAD"), 9, 0.0, 9007199254740991.0, &horizontal)
            || !parameter(arrival, QStringLiteral("NAV_MC_ALT_RAD"), 9, 0.0, 9007199254740991.0, &vertical)
            || !parameter(phases, QStringLiteral("COM_OBS_AVOID"), 6, 0.0, 1.0, &avoidance))
            return fail(QStringLiteral("waypoint-switch-supported-acceptance-parameters-required"));
        qint64 horizontalCeiling = 0, verticalCeiling = 0;
        if (!radiusMillimetreCeiling(horizontal, &horizontalCeiling) || !radiusMillimetreCeiling(vertical, &verticalCeiling))
            return fail(QStringLiteral("waypoint-switch-acceptance-mm-domain-unsupported"));
        Model model; model.vehicleId = id; model.wire = wire; model.declaration = declaration;
        model.baseline = source.value(QStringLiteral("baselineGlobalPosition")).toMap();
        model.horizontalRadiusMm = horizontalCeiling; model.verticalRadiusMm = verticalCeiling; model.staticBranch = avoidance == 0.0;
        allStatic = allStatic && model.staticBranch;
        int previous = -1, previousSequence = -1;
        for (const QVariant& item : wire) {
            const QVariantMap value = item.toMap(); const int command = value.value(QStringLiteral("command")).toInt();
            if (command == 178) continue;
            if (command == 16) {
                if (model.firstNavSequence < 0) {
                    model.firstNavSequence = value.value(QStringLiteral("sequenceNumber")).toInt();
                    model.firstNavItem = value;
                }
                ++result.waypointCount; if (previous == 16) ++result.connectionCount;
            }
            else {
                if (result.actions.size() >= MaximumActions) return fail(QStringLiteral("waypoint-switch-action-budget-exceeded"));
                const QString reason = command == 22 ? QStringLiteral("takeoff-independent-action")
                    : command == 21 ? QStringLiteral("land-independent-action") : QStringLiteral("hold-independent-action");
                result.actions.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("sequenceNumber"), value.value(QStringLiteral("sequenceNumber"))},
                    {QStringLiteral("command"), command}, {QStringLiteral("reason"), reason}});
                if (command == 22) {
                    if (model.takeoffSequence >= 0 || previous >= 0)
                        return fail(QStringLiteral("takeoff-reference-requires-one-launch-before-ordinary-nav"));
                    model.takeoffSequence = value.value(QStringLiteral("sequenceNumber")).toInt();
                    model.takeoffItem = value;
                }
                if (command == 21) {
                    if (model.landSequence >= 0 || previous != 16)
                        return fail(QStringLiteral("land-reference-requires-one-terminal-land-after-ordinary-nav"));
                    model.landSequence = value.value(QStringLiteral("sequenceNumber")).toInt();
                    model.landFromSequence = previousSequence;
                    anyLand = true;
                }
            }
            previous = command; previousSequence = value.value(QStringLiteral("sequenceNumber")).toInt();
        }
        if (model.takeoffSequence >= 0) {
            if (model.firstNavSequence <= model.takeoffSequence
                || model.takeoffItem.value(QStringLiteral("latitudeE7")).toDouble() != model.firstNavItem.value(QStringLiteral("latitudeE7")).toDouble()
                || model.takeoffItem.value(QStringLiteral("longitudeE7")).toDouble() != model.firstNavItem.value(QStringLiteral("longitudeE7")).toDouble()
                || model.takeoffItem.value(QStringLiteral("param7")).toDouble() != model.firstNavItem.value(QStringLiteral("param7")).toDouble())
                return fail(QStringLiteral("takeoff-reference-first-nav-or-request-mismatch"));
            model.takeoffParameterSupported = parameter(legacy, QStringLiteral("MIS_TAKEOFF_ALT"), 9, 0.0,
                9007199254740991.0, &model.minimumTakeoff);
        }
        // Only members carrying the corresponding actual encoded action
        // acquire its parameter restrictions. A NAV-only member stays so.
        if (model.landSequence >= 0 || model.takeoffSequence >= 0) {
            const QVariantMap phaseReview = FleetMissionActionPhaseReview::evaluate(legacy, phases);
            if (model.landSequence >= 0)
                allLandCompatible = allLandCompatible && flag(phaseReview, "ok", true) && flag(phaseReview, "landCompatible", true);
            if (model.takeoffSequence >= 0)
                model.takeoffPhaseCompatible = flag(phaseReview, "ok", true) && flag(phaseReview, "takeoffCompatible", true);
        }
        models.append(model);
    }
    if (result.waypointCount == 0) return fail(QStringLiteral("waypoint-switch-ordinary-nav-scope-empty"));
    // Finish the complete group source/parameter scan above before deciding
    // whether any conditional TAKEOFF reference is unsupported. Never retain
    // other members' useful geometry as a successful preparation prefix.
    const QVariantMap frame = result.input.value(QStringLiteral("scene")).toMap().value(QStringLiteral("localFrame")).toMap();
    bool allTakeoffSupported = true, anyTakeoff = false;
    for (int index = 0; index < models.size(); ++index) {
        if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
        Model& model = models[index];
        if (model.takeoffSequence < 0) continue;
        anyTakeoff = true;
        const QVariantMap member = coverage.at(index).toMap();
        if (!flag(member, "executionProfileAvailable", true) || !flag(member, "takeoffSequenceVerified", true)) {
            allTakeoffSupported = false;
            addReason(&result.reasons, QStringLiteral("takeoff-reference-original-startup-profile-or-sequence-binding-unavailable"));
            continue;
        }
        TakeoffHeights heights;
        const bool heightsKnown = takeoffHeights(model, &heights);
        if (!takeoffJointMatches(model, member, joint, heightsKnown ? &heights : nullptr, token))
            return fail(QStringLiteral("takeoff-reference-joint-baseline-sequence-or-source-mismatch"));
        if (!model.takeoffParameterSupported) {
            allTakeoffSupported = false;
            addReason(&result.reasons, QStringLiteral("takeoff-reference-original-REAL32-MIS_TAKEOFF_ALT-required"));
        }
        if (!model.takeoffPhaseCompatible) {
            allTakeoffSupported = false;
            addReason(&result.reasons, QStringLiteral("takeoff-reference-action-phase-parameter-model-unsupported"));
        }
        const qint64 latitude = qint64(model.baseline.value(QStringLiteral("latitudeE7")).toDouble());
        const qint64 longitude = qint64(model.baseline.value(QStringLiteral("longitudeE7")).toDouble());
        if (!heightsKnown
            || !project({latitude, latitude, longitude, longitude}, heights.currentLowerMm, heights.currentUpperMm, frame, &model.takeoffStart)
            || !project({latitude, latitude, longitude, longitude}, heights.lowerMm, heights.upperMm, frame, &model.takeoffTop)
            || !boundsValid(model.takeoffStart.bounds) || !boundsValid(model.takeoffTop.bounds)) {
            allTakeoffSupported = false;
            addReason(&result.reasons, QStringLiteral("takeoff-reference-complete-encoding-domain-unsupported"));
        }
    }
    if (anyTakeoff)
        addReason(&result.reasons, QStringLiteral("takeoff-ground-branch-conditional-reference-not-fresh-ground-state-or-actual-path-proof"));
    if (!allStatic) addReason(&result.reasons, QStringLiteral("avoidance-dynamic-acceptance-branch-unsupported"));
    if (!allLandCompatible) addReason(&result.reasons, QStringLiteral("land-action-phase-parameter-model-unsupported"));
    if (!allStatic || !allLandCompatible) { result.ok = true; return result; }
    if (!allTakeoffSupported)
        return incomplete(QStringLiteral("takeoff-reference-complete-supported-parameter-branches-required"));
    if (anyLand || anyTakeoff) {
        if (result.input.value(QStringLiteral("schemaVersion")).toInt() != 2)
            return incomplete(anyTakeoff ? QStringLiteral("takeoff-vertical-reference-requires-independent-contact-model")
                : QStringLiteral("land-reference-requires-independent-contact-model"));
        // Reuse the same normalized input and complete Joint once. This is
        // same-call native action/reference material, never a caller contact
        // credential or an inferred ground/landed-state observation.
        const auto contact = FleetMissionContactModel::prepare(result.input, joint, token);
        if (!contact.ok) return incomplete(anyTakeoff ? QStringLiteral("takeoff-vertical-bound-contact-reference-unsupported")
            : QStringLiteral("land-bound-contact-reference-unsupported"));
        QMap<int, int> modelById;
        for (int index = 0; index < models.size(); ++index) modelById.insert(models.at(index).vehicleId, index);
        QMap<int, QString> expectedSegments;
        QMap<int, QString> expectedTakeoffSegments;
        QMap<int, QVariantMap> originalTakeoffStarts;
        const QVariantList segments = joint.value(QStringLiteral("segments")).toList();
        const QVariantList sources = joint.value(QStringLiteral("segmentSources")).toList();
        if (segments.size() != sources.size()) return fail(QStringLiteral("land-reference-joint-source-invalid"));
        for (int index = 0; index < segments.size(); ++index) {
            if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
            const QVariantMap segment = segments.at(index).toMap(), source = sources.at(index).toMap();
            if (anyTakeoff && segment.value(QStringLiteral("kind")) == QVariant(QStringLiteral("vertical-takeoff"))) {
                const int id = segment.value(QStringLiteral("vehicleId")).toInt();
                if (!modelById.contains(id) || models.at(modelById.value(id)).takeoffSequence < 0
                    || expectedTakeoffSegments.contains(id)
                    || source.value(QStringLiteral("segmentId")) != segment.value(QStringLiteral("segmentId"))
                    || !sourceMatches(source, QStringLiteral("startup-hypothesis"), -1, -1, QStringLiteral("currentPosition"),
                        QStringLiteral("takeoffDerivation.nominalVerticalTop")))
                    return fail(QStringLiteral("takeoff-vertical-reference-joint-source-mismatch"));
                expectedTakeoffSegments.insert(id, segment.value(QStringLiteral("segmentId")).toString());
                originalTakeoffStarts.insert(id, segment.value(QStringLiteral("from")).toMap());
                continue;
            }
            if (segment.value(QStringLiteral("kind")) != QVariant(QStringLiteral("mission-terminal-land"))) continue;
            const int id = segment.value(QStringLiteral("vehicleId")).toInt();
            if (!modelById.contains(id) || expectedSegments.contains(id))
                return fail(QStringLiteral("land-reference-joint-member-invalid"));
            const Model& model = models.at(modelById.value(id));
            if (model.landSequence < 0 || source.value(QStringLiteral("sourceKind")) != QVariant(QStringLiteral("encoded-plan"))
                || source.value(QStringLiteral("toSourcePath")) != QVariant(QStringLiteral("executionProfile.endAction"))
                || !integer(source.value(QStringLiteral("fromSequenceNumber")), model.landFromSequence, model.landFromSequence)
                || !integer(source.value(QStringLiteral("toSequenceNumber")), model.landSequence, model.landSequence)
                || source.value(QStringLiteral("segmentId")) != segment.value(QStringLiteral("segmentId")))
                return fail(QStringLiteral("land-reference-joint-sequence-or-source-mismatch"));
            expectedSegments.insert(id, segment.value(QStringLiteral("segmentId")).toString());
        }
        QMap<int, FleetMissionContactModel::LandingReference> references;
        for (const auto& reference : contact.landingReferences) {
            if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
            if (!modelById.contains(reference.vehicleId) || references.contains(reference.vehicleId))
                return fail(QStringLiteral("land-reference-contact-member-invalid"));
            const Model& model = models.at(modelById.value(reference.vehicleId));
            if (model.landSequence < 0 || reference.missionSequenceNumber != model.landSequence
                || !expectedSegments.contains(reference.vehicleId)
                || reference.segmentId != expectedSegments.value(reference.vehicleId)
                || reference.contactId.isEmpty() || reference.allowedTerrainCellIds.isEmpty() || reference.contactVolume.isEmpty())
                return fail(QStringLiteral("land-reference-contact-identity-mismatch"));
            references.insert(reference.vehicleId, reference);
        }
        QMap<int, FleetMissionContactModel::TakeoffReference> takeoffReferences;
        if (anyTakeoff) for (const auto& reference : contact.takeoffReferences) {
            if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
            if (!modelById.contains(reference.vehicleId) || takeoffReferences.contains(reference.vehicleId))
                return fail(QStringLiteral("takeoff-vertical-reference-contact-member-invalid"));
            const Model& model = models.at(modelById.value(reference.vehicleId));
            if (model.takeoffSequence < 0 || reference.missionSequenceNumber != model.takeoffSequence
                || !expectedTakeoffSegments.contains(reference.vehicleId)
                || reference.segmentId != expectedTakeoffSegments.value(reference.vehicleId)
                || reference.contactId.isEmpty() || reference.allowedTerrainCellIds.isEmpty() || reference.contactVolume.isEmpty()
                || reference.minimumReferenceAltitudeMm > reference.maximumReferenceAltitudeMm)
                return fail(QStringLiteral("takeoff-vertical-reference-contact-identity-mismatch"));
            qint64 originalLower[3]{}, originalUpper[3]{}, integerLower[3]{}, integerUpper[3]{};
            if (!FleetMissionContactModel::pointMillimetres(originalTakeoffStarts.value(reference.vehicleId), frame, originalLower, originalUpper)
                || !FleetMissionContactModel::integerPointMillimetres(qint64(model.baseline.value(QStringLiteral("latitudeE7")).toDouble()),
                    qint64(model.baseline.value(QStringLiteral("longitudeE7")).toDouble()),
                    qint64(model.baseline.value(QStringLiteral("altitudeAmslMm")).toDouble()), frame, integerLower, integerUpper))
                return fail(QStringLiteral("takeoff-vertical-reference-original-start-projection-unsupported"));
            for (int axis = 0; axis < 3; ++axis) {
                // The old source used binary64 millimetres/1000 and therefore
                // can straddle the original integer Z. Match that SAME Joint
                // enclosure, then check containment of the direct integer
                // point; never demand that the two Z arrays be identical.
                if (reference.referenceLower[axis] != originalLower[axis] || reference.referenceUpper[axis] != originalUpper[axis]
                    || integerLower[axis] < reference.referenceLower[axis] || integerUpper[axis] > reference.referenceUpper[axis])
                    return fail(QStringLiteral("takeoff-vertical-reference-original-start-projection-mismatch"));
            }
            takeoffReferences.insert(reference.vehicleId, reference);
        }
        for (Model& model : models) {
            if (model.landSequence < 0) continue;
            if (!references.contains(model.vehicleId)) return incomplete(QStringLiteral("land-bound-contact-reference-missing"));
            model.landingReference = references.value(model.vehicleId);
        }
        for (Model& model : models) {
            if (model.takeoffSequence < 0) continue;
            if (!takeoffReferences.contains(model.vehicleId))
                return incomplete(QStringLiteral("takeoff-vertical-bound-contact-reference-missing"));
            const auto reference = takeoffReferences.value(model.vehicleId);
            if (model.takeoffStart.bounds.minimumAltitude < reference.minimumReferenceAltitudeMm
                || model.takeoffStart.bounds.maximumAltitude > reference.maximumReferenceAltitudeMm)
                return incomplete(QStringLiteral("takeoff-vertical-reference-full-start-encoding-outside-contact-height-window"));
            model.takeoffReference = reference;
        }
        if (anyLand) addReason(&result.reasons, QStringLiteral("land-entry-and-descent-are-declared-reference-envelopes-only"));
        if (anyTakeoff) addReason(&result.reasons, QStringLiteral("takeoff-vertical-complete-encoding-reference-not-actual-ascent-proof"));
    }
    const QVariantMap policy = result.input.value(QStringLiteral("policy")).toMap();
    const int parts = result.input.value(QStringLiteral("schemaVersion")).toInt() == 2 ? 2 : 1;
    const auto append = [&](const Model& model, const Polytope& reference, int from, int to, const QString& kind,
                            const FleetMissionContactModel::LandingReference* landing,
                            const FleetMissionContactModel::TakeoffReference* takeoff) {
        if ((landing && takeoff) || (takeoff && (parts != 2 || kind != QStringLiteral("takeoff-vertical-reference")))
            || result.regionCount >= MaximumRegions || result.components.size() > MaximumComponents - parts) return false;
        for (int part = 0; part < parts; ++part) {
            Component component; component.vehicleId = model.vehicleId; component.fromSequence = from; component.toSequence = to;
            component.regionKind = kind; component.componentKind = part ? QStringLiteral("support") : QStringLiteral("protected");
            if (!expanded(reference, model.declaration, policy, part != 0, &component.envelope)) return false;
            if (landing && part != 0) {
                component.contactId = landing->contactId;
                component.allowedTerrainCellIds = landing->allowedTerrainCellIds;
                component.contactVolume = landing->contactVolume;
            }
            if (takeoff && part != 0) {
                component.contactId = takeoff->contactId;
                component.allowedTerrainCellIds = takeoff->allowedTerrainCellIds;
                component.contactVolume = takeoff->contactVolume;
            }
            result.components.append(component);
        }
        ++result.regionCount; return true;
    };
    for (const Model& model : models) {
        if (model.takeoffSequence >= 0) {
            if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
            Polytope vertical;
            // Keep BOTH full endpoint encoding boxes. Intervals may overlap;
            // this is their complete XY hull x full Z reference, not a claim
            // that every source-consistent altitude strictly ascends.
            // Both sequence fields identify the original TAKEOFF ACTION.
            // The current-position endpoint is not itself a wire22 item or
            // an observed mission/flight-trajectory sequence.
            if (!connectionHull(model.takeoffStart, model.takeoffTop, &vertical)
                || !append(model, vertical, model.takeoffSequence, model.takeoffSequence,
                    QStringLiteral("takeoff-vertical-reference"), nullptr, &model.takeoffReference))
                return fail(QStringLiteral("takeoff-vertical-reference-complete-domain-unsupported"));
        }
        QVector<Polytope> previousRegions;
        int previousSequence = -1, previousCommand = -1;
        for (const QVariant& item : model.wire) {
            if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
            const QVariantMap row = item.toMap(); const int command = row.value(QStringLiteral("command")).toInt();
            if (command == 178) continue;
            const int sequence = row.value(QStringLiteral("sequenceNumber")).toInt();
            if (command == 21) {
                if (previousCommand != 16 || previousSequence != model.landFromSequence || previousRegions.isEmpty()
                    || sequence != model.landSequence) return incomplete(QStringLiteral("land-reference-complete-entry-acceptance-unavailable"));
                const auto& landing = model.landingReference;
                qint64 targetLower[3]{}, targetUpper[3]{};
                if (!FleetMissionContactModel::integerPointMillimetres(qint64(row.value(QStringLiteral("latitudeE7")).toDouble()),
                        qint64(row.value(QStringLiteral("longitudeE7")).toDouble()), landing.referenceLower[2], frame, targetLower, targetUpper))
                    return fail(QStringLiteral("land-reference-target-projection-unsupported"));
                for (int axis = 0; axis < 3; ++axis)
                    if (targetLower[axis] != landing.referenceLower[axis] || targetUpper[axis] != landing.referenceUpper[axis])
                        return fail(QStringLiteral("land-reference-original-target-projection-mismatch"));
                for (const Polytope& first : previousRegions) {
                    if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
                    // Preserve the whole prior acceptance slab, including its
                    // low tail. Never clip it upward to manufacture descent.
                    if (first.bounds.minimumAltitude <= landing.referenceUpper[2])
                        return incomplete(QStringLiteral("land-reference-entry-slab-not-wholly-above-resting-reference"));
                    Polytope target;
                    target.bounds = first.bounds;
                    target.bounds.minimumEast = landing.referenceLower[0]; target.bounds.maximumEast = landing.referenceUpper[0];
                    target.bounds.minimumNorth = landing.referenceLower[1]; target.bounds.maximumNorth = landing.referenceUpper[1];
                    Polytope entry;
                    if (!connectionHull(first, target, &entry)
                        || !append(model, entry, previousSequence, sequence, QStringLiteral("land-entry-connection"), nullptr, nullptr))
                        return fail(QStringLiteral("land-reference-complete-entry-domain-unsupported"));
                    // Keep the same FULL XY hull over the complete height
                    // interval. A target-only vertical pillar would omit
                    // simultaneous horizontal convergence and descent.
                    Polytope descent = entry;
                    descent.bounds.minimumAltitude = landing.referenceLower[2];
                    if (!append(model, descent, previousSequence, sequence, QStringLiteral("land-descent-reference"), &landing, nullptr))
                        return fail(QStringLiteral("land-reference-complete-descent-domain-unsupported"));
                }
                previousRegions.clear(); previousSequence = sequence; previousCommand = command; continue;
            }
            if (command != 16) { previousRegions.clear(); previousSequence = sequence; previousCommand = command; continue; }
            qint64 altitudeFloor = 0, altitudeCeiling = 0;
            if (!FleetMissionContactModel::altitudeMillimetreBounds(row.value(QStringLiteral("param7")).toDouble(), &altitudeFloor, &altitudeCeiling))
                return fail(QStringLiteral("waypoint-switch-altitude-domain-unsupported"));
            // Separate exact bounds avoid losing a small radius in p7 +/- V.
            const qint64 minimumAltitude = altitudeFloor - model.verticalRadiusMm, maximumAltitude = altitudeCeiling + model.verticalRadiusMm;
            QVector<Polytope> currentRegions;
            const auto raw = cap(qint64(row.value(QStringLiteral("latitudeE7")).toDouble()), qint64(row.value(QStringLiteral("longitudeE7")).toDouble()), model.horizontalRadiusMm);
            if (raw.isEmpty()) return fail(QStringLiteral("waypoint-switch-complete-geographic-extent-unsupported"));
            for (const RawBox& box : raw) {
                Polytope reference;
                if (!project(box, minimumAltitude, maximumAltitude, frame, &reference)
                    || !append(model, reference, sequence, sequence, QStringLiteral("waypoint-acceptance"), nullptr, nullptr))
                    return fail(QStringLiteral("waypoint-switch-complete-region-domain-unsupported"));
                currentRegions.append(reference);
            }
            if (model.takeoffSequence >= 0 && sequence == model.firstNavSequence) {
                if (previousCommand != 22 || previousSequence != model.takeoffSequence)
                    return fail(QStringLiteral("takeoff-reference-first-nav-connection-source-mismatch"));
                // This conditional ground-branch top includes the complete
                // encoding interval, not just the Joint's nominal altitude.
                // Preserve EVERY first-NAV date-line split and both endpoint
                // boxes' full Z. No contact/pad permission crosses this edge.
                for (const Polytope& last : currentRegions) {
                    if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
                    Polytope connection;
                    if (!connectionHull(model.takeoffTop, last, &connection)
                        || !append(model, connection, model.takeoffSequence, sequence,
                            QStringLiteral("takeoff-nav-reference-connection"), nullptr, nullptr))
                        return fail(QStringLiteral("takeoff-reference-complete-first-nav-connection-domain-unsupported"));
                }
            }
            if (previousCommand == 16) {
                // Keep every date-line split pair, never just one apparently
                // nearby rectangle or a truncated usable prefix.
                for (const Polytope& first : previousRegions) for (const Polytope& last : currentRegions) {
                    if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
                    Polytope connection;
                    if (!connectionHull(first, last, &connection)
                        || !append(model, connection, previousSequence, sequence, QStringLiteral("nav-transition"), nullptr, nullptr))
                        return fail(QStringLiteral("waypoint-switch-complete-connection-domain-unsupported"));
                }
            }
            previousRegions = currentRegions; previousSequence = sequence; previousCommand = command;
        }
    }
    if (cancelled(token)) return fail(QStringLiteral("waypoint-switch-review-cancelled"));
    result.modelComplete = true; result.ok = true;
    return result;
}

using Volumes = QVector<QVector<Plane>>;
bool prepareCoverageIndex(const QVariantList& rows, bool horizontalOnly,
                          Geometry::PreparedPrismIndex* index, Geometry::Budget* budget,
                          const CancellationToken& token)
{
    if (!index || !budget) return false;
    QVector<Geometry::PreparedPrism> prisms;
    prisms.reserve(rows.size());
    for (const QVariant& value : rows) {
        if (cancelled(token)) return false;
        const QVariantMap row = value.toMap();
        QVector<Geometry::Point2> footprint;
        const QVariantList polygon = row.value(QStringLiteral("polygon")).toList();
        footprint.reserve(polygon.size());
        for (const QVariant& pointValue : polygon) {
            const QVariantMap point = pointValue.toMap();
            Geometry::Point2 vertex;
            vertex.east = qint64(point.value(QStringLiteral("eastMm")).toDouble());
            vertex.north = qint64(point.value(QStringLiteral("northMm")).toDouble());
            footprint.append(vertex);
        }
        // Terrain coverage is independent of its contact height. This index
        // contains the full Z domain of every accepted component query.
        const qint64 lower = horizontalOnly ? -CoordinateBound
            : qint64(row.value(QStringLiteral("minimumAmslMm")).toDouble());
        const qint64 upper = horizontalOnly ? CoordinateBound
            : qint64(row.value(QStringLiteral("maximumAmslMm")).toDouble());
        Geometry::PreparedPrism prism;
        if (!Geometry::prepareConvexPrism(footprint, lower, upper, &prism, budget)) return false;
        prisms.append(prism);
    }
    return Geometry::preparePrismIndex(prisms, index, budget);
}
QStringList reasonsFor(const Prepared& prepared, bool complete, bool resource, int violations)
{
    QStringList reasons = prepared.reasons;
    if (prepared.modelComplete && !complete) addReason(&reasons, resource ? ResourceReason : UnknownReason);
    if (violations) addReason(&reasons, QStringLiteral("waypoint-switch-envelope-candidates-observed"));
    if (violations > MaximumDetails) addReason(&reasons, QStringLiteral("waypoint-switch-violation-details-truncated"));
    return reasons;
}
QVariantMap report(const Prepared& prepared, const QString& basis, bool complete, int processed, int violationCount,
                   const QVariantList& violations, int workUnits, bool resource)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-waypoint-switch-review")},
        {QStringLiteral("modelName"), ModelName}, {QStringLiteral("geometryBasis"), basis},
        {QStringLiteral("clearanceInputFingerprint"), prepared.inputFingerprint}, {QStringLiteral("modelsFingerprint"), prepared.modelsFingerprint},
        {QStringLiteral("memberIds"), prepared.memberIds}, {QStringLiteral("memberCount"), prepared.memberIds.size()},
        {QStringLiteral("navWaypointCount"), prepared.waypointCount}, {QStringLiteral("navConnectionCount"), prepared.connectionCount},
        {QStringLiteral("referenceRegionCount"), prepared.regionCount}, {QStringLiteral("componentCount"), prepared.components.size()},
        {QStringLiteral("processedComponentCount"), processed}, {QStringLiteral("actionBoundaries"), prepared.actions},
        {QStringLiteral("modelComplete"), prepared.modelComplete}, {QStringLiteral("comparisonComplete"), complete},
        {QStringLiteral("declaredModelSatisfied"), prepared.modelComplete && complete && violationCount == 0},
        {QStringLiteral("violationCount"), violationCount}, {QStringLiteral("violations"), violations},
        {QStringLiteral("violationsTruncated"), violationCount > MaximumDetails},
        {QStringLiteral("reasonCodes"), reasonsFor(prepared, complete, resource, violationCount)}, {QStringLiteral("workUnits"), workUnits},
        {QStringLiteral("limits"), limits()}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("numericAcceptanceContainmentProven"), false}, {QStringLiteral("actualPathCertified"), false},
        {QStringLiteral("dynamicAltitudeAcceptanceProven"), false}, {QStringLiteral("smoothingReachableSetProven"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightReleased"), false}, {QStringLiteral("clearanceCertified"), false}};
}
QVariantMap success(const QVariantMap& value, const CancellationToken& token)
{
    if (cancelled(token)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
    QVariantMap detached;
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("report"), value}};
    if (!FleetMissionPlanningWorker::snapshot(result, &detached) || json(detached).size() > MaximumOutputBytes)
        return failure(cancelled(token) ? QStringLiteral("waypoint-switch-review-cancelled") : QStringLiteral("waypoint-switch-output-resource-invalid"));
    return cancelled(token) ? failure(QStringLiteral("waypoint-switch-review-cancelled")) : detached;
}
} // namespace

QVariantMap FleetMissionWaypointSwitchReview::build(const QVariant& clearanceInput, const QVariantList& models,
    const QVariantMap& frozenJointView, const QString& geometryBasis, const CancellationToken& cancellation, PreparedPtr* exported)
{
    if (exported) exported->reset();
    const Prepared prepared = prepare(clearanceInput, models, frozenJointView, geometryBasis, cancellation);
    if (!prepared.ok) return failure(prepared.error);
    const auto finish = [&](const QVariantMap& result) {
        if (exported && flag(result, "ok", true) && !cancelled(cancellation)) {
            using Envelope = FleetMissionInterVehicleEnvelopeReview::Prepared;
            const auto value = std::shared_ptr<Envelope>(new Envelope);
            value->source = QStringLiteral("waypoint"); value->geometryBasis = geometryBasis;
            value->inputFingerprint = prepared.inputFingerprint; value->modelsFingerprint = prepared.modelsFingerprint;
            value->motionFingerprint = motionFingerprint(models);
            value->memberIds = prepared.memberIds; value->geometryComplete = prepared.modelComplete;
            value->calculationSource = true;
            if (prepared.modelComplete) {
                value->components.reserve(prepared.components.size());
                for (const Component& component : prepared.components) {
                    if (cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
                    Envelope::Component copy;
                    copy.vehicleId = component.vehicleId; copy.kind = component.componentKind;
                    copy.reference = {{QStringLiteral("fromSequence"), component.fromSequence},
                        {QStringLiteral("toSequence"), component.toSequence}, {QStringLiteral("regionKind"), component.regionKind}};
                    copy.envelope = component.envelope;
                    value->components.append(copy); // No terrain contact-pad waiver is exported.
                }
            }
            *exported = value;
        }
        if (cancelled(cancellation)) {
            if (exported) exported->reset();
            return failure(QStringLiteral("waypoint-switch-review-cancelled"));
        }
        return result;
    };
    if (!prepared.modelComplete) return finish(success(report(prepared, geometryBasis, false, 0, 0, {}, 0, false), cancellation));
    const QVariantMap scene = prepared.input.value(QStringLiteral("scene")).toMap();
    const QVariantList terrainRows = scene.value(QStringLiteral("terrainCells")).toList();
    const QVariantList obstacleRows = scene.value(QStringLiteral("obstacleVolumes")).toList();
    Geometry::Budget budget;
    budget.remainingWork = MaximumWork; budget.remainingFragments = MaximumFragments; budget.cancellation = cancellation;
    const auto preparationFailure = [&](const QString& reason) {
        if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
        if (budget.exhausted) return finish(success(report(prepared, geometryBasis, false, 0, 0, {}, MaximumWork - int(budget.remainingWork), true), cancellation));
        return failure(reason);
    };
    QVector<Geometry::PreparedPrism> terrainPrisms, obstaclePrisms;
    Volumes terrainContact, obstacleContact;
    // Prepare all original scene rows and both complete indices before any
    // query; preparation and every component share this SAME group budget.
    for (int collection = 0; collection < 2; ++collection) {
        const QVariantList& rows = collection == 0 ? terrainRows : obstacleRows;
        auto& prisms = collection == 0 ? terrainPrisms : obstaclePrisms;
        auto& contact = collection == 0 ? terrainContact : obstacleContact;
        prisms.reserve(rows.size());
        contact.reserve(rows.size());
        for (const QVariant& value : rows) {
            if (cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
            const QVariantMap row = value.toMap();
            QVector<Geometry::Point2> footprint;
            for (const QVariant& pointValue : row.value(QStringLiteral("polygon")).toList()) {
                const QVariantMap point = pointValue.toMap();
                Geometry::Point2 vertex; vertex.east = qint64(point.value(QStringLiteral("eastMm")).toDouble());
                vertex.north = qint64(point.value(QStringLiteral("northMm")).toDouble()); footprint.append(vertex);
            }
            const qint64 lower = collection == 0 ? -CoordinateBound : qint64(row.value(QStringLiteral("minimumAmslMm")).toDouble());
            const qint64 upper = qint64(row.value(collection == 0 ? QStringLiteral("maximumTerrainAmslMm") : QStringLiteral("maximumAmslMm")).toDouble());
            Geometry::PreparedPrism prism;
            if (!Geometry::prepareConvexPrism(footprint, lower, upper, &prism, &budget))
                return preparationFailure(QStringLiteral("waypoint-switch-scene-prism-invalid"));
            prisms.append(prism);
            // Exact same complete normalized scene row/order as the index.
            // The index supplies candidates only; these closed planes are
            // retained for the full-hull narrow intersection below.
            auto planes = FleetMissionContactModel::polygonPlanes(row.value(QStringLiteral("polygon")).toList());
            FleetMissionContactModel::altitudePlanes(&planes, lower, upper);
            contact.append(planes);
        }
    }
    Geometry::PreparedPrismIndex terrainIndex, obstacleIndex;
    if (!Geometry::preparePrismIndex(terrainPrisms, &terrainIndex, &budget)
        || !Geometry::preparePrismIndex(obstaclePrisms, &obstacleIndex, &budget))
        return preparationFailure(QStringLiteral("waypoint-switch-scene-index-invalid"));
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
    Geometry::PreparedPrismIndex coverageIndex, corridorIndex, terrainHorizontalIndex;
    if (!prepareCoverageIndex(scene.value(QStringLiteral("coverageVolumes")).toList(), false, &coverageIndex, &budget, cancellation)
        || !prepareCoverageIndex(scene.value(QStringLiteral("corridorVolumes")).toList(), false, &corridorIndex, &budget, cancellation)
        || !prepareCoverageIndex(terrainRows, true, &terrainHorizontalIndex, &budget, cancellation))
        return preparationFailure(QStringLiteral("waypoint-switch-coverage-prism-index-invalid"));
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
    int processed = 0, violationCount = 0;
    QVariantList violations;
    bool comparisonComplete = true;
    const auto consume = [&](Verdict value) {
        if (value != Verdict::Unknown) return true;
        comparisonComplete = false; return false;
    };
    const auto violate = [&](int index, const Component& component, const QString& code, const QString& volumeId) {
        ++violationCount;
        if (violations.size() < MaximumDetails) violations.append(QVariantMap{
            {QStringLiteral("componentIndex"), index}, {QStringLiteral("vehicleId"), component.vehicleId},
            {QStringLiteral("fromSequenceNumber"), component.fromSequence}, {QStringLiteral("toSequenceNumber"), component.toSequence},
            {QStringLiteral("regionKind"), component.regionKind}, {QStringLiteral("componentKind"), component.componentKind},
            {QStringLiteral("code"), code}, {QStringLiteral("volumeId"), volumeId}});
    };
    const auto intersects = [&budget](const Polytope& query, const Volumes& originalVolumes, int candidate) {
        if (candidate < 0 || candidate >= originalVolumes.size()) return Verdict::Unknown;
        const auto& volume = originalVolumes.at(candidate);
        if (volume.isEmpty() || volume.size() > 18 || query.planes.size() > 64 - volume.size()) return Verdict::Unknown;
        Polytope intersection = query;
        // Never replace the query by its AABB here. In particular a broad
        // hit near a diagonal hull's unused corner is not a contact verdict.
        intersection.planes += volume;
        return Geometry::feasible(intersection, &budget);
    };
    for (int index = 0; index < prepared.components.size(); ++index) {
        if (cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
        const Component& component = prepared.components.at(index);
        Verdict verdict = Geometry::coveredByIndexedPrisms(component.envelope, coverageIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(index, component, QStringLiteral("acceptance-envelope-not-covered"), QStringLiteral(""));
        verdict = Geometry::coveredByIndexedPrisms(component.envelope, corridorIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(index, component, QStringLiteral("acceptance-envelope-outside-corridor-union"), QStringLiteral(""));
        verdict = Geometry::coveredByIndexedPrisms(component.envelope, terrainHorizontalIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(index, component, QStringLiteral("acceptance-envelope-terrain-coverage-incomplete"), QStringLiteral(""));
        Polytope broadPhase;
        broadPhase.bounds = component.envelope.bounds;
        // Explicit bounds-only COPY for this AABB-only index API. Its hits
        // must be resolved using the original complete component below.
        QVector<int> candidates;
        if (!Geometry::prismCandidates(broadPhase, terrainIndex, &candidates, &budget)) { consume(Verdict::Unknown); break; }
        for (int candidate : candidates) {
            if (cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
            verdict = component.envelope.planes.isEmpty()
                ? Geometry::axisAlignedBoxIntersectsIndexedPrism(component.envelope, terrainIndex, candidate, &budget)
                : intersects(component.envelope, terrainContact, candidate);
            if (!consume(verdict)) break;
            const bool terrainHit = verdict == Verdict::ProvenTrue;
            bool permittedSupportContact = false;
            if (terrainHit && (component.regionKind == QStringLiteral("land-descent-reference")
                    || component.regionKind == QStringLiteral("takeoff-vertical-reference"))
                && component.componentKind == QStringLiteral("support") && !component.contactId.isEmpty()
                && !component.contactVolume.isEmpty()
                && component.allowedTerrainCellIds.contains(terrainRows.at(candidate).toMap().value(QStringLiteral("cellId")).toString())) {
                // Only the COMPLETE support/terrain intersection may fit in
                // this same-call action-bound finite pad band. Entry, NAV,
                // top-to-NAV, protected/body and every obstacle keep their
                // old rules; no nominal pad is transferred to DomainReview.
                const auto& terrain = terrainContact.at(candidate);
                if (terrain.isEmpty() || component.envelope.planes.size() > 64 - terrain.size()) {
                    consume(Verdict::Unknown); break;
                }
                Polytope intersection = component.envelope;
                intersection.planes += terrain;
                verdict = Geometry::coveredByUnion(intersection, {component.contactVolume}, &budget);
                if (!consume(verdict)) break;
                permittedSupportContact = verdict == Verdict::ProvenTrue;
            }
            if (terrainHit && !permittedSupportContact) violate(index, component, QStringLiteral("acceptance-envelope-terrain-contact-candidate"),
                terrainRows.at(candidate).toMap().value(QStringLiteral("cellId")).toString());
        }
        if (!comparisonComplete) break;
        if (!Geometry::prismCandidates(broadPhase, obstacleIndex, &candidates, &budget)) { consume(Verdict::Unknown); break; }
        for (int candidate : candidates) {
            if (cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
            verdict = component.envelope.planes.isEmpty()
                ? Geometry::axisAlignedBoxIntersectsIndexedPrism(component.envelope, obstacleIndex, candidate, &budget)
                : intersects(component.envelope, obstacleContact, candidate);
            if (!consume(verdict)) break;
            if (verdict == Verdict::ProvenTrue) violate(index, component, QStringLiteral("acceptance-envelope-obstacle-contact-candidate"),
                obstacleRows.at(candidate).toMap().value(QStringLiteral("volumeId")).toString());
        }
        if (!comparisonComplete) break;
        ++processed;
    }
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("waypoint-switch-review-cancelled"));
    return finish(success(report(prepared, geometryBasis, comparisonComplete, processed, violationCount, violations,
        MaximumWork - int(budget.remainingWork), budget.exhausted), cancellation));
}

bool FleetMissionWaypointSwitchReview::resultMatches(const QVariant& clearanceInput, const QVariantList& models,
    const QVariantMap& frozenJointView, const QString& geometryBasis, const QVariantMap& result, PreparedPtr* exported)
{
    if (exported) exported->reset();
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(result, &detached) || json(detached).size() > MaximumOutputBytes
        || !exact(detached, {"ok", "error", "report"}) || !flag(detached, "ok", true)
        || !text(detached.value(QStringLiteral("error")), 256, true) || !detached.value(QStringLiteral("error")).toString().isEmpty()
        || !mapValue(detached.value(QStringLiteral("report")))) return false;
    const QVariantMap observed = detached.value(QStringLiteral("report")).toMap();
    if (!exact(observed, {"schemaVersion", "reportType", "modelName", "geometryBasis", "clearanceInputFingerprint", "modelsFingerprint",
            "memberIds", "memberCount", "navWaypointCount", "navConnectionCount", "referenceRegionCount", "componentCount",
            "processedComponentCount", "actionBoundaries", "modelComplete", "comparisonComplete", "declaredModelSatisfied", "violationCount",
            "violations", "violationsTruncated", "reasonCodes", "workUnits", "limits", "snapshotOnly", "numericAcceptanceContainmentProven",
            "actualPathCertified", "dynamicAltitudeAcceptanceProven", "smoothingReachableSetProven", "executionPermit", "flightReleased", "clearanceCertified"})
        || !integer(observed.value(QStringLiteral("schemaVersion")), 1, 1)
        || observed.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-waypoint-switch-review"))
        || observed.value(QStringLiteral("modelName")) != QVariant(ModelName) || !flag(observed, "snapshotOnly", true)
        || !mapValue(observed.value(QStringLiteral("limits"))) || json(observed.value(QStringLiteral("limits"))) != json(limits())) return false;
    for (const char* name : {"numericAcceptanceContainmentProven", "actualPathCertified", "dynamicAltitudeAcceptanceProven",
            "smoothingReachableSetProven", "executionPermit", "flightReleased", "clearanceCertified"})
        if (!flag(observed, name, false)) return false;
    for (const char* name : {"modelComplete", "comparisonComplete", "declaredModelSatisfied", "violationsTruncated"})
        if (!boolean(observed, name)) return false;
    for (const char* name : {"memberIds", "actionBoundaries", "violations", "reasonCodes"})
        if (!listValue(observed.value(QString::fromLatin1(name)))) return false;
    const Prepared prepared = prepare(clearanceInput, models, frozenJointView, geometryBasis, {});
    if (!prepared.ok || observed.value(QStringLiteral("geometryBasis")) != QVariant(geometryBasis)
        || observed.value(QStringLiteral("clearanceInputFingerprint")) != QVariant(prepared.inputFingerprint)
        || observed.value(QStringLiteral("modelsFingerprint")) != QVariant(prepared.modelsFingerprint)
        || json(observed.value(QStringLiteral("memberIds"))) != json(prepared.memberIds)
        || json(observed.value(QStringLiteral("actionBoundaries"))) != json(prepared.actions)
        || !integer(observed.value(QStringLiteral("memberCount")), prepared.memberIds.size(), prepared.memberIds.size())
        || !integer(observed.value(QStringLiteral("navWaypointCount")), prepared.waypointCount, prepared.waypointCount)
        || !integer(observed.value(QStringLiteral("navConnectionCount")), prepared.connectionCount, prepared.connectionCount)
        || !integer(observed.value(QStringLiteral("referenceRegionCount")), prepared.regionCount, prepared.regionCount)
        || !integer(observed.value(QStringLiteral("componentCount")), prepared.components.size(), prepared.components.size())
        || !flag(observed, "modelComplete", prepared.modelComplete)
        || !integer(observed.value(QStringLiteral("processedComponentCount")), 0, prepared.components.size())
        || !integer(observed.value(QStringLiteral("workUnits")), 0, MaximumWork)) return false;
    const int components = prepared.components.size();
    const int processed = observed.value(QStringLiteral("processedComponentCount")).toInt();
    const bool complete = observed.value(QStringLiteral("comparisonComplete")).toBool();
    const QVariantMap scene = prepared.input.value(QStringLiteral("scene")).toMap();
    const int checks = 3 + scene.value(QStringLiteral("terrainCells")).toList().size() + scene.value(QStringLiteral("obstacleVolumes")).toList().size();
    if (!integer(observed.value(QStringLiteral("violationCount")), 0, (complete ? components : std::min(components, processed + 1)) * checks)) return false;
    const int count = observed.value(QStringLiteral("violationCount")).toInt();
    const QVariantList violations = observed.value(QStringLiteral("violations")).toList();
    if (violations.size() != std::min(count, MaximumDetails) || !flag(observed, "violationsTruncated", count > MaximumDetails)
        || !flag(observed, "declaredModelSatisfied", prepared.modelComplete && complete && count == 0)
        || (prepared.modelComplete ? (complete ? processed != components : processed >= components)
            : (complete || processed != 0 || count != 0 || observed.value(QStringLiteral("workUnits")).toInt() != 0))) return false;
    const QVariantList reasons = observed.value(QStringLiteral("reasonCodes")).toList();
    if (json(reasons) != json(reasonsFor(prepared, complete, reasons.contains(ResourceReason), count))) return false;
    QSet<QString> terrainIds, obstacleIds;
    for (const QVariant& value : scene.value(QStringLiteral("terrainCells")).toList()) terrainIds.insert(value.toMap().value(QStringLiteral("cellId")).toString());
    for (const QVariant& value : scene.value(QStringLiteral("obstacleVolumes")).toList()) obstacleIds.insert(value.toMap().value(QStringLiteral("volumeId")).toString());
    const QSet<QString> noVolumeCodes{QStringLiteral("acceptance-envelope-not-covered"), QStringLiteral("acceptance-envelope-outside-corridor-union"),
        QStringLiteral("acceptance-envelope-terrain-coverage-incomplete")};
    for (const QVariant& value : violations) {
        if (!mapValue(value)) return false;
        const QVariantMap row = value.toMap();
        if (!exact(row, {"componentIndex", "vehicleId", "fromSequenceNumber", "toSequenceNumber", "regionKind", "componentKind", "code", "volumeId"})
            || !integer(row.value(QStringLiteral("componentIndex")), 0, components - 1) || !text(row.value(QStringLiteral("code")))
            || !text(row.value(QStringLiteral("volumeId")), 128, true)) return false;
        const int index = row.value(QStringLiteral("componentIndex")).toInt();
        if (!complete && index > processed) return false;
        const Component& component = prepared.components.at(index);
        if (!integer(row.value(QStringLiteral("vehicleId")), component.vehicleId, component.vehicleId)
            || !integer(row.value(QStringLiteral("fromSequenceNumber")), component.fromSequence, component.fromSequence)
            || !integer(row.value(QStringLiteral("toSequenceNumber")), component.toSequence, component.toSequence)
            || row.value(QStringLiteral("regionKind")) != QVariant(component.regionKind)
            || row.value(QStringLiteral("componentKind")) != QVariant(component.componentKind)) return false;
        const QString code = row.value(QStringLiteral("code")).toString(), id = row.value(QStringLiteral("volumeId")).toString();
        if (noVolumeCodes.contains(code)) { if (!id.isEmpty()) return false; }
        else if (code == QStringLiteral("acceptance-envelope-terrain-contact-candidate")) { if (!terrainIds.contains(id)) return false; }
        else if (code == QStringLiteral("acceptance-envelope-obstacle-contact-candidate")) { if (!obstacleIds.contains(id)) return false; }
        else return false;
    }
    if (exported) {
        using Envelope = FleetMissionInterVehicleEnvelopeReview::Prepared;
        const auto value = std::shared_ptr<Envelope>(new Envelope);
        value->source = QStringLiteral("waypoint"); value->geometryBasis = geometryBasis;
        value->inputFingerprint = prepared.inputFingerprint; value->modelsFingerprint = prepared.modelsFingerprint;
        value->motionFingerprint = motionFingerprint(models);
        value->memberIds = prepared.memberIds; value->geometryComplete = prepared.modelComplete;
        value->calculationSource = false;
        if (prepared.modelComplete) {
            value->components.reserve(prepared.components.size());
            for (const Component& component : prepared.components) {
                Envelope::Component copy;
                copy.vehicleId = component.vehicleId; copy.kind = component.componentKind;
                copy.reference = {{QStringLiteral("fromSequence"), component.fromSequence},
                    {QStringLiteral("toSequence"), component.toSequence}, {QStringLiteral("regionKind"), component.regionKind}};
                copy.envelope = component.envelope;
                value->components.append(copy);
            }
        }
        *exported = value;
    }
    return true;
}

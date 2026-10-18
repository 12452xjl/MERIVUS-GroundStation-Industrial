#include "FleetAirspaceConflictChecker.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>

namespace {

struct RawPoint {
    bool valid = false;
    bool geodetic = false;
    bool altitudeKnown = false;
    double first = 0.0;
    double second = 0.0;
    double altitude = 0.0;
};

struct Point {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct DeclaredReference {
    QSet<QString> values;
    bool missing = false;
    bool invalid = false;
};

struct RawPath {
    QString vehicleId;
    QString kind;
    int planIndex = -1;
    QVariantList sourcePoints;
    QList<RawPoint> rawPoints;
    QList<double> times;
    QString timingEvidence;
    DeclaredReference frameReference;
    DeclaredReference altitudeReference;
    DeclaredReference timeReference;
    bool timingKnown = false;
    bool timingInvalid = false;
    bool coordinateKnown = true;
    bool altitudeKnown = true;
    bool returnRequired = false;
};

struct Path {
    QString vehicleId;
    QString kind;
    int planIndex = -1;
    QList<Point> points;
    QList<double> times;
    QString timingEvidence;
    bool timingKnown = false;
    bool timingInvalid = false;
    bool coordinateKnown = true;
    bool altitudeKnown = true;
    bool returnRequired = false;
};

struct Zone {
    QString id;
    QList<RawPoint> rawPolygon;
    DeclaredReference frameReference;
    DeclaredReference altitudeReference;
    DeclaredReference timeReference;
    double minAltitude = -1.0e30;
    double maxAltitude = 1.0e30;
    double startTime = -1.0e30;
    double endTime = 1.0e30;
    bool timingKnown = false;
    bool polygonKnown = false;
    bool timeWindowInvalid = false;
    bool altitudeInvalid = false;
    bool altitudeBounded = false;
};

QVariant firstValue(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (map.contains(key) && map.value(key).isValid() && !map.value(key).isNull()) {
            return map.value(key);
        }
    }
    return QVariant();
}

bool finiteNumber(const QVariant& value, double* output)
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
    if (output) *output = number;
    return true;
}

bool strictBool(const QVariantMap& map, const QString& key, bool defaultValue,
                bool* valid = nullptr)
{
    if (valid) {
        *valid = true;
    }
    if (!map.contains(key)) {
        return defaultValue;
    }
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool) {
        if (valid) {
            *valid = false;
        }
        return defaultValue;
    }
    return value.toBool();
}

QVariantList listValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) return value.toList();
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) result.append(item);
        return result;
    }
    if (value.type() == QVariant::Map) return QVariantList() << value;
    return QVariantList() << value;
}

void mergeReference(DeclaredReference* output, const DeclaredReference& source)
{
    output->values.unite(source.values);
    output->missing = output->missing || source.missing;
    output->invalid = output->invalid || source.invalid;
}

DeclaredReference declaredReference(const QList<QVariantMap>& sources, const QString& key)
{
    DeclaredReference result;
    bool present = false;
    for (const QVariantMap& source : sources) {
        if (!source.contains(key)) continue;
        present = true;
        const QVariant value = source.value(key);
        if (value.type() != QVariant::String || value.toString().size() > 128) {
            result.invalid = true;
            continue;
        }
        const QString reference = value.toString().trimmed();
        const QString lower = reference.toLower();
        bool invalid = reference.isEmpty() || lower == QStringLiteral("unknown")
            || lower == QStringLiteral("unspecified") || lower == QStringLiteral("none");
        for (const QChar character : reference) invalid = invalid || character.isNull()
            || character.category() == QChar::Other_Control;
        if (key == QStringLiteral("altitudeReference")) {
            // Home-relative and terrain-relative heights have no common
            // origin supplied by this schema; they cannot certify separation.
            invalid = invalid || (lower != QStringLiteral("amsl") && lower != QStringLiteral("ellipsoid")
                && lower != QStringLiteral("local_origin"));
        }
        if (invalid) result.invalid = true;
        else result.values.insert(key == QStringLiteral("altitudeReference") ? lower : reference);
    }
    result.missing = !present;
    result.invalid = result.invalid || result.values.size() > 1;
    return result;
}

bool referenceIncompatible(const DeclaredReference& reference)
{
    return reference.invalid || reference.values.size() > 1
        || (!reference.values.isEmpty() && reference.missing);
}

QString idFor(const QVariantMap& map)
{
    const QVariant value = firstValue(map, QStringList()
        << QStringLiteral("vehicleId") << QStringLiteral("systemId")
        << QStringLiteral("id"));
    if (!value.isValid() || value.isNull()) return QString();
    double numeric = 0.0;
    if (finiteNumber(value, &numeric)) {
        if (numeric < 1.0 || numeric > 255.0 || qFloor(numeric) != numeric) return QString();
        return QString::number(static_cast<int>(numeric));
    }
    // Keep the airspace checker aligned with FleetMissionCompiler and
    // ContinuityPlanner: a MAVLink identity must be a real finite integer,
    // not a numeric string/alias such as "01" or a display name.
    return QString();
}

bool hasTimeField(const QVariantMap& map)
{
    const QStringList keys = QStringList()
        << QStringLiteral("timeSeconds") << QStringLiteral("timestamp")
        << QStringLiteral("time") << QStringLiteral("t")
        << QStringLiteral("etaSeconds");
    for (const QString& key : keys) {
        if (map.contains(key)) return true;
    }
    return false;
}

RawPoint rawPoint(const QVariant& value)
{
    QVariantMap map = value.toMap();
    if (map.contains(QStringLiteral("position"))) {
        const QVariantMap nested = map.value(QStringLiteral("position")).toMap();
        if (!nested.isEmpty()) map = nested;
    }

    double first = 0.0;
    double second = 0.0;
    double altitude = 0.0;
    RawPoint point;
    const bool hasLat = finiteNumber(firstValue(map,
        QStringList() << QStringLiteral("latitude") << QStringLiteral("lat")), &first);
    const bool hasLon = finiteNumber(firstValue(map,
        QStringList() << QStringLiteral("longitude") << QStringLiteral("lon")
                       << QStringLiteral("lng")), &second);
    if (hasLat && hasLon && first >= -90.0 && first <= 90.0 && second >= -180.0 && second <= 180.0) {
        point.geodetic = true;
        point.first = first;
        point.second = second;
    } else {
        const bool hasX = finiteNumber(firstValue(map,
            QStringList() << QStringLiteral("x") << QStringLiteral("east")
                           << QStringLiteral("eastMeters")), &first);
        const bool hasY = finiteNumber(firstValue(map,
            QStringList() << QStringLiteral("y") << QStringLiteral("north")
                           << QStringLiteral("northMeters")), &second);
        if (!hasX || !hasY) return point;
        point.first = first;
        point.second = second;
    }
    const QVariant altitudeValue = firstValue(map,
        QStringList() << QStringLiteral("altitude") << QStringLiteral("alt")
                       << QStringLiteral("z") << QStringLiteral("height"));
    const bool altitudePresent = altitudeValue.isValid() && !altitudeValue.isNull();
    if (altitudePresent && !finiteNumber(altitudeValue, &altitude)) return point;
    point.altitude = altitude;
    point.altitudeKnown = altitudePresent;
    point.valid = true;
    return point;
}

bool timeValue(const QVariantMap& map, double* output)
{
    return finiteNumber(firstValue(map, QStringList()
        << QStringLiteral("timeSeconds") << QStringLiteral("timestamp")
        << QStringLiteral("time") << QStringLiteral("t")
        << QStringLiteral("etaSeconds")), output);
}

bool timingEvidenceIsAuthoritative(const QVariantMap& map, bool* invalid)
{
    if (!map.contains(QStringLiteral("timingEvidence"))) return true;
    const QVariant value = map.value(QStringLiteral("timingEvidence"));
    if (value.type() != QVariant::String || value.toString().size() > 128 || value.toString().trimmed().isEmpty()) {
        if (invalid) *invalid = true;
        return false;
    }
    const QString evidence = value.toString().trimmed().toLower();
    // Explicit timestamps/duration remain the existing caller-supplied
    // contract. A generated constant-speed schedule is not 4-D evidence.
    return evidence == QStringLiteral("authoritative")
        || evidence == QStringLiteral("explicit")
        || evidence == QStringLiteral("waypoint-timestamps")
        || evidence == QStringLiteral("explicit-duration");
}

QString boundedEvidenceName(const QVariant& value)
{
    return value.type() == QVariant::String && value.toString().size() <= 128
        ? value.toString() : value.isValid() ? QStringLiteral("invalid") : QString();
}

void fillTimes(const QVariantMap& plan, RawPath* path)
{
    if (!path || path->rawPoints.isEmpty()) return;
    const QVariantList source = path->sourcePoints;
    bool anyKnown = false;
    bool allKnown = true;
    bool authoritative = timingEvidenceIsAuthoritative(plan, &path->timingInvalid);
    path->timingEvidence = boundedEvidenceName(plan.value(QStringLiteral("timingEvidence")));
    for (int index = 0; index < path->rawPoints.size(); ++index) {
        double time = 0.0;
        const QVariantMap point = index < source.size() ? source.at(index).toMap() : QVariantMap();
        const bool pointAuthoritative = timingEvidenceIsAuthoritative(point, &path->timingInvalid);
        authoritative = pointAuthoritative && authoritative;
        if ((!pointAuthoritative || path->timingEvidence.isEmpty())
            && point.contains(QStringLiteral("timingEvidence"))) {
            path->timingEvidence = boundedEvidenceName(point.value(QStringLiteral("timingEvidence")));
        }
        if (timeValue(point, &time)) {
            path->times.append(time);
            anyKnown = true;
        } else {
            if (hasTimeField(point)) path->timingInvalid = true;
            allKnown = false;
            path->times.append(qQNaN());
        }
    }

    double start = 0.0;
    bool hasStart = timeValue(plan, &start);
    if (!hasStart && plan.contains(QStringLiteral("startTime"))) {
        if (!finiteNumber(plan.value(QStringLiteral("startTime")), &start)) path->timingInvalid = true;
        else hasStart = true;
    }
    double end = 0.0;
    const bool hasEnd = plan.contains(QStringLiteral("endTime"))
        && finiteNumber(plan.value(QStringLiteral("endTime")), &end);
    if (plan.contains(QStringLiteral("endTime")) && !hasEnd) path->timingInvalid = true;
    if (hasStart && hasEnd && end < start) path->timingInvalid = true;
    double duration = 0.0;
    const QVariant durationSource = firstValue(plan, QStringList()
        << QStringLiteral("durationSeconds") << QStringLiteral("missionDurationSeconds"));
    const bool durationFieldPresent = plan.contains(QStringLiteral("durationSeconds"))
        || plan.contains(QStringLiteral("missionDurationSeconds"));
    const bool hasDuration = durationSource.isValid()
        && finiteNumber(durationSource, &duration) && duration > 0.0;
    if (durationFieldPresent && !hasDuration) path->timingInvalid = true;
    if (!anyKnown) {
        if (hasDuration && path->rawPoints.size() > 1) {
            for (int index = 0; index < path->times.size(); ++index) {
                path->times[index] = start + duration * index / (path->times.size() - 1);
                if (!qIsFinite(path->times.at(index))) path->timingInvalid = true;
            }
            path->timingKnown = authoritative && !path->timingInvalid
                && (path->kind != QStringLiteral("return") || hasStart);
        } else {
            // A deterministic relative timeline lets the checker still find
            // spatial conflicts, but the result remains marked for review.
            for (int index = 0; index < path->times.size(); ++index) path->times[index] = start + index;
            path->timingKnown = false;
        }
        if (hasEnd && hasDuration && start + duration > end) {
            path->timingInvalid = true;
            path->timingKnown = false;
        }
        return;
    }

    // Visit each gap once. Repeated right-anchor scans over a sparse path
    // would make timing preparation quadratic before geometry work budgets.
    int previousAnchor = -1;
    for (int index = 0; index < path->times.size(); ++index) {
        if (!qIsFinite(path->times.at(index))) continue;
        if (previousAnchor < 0) {
            for (int missing = 0; missing < index; ++missing) {
                path->times[missing] = path->times.at(index) - (index - missing);
            }
        } else {
            for (int missing = previousAnchor + 1; missing < index; ++missing) {
                const double fraction = double(missing - previousAnchor) / double(index - previousAnchor);
                path->times[missing] = path->times.at(previousAnchor)
                    + (path->times.at(index) - path->times.at(previousAnchor)) * fraction;
            }
        }
        previousAnchor = index;
    }
    for (int missing = previousAnchor + 1; previousAnchor >= 0 && missing < path->times.size(); ++missing) {
        path->times[missing] = path->times.at(previousAnchor) + (missing - previousAnchor);
    }
    // Interpolated points are useful for a conservative spatial fallback but
    // are not authoritative 4-D timing evidence.  A partial timestamp set
    // must therefore remain timingUnknown even when the filled sequence is
    // monotonic; otherwise pairwise conflict checks would treat estimates as
    // exact time windows.
    path->timingKnown = authoritative && allKnown && !path->timingInvalid;
    for (int index = 1; index < path->times.size(); ++index) {
        if (!qIsFinite(path->times.at(index)) || path->times.at(index) < path->times.at(index - 1)) {
            path->timingKnown = false;
            path->timingInvalid = true;
        }
        if (path->times.at(index) == path->times.at(index - 1)) {
            const RawPoint& prior = path->rawPoints.at(index - 1);
            const RawPoint& current = path->rawPoints.at(index);
            if (prior.geodetic != current.geodetic || prior.first != current.first
                || prior.second != current.second || prior.altitude != current.altitude) {
                // Different positions cannot be traversed in zero time;
                // duplicate timestamps are only valid for a stationary point.
                path->timingKnown = false;
                path->timingInvalid = true;
            }
        }
    }
    if (!path->times.isEmpty()
        && ((hasStart && path->times.first() < start) || (hasEnd && path->times.last() > end))) {
        path->timingKnown = false;
        path->timingInvalid = true;
    }
}

QVariantList pathSource(const QVariantMap& map, bool returnPath)
{
    QVariant value;
    if (returnPath) {
        value = firstValue(map, QStringList()
            << QStringLiteral("returnPath") << QStringLiteral("returnWaypoints")
            << QStringLiteral("rtlPath") << QStringLiteral("homePath"));
    } else {
        value = firstValue(map, QStringList()
            << QStringLiteral("waypoints") << QStringLiteral("route")
            << QStringLiteral("path"));
    }
    if (value.type() == QVariant::Map) {
        const QVariantMap nested = value.toMap();
        const QVariant nestedPoints = firstValue(nested,
            QStringList() << QStringLiteral("waypoints") << QStringLiteral("route")
                           << QStringLiteral("points") << QStringLiteral("path"));
        if (nestedPoints.isValid()) value = nestedPoints;
    }
    return listValue(value);
}

int geometryPointCount(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return 0;
    if (value.type() == QVariant::List) return value.toList().size();
    if (value.type() == QVariant::StringList) return value.toStringList().size();
    return 1;
}

int pathPointCount(const QVariantMap& map, bool returning)
{
    QVariant source = returning
        ? firstValue(map, QStringList() << QStringLiteral("returnPath") << QStringLiteral("returnWaypoints")
                     << QStringLiteral("rtlPath") << QStringLiteral("homePath"))
        : firstValue(map, QStringList() << QStringLiteral("waypoints") << QStringLiteral("route") << QStringLiteral("path"));
    if (source.type() == QVariant::Map) {
        const QVariant nested = firstValue(source.toMap(), QStringList() << QStringLiteral("waypoints")
            << QStringLiteral("route") << QStringLiteral("points") << QStringLiteral("path"));
        if (nested.isValid()) source = nested;
    }
    return geometryPointCount(source);
}

RawPath makePath(const QVariantMap& plan, int planIndex, const QString& kind, bool returnRequired)
{
    RawPath path;
    path.planIndex = planIndex;
    path.kind = kind;
    path.vehicleId = idFor(plan);
    path.returnRequired = returnRequired;
    path.sourcePoints = pathSource(plan, kind == QStringLiteral("return"));
    for (const QVariant& value : path.sourcePoints) {
        const RawPoint point = rawPoint(value);
        path.rawPoints.append(point);
        if (!point.valid) path.coordinateKnown = false;
        if (!point.altitudeKnown) path.altitudeKnown = false;
    }
    QVariantMap timingPlan = plan;
    const bool returnPath = kind == QStringLiteral("return");
    const QStringList timingKeys = QStringList() << QStringLiteral("durationSeconds")
        << QStringLiteral("startTime") << QStringLiteral("endTime")
        << QStringLiteral("timingEvidence") << QStringLiteral("timeReference");
    if (returnPath) {
        // Outbound travel duration/start do not describe a return corridor.
        // A nested return route or explicit return-prefixed fields supply
        // an independent schedule; otherwise return timing stays unknown.
        timingPlan.clear();
        const QStringList returnKeys = QStringList() << QStringLiteral("returnDurationSeconds")
            << QStringLiteral("returnStartTime") << QStringLiteral("returnEndTime")
            << QStringLiteral("returnTimingEvidence") << QStringLiteral("returnTimeReference");
        for (int index = 0; index < returnKeys.size(); ++index) {
            if (plan.contains(returnKeys.at(index))) {
                timingPlan.insert(timingKeys.at(index), plan.value(returnKeys.at(index)));
            }
        }
    }
    const QVariant routeSource = returnPath
        ? firstValue(plan, QStringList()
            << QStringLiteral("returnPath") << QStringLiteral("returnWaypoints")
            << QStringLiteral("rtlPath") << QStringLiteral("homePath"))
        : firstValue(plan, QStringList() << QStringLiteral("waypoints")
                     << QStringLiteral("route") << QStringLiteral("path"));
    if (routeSource.type() == QVariant::Map) {
        const QVariantMap nested = routeSource.toMap();
        bool timingConflict = false;
        const auto mergeField = [&timingPlan, &nested, &timingConflict](const QString& key,
                                                                    const QString& alias) {
            if (!nested.contains(alias)) return;
            const QVariant value = nested.value(alias);
            if (timingPlan.contains(key) && timingPlan.value(key) != value) timingConflict = true;
            timingPlan.insert(key, value);
        };
        for (const QString& key : timingKeys) mergeField(key, key);
        mergeField(QStringLiteral("startTime"), QStringLiteral("startAt"));
        mergeField(QStringLiteral("endTime"), QStringLiteral("deadline"));
        mergeField(QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"));
        mergeField(QStringLiteral("durationSeconds"), QStringLiteral("taskDurationSeconds"));
        if (timingConflict) {
            // Do not hide a malformed/conflicting outer schedule by choosing
            // a valid nested one (or vice versa).
            timingPlan.insert(QStringLiteral("timingEvidence"), QVariant());
        }
    }
    const QVariantMap routeMetadata = routeSource.toMap();
    for (int index = 0; index < path.sourcePoints.size(); ++index) {
        const QVariantMap point = path.sourcePoints.at(index).toMap();
        const QList<QVariantMap> coordinateSources = QList<QVariantMap>() << plan << routeMetadata
            << point << point.value(QStringLiteral("position")).toMap();
        DeclaredReference frame = declaredReference(coordinateSources, QStringLiteral("localFrameId"));
        DeclaredReference altitude = declaredReference(coordinateSources, QStringLiteral("altitudeReference"));
        if (index < path.rawPoints.size() && path.rawPoints.at(index).geodetic && !frame.values.isEmpty()) {
            frame.invalid = true;
        }
        if (altitude.values.contains(QStringLiteral("local_origin"))
            && (frame.values.isEmpty() || referenceIncompatible(frame))) altitude.invalid = true;
        mergeReference(&path.frameReference, frame);
        mergeReference(&path.altitudeReference, altitude);
        mergeReference(&path.timeReference, declaredReference(QList<QVariantMap>() << timingPlan << point,
                                                             QStringLiteral("timeReference")));
    }
    fillTimes(timingPlan, &path);
    return path;
}

Point toLocal(const RawPoint& raw, bool* coordinateMismatch, bool referenceSet,
             double referenceLat, double referenceLon)
{
    Point point;
    if (!raw.valid) return point;
    if (!referenceSet && raw.geodetic) {
        if (coordinateMismatch) *coordinateMismatch = true;
        return point;
    }
    if (raw.geodetic) {
        const double metersPerDegreeLat = 110540.0;
        const double metersPerDegreeLon = 111320.0 * qCos(qDegreesToRadians(referenceLat));
        point.x = (raw.second - referenceLon) * metersPerDegreeLon;
        point.y = (raw.first - referenceLat) * metersPerDegreeLat;
    } else {
        point.x = raw.first;
        point.y = raw.second;
    }
    point.z = raw.altitude;
    return point;
}

double distance(const Point& a, const Point& b)
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    const double dz = a.z - b.z;
    return qSqrt(dx * dx + dy * dy + dz * dz);
}

Point interpolate(const Point& a, const Point& b, double fraction)
{
    Point point;
    point.x = a.x + (b.x - a.x) * fraction;
    point.y = a.y + (b.y - a.y) * fraction;
    point.z = a.z + (b.z - a.z) * fraction;
    return point;
}

double minSpatialDistance(const Point& a0, const Point& a1,
                          const Point& b0, const Point& b1)
{
    // Minimize the convex distance quadratic over [0,1]^2. Checking the
    // four boundary minima and a valid interior stationary point covers
    // crossing, skew, parallel and degenerate segments without sampling.
    const Point u{a1.x - a0.x, a1.y - a0.y, a1.z - a0.z};
    const Point v{b1.x - b0.x, b1.y - b0.y, b1.z - b0.z};
    const Point w{a0.x - b0.x, a0.y - b0.y, a0.z - b0.z};
    const auto dot = [](const Point& left, const Point& right) {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    };
    const auto cross = [](const Point& left, const Point& right) {
        return Point{left.y * right.z - left.z * right.y,
                     left.z * right.x - left.x * right.z,
                     left.x * right.y - left.y * right.x};
    };
    const double uu = dot(u, u);
    const double uv = dot(u, v);
    const double vv = dot(v, v);
    const double uw = dot(u, w);
    const double vw = dot(v, w);
    const auto candidate = [&a0, &a1, &b0, &b1](double s, double t) {
        return distance(interpolate(a0, a1, qBound(0.0, s, 1.0)),
                        interpolate(b0, b1, qBound(0.0, t, 1.0)));
    };
    double best = qMin(candidate(0.0, vv > 0.0 ? vw / vv : 0.0),
                       candidate(1.0, vv > 0.0 ? (vw + uv) / vv : 0.0));
    best = qMin(best, candidate(uu > 0.0 ? -uw / uu : 0.0, 0.0));
    best = qMin(best, candidate(uu > 0.0 ? (uv - uw) / uu : 0.0, 1.0));
    const Point normal = cross(u, v);
    const double determinant = dot(normal, normal);
    if (determinant > 0.0) {
        const double s = dot(cross(v, w), normal) / determinant;
        const double t = dot(cross(u, w), normal) / determinant;
        if (s >= 0.0 && s <= 1.0 && t >= 0.0 && t <= 1.0) best = qMin(best, candidate(s, t));
    }
    return best;
}

bool pointInPolygon(const Point& point, const QList<Point>& polygon)
{
    if (polygon.size() < 3) return false;
    bool inside = false;
    for (int index = 0, previous = polygon.size() - 1; index < polygon.size(); previous = index++) {
        const Point& current = polygon.at(index);
        const Point& prior = polygon.at(previous);
        const bool intersects = ((current.y > point.y) != (prior.y > point.y))
            && (point.x < (prior.x - current.x) * (point.y - current.y)
                / ((prior.y - current.y) == 0.0 ? 1.0e-12 : (prior.y - current.y)) + current.x);
        if (intersects) inside = !inside;
    }
    return inside;
}

double cross2D(const Point& origin, const Point& first, const Point& second)
{
    return (first.x - origin.x) * (second.y - origin.y)
        - (first.y - origin.y) * (second.x - origin.x);
}

bool pointOnSegment2D(const Point& point, const Point& first, const Point& second)
{
    constexpr double epsilon = 1.0e-9;
    if (qAbs(cross2D(first, second, point)) > epsilon) return false;
    return point.x >= qMin(first.x, second.x) - epsilon
        && point.x <= qMax(first.x, second.x) + epsilon
        && point.y >= qMin(first.y, second.y) - epsilon
        && point.y <= qMax(first.y, second.y) + epsilon;
}

bool segmentsIntersect2D(const Point& firstStart, const Point& firstEnd,
                         const Point& secondStart, const Point& secondEnd)
{
    constexpr double epsilon = 1.0e-9;
    const double firstOrientation = cross2D(firstStart, firstEnd, secondStart);
    const double secondOrientation = cross2D(firstStart, firstEnd, secondEnd);
    const double thirdOrientation = cross2D(secondStart, secondEnd, firstStart);
    const double fourthOrientation = cross2D(secondStart, secondEnd, firstEnd);
    const bool properIntersection = ((firstOrientation > epsilon && secondOrientation < -epsilon)
                                     || (firstOrientation < -epsilon && secondOrientation > epsilon))
        && ((thirdOrientation > epsilon && fourthOrientation < -epsilon)
            || (thirdOrientation < -epsilon && fourthOrientation > epsilon));
    if (properIntersection) return true;
    return (qAbs(firstOrientation) <= epsilon && pointOnSegment2D(secondStart, firstStart, firstEnd))
        || (qAbs(secondOrientation) <= epsilon && pointOnSegment2D(secondEnd, firstStart, firstEnd))
        || (qAbs(thirdOrientation) <= epsilon && pointOnSegment2D(firstStart, secondStart, secondEnd))
        || (qAbs(fourthOrientation) <= epsilon && pointOnSegment2D(firstEnd, secondStart, secondEnd));
}

bool segmentIntersectsPolygon2D(const Point& start, const Point& end,
                                const QList<Point>& polygon)
{
    if (polygon.size() < 3) return false;
    if (pointInPolygon(start, polygon) || pointInPolygon(end, polygon)) return true;
    for (int index = 0; index < polygon.size(); ++index) {
        const Point& edgeStart = polygon.at(index);
        const Point& edgeEnd = polygon.at((index + 1) % polygon.size());
        if (segmentsIntersect2D(start, end, edgeStart, edgeEnd)) return true;
    }
    return false;
}

QString severityForDistance(double actual, double minimum)
{
    if (actual <= 0.01) return QStringLiteral("critical");
    if (actual < minimum * 0.5) return QStringLiteral("high");
    return QStringLiteral("warning");
}

void addAlternative(QStringList* alternatives, const QString& value)
{
    if (alternatives && !alternatives->contains(value)) alternatives->append(value);
}

QVariantMap issue(const QString& type, const QString& severity, const QString& reason)
{
    QVariantMap result;
    result.insert(QStringLiteral("type"), type);
    result.insert(QStringLiteral("severity"), severity);
    result.insert(QStringLiteral("reason"), reason);
    return result;
}

constexpr qint64 ArchivedSpatialBytes = 16LL * 1024 * 1024;
constexpr int ArchivedSpatialNodes = 1000000;

bool archivedNull(const QVariant& value)
{
    return !value.isValid() || value.userType() == QMetaType::Nullptr;
}

bool archivedTree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 20 || ++*nodes > ArchivedSpatialNodes) return false;
    if (archivedNull(value)) return true;
    // Native empty QStrings remain ordinary JSON strings. Other typed-null
    // values (including QObject pointers) are not archived JSON evidence.
    if (value.isNull() && value.type() != QVariant::String) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > ArchivedSpatialNodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > ArchivedSpatialBytes) return false;
            const QByteArray key = it.key().toUtf8();
            *bytes += key.size();
            if (*bytes > ArchivedSpatialBytes || QString::fromUtf8(key) != it.key()
                || !archivedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > ArchivedSpatialNodes) return false;
        for (const QVariant& item : list)
            if (!archivedTree(item, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        const QStringList list = value.toStringList();
        if (list.size() > ArchivedSpatialNodes) return false;
        for (const QString& item : list)
            if (!archivedTree(item, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        const QString string = value.toString();
        if (string.size() > ArchivedSpatialBytes) return false;
        const QByteArray encoded = string.toUtf8();
        *bytes += encoded.size();
        return *bytes <= ArchivedSpatialBytes && QString::fromUtf8(encoded) == string;
    }
    if (value.type() == QVariant::Bool) return true;
    double number = 0.0;
    if (!finiteNumber(value, &number)) return false;
    return (value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong)
        || qAbs(number) <= 9007199254740991.0;
}

bool boundedArchivedEvidence(const QVariantMap& plan, const QVariantMap& evidence)
{
    const QVariantMap group{{QStringLiteral("plan"), plan}, {QStringLiteral("reviewEvidence"), evidence}};
    int nodes = 0;
    qint64 bytes = 0;
    if (!archivedTree(group, 0, &nodes, &bytes)) return false;
    const QByteArray encoded = QJsonDocument::fromVariant(group).toJson(QJsonDocument::Compact);
    return !encoded.isEmpty() && encoded.size() <= ArchivedSpatialBytes;
}

}

FleetAirspaceConflictChecker::FleetAirspaceConflictChecker(QObject* parent)
    : QObject(parent)
{
}

QVariantMap FleetAirspaceConflictChecker::inspectNoFlyZoneInput(const QVariant& input)
{
    QVariantMap result{
        {QStringLiteral("valid"), false},
        {QStringLiteral("state"), QStringLiteral("invalid")},
        {QStringLiteral("reason"), QStringLiteral("invalid_no_fly_zones_type")},
        {QStringLiteral("zoneCount"), QJsonValue(QJsonValue::Null).toVariant()},
        {QStringLiteral("fingerprint"), QStringLiteral("")}
    };
    if (input.type() != QVariant::List || input.isNull()) return result;
    const int count = input.toList().size();
    if (count > 1024) {
        result.insert(QStringLiteral("reason"), QStringLiteral("no_fly_zone_collection_budget_exceeded"));
        return result;
    }
    int nodes = 0;
    qint64 bytes = 0;
    if (!archivedTree(input, 0, &nodes, &bytes)) {
        result.insert(QStringLiteral("reason"), QStringLiteral("no_fly_zone_input_budget_or_value_invalid"));
        return result;
    }
    const QJsonDocument document = QJsonDocument::fromVariant(input);
    const QByteArray encoded = document.toJson(QJsonDocument::Compact);
    if (!document.isArray() || encoded.isEmpty() || encoded.size() > ArchivedSpatialBytes) {
        result.insert(QStringLiteral("reason"), QStringLiteral("no_fly_zone_input_budget_or_value_invalid"));
        return result;
    }
    result.insert(QStringLiteral("valid"), true);
    result.insert(QStringLiteral("state"), QStringLiteral("list"));
    result.insert(QStringLiteral("reason"), QStringLiteral(""));
    result.insert(QStringLiteral("zoneCount"), count);
    result.insert(QStringLiteral("fingerprint"), QString::fromLatin1(
        QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex()));
    return result;
}

QVariantMap FleetAirspaceConflictChecker::archivedMissionSpatialEvidence(
    const QVariantMap& plan, const QVariantMap& reviewEvidence)
{
    const QVariant unknown = QJsonValue(QJsonValue::Null).toVariant();
    QStringList limitations{
        QStringLiteral("historical-archived-evidence-not-a-new-airspace-check"),
        QStringLiteral("archived-planning-routes-only-not-startup-or-end-action-clearance"),
        QStringLiteral("zone-list-presence-does-not-certify-coverage-freshness-or-geometry"),
        QStringLiteral("archived-report-does-not-certify-timing-terrain-obstacles-or-clearance"),
        QStringLiteral("archived-applied-threshold-is-not-proof-of-explicit-policy")
    };
    QVariantMap result{
        {QStringLiteral("available"), false},
        {QStringLiteral("inputState"), QStringLiteral("legacy-unknown")},
        {QStringLiteral("noFlyZones"), QVariantList()},
        {QStringLiteral("zoneCount"), unknown},
        {QStringLiteral("minimumSeparationMeters"), unknown},
        {QStringLiteral("thresholdSource"), QStringLiteral("unavailable")},
        {QStringLiteral("report"), QVariantMap()},
        {QStringLiteral("scope"), QStringLiteral("archived-planning-routes-only")},
        {QStringLiteral("limitations"), limitations},
        {QStringLiteral("coverageCertified"), false},
        {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("timingCertified"), false}
    };
    const QVariantMap unavailable = result;
    const auto reject = [&unavailable, &limitations](const QString& reason) {
        // Whole-group rejection: never expose a valid report or zone prefix
        // from a record whose evidence shape/provenance cannot be preserved.
        QVariantMap rejected = unavailable;
        QStringList explanations = limitations;
        explanations.append(reason);
        rejected.insert(QStringLiteral("inputState"), QStringLiteral("recorded-invalid"));
        rejected.insert(QStringLiteral("limitations"), explanations);
        return QJsonDocument::fromVariant(rejected).toVariant().toMap();
    };
    if (!boundedArchivedEvidence(plan, reviewEvidence))
        return reject(QStringLiteral("archived-evidence-json-or-resource-budget-invalid"));

    const bool zonesPresent = reviewEvidence.contains(QStringLiteral("noFlyZones"));
    const QVariant zonesValue = reviewEvidence.value(QStringLiteral("noFlyZones"));
    const bool zonesList = zonesValue.type() == QVariant::List && !zonesValue.isNull();
    const QVariantList zones = zonesList ? zonesValue.toList() : QVariantList();
    QString state;
    QVariant count = unknown;
    if (reviewEvidence.contains(QStringLiteral("airspaceInputEvidence"))) {
        const QVariant metadataValue = reviewEvidence.value(QStringLiteral("airspaceInputEvidence"));
        if (metadataValue.type() != QVariant::Map || metadataValue.isNull())
            return reject(QStringLiteral("archived-airspace-input-metadata-invalid"));
        const QVariantMap metadata = metadataValue.toMap();
        const QStringList keys{QStringLiteral("schemaVersion"), QStringLiteral("noFlyZonesProvided"),
            QStringLiteral("noFlyZonesListValid"), QStringLiteral("noFlyZonesCount")};
        if (metadata.size() != keys.size())
            return reject(QStringLiteral("archived-airspace-input-metadata-invalid"));
        for (const QString& key : keys)
            if (!metadata.contains(key)) return reject(QStringLiteral("archived-airspace-input-metadata-invalid"));
        double schema = 0.0;
        const QVariant providedValue = metadata.value(QStringLiteral("noFlyZonesProvided"));
        const QVariant validValue = metadata.value(QStringLiteral("noFlyZonesListValid"));
        if (metadata.value(QStringLiteral("schemaVersion")).isNull()
            || !finiteNumber(metadata.value(QStringLiteral("schemaVersion")), &schema) || schema != 1.0
            || providedValue.type() != QVariant::Bool || providedValue.isNull()
            || validValue.type() != QVariant::Bool || validValue.isNull())
            return reject(QStringLiteral("archived-airspace-input-metadata-invalid"));
        const bool provided = providedValue.toBool();
        const bool listValid = validValue.toBool();
        const QVariant countValue = metadata.value(QStringLiteral("noFlyZonesCount"));
        if (listValid) {
            double recordedCount = -1.0;
            if (!provided || !zonesList || countValue.isNull()
                || !finiteNumber(countValue, &recordedCount) || recordedCount != zones.size())
                return reject(QStringLiteral("archived-airspace-input-metadata-inconsistent"));
            state = QStringLiteral("recorded-list");
            count = zones.size();
        } else {
            if (!archivedNull(countValue))
                return reject(QStringLiteral("archived-airspace-input-metadata-inconsistent"));
            if (!provided && zonesPresent && (!zonesList || !zones.isEmpty()))
                return reject(QStringLiteral("archived-airspace-input-metadata-inconsistent"));
            state = provided ? QStringLiteral("recorded-invalid") : QStringLiteral("recorded-omitted");
            // An invalid original input may have been folded to [] by the old
            // archiving expression. Never reinterpret that as a loaded list.
        }
    } else if (zonesList && !zones.isEmpty()) {
        state = QStringLiteral("recorded-list");
        count = zones.size();
        limitations.append(QStringLiteral("legacy-list-has-no-original-input-presence-metadata"));
    } else if (zonesPresent && !zonesList) {
        state = QStringLiteral("recorded-invalid");
    } else {
        state = QStringLiteral("legacy-unknown");
        limitations.append(QStringLiteral("legacy-empty-or-missing-list-does-not-prove-zones-were-loaded"));
    }

    const QVariant reportValue = reviewEvidence.value(QStringLiteral("airspaceReport"));
    const bool reportAvailable = reportValue.type() == QVariant::Map && !reportValue.isNull()
        && !reportValue.toMap().isEmpty();
    const QVariantMap report = reportAvailable ? reportValue.toMap() : QVariantMap();
    if (!reportAvailable) limitations.append(QStringLiteral("archived-airspace-report-missing-or-invalid"));
    double minimum = 0.0;
    const bool minimumKnown = reportAvailable && !report.value(QStringLiteral("minimumSeparationMeters")).isNull()
        && finiteNumber(report.value(QStringLiteral("minimumSeparationMeters")), &minimum) && minimum > 0.0;
    if (minimumKnown) {
        result.insert(QStringLiteral("minimumSeparationMeters"), minimum);
        result.insert(QStringLiteral("thresholdSource"), QStringLiteral("archived-applied-value-not-explicit-policy"));
    }
    const QVariant optionsValue = reviewEvidence.value(QStringLiteral("reviewOptions"));
    if (optionsValue.type() == QVariant::Map && !optionsValue.isNull()) {
        limitations.append(QStringLiteral("archived-review-options-not-replayed-or-used-to-replace-applied-threshold"));
        const QVariantMap options = optionsValue.toMap();
        double optionMinimum = 0.0;
        if (options.contains(QStringLiteral("minimumSeparationMeters"))) {
            if (options.value(QStringLiteral("minimumSeparationMeters")).isNull()
                || !finiteNumber(options.value(QStringLiteral("minimumSeparationMeters")), &optionMinimum)
                || optionMinimum <= 0.0)
                limitations.append(QStringLiteral("archived-review-options-minimum-invalid"));
            else if (minimumKnown && minimum != optionMinimum)
                limitations.append(QStringLiteral("archived-applied-threshold-differs-from-review-options"));
        }
    } else {
        limitations.append(QStringLiteral("archived-review-options-missing-or-invalid-not-reconstructed"));
    }
    const QVariant constraintsValue = plan.value(QStringLiteral("constraints"));
    double planMinimum = 0.0;
    if (minimumKnown && constraintsValue.type() == QVariant::Map && !constraintsValue.isNull()) {
        const QVariant policy = constraintsValue.toMap().value(QStringLiteral("minimumSeparationMeters"));
        if (!policy.isNull() && finiteNumber(policy, &planMinimum) && planMinimum > 0.0 && planMinimum != minimum)
            limitations.append(QStringLiteral("archived-applied-threshold-differs-from-plan-constraint"));
    }
    result.insert(QStringLiteral("available"), reportAvailable);
    result.insert(QStringLiteral("inputState"), state);
    result.insert(QStringLiteral("noFlyZones"), zones);
    result.insert(QStringLiteral("zoneCount"), count);
    result.insert(QStringLiteral("report"), report);
    result.insert(QStringLiteral("limitations"), limitations);
    // Only JSON representation is normalized. Do not repair or rerun the
    // original report, interpret zone geometry, or upgrade its safety claims.
    int outputNodes = 0;
    qint64 outputBytes = 0;
    if (!archivedTree(result, 0, &outputNodes, &outputBytes))
        return reject(QStringLiteral("archived-evidence-display-budget-exceeded"));
    const QJsonDocument document = QJsonDocument::fromVariant(result);
    if (document.toJson(QJsonDocument::Compact).size() > ArchivedSpatialBytes)
        return reject(QStringLiteral("archived-evidence-display-budget-exceeded"));
    return document.toVariant().toMap();
}

QVariantMap FleetAirspaceConflictChecker::checkNominalStartupSegments(
    const QVariantList& segments, const QVariant& minimumSeparationMeters)
{
    const QVariant unknown = QJsonValue(QJsonValue::Null).toVariant();
    QVariantMap projection{
        {QStringLiteral("model"), QStringLiteral("local-equirectangular-approximation")},
        {QStringLiteral("referenceLatitude"), unknown},
        {QStringLiteral("referenceLongitude"), unknown},
        {QStringLiteral("latitudeSpanDegrees"), unknown},
        {QStringLiteral("longitudeSpanDegrees"), unknown},
        {QStringLiteral("maxAbsoluteLatitudeDegrees"), unknown},
        {QStringLiteral("withinSupportedDomain"), false}
    };
    QVariantMap result{
        {QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("comparisonComplete"), false},
        {QStringLiteral("candidateCount"), 0}, {QStringLiteral("pairCount"), 0},
        {QStringLiteral("candidates"), QVariantList()},
        {QStringLiteral("candidatesTruncated"), false},
        {QStringLiteral("projection"), projection},
        {QStringLiteral("minimumSeparationMeters"), unknown},
        {QStringLiteral("timingKnown"), false},
        {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("executable"), false}
    };
    const auto reject = [&result](const QString& error) {
        // No partially checked pair or candidate prefix is published as a
        // complete result. Projection metadata may explain an unsupported area.
        result.insert(QStringLiteral("error"), error);
        return result;
    };
    const auto number = [](const QVariant& value, double* output) {
        return value.isValid() && !value.isNull() && finiteNumber(value, output);
    };
    const auto exactMap = [](const QVariant& value, const QStringList& keys) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap map = value.toMap();
        if (map.size() != keys.size()) return false;
        for (const QString& key : keys) if (!map.contains(key)) return false;
        return true;
    };
    const auto text = [](const QVariant& value) {
        if (value.type() != QVariant::String || value.isNull()) return false;
        const QString string = value.toString();
        if (string.isEmpty() || string.size() > 128 || string.trimmed() != string) return false;
        for (const QChar character : string) {
            if (character.isNull() || character.category() == QChar::Other_Control) return false;
        }
        return true;
    };
    const auto flag = [](const QVariantMap& map, const QString& key, bool expected) {
        const QVariant value = map.value(key);
        return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
    };

    double minimum = 0.0;
    if (!number(minimumSeparationMeters, &minimum) || minimum <= 0.0)
        return reject(QStringLiteral("minimum-separation-missing-or-invalid"));
    result.insert(QStringLiteral("minimumSeparationMeters"), minimum);
    if (segments.isEmpty()) return reject(QStringLiteral("nominal-segments-missing"));
    if (segments.size() > 96) return reject(QStringLiteral("nominal-segment-budget-exceeded"));

    struct Segment {
        QString id;
        int vehicleId = 0;
        RawPoint from;
        RawPoint to;
        Point localFrom;
        Point localTo;
    };
    const QStringList segmentKeys{
        QStringLiteral("segmentId"), QStringLiteral("vehicleId"), QStringLiteral("kind"),
        QStringLiteral("from"), QStringLiteral("to"), QStringLiteral("nominalOnly"),
        QStringLiteral("actualPathCertified"), QStringLiteral("timingKnown")
    };
    const QStringList pointKeys{
        QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")
    };
    const auto point = [&exactMap, &number, &pointKeys](const QVariant& value, RawPoint* output) {
        if (!exactMap(value, pointKeys)) return false;
        const QVariantMap map = value.toMap();
        double latitude = 0.0, longitude = 0.0, altitude = 0.0;
        if (!number(map.value(QStringLiteral("latitude")), &latitude)
            || !number(map.value(QStringLiteral("longitude")), &longitude)
            || !number(map.value(QStringLiteral("altitude")), &altitude)
            || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
            return false;
        output->valid = output->geodetic = output->altitudeKnown = true;
        output->first = latitude; output->second = longitude; output->altitude = altitude;
        return true;
    };

    QList<Segment> validated;
    QSet<QString> seenSegments;
    QSet<int> seenVehicles;
    double minLatitude = 90.0, maxLatitude = -90.0;
    double minLongitude = 180.0, maxLongitude = -180.0;
    for (const QVariant& value : segments) {
        if (!exactMap(value, segmentKeys)) return reject(QStringLiteral("nominal-segment-shape-invalid"));
        const QVariantMap map = value.toMap();
        double vehicleId = 0.0;
        if (!text(map.value(QStringLiteral("segmentId"))) || !text(map.value(QStringLiteral("kind")))
            || !number(map.value(QStringLiteral("vehicleId")), &vehicleId)
            || vehicleId < 1.0 || vehicleId > 255.0 || qFloor(vehicleId) != vehicleId
            || !flag(map, QStringLiteral("nominalOnly"), true)
            || !flag(map, QStringLiteral("actualPathCertified"), false)
            || !flag(map, QStringLiteral("timingKnown"), false))
            return reject(QStringLiteral("nominal-segment-fields-invalid"));
        Segment segment;
        segment.id = map.value(QStringLiteral("segmentId")).toString();
        segment.vehicleId = static_cast<int>(vehicleId);
        if (seenSegments.contains(segment.id)) return reject(QStringLiteral("nominal-segment-id-duplicate"));
        if (!point(map.value(QStringLiteral("from")), &segment.from)
            || !point(map.value(QStringLiteral("to")), &segment.to))
            return reject(QStringLiteral("nominal-segment-coordinate-invalid"));
        seenSegments.insert(segment.id);
        seenVehicles.insert(segment.vehicleId);
        if (seenVehicles.size() > 32) return reject(QStringLiteral("nominal-vehicle-budget-exceeded"));
        for (const RawPoint& endpoint : {segment.from, segment.to}) {
            minLatitude = qMin(minLatitude, endpoint.first);
            maxLatitude = qMax(maxLatitude, endpoint.first);
            minLongitude = qMin(minLongitude, endpoint.second);
            maxLongitude = qMax(maxLongitude, endpoint.second);
        }
        validated.append(segment);
    }

    const double referenceLatitude = validated.first().from.first;
    const double referenceLongitude = validated.first().from.second;
    const double referenceAltitude = validated.first().from.altitude;
    const double latitudeSpan = maxLatitude - minLatitude;
    const double longitudeSpan = maxLongitude - minLongitude;
    const double maxAbsoluteLatitude = qMax(qAbs(minLatitude), qAbs(maxLatitude));
    const bool supported = maxAbsoluteLatitude <= 80.0 && latitudeSpan <= 0.2 && longitudeSpan <= 0.2;
    projection.insert(QStringLiteral("referenceLatitude"), referenceLatitude);
    projection.insert(QStringLiteral("referenceLongitude"), referenceLongitude);
    projection.insert(QStringLiteral("latitudeSpanDegrees"), latitudeSpan);
    projection.insert(QStringLiteral("longitudeSpanDegrees"), longitudeSpan);
    projection.insert(QStringLiteral("maxAbsoluteLatitudeDegrees"), maxAbsoluteLatitude);
    projection.insert(QStringLiteral("withinSupportedDomain"), supported);
    result.insert(QStringLiteral("projection"), projection);
    // This is a bounded local approximation, not a globe / antimeridian
    // transform. Never unwrap distant coordinates or compare a valid prefix.
    if (!supported) return reject(QStringLiteral("local-projection-domain-unsupported"));

    const auto numericalPoint = [](const Point& value) {
        // The reused segment minimizer has fourth-degree products. A common
        // altitude translation avoids a huge datum offset; bounding remaining
        // components prevents finite inputs overflowing intermediate products.
        return qIsFinite(value.x) && qIsFinite(value.y) && qIsFinite(value.z)
            && qAbs(value.x) <= 1.0e50 && qAbs(value.y) <= 1.0e50 && qAbs(value.z) <= 1.0e50;
    };
    for (Segment& segment : validated) {
        segment.localFrom = toLocal(segment.from, nullptr, true, referenceLatitude, referenceLongitude);
        segment.localTo = toLocal(segment.to, nullptr, true, referenceLatitude, referenceLongitude);
        segment.localFrom.z -= referenceAltitude;
        segment.localTo.z -= referenceAltitude;
        if (!numericalPoint(segment.localFrom) || !numericalPoint(segment.localTo))
            return reject(QStringLiteral("numerical-domain-unsupported"));
    }

    QVariantList candidates;
    int candidateCount = 0, pairCount = 0;
    for (int first = 0; first < validated.size(); ++first) {
        const Segment& left = validated.at(first);
        for (int second = first + 1; second < validated.size(); ++second) {
            const Segment& right = validated.at(second);
            // Alternative diagnostic branches of one aircraft are not a
            // multi-aircraft conflict and are never joined into one route.
            if (left.vehicleId == right.vehicleId) continue;
            ++pairCount;
            const double separation = minSpatialDistance(left.localFrom, left.localTo,
                                                         right.localFrom, right.localTo);
            if (!qIsFinite(separation) || separation < 0.0)
                return reject(QStringLiteral("nominal-distance-computation-invalid"));
            if (separation >= minimum) continue;
            ++candidateCount;
            if (candidates.size() < 512) {
                candidates.append(QVariantMap{
                    {QStringLiteral("vehicleA"), left.vehicleId}, {QStringLiteral("segmentA"), left.id},
                    {QStringLiteral("vehicleB"), right.vehicleId}, {QStringLiteral("segmentB"), right.id},
                    {QStringLiteral("distanceMeters"), separation},
                    {QStringLiteral("kind"), QStringLiteral("nominal-spatial-proximity")}
                });
            }
        }
    }
    // With <=96 segments on <=32 vehicles there are at most 4464 cross-vehicle
    // pairs. Even after the display budget is reached, every pair is counted.
    const bool truncated = candidateCount > candidates.size();
    result.insert(QStringLiteral("ok"), !truncated);
    result.insert(QStringLiteral("error"), truncated
        ? QStringLiteral("candidate-details-truncated") : QStringLiteral(""));
    result.insert(QStringLiteral("comparisonComplete"), true);
    result.insert(QStringLiteral("candidateCount"), candidateCount);
    result.insert(QStringLiteral("pairCount"), pairCount);
    result.insert(QStringLiteral("candidates"), candidates);
    result.insert(QStringLiteral("candidatesTruncated"), truncated);
    return result;
}

QVariantMap FleetAirspaceConflictChecker::checkJointSpatialSegments(
    const QVariant& segmentsInput, const QVariant& noFlyZonesInput,
    const QVariant& minimumSeparationMeters)
{
    const QVariant unknown = QJsonValue(QJsonValue::Null).toVariant();
    const QVariantMap zoneInputCheck = inspectNoFlyZoneInput(noFlyZonesInput);
    QVariantMap projection{
        {QStringLiteral("model"), QStringLiteral("local-equirectangular-approximation")},
        {QStringLiteral("referenceLatitude"), unknown},
        {QStringLiteral("referenceLongitude"), unknown},
        {QStringLiteral("latitudeSpanDegrees"), unknown},
        {QStringLiteral("longitudeSpanDegrees"), unknown},
        {QStringLiteral("maxAbsoluteLatitudeDegrees"), unknown},
        {QStringLiteral("withinSupportedDomain"), false}
    };
    QVariantMap result{
        {QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("comparisonComplete"), false},
        {QStringLiteral("segmentCount"), unknown}, {QStringLiteral("vehicleCount"), unknown},
        {QStringLiteral("zoneCount"), unknown}, {QStringLiteral("zoneInputCheck"), zoneInputCheck},
        {QStringLiteral("pairCount"), 0}, {QStringLiteral("candidateCount"), 0},
        {QStringLiteral("candidates"), QVariantList()}, {QStringLiteral("candidatesTruncated"), false},
        {QStringLiteral("zoneComparisonCount"), 0}, {QStringLiteral("zoneHitCount"), 0},
        {QStringLiteral("zoneHits"), QVariantList()}, {QStringLiteral("zoneHitsTruncated"), false},
        {QStringLiteral("projection"), projection},
        {QStringLiteral("minimumSeparationMeters"), unknown},
        {QStringLiteral("timingKnown"), false}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}
    };
    const auto reject = [&result](const QString& error) {
        // All calculated pairs and hits stay local until the entire supported
        // input has been compared. Failure cannot publish a successful prefix.
        result.insert(QStringLiteral("error"), error);
        return result;
    };
    const auto number = [](const QVariant& value, double* output) {
        return value.isValid() && !value.isNull() && finiteNumber(value, output);
    };
    const auto exactMap = [](const QVariant& value, const QStringList& keys) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap map = value.toMap();
        if (map.size() != keys.size()) return false;
        for (const QString& key : keys) if (!map.contains(key)) return false;
        return true;
    };
    const auto text = [](const QVariant& value) {
        if (value.type() != QVariant::String || value.isNull()) return false;
        const QString string = value.toString();
        if (string.isEmpty() || string.size() > 128 || string.trimmed() != string) return false;
        for (const QChar character : string)
            if (character.isNull() || character.category() == QChar::Other_Control) return false;
        return true;
    };
    const auto constant = [](const QVariant& value, const QString& expected) {
        return value.type() == QVariant::String && !value.isNull() && value.toString() == expected;
    };
    const auto flag = [](const QVariantMap& map, const QString& key, bool expected) {
        const QVariant value = map.value(key);
        return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
    };
    if (!zoneInputCheck.value(QStringLiteral("valid")).toBool())
        return reject(zoneInputCheck.value(QStringLiteral("reason")).toString());
    if (segmentsInput.type() != QVariant::List || segmentsInput.isNull())
        return reject(QStringLiteral("joint-segments-type-invalid"));
    const QVariantList segmentValues = segmentsInput.toList();
    if (segmentValues.isEmpty()) return reject(QStringLiteral("joint-segments-missing"));
    if (segmentValues.size() > 8320) return reject(QStringLiteral("joint-segment-budget-exceeded"));
    const QVariantMap inputs{{QStringLiteral("segments"), segmentsInput},
                            {QStringLiteral("zones"), noFlyZonesInput},
                            {QStringLiteral("minimum"), minimumSeparationMeters}};
    int nodes = 0;
    qint64 bytes = 0;
    if (!archivedTree(inputs, 0, &nodes, &bytes))
        return reject(QStringLiteral("joint-input-budget-or-value-invalid"));
    const QByteArray encoded = QJsonDocument::fromVariant(inputs).toJson(QJsonDocument::Compact);
    if (encoded.isEmpty() || encoded.size() > ArchivedSpatialBytes)
        return reject(QStringLiteral("joint-input-budget-or-value-invalid"));
    double minimum = 0.0;
    if (!number(minimumSeparationMeters, &minimum) || minimum <= 0.0)
        return reject(QStringLiteral("minimum-separation-missing-or-invalid"));
    result.insert(QStringLiteral("minimumSeparationMeters"), minimum);

    struct JointSegment {
        QString id;
        QString kind;
        int vehicleId = 0;
        RawPoint from;
        RawPoint to;
        Point localFrom;
        Point localTo;
    };
    struct JointZone {
        QString id;
        QList<RawPoint> vertices;
        QList<Point> polygon;
        bool lowerKnown = false;
        bool upperKnown = false;
        double lower = 0.0;
        double upper = 0.0;
        bool timeWindowIgnored = false;
    };
    const QStringList segmentKeys{
        QStringLiteral("segmentId"), QStringLiteral("vehicleId"), QStringLiteral("kind"),
        QStringLiteral("from"), QStringLiteral("to"), QStringLiteral("nominalOnly"),
        QStringLiteral("actualPathCertified"), QStringLiteral("timingKnown")
    };
    const QStringList pointKeys{
        QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")
    };
    const QStringList kinds{
        QStringLiteral("mission-leg"), QStringLiteral("mission-anchor"),
        QStringLiteral("mission-return-leg"), QStringLiteral("mission-return-connection"),
        QStringLiteral("mission-launch-connection"), QStringLiteral("mission-terminal-hold"),
        QStringLiteral("mission-terminal-land"), QStringLiteral("vertical-takeoff"),
        QStringLiteral("takeoff-request-connection"), QStringLiteral("current-first-waypoint")
    };
    const auto point = [&exactMap, &number, &pointKeys](const QVariant& value, RawPoint* output) {
        if (!exactMap(value, pointKeys)) return false;
        const QVariantMap map = value.toMap();
        double latitude = 0.0, longitude = 0.0, altitude = 0.0;
        if (!number(map.value(QStringLiteral("latitude")), &latitude)
            || !number(map.value(QStringLiteral("longitude")), &longitude)
            || !number(map.value(QStringLiteral("altitude")), &altitude)
            || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
            return false;
        output->valid = output->geodetic = output->altitudeKnown = true;
        output->first = latitude; output->second = longitude; output->altitude = altitude;
        return true;
    };
    double minLatitude = 90.0, maxLatitude = -90.0;
    double minLongitude = 180.0, maxLongitude = -180.0;
    const auto includePoint = [&](const RawPoint& value) {
        minLatitude = qMin(minLatitude, value.first);
        maxLatitude = qMax(maxLatitude, value.first);
        minLongitude = qMin(minLongitude, value.second);
        maxLongitude = qMax(maxLongitude, value.second);
    };
    QList<JointSegment> segments;
    QSet<QString> segmentIds;
    QMap<int, int> vehicleSegmentCounts;
    for (const QVariant& value : segmentValues) {
        if (!exactMap(value, segmentKeys)) return reject(QStringLiteral("joint-segment-shape-invalid"));
        const QVariantMap map = value.toMap();
        double vehicleId = 0.0;
        if (!text(map.value(QStringLiteral("segmentId")))
            || !text(map.value(QStringLiteral("kind")))
            || !kinds.contains(map.value(QStringLiteral("kind")).toString())
            || !number(map.value(QStringLiteral("vehicleId")), &vehicleId)
            || vehicleId < 1.0 || vehicleId > 255.0 || qFloor(vehicleId) != vehicleId
            || !flag(map, QStringLiteral("nominalOnly"), true)
            || !flag(map, QStringLiteral("actualPathCertified"), false)
            || !flag(map, QStringLiteral("timingKnown"), false))
            return reject(QStringLiteral("joint-segment-fields-invalid"));
        JointSegment segment;
        segment.id = map.value(QStringLiteral("segmentId")).toString();
        segment.kind = map.value(QStringLiteral("kind")).toString();
        segment.vehicleId = static_cast<int>(vehicleId);
        if (segmentIds.contains(segment.id)) return reject(QStringLiteral("joint-segment-id-duplicate"));
        if (!point(map.value(QStringLiteral("from")), &segment.from)
            || !point(map.value(QStringLiteral("to")), &segment.to))
            return reject(QStringLiteral("joint-segment-coordinate-invalid"));
        segmentIds.insert(segment.id);
        vehicleSegmentCounts[segment.vehicleId] += 1;
        if (vehicleSegmentCounts.size() > 32) return reject(QStringLiteral("joint-vehicle-budget-exceeded"));
        includePoint(segment.from);
        includePoint(segment.to);
        segments.append(segment);
    }

    const QStringList geometryKeys{
        QStringLiteral("polygon"), QStringLiteral("vertices"), QStringLiteral("points"),
        QStringLiteral("boundary"), QStringLiteral("coordinates"), QStringLiteral("bounds")
    };
    const QStringList idKeys{QStringLiteral("id"), QStringLiteral("zoneId"), QStringLiteral("name")};
    const QStringList lowerKeys{QStringLiteral("minAltitude"), QStringLiteral("minAltitudeMeters"), QStringLiteral("floor")};
    const QStringList upperKeys{QStringLiteral("maxAltitude"), QStringLiteral("maxAltitudeMeters"), QStringLiteral("ceiling")};
    const QStringList startKeys{QStringLiteral("startTime"), QStringLiteral("activeFrom")};
    const QStringList endKeys{QStringLiteral("endTime"), QStringLiteral("activeUntil")};
    QStringList allowedKeys = geometryKeys + idKeys + lowerKeys + upperKeys + startKeys + endKeys;
    allowedKeys << QStringLiteral("coordinateSystem") << QStringLiteral("altitudeReference") << QStringLiteral("timeReference");
    const auto selectedKey = [](const QVariantMap& map, const QStringList& alternatives, QString* key) {
        key->clear();
        for (const QString& candidate : alternatives) {
            if (!map.contains(candidate)) continue;
            if (!key->isEmpty()) return false;
            *key = candidate;
        }
        return true;
    };
    const auto vertex = [&number, &constant](const QVariant& value, RawPoint* output) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap map = value.toMap();
        const bool explicitFrame = map.contains(QStringLiteral("coordinateSystem"));
        if (map.size() != (explicitFrame ? 3 : 2)
            || !map.contains(QStringLiteral("latitude")) || !map.contains(QStringLiteral("longitude"))
            || (explicitFrame && !constant(map.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic"))))
            return false;
        double latitude = 0.0, longitude = 0.0;
        if (!number(map.value(QStringLiteral("latitude")), &latitude)
            || !number(map.value(QStringLiteral("longitude")), &longitude)
            || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
            return false;
        output->valid = output->geodetic = true;
        output->first = latitude; output->second = longitude; output->altitude = 0.0;
        return true;
    };
    const auto sameVertex = [](const RawPoint& first, const RawPoint& second) {
        return first.first == second.first && first.second == second.second;
    };
    QList<JointZone> zones;
    QSet<QString> zoneIds;
    const QVariantList zoneValues = noFlyZonesInput.toList();
    for (int zoneIndex = 0; zoneIndex < zoneValues.size(); ++zoneIndex) {
        const QVariant& value = zoneValues.at(zoneIndex);
        if (value.type() != QVariant::Map || value.isNull())
            return reject(QStringLiteral("joint-zone-shape-unsupported"));
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            if (!allowedKeys.contains(it.key())) return reject(QStringLiteral("joint-zone-field-unsupported"));
        QString geometryKey, idKey, lowerKey, upperKey, startKey, endKey;
        if (!selectedKey(map, geometryKeys, &geometryKey) || geometryKey.isEmpty()
            || !selectedKey(map, idKeys, &idKey) || !selectedKey(map, lowerKeys, &lowerKey)
            || !selectedKey(map, upperKeys, &upperKey) || !selectedKey(map, startKeys, &startKey)
            || !selectedKey(map, endKeys, &endKey))
            return reject(QStringLiteral("joint-zone-alias-or-geometry-invalid"));
        if (map.contains(QStringLiteral("coordinateSystem"))
            && !constant(map.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic")))
            return reject(QStringLiteral("joint-zone-coordinate-system-unsupported"));
        JointZone zone;
        if (!idKey.isEmpty() && !text(map.value(idKey))) return reject(QStringLiteral("joint-zone-id-invalid"));
        zone.id = idKey.isEmpty() ? QStringLiteral("zone-%1").arg(zoneIndex + 1) : map.value(idKey).toString();
        if (zoneIds.contains(zone.id)) return reject(QStringLiteral("joint-zone-id-duplicate"));
        zoneIds.insert(zone.id);
        zone.lowerKnown = !lowerKey.isEmpty();
        zone.upperKnown = !upperKey.isEmpty();
        const bool altitudeDeclared = map.contains(QStringLiteral("altitudeReference"));
        if (((zone.lowerKnown || zone.upperKnown) && !altitudeDeclared)
            || (altitudeDeclared && !constant(map.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl")))
            || (zone.lowerKnown && !number(map.value(lowerKey), &zone.lower))
            || (zone.upperKnown && !number(map.value(upperKey), &zone.upper))
            || (zone.lowerKnown && zone.upperKnown && zone.lower > zone.upper))
            return reject(QStringLiteral("joint-zone-altitude-invalid-or-unsupported"));
        double startTime = 0.0, endTime = 0.0;
        if ((!startKey.isEmpty() && !number(map.value(startKey), &startTime))
            || (!endKey.isEmpty() && !number(map.value(endKey), &endTime))
            || (!startKey.isEmpty() && !endKey.isEmpty() && startTime > endTime)
            || (map.contains(QStringLiteral("timeReference")) && !text(map.value(QStringLiteral("timeReference")))))
            return reject(QStringLiteral("joint-zone-time-window-invalid"));
        zone.timeWindowIgnored = !startKey.isEmpty() || !endKey.isEmpty();
        if (geometryKey == QStringLiteral("bounds")) {
            const QStringList boundsKeys{QStringLiteral("west"), QStringLiteral("east"),
                                         QStringLiteral("south"), QStringLiteral("north")};
            const QVariant boundsValue = map.value(geometryKey);
            if (!exactMap(boundsValue, boundsKeys)) return reject(QStringLiteral("joint-zone-bounds-invalid"));
            const QVariantMap bounds = boundsValue.toMap();
            double west = 0.0, east = 0.0, south = 0.0, north = 0.0;
            if (!number(bounds.value(QStringLiteral("west")), &west)
                || !number(bounds.value(QStringLiteral("east")), &east)
                || !number(bounds.value(QStringLiteral("south")), &south)
                || !number(bounds.value(QStringLiteral("north")), &north)
                || west < -180.0 || east > 180.0 || west >= east
                || south < -90.0 || north > 90.0 || south >= north)
                return reject(QStringLiteral("joint-zone-bounds-invalid"));
            for (const QVariantMap& coordinates : {
                     QVariantMap{{QStringLiteral("latitude"), south}, {QStringLiteral("longitude"), west}},
                     QVariantMap{{QStringLiteral("latitude"), south}, {QStringLiteral("longitude"), east}},
                     QVariantMap{{QStringLiteral("latitude"), north}, {QStringLiteral("longitude"), east}},
                     QVariantMap{{QStringLiteral("latitude"), north}, {QStringLiteral("longitude"), west}}}) {
                RawPoint raw;
                if (!vertex(coordinates, &raw)) return reject(QStringLiteral("joint-zone-bounds-invalid"));
                zone.vertices.append(raw);
            }
        } else {
            const QVariant verticesValue = map.value(geometryKey);
            if (verticesValue.type() != QVariant::List || verticesValue.isNull())
                return reject(QStringLiteral("joint-zone-polygon-unsupported"));
            for (const QVariant& coordinates : verticesValue.toList()) {
                RawPoint raw;
                if (!vertex(coordinates, &raw)) return reject(QStringLiteral("joint-zone-vertex-unsupported"));
                zone.vertices.append(raw);
            }
            // One conventional closing vertex is permitted, not arbitrary
            // deduplication or repairs of a malformed/self-touching polygon.
            if (zone.vertices.size() >= 2 && sameVertex(zone.vertices.first(), zone.vertices.last()))
                zone.vertices.removeLast();
        }
        if (zone.vertices.size() < 3) return reject(QStringLiteral("joint-zone-polygon-degenerate"));
        for (const RawPoint& raw : zone.vertices) includePoint(raw);
        zones.append(zone);
    }
    result.insert(QStringLiteral("segmentCount"), segments.size());
    result.insert(QStringLiteral("vehicleCount"), vehicleSegmentCounts.size());
    result.insert(QStringLiteral("zoneCount"), zones.size());

    // Preflight the complete workload before quadratic polygon validation or
    // comparisons. Same-vehicle segment pairs are not iterated at all below.
    constexpr qint64 maximumWork = 1000000;
    qint64 work = 0;
    const auto reserveWork = [&work](qint64 amount) {
        if (amount < 0 || amount > maximumWork - work) return false;
        work += amount;
        return true;
    };
    qint64 precedingSegments = 0;
    for (auto it = vehicleSegmentCounts.constBegin(); it != vehicleSegmentCounts.constEnd(); ++it) {
        if (!reserveWork(precedingSegments * it.value())) return reject(QStringLiteral("joint-comparison-budget-exceeded"));
        precedingSegments += it.value();
    }
    for (const JointZone& zone : zones) {
        const qint64 vertices = zone.vertices.size();
        // Two endpoint-in-polygon traversals and one edge traversal per pair;
        // count all possible edges even where an early intersection is found.
        if (!reserveWork(vertices * (vertices - 1) / 2 + 3 * vertices)
            || !reserveWork(static_cast<qint64>(segments.size()) * (3 * vertices + 1)))
            return reject(QStringLiteral("joint-comparison-budget-exceeded"));
    }

    const double referenceLatitude = segments.first().from.first;
    const double referenceLongitude = segments.first().from.second;
    const double referenceAltitude = segments.first().from.altitude;
    const double latitudeSpan = maxLatitude - minLatitude;
    const double longitudeSpan = maxLongitude - minLongitude;
    const double maxAbsoluteLatitude = qMax(qAbs(minLatitude), qAbs(maxLatitude));
    const bool supported = maxAbsoluteLatitude <= 80.0 && latitudeSpan <= 0.2 && longitudeSpan <= 0.2;
    projection.insert(QStringLiteral("referenceLatitude"), referenceLatitude);
    projection.insert(QStringLiteral("referenceLongitude"), referenceLongitude);
    projection.insert(QStringLiteral("latitudeSpanDegrees"), latitudeSpan);
    projection.insert(QStringLiteral("longitudeSpanDegrees"), longitudeSpan);
    projection.insert(QStringLiteral("maxAbsoluteLatitudeDegrees"), maxAbsoluteLatitude);
    projection.insert(QStringLiteral("withinSupportedDomain"), supported);
    result.insert(QStringLiteral("projection"), projection);
    if (!supported) return reject(QStringLiteral("local-projection-domain-unsupported"));
    const auto numericalPoint = [](const Point& value) {
        return qIsFinite(value.x) && qIsFinite(value.y) && qIsFinite(value.z)
            && qAbs(value.x) <= 1.0e50 && qAbs(value.y) <= 1.0e50 && qAbs(value.z) <= 1.0e50;
    };
    QMap<int, QList<int>> vehicleSegments;
    for (int index = 0; index < segments.size(); ++index) {
        JointSegment& segment = segments[index];
        segment.localFrom = toLocal(segment.from, nullptr, true, referenceLatitude, referenceLongitude);
        segment.localTo = toLocal(segment.to, nullptr, true, referenceLatitude, referenceLongitude);
        segment.localFrom.z -= referenceAltitude;
        segment.localTo.z -= referenceAltitude;
        if (!numericalPoint(segment.localFrom) || !numericalPoint(segment.localTo))
            return reject(QStringLiteral("numerical-domain-unsupported"));
        vehicleSegments[segment.vehicleId].append(index);
    }
    for (JointZone& zone : zones) {
        for (const RawPoint& raw : zone.vertices) {
            Point local = toLocal(raw, nullptr, true, referenceLatitude, referenceLongitude);
            local.z = 0.0;
            if (!numericalPoint(local)) return reject(QStringLiteral("numerical-domain-unsupported"));
            zone.polygon.append(local);
        }
        double twiceArea = 0.0;
        for (int index = 0; index < zone.polygon.size(); ++index) {
            const Point& first = zone.polygon.at(index);
            const Point& second = zone.polygon.at((index + 1) % zone.polygon.size());
            const Point& third = zone.polygon.at((index + 2) % zone.polygon.size());
            if (first.x == second.x && first.y == second.y)
                return reject(QStringLiteral("joint-zone-polygon-degenerate"));
            // Reject adjacent collinear backtracking/overlap as well as all
            // nonadjacent crossings or touches; do not guess intended holes.
            if (qAbs(cross2D(first, second, third)) <= 1.0e-9
                && (second.x - first.x) * (third.x - second.x)
                    + (second.y - first.y) * (third.y - second.y) < 0.0)
                return reject(QStringLiteral("joint-zone-polygon-self-intersecting"));
            twiceArea += first.x * second.y - second.x * first.y;
            for (int other = index + 1; other < zone.polygon.size(); ++other) {
                if (other == index + 1 || (index == 0 && other == zone.polygon.size() - 1)) continue;
                if (segmentsIntersect2D(first, second, zone.polygon.at(other),
                                        zone.polygon.at((other + 1) % zone.polygon.size())))
                    return reject(QStringLiteral("joint-zone-polygon-self-intersecting"));
            }
        }
        if (!qIsFinite(twiceArea)) return reject(QStringLiteral("numerical-domain-unsupported"));
        if (qAbs(twiceArea) <= 1.0e-9) return reject(QStringLiteral("joint-zone-polygon-degenerate"));
    }

    QVariantList candidates;
    int pairCount = 0, candidateCount = 0;
    for (int first = 0; first < segments.size(); ++first) {
        const JointSegment& left = segments.at(first);
        // Grouped iteration avoids O(n^2) no-op work for one large vehicle.
        // Each cross-vehicle pair is visited once, in source-first order.
        for (auto it = vehicleSegments.constBegin(); it != vehicleSegments.constEnd(); ++it) {
            if (it.key() == left.vehicleId) continue;
            for (int second : it.value()) {
                if (second <= first) continue;
                const JointSegment& right = segments.at(second);
                ++pairCount;
                const double separation = minSpatialDistance(left.localFrom, left.localTo,
                                                             right.localFrom, right.localTo);
                if (!qIsFinite(separation) || separation < 0.0)
                    return reject(QStringLiteral("nominal-distance-computation-invalid"));
                if (separation >= minimum) continue;
                ++candidateCount;
                if (candidates.size() < 512) candidates.append(QVariantMap{
                    {QStringLiteral("vehicleA"), left.vehicleId}, {QStringLiteral("segmentA"), left.id},
                    {QStringLiteral("kindA"), left.kind}, {QStringLiteral("vehicleB"), right.vehicleId},
                    {QStringLiteral("segmentB"), right.id}, {QStringLiteral("kindB"), right.kind},
                    {QStringLiteral("distanceMeters"), separation},
                    {QStringLiteral("kind"), QStringLiteral("nominal-spatial-proximity")}
                });
            }
        }
    }
    const auto clipToAltitude = [](const JointSegment& segment, const JointZone& zone,
                                   Point* clippedFrom, Point* clippedTo, bool* overlaps) {
        *overlaps = false;
        const double lower = qMin(segment.from.altitude, segment.to.altitude);
        const double upper = qMax(segment.from.altitude, segment.to.altitude);
        if ((zone.lowerKnown && upper < zone.lower) || (zone.upperKnown && lower > zone.upper)) return true;
        double first = 0.0, last = 1.0;
        const double delta = segment.to.altitude - segment.from.altitude;
        if (!qIsFinite(delta)) return false;
        if (delta != 0.0) {
            // Only subtract a boundary lying within the segment's finite
            // altitude span; remote one-sided bounds cannot overflow here.
            if (zone.lowerKnown && zone.lower > lower && zone.lower <= upper) {
                const double fraction = (zone.lower - segment.from.altitude) / delta;
                if (!qIsFinite(fraction)) return false;
                if (delta > 0.0) first = qMax(first, fraction); else last = qMin(last, fraction);
            }
            if (zone.upperKnown && zone.upper >= lower && zone.upper < upper) {
                const double fraction = (zone.upper - segment.from.altitude) / delta;
                if (!qIsFinite(fraction)) return false;
                if (delta > 0.0) last = qMin(last, fraction); else first = qMax(first, fraction);
            }
        }
        if (first > last) return true;
        *clippedFrom = interpolate(segment.localFrom, segment.localTo, qBound(0.0, first, 1.0));
        *clippedTo = interpolate(segment.localFrom, segment.localTo, qBound(0.0, last, 1.0));
        *overlaps = true;
        return true;
    };
    QVariantList zoneHits;
    int zoneComparisonCount = 0, zoneHitCount = 0;
    for (const JointSegment& segment : segments) {
        for (int zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex) {
            const JointZone& zone = zones.at(zoneIndex);
            ++zoneComparisonCount;
            Point clippedFrom, clippedTo;
            bool overlaps = false;
            if (!clipToAltitude(segment, zone, &clippedFrom, &clippedTo, &overlaps)
                || (overlaps && (!numericalPoint(clippedFrom) || !numericalPoint(clippedTo))))
                return reject(QStringLiteral("nominal-zone-computation-invalid"));
            // No use of time windows: a window cannot exclude a spatial
            // candidate when segment timing and clock alignment are unknown.
            if (!overlaps || !segmentIntersectsPolygon2D(clippedFrom, clippedTo, zone.polygon)) continue;
            ++zoneHitCount;
            if (zoneHits.size() < 512) zoneHits.append(QVariantMap{
                {QStringLiteral("zoneIndex"), zoneIndex}, {QStringLiteral("zoneId"), zone.id},
                {QStringLiteral("segmentId"), segment.id}, {QStringLiteral("vehicleId"), segment.vehicleId},
                {QStringLiteral("segmentKind"), segment.kind},
                {QStringLiteral("kind"), QStringLiteral("nominal-zone-intersection")},
                {QStringLiteral("timeWindowIgnored"), zone.timeWindowIgnored}
            });
        }
    }
    const bool candidatesTruncated = candidateCount > candidates.size();
    const bool hitsTruncated = zoneHitCount > zoneHits.size();
    result.insert(QStringLiteral("ok"), !candidatesTruncated && !hitsTruncated);
    result.insert(QStringLiteral("error"), (candidatesTruncated || hitsTruncated)
        ? QStringLiteral("joint-spatial-details-truncated") : QStringLiteral(""));
    result.insert(QStringLiteral("comparisonComplete"), true);
    result.insert(QStringLiteral("pairCount"), pairCount);
    result.insert(QStringLiteral("candidateCount"), candidateCount);
    result.insert(QStringLiteral("candidates"), candidates);
    result.insert(QStringLiteral("candidatesTruncated"), candidatesTruncated);
    result.insert(QStringLiteral("zoneComparisonCount"), zoneComparisonCount);
    result.insert(QStringLiteral("zoneHitCount"), zoneHitCount);
    result.insert(QStringLiteral("zoneHits"), zoneHits);
    result.insert(QStringLiteral("zoneHitsTruncated"), hitsTruncated);
    return result;
}

QVariantMap FleetAirspaceConflictChecker::check(const QVariantList& missionPlans,
                                                const QVariantList& vehicles,
                                                const QVariant& noFlyZonesInput,
                                                const QVariantMap& options)
{
    const QVariantMap inputCheck = inspectNoFlyZoneInput(noFlyZonesInput);
    const bool sourceInvalid = options.value(QStringLiteral("noFlyZonesSourceInvalid")).type() == QVariant::Bool
        && options.value(QStringLiteral("noFlyZonesSourceInvalid")).toBool();
    const auto incompleteReport = [this, &missionPlans, &inputCheck, sourceInvalid](const QString& code,
            qint64 work = 0, int issues = 0, const QString& detail = QString()) {
        QVariantMap report;
        report.insert(QStringLiteral("schemaVersion"), 1);
        report.insert(QStringLiteral("checkerVersion"), QStringLiteral("0.1.0"));
        report.insert(QStringLiteral("missionPlanCount"), missionPlans.size());
        report.insert(QStringLiteral("noFlyZonesInputCheck"), inputCheck);
        report.insert(QStringLiteral("noFlyZoneCount"), sourceInvalid
            ? QJsonValue(QJsonValue::Null).toVariant() : inputCheck.value(QStringLiteral("zoneCount")));
        // No threshold is applied when the whole review is incomplete. In
        // particular, an invalid supplied minimum must not display default 5m.
        report.insert(QStringLiteral("minimumSeparationMeters"), QJsonValue(QJsonValue::Null).toVariant());
        QVariantMap violation = issue(code, QStringLiteral("blocking"),
            detail.isEmpty() ? QStringLiteral("Airspace review exceeded its bounded input/work/result budget; no complete analysis is available.")
                             : detail);
        violation.insert(QStringLiteral("code"), code);
        report.insert(QStringLiteral("violations"), QVariantList() << violation);
        report.insert(QStringLiteral("conflicts"), QVariantList());
        report.insert(QStringLiteral("analysisComplete"), false);
        report.insert(QStringLiteral("feasible"), false);
        report.insert(QStringLiteral("requiresReview"), true);
        report.insert(QStringLiteral("severity"), QStringLiteral("high"));
        report.insert(QStringLiteral("reviewStatus"), QStringLiteral("blocked-incomplete"));
        report.insert(QStringLiteral("estimatedComparisonWork"), work);
        report.insert(QStringLiteral("discardedDiagnosticCount"), issues);
        report.insert(QStringLiteral("maximumRawPointCount"), 16384);
        report.insert(QStringLiteral("maximumComparisonWork"), 1000000);
        report.insert(QStringLiteral("maximumDiagnosticCount"), 2048);
        report.insert(QStringLiteral("manualConfirmationRequired"), true);
        report.insert(QStringLiteral("flightCommandReleased"), false);
        _lastReport = report;
        emit reportChanged();
        return report;
    };
    if (!inputCheck.value(QStringLiteral("valid")).toBool()) {
        const QString reason = inputCheck.value(QStringLiteral("reason")).toString();
        return incompleteReport(reason, 0, 0,
            reason == QStringLiteral("invalid_no_fly_zones_type")
                ? QStringLiteral("noFlyZones must be an actual non-null JSON list; the original invalid value was not replaced with an empty list.")
                : reason == QStringLiteral("no_fly_zone_collection_budget_exceeded")
                    ? QStringLiteral("noFlyZones exceeds the 1024-zone input limit; no zone prefix was analyzed.")
                    : QStringLiteral("noFlyZones contains unsupported JSON values or exceeds its complete input budget; no zone prefix was analyzed."));
    }
    if (sourceInvalid) {
        const QVariant reasonValue = options.value(QStringLiteral("noFlyZonesSourceInvalidReason"));
        const QString requestedReason = reasonValue.type() == QVariant::String ? reasonValue.toString() : QString();
        const QString reason = requestedReason == QStringLiteral("task_no_fly_zones_input_invalid")
            || requestedReason == QStringLiteral("no_fly_zones_source_mismatch")
            ? requestedReason : QStringLiteral("invalid_no_fly_zones_source");
        return incompleteReport(reason, 0, 0,
            reason == QStringLiteral("task_no_fly_zones_input_invalid")
                ? QStringLiteral("The actual task contains an invalid noFlyZones input; a separately supplied valid list cannot replace it.")
                : reason == QStringLiteral("no_fly_zones_source_mismatch")
                    ? QStringLiteral("The actual task and independently supplied noFlyZones lists differ; neither source was silently selected.")
                    : QStringLiteral("The noFlyZones source is invalid or inconsistent; no source was silently selected."));
    }
    // Preserve the original value until its type, complete JSON tree and
    // collection/byte budgets have all passed the native inspection.
    const QVariantList noFlyZones = noFlyZonesInput.toList();
    if (missionPlans.size() > 512) {
        return incompleteReport(QStringLiteral("airspace_collection_budget_exceeded"));
    }
    int rawPointBudget = 0;
    for (const QVariant& value : missionPlans) {
        const QVariantMap plan = value.toMap();
        for (const bool returning : {false, true}) {
            const int count = pathPointCount(plan, returning);
            if (count > 16384 - rawPointBudget) return incompleteReport(QStringLiteral("airspace_point_budget_exceeded"));
            rawPointBudget += count;
        }
    }
    for (const QVariant& value : noFlyZones) {
        const QVariantMap zone = value.toMap();
        const QVariant polygon = firstValue(zone, QStringList()
            << QStringLiteral("polygon") << QStringLiteral("vertices") << QStringLiteral("points")
            << QStringLiteral("boundary") << QStringLiteral("coordinates"));
        const int polygonCount = geometryPointCount(polygon);
        const int count = polygonCount == 0 && zone.contains(QStringLiteral("bounds")) ? 4 : polygonCount;
        if (count > 16384 - rawPointBudget) return incompleteReport(QStringLiteral("airspace_point_budget_exceeded"));
        rawPointBudget += count;
    }
    double minimumSeparation = 5.0;
    bool invalidMinimumSeparation = false;
    if (options.value(QStringLiteral("minimumSeparationInputInvalid")).type() == QVariant::Bool
        && options.value(QStringLiteral("minimumSeparationInputInvalid")).toBool()) invalidMinimumSeparation = true;
    if (options.contains(QStringLiteral("minimumSeparationMeters"))) {
        double suppliedMinimumSeparation = 0.0;
        if (!finiteNumber(options.value(QStringLiteral("minimumSeparationMeters")),
                          &suppliedMinimumSeparation)
            || suppliedMinimumSeparation <= 0.0) {
            invalidMinimumSeparation = true;
        } else {
            minimumSeparation = suppliedMinimumSeparation;
        }
    }
    if (invalidMinimumSeparation) {
        return incompleteReport(QStringLiteral("invalid_minimum_separation"), 0, 0,
            QStringLiteral("minimumSeparationMeters must be a finite positive number; no default threshold or partial geometric analysis was substituted."));
    }
    bool returnRequiredOptionValid = true;
    const bool returnRequiredByOption = strictBool(options, QStringLiteral("returnRequired"), false,
                                                   &returnRequiredOptionValid);

    QList<RawPath> rawPaths;
    QStringList reasons;
    QStringList alternatives;
    QVariantList violations;
    QVariantList conflicts;
    const auto appendDiagnostic = [&conflicts, &violations](QVariantList* target, const QVariantMap& item) {
        if (conflicts.size() + violations.size() >= 2048) return false;
        target->append(item);
        return true;
    };
    if (!returnRequiredOptionValid) {
        QVariantMap item = issue(QStringLiteral("invalid_return_required_option"), QStringLiteral("blocking"),
                                 QStringLiteral("returnRequired must be a boolean."));
        if (!appendDiagnostic(&violations, item)) return incompleteReport(
            QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
        addAlternative(&alternatives, QStringLiteral("provide_a_boolean_return_requirement"));
    }
    if (options.contains(QStringLiteral("timeStepSeconds"))) {
        double timeStep = 0.0;
        if (!finiteNumber(options.value(QStringLiteral("timeStepSeconds")), &timeStep)
            || timeStep <= 0.0) {
            QVariantMap item = issue(QStringLiteral("invalid_time_step"), QStringLiteral("blocking"),
                                     QStringLiteral("timeStepSeconds must be a finite positive number."));
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_a_finite_positive_time_step"));
        }
    }
    int missingVehicleIdCount = 0;
    int missingPathCount = 0;
    bool anyGeodetic = false;
    bool anyLocal = false;
    double referenceLat = 0.0;
    double referenceLon = 0.0;
    bool referenceSet = false;
    QSet<QString> seenVehicleIds;
    QSet<QString> seenZoneIds;
    // Vehicle snapshots are reserved for a future adapter.  This checker
    // requires an explicit, reviewable returnPath and never infers one from a
    // home/RTL point whose frame or freshness may be unknown.
    Q_UNUSED(vehicles)

    for (int planIndex = 0; planIndex < missionPlans.size(); ++planIndex) {
        const QVariantMap plan = missionPlans.at(planIndex).toMap();
        const QString vehicleId = idFor(plan);
        if (vehicleId.isEmpty()) {
            ++missingVehicleIdCount;
            QVariantMap item = issue(QStringLiteral("vehicle_id_missing"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan %1 has no vehicle identifier.").arg(planIndex));
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("assign_unique_vehicle_ids"));
        } else if (seenVehicleIds.contains(vehicleId)) {
            QVariantMap item = issue(QStringLiteral("duplicate_vehicle_id"), QStringLiteral("blocking"),
                                     QStringLiteral("Vehicle %1 appears in more than one mission plan.").arg(vehicleId));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_one_authoritative_plan_per_vehicle"));
        } else {
            seenVehicleIds.insert(vehicleId);
        }
        bool planReturnRequiredValid = true;
        const bool planReturnRequired = strictBool(plan, QStringLiteral("returnRequired"), false,
                                                   &planReturnRequiredValid);
        if (!planReturnRequiredValid) {
            QVariantMap item = issue(QStringLiteral("invalid_plan_return_required"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan returnRequired must be a boolean."));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_a_boolean_plan_return_requirement"));
        }
        const bool returnRequired = returnRequiredByOption || planReturnRequired;
        RawPath mainPath = makePath(plan, planIndex, QStringLiteral("mission"), returnRequired);
        if (!mainPath.coordinateKnown) {
            QVariantMap item = issue(QStringLiteral("mission_coordinate_missing"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan %1 contains an invalid or incomplete coordinate.").arg(planIndex));
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_finite_coordinates_for_all_waypoints"));
        }
        if (mainPath.timingInvalid) {
            QVariantMap item = issue(QStringLiteral("mission_timing_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan %1 contains malformed or non-monotonic timing evidence.").arg(planIndex));
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_finite_monotonic_waypoint_times"));
        } else if (!mainPath.timingKnown && mainPath.rawPoints.size() >= 2) {
            QVariantMap item = issue(QStringLiteral("mission_timing_unknown"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan %1 has no authoritative timing evidence.").arg(planIndex));
            item.insert(QStringLiteral("planIndex"), planIndex);
            item.insert(QStringLiteral("timingEvidence"), mainPath.timingEvidence);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_waypoint_timestamps_or_duration"));
        }
        if (mainPath.rawPoints.size() < 2) {
            ++missingPathCount;
            QVariantMap item = issue(QStringLiteral("mission_path_missing"), QStringLiteral("blocking"),
                                     QStringLiteral("Mission plan %1 has fewer than two valid path points.").arg(planIndex));
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_timestamped_waypoints"));
        } else {
            rawPaths.append(mainPath);
        }
        const QVariantMap returnPlan = plan;
        RawPath returnPath = makePath(returnPlan, planIndex, QStringLiteral("return"), returnRequired);
        if (!returnPath.coordinateKnown && !returnPath.rawPoints.isEmpty()) {
            QVariantMap item = issue(QStringLiteral("return_coordinate_missing"), QStringLiteral("blocking"),
                                     QStringLiteral("Return path for plan %1 contains an invalid coordinate.").arg(planIndex));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_finite_return_coordinates"));
        }
        if (returnPath.timingInvalid) {
            QVariantMap item = issue(QStringLiteral("return_timing_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("Return path for plan %1 contains malformed or non-monotonic timing evidence.").arg(planIndex));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_finite_monotonic_return_times"));
        } else if (!returnPath.timingKnown && returnPath.rawPoints.size() >= 2) {
            QVariantMap item = issue(QStringLiteral("return_timing_unknown"), QStringLiteral("blocking"),
                                     QStringLiteral("Return path for plan %1 has no authoritative timing evidence.").arg(planIndex));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            item.insert(QStringLiteral("timingEvidence"), returnPath.timingEvidence);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("supply_return_waypoint_timestamps_or_duration"));
            addAlternative(&alternatives, QStringLiteral("supply_an_independent_return_start_for_duration_only_routes"));
        }
        if (!returnPath.rawPoints.isEmpty()) {
            if (returnPath.rawPoints.size() < 2) {
                QVariantMap item = issue(QStringLiteral("return_path_incomplete"), QStringLiteral("blocking"),
                                         QStringLiteral("Return path for plan %1 has fewer than two points.").arg(planIndex));
                item.insert(QStringLiteral("vehicleId"), vehicleId);
                if (!appendDiagnostic(&violations, item)) return incompleteReport(
                    QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            } else {
                rawPaths.append(returnPath);
            }
        } else if (returnRequired) {
            QVariantMap item = issue(QStringLiteral("return_path_missing"), QStringLiteral("high"),
                                     QStringLiteral("A safe return corridor is required but no return path was supplied."));
            item.insert(QStringLiteral("vehicleId"), vehicleId);
            item.insert(QStringLiteral("planIndex"), planIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_and_review_return_corridor"));
        }
        for (const RawPoint& point : mainPath.rawPoints) {
            if (point.geodetic) {
                anyGeodetic = true;
                if (!referenceSet) { referenceLat = point.first; referenceLon = point.second; referenceSet = true; }
            } else if (point.valid) anyLocal = true;
        }
        for (const RawPoint& point : returnPath.rawPoints) {
            if (point.geodetic) {
                anyGeodetic = true;
                if (!referenceSet) { referenceLat = point.first; referenceLon = point.second; referenceSet = true; }
            } else if (point.valid) anyLocal = true;
        }
    }

    QList<Zone> zones;
    for (int zoneIndex = 0; zoneIndex < noFlyZones.size(); ++zoneIndex) {
        const QVariantMap source = noFlyZones.at(zoneIndex).toMap();
        Zone zone;
        const QVariant idValue = firstValue(source, QStringList() << QStringLiteral("id")
            << QStringLiteral("zoneId") << QStringLiteral("name"));
        const bool invalidZoneId = idValue.isValid()
            && (idValue.type() != QVariant::String || idValue.toString().size() > 128);
        zone.id = invalidZoneId ? QString() : idValue.toString().trimmed();
        if (zone.id.isEmpty()) zone.id = QStringLiteral("zone-%1").arg(zoneIndex + 1);
        if (invalidZoneId) {
            QVariantMap item = issue(QStringLiteral("no_fly_zone_id_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone identity must be a bounded string."));
            item.insert(QStringLiteral("zoneIndex"), zoneIndex);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
        }
        if (seenZoneIds.contains(zone.id)) {
            QVariantMap item = issue(QStringLiteral("duplicate_no_fly_zone_id"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone %1 is declared more than once.").arg(zone.id));
            item.insert(QStringLiteral("zoneId"), zone.id);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_unique_no_fly_zone_ids"));
        }
        seenZoneIds.insert(zone.id);
        QVariant polygonValue = firstValue(source, QStringList()
            << QStringLiteral("polygon") << QStringLiteral("vertices")
            << QStringLiteral("points") << QStringLiteral("boundary")
            << QStringLiteral("coordinates"));
        QVariantList polygon = listValue(polygonValue);
        if (polygon.isEmpty() && source.contains(QStringLiteral("bounds"))) {
            const QVariantMap bounds = source.value(QStringLiteral("bounds")).toMap();
            const bool geodeticBoundsPresent = bounds.contains(QStringLiteral("west"))
                && bounds.contains(QStringLiteral("east"))
                && bounds.contains(QStringLiteral("south"))
                && bounds.contains(QStringLiteral("north"));
            if (geodeticBoundsPresent) {
                double west = 0.0, east = 0.0, south = 0.0, north = 0.0;
                if (finiteNumber(bounds.value(QStringLiteral("west")), &west)
                        && finiteNumber(bounds.value(QStringLiteral("east")), &east)
                        && finiteNumber(bounds.value(QStringLiteral("south")), &south)
                        && finiteNumber(bounds.value(QStringLiteral("north")), &north)
                        && west >= -180.0 && west <= 180.0
                        && east >= -180.0 && east <= 180.0
                        && south >= -90.0 && south <= 90.0
                        && north >= -90.0 && north <= 90.0
                        && west < east && south < north) {
                    QVariantMap p1; p1.insert(QStringLiteral("latitude"), south); p1.insert(QStringLiteral("longitude"), west);
                    QVariantMap p2; p2.insert(QStringLiteral("latitude"), south); p2.insert(QStringLiteral("longitude"), east);
                    QVariantMap p3; p3.insert(QStringLiteral("latitude"), north); p3.insert(QStringLiteral("longitude"), east);
                    QVariantMap p4; p4.insert(QStringLiteral("latitude"), north); p4.insert(QStringLiteral("longitude"), west);
                    polygon = QVariantList() << p1 << p2 << p3 << p4;
                }
            } else {
                double minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;
                if (finiteNumber(bounds.value(QStringLiteral("minX")), &minX)
                        && finiteNumber(bounds.value(QStringLiteral("maxX")), &maxX)
                        && finiteNumber(bounds.value(QStringLiteral("minY")), &minY)
                        && finiteNumber(bounds.value(QStringLiteral("maxY")), &maxY)
                        && minX < maxX && minY < maxY) {
                    QVariantMap p1; p1.insert(QStringLiteral("x"), minX); p1.insert(QStringLiteral("y"), minY);
                    QVariantMap p2; p2.insert(QStringLiteral("x"), maxX); p2.insert(QStringLiteral("y"), minY);
                    QVariantMap p3; p3.insert(QStringLiteral("x"), maxX); p3.insert(QStringLiteral("y"), maxY);
                    QVariantMap p4; p4.insert(QStringLiteral("x"), minX); p4.insert(QStringLiteral("y"), maxY);
                    polygon = QVariantList() << p1 << p2 << p3 << p4;
                }
            }
        }
        bool allPolygonPointsValid = !polygon.isEmpty();
        for (const QVariant& value : polygon) {
            const RawPoint point = rawPoint(value);
            const QVariantMap vertex = value.toMap();
            const QList<QVariantMap> referenceSources = QList<QVariantMap>() << source
                << source.value(QStringLiteral("bounds")).toMap() << vertex
                << vertex.value(QStringLiteral("position")).toMap();
            DeclaredReference frame = declaredReference(referenceSources, QStringLiteral("localFrameId"));
            DeclaredReference altitude = declaredReference(referenceSources, QStringLiteral("altitudeReference"));
            if (point.geodetic && !frame.values.isEmpty()) frame.invalid = true;
            if (altitude.values.contains(QStringLiteral("local_origin"))
                && (frame.values.isEmpty() || referenceIncompatible(frame))) altitude.invalid = true;
            mergeReference(&zone.frameReference, frame);
            mergeReference(&zone.altitudeReference, altitude);
            mergeReference(&zone.timeReference, declaredReference(referenceSources, QStringLiteral("timeReference")));
            zone.rawPolygon.append(point);
            if (!point.valid) allPolygonPointsValid = false;
            if (point.geodetic) {
                anyGeodetic = true;
                if (!referenceSet) { referenceLat = point.first; referenceLon = point.second; referenceSet = true; }
            } else if (point.valid) anyLocal = true;
        }
        zone.polygonKnown = zone.rawPolygon.size() >= 3 && allPolygonPointsValid;
        const QVariant minAltitudeValue = firstValue(source, QStringList() << QStringLiteral("minAltitude")
            << QStringLiteral("minAltitudeMeters") << QStringLiteral("floor"));
        const QVariant maxAltitudeValue = firstValue(source, QStringList() << QStringLiteral("maxAltitude")
            << QStringLiteral("maxAltitudeMeters") << QStringLiteral("ceiling"));
        const bool minAltitudePresent = minAltitudeValue.isValid();
        const bool maxAltitudePresent = maxAltitudeValue.isValid();
        zone.altitudeBounded = minAltitudePresent || maxAltitudePresent;
        const bool minAltitudeValid = !minAltitudePresent || finiteNumber(minAltitudeValue, &zone.minAltitude);
        const bool maxAltitudeValid = !maxAltitudePresent || finiteNumber(maxAltitudeValue, &zone.maxAltitude);
        zone.altitudeInvalid = !minAltitudeValid || !maxAltitudeValid || zone.minAltitude > zone.maxAltitude;
        if (zone.altitudeInvalid) {
            QVariantMap item = issue(QStringLiteral("no_fly_zone_altitude_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone %1 has invalid altitude limits.").arg(zone.id));
            item.insert(QStringLiteral("zoneId"), zone.id);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_finite_ordered_altitude_limits"));
        }
        const QVariant startValue = firstValue(source, QStringList()
            << QStringLiteral("startTime") << QStringLiteral("activeFrom"));
        const QVariant endValue = firstValue(source, QStringList()
            << QStringLiteral("endTime") << QStringLiteral("activeUntil"));
        const bool hasStartTime = finiteNumber(startValue, &zone.startTime);
        const bool hasEndTime = finiteNumber(endValue, &zone.endTime);
        if ((startValue.isValid() && !hasStartTime) || (endValue.isValid() && !hasEndTime)) {
            zone.timeWindowInvalid = true;
            QVariantMap item = issue(QStringLiteral("no_fly_zone_time_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone %1 has a non-finite time boundary.").arg(zone.id));
            item.insert(QStringLiteral("zoneId"), zone.id);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_finite_no_fly_zone_times"));
        }
        zone.timingKnown = hasStartTime || hasEndTime;
        if (hasStartTime && hasEndTime && zone.startTime > zone.endTime) {
            zone.timeWindowInvalid = true;
            QVariantMap item = issue(QStringLiteral("no_fly_zone_time_window_invalid"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone %1 has a start time after its end time.").arg(zone.id));
            item.insert(QStringLiteral("zoneId"), zone.id);
            item.insert(QStringLiteral("startTime"), zone.startTime);
            item.insert(QStringLiteral("endTime"), zone.endTime);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_an_ordered_no_fly_zone_time_window"));
        }
        if (!zone.polygonKnown) {
            QVariantMap item = issue(QStringLiteral("no_fly_zone_geometry_missing"), QStringLiteral("blocking"),
                                     QStringLiteral("No-fly zone %1 has no valid polygon or bounds.").arg(zone.id));
            item.insert(QStringLiteral("zoneId"), zone.id);
            if (!appendDiagnostic(&violations, item)) return incompleteReport(
                QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
            addAlternative(&alternatives, QStringLiteral("provide_no_fly_zone_geometry"));
        }
        zones.append(zone);
    }

    DeclaredReference frameReferences;
    DeclaredReference altitudeReferences;
    DeclaredReference timeReferences;
    for (const RawPath& path : rawPaths) {
        mergeReference(&frameReferences, path.frameReference);
        mergeReference(&altitudeReferences, path.altitudeReference);
        mergeReference(&timeReferences, path.timeReference);
    }
    for (const Zone& zone : zones) {
        mergeReference(&frameReferences, zone.frameReference);
        if (zone.altitudeBounded || zone.altitudeReference.invalid) {
            mergeReference(&altitudeReferences, zone.altitudeReference);
        }
        if (zone.timingKnown || zone.timeReference.invalid) mergeReference(&timeReferences, zone.timeReference);
    }
    const bool frameReferencesIncompatible = referenceIncompatible(frameReferences);
    const bool altitudeReferencesIncompatible = referenceIncompatible(altitudeReferences);
    const bool timeReferencesIncompatible = referenceIncompatible(timeReferences);
    const auto addReferenceViolation = [&appendDiagnostic, &violations](const QString& code,
                                                                       const DeclaredReference& reference) {
        QVariantMap item = issue(code, QStringLiteral("blocking"),
            QStringLiteral("Declared references are invalid, conflicting, or mixed with an unknown reference; numeric exclusions are not certified."));
        QStringList values = reference.values.values();
        values.sort();
        item.insert(QStringLiteral("declaredReferences"), values.mid(0, 32));
        item.insert(QStringLiteral("declaredReferenceCount"), values.size());
        item.insert(QStringLiteral("referenceListTruncated"), values.size() > 32);
        item.insert(QStringLiteral("unknownReferencePresent"), reference.missing);
        item.insert(QStringLiteral("invalidDeclarationPresent"), reference.invalid);
        return appendDiagnostic(&violations, item);
    };
    if ((frameReferencesIncompatible && !addReferenceViolation(QStringLiteral("local_frame_reference_incompatible"), frameReferences))
        || (altitudeReferencesIncompatible && !addReferenceViolation(QStringLiteral("altitude_reference_incompatible"), altitudeReferences))
        || (timeReferencesIncompatible && !addReferenceViolation(QStringLiteral("time_reference_incompatible"), timeReferences))) {
        return incompleteReport(QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
    }

    qint64 comparisonWork = 0;
    const auto reserveWork = [&comparisonWork](qint64 amount) {
        if (amount < 0 || amount > 1000000 - comparisonWork) return false;
        comparisonWork += amount;
        return true;
    };
    for (int first = 0; first < rawPaths.size(); ++first) {
        const RawPath& left = rawPaths.at(first);
        const qint64 leftSegments = qMax(0, left.rawPoints.size() - 1);
        for (int second = first + 1; second < rawPaths.size(); ++second) {
            const RawPath& right = rawPaths.at(second);
            if (left.vehicleId == right.vehicleId) continue;
            const qint64 rightSegments = qMax(0, right.rawPoints.size() - 1);
            if (!reserveWork(leftSegments * rightSegments)) {
                return incompleteReport(QStringLiteral("airspace_work_budget_exceeded"), 1000001);
            }
        }
        for (const Zone& zone : zones) {
            if (!reserveWork(leftSegments * static_cast<qint64>(zone.rawPolygon.size()))) {
                return incompleteReport(QStringLiteral("airspace_work_budget_exceeded"), 1000001);
            }
        }
    }
    QList<Path> paths;
    bool coordinateMismatch = false;
    bool allPathAltitudesKnown = true;
    bool allPathTimesKnown = true;
    for (const RawPath& raw : rawPaths) {
        Path path;
        path.vehicleId = raw.vehicleId;
        path.kind = raw.kind;
        path.planIndex = raw.planIndex;
        path.times = raw.times;
        path.timingEvidence = raw.timingEvidence;
        path.timingKnown = raw.timingKnown && !timeReferencesIncompatible;
        path.timingInvalid = raw.timingInvalid;
        path.coordinateKnown = raw.coordinateKnown;
        path.altitudeKnown = raw.altitudeKnown && !altitudeReferencesIncompatible;
        allPathAltitudesKnown = allPathAltitudesKnown && path.altitudeKnown;
        allPathTimesKnown = allPathTimesKnown && path.timingKnown;
        path.returnRequired = raw.returnRequired;
        bool hasGeodetic = false;
        bool hasLocal = false;
        for (const RawPoint& point : raw.rawPoints) {
            if (point.geodetic) hasGeodetic = true; else if (point.valid) hasLocal = true;
            Point local = toLocal(point, &coordinateMismatch, referenceSet, referenceLat, referenceLon);
            if (altitudeReferencesIncompatible) local.z = 0.0;
            path.points.append(local);
        }
        if (hasGeodetic && hasLocal) path.coordinateKnown = false;
        paths.append(path);
    }
    // Mixing an unspecified local frame with geodetic paths is unsafe because
    // their origins/units cannot be inferred by this transport-neutral class.
    if (anyGeodetic && anyLocal) coordinateMismatch = true;
    // A local ENU path and a latitude/longitude path cannot be compared until
    // the caller supplies an explicit common frame. Do not silently interpret
    // their numeric values as the same metres-based coordinate system.
    if ((anyGeodetic && !referenceSet) || (anyGeodetic && anyLocal)) {
        coordinateMismatch = true;
    }
    if (coordinateMismatch) {
        QVariantMap item = issue(QStringLiteral("coordinate_system_mismatch"), QStringLiteral("blocking"),
                                 QStringLiteral("Plans or zones use incompatible or incomplete coordinate systems."));
        if (!appendDiagnostic(&violations, item)) return incompleteReport(
            QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
        addAlternative(&alternatives, QStringLiteral("normalize_all_paths_to_one_coordinate_frame"));
    }

    // Check pairwise paths with exact linear relative-motion minimum inside
    // each overlapping time interval when timestamps are available.
    for (int first = 0; first < paths.size(); ++first) {
        if (coordinateMismatch || frameReferencesIncompatible) break;
        const Path& left = paths.at(first);
        if (left.points.size() < 2 || left.vehicleId.isEmpty()) continue;
        for (int second = first + 1; second < paths.size(); ++second) {
            const Path& right = paths.at(second);
            if (right.points.size() < 2 || right.vehicleId.isEmpty() || left.vehicleId == right.vehicleId) continue;
            for (int leftSegment = 0; leftSegment + 1 < left.points.size(); ++leftSegment) {
                for (int rightSegment = 0; rightSegment + 1 < right.points.size(); ++rightSegment) {
                    Point leftFirst = left.points.at(leftSegment);
                    Point leftLast = left.points.at(leftSegment + 1);
                    Point rightFirst = right.points.at(rightSegment);
                    Point rightLast = right.points.at(rightSegment + 1);
                    const bool pairAltitudeKnown = left.altitudeKnown && right.altitudeKnown;
                    if (!pairAltitudeKnown) {
                        // Matching datum labels do not supply a missing
                        // height. Review this pair conservatively in XY;
                        // neither aircraft may be numerically placed at z=0.
                        leftFirst.z = leftLast.z = rightFirst.z = rightLast.z = 0.0;
                    }
                    const bool timed = left.timingKnown && right.timingKnown
                        && leftSegment + 1 < left.times.size() && rightSegment + 1 < right.times.size();
                    double distanceAtClosest = 0.0;
                    double overlapStart = 0.0;
                    double overlapEnd = 0.0;
                    if (timed) {
                        const double leftStart = left.times.at(leftSegment);
                        const double leftEnd = left.times.at(leftSegment + 1);
                        const double rightStart = right.times.at(rightSegment);
                        const double rightEnd = right.times.at(rightSegment + 1);
                        overlapStart = qMax(leftStart, rightStart);
                        overlapEnd = qMin(leftEnd, rightEnd);
                        if (overlapEnd < overlapStart) continue;
                        const double leftDuration = qMax(1.0e-9, leftEnd - leftStart);
                        const double rightDuration = qMax(1.0e-9, rightEnd - rightStart);
                        const Point leftAtStart = interpolate(leftFirst, leftLast,
                            qBound(0.0, (overlapStart - leftStart) / leftDuration, 1.0));
                        const Point rightAtStart = interpolate(rightFirst, rightLast,
                            qBound(0.0, (overlapStart - rightStart) / rightDuration, 1.0));
                        const Point leftAtEnd = interpolate(leftFirst, leftLast,
                            qBound(0.0, (overlapEnd - leftStart) / leftDuration, 1.0));
                        const Point rightAtEnd = interpolate(rightFirst, rightLast,
                            qBound(0.0, (overlapEnd - rightStart) / rightDuration, 1.0));
                        const Point relativeStart{leftAtStart.x - rightAtStart.x, leftAtStart.y - rightAtStart.y, leftAtStart.z - rightAtStart.z};
                        const Point relativeDelta{(leftAtEnd.x - rightAtEnd.x) - relativeStart.x,
                                                  (leftAtEnd.y - rightAtEnd.y) - relativeStart.y,
                                                  (leftAtEnd.z - rightAtEnd.z) - relativeStart.z};
                        const double denominator = relativeDelta.x * relativeDelta.x
                            + relativeDelta.y * relativeDelta.y + relativeDelta.z * relativeDelta.z;
                        const double fraction = denominator > 1.0e-12
                            ? qBound(0.0, -(relativeStart.x * relativeDelta.x
                                + relativeStart.y * relativeDelta.y + relativeStart.z * relativeDelta.z) / denominator, 1.0)
                            : 0.0;
                        distanceAtClosest = qSqrt((relativeStart.x + relativeDelta.x * fraction) * (relativeStart.x + relativeDelta.x * fraction)
                            + (relativeStart.y + relativeDelta.y * fraction) * (relativeStart.y + relativeDelta.y * fraction)
                            + (relativeStart.z + relativeDelta.z * fraction) * (relativeStart.z + relativeDelta.z * fraction));
                    } else {
                        distanceAtClosest = minSpatialDistance(leftFirst, leftLast, rightFirst, rightLast);
                        overlapStart = qMin(leftSegment, rightSegment);
                        overlapEnd = qMax(leftSegment + 1, rightSegment + 1);
                    }
                    if (distanceAtClosest >= minimumSeparation) continue;
                    QVariantMap item = issue(QStringLiteral("4d_separation_conflict"),
                        severityForDistance(distanceAtClosest, minimumSeparation),
                        QStringLiteral("Two planned paths come within the configured minimum separation."));
                    item.insert(QStringLiteral("vehicleA"), left.vehicleId);
                    item.insert(QStringLiteral("vehicleB"), right.vehicleId);
                    item.insert(QStringLiteral("pathA"), left.kind);
                    item.insert(QStringLiteral("pathB"), right.kind);
                    item.insert(QStringLiteral("segmentA"), leftSegment);
                    item.insert(QStringLiteral("segmentB"), rightSegment);
                    item.insert(QStringLiteral("distanceMeters"), distanceAtClosest);
                    item.insert(QStringLiteral("timeStart"), overlapStart);
                    item.insert(QStringLiteral("timeEnd"), overlapEnd);
                    item.insert(QStringLiteral("timingKnown"), timed);
                    item.insert(QStringLiteral("altitudeKnown"), pairAltitudeKnown);
                    item.insert(QStringLiteral("distanceDimensions"), pairAltitudeKnown ? 3 : 2);
                    item.insert(QStringLiteral("timingEvidenceA"), left.timingEvidence);
                    item.insert(QStringLiteral("timingEvidenceB"), right.timingEvidence);
                    item.insert(QStringLiteral("checkMode"), timed
                        ? QStringLiteral("time-window") : QStringLiteral("spatial-only"));
                    if (!appendDiagnostic(&conflicts, item)) return incompleteReport(
                        QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
                    addAlternative(&alternatives, timed ? QStringLiteral("stagger_conflicting_time_windows")
                                                        : QStringLiteral("provide_waypoint_timestamps_and_replan"));
                }
            }
        }
    }

    // Check each path against each no-fly zone. Use exact 2D segment/polygon
    // intersection instead of a fixed sample count: a long segment must not
    // be able to jump over a narrow or transparent boundary. Altitude and
    // time are checked conservatively over each segment; unknown altitude is
    // a blocking review item whenever the zone declares an altitude limit.
    for (const Path& path : paths) {
        if (coordinateMismatch || frameReferencesIncompatible) break;
        if (path.points.size() < 2 || path.vehicleId.isEmpty()) continue;
        for (const Zone& zone : zones) {
            if (!zone.polygonKnown || zone.timeWindowInvalid || zone.altitudeInvalid) continue;
            QList<Point> polygon;
            for (const RawPoint& raw : zone.rawPolygon) polygon.append(toLocal(raw, &coordinateMismatch, referenceSet, referenceLat, referenceLon));
            bool altitudeEvidenceMissingReported = false;
            for (int segment = 0; segment + 1 < path.points.size(); ++segment) {
                const double start = segment < path.times.size() ? path.times.at(segment) : segment;
                const double end = segment + 1 < path.times.size() ? path.times.at(segment + 1) : segment + 1;
                const double lowTime = qMin(start, end);
                const double highTime = qMax(start, end);
                // A synthetic relative/constant-speed timeline must not
                // skip a time-bounded exclusion zone. Unknown time means
                // conservative spatial review against the entire zone.
                if (path.timingKnown && zone.timingKnown
                    && (highTime < zone.startTime || lowTime > zone.endTime)) continue;
                const Point& startPoint = path.points.at(segment);
                const Point& endPoint = path.points.at(segment + 1);
                if (!segmentIntersectsPolygon2D(startPoint, endPoint, polygon)) continue;
                if (zone.altitudeBounded && !path.altitudeKnown) {
                    if (!altitudeEvidenceMissingReported) {
                        QVariantMap item = issue(QStringLiteral("path_altitude_missing"), QStringLiteral("blocking"),
                                                 QStringLiteral("Path %1 has no authoritative altitude for bounded no-fly zone %2.")
                                                     .arg(path.vehicleId, zone.id));
                        item.insert(QStringLiteral("vehicleId"), path.vehicleId);
                        item.insert(QStringLiteral("path"), path.kind);
                        item.insert(QStringLiteral("zoneId"), zone.id);
                        if (!appendDiagnostic(&violations, item)) return incompleteReport(
                            QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
                        addAlternative(&alternatives, QStringLiteral("supply_finite_altitude_for_all_waypoints"));
                        altitudeEvidenceMissingReported = true;
                    }
                    break;
                }
                const double minPathAltitude = qMin(startPoint.z, endPoint.z);
                const double maxPathAltitude = qMax(startPoint.z, endPoint.z);
                if (maxPathAltitude < zone.minAltitude || minPathAltitude > zone.maxAltitude) continue;
                const double enteredTime = qMax(lowTime, zone.timingKnown ? zone.startTime : lowTime);
                QVariantMap item = issue(path.kind == QStringLiteral("return")
                    ? QStringLiteral("return_corridor_no_fly_violation") : QStringLiteral("no_fly_zone_violation"),
                    QStringLiteral("critical"),
                    QStringLiteral("Planned path enters no-fly zone %1.").arg(zone.id));
                item.insert(QStringLiteral("vehicleId"), path.vehicleId);
                item.insert(QStringLiteral("path"), path.kind);
                item.insert(QStringLiteral("zoneId"), zone.id);
                item.insert(QStringLiteral("segment"), segment);
                item.insert(QStringLiteral("time"), path.timingKnown ? QVariant(enteredTime) : QVariant());
                item.insert(QStringLiteral("timingKnown"), path.timingKnown && zone.timingKnown);
                item.insert(QStringLiteral("timingEvidence"), path.timingEvidence);
                item.insert(QStringLiteral("checkMode"), path.timingKnown && zone.timingKnown
                    ? QStringLiteral("time-window") : QStringLiteral("spatial-only"));
                if (!appendDiagnostic(&violations, item)) return incompleteReport(
                    QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
                addAlternative(&alternatives, path.kind == QStringLiteral("return")
                    ? QStringLiteral("reserve_a_safe_return_corridor") : QStringLiteral("reroute_around_no_fly_zone"));
                break;
            }
        }
    }

    if (missionPlans.isEmpty()) {
        QVariantMap item = issue(QStringLiteral("mission_plans_missing"), QStringLiteral("blocking"),
                                 QStringLiteral("No mission plans were supplied for airspace review."));
        if (!appendDiagnostic(&violations, item)) return incompleteReport(
            QStringLiteral("airspace_diagnostic_budget_exceeded"), 0, conflicts.size() + violations.size());
        addAlternative(&alternatives, QStringLiteral("supply_at_least_one_mission_plan"));
    }

    bool hasCritical = false;
    bool hasHigh = false;
    bool requiresReview = false;
    for (const QVariant& item : violations) {
        const QString severity = item.toMap().value(QStringLiteral("severity")).toString();
        hasCritical = hasCritical || severity == QStringLiteral("critical");
        hasHigh = hasHigh || severity == QStringLiteral("high") || severity == QStringLiteral("blocking");
        requiresReview = true;
    }
    for (const QVariant& item : conflicts) {
        const QString severity = item.toMap().value(QStringLiteral("severity")).toString();
        hasCritical = hasCritical || severity == QStringLiteral("critical");
        hasHigh = hasHigh || severity == QStringLiteral("high");
        requiresReview = true;
    }
    if (missingVehicleIdCount > 0 || missingPathCount > 0) requiresReview = true;
    if (!conflicts.isEmpty()) reasons << QStringLiteral("4D 路径间距不足");
    if (!violations.isEmpty()) reasons << QStringLiteral("存在禁飞区、返航通道或输入完整性问题");
    for (const QString& reason : reasons) addAlternative(&alternatives, reason == QStringLiteral("4D 路径间距不足")
        ? QStringLiteral("review_conflicting_paths_before_release") : QStringLiteral("resolve_all_airspace_violations"));

    QVariantMap report;
    report.insert(QStringLiteral("schemaVersion"), 1);
    report.insert(QStringLiteral("analysisComplete"), true);
    report.insert(QStringLiteral("rawPointCount"), rawPointBudget);
    report.insert(QStringLiteral("estimatedComparisonWork"), comparisonWork);
    report.insert(QStringLiteral("maximumRawPointCount"), 16384);
    report.insert(QStringLiteral("maximumComparisonWork"), 1000000);
    report.insert(QStringLiteral("maximumDiagnosticCount"), 2048);
    report.insert(QStringLiteral("coordinateComparisonAvailable"), !coordinateMismatch && !frameReferencesIncompatible);
    report.insert(QStringLiteral("altitudeComparisonAvailable"), allPathAltitudesKnown && !altitudeReferencesIncompatible);
    report.insert(QStringLiteral("timeComparisonAvailable"), allPathTimesKnown && !timeReferencesIncompatible);
    report.insert(QStringLiteral("referencePolicy"), QStringLiteral("declared-compatible-or-all-absent-legacy"));
    report.insert(QStringLiteral("checkerVersion"), QStringLiteral("0.1.0"));
    report.insert(QStringLiteral("missionPlanCount"), missionPlans.size());
    report.insert(QStringLiteral("pathCount"), paths.size());
    report.insert(QStringLiteral("noFlyZoneCount"), zones.size());
    report.insert(QStringLiteral("noFlyZonesInputCheck"), inputCheck);
    report.insert(QStringLiteral("minimumSeparationMeters"), minimumSeparation);
    report.insert(QStringLiteral("conflicts"), conflicts);
    report.insert(QStringLiteral("violations"), violations);
    report.insert(QStringLiteral("reasons"), reasons);
    report.insert(QStringLiteral("alternatives"), alternatives);
    report.insert(QStringLiteral("requiresReview"), requiresReview);
    report.insert(QStringLiteral("feasible"), conflicts.isEmpty() && violations.isEmpty()
        && missingVehicleIdCount == 0 && missingPathCount == 0);
    report.insert(QStringLiteral("severity"), hasCritical ? QStringLiteral("critical")
        : hasHigh ? QStringLiteral("high") : requiresReview ? QStringLiteral("warning") : QStringLiteral("informational"));
    report.insert(QStringLiteral("manualConfirmationRequired"), true);
    report.insert(QStringLiteral("flightCommandReleased"), false);
    report.insert(QStringLiteral("coordinateFrame"), anyGeodetic ? QStringLiteral("local_projection") : QStringLiteral("local_or_unspecified"));
    _lastReport = report;
    emit reportChanged();
    return report;
}

void FleetAirspaceConflictChecker::clear()
{
    if (_lastReport.isEmpty()) return;
    _lastReport.clear();
    emit reportChanged();
}

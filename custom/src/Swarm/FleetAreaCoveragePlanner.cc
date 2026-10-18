#include "FleetAreaCoveragePlanner.h"

#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QMap>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr int MaxVertices = 64;
constexpr int MaxHoles = 8;
constexpr int MaxDomainVertices = 128;
constexpr int MaxSetbackContours = 32;
constexpr int MaxSetbackVertices = 256;
constexpr int MaxBooleanElements = 2048;
constexpr int MaxLegs = 1024;
constexpr int MaxRoutePoints = 8192;
constexpr int MaxVisibilityChecks = 32768;
constexpr double Epsilon = 1e-6; // metres in the bounded local planning frame

struct Leg { QPointF start; QPointF end; int lane; int component = 0; };

struct RoutePoint {
    QPointF position;
    double altitude;
    // Preserve exact caller coordinates at endpoints rather than round-tripping
    // them through the approximate geographic planning projection.
    QVariantMap suppliedCoordinates;
};

struct Endpoints { RoutePoint start; RoutePoint returnTo; };

struct Domain {
    QPointF origin;
    QGeoCoordinate geoOrigin;
    QVector<QPointF> polygon;
    QVector<QVector<QPointF>> holes;
    QVariantList canonicalHoles;
    QVariantList originalVertices;
    QVariantList originalHoles;
    double twiceArea = 0.0;
    double removedArea = 0.0;
    int vertexCount = 0;
    double setback = 0.0;
    double setbackArea = 0.0;
    QVector<QVector<QPointF>> setbackContours;
    QVector<int> componentContours;
};

bool number(const QVariant& value, double* output)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    bool ok = false;
    const double result = value.toDouble(&ok);
    if (!ok || !qIsFinite(result)) return false;
    *output = result;
    return true;
}

QString readSetback(const QVariant& area, double* value)
{
    *value = 0.0;
    if (area.type() != QVariant::Map || area.isNull()
        || !area.toMap().contains(QStringLiteral("boundarySetbackMeters"))) return QString();
    if (!number(area.toMap().value(QStringLiteral("boundarySetbackMeters")), value)
        || *value < 0.0 || *value > 500.0) return QStringLiteral("coverage_invalid_boundary_setback");
    return QString();
}

bool coordinate(const QVariantMap& point, const QStringList& aliases, double* output)
{
    bool found = false;
    for (const QString& key : aliases) {
        if (!point.contains(key)) continue;
        double value = 0.0;
        if (!number(point.value(key), &value) || (found && value != *output)) return false;
        *output = value;
        found = true;
    }
    return found;
}

double cross(const QPointF& a, const QPointF& b) { return a.x() * b.y() - a.y() * b.x(); }
double dot(const QPointF& a, const QPointF& b) { return a.x() * b.x() + a.y() * b.y(); }
double length(const QPointF& value) { return std::hypot(value.x(), value.y()); }

bool onSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    return std::abs(cross(b - a, p - a)) <= Epsilon * qMax(1.0, length(b - a))
        && dot(p - a, p - b) <= Epsilon * Epsilon;
}

bool inside(const QVector<QPointF>& polygon, const QPointF& p)
{
    bool contained = false;
    for (int i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const QPointF& a = polygon.at(j);
        const QPointF& b = polygon.at(i);
        if (onSegment(p, a, b)) return true;
        if ((a.y() > p.y()) != (b.y() > p.y())
            && p.x() < a.x() + (p.y() - a.y()) * (b.x() - a.x()) / (b.y() - a.y())) {
            contained = !contained;
        }
    }
    return contained;
}

// Exclusion boundaries may be followed by this nominal geometry proposal.
// Only the strict interior is removed; this is deliberately not a setback or
// obstacle-clearance claim, and no vehicle radius is inferred.
bool strictlyInside(const QVector<QPointF>& polygon, const QPointF& p)
{
    for (int i = 0; i < polygon.size(); ++i) {
        if (onSegment(p, polygon.at(i), polygon.at((i + 1) % polygon.size()))) return false;
    }
    return inside(polygon, p);
}

bool allowedPoint(const QVector<QPointF>& polygon, const QVector<QVector<QPointF>>& holes,
                  const QPointF& point, bool allowOutside)
{
    if (!allowOutside && !inside(polygon, point)) return false;
    for (const QVector<QPointF>& hole : holes) if (strictlyInside(hole, point)) return false;
    return true;
}

bool edgesIntersect(const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d)
{
    if (onSegment(a, c, d) || onSegment(b, c, d)
        || onSegment(c, a, b) || onSegment(d, a, b)) return true;
    const double first = cross(b - a, c - a);
    const double second = cross(b - a, d - a);
    const double third = cross(d - c, a - c);
    const double fourth = cross(d - c, b - c);
    return ((first > 0.0 && second < 0.0) || (first < 0.0 && second > 0.0))
        && ((third > 0.0 && fourth < 0.0) || (third < 0.0 && fourth > 0.0));
}

double pointSegmentDistance(const QPointF& point, const QPointF& a, const QPointF& b)
{
    const QPointF edge = b - a;
    const double squared = dot(edge, edge);
    const double ratio = squared > 0.0 ? qBound(0.0, dot(point - a, edge) / squared, 1.0) : 0.0;
    return length(point - (a + edge * ratio));
}

bool setbackClearance(const Domain& domain, const QPointF& a, const QPointF& b)
{
    if (domain.setback <= 0.0) return true;
    // The Qt boolean construction only supplies candidates. This independent
    // check uses every original edge and never reduces the requested distance
    // by a numeric tolerance. Original interior/holes are checked separately.
    QVector<QVector<QPointF>> rings;
    rings.append(domain.polygon);
    rings += domain.holes;
    for (const QVector<QPointF>& ring : rings) {
        for (int i = 0; i < ring.size(); ++i) {
            const QPointF c = ring.at(i), d = ring.at((i + 1) % ring.size());
            if (edgesIntersect(a, b, c, d)) return false;
            const double distance = qMin(qMin(pointSegmentDistance(a, c, d), pointSegmentDistance(b, c, d)),
                qMin(pointSegmentDistance(c, a, b), pointSegmentDistance(d, a, b)));
            if (!qIsFinite(distance) || distance < domain.setback) return false;
        }
    }
    return true;
}

QString validateRing(QVector<QPointF>* polygon, double* twiceArea)
{
    *twiceArea = 0.0;
    for (int i = 0; i < polygon->size(); ++i) {
        const int next = (i + 1) % polygon->size();
        if (length(polygon->at(i) - polygon->at(next)) <= Epsilon) return QStringLiteral("degenerate");
        const QPointF incoming = polygon->at(i) - polygon->at((i + polygon->size() - 1) % polygon->size());
        const QPointF outgoing = polygon->at(next) - polygon->at(i);
        if (std::abs(cross(incoming, outgoing)) <= Epsilon * qMax(1.0, length(incoming))
            && dot(incoming, outgoing) < 0.0) return QStringLiteral("overlapping_edges");
        *twiceArea += cross(polygon->at(i), polygon->at(next));
        for (int j = i + 1; j < polygon->size(); ++j) {
            if (j == next || (j + 1) % polygon->size() == i) continue;
            if (edgesIntersect(polygon->at(i), polygon->at(next), polygon->at(j), polygon->at((j + 1) % polygon->size()))) {
                return QStringLiteral("self_intersecting");
            }
        }
    }
    if (std::abs(*twiceArea) <= Epsilon) return QStringLiteral("degenerate");
    if (*twiceArea < 0.0) std::reverse(polygon->begin(), polygon->end());
    return QString();
}

bool ringsTouch(const QVector<QPointF>& first, const QVector<QPointF>& second)
{
    for (int i = 0; i < first.size(); ++i) {
        for (int j = 0; j < second.size(); ++j) {
            if (edgesIntersect(first.at(i), first.at((i + 1) % first.size()),
                               second.at(j), second.at((j + 1) % second.size()))) return true;
        }
    }
    return false;
}

QPainterPath ringPath(const QVector<QPointF>& ring)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.moveTo(ring.first());
    for (int i = 1; i < ring.size(); ++i) path.lineTo(ring.at(i));
    path.closeSubpath();
    return path;
}

bool boundedPath(const QPainterPath& path)
{
    if (path.elementCount() > MaxBooleanElements) return false;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element element = path.elementAt(i);
        if ((!element.isMoveTo() && !element.isLineTo()) || !qIsFinite(element.x) || !qIsFinite(element.y)
            || std::abs(element.x) > 20000.0 || std::abs(element.y) > 20000.0) return false;
    }
    return true;
}

QVector<QPointF> squareEdgeHull(const QPointF& a, const QPointF& b, double radius)
{
    QVector<QPointF> points;
    for (const QPointF& end : {a, b}) {
        for (double dx : {-radius, radius}) {
            for (double dy : {-radius, radius}) points.append(end + QPointF(dx, dy));
        }
    }
    const auto less = [](const QPointF& left, const QPointF& right) {
        return left.x() < right.x() || (left.x() == right.x() && left.y() < right.y());
    };
    std::sort(points.begin(), points.end(), less);
    points.erase(std::unique(points.begin(), points.end()), points.end());
    QVector<QPointF> hull;
    for (const QPointF& point : points) {
        while (hull.size() >= 2 && cross(hull.last() - hull.at(hull.size() - 2), point - hull.last()) <= 0.0) hull.removeLast();
        hull.append(point);
    }
    const int lower = hull.size();
    for (int i = points.size() - 2; i >= 0; --i) {
        const QPointF point = points.at(i);
        while (hull.size() > lower && cross(hull.last() - hull.at(hull.size() - 2), point - hull.last()) <= 0.0) hull.removeLast();
        hull.append(point);
    }
    hull.removeLast();
    return hull;
}

QString buildSetbackDomain(Domain* domain)
{
    if (domain->setback <= 0.0) return QString();
    QPainterPath freePath = ringPath(domain->polygon);
    QPainterPath buffers;
    buffers.setFillRule(Qt::WindingFill);
    QVector<QVector<QPointF>> originalRings;
    originalRings.append(domain->polygon);
    originalRings += domain->holes;
    // An axis-aligned square contains the Euclidean radius disk. Edge-square
    // Minkowski hulls therefore conservatively remove at least the requested
    // distance. The small construction guard is never subtracted at validation.
    const double radius = domain->setback + 16.0 * Epsilon;
    for (const QVector<QPointF>& ring : originalRings) {
        for (int i = 0; i < ring.size(); ++i) {
            buffers = buffers.united(ringPath(squareEdgeHull(ring.at(i), ring.at((i + 1) % ring.size()), radius)));
            if (!boundedPath(buffers)) return QStringLiteral("coverage_setback_boolean_budget_exceeded");
        }
    }
    for (const QVector<QPointF>& hole : domain->holes) {
        freePath = freePath.subtracted(ringPath(hole));
        if (!boundedPath(freePath)) return QStringLiteral("coverage_setback_boolean_budget_exceeded");
    }
    freePath = freePath.subtracted(buffers).simplified();
    if (!boundedPath(freePath)) return QStringLiteral("coverage_setback_boolean_budget_exceeded");
    // toFillPolygon(s) introduces rewind bridges between rings. Keep each
    // actual subpath instead: holes and disconnected free components survive.
    const QList<QPolygonF> polygons = freePath.toSubpathPolygons();
    if (polygons.isEmpty()) return QStringLiteral("coverage_setback_no_remaining_area");
    if (polygons.size() > MaxSetbackContours) return QStringLiteral("coverage_setback_contour_limit_exceeded");
    int vertices = 0;
    QVector<double> areas;
    for (const QPolygonF& polygon : polygons) {
        QVector<QPointF> ring;
        for (const QPointF& point : polygon) {
            if (ring.isEmpty() || length(point - ring.last()) > Epsilon) ring.append(point);
        }
        if (ring.size() > 1 && length(ring.first() - ring.last()) <= Epsilon) ring.removeLast();
        vertices += ring.size();
        if (vertices > MaxSetbackVertices) return QStringLiteral("coverage_setback_vertex_limit_exceeded");
        double twiceArea = 0.0;
        if (ring.size() < 3 || !validateRing(&ring, &twiceArea).isEmpty()) return QStringLiteral("coverage_setback_topology_ambiguous");
        for (const QPointF& point : ring) {
            if (!allowedPoint(domain->polygon, domain->holes, point, false)
                || !setbackClearance(*domain, point, point)) return QStringLiteral("coverage_setback_numeric_conflict");
        }
        domain->setbackContours.append(ring);
        areas.append(std::abs(twiceArea) * 0.5);
    }
    for (int i = 0; i < domain->setbackContours.size(); ++i) {
        int depth = 0;
        for (int j = 0; j < domain->setbackContours.size(); ++j) {
            if (i == j) continue;
            if (ringsTouch(domain->setbackContours.at(i), domain->setbackContours.at(j))) return QStringLiteral("coverage_setback_topology_ambiguous");
            if (strictlyInside(domain->setbackContours.at(j), domain->setbackContours.at(i).first())) ++depth;
        }
        if (depth % 2 == 0) domain->componentContours.append(i);
        domain->setbackArea += depth % 2 == 0 ? areas.at(i) : -areas.at(i);
    }
    if (domain->componentContours.isEmpty() || !qIsFinite(domain->setbackArea) || domain->setbackArea <= Epsilon) {
        return QStringLiteral("coverage_setback_no_remaining_area");
    }
    return QString();
}

int setbackComponent(const Domain& domain, const QPointF& point)
{
    bool contained = false, boundary = false;
    for (const QVector<QPointF>& ring : domain.setbackContours) {
        for (int i = 0; i < ring.size(); ++i) {
            if (onSegment(point, ring.at(i), ring.at((i + 1) % ring.size()))) boundary = true;
        }
        if (strictlyInside(ring, point)) contained = !contained;
    }
    if (!boundary && !contained) return -1;
    int found = -1;
    for (int i = 0; i < domain.componentContours.size(); ++i) {
        if (!inside(domain.setbackContours.at(domain.componentContours.at(i)), point)) continue;
        // Prefer the deepest containing even contour if nested components ever
        // occur. Full candidate parity above excludes expanded-hole interiors;
        // original-domain and requested distance checks are still independent.
        if (found < 0 || strictlyInside(domain.setbackContours.at(domain.componentContours.at(found)),
                                       domain.setbackContours.at(domain.componentContours.at(i)).first())) found = i;
    }
    return found;
}

bool projectPoint(const QPointF& point, bool geodetic, const QPointF& origin,
                  const QGeoCoordinate& geoOrigin, QPointF* projected)
{
    if ((geodetic && (std::abs(point.y()) > 80.0 || std::abs(point.x()) > 180.0
                     || std::abs(point.x() - origin.x()) >= 180.0))
        || (!geodetic && (std::abs(point.x()) > 1e7 || std::abs(point.y()) > 1e7))) return false;
    QPointF local = point - origin;
    if (geodetic) {
        const QGeoCoordinate geo(point.y(), point.x());
        const double distance = geoOrigin.distanceTo(geo);
        const double azimuth = qDegreesToRadians(geoOrigin.azimuthTo(geo));
        local = QPointF(distance * std::sin(azimuth), distance * std::cos(azimuth));
    }
    if (!qIsFinite(local.x()) || !qIsFinite(local.y())
        || length(local) > (geodetic ? 5000.0 : 10000.0)) return false;
    *projected = local;
    return true;
}

bool segmentInside(const QVector<QPointF>& polygon, const QVector<QVector<QPointF>>& holes,
                   const QPointF& a, const QPointF& b, bool allowOutside, int* checks,
                   const Domain* domain = nullptr)
{
    if (++(*checks) > MaxVisibilityChecks) return false;
    if (domain && domain->setback > 0.0) {
        allowOutside = false;
        if (!setbackClearance(*domain, a, b)) return false;
    }
    if (!allowedPoint(polygon, holes, a, allowOutside)
        || !allowedPoint(polygon, holes, b, allowOutside)) return false;
    const QPointF direction = b - a;
    const double squared = dot(direction, direction);
    if (squared <= Epsilon * Epsilon) return true;
    QVector<double> cuts;
    cuts << 0.0 << 1.0;
    QVector<QVector<QPointF>> rings;
    if (!allowOutside) rings.append(polygon);
    rings += holes;
    for (const QVector<QPointF>& ring : rings) {
        for (int i = 0; i < ring.size(); ++i) {
            const QPointF c = ring.at(i);
            const QPointF d = ring.at((i + 1) % ring.size());
            const QPointF edge = d - c;
            const double denominator = cross(direction, edge);
            if (std::abs(denominator) > 1e-12 * qMax(1.0, length(direction) * length(edge))) {
                const double t = cross(c - a, edge) / denominator;
                const double u = cross(c - a, direction) / denominator;
                if (t > 0.0 && t < 1.0 && u >= -1e-9 && u <= 1.0 + 1e-9) cuts.append(t);
            } else if (std::abs(cross(direction, c - a)) <= Epsilon * qMax(1.0, length(direction))) {
                for (const QPointF& end : {c, d}) {
                    const double t = dot(end - a, direction) / squared;
                    if (t > 0.0 && t < 1.0) cuts.append(t);
                }
            }
        }
    }
    std::sort(cuts.begin(), cuts.end());
    for (int i = 1; i < cuts.size(); ++i) {
        if (cuts.at(i) - cuts.at(i - 1) > 1e-10
            && !allowedPoint(polygon, holes,
                             a + direction * ((cuts.at(i) + cuts.at(i - 1)) * 0.5), allowOutside)) return false;
    }
    return true;
}

// Visibility graph connectors avoid a direct shortcut through a concave
// polygon's exterior. Boundary-following proposals still require clearance
// review; a mathematical polygon is not an obstacle/terrain model.
bool connect(const QVector<QPointF>& polygon, const QVector<QVector<QPointF>>& holes,
             const QPointF& start, const QPointF& end, bool allowOutside,
             QVector<QVector<double>>* graph, int* checks, QVector<QPointF>* route,
             const Domain* domain = nullptr)
{
    if (segmentInside(polygon, holes, start, end, allowOutside, checks, domain)) {
        route->append(end);
        return true;
    }
    if (*checks > MaxVisibilityChecks) return false;
    QVector<QPointF> nodes;
    if (domain && domain->setback > 0.0) {
        for (const QVector<QPointF>& ring : domain->setbackContours) nodes += ring;
    } else {
        nodes = polygon;
        for (const QVector<QPointF>& hole : holes) nodes += hole;
    }
    const int n = nodes.size();
    const double infinity = std::numeric_limits<double>::infinity();
    if (graph->isEmpty()) {
        *graph = QVector<QVector<double>>(n, QVector<double>(n, infinity));
        for (int i = 0; i < n; ++i) {
            (*graph)[i][i] = 0.0;
            for (int j = i + 1; j < n; ++j) {
                if (segmentInside(polygon, holes, nodes.at(i), nodes.at(j), allowOutside, checks, domain)) {
                    (*graph)[i][j] = (*graph)[j][i] = length(nodes.at(i) - nodes.at(j));
                }
                if (*checks > MaxVisibilityChecks) return false;
            }
        }
    }
    QVector<double> fromStart(n, infinity), toEnd(n, infinity);
    for (int i = 0; i < n; ++i) {
        if (segmentInside(polygon, holes, start, nodes.at(i), allowOutside, checks, domain)) fromStart[i] = length(start - nodes.at(i));
        if (segmentInside(polygon, holes, nodes.at(i), end, allowOutside, checks, domain)) toEnd[i] = length(end - nodes.at(i));
        if (*checks > MaxVisibilityChecks) return false;
    }
    QVector<double> distances = fromStart;
    QVector<int> previous(n, -1);
    QVector<bool> visited(n, false);
    for (int step = 0; step < n; ++step) {
        int nearest = -1;
        for (int i = 0; i < n; ++i) {
            if (!visited.at(i) && (nearest < 0 || distances.at(i) < distances.at(nearest))) nearest = i;
        }
        if (nearest < 0 || !qIsFinite(distances.at(nearest))) break;
        visited[nearest] = true;
        for (int i = 0; i < n; ++i) {
            const double candidate = distances.at(nearest) + graph->at(nearest).at(i);
            if (!visited.at(i) && candidate < distances.at(i)) {
                distances[i] = candidate;
                previous[i] = nearest;
            }
        }
    }
    int best = -1;
    double bestDistance = infinity;
    for (int i = 0; i < n; ++i) {
        if (distances.at(i) + toEnd.at(i) < bestDistance) {
            best = i;
            bestDistance = distances.at(i) + toEnd.at(i);
        }
    }
    if (best < 0) return false;
    QVector<QPointF> backwards;
    for (int i = best; i >= 0; i = previous.at(i)) backwards.append(nodes.at(i));
    for (int i = backwards.size() - 1; i >= 0; --i) {
        if (route->isEmpty() || length(route->last() - backwards.at(i)) > Epsilon) route->append(backwards.at(i));
    }
    if (route->isEmpty() || length(route->last() - end) > Epsilon) route->append(end);
    return true;
}

QVariantMap failure(const QString& code)
{
    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("inputValid"), false);
    result.insert(QStringLiteral("generated"), false);
    result.insert(QStringLiteral("errors"), QStringList() << code);
    result.insert(QStringLiteral("assignments"), QVariantList());
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    return result;
}

bool compatibleDeclaration(const QVariantMap& map, bool geodetic, const QString& frameId)
{
    if (map.contains(QStringLiteral("coordinateSystem"))) {
        const QVariant value = map.value(QStringLiteral("coordinateSystem"));
        if (value.type() != QVariant::String) return false;
        const QString declared = value.toString();
        if (geodetic ? declared != QStringLiteral("geodetic")
                     : declared != QStringLiteral("localENU") && declared != QStringLiteral("local")) return false;
    }
    if (map.contains(QStringLiteral("localFrameId"))) {
        const QVariant value = map.value(QStringLiteral("localFrameId"));
        if (geodetic || value.type() != QVariant::String || value.toString() != frameId) return false;
    }
    return true;
}

QString misplacedExclusions(const QVariant& area)
{
    const auto checkVertices = [](const QVariant& value) {
        if (value.type() == QVariant::Map) {
            const QVariantMap map = value.toMap();
            return map.contains(QStringLiteral("holes")) || map.contains(QStringLiteral("polygons"))
                || map.contains(QStringLiteral("boundarySetbackMeters"))
                ? QStringLiteral("coverage_misplaced_exclusions") : QString();
        }
        if (value.type() != QVariant::List) return QString();
        const QVariantList vertices = value.toList();
        // Fixed-depth, linear inspection of already-owned, implicitly shared
        // values before the legacy no-op; no recursive tree copy or coverage-
        // specific vertex cap is imposed on ordinary explicit-route metadata.
        // The caller's original snapshot budget still applies. readDomain /
        // parseArea independently enforce 64/128 for actual hole geometry.
        for (const QVariant& point : vertices) {
            if (point.type() != QVariant::Map) continue;
            const QVariantMap map = point.toMap();
            if (map.contains(QStringLiteral("holes")) || map.contains(QStringLiteral("polygons"))
                || map.contains(QStringLiteral("boundarySetbackMeters"))) {
                return QStringLiteral("coverage_misplaced_exclusions");
            }
        }
        return QString();
    };
    if (area.type() == QVariant::List) return checkVertices(area);
    if (area.type() != QVariant::Map || area.isNull()) return QString();
    const QVariantMap source = area.toMap();
    if (source.contains(QStringLiteral("polygons"))) return QStringLiteral("coverage_misplaced_exclusions");
    const QVariant bounds = source.value(QStringLiteral("bounds"));
    if (bounds.type() == QVariant::Map && (bounds.toMap().contains(QStringLiteral("holes"))
        || bounds.toMap().contains(QStringLiteral("polygons"))
        || bounds.toMap().contains(QStringLiteral("boundarySetbackMeters")))) return QStringLiteral("coverage_misplaced_exclusions");
    for (const QString& key : {QStringLiteral("vertices"), QStringLiteral("points"),
                              QStringLiteral("boundary"), QStringLiteral("coordinates"), QStringLiteral("polygon")}) {
        if (!source.contains(key)) continue;
        const QString error = checkVertices(source.value(key));
        if (!error.isEmpty()) return error;
    }
    return QString();
}

bool parseArea(const QVariant& area, bool geodetic, const QString& frameId, QVector<QPointF>* raw)
{
    QVariant vertices = area;
    if (area.type() == QVariant::Map) {
        const QVariantMap map = area.toMap();
        if (!compatibleDeclaration(map, geodetic, frameId)) return false;
        if (map.contains(QStringLiteral("polygons"))) return false;
        bool hasVertices = false;
        for (const QString& key : {QStringLiteral("vertices"), QStringLiteral("points"),
                                  QStringLiteral("boundary"), QStringLiteral("coordinates"), QStringLiteral("polygon")}) {
            if (!map.contains(key)) continue;
            // Reject duplicate shape aliases before recursively comparing
            // untrusted QVariant trees. Exactly one boundary is required.
            if (hasVertices) return false;
            vertices = map.value(key);
            hasVertices = true;
        }
        if (hasVertices && map.contains(QStringLiteral("bounds"))) return false;
        if (!hasVertices) {
            const QVariant boundsValue = map.contains(QStringLiteral("bounds")) ? map.value(QStringLiteral("bounds")) : area;
            if (boundsValue.type() != QVariant::Map) return false;
            const QVariantMap bounds = boundsValue.toMap();
            // Exclusions are read once from the outer area only. A nested
            // placement must not silently disappear behind the bounds alias.
            if (map.contains(QStringLiteral("bounds"))
                && (bounds.contains(QStringLiteral("holes")) || bounds.contains(QStringLiteral("polygons"))
                    || bounds.contains(QStringLiteral("boundarySetbackMeters")))) return false;
            if (!compatibleDeclaration(bounds, geodetic, frameId)) return false;
            for (const QString& key : geodetic
                    ? QStringList() << QStringLiteral("minX") << QStringLiteral("maxX") << QStringLiteral("minY") << QStringLiteral("maxY")
                    : QStringList() << QStringLiteral("west") << QStringLiteral("east") << QStringLiteral("south") << QStringLiteral("north")) {
                if (bounds.contains(key)) return false;
            }
            double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
            if (!number(bounds.value(geodetic ? QStringLiteral("west") : QStringLiteral("minX")), &left)
                || !number(bounds.value(geodetic ? QStringLiteral("east") : QStringLiteral("maxX")), &right)
                || !number(bounds.value(geodetic ? QStringLiteral("south") : QStringLiteral("minY")), &bottom)
                || !number(bounds.value(geodetic ? QStringLiteral("north") : QStringLiteral("maxY")), &top)
                || left >= right || bottom >= top) return false;
            *raw = QVector<QPointF>() << QPointF(left, bottom) << QPointF(right, bottom)
                                      << QPointF(right, top) << QPointF(left, top);
            return true;
        }
    }
    if (vertices.type() != QVariant::List) return false;
    const QVariantList values = vertices.toList();
    if (values.size() < 3 || values.size() > MaxVertices + 1) return false;
    for (const QVariant& value : values) {
        if (value.type() != QVariant::Map) return false;
        const QVariantMap point = value.toMap();
        if (point.contains(QStringLiteral("holes")) || point.contains(QStringLiteral("polygons"))
            || point.contains(QStringLiteral("boundarySetbackMeters"))) return false;
        if (!compatibleDeclaration(point, geodetic, frameId)) return false;
        double x = 0.0, y = 0.0;
        const bool hasGeo = point.contains(QStringLiteral("lat")) || point.contains(QStringLiteral("latitude"))
            || point.contains(QStringLiteral("lon")) || point.contains(QStringLiteral("longitude")) || point.contains(QStringLiteral("lng"));
        const bool hasLocal = point.contains(QStringLiteral("x")) || point.contains(QStringLiteral("east"))
            || point.contains(QStringLiteral("y")) || point.contains(QStringLiteral("north"));
        if ((geodetic && hasLocal) || (!geodetic && hasGeo)) return false;
        if (!coordinate(point, geodetic ? QStringList() << QStringLiteral("longitude") << QStringLiteral("lon") << QStringLiteral("lng")
                                       : QStringList() << QStringLiteral("x") << QStringLiteral("east"), &x)
            || !coordinate(point, geodetic ? QStringList() << QStringLiteral("latitude") << QStringLiteral("lat")
                                          : QStringList() << QStringLiteral("y") << QStringLiteral("north"), &y)) return false;
        raw->append(QPointF(x, y));
    }
    if (raw->size() > 3 && raw->first() == raw->last()) raw->removeLast();
    return raw->size() >= 3 && raw->size() <= MaxVertices;
}

QString readDomain(const QVariant& area, bool geodetic, const QString& frameId, Domain* domain,
                   bool buildCandidates = true)
{
    const QString misplaced = misplacedExclusions(area);
    if (!misplaced.isEmpty()) return misplaced;
    const QString setbackError = readSetback(area, &domain->setback);
    if (!setbackError.isEmpty()) return setbackError;
    QVector<QPointF> raw;
    if (!parseArea(area, geodetic, frameId, &raw)) return QStringLiteral("coverage_invalid_area");
    QVector<QVector<QPointF>> rawHoles;
    domain->vertexCount = raw.size();
    if (area.type() == QVariant::Map && area.toMap().contains(QStringLiteral("holes"))) {
        const QVariant suppliedHoles = area.toMap().value(QStringLiteral("holes"));
        if (suppliedHoles.type() != QVariant::List || suppliedHoles.isNull()
            || suppliedHoles.toList().size() > MaxHoles) return QStringLiteral("coverage_invalid_holes");
        for (const QVariant& value : suppliedHoles.toList()) {
            QVector<QPointF> ring;
            if (value.type() != QVariant::List || value.isNull()
                || !parseArea(value, geodetic, frameId, &ring)) return QStringLiteral("coverage_invalid_hole");
            domain->vertexCount += ring.size();
            if (domain->vertexCount > MaxDomainVertices) return QStringLiteral("coverage_domain_vertex_limit_exceeded");
            rawHoles.append(ring);
        }
    }
    for (const QPointF& point : raw) {
        if ((geodetic && (std::abs(point.y()) > 80.0 || std::abs(point.x()) > 180.0))
            || (!geodetic && (std::abs(point.x()) > 1e7 || std::abs(point.y()) > 1e7))) return QStringLiteral("coverage_area_out_of_range");
    }
    const auto less = [](const QPointF& a, const QPointF& b) { return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y()); };
    domain->origin = *std::min_element(raw.constBegin(), raw.constEnd(), less);
    domain->geoOrigin = QGeoCoordinate(domain->origin.y(), domain->origin.x());
    for (const QPointF& point : raw) {
        if (geodetic && std::abs(point.x() - domain->origin.x()) >= 180.0) return QStringLiteral("coverage_antimeridian_unsupported");
        QPointF projected;
        if (!projectPoint(point, geodetic, domain->origin, domain->geoOrigin, &projected)) return QStringLiteral("coverage_area_extent_exceeded");
        domain->polygon.append(projected);
        domain->originalVertices.append(QVariantMap{
            {geodetic ? QStringLiteral("longitude") : QStringLiteral("x"), point.x()},
            {geodetic ? QStringLiteral("latitude") : QStringLiteral("y"), point.y()}});
    }
    for (const QVector<QPointF>& rawHole : rawHoles) {
        QVector<QPointF> ring;
        QVariantList canonicalRing;
        QVariantList originalRing;
        for (const QPointF& point : rawHole) {
            QPointF projected;
            if (!projectPoint(point, geodetic, domain->origin, domain->geoOrigin, &projected)) return QStringLiteral("coverage_hole_out_of_range");
            ring.append(projected);
            QVariantMap canonicalPoint{{QStringLiteral("coordinateSystem"), geodetic ? QStringLiteral("geodetic") : QStringLiteral("localENU")},
                {geodetic ? QStringLiteral("longitude") : QStringLiteral("x"), point.x()},
                {geodetic ? QStringLiteral("latitude") : QStringLiteral("y"), point.y()}};
            if (!geodetic) canonicalPoint.insert(QStringLiteral("localFrameId"), frameId);
            canonicalRing.append(canonicalPoint);
            originalRing.append(QVariantMap{
                {geodetic ? QStringLiteral("longitude") : QStringLiteral("x"), point.x()},
                {geodetic ? QStringLiteral("latitude") : QStringLiteral("y"), point.y()}});
        }
        domain->holes.append(ring);
        // QVariantList::append(list) concatenates; preserve each complete ring
        // as one nested QVariant list instead of flattening the exclusions.
        domain->canonicalHoles.append(QVariant(canonicalRing));
        domain->originalHoles.append(QVariant(originalRing));
    }
    const QString outerError = validateRing(&domain->polygon, &domain->twiceArea);
    if (!outerError.isEmpty()) {
        return outerError == QStringLiteral("overlapping_edges") ? QStringLiteral("coverage_overlapping_polygon_edges")
            : outerError == QStringLiteral("self_intersecting") ? QStringLiteral("coverage_self_intersecting_polygon")
                                                              : QStringLiteral("coverage_degenerate_polygon");
    }
    for (int i = 0; i < domain->holes.size(); ++i) {
        double holeArea = 0.0;
        if (!validateRing(&domain->holes[i], &holeArea).isEmpty()) return QStringLiteral("coverage_invalid_hole_geometry");
        if (ringsTouch(domain->polygon, domain->holes.at(i))) return QStringLiteral("coverage_hole_touches_boundary");
        for (const QPointF& point : domain->holes.at(i)) {
            if (!strictlyInside(domain->polygon, point)) return QStringLiteral("coverage_hole_outside_area");
        }
        for (int j = 0; j < i; ++j) {
            if (ringsTouch(domain->holes.at(i), domain->holes.at(j))
                || inside(domain->holes.at(i), domain->holes.at(j).first())
                || inside(domain->holes.at(j), domain->holes.at(i).first())) return QStringLiteral("coverage_holes_overlap_or_nest");
        }
        domain->removedArea += std::abs(holeArea) * 0.5;
    }
    if (!domain->holes.isEmpty() && domain->removedArea >= std::abs(domain->twiceArea) * 0.5 - Epsilon) {
        return QStringLiteral("coverage_no_remaining_area");
    }
    return buildCandidates ? buildSetbackDomain(domain) : QString();
}

bool compatibleRoute(const QVariant& source, const QVariantMap& member, bool geodetic,
                     const QString& altitudeReference, const QString& frameId)
{
    if (source.type() != QVariant::List || source.toList().isEmpty()) return false;
    for (const QVariant& value : source.toList()) {
        if (value.type() != QVariant::Map) return false;
        const QVariantMap point = value.toMap();
        const QVariant reference = point.contains(QStringLiteral("altitudeReference"))
            ? point.value(QStringLiteral("altitudeReference")) : member.value(QStringLiteral("altitudeReference"));
        if (reference.type() != QVariant::String || reference.toString() != altitudeReference) return false;
        if (!geodetic) {
            const QVariant localFrame = point.contains(QStringLiteral("localFrameId"))
                ? point.value(QStringLiteral("localFrameId")) : member.value(QStringLiteral("localFrameId"));
            if (localFrame.type() != QVariant::String || localFrame.toString() != frameId) return false;
        } else if (point.contains(QStringLiteral("localFrameId"))) {
            return false;
        }
        const QString coordinateSystem = point.value(QStringLiteral("coordinateSystem")).toString();
        if (point.value(QStringLiteral("coordinateSystem")).type() != QVariant::String
            || (geodetic ? coordinateSystem != QStringLiteral("geodetic")
                         : coordinateSystem != QStringLiteral("local") && coordinateSystem != QStringLiteral("localENU"))) return false;
    }
    return true;
}

bool routeAvoidsHoles(const QVariantList& route, const QVector<QPointF>& polygon,
                     const QVector<QVector<QPointF>>& holes, bool geodetic,
                     const QPointF& origin, const QGeoCoordinate& geoOrigin, int* checks,
                     const Domain* domain = nullptr)
{
    QPointF previous;
    for (int i = 0; i < route.size(); ++i) {
        if (route.at(i).type() != QVariant::Map || route.at(i).isNull()) return false;
        const QVariantMap point = route.at(i).toMap();
        double x = 0.0, y = 0.0;
        const bool hasGeo = point.contains(QStringLiteral("lat")) || point.contains(QStringLiteral("latitude"))
            || point.contains(QStringLiteral("lon")) || point.contains(QStringLiteral("longitude")) || point.contains(QStringLiteral("lng"));
        const bool hasLocal = point.contains(QStringLiteral("x")) || point.contains(QStringLiteral("east"))
            || point.contains(QStringLiteral("y")) || point.contains(QStringLiteral("north"));
        if ((geodetic && hasLocal) || (!geodetic && hasGeo)
            || !coordinate(point, geodetic ? QStringList() << QStringLiteral("longitude") << QStringLiteral("lon") << QStringLiteral("lng")
                                           : QStringList() << QStringLiteral("x") << QStringLiteral("east"), &x)
            || !coordinate(point, geodetic ? QStringList() << QStringLiteral("latitude") << QStringLiteral("lat")
                                           : QStringList() << QStringLiteral("y") << QStringLiteral("north"), &y)) return false;
        QPointF projected;
        if (!projectPoint(QPointF(x, y), geodetic, origin, geoOrigin, &projected)
            || !allowedPoint(polygon, holes, projected, true)) return false;
        // Supplied auxiliary and return routes may lie outside the survey
        // boundary, but may not shortcut through a declared exclusion. This
        // check never rewrites an operator's route or drops its metadata.
        if ((i > 0 || (domain && domain->setback > 0.0))
            && !segmentInside(polygon, holes, i > 0 ? previous : projected, projected, true, checks, domain)) return false;
        previous = projected;
    }
    return true;
}

bool returnBridgeAvoidsHoles(const QVariantMap& member, const Domain& domain,
                            bool geodetic, int* checks)
{
    if (!member.contains(QStringLiteral("waypoints")) || !member.contains(QStringLiteral("returnPath"))) return true;
    const QVariantList outbound = member.value(QStringLiteral("waypoints")).toList();
    const QVariantList returning = member.value(QStringLiteral("returnPath")).toList();
    if (outbound.isEmpty() || returning.isEmpty()) return false;
    // The payload concatenates these paths: the edge between them is real
    // nominal geometry too, even when neither path separately crosses a hole.
    const QVariantList bridge{outbound.last(), returning.first()};
    return routeAvoidsHoles(bridge, domain.polygon, domain.holes, geodetic,
                           domain.origin, domain.geoOrigin, checks, &domain);
}

bool endpoint(const QVariant& source, bool geodetic, const QString& altitudeReference,
              const QString& frameId, const QPointF& origin, const QGeoCoordinate& geoOrigin,
              RoutePoint* result)
{
    if (source.type() != QVariant::Map || source.isNull()) return false;
    const QVariantMap point = source.toMap();
    const QSet<QString> allowed = QSet<QString>() << QStringLiteral("coordinateSystem")
        << QStringLiteral("altitudeReference") << QStringLiteral("localFrameId")
        << QStringLiteral("latitude") << QStringLiteral("lat") << QStringLiteral("longitude")
        << QStringLiteral("lon") << QStringLiteral("lng") << QStringLiteral("x")
        << QStringLiteral("east") << QStringLiteral("y") << QStringLiteral("north")
        << QStringLiteral("altitude") << QStringLiteral("alt") << QStringLiteral("z") << QStringLiteral("height");
    if (point.size() > allowed.size()) return false;
    for (auto it = point.constBegin(); it != point.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return false;
    }
    const QVariant system = point.value(QStringLiteral("coordinateSystem"));
    const QVariant reference = point.value(QStringLiteral("altitudeReference"));
    if (system.type() != QVariant::String
        || (geodetic ? system.toString() != QStringLiteral("geodetic")
                     : system.toString() != QStringLiteral("local") && system.toString() != QStringLiteral("localENU"))
        || reference.type() != QVariant::String || reference.toString() != altitudeReference
        || !compatibleDeclaration(point, geodetic, frameId)) return false;
    if (!geodetic && (point.value(QStringLiteral("localFrameId")).type() != QVariant::String
        || point.value(QStringLiteral("localFrameId")).toString() != frameId)) return false;
    for (const QString& key : geodetic
            ? QStringList() << QStringLiteral("x") << QStringLiteral("east") << QStringLiteral("y") << QStringLiteral("north")
            : QStringList() << QStringLiteral("latitude") << QStringLiteral("lat") << QStringLiteral("longitude")
                            << QStringLiteral("lon") << QStringLiteral("lng")) {
        if (point.contains(key)) return false;
    }
    double x = 0.0, y = 0.0, height = 0.0;
    if (!coordinate(point, geodetic ? QStringList() << QStringLiteral("longitude") << QStringLiteral("lon") << QStringLiteral("lng")
                                   : QStringList() << QStringLiteral("x") << QStringLiteral("east"), &x)
        || !coordinate(point, geodetic ? QStringList() << QStringLiteral("latitude") << QStringLiteral("lat")
                                      : QStringList() << QStringLiteral("y") << QStringLiteral("north"), &y)
        || !coordinate(point, QStringList() << QStringLiteral("altitude") << QStringLiteral("alt")
                                            << QStringLiteral("z") << QStringLiteral("height"), &height)
        || height < -1000.0 || height > 20000.0) return false;
    QPointF local(x - origin.x(), y - origin.y());
    if (geodetic) {
        if (std::abs(y) > 80.0 || std::abs(x) > 180.0 || std::abs(x - origin.x()) >= 180.0) return false;
        const QGeoCoordinate geo(y, x);
        const double distance = geoOrigin.distanceTo(geo);
        const double azimuth = qDegreesToRadians(geoOrigin.azimuthTo(geo));
        local = QPointF(distance * std::sin(azimuth), distance * std::cos(azimuth));
    } else if (std::abs(x) > 1e7 || std::abs(y) > 1e7) {
        return false;
    }
    if (!qIsFinite(local.x()) || !qIsFinite(local.y())
        || length(local) > (geodetic ? 5000.0 : 10000.0)) return false;
    result->position = local;
    result->altitude = height;
    result->suppliedCoordinates.insert(geodetic ? QStringLiteral("longitude") : QStringLiteral("x"), x);
    result->suppliedCoordinates.insert(geodetic ? QStringLiteral("latitude") : QStringLiteral("y"), y);
    return true;
}

void appendPoint(QVector<RoutePoint>* route, const RoutePoint& point)
{
    if (route->isEmpty() || length(route->last().position - point.position) > Epsilon
        || route->last().altitude != point.altitude) {
        route->append(point);
    } else if (!point.suppliedCoordinates.isEmpty()) {
        route->last().suppliedCoordinates = point.suppliedCoordinates;
    }
}

QVariantMap section(const QString& path, const QString& kind, const QString& phase,
                    int first, int last)
{
    QVariantMap result;
    result.insert(QStringLiteral("path"), path);
    result.insert(QStringLiteral("kind"), kind);
    result.insert(QStringLiteral("phase"), phase);
    result.insert(QStringLiteral("startIndex"), first);
    result.insert(QStringLiteral("endIndex"), last);
    return result;
}

QVariantList routeWaypoints(const QVector<RoutePoint>& route, bool geodetic,
                           const QString& altitudeReference, const QString& frameId,
                           const QPointF& origin, const QGeoCoordinate& geoOrigin,
                           double speed, double climbSpeed, double* distance, double* duration)
{
    QVariantList waypoints;
    *distance = 0.0;
    *duration = 0.0;
    double horizontalDistance = 0.0, verticalDistance = 0.0;
    for (int i = 0; i < route.size(); ++i) {
        if (i > 0) {
            const double horizontal = length(route.at(i).position - route.at(i - 1).position);
            const double vertical = std::abs(route.at(i).altitude - route.at(i - 1).altitude);
            *distance += std::hypot(horizontal, vertical);
            horizontalDistance += horizontal;
            verticalDistance += vertical;
            // All generated vertical portions are sequential, not diagonal.
            // climbSpeed has been validated before any altitude change.
            *duration = horizontalDistance / speed;
            if (verticalDistance > 0.0) *duration += verticalDistance / climbSpeed;
        }
        QVariantMap waypoint;
        waypoint.insert(QStringLiteral("sequence"), i);
        waypoint.insert(QStringLiteral("coordinateSystem"), geodetic ? QStringLiteral("geodetic") : QStringLiteral("local"));
        if (geodetic) {
            double azimuth = qRadiansToDegrees(std::atan2(route.at(i).position.x(), route.at(i).position.y()));
            if (azimuth < 0.0) azimuth += 360.0;
            const QGeoCoordinate geo = geoOrigin.atDistanceAndAzimuth(length(route.at(i).position), azimuth);
            waypoint.insert(QStringLiteral("latitude"), geo.latitude());
            waypoint.insert(QStringLiteral("longitude"), geo.longitude());
        } else {
            waypoint.insert(QStringLiteral("x"), route.at(i).position.x() + origin.x());
            waypoint.insert(QStringLiteral("y"), route.at(i).position.y() + origin.y());
            waypoint.insert(QStringLiteral("localFrameId"), frameId);
        }
        for (auto it = route.at(i).suppliedCoordinates.constBegin(); it != route.at(i).suppliedCoordinates.constEnd(); ++it) {
            waypoint.insert(it.key(), it.value());
        }
        waypoint.insert(QStringLiteral("altitude"), route.at(i).altitude);
        waypoint.insert(QStringLiteral("altitudeReference"), altitudeReference);
        waypoint.insert(QStringLiteral("estimatedElapsedSeconds"), *duration);
        waypoints.append(waypoint);
    }
    return waypoints;
}

} // namespace

QVariantMap FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(const QVariant& area,
                                                               const QVariantList& members)
{
    const auto result = [](bool ok, const QString& reason, const QVariantMap& policy = QVariantMap()) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
            {QStringLiteral("policy"), policy}};
    };
    const QString misplaced = misplacedExclusions(area);
    if (!misplaced.isEmpty()) return result(false, misplaced);
    double setback = 0.0;
    const QString setbackError = readSetback(area, &setback);
    if (!setbackError.isEmpty()) return result(false, setbackError);
    const QVariantMap source = area.type() == QVariant::Map ? area.toMap() : QVariantMap();
    int holeCount = 0;
    if (source.contains(QStringLiteral("holes"))) {
        const QVariant holes = source.value(QStringLiteral("holes"));
        if (holes.type() != QVariant::List || holes.isNull() || holes.toList().size() > MaxHoles) {
            return result(false, QStringLiteral("coverage_invalid_holes"));
        }
        holeCount = holes.toList().size();
    }
    QVariantMap policy{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("active"), false},
        {QStringLiteral("area"), QVariantMap()}};
    // Match the existing legacy no-op exactly: ordinary area metadata is not
    // promoted into a new outer-boundary constraint or a 64-vertex limit.
    if (holeCount == 0 && setback == 0.0) return result(true, QStringLiteral(""), policy);

    QString frame, frameId;
    if (source.contains(QStringLiteral("coordinateSystem"))) {
        if (source.value(QStringLiteral("coordinateSystem")).type() != QVariant::String) {
            return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
        }
        frame = source.value(QStringLiteral("coordinateSystem")).toString();
        if (frame == QStringLiteral("local")) frame = QStringLiteral("localENU");
        if (frame != QStringLiteral("geodetic") && frame != QStringLiteral("localENU")) {
            return result(false, QStringLiteral("coverage_exclusion_frame_mismatch"));
        }
        if (frame == QStringLiteral("localENU")) {
            const QVariant local = source.value(QStringLiteral("localFrameId"));
            if (source.contains(QStringLiteral("localFrameId"))
                && (local.type() != QVariant::String || local.isNull() || local.toString().isEmpty()
                    || local.toString().size() > 128 || local.toString() != local.toString().trimmed())) {
                return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
            }
            if (source.contains(QStringLiteral("localFrameId"))) frameId = local.toString();
        }
    }
    if (frame.isEmpty() || (frame == QStringLiteral("localENU") && frameId.isEmpty())) {
        // An omitted area declaration can only use actual explicit route
        // frames, never infer geodetic/local from numeric coordinate ranges.
        if (members.isEmpty() || members.size() > 255) {
            return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
        }
        int pointCount = 0;
        for (const QVariant& memberValue : members) {
            if (memberValue.type() != QVariant::Map || memberValue.isNull()) {
                return result(false, QStringLiteral("coverage_exclusion_members_invalid"));
            }
            const QVariantMap member = memberValue.toMap();
            for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
                if (!member.contains(key)) continue;
                const QVariant route = member.value(key);
                if (route.type() != QVariant::List || route.isNull()) {
                    return result(false, QStringLiteral("coverage_exclusion_route_missing"));
                }
                for (const QVariant& pointValue : route.toList()) {
                    if (++pointCount > MaxRoutePoints || pointValue.type() != QVariant::Map || pointValue.isNull()) {
                        return result(false, QStringLiteral("coverage_exclusion_route_invalid"));
                    }
                    const QVariantMap point = pointValue.toMap();
                    const QVariant declared = point.value(QStringLiteral("coordinateSystem"));
                    if (declared.type() != QVariant::String) {
                        return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
                    }
                    QString resolved = declared.toString();
                    if (resolved == QStringLiteral("local")) resolved = QStringLiteral("localENU");
                    if (resolved != QStringLiteral("geodetic") && resolved != QStringLiteral("localENU")) {
                        return result(false, QStringLiteral("coverage_exclusion_frame_mismatch"));
                    }
                    QString localFrame;
                    if (resolved == QStringLiteral("localENU")) {
                        const QVariant local = point.contains(QStringLiteral("localFrameId"))
                            ? point.value(QStringLiteral("localFrameId")) : member.value(QStringLiteral("localFrameId"));
                        if (local.type() != QVariant::String || local.isNull() || local.toString().isEmpty()
                            || local.toString().size() > 128 || local.toString() != local.toString().trimmed()) {
                            return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
                        }
                        localFrame = local.toString();
                    }
                    const bool geodetic = resolved == QStringLiteral("geodetic");
                    if (!compatibleDeclaration(member, geodetic, localFrame)
                        || !compatibleDeclaration(point, geodetic, localFrame)
                        || (!frame.isEmpty() && (frame != resolved || (!frameId.isEmpty() && frameId != localFrame)))) {
                        return result(false, QStringLiteral("coverage_exclusion_frame_mismatch"));
                    }
                    frame = resolved;
                    frameId = localFrame;
                }
            }
        }
    }
    if (frame.isEmpty()) return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
    if (QString::fromUtf8(frameId.toUtf8()) != frameId) {
        return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
    }
    for (const QChar ch : frameId) {
        if (ch.isNull() || ch.category() == QChar::Other_Control) {
            return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
        }
    }
    Domain domain;
    const QString domainError = readDomain(area, frame == QStringLiteral("geodetic"), frameId, &domain, false);
    if (!domainError.isEmpty()) return result(false, domainError);
    QVariantMap canonicalArea{{QStringLiteral("coordinateSystem"), frame},
        {QStringLiteral("vertices"), domain.originalVertices}, {QStringLiteral("holes"), domain.originalHoles},
        {QStringLiteral("boundarySetbackMeters"), domain.setback}};
    if (frame == QStringLiteral("localENU")) canonicalArea.insert(QStringLiteral("localFrameId"), frameId);
    policy.insert(QStringLiteral("active"), true);
    policy.insert(QStringLiteral("area"), canonicalArea);
    return result(true, QStringLiteral(""), policy);
}

QVariantMap FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(const QVariantMap& policy)
{
    const auto reject = [] {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("route-boundary-policy-invalid")},
            {QStringLiteral("policy"), QVariantMap()}};
    };
    double schema = 0.0;
    if (policy.size() != 3 || !number(policy.value(QStringLiteral("schemaVersion")), &schema) || schema != 1.0
        || policy.value(QStringLiteral("active")).type() != QVariant::Bool || policy.value(QStringLiteral("active")).isNull()
        || policy.value(QStringLiteral("area")).type() != QVariant::Map || policy.value(QStringLiteral("area")).isNull()) return reject();
    const QVariantMap area = policy.value(QStringLiteral("area")).toMap();
    if (!policy.value(QStringLiteral("active")).toBool()) {
        if (!area.isEmpty()) return reject();
    } else {
        const bool local = area.value(QStringLiteral("coordinateSystem")) == QVariant(QStringLiteral("localENU"));
        const QSet<QString> keys = local
            ? QSet<QString>{QStringLiteral("coordinateSystem"), QStringLiteral("vertices"), QStringLiteral("holes"),
                QStringLiteral("boundarySetbackMeters"), QStringLiteral("localFrameId")}
            : QSet<QString>{QStringLiteral("coordinateSystem"), QStringLiteral("vertices"), QStringLiteral("holes"),
                QStringLiteral("boundarySetbackMeters")};
        if (area.size() != keys.size()) return reject();
        for (auto it = area.constBegin(); it != area.constEnd(); ++it) if (!keys.contains(it.key())) return reject();
        // Bound every canonical leaf before JSON equality. parseArea accepts
        // ordinary source metadata, but a retained policy cannot attach an
        // arbitrary unknown QVariant tree to an otherwise valid vertex and
        // force that tree to be serialized before being rejected.
        int vertices = 0;
        const auto canonicalRing = [local, &vertices](const QVariant& value) {
            if (value.type() != QVariant::List || value.isNull()) return false;
            const QVariantList ring = value.toList();
            if (ring.size() < 3 || ring.size() > MaxVertices
                || ring.size() > MaxDomainVertices - vertices) return false;
            vertices += ring.size();
            const QString x = local ? QStringLiteral("x") : QStringLiteral("longitude");
            const QString y = local ? QStringLiteral("y") : QStringLiteral("latitude");
            for (const QVariant& pointValue : ring) {
                if (pointValue.type() != QVariant::Map || pointValue.isNull()) return false;
                const QVariantMap point = pointValue.toMap();
                double coordinateValue = 0.0;
                if (point.size() != 2 || !number(point.value(x), &coordinateValue)
                    || !number(point.value(y), &coordinateValue)) return false;
            }
            return true;
        };
        if (!canonicalRing(area.value(QStringLiteral("vertices")))) return reject();
        const QVariant holesValue = area.value(QStringLiteral("holes"));
        if (holesValue.type() != QVariant::List || holesValue.isNull() || holesValue.toList().size() > MaxHoles) return reject();
        for (const QVariant& ring : holesValue.toList()) if (!canonicalRing(ring)) return reject();
    }
    const QVariantMap rebuilt = buildRouteBoundaryPolicy(area, QVariantList());
    if (rebuilt.value(QStringLiteral("ok")).type() != QVariant::Bool || !rebuilt.value(QStringLiteral("ok")).toBool()
        || QJsonDocument::fromVariant(policy).toJson(QJsonDocument::Compact)
            != QJsonDocument::fromVariant(rebuilt.value(QStringLiteral("policy"))).toJson(QJsonDocument::Compact)) return reject();
    return rebuilt;
}

QVariantMap FleetAreaCoveragePlanner::validateBoundaryPolicyRoutes(const QVariantMap& policy,
                                                                   const QVariantList& members)
{
    const QVariantMap validated = validateRouteBoundaryPolicy(policy);
    if (!validated.value(QStringLiteral("ok")).toBool()) return validated;
    const QVariantMap routes = validateExclusionRoutes(policy.value(QStringLiteral("area")), members);
    if (routes.value(QStringLiteral("ok")).type() != QVariant::Bool || !routes.value(QStringLiteral("ok")).toBool()) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), routes.value(QStringLiteral("reason"))},
            {QStringLiteral("policy"), QVariantMap()}};
    }
    return validated;
}

QVariantMap FleetAreaCoveragePlanner::validateExclusionRoutes(const QVariant& area,
                                                              const QVariantList& members)
{
    bool provided = false;
    int holeCount = 0;
    const auto result = [&](bool ok, const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
            {QStringLiteral("holesProvided"), provided}, {QStringLiteral("holeCount"), holeCount},
            {QStringLiteral("flightCommandReleased"), false}};
    };
    const QString misplaced = misplacedExclusions(area);
    if (!misplaced.isEmpty()) {
        provided = true;
        return result(false, misplaced);
    }
    if (area.type() != QVariant::Map || area.isNull()) return result(true, QStringLiteral(""));
    const QVariantMap source = area.toMap();
    double setback = 0.0;
    const QString setbackError = readSetback(area, &setback);
    if (!setbackError.isEmpty()) return result(false, setbackError);
    if (source.contains(QStringLiteral("holes"))) {
        const QVariant holes = source.value(QStringLiteral("holes"));
        provided = true;
        if (holes.type() != QVariant::List || holes.isNull() || holes.toList().size() > MaxHoles) {
            return result(false, QStringLiteral("coverage_invalid_holes"));
        }
        holeCount = holes.toList().size();
        provided = holeCount > 0;
    }
    if (!provided && setback == 0.0) return result(true, QStringLiteral(""));
    if (members.isEmpty() || members.size() > 255) return result(false, QStringLiteral("coverage_exclusion_members_invalid"));
    QString frame, frameId;
    int pointCount = 0;
    for (const QVariant& value : members) {
        if (value.type() != QVariant::Map || value.isNull()) return result(false, QStringLiteral("coverage_exclusion_members_invalid"));
        const QVariantMap member = value.toMap();
        if (!member.contains(QStringLiteral("waypoints"))) return result(false, QStringLiteral("coverage_exclusion_route_missing"));
        for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            if (!member.contains(key)) continue;
            const QVariant route = member.value(key);
            if (route.type() != QVariant::List || route.isNull() || route.toList().isEmpty()) return result(false, QStringLiteral("coverage_exclusion_route_invalid"));
            if (route.toList().size() > MaxRoutePoints - pointCount) return result(false, QStringLiteral("coverage_exclusion_route_limit_exceeded"));
            pointCount += route.toList().size();
            for (const QVariant& pointValue : route.toList()) {
                if (pointValue.type() != QVariant::Map || pointValue.isNull()) return result(false, QStringLiteral("coverage_exclusion_point_invalid"));
                const QVariantMap point = pointValue.toMap();
                const QVariant declaration = point.value(QStringLiteral("coordinateSystem"));
                if (declaration.type() != QVariant::String || declaration.isNull()) return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
                const QString declared = declaration.toString();
                const QString resolved = declared == QStringLiteral("local") ? QStringLiteral("localENU") : declared;
                if (resolved != QStringLiteral("localENU") && resolved != QStringLiteral("geodetic")) return result(false, QStringLiteral("coverage_exclusion_frame_invalid"));
                const bool geodetic = resolved == QStringLiteral("geodetic");
                QString localFrame;
                if (!geodetic) {
                    const QVariant local = point.contains(QStringLiteral("localFrameId"))
                        ? point.value(QStringLiteral("localFrameId")) : member.value(QStringLiteral("localFrameId"));
                    if (local.type() != QVariant::String || local.isNull() || local.toString().isEmpty()
                        || local.toString().size() > 128 || local.toString() != local.toString().trimmed()) return result(false, QStringLiteral("coverage_exclusion_frame_missing"));
                    localFrame = local.toString();
                }
                if (!compatibleDeclaration(member, geodetic, localFrame)
                    || !compatibleDeclaration(point, geodetic, localFrame)
                    || (!frame.isEmpty() && (frame != resolved || frameId != localFrame))) {
                    return result(false, QStringLiteral("coverage_exclusion_frame_mismatch"));
                }
                frame = resolved;
                frameId = localFrame;
            }
        }
    }
    if (frame.isEmpty()) return result(false, QStringLiteral("coverage_exclusion_route_missing"));
    const bool geodetic = frame == QStringLiteral("geodetic");
    Domain domain;
    // Existing explicit geometry needs no candidate search. Validate directly
    // against original rings and distance; conservative square topology/limits
    // must not reject an otherwise representable explicit Euclidean route.
    const QString error = readDomain(area, geodetic, frameId, &domain, false);
    if (!error.isEmpty()) return result(false, error);
    int checks = 0;
    for (const QVariant& value : members) {
        const QVariantMap member = value.toMap();
        for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            if (!member.contains(key)) continue;
            if (!routeAvoidsHoles(member.value(key).toList(), domain.polygon, domain.holes,
                                 geodetic, domain.origin, domain.geoOrigin, &checks, &domain)) {
                return result(false, checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                    : QStringLiteral("coverage_supplied_route_exclusion_conflict"));
            }
        }
        if (!returnBridgeAvoidsHoles(member, domain, geodetic, &checks)) {
            return result(false, checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                : QStringLiteral("coverage_return_bridge_exclusion_conflict"));
        }
    }
    return result(true, QStringLiteral(""));
}

QVariantMap FleetAreaCoveragePlanner::generate(const QVariant& area, const QVariantMap& options,
                                               const QVariantList& members)
{
    const QSet<QString> allowed = QSet<QString>() << QStringLiteral("strategy") << QStringLiteral("coordinateSystem")
        << QStringLiteral("localFrameId") << QStringLiteral("altitudeReference") << QStringLiteral("altitudeMeters")
        << QStringLiteral("spacingMeters") << QStringLiteral("cruiseSpeedMetersPerSecond")
        << QStringLiteral("headingDegrees") << QStringLiteral("vehicleIds")
        << QStringLiteral("vehicleEndpoints") << QStringLiteral("climbSpeedMetersPerSecond");
    for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return failure(QStringLiteral("coverage_unknown_option"));
    }
    for (const QString& key : {QStringLiteral("strategy"), QStringLiteral("coordinateSystem"),
                              QStringLiteral("localFrameId"), QStringLiteral("altitudeReference")}) {
        if (options.contains(key) && (options.value(key).type() != QVariant::String
            || options.value(key).toString().size() > 128)) return failure(QStringLiteral("coverage_invalid_frame"));
    }
    if (options.value(QStringLiteral("strategy"), QStringLiteral("lawnmower")).toString() != QStringLiteral("lawnmower")) {
        return failure(QStringLiteral("coverage_unsupported_strategy"));
    }
    const QString frame = options.value(QStringLiteral("coordinateSystem")).toString();
    const bool geodetic = frame == QStringLiteral("geodetic");
    const QString altitudeReference = options.value(QStringLiteral("altitudeReference")).toString();
    const QString frameId = options.value(QStringLiteral("localFrameId")).toString().trimmed();
    if ((!geodetic && frame != QStringLiteral("localENU"))
        || (geodetic && altitudeReference != QStringLiteral("amsl"))
        || (!geodetic && (frameId.isEmpty() || altitudeReference != QStringLiteral("local_origin")))
        || (geodetic && options.contains(QStringLiteral("localFrameId")))) return failure(QStringLiteral("coverage_invalid_frame"));
    double spacing = 0.0, speed = 0.0, altitude = 0.0, heading = 0.0, climbSpeed = 0.0;
    if (!number(options.value(QStringLiteral("spacingMeters")), &spacing) || spacing < 0.25 || spacing > 500.0
        || !number(options.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &speed) || speed < 0.1 || speed > 300.0
        || !number(options.value(QStringLiteral("altitudeMeters")), &altitude) || altitude < -1000.0 || altitude > 20000.0
        || (options.contains(QStringLiteral("headingDegrees"))
            && (!number(options.value(QStringLiteral("headingDegrees")), &heading) || heading < 0.0 || heading >= 360.0))) {
        return failure(QStringLiteral("coverage_invalid_geometry_options"));
    }
    const bool endpointsRequested = options.contains(QStringLiteral("vehicleEndpoints"));
    if (options.contains(QStringLiteral("climbSpeedMetersPerSecond"))
        && (!number(options.value(QStringLiteral("climbSpeedMetersPerSecond")), &climbSpeed)
            || climbSpeed < 0.1 || climbSpeed > 20.0)) return failure(QStringLiteral("coverage_invalid_climb_speed"));
    if (endpointsRequested && (options.value(QStringLiteral("vehicleEndpoints")).type() != QVariant::Map
        || options.value(QStringLiteral("vehicleEndpoints")).isNull())) return failure(QStringLiteral("coverage_invalid_endpoints"));
    if (members.isEmpty() || members.size() > 255) return failure(QStringLiteral("coverage_invalid_members"));
    QSet<int> selected;
    if (options.contains(QStringLiteral("vehicleIds"))) {
        if (options.value(QStringLiteral("vehicleIds")).type() != QVariant::List) return failure(QStringLiteral("coverage_invalid_vehicle_ids"));
        const QVariantList ids = options.value(QStringLiteral("vehicleIds")).toList();
        if (ids.isEmpty() || ids.size() > 255) return failure(QStringLiteral("coverage_invalid_vehicle_ids"));
        for (const QVariant& id : ids) {
            double value = 0.0;
            if (!number(id, &value) || value < 1.0 || value > 255.0 || std::floor(value) != value
                || selected.contains(static_cast<int>(value))) return failure(QStringLiteral("coverage_invalid_vehicle_ids"));
            selected.insert(static_cast<int>(value));
        }
    }
    QSet<int> seen;
    QVector<int> participants;
    int suppliedPointCount = 0;
    for (int index = 0; index < members.size(); ++index) {
        if (members.at(index).type() != QVariant::Map) return failure(QStringLiteral("coverage_invalid_members"));
        const QVariantMap member = members.at(index).toMap();
        if (!compatibleDeclaration(member, geodetic, frameId)
            || (member.contains(QStringLiteral("altitudeReference"))
                && (member.value(QStringLiteral("altitudeReference")).type() != QVariant::String
                    || member.value(QStringLiteral("altitudeReference")).toString() != altitudeReference))) {
            return failure(QStringLiteral("coverage_member_frame_mismatch"));
        }
        double idNumber = 0.0;
        if (!number(member.value(QStringLiteral("vehicleId")), &idNumber) || idNumber < 1.0 || idNumber > 255.0
            || std::floor(idNumber) != idNumber || seen.contains(static_cast<int>(idNumber))
            || member.value(QStringLiteral("role")).type() != QVariant::String) return failure(QStringLiteral("coverage_invalid_members"));
        const int id = static_cast<int>(idNumber);
        seen.insert(id);
        const QString role = member.value(QStringLiteral("role")).toString();
        if (role != QStringLiteral("leader") && role != QStringLiteral("scout")
            && role != QStringLiteral("relay") && role != QStringLiteral("responder")) {
            return failure(QStringLiteral("coverage_invalid_members"));
        }
        const bool participant = options.contains(QStringLiteral("vehicleIds")) ? selected.contains(id) : role == QStringLiteral("scout");
        if (participant) {
            if (role != QStringLiteral("scout") && role != QStringLiteral("leader")) return failure(QStringLiteral("coverage_unsupported_participant_role"));
            if (member.contains(QStringLiteral("waypoints")) || member.contains(QStringLiteral("route"))) return failure(QStringLiteral("coverage_conflicting_member_route"));
            participants.append(index);
        } else {
            const QVariant route = member.value(QStringLiteral("waypoints"));
            if (route.type() != QVariant::List || route.toList().isEmpty()) return failure(QStringLiteral("coverage_support_route_missing"));
            if (route.toList().size() > MaxRoutePoints - suppliedPointCount) return failure(QStringLiteral("coverage_auxiliary_route_limit_exceeded"));
            suppliedPointCount += route.toList().size();
            if (!compatibleRoute(route, member, geodetic, altitudeReference, frameId)) return failure(QStringLiteral("coverage_support_frame_mismatch"));
        }
        if (member.contains(QStringLiteral("returnPath"))) {
            if (participant && endpointsRequested) return failure(QStringLiteral("coverage_endpoint_return_conflict"));
            const QVariant route = member.value(QStringLiteral("returnPath"));
            if (route.toList().size() > MaxRoutePoints - suppliedPointCount) return failure(QStringLiteral("coverage_auxiliary_route_limit_exceeded"));
            suppliedPointCount += route.toList().size();
            if (!compatibleRoute(route, member, geodetic, altitudeReference, frameId)) return failure(QStringLiteral("coverage_return_frame_mismatch"));
        }
    }
    if (!options.contains(QStringLiteral("vehicleIds"))) {
        for (int index : participants) selected.insert(members.at(index).toMap().value(QStringLiteral("vehicleId")).toInt());
    }
    QSet<int> missing = selected;
    missing.subtract(seen);
    if (participants.isEmpty() || !missing.isEmpty()) return failure(QStringLiteral("coverage_participant_missing"));
    std::sort(participants.begin(), participants.end(), [&members](int a, int b) {
        return members.at(a).toMap().value(QStringLiteral("vehicleId")).toInt()
            < members.at(b).toMap().value(QStringLiteral("vehicleId")).toInt();
    });
    Domain domain;
    const QString domainError = readDomain(area, geodetic, frameId, &domain);
    if (!domainError.isEmpty()) return failure(domainError);
    const QPointF& origin = domain.origin;
    const QGeoCoordinate& geoOrigin = domain.geoOrigin;
    const QVector<QPointF>& polygon = domain.polygon;
    const QVector<QVector<QPointF>>& holes = domain.holes;
    QMap<int, Endpoints> endpoints;
    if (endpointsRequested) {
        const QVariantMap values = options.value(QStringLiteral("vehicleEndpoints")).toMap();
        if (values.size() != participants.size()) return failure(QStringLiteral("coverage_endpoint_participants_mismatch"));
        for (int index : participants) {
            const int id = members.at(index).toMap().value(QStringLiteral("vehicleId")).toInt();
            // Exactly the canonical decimal ID key is accepted. Equal map and
            // participant counts plus unique IDs also exclude unknown keys.
            const QString key = QString::number(id);
            if (!values.contains(key) || values.value(key).type() != QVariant::Map || values.value(key).isNull()) {
                return failure(QStringLiteral("coverage_endpoint_participants_mismatch"));
            }
            const QVariantMap value = values.value(key).toMap();
            if (value.size() != 2 || !value.contains(QStringLiteral("start")) || !value.contains(QStringLiteral("returnTo"))) {
                return failure(QStringLiteral("coverage_invalid_endpoints"));
            }
            Endpoints parsed;
            if (!endpoint(value.value(QStringLiteral("start")), geodetic, altitudeReference, frameId, origin, geoOrigin, &parsed.start)
                || !endpoint(value.value(QStringLiteral("returnTo")), geodetic, altitudeReference, frameId, origin, geoOrigin, &parsed.returnTo)) {
                return failure(QStringLiteral("coverage_invalid_endpoint_point"));
            }
            if ((parsed.start.altitude != altitude || parsed.returnTo.altitude != altitude) && climbSpeed == 0.0) {
                return failure(QStringLiteral("coverage_climb_speed_required"));
            }
            endpoints.insert(id, parsed);
        }
    }
    int checks = 0;
    if (!holes.isEmpty() || domain.setback > 0.0) {
        for (int index = 0; index < members.size(); ++index) {
            const QVariantMap member = members.at(index).toMap();
            for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
                if (!member.contains(key)) continue;
                if (!routeAvoidsHoles(member.value(key).toList(), polygon, holes, geodetic, origin, geoOrigin, &checks, &domain)) {
                    return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                                                               : QStringLiteral("coverage_supplied_route_exclusion_conflict"));
                }
            }
            if (!returnBridgeAvoidsHoles(member, domain, geodetic, &checks)) {
                return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                    : QStringLiteral("coverage_return_bridge_exclusion_conflict"));
            }
        }
        for (auto it = endpoints.constBegin(); it != endpoints.constEnd(); ++it) {
            if (!allowedPoint(polygon, holes, it.value().start.position, true)
                || !allowedPoint(polygon, holes, it.value().returnTo.position, true)
                || (domain.setback > 0.0
                    && (!segmentInside(polygon, holes, it.value().start.position, it.value().start.position, false, &checks, &domain)
                        || !segmentInside(polygon, holes, it.value().returnTo.position, it.value().returnTo.position, false, &checks, &domain)))) {
                return failure(QStringLiteral("coverage_endpoint_in_exclusion"));
            }
        }
    }
    const double radians = qDegreesToRadians(heading);
    const QPointF along(std::sin(radians), std::cos(radians));
    const QPointF across(std::cos(radians), -std::sin(radians));
    double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum;
    QVector<QVector<QPointF>> scanContours;
    if (domain.setback > 0.0) scanContours = domain.setbackContours;
    else { scanContours.append(polygon); scanContours += holes; }
    QVector<QVector<QPointF>> sweptRings;
    for (const QVector<QPointF>& ring : scanContours) {
        QVector<QPointF> projected;
        for (const QPointF& point : ring) {
            const QPointF value(dot(point, along), dot(point, across));
            projected.append(value);
            minimum = qMin(minimum, value.y()); maximum = qMax(maximum, value.y());
        }
        sweptRings.append(projected);
    }
    const double laneCountNumber = std::ceil((maximum - minimum) / spacing);
    if (!qIsFinite(laneCountNumber) || laneCountNumber < 1.0 || laneCountNumber > MaxLegs) return failure(QStringLiteral("coverage_lane_limit_exceeded"));
    const int laneCount = static_cast<int>(laneCountNumber);
    const double actualSpacing = (maximum - minimum) / laneCount;
    QVector<Leg> legs;
    for (int lane = 0; lane < laneCount; ++lane) {
        const double ordinate = minimum + (lane + 0.5) * actualSpacing;
        QVector<double> intersections;
        for (const QVector<QPointF>& ring : sweptRings) {
            for (int i = 0; i < ring.size(); ++i) {
                const QPointF a = ring.at(i), b = ring.at((i + 1) % ring.size());
                if ((a.y() <= ordinate && b.y() > ordinate) || (b.y() <= ordinate && a.y() > ordinate)) {
                    intersections.append(a.x() + (ordinate - a.y()) * (b.x() - a.x()) / (b.y() - a.y()));
                }
            }
        }
        std::sort(intersections.begin(), intersections.end());
        if (intersections.size() % 2 != 0) return failure(QStringLiteral("coverage_scanline_ambiguous"));
        QVector<Leg> laneLegs;
        for (int i = 0; i + 1 < intersections.size(); i += 2) {
            if (intersections.at(i + 1) - intersections.at(i) <= Epsilon) continue;
            const QPointF a = along * intersections.at(i) + across * ordinate;
            const QPointF b = along * intersections.at(i + 1) + across * ordinate;
            if ((!holes.isEmpty() || domain.setback > 0.0) && !segmentInside(polygon, holes, a, b, false, &checks, &domain)) {
                return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                                                           : QStringLiteral("coverage_scanline_exclusion_conflict"));
            }
            const int component = domain.setback > 0.0 ? setbackComponent(domain, (a + b) * 0.5) : 0;
            if (component < 0) return failure(QStringLiteral("coverage_setback_component_ambiguous"));
            laneLegs.append({a, b, lane, component});
        }
        if (lane % 2 != 0) {
            std::reverse(laneLegs.begin(), laneLegs.end());
            for (Leg& leg : laneLegs) std::swap(leg.start, leg.end);
        }
        legs += laneLegs;
        if (legs.size() > MaxLegs) return failure(QStringLiteral("coverage_leg_limit_exceeded"));
    }
    if (legs.size() < participants.size()) return failure(QStringLiteral("coverage_insufficient_legs"));
    QVector<int> participantLegCounts;
    QVariantList componentAssignments;
    if (domain.setback > 0.0) {
        const int componentCount = domain.componentContours.size();
        QVector<QVector<Leg>> componentLegs(componentCount);
        QVector<QVector<int>> componentMembers(componentCount);
        for (const Leg& leg : legs) componentLegs[leg.component].append(leg);
        for (const QVector<Leg>& group : componentLegs) {
            if (group.isEmpty()) return failure(QStringLiteral("coverage_setback_component_unsampled"));
        }
        if (participants.size() < componentCount) return failure(QStringLiteral("coverage_setback_components_need_more_vehicles"));
        if (endpointsRequested) {
            // Explicit per-ID endpoints constrain ownership. Do not first bind
            // sorted IDs to arbitrary components and reject an otherwise valid
            // task merely because its aircraft were ordered differently.
            for (int index : participants) {
                const int id = members.at(index).toMap().value(QStringLiteral("vehicleId")).toInt();
                const Endpoints pair = endpoints.value(id);
                const int component = setbackComponent(domain, pair.start.position);
                if (component < 0 || component != setbackComponent(domain, pair.returnTo.position)) {
                    return failure(QStringLiteral("coverage_setback_endpoint_component_conflict"));
                }
                componentMembers[component].append(index);
            }
        } else {
            for (int i = 0; i < componentCount; ++i) componentMembers[i].append(participants.at(i));
            for (int i = componentCount; i < participants.size(); ++i) {
                int best = -1;
                double load = -1.0;
                for (int c = 0; c < componentCount; ++c) {
                    if (componentMembers.at(c).size() >= componentLegs.at(c).size()) continue;
                    const double candidate = static_cast<double>(componentLegs.at(c).size()) / componentMembers.at(c).size();
                    if (candidate > load) { best = c; load = candidate; }
                }
                if (best < 0) return failure(QStringLiteral("coverage_insufficient_legs"));
                componentMembers[best].append(participants.at(i));
            }
        }
        QVector<Leg> orderedLegs;
        QVector<int> orderedParticipants;
        for (int c = 0; c < componentCount; ++c) {
            const QVector<int>& group = componentMembers.at(c);
            if (group.isEmpty()) return failure(QStringLiteral("coverage_setback_component_without_vehicle"));
            if (group.size() > componentLegs.at(c).size()) return failure(QStringLiteral("coverage_setback_component_insufficient_legs"));
            QVariantList ids;
            for (int p = 0; p < group.size(); ++p) {
                orderedParticipants.append(group.at(p));
                participantLegCounts.append(componentLegs.at(c).size() / group.size()
                    + (p < componentLegs.at(c).size() % group.size() ? 1 : 0));
                ids.append(members.at(group.at(p)).toMap().value(QStringLiteral("vehicleId")));
            }
            orderedLegs += componentLegs.at(c);
            componentAssignments.append(QVariantMap{{QStringLiteral("componentIndex"), c},
                {QStringLiteral("vehicleIds"), ids}, {QStringLiteral("legCount"), componentLegs.at(c).size()}});
        }
        legs = orderedLegs;
        participants = orderedParticipants;
    } else {
        for (int p = 0; p < participants.size(); ++p) {
            participantLegCounts.append(legs.size() / participants.size() + (p < legs.size() % participants.size() ? 1 : 0));
        }
    }
    QVariantList assignments = members;
    QVariantList participantIds;
    QVariantList ingressOutsideIds, returnOutsideIds;
    QVector<QVector<double>> graph;
    // Outside-area connectors deliberately use another graph: an edge valid
    // outside the boundary must not become a shortcut in a survey connector.
    QVector<QVector<double>> outsideGraph;
    int totalPoints = 0, storedPoints = suppliedPointCount, firstLeg = 0;
    double totalDistance = 0.0, maximumDuration = 0.0;
    double totalReturnDistance = 0.0, maximumReturnDuration = 0.0, maximumRoundTripDuration = 0.0;
    for (int p = 0; p < participants.size(); ++p) {
        // Contiguous disjoint survey-leg ownership, not shared global routes.
        // Balance by leg count; distance/energy optimization is a later layer.
        const int count = participantLegCounts.at(p);
        QVector<QPointF> route;
        QVariantList surveySections;
        route.append(legs.at(firstLeg).start);
        for (int i = firstLeg; i < firstLeg + count; ++i) {
            const Leg& leg = legs.at(i);
            if (length(route.last() - leg.start) > Epsilon) {
                const int before = route.size() - 1;
                if (!connect(polygon, holes, route.last(), leg.start, false, &graph, &checks, &route, &domain)) {
                    return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded") : QStringLiteral("coverage_connector_failed"));
                }
                if (endpointsRequested) {
                    QVariantMap connector = section(QStringLiteral("waypoints"), QStringLiteral("connector"),
                                                    QStringLiteral("survey"), before, route.size() - 1);
                    connector.insert(QStringLiteral("geometryEvidence"), domain.setback > 0.0
                        ? QStringLiteral("nominal-original-boundary-distance-checked") : holes.isEmpty()
                        ? QStringLiteral("within-area-polygon-only") : QStringLiteral("within-area-excluding-hole-interiors-only"));
                    surveySections.append(connector);
                }
            }
            const int before = route.size() - 1;
            if (length(route.last() - leg.end) > Epsilon) route.append(leg.end);
            if (endpointsRequested) {
                QVariantMap survey = section(QStringLiteral("waypoints"), QStringLiteral("survey"),
                                             QStringLiteral("survey"), before, route.size() - 1);
                survey.insert(QStringLiteral("coverageLegIndex"), i);
                survey.insert(QStringLiteral("laneIndex"), leg.lane);
                surveySections.append(survey);
            }
            if (route.size() + storedPoints > MaxRoutePoints) return failure(QStringLiteral("coverage_waypoint_limit_exceeded"));
        }
        QVector<RoutePoint> surveyRoute;
        for (const QPointF& point : route) surveyRoute.append({point, altitude, QVariantMap()});
        QVariantMap member = assignments.at(participants.at(p)).toMap();
        QVector<RoutePoint> outboundRoute = surveyRoute;
        QVector<RoutePoint> ingressRoute, returnRoute;
        QVariantList routeSections;
        double ingressDistance = 0.0, ingressDuration = 0.0, returnDistance = 0.0, returnDuration = 0.0;
        if (endpointsRequested) {
            const int id = member.value(QStringLiteral("vehicleId")).toInt();
            const Endpoints pair = endpoints.value(id);
            const bool ingressOutside = !inside(polygon, pair.start.position);
            const bool returnOutside = !inside(polygon, pair.returnTo.position);
            if (ingressOutside) ingressOutsideIds.append(id);
            if (returnOutside) returnOutsideIds.append(id);
            appendPoint(&ingressRoute, pair.start);
            RoutePoint ingressLevel = pair.start;
            ingressLevel.altitude = altitude;
            appendPoint(&ingressRoute, ingressLevel);
            if (ingressRoute.size() > 1) {
                QVariantMap vertical = section(QStringLiteral("waypoints"), QStringLiteral("ingress"),
                                               QStringLiteral("ingress"), 0, ingressRoute.size() - 1);
                vertical.insert(QStringLiteral("component"), QStringLiteral("vertical"));
                vertical.insert(QStringLiteral("geometryEvidence"), QStringLiteral("vertical-clearance-unverified"));
                routeSections.append(vertical);
            }
            const int ingressHorizontalStart = ingressRoute.size() - 1;
            QVector<QPointF> ingressConnector;
            if (length(pair.start.position - route.first()) > Epsilon) {
                if (ingressOutside && holes.isEmpty()) {
                    // Outside the supplied area there is no obstacle model.
                    // This is an unverified direct proposal, never a clearance
                    // claim or a permission to execute the connector.
                    ingressConnector.append(route.first());
                } else if (!connect(polygon, holes, pair.start.position, route.first(), ingressOutside,
                                    ingressOutside ? &outsideGraph : &graph, &checks, &ingressConnector, &domain)) {
                    return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded") : QStringLiteral("coverage_ingress_connector_failed"));
                }
            }
            for (const QPointF& point : ingressConnector) appendPoint(&ingressRoute, {point, altitude, QVariantMap()});
            QVariantMap ingressSection = section(QStringLiteral("waypoints"), QStringLiteral("ingress"),
                                                 QStringLiteral("ingress"), ingressHorizontalStart, ingressRoute.size() - 1);
            ingressSection.insert(QStringLiteral("component"), QStringLiteral("horizontal"));
            ingressSection.insert(QStringLiteral("geometryEvidence"), domain.setback > 0.0
                ? QStringLiteral("nominal-original-boundary-distance-checked") : holes.isEmpty()
                ? (ingressOutside ? QStringLiteral("outside-area-direct-unverified") : QStringLiteral("within-area-polygon-only"))
                : (ingressOutside ? QStringLiteral("outside-area-hole-interiors-avoided-no-clearance")
                                  : QStringLiteral("within-area-excluding-hole-interiors-only")));
            routeSections.append(ingressSection);
            outboundRoute = ingressRoute;
            const int surveyOffset = outboundRoute.size() - 1;
            for (int i = 1; i < surveyRoute.size(); ++i) appendPoint(&outboundRoute, surveyRoute.at(i));
            for (const QVariant& value : surveySections) {
                QVariantMap shifted = value.toMap();
                shifted.insert(QStringLiteral("startIndex"), shifted.value(QStringLiteral("startIndex")).toInt() + surveyOffset);
                shifted.insert(QStringLiteral("endIndex"), shifted.value(QStringLiteral("endIndex")).toInt() + surveyOffset);
                routeSections.append(shifted);
            }
            appendPoint(&returnRoute, surveyRoute.last());
            QVector<QPointF> returnConnector;
            if (length(route.last() - pair.returnTo.position) > Epsilon) {
                if (returnOutside && holes.isEmpty()) {
                    returnConnector.append(pair.returnTo.position);
                } else if (!connect(polygon, holes, route.last(), pair.returnTo.position, returnOutside,
                                    returnOutside ? &outsideGraph : &graph, &checks, &returnConnector, &domain)) {
                    return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded") : QStringLiteral("coverage_return_connector_failed"));
                }
            }
            for (const QPointF& point : returnConnector) appendPoint(&returnRoute, {point, altitude, QVariantMap()});
            RoutePoint returnLevel = pair.returnTo;
            returnLevel.altitude = altitude;
            appendPoint(&returnRoute, returnLevel);
            QVariantMap returnSection = section(QStringLiteral("returnPath"), QStringLiteral("return"),
                                                QStringLiteral("return"), 0, returnRoute.size() - 1);
            returnSection.insert(QStringLiteral("component"), QStringLiteral("horizontal"));
            returnSection.insert(QStringLiteral("geometryEvidence"), domain.setback > 0.0
                ? QStringLiteral("nominal-original-boundary-distance-checked") : holes.isEmpty()
                ? (returnOutside ? QStringLiteral("outside-area-direct-unverified") : QStringLiteral("within-area-polygon-only"))
                : (returnOutside ? QStringLiteral("outside-area-hole-interiors-avoided-no-clearance")
                                 : QStringLiteral("within-area-excluding-hole-interiors-only")));
            routeSections.append(returnSection);
            const int returnVerticalStart = returnRoute.size() - 1;
            appendPoint(&returnRoute, pair.returnTo);
            if (returnRoute.size() - 1 > returnVerticalStart) {
                QVariantMap vertical = section(QStringLiteral("returnPath"), QStringLiteral("return"),
                                               QStringLiteral("return"), returnVerticalStart, returnRoute.size() - 1);
                vertical.insert(QStringLiteral("component"), QStringLiteral("vertical"));
                vertical.insert(QStringLiteral("geometryEvidence"), QStringLiteral("vertical-clearance-unverified"));
                routeSections.append(vertical);
            }
            // The inspection views are bounded too, not just the outbound
            // canonical route. A rejected later participant returns no prefix.
            const int emitted = surveyRoute.size() + ingressRoute.size() + outboundRoute.size() + returnRoute.size();
            if (emitted > MaxRoutePoints - storedPoints) return failure(QStringLiteral("coverage_waypoint_limit_exceeded"));
            storedPoints += emitted;
            double surveyDistance = 0.0, surveyDuration = 0.0;
            member.insert(QStringLiteral("surveyWaypoints"), routeWaypoints(surveyRoute, geodetic, altitudeReference, frameId,
                          origin, geoOrigin, speed, climbSpeed, &surveyDistance, &surveyDuration));
            member.insert(QStringLiteral("ingressWaypoints"), routeWaypoints(ingressRoute, geodetic, altitudeReference, frameId,
                          origin, geoOrigin, speed, climbSpeed, &ingressDistance, &ingressDuration));
            member.insert(QStringLiteral("returnPath"), routeWaypoints(returnRoute, geodetic, altitudeReference, frameId,
                          origin, geoOrigin, speed, climbSpeed, &returnDistance, &returnDuration));
            member.insert(QStringLiteral("routeSections"), routeSections);
            member.insert(QStringLiteral("routeSectionIndexConvention"), QStringLiteral("zero-based-inclusive-shared-boundary"));
            member.insert(QStringLiteral("ingressGenerated"), true);
            member.insert(QStringLiteral("returnGenerated"), true);
            member.insert(QStringLiteral("ingressOutsideArea"), ingressOutside);
            member.insert(QStringLiteral("returnOutsideArea"), returnOutside);
            member.insert(QStringLiteral("endpointPathsClearanceVerified"), false);
            member.insert(QStringLiteral("launchLandingInferred"), false);
            member.insert(QStringLiteral("verticalSequencing"), QStringLiteral("ingress-vertical-before-horizontal-return-horizontal-before-vertical"));
            member.insert(QStringLiteral("ingressPathLengthMeters"), ingressDistance);
            member.insert(QStringLiteral("estimatedIngressDurationSeconds"), ingressDuration);
            member.insert(QStringLiteral("surveyPathLengthMeters"), surveyDistance);
            member.insert(QStringLiteral("estimatedSurveyDurationSeconds"), surveyDuration);
            member.insert(QStringLiteral("returnPathLengthMeters"), returnDistance);
            member.insert(QStringLiteral("estimatedReturnDurationSeconds"), returnDuration);
            member.insert(QStringLiteral("returnTimingEvidence"), QStringLiteral("constant-speed-estimate"));
            member.insert(QStringLiteral("returnRouteSource"), QStringLiteral("area-coverage-return-proposal"));
            member.insert(QStringLiteral("flightCommandReleased"), false);
        } else {
            storedPoints += outboundRoute.size();
        }
        double distance = 0.0, duration = 0.0;
        const QVariantList waypoints = routeWaypoints(outboundRoute, geodetic, altitudeReference, frameId,
                                                     origin, geoOrigin, speed, climbSpeed, &distance, &duration);
        member.insert(QStringLiteral("waypoints"), waypoints);
        if (domain.setback > 0.0) {
            // Validate the actual emitted coordinates too, including geodetic
            // inverse/projection round trips. No candidate-only proof escapes
            // through a generated path or its outbound/return join.
            for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
                if (!member.contains(key)) continue;
                if (!routeAvoidsHoles(member.value(key).toList(), polygon, holes, geodetic, origin, geoOrigin, &checks, &domain)) {
                    return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                        : QStringLiteral("coverage_generated_setback_conflict"));
                }
            }
        }
        if ((!holes.isEmpty() || domain.setback > 0.0) && !returnBridgeAvoidsHoles(member, domain, geodetic, &checks)) {
            return failure(checks > MaxVisibilityChecks ? QStringLiteral("coverage_routing_budget_exceeded")
                : QStringLiteral("coverage_return_bridge_exclusion_conflict"));
        }
        member.insert(QStringLiteral("waypointCount"), waypoints.size());
        member.insert(QStringLiteral("coverageLegStart"), firstLeg);
        member.insert(QStringLiteral("coverageLegCount"), count);
        member.insert(QStringLiteral("pathLengthMeters"), distance);
        member.insert(QStringLiteral("estimatedDurationSeconds"), duration);
        member.insert(QStringLiteral("timingEvidence"), QStringLiteral("constant-speed-estimate"));
        member.insert(QStringLiteral("routeSource"), QStringLiteral("area-coverage-proposal"));
        member.insert(QStringLiteral("coordinateSystem"), geodetic ? QStringLiteral("geodetic") : QStringLiteral("local"));
        member.insert(QStringLiteral("altitudeReference"), altitudeReference);
        if (!geodetic) member.insert(QStringLiteral("localFrameId"), frameId);
        member.insert(QStringLiteral("manualConfirmationRequired"), true);
        assignments[participants.at(p)] = member;
        participantIds.append(member.value(QStringLiteral("vehicleId")));
        totalPoints += outboundRoute.size() + returnRoute.size(); totalDistance += distance;
        totalReturnDistance += returnDistance;
        maximumDuration = qMax(maximumDuration, duration);
        maximumReturnDuration = qMax(maximumReturnDuration, returnDuration);
        maximumRoundTripDuration = qMax(maximumRoundTripDuration, duration + returnDuration);
        firstLeg += count;
    }
    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("plannerVersion"), domain.setback > 0.0 ? QStringLiteral("0.4.0") : !holes.isEmpty() ? QStringLiteral("0.3.0")
        : endpointsRequested ? QStringLiteral("0.2.0") : QStringLiteral("0.1.0"));
    result.insert(QStringLiteral("inputValid"), true);
    result.insert(QStringLiteral("generated"), true);
    result.insert(QStringLiteral("errors"), QStringList());
    result.insert(QStringLiteral("assignments"), assignments);
    result.insert(QStringLiteral("participantVehicleIds"), participantIds);
    result.insert(QStringLiteral("coordinateSystem"), frame);
    if (!geodetic) result.insert(QStringLiteral("localFrameId"), frameId);
    result.insert(QStringLiteral("altitudeReference"), altitudeReference);
    // Retain explicit, validated planning requirements in the returned plan
    // identity. Estimates cannot reconstruct an approved speed/climb intent.
    result.insert(QStringLiteral("altitudeMeters"), altitude);
    result.insert(QStringLiteral("cruiseSpeedMetersPerSecond"), speed);
    if (options.contains(QStringLiteral("climbSpeedMetersPerSecond"))) {
        result.insert(QStringLiteral("climbSpeedMetersPerSecond"), climbSpeed);
    }
    result.insert(QStringLiteral("headingDegrees"), heading);
    result.insert(QStringLiteral("spacingMeters"), spacing);
    result.insert(QStringLiteral("actualLaneSpacingMeters"), actualSpacing);
    result.insert(QStringLiteral("areaSquareMetersApproximate"), std::abs(domain.twiceArea) * 0.5 - domain.removedArea);
    if (!holes.isEmpty() || domain.setback > 0.0) {
        result.insert(QStringLiteral("holes"), domain.canonicalHoles);
        result.insert(QStringLiteral("holeCount"), holes.size());
        result.insert(QStringLiteral("domainVertexCount"), domain.vertexCount);
        result.insert(QStringLiteral("outerAreaSquareMetersApproximate"), std::abs(domain.twiceArea) * 0.5);
        result.insert(QStringLiteral("excludedAreaSquareMetersApproximate"), domain.removedArea);
        result.insert(QStringLiteral("holeBoundaryPolicy"), domain.setback > 0.0
            ? QStringLiteral("original-boundaries-full-segment-euclidean-distance-checked")
            : QStringLiteral("interior-excluded-boundary-may-be-followed-no-setback"));
        result.insert(QStringLiteral("boundarySetbackMeters"), domain.setback);
        result.insert(QStringLiteral("exclusionClearanceVerified"), false);
        result.insert(QStringLiteral("executable"), false);
    }
    if (domain.setback > 0.0) {
        int derivedVertices = 0;
        for (const QVector<QPointF>& ring : domain.setbackContours) derivedVertices += ring.size();
        result.insert(QStringLiteral("setbackCandidateModel"), QStringLiteral("axis-aligned-square-minkowski-edge-boolean"));
        result.insert(QStringLiteral("setbackConstructionGuardMeters"), 16.0 * Epsilon);
        result.insert(QStringLiteral("setbackContourCount"), domain.setbackContours.size());
        result.insert(QStringLiteral("setbackVertexCount"), derivedVertices);
        result.insert(QStringLiteral("setbackComponentCount"), domain.componentContours.size());
        result.insert(QStringLiteral("setbackComponentAssignments"), componentAssignments);
        result.insert(QStringLiteral("setbackCandidateAreaSquareMetersApproximate"), domain.setbackArea);
        result.insert(QStringLiteral("nominalBoundaryDistanceChecked"), true);
        result.insert(QStringLiteral("numericalErrorCertified"), false);
        result.insert(QStringLiteral("physicalClearanceVerified"), false);
    }
    result.insert(QStringLiteral("laneCount"), laneCount);
    result.insert(QStringLiteral("legCount"), legs.size());
    result.insert(QStringLiteral("waypointCount"), totalPoints);
    result.insert(QStringLiteral("pathLengthMetersTotal"), totalDistance);
    result.insert(QStringLiteral("estimatedDurationSecondsMax"), maximumDuration);
    if (endpointsRequested) {
        result.insert(QStringLiteral("ingressGenerated"), true);
        result.insert(QStringLiteral("returnGenerated"), true);
        result.insert(QStringLiteral("endpointPathsClearanceVerified"), false);
        result.insert(QStringLiteral("storedWaypointCount"), storedPoints);
        result.insert(QStringLiteral("returnPathLengthMetersTotal"), totalReturnDistance);
        result.insert(QStringLiteral("estimatedReturnDurationSecondsMax"), maximumReturnDuration);
        result.insert(QStringLiteral("estimatedRoundTripDurationSecondsMax"), maximumRoundTripDuration);
        result.insert(QStringLiteral("ingressOutsideAreaVehicleIds"), ingressOutsideIds);
        result.insert(QStringLiteral("returnOutsideAreaVehicleIds"), returnOutsideIds);
        result.insert(QStringLiteral("verticalSequencing"), QStringLiteral("ingress-vertical-before-horizontal-return-horizontal-before-vertical"));
        result.insert(QStringLiteral("launchLandingInferred"), false);
    }
    result.insert(QStringLiteral("timingEvidence"), QStringLiteral("constant-speed-estimate"));
    result.insert(QStringLiteral("coverageEvidence"), QStringLiteral("geometric-survey-legs-only"));
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("limitations"), QStringList()
        << QStringLiteral("Coverage requires a suitable sensor footprint; lane spacing alone proves no observation coverage.")
        << (domain.setback > 0.0
            ? QStringLiteral("Positive setback checks nominal 2D point/segment distances from all original boundaries in the bounded projection only. It certifies no numerical error bound, tracking envelope, vehicle radius, obstacle clearance, turn, terrain, wind or dynamics.")
            : QStringLiteral("Connectors may follow the polygon boundary. Obstacle clearance, turns, terrain, wind and dynamics are not modelled."))
        << (endpointsRequested
            ? QStringLiteral("Ingress/return connectors are proposals only; outside-area direct connectors and vertical portions have no obstacle or terrain clearance evidence. No launch or landing is inferred. Disjoint survey legs do not prove inter-vehicle separation.")
            : QStringLiteral("Ingress and return paths are not generated. Disjoint survey legs do not prove inter-vehicle separation."))
        << QStringLiteral("Distance/speed timing is estimated and must not be treated as authoritative 4D timing.")
        << QStringLiteral("Geodetic geometry uses a bounded local projection, not a certified GIS model."));
    if (domain.setback > 0.0) {
        QStringList limitations = result.value(QStringLiteral("limitations")).toStringList();
        limitations << QStringLiteral("Square edge buffers are conservative candidates, not exact Euclidean offsets. Every disconnected candidate component is retained and needs at least one participant and one scan leg; explicit per-ID start/return endpoints must belong to the same component. No largest-component filtering or outside-area escape is permitted. The requested margin is not automatically inferred from aircraft or sensor performance.");
        result.insert(QStringLiteral("limitations"), limitations);
    } else if (!holes.isEmpty()) {
        QStringList limitations = result.value(QStringLiteral("limitations")).toStringList();
        limitations << QStringLiteral("Declared holes exclude nominal 2D interiors only; routes may follow their boundaries. No safety setback, vehicle radius, tracking error, sensor coverage, obstacle clearance or execution permission is inferred. Outside-area connectors avoid the declared hole interiors only.");
        result.insert(QStringLiteral("limitations"), limitations);
    }
    return result;
}

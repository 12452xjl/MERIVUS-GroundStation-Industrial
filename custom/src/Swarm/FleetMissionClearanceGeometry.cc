#include "FleetMissionClearanceGeometry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Plane = Geometry::Plane;
using Bounds = Geometry::Bounds;
using Polytope = Geometry::Polytope;
using Budget = Geometry::Budget;
using Verdict = Geometry::Verdict;
constexpr qint64 CoordinateBound = 51000000;
constexpr qint64 NormalBound = 100000000;
constexpr qint64 LimitBound = Q_INT64_C(20000000000000000);
constexpr qint64 WorkBound = 2000000;
constexpr int FragmentBound = 2048;
constexpr int ExtraPlaneBound = 64;
constexpr int TotalPlaneBound = ExtraPlaneBound + 6;
constexpr std::size_t WordCount = 8;
using Magnitude = std::array<std::uint32_t, WordCount>;

struct Integer {
    Magnitude words{};
    bool negative = false;
};

bool supported()
{
    return std::numeric_limits<std::uint32_t>::digits == 32
        && std::numeric_limits<std::uint64_t>::digits == 64
        && std::numeric_limits<qint64>::digits == 63;
}

bool zero(const Magnitude& value)
{
    for (std::uint32_t word : value) if (word != 0) return false;
    return true;
}

int compare(const Magnitude& left, const Magnitude& right)
{
    for (std::size_t i = WordCount; i != 0; --i) {
        if (left[i - 1] < right[i - 1]) return -1;
        if (left[i - 1] > right[i - 1]) return 1;
    }
    return 0;
}

std::uint64_t magnitude(qint64 value)
{
    // Defined even for the most negative qint64; public bounds are smaller.
    return value < 0 ? std::uint64_t(-(value + 1)) + 1 : std::uint64_t(value);
}

Integer integer(qint64 value)
{
    Integer result;
    const std::uint64_t bits = magnitude(value);
    result.words[0] = std::uint32_t(bits);
    result.words[1] = std::uint32_t(bits >> 32);
    result.negative = value < 0 && bits != 0;
    return result;
}

void negate(Integer* value)
{
    if (!zero(value->words)) value->negative = !value->negative;
}

bool addWord(Magnitude* result, std::size_t index, std::uint32_t word)
{
    std::uint64_t carry = word;
    while (carry != 0) {
        if (index >= WordCount) return false;
        const std::uint64_t sum = std::uint64_t((*result)[index]) + carry;
        (*result)[index] = std::uint32_t(sum);
        carry = sum >> 32;
        ++index;
    }
    return true;
}

bool subtract(const Magnitude& greater, const Magnitude& lesser, Magnitude* result)
{
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < WordCount; ++i) {
        const std::uint64_t subtrahend = std::uint64_t(lesser[i]) + borrow;
        const std::uint64_t minuend = greater[i];
        if (minuend >= subtrahend) {
            (*result)[i] = std::uint32_t(minuend - subtrahend);
            borrow = 0;
        } else {
            (*result)[i] = std::uint32_t((UINT64_C(1) << 32) + minuend - subtrahend);
            borrow = 1;
        }
    }
    return borrow == 0;
}

bool add(const Integer& left, const Integer& right, Integer* output)
{
    Integer result;
    if (left.negative == right.negative) {
        result = left;
        for (std::size_t i = 0; i < WordCount; ++i)
            if (!addWord(&result.words, i, right.words[i])) return false;
    } else {
        const int ordering = compare(left.words, right.words);
        if (ordering >= 0) {
            result.negative = left.negative;
            if (!subtract(left.words, right.words, &result.words)) return false;
        } else {
            result.negative = right.negative;
            if (!subtract(right.words, left.words, &result.words)) return false;
        }
    }
    if (zero(result.words)) result.negative = false;
    *output = result;
    return true;
}

bool multiply(const Integer& value, qint64 factor, Integer* output)
{
    Integer result;
    const std::uint64_t absoluteFactor = magnitude(factor);
    const std::uint32_t parts[2] = {std::uint32_t(absoluteFactor), std::uint32_t(absoluteFactor >> 32)};
    for (std::size_t i = 0; i < WordCount; ++i) {
        for (std::size_t j = 0; j < 2; ++j) {
            // Each uint32 product fits uint64 exactly; carries are added
            // separately and every carry beyond the fixed capacity rejects.
            const std::uint64_t product = std::uint64_t(value.words[i]) * parts[j];
            if (!addWord(&result.words, i + j, std::uint32_t(product))
                || !addWord(&result.words, i + j + 1, std::uint32_t(product >> 32))) return false;
        }
    }
    result.negative = !zero(result.words) && (value.negative != (factor < 0));
    *output = result;
    return true;
}

bool determinant(const qint64 matrix[3][3], Integer* output)
{
    static const int columns[6][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1},
                                      {2, 1, 0}, {1, 0, 2}, {0, 2, 1}};
    Integer sum;
    for (int i = 0; i < 6; ++i) {
        Integer term = integer(matrix[0][columns[i][0]]);
        if (!multiply(term, matrix[1][columns[i][1]], &term)
            || !multiply(term, matrix[2][columns[i][2]], &term)) return false;
        if (i >= 3) negate(&term);
        if (!add(sum, term, &sum)) return false;
    }
    *output = sum;
    return true;
}

bool residual(const Plane& plane, const Integer& x, const Integer& y,
              const Integer& z, const Integer& denominator, int* sign)
{
    Integer sum, term;
    if (!multiply(x, plane.east, &sum) || !multiply(y, plane.north, &term)
        || !add(sum, term, &sum) || !multiply(z, plane.altitude, &term)
        || !add(sum, term, &sum) || !multiply(denominator, -plane.limit, &term)
        || !add(sum, term, &sum)) return false;
    *sign = zero(sum.words) ? 0 : (sum.negative ? -1 : 1);
    return true;
}

bool inRange(qint64 value, qint64 bound)
{
    return value >= -bound && value <= bound;
}

bool planeValid(const Plane& plane)
{
    return (plane.east != 0 || plane.north != 0 || plane.altitude != 0)
        && inRange(plane.east, NormalBound) && inRange(plane.north, NormalBound)
        && inRange(plane.altitude, NormalBound) && inRange(plane.limit, LimitBound);
}

bool polytopeValid(const Polytope& polytope)
{
    const Bounds& b = polytope.bounds;
    if (!inRange(b.minimumEast, CoordinateBound) || !inRange(b.maximumEast, CoordinateBound)
        || !inRange(b.minimumNorth, CoordinateBound) || !inRange(b.maximumNorth, CoordinateBound)
        || !inRange(b.minimumAltitude, CoordinateBound) || !inRange(b.maximumAltitude, CoordinateBound)
        || b.minimumEast > b.maximumEast || b.minimumNorth > b.maximumNorth
        || b.minimumAltitude > b.maximumAltitude || polytope.planes.size() > ExtraPlaneBound) return false;
    for (const Plane& plane : polytope.planes) if (!planeValid(plane)) return false;
    return true;
}

bool cancellationObserved(Budget* budget)
{
    if (!budget) return false;
    if (budget->cancelled) return true;
    if (budget->cancellation && budget->cancellation->load(std::memory_order_acquire))
        budget->cancelled = true;
    return budget->cancelled;
}

Verdict conclusion(Verdict verdict, Budget* budget)
{
    return cancellationObserved(budget) ? Verdict::Unknown : verdict;
}

bool budgetValid(Budget* budget)
{
    if (!budget) return false;
    if (cancellationObserved(budget)) return false;
    if (budget->exhausted) return false;
    if (budget->remainingWork <= 0 || budget->remainingWork > WorkBound
        || budget->remainingFragments < 0 || budget->remainingFragments > FragmentBound) {
        budget->exhausted = true;
        return false;
    }
    return true;
}

bool work(Budget* budget)
{
    if (!budget || cancellationObserved(budget)) return false;
    if (budget->remainingWork <= 0) {
        budget->exhausted = true;
        return false;
    }
    --budget->remainingWork;
    return true;
}

bool fragment(Budget* budget)
{
    if (!budget || cancellationObserved(budget)) return false;
    if (budget->remainingFragments <= 0) {
        budget->exhausted = true;
        return false;
    }
    --budget->remainingFragments;
    return true;
}

struct PrismOrderRecord {
    quint64 key = 0;
    int row = -1;
};

// Cancellation is observed at every output, never from inside a comparator
// whose interrupted ordering could invalidate a library sort's contract.
bool stablePrismOrder(QVector<PrismOrderRecord>* records, Budget* budget)
{
    const int count = records->size();
    QVector<PrismOrderRecord> buffer(count);
    for (int width = 1; width < count; width *= 2) {
        for (int first = 0; first < count; first += 2 * width) {
            const int middle = std::min(first + width, count);
            const int last = std::min(first + 2 * width, count);
            int left = first, right = middle;
            for (int destination = first; destination < last; ++destination) {
                if (!work(budget)) return false;
                if (left < middle && (right >= last || records->at(left).key <= records->at(right).key))
                    buffer[destination] = records->at(left++);
                else buffer[destination] = records->at(right++);
            }
        }
        records->swap(buffer);
    }
    return !cancellationObserved(budget);
}

Plane plane(qint64 east, qint64 north, qint64 altitude, qint64 limit, bool strict = false)
{
    Plane value;
    value.east = east; value.north = north; value.altitude = altitude;
    value.limit = limit; value.strict = strict;
    return value;
}

qint64 boxSupport(const Bounds& b, const Plane& p, bool maximum)
{
    const auto coordinate = [maximum](qint64 normal, qint64 low, qint64 high) {
        return ((normal >= 0) == maximum) ? high : low;
    };
    // Three products <= 5.1e15 each, sum <= 1.53e16. Inputs were checked.
    return p.east * coordinate(p.east, b.minimumEast, b.maximumEast)
        + p.north * coordinate(p.north, b.minimumNorth, b.maximumNorth)
        + p.altitude * coordinate(p.altitude, b.minimumAltitude, b.maximumAltitude);
}

QVector<Plane> boundedPlanes(const Polytope& value)
{
    const Bounds& b = value.bounds;
    QVector<Plane> result = value.planes;
    result.append(plane(1, 0, 0, b.maximumEast));
    result.append(plane(-1, 0, 0, -b.minimumEast));
    result.append(plane(0, 1, 0, b.maximumNorth));
    result.append(plane(0, -1, 0, -b.minimumNorth));
    result.append(plane(0, 0, 1, b.maximumAltitude));
    result.append(plane(0, 0, -1, -b.minimumAltitude));
    return result;
}

Verdict feasibleChecked(const Polytope& polytope, Budget* budget)
{
    if (!work(budget)) return Verdict::Unknown;
    bool wholeBoxInside = true;
    for (const Plane& p : polytope.planes) {
        if (!work(budget)) return Verdict::Unknown;
        const qint64 minimum = boxSupport(polytope.bounds, p, false);
        if (p.strict ? minimum >= p.limit : minimum > p.limit) return conclusion(Verdict::ProvenFalse, budget);
        const qint64 maximum = boxSupport(polytope.bounds, p, true);
        if (p.strict ? maximum >= p.limit : maximum > p.limit) wholeBoxInside = false;
    }
    if (wholeBoxInside) return conclusion(Verdict::ProvenTrue, budget);
    const QVector<Plane> planes = boundedPlanes(polytope);
    std::array<bool, TotalPlaneBound> witnessed{};
    std::array<int, TotalPlaneBound> signs{};
    int missingStrict = 0;
    for (const Plane& p : planes) if (p.strict) ++missingStrict;
    for (int first = 0; first < planes.size() - 2; ++first) {
        for (int second = first + 1; second < planes.size() - 1; ++second) {
            for (int third = second + 1; third < planes.size(); ++third) {
                if (!work(budget)) return Verdict::Unknown;
                const Plane* rows[3] = {&planes.at(first), &planes.at(second), &planes.at(third)};
                qint64 matrix[3][3];
                for (int row = 0; row < 3; ++row) {
                    matrix[row][0] = rows[row]->east;
                    matrix[row][1] = rows[row]->north;
                    matrix[row][2] = rows[row]->altitude;
                }
                Integer denominator;
                if (!determinant(matrix, &denominator)) return Verdict::Unknown;
                if (zero(denominator.words)) continue;
                Integer numerators[3];
                for (int column = 0; column < 3; ++column) {
                    qint64 replaced[3][3];
                    for (int row = 0; row < 3; ++row)
                        for (int other = 0; other < 3; ++other)
                            replaced[row][other] = other == column ? rows[row]->limit : matrix[row][other];
                    if (!determinant(replaced, &numerators[column])) return Verdict::Unknown;
                }
                if (denominator.negative) {
                    negate(&denominator);
                    for (Integer& numerator : numerators) negate(&numerator);
                }
                bool closureVertex = true;
                for (int i = 0; i < planes.size(); ++i) {
                    if (!work(budget)) return Verdict::Unknown;
                    if (!residual(planes.at(i), numerators[0], numerators[1], numerators[2], denominator, &signs[i]))
                        return Verdict::Unknown;
                    if (signs[i] > 0) { closureVertex = false; break; }
                }
                if (!closureVertex) continue;
                if (missingStrict == 0) return conclusion(Verdict::ProvenTrue, budget);
                // Each strict face may have a DIFFERENT witness. The average
                // of all witness vertices satisfies all strict faces at once.
                for (int i = 0; i < planes.size(); ++i) {
                    if (planes.at(i).strict && !witnessed[i] && signs[i] < 0) {
                        witnessed[i] = true;
                        --missingStrict;
                    }
                }
                if (missingStrict == 0) return conclusion(Verdict::ProvenTrue, budget);
            }
        }
    }
    // The closure is bounded by six explicit axis faces. A nonempty closure
    // has a vertex, including lower-dimensional boxes; exhaustive failure of
    // a strict face witness proves the strict intersection empty.
    return conclusion(Verdict::ProvenFalse, budget);
}
} // namespace

bool FleetMissionClearanceGeometry::prepareConvexPrism(const QVector<Point2>& footprint,
    qint64 minimumAltitude, qint64 maximumAltitude, PreparedPrism* result, Budget* budget)
{
    if (!result) return false;
    *result = PreparedPrism();
    if (!budgetValid(budget) || !supported() || !work(budget)) return false;
    if (footprint.size() < 3 || footprint.size() > 16
        || !inRange(minimumAltitude, CoordinateBound) || !inRange(maximumAltitude, CoordinateBound)
        || minimumAltitude > maximumAltitude) return false;
    PreparedPrism output;
    output._bounds.minimumAltitude = minimumAltitude;
    output._bounds.maximumAltitude = maximumAltitude;
    // Check the complete coordinate domain before computing any difference,
    // product or early geometry result. No validated-prefix object is exposed.
    for (int index = 0; index < footprint.size(); ++index) {
        if (!work(budget)) return false;
        const Point2& point = footprint.at(index);
        if (!inRange(point.east, 50000000) || !inRange(point.north, 50000000)) return false;
        if (index == 0) {
            output._bounds.minimumEast = output._bounds.maximumEast = point.east;
            output._bounds.minimumNorth = output._bounds.maximumNorth = point.north;
        } else {
            output._bounds.minimumEast = std::min(output._bounds.minimumEast, point.east);
            output._bounds.maximumEast = std::max(output._bounds.maximumEast, point.east);
            output._bounds.minimumNorth = std::min(output._bounds.minimumNorth, point.north);
            output._bounds.maximumNorth = std::max(output._bounds.maximumNorth, point.north);
        }
    }
    output._axes.reserve(footprint.size());
    for (int index = 0; index < footprint.size(); ++index) {
        if (!work(budget)) return false;
        const int next = (index + 1) % footprint.size();
        const Point2& first = footprint.at(index);
        const Point2& second = footprint.at(next);
        PreparedPrism::ProjectionAxis axis;
        axis.east = second.north - first.north;
        axis.north = first.east - second.east;
        if (axis.east == 0 && axis.north == 0) return false;
        // |normal| <= 1e8 per coordinate; every projection has absolute
        // value <= 1e16. The strict comparison is exactly the CCW cross>0
        // condition without subtracting two separately rounded quantities.
        const qint64 boundary = axis.east * first.east + axis.north * first.north;
        axis.minimum = axis.maximum = boundary;
        for (int vertex = 0; vertex < footprint.size(); ++vertex) {
            if (!work(budget)) return false;
            const Point2& point = footprint.at(vertex);
            const qint64 projection = axis.east * point.east + axis.north * point.north;
            if (vertex != index && vertex != next && projection >= boundary) return false;
            axis.minimum = std::min(axis.minimum, projection);
            axis.maximum = std::max(axis.maximum, projection);
        }
        output._axes.append(axis);
    }
    if (cancellationObserved(budget)) return false;
    output._valid = true;
    *result = output;
    if (cancellationObserved(budget)) {
        *result = PreparedPrism();
        return false;
    }
    return true;
}

FleetMissionClearanceGeometry::Verdict FleetMissionClearanceGeometry::axisAlignedBoxIntersectsPrism(
    const Polytope& query, const PreparedPrism& prism, Budget* budget)
{
    if (!budgetValid(budget) || !supported() || !work(budget)) return Verdict::Unknown;
    // Bounds alone are not an equivalent representation of a general
    // Polytope. Never let an AABB separation hide an extra/strict query face.
    if (!query.planes.isEmpty() || !polytopeValid(query) || !prism._valid) return Verdict::Unknown;
    const Bounds& box = query.bounds;
    const Bounds& other = prism._bounds;
    if (!work(budget)) return Verdict::Unknown;
    if (box.maximumEast < other.minimumEast || box.minimumEast > other.maximumEast)
        return conclusion(Verdict::ProvenFalse, budget);
    if (!work(budget)) return Verdict::Unknown;
    if (box.maximumNorth < other.minimumNorth || box.minimumNorth > other.maximumNorth)
        return conclusion(Verdict::ProvenFalse, budget);
    if (!work(budget)) return Verdict::Unknown;
    if (box.maximumAltitude < other.minimumAltitude || box.minimumAltitude > other.maximumAltitude)
        return conclusion(Verdict::ProvenFalse, budget);
    for (const PreparedPrism::ProjectionAxis& axis : prism._axes) {
        if (!work(budget)) return Verdict::Unknown;
        // Only two products: each <=5.1e15, their absolute sum <=1.02e16.
        // Polygon intervals were prepared once from every validated vertex.
        const qint64 minimum = axis.east * (axis.east >= 0 ? box.minimumEast : box.maximumEast)
            + axis.north * (axis.north >= 0 ? box.minimumNorth : box.maximumNorth);
        const qint64 maximum = axis.east * (axis.east >= 0 ? box.maximumEast : box.minimumEast)
            + axis.north * (axis.north >= 0 ? box.maximumNorth : box.minimumNorth);
        if (minimum > axis.maximum || maximum < axis.minimum)
            return conclusion(Verdict::ProvenFalse, budget);
    }
    // The 2D convex polygon/rectangle edge normals and the Z axis form the
    // complete SAT set for these Cartesian products, even for a flat prism
    // or point/line/slab query. Equality is deliberately NOT a separating gap.
    return conclusion(Verdict::ProvenTrue, budget);
}

bool FleetMissionClearanceGeometry::preparePrismIndex(const QVector<PreparedPrism>& prisms,
    PreparedPrismIndex* result, Budget* budget)
{
    if (!result) return false;
    const QVector<PreparedPrism> sources = prisms;
    *result = PreparedPrismIndex();
    if (!budgetValid(budget) || !supported() || !work(budget) || sources.size() > 1024) return false;
    // Validate the entire suffix before building a usable index. Native
    // private prepared states cannot hide an unvalidated last prism.
    for (const PreparedPrism& prism : sources) {
        if (!work(budget) || !prism._valid) return false;
    }
    PreparedPrismIndex output;
    output._prisms.reserve(sources.size());
    QVector<PrismOrderRecord> order;
    order.reserve(sources.size());
    for (int row = 0; row < sources.size(); ++row) {
        if (!work(budget)) return false;
        const PreparedPrism& prism = sources.at(row);
        // Append individual values into distinct container storage. A caller
        // might hold an old mutable element reference into the source vector;
        // merely sharing that vector would let it invalidate cached bounds.
        output._prisms.append(prism);
        const qint64 east = prism._bounds.minimumEast + prism._bounds.maximumEast + 100000000;
        const qint64 north = prism._bounds.minimumNorth + prism._bounds.maximumNorth + 100000000;
        if (east < 0 || east > 200000000 || north < 0 || north > 200000000) return false;
        PrismOrderRecord record;
        record.row = row;
        // Doubled XY midpoints fit 28 bits after the shift. Morton ordering
        // only arranges leaves; it never replaces the exact original bounds.
        for (int bit = 0; bit < 28; ++bit) {
            if (!work(budget)) return false;
            record.key |= ((quint64(east) >> bit) & quint64(1)) << (2 * bit);
            if (!work(budget)) return false;
            record.key |= ((quint64(north) >> bit) & quint64(1)) << (2 * bit + 1);
        }
        order.append(record);
    }
    if (!stablePrismOrder(&order, budget)) return false;
    if (!sources.isEmpty()) {
        int base = 1;
        while (base < sources.size()) base *= 2;
        output._leafOffset = base - 1;
        output._nodes.resize(2 * base - 1);
        for (int leaf = 0; leaf < base; ++leaf) {
            if (!work(budget)) return false;
            PreparedPrismIndex::Node& node = output._nodes[output._leafOffset + leaf];
            if (leaf >= order.size()) continue; // Explicit empty padding leaf.
            node.row = order.at(leaf).row;
            node.bounds = output._prisms.at(node.row)._bounds;
            node.occupied = true;
        }
        for (int parent = output._leafOffset - 1; parent >= 0; --parent) {
            if (!work(budget)) return false;
            PreparedPrismIndex::Node& node = output._nodes[parent];
            const PreparedPrismIndex::Node& left = output._nodes.at(2 * parent + 1);
            const PreparedPrismIndex::Node& right = output._nodes.at(2 * parent + 2);
            if (!left.occupied && !right.occupied) continue;
            node.occupied = true;
            if (!left.occupied) node.bounds = right.bounds;
            else if (!right.occupied) node.bounds = left.bounds;
            else {
                node.bounds.minimumEast = std::min(left.bounds.minimumEast, right.bounds.minimumEast);
                node.bounds.maximumEast = std::max(left.bounds.maximumEast, right.bounds.maximumEast);
                node.bounds.minimumNorth = std::min(left.bounds.minimumNorth, right.bounds.minimumNorth);
                node.bounds.maximumNorth = std::max(left.bounds.maximumNorth, right.bounds.maximumNorth);
                node.bounds.minimumAltitude = std::min(left.bounds.minimumAltitude, right.bounds.minimumAltitude);
                node.bounds.maximumAltitude = std::max(left.bounds.maximumAltitude, right.bounds.maximumAltitude);
            }
        }
    }
    if (cancellationObserved(budget)) return false;
    output._valid = true; // Empty sources intentionally produce a valid empty index.
    *result = output;
    if (cancellationObserved(budget)) {
        *result = PreparedPrismIndex();
        return false;
    }
    return true;
}

bool FleetMissionClearanceGeometry::prismCandidates(const Polytope& query,
    const PreparedPrismIndex& index, QVector<int>* result, Budget* budget)
{
    if (!result) return false;
    result->clear();
    if (!budgetValid(budget) || !supported() || !work(budget)
        || !query.planes.isEmpty() || !polytopeValid(query) || !index._valid) return false;
    if (!index._nodes.isEmpty()) {
        if (!work(budget)) return false;
        const PreparedPrismIndex::Node& root = index._nodes.first();
        const Bounds& box = query.bounds;
        const Bounds& other = root.bounds;
        if (root.occupied && (box.maximumEast < other.minimumEast || box.minimumEast > other.maximumEast
            || box.maximumNorth < other.minimumNorth || box.minimumNorth > other.maximumNorth
            || box.maximumAltitude < other.minimumAltitude || box.minimumAltitude > other.maximumAltitude)) {
            // Complete query/index validation already happened. Strict root
            // separation proves the ENTIRE candidate set empty; avoid even
            // allocating traversal/candidate storage for this exact case.
            return !cancellationObserved(budget);
        }
        if (root.occupied && box.minimumEast <= other.minimumEast && box.maximumEast >= other.maximumEast
            && box.minimumNorth <= other.minimumNorth && box.maximumNorth >= other.maximumNorth
            && box.minimumAltitude <= other.minimumAltitude && box.maximumAltitude >= other.maximumAltitude) {
            // The factory already validated EVERY owned prism. Full root
            // containment therefore proves all rows are candidates without
            // traversing or re-sorting; it is not a heuristic hit threshold.
            QVector<int> rows;
            rows.reserve(index._prisms.size());
            for (int row = 0; row < index._prisms.size(); ++row) {
                if (!work(budget)) return false;
                rows.append(row);
            }
            if (cancellationObserved(budget)) return false;
            *result = rows;
            if (cancellationObserved(budget)) {
                result->clear();
                return false;
            }
            return true;
        }
    }
    QVector<PrismOrderRecord> candidates;
    candidates.reserve(index._prisms.size());
    QVector<int> stack;
    if (!index._nodes.isEmpty()) {
        // The complete heap has at most 1024 leaves / depth 10. DFS retains
        // only one sibling per level, not storage for all 2047 tree nodes.
        stack.reserve(12);
        stack.append(0);
    }
    while (!stack.isEmpty()) {
        if (!work(budget)) return false;
        const int nodeIndex = stack.takeLast();
        if (nodeIndex < 0 || nodeIndex >= index._nodes.size()) return false;
        const PreparedPrismIndex::Node& node = index._nodes.at(nodeIndex);
        if (!node.occupied) continue;
        const Bounds& box = query.bounds;
        const Bounds& other = node.bounds;
        if (box.maximumEast < other.minimumEast || box.minimumEast > other.maximumEast
            || box.maximumNorth < other.minimumNorth || box.minimumNorth > other.maximumNorth
            || box.maximumAltitude < other.minimumAltitude || box.minimumAltitude > other.maximumAltitude) continue;
        if (nodeIndex >= index._leafOffset) {
            if (node.row < 0 || node.row >= index._prisms.size()) return false;
            PrismOrderRecord record;
            record.key = quint64(node.row);
            record.row = node.row;
            candidates.append(record);
        } else {
            stack.append(2 * nodeIndex + 2);
            stack.append(2 * nodeIndex + 1);
        }
    }
    // Morton traversal order never changes externally visible original-row
    // ordering. Reuse the cancellable stable merge to restore that order.
    if (!stablePrismOrder(&candidates, budget)) return false;
    QVector<int> rows;
    rows.reserve(candidates.size());
    int previous = -1;
    for (const PrismOrderRecord& candidate : candidates) {
        if (!work(budget)) return false;
        if (candidate.row <= previous || candidate.row >= index._prisms.size()) return false;
        rows.append(candidate.row);
        previous = candidate.row;
    }
    if (cancellationObserved(budget)) return false;
    *result = rows;
    if (cancellationObserved(budget)) {
        result->clear();
        return false;
    }
    return true;
}

FleetMissionClearanceGeometry::Verdict FleetMissionClearanceGeometry::axisAlignedBoxIntersectsIndexedPrism(
    const Polytope& query, const PreparedPrismIndex& index, int originalRow, Budget* budget)
{
    if (!budgetValid(budget) || !supported() || !work(budget) || !index._valid
        || originalRow < 0 || originalRow >= index._prisms.size()) return Verdict::Unknown;
    return axisAlignedBoxIntersectsPrism(query, index._prisms.at(originalRow), budget);
}

FleetMissionClearanceGeometry::Verdict FleetMissionClearanceGeometry::feasible(
    const Polytope& polytope, Budget* budget)
{
    if (!budgetValid(budget) || !supported() || !polytopeValid(polytope)) return Verdict::Unknown;
    return conclusion(feasibleChecked(polytope, budget), budget);
}

FleetMissionClearanceGeometry::Verdict FleetMissionClearanceGeometry::coveredByUnion(
    const Polytope& polytope, const QVector<QVector<Plane>>& volumes, Budget* budget)
{
    if (!budgetValid(budget) || !supported() || !polytopeValid(polytope) || volumes.size() > 1024)
        return Verdict::Unknown;
    // Validate ALL volume inputs before any early geometry conclusion.
    // Bounds containment is sufficient even for an empty query. Accumulate
    // that proof without spending a fragment or enumerating query vertices,
    // but never let it hide an invalid or unexamined union suffix.
    bool wholeBoundsCovered = false;
    for (const QVector<Plane>& volume : volumes) {
        if (cancellationObserved(budget)) return Verdict::Unknown;
        if (volume.isEmpty() || volume.size() > 18) return Verdict::Unknown;
        bool wholeBoundsInside = true;
        for (const Plane& p : volume) {
            if (!work(budget)) return Verdict::Unknown;
            if (!planeValid(p) || p.strict) return Verdict::Unknown;
            if (boxSupport(polytope.bounds, p, true) > p.limit) wholeBoundsInside = false;
        }
        wholeBoundsCovered = wholeBoundsCovered || wholeBoundsInside;
    }
    if (wholeBoundsCovered) return conclusion(Verdict::ProvenTrue, budget);
    const Verdict initial = feasibleChecked(polytope, budget);
    if (initial == Verdict::Unknown) return initial;
    if (initial == Verdict::ProvenFalse) return conclusion(Verdict::ProvenTrue, budget);
    if (volumes.isEmpty()) return conclusion(Verdict::ProvenFalse, budget);
    if (!fragment(budget)) return Verdict::Unknown;
    struct Task { Polytope polytope; int index = 0; };
    QVector<Task> stack;
    Task root; root.polytope = polytope;
    stack.append(root);
    while (!stack.isEmpty()) {
        if (cancellationObserved(budget)) return Verdict::Unknown;
        Task task = stack.takeLast();
        // Every stacked fragment has already been proved nonempty.
        if (task.index >= volumes.size()) return conclusion(Verdict::ProvenFalse, budget);
        Polytope prefix = task.polytope;
        const QVector<Plane>& volume = volumes.at(task.index);
        for (const Plane& p : volume) {
            if (!work(budget)) return Verdict::Unknown;
            if (boxSupport(prefix.bounds, p, true) <= p.limit) continue;
            if (boxSupport(prefix.bounds, p, false) > p.limit) {
                // All of this nonempty prefix is outside this volume. No new
                // clipping plane or geometrically new fragment is needed.
                task.polytope = prefix;
                ++task.index;
                stack.append(task);
                break;
            }
            if (prefix.planes.size() >= ExtraPlaneBound) return Verdict::Unknown;
            if (!fragment(budget)) return Verdict::Unknown;
            Polytope outside = prefix;
            outside.planes.append(plane(-p.east, -p.north, -p.altitude, -p.limit, true));
            const Verdict outsideState = feasibleChecked(outside, budget);
            if (outsideState == Verdict::Unknown) return Verdict::Unknown;
            if (outsideState == Verdict::ProvenTrue) {
                if (task.index + 1 == volumes.size()) return conclusion(Verdict::ProvenFalse, budget);
                Task next; next.polytope = outside; next.index = task.index + 1;
                stack.append(next);
            }
            // Keep the closed inside prefix for the remaining faces. Its
            // fully inside remainder is covered and is discarded at the end.
            prefix.planes.append(p);
            const Verdict prefixState = feasibleChecked(prefix, budget);
            if (prefixState == Verdict::Unknown) return Verdict::Unknown;
            if (prefixState == Verdict::ProvenFalse) break;
        }
    }
    return conclusion(Verdict::ProvenTrue, budget);
}

FleetMissionClearanceGeometry::Verdict FleetMissionClearanceGeometry::coveredByIndexedPrisms(
    const Polytope& polytope, const PreparedPrismIndex& index, Budget* budget)
{
    // Validate the ORIGINAL complete query before any broad-phase shortcut.
    // An invalid extra/strict face must not disappear with an empty candidate
    // set. The private index factory already validated and owns all original
    // prisms, including rows that the bounds traversal will exclude.
    if (!budgetValid(budget) || !supported() || !polytopeValid(polytope)
        || !index._valid || !work(budget)) return Verdict::Unknown;
    Polytope broadPhase;
    broadPhase.bounds = polytope.bounds;
    QVector<int> candidates;
    if (!prismCandidates(broadPhase, index, &candidates, budget)) return Verdict::Unknown;

    QVector<QVector<Plane>> volumes;
    volumes.reserve(candidates.size());
    for (int row : candidates) {
        if (!work(budget) || row < 0 || row >= index._prisms.size()) return Verdict::Unknown;
        const PreparedPrism& prism = index._prisms.at(row);
        if (!prism._valid || prism._axes.size() < 3 || prism._axes.size() > 16) return Verdict::Unknown;
        QVector<Plane> volume;
        volume.reserve(prism._axes.size() + 2);
        for (const PreparedPrism::ProjectionAxis& axis : prism._axes) {
            if (!work(budget)) return Verdict::Unknown;
            // prepareConvexPrism proved every nonadjacent vertex strictly
            // below this CCW edge's boundary. Its maximum therefore IS the
            // original closed supporting face, not a rounded SAT surrogate.
            volume.append(plane(axis.east, axis.north, 0, axis.maximum));
        }
        if (!work(budget)) return Verdict::Unknown;
        volume.append(plane(0, 0, -1, -prism._bounds.minimumAltitude));
        if (!work(budget)) return Verdict::Unknown;
        volume.append(plane(0, 0, 1, prism._bounds.maximumAltitude));
        volumes.append(volume);
    }
    // Candidate rows retain original order, and only STRICTLY separated
    // prisms were removed: touching and lower-dimensional queries remain.
    // Never replace the original query by broadPhase, nor infer false from
    // zero candidates: a valid but empty full query is vacuously covered.
    return conclusion(coveredByUnion(polytope, volumes, budget), budget);
}

bool FleetMissionClearanceGeometry::sweptBox(const qint64 from[3], const qint64 to[3],
    const qint64 extent[3], Polytope* result)
{
    if (!result) return false;
    *result = Polytope();
    if (!supported() || !from || !to || !extent) return false;
    qint64 lowerOffset[3], upperOffset[3];
    for (int i = 0; i < 3; ++i) {
        if (extent[i] < 1 || extent[i] > 500001) return false;
        lowerOffset[i] = -extent[i];
        upperOffset[i] = extent[i];
    }
    return sweptBoxOffsets(from, to, lowerOffset, upperOffset, result);
}

bool FleetMissionClearanceGeometry::sweptBoxOffsets(const qint64 from[3], const qint64 to[3],
    const qint64 lowerOffset[3], const qint64 upperOffset[3], Polytope* result)
{
    if (!result) return false;
    *result = Polytope();
    if (!supported() || !from || !to || !lowerOffset || !upperOffset) return false;
    qint64 low[3], high[3], direction[3];
    for (int i = 0; i < 3; ++i) {
        if (!inRange(from[i], 50000000) || !inRange(to[i], 50000000)
            || !inRange(lowerOffset[i], 500001) || !inRange(upperOffset[i], 500001)
            || lowerOffset[i] > upperOffset[i]) return false;
        low[i] = std::min(from[i], to[i]) + lowerOffset[i];
        high[i] = std::max(from[i], to[i]) + upperOffset[i];
        if (!inRange(low[i], CoordinateBound) || !inRange(high[i], CoordinateBound)) return false;
        direction[i] = to[i] - from[i];
    }
    Polytope output;
    output.bounds.minimumEast = low[0]; output.bounds.maximumEast = high[0];
    output.bounds.minimumNorth = low[1]; output.bounds.maximumNorth = high[1];
    output.bounds.minimumAltitude = low[2]; output.bounds.maximumAltitude = high[2];
    const qint64 normals[3][3] = {{0, -direction[2], direction[1]},
        {direction[2], 0, -direction[0]}, {-direction[1], direction[0], 0}};
    // Translating the box changes its support, not these normal directions.
    // Keep the paired constraints for zero-width boxes too: their limiting
    // closed halfspaces also preserve the swept set's lower-dimensional hull.
    // Axis support is exactly represented by Bounds, not an AABB substitute.
    for (const auto& normal : normals) {
        if (normal[0] == 0 && normal[1] == 0 && normal[2] == 0) continue;
        for (qint64 sign : {qint64(1), qint64(-1)}) {
            qint64 n[3] = {normal[0] * sign, normal[1] * sign, normal[2] * sign};
            qint64 fromSupport = 0, toSupport = 0, offsetSupport = 0;
            for (int i = 0; i < 3; ++i) {
                fromSupport += n[i] * from[i];
                toSupport += n[i] * to[i];
                offsetSupport += n[i] * (n[i] >= 0 ? upperOffset[i] : lowerOffset[i]);
            }
            const Plane p = plane(n[0], n[1], n[2], std::max(fromSupport, toSupport) + offsetSupport);
            if (!planeValid(p)) return false;
            output.planes.append(p);
        }
    }
    if (!polytopeValid(output)) return false;
    *result = output;
    return true;
}

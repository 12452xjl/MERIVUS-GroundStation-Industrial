#pragma once

#include <QtGlobal>
#include <QVector>

#include <atomic>
#include <memory>

// Exact, bounded millimetre geometry only. These values are not observations,
// source authentication, current scene validity or permission to fly.
class FleetMissionClearanceGeometry
{
public:
    // Shared lifetime, read-only to the calculation. The owning request may
    // store true once; it must never reset/reuse a token for another request.
    using CancellationToken = std::shared_ptr<const std::atomic_bool>;
    struct Plane {
        qint64 east = 0;
        qint64 north = 0;
        qint64 altitude = 0;
        qint64 limit = 0;
        bool strict = false; // n.p < limit; otherwise n.p <= limit; n != 0.
    };
    struct Bounds {
        qint64 minimumEast = 0;
        qint64 maximumEast = 0;
        qint64 minimumNorth = 0;
        qint64 maximumNorth = 0;
        qint64 minimumAltitude = 0;
        qint64 maximumAltitude = 0;
    };
    struct Polytope {
        Bounds bounds;
        QVector<Plane> planes; // 0..64 extra planes; six bounds are implicit.
    };
    struct Point2 {
        qint64 east = 0;
        qint64 north = 0;
    };
    // Native-only validated vertical convex prism. Callers cannot supply
    // arbitrary projection axes or manufacture the valid state. Explicit
    // copies suppress implicit moves: a moved-from valid/empty-axis object
    // must never become another apparently prepared prism.
    class PreparedPrism {
    public:
        PreparedPrism() = default;
        PreparedPrism(const PreparedPrism&) = default;
        PreparedPrism& operator=(const PreparedPrism&) = default;

    private:
        friend class FleetMissionClearanceGeometry;
        struct ProjectionAxis {
            qint64 east = 0;
            qint64 north = 0;
            qint64 minimum = 0;
            qint64 maximum = 0;
        };
        Bounds _bounds;
        QVector<ProjectionAxis> _axes;
        bool _valid = false;
    };
    // Native-only broad-phase index owning the same prism values later used
    // by indexed SAT. A candidate row cannot be paired with another array.
    // Copy-only for the same valid/moved-from-state reason as PreparedPrism.
    class PreparedPrismIndex {
    public:
        PreparedPrismIndex() = default;
        PreparedPrismIndex(const PreparedPrismIndex&) = default;
        PreparedPrismIndex& operator=(const PreparedPrismIndex&) = default;

    private:
        friend class FleetMissionClearanceGeometry;
        struct Node {
            Bounds bounds;
            bool occupied = false;
            int row = -1;
        };
        QVector<PreparedPrism> _prisms;
        QVector<Node> _nodes;
        int _leafOffset = 0;
        bool _valid = false;
    };
    enum class Verdict { ProvenFalse, ProvenTrue, Unknown };
    struct Budget {
        qint64 remainingWork = 2000000;
        int remainingFragments = 2048;
        bool exhausted = false;
        CancellationToken cancellation;
        bool cancelled = false; // sticky; distinct from resource exhaustion.
    };

    // Validate all 3..16 strict convex CCW footprint points within +/-50000000
    // mm and the closed Z interval within +/-51000000 mm. Failure clears out.
    // Preparation and SAT queries charge the same caller-owned work budget.
    static bool prepareConvexPrism(const QVector<Point2>& footprint,
                                   qint64 minimumAltitude, qint64 maximumAltitude,
                                   PreparedPrism* result, Budget* budget);
    // Exact closed AABB/prism intersection only. Nonempty query.planes,
    // invalid bounds or an unprepared prism return Unknown, never ignore
    // extra planes. Touching and zero-width query axes remain intersections.
    static Verdict axisAlignedBoxIntersectsPrism(const Polytope& query,
                                                 const PreparedPrism& prism,
                                                 Budget* budget);
    // Complete 0..1024-prism index preparation; empty is a valid empty index.
    // Failure clears the output. No partial candidates survive a failed or
    // cancelled traversal. Candidate rows are unique original-row order.
    static bool preparePrismIndex(const QVector<PreparedPrism>& prisms,
                                  PreparedPrismIndex* result, Budget* budget);
    static bool prismCandidates(const Polytope& query, const PreparedPrismIndex& index,
                                QVector<int>* result, Budget* budget);
    // A broad-phase hit is not an intersection verdict. Resolve it against
    // the exact same index-owned original prism and the existing SAT query.
    static Verdict axisAlignedBoxIntersectsIndexedPrism(const Polytope& query,
                                                        const PreparedPrismIndex& index,
                                                        int originalRow, Budget* budget);

    static Verdict feasible(const Polytope& polytope, Budget* budget);
    // Each union member is a closed convex volume with 1..18 planes.
    // The query is always bounded; an empty union is permitted.
    static Verdict coveredByUnion(const Polytope& polytope,
                                  const QVector<QVector<Plane>>& volumes,
                                  Budget* budget);
    // Equivalent closed-prism union coverage, using only an index whose
    // factory validated and owns EVERY original prism. Strict AABB separation
    // only removes disjoint members; the full query (including extra/strict
    // planes) still reaches coveredByUnion. An empty candidate set retains
    // that solver's empty-query semantics. All work shares the same budget.
    static Verdict coveredByIndexedPrisms(const Polytope& polytope,
                                          const PreparedPrismIndex& index,
                                          Budget* budget);
    static bool sweptBox(const qint64 from[3], const qint64 to[3],
                         const qint64 extent[3], Polytope* result);
    // Exact segment + translated axis box. Each offset is within +/-500001;
    // lower <= upper on every axis, including zero-width/point boxes.
    static bool sweptBoxOffsets(const qint64 from[3], const qint64 to[3],
                                const qint64 lowerOffset[3], const qint64 upperOffset[3],
                                Polytope* result);
};

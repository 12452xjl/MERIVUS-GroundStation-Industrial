#pragma once

#include "FleetMissionClearanceGeometry.h"
#include "FleetMissionStartupGeographicMotion.h"

#include <QString>
#include <QVariantMap>
#include <QVector>

// Legacy: a cover of the IDEAL sphere corridor, not of Qt's numeric predicate.
// Explicit guarded model: projects the original immutable integer domain
// also enforced by contains(). Neither branch certifies physical clearance.
class FleetMissionStartupDomainCover final
{
public:
    using Geometry = FleetMissionClearanceGeometry;
    using CancellationToken = Geometry::CancellationToken;

    struct Cell {
        Geometry::Polytope reference; // Reference domain only; no body margins.
        int segmentIndex = 0;         // Original geographic chain, zero-based.
        int endpointIndex = 0;        // 0 = first, 1 = second endpoint cap.
        qint64 minLatE7 = 0, maxLatE7 = 0;
        qint64 minLonE7 = 0, maxLonE7 = 0;
    };
    struct Result {
        bool ok = false;
        QString error;
        // Full planned totals only. Zero until raw-cell planning completes;
        // projection failure/cancellation retains known totals, not cells.
        int segmentCount = 0;
        int cellCount = 0;
        // True only after EVERY original native integer cell/slab projects.
        // This is construction containment, not actual flight dynamics.
        bool nativeIntegerDomainCovered = false;
        QVector<Cell> cells;
    };

    // The caller fully normalizes the engineering input once for the group.
    // This entry validates its consumed frame and the original model's value
    // structure. It runs no solver, trig, I/O, clock or source authentication.
    // At most 2048 original geographic legs / 8192 cells per model. Failure
    // returns no cell prefix, including frame overflow and cancellation.
    static Result prepare(const FleetMissionStartupGeographicMotion::Ptr& originalModel,
                          const QVariantMap& normalizedClearanceInput,
                          const CancellationToken& cancellation = CancellationToken());
};

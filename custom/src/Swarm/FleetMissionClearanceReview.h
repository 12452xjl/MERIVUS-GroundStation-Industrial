#pragma once

#include <QVariantMap>

#include "FleetMissionClearanceGeometry.h"

// Pure, bounded declared-model calculation. The caller must establish the
// actual source binding. No observations, clock, I/O or permissions are used.
class FleetMissionClearanceReview
{
public:
    // Exact3 {ok, error, report}. Invalid input has an empty report. A built
    // report may be incomplete or contain violations: ok is not clearance.
    static QVariantMap build(const QVariant& clearanceInput,
                             const QVariantMap& jointView);

    // Geometry-scope joint22 -> report schema2 for input1, or schema3 with
    // explicit protected/support components for input2. The captured-review
    // contract above remains input1 only. Neither scope is a final permit.
    static QVariantMap buildForGeometryBasis(const QVariant& clearanceInput,
                                            const QVariantMap& jointView);

    // Native worker entry points. Cancellation returns the usual result3
    // with ok=false, error=clearance-review-cancelled and an empty report.
    static QVariantMap buildCancelable(const QVariant& clearanceInput,
                                       const QVariantMap& jointView,
                                       const FleetMissionClearanceGeometry::CancellationToken& cancellation);
    static QVariantMap buildForGeometryBasisCancelable(const QVariant& clearanceInput,
                                                       const QVariantMap& jointView,
                                                       const FleetMissionClearanceGeometry::CancellationToken& cancellation);

    // Shared value/source validation for additional native geometry reviews.
    // Reuses the original limits, input normalization and complete Joint scope
    // validator without running an environment solver. On success, returns the
    // complete canonical declaration; this does not authenticate its source.
    // Failure/cancellation clears output, including when it aliases an input.
    static bool geometrySourceValid(const QVariant& clearanceInput,
                                    const QVariantMap& jointView,
                                    QVariantMap* normalizedInput,
                                    const FleetMissionClearanceGeometry::CancellationToken& cancellation = {});

    // Checks a successful schema2/3 encoded-planning result against the same
    // plan input and a newly extracted Joint::buildPlanning(plan, {}). Does
    // not rerun the environment solver or authenticate its conclusions.
    static bool planningResultMatches(const QVariantMap& plan,
                                      const QVariantMap& result);

    // Checks a successful schema2/3 result against the complete frozen
    // geometry-scope joint22 and declaration. Supports both declared scopes;
    // no solver, live source capture, authority or final-write proof is run.
    static bool geometryResultMatches(const QVariant& clearanceInput,
                                      const QVariantMap& frozenJointView,
                                      const QVariantMap& result);
};

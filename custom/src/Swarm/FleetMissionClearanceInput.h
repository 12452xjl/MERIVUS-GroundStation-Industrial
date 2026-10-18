#pragma once

#include <QVariant>
#include <QVariantMap>

// Pure bounded engineering declarations. No geometry solver, actual-source
// authentication, current-time check, I/O or flight permission is provided.
class FleetMissionClearanceInput
{
public:
    // Exact four fields: ok, reason, input, fingerprint. Failure never returns
    // a normalized prefix. All accepted numeric fields normalize to Double.
    // Schema 1 is unchanged. Schema 2 requires explicit support dimensions
    // and a complete contacts list; no contact or physical fact is inferred.
    static QVariantMap normalize(const QVariant& source);

    // Exact seven fields: provided, inputValid, reason, input, fingerprint,
    // memberIdsMatch, snapshotOnly. Optional input lives in plan.constraints.
    // A present valid declaration must match all active plan.members in order.
    static QVariantMap inspectForPlan(const QVariantMap& plan);
};

#pragma once

#include <QVariantList>
#include <QVariantMap>

// Pure local-source parameter comparison and branch description. It does not
// sample a Vehicle, replace raw receipts, advance an onboard state machine,
// calculate a trajectory, or grant permission to start an action.
class FleetMissionActionPhaseReview final
{
public:
    // parameters is the original sorted twelve-row set; phaseParameters is
    // the fixed sorted fourteen-row action-phase set. Receipt freshness and
    // native-source continuity must be checked separately by the caller.
    // Exact result: {ok, reason, model, expectedParameters, actionBranches,
    //                takeoffCompatible, landCompatible,
    //                actualActionStateProven:false}.
    // The compatibility flags only describe local parameter preconditions:
    // takeoff requires no external avoidance injection, a finite positive
    // near-ground acceleration and a finite controller-ramp initial reference;
    // landing requires no injection, RC assistance disabled, and finite
    // interpolation coefficients. Unrelated actions do not inherit failures.
    // Negative-zero ramp is not normalized to positive zero: its source
    // near-ground acceleration is nonfinite, reported as null plus flags.
    // Coefficients outside the diagnostic materialization domain are reported
    // as a pair of nulls, not treated as invalid input or silently clamped.
    // Neither flag is an execution permit.
    // Failure returns no expected-parameter or branch prefix.
    static QVariantMap evaluate(const QVariantList& parameters,
                                const QVariantList& phaseParameters);
};

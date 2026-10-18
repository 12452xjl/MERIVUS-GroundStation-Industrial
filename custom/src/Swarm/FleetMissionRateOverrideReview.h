#pragma once

#include <QVariantList>
#include <QVariantMap>

// Pure comparison against the local PX4 source's parameter-update rules.
// It does not read a Vehicle, certify the installed binary, replace observed
// parameters, calculate an actual trajectory or grant flight permission.
class FleetMissionRateOverrideReview final
{
public:
    // parameters is the original sorted twelve-row content/observation set;
    // guardParameters is the sorted XY_VEL_ALL/Z_VEL_ALL pair. No freshness or
    // native-source claim is made here: callers must verify both separately.
    // Exact result: {ok, reason, model, expectedParameters,
    //                macroOverridesActive, actualInternalValuesProven:false}.
    // Failure never returns a prefix of expectedParameters. Positive or zero
    // ALL settings are active; negative settings leave individual rates alone.
    static QVariantMap evaluate(const QVariantList& parameters,
                                const QVariantList& guardParameters);
};

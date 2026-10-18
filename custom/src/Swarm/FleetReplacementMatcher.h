#pragma once

#include <QVariantList>
#include <QVariantMap>

// Bounded value-only one-to-one matching for reviewed replacement proposals.
// Callers, not this helper, establish role/health/ownership eligibility. The
// result neither reserves a real vehicle nor authorizes a task/flight change.
class FleetReplacementMatcher
{
public:
    // Each slot has exactly {slotId: QString, eligibleVehicleIds: QVariantList}.
    // Candidate IDs are ordered by the caller's deterministic preference.
    // Both collections are bounded to 255, and each numeric ID is 1..255.
    static QVariantMap match(const QVariantList& replacementSlots,
                             const QVariantList& orderedCandidateVehicleIds);
};

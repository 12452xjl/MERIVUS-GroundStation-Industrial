#pragma once

#include <QVariantMap>

// Pure checks of a passive native GPS_GLOBAL_ORIGIN observation. The caller
// must separately retain and verify the actual Vehicle and PRIMARY owner.
// No map, hash or successful comparison authenticates a shared origin.
class FleetMissionStartupGlobalOrigin
{
public:
    // Strict native24 -> stable18. Capture can preserve unavailable/invalid
    // observations; admissible is a separate predicate, not a source proof.
    static bool capture(const QVariantMap& observation, int vehicleId,
        int componentId, QVariantMap* reference);

    // Only a genuinely never-observed empty source, or a currently available
    // in-range observed declaration, can match this supplementary check.
    // This event message has no periodic freshness requirement.
    static bool admissible(const QVariantMap& reference);

    // Includes source epochs and the monotonic reference revision, so an
    // observed A -> B -> A cannot reuse the earlier reference.
    static bool same(const QVariantMap& reference, const QVariantMap& current);
};

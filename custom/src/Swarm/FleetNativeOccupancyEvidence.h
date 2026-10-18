#pragma once

#include <QVariantMap>

// Pure value consistency, not native provenance or aircraft availability.
// Only the actual pinned Swarm owner may admit a local assignment change.
class FleetNativeOccupancyEvidence
{
public:
    static bool snapshotValid(const QVariantMap& snapshot,
                              QVariantMap* canonical = nullptr);
    static QVariantMap stableIdentity(const QVariantMap& snapshot);
};

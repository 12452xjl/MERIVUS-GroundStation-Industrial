#pragma once

#include <QVariantMap>

// Bounded display-only projection of one compiled remainder plan. Hashes and
// provenance establish local value consistency, not native ownership, flight
// permission, current onboard contents or route clearance. No Vehicle or I/O.
class FleetMissionRemainderView
{
public:
    static QVariantMap build(const QVariantMap& plan);
    // Complete multi-member projection with its own schema; no first-member
    // fallback and no change to the legacy single-member public contract.
    static QVariantMap buildGroup(const QVariantMap& plan);
};

#pragma once

#include <QMap>
#include <QVariantMap>

#include <functional>

class FleetRegistry;
class Vehicle;

// Native current-source capture only. Neither configuration correspondence
// nor this computer's UTC window authenticates a physical envelope or datum.
class FleetMissionClearanceSource
{
public:
    // Exact7: provided, inputValid, configurationAndTimeCurrent, reason,
    // evidence, remainingValidityMs, observedAtUtcMs. Both times are qint64;
    // absent/failed captures return -1 and never an evidence prefix.
    // actualVehicles must contain exactly this plan's active member IDs.
    // The caller freezes the FIRST monotonic deadline and detects UTC rollback
    // across calls; this helper never grants or renews a validity lease.
    static QVariantMap capture(const QVariantMap& plan, FleetRegistry* actualRegistry,
        const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence = {});

    // Only the actual native owner's FIRST successful evidence18 is accepted.
    // Rechecks actual sources/time without normalizing or hashing the scene
    // again; success preserves that exact evidence and never renews a lease.
    // This is not an import/QML recovery or evidence-authentication API.
    static QVariantMap recapture(const QVariantMap& frozenEvidence, FleetRegistry* actualRegistry,
        const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence = {});
};

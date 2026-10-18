#pragma once

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// Pure planning geometry and nominal-load time allowances. No Vehicle lookup,
// mission upload, standby command, role reassignment, or permission restoration.
class FleetReserveEnergyPlan
{
public:
    // Exact {ok, policy, reason}; every one of the eight policy fields is
    // explicit. Unknown/malformed policies never acquire zero-cost defaults.
    static QVariantMap normalize(const QVariantMap& policy);

    // Descriptor keys: vehicleId/start/returnTo/reservePlan. Missing keys are
    // retained as an explained incomplete result, not a borrowed primary route.
    // Complete means this bounded proposal is calculable, not flight-ready.
    // Explicit actual bool performancePolicy=true requires the reserve's
    // current declaration/baseline canonical max speed for the COMPLETE own
    // standby/ingress/primary-route-redo/return budget. No constraints alias
    // enables the policy, and invalid/typed-null policy never repairs to false.
    // Four-argument callers preserve the old false-policy result shape.
    static QVariantMap build(const QVariantMap& reserveDescriptor,
                             const QVariantList& primaryMembers,
                             const QVariantMap& constraints,
                             const QVariantMap& reserveVehicle,
                             const QVariant& performancePolicy = QVariant(false));
};

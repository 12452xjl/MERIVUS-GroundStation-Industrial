#pragma once

#include "LinkInterface.h"

#include <QVariantMap>
#include <memory>

class Vehicle;
class ParameterManager;

// Native, non-portable continuity seal for one complete startup phase check.
// It is not an authority, a current-state decoder, or a flight permission.
class FleetStartupMemorySeal final
{
public:
    using Ptr = std::shared_ptr<const FleetStartupMemorySeal>;

    static Ptr capture(Vehicle* actualVehicle, LinkInterface* actualPrimary,
        ParameterManager* actualParameters, const WeakLinkInterfacePtr& originalPrimaryOwner,
        const QVariantMap& originalExecutionParameters, const QVariantMap& originalStartupPolicy);
    bool current() const;
    ~FleetStartupMemorySeal();

    FleetStartupMemorySeal(const FleetStartupMemorySeal&) = delete;
    FleetStartupMemorySeal& operator=(const FleetStartupMemorySeal&) = delete;

private:
    struct State;
    explicit FleetStartupMemorySeal(std::unique_ptr<State> state);
    const std::unique_ptr<State> _state;
};

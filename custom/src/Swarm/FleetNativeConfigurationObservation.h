#pragma once

#include <QVariantMap>
#include <functional>

class Vehicle;

// Local decoded properties only: no parameter requests, writes, raw-sample
// certification, physical-envelope measurement or flight permission.
class FleetNativeConfigurationObservation
{
public:
    // Exact3 {ok, reason, configuration}. On failure configuration is empty.
    // The optional owned fence is checked before/after calls into plugins;
    // callers must not pass a fence which itself sends or mutates a vehicle.
    static QVariantMap capture(Vehicle* actualVehicle,
                               const std::function<bool()>& fence = std::function<bool()>());
};

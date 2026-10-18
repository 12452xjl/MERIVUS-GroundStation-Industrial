#pragma once

#include <QVariantMap>

class Vehicle;
class MissionManager;

// Native-only read-only correlation, not a task execution/completion permit.
// The current onboard cache and source-map identity are checked independently
// from fresh raw progress telemetry. No upload, start, mode or ARM calls.
class FleetMissionProgressObservation
{
public:
    static QVariantMap capture(Vehicle* vehicle, MissionManager* manager,
                               const QVariantMap& payload);
};

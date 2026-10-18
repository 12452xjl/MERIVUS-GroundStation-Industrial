#pragma once

#include <QtGlobal>
#include <QVariantMap>

class Vehicle;
class FleetFirmwareAdapterCatalog;

// Native-only, read-only capture of actual Vehicle/ParameterManager sources.
// It never requests parameters, writes, uploads, arms, or grants execution.
class FleetMissionVehicleEvidence
{
public:
    static QVariantMap capture(Vehicle* vehicle, const QVariantMap& member,
        const QVariantMap& constraints, qint64 maximumParameterAgeMs = 30000,
        FleetFirmwareAdapterCatalog* firmwareAdapters = nullptr);
};

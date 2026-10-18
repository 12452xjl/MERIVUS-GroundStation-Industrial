#pragma once

#include <QVariantList>
#include <QVariantMap>

class QObject;
class Vehicle;
class FleetFirmwareAdapterCatalog;

// Native observed phase consistency, NOT authentication, a StartPermit or
// proof that telemetry was caused by this ground station's command.
class FleetMissionStartupTransition
{
public:
    enum class Phase { BeforeSeq0, AwaitSeq0, ReadyToArm, AwaitArm, ReadyToMode, AwaitMode, ModeObserved };
    static QVariantMap capture(Vehicle* vehicle, QObject* actualTransaction, const QString& startupId,
        const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& payload,
        const QVariantMap& baselineBeforeArm, const QVariantList& orderedStepIds, Phase phase,
        FleetFirmwareAdapterCatalog* catalog = nullptr);
};

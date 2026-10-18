#pragma once

#include <QVariantMap>

class Vehicle;
class FleetFirmwareAdapterCatalog;

// Native-only, bounded, read-only physical precondition evidence. This is
// neither a planning approval identity nor a mission-start/arming permit.
class FleetMissionStartReadiness
{
public:
    enum class Phase { BeforeArm, ArmedGroundReady };
    static QVariantMap capture(Vehicle* vehicle, const QVariantMap& member,
        const QVariantMap& constraints, const QVariantMap& payload,
        FleetFirmwareAdapterCatalog* catalog = nullptr);
    // Typed native phase facts only. They do not bypass manual-arm risk or
    // authorize commands; actual startup authority/transactions are separate.
    static QVariantMap captureForStartup(Vehicle* vehicle, const QVariantMap& member,
        const QVariantMap& constraints, const QVariantMap& payload, Phase phase,
        FleetFirmwareAdapterCatalog* catalog = nullptr);
};

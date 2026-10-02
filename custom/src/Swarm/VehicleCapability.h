#pragma once

#include <QDateTime>
#include <QStringList>
#include <QVariantMap>

/// Capability and runtime summary advertised by one vehicle in the fleet.
///
/// This is deliberately independent from QGroundControl's Vehicle class.  The
/// registry can therefore be populated from MAVLink, a replay, or a future
/// backend without coupling the swarm model to a particular transport.
struct VehicleCapability
{
    int systemId = 0;
    QString vehicleType;
    QString autopilotType;
    QString firmwareVersion;
    QStringList capabilities;

    double maxSpeedMps = -1.0;
    double enduranceSeconds = -1.0;
    bool canUseGnss = false;
    bool canUseRtk = false;
    bool canUseVision = false;
    bool canUseRangeSensor = false;
    bool canRelay = false;

    QString role;
    bool online = false;
    QDateTime lastSeenUtc;

    // Runtime health values are -1 when the source has not reported one yet.
    double batteryPercent = -1.0;
    int linkQualityPercent = -1;
    bool estimatorHealthy = true;
    bool estimatorHealthKnown = false;
    bool positionHealthy = true;
    bool positionHealthKnown = false;
    bool hasFault = false;
    QString faultSummary;
    int healthScore = 0;
    QString healthState;
    QString healthReason;

    // Backend-specific fields are retained so adding a capability does not
    // require a schema change in the registry.
    QVariantMap metadata;

    QVariantMap toVariantMap() const;
    static VehicleCapability fromVariantMap(const QVariantMap& descriptor,
                                            int fallbackSystemId = 0);
};

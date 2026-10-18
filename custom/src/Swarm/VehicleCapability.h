#pragma once

#include <QDateTime>
#include <QString>
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

    // Static configuration evidence supplied by the vehicle/backend.  These
    // fields are deliberately separate from runtime health so a heartbeat
    // cannot make a modified airframe look like its approved baseline.
    QVariantMap configuration;
    QString configurationFingerprint;
    QString baselineConfigurationFingerprint;
    // Only "trusted_manifest" or an explicit local operator approval are
    // accepted as approval sources. A value echoed by vehicle telemetry is
    // intentionally treated as unverified.
    QString baselineSource;
    // unknown: no configuration evidence; unverified: evidence exists but no
    // approved baseline; baseline: evidence matches the approved baseline;
    // modified: evidence differs from the approved baseline.
    QString configurationState;

    // Runtime health values are -1 when the source has not reported one yet.
    double batteryPercent = -1.0;
    int linkQualityPercent = -1;
    bool estimatorHealthy = true;
    bool estimatorHealthKnown = false;
    bool positionHealthy = true;
    bool positionHealthKnown = false;
    bool hasFault = false;
    QString faultSummary;
    // Fault status is a separate evidence channel.  `hasFault=false` alone
    // must never be interpreted as a current no-fault observation when the
    // source stopped reporting.  The age is measured by the adapter's
    // observation layer; -1 means that freshness was not supplied.
    bool faultHealthKnown = false;
    int faultEvidenceAgeMs = -1;
    // Ground-station monotonic freshness anchor for the last paired fault
    // assessment. The adapter-reported age is advanced from this point on
    // every health refresh, so a one-time age=0 report cannot stay fresh
    // forever when only heartbeats continue.
    QDateTime faultEvidenceObservedUtc;
    int healthScore = 0;
    QString healthState;
    QString healthReason;

    // Backend-specific fields are retained so adding a capability does not
    // require a schema change in the registry.
    QVariantMap metadata;

    QVariantMap toVariantMap() const;
    /// Compute a deterministic SHA-256 fingerprint for a configuration map.
    /// Keys are sorted recursively and runtime fields are not added by this
    /// helper, so the result is stable across transports and process restarts.
    static QString fingerprintForConfiguration(const QVariantMap& configuration);
    static VehicleCapability fromVariantMap(const QVariantMap& descriptor,
                                            int fallbackSystemId = 0);
};

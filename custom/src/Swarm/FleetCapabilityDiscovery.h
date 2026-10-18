#pragma once

#include <QObject>
#include <QVariantMap>

/// Normalizes identity and static evidence collected by a transport adapter
/// into the descriptor consumed by FleetRegistry.  This class deliberately
/// does not read MAVLink messages or send commands; a backend supplies the
/// already-decoded maps and a locally trusted manifest supplies the approved
/// baseline.
class FleetCapabilityDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit FleetCapabilityDiscovery(QObject* parent = nullptr);

    /// Build one registry descriptor from decoded transport evidence.
    /// `trustedManifest` is an operator/backend input, not vehicle telemetry.
    Q_INVOKABLE QVariantMap buildDescriptor(const QVariantMap& heartbeat,
                                            const QVariantMap& autopilotVersion = QVariantMap(),
                                            const QVariantMap& componentMetadata = QVariantMap(),
                                            const QVariantMap& parameterSnapshot = QVariantMap(),
                                            const QVariantMap& trustedManifest = QVariantMap()) const;

    /// Validate the invariants required before a descriptor is registered or
    /// used as a task-planning candidate. The result contains `valid`,
    /// `errors`, `warnings`, and the derived configuration state. Discovery
    /// evidence is resource-bounded so malformed adapters cannot grow a
    /// descriptor without limit.
    Q_INVOKABLE QVariantMap validateDescriptor(const QVariantMap& descriptor) const;
};

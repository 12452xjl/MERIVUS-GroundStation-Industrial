#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "VehicleCapability.h"

/// Registry and health summary for a heterogeneous MERIVUS fleet.
///
/// The registry intentionally accepts QVariantMap descriptors so it can be
/// fed by QML, SwarmController, MAVLink handlers, or a future MAVSDK backend.
/// It does not own QGroundControl Vehicle objects and does not send commands.
class FleetRegistry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int vehicleCount READ vehicleCount NOTIFY registryChanged)
    Q_PROPERTY(int onlineVehicleCount READ onlineVehicleCount NOTIFY registryChanged)
    Q_PROPERTY(int healthyVehicleCount READ healthyVehicleCount NOTIFY healthChanged)
    Q_PROPERTY(int offlineTimeoutMs READ offlineTimeoutMs WRITE setOfflineTimeoutMs NOTIFY offlineTimeoutMsChanged)
    Q_PROPERTY(QVariantList vehicles READ vehicles NOTIFY registryChanged)
    Q_PROPERTY(QVariantMap fleetHealthSummary READ fleetHealthSummary NOTIFY healthChanged)

public:
    explicit FleetRegistry(QObject* parent = nullptr);

    int vehicleCount() const { return _vehicles.size(); }
    int onlineVehicleCount() const;
    int healthyVehicleCount() const;
    int offlineTimeoutMs() const { return _offlineTimeoutMs; }
    QVariantList vehicles() const;
    QVariantMap fleetHealthSummary() const;

    void setOfflineTimeoutMs(int timeoutMs);

    /// Register or replace a vehicle descriptor.  The descriptor must contain
    /// a positive systemId (vehicleId/id are accepted as aliases).
    Q_INVOKABLE bool registerVehicle(const QVariantMap& descriptor);
    /// Update only fields present in updates; unknown fields are retained in
    /// metadata.  This makes telemetry updates safe for older clients.
    Q_INVOKABLE bool updateVehicle(int systemId, const QVariantMap& updates);
    Q_INVOKABLE bool unregisterVehicle(int systemId);
    Q_INVOKABLE bool containsVehicle(int systemId) const;
    Q_INVOKABLE QVariantMap vehicle(int systemId) const;
    Q_INVOKABLE QVariantList onlineVehicles() const;
    Q_INVOKABLE QVariantList vehiclesWithCapability(const QString& capability) const;
    Q_INVOKABLE void markSeen(int systemId, const QVariantMap& telemetry = QVariantMap());
    Q_INVOKABLE void refreshHealth();
    Q_INVOKABLE void clear();

signals:
    void registryChanged();
    void healthChanged();
    void vehicleAdded(int systemId);
    void vehicleUpdated(int systemId);
    void vehicleRemoved(int systemId);
    void vehicleHealthChanged(int systemId, const QString& state, int score);
    void offlineTimeoutMsChanged();

private slots:
    void _refreshHealthTimer();

private:
    VehicleCapability* _find(int systemId);
    const VehicleCapability* _find(int systemId) const;
    void _recomputeHealth(VehicleCapability& vehicle, const QDateTime& nowUtc);
    bool _mergeDescriptor(VehicleCapability& target, const QVariantMap& updates);

    QHash<int, VehicleCapability> _vehicles;
    int _offlineTimeoutMs = 3000;
    QTimer _healthTimer;
};

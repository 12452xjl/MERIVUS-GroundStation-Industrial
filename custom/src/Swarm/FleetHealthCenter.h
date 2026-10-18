#pragma once

#include <QPointer>
#include <QList>
#include <QObject>
#include <QMetaObject>
#include <QVariantList>
#include <QVariantMap>

#include "FaultToleranceManager.h"
#include "FleetRegistry.h"

/// Read-only fleet health aggregation and explainable flight/task gating.
///
/// FleetHealthCenter is deliberately an observation layer. It combines the
/// capability registry and the transport-independent fault policy, but never
/// sends MAVLink messages, changes vehicle state, or selects a command.
class FleetHealthCenter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FleetRegistry* fleetRegistry READ fleetRegistry WRITE setFleetRegistry
               NOTIFY sourcesChanged)
    Q_PROPERTY(FaultToleranceManager* faultToleranceManager READ faultToleranceManager
               WRITE setFaultToleranceManager NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantMap healthSummary READ healthSummary NOTIFY healthChanged)
    Q_PROPERTY(QVariantList vehicleHealth READ vehicleHealth NOTIFY healthChanged)

public:
    explicit FleetHealthCenter(QObject* parent = nullptr);

    FleetRegistry* fleetRegistry() const { return _fleetRegistry.data(); }
    FaultToleranceManager* faultToleranceManager() const { return _faultToleranceManager.data(); }
    QVariantMap healthSummary() const;
    QVariantList vehicleHealth() const;

    void setFleetRegistry(FleetRegistry* registry);
    void setFaultToleranceManager(FaultToleranceManager* manager);

    /// Re-read attached sources. This only refreshes their in-memory reports;
    /// it does not issue flight commands or write vehicle parameters.
    Q_INVOKABLE void refresh();

    /// Conservative preflight gate. Every selected vehicle must be online,
    /// have known estimator/position health, and not report a fault.
    Q_INVOKABLE QVariantMap evaluateTakeoffGate(const QVariantList& vehicleIds) const;

    /// Task gate with the same vehicle checks plus an optional leader check.
    /// The return value always contains machine-readable codes and human-
    /// readable reasons so QML can explain why a task is held.
    Q_INVOKABLE QVariantMap evaluateTaskGate(const QVariantList& vehicleIds,
                                             bool requireLeader = true) const;

signals:
    void sourcesChanged();
    void healthChanged();

private slots:
    void _sourceChanged();

private:
    QVariantMap _evaluateGate(const QVariantList& vehicleIds, bool requireLeader) const;
    static int _readVehicleId(const QVariantMap& value);
    static void _mergeMissing(QVariantMap& target, const QVariantMap& source);
    void _disconnectSources();
    void _connectSources();

    QPointer<FleetRegistry> _fleetRegistry;
    QPointer<FaultToleranceManager> _faultToleranceManager;
    QList<QMetaObject::Connection> _sourceConnections;
};

#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <memory>

#include "LinkInterface.h"

class FleetLinkLatencyRun;
class FleetOperatorSession;
class FleetRegistry;
class FleetRolePolicy;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// One explicit targeted TIMESYNC-v2 diagnostic, not a stream configuration,
// remote identity/clock proof, pure-network delay, or execution permission.
class FleetLinkLatencyCapture final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetLinkLatencyCapture() override;
    Q_INVOKABLE QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestCapture(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, int vehicleId, bool confirmed);
    Q_INVOKABLE bool cancel(const QString& requestId);
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
    Q_INVOKABLE QVariantMap latencyReview(int vehicleId) const;
signals:
    void changed();
private:
    friend class SwarmController;
    friend class Vehicle;
    FleetLinkLatencyCapture(SwarmController* actualSwarm, FleetRegistry* actualRegistry);
    bool nativeVehicleMatches(const Vehicle* actualVehicle) const;
    bool nativeResponseCurrent(const std::shared_ptr<FleetLinkLatencyRun>& run) const;
    bool memoryCurrent(quint64 generation) const;
    bool gate(quint64 generation);
    bool publish(quint64 generation);
    bool watchSources(quint64 generation);
    void disconnectSources();
    void tick();
    void finish(quint64 generation, const QString& state, const QString& reason);
    static QVariantMap result(bool accepted, const QString& reason, const QString& requestId);

    QPointer<SwarmController> _swarm;
    QPointer<FleetRegistry> _registry;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    QPointer<Vehicle> _vehicle;
    QPointer<VehicleLinkManager> _links;
    QPointer<LinkInterface> _primary;
    WeakLinkInterfacePtr _primaryOwner;
    QString _recordEpoch, _bindingEpoch, _scopeToken, _requestId;
    QString _state = QStringLiteral("empty"), _reason;
    QVariantMap _identity, _role, _observation;
    std::shared_ptr<FleetLinkLatencyRun> _run;
    quint64 _generation = 0;
    qint64 _deadlineMs = -1;
    int _vehicleId = 0;
    bool _active = false, _busy = false, _checking = false;
    bool _reentered = false, _destroying = false;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};

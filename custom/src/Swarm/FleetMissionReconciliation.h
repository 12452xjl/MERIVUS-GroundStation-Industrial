#pragma once

#include <QObject>
#include <QPointer>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QList>
#include <QMetaObject>
#include "LinkInterface.h"

class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetOperatorSession;
class FleetRolePolicy;
class MissionManager;
class ParameterManager;
class PlanManager;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// Deployment-owned diagnostic READ workflow. Never a StartContentCheck,
// approval producer, shared cache writer, or transferable controller lease.
class FleetMissionReconciliation final : public QObject
{
    Q_OBJECT
signals:
    void changed();
private:
    friend class FleetMissionDeployment;
    friend class PlanManager;
    explicit FleetMissionReconciliation(FleetMissionDeployment* owner);
    ~FleetMissionReconciliation() override;
    QVariantMap current() const;
    QVariantMap request(QObject* sessionObject, QObject* roleObject, QObject* storeObject,
        QObject* swarmObject, QObject* authorizationOwner, const QString& deploymentId, bool confirmed);
    bool cancel(const QString& requestId);
    bool busy() const;
    bool nativeReadContextCurrent(PlanManager* manager, Vehicle* vehicle, LinkInterface* primary,
        const QString& scopeId, quint64 operationId) const;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> manager;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        QVariantMap payload;
        QString versionEpoch;
        QString parameterEpoch;
        QString managerEpoch;
        QString scopeId;
        quint64 operationId = 0;
        int maximumItemCount = 0;
        qint64 deadlineMs = -1;
        bool completionSeen = false;
        bool completionSuccess = false;
        QString state = QStringLiteral("queued");
        QString reason;
        int observedCount = -1;
        QString observedHash;
        QString rawHash;
        QString comparisonBasis = QStringLiteral("none");
        qint64 observedAt = -1;
    };
    bool _same(quint64 generation) const;
    bool _owned() const;
    bool _globalNativeCurrent() const;
    bool _targetSourceCurrent(const Target& target) const;
    bool _targetReadable(const Target& target, bool ownRead) const;
    bool _gate(quint64 generation);
    bool _publish(quint64 generation);
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _tick();
    void _advanceUnknown(const QString& reason);
    void _finish(const QString& state, const QString& reason);
    void _drain();
    void _detachDrain();
    bool _captureResult(const QVariantMap& result, Target* target);
    QPointer<FleetMissionDeployment> _owner;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<SwarmController> _swarm;
    QPointer<QObject> _authorization;
    QVariantMap _identity;
    QVariantMap _role;
    QVariantMap _binding;
    QVariantMap _record;
    QVariantList _vehicleIds;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    const QString _runEpoch;
    QString _requestId;
    QString _deploymentId;
    QString _deploymentState;
    QString _state;
    QString _reason;
    quint64 _generation = 0;
    quint64 _deploymentGeneration = 0;
    int _targetIndex = 0;
    int _expectedTotal = 0;
    int _observedTotal = 0;
    qint64 _startedAt = -1;
    bool _active = false;
    bool _reservationHeld = false;
    bool _transportDraining = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _dispatching = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

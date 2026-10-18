#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetOperatorSession;
class FleetRolePolicy;
class LinkInterface;
class MissionManager;
class ParameterManager;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// An explicit, bounded, complete RAW INT read. Not a start permit, remote
// atomic snapshot, authenticated aircraft identity, or command dispatcher.
class FleetMissionStartContentCheck : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionStartContentCheck(QObject* parent = nullptr);
    ~FleetMissionStartContentCheck() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestCheck(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* deploymentObject, QObject* swarmObject,
        QObject* authorizationOwner, const QString& expectedDeploymentId, bool operatorConfirmed);
    Q_INVOKABLE bool cancel(const QString& requestId);

    // Native-only Review integration. Caller maps never establish evidence.
    QVariantMap reviewContext(const QString& checkId, const QString& deploymentId);
    bool reviewContextCurrent(const QString& checkId, const QString& deploymentId,
        const QVariantMap& expectedContext);
    Vehicle* reviewVehicle(const QString& checkId, const QString& deploymentId, int vehicleId) const;
    MissionManager* reviewManager(const QString& checkId, const QString& deploymentId, int vehicleId) const;
    LinkInterface* reviewPrimaryLink(const QString& checkId, const QString& deploymentId, int vehicleId) const;
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        QVariantMap payload;
        QVariantMap receipt;
        QVariantMap canonical;
        QString versionEpoch;
        QString parameterEpoch;
        QString managerEpoch;
        QString readScopeId;
        QString baselineSequence;
        quint64 operationId = 0;
        QString state = QStringLiteral("queued");
        QString reason;
    };
    bool _live(quint64 generation) const;
    bool _targetCurrent(const Target& target, quint64 generation, bool allowOwnRead) const;
    bool _receiptCurrent(const Target& target, quint64 generation, QVariantMap* receipt) const;
    bool _gate(quint64 generation, bool requireComplete = false);
    bool _deploymentCurrent(quint64 generation, bool requireIdle) const;
    bool _publish(quint64 generation);
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _tick();
    void _finish(quint64 generation, const QString& state, const QString& reason);
    void _drain();
    void _detachDrain();
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<SwarmController> _swarm;
    QPointer<QObject> _authorization;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QVariantMap _deploymentContext;
    QVariantMap _binding;
    QVariantList _vehicleIds;
    QByteArray _recordBytes;
    QByteArray _deploymentBytes;
    QString _checkId;
    QString _checkEpoch;
    QString _deploymentId;
    QString _sourceFingerprint;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _createdAtMs = -1;
    qint64 _targetDeadlineMs = -1;
    qint64 _lastRequestAtMs = -1;
    qint64 _verifiedUntilMs = -1;
    int _targetIndex = 0;
    bool _active = false;
    bool _reservationHeld = false;
    bool _transportDraining = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _issuedAnyRequest = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

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
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetFirmwareAdapterCatalog;
class FleetHealthCenter;
class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
class FleetMissionStartApproval;
class FleetMissionStartContentCheck;
class FleetMissionStartReview;
class FleetMissionStartupTransaction;
class FleetMissionStartupClearanceWitness;
class FleetOperatorSession;
class FleetRegistry;
class FleetRolePolicy;
class FaultToleranceManager;
class LinkInterface;
class MissionManager;
class ParameterManager;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// Takes ownership of actual finite two-account/source evidence. This is NOT
// the final startup permit, an ARM/start adapter, or a remote control lease.
class FleetMissionStartupAuthority : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionStartupAuthority(QObject* parent = nullptr);
    ~FleetMissionStartupAuthority() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestAuthority(const QVariantMap& actualSources,
        const QString& reviewId, bool operatorConfirmed);
    Q_INVOKABLE bool revokeAuthority(const QString& startupId);

    QString authorityEpoch() const { return _authorityEpoch; }
    bool authenticationTransferContextCurrent(const QString& startupId, const QString& sourceHash,
        QObject* actualStartApproval, const QVariantMap& conditionalGrant);
    QVariantMap authorityContext(const QString& startupId);
    bool authorityContextCurrent(const QString& startupId, const QVariantMap& expectedContext);
    Vehicle* authorityVehicle(const QString& startupId, int vehicleId) const;
    MissionManager* authorityManager(const QString& startupId, int vehicleId) const;
    LinkInterface* authorityPrimaryLink(const QString& startupId, int vehicleId) const;
    QVariantMap actualSourcesForStartupTransaction(const QString& startupId);
    QVariantMap transferToStartupTransaction(QObject* transactionObject, const QVariantMap& authorityContext,
        const QString& startupId, const QString& sourceHash, qint64 maximumRemainingMs);
    FleetFirmwareAdapterCatalog* nativeFirmwareCatalog(const QString& startupId);
    FleetMissionReviewCoordinator* nativeReviewCoordinator(const QString& startupId);
    FleetRegistry* nativeRegistry(const QString& startupId);
    FaultToleranceManager* nativeFaultPolicy(const QString& startupId);
    // Same native review witness, never installed from a portable map.
    std::shared_ptr<FleetMissionStartupClearanceWitness> startupClearanceWitness(const QString& startupId);
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
        QVariantMap member;
        QVariantMap payload;
        QVariantMap readReceipt;
        QVariantMap readiness;
        QString readinessFingerprint;
    };
    bool _live(quint64 generation) const;
    bool _targetsCurrent(quint64 generation) const;
    bool _gate(quint64 generation, bool requireAuthentication);
    void _invalidate(quint64 generation, const QString& reason, const QString& state = QStringLiteral("stale"));
    void _connectSources(quint64 generation);
    void _disconnectSources();
    QVariantMap _bundle() const;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartReview> _review;
    QPointer<FleetMissionStartApproval> _approval;
    QPointer<FleetMissionStartContentCheck> _content;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<SwarmController> _swarm;
    QPointer<FleetHealthCenter> _health;
    QPointer<QObject> _authorization;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetRegistry> _registry;
    QPointer<FaultToleranceManager> _faultPolicy;
    QPointer<FleetMissionStartupTransaction> _transferredTransaction;
    QString _transferredTransactionEpoch;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _reviewContext;
    QVariantMap _contentContext;
    QVariantMap _deploymentContext;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QVariantMap _conditionalGrant;
    QVariantMap _authenticationReceipt;
    QVariantMap _binding;
    QVariantMap _constraints;
    QVariantMap _healthGate;
    QVariantMap _catalogSnapshot;
    std::shared_ptr<FleetMissionStartupClearanceWitness> _clearanceWitness;
    bool _clearanceRequired = false;
    QByteArray _recordBytes;
    QByteArray _deploymentBytes;
    QVariantList _vehicleIds;
    QString _authorityEpoch;
    QString _startupId;
    QString _sourceHash;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _expiresAtMs = -1;
    int _leaderId = 0;
    bool _requireLeader = false;
    bool _leaderRegistered = false;
    bool _reservationHeld = false;
    bool _reservationTransferred = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

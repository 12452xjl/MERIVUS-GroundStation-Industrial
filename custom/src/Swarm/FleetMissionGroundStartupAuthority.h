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
class FleetMissionArmedGroundReview;
class FleetMissionDeployment;
class FleetMissionGroundStartApproval;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
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

// One NEW native conditional basis, one actual independent owner and scope.
// This class neither restores old authority nor sends a flight command.
class FleetMissionGroundStartupAuthority : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionGroundStartupAuthority(QObject* parent = nullptr);
    ~FleetMissionGroundStartupAuthority() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestAuthority(const QVariantMap& actualSources,
        const QString& reviewId, bool operatorConfirmed);
    Q_INVOKABLE bool revokeAuthority(const QString& startupId);
    QString authorityEpoch() const { return _authorityEpoch; }
    // PURE ownership/transfer context, never a recursive OS/source gate.
    bool authenticationTransferContextCurrent(const QString& startupId, const QString& sourceHash,
        QObject* actualApproval, const QVariantMap& exactAuthenticationBasis);
    QVariantMap authorityContext(const QString& startupId);
    bool authorityContextCurrent(const QString& startupId, const QVariantMap& exactContext);
    QVariantMap actualSourcesForStartupTransaction(const QString& startupId);
    QVariantMap transferToStartupTransaction(QObject* actualTransaction, const QVariantMap& exactContext,
        const QString& startupId, const QString& sourceHash, qint64 maximumRemainingMs);
    // Actual pending source context for Session; PURE memory, not a gate or
    // permission, and unavailable outside this exact native transfer stack.
    QVariantMap pendingStartupTransactionContext(QObject* actualTransaction, const QString& startupId,
        const QString& transactionEpoch, const QString& sourceHash) const;
    FleetFirmwareAdapterCatalog* nativeFirmwareCatalog(const QString& startupId);
    FleetMissionReviewCoordinator* nativeReviewCoordinator(const QString& startupId);
    FleetRegistry* nativeRegistry(const QString& startupId);
    FaultToleranceManager* nativeFaultPolicy(const QString& startupId);
    FleetMissionArmedGroundReview* nativeGroundReview(const QString& startupId);
    FleetMissionStartupTransaction* nativePreviousStartup(const QString& startupId);
    // Same native review witness, never installed from a portable map.
    std::shared_ptr<FleetMissionStartupClearanceWitness> startupClearanceWitness(const QString& startupId);
    Vehicle* authorityVehicle(const QString& startupId, int vehicleId) const;
    MissionManager* authorityManager(const QString& startupId, int vehicleId) const;
    LinkInterface* authorityPrimaryLink(const QString& startupId, int vehicleId) const;
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        bool armed = false;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        QVariantMap member;
        QVariantMap payload;
        QVariantMap readReceipt;
        QVariantMap readiness;
        QString versionEpoch;
        QString parameterEpoch;
        QString flightEpoch;
    };
    bool _live(quint64 generation) const;
    bool _same(quint64 generation) const;
    bool _targetsCurrent(quint64 generation) const;
    bool _gate(quint64 generation, bool requireAuthentication);
    void _invalidate(quint64 generation, const QString& reason, const QString& state = QStringLiteral("stale"));
    void _connectSources(quint64 generation);
    void _disconnectSources();
    QVariantMap _bundle() const;
    QVariantMap _context() const;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionArmedGroundReview> _review;
    QPointer<FleetMissionGroundStartApproval> _approval;
    QPointer<FleetMissionStartupTransaction> _previous;
    QPointer<FleetMissionStartupTransaction> _pendingTransaction;
    QPointer<FleetMissionStartupTransaction> _transferredTransaction;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<SwarmController> _swarm;
    QPointer<FleetHealthCenter> _health;
    QPointer<QObject> _authorization;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetRegistry> _registry;
    QPointer<FaultToleranceManager> _faults;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _reviewContext;
    QVariantMap _binding;
    QVariantMap _constraints;
    QVariantMap _identity;
    QVariantMap _role;
    QVariantMap _healthGate;
    QVariantMap _catalogSnapshot;
    QVariantMap _grant;
    QVariantMap _authentication;
    QVariantMap _transferContext;
    std::shared_ptr<FleetMissionStartupClearanceWitness> _clearanceWitness;
    bool _clearanceRequired = false;
    QVariantList _vehicleIds;
    QByteArray _recordBytes;
    QString _startupId;
    QString _authorityEpoch;
    QString _sourceHash;
    QString _pendingTransactionEpoch;
    QString _transferredTransactionEpoch;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _expiresAtMs = -1;
    bool _active = false;
    bool _reservationHeld = false;
    bool _reservationTransferred = false;
    bool _transportDraining = false;
    bool _transferInProgress = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _requireLeader = false;
    int _leaderId = 0;
    bool _leaderRegistered = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

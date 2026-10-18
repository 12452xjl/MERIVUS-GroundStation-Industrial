#pragma once

#include "FleetMissionStartupGeographicMotion.h"
#include "LinkInterface.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetFirmwareAdapterCatalog;
class FleetHealthCenter;
class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
class FleetMissionStartupClearanceWorker;
class FleetMissionStartupClearanceWitness;
class FleetMissionStartupTransaction;
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

// A NEW explicitly requested complete read and risk acknowledgement after a
// stopped, drained, never-MODE-queued startup. No old proof is restored, no
// aircraft is disarmed/armed and no mode/start/rollback command is released.
class FleetMissionArmedGroundReview : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap riskSummary READ riskSummary NOTIFY changed)
public:
    explicit FleetMissionArmedGroundReview(QObject* parent = nullptr);
    ~FleetMissionArmedGroundReview() override;
    QVariantMap current() const;
    // Bounded, private-artifact-derived display only; never an admission,
    // identity receipt, raw mission payload or refreshed physical observation.
    QVariantMap riskSummary() const;
    Q_INVOKABLE QVariantMap requestReview(const QVariantMap& actualSources,
        const QString& previousStartupId, bool operatorConfirmed);
    Q_INVOKABLE bool acknowledgeReview(const QString& reviewId, bool operatorConfirmed);
    Q_INVOKABLE bool cancel(const QString& reviewId);
    QString reviewEpoch() const { return _reviewEpoch; }

    // Native scope ownership only. These pure retained getters neither
    // authenticate a person nor substitute for each mandatory send fence.
    bool retainedPreviousSourceContextMatches(QObject* actualPreviousTransaction,
        const QString& previousStartupId, const QVariantMap& exactContext) const;
    QString readScopeForTarget(Vehicle* actualVehicle) const;
    bool nativeReadAdmissionCurrent(QObject* actualVehicle, QObject* actualManager,
        QObject* actualPrimary, const QString& readScopeId);
    QVariantMap reviewContext(const QString& reviewId);
    bool reviewContextCurrent(const QString& reviewId, const QVariantMap& exactContext);
    // Exact acknowledged native review sources for the NEW independent
    // account ledger. These are not a restored old authority or dispatcher.
    QVariantMap actualSourcesForStartApproval(const QString& reviewId);
    Vehicle* reviewVehicle(const QString& reviewId, int vehicleId) const;
    MissionManager* reviewManager(const QString& reviewId, int vehicleId) const;
    LinkInterface* reviewPrimaryLink(const QString& reviewId, int vehicleId) const;
    FleetFirmwareAdapterCatalog* nativeReviewFirmwareCatalog(const QString& reviewId) const;
    FleetMissionReviewCoordinator* nativeReviewCoordinator(const QString& reviewId) const;
    FleetRegistry* nativeReviewRegistry(const QString& reviewId) const;
    FaultToleranceManager* nativeReviewFaultPolicy(const QString& reviewId) const;
    std::shared_ptr<FleetMissionStartupClearanceWitness> startupClearanceWitness(const QString& reviewId);
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
        WeakLinkInterfacePtr primaryOwner;
        QVariantMap parameterSnapshot;
        QVariantMap member;
        QVariantMap payload;
        QVariantMap receipt;
        QVariantMap canonical;
        QVariantMap physicalBaseline;
        QVariantMap reviewReadiness;
        QString versionEpoch;
        QString parameterEpoch;
        QString managerEpoch;
        QString readScopeId;
        QString baselineSequence;
        QString state = QStringLiteral("queued");
        QString reason;
        quint64 operationId = 0;
        qint64 readStartedAtMs = -1;
    };
    bool _live(quint64 generation) const;
    bool _same(quint64 generation) const;
    bool _targetCurrent(const Target& target, bool allowOwnedRead, bool captureRisk) const;
    bool _receiptCurrent(const Target& target, quint64 generation, QVariantMap* receipt) const;
    bool _gate(quint64 generation, bool requireComplete = false);
    bool _captureRisk(quint64 generation);
    bool _beginClearanceCalculation(quint64 generation);
    void _clearanceCalculationFinished(FleetMissionStartupClearanceWorker* worker,
        quint64 generation, const QString& reviewId, const QString& reviewEpoch,
        const std::shared_ptr<std::atomic_bool>& cancellation);
    void _acceptClearanceCalculation(quint64 generation);
    void _retireClearanceCalculation();
    bool _captureClearanceSource(quint64 generation);
    bool _clearanceSourceMemoryCurrent(quint64 generation);
    bool _executionParametersMemoryCurrent() const;
    void _tick();
    void _finish(quint64 generation, const QString& state, const QString& reason);
    void _drain();
    void _detachDrain();
    void _connectSources(quint64 generation);
    void _disconnectSources();
    bool _publish(quint64 generation);
    QVariantMap _bundle() const;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartupTransaction> _previous;
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
    QVariantMap _previousContext;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QVariantMap _binding;
    QVariantMap _constraints;
    QVariantMap _healthGate;
    QVariantMap _catalogSnapshot;
    QVariantMap _artifact;
    QVariantMap _recordReviewEvidence;
    QVariantMap _clearanceSourcePlan;
    QVariantMap _clearanceSourceEvidence;
    QVariantMap _clearanceDatumEvidence;
    qint64 _clearanceLastUtcMs = -1;
    qint64 _clearanceValidUntilUtcMs = -1;
    qint64 _clearanceDeadlineMs = -1;
    bool _clearanceSourceFrozen = false;
    bool _clearanceSourceProvided = false;
    bool _clearanceSourceInvalidated = false;
    bool _clearanceSourceChecking = false;
    QPointer<FleetMissionStartupClearanceWorker> _clearanceWorker;
    std::shared_ptr<std::atomic_bool> _clearanceCancellation;
    std::shared_ptr<std::atomic_bool> _clearanceWorkerFinished;
    QVariantMap _clearanceWorkerInput;
    FleetMissionStartupGeographicMotion::Models _clearanceGeographicModels;
    FleetMissionStartupGeographicMotion::Models _clearanceCompletedModels;
    QVariantMap _clearanceCompletedResult;
    std::shared_ptr<FleetMissionStartupClearanceWitness> _startupClearanceWitness;
    bool _clearanceWorkerActive = false;
    bool _clearanceCompletionPending = false;
    QVariantList _vehicleIds;
    QByteArray _recordBytes;
    QString _previousStartupId;
    QString _reviewId;
    QString _reviewEpoch;
    QString _sourceFingerprint;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    int _targetIndex = 0;
    qint64 _expiresAtMs = -1;
    qint64 _targetDeadlineMs = -1;
    qint64 _lastRequestAtMs = -1;
    qint64 _reviewExpiresAtMs = -1;
    bool _active = false;
    bool _reservationHeld = false;
    bool _transportDraining = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _requireLeader = false;
    bool _issuedAnyRequest = false;
    int _leaderId = 0;
    bool _leaderRegistered = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

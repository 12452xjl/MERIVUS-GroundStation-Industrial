#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QGeoCoordinate>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUuid>
#include <QVariantList>
#include <QVariantMap>

#include "MAVLinkProtocol.h"
#include "CommandTransaction.h"
#include "FleetRegistry.h"
#include "FleetCapabilityDiscovery.h"

class Vehicle;
class MissionItem;
class MissionManager;
class LinkInterface;
class SwarmMissionOrchestrator;
class FleetMissionStartupTransaction;
class FleetOccupancyJournalWorker;
class FleetOccupancyRecoveryReview;
class FleetCapabilityReview;
class FleetOptionalTelemetryCapture;
class FleetOptionalTelemetryConfiguration;
class FleetLinkLatencyCapture;
class FleetCameraStreamCapture;
class FleetCameraStreamControl;
class FleetFormationQueue;

/// MERIVUS 人工多机调度入口。受信任的 GuidedActionsController 约定通过显式
/// 选择和确认调用本类，AI proposal 不接入这里；该约定不是 C++ 权限边界。
/// 返回结果携带 transactionId，必须继续观察事务 ACK/遥测状态，不能把“已下发”
/// 误认为“飞行器已完成动作”。
class SwarmController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FleetRegistry* fleetRegistry READ fleetRegistry NOTIFY fleetRegistryChanged)
    Q_PROPERTY(QObject* capabilityReview READ capabilityReview NOTIFY capabilityReviewChanged)
    Q_PROPERTY(QObject* optionalTelemetryCapture READ optionalTelemetryCapture NOTIFY optionalTelemetryCaptureChanged)
    Q_PROPERTY(QObject* optionalTelemetryConfiguration READ optionalTelemetryConfiguration NOTIFY optionalTelemetryConfigurationChanged)
    Q_PROPERTY(QObject* linkLatencyCapture READ linkLatencyCapture NOTIFY linkLatencyCaptureChanged)
    Q_PROPERTY(QObject* cameraStreamCapture READ cameraStreamCapture NOTIFY cameraStreamCaptureChanged)
    Q_PROPERTY(QObject* cameraStreamControl READ cameraStreamControl NOTIFY cameraStreamControlChanged)
    // Local waiting intentions, not a flight queue or a remote control lease.
    Q_PROPERTY(QObject* formationQueue READ formationQueue NOTIFY formationQueueChanged)
    Q_PROPERTY(FleetCapabilityDiscovery* capabilityDiscovery READ capabilityDiscovery CONSTANT)
    Q_PROPERTY(bool swarmModeEnabled READ swarmModeEnabled CONSTANT)
    Q_PROPERTY(bool temporaryMissionExecutionEnabled READ temporaryMissionExecutionEnabled CONSTANT)
    Q_PROPERTY(bool formationActive READ formationActive NOTIFY formationActiveChanged)
    Q_PROPERTY(bool formationBusy READ formationBusy NOTIFY formationBusyChanged)
    Q_PROPERTY(QString formationStatus READ formationStatus NOTIFY formationStatusChanged)
    Q_PROPERTY(QVariantList transactions READ transactions NOTIFY transactionsChanged)
    Q_PROPERTY(QVariantMap continuityOccupancyPersistence READ continuityOccupancyPersistence NOTIFY continuityOccupancyPersistenceChanged)
    Q_PROPERTY(QObject* continuityRecovery READ continuityRecovery CONSTANT)
    // Internal adapter association only, assigned once; not a flight permit.
    Q_PROPERTY(QObject* executionTaskOrchestrator READ executionTaskOrchestrator WRITE setExecutionTaskOrchestrator NOTIFY executionTaskOrchestratorChanged)
    Q_PROPERTY(QVariantMap protectedExecutionReportCatalog READ protectedExecutionReportCatalog NOTIFY protectedExecutionReportCatalogChanged)

public:
    explicit SwarmController(QObject* parent = nullptr);
    ~SwarmController() override;

    FleetRegistry* fleetRegistry() const { return _fleetRegistry.data(); }
    QObject* capabilityReview() const;
    // Actual local source pairing only, not an account or flight grant.
    Q_INVOKABLE bool ownsCapabilityReview(QObject* actualReview, QObject* actualRegistry) const;
    QObject* optionalTelemetryCapture() const;
    Q_INVOKABLE bool ownsOptionalTelemetryCapture(QObject* actualCapture, QObject* actualRegistry) const;
    QObject* optionalTelemetryConfiguration() const;
    Q_INVOKABLE bool ownsOptionalTelemetryConfiguration(QObject* actualConfiguration, QObject* actualRegistry,
        QObject* actualCapture) const;
    // Native actual Registry binding only. Not invokable: no imported map or
    // caller-selected Vehicle can establish the source for an active query.
    Vehicle* optionalTelemetryCaptureVehicle(QObject* actualCapture, int systemId,
        QString* recordEpoch, QString* bindingEpoch) const;
    QObject* linkLatencyCapture() const;
    Q_INVOKABLE bool ownsLinkLatencyCapture(QObject* actualCapture, QObject* actualRegistry) const;
    // This independent owner cannot borrow the interval capture's qualification.
    Vehicle* linkLatencyCaptureVehicle(QObject* actualCapture, int systemId,
        QString* recordEpoch, QString* bindingEpoch) const;
    QObject* cameraStreamCapture() const;
    Q_INVOKABLE bool ownsCameraStreamCapture(QObject* actualCapture, QObject* actualRegistry) const;
    // The camera READ owns an independent actual source pairing; no caller
    // Vehicle/map or interval/RTT/configuration owner can supply this bridge.
    Vehicle* cameraStreamCaptureVehicle(QObject* actualCapture, int systemId,
        QString* recordEpoch, QString* bindingEpoch) const;
    QObject* cameraStreamControl() const;
    QObject* formationQueue() const;
    Q_INVOKABLE bool ownsCameraStreamControl(QObject* actualControl, QObject* actualRegistry, QObject* actualCapture) const;
    FleetCapabilityDiscovery* capabilityDiscovery() const { return _capabilityDiscovery; }
    QObject* executionTaskOrchestrator() const;
    void setExecutionTaskOrchestrator(QObject* actualOrchestrator);
    SwarmMissionOrchestrator* executionTaskOrchestratorBackend() const;
    // Narrow historical read only: exact source/task/revision/token selection,
    // no caller observation, live owner, file, control or retirement input.
    QVariantMap protectedExecutionReportCatalog() const;
    Q_INVOKABLE QVariantMap captureProtectedExecutionReport(const QVariantMap& request, bool confirmed) const;

    Q_INVOKABLE QVariantMap executeGoto(const QVariantList& selectedVehicleIds, const QVariant& targetCoordinate);
    Q_INVOKABLE QVariantMap executeQueuedGoto(const QVariantList& selectedVehicleIds,
                                              const QVariantList& queuedCoordinates,
                                              const QVariantList& referenceCoordinates,
                                              bool replaceExisting);
    Q_INVOKABLE QVariantMap executeTakeoff(const QVariantList& selectedVehicleIds, double altitudeMeters);
    Q_INVOKABLE QVariantMap executeLand(const QVariantList& selectedVehicleIds);
    Q_INVOKABLE QVariantMap executeRTL(const QVariantList& selectedVehicleIds);
    /// Starts an already uploaded mission only after a per-vehicle transaction
    /// observes mission-start acceptance or Mission flight-mode telemetry.
    /// Capture the per-vehicle MissionManager snapshot used by a confirmation
    /// UI. The returned fingerprints are evidence locks, not a replacement
    /// for Vehicle/PX4 authority.
    Q_INVOKABLE QVariantMap missionStartSnapshot(const QVariantList& selectedVehicleIds) const;
    /// The optional snapshot values are operator/UI evidence and are echoed in
    /// the transaction result; when supplied they are rechecked before every
    /// actual startMission() call.
    Q_INVOKABLE QVariantMap executeStartMission(const QVariantList& selectedVehicleIds,
                                                int expectedMissionItemCount = -1,
                                                int expectedCurrentMissionIndex = -1,
                                                const QVariantMap& expectedSnapshotFingerprints = QVariantMap());
    Q_INVOKABLE bool hasActiveTemporaryMission(const QVariantList& selectedVehicleIds) const;
    Q_INVOKABLE QVariantMap sendStartCommand(const QVariantList& selectedVehicleIds);
    Q_INVOKABLE QVariantMap endFormationSession();

    bool swarmModeEnabled() const;
    bool temporaryMissionExecutionEnabled() const;
    bool formationActive() const { return _formationActive; }
    bool formationBusy() const;
    QString formationStatus() const { return _formationStatus; }
    QVariantList transactions() const;

    /// Returns the latest per-vehicle ACK/telemetry state for a command batch.
    /// A terminal transaction means the command was acknowledged or definitively
    /// failed; it does not imply that the physical manoeuvre has completed.
    Q_INVOKABLE QVariantMap transactionSummary(const QString& transactionId) const;
    /// Cancels a live transaction and prevents queued per-vehicle callbacks
    /// from sending. Temporary missions also enter the existing cleanup path;
    /// this never claims that an already-sent onboard action was reversed.
    Q_INVOKABLE bool cancelTransaction(const QString& transactionId,
                                       const QString& reason = QString());
    // Actual in-process local occupancy only. Empty means invalid/unknown,
    // never that aircraft or an onboard task is free.
    Q_INVOKABLE QVariantMap continuityOccupancySnapshot() const;
    // Last-poll diagnostic values only; no path, import, reset or resolve API.
    QVariantMap continuityOccupancyPersistence() const;
    QObject* continuityRecovery() const;

    // C++ Fleet deployment adapter only. A whole-set local reservation blocks
    // ordinary commands during grounded upload/readback. It is not a remote
    // control lease. Safety escapes explicitly revoke it and remain available.
    bool reserveMissionDeployment(const QString& owner, const QVariantList& vehicleIds);
    bool missionDeploymentOwns(const QString& owner, const QVariantList& vehicleIds) const;
    // Native same-thread whole-set ownership move only. It neither restores a
    // revoked/missing reservation nor grants a remote lease or flight permit.
    // No release/reserve gap or outward notification occurs during the move.
    bool transferMissionDeployment(const QString& previousOwner, const QString& nextOwner,
                                   const QVariantList& exactVehicleIds);
    void releaseMissionDeployment(const QString& owner);
    void noteMissionDeploymentWrite(const QString& owner, int vehicleId, quint64 operationId,
                                    MissionManager* expectedManager);

signals:
    void fleetRegistryChanged();
    void capabilityReviewChanged();
    void optionalTelemetryCaptureChanged();
    void optionalTelemetryConfigurationChanged();
    void linkLatencyCaptureChanged();
    void cameraStreamCaptureChanged();
    void cameraStreamControlChanged();
    void formationQueueChanged();
    void continuityOccupancyPersistenceChanged();
    void executionTaskOrchestratorChanged();
    void protectedExecutionReportCatalogChanged();
    void formationActiveChanged();
    void formationBusyChanged();
    void formationStatusChanged();
    void formationFault(const QString& message);
    void temporaryMissionCompleted(int vehicleId, bool clearError);
    void transactionsChanged();
    void transactionUpdated(const QString& transactionId);

private slots:
    void _receiveMessage(LinkInterface* link, mavlink_message_t message);
    void _checkFormationHealth();
    void _handleFormationCommandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode);
    void _handleCommandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode);
    void _handleFormationCommandTimeout();
    void _checkTemporaryMissionProgress();
    void _checkCommandTransactions();
    void _archiveTerminalTransactions();
    void _refreshFleetRegistry();
    void _pollContinuityJournal();

private:
    friend class SwarmMissionOrchestrator;
    friend class FleetMissionDeploymentEpisode;
    friend class FleetMissionStartupTransaction;
    friend class FleetOccupancyRecoveryReview;
    // Read only actual map occupancy, including safety-revoked owners. An
    // ineffective authority is not evidence that its local entries are gone.
    // Empty result means unknown/invalid, not an unoccupied reservation.
    QVariantMap _missionDeploymentReservationState(const QString& owner,
                                                   const QVariantList& originalVehicleIds) const;
    bool _continuityAssignmentAllowed(const SwarmMissionOrchestrator* actual,
                                      const QVariantList& previous,
                                      const QVariantList& proposed,
                                      const QVariantMap& expectedIdentity) const;
    void _bumpContinuityOccupancyRevision();
    void _startContinuityJournal();
    QVariantMap _captureContinuityOccupancySnapshot() const;
    bool _continuityJournalStatusMatches(const QVariantMap& status, bool requireCommitted) const;
    QVariantMap _continuityRecoveryContext(const QVariantList& exactVehicleIds) const;
    bool _resolveContinuityRecovery(FleetOccupancyRecoveryReview* actualReview,
        const QVariantMap& expectedContext, const QVariantList& exactVehicleIds);
    void _reinstateContinuityRecoveryHold(int vehicleId);
    void _markContinuityUncertain(int vehicleId);
    void _markFormationContinuityUncertain();
    void _observeContinuityTransaction(const QString& transactionId,
                                       CommandTransaction* actualTransaction);
    void _watchContinuityTemporarySource(int vehicleId, Vehicle* actualVehicle,
                                         MissionManager* actualManager);
    struct ContinuityCommandStage {
        ContinuityCommandStage() = default;
        ContinuityCommandStage(const ContinuityCommandStage&) = delete;
        ContinuityCommandStage& operator=(const ContinuityCommandStage&) = delete;
        QPointer<SwarmController> owner;
        quint64 token = 0;
        ~ContinuityCommandStage();
    };
    quint64 _beginContinuityCommandStage(const QList<int>& originalVehicleIds);
    void _endContinuityCommandStage(quint64 token);
    struct FrozenCommandTarget {
        int vehicleId = 0;
        QPointer<Vehicle> vehicle;
    };
    static QList<FrozenCommandTarget> _freezeContinuityCommandTargets(const QList<Vehicle*>& vehicles);
    bool _continuityCommandTargetCurrent(const QString& transactionId,
                                         const FrozenCommandTarget& target) const;
    bool _continuityCommandTransactionCurrent(const QString& transactionId,
                                              CommandTransaction* actualTransaction) const;
    enum class TemporaryMissionStage {
        Uploading,
        Starting,
        Executing,
        Clearing,
    };

    enum class FormationPhase {
        Idle,
        Preparing,
        Committing,
        Releasing,
        Active,
        Aborting,
    };

    // 临时任务必须保留逐机阶段，避免把“上传完成”“执行完成”和“清理完成”混为一谈。
    struct TemporaryMissionState {
        QString sourceIdentity = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QPointer<Vehicle> vehicle;
        const QObject* vehicleIdentity = nullptr;
        QPointer<MissionManager> manager;
        const QObject* managerIdentity = nullptr; // Address only, never dereferenced.
        QGeoCoordinate finalCoordinate;
        int expectedMissionItemCount = 0;
        int finalMissionIndex = -1;
        TemporaryMissionStage stage = TemporaryMissionStage::Uploading;
        QString transactionId;
        qint64 missionStartRequestedAtMs = 0;
        int expectedUploadedMissionItemCount = 0;
        bool uploadAwaitingCompletion = false;
        quint64 uploadOperationId = 0;
        quint64 clearOperationId = 0;
        bool clearAwaitingCompletion = false;
        bool executionFailed = false;
        QString failureReason;
        bool superseded = false;
        bool completionReported = false;
    };

    QList<Vehicle*> _selectedVehicles(const QVariantList& selectedVehicleIds) const;
    QVariantMap _executeGotoInternal(const QVariantList& selectedVehicleIds,
                                     const QList<QGeoCoordinate>& coordinates,
                                     bool queued,
                                     const QList<QGeoCoordinate>& referenceCoordinates = QList<QGeoCoordinate>(),
                                     bool replaceExisting = false);
    // safetyEscape keeps FleetRegistry advisory blocks visible as warnings
    // for batch Land/RTL without suppressing a reachable safety action.
    QVariantMap _result(bool ok, const QString& message, const QList<int>& dispatchedIds = QList<int>(), const QList<int>& skippedIds = QList<int>(), const QString& transactionId = QString(), bool safetyEscape = false) const;
    QGeoCoordinate _coordinateFromVariant(const QVariant& value) const;
    bool _vehicleReady(Vehicle* vehicle) const;
    bool _vehicleTakeoffReady(Vehicle* vehicle, double altitudeMeters) const;
    bool _vehicleMissionStartReady(Vehicle* vehicle) const;
    void _watchFleetMissionQuarantine(int vehicleId, MissionManager* manager);
    void _checkFleetMissionQuarantineReplacement(int vehicleId, MissionManager* manager);
    bool _vehicleLandReady(Vehicle* vehicle) const;
    bool _vehicleRTLReady(Vehicle* vehicle) const;
    bool _vehicleFormationReady(Vehicle* vehicle) const;
    // Command-level health preflight fails closed when no live registry
    // snapshot exists or discovery is invalid. Optional fields inside an
    // otherwise valid snapshot remain explicit unknown evidence.
    bool _healthGateBlocks(int vehicleId, QString* reason = nullptr) const;
    bool _missionStartHealthGateBlocks(int vehicleId, QString* reason = nullptr) const;
    QVariantMap _healthGateSummary(const QList<int>& vehicleIds, bool safetyEscape = false) const;
    void _dispatchGoto(Vehicle* vehicle, const QGeoCoordinate& coordinate, int delayMs,
                       const QString& transactionId = QString(),
                       bool gotoModeRequested = false);
    void _dispatchTakeoff(Vehicle* vehicle, double altitudeMeters, int delayMs, const QString& transactionId = QString());
    void _dispatchLand(Vehicle* vehicle, int delayMs, const QString& transactionId = QString());
    void _dispatchRTL(Vehicle* vehicle, int delayMs, const QString& transactionId = QString());
    void _dispatchTemporaryMission(Vehicle* vehicle, const QList<QGeoCoordinate>& coordinates,
                                   int delayMs, bool replaceExisting,
                                   const QString& transactionId = QString());
    void _ensureTemporaryMissionConnections(Vehicle* vehicle);
    void _handleTemporaryMissionSendComplete(Vehicle* vehicle, quint64 operationId, bool error);
    void _handleTemporaryMissionRemoveAllComplete(Vehicle* vehicle, quint64 operationId, bool error);
    void _cancelTemporaryMissionForGoto(Vehicle* vehicle);
    void _requestTemporaryMissionClear(Vehicle* vehicle);
    bool _temporaryMissionUploadMatches(Vehicle* vehicle,
                                        const TemporaryMissionState& state) const;
    QList<MissionItem*> _buildTemporaryMissionItems(Vehicle* vehicle, const QList<QGeoCoordinate>& coordinates) const;
    double _relativeAltitudeMeters(Vehicle* vehicle, double fallbackMeters) const;
    void _ensureFormationForwardingConnected();
    void _ensureFormationCommandConnection(Vehicle* vehicle);
    // Delayed dispatches and formation ACK handlers must prove that the
    // transaction still owns this vehicle before sending or advancing state.
    bool _transactionVehicleCanDispatch(const QString& transactionId,
                                        int vehicleId) const;
    // Mark the exact point at which a transaction's command was handed to
    // Vehicle/MissionManager.  Telemetry observed before this point cannot be
    // used as confirmation for the transaction.
    bool _markTransactionDispatched(const QString& transactionId, int vehicleId);
    // A temporary mission has two distinct ownership points: upload and the
    // subsequent MAV_CMD_MISSION_START.  Keep a separate marker so an ACK
    // from an external mission start cannot be attributed to the upload.
    bool _markTemporaryMissionStartDispatched(const QString& transactionId,
                                               int vehicleId);
    bool _sendFormationCommand(MAV_CMD command,
                               const QList<int>& vehicleIds,
                               QList<int>* dispatchedIds = nullptr,
                               QList<int>* skippedIds = nullptr);
    void _beginFormationCommit();
    void _beginFormationRelease();
    void _beginFormationAbort(const QString& reason, bool operatorRequested = false);
    void _startFormationCommandTimer(FormationPhase expectedPhase, int timeoutMs);
    void _finishFormationIdle(const QString& status);
    void _setFormationPhase(FormationPhase phase);
    void _setFormationStatus(const QString& status);
    quint8 _memberMask(const QList<int>& vehicleIds) const;
    void _setFormationActive(bool active);
    void _pauseFormationFollowers();
    // Dry ownership check for a complete normal batch. Direct goto may
    // reserve a successor before superseding temporary missions, but never
    // overlap any other active normal or safety command.
    bool _normalCommandTargetsAvailable(const QList<int>& vehicleIds,
                                        bool allowTemporaryMissionPreemption = false) const;
    CommandTransaction* _createCommandTransaction(const QString& command,
                                                   const QList<int>& vehicleIds,
                                                   int timeoutMs,
                                                   int maxRetries = 0,
                                                   int expectedMavCommand = -1);
    void _ensureCommandConnection(Vehicle* vehicle);
    void _markTransactionFailed(const QString& transactionId, int vehicleId, const QString& reason);

    struct ManagedTransaction {
        QPointer<CommandTransaction> transaction;
        const QObject* transactionIdentity = nullptr;
        QList<int> originalVehicleIds;
        QHash<int, QPointer<Vehicle>> originalVehicles;
        QString operation;
        int expectedMavCommand = -1;
        QSet<int> dispatchStarted;
        QHash<int, quint64> dispatchEpoch;
        QHash<int, QString> dispatchBaselineFlightMode;
        QHash<int, qint64> dispatchStartedAtMs;
        QSet<int> temporaryMissionStartDispatched;
        QHash<int, quint64> temporaryMissionStartEpoch;
    };

    bool _formationForwardingConnected = false;
    bool _formationForwardingEnabled = false;
    bool _formationActive = false;
    FormationPhase _formationPhase = FormationPhase::Idle;
    QString _formationStatus;
    QList<int> _formationVehicleIds;
    QSet<int> _pendingFormationCommandIds;
    QSet<int> _successfulFormationCommandIds;
    QSet<int> _failedFormationCommandIds;
    QSet<int> _reportedLostFollowerIds;
    quint8 _formationMemberMask = 0;
    quint32 _formationSessionId = 0;
    QElapsedTimer _leaderGpsTimer;
    QTimer _formationWatchdogTimer;
    QTimer _formationCommandTimer;
    QHash<int, TemporaryMissionState> _temporaryMissions;
    QHash<int, QPointer<Vehicle>> _temporaryMissionConnections;
    QSet<int> _staleTemporaryMissionIds;
    QTimer _temporaryMissionTimer;
    QTimer _commandTransactionTimer;
    QPointer<FleetRegistry> _fleetRegistry;
    QPointer<FleetCapabilityReview> _capabilityReview;
    QPointer<FleetOptionalTelemetryCapture> _optionalTelemetryCapture;
    QPointer<FleetOptionalTelemetryConfiguration> _optionalTelemetryConfiguration;
    QPointer<FleetLinkLatencyCapture> _linkLatencyCapture;
    QPointer<FleetCameraStreamCapture> _cameraStreamCapture;
    QPointer<FleetCameraStreamControl> _cameraStreamControl;
    QPointer<FleetFormationQueue> _formationQueue;
    QPointer<SwarmMissionOrchestrator> _executionTaskOrchestrator;
    QString _protectedExecutionCatalogSourceId;
    bool _executionTaskOrchestratorAssigned = false;
    FleetCapabilityDiscovery* _capabilityDiscovery = nullptr;
    QTimer _fleetRegistryTimer;
    QHash<QString, ManagedTransaction> _managedTransactions;
    QHash<QString, QPointer<CommandTransaction>> _transactions;
    QHash<QString, quint64> _transactionCreationOrder;
    QHash<QString, QVariantMap> _missionStartEvidence;
    QHash<int, QString> _safetyEscapeOwners;
    QHash<int, QString> _missionDeploymentOwners;
    QSet<QString> _revokedMissionDeploymentOwners;
    const QString _continuitySwarmEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    quint64 _continuityOccupancyRevision = 1;
    bool _continuityOccupancyExhausted = false;
    QSet<int> _continuityUncertainVehicleIds;
    // Separate from actual observed outcomes: every new native instance must
    // re-review its protocol address space. Disk absence/old empty history is
    // not evidence that no old aircraft action remains. Only the dedicated
    // actual-source/two-account local recovery review may retire these holds.
    QSet<int> _continuityRestartReviewVehicleIds;
    // Historical observation is NOT a new uncertain outcome in this process.
    // Dedicated recovery never removes _continuityUncertainVehicleIds.
    QSet<int> _continuityRestoredHistoricalReviewVehicleIds;
    QPointer<FleetOccupancyRecoveryReview> _continuityRecoveryReview;
    QSet<int> _continuityJournalEverObservedIds;
    QPointer<FleetOccupancyJournalWorker> _continuityJournalWorker;
    QTimer _continuityJournalTimer;
    QVariantMap _continuityJournalLastStatus;
    QVariantMap _continuityJournalOfferedIdentity;
    QString _continuityJournalWorkerRunId;
    QString _continuityJournalError;
    quint64 _continuityJournalLastGeneration = 0;
    bool _continuityJournalBootstrapAccepted = false;
    bool _continuityJournalFailed = false;
    bool _continuityJournalDirty = true;
    bool _continuityJournalRetiring = false;
    QHash<quint64, QList<int>> _continuityCommandStages;
    quint64 _nextContinuityCommandStage = 0;
    QHash<int, QPointer<MissionManager>> _continuityTemporaryWatchedManagers;
    QHash<int, QMetaObject::Connection> _continuityTemporaryManagerConnections;
    struct FleetMissionQuarantine {
        QPointer<MissionManager> manager;
        const QObject* managerIdentity = nullptr;
        quint64 deploymentWriteOperationId = 0;
        QPointer<MissionManager> watchedManager;
        const QObject* watchedManagerIdentity = nullptr;
        quint64 replacementWriteOperationId = 0;
    };
    QHash<int, FleetMissionQuarantine> _fleetMissionQuarantine;
    QHash<int, QMetaObject::Connection> _fleetMissionQuarantineWriteConnections;
    // HealthAndArmingCheckReport exposes a monotonically increasing local
    // update sequence but not the originating MAVLink sample timestamp. Keep
    // the last sequence/observation time so a stalled report ages out instead
    // of being refreshed to age=0 on every registry tick.
    QHash<int, quint64> _healthReportSequences;
    QHash<int, qint64> _healthReportObservedAtMs;
    quint64 _nextDispatchEpoch = 0;
    quint64 _nextTransactionCreationOrder = 0;
    QHash<QString, QVariantMap> _transactionArchive;
    QStringList _transactionArchiveOrder;
    QSet<int> _commandConnectionIds;
    QHash<int, QMetaObject::Connection> _formationCommandConnections;
    quint64 _formationCommandGeneration = 0;
    qint64 _formationCommandDeadlineMs = 0;
    // Vehicle::sendMavCommand() can emit a local failure synchronously (for
    // example duplicate-command or a link which disappears between the
    // preflight and send).  Defer the phase-level rollback until the current
    // member dispatch loop has finished; otherwise an ABORT can be sent
    // re-entrantly while the outer loop is still sending PREPARE/COMMIT.
    bool _formationDispatchInProgress = false;
    QString _formationDeferredAbortReason;
    bool _formationDeferredAdvance = false;
    QString _lastFormationTransactionId;
    // 产品契约：支持的 PX4 实机与 SITL 使用相同能力开关，不能依赖运行环境偶然启用。
    static constexpr bool kFormationFeatureEnabled = true;
    static constexpr bool kAutoStartMissionFeatureEnabled = true;
    static constexpr uint8_t kSwarmProtocolVersion = 2;
    static constexpr uint8_t kSwarmVehicleCount = 6;
    static constexpr int kMaxArchivedTransactions = 100;
    static constexpr int kTemporaryMissionTimeoutMs = 300000;
    static constexpr int kMissionStartTimeoutMs = 10000;
    static constexpr uint8_t kSwarmLeaderSystemId = 1;
    static constexpr quint32 kMaximumSessionId = 0x00ffffffU;
    static constexpr quint64 kFollowTargetMagicPrefix = 0x4d45524900000000ULL; // "MERI" + session
    static const double kMinimumGuidedAltitudeMeters;
    static const double kDefaultMissionAltitudeMeters;
};

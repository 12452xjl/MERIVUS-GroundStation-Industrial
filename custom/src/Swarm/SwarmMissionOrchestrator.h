#pragma once

#include <QDateTime>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class FleetMissionExecutionObservation;
class FleetMissionDeploymentEpisode;
class FleetMissionDeploymentLifecycle;
class FleetMissionStartupTransaction;
class SwarmController;
class FleetRunCheckpointWorker;
struct FleetRunCheckpointHistory;

// Local task lifecycle and assignment coordinator; it never sends transport
// commands. Generic tasks retain their explicit lifecycle/progress APIs.
// A private native bridge also consumes bounded value evidence from an actual
// FleetMissionExecutionObservation Run. Those sticky, read-only records are
// observations, not generic progress, remote mission identity or completion.
class SwarmMissionOrchestrator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int taskCount READ taskCount NOTIFY taskCollectionChanged)
    Q_PROPERTY(QVariantList taskSummaries READ taskSummaries NOTIFY taskCollectionChanged)
    Q_PROPERTY(QVariantList executionObservationTaskSummaries READ executionObservationTaskSummaries NOTIFY executionObservationTasksChanged)
    Q_PROPERTY(QVariantMap executionObservationEventStatus READ executionObservationEventStatus NOTIFY executionObservationEventStatusChanged)
    Q_PROPERTY(QVariantMap fleetSummary READ fleetSummary NOTIFY fleetSummaryChanged)
    Q_PROPERTY(QVariantMap runCheckpointStatus READ runCheckpointStatus NOTIFY runCheckpointChanged)
    Q_PROPERTY(QVariantMap runCheckpointHistory READ runCheckpointHistory NOTIFY runCheckpointChanged)

public:
    enum class TaskType {
        Custom,
        Patrol,
        Search,
        Follow,
        Waypoint,
        Relay,
    };
    Q_ENUM(TaskType)

    enum class Phase {
        Draft,
        Running,
        Paused,
        Completed,
        Cancelled,
        Failed,
        Observing,
        ObservationClosed,
    };
    Q_ENUM(Phase)

    explicit SwarmMissionOrchestrator(QObject* parent = nullptr);
    ~SwarmMissionOrchestrator() override;

    QVariantMap runCheckpointStatus() const;
    QVariantMap runCheckpointHistory() const;
    Q_INVOKABLE bool setRunCheckpointEnabled(bool enabled, bool confirmed);
    Q_INVOKABLE QString requestRunCheckpoint(bool confirmed);
    Q_INVOKABLE QString loadRunCheckpointHistory(bool confirmed);
    Q_INVOKABLE bool cancelRunCheckpoint(const QString& jobId);
    Q_INVOKABLE QVariantMap runCheckpointHistoryPage(const QString& historyId, const QVariant& offset) const;
    Q_INVOKABLE QVariantMap runCheckpointHistoricalReport(const QString& historyId, const QVariant& index) const;

    int taskCount() const { return _taskOrder.size(); }
    QVariantList taskSummaries() const;
    // Same records as taskSummaries, restricted to protected native observers.
    // Observation-only changes intentionally do not use legacy taskUpdated.
    QVariantList executionObservationTaskSummaries() const;
    QVariantMap executionObservationEventStatus() const;
    // Only the exact payload currently emitted by this instance is a native
    // transient receipt. Imported maps and previously delivered rows do not
    // regain that receipt, a live Run, result attribution or authorization.
    Q_INVOKABLE bool executionObservationEventReceiptMatches(const QString& sourceEpoch,
        const QString& sequence, const QVariantMap& payload) const;
    QVariantMap fleetSummary() const;

    // Returns a generated task ID. An empty taskType is rejected. Vehicle IDs
    // must be integral MAVLink system IDs in 1..255. Any invalid or duplicate
    // entry rejects the whole assignment; no caller input is silently dropped
    // or replaced by another vehicle ID. Metadata/reason text is resource
    // bounded before it enters serialisable task state. An empty list is
    // allowed for a draft task which has not been assigned yet.
    Q_INVOKABLE QString createTask(const QString& taskType,
                                   const QVariantList& targetVehicleIds = QVariantList(),
                                   const QVariantMap& metadata = QVariantMap());
    Q_INVOKABLE bool startTask(const QString& taskId);
    Q_INVOKABLE bool pauseTask(const QString& taskId, const QString& reason = QString());
    Q_INVOKABLE bool resumeTask(const QString& taskId);
    Q_INVOKABLE bool cancelTask(const QString& taskId, const QString& reason = QString());
    Q_INVOKABLE bool completeTask(const QString& taskId);
    Q_INVOKABLE bool failTask(const QString& taskId, const QString& reason = QString());

    // Replaces the assignment for a non-terminal task. This is intended for
    // leader loss, low-battery exits, and other task reallocation decisions.
    Q_INVOKABLE bool reassignTask(const QString& taskId,
                                  const QVariantList& targetVehicleIds);
    // Legacy local-model CAS. Reject stale task revisions; a non-negative
    // leaderVehicleId updates membership and leader metadata atomically.
    // Ever-bound instances still check actual current native occupancy.
    // Product continuity apply must use the six-argument occupancy CAS below.
    Q_INVOKABLE bool reassignTaskAtRevision(const QString& taskId,
                                            const QVariantList& targetVehicleIds,
                                            qint64 expectedRevision,
                                            int leaderVehicleId = -1);
    // One local CAS for membership, resolved leader and the complete role
    // mapping. Never invokes either legacy reassignment API and then patches
    // metadata afterwards. This grants no vehicle/control authority.
    Q_INVOKABLE bool reassignTaskRolesAtRevision(const QString& taskId,
                                                const QVariantList& targetVehicleIds,
                                                qint64 expectedRevision,
                                                int leaderVehicleId,
                                                const QVariantList& roleAssignments);
    // Product path: compare the reviewed native Swarm occupancy identity and
    // check its actual old/new member occupancy immediately before local CAS.
    Q_INVOKABLE bool reassignTaskRolesWithOccupancyAtRevision(const QString& taskId,
                                                             const QVariantList& targetVehicleIds,
                                                             qint64 expectedRevision,
                                                             int leaderVehicleId,
                                                             const QVariantList& roleAssignments,
                                                             const QVariantMap& expectedOccupancyIdentity);
    Q_INVOKABLE bool setTaskProgress(const QString& taskId, double progress);
    Q_INVOKABLE QVariantMap taskSummary(const QString& taskId) const;
    // Local task-model reservation only, not onboard/control authorization.
    // Running/paused generic tasks retain members until terminal/reassigned.
    // Native observation assignments are protected until observation closure,
    // even while Draft, and are never writable through these generic APIs.
    // Generic draft tasks may propose overlapping assignments.
    Q_INVOKABLE QVariantMap assignmentAvailability(const QString& taskId) const;
    // Native queue read only: one first blocker per system ID (at most 255),
    // without copying task metadata or full Run observations every timer tick.
    // Empty means the original reciprocal Swarm association is unavailable.
    // This projection is neither a reservation nor an aircraft release.
    QVariantMap formationQueueAssignmentSnapshot() const;
    Q_INVOKABLE bool hasTask(const QString& taskId) const;
    Q_INVOKABLE bool removeTask(const QString& taskId);

    static QString taskTypeToString(TaskType type);
    static QString phaseToString(Phase phase);

signals:
    void runCheckpointChanged();
    void taskCreated(const QString& taskId, const QVariantMap& summary);
    void taskStarted(const QString& taskId, const QVariantMap& summary);
    void taskPaused(const QString& taskId, const QVariantMap& summary);
    void taskResumed(const QString& taskId, const QVariantMap& summary);
    void taskCancelled(const QString& taskId, const QVariantMap& summary);
    void taskCompleted(const QString& taskId, const QVariantMap& summary);
    void taskFailed(const QString& taskId, const QVariantMap& summary);
    void taskReassigned(const QString& taskId,
                        const QVariantList& previousVehicleIds,
                        const QVariantList& targetVehicleIds,
                        const QVariantMap& summary);
    void taskUpdated(const QString& taskId, const QVariantMap& summary);
    void taskObservationChanged(const QString& taskId, const QVariantMap& summary);
    void executionObservationTasksChanged();
    void executionObservationEventReady(const QString& sourceEpoch, const QString& sequence,
        const QVariantMap& payload);
    void executionObservationEventStatusChanged();
    void taskCollectionChanged();
    void fleetSummaryChanged(const QVariantMap& summary);

private:
    void markRunCheckpointDirty();
    void scheduleRunCheckpoint();
    void notifyRunCheckpoint();
    QString startRunCheckpoint(const QString& action);
    void finishRunCheckpoint(FleetRunCheckpointWorker* worker, const QString& jobId);
    friend class SwarmController;
    friend class FleetMissionExecutionObservation;
    friend class FleetMissionStartupTransaction;
    friend class FleetMissionDeploymentEpisode;
    friend class FleetMissionDeploymentLifecycle;
    // Actual native Run only. These are neither QML setters nor flight permits.
    QString registerExecutionObservation(FleetMissionExecutionObservation* owner);
    bool executionObservationMatches(FleetMissionExecutionObservation* owner, const QString& taskId) const;
    bool executionObservationRetirementMatches(FleetMissionExecutionObservation* owner, const QString& taskId) const;
    bool updateExecutionObservation(FleetMissionExecutionObservation* owner, const QString& taskId);
    // Read the same sticky native records, never their live/destroyed owner.
    // Only Swarm's pinned native bridge exposes the bounded directory and a
    // single exact selected record; no public JSON setter or restore path.
    QVariantMap protectedExecutionReportCatalog() const;
    QVariantMap captureProtectedExecutionRecord(const QVariantMap& request) const;
    QVariantMap captureClosedExecutionRecord(FleetMissionExecutionObservation* actualOwner, const QString& taskId) const;
    // Dedicated native retirement only, after the Lifecycle has archived and
    // rechecked the entire original Episode. Generic remove stays forbidden.
    bool retireArchivedExecutionRecords(const QVariantList& exactRecords);
    // Installed only by the actual Swarm's one-use native association, before
    // its public notification. Lost sources never revert to standalone mode.
    bool _bindContinuitySwarm(SwarmController* actualSwarm);

    struct TaskRecord {
        QString id;
        QString type;
        Phase phase = Phase::Draft;
        QVariantList targetVehicleIds;
        QVariantMap metadata;
        QString reason;
        double progress = 0.0;
        // Monotonic in-memory task revision.  Reviewers can bind a planner
        // suggestion to the exact task snapshot that produced it.
        qint64 revision = 1;
        QDateTime createdAt;
        QDateTime startedAt;
        QDateTime updatedAt;
        QDateTime finishedAt;
        // Sticky even after the actual owner is retired/destroyed. Generic
        // task APIs must never convert this record into controllable work.
        bool nativeExecutionObservation = false;
        QPointer<FleetMissionExecutionObservation> executionOwner;
        QVariantMap executionIdentity;
        QVariantMap executionObservation;
        quint64 observationRevision = 0;
        QDateTime observationUpdatedAt;
        QDateTime observationClosedAt;
        bool modeCommitted = false;
        bool observationBusy = false;
        bool observationClosed = false;
        bool observationOwnerDestroyed = false;
        bool observationRevisionExhausted = false;
    };

    // Compare the complete retained native record after an owner callback.
    // A QHash element pointer must never survive that callback boundary.
    static bool sameExecutionTaskRecord(const TaskRecord& left, const TaskRecord& right);

    static QString normalizeTaskId(const QString& taskId);
    static bool normalizeVehicleIdsStrict(const QVariantList& vehicleIds,
                                          QVariantList* normalized);
    static bool validateLeaderMetadata(const QVariantMap& metadata,
                                      const QVariantList& targetVehicleIds);
    static bool normalizeRoleMetadata(QVariantMap* metadata,
                                      const QVariantList& targetVehicleIds);
    static QString vehicleKey(const QVariant& vehicleId);
    static QString timestamp(const QDateTime& value);
    static bool isTerminal(Phase phase);

    TaskRecord* findTask(const QString& taskId);
    const TaskRecord* findTask(const QString& taskId) const;
    QVariantMap toSummary(const TaskRecord& task) const;
    QVariantMap protectedExecutionRecordMetadata(const TaskRecord& task) const;
    QVariantList assignmentConflicts(const QString& taskId,
                                    const QVariantList& targetVehicleIds) const;
    bool reassignTaskInternal(const QString& taskId,
                              const QVariantList& targetVehicleIds,
                              int leaderVehicleId,
                              const QVariantList* roleAssignments = nullptr,
                              const QVariantMap* expectedOccupancyIdentity = nullptr);
    void touch(TaskRecord& task);
    void emitCollectionChanges();
    void executionObservationOwnerDestroyed(const QString& taskId, const QVariantMap& frozenIdentity);
    void queueExecutionObservationNotifications(const QString& taskId, bool created, bool structural, bool observation);
    void flushExecutionObservationNotifications();
    void captureExecutionObservationEvents(const TaskRecord& task);
    void queueExecutionObservationEvent(const TaskRecord& task, int vehicleId,
        const QVariantMap& semantic, const QString& transitionKind);
    bool appendExecutionObservationEvent(QVariantMap payload);
    bool appendExecutionObservationCaptureGap();
    void loseExecutionObservationTransition();
    void scheduleExecutionObservationEvents();
    void flushExecutionObservationEvents();

    struct ExecutionObservationEvent {
        QString sequence;
        QVariantMap payload;
        qint64 bytes = 0;
    };
    // Separate from TaskRecord and the existing coalesced observation channel.
    // Capture semantic changes at their native commit, not at a later flush.
    QHash<QString, QHash<int, QVariantMap>> _executionObservationLastSemantic;
    QQueue<ExecutionObservationEvent> _executionObservationEvents;
    QHash<quint64, QVariantMap> _executionObservationPublishing;
    const QString _executionObservationEventEpoch;
    const QString _executionObservationEventSourceHash;
    quint64 _executionObservationEventSequence = 0;
    quint64 _executionObservationDroppedTransitions = 0;
    qint64 _executionObservationQueuedBytes = 0;
    bool _executionObservationSequenceExhausted = false;
    bool _executionObservationCaptureGapPending = false;
    bool _executionObservationEventFlushScheduled = false;

    const QString _protectedExecutionSourceId;
    QHash<QString, TaskRecord> _tasks;
    QHash<QString, QMetaObject::Connection> _executionOwnerConnections;
    QStringList _taskOrder;
    QSet<QString> _pendingObservationCreated;
    QSet<QString> _pendingObservationStructural;
    QSet<QString> _pendingObservationChanged;
    bool _observationNotificationScheduled = false;
    QPointer<SwarmController> _continuitySwarm;
    bool _continuitySwarmAssigned = false;
    const QString _runCheckpointPath;
    QPointer<FleetRunCheckpointWorker> _runCheckpointWorker;
    std::shared_ptr<const FleetRunCheckpointHistory> _runCheckpointHistorical;
    QVariantMap _runCheckpointResult;
    QString _runCheckpointJobId = QStringLiteral("");
    quint64 _runCheckpointGeneration = 1;
    quint64 _runCheckpointAttemptedGeneration = 0;
    quint64 _runCheckpointSavedGeneration = 0;
    quint64 _runCheckpointCapturedGeneration = 0;
    bool _runCheckpointEnabled = false;
    bool _runCheckpointDirty = false;
    bool _runCheckpointBusy = false;
    bool _runCheckpointEntryBusy = false;
    bool _runCheckpointScheduled = false;
    bool _runCheckpointNotifyScheduled = false;
    bool _runCheckpointExhausted = false;
    bool _runCheckpointDestroying = false;
};

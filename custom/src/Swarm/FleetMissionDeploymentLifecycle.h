#pragma once

#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetRolePolicy;
class FleetMissionPlanStore;
class FleetHealthCenter;
class SwarmController;
class SwarmMissionOrchestrator;
class FleetDeploymentArchiveWorker;

// Retained native ownership and local synchronous controller-call scopes,
// never account authentication, flight permission, or remote-idle evidence.
class FleetMissionDeploymentLifecycle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* currentEpisode READ currentEpisode NOTIFY changed)
    Q_PROPERTY(QObject* selectedObservationEpisode READ selectedObservationEpisode NOTIFY changed)
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap archiveState READ archiveState NOTIFY archiveChanged)
    Q_PROPERTY(QVariantMap archiveResult READ archiveResult NOTIFY archiveChanged)
public:
    explicit FleetMissionDeploymentLifecycle(QObject* parent = nullptr);
    ~FleetMissionDeploymentLifecycle() override;
    QObject* currentEpisode() const;
    QObject* selectedObservationEpisode() const;
    QVariantMap current() const;
    QVariantMap archiveState() const;
    QVariantMap archiveResult() const;
    Q_INVOKABLE QVariantMap requestArchive(QObject* actualController, QVariantMap exactIntent, bool confirmed);
    // In-process actual QObject references only, not serializable authority.
    // Matching ignores transient transport/prompt activity within one group.
    Q_INVOKABLE QVariantMap captureCurrentScope() const;
    Q_INVOKABLE bool currentScopeMatches(const QVariantMap& scope) const;
    Q_INVOKABLE QVariantMap captureSelectedObservationScope() const;
    Q_INVOKABLE QVariantMap captureObservationCloseScope(QObject* actualWorkflow) const;
    Q_INVOKABLE bool observationScopeMatches(const QVariantMap& scope) const;
    Q_INVOKABLE QVariantMap beginControllerCall(QObject* actualController, const QString& purpose, QObject* actualCloseWorkflow);
    Q_INVOKABLE bool controllerCallMatches(QObject* actualController, const QString& callId) const;
    Q_INVOKABLE bool endControllerCall(QObject* actualController, const QString& callId);
    // Actual five shared objects, not their IDs or JSON descriptors. The old
    // bundle stays alive; allocation never cancels or releases its work.
    Q_INVOKABLE QVariantMap prepareNextEpisode(QObject* expectedActualEpisode,
        const QString& expectedEpisodeId, const QVariantMap& actualSharedSources, bool operatorConfirmed);
    Q_INVOKABLE bool selectObservationEpisode(const QString& episodeId, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap prepareNextEpisodeForController(QObject* actualController, const QVariantMap& expectedCurrentScope,
        const QVariantMap& actualSharedSources, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap selectObservationEpisodeForController(QObject* actualController, const QVariantMap& expectedSelectedScope,
        const QString& episodeId, bool operatorConfirmed);
    // Actual retained lookup for a known native Close owner. Not an ID-based
    // authority lookup and never permission for a retained outbound sender.
    QObject* episodeForObservationClose(QObject* actualWorkflow) const;
    bool isCurrentEpisode(QObject* actualEpisode, const QString& episodeId) const;
signals:
    void changed();
    void archiveChanged();
private:
    struct Record {
        QPointer<FleetMissionDeploymentEpisode> episode;
        QString episodeId;
        bool published = false;
        bool sourceLost = false;
        bool retiring = false;
        QList<QMetaObject::Connection> connections;
    };
    enum class ControllerTransitionKind { PrepareNextEpisode, SelectObservationEpisode };
    struct ControllerTransitionPermit {
        QPointer<QObject> owner;
        QString callId, purpose;
        QVariantMap scope;
    };
    struct ControllerTransitionReceipt {
        bool mutationCommitted = false;
        bool newEpisodePublished = false;
        bool observationSelectionChanged = false;
        QString currentEpisodeId, selectedObservationEpisodeId;
        QVariantMap resultScope;
    };
    bool controllerTransitionHeld(const ControllerTransitionPermit* permit) const;
    bool controllerMutationBlocked(const ControllerTransitionPermit* permit, const QString& purpose) const;
    bool endControllerCallImpl(QObject* actualController, const QString& callId, const ControllerTransitionPermit* permit);
    QVariantMap runControllerTransition(QObject* actualController, const QVariantMap& expectedScope,
        const QVariantMap& sharedSources, const QString& selectedEpisodeId, bool operatorConfirmed, ControllerTransitionKind kind);
    QVariantMap prepareNextEpisodeImpl(QObject* actualPrevious, const QString& episodeId, const QVariantMap& sharedSources,
        bool operatorConfirmed, const ControllerTransitionPermit* permit, ControllerTransitionReceipt* receipt);
    bool selectObservationEpisodeImpl(const QString& episodeId, bool operatorConfirmed,
        const ControllerTransitionPermit* permit, ControllerTransitionReceipt* receipt, QString* reason);
    bool retainedEpisodeCurrent(FleetMissionDeploymentEpisode* episode) const;
    QVariantMap captureScope(FleetMissionDeploymentEpisode* episode, const QString& kind, const QString& generation) const;
    void retain(FleetMissionDeploymentEpisode* episode, bool published);
    void notify();
    void startArchiveWorker(const QVariantMap& request, const QVariantMap& input, const QVariantMap& cache);
    void finishArchiveIfReady();
    void completeArchive(bool ok, const QString& error, const QVariantMap& reply);
    void queueArchivedDestructionCheck(const QString& episodeId, const QString& jobId, quint64 serial);
    void archivedEpisodeDestroyed(const QString& episodeId, const QString& jobId, quint64 serial);
    bool archiveTargetCurrent() const;
    QList<Record> _records;
    QPointer<FleetMissionDeploymentEpisode> _current;
    QPointer<FleetMissionDeploymentEpisode> _selected;
    QPointer<SwarmController> _swarm;
    QPointer<SwarmMissionOrchestrator> _orchestrator;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetHealthCenter> _health;
    QString _lifecycleEpoch;
    QString _lastError;
    quint64 _revision = 1;
    quint64 _outboundScopeGeneration = 1;
    quint64 _observationScopeGeneration = 1;
    quint64 _controllerCallGeneration = 0;
    QPointer<QObject> _controllerCallOwner;
    QString _controllerCallId;
    QString _controllerCallPurpose;
    QVariantMap _controllerCallScope;
    const ControllerTransitionPermit* _controllerTransitionPermit = nullptr;
    int _allocationCount = 0;
    quint64 _lifetimeAllocationAttempts = 0;
    quint64 _archiveSerial = 0;
    QString _archiveJobId = QStringLiteral("");
    QString _archiveNonce;
    QString _archiveAction = QStringLiteral("");
    QString _archiveStage = QStringLiteral("idle");
    QString _archiveEpisodeId = QStringLiteral("");
    QVariantMap _archiveInput;
    QVariantMap _archiveRequest;
    QVariantMap _archiveEvidence;
    QVariantMap _archivePendingResult;
    QVariantMap _archiveResult;
    QVariantMap _archiveOpened;
    QVariantMap _archiveWriteReply;
    QList<QPointer<QObject>> _archiveParts;
    QList<QPointer<QObject>> _archiveDescendants;
    QList<QPointer<QObject>> _archiveDestructionObjects;
    QPointer<FleetMissionDeploymentEpisode> _archiveTarget;
    QPointer<QObject> _archiveController;
    QPointer<FleetDeploymentArchiveWorker> _archiveWorker;
    FleetDeploymentArchiveWorker* _archiveWorkerIdentity = nullptr; // equality only
    bool _archiveBusy = false;
    bool _archiveEntryBusy = false;
    bool _archiveCancelRequested = false;
    bool _archiveResultSeen = false;
    bool _archiveFinishedSeen = false;
    bool _archiveRetirementCommitted = false;
    bool _archiveEpisodeDestroyed = false;
    bool _archiveCapacityReclaimed = false;
    bool _archiveDestructionCheckPending = false;
    int _archiveRetiredTaskCount = 0;
    bool _sharedPinned = false;
    bool _sourceLost = false;
    bool _switching = false;
    bool _controllerCallActive = false;
    bool _reentered = false;
    bool _notificationPending = false;
    bool _destroying = false;
};

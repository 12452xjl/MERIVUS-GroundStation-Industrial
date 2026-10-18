#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class FleetEventBlackBox;
class FleetEventArchiveWorker;
class FleetEventArchiveHistoryReader;
class FleetEventArchiveMaintenanceWorker;

// Default-off diagnostic persistence. Native configuration is explicit;
// Product sources use one fixed local directory/policy; explicit backup
// previews/catalogs/diagnostics admit a separate directory. Arbitrary source/native
// configuration remains non-invokable. No replay or control methods.
class FleetPersistentEventStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* sourceBlackBox READ sourceBlackBox WRITE setSourceBlackBox NOTIFY sourceBlackBoxChanged)
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap historyPage READ historyPage NOTIFY historyPageChanged)
    Q_PROPERTY(QVariantMap historyCatalog READ historyCatalog NOTIFY historyCatalogChanged)
    Q_PROPERTY(QVariantMap historySearch READ historySearch NOTIFY historySearchChanged)
    Q_PROPERTY(QVariantMap historyTasks READ historyTasks NOTIFY historyTasksChanged)
    Q_PROPERTY(QVariantMap historyTaskRecords READ historyTaskRecords NOTIFY historyTaskRecordsChanged)
    Q_PROPERTY(QVariantMap historyTaskIndex READ historyTaskIndex NOTIFY historyTaskIndexChanged)
    Q_PROPERTY(QVariantMap maintenanceInventory READ maintenanceInventory NOTIFY maintenanceInventoryChanged)
    Q_PROPERTY(QVariantMap maintenanceBackupCatalog READ maintenanceBackupCatalog NOTIFY maintenanceBackupCatalogChanged)
    Q_PROPERTY(QVariantMap maintenanceBackupDiagnostics READ maintenanceBackupDiagnostics NOTIFY maintenanceBackupDiagnosticsChanged)
    Q_PROPERTY(QVariantMap maintenancePreview READ maintenancePreview NOTIFY maintenancePreviewChanged)
    Q_PROPERTY(QVariantMap maintenanceResult READ maintenanceResult NOTIFY maintenanceResultChanged)
    Q_PROPERTY(QVariantMap maintenanceCapacityPlan READ maintenanceCapacityPlan NOTIFY maintenanceCapacityChanged)
    Q_PROPERTY(QVariantMap maintenanceCapacityStatus READ maintenanceCapacityStatus NOTIFY maintenanceCapacityChanged)
    Q_PROPERTY(QVariantMap retiredPurgeState READ retiredPurgeState NOTIFY retiredPurgeChanged)
    Q_PROPERTY(QVariantMap retiredPurgePreview READ retiredPurgePreview NOTIFY retiredPurgeChanged)
    Q_PROPERTY(QVariantMap retiredPurgeResult READ retiredPurgeResult NOTIFY retiredPurgeChanged)
    Q_PROPERTY(QVariantMap localArchiveOptions READ localArchiveOptions CONSTANT)
public:
    explicit FleetPersistentEventStore(QObject* parent = nullptr);
    ~FleetPersistentEventStore() override;
    QObject* sourceBlackBox() const;
    void setSourceBlackBox(QObject* source);
    QVariantMap current() const;
    QVariantMap historyPage() const;
    QVariantMap historyCatalog() const;
    QVariantMap historySearch() const;
    QVariantMap historyTasks() const;
    QVariantMap historyTaskRecords() const;
    QVariantMap historyTaskIndex() const;
    QVariantMap maintenanceInventory() const;
    QVariantMap maintenanceBackupCatalog() const;
    QVariantMap maintenanceBackupDiagnostics() const;
    QVariantMap maintenancePreview() const;
    QVariantMap maintenanceResult() const;
    QVariantMap maintenanceCapacityPlan() const;
    QVariantMap maintenanceCapacityStatus() const;
    Q_INVOKABLE QVariantMap requestLocalArchiveCapacity(const QVariantMap& intent, bool confirmed);
    QVariantMap retiredPurgeState() const;
    QVariantMap retiredPurgePreview() const;
    QVariantMap retiredPurgeResult() const;
    Q_INVOKABLE QVariantMap requestRetiredPurge(QObject* actualController, const QVariantMap& intent, bool confirmed);
    QVariantMap localArchiveOptions() const;
    Q_INVOKABLE QVariantMap requestLocalArchiveAction(const QVariantMap& intent, bool confirmed);
    Q_INVOKABLE QVariantMap requestLocalArchiveSearch(const QVariantMap& intent, bool confirmed);
    Q_INVOKABLE QVariantMap requestLocalArchiveTasks(const QVariantMap& intent, bool confirmed);
    // Explicit derived-index I/O. Paths, checkpoint hashes and raw cursors are
    // selected by this native owner, never reconstructed from QML snapshots.
    Q_INVOKABLE QVariantMap requestLocalArchiveTaskIndex(const QVariantMap& intent, bool confirmed);
    // Original actions require intent6; prepare-backup/backup-catalog and
    // backup-diagnostics accept intent7 with the explicit backupDirectory.
    // Import actions instead use intent7 with a native diagnosticsId. Only
    // original readable diagnostic evidence can prepare a historical import;
    // no public map/path reconstructs its native preview or fixed destination.
    Q_INVOKABLE QVariantMap requestLocalArchiveMaintenance(const QVariantMap& intent, bool confirmed);
    Q_INVOKABLE QVariantMap recordLocalEngineeringReview(int vehicleId, const QString& operation,
        const QVariantMap& result, const QVariantMap& frozenComparison);

    // Native backend only. Consent is not an authenticated operator identity.
    // These are deliberately NOT Q_INVOKABLE and perform no flight action.
    QVariantMap beginPersistence(const QString& absoluteDirectory,
                                 const QVariantMap& exactPolicy, bool operatorConfirmed);
    bool stopPersistence(bool drain = true);
    bool requestRollover(bool operatorConfirmed, int maximumSealedSegments = 32);
    bool requestHistory(const QString& queryId, const QString& runFilter,
                        const QString& afterRowId, int limit, bool operatorConfirmed);
    bool requestArchiveCatalog(const QString& queryId, const QString& absoluteDirectory,
                               const QVariantMap& exactPolicy, bool operatorConfirmed);
    bool requestArchivedPage(const QString& queryId, const QString& catalogId, const QString& basename,
                             const QString& runFilter, const QString& afterRowId, int limit, bool operatorConfirmed);
    bool cancelArchivedHistory();
signals:
    void sourceBlackBoxChanged();
    void changed();
    void historyPageChanged();
    void historyCatalogChanged();
    void historySearchChanged();
    void historyTasksChanged();
    void historyTaskRecordsChanged();
    void historyTaskIndexChanged();
    void maintenanceInventoryChanged();
    void maintenanceBackupCatalogChanged();
    void maintenanceBackupDiagnosticsChanged();
    void maintenancePreviewChanged();
    void maintenanceResultChanged();
    void maintenanceCapacityChanged();
    void retiredPurgeChanged();
private:
    void acceptNative(FleetEventBlackBox* source, const QString& epoch,
                      const QString& sequence, const QVariantMap& exactEvent);
    void observeWorker();
    void externalGap(quint64 lostCount = 1);
    bool increment(quint64* counter, quint64 amount = 1);
    bool startArchiveReader(const QString& queryId, const QString& directory,
                            const QVariantMap& policy, const QVariantMap& request);
    void retireLocalHistoryRequest(const QString& queryId);
    bool acceptRetainedTaskResult(const QString& purpose, const QString& queryId,
        const QVariantMap& request, const QVariantMap& result);
    void finishRetainedTaskRequest(const QString& purpose, const QString& queryId);
    void retireArchiveContinuations();
    void retireMaintenanceValues();
    void notifyArchiveViews();
    bool startArchiveMaintenance(const QString& queryId, const QVariantMap& request);
    void expireMaintenancePreview(const QString& previewId);
    void notifyCapacity();
    bool capacityContextCurrent() const;
    void advanceCapacity();
    void launchCapacityWorker(const QString& phase, const QVariantMap& request);
    void completeCapacityWorker(const QString& planId, const QString& jobId, const QString& phase,
        const QVariantMap& request, const QVariantMap& result, const QString& error);
    void finishCapacity(bool ok, const QString& error);
    void notifyRetiredPurge();
    void retireRetiredPurgeBasis();
    bool retiredPurgeBasisCurrent() const;
    bool retiredPurgeSharedIdle() const;
    void installRetiredPurgeBasis(const QVariantMap& actualBackupResult, const QVariantMap& actualRequest,
        const QString& originalRun, const QString& originalBinding, quint64 originalGeneration);
    void startRetiredPurgeWorker(const QVariantMap& request);
    void finishRetiredPurgeIfReady();
    void expireRetiredPurgePreview(const QString& basisId, const QString& previewId);

    const QString _localArchiveAppData;
    const QString _localArchiveDirectory;
    QString _localPendingHistoryId;
    QString _localPendingHistoryKind;
    QString _localCancelledCurrentId;
    QString _localCurrentPageQueryId;
    QString _localCurrentPageRunId;
    QVariantMap _localOfflineCursor;
    QVariantMap _localSearchContext;
    // Private native continuations are separate from ordinary historySearch.
    // Public DTOs never restore these contexts or select an arbitrary path.
    QVariantMap _taskDirectoryContext;
    QVariantMap _taskRecordsContext;
    QVariantMap _taskDirectoryRows;
    QVariantMap _taskDirectoryFiles;
    QVariantMap _historyTasks;
    QVariantMap _historyTaskRecords;
    QVariantMap _historyTaskIndex;
    QVariantMap _taskIndexContext;
    QVariantMap _taskIndexPageContext;
    QString _taskIndexPendingQueryId;
    QString _taskPendingQueryId;
    QString _taskPendingPurpose;
    bool _localActionBusy = false;
    bool _localRecordBusy = false;
    bool _localMaintenanceBusy = false;
    QPointer<FleetEventBlackBox> _source;
    QPointer<FleetEventArchiveWorker> _worker;
    QPointer<FleetEventArchiveHistoryReader> _reader;
    QPointer<FleetEventArchiveMaintenanceWorker> _maintenance;
    QVariantMap _maintenanceSnapshot;
    QVariantMap _maintenanceInventory;
    QVariantMap _maintenanceBackupCatalog;
    QVariantMap _maintenanceBackupDiagnostics;
    QVariantMap _maintenancePreview;
    QVariantMap _maintenanceResult;
    // Only these native contexts select a descriptor. Public snapshots cannot
    // reconstruct an inventory, preview, continuation, or maintenance grant.
    QVariantMap _maintenanceInventoryContext;
    QVariantMap _maintenanceEntries;
    QVariantMap _maintenanceDiagnosticsContext;
    QVariantMap _maintenanceReadableBackupEntries;
    QVariantMap _maintenancePreviewContext;
    QElapsedTimer _maintenancePreviewClock;
    QString _maintenanceQueryId;
    bool _maintenanceTerminalReceived = false;
    QVariantMap _capacityPlan;
    QVariantMap _capacityStatus;
    QVariantList _capacityExpectedEntries;
    QVariantList _capacityResults;
    QString _capacityBinding;
    QPointer<FleetEventBlackBox> _capacitySource;
    QString _capacityJobId;
    QString _capacityPhase;
    QPointer<FleetEventArchiveMaintenanceWorker> _capacityWorker;
    QElapsedTimer _capacityClock;
    int _capacityIndex = 0;
    int _capacityCompleted = 0;
    bool _capacityActive = false;
    bool _capacityHadSource = false;
    bool _capacityEntryBusy = false;
    bool _capacityCancel = false;
    bool _capacityNotifyScheduled = false;
    // Independent native basis: only an actual verified backup result AND
    // that exact worker's finished event may install it. Public DTOs/backup
    // catalogs never reconstruct this single destructive-operation source.
    QVariantMap _retiredPurgeBasis;
    QVariantMap _retiredPurgeBasisRequest;
    QString _retiredPurgeBasisId = QStringLiteral("");
    QString _retiredPurgeBasisRun = QStringLiteral("");
    QString _retiredPurgeBasisBinding;
    quint64 _retiredPurgeBasisGeneration = 0;
    bool _retiredPurgeBasisReady = false;
    bool _retiredPurgeBasisExhausted = false;
    QVariantMap _retiredPurgePreview;
    QVariantMap _retiredPurgeProof;
    QElapsedTimer _retiredPurgePreviewClock;
    QVariantMap _retiredPurgeResult;
    QVariantMap _retiredPurgePendingResult;
    QVariantMap _retiredPurgeJobRequest;
    QString _retiredPurgeJobId = QStringLiteral("");
    QString _retiredPurgeAction = QStringLiteral("");
    QString _retiredPurgeStage = QStringLiteral("idle");
    QString _retiredPurgeJobBasisId = QStringLiteral("");
    QString _retiredPurgeJobRun = QStringLiteral("");
    QString _retiredPurgeJobBinding;
    QString _retiredPurgeNonce;
    quint64 _retiredPurgeJobGeneration = 0;
    quint64 _retiredPurgeSerial = 0;
    QPointer<QObject> _retiredPurgeController;
    FleetEventArchiveMaintenanceWorker* _retiredPurgeWorkerIdentity = nullptr; // equality only
    bool _retiredPurgeBusy = false;
    bool _retiredPurgeEntryBusy = false;
    bool _retiredPurgeCancelRequested = false;
    bool _retiredPurgeResultSeen = false;
    bool _retiredPurgeFinishedSeen = false;
    bool _retiredPurgeNotifyPending = false;
    bool _retiredPurgeDestroying = false;
    std::function<void()> _retiredPurgeDeferredBasis;
    QTimer _statusTimer;
    QElapsedTimer _receiptClock;
    QVariantMap _workerSnapshot;
    QVariantMap _historyPage;
    QVariantMap _historyCatalog;
    QVariantMap _historySearch;
    QVariantMap _archiveReaderSnapshot;
    QVariantMap _catalogPolicy;
    QVariantMap _archivedCursors;
    QString _catalogDirectory;
    QString _catalogId;
    QString _pendingHistoryKind;
    bool _currentHistoryCancelled = false;
    bool _archiveReadCancelled = false;
    bool _archiveResultAccepted = false;
    QString _archiveResultErrorCode;
    QVariantMap _configuredPolicy;
    QVariantMap _lastRollover;
    QString _configuredDirectory;
    QString _sourceBindingEpoch;
    QPointer<FleetEventBlackBox> _rolloverSource;
    QString _rolloverSourceEpoch;
    QString _rolloverDatabaseEpoch;
    quint64 _rolloverPauseSequence = 0;
    quint64 _rolloverUnrecordedSourceEvents = 0;
    bool _rolloverWindowKnown = false;
    bool _rolloverRequested = false;
    QString _state = QStringLiteral("disabled");
    QString _errorCode;
    QString _runId;
    QString _sourceEpoch;
    QString _captureStartSequence;
    QString _pendingHistoryId;
    QString _lastHistoryId;
    quint64 _lastSourceSequence = 0;
    quint64 _archiveSequence = 0;
    quint64 _projectionRejectedCount = 0;
    quint64 _sourceRejectedCount = 0;
    quint64 _sourceMissingCount = 0;
    quint64 _queueRejectedCount = 0;
    qint64 _lastUtcMs = -1;
    qint64 _lastMonotonicMs = -1;
    bool _captureEnabled = false;
    bool _captureBusy = false;
    bool _gapObserved = false;
    bool _gapAccountingAccepted = true;
    bool _clockDiscontinuityObserved = false;
    bool _sourceLost = false;
};

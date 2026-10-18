#pragma once

#include "VehicleTelemetryObserver.h"

#include <QElapsedTimer>
#include <QMap>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class FleetTelemetryArchiveWorker;
class FleetTelemetryArchiveHistoryReader;

// Explicit opt-in recorder of approved parser-accepted RX observations.
// QML receives status/derived values and a closed fixed-local history action,
// never arbitrary paths/policies, raw pages, recorder start or import methods.
class FleetTelemetryArchive : public VehicleTelemetryObserver
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap historyCatalog READ historyCatalog NOTIFY historyCatalogChanged)
    Q_PROPERTY(QVariantMap historyPage READ historyPage NOTIFY historyPageChanged)
    Q_PROPERTY(QVariantMap historySearch READ historySearch NOTIFY historySearchChanged)
    Q_PROPERTY(QVariantMap localHistoryOptions READ localHistoryOptions NOTIFY changed)
    Q_PROPERTY(QVariantMap motionReview READ motionReview NOTIFY changed)
    Q_PROPERTY(QVariantMap motionDossier READ motionDossier NOTIFY changed)
    Q_PROPERTY(QVariantMap gnssReview READ gnssReview NOTIFY changed)
    Q_PROPERTY(QString gnssReviewToken READ gnssReviewToken NOTIFY changed)
    Q_PROPERTY(QVariantMap maintenanceHistoryPreview READ maintenanceHistoryPreview NOTIFY maintenanceHistoryChanged)
public:
    explicit FleetTelemetryArchive(QObject* parent = nullptr);
    ~FleetTelemetryArchive() override;
    QVariantMap current() const;
    // Native backend only. Consent is not an authenticated operator identity.
    QVariantMap beginCapture(const QString& absoluteDirectory, const QVariantMap& exactPolicy,
        const QVariantList& exactVehicleIds, bool includePositionTelemetry, bool operatorConfirmed);
    bool stopCapture(bool drain = true);
    QVariantMap historyCatalog() const;
    QVariantMap historyPage() const;
    QVariantMap historySearch() const;
    QVariantMap localHistoryOptions() const;
    QVariantMap motionReview() const;
    QVariantMap motionDossier() const;
    QVariantMap gnssReview() const;
    QString gnssReviewToken() const;
    // Native-only current-report read. A portable report/token pair cannot
    // replace this Archive's actual retained page and fresh report token.
    QVariantMap nativeGnssReview(const QString& token, bool confirmed) const;
    // Closed fixed-local read workflow only. The actual catalog/page supplies
    // every path, query, policy and continuation; no raw data reaches QML.
    Q_INVOKABLE QVariantMap requestLocalHistoryAction(const QVariantMap& intent, bool confirmed);
    // Separate closed search intent; the legacy six-field page action stays
    // unchanged. Native alone owns manifests, cursors and hit file hashes.
    Q_INVOKABLE QVariantMap requestLocalHistorySearch(const QVariantMap& intent, bool confirmed);
    bool requestHistoryCatalog(const QString& queryId, const QString& absoluteDirectory,
        const QVariantMap& exactReadPolicy, bool operatorConfirmed);
    bool requestHistoryPage(const QString& queryId, const QString& catalogId, const QString& basename,
        const QString& afterSequence, int vehicleId, int limit, bool includePositionPayload, bool operatorConfirmed);
    bool requestHistorySearch(const QString& queryId, const QString& catalogId,
        int vehicleId, int messageId, int limit, bool nextPage, bool operatorConfirmed);
    bool cancelHistory();
    // Raw data is native-only, for this actual latest successful page and a
    // fresh explicit read confirmation. It is not a Q_PROPERTY or invokable.
    QVariantMap nativeHistoryPage(const QString& queryId, bool operatorConfirmed) const;

    // Fixed-local, current native page only. No caller-supplied payload or
    // public history summary can construct these battery observations.
    Q_INVOKABLE QVariantMap maintenanceHistoryCandidates(QString queryId, bool confirmed) const;
    Q_INVOKABLE QVariantMap prepareMaintenanceHistory(QVariantMap exactSelection, bool confirmed);
    Q_INVOKABLE bool discardMaintenanceHistory(QString previewId);
    QVariantMap maintenanceHistoryPreview() const;
    // Native Passport consumer only; direct record body or empty on stale,
    // busy, unconfirmed or diagnostic-only preview. Not an invokable.
    QVariantMap nativeMaintenanceHistoryRecord(QString previewId, bool confirmed) const;

signals:
    void changed();
    void historyCatalogChanged();
    void historyPageChanged();
    void historySearchChanged();
    void maintenanceHistoryChanged();

protected:
    void observeTelemetryFrame(Vehicle* vehicle, LinkInterface* link,
        const mavlink_message_t& originalMessage, const QString& sourceEpoch,
        quint64 sourceSequence, qint64 receivedMonotonicMs) override;
    void observeTelemetrySourceRetired(Vehicle* vehicle, const QString& sourceEpoch,
        quint64 lastSourceSequence, const QString& fixedCode) override;

private:
    struct Binding {
        QPointer<Vehicle> vehicle;
        QString epoch;
        quint64 lastSequence = 0;
        QMetaObject::Connection destruction;
    };
    void detachBindings();
    void pollWorker();
    void failCapture(const QString& fixedCode);
    QVariantMap captureSummary() const;
    bool advance(qint64* counter, quint64 amount = 1);
    bool startHistoryReader(const QString& queryId, const QString& directory,
        const QVariantMap& policy, const QVariantMap& request, const QVariantMap& context);
    void receiveHistoryResult(const QVariantMap& result);
    void releaseHistorySlotIfFinished();
    bool motionReviewCurrent() const;
    bool gnssReviewCurrent() const;
    void retireGnssReview();
    bool maintenanceHistoryPageCurrent(const QString& queryId) const;
    bool maintenanceHistoryCurrent() const;
    void retireMaintenanceHistory();
    bool searchSourceCurrent(const QVariantMap& source) const;
    bool requestHistoryPageWithHash(const QString& queryId, const QString& catalogId, const QString& basename,
        const QString& afterSequence, int vehicleId, int limit, bool includePositionPayload,
        const QString& expectedFirstFileHash, bool operatorConfirmed);

    QMap<int, Binding> _bindings;
    QPointer<FleetTelemetryArchiveWorker> _worker;
    QPointer<FleetTelemetryArchiveHistoryReader> _reader;
    QVariantMap _readerSnapshot;
    QVariantMap _historyCatalog;
    QVariantMap _historyPage;
    QVariantMap _historySearch;
    QVariantMap _searchSource;
    QVariantMap _searchCursor;
    QVariantMap _nativeHistoryPage;
    QVariantMap _motionReview;
    QVariantMap _motionDossier;
    QVariantMap _gnssReview;
    QString _gnssReviewToken;
    QVariantMap _maintenanceHistoryPreview;
    QVariantMap _maintenanceHistoryRecord;
    QVariantMap _maintenanceHistoryPage;
    quint64 _maintenanceHistorySourceSerial = 0;
    quint64 _maintenanceHistoryGeneration = 0;
    bool _maintenanceHistoryBusy = false;
    bool _maintenanceHistoryClosing = false;
    QVariantMap _catalogPolicy;
    QVariantMap _pendingHistoryContext;
    QMap<QString, QVariantMap> _catalogFiles;
    QMap<QString, QVariantMap> _historyCursors;
    QString _catalogDirectory;
    QString _catalogId;
    QString _readerNonce;
    QString _pendingHistoryId;
    QString _pendingHistoryKind;
    QString _latestHistoryId;
    QString _lastPageId;
    QString _lastSearchId;
    QString _historyError;
    const QString _localHistoryDirectory;
    quint64 _historyRequestSerial = 0;
    bool _localHistoryActionBusy = false;
    bool _historyCancelled = false;
    bool _historyResultAccepted = false;
    bool _historyTerminalDelivered = false;
    bool _historyFinishedObserved = false;
    QTimer _statusTimer;
    QElapsedTimer _receiptClock;
    QVariantMap _workerSnapshot;
    QVariantList _selectedVehicleIds;
    QString _runId;
    QString _state = QStringLiteral("disabled");
    QString _errorCode;
    qint64 _receivedFrames = 0;
    qint64 _filteredFrames = 0;
    qint64 _invalidFrames = 0;
    qint64 _queueRejectedFrames = 0;
    qint64 _sourceMissingFrames = 0;
    qint64 _sourceRetiredCount = 0;
    qint64 _offeredSequence = 0;
    qint64 _captureStartUtcMs = 0;
    qint64 _captureEndUtcMs = 0;
    qint64 _lastUtcMs = -1;
    qint64 _lastReceiptMonotonicMs = -1;
    bool _includePositionTelemetry = false;
    bool _captureEnabled = false;
    bool _captureBusy = false;
    bool _sourceCoverageClosed = false;
    bool _clockDiscontinuityObserved = false;
};

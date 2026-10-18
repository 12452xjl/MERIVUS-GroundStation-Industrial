#pragma once

#include <QMutex>
#include <QString>
#include <QThread>
#include <QVariantMap>
#include <QVariantList>

#include <atomic>

// One parentless, value-only historical work item. Existing history reads
// remain read-only; task-index may write its fixed derived checkpoint.
// No live BlackBox, Vehicle, replay, registry, approval or authority references.
class FleetEventArchiveHistoryReader : public QThread
{
    Q_OBJECT
public:
    FleetEventArchiveHistoryReader(const QString& canonicalAbsoluteDirectory,
                                  const QVariantMap& exactPolicy,
                                  const QString& queryId,
                                  const QVariantMap& exactRequest);
    ~FleetEventArchiveHistoryReader() override;

    void requestCancel();
    QVariantMap current() const;

    // Value-only contracts shared with the Store; no file, SQL or thread I/O.
    // Legacy search is exact 5 keys -> schema 1/exact 28 result keys.
    // Optional taskIdHash adds one exact hash key -> schema 2/exact 29.
    // Task-filtered results reserve 2048 facade bytes; legacy keeps 512.
    static bool searchRequestValid(const QVariantMap& request);
    static bool searchResultMatches(const QVariantMap& result, const QVariantMap& request,
                                    const QVariantMap& policy);

    // Native worker-thread reuse only. These perform the existing bounded
    // directory/hash/SQL reads, never on the application owner thread.
    static bool searchInWorker(const QString& directory, const QVariantMap& policy,
                               const QVariantMap& request, const std::atomic_bool& cancelled,
                               QVariantMap* result, QString* error);
    // Freeze all actual files of the same inventory, ordered by fileIndex.
    // Failure leaves no usable identity prefix.
    static bool snapshotIndexSourcesInWorker(const QString& directory, const QVariantMap& policy,
        const QString& scope, const QString& inventoryFingerprint, const std::atomic_bool& cancelled,
        QVariantList* fileIdentities, QString* error);
    // Every identity is the exact six-field search.scannedPages projection,
    // unique by fileIndex and covering the complete inventory. Recheck the
    // complete bytes/SQL identity, not merely size and mtime.
    static bool verifyIndexSourcesInWorker(const QString& directory, const QVariantMap& policy,
        const QString& scope, const QString& inventoryFingerprint, const QVariantList& fileIdentities,
        const std::atomic_bool& cancelled, QString* error);

signals:
    void resultReady(const QString& queryId, const QVariantMap& historicalResult);
    void failure(const QString& queryId, const QString& fixedCode);

protected:
    void run() override;

private:
    void setTerminal(const QString& state, const QString& fixedCode = QString());

    const QString _directory;
    const QVariantMap _policy;
    const QString _queryId;
    const QVariantMap _request;
    const bool _configurationValid;
    std::atomic_bool _cancelled{false};
    mutable QMutex _mutex;
    QVariantMap _status;
};

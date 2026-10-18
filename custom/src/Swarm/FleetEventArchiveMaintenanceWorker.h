#pragma once

#include <QMutex>
#include <QString>
#include <QThread>
#include <QVariantMap>

#include <atomic>

// One parentless, value-only archive maintenance operation. It may rename one
// sealed/retired file or stream one new external backup with readback, never
// mutate the current database or overwrite an archive or partial copy. Explicit
// Windows retired purge is separate: it rechecks a verified backup and removes
// only the originally opened source handle after a new local intent receipt.
// Explicit backup import copies to a new staging file before no-replace
// publication as retired history; no task, account or permission is restored.
// Independent backup-catalog reads do not require the original source folder.
// The owner must retain the worker until actual QThread::finished.
class FleetEventArchiveMaintenanceWorker : public QThread
{
    Q_OBJECT
public:
    FleetEventArchiveMaintenanceWorker(const QString& canonicalAbsoluteDirectory,
                                       const QVariantMap& exactPolicy,
                                       const QString& requestId,
                                       const QVariantMap& exactRequest);
    ~FleetEventArchiveMaintenanceWorker() override;

    void requestCancel();
    QVariantMap current() const;

    // Pure value contracts. No filesystem, SQL or thread I/O.
    static bool requestValid(const QVariantMap& request);
    static bool resultMatches(const QVariantMap& result, const QVariantMap& request,
                              const QVariantMap& policy);

signals:
    void resultReady(const QString& requestId, const QVariantMap& observedResult);
    void failure(const QString& requestId, const QString& fixedCode);

protected:
    void run() override;

private:
    void setState(const QString& state, const QString& fixedCode = QString());
    void runBackup();
    void runBackupCatalog();
    void runBackupDiagnostics();
    void runImport();
    void runPurge();

    const QString _directory;
    const QVariantMap _policy;
    const QString _requestId;
    const QVariantMap _request;
    const bool _configurationValid;
    std::atomic_bool _cancelled{false};
    mutable QMutex _mutex;
    QVariantMap _status;
};

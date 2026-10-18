#pragma once

#include <QMutex>
#include <QSet>
#include <QString>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QWaitCondition>

// Parentless, value-only local history writer. The native owner must request
// stop and delete only after QThread::finished; never GUI-wait or terminate.
// Neither a successful commit nor this object's state restores live owners,
// permissions, remote availability, clean coverage or physical release.
class FleetOccupancyJournalWorker final : public QThread
{
    Q_OBJECT
public:
    FleetOccupancyJournalWorker(const QString& absoluteAppDataDirectory,
                                const QString& writerEpoch);
    ~FleetOccupancyJournalWorker() override;

    // One coalescing slot: all accepted IDs remain in a bounded monotonic
    // union, including IDs belonging to an in-flight older request. Only the
    // latest request/identity labels the next write, not every prior offer.
    bool offerObservation(const QString& requestId, const QVariantList& actualObservedIds,
                          const QVariantMap& actualIdentity4);
    QVariantMap statusSnapshot() const;
    QVariantMap takeBootstrapResult();
    void requestStop();

protected:
    void run() override;

private:
    bool advanceLocked();
    void updateStatusLocked();
    void fail(const QString& fixedCode, bool replacementMayHaveOccurred = false);

    const QString _appDataDirectory;
    const QString _writerEpoch;
    const QString _workerRunId;
    const bool _configurationValid;
    mutable QMutex _mutex;
    QWaitCondition _wake;
    QSet<int> _offeredIds;
    QString _pendingRequestId;
    QVariantMap _pendingIdentity;
    QVariantMap _status;
    QVariantMap _bootstrapResult;
    quint64 _generation = 1;
    bool _accepting = false;
    bool _stopRequested = false;
    bool _pending = false;
    bool _inFlight = false;
    bool _terminal = false;
    bool _bootstrapPublished = false;
};

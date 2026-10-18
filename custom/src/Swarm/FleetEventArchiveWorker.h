#pragma once

#include "FleetEventArchiveCodec.h"

#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QThread>
#include <QVariantMap>
#include <QWaitCondition>

// Value-only storage worker. Construct without a parent and dispose only after
// QThread::finished. Neither this object nor the SQL connection owns a live
// BlackBox, Vehicle, approval, registry, replay, or command source.
class FleetEventArchiveWorker : public QThread
{
    Q_OBJECT
public:
    FleetEventArchiveWorker(const QString& absoluteDirectory,
                            const QVariantMap& exactPolicy,
                            const QString& runId);
    ~FleetEventArchiveWorker() override;

    bool enqueue(const FleetEventArchiveCodec::Record& record);
    bool noteExternalGap(quint64 lostCount = 1);
    bool requestPage(const QString& queryId, const QString& runFilter,
                     const QString& afterRowId, int limit);
    void requestStop(bool drain = true);
    bool requestStopAndSeal(int maximumSealedSegments = 32);
    QVariantMap current() const;

signals:
    void ready(const QString& runId);
    void failure(const QString& runId, const QString& fixedCode);
    void pageReady(const QString& runId, const QString& queryId,
                   const QVariantMap& historicalPage);

protected:
    void run() override;

private:
    struct PageRequest {
        QString queryId;
        QString runFilter;
        qint64 afterRowId = 0;
        int limit = 0;
    };

    void fail(const QString& fixedCode);
    void updatePendingLocked();
    bool countLocked(const QString& key, qint64 amount = 1);

    const QString _directory;
    const QVariantMap _policy;
    const QString _runId;
    bool _configurationValid = false;
    int _maximumQueueEvents = 0;
    qint64 _maximumQueueBytes = 0;
    int _batchEventLimit = 0;
    mutable QMutex _mutex;
    QWaitCondition _wake;
    QQueue<FleetEventArchiveCodec::Record> _records;
    QQueue<PageRequest> _queries;
    QSet<QString> _queryIds;
    qint64 _queuedBytes = 0;
    int _inFlightCount = 0;
    qint64 _inFlightBytes = 0;
    qint64 _lastOfferedSequence = 0;
    bool _accepting = false;
    bool _stopRequested = false;
    bool _drainRequested = true;
    bool _stopDecisionTaken = false;
    bool _sealRequested = false;
    int _maximumSealedSegments = 32;
    QVariantMap _status;
};

#pragma once

#include "FleetTelemetryArchiveCodec.h"

#include <QMap>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QWaitCondition>

// Parentless, value-only NDJSON writer. Owner connects finished -> deleteLater;
// no live Vehicle/source pointer, SQL, importer, replay or control permission.
class FleetTelemetryArchiveWorker : public QThread
{
    Q_OBJECT
public:
    FleetTelemetryArchiveWorker(const QString& directory, const QVariantMap& exactPolicy,
                                const QString& runId, bool includePositionTelemetry,
                                const QVariantList& exactSources);
    ~FleetTelemetryArchiveWorker() override;

    static bool policyValid(const QVariantMap& exactPolicy);
    bool enqueue(const FleetTelemetryArchiveCodec::Record& record);
    bool noteCaptureSummary(const QVariantMap& exactSummary);
    void requestStop(bool drain = true);
    QVariantMap current() const;

signals:
    void ready(const QString& runId);
    void failure(const QString& runId, const QString& fixedCode);

protected:
    void run() override;

private:
    bool admittedRecord(const FleetTelemetryArchiveCodec::Record& record) const;
    bool countLocked(const QString& key, qint64 amount = 1);
    void updatePendingLocked();
    bool stopBeforeOpen();
    void fail(const QString& fixedCode);

    const QString _directory;
    const QVariantMap _policy;
    const QString _runId;
    const bool _includePositionTelemetry;
    const QVariantList _sources;
    QMap<QString, int> _sourceVehicleIds;
    bool _configurationValid = false;
    int _maximumQueueFrames = 0;
    qint64 _maximumQueueBytes = 0;
    int _batchFrameLimit = 0;
    mutable QMutex _mutex;
    QWaitCondition _wake;
    QQueue<FleetTelemetryArchiveCodec::Record> _records;
    qint64 _queuedBytes = 0;
    int _inFlightFrames = 0;
    qint64 _inFlightBytes = 0;
    qint64 _lastOfferedSequence = 0;
    bool _accepting = false;
    bool _stopRequested = false;
    bool _drainRequested = true;
    bool _stopDecisionFinal = false;
    bool _captureSummaryProvided = false;
    bool _failureEmitted = false;
    QVariantMap _captureSummary;
    QVariantMap _status;
};

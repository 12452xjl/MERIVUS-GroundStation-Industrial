#pragma once

#include <QThread>
#include <QString>
#include <QVariantMap>

#include <atomic>

// One-shot value-only archive/file job. The native Lifecycle alone owns
// actual source capture, double-terminal acceptance and final retirement.
class FleetDeploymentArchiveWorker : public QThread
{
    Q_OBJECT
public:
    FleetDeploymentArchiveWorker(const QString& jobId, const QString& action,
        const QVariantMap& request, const QVariantMap& privateInput = {},
        const QVariantMap& privateArchiveCache = {});
    void requestCancel();

signals:
    // exact5 {ok,error,action,cache,reply}; cache is native-private only.
    void resultReady(const QString& jobId, const QVariantMap& result);

protected:
    void run() override;

private:
    const QString _jobId;
    const QString _action;
    const QVariantMap _request;
    const QVariantMap _input;
    const QVariantMap _cache;
    std::atomic_bool _cancelled{false};
};

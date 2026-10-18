#pragma once

#include <QString>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>

#include <atomic>

// One-shot, value-only GNSS history snapshot/file job. The actual Memory
// owner alone retains cache and arbitrates resultReady plus actual finished.
class FleetGnssMemoryWorkspaceWorker : public QThread
{
    Q_OBJECT
public:
    FleetGnssMemoryWorkspaceWorker(const QString& jobId, const QString& action,
                                  const QVariantMap& request,
                                  const QVariantMap& privateSnapshot = {},
                                  const QVariantList& freezeReports = {});
    void requestCancel();

signals:
    // exact5 {ok,error,action,cache,reply}; cache is never public QML data.
    void resultReady(const QString& jobId, const QVariantMap& result);

protected:
    void run() override;

private:
    const QString _jobId;
    const QString _action;
    const QVariantMap _request;
    const QVariantMap _snapshot;
    const QVariantList _reports;
    std::atomic_bool _cancelled{false};
};

#pragma once

#include <QString>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>

#include <atomic>

// One-shot native maintenance snapshot/file/page job. Only the owning
// Passport may retain the private cache in resultReady; it is never a QML
// document or an authority token. No live Passport/Archive is used in run().
class FleetMaintenanceWorkspaceWorker : public QThread
{
    Q_OBJECT
public:
    FleetMaintenanceWorkspaceWorker(const QString& jobId, const QString& action,
                                    const QVariantMap& request,
                                    const QVariantMap& privateSnapshot = {},
                                    const QVariantList& freezeRecords = {});

    void requestCancel();

signals:
    // exact5 {ok,error,action,cache,reply}. cache is native-private only.
    void resultReady(const QString& jobId, const QVariantMap& result);

protected:
    void run() override;

private:
    const QString _jobId;
    const QString _action;
    const QVariantMap _request;
    const QVariantMap _snapshot;
    const QVariantList _records;
    std::atomic_bool _cancelled{false};
};

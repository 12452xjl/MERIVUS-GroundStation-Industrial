#pragma once

#include <QThread>
#include <QVariantMap>
#include <atomic>
#include <memory>

// One immutable value calculation. No parent, live QObject, GUI, registry,
// authentication, file or command object is retained by the worker.
class FleetMissionPlanningWorker final : public QThread
{
public:
    using Cancellation = std::shared_ptr<std::atomic_bool>;
    FleetMissionPlanningWorker(const QVariantMap& task, const QVariantList& fleet,
                               const Cancellation& cancellation);

    // Bounded pure-JSON deep copy used before crossing the thread boundary.
    // Failure clears output. This function does not execute planning.
    static bool snapshot(const QVariantMap& input, QVariantMap* output);

    // Read only from the owning thread's queued QThread::finished handler.
    QVariantMap finishedPlan() const { return _plan; }
    QString finishedError() const { return _error; }

protected:
    void run() override;

private:
    const QVariantMap _task;
    const QVariantList _fleet;
    const Cancellation _cancellation;
    QVariantMap _plan;
    QString _error;
};

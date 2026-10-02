#include "SwarmMissionOrchestrator.h"

#include <QSet>
#include <QUuid>
#include <QtMath>

namespace {
const QString kCustom = QStringLiteral("custom");
const QString kDraft = QStringLiteral("draft");
const QString kRunning = QStringLiteral("running");
const QString kPaused = QStringLiteral("paused");
const QString kCompleted = QStringLiteral("completed");
const QString kCancelled = QStringLiteral("cancelled");
const QString kFailed = QStringLiteral("failed");
}

SwarmMissionOrchestrator::SwarmMissionOrchestrator(QObject* parent)
    : QObject(parent)
{
}

QVariantList SwarmMissionOrchestrator::taskSummaries() const
{
    QVariantList summaries;
    summaries.reserve(_taskOrder.size());
    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (task) {
            summaries.append(toSummary(*task));
        }
    }
    return summaries;
}

QVariantMap SwarmMissionOrchestrator::fleetSummary() const
{
    int draft = 0;
    int running = 0;
    int paused = 0;
    int completed = 0;
    int cancelled = 0;
    int failed = 0;
    int assignedVehicles = 0;
    QSet<QString> uniqueVehicles;

    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (!task) {
            continue;
        }
        switch (task->phase) {
        case Phase::Draft: ++draft; break;
        case Phase::Running: ++running; break;
        case Phase::Paused: ++paused; break;
        case Phase::Completed: ++completed; break;
        case Phase::Cancelled: ++cancelled; break;
        case Phase::Failed: ++failed; break;
        }
        assignedVehicles += task->targetVehicleIds.size();
        for (const QVariant& vehicleId : task->targetVehicleIds) {
            const QString key = vehicleKey(vehicleId);
            if (!key.isEmpty()) {
                uniqueVehicles.insert(key);
            }
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("taskCount"), _taskOrder.size());
    result.insert(QStringLiteral("draftCount"), draft);
    result.insert(QStringLiteral("runningCount"), running);
    result.insert(QStringLiteral("pausedCount"), paused);
    result.insert(QStringLiteral("completedCount"), completed);
    result.insert(QStringLiteral("cancelledCount"), cancelled);
    result.insert(QStringLiteral("failedCount"), failed);
    result.insert(QStringLiteral("assignedVehicleCount"), assignedVehicles);
    result.insert(QStringLiteral("uniqueVehicleCount"), uniqueVehicles.size());
    return result;
}

QString SwarmMissionOrchestrator::createTask(const QString& taskType,
                                             const QVariantList& targetVehicleIds,
                                             const QVariantMap& metadata)
{
    const QString normalizedType = taskType.trimmed().toLower();
    if (normalizedType.isEmpty()) {
        return QString();
    }

    TaskRecord task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type = normalizedType;
    task.targetVehicleIds = normalizeVehicleIds(targetVehicleIds);
    task.metadata = metadata;
    task.createdAt = QDateTime::currentDateTimeUtc();
    task.updatedAt = task.createdAt;
    _tasks.insert(task.id, task);
    _taskOrder.append(task.id);

    const QVariantMap summary = toSummary(task);
    emit taskCreated(task.id, summary);
    emit taskUpdated(task.id, summary);
    emitCollectionChanges();
    return task.id;
}

bool SwarmMissionOrchestrator::startTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    if (!task || task->phase != Phase::Draft) {
        return false;
    }
    task->phase = Phase::Running;
    task->reason.clear();
    task->startedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskStarted(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::pauseTask(const QString& taskId, const QString& reason)
{
    TaskRecord* task = findTask(taskId);
    if (!task || task->phase != Phase::Running) {
        return false;
    }
    task->phase = Phase::Paused;
    task->reason = reason.trimmed();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskPaused(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::resumeTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    if (!task || task->phase != Phase::Paused) {
        return false;
    }
    task->phase = Phase::Running;
    task->reason.clear();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskResumed(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::cancelTask(const QString& taskId, const QString& reason)
{
    TaskRecord* task = findTask(taskId);
    if (!task || isTerminal(task->phase)) {
        return false;
    }
    task->phase = Phase::Cancelled;
    task->reason = reason.trimmed().isEmpty() ? QStringLiteral("Cancelled") : reason.trimmed();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskCancelled(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::completeTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    if (!task || (task->phase != Phase::Running && task->phase != Phase::Paused)) {
        return false;
    }
    task->phase = Phase::Completed;
    task->progress = 1.0;
    task->reason.clear();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskCompleted(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::failTask(const QString& taskId, const QString& reason)
{
    TaskRecord* task = findTask(taskId);
    if (!task || isTerminal(task->phase)) {
        return false;
    }
    task->phase = Phase::Failed;
    task->reason = reason.trimmed().isEmpty() ? QStringLiteral("Task failed") : reason.trimmed();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskFailed(task->id, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::reassignTask(const QString& taskId,
                                            const QVariantList& targetVehicleIds)
{
    TaskRecord* task = findTask(taskId);
    if (!task || isTerminal(task->phase)) {
        return false;
    }
    const QVariantList normalized = normalizeVehicleIds(targetVehicleIds);
    if (normalized == task->targetVehicleIds) {
        return false;
    }
    const QVariantList previous = task->targetVehicleIds;
    task->targetVehicleIds = normalized;
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskReassigned(task->id, previous, normalized, summary);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::setTaskProgress(const QString& taskId, double progress)
{
    TaskRecord* task = findTask(taskId);
    if (!task || (task->phase != Phase::Running && task->phase != Phase::Paused)) {
        return false;
    }
    const double normalized = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(task->progress, normalized)) {
        return false;
    }
    task->progress = normalized;
    touch(*task);
    const QVariantMap summary = toSummary(*task);
    emit taskUpdated(task->id, summary);
    emitCollectionChanges();
    return true;
}

QVariantMap SwarmMissionOrchestrator::taskSummary(const QString& taskId) const
{
    const TaskRecord* task = findTask(taskId);
    return task ? toSummary(*task) : QVariantMap();
}

bool SwarmMissionOrchestrator::hasTask(const QString& taskId) const
{
    return findTask(taskId) != nullptr;
}

bool SwarmMissionOrchestrator::removeTask(const QString& taskId)
{
    const QString normalizedId = normalizeTaskId(taskId);
    TaskRecord* task = findTask(normalizedId);
    if (!task || !isTerminal(task->phase)) {
        return false;
    }
    _tasks.remove(normalizedId);
    _taskOrder.removeAll(normalizedId);
    emit taskCollectionChanged();
    emit fleetSummaryChanged(fleetSummary());
    return true;
}

QString SwarmMissionOrchestrator::taskTypeToString(TaskType type)
{
    switch (type) {
    case TaskType::Custom: return kCustom;
    case TaskType::Patrol: return QStringLiteral("patrol");
    case TaskType::Search: return QStringLiteral("search");
    case TaskType::Follow: return QStringLiteral("follow");
    case TaskType::Waypoint: return QStringLiteral("waypoint");
    case TaskType::Relay: return QStringLiteral("relay");
    }
    return kCustom;
}

QString SwarmMissionOrchestrator::phaseToString(Phase phase)
{
    switch (phase) {
    case Phase::Draft: return kDraft;
    case Phase::Running: return kRunning;
    case Phase::Paused: return kPaused;
    case Phase::Completed: return kCompleted;
    case Phase::Cancelled: return kCancelled;
    case Phase::Failed: return kFailed;
    }
    return kDraft;
}

QString SwarmMissionOrchestrator::normalizeTaskId(const QString& taskId)
{
    return taskId.trimmed();
}

QVariantList SwarmMissionOrchestrator::normalizeVehicleIds(const QVariantList& vehicleIds)
{
    QVariantList normalized;
    QSet<QString> seen;
    for (const QVariant& vehicleId : vehicleIds) {
        const QString key = vehicleKey(vehicleId);
        if (key.isEmpty() || seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        normalized.append(vehicleId);
    }
    return normalized;
}

QString SwarmMissionOrchestrator::vehicleKey(const QVariant& vehicleId)
{
    if (!vehicleId.isValid() || vehicleId.isNull()) {
        return QString();
    }
    return vehicleId.toString().trimmed();
}

QString SwarmMissionOrchestrator::timestamp(const QDateTime& value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODateWithMs) : QString();
}

bool SwarmMissionOrchestrator::isTerminal(Phase phase)
{
    return phase == Phase::Completed || phase == Phase::Cancelled || phase == Phase::Failed;
}

SwarmMissionOrchestrator::TaskRecord* SwarmMissionOrchestrator::findTask(const QString& taskId)
{
    const QString normalizedId = normalizeTaskId(taskId);
    if (normalizedId.isEmpty() || !_tasks.contains(normalizedId)) {
        return nullptr;
    }
    return &_tasks[normalizedId];
}

const SwarmMissionOrchestrator::TaskRecord* SwarmMissionOrchestrator::findTask(const QString& taskId) const
{
    const QString normalizedId = normalizeTaskId(taskId);
    if (normalizedId.isEmpty()) {
        return nullptr;
    }
    const auto iterator = _tasks.constFind(normalizedId);
    return iterator == _tasks.constEnd() ? nullptr : &iterator.value();
}

QVariantMap SwarmMissionOrchestrator::toSummary(const TaskRecord& task) const
{
    QVariantMap summary;
    summary.insert(QStringLiteral("taskId"), task.id);
    summary.insert(QStringLiteral("id"), task.id);
    summary.insert(QStringLiteral("taskType"), task.type);
    summary.insert(QStringLiteral("type"), task.type);
    summary.insert(QStringLiteral("phase"), phaseToString(task.phase));
    summary.insert(QStringLiteral("targetVehicleIds"), task.targetVehicleIds);
    summary.insert(QStringLiteral("vehicleIds"), task.targetVehicleIds);
    summary.insert(QStringLiteral("metadata"), task.metadata);
    summary.insert(QStringLiteral("reason"), task.reason);
    summary.insert(QStringLiteral("progress"), task.progress);
    summary.insert(QStringLiteral("terminal"), isTerminal(task.phase));
    summary.insert(QStringLiteral("createdAt"), timestamp(task.createdAt));
    summary.insert(QStringLiteral("startedAt"), timestamp(task.startedAt));
    summary.insert(QStringLiteral("updatedAt"), timestamp(task.updatedAt));
    summary.insert(QStringLiteral("finishedAt"), timestamp(task.finishedAt));
    return summary;
}

void SwarmMissionOrchestrator::touch(TaskRecord& task)
{
    task.updatedAt = QDateTime::currentDateTimeUtc();
}

void SwarmMissionOrchestrator::emitCollectionChanges()
{
    emit taskCollectionChanged();
    emit fleetSummaryChanged(fleetSummary());
}

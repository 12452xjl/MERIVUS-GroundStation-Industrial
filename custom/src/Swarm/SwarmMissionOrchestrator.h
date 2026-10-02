#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Transport-independent task lifecycle and assignment coordinator.
//
// This class deliberately does not depend on QGC Vehicle (or MAVLink) types.
// A QGC/MAVLink adapter can observe the signals and report progress through
// setTaskProgress(), while the orchestrator keeps task state deterministic and
// serialisable for QML, logging, and future backends.
class SwarmMissionOrchestrator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int taskCount READ taskCount NOTIFY taskCollectionChanged)
    Q_PROPERTY(QVariantList taskSummaries READ taskSummaries NOTIFY taskCollectionChanged)
    Q_PROPERTY(QVariantMap fleetSummary READ fleetSummary NOTIFY fleetSummaryChanged)

public:
    enum class TaskType {
        Custom,
        Patrol,
        Search,
        Follow,
        Waypoint,
        Relay,
    };
    Q_ENUM(TaskType)

    enum class Phase {
        Draft,
        Running,
        Paused,
        Completed,
        Cancelled,
        Failed,
    };
    Q_ENUM(Phase)

    explicit SwarmMissionOrchestrator(QObject* parent = nullptr);

    int taskCount() const { return _taskOrder.size(); }
    QVariantList taskSummaries() const;
    QVariantMap fleetSummary() const;

    // Returns a generated task ID. An empty taskType is rejected. Vehicle IDs
    // are de-duplicated while preserving their first-seen order.
    Q_INVOKABLE QString createTask(const QString& taskType,
                                   const QVariantList& targetVehicleIds = QVariantList(),
                                   const QVariantMap& metadata = QVariantMap());
    Q_INVOKABLE bool startTask(const QString& taskId);
    Q_INVOKABLE bool pauseTask(const QString& taskId, const QString& reason = QString());
    Q_INVOKABLE bool resumeTask(const QString& taskId);
    Q_INVOKABLE bool cancelTask(const QString& taskId, const QString& reason = QString());
    Q_INVOKABLE bool completeTask(const QString& taskId);
    Q_INVOKABLE bool failTask(const QString& taskId, const QString& reason = QString());

    // Replaces the assignment for a non-terminal task. This is intended for
    // leader loss, low-battery exits, and other task reallocation decisions.
    Q_INVOKABLE bool reassignTask(const QString& taskId,
                                  const QVariantList& targetVehicleIds);
    Q_INVOKABLE bool setTaskProgress(const QString& taskId, double progress);
    Q_INVOKABLE QVariantMap taskSummary(const QString& taskId) const;
    Q_INVOKABLE bool hasTask(const QString& taskId) const;
    Q_INVOKABLE bool removeTask(const QString& taskId);

    static QString taskTypeToString(TaskType type);
    static QString phaseToString(Phase phase);

signals:
    void taskCreated(const QString& taskId, const QVariantMap& summary);
    void taskStarted(const QString& taskId, const QVariantMap& summary);
    void taskPaused(const QString& taskId, const QVariantMap& summary);
    void taskResumed(const QString& taskId, const QVariantMap& summary);
    void taskCancelled(const QString& taskId, const QVariantMap& summary);
    void taskCompleted(const QString& taskId, const QVariantMap& summary);
    void taskFailed(const QString& taskId, const QVariantMap& summary);
    void taskReassigned(const QString& taskId,
                        const QVariantList& previousVehicleIds,
                        const QVariantList& targetVehicleIds,
                        const QVariantMap& summary);
    void taskUpdated(const QString& taskId, const QVariantMap& summary);
    void taskCollectionChanged();
    void fleetSummaryChanged(const QVariantMap& summary);

private:
    struct TaskRecord {
        QString id;
        QString type;
        Phase phase = Phase::Draft;
        QVariantList targetVehicleIds;
        QVariantMap metadata;
        QString reason;
        double progress = 0.0;
        QDateTime createdAt;
        QDateTime startedAt;
        QDateTime updatedAt;
        QDateTime finishedAt;
    };

    static QString normalizeTaskId(const QString& taskId);
    static QVariantList normalizeVehicleIds(const QVariantList& vehicleIds);
    static QString vehicleKey(const QVariant& vehicleId);
    static QString timestamp(const QDateTime& value);
    static bool isTerminal(Phase phase);

    TaskRecord* findTask(const QString& taskId);
    const TaskRecord* findTask(const QString& taskId) const;
    QVariantMap toSummary(const TaskRecord& task) const;
    void touch(TaskRecord& task);
    void emitCollectionChanges();

    QHash<QString, TaskRecord> _tasks;
    QStringList _taskOrder;
};

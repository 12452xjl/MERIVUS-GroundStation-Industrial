#pragma once

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>

#include <atomic>
#include <memory>

class FleetMissionReviewCoordinator;
class FleetMissionPlanStore;
class FleetMissionReportWorkspaceWorker;

// Independent read-only review dossiers. Never a protected actual Run report,
// permission source, PlanStore import or a caller-supplied executable plan.
class FleetMissionReportWorkspace final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    Q_PROPERTY(QVariantMap result READ result NOTIFY changed)
public:
    explicit FleetMissionReportWorkspace(QObject* parent = nullptr);
    ~FleetMissionReportWorkspace() override;
    QVariantMap state() const;
    QVariantMap summary() const;
    QVariantMap result() const;
    Q_INVOKABLE QVariantMap capture(FleetMissionReviewCoordinator* actualCoordinator,
        FleetMissionPlanStore* actualStore, const QVariantMap& expectedBinding, bool confirmed);
    Q_INVOKABLE QVariantMap requestFile(const QString& action, const QString& path,
        const QString& expectedJobId, const QString& expectedSnapshotId,
        bool overwriteConfirmed, bool confirmed);
    Q_INVOKABLE bool cancel(const QString& expectedJobId);
    Q_INVOKABLE bool clear(const QString& expectedSnapshotId);
    Q_INVOKABLE QVariantMap viewPage(const QString& expectedSnapshotId,
        const QString& audience, const QVariant& offset) const;
signals:
    void changed();
private:
    bool available() const;
    QVariantMap start(FleetMissionReportWorkspaceWorker* worker);
    void finish(FleetMissionReportWorkspaceWorker* worker, const QString& jobId);
    void disconnectCapture();
    QPointer<FleetMissionReportWorkspaceWorker> _worker;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetMissionPlanStore> _store;
    QMetaObject::Connection _reviewConnection;
    std::shared_ptr<std::atomic_bool> _sourceChanged;
    QVariantMap _captureBinding;
    QVariantMap _captureReport;
    QVariantMap _summary;
    QVariantMap _result;
    QVariantMap _report;
    QMap<QString, QString> _views;
    QString _snapshotId = QStringLiteral("");
    QString _jobId = QStringLiteral("");
    QString _action = QStringLiteral("");
    QString _stage = QStringLiteral("idle");
    bool _busy = false;
    bool _entryBusy = false;
    bool _cancelRequested = false;
    bool _destroying = false;
};

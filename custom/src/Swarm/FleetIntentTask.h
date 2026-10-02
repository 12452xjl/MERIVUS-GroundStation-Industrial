#pragma once

#include <QObject>
#include <QVariantMap>

// Transport independent, human approval gated representation of a fleet task.
class FleetIntentTask : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString taskId READ taskId CONSTANT)
    Q_PROPERTY(QString intent READ intent WRITE setIntent NOTIFY intentChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool manualApprovalRequired READ manualApprovalRequired CONSTANT)
    Q_PROPERTY(QVariantMap parameters READ parameters WRITE setParameters NOTIFY parametersChanged)
    Q_PROPERTY(QVariantMap preview READ preview NOTIFY previewChanged)
public:
    explicit FleetIntentTask(QObject* parent = nullptr);

    QString taskId() const { return _taskId; }
    QString intent() const { return _intent; }
    QString state() const { return _state; }
    bool manualApprovalRequired() const { return true; }
    QVariantMap parameters() const { return _parameters; }
    QVariantMap preview() const { return _preview; }

    void setIntent(const QString& value);
    void setParameters(const QVariantMap& value);

    Q_INVOKABLE void preparePreview();
    Q_INVOKABLE bool requestApproval();
    Q_INVOKABLE bool approve();
    Q_INVOKABLE bool reject(const QString& reason = QString());
    Q_INVOKABLE void cancel();

signals:
    void intentChanged();
    void stateChanged();
    void parametersChanged();
    void previewChanged();

private:
    QString _taskId;
    QString _intent;
    QString _state = QStringLiteral("draft");
    QVariantMap _parameters;
    QVariantMap _preview;
    QString _approvalReason;
};

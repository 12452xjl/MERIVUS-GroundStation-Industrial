#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// Transport-independent command transaction. A QGC/MAVLink adapter can map
// vehicle acknowledgements to the markVehicle* methods without exposing QGC
// Vehicle types to this class.
class CommandTransaction : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString transactionId READ transactionId CONSTANT)
    Q_PROPERTY(QString command READ command CONSTANT)
    Q_PROPERTY(QVariantList targetVehicleIds READ targetVehicleIds CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int timeoutMs READ timeoutMs WRITE setTimeoutMs NOTIFY timeoutMsChanged)
    Q_PROPERTY(int maxRetries READ maxRetries WRITE setMaxRetries NOTIFY maxRetriesChanged)
    Q_PROPERTY(int retryCount READ retryCount NOTIFY retryCountChanged)
    Q_PROPERTY(int attempt READ attempt NOTIFY attemptChanged)
    Q_PROPERTY(double completionRatio READ completionRatio NOTIFY progressChanged)
    Q_PROPERTY(bool terminal READ isTerminal NOTIFY statusChanged)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY retryAvailableChanged)
    Q_PROPERTY(QVariantMap resultSummary READ resultSummary NOTIFY resultChanged)

public:
    enum class Status {
        Pending,
        Running,
        Succeeded,
        PartiallySucceeded,
        Failed,
        TimedOut,
        Cancelled,
    };
    Q_ENUM(Status)

    explicit CommandTransaction(const QString& command = QString(),
                                const QVariantList& targetVehicleIds = QVariantList(),
                                QObject* parent = nullptr);

    QString transactionId() const { return _transactionId; }
    QString command() const { return _command; }
    QVariantList targetVehicleIds() const { return _targetVehicleIds; }
    QString status() const { return statusToString(_status); }
    Status statusValue() const { return _status; }
    int timeoutMs() const { return _timeoutMs; }
    int maxRetries() const { return _maxRetries; }
    int retryCount() const { return _retryCount; }
    int attempt() const { return _attempt; }
    double completionRatio() const;
    bool isTerminal() const;
    bool canRetry() const;
    QVariantMap resultSummary() const;

    void setTimeoutMs(int timeoutMs);
    void setMaxRetries(int maxRetries);

    Q_INVOKABLE bool start();
    Q_INVOKABLE bool beginRetry();
    Q_INVOKABLE bool markVehicleSucceeded(const QVariant& vehicleId,
                                          const QVariantMap& details = QVariantMap());
    Q_INVOKABLE bool markVehicleFailed(const QVariant& vehicleId,
                                       const QString& reason = QString(),
                                       const QVariantMap& details = QVariantMap());
    Q_INVOKABLE bool markVehicleTimedOut(const QVariant& vehicleId,
                                         const QString& reason = QString());
    Q_INVOKABLE bool cancel(const QString& reason = QString());
    Q_INVOKABLE bool checkTimeout();
    Q_INVOKABLE QVariantMap vehicleResult(const QVariant& vehicleId) const;

    static QString statusToString(Status status);

signals:
    void statusChanged();
    void timeoutMsChanged();
    void maxRetriesChanged();
    void retryCountChanged();
    void attemptChanged();
    void progressChanged();
    void resultChanged();
    void retryAvailableChanged();
    void timeoutReached();
    void retryRequested(int nextAttempt);

private slots:
    void _handleTimeout();

private:
    struct VehicleState {
        QVariant id;
        QString state;
        QString reason;
        QVariantMap details;
    };

    QString _vehicleKey(const QVariant& vehicleId) const;
    bool _setVehicleState(const QVariant& vehicleId, const QString& state,
                          const QString& reason, const QVariantMap& details);
    void _startAttempt(bool resetRetryableVehicles);
    void _stopTimer();
    void _finishCurrentAttempt();
    void _setStatus(Status status);
    void _emitResultChanges();
    QVariantList _idsForState(const QString& state) const;
    int _countForState(const QString& state) const;
    QString _summaryMessage() const;

    QString _transactionId;
    QString _command;
    QVariantList _targetVehicleIds;
    QStringList _vehicleOrder;
    QHash<QString, VehicleState> _vehicles;
    Status _status = Status::Pending;
    int _timeoutMs = 5000;
    int _maxRetries = 0;
    int _retryCount = 0;
    int _attempt = 0;
    QString _message;
    QDateTime _startedAt;
    QDateTime _finishedAt;
    QTimer _timeoutTimer;
};

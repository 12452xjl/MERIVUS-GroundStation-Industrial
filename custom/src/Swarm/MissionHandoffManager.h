#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>

// Coordinates task ownership handoff between human operators or control
// centers.  It records intent and acknowledgement only; it does not dispatch
// a mission, change vehicle mode, or bypass flight-safety gates.
class MissionHandoffManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY handoffCollectionChanged)
    Q_PROPERTY(QVariantList handoffs READ handoffs NOTIFY handoffCollectionChanged)

public:
    enum class Status {
        Pending,
        Accepted,
        Rejected,
        Cancelled,
        Expired,
    };
    Q_ENUM(Status)

    explicit MissionHandoffManager(QObject* parent = nullptr);

    int pendingCount() const;
    QVariantList handoffs() const;

    Q_INVOKABLE QString requestHandoff(const QString& taskId,
                                       const QString& fromOperator,
                                       const QString& toOperator,
                                       const QString& reason = QString(),
                                       const QVariantMap& metadata = QVariantMap(),
                                       bool requiresSecondApproval = true,
                                       int expirySeconds = 300);
    Q_INVOKABLE bool acceptHandoff(const QString& handoffId,
                                   const QString& acceptingOperator,
                                   const QString& note = QString());
    Q_INVOKABLE bool rejectHandoff(const QString& handoffId,
                                   const QString& rejectingOperator,
                                   const QString& note = QString());
    Q_INVOKABLE bool cancelHandoff(const QString& handoffId,
                                   const QString& cancellingOperator,
                                   const QString& note = QString());
    Q_INVOKABLE bool expireHandoff(const QString& handoffId,
                                   const QString& note = QString());
    Q_INVOKABLE QVariantMap handoff(const QString& handoffId) const;
    Q_INVOKABLE bool hasHandoff(const QString& handoffId) const;
    Q_INVOKABLE bool isPending(const QString& handoffId) const;

    static QString statusToString(Status status);

signals:
    void handoffRequested(const QString& handoffId, const QVariantMap& summary);
    void handoffAccepted(const QString& handoffId, const QVariantMap& summary);
    void handoffRejected(const QString& handoffId, const QVariantMap& summary);
    void handoffCancelled(const QString& handoffId, const QVariantMap& summary);
    void handoffExpired(const QString& handoffId, const QVariantMap& summary);
    void handoffCollectionChanged();

private:
    void _expireDueHandoffs();

    struct Record {
        QString id;
        QString taskId;
        QString fromOperator;
        QString toOperator;
        QString reason;
        QString note;
        QVariantMap metadata;
        Status status = Status::Pending;
        bool requiresSecondApproval = true;
        QDateTime createdAt;
        QDateTime expiresAt;
        QDateTime resolvedAt;
        QString resolvedBy;
    };

    static QString timestamp(const QDateTime& value);
    QVariantMap toSummary(const Record& record) const;
    Record* find(const QString& handoffId);
    const Record* find(const QString& handoffId) const;
    bool resolve(const QString& handoffId, Status status, const QString& operatorId,
                 const QString& note);

    QHash<QString, Record> _records;
    QStringList _order;
    quint64 _sequence = 0;
    QTimer _expiryTimer;
};

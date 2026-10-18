#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QPointer>
#include <QJsonObject>
#include <atomic>
#include <memory>

class MissionHandoffHistoryWorker;

// Records local informational handoff intent and receipt between operators.
// It does not transfer task ownership or control between stations, dispatch
// a mission, change vehicle mode, or bypass flight-safety gates.
class MissionHandoffManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY handoffCollectionChanged)
    Q_PROPERTY(QVariantList handoffs READ handoffs NOTIFY handoffCollectionChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion CONSTANT)
    Q_PROPERTY(QVariantMap historyState READ historyState NOTIFY historyWorkspaceChanged)
    Q_PROPERTY(QVariantMap historyPreview READ historyPreview NOTIFY historyWorkspaceChanged)
    Q_PROPERTY(QVariantMap historyResult READ historyResult NOTIFY historyWorkspaceChanged)

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
    ~MissionHandoffManager() override;

    int pendingCount() const;
    QVariantList handoffs() const;
    int schemaVersion() const { return kSchemaVersion; }

    Q_INVOKABLE QString requestHandoff(const QString& taskId,
                                       const QString& fromOperator,
                                       const QString& toOperator,
                                       const QString& reason = QString(),
                                       const QVariantMap& metadata = QVariantMap(),
                                       bool requiresSecondApproval = true,
                                       int expirySeconds = 300);
    // An immutable local Run briefing, not transfer of vehicle control or
    // restoration of a startup approval. The receiving acknowledgement and
    // 300 s expiry cannot be weakened by a caller.
    Q_INVOKABLE QString requestRunHandoff(const QVariantMap& brief,
                                          const QString& fromOperator,
                                          const QString& toOperator,
                                          const QString& reason = QString());
    Q_INVOKABLE bool acceptHandoff(const QString& handoffId,
                                   const QString& acceptingOperator,
                                   const QString& note = QString(),
                                   quint64 expectedRevision = 0);
    Q_INVOKABLE bool rejectHandoff(const QString& handoffId,
                                   const QString& rejectingOperator,
                                   const QString& note = QString(),
                                   quint64 expectedRevision = 0);
    Q_INVOKABLE bool cancelHandoff(const QString& handoffId,
                                   const QString& cancellingOperator,
                                   const QString& note = QString(),
                                   quint64 expectedRevision = 0);
    Q_INVOKABLE bool expireHandoff(const QString& handoffId,
                                   const QString& note = QString());
    Q_INVOKABLE QVariantMap handoff(const QString& handoffId) const;
    // Zero-based, newest-first pages of four small summaries. No metadata or
    // complete briefing is copied until handoff(id) is explicitly requested.
    Q_INVOKABLE QVariantMap handoffPage(const QString& actorId,
                                       const QString& filter,
                                       const QVariant& page) const;
    Q_INVOKABLE bool hasHandoff(const QString& handoffId) const;
    Q_INVOKABLE bool isPending(const QString& handoffId) const;
    // Versioned, audit-only snapshot. Import is atomic and never releases a
    // flight command or restores an active control session.
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);

    // Separate historical workspace. Loading never replaces the live library;
    // only an explicit merge may append the privately decoded candidates.
    Q_INVOKABLE QVariantMap requestHistory(QVariantMap exactIntent, bool confirmed);
    QVariantMap historyState() const;
    QVariantMap historyPreview() const;
    QVariantMap historyResult() const;
    Q_INVOKABLE QVariantMap historyPreviewPage(const QString& previewId, const QVariant& page) const;
    Q_INVOKABLE QVariantMap historyPreviewRecord(const QString& previewId, const QString& handoffId) const;

    static QString statusToString(Status status);

signals:
    void handoffRequested(const QString& handoffId, const QVariantMap& summary);
    void handoffAccepted(const QString& handoffId, const QVariantMap& summary);
    void handoffRejected(const QString& handoffId, const QVariantMap& summary);
    void handoffCancelled(const QString& handoffId, const QVariantMap& summary);
    void handoffExpired(const QString& handoffId, const QVariantMap& summary);
    void handoffCollectionChanged();
    void historyWorkspaceChanged();

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
        quint64 revision = 0;
        QDateTime createdAt;
        QDateTime expiresAt;
        QDateTime resolvedAt;
        QString resolvedBy;
        // Runtime provenance only: never imported from or written to JSON.
        bool imported = false;
        // New workspace imports, including generic pending records, are inert.
        // Kept private so a schema-1 file cannot assert current provenance.
        bool historicalOnly = false;
    };

    struct Snapshot {
        QHash<QString, Record> records;
        QStringList order;
        quint64 sequence = 0;
        quint64 revision = 0;
        QHash<QString, QByteArray> recordRepresentations;
    };
    friend class MissionHandoffHistoryWorker;
    static QJsonObject persistentRecord(const Record& record);
    static bool encodeSnapshot(const Snapshot& snapshot, QByteArray& bytes,
                               const std::atomic_bool* cancelled = nullptr);
    static bool decodeSnapshot(const QByteArray& bytes, Snapshot& snapshot,
                               const std::atomic_bool* cancelled = nullptr);
    static QVariantMap pageRow(const Record& record);
    void finishHistory(MissionHandoffHistoryWorker* worker, const QString& jobId);

    static QString timestamp(const QDateTime& value);
    static Status statusFromString(const QString& status, bool* ok = nullptr);
    QVariantMap toSummary(const Record& record) const;
    Record* find(const QString& handoffId);
    const Record* find(const QString& handoffId) const;
    // Resolution re-checks the deadline synchronously; the periodic timer is
    // only housekeeping and must not create an expiry race with user actions.
    bool resolve(const QString& handoffId, Status status, const QString& operatorId,
                 const QString& note, quint64 expectedRevision = 0);

    QHash<QString, Record> _records;
    QStringList _order;
    quint64 _sequence = 0;
    quint64 _revision = 0;
    QTimer _expiryTimer;

    bool _historyEntryBusy = false;
    bool _historyBusy = false;
    bool _historyCancelRequested = false;
    bool _historyDestroying = false;
    QString _historyJobId = QStringLiteral("");
    QString _historyAction = QStringLiteral("");
    QString _historyStage = QStringLiteral("idle");
    QString _historyPreviewId = QStringLiteral("");
    QVariantMap _historyPreviewDescriptor;
    QVariantMap _historyResult;
    std::shared_ptr<const Snapshot> _historyPreviewSnapshot;
    QPointer<MissionHandoffHistoryWorker> _historyWorker;
    QString _collectionGeneration;

    static const int kSchemaVersion = 1;
};

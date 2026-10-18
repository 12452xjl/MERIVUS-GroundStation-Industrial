#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QPointer>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <memory>

class FleetMissionEvidenceFileWorker;

// A transport-independent, append-only evidence chain for mission review.
//
// This class deliberately has no vehicle, command, or mission-manager
// dependency.  It only stores normalized evidence supplied by the ground
// station and links each record to the previous record with SHA-256.  A
// verified JSON snapshot can be imported atomically; malformed or tampered
// snapshots are rejected without changing the current chain.  Each payload
// also has bounded depth/size so an untrusted handoff file cannot turn hash
// verification into an unbounded recursive or allocation workload.
class FleetMissionEvidenceChain : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(int count READ count NOTIFY recordsChanged)
    Q_PROPERTY(QString headHash READ headHash NOTIFY headHashChanged)
    Q_PROPERTY(bool fileOperationBusy READ fileOperationBusy NOTIFY fileOperationChanged)
    Q_PROPERTY(QVariantMap fileOperationStatus READ fileOperationStatus NOTIFY fileOperationChanged)
    Q_PROPERTY(QVariantMap historicalSummary READ historicalSummary NOTIFY fileOperationChanged)

public:
    explicit FleetMissionEvidenceChain(QObject* parent = nullptr);
    ~FleetMissionEvidenceChain() override;

    QVariantList records() const { return _records; }
    int count() const { return _records.size(); }
    QString headHash() const { return _headHash; }

    // Append one immutable evidence record.  `type` should identify a
    // mission-plan, capability, approval, risk, event, or final-result
    // evidence category.  No action is sent to a vehicle.
    Q_INVOKABLE bool appendEvidence(const QString& type,
                                    const QVariantMap& payload = QVariantMap(),
                                    qint64 timestampMs = 0);

    // Export/import a versioned snapshot.  Import validates every link and
    // hash before atomically replacing the in-memory snapshot.
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);

    // Recompute every record and link.  This is safe to call before handing a
    // snapshot to an engineer or persisting it to disk.
    Q_INVOKABLE bool verifyChain() const;
    Q_INVOKABLE QVariantMap recordAt(int index) const;

    bool fileOperationBusy() const;
    QVariantMap fileOperationStatus() const;
    QVariantMap historicalSummary() const;
    // A nonempty returned UUID acknowledges admission, not file completion.
    // New file APIs never replace the active chain or restore authority.
    Q_INVOKABLE QString requestSaveSnapshot(const QString& path, const QVariant& expectedCount,
        const QString& expectedHead, bool overwriteConfirmed);
    Q_INVOKABLE QString requestLoadHistoricalSnapshot(const QString& path);
    Q_INVOKABLE bool cancelFileOperation(const QString& requestId);
    Q_INVOKABLE QVariantMap historyPage(const QString& expectedRequestId, const QVariant& offset) const;
    Q_INVOKABLE QVariantMap historicalRecord(const QString& expectedRequestId, const QVariant& index) const;

signals:
    void recordsChanged();
    void headHashChanged();
    void evidenceAppended(const QVariantMap& evidence);
    void fileOperationChanged();

private:
    friend class FleetMissionEvidenceFileWorker;
    struct FileSnapshot;
    QString startFileOperation(const QString& action, const QString& path,
        const QVariantList& records, const QString& head);
    void finishFileOperation(FleetMissionEvidenceFileWorker* actualWorker, const QString& requestId);
    static const int kFormatVersion = 1;
    static const int kMaxRecords = 100000;

    static QString hashRecord(const QVariantMap& recordWithoutHash);
    static QByteArray canonicalJson(const QVariantMap& value);
    static bool verifyRecords(const QVariantList& records, const QString& expectedHeadHash,
        QString* error = nullptr, const std::atomic_bool* cancelled = nullptr);
    static bool isSha256(const QString& value, bool allowEmpty);
    static bool isIntegerJsonNumber(const QVariant& value);
    static bool payloadWithinLimits(const QVariant& value,
                                    int depth = 0,
                                    int* nodeCount = nullptr,
                                    qint64* scalarBytes = nullptr);
    static QVariantMap recordWithoutHash(const QVariantMap& record);

    QVariantList _records;
    QString _headHash;
    QPointer<FleetMissionEvidenceFileWorker> _fileWorker;
    std::shared_ptr<const FileSnapshot> _historical;
    QVariantMap _fileStatus;
    QString _fileRequestId = QStringLiteral("");
    bool _fileBusy = false;
    bool _fileEntryBusy = false;
    bool _fileDestroying = false;
};

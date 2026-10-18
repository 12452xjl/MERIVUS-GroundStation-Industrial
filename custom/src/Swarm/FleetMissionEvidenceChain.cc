#include "FleetMissionEvidenceChain.h"
#include "FleetSnapshotFileIo.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QVariantHash>
#include <QVector>
#include <QtMath>
#include <cmath>
#include <utility>

namespace {

constexpr qint64 kMaxEvidenceSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr qint64 kFutureEvidenceToleranceMs = 5LL * 60LL * 1000LL;
// These limits are intentionally below the whole-snapshot limit.  Evidence
// payloads are supplied by adapters/imports and are hashed recursively; bound
// them before canonicalization so a valid-looking file cannot exhaust the UI
// thread with deeply nested maps or very large strings.
constexpr int kMaxEvidencePayloadDepth = 16;
constexpr int kMaxEvidencePayloadNodes = 50000;
constexpr qint64 kMaxEvidencePayloadBytes = 512LL * 1024LL;
constexpr int kMaxEvidenceTypeLength = 128;
constexpr int kHistoryPreviewCharacters = 8192;
constexpr qint64 kHistoryPreviewBytes = 32LL * 1024LL * 1024LL;

QString explicitText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

QVariantMap fileStatus(const QString& id, const QString& action, const QString& path,
    int count, const QString& head, bool busy)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), id},
        {QStringLiteral("action"), action}, {QStringLiteral("stage"), busy ? QStringLiteral("running") : QStringLiteral("idle")},
        {QStringLiteral("busy"), busy}, {QStringLiteral("cancelRequested"), false},
        {QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("filePath"), path}, {QStringLiteral("snapshotCount"), count},
        {QStringLiteral("snapshotHeadHash"), explicitText(head)},
        {QStringLiteral("fileWriteAttempted"), false}, {QStringLiteral("fileWrittenKnown"), false},
        {QStringLiteral("fileWritten"), false}, {QStringLiteral("readbackVerified"), false},
        {QStringLiteral("readbackError"), QStringLiteral("")}, {QStringLiteral("fsyncVerified"), false},
        {QStringLiteral("durable"), false}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool fileIndex(const QVariant& value, int maximum, int* output)
{
    if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
        && value.type() != QVariant::Double)) return false;
    const double number = value.toDouble();
    if (!qIsFinite(number) || number < 0.0 || number > maximum || std::floor(number) != number) return false;
    *output = static_cast<int>(number);
    return true;
}

// This lower-bound accounting precedes ANY whole-chain serialization. A node
// needs at least one JSON byte, and string UTF-16 length never exceeds its
// JSON UTF-8 representation. Actual encoded bytes are checked incrementally
// afterwards. Do not use a six-times escaping upper bound to reject an
// otherwise ordinary near-limit ASCII snapshot.
bool fileValueBudget(const QVariant& value, int depth, qint64* units,
    const std::atomic_bool& cancelled)
{
    if (cancelled.load() || depth > 20 || ++*units > kMaxEvidenceSnapshotBytes) return false;
    const auto add = [units](qint64 amount) {
        if (amount < 0 || amount > kMaxEvidenceSnapshotBytes - *units) return false;
        *units += amount;
        return true;
    };
    if (!value.isValid() || value.isNull()) return true;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            if (!add(it.key().size() + qint64(2)) || !fileValueBudget(it.value(), depth + 1, units, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash map = value.toHash();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            if (!add(it.key().size() + qint64(2)) || !fileValueBudget(it.value(), depth + 1, units, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& child : value.toList())
            if (!fileValueBudget(child, depth + 1, units, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& child : value.toStringList())
            if (cancelled.load() || !add(child.size() + qint64(1))) return false;
        return true;
    }
    if (value.type() == QVariant::String) return add(value.toString().size());
    if (value.type() == QVariant::ByteArray) return add((qint64(value.toByteArray().size()) + 2) / 3 * 4);
    if (value.userType() == QMetaType::Float) return true;
    switch (value.type()) {
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt:
    case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
    case QVariant::Date: case QVariant::Time: case QVariant::DateTime:
        return true;
    default:
        // Do not invoke opaque QVariant converters or touch a GUI QObject on
        // the worker. The legacy synchronous APIs retain their original domain.
        return false;
    }
}

QJsonValue canonicalValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QJsonValue(QJsonValue::Null);
    }

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        QJsonObject object;
        QStringList keys = map.keys();
        keys.sort(Qt::CaseSensitive);
        for (const QString& key : keys) {
            object.insert(key, canonicalValue(map.value(key)));
        }
        return object;
    }

    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        QJsonObject object;
        QStringList keys = hash.keys();
        keys.sort(Qt::CaseSensitive);
        for (const QString& key : keys) {
            object.insert(key, canonicalValue(hash.value(key)));
        }
        return object;
    }

    if (value.type() == QVariant::List) {
        QJsonArray array;
        const QVariantList list = value.toList();
        for (const QVariant& item : list) {
            array.append(canonicalValue(item));
        }
        return array;
    }

    if (value.type() == QVariant::StringList) {
        QJsonArray array;
        const QStringList list = value.toStringList();
        for (const QString& item : list) {
            array.append(item);
        }
        return array;
    }

    if (value.type() == QVariant::ByteArray) {
        return QString::fromLatin1(value.toByteArray().toBase64());
    }

    return QJsonValue::fromVariant(value);
}

QString validationError(const QString& text, QString* error)
{
    if (error) {
        *error = text;
    }
    return text;
}

} // namespace

struct FleetMissionEvidenceChain::FileSnapshot {
    QString requestId;
    QString path;
    QVariantList records;
    QString head;
    QVector<QString> previews;
    QVector<bool> truncated;
};

// Private one-shot native worker: no Q_OBJECT, live chain or GUI references.
// Its result is read only after the original QThread::finished delivery.
class FleetMissionEvidenceFileWorker final : public QThread
{
public:
    FleetMissionEvidenceFileWorker(QString id, QString action, QString path,
        QVariantList records, QString head)
        : _id(std::move(id)), _action(std::move(action)), _path(std::move(path)),
          _records(std::move(records)), _head(std::move(head)) {}
    void requestCancel() { _cancelled.store(true); }
    bool cancelled() const { return _cancelled.load(); }
    QVariantMap status;
    std::shared_ptr<const FleetMissionEvidenceChain::FileSnapshot> snapshot;
protected:
    void run() override
    {
        status = fileStatus(_id, _action, _path, _records.size(), _head, false);
        QString error;
        if (_action == QStringLiteral("load")) {
            QByteArray input;
            if (!FleetSnapshotFileIo::readUtf8(_path, kMaxEvidenceSnapshotBytes, _cancelled,
                    QStringLiteral("evidence-history"), &input, &error)) { fail(error); return; }
            if (_cancelled.load()) { fail(QStringLiteral("evidence-history-cancelled")); return; }
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(input, &parseError);
            if (_cancelled.load()) { fail(QStringLiteral("evidence-history-cancelled")); return; }
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                fail(QStringLiteral("evidence-history-json-invalid")); return;
            }
            const QJsonObject root = document.object();
            if (root.size() != 4 || root.value(QStringLiteral("format")).toString() != QStringLiteral("merivus.mission-evidence-chain")
                || !root.value(QStringLiteral("version")).isDouble() || root.value(QStringLiteral("version")).toDouble() != 1.0
                || !root.value(QStringLiteral("headHash")).isString() || !root.value(QStringLiteral("records")).isArray()
                || root.value(QStringLiteral("records")).toArray().size() > FleetMissionEvidenceChain::kMaxRecords) {
                fail(QStringLiteral("evidence-history-format-invalid")); return;
            }
            _head = root.value(QStringLiteral("headHash")).toString();
            const QJsonArray rows = root.value(QStringLiteral("records")).toArray();
            _records.reserve(rows.size());
            for (const QJsonValue& row : rows) {
                if (_cancelled.load()) { fail(QStringLiteral("evidence-history-cancelled")); return; }
                if (!row.isObject()) { fail(QStringLiteral("evidence-history-record-invalid")); return; }
                _records.append(row.toObject().toVariantMap());
            }
        }
        qint64 units = 0;
        for (const QVariant& row : _records) {
            if (!fileValueBudget(row, 0, &units, _cancelled)) {
                fail(_cancelled.load() ? QStringLiteral("evidence-history-cancelled") : QStringLiteral("evidence-history-value-budget-or-type")); return;
            }
            const QVariant type = row.toMap().value(QStringLiteral("type"));
            if (type.type() != QVariant::String || type.isNull() || type.toString().size() > kMaxEvidenceTypeLength) {
                fail(QStringLiteral("evidence-history-type-invalid")); return;
            }
        }
        if (!FleetMissionEvidenceChain::verifyRecords(_records, _head, &error, &_cancelled)) {
            fail(_cancelled.load() ? QStringLiteral("evidence-history-cancelled") : QStringLiteral("evidence-history-chain-invalid")); return;
        }
        if (_action == QStringLiteral("save")) {
            // Append each independently bounded record; never materialize a
            // potentially 100000 * 512 KiB JSON tree and only then check size.
            QJsonArray head;
            head.append(_head);
            QByteArray headBytes = QJsonDocument(head).toJson(QJsonDocument::Compact);
            QByteArray bytes("{\"format\":\"merivus.mission-evidence-chain\",\"headHash\":");
            bytes.append(headBytes.mid(1, headBytes.size() - 2));
            bytes.append(",\"records\":[");
            const QByteArray suffix("],\"version\":1}");
            for (int index = 0; index < _records.size(); ++index) {
                if (_cancelled.load()) { fail(QStringLiteral("evidence-history-cancelled")); return; }
                const QByteArray encoded = FleetMissionEvidenceChain::canonicalJson(_records.at(index).toMap());
                if (encoded.size() + qint64(index ? 1 : 0)
                    > kMaxEvidenceSnapshotBytes - bytes.size() - suffix.size()) {
                    fail(QStringLiteral("evidence-history-file-budget")); return;
                }
                if (index) bytes.append(',');
                bytes.append(encoded);
            }
            bytes.append(suffix);
            const auto saved = FleetSnapshotFileIo::saveAndReadback(_path, bytes, kMaxEvidenceSnapshotBytes,
                _cancelled, QStringLiteral("evidence-history"));
            status.insert(QStringLiteral("fileWriteAttempted"), saved.fileWriteAttempted);
            status.insert(QStringLiteral("fileWrittenKnown"), saved.fileWrittenKnown);
            status.insert(QStringLiteral("fileWritten"), saved.fileWritten);
            status.insert(QStringLiteral("readbackVerified"), saved.readbackVerified);
            status.insert(QStringLiteral("readbackError"), explicitText(saved.readbackError));
            if (!saved.ok) { fail(saved.error); return; }
            status.insert(QStringLiteral("error"), explicitText(saved.error)); // Commit survives readback failure/cancel.
        } else {
            auto candidate = std::make_shared<FleetMissionEvidenceChain::FileSnapshot>();
            candidate->requestId = _id; candidate->path = _path; candidate->records = _records; candidate->head = explicitText(_head);
            qint64 previewBytes = 0;
            candidate->previews.reserve(_records.size()); candidate->truncated.reserve(_records.size());
            for (const QVariant& row : _records) {
                if (_cancelled.load()) { fail(QStringLiteral("evidence-history-cancelled")); return; }
                const QByteArray payload = FleetMissionEvidenceChain::canonicalJson(row.toMap().value(QStringLiteral("payload")).toMap());
                const QString text = QString::fromUtf8(payload);
                int length = qMin(text.size(), kHistoryPreviewCharacters);
                if (length && length < text.size() && text.at(length - 1).isHighSurrogate()) --length;
                const qint64 size = qint64(length) * 2;
                if (size > kHistoryPreviewBytes - previewBytes) { fail(QStringLiteral("evidence-history-preview-budget")); return; }
                previewBytes += size;
                candidate->previews.append(text.left(length)); candidate->truncated.append(length < text.size());
            }
            snapshot = std::move(candidate);
            status.insert(QStringLiteral("fileWrittenKnown"), true); // Load never writes an output.
        }
        status.insert(QStringLiteral("snapshotCount"), _records.size());
        status.insert(QStringLiteral("snapshotHeadHash"), explicitText(_head));
        status.insert(QStringLiteral("ok"), true);
        status.insert(QStringLiteral("stage"), QStringLiteral("complete"));
        status.insert(QStringLiteral("cancelRequested"), _cancelled.load());
    }
private:
    void fail(const QString& error)
    {
        status.insert(QStringLiteral("error"), explicitText(error));
        status.insert(QStringLiteral("stage"), _cancelled.load() ? QStringLiteral("cancelled") : QStringLiteral("failed"));
        status.insert(QStringLiteral("cancelRequested"), _cancelled.load());
    }
    const QString _id, _action, _path;
    QVariantList _records;
    QString _head;
    std::atomic_bool _cancelled{false};
};

FleetMissionEvidenceChain::FleetMissionEvidenceChain(QObject* parent)
    : QObject(parent)
{
    _fileStatus = fileStatus(QStringLiteral(""), QStringLiteral(""), QStringLiteral(""), 0, QStringLiteral(""), false);
}

FleetMissionEvidenceChain::~FleetMissionEvidenceChain()
{
    _fileDestroying = true;
    if (_fileWorker) _fileWorker->requestCancel();
    // Parentless worker owns only frozen values; actual finished deletes it.
    // Never wait for or destroy a running QThread on the GUI thread.
}

bool FleetMissionEvidenceChain::appendEvidence(const QString& type,
                                               const QVariantMap& payload,
                                               qint64 timestampMs)
{
    const QString normalizedType = type.trimmed();
    int nodeCount = 0;
    qint64 scalarBytes = 0;
    if (normalizedType.isEmpty() || normalizedType.size() > kMaxEvidenceTypeLength
        || _records.size() >= kMaxRecords
        || !payloadWithinLimits(payload, 0, &nodeCount, &scalarBytes)
        || scalarBytes > kMaxEvidencePayloadBytes) {
        return false;
    }

    if (timestampMs <= 0) {
        timestampMs = QDateTime::currentMSecsSinceEpoch();
    }
    if (timestampMs <= 0
        || timestampMs > QDateTime::currentMSecsSinceEpoch() + kFutureEvidenceToleranceMs) {
        return false;
    }
    if (!_records.isEmpty()) {
        const qint64 previousTimestamp = _records.constLast()
            .toMap().value(QStringLiteral("timestampMs")).toLongLong();
        // Evidence is an append-only audit timeline.  A clock rollback or a
        // caller-supplied older timestamp must not make the chain appear to
        // move backwards while still producing valid hashes.
        if (previousTimestamp > 0 && timestampMs < previousTimestamp) {
            return false;
        }
    }

    QVariantMap record;
    record.insert(QStringLiteral("index"), _records.size());
    record.insert(QStringLiteral("timestampMs"), timestampMs);
    record.insert(QStringLiteral("type"), normalizedType);
    record.insert(QStringLiteral("payload"), payload);
    record.insert(QStringLiteral("previousHash"), _headHash);
    record.insert(QStringLiteral("hash"), hashRecord(recordWithoutHash(record)));

    // Keep the append operation atomic if a future change introduces a
    // malformed payload type: the in-memory chain must remain verifiable.
    const QString computedHash = record.value(QStringLiteral("hash")).toString();
    if (!isSha256(computedHash, false)) {
        return false;
    }

    _records.append(record);
    const bool headChanged = _headHash != computedHash;
    _headHash = computedHash;
    emit recordsChanged();
    if (headChanged) {
        emit headHashChanged();
    }
    emit evidenceAppended(record);
    return true;
}

QString FleetMissionEvidenceChain::exportJson() const
{
    QJsonArray records;
    for (const QVariant& value : _records) {
        records.append(canonicalValue(value));
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("merivus.mission-evidence-chain"));
    root.insert(QStringLiteral("version"), kFormatVersion);
    root.insert(QStringLiteral("headHash"), _headHash);
    root.insert(QStringLiteral("records"), records);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool FleetMissionEvidenceChain::importJson(const QString& json)
{
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxEvidenceSnapshotBytes) {
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QSet<QString> allowedRootKeys = QSet<QString>()
        << QStringLiteral("format") << QStringLiteral("version")
        << QStringLiteral("headHash") << QStringLiteral("records");
    for (const QString& key : root.keys()) {
        if (!allowedRootKeys.contains(key)) return false;
    }
    const QJsonValue version = root.value(QStringLiteral("version"));
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("merivus.mission-evidence-chain")
        || !version.isDouble() || !qIsFinite(version.toDouble())
        || qFloor(version.toDouble()) != version.toDouble()
        || version.toInt(-1) != kFormatVersion
        || !root.value(QStringLiteral("records")).isArray()
        || !root.value(QStringLiteral("headHash")).isString()) {
        return false;
    }

    const QJsonArray array = root.value(QStringLiteral("records")).toArray();
    if (array.size() > kMaxRecords) {
        return false;
    }

    QVariantList imported;
    imported.reserve(array.size());
    for (const QJsonValue& value : array) {
        if (!value.isObject()) {
            return false;
        }
        imported.append(value.toObject().toVariantMap());
    }

    const QString importedHead = root.value(QStringLiteral("headHash")).toString();
    if (!verifyRecords(imported, importedHead)) {
        return false;
    }

    // No mutation happens before the complete snapshot has been parsed and
    // verified, so a failed import cannot corrupt an existing chain.
    const bool headChanged = _headHash != importedHead;
    _records = imported;
    _headHash = importedHead;
    emit recordsChanged();
    if (headChanged) {
        emit headHashChanged();
    }
    return true;
}

bool FleetMissionEvidenceChain::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty() || !verifyChain()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxEvidenceSnapshotBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetMissionEvidenceChain::loadFromFile(const QString& filePath)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxEvidenceSnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxEvidenceSnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importJson verifies the complete hash chain before replacing records;
    // failed loads therefore preserve the current evidence chain.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importJson(json);
}

bool FleetMissionEvidenceChain::verifyChain() const
{
    return verifyRecords(_records, _headHash);
}

QVariantMap FleetMissionEvidenceChain::recordAt(int index) const
{
    if (index < 0 || index >= _records.size()) {
        return QVariantMap();
    }
    return _records.at(index).toMap();
}

bool FleetMissionEvidenceChain::fileOperationBusy() const
{
    return QThread::currentThread() == thread() && !_fileDestroying && _fileBusy;
}

QVariantMap FleetMissionEvidenceChain::fileOperationStatus() const
{
    return QThread::currentThread() == thread() && !_fileDestroying ? _fileStatus : QVariantMap();
}

QVariantMap FleetMissionEvidenceChain::historicalSummary() const
{
    if (QThread::currentThread() != thread() || _fileDestroying || !_historical) return {};
    const auto snapshot = _historical;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), snapshot->requestId},
        {QStringLiteral("filePath"), snapshot->path}, {QStringLiteral("recordCount"), snapshot->records.size()},
        {QStringLiteral("headHash"), snapshot->head}, {QStringLiteral("readOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QString FleetMissionEvidenceChain::requestSaveSnapshot(const QString& path, const QVariant& expectedCount,
    const QString& expectedHead, bool overwriteConfirmed)
{
    if (QThread::currentThread() != thread() || _fileDestroying || _fileBusy || _fileEntryBusy)
        return QStringLiteral("");
    int count = -1;
    if (!overwriteConfirmed || !fileIndex(expectedCount, kMaxRecords, &count)
        || count != _records.size() || expectedHead != _headHash
        || !FleetSnapshotFileIo::localJsonPath(path)) return QStringLiteral("");
    // COW values only. A later append/import does not mutate this snapshot or
    // retroactively invalidate a file that has actually been committed.
    return startFileOperation(QStringLiteral("save"), path, _records, _headHash);
}

QString FleetMissionEvidenceChain::requestLoadHistoricalSnapshot(const QString& path)
{
    if (QThread::currentThread() != thread() || _fileDestroying || _fileBusy || _fileEntryBusy
        || !FleetSnapshotFileIo::localJsonPath(path)) return QStringLiteral("");
    return startFileOperation(QStringLiteral("load"), path, {}, QStringLiteral(""));
}

QString FleetMissionEvidenceChain::startFileOperation(const QString& action, const QString& path,
    const QVariantList& records, const QString& head)
{
    const QPointer<FleetMissionEvidenceChain> self(this);
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (id.isEmpty() || id == _fileRequestId) return QStringLiteral("");
    _fileEntryBusy = true;
    const QString frozenPath(path.constData(), path.size());
    const QString frozenHead = explicitText(head);
    const QPointer<FleetMissionEvidenceFileWorker> worker(new FleetMissionEvidenceFileWorker(
        id, action, frozenPath, records, frozenHead));
    connect(worker.data(), &QThread::finished, this, [self, worker, id]() {
        if (self && worker) self->finishFileOperation(worker.data(), id);
    }, Qt::QueuedConnection);
    // Separate from slot accounting. The parent's death cannot delete a
    // running worker, and this connection still reclaims it after finished.
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    _fileWorker = worker;
    _fileRequestId = id;
    _fileBusy = true;
    _fileStatus = fileStatus(id, action, frozenPath, records.size(), frozenHead, true);
    worker->start();
    emit fileOperationChanged();
    if (self) _fileEntryBusy = false;
    return id; // Never substitute a nested or later job's receipt.
}

bool FleetMissionEvidenceChain::cancelFileOperation(const QString& requestId)
{
    if (QThread::currentThread() != thread() || _fileDestroying || !_fileBusy || !_fileWorker
        || requestId.isEmpty() || requestId != _fileRequestId) return false;
    if (_fileStatus.value(QStringLiteral("cancelRequested")).toBool()) return true;
    _fileStatus.insert(QStringLiteral("cancelRequested"), true);
    _fileStatus.insert(QStringLiteral("stage"), QStringLiteral("cancelling"));
    _fileWorker->requestCancel();
    emit fileOperationChanged();
    return true; // Request only; actual finished still owns the slot.
}

void FleetMissionEvidenceChain::finishFileOperation(FleetMissionEvidenceFileWorker* actualWorker,
    const QString& requestId)
{
    if (_fileDestroying || QThread::currentThread() != thread() || !_fileBusy
        || _fileWorker.data() != actualWorker || _fileRequestId != requestId || !actualWorker->isFinished()) return;
    const QPointer<FleetMissionEvidenceChain> self(this);
    const bool previousEntry = _fileEntryBusy;
    _fileEntryBusy = true;
    QVariantMap result = actualWorker->status;
    const bool cancelled = actualWorker->cancelled() || _fileStatus.value(QStringLiteral("cancelRequested")).toBool();
    const bool valid = result.size() == 23 && result.value(QStringLiteral("requestId")) == QVariant(requestId)
        && result.value(QStringLiteral("action")) == _fileStatus.value(QStringLiteral("action"));
    if (!valid) {
        result = _fileStatus;
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("error"), QStringLiteral("evidence-history-worker-result-unavailable"));
        result.insert(QStringLiteral("stage"), QStringLiteral("failed"));
    } else if (result.value(QStringLiteral("action")) == QVariant(QStringLiteral("load"))) {
        if (cancelled) {
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("error"), QStringLiteral("evidence-history-cancelled"));
            result.insert(QStringLiteral("stage"), QStringLiteral("cancelled"));
        } else if (result.value(QStringLiteral("ok")).toBool()) {
            if (actualWorker->snapshot && actualWorker->snapshot->requestId == requestId)
                _historical = actualWorker->snapshot;
            else {
                result.insert(QStringLiteral("ok"), false);
                result.insert(QStringLiteral("error"), QStringLiteral("evidence-history-worker-snapshot-unavailable"));
                result.insert(QStringLiteral("stage"), QStringLiteral("failed"));
            }
        }
    }
    result.insert(QStringLiteral("cancelRequested"), cancelled);
    result.insert(QStringLiteral("busy"), false);
    _fileStatus = result;
    _fileWorker.clear();
    _fileBusy = false;
    emit fileOperationChanged();
    if (self) _fileEntryBusy = previousEntry;
}

QVariantMap FleetMissionEvidenceChain::historyPage(const QString& expectedRequestId, const QVariant& offset) const
{
    if (QThread::currentThread() != thread() || _fileDestroying || !_historical
        || expectedRequestId != _historical->requestId) return {};
    const auto snapshot = _historical;
    int first = 0;
    if (!fileIndex(offset, snapshot->records.size(), &first)) return {};
    const int next = qMin(snapshot->records.size(), first + 4);
    QVariantList rows;
    for (int index = first; index < next; ++index) {
        const QVariantMap record = snapshot->records.at(index).toMap();
        rows.append(QVariantMap{{QStringLiteral("index"), index}, {QStringLiteral("timestampMs"), record.value(QStringLiteral("timestampMs"))},
            {QStringLiteral("type"), record.value(QStringLiteral("type"))}, {QStringLiteral("hash"), record.value(QStringLiteral("hash"))},
            {QStringLiteral("previousHash"), explicitText(record.value(QStringLiteral("previousHash")).toString())}});
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("requestId"), snapshot->requestId}, {QStringLiteral("offset"), first},
        {QStringLiteral("nextOffset"), next}, {QStringLiteral("totalCount"), snapshot->records.size()},
        {QStringLiteral("hasMore"), next < snapshot->records.size()}, {QStringLiteral("records"), rows}};
}

QVariantMap FleetMissionEvidenceChain::historicalRecord(const QString& expectedRequestId, const QVariant& requestedIndex) const
{
    if (QThread::currentThread() != thread() || _fileDestroying || !_historical
        || expectedRequestId != _historical->requestId) return {};
    const auto snapshot = _historical;
    int index = 0;
    if (snapshot->records.isEmpty() || !fileIndex(requestedIndex, snapshot->records.size() - 1, &index)) return {};
    const QVariantMap record = snapshot->records.at(index).toMap();
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("requestId"), snapshot->requestId}, {QStringLiteral("index"), index},
        {QStringLiteral("timestampMs"), record.value(QStringLiteral("timestampMs"))},
        {QStringLiteral("type"), record.value(QStringLiteral("type"))}, {QStringLiteral("hash"), record.value(QStringLiteral("hash"))},
        {QStringLiteral("previousHash"), explicitText(record.value(QStringLiteral("previousHash")).toString())},
        {QStringLiteral("payloadPreview"), snapshot->previews.at(index)}, {QStringLiteral("truncated"), snapshot->truncated.at(index)}};
}

QString FleetMissionEvidenceChain::hashRecord(const QVariantMap& record)
{
    return QString::fromLatin1(QCryptographicHash::hash(canonicalJson(record), QCryptographicHash::Sha256).toHex());
}

QByteArray FleetMissionEvidenceChain::canonicalJson(const QVariantMap& value)
{
    return QJsonDocument(canonicalValue(value).toObject()).toJson(QJsonDocument::Compact);
}

bool FleetMissionEvidenceChain::verifyRecords(const QVariantList& records,
                                              const QString& expectedHeadHash,
                                              QString* error, const std::atomic_bool* cancelled)
{
    if (records.size() > kMaxRecords || !isSha256(expectedHeadHash, true)) {
        validationError(QStringLiteral("invalid chain size or head hash"), error);
        return false;
    }

    QString previousHash;
    int expectedIndex = 0;
    qint64 previousTimestamp = 0;
    const QSet<QString> allowedKeys = QSet<QString>()
        << QStringLiteral("index")
        << QStringLiteral("timestampMs")
        << QStringLiteral("type")
        << QStringLiteral("payload")
        << QStringLiteral("previousHash")
        << QStringLiteral("hash");

    for (const QVariant& value : records) {
        if (cancelled && cancelled->load()) return false;
        if (!value.canConvert<QVariantMap>()) {
            validationError(QStringLiteral("record is not an object"), error);
            return false;
        }
        const QVariantMap record = value.toMap();
        const QStringList keys = record.keys();
        for (const QString& key : keys) {
            if (!allowedKeys.contains(key)) {
                validationError(QStringLiteral("record contains an unknown field"), error);
                return false;
            }
        }

        if (!record.contains(QStringLiteral("index"))
            || !record.contains(QStringLiteral("timestampMs"))
            || !record.contains(QStringLiteral("type"))
            || !record.contains(QStringLiteral("payload"))
            || !record.contains(QStringLiteral("previousHash"))
            || !record.contains(QStringLiteral("hash"))) {
            validationError(QStringLiteral("record is missing a required field"), error);
            return false;
        }
        if (!isIntegerJsonNumber(record.value(QStringLiteral("index")))
            // expectedIndex is at most 100000 and exactly representable.
            // Compare before narrowing: a huge integer must not wrap or
            // fail conversion into an otherwise valid local sequence.
            || record.value(QStringLiteral("index")).toDouble() != double(expectedIndex)
            || !isIntegerJsonNumber(record.value(QStringLiteral("timestampMs")))
            || record.value(QStringLiteral("timestampMs")).toLongLong() <= 0
            || record.value(QStringLiteral("timestampMs")).toLongLong()
                > QDateTime::currentMSecsSinceEpoch() + kFutureEvidenceToleranceMs) {
            validationError(QStringLiteral("record index or timestamp is invalid"), error);
            return false;
        }
        const qint64 timestampMs = record.value(QStringLiteral("timestampMs")).toLongLong();
        if (previousTimestamp > 0 && timestampMs < previousTimestamp) {
            validationError(QStringLiteral("record timestamps are not monotonic"), error);
            return false;
        }

        const QString type = record.value(QStringLiteral("type")).toString().trimmed();
        const QString recordPreviousHash = record.value(QStringLiteral("previousHash")).toString();
        const QString recordHash = record.value(QStringLiteral("hash")).toString();
        int payloadNodeCount = 0;
        qint64 payloadScalarBytes = 0;
        if (type.isEmpty() || record.value(QStringLiteral("payload")).type() != QVariant::Map
            || type.size() > kMaxEvidenceTypeLength
            || !payloadWithinLimits(record.value(QStringLiteral("payload")), 0,
                                    &payloadNodeCount, &payloadScalarBytes)
            || payloadScalarBytes > kMaxEvidencePayloadBytes
            || !isSha256(recordPreviousHash, true) || !isSha256(recordHash, false)
            || recordPreviousHash != previousHash
            || hashRecord(recordWithoutHash(record)) != recordHash) {
            validationError(QStringLiteral("record link or hash is invalid"), error);
            return false;
        }

        previousHash = recordHash;
        previousTimestamp = timestampMs;
        ++expectedIndex;
    }

    if (previousHash != expectedHeadHash) {
        validationError(QStringLiteral("head hash does not match the final record"), error);
        return false;
    }
    return true;
}

bool FleetMissionEvidenceChain::isSha256(const QString& value, bool allowEmpty)
{
    if (allowEmpty && value.isEmpty()) {
        return true;
    }
    if (value.size() != 64) {
        return false;
    }
    for (const QChar character : value) {
        const bool hex = (character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f'))
            || (character >= QLatin1Char('A') && character <= QLatin1Char('F'));
        if (!hex) {
            return false;
        }
    }
    return true;
}

bool FleetMissionEvidenceChain::isIntegerJsonNumber(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return false;
    }
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
        return true;
    }
    if (value.type() == QVariant::Double) {
        const double number = value.toDouble();
        return number >= 0.0 && number <= 9007199254740992.0
            && number == static_cast<double>(static_cast<qint64>(number));
    }
    return false;
}

bool FleetMissionEvidenceChain::payloadWithinLimits(const QVariant& value,
                                                    int depth,
                                                    int* nodeCount,
                                                    qint64* scalarBytes)
{
    if (depth > kMaxEvidencePayloadDepth || !nodeCount || !scalarBytes) {
        return false;
    }
    if (++(*nodeCount) > kMaxEvidencePayloadNodes) {
        return false;
    }

    if (!value.isValid() || value.isNull()) {
        return true;
    }

    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            *scalarBytes += static_cast<qint64>(it.key().size()) * 2;
            if (*scalarBytes > kMaxEvidencePayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    case QVariant::Hash: {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            *scalarBytes += static_cast<qint64>(it.key().size()) * 2;
            if (*scalarBytes > kMaxEvidencePayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    case QVariant::List: {
        for (const QVariant& item : value.toList()) {
            if (!payloadWithinLimits(item, depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    case QVariant::String:
        *scalarBytes += static_cast<qint64>(value.toString().size()) * 2;
        return *scalarBytes <= kMaxEvidencePayloadBytes;
    case QVariant::StringList:
        for (const QString& item : value.toStringList()) {
            *scalarBytes += static_cast<qint64>(item.size()) * 2;
            if (*scalarBytes > kMaxEvidencePayloadBytes) return false;
        }
        return true;
    case QVariant::ByteArray:
        *scalarBytes += value.toByteArray().size();
        return *scalarBytes <= kMaxEvidencePayloadBytes;
    default:
        // Scalar numeric/bool/date values are bounded by QVariant itself and
        // are still represented deterministically by canonicalValue().
        return true;
    }
}

QVariantMap FleetMissionEvidenceChain::recordWithoutHash(const QVariantMap& record)
{
    QVariantMap value = record;
    value.remove(QStringLiteral("hash"));
    return value;
}

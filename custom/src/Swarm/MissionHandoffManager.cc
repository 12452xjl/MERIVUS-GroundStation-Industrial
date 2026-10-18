#include "MissionHandoffManager.h"
#include "FleetMissionExecutionReport.h"
#include "FleetSnapshotFileIo.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPointer>
#include <QSaveFile>
#include <QThread>
#include <QVariantHash>
#include <QUuid>
#include <QCryptographicHash>

#include <cmath>
#include <utility>

namespace {

constexpr qint64 kMaxHandoffSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr int kMaxHandoffRecords = 100000;
constexpr int kMaxRunHandoffRecords = 256;
constexpr int kHandoffPageSize = 4;
constexpr quint64 kMaximumSafeInteger = 9007199254740991ULL;
constexpr qint64 kFutureHandoffTimestampToleranceMs = 5LL * 60LL * 1000LL;
// A handoff is a short-lived informational acknowledgement. Keep the backend
// bounded even when a non-QML caller bypasses the fixed 300 s facade policy.
constexpr int kMaxHandoffExpirySeconds = 24 * 60 * 60;
constexpr int kMaxTaskIdLength = 256;
constexpr int kMaxOperatorIdLength = 128;
constexpr int kMaxReasonLength = 1024;
constexpr int kMaxNoteLength = 1024;
constexpr int kMaxMetadataDepth = 8;
constexpr int kMaxMetadataNodes = 2048;
constexpr qint64 kMaxMetadataScalarBytes = 128LL * 1024LL;

QString normalize(const QString& value)
{
    return value.trimmed();
}

bool boundedText(const QString& value, int maximumLength, bool allowEmpty = true)
{
    const QString normalized = value.trimmed();
    return (allowEmpty || !normalized.isEmpty()) && normalized.size() <= maximumLength;
}

bool runMetadata(const QVariantMap& metadata)
{
    return metadata.value(QStringLiteral("kind")).type() == QVariant::String
        && metadata.value(QStringLiteral("kind")).toString() == QStringLiteral("run-brief");
}

bool validRunMetadata(const QVariantMap& metadata, const QString& taskId)
{
    if (!runMetadata(metadata)) return true;
    if (metadata.size() != 2 || metadata.value(QStringLiteral("brief")).type() != QVariant::Map)
        return false;
    const QVariantMap brief = metadata.value(QStringLiteral("brief")).toMap();
    return FleetMissionExecutionReport::handoffBriefValid(brief)
        && brief.value(QStringLiteral("taskId")).type() == QVariant::String
        && brief.value(QStringLiteral("taskId")).toString() == taskId;
}

bool pageIndex(const QVariant& value, quint64* result)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (value.isNull() || !std::isfinite(number) || number < 0
        || number > double(kMaximumSafeInteger) || std::floor(number) != number) return false;
    *result = quint64(number);
    return true;
}

QVariantMap pageRejection(const QString& error)
{
    return {{QStringLiteral("page"), 0}, {QStringLiteral("pageCount"), 0},
        {QStringLiteral("total"), 0}, {QStringLiteral("records"), QVariantList()},
        {QStringLiteral("error"), error}};
}

bool metadataWithinLimits(const QVariant& value,
                          int depth,
                          int* nodeCount,
                          qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxMetadataDepth) return false;
    if (!value.isValid() || value.isNull()) return true;
    if (++(*nodeCount) > kMaxMetadataNodes) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes
                || !metadataWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes
                || !metadataWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!metadataWithinLimits(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            if (++(*nodeCount) > kMaxMetadataNodes) return false;
            *scalarBytes += item.toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        *scalarBytes += value.toString().toUtf8().size();
    } else if (value.type() == QVariant::ByteArray) {
        *scalarBytes += value.toByteArray().size();
    } else if (value.type() == QVariant::Double) {
        if (!std::isfinite(value.toDouble())) return false;
    } else if (value.type() != QVariant::Bool
               && value.type() != QVariant::Int
               && value.type() != QVariant::UInt
               && value.type() != QVariant::LongLong
               && value.type() != QVariant::ULongLong) {
        return false;
    }
    return *scalarBytes <= kMaxMetadataScalarBytes;
}

bool boundedHandoffMetadata(const QVariantMap& metadata)
{
    const QVariant briefValue = metadata.value(QStringLiteral("brief"));
    quint64 version = 0;
    if (runMetadata(metadata) && briefValue.type() == QVariant::Map
        && pageIndex(briefValue.toMap().value(QStringLiteral("schemaVersion")), &version) && version == 2) {
        // The closed v2 brief validator bounds its own tree (8/4096/96 KiB).
        // This exact two-key wrapper adds one level and two nodes, not an
        // enlarged acceptance domain for arbitrary legacy/generic metadata.
        if (metadata.size() != 2 || !FleetMissionExecutionReport::handoffBriefValid(briefValue.toMap())) return false;
        return QJsonDocument::fromVariant(metadata).toJson(QJsonDocument::Compact).size() <= kMaxMetadataScalarBytes;
    }
    int nodes = 0;
    qint64 bytes = 0;
    return metadataWithinLimits(metadata, 0, &nodes, &bytes);
}

bool jsonUnsignedInteger(const QJsonValue& value, quint64* result)
{
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0.0
        || std::floor(number) != number
        // JSON numbers are IEEE-754 doubles.  Keep revisions within the
        // exactly representable integer range instead of accepting a rounded
        // value that could change after a save/load round trip.
        || number > 9007199254740991.0) {
        return false;
    }
    if (result) {
        *result = static_cast<quint64>(number);
    }
    return true;
}

bool optionalJsonString(const QJsonObject& object, const QString& key, QString* result)
{
    const QJsonValue value = object.value(key);
    if (value.isUndefined() || value.isNull()) {
        if (result) {
            result->clear();
        }
        return true;
    }
    if (!value.isString()) {
        return false;
    }
    if (result) {
        *result = value.toString().trimmed();
    }
    return true;
}

}

MissionHandoffManager::MissionHandoffManager(QObject* parent)
    : QObject(parent)
    , _collectionGeneration(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _expiryTimer.setInterval(1000);
    connect(&_expiryTimer, &QTimer::timeout, this, &MissionHandoffManager::_expireDueHandoffs);
    _expiryTimer.start();
}

void MissionHandoffManager::_expireDueHandoffs()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QStringList ids = _order;
    const QPointer<MissionHandoffManager> self(this);
    for (const QString& id : ids) {
        const Record* record = find(id);
        if (record && !record->historicalOnly && record->status == Status::Pending && record->expiresAt.isValid()
                && record->expiresAt <= now) {
            expireHandoff(id, QStringLiteral("handoff request expired"));
            if (!self) return;
        }
    }
}

QString MissionHandoffManager::statusToString(Status status)
{
    switch (status) {
    case Status::Pending: return QStringLiteral("pending");
    case Status::Accepted: return QStringLiteral("accepted");
    case Status::Rejected: return QStringLiteral("rejected");
    case Status::Cancelled: return QStringLiteral("cancelled");
    case Status::Expired: return QStringLiteral("expired");
    }
    return QStringLiteral("pending");
}

MissionHandoffManager::Status MissionHandoffManager::statusFromString(const QString& status,
                                                                        bool* ok)
{
    const QString normalized = status.trimmed().toLower();
    if (ok) {
        *ok = true;
    }
    if (normalized == QStringLiteral("pending")) return Status::Pending;
    if (normalized == QStringLiteral("accepted")) return Status::Accepted;
    if (normalized == QStringLiteral("rejected")) return Status::Rejected;
    if (normalized == QStringLiteral("cancelled")) return Status::Cancelled;
    if (normalized == QStringLiteral("expired")) return Status::Expired;
    if (ok) {
        *ok = false;
    }
    return Status::Pending;
}

QString MissionHandoffManager::timestamp(const QDateTime& value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("");
}

QVariantMap MissionHandoffManager::toSummary(const Record& record) const
{
    QVariantMap result;
    result[QStringLiteral("handoffId")] = record.id;
    result[QStringLiteral("taskId")] = record.taskId;
    result[QStringLiteral("fromOperator")] = record.fromOperator;
    result[QStringLiteral("toOperator")] = record.toOperator;
    result[QStringLiteral("reason")] = record.reason;
    result[QStringLiteral("note")] = record.note;
    result[QStringLiteral("metadata")] = record.metadata;
    result[QStringLiteral("status")] = statusToString(record.status);
    result[QStringLiteral("requiresSecondApproval")] = record.requiresSecondApproval;
    result[QStringLiteral("recordRevision")] = static_cast<qint64>(record.revision);
    result[QStringLiteral("createdAt")] = timestamp(record.createdAt);
    result[QStringLiteral("expiresAt")] = timestamp(record.expiresAt);
    result[QStringLiteral("resolvedAt")] = timestamp(record.resolvedAt);
    result[QStringLiteral("resolvedBy")] = record.resolvedBy;
    result[QStringLiteral("origin")] = record.imported ? QStringLiteral("imported") : QStringLiteral("local");
    // This explicit field prevents a UI handoff acknowledgement from being
    // misinterpreted as a MAVLink command acknowledgement.
    result[QStringLiteral("flightCommandReleased")] = false;
    return result;
}

MissionHandoffManager::Record* MissionHandoffManager::find(const QString& handoffId)
{
    const auto it = _records.find(handoffId.trimmed());
    return it == _records.end() ? nullptr : &it.value();
}

const MissionHandoffManager::Record* MissionHandoffManager::find(const QString& handoffId) const
{
    const auto it = _records.constFind(handoffId.trimmed());
    return it == _records.constEnd() ? nullptr : &it.value();
}

int MissionHandoffManager::pendingCount() const
{
    int count = 0;
    for (const Record& record : _records) {
        if (!record.historicalOnly && record.status == Status::Pending) {
            ++count;
        }
    }
    return count;
}

QVariantList MissionHandoffManager::handoffs() const
{
    QVariantList result;
    for (const QString& id : _order) {
        const Record* record = find(id);
        if (record) {
            result.append(toSummary(*record));
        }
    }
    return result;
}

QString MissionHandoffManager::requestHandoff(const QString& taskId,
                                              const QString& fromOperator,
                                              const QString& toOperator,
                                              const QString& reason,
                                              const QVariantMap& metadata,
                                              bool requiresSecondApproval,
                                              int expirySeconds)
{
    if (thread() != QThread::currentThread()) return QString();
    const QString task = normalize(taskId);
    const QString from = normalize(fromOperator);
    const QString to = normalize(toOperator);
    // Every handoff must require a distinct receiving operator acknowledgement.
    // The facade already forces this value, but enforcing it here prevents a
    // future/raw QObject caller from creating a one-sided approval record.
    if (!boundedText(task, kMaxTaskIdLength, false)
            || !boundedText(from, kMaxOperatorIdLength, false)
            || !boundedText(to, kMaxOperatorIdLength, false)
            || !boundedText(reason, kMaxReasonLength)
            || !boundedHandoffMetadata(metadata)
            || !validRunMetadata(metadata, task)
            || task.isEmpty() || from.isEmpty() || to.isEmpty() || from == to
            || !requiresSecondApproval || expirySeconds <= 0
            || expirySeconds > kMaxHandoffExpirySeconds
            || (runMetadata(metadata) && expirySeconds != 300)) {
        return QString();
    }
    if (_records.size() >= kMaxHandoffRecords || _sequence >= kMaximumSafeInteger
        || _revision >= kMaximumSafeInteger) {
        return QString();
    }
    if (runMetadata(metadata)) {
        int count = 0;
        for (const Record& existing : _records) if (runMetadata(existing.metadata)) ++count;
        if (count >= kMaxRunHandoffRecords) return QString();
    }

    Record record;
    record.createdAt = QDateTime::currentDateTimeUtc();
    record.id = QStringLiteral("handoff-%1-%2")
            .arg(record.createdAt.toString(QStringLiteral("yyyyMMddhhmmsszzz")))
            .arg(_sequence + 1);
    // Imported identifiers need not have been generated by this instance.
    // A collision must not overwrite an immutable record or its briefing.
    if (_records.contains(record.id)) return QString();
    record.taskId = task;
    record.fromOperator = from;
    record.toOperator = to;
    record.reason = normalize(reason);
    record.metadata = metadata;
    record.status = Status::Pending;
    record.requiresSecondApproval = true;
    record.revision = ++_revision;
    ++_sequence;
    record.expiresAt = record.createdAt.addSecs(qMax(1, expirySeconds));
    _records.insert(record.id, record);
    _order.append(record.id);
    _collectionGeneration = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QPointer<MissionHandoffManager> self(this);
    emit handoffRequested(record.id, toSummary(record));
    if (self) emit handoffCollectionChanged();
    // This is the committed request identity even if a direct signal handler
    // resolves it, imports another collection or destroys this manager.
    return record.id;
}

QString MissionHandoffManager::requestRunHandoff(const QVariantMap& brief,
                                                const QString& fromOperator,
                                                const QString& toOperator,
                                                const QString& reason)
{
    if (thread() != QThread::currentThread() || !FleetMissionExecutionReport::handoffBriefValid(brief))
        return QString();
    const QVariantMap metadata{{QStringLiteral("kind"), QStringLiteral("run-brief")},
        {QStringLiteral("brief"), brief}};
    return requestHandoff(brief.value(QStringLiteral("taskId")).toString(),
        fromOperator, toOperator, reason, metadata, true, 300);
}

bool MissionHandoffManager::resolve(const QString& handoffId, Status status,
                                    const QString& operatorId, const QString& note,
                                    quint64 expectedRevision)
{
    if (thread() != QThread::currentThread()) return false;
    Record* record = find(handoffId);
    if (!record || record->historicalOnly || record->status != Status::Pending) {
        return false;
    }
    if (status == Status::Pending) {
        return false;
    }
    // Interactive accept/reject/cancel calls must carry the record revision.
    // Only the internal timer/expiry path is allowed to resolve without a
    // compare-and-swap token.
    if (status != Status::Expired && expectedRevision == 0) {
        return false;
    }
    if (expectedRevision != 0 && record->revision != expectedRevision) {
        return false;
    }
    if (_revision >= kMaximumSafeInteger || expectedRevision > kMaximumSafeInteger) {
        return false;
    }
    // Imported Run briefs are historical material, not an acknowledgement
    // request created from this process's selected native Run. They may age
    // into expired history but cannot be accepted/rejected/cancelled as live.
    if (record->imported && runMetadata(record->metadata) && status != Status::Expired) return false;
    // The 1 s housekeeping timer is not the authority for expiry.  A user
    // action can arrive between timer ticks (or immediately after an import),
    // so resolve the request as expired before accepting/rejecting/cancelling
    // it when its deadline has already passed.
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (status != Status::Expired && record->expiresAt.isValid()
            && record->expiresAt <= now) {
        record->status = Status::Expired;
        record->resolvedBy.clear();
        record->note = QStringLiteral("handoff request expired");
        record->resolvedAt = now;
        record->revision = ++_revision;
        _collectionGeneration = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString committedId = record->id;
        const QVariantMap summary = toSummary(*record);
        const QPointer<MissionHandoffManager> self(this);
        emit handoffExpired(committedId, summary);
        if (self) emit handoffCollectionChanged();
        return false;
    }
    const QString normalizedOperator = normalize(operatorId);
    const QString normalizedNote = normalize(note);
    if ((status != Status::Expired
             && (!boundedText(normalizedOperator, kMaxOperatorIdLength, false)
                 || normalizedOperator.isEmpty()))
            || !boundedText(normalizedNote, kMaxNoteLength)) {
        return false;
    }
    record->status = status;
    record->resolvedBy = normalizedOperator;
    record->note = normalizedNote;
    record->resolvedAt = now;
    record->revision = ++_revision;
    _collectionGeneration = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString committedId = record->id;
    const QVariantMap summary = toSummary(*record);
    const QPointer<MissionHandoffManager> self(this);
    switch (status) {
    case Status::Accepted: emit handoffAccepted(committedId, summary); break;
    case Status::Rejected: emit handoffRejected(committedId, summary); break;
    case Status::Cancelled: emit handoffCancelled(committedId, summary); break;
    case Status::Expired: emit handoffExpired(committedId, summary); break;
    case Status::Pending: break;
    }
    if (self) emit handoffCollectionChanged();
    return true;
}

bool MissionHandoffManager::acceptHandoff(const QString& handoffId,
                                          const QString& acceptingOperator,
                                          const QString& note,
                                          quint64 expectedRevision)
{
    Record* record = find(handoffId);
    if (!record || normalize(acceptingOperator) != record->toOperator) {
        return false;
    }
    return resolve(handoffId, Status::Accepted, acceptingOperator, note, expectedRevision);
}

bool MissionHandoffManager::rejectHandoff(const QString& handoffId,
                                          const QString& rejectingOperator,
                                          const QString& note,
                                          quint64 expectedRevision)
{
    Record* record = find(handoffId);
    if (!record || normalize(rejectingOperator) != record->toOperator) {
        return false;
    }
    return resolve(handoffId, Status::Rejected, rejectingOperator, note, expectedRevision);
}

bool MissionHandoffManager::cancelHandoff(const QString& handoffId,
                                          const QString& cancellingOperator,
                                          const QString& note,
                                          quint64 expectedRevision)
{
    Record* record = find(handoffId);
    if (!record || normalize(cancellingOperator) != record->fromOperator) {
        return false;
    }
    return resolve(handoffId, Status::Cancelled, cancellingOperator, note, expectedRevision);
}

bool MissionHandoffManager::expireHandoff(const QString& handoffId, const QString& note)
{
    Record* record = find(handoffId);
    if (!record || record->status != Status::Pending) {
        return false;
    }
    return resolve(handoffId, Status::Expired, QString(), note);
}

QVariantMap MissionHandoffManager::handoff(const QString& handoffId) const
{
    const Record* record = find(handoffId);
    return record ? toSummary(*record) : QVariantMap();
}

QVariantMap MissionHandoffManager::handoffPage(const QString& actorId,
                                             const QString& filter,
                                             const QVariant& page) const
{
    if (thread() != QThread::currentThread()) return pageRejection(QStringLiteral("wrong-thread"));
    const bool all = filter == QStringLiteral("all");
    if (!all && filter != QStringLiteral("inbox") && filter != QStringLiteral("outbox")
        && filter != QStringLiteral("history")) return pageRejection(QStringLiteral("invalid-filter"));
    const QString actor = normalize(actorId);
    // Local audit browsing does not need a flight-account session. Filtered
    // operator inboxes do need an explicit identity; this is not its proof.
    if (!boundedText(actor, kMaxOperatorIdLength, all)) return pageRejection(QStringLiteral("invalid-actor"));
    quint64 requestedPage = 0;
    if (!pageIndex(page, &requestedPage)) return pageRejection(QStringLiteral("invalid-page"));
    const auto included = [&](const Record& record) {
        if (all) return true;
        if (filter == QStringLiteral("inbox")) return !record.historicalOnly && record.status == Status::Pending && record.toOperator == actor;
        if (filter == QStringLiteral("outbox")) return !record.historicalOnly && record.status == Status::Pending && record.fromOperator == actor;
        return (record.historicalOnly || record.status != Status::Pending) && (record.fromOperator == actor || record.toOperator == actor);
    };
    int total = 0;
    for (auto it = _order.crbegin(); it != _order.crend(); ++it) {
        const Record* record = find(*it);
        if (record && included(*record)) ++total;
    }
    const int pageCount = (total + kHandoffPageSize - 1) / kHandoffPageSize;
    const int selectedPage = pageCount == 0 ? 0 : int(qMin(requestedPage, quint64(pageCount - 1)));
    int skipped = 0;
    QVariantList records;
    for (auto it = _order.crbegin(); it != _order.crend() && records.size() < kHandoffPageSize; ++it) {
        const Record* record = find(*it);
        if (!record || !included(*record)) continue;
        if (skipped++ < selectedPage * kHandoffPageSize) continue;
        // Validated and immutable at creation/import. Only these three brief
        // identity strings are projected; never copy the complete metadata.
        const bool available = runMetadata(record->metadata);
        const QVariantMap brief = available ? record->metadata.value(QStringLiteral("brief")).toMap() : QVariantMap();
        records.append(QVariantMap{{QStringLiteral("handoffId"), record->id}, {QStringLiteral("taskId"), record->taskId},
            {QStringLiteral("fromOperator"), record->fromOperator}, {QStringLiteral("toOperator"), record->toOperator},
            {QStringLiteral("status"), statusToString(record->status)}, {QStringLiteral("recordRevision"), qint64(record->revision)},
            {QStringLiteral("createdAt"), timestamp(record->createdAt)}, {QStringLiteral("expiresAt"), timestamp(record->expiresAt)},
            {QStringLiteral("resolvedAt"), timestamp(record->resolvedAt)}, {QStringLiteral("resolvedBy"), record->resolvedBy},
            {QStringLiteral("briefAvailable"), available},
            {QStringLiteral("briefHash"), available ? brief.value(QStringLiteral("contentHash")).toString() : QStringLiteral("")},
            {QStringLiteral("runId"), available ? brief.value(QStringLiteral("runId")).toString() : QStringLiteral("")},
            {QStringLiteral("origin"), record->imported ? QStringLiteral("imported") : QStringLiteral("local")}});
    }
    return {{QStringLiteral("page"), selectedPage}, {QStringLiteral("pageCount"), pageCount},
        {QStringLiteral("total"), total}, {QStringLiteral("records"), records}};
}

bool MissionHandoffManager::hasHandoff(const QString& handoffId) const
{
    return find(handoffId) != nullptr;
}

bool MissionHandoffManager::isPending(const QString& handoffId) const
{
    const Record* record = find(handoffId);
    return record && !record->historicalOnly && record->status == Status::Pending;
}

QJsonObject MissionHandoffManager::persistentRecord(const Record& record)
{
    return {{QStringLiteral("handoffId"), record.id}, {QStringLiteral("taskId"), record.taskId},
        {QStringLiteral("fromOperator"), record.fromOperator}, {QStringLiteral("toOperator"), record.toOperator},
        {QStringLiteral("reason"), record.reason}, {QStringLiteral("note"), record.note},
        {QStringLiteral("metadata"), QJsonObject::fromVariantMap(record.metadata)},
        {QStringLiteral("status"), statusToString(record.status)},
        {QStringLiteral("requiresSecondApproval"), record.requiresSecondApproval},
        {QStringLiteral("recordRevision"), qint64(record.revision)},
        {QStringLiteral("createdAt"), timestamp(record.createdAt)},
        {QStringLiteral("expiresAt"), timestamp(record.expiresAt)},
        {QStringLiteral("resolvedAt"), timestamp(record.resolvedAt)}, {QStringLiteral("resolvedBy"), record.resolvedBy}};
}

bool MissionHandoffManager::encodeSnapshot(const Snapshot& snapshot, QByteArray& bytes,
    const std::atomic_bool* cancelled)
{
    bytes.clear();
    if (snapshot.records.size() > kMaxHandoffRecords || snapshot.order.size() != snapshot.records.size()
        || snapshot.sequence > kMaximumSafeInteger || snapshot.revision > kMaximumSafeInteger) return false;
    QJsonArray serialized;
    qint64 estimatedBytes = 128;
    int runCount = 0;
    for (const QString& id : snapshot.order) {
        if (cancelled && cancelled->load()) return false;
        const auto found = snapshot.records.constFind(id);
        if (found == snapshot.records.constEnd()) return false;
        if (!boundedHandoffMetadata(found->metadata)
            || !validRunMetadata(found->metadata, found->taskId)
            || (runMetadata(found->metadata) && ++runCount > kMaxRunHandoffRecords)) return false;
        const QJsonObject item = persistentRecord(found.value());
        estimatedBytes += QJsonDocument(item).toJson(QJsonDocument::Compact).size() + 1;
        if (estimatedBytes > kMaxHandoffSnapshotBytes) return false;
        serialized.append(item);
    }

    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("merivus.mission-handoff");
    root[QStringLiteral("schemaVersion")] = kSchemaVersion;
    root[QStringLiteral("sequence")] = static_cast<qint64>(snapshot.sequence);
    root[QStringLiteral("revision")] = static_cast<qint64>(snapshot.revision);
    root[QStringLiteral("handoffs")] = serialized;
    if (cancelled && cancelled->load()) return false;
    const QByteArray encoded = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (encoded.size() > kMaxHandoffSnapshotBytes || (cancelled && cancelled->load())) return false;
    bytes = encoded;
    return true;
}

QString MissionHandoffManager::exportJson() const
{
    if (thread() != QThread::currentThread()) return QString();
    Snapshot snapshot;
    snapshot.records = _records; snapshot.order = _order;
    snapshot.sequence = _sequence; snapshot.revision = _revision;
    QByteArray bytes;
    return encodeSnapshot(snapshot, bytes) ? QString::fromUtf8(bytes) : QString();
}

bool MissionHandoffManager::decodeSnapshot(const QByteArray& encoded, Snapshot& snapshot,
    const std::atomic_bool* cancelled)
{
    snapshot = Snapshot();
    if (encoded.isEmpty() || encoded.size() > kMaxHandoffSnapshotBytes) {
        return false;
    }
    // Bound nesting before Qt constructs the document; all admitted metadata
    // is bounded below: legacy/generic depth eight, exact v2 run-brief
    // wrapper depth nine. The file's surrounding containers still fit 16.
    bool quoted = false, escaped = false;
    int depth = 0;
    for (int i = 0; i < encoded.size(); ++i) {
        if ((i & 65535) == 0 && cancelled && cancelled->load()) return false;
        const char c = encoded.at(i);
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') { if (++depth > 16) return false; }
        else if (c == '}' || c == ']') { if (--depth < 0) return false; }
    }
    if (quoted || depth != 0 || QString::fromUtf8(encoded).toUtf8() != encoded) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();
    const QStringList allowedRootKeys = {
        QStringLiteral("format"), QStringLiteral("schemaVersion"),
        QStringLiteral("sequence"), QStringLiteral("revision"),
        QStringLiteral("handoffs")
    };
    for (const QString& key : root.keys()) {
        if (!allowedRootKeys.contains(key)) {
            return false;
        }
    }
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("merivus.mission-handoff")
            || !schemaVersion.isDouble() || schemaVersion.toDouble() != kSchemaVersion
            || !root.contains(QStringLiteral("sequence"))
            || !root.contains(QStringLiteral("revision"))
            || !root.value(QStringLiteral("handoffs")).isArray()) {
        return false;
    }

    quint64 importedSequence = 0;
    const QJsonValue sequenceValue = root.value(QStringLiteral("sequence"));
    if (!sequenceValue.isUndefined() && !jsonUnsignedInteger(sequenceValue, &importedSequence)) {
        return false;
    }
    quint64 importedRevision = 0;
    const QJsonValue revisionValue = root.value(QStringLiteral("revision"));
    if (!revisionValue.isUndefined() && !jsonUnsignedInteger(revisionValue, &importedRevision)) {
        return false;
    }

    QHash<QString, Record> stagedRecords;
    QStringList stagedOrder;
    quint64 maxSequence = importedSequence;
    quint64 maxRevision = importedRevision;
    int runRecordCount = 0;
    const QJsonArray items = root.value(QStringLiteral("handoffs")).toArray();
    if (items.size() > kMaxHandoffRecords) {
        return false;
    }
    for (const QJsonValue& value : items) {
        if (cancelled && cancelled->load()) return false;
        if (!value.isObject()) {
            return false;
        }
        const QJsonObject item = value.toObject();
        const QStringList allowedItemKeys = {
            QStringLiteral("handoffId"), QStringLiteral("taskId"),
            QStringLiteral("fromOperator"), QStringLiteral("toOperator"),
            QStringLiteral("reason"), QStringLiteral("note"),
            QStringLiteral("metadata"), QStringLiteral("status"),
            QStringLiteral("requiresSecondApproval"),
            QStringLiteral("recordRevision"), QStringLiteral("createdAt"),
            QStringLiteral("expiresAt"), QStringLiteral("resolvedAt"),
            QStringLiteral("resolvedBy")
        };
        for (const QString& key : item.keys()) {
            if (!allowedItemKeys.contains(key)) {
                return false;
            }
        }
        if (!item.value(QStringLiteral("handoffId")).isString()
            || !item.value(QStringLiteral("taskId")).isString()
            || !item.value(QStringLiteral("fromOperator")).isString()
            || !item.value(QStringLiteral("toOperator")).isString()) {
            return false;
        }
        const QString id = item.value(QStringLiteral("handoffId")).toString().trimmed();
        const QString task = item.value(QStringLiteral("taskId")).toString().trimmed();
        const QString from = item.value(QStringLiteral("fromOperator")).toString().trimmed();
        const QString to = item.value(QStringLiteral("toOperator")).toString().trimmed();
        QVariantMap itemMetadata;
        if (item.contains(QStringLiteral("metadata"))) {
            if (!item.value(QStringLiteral("metadata")).isObject()) return false;
            itemMetadata = item.value(QStringLiteral("metadata")).toObject().toVariantMap();
            if (!boundedHandoffMetadata(itemMetadata)) {
                return false;
            }
        }
        if (!validRunMetadata(itemMetadata, task)
            || (runMetadata(itemMetadata) && ++runRecordCount > kMaxRunHandoffRecords)) return false;
        bool statusOk = false;
        const Status status = statusFromString(item.value(QStringLiteral("status")).toString(),
                                               &statusOk);
        const QDateTime created = QDateTime::fromString(
                item.value(QStringLiteral("createdAt")).toString(), Qt::ISODateWithMs);
        const QDateTime expires = QDateTime::fromString(
                item.value(QStringLiteral("expiresAt")).toString(), Qt::ISODateWithMs);
        const QDateTime now = QDateTime::currentDateTimeUtc();
        if (id.isEmpty() || id.size() > 128
                || task.isEmpty() || task.size() > kMaxTaskIdLength
                || from.isEmpty() || from.size() > kMaxOperatorIdLength
                || to.isEmpty() || to.size() > kMaxOperatorIdLength || from == to
                || !statusOk || !created.isValid() || !expires.isValid()
                || expires <= created
                || created.msecsTo(expires) > static_cast<qint64>(kMaxHandoffExpirySeconds) * 1000LL
                || (runMetadata(itemMetadata) && created.msecsTo(expires) != 300000)
                || created.toUTC().msecsTo(now) < -kFutureHandoffTimestampToleranceMs
                || !item.contains(QStringLiteral("requiresSecondApproval"))
                || !item.value(QStringLiteral("requiresSecondApproval")).isBool()
                || !item.value(QStringLiteral("requiresSecondApproval")).toBool()
                || stagedRecords.contains(id)
                || !item.value(QStringLiteral("status")).isString()
                || !item.value(QStringLiteral("createdAt")).isString()
                || !item.value(QStringLiteral("expiresAt")).isString()
                // recordRevision is part of the schema, not an optional UI
                // hint.  Omitting it would silently reset a record to 1 and
                // could let an old view pass an expectedRevision check after
                // a snapshot restore.
                || !item.contains(QStringLiteral("recordRevision"))) {
            return false;
        }

        Record record;
        record.id = id;
        record.taskId = task;
        record.fromOperator = from;
        record.toOperator = to;
        if (!optionalJsonString(item, QStringLiteral("reason"), &record.reason)
            || !optionalJsonString(item, QStringLiteral("note"), &record.note)) {
            return false;
        }
        if (!boundedText(record.reason, kMaxReasonLength)
                || !boundedText(record.note, kMaxNoteLength)) {
            return false;
        }
        record.metadata = itemMetadata;
        record.imported = true;
        record.status = status;
        record.requiresSecondApproval = item.value(QStringLiteral("requiresSecondApproval"))
                .toBool(true);
        quint64 recordRevision = 1;
        const QJsonValue recordRevisionValue = item.value(QStringLiteral("recordRevision"));
        if (!recordRevisionValue.isUndefined()
            && !jsonUnsignedInteger(recordRevisionValue, &recordRevision)) {
            return false;
        }
        if (recordRevision == 0) {
            return false;
        }
        record.revision = recordRevision;
        record.createdAt = created.toUTC();
        record.expiresAt = expires.toUTC();
        QString resolvedAt;
        QString resolvedBy;
        if (!optionalJsonString(item, QStringLiteral("resolvedAt"), &resolvedAt)
            || !optionalJsonString(item, QStringLiteral("resolvedBy"), &resolvedBy)) {
            return false;
        }
        record.resolvedAt = resolvedAt.isEmpty()
                ? QDateTime()
                : QDateTime::fromString(resolvedAt, Qt::ISODateWithMs).toUTC();
        if (!resolvedAt.isEmpty() && !record.resolvedAt.isValid()) {
            return false;
        }
        if (record.resolvedAt.isValid()
                && record.resolvedAt.msecsTo(now) < -kFutureHandoffTimestampToleranceMs) {
            return false;
        }
        if (record.resolvedAt.isValid() && record.resolvedAt < record.createdAt) {
            return false;
        }
        record.resolvedBy = resolvedBy;
        if (!boundedText(record.resolvedBy, kMaxOperatorIdLength)) {
            return false;
        }
        if (record.status == Status::Pending
                && (record.resolvedAt.isValid() || !record.resolvedBy.isEmpty())) {
            return false;
        }
        if (record.status != Status::Pending && !record.resolvedAt.isValid()) {
            return false;
        }
        if (record.status != Status::Pending && record.status != Status::Expired
                && record.resolvedBy.isEmpty()) {
            return false;
        }
        if (record.status == Status::Expired
                && record.expiresAt.toUTC().msecsTo(now) < -kFutureHandoffTimestampToleranceMs) {
            return false;
        }
        stagedRecords.insert(record.id, record);
        stagedOrder.append(record.id);
        maxRevision = qMax(maxRevision, record.revision);
    }

    if (cancelled && cancelled->load()) return false;
    Snapshot complete;
    complete.records = stagedRecords; complete.order = stagedOrder;
    complete.sequence = maxSequence; complete.revision = maxRevision;
    for (const QString& id : stagedOrder) {
        if (cancelled && cancelled->load()) return false;
        complete.recordRepresentations.insert(id,
            QJsonDocument(persistentRecord(stagedRecords.value(id))).toJson(QJsonDocument::Compact));
    }
    snapshot = complete;
    return true;
}

bool MissionHandoffManager::importJson(const QString& json)
{
    if (thread() != QThread::currentThread()) return false;
    const QString frozen = json;
    Snapshot snapshot;
    if (!decodeSnapshot(frozen.toUtf8(), snapshot)) return false;
    // Legacy replacement API remains separate from the new append-only path.
    _records = snapshot.records; _order = snapshot.order;
    _sequence = snapshot.sequence; _revision = snapshot.revision;
    _collectionGeneration = QUuid::createUuid().toString(QUuid::WithoutBraces);
    emit handoffCollectionChanged();
    return true;
}

bool MissionHandoffManager::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxHandoffSnapshotBytes) {
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

bool MissionHandoffManager::loadFromFile(const QString& filePath)
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
    if (size < 0 || size > kMaxHandoffSnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxHandoffSnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importJson validates every record before replacing the collection, so a
    // malformed or stale file cannot partially change handoff state.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importJson(json);
}

// No live GUI QObject, manager instance, account, transport or live record is
// used in run(); file I/O owns only its worker-local QFile/QSaveFile objects.
// The QThread object is parentless and only its actual finished signal releases
// the manager's slot. Its inputs are immutable COW value snapshots.
class MissionHandoffHistoryWorker final : public QThread
{
public:
    MissionHandoffHistoryWorker(QString id, QString action, QString path,
        QString generation, MissionHandoffManager::Snapshot source,
        std::shared_ptr<const MissionHandoffManager::Snapshot> preview, QString previewId)
        : id(std::move(id)), action(std::move(action)), path(std::move(path)),
          generation(std::move(generation)), source(std::move(source)), preview(std::move(preview)),
          previewId(std::move(previewId)) {}
    void requestCancel() { cancelled.store(true); }
    const QString id, action, path, generation;
    const MissionHandoffManager::Snapshot source;
    const std::shared_ptr<const MissionHandoffManager::Snapshot> preview;
    const QString previewId;
    std::atomic_bool cancelled{false};
    bool ok = false;
    QString error = QStringLiteral("");
    QVariantMap reply;
    std::shared_ptr<const MissionHandoffManager::Snapshot> decoded;
    QString contentHash;
    int runCount = 0;
    int addedCount = 0;
    int duplicateCount = 0;
protected:
    void run() override
    {
        const auto fail = [&](const char* code) { error = QString::fromLatin1(code); };
        if (cancelled.load()) { fail("handoff-history-cancelled"); return; }
        if (action == QStringLiteral("save")) {
            QByteArray bytes;
            if (!MissionHandoffManager::encodeSnapshot(source, bytes, &cancelled)) {
                fail(cancelled.load() ? "handoff-history-cancelled" : "handoff-history-snapshot-budget-or-invalid"); return;
            }
            contentHash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            const auto saved = FleetSnapshotFileIo::saveAndReadback(path, bytes,
                kMaxHandoffSnapshotBytes, cancelled, QStringLiteral("handoff-history"));
            reply = {{QStringLiteral("snapshotId"), id}, {QStringLiteral("contentHash"), contentHash},
                {QStringLiteral("path"), path}, {QStringLiteral("fileWriteAttempted"), saved.fileWriteAttempted},
                {QStringLiteral("fileWrittenKnown"), saved.fileWrittenKnown}, {QStringLiteral("fileWritten"), saved.fileWritten},
                {QStringLiteral("readbackVerified"), saved.readbackVerified}, {QStringLiteral("readbackError"), saved.readbackError},
                {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}, {QStringLiteral("authenticated"), false}};
            ok = saved.ok; error = saved.error;
            return;
        }
        if (action == QStringLiteral("load")) {
            QByteArray bytes;
            if (!FleetSnapshotFileIo::readUtf8(path, kMaxHandoffSnapshotBytes,
                    cancelled, QStringLiteral("handoff-history"), &bytes, &error)) return;
            MissionHandoffManager::Snapshot candidate;
            if (!MissionHandoffManager::decodeSnapshot(bytes, candidate, &cancelled)) {
                fail(cancelled.load() ? "handoff-history-cancelled" : "handoff-history-snapshot-invalid"); return;
            }
            for (auto it = candidate.records.begin(); it != candidate.records.end(); ++it) {
                if (cancelled.load()) { fail("handoff-history-cancelled"); return; }
                it->imported = true; it->historicalOnly = true;
                if (runMetadata(it->metadata)) ++runCount;
            }
            contentHash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            if (cancelled.load()) { fail("handoff-history-cancelled"); return; }
            decoded = std::make_shared<const MissionHandoffManager::Snapshot>(std::move(candidate));
            ok = true;
            return;
        }
        if (action != QStringLiteral("merge") || !preview) {
            fail("handoff-history-action-invalid"); return;
        }
        MissionHandoffManager::Snapshot merged = source;
        for (const auto& record : source.records) if (runMetadata(record.metadata)) ++runCount;
        for (const QString& key : preview->order) {
            if (cancelled.load()) { fail("handoff-history-cancelled"); return; }
            const auto incoming = preview->records.constFind(key);
            if (incoming == preview->records.constEnd()) { fail("handoff-history-candidate-invalid"); return; }
            const auto existing = source.records.constFind(key);
            if (existing != source.records.constEnd()) {
                const QByteArray current = QJsonDocument(MissionHandoffManager::persistentRecord(existing.value()))
                    .toJson(QJsonDocument::Compact);
                if (current != preview->recordRepresentations.value(key)) {
                    fail("handoff-history-id-content-conflict"); return;
                }
                ++duplicateCount;
                continue;
            }
            if (merged.records.size() >= kMaxHandoffRecords
                || (runMetadata(incoming->metadata) && ++runCount > kMaxRunHandoffRecords)) {
                fail("handoff-history-collection-budget"); return;
            }
            auto record = incoming.value(); record.imported = true; record.historicalOnly = true;
            merged.records.insert(key, record); merged.order.append(key); ++addedCount;
        }
        if (addedCount > 0) {
            merged.sequence = qMax(merged.sequence, preview->sequence);
            merged.revision = qMax(merged.revision, preview->revision);
        }
        QByteArray bounded;
        if (!MissionHandoffManager::encodeSnapshot(merged, bounded, &cancelled)) {
            fail(cancelled.load() ? "handoff-history-cancelled" : "handoff-history-collection-budget"); return;
        }
        if (cancelled.load()) { fail("handoff-history-cancelled"); return; }
        decoded = std::make_shared<const MissionHandoffManager::Snapshot>(std::move(merged));
        ok = true;
    }
};

MissionHandoffManager::~MissionHandoffManager()
{
    _historyDestroying = true;
    if (_historyWorker) _historyWorker->requestCancel();
    // Never delete or wait on a running thread. Its finished->deleteLater
    // connection owns eventual cleanup even after this receiver is gone.
}

QVariantMap MissionHandoffManager::historyState() const
{
    if (QThread::currentThread() != thread() || _historyDestroying) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("busy"), _historyBusy},
        {QStringLiteral("jobId"), _historyJobId}, {QStringLiteral("action"), _historyAction},
        {QStringLiteral("stage"), _historyStage}, {QStringLiteral("cancelRequested"), _historyCancelRequested},
        {QStringLiteral("previewId"), _historyPreviewId}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap MissionHandoffManager::historyPreview() const
{
    return QThread::currentThread() == thread() && !_historyDestroying ? _historyPreviewDescriptor : QVariantMap();
}

QVariantMap MissionHandoffManager::historyResult() const
{
    return QThread::currentThread() == thread() && !_historyDestroying ? _historyResult : QVariantMap();
}

QVariantMap MissionHandoffManager::pageRow(const Record& record)
{
    const bool available = runMetadata(record.metadata);
    const QVariantMap brief = available ? record.metadata.value(QStringLiteral("brief")).toMap() : QVariantMap();
    return {{QStringLiteral("handoffId"), record.id}, {QStringLiteral("taskId"), record.taskId},
        {QStringLiteral("fromOperator"), record.fromOperator}, {QStringLiteral("toOperator"), record.toOperator},
        {QStringLiteral("status"), statusToString(record.status)}, {QStringLiteral("recordRevision"), qint64(record.revision)},
        {QStringLiteral("createdAt"), timestamp(record.createdAt)}, {QStringLiteral("expiresAt"), timestamp(record.expiresAt)},
        {QStringLiteral("resolvedAt"), timestamp(record.resolvedAt)}, {QStringLiteral("resolvedBy"), record.resolvedBy},
        {QStringLiteral("briefAvailable"), available},
        {QStringLiteral("briefHash"), available ? brief.value(QStringLiteral("contentHash")).toString() : QStringLiteral("")},
        {QStringLiteral("runId"), available ? brief.value(QStringLiteral("runId")).toString() : QStringLiteral("")},
        {QStringLiteral("origin"), record.imported ? QStringLiteral("imported") : QStringLiteral("local")}};
}

QVariantMap MissionHandoffManager::historyPreviewPage(const QString& previewId, const QVariant& page) const
{
    if (QThread::currentThread() != thread() || _historyDestroying) return pageRejection(QStringLiteral("wrong-thread"));
    quint64 requested = 0;
    if (!pageIndex(page, &requested)) return pageRejection(QStringLiteral("invalid-page"));
    const auto snapshot = _historyPreviewSnapshot;
    if (!snapshot || previewId.isEmpty() || previewId != _historyPreviewId)
        return pageRejection(QStringLiteral("history-preview-not-current"));
    const int total = snapshot->order.size(), count = (total + kHandoffPageSize - 1) / kHandoffPageSize;
    const int selected = count == 0 ? 0 : int(qMin(requested, quint64(count - 1)));
    QVariantList rows;
    for (int i = selected * kHandoffPageSize; i < total && rows.size() < kHandoffPageSize; ++i) {
        const QString id = snapshot->order.at(total - 1 - i);
        const auto found = snapshot->records.constFind(id);
        if (found == snapshot->records.constEnd()) return pageRejection(QStringLiteral("history-preview-invalid"));
        rows.append(pageRow(found.value()));
    }
    return {{QStringLiteral("page"), selected}, {QStringLiteral("pageCount"), count},
        {QStringLiteral("total"), total}, {QStringLiteral("records"), rows}};
}

QVariantMap MissionHandoffManager::historyPreviewRecord(const QString& previewId, const QString& handoffId) const
{
    if (QThread::currentThread() != thread() || _historyDestroying || previewId.isEmpty()
        || previewId != _historyPreviewId || !_historyPreviewSnapshot) return {};
    const auto snapshot = _historyPreviewSnapshot;
    const auto found = snapshot->records.constFind(handoffId);
    return found == snapshot->records.constEnd() ? QVariantMap() : toSummary(found.value());
}

QVariantMap MissionHandoffManager::requestHistory(QVariantMap intent, bool confirmed)
{
    QString action = QStringLiteral("");
    const auto receipt = [&](bool accepted, const QString& jobId, const QString& reason) {
        return QVariantMap{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
            {QStringLiteral("accepted"), accepted}, {QStringLiteral("jobId"), jobId}, {QStringLiteral("reason"), reason},
            {QStringLiteral("requestOnly"), true}, {QStringLiteral("historicalOnly"), true},
            {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
            {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    };
    if (QThread::currentThread() != thread() || _historyDestroying)
        return receipt(false, QStringLiteral(""), QStringLiteral("handoff-history-wrong-thread-or-retired"));
    const QStringList strings{QStringLiteral("action"), QStringLiteral("expectedJobId"),
        QStringLiteral("filePath"), QStringLiteral("previewId")};
    if (!confirmed || intent.size() != 5) return receipt(false, _historyJobId, QStringLiteral("handoff-history-intent-invalid"));
    for (const QString& key : strings) if (!intent.contains(key) || intent.value(key).type() != QVariant::String
        || intent.value(key).isNull()) return receipt(false, _historyJobId, QStringLiteral("handoff-history-intent-invalid"));
    if (intent.value(QStringLiteral("overwriteConfirmed")).type() != QVariant::Bool
        || intent.value(QStringLiteral("overwriteConfirmed")).isNull())
        return receipt(false, _historyJobId, QStringLiteral("handoff-history-intent-invalid"));
    action = intent.value(QStringLiteral("action")).toString();
    const QString expected = intent.value(QStringLiteral("expectedJobId")).toString();
    const QString path = intent.value(QStringLiteral("filePath")).toString();
    const QString previewId = intent.value(QStringLiteral("previewId")).toString();
    const bool overwrite = intent.value(QStringLiteral("overwriteConfirmed")).toBool();
    if (_historyEntryBusy || expected != _historyJobId)
        return receipt(false, _historyJobId, QStringLiteral("handoff-history-job-changed-or-entry-busy"));
    if (action == QStringLiteral("cancel")) {
        if (!path.isEmpty() || !previewId.isEmpty() || overwrite || !_historyBusy || !_historyWorker)
            return receipt(false, _historyJobId, QStringLiteral("handoff-history-cancel-not-current"));
        const QString id = _historyJobId;
        if (_historyCancelRequested) return receipt(true, id, QStringLiteral(""));
        _historyCancelRequested = true; _historyStage = QStringLiteral("cancelling");
        _historyWorker->requestCancel();
        const auto accepted = receipt(true, id, QStringLiteral(""));
        emit historyWorkspaceChanged();
        return accepted;
    }
    if (_historyBusy) return receipt(false, _historyJobId, QStringLiteral("handoff-history-busy"));
    if (action == QStringLiteral("discard")) {
        if (!path.isEmpty() || overwrite || previewId.isEmpty() || previewId != _historyPreviewId
            || !_historyPreviewSnapshot) return receipt(false, _historyJobId, QStringLiteral("handoff-history-preview-not-current"));
        _historyPreviewSnapshot.reset(); _historyPreviewDescriptor.clear(); _historyPreviewId = QStringLiteral("");
        const auto accepted = receipt(true, _historyJobId, QStringLiteral(""));
        emit historyWorkspaceChanged();
        return accepted;
    }
    const bool save = action == QStringLiteral("save"), load = action == QStringLiteral("load");
    const bool merge = action == QStringLiteral("merge");
    if ((!save && !load && !merge) || ((save || load) && (!previewId.isEmpty()
            || !FleetSnapshotFileIo::localJsonPath(path) || overwrite != save))
        || (merge && (!path.isEmpty() || overwrite || previewId.isEmpty()
            || previewId != _historyPreviewId || !_historyPreviewSnapshot)))
        return receipt(false, _historyJobId, QStringLiteral("handoff-history-intent-invalid"));
    _historyEntryBusy = true;
    const QPointer<MissionHandoffManager> self(this);
    struct Release {
        QPointer<MissionHandoffManager> owner;
        ~Release() { if (owner) owner->_historyEntryBusy = false; }
    } release{self};
    Snapshot source;
    if (save || merge) {
        source.records = _records; source.order = _order;
        source.sequence = _sequence; source.revision = _revision;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto* worker = new MissionHandoffHistoryWorker(id, action, path, _collectionGeneration,
        std::move(source), merge ? _historyPreviewSnapshot : nullptr, previewId);
    _historyJobId = id; _historyAction = action; _historyStage = QStringLiteral("running");
    _historyBusy = true; _historyCancelRequested = false; _historyWorker = worker;
    if (load) {
        _historyPreviewSnapshot.reset(); _historyPreviewDescriptor.clear(); _historyPreviewId = QStringLiteral("");
    }
    connect(worker, &QThread::finished, this, [self, worker, id]() {
        if (self) self->finishHistory(worker, id);
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
    const auto accepted = receipt(true, id, QStringLiteral(""));
    emit historyWorkspaceChanged();
    return accepted;
}

void MissionHandoffManager::finishHistory(MissionHandoffHistoryWorker* worker, const QString& id)
{
    if (QThread::currentThread() != thread() || _historyDestroying || !_historyBusy
        || _historyWorker != worker || _historyJobId != id || !worker->isFinished()) return;
    const QString action = worker->action;
    bool ok = worker->ok;
    QString error = worker->error;
    QVariantMap reply = worker->reply;
    bool changed = false;
    // A save commit is a fact even if cancellation/readback failed afterwards.
    const bool saved = action == QStringLiteral("save") && reply.value(QStringLiteral("fileWritten")).toBool()
        && reply.value(QStringLiteral("fileWrittenKnown")).toBool();
    if (!saved && (_historyCancelRequested || worker->cancelled.load())) {
        ok = false; error = QStringLiteral("handoff-history-cancelled");
    }
    if (ok && action == QStringLiteral("load")) {
        if (!worker->decoded) { ok = false; error = QStringLiteral("handoff-history-worker-output-invalid"); }
        else {
            _historyPreviewSnapshot = worker->decoded;
            _historyPreviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            _historyPreviewDescriptor = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("previewId"), _historyPreviewId},
                {QStringLiteral("jobId"), id}, {QStringLiteral("path"), worker->path}, {QStringLiteral("contentHash"), worker->contentHash},
                {QStringLiteral("recordCount"), worker->decoded->records.size()}, {QStringLiteral("runRecordCount"), worker->runCount},
                {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
                {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
                {QStringLiteral("flightCommandReleased"), false}};
            reply = {{QStringLiteral("preview"), _historyPreviewDescriptor}};
        }
    }
    if (ok && action == QStringLiteral("merge")) {
        if (!worker->decoded || worker->generation != _collectionGeneration
            || worker->preview != _historyPreviewSnapshot || worker->previewId != _historyPreviewId) {
            ok = false; error = QStringLiteral("handoff-history-collection-or-preview-changed");
        } else {
            _records = worker->decoded->records; _order = worker->decoded->order;
            _sequence = worker->decoded->sequence; _revision = worker->decoded->revision;
            if (worker->addedCount > 0) {
                _collectionGeneration = QUuid::createUuid().toString(QUuid::WithoutBraces);
                changed = true;
            }
            reply = {{QStringLiteral("previewId"), worker->previewId}, {QStringLiteral("addedCount"), worker->addedCount},
                {QStringLiteral("duplicateCount"), worker->duplicateCount}, {QStringLiteral("recordCount"), _records.size()}};
            _historyPreviewSnapshot.reset(); _historyPreviewDescriptor.clear(); _historyPreviewId = QStringLiteral("");
        }
    }
    _historyResult = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("jobId"), id},
        {QStringLiteral("action"), action}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), error},
        {QStringLiteral("reply"), reply}, {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    _historyStage = ok ? QStringLiteral("complete")
        : (_historyCancelRequested || worker->cancelled.load()) ? QStringLiteral("cancelled") : QStringLiteral("failed");
    _historyWorker.clear(); _historyBusy = false;
    const QPointer<MissionHandoffManager> self(this);
    if (changed) emit handoffCollectionChanged();
    if (self) emit historyWorkspaceChanged();
}

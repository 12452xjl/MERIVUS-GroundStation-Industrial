#include "FleetEventArchiveHistoryReader.h"

#include "FleetArchiveDirectoryInventory.h"
#include "FleetArchiveTaskIndex.h"
#include "FleetEventArchiveSql.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QLockFile>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVariantList>
#include <QtEndian>

#include <algorithm>

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr int kMaximumDirectoryEntries = 4096;
constexpr int kMaximumSegments = 128;
constexpr qint64 kFacadeMetadataReserve = 512;
constexpr qint64 kTaskSearchFacadeMetadataReserve = 2048;
const QString kPrefix = QStringLiteral("fleet-events-sealed-");
const QString kSuffix = QStringLiteral(".sqlite");
const QString kCurrentBasename = QStringLiteral("fleet-events-v1.sqlite");

enum class ArchiveFileKind { SealedSegment, OfflineCurrent };

struct Segment {
    QString basename;
    QString databaseEpoch;
    QString sealRunId;
    qint64 bytes = 0;
    QDateTime modified;
};

struct Fingerprint {
    QByteArray header;
    QByteArray sha256;
    qint64 bytes = 0;
    QDateTime modified;
};

bool uuid(const QString& value)
{
    return !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces).toLower() == value;
}

bool decimal(const QString& value, qint64* number = nullptr)
{
    if (value.isEmpty() || value.size() > 19 || (value.size() > 1 && value.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : value) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = value.toLongLong(&ok, 10);
    if (!ok || parsed < 0 || QString::number(parsed) != value) return false;
    if (number) *number = parsed;
    return true;
}

bool lowerSha256(const QString& value)
{
    if (value.size() != 64) return false;
    for (const QChar ch : value) {
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    }
    return true;
}

bool identifier(const QString& value)
{
    if (value.isEmpty() || value.size() > 128) return false;
    for (const QChar ch : value) {
        if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || ch == QLatin1Char('-') || ch == QLatin1Char('_'))) return false;
    }
    return true;
}

bool integerIn(const QVariant& value, qint64 minimum, qint64 maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false;
    }
    bool ok = false;
    const qint64 parsed = value.toLongLong(&ok);
    return ok && parsed >= minimum && parsed <= maximum;
}

bool policyValid(const QVariantMap& policy)
{
    return policy.size() == 7
        && integerIn(policy.value(QStringLiteral("maximumEvents")), 1000, 1000000)
        && integerIn(policy.value(QStringLiteral("maximumDatabaseMiB")), 32, 4096)
        && integerIn(policy.value(QStringLiteral("maximumQueueEvents")), 16, 4096)
        && integerIn(policy.value(QStringLiteral("maximumQueueMiB")), 1, 64)
        && integerIn(policy.value(QStringLiteral("batchEventLimit")), 1, 128)
        && integerIn(policy.value(QStringLiteral("busyTimeoutMs")), 1, 1000)
        && integerIn(policy.value(QStringLiteral("maximumHistoryMiB")), 1, 8);
}

bool parseBasename(const QString& name, Segment* segment)
{
    if (!segment || !name.startsWith(kPrefix) || !name.endsWith(kSuffix)
        || name.size() != kPrefix.size() + 36 + 1 + 36 + kSuffix.size()
        || name.at(kPrefix.size() + 36) != QLatin1Char('-')) return false;
    const QString epoch = name.mid(kPrefix.size(), 36);
    const QString run = name.mid(kPrefix.size() + 37, 36);
    if (!uuid(epoch) || !uuid(run)) return false;
    segment->basename = name; segment->databaseEpoch = epoch; segment->sealRunId = run;
    return true;
}

bool requestValid(const QVariantMap& request)
{
    if (request.value(QStringLiteral("kind")).type() != QVariant::String
        || request.value(QStringLiteral("kind")).isNull()) return false;
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("catalog")) return request.size() == 1;
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("search"))
        return FleetEventArchiveHistoryReader::searchRequestValid(request);
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("task-index"))
        return FleetArchiveTaskIndex::requestValid(request);
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("offline")) {
        if (request.size() != 6) return false;
        for (const QString& key : {QStringLiteral("databaseEpoch"), QStringLiteral("fileFingerprint"),
            QStringLiteral("runFilter"), QStringLiteral("afterRowId")}) {
            if (request.value(key).type() != QVariant::String || request.value(key).isNull()) return false;
        }
        qint64 cursor = 0;
        if (!request.value(QStringLiteral("runFilter")).toString().isEmpty()
            || !decimal(request.value(QStringLiteral("afterRowId")).toString(), &cursor)
            || !integerIn(request.value(QStringLiteral("limit")), 1, 256)) return false;
        const QString epoch = request.value(QStringLiteral("databaseEpoch")).toString();
        const QString hash = request.value(QStringLiteral("fileFingerprint")).toString();
        return cursor == 0 ? (epoch.isEmpty() && hash.isEmpty()) : (uuid(epoch) && lowerSha256(hash));
    }
    if (request.size() != 7 || request.value(QStringLiteral("kind")).toString() != QStringLiteral("page")) return false;
    const QStringList keys{QStringLiteral("basename"), QStringLiteral("databaseEpoch"), QStringLiteral("sealRunId"),
        QStringLiteral("runFilter"), QStringLiteral("afterRowId")};
    for (const QString& key : keys) if (request.value(key).type() != QVariant::String) return false;
    Segment segment;
    const QString filter = request.value(QStringLiteral("runFilter")).toString();
    return parseBasename(request.value(QStringLiteral("basename")).toString(), &segment)
        && segment.databaseEpoch == request.value(QStringLiteral("databaseEpoch")).toString()
        && segment.sealRunId == request.value(QStringLiteral("sealRunId")).toString()
        && (filter.isEmpty() || uuid(filter))
        && decimal(request.value(QStringLiteral("afterRowId")).toString())
        && integerIn(request.value(QStringLiteral("limit")), 1, 256);
}

bool localDirectory(const QString& directory)
{
    const QString normalized = QDir::fromNativeSeparators(directory);
    const QFileInfo info(normalized);
    return !normalized.isEmpty() && normalized.size() <= 4096 && QDir::isAbsolutePath(normalized)
        && !normalized.startsWith(QStringLiteral("//")) && !normalized.contains(QChar::Null)
        && QDir::cleanPath(normalized) == normalized && info.exists() && info.isDir() && info.isReadable()
        && !info.isSymLink() && QDir::fromNativeSeparators(info.canonicalFilePath()) == normalized;
}

bool sidecarsAbsent(const QString& path)
{
    for (const QString& suffix : {QStringLiteral("-journal"), QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        const QFileInfo info(path + suffix);
        if (info.exists() || info.isSymLink()) return false;
    }
    return true;
}

bool safeFile(const QString& directory, const QString& basename, qint64 maximumBytes, QFileInfo* actual = nullptr,
              ArchiveFileKind kind = ArchiveFileKind::SealedSegment)
{
    Segment ignored;
    if (!localDirectory(directory)) return false;
    // These namespaces remain disjoint. Sealed callers never gain permission
    // to open a current file; offline callers can only open this one basename.
    if (kind == ArchiveFileKind::SealedSegment) {
        if (!parseBasename(basename, &ignored)) return false;
    } else if (kind != ArchiveFileKind::OfflineCurrent || basename != kCurrentBasename) return false;
    const QString path = QDir(directory).filePath(basename);
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || info.isSymLink() || info.size() < 512 || info.size() > maximumBytes
        || QDir::fromNativeSeparators(info.canonicalFilePath()) != path || !sidecarsAbsent(path)) return false;
    if (actual) *actual = info;
    return true;
}

bool fingerprint(const QString& directory, const QString& basename, qint64 maximumBytes,
                 const std::atomic_bool& cancelled, Fingerprint* snapshot,
                 ArchiveFileKind kind = ArchiveFileKind::SealedSegment)
{
    QFileInfo before;
    if (!snapshot || cancelled.load() || !safeFile(directory, basename, maximumBytes, &before, kind)) return false;
    QFile file(before.filePath());
    if (!file.open(QIODevice::ReadOnly) || file.size() != before.size()) return false;
    const QByteArray header = file.read(100);
    if (header.size() != 100 || header.left(16) != QByteArray("SQLite format 3\0", 16)
        || static_cast<unsigned char>(header.at(18)) != 1 || static_cast<unsigned char>(header.at(19)) != 1) return false;
    const auto unsigned32 = [&header](int offset) {
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(header.constData() + offset));
    };
    const quint16 encodedPageSize = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(header.constData() + 16));
    const qint64 pageSize = encodedPageSize == 1 ? 65536 : encodedPageSize;
    if (pageSize < 512 || pageSize > 65536 || (pageSize & (pageSize - 1)) != 0
        || before.size() % pageSize != 0
        || unsigned32(60) != static_cast<quint32>(FleetEventArchiveSql::schemaVersion())
        || unsigned32(68) != static_cast<quint32>(FleetEventArchiveSql::applicationId())
        || unsigned32(24) != unsigned32(92) || unsigned32(28) != static_cast<quint64>(before.size() / pageSize)
        || !file.seek(0)) return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 remaining = before.size();
    while (remaining > 0) {
        if (cancelled.load()) return false;
        const QByteArray chunk = file.read(qMin<qint64>(remaining, 64LL * 1024LL));
        if (chunk.isEmpty() || chunk.size() > remaining) return false;
        hash.addData(chunk); remaining -= chunk.size();
    }
    if (cancelled.load() || !file.read(1).isEmpty() || file.error() != QFileDevice::NoError || file.size() != before.size()) return false;
    file.close();
    QFileInfo after;
    if (!safeFile(directory, basename, maximumBytes, &after, kind)
        || after.size() != before.size() || after.lastModified() != before.lastModified()) return false;
    snapshot->header = header; snapshot->sha256 = hash.result();
    snapshot->bytes = before.size(); snapshot->modified = before.lastModified();
    return true;
}

bool sameFingerprint(const Fingerprint& first, const Fingerprint& second)
{
    return first.bytes == second.bytes && first.modified == second.modified
        && first.header == second.header && first.sha256 == second.sha256;
}

bool scanSegments(const QString& directory, qint64 maximumBytes, const std::atomic_bool& cancelled,
                  QList<Segment>* segments, QString* directoryIdentity, QString* error)
{
    if (!segments || !directoryIdentity || !error || !localDirectory(directory)) return false;
    const auto inventory = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::SealedEventArchives, kMaximumSegments, &cancelled);
    if (!inventory.ok || !inventory.enumerationCompleted || inventory.directoryIdentity.isEmpty()) {
        *error = cancelled.load() ? QStringLiteral("archive-sealed-history-cancelled")
                                  : QStringLiteral("archive-sealed-directory-inventory-failed");
        return false;
    }
    QList<Segment> found;
    for (const auto& entry : inventory.entries) {
        if (cancelled.load()) { *error = QStringLiteral("archive-sealed-history-cancelled"); return false; }
        const QString& name = entry.basename;
        Segment segment;
        QFileInfo actual;
        // An invalid case variant, namespace sidecar or directory is an error,
        // not an omitted segment whose absence could make a catalog look whole.
        if (!entry.regularFile || entry.symlink || !parseBasename(name, &segment)
            || !safeFile(directory, name, maximumBytes, &actual)) {
            *error = QStringLiteral("archive-sealed-namespace-rejected"); return false;
        }
        segment.bytes = actual.size(); segment.modified = actual.lastModified();
        found.append(segment);
        if (found.size() > kMaximumSegments) { *error = QStringLiteral("archive-sealed-segment-limit"); return false; }
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-sealed-history-cancelled"); return false; }
    if (!localDirectory(directory)) { *error = QStringLiteral("archive-sealed-directory-changed"); return false; }
    std::sort(found.begin(), found.end(), [](const Segment& first, const Segment& second) { return first.basename < second.basename; });
    *segments = found;
    *directoryIdentity = inventory.directoryIdentity;
    return true;
}

bool sameSegments(const QList<Segment>& first, const QList<Segment>& second)
{
    if (first.size() != second.size()) return false;
    for (int index = 0; index < first.size(); ++index) {
        if (first.at(index).basename != second.at(index).basename
            || first.at(index).databaseEpoch != second.at(index).databaseEpoch
            || first.at(index).sealRunId != second.at(index).sealRunId
            || first.at(index).bytes != second.at(index).bytes
            || first.at(index).modified != second.at(index).modified) return false;
    }
    return true;
}

bool connectionSettingsMatch(QSqlDatabase& database, int expectedTimeout)
{
    QSqlQuery busy(database);
    if (!busy.exec(QStringLiteral("PRAGMA busy_timeout")) || !busy.next()
        || !integerIn(busy.value(0), expectedTimeout, expectedTimeout) || busy.next() || busy.lastError().isValid()) return false;
    QSqlQuery journal(database);
    return journal.exec(QStringLiteral("PRAGMA journal_mode")) && journal.next()
        && journal.value(0).type() == QVariant::String && journal.value(0).toString() == QStringLiteral("delete")
        && !journal.next() && !journal.lastError().isValid();
}

bool descriptorValid(const QVariantMap& metadata, const QString& epoch)
{
    if (metadata.size() != 4 || metadata.value(QStringLiteral("databaseEpoch")).type() != QVariant::String
        || metadata.value(QStringLiteral("databaseEpoch")).toString() != epoch) return false;
    qint64 first = 0, last = 0;
    for (const QString& key : {QStringLiteral("retiredCount"), QStringLiteral("retainedFromRowId"), QStringLiteral("retainedThroughRowId")}) {
        if (metadata.value(key).type() != QVariant::String || !decimal(metadata.value(key).toString())) return false;
    }
    return decimal(metadata.value(QStringLiteral("retainedFromRowId")).toString(), &first)
        && decimal(metadata.value(QStringLiteral("retainedThroughRowId")).toString(), &last)
        && ((first == 0 && last == 0) || (first > 0 && last >= first));
}

bool readSegment(const QString& directory, const Segment& segment, const QVariantMap& policy,
                 const QVariantMap& request, const std::atomic_bool& cancelled,
                 QVariantMap* result, QString* error, Fingerprint* observed = nullptr)
{
    const qint64 maximumBytes = policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
    const qint64 maximumResult = policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB;
    Fingerprint before;
    if (!fingerprint(directory, segment.basename, maximumBytes, cancelled, &before)) {
        *error = QStringLiteral("archive-sealed-file-rejected"); return false;
    }
    if (observed && (segment.bytes != before.bytes || segment.modified != before.modified)) {
        *error = QStringLiteral("archive-search-inventory-file-changed"); return false;
    }
    const QString connection = QStringLiteral("merivus-event-sealed-history-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool valid = false;
    QVariantMap value;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(QDir(directory).filePath(segment.basename));
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=")
            + QString::number(policy.value(QStringLiteral("busyTimeoutMs")).toInt()));
        const bool opened = !cancelled.load() && database.open();
        if (!opened) *error = QStringLiteral("archive-sealed-readonly-open-failed");
        else if (!connectionSettingsMatch(database, policy.value(QStringLiteral("busyTimeoutMs")).toInt())) {
            *error = QStringLiteral("archive-sealed-readonly-policy-rejected");
        } else if (!cancelled.load() && database.transaction()) {
            QVariantMap descriptor;
            valid = !cancelled.load()
                && FleetEventArchiveSql::sealedRunMatches(database, segment.databaseEpoch, segment.sealRunId)
                && !cancelled.load() && FleetEventArchiveSql::inspect(database, &descriptor)
                && descriptorValid(descriptor, segment.databaseEpoch);
            if (!database.rollback()) valid = false; // End this read-only descriptor snapshot.
            if (!valid) *error = QStringLiteral("archive-sealed-identity-rejected");
            if (valid && request.value(QStringLiteral("kind")).toString() == QStringLiteral("page")) {
                qint64 cursor = 0;
                valid = decimal(request.value(QStringLiteral("afterRowId")).toString(), &cursor)
                    && !cancelled.load()
                    && FleetEventArchiveSql::readPage(database, request.value(QStringLiteral("runFilter")).toString(), cursor,
                        request.value(QStringLiteral("limit")).toInt(), maximumResult - 2 * kFacadeMetadataReserve,
                        segment.databaseEpoch, &value, error, &cancelled);
                if (valid) {
                    value.insert(QStringLiteral("basename"), segment.basename);
                    value.insert(QStringLiteral("sealRunId"), segment.sealRunId);
                    value.insert(QStringLiteral("sealedSegmentOnly"), true);
                    value.insert(QStringLiteral("executionPermit"), false);
                    value.insert(QStringLiteral("flightCommandReleased"), false);
                    value.insert(QStringLiteral("restoresAuthorization"), false);
                    // The shared field excludes its own field. Recompute it
                    // after adding the sealed-context fields to this envelope.
                    value.remove(QStringLiteral("pageBytesWithoutThisField"));
                    value.insert(QStringLiteral("pageBytesWithoutThisField"),
                        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size());
                }
            } else if (valid) {
                value = {{QStringLiteral("basename"), segment.basename},
                    {QStringLiteral("databaseEpoch"), segment.databaseEpoch}, {QStringLiteral("sealRunId"), segment.sealRunId},
                    {QStringLiteral("retainedFromRowId"), descriptor.value(QStringLiteral("retainedFromRowId"))},
                    {QStringLiteral("retainedThroughRowId"), descriptor.value(QStringLiteral("retainedThroughRowId"))},
                    {QStringLiteral("retiredCount"), descriptor.value(QStringLiteral("retiredCount"))},
                    {QStringLiteral("fileBytes"), QString::number(before.bytes)}};
            }
        } else *error = QStringLiteral("archive-sealed-descriptor-begin-failed");
        // All queries from helpers have left scope. No SQL object or connection
        // is transferred to the UI or survives removeDatabase below.
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    if (!valid || cancelled.load()) return false;
    Fingerprint after;
    if (!fingerprint(directory, segment.basename, maximumBytes, cancelled, &after) || !sameFingerprint(before, after)) {
        *error = QStringLiteral("archive-sealed-file-changed"); return false;
    }
    if (QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size()
        > maximumResult - kFacadeMetadataReserve) {
        *error = QStringLiteral("archive-sealed-result-byte-limit"); return false;
    }
    *result = value;
    if (observed) *observed = before;
    return true;
}

bool readOfflineCurrent(const QString& directory, const QVariantMap& policy,
                        const QVariantMap& request, const std::atomic_bool& cancelled,
                        QVariantMap* result, QString* error, Fingerprint* observed = nullptr,
                        const Segment* expectedInventory = nullptr)
{
    if (cancelled.load() || !localDirectory(directory)) {
        *error = QStringLiteral("archive-offline-directory-rejected"); return false;
    }
    const QString currentPath = QDir(directory).filePath(kCurrentBasename);
    const QString ownerPath = currentPath + QStringLiteral(".owner.lock");
    const QFileInfo ownerInfo(ownerPath);
    if (ownerInfo.isSymLink() || (ownerInfo.exists() && (!ownerInfo.isFile() || ownerInfo.size() > 16384
        || QDir::fromNativeSeparators(ownerInfo.canonicalFilePath()) != ownerPath))) {
        *error = QStringLiteral("archive-offline-owner-lock-path-rejected"); return false;
    }
    // Match the writer's cooperative ownership protocol, including path checks.
    // Only this lock is created/released; the database is never created or fixed.
    // Zero disables age-based expiry, not Qt's dead-process lock handling.
    QLockFile owner(ownerPath);
    owner.setStaleLockTime(0);
    if (!owner.tryLock(0)) {
        *error = QStringLiteral("archive-offline-owner-lock-unavailable"); return false;
    }
    const qint64 maximumBytes = policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
    const qint64 maximumResult = policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB;
    Fingerprint before;
    if (!fingerprint(directory, kCurrentBasename, maximumBytes, cancelled, &before, ArchiveFileKind::OfflineCurrent)) {
        *error = QStringLiteral("archive-offline-file-rejected"); return false;
    }
    if (expectedInventory && (expectedInventory->bytes != before.bytes || expectedInventory->modified != before.modified)) {
        *error = QStringLiteral("archive-search-inventory-file-changed"); return false;
    }
    const QString fileHash = QString::fromLatin1(before.sha256.toHex());
    const QString expectedHash = request.value(QStringLiteral("fileFingerprint")).toString();
    if (!expectedHash.isEmpty() && expectedHash != fileHash) {
        *error = QStringLiteral("archive-offline-file-changed-restart-required"); return false;
    }
    const QString connection = QStringLiteral("merivus-event-offline-history-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool valid = false;
    QVariantMap value;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(currentPath);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=")
            + QString::number(policy.value(QStringLiteral("busyTimeoutMs")).toInt()));
        const bool opened = !cancelled.load() && database.open();
        if (!opened) *error = QStringLiteral("archive-offline-readonly-open-failed");
        else if (!connectionSettingsMatch(database, policy.value(QStringLiteral("busyTimeoutMs")).toInt())) {
            *error = QStringLiteral("archive-offline-readonly-policy-rejected");
        } else if (!cancelled.load() && database.transaction()) {
            QVariantMap descriptor;
            valid = !cancelled.load() && FleetEventArchiveSql::inspect(database, &descriptor);
            const QString epoch = descriptor.value(QStringLiteral("databaseEpoch")).toString();
            const QString expectedEpoch = request.value(QStringLiteral("databaseEpoch")).toString();
            valid = valid && uuid(epoch) && descriptorValid(descriptor, epoch)
                && (expectedEpoch.isEmpty() || expectedEpoch == epoch);
            if (!database.rollback()) valid = false;
            if (!valid) *error = QStringLiteral("archive-offline-identity-rejected");
            if (valid) {
                qint64 cursor = 0;
                valid = decimal(request.value(QStringLiteral("afterRowId")).toString(), &cursor)
                    && !cancelled.load()
                    && FleetEventArchiveSql::readPage(database, QStringLiteral(""), cursor,
                        request.value(QStringLiteral("limit")).toInt(), maximumResult - 2 * kFacadeMetadataReserve,
                        epoch, &value, error, &cancelled);
                if (valid) {
                    value.insert(QStringLiteral("basename"), kCurrentBasename);
                    value.insert(QStringLiteral("offlineCurrentOnly"), true);
                    value.insert(QStringLiteral("sealedSegmentOnly"), false);
                    value.insert(QStringLiteral("fileFingerprint"), fileHash);
                    value.insert(QStringLiteral("cooperativeOwnerLockHeldDuringRead"), true);
                    value.insert(QStringLiteral("executionPermit"), false);
                    value.insert(QStringLiteral("flightCommandReleased"), false);
                    value.insert(QStringLiteral("restoresAuthorization"), false);
                    value.remove(QStringLiteral("pageBytesWithoutThisField"));
                    value.insert(QStringLiteral("pageBytesWithoutThisField"),
                        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size());
                }
            }
        } else *error = QStringLiteral("archive-offline-descriptor-begin-failed");
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    if (!valid || cancelled.load()) return false;
    // Still hold owner through SQL handle destruction and the post-read hash.
    // A later page must match this whole-file digest and actual database epoch.
    Fingerprint after;
    if (!fingerprint(directory, kCurrentBasename, maximumBytes, cancelled, &after, ArchiveFileKind::OfflineCurrent)
        || !sameFingerprint(before, after)) {
        *error = QStringLiteral("archive-offline-file-changed-restart-required"); return false;
    }
    if (QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size()
        > maximumResult - kFacadeMetadataReserve) {
        *error = QStringLiteral("archive-offline-result-byte-limit"); return false;
    }
    *result = value;
    if (observed) *observed = before;
    return true;
}

bool textValue(const QVariant& value)
{
    return value.type() == QVariant::String && !value.isNull();
}

bool intValue(const QVariant& value, int minimum, int maximum)
{
    return value.type() == QVariant::Int && !value.isNull()
        && value.toInt() >= minimum && value.toInt() <= maximum;
}

bool flagValue(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool searchCursorValid(const QVariant& value, bool allowEmpty)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap cursor = value.toMap();
    if (cursor.isEmpty()) return allowEmpty;
    if (cursor.size() != 5 || !intValue(cursor.value(QStringLiteral("fileIndex")), 0, 129)) return false;
    for (const QString& key : {QStringLiteral("inventoryFingerprint"), QStringLiteral("afterRowId"),
        QStringLiteral("fileFingerprint"), QStringLiteral("databaseEpoch")})
        if (!textValue(cursor.value(key))) return false;
    qint64 after = 0;
    if (!lowerSha256(cursor.value(QStringLiteral("inventoryFingerprint")).toString())
        || !decimal(cursor.value(QStringLiteral("afterRowId")).toString(), &after)) return false;
    return after == 0 ? cursor.value(QStringLiteral("fileFingerprint")).toString().isEmpty()
        && cursor.value(QStringLiteral("databaseEpoch")).toString().isEmpty()
        : lowerSha256(cursor.value(QStringLiteral("fileFingerprint")).toString())
            && uuid(cursor.value(QStringLiteral("databaseEpoch")).toString());
}

bool searchInventory(const QString& directory, const QVariantMap& policy, const QString& scope,
                     const std::atomic_bool& cancelled, QList<Segment>* files, QString* digest, QString* error)
{
    QString identity;
    const qint64 maximumBytes = policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
    if (!scanSegments(directory, maximumBytes, cancelled, files, &identity, error)) return false;
    if (scope == QStringLiteral("sealed-and-offline")) {
        QFileInfo info;
        if (!safeFile(directory, kCurrentBasename, maximumBytes, &info, ArchiveFileKind::OfflineCurrent)) {
            *error = QStringLiteral("archive-search-required-current-file-rejected"); return false;
        }
        Segment current;
        current.basename = kCurrentBasename; current.bytes = info.size(); current.modified = info.lastModified();
        files->append(current);
    }
    QVariantList descriptors;
    for (const Segment& file : *files) {
        if (cancelled.load()) return false;
        if (!file.modified.isValid()) {
            *error = QStringLiteral("archive-search-file-metadata-unknown"); return false;
        }
        descriptors.append(QVariantMap{{QStringLiteral("basename"), file.basename},
            {QStringLiteral("databaseEpoch"), file.databaseEpoch}, {QStringLiteral("sealRunId"), file.sealRunId},
            {QStringLiteral("bytes"), QString::number(file.bytes)},
            {QStringLiteral("modifiedUtcMs"), QString::number(file.modified.toMSecsSinceEpoch())}});
    }
    const QVariantMap inventory{{QStringLiteral("directoryIdentity"), identity},
        {QStringLiteral("scope"), scope}, {QStringLiteral("files"), descriptors}};
    *digest = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(inventory)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    return !cancelled.load();
}

bool rowMatchesSearch(const QVariantMap& row, const QVariantMap& request)
{
    // SQL has already decoded/checked the actual codec envelope. Its payload
    // is record.event.payload, not an invented projectedPayload field.
    const QVariantMap payload = row.value(QStringLiteral("record")).toMap()
        .value(QStringLiteral("event")).toMap().value(QStringLiteral("payload")).toMap();
    const auto decodedIntegerEquals = [](const QVariant& value, int expected) {
        // Canonical JSON decoding may represent an exact integer as Double.
        // Do not accept strings/bools/null, or coerce a fractional number.
        if (value.isNull()) return false;
        switch (value.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong:
        case QVariant::Double: return value.toDouble() == static_cast<double>(expected);
        default: return false;
        }
    };
    const int vehicle = request.value(QStringLiteral("vehicleId")).toInt();
    if (vehicle != 0 && !decodedIntegerEquals(payload.value(QStringLiteral("vehicleId")), vehicle)) return false;
    if (request.contains(QStringLiteral("taskIdHash"))) {
        const QVariant taskIdHash = payload.value(QStringLiteral("taskIdHash"));
        if (!textValue(taskIdHash) || taskIdHash != request.value(QStringLiteral("taskIdHash"))) return false;
    }
    const QString operation = request.value(QStringLiteral("operation")).toString();
    if (operation.isEmpty()) return true;
    if (payload.value(QStringLiteral("payloadProjection")).toString() != QStringLiteral("fixed-diagnostic-summary-v2")
        || payload.value(QStringLiteral("engineeringReview")).type() != QVariant::Map) return false;
    const QVariantMap review = payload.value(QStringLiteral("engineeringReview")).toMap();
    return review.size() == 21 && decodedIntegerEquals(review.value(QStringLiteral("schemaVersion")), 1)
        && textValue(review.value(QStringLiteral("operation")))
        && review.value(QStringLiteral("operation")).toString() == operation;
}

QVariantMap searchIdentity(const Segment& file, int index, const QVariantMap& page, const Fingerprint& fingerprint)
{
    return {{QStringLiteral("fileIndex"), index}, {QStringLiteral("basename"), file.basename},
        {QStringLiteral("databaseEpoch"), page.value(QStringLiteral("databaseEpoch"))},
        {QStringLiteral("sealRunId"), file.basename == kCurrentBasename ? QStringLiteral("") : file.sealRunId},
        {QStringLiteral("fileFingerprint"), QString::fromLatin1(fingerprint.sha256.toHex())},
        {QStringLiteral("offlineCurrentOnly"), file.basename == kCurrentBasename}};
}

bool readSearch(const QString& directory, const QVariantMap& policy, const QVariantMap& request,
                const std::atomic_bool& cancelled, QVariantMap* result, QString* error)
{
    QList<Segment> files;
    QString inventory;
    const QString scope = request.value(QStringLiteral("scope")).toString();
    if (!searchInventory(directory, policy, scope, cancelled, &files, &inventory, error)) return false;
    const QVariantMap originalCursor = request.value(QStringLiteral("cursor")).toMap();
    int index = originalCursor.isEmpty() ? 0 : originalCursor.value(QStringLiteral("fileIndex")).toInt();
    QString after = originalCursor.isEmpty() ? QStringLiteral("0") : originalCursor.value(QStringLiteral("afterRowId")).toString();
    QString expectedHash = originalCursor.isEmpty() ? QStringLiteral("") : originalCursor.value(QStringLiteral("fileFingerprint")).toString();
    QString expectedEpoch = originalCursor.isEmpty() ? QStringLiteral("") : originalCursor.value(QStringLiteral("databaseEpoch")).toString();
    if (!originalCursor.isEmpty() && (originalCursor.value(QStringLiteral("inventoryFingerprint")).toString() != inventory
        || index >= files.size())) {
        *error = QStringLiteral("archive-search-inventory-changed-restart-required"); return false;
    }
    QVariantList matches, summaries;
    int scanned = 0;
    bool byteBoundary = false;
    const qint64 maximumResult = policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB;
    // Leave ample room for the final envelope and all four file-page summaries.
    const qint64 rowsBudget = maximumResult - 16384;
    qint64 rowsBytes = 2;
    while (index < files.size() && summaries.size() < 4 && matches.size() < 64 && !byteBoundary) {
        if (cancelled.load()) return false;
        const Segment& file = files.at(index);
        const bool offline = file.basename == kCurrentBasename;
        const int limit = qMin(64, 64 - matches.size());
        QVariantMap page;
        Fingerprint observed;
        const QVariantMap pageRequest = offline
            ? QVariantMap{{QStringLiteral("kind"), QStringLiteral("offline")},
                {QStringLiteral("databaseEpoch"), expectedEpoch}, {QStringLiteral("fileFingerprint"), expectedHash},
                {QStringLiteral("runFilter"), QStringLiteral("")}, {QStringLiteral("afterRowId"), after}, {QStringLiteral("limit"), limit}}
            : QVariantMap{{QStringLiteral("kind"), QStringLiteral("page")}, {QStringLiteral("basename"), file.basename},
                {QStringLiteral("databaseEpoch"), file.databaseEpoch}, {QStringLiteral("sealRunId"), file.sealRunId},
                {QStringLiteral("runFilter"), QStringLiteral("")}, {QStringLiteral("afterRowId"), after}, {QStringLiteral("limit"), limit}};
        const bool ok = offline ? readOfflineCurrent(directory, policy, pageRequest, cancelled, &page, error, &observed, &file)
                                : readSegment(directory, file, policy, pageRequest, cancelled, &page, error, &observed);
        if (!ok) return false;
        const QString actualHash = QString::fromLatin1(observed.sha256.toHex());
        const QString epoch = page.value(QStringLiteral("databaseEpoch")).toString();
        if (after != QStringLiteral("0") && (expectedHash != actualHash || expectedEpoch != epoch)) {
            *error = QStringLiteral("archive-search-file-changed-restart-required"); return false;
        }
        const QVariantMap identity = searchIdentity(file, index, page, observed);
        const QVariantList pageRows = page.value(QStringLiteral("rows")).toList();
        QString next = after;
        int pageScanned = 0, pageMatched = 0, unclean = 0, dropped = 0, discarded = 0;
        for (const QVariant& item : pageRows) {
            if (cancelled.load()) return false;
            const QVariantMap row = item.toMap();
            if (rowMatchesSearch(row, request)) {
                QVariantMap match = identity;
                match.insert(QStringLiteral("row"), row);
                const qint64 charge = QJsonDocument(QJsonObject::fromVariantMap(match)).toJson(QJsonDocument::Compact).size() + 1LL;
                if (rowsBytes + charge > rowsBudget) { byteBoundary = true; break; }
                matches.append(match); rowsBytes += charge; ++pageMatched;
            }
            ++pageScanned; ++scanned;
            next = row.value(QStringLiteral("rowId")).toString();
            if (row.value(QStringLiteral("uncleanRunMayHaveLostTail")).toBool()) ++unclean;
            if (row.value(QStringLiteral("runDroppedCount")).toString() != QStringLiteral("0")) ++dropped;
            if (row.value(QStringLiteral("runDiscardedCount")).toString() != QStringLiteral("0")) ++discarded;
        }
        const bool hasMore = byteBoundary || page.value(QStringLiteral("hasMore")).toBool();
        QVariantMap summary = identity;
        for (const QString& key : {QStringLiteral("retainedFromRowId"), QStringLiteral("retainedThroughRowId"),
            QStringLiteral("retiredCount"), QStringLiteral("cursorPrecedesRetainedHistory")}) summary.insert(key, page.value(key));
        summary.insert(QStringLiteral("afterRowId"), after); summary.insert(QStringLiteral("nextRowId"), next);
        summary.insert(QStringLiteral("hasMore"), hasMore); summary.insert(QStringLiteral("scannedRows"), pageScanned);
        summary.insert(QStringLiteral("matchedRows"), pageMatched); summary.insert(QStringLiteral("uncleanRows"), unclean);
        summary.insert(QStringLiteral("rowsWithDroppedEvents"), dropped); summary.insert(QStringLiteral("rowsWithDiscardedEvents"), discarded);
        summaries.append(summary);
        if (hasMore) {
            after = next;
            expectedHash = after == QStringLiteral("0") ? QStringLiteral("") : actualHash;
            expectedEpoch = after == QStringLiteral("0") ? QStringLiteral("") : epoch;
        } else { ++index; after = QStringLiteral("0"); expectedHash = QStringLiteral(""); expectedEpoch = QStringLiteral(""); }
    }
    QList<Segment> afterFiles;
    QString afterInventory;
    if (!searchInventory(directory, policy, scope, cancelled, &afterFiles, &afterInventory, error)) return false;
    if (inventory != afterInventory || !sameSegments(files, afterFiles)) {
        *error = QStringLiteral("archive-search-inventory-changed-restart-required"); return false;
    }
    const bool complete = index == files.size();
    const QVariantMap nextCursor = complete ? QVariantMap{} : QVariantMap{
        {QStringLiteral("inventoryFingerprint"), inventory}, {QStringLiteral("fileIndex"), index},
        {QStringLiteral("afterRowId"), after}, {QStringLiteral("fileFingerprint"), expectedHash}, {QStringLiteral("databaseEpoch"), expectedEpoch}};
    QVariantMap value{{QStringLiteral("format"), QStringLiteral("merivus-fleet-event-history-search")},
        {QStringLiteral("schemaVersion"), request.contains(QStringLiteral("taskIdHash")) ? 2 : 1}, {QStringLiteral("scope"), scope},
        {QStringLiteral("vehicleId"), request.value(QStringLiteral("vehicleId"))}, {QStringLiteral("operation"), request.value(QStringLiteral("operation"))},
        {QStringLiteral("inventoryFingerprint"), inventory}, {QStringLiteral("filesTotal"), files.size()},
        {QStringLiteral("filesCompleted"), index}, {QStringLiteral("scannedRows"), scanned},
        {QStringLiteral("matchedRows"), matches.size()}, {QStringLiteral("scanComplete"), complete},
        {QStringLiteral("nextCursor"), nextCursor}, {QStringLiteral("rows"), matches}, {QStringLiteral("scannedPages"), summaries},
        {QStringLiteral("ordering"), QStringLiteral("basename-then-row-not-flight-time")},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("signatureVerified"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("completionVerified"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("restoresAuthorization"), false},
        {QStringLiteral("historicalCompletenessVerified"), false}, {QStringLiteral("crossSegmentAtomicSnapshot"), false},
        {QStringLiteral("wallClockOrderingUsed"), false}};
    if (request.contains(QStringLiteral("taskIdHash")))
        value.insert(QStringLiteral("taskIdHash"), request.value(QStringLiteral("taskIdHash")));
    value.insert(QStringLiteral("resultBytesWithoutThisField"), QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size());
    const qint64 facadeReserve = request.contains(QStringLiteral("taskIdHash"))
        ? kTaskSearchFacadeMetadataReserve : kFacadeMetadataReserve;
    if (QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size() > maximumResult - facadeReserve) {
        *error = QStringLiteral("archive-search-result-byte-limit"); return false;
    }
    *result = value;
    return !cancelled.load();
}

bool readIndexIdentity(const QString& directory, const QVariantMap& policy, const Segment& file, int index,
    const QVariantMap& expected, const std::atomic_bool& cancelled, QVariantMap* result, QString* error)
{
    const bool offline = file.basename == kCurrentBasename;
    // A verifier binds the old hash/epoch even at row zero. A first full
    // snapshot has no such prior identity and freezes the actual values.
    const QVariantMap probe = offline ? QVariantMap{
        {QStringLiteral("kind"), QStringLiteral("offline")}, {QStringLiteral("runFilter"), QStringLiteral("")},
        {QStringLiteral("databaseEpoch"), expected.isEmpty() ? QVariant(QStringLiteral("")) : expected.value(QStringLiteral("databaseEpoch"))},
        {QStringLiteral("fileFingerprint"), expected.isEmpty() ? QVariant(QStringLiteral("")) : expected.value(QStringLiteral("fileFingerprint"))},
        {QStringLiteral("afterRowId"), QStringLiteral("0")}, {QStringLiteral("limit"), 1}}
        : QVariantMap{{QStringLiteral("kind"), QStringLiteral("catalog")}};
    QVariantMap observedPage;
    Fingerprint observed;
    const bool read = offline
        ? readOfflineCurrent(directory, policy, probe, cancelled, &observedPage, error, &observed, &file)
        : readSegment(directory, file, policy, probe, cancelled, &observedPage, error, &observed);
    if (!read || cancelled.load()) {
        if (error->isEmpty()) *error = cancelled.load() ? QStringLiteral("archive-task-index-cancelled")
            : QStringLiteral("archive-task-index-source-content-or-sql-identity-changed");
        return false;
    }
    *result = searchIdentity(file, index, observedPage, observed);
    return true;
}
} // namespace

bool FleetEventArchiveHistoryReader::searchRequestValid(const QVariantMap& request)
{
    const bool taskFiltered = request.contains(QStringLiteral("taskIdHash"));
    if (request.size() != (taskFiltered ? 6 : 5) || !textValue(request.value(QStringLiteral("kind")))
        || request.value(QStringLiteral("kind")).toString() != QStringLiteral("search")
        || !textValue(request.value(QStringLiteral("scope"))) || !textValue(request.value(QStringLiteral("operation")))
        || !intValue(request.value(QStringLiteral("vehicleId")), 0, 255)
        || !searchCursorValid(request.value(QStringLiteral("cursor")), true)) return false;
    if (taskFiltered && (!textValue(request.value(QStringLiteral("taskIdHash")))
        || !lowerSha256(request.value(QStringLiteral("taskIdHash")).toString()))) return false;
    const QString scope = request.value(QStringLiteral("scope")).toString();
    const QStringList operations{QStringLiteral(""), QStringLiteral("approve"), QStringLiteral("revoke"),
        QStringLiteral("approve-baseline"), QStringLiteral("revoke-baseline"), QStringLiteral("approve-performance")};
    return (scope == QStringLiteral("sealed") || scope == QStringLiteral("sealed-and-offline"))
        && operations.contains(request.value(QStringLiteral("operation")).toString());
}

bool FleetEventArchiveHistoryReader::searchInWorker(const QString& directory, const QVariantMap& policy,
    const QVariantMap& request, const std::atomic_bool& cancelled, QVariantMap* result, QString* error)
{
    // Freeze before clearing outputs: native callers may alias a value input.
    const QString fixedDirectory = directory;
    const QVariantMap fixedPolicy = policy, fixedRequest = request;
    if (result) result->clear();
    if (error) *error = QStringLiteral("");
    if (!result || !error) return false;
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application || QThread::currentThread() == application->thread()) {
        *error = QStringLiteral("archive-task-index-worker-thread-required"); return false;
    }
    if (!policyValid(fixedPolicy) || !searchRequestValid(fixedRequest)) {
        *error = QStringLiteral("archive-search-configuration-rejected"); return false;
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-search-history-cancelled"); return false; }
    if (!localDirectory(fixedDirectory)) { *error = QStringLiteral("archive-search-directory-rejected"); return false; }
    QVariantMap candidate;
    if (!readSearch(fixedDirectory, fixedPolicy, fixedRequest, cancelled, &candidate, error)) {
        if (error->isEmpty()) *error = cancelled.load() ? QStringLiteral("archive-search-history-cancelled")
            : QStringLiteral("archive-search-history-rejected");
        return false;
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-search-history-cancelled"); return false; }
    if (!searchResultMatches(candidate, fixedRequest, fixedPolicy)) {
        *error = QStringLiteral("archive-search-result-contract-rejected"); return false;
    }
    *result = candidate;
    return true;
}

bool FleetEventArchiveHistoryReader::snapshotIndexSourcesInWorker(const QString& directory, const QVariantMap& policy,
    const QString& scope, const QString& inventoryFingerprint, const std::atomic_bool& cancelled,
    QVariantList* fileIdentities, QString* error)
{
    const QString fixedDirectory = directory, fixedScope = scope, expectedInventory = inventoryFingerprint;
    const QVariantMap fixedPolicy = policy;
    if (fileIdentities) fileIdentities->clear();
    if (error) *error = QStringLiteral("");
    if (!fileIdentities || !error) return false;
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application || QThread::currentThread() == application->thread()) {
        *error = QStringLiteral("archive-task-index-worker-thread-required"); return false;
    }
    if (!policyValid(fixedPolicy) || (fixedScope != QStringLiteral("sealed") && fixedScope != QStringLiteral("sealed-and-offline"))
        || !lowerSha256(expectedInventory)) {
        *error = QStringLiteral("archive-task-index-source-contract-rejected"); return false;
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
    QList<Segment> files;
    QString actualInventory;
    if (!localDirectory(fixedDirectory)
        || !searchInventory(fixedDirectory, fixedPolicy, fixedScope, cancelled, &files, &actualInventory, error)) {
        if (error->isEmpty()) *error = QStringLiteral("archive-task-index-source-inventory-rejected");
        return false;
    }
    if (actualInventory != expectedInventory) {
        *error = QStringLiteral("archive-task-index-source-inventory-changed"); return false;
    }
    QVariantList candidate;
    for (int index = 0; index < files.size(); ++index) {
        if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
        QVariantMap identity;
        if (!readIndexIdentity(fixedDirectory, fixedPolicy, files.at(index), index, QVariantMap{}, cancelled, &identity, error)) return false;
        candidate.append(identity);
    }
    QList<Segment> afterFiles;
    QString afterInventory;
    if (!searchInventory(fixedDirectory, fixedPolicy, fixedScope, cancelled, &afterFiles, &afterInventory, error)) {
        if (error->isEmpty()) *error = QStringLiteral("archive-task-index-source-inventory-rejected");
        return false;
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
    if (afterInventory != expectedInventory || !sameSegments(files, afterFiles)) {
        *error = QStringLiteral("archive-task-index-source-inventory-changed"); return false;
    }
    *fileIdentities = candidate;
    return true;
}

bool FleetEventArchiveHistoryReader::verifyIndexSourcesInWorker(const QString& directory, const QVariantMap& policy,
    const QString& scope, const QString& inventoryFingerprint, const QVariantList& fileIdentities,
    const std::atomic_bool& cancelled, QString* error)
{
    const QString fixedDirectory = directory, fixedScope = scope, expectedInventory = inventoryFingerprint;
    const QVariantMap fixedPolicy = policy;
    const QVariantList identities = fileIdentities;
    if (!error) return false;
    *error = QStringLiteral("");
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application || QThread::currentThread() == application->thread()) {
        *error = QStringLiteral("archive-task-index-worker-thread-required"); return false;
    }
    if (!policyValid(fixedPolicy) || (fixedScope != QStringLiteral("sealed") && fixedScope != QStringLiteral("sealed-and-offline"))
        || !lowerSha256(expectedInventory) || identities.size() > 129) {
        *error = QStringLiteral("archive-task-index-source-contract-rejected"); return false;
    }
    QSet<int> indices;
    for (const QVariant& value : identities) {
        if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
        if (value.type() != QVariant::Map || value.isNull()) {
            *error = QStringLiteral("archive-task-index-source-contract-rejected"); return false;
        }
        const QVariantMap identity = value.toMap();
        if (identity.size() != 6 || !intValue(identity.value(QStringLiteral("fileIndex")), 0, 128)
            || indices.contains(identity.value(QStringLiteral("fileIndex")).toInt())
            || !textValue(identity.value(QStringLiteral("basename")))
            || !textValue(identity.value(QStringLiteral("databaseEpoch")))
            || !textValue(identity.value(QStringLiteral("sealRunId")))
            || !textValue(identity.value(QStringLiteral("fileFingerprint")))
            || !uuid(identity.value(QStringLiteral("databaseEpoch")).toString())
            || !lowerSha256(identity.value(QStringLiteral("fileFingerprint")).toString())
            || identity.value(QStringLiteral("offlineCurrentOnly")).type() != QVariant::Bool
            || identity.value(QStringLiteral("offlineCurrentOnly")).isNull()) {
            *error = QStringLiteral("archive-task-index-source-contract-rejected"); return false;
        }
        indices.insert(identity.value(QStringLiteral("fileIndex")).toInt());
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
    QList<Segment> files;
    QString actualInventory;
    if (!localDirectory(fixedDirectory)
        || !searchInventory(fixedDirectory, fixedPolicy, fixedScope, cancelled, &files, &actualInventory, error)) {
        if (error->isEmpty()) *error = QStringLiteral("archive-task-index-source-inventory-rejected");
        return false;
    }
    if (actualInventory != expectedInventory || identities.size() != files.size()) {
        *error = QStringLiteral("archive-task-index-source-inventory-changed"); return false;
    }
    for (const QVariant& value : identities) {
        if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
        const QVariantMap identity = value.toMap();
        const int index = identity.value(QStringLiteral("fileIndex")).toInt();
        if (index >= files.size()) { *error = QStringLiteral("archive-task-index-source-file-changed"); return false; }
        const Segment& file = files.at(index);
        const bool offline = file.basename == kCurrentBasename;
        if (!flagValue(identity.value(QStringLiteral("offlineCurrentOnly")), offline)
            || identity.value(QStringLiteral("basename")).toString() != file.basename
            || (offline ? !identity.value(QStringLiteral("sealRunId")).toString().isEmpty()
                : identity.value(QStringLiteral("databaseEpoch")).toString() != file.databaseEpoch
                    || identity.value(QStringLiteral("sealRunId")).toString() != file.sealRunId)) {
            *error = QStringLiteral("archive-task-index-source-file-changed"); return false;
        }
        QVariantMap observedIdentity;
        if (!readIndexIdentity(fixedDirectory, fixedPolicy, file, index, identity, cancelled, &observedIdentity, error)
            || observedIdentity != identity) {
            if (error->isEmpty()) *error = QStringLiteral("archive-task-index-source-content-or-sql-identity-changed");
            return false;
        }
    }
    QList<Segment> afterFiles;
    QString afterInventory;
    if (!searchInventory(fixedDirectory, fixedPolicy, fixedScope, cancelled, &afterFiles, &afterInventory, error)) {
        if (error->isEmpty()) *error = QStringLiteral("archive-task-index-source-inventory-rejected");
        return false;
    }
    if (cancelled.load()) { *error = QStringLiteral("archive-task-index-cancelled"); return false; }
    if (afterInventory != expectedInventory || !sameSegments(files, afterFiles)) {
        *error = QStringLiteral("archive-task-index-source-inventory-changed"); return false;
    }
    return true;
}

bool FleetEventArchiveHistoryReader::searchResultMatches(const QVariantMap& result, const QVariantMap& request,
                                                        const QVariantMap& policy)
{
    const bool taskFiltered = request.contains(QStringLiteral("taskIdHash"));
    const int schemaVersion = taskFiltered ? 2 : 1;
    if (!searchRequestValid(request) || !policyValid(policy) || result.size() != (taskFiltered ? 29 : 28)
        || result.contains(QStringLiteral("taskIdHash")) != taskFiltered) return false;
    if (taskFiltered && (!textValue(result.value(QStringLiteral("taskIdHash")))
        || result.value(QStringLiteral("taskIdHash")) != request.value(QStringLiteral("taskIdHash")))) return false;
    for (const QString& key : {QStringLiteral("format"), QStringLiteral("scope"), QStringLiteral("operation"),
        QStringLiteral("inventoryFingerprint"), QStringLiteral("ordering")})
        if (!textValue(result.value(key))) return false;
    if (result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-event-history-search")
        || result.value(QStringLiteral("scope")) != request.value(QStringLiteral("scope"))
        || result.value(QStringLiteral("operation")) != request.value(QStringLiteral("operation"))
        || !intValue(result.value(QStringLiteral("vehicleId")), 0, 255)
        || result.value(QStringLiteral("vehicleId")) != request.value(QStringLiteral("vehicleId"))
        || !intValue(result.value(QStringLiteral("schemaVersion")), schemaVersion, schemaVersion)
        || result.value(QStringLiteral("ordering")).toString() != QStringLiteral("basename-then-row-not-flight-time")
        || !lowerSha256(result.value(QStringLiteral("inventoryFingerprint")).toString())
        || !flagValue(result.value(QStringLiteral("historicalOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("permissionsRestored"), QStringLiteral("authenticated"),
        QStringLiteral("signatureVerified"), QStringLiteral("executable"), QStringLiteral("completionVerified"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("restoresAuthorization"),
        QStringLiteral("historicalCompletenessVerified"), QStringLiteral("crossSegmentAtomicSnapshot"), QStringLiteral("wallClockOrderingUsed")})
        if (!flagValue(result.value(key), false)) return false;
    const bool includesCurrent = request.value(QStringLiteral("scope")).toString() == QStringLiteral("sealed-and-offline");
    if (!intValue(result.value(QStringLiteral("filesTotal")), includesCurrent ? 1 : 0, includesCurrent ? 129 : 128)
        || !intValue(result.value(QStringLiteral("filesCompleted")), 0, result.value(QStringLiteral("filesTotal")).toInt())
        || !intValue(result.value(QStringLiteral("scannedRows")), 0, 256)
        || !intValue(result.value(QStringLiteral("matchedRows")), 0, 64)
        || result.value(QStringLiteral("scanComplete")).type() != QVariant::Bool || result.value(QStringLiteral("scanComplete")).isNull()
        || result.value(QStringLiteral("rows")).type() != QVariant::List || result.value(QStringLiteral("rows")).isNull()
        || result.value(QStringLiteral("scannedPages")).type() != QVariant::List || result.value(QStringLiteral("scannedPages")).isNull()
        || !searchCursorValid(result.value(QStringLiteral("nextCursor")), true)) return false;
    const QVariantList rows = result.value(QStringLiteral("rows")).toList();
    const QVariantList summaries = result.value(QStringLiteral("scannedPages")).toList();
    if (rows.size() != result.value(QStringLiteral("matchedRows")).toInt() || summaries.size() > 4) return false;
    const QString inventory = result.value(QStringLiteral("inventoryFingerprint")).toString();
    const QVariantMap original = request.value(QStringLiteral("cursor")).toMap();
    int index = original.isEmpty() ? 0 : original.value(QStringLiteral("fileIndex")).toInt();
    QString after = original.isEmpty() ? QStringLiteral("0") : original.value(QStringLiteral("afterRowId")).toString();
    QString activeHash = original.isEmpty() ? QStringLiteral("") : original.value(QStringLiteral("fileFingerprint")).toString();
    QString activeEpoch = original.isEmpty() ? QStringLiteral("") : original.value(QStringLiteral("databaseEpoch")).toString();
    const int total = result.value(QStringLiteral("filesTotal")).toInt();
    if (!original.isEmpty() && (original.value(QStringLiteral("inventoryFingerprint")).toString() != inventory || index >= total)) return false;
    const QStringList identityKeys{QStringLiteral("fileIndex"), QStringLiteral("basename"), QStringLiteral("databaseEpoch"),
        QStringLiteral("sealRunId"), QStringLiteral("fileFingerprint"), QStringLiteral("offlineCurrentOnly")};
    int rowOffset = 0, scanned = 0;
    for (const QVariant& item : summaries) {
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap summary = item.toMap();
        if (summary.size() != 18 || index >= total || !intValue(summary.value(QStringLiteral("fileIndex")), index, index)) return false;
        for (const QString& key : {QStringLiteral("basename"), QStringLiteral("databaseEpoch"), QStringLiteral("sealRunId"),
            QStringLiteral("fileFingerprint"), QStringLiteral("afterRowId"), QStringLiteral("nextRowId"),
            QStringLiteral("retainedFromRowId"), QStringLiteral("retainedThroughRowId"), QStringLiteral("retiredCount")})
            if (!textValue(summary.value(key))) return false;
        const bool offline = includesCurrent && index == total - 1;
        const QString epoch = summary.value(QStringLiteral("databaseEpoch")).toString();
        const QString hash = summary.value(QStringLiteral("fileFingerprint")).toString();
        if (!flagValue(summary.value(QStringLiteral("offlineCurrentOnly")), offline) || !uuid(epoch) || !lowerSha256(hash)
            || summary.value(QStringLiteral("afterRowId")).toString() != after
            || (after != QStringLiteral("0") && (activeHash != hash || activeEpoch != epoch))) return false;
        if (offline) {
            if (summary.value(QStringLiteral("basename")).toString() != kCurrentBasename
                || !summary.value(QStringLiteral("sealRunId")).toString().isEmpty()) return false;
        } else {
            Segment segment;
            if (!parseBasename(summary.value(QStringLiteral("basename")).toString(), &segment)
                || segment.databaseEpoch != epoch || segment.sealRunId != summary.value(QStringLiteral("sealRunId")).toString()) return false;
        }
        qint64 cursor = 0, next = 0, first = 0, last = 0;
        if (!decimal(after, &cursor) || !decimal(summary.value(QStringLiteral("nextRowId")).toString(), &next)
            || !decimal(summary.value(QStringLiteral("retainedFromRowId")).toString(), &first)
            || !decimal(summary.value(QStringLiteral("retainedThroughRowId")).toString(), &last)
            || !decimal(summary.value(QStringLiteral("retiredCount")).toString())
            || next < cursor || first > last || (first == 0 && last != 0)
            || !flagValue(summary.value(QStringLiteral("cursorPrecedesRetainedHistory")), cursor > 0 && first > 0 && cursor < first - 1)
            || summary.value(QStringLiteral("hasMore")).type() != QVariant::Bool || summary.value(QStringLiteral("hasMore")).isNull()
            || !intValue(summary.value(QStringLiteral("scannedRows")), 0, qMin(64, 64 - rowOffset))) return false;
        const int count = summary.value(QStringLiteral("scannedRows")).toInt();
        if (!intValue(summary.value(QStringLiteral("matchedRows")), 0, count)
            || (count == 0 ? next != cursor : next <= cursor || next > last)) return false;
        for (const QString& key : {QStringLiteral("uncleanRows"), QStringLiteral("rowsWithDroppedEvents"), QStringLiteral("rowsWithDiscardedEvents")})
            if (!intValue(summary.value(key), 0, count)) return false;
        const int matched = summary.value(QStringLiteral("matchedRows")).toInt();
        if (rowOffset + matched > rows.size()) return false;
        qint64 previousMatch = cursor;
        for (int offset = 0; offset < matched; ++offset) {
            const QVariant wrapped = rows.at(rowOffset + offset);
            if (wrapped.type() != QVariant::Map || wrapped.isNull()) return false;
            const QVariantMap match = wrapped.toMap();
            if (match.size() != 7 || match.value(QStringLiteral("row")).type() != QVariant::Map || match.value(QStringLiteral("row")).isNull()) return false;
            for (const QString& key : identityKeys) if (match.value(key).type() != summary.value(key).type() || match.value(key) != summary.value(key)) return false;
            const QVariantMap row = match.value(QStringLiteral("row")).toMap();
            qint64 rowId = 0;
            if (row.size() != 8 || !textValue(row.value(QStringLiteral("rowId"))) || !decimal(row.value(QStringLiteral("rowId")).toString(), &rowId)
                || rowId <= previousMatch || rowId > next || row.value(QStringLiteral("record")).type() != QVariant::Map
                || row.value(QStringLiteral("record")).isNull() || !textValue(row.value(QStringLiteral("contentHash")))
                || !lowerSha256(row.value(QStringLiteral("contentHash")).toString()) || !rowMatchesSearch(row, request)) return false;
            if (row.value(QStringLiteral("runCleanShutdown")).type() != QVariant::Bool || row.value(QStringLiteral("runCleanShutdown")).isNull()
                || !flagValue(row.value(QStringLiteral("uncleanRunMayHaveLostTail")), !row.value(QStringLiteral("runCleanShutdown")).toBool())) return false;
            for (const QString& key : {QStringLiteral("runDurableSequence"), QStringLiteral("runDroppedCount"), QStringLiteral("runDiscardedCount")})
                if (!textValue(row.value(key)) || !decimal(row.value(key).toString())) return false;
            previousMatch = rowId;
        }
        rowOffset += matched; scanned += count;
        if (summary.value(QStringLiteral("hasMore")).toBool()) {
            after = QString::number(next);
            activeHash = next == 0 ? QStringLiteral("") : hash;
            activeEpoch = next == 0 ? QStringLiteral("") : epoch;
        } else { ++index; after = QStringLiteral("0"); activeHash = QStringLiteral(""); activeEpoch = QStringLiteral(""); }
    }
    if (rowOffset != rows.size() || scanned != result.value(QStringLiteral("scannedRows")).toInt()
        || index != result.value(QStringLiteral("filesCompleted")).toInt()) return false;
    const bool complete = index == total;
    if (!flagValue(result.value(QStringLiteral("scanComplete")), complete)) return false;
    const QVariantMap nextCursor = result.value(QStringLiteral("nextCursor")).toMap();
    if (complete) { if (!nextCursor.isEmpty()) return false; }
    else {
        const QVariantMap expected{{QStringLiteral("inventoryFingerprint"), inventory}, {QStringLiteral("fileIndex"), index},
            {QStringLiteral("afterRowId"), after}, {QStringLiteral("fileFingerprint"), activeHash}, {QStringLiteral("databaseEpoch"), activeEpoch}};
        if (summaries.isEmpty() || nextCursor != expected || (!original.isEmpty() && nextCursor == original)
            || (original.isEmpty() && index == 0 && after == QStringLiteral("0"))) return false;
    }
    QVariantMap withoutBytes = result;
    withoutBytes.remove(QStringLiteral("resultBytesWithoutThisField"));
    const int actualBytes = QJsonDocument(QJsonObject::fromVariantMap(withoutBytes)).toJson(QJsonDocument::Compact).size();
    const qint64 facadeReserve = taskFiltered ? kTaskSearchFacadeMetadataReserve : kFacadeMetadataReserve;
    return intValue(result.value(QStringLiteral("resultBytesWithoutThisField")), actualBytes, actualBytes)
        && QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size()
            <= policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB - facadeReserve;
}

FleetEventArchiveHistoryReader::FleetEventArchiveHistoryReader(const QString& canonicalAbsoluteDirectory,
                                                             const QVariantMap& exactPolicy,
                                                             const QString& queryId,
                                                             const QVariantMap& exactRequest)
    : QThread(nullptr), _directory(QDir::fromNativeSeparators(canonicalAbsoluteDirectory)),
      _policy(exactPolicy), _queryId(identifier(queryId) ? queryId : QString()), _request(exactRequest),
      _configurationValid(policyValid(exactPolicy) && identifier(queryId) && requestValid(exactRequest))
{
    _status = {{QStringLiteral("state"), QStringLiteral("created")}, {QStringLiteral("errorCode"), QString()},
        {QStringLiteral("queryId"), _queryId}, {QStringLiteral("cancelRequested"), false},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
        {QStringLiteral("restoresAuthorization"), false}};
}

FleetEventArchiveHistoryReader::~FleetEventArchiveHistoryReader()
{
    Q_ASSERT(!isRunning());
}

void FleetEventArchiveHistoryReader::requestCancel()
{
    _cancelled.store(true);
    QMutexLocker locker(&_mutex);
    _status.insert(QStringLiteral("cancelRequested"), true);
}

QVariantMap FleetEventArchiveHistoryReader::current() const
{
    QMutexLocker locker(&_mutex);
    return _status;
}

void FleetEventArchiveHistoryReader::setTerminal(const QString& state, const QString& fixedCode)
{
    QMutexLocker locker(&_mutex);
    _status.insert(QStringLiteral("state"), state);
    _status.insert(QStringLiteral("errorCode"), fixedCode);
}

void FleetEventArchiveHistoryReader::run()
{
    setTerminal(QStringLiteral("reading"));
    QString error;
    QVariantMap result;
    const bool offline = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("offline");
    const bool search = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("search");
    const bool taskIndex = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("task-index");
    bool taskIndexExecuted = false;
    const QString prefix = taskIndex ? QStringLiteral("archive-task-index-") : search ? QStringLiteral("archive-search-")
        : offline ? QStringLiteral("archive-offline-") : QStringLiteral("archive-sealed-");
    if (!_configurationValid) error = prefix + QStringLiteral("configuration-rejected");
    else if (!localDirectory(_directory)) error = prefix + QStringLiteral("directory-rejected");
    else if (_cancelled.load()) error = prefix + QStringLiteral("history-cancelled");
    else if (taskIndex) {
        taskIndexExecuted = FleetArchiveTaskIndex::execute(_directory, _policy, _request, _cancelled, &result, &error);
        if (!taskIndexExecuted && error.isEmpty())
            error = QStringLiteral("archive-task-index-rejected");
        if (error.isEmpty() && !FleetArchiveTaskIndex::resultMatches(result, _request, _policy))
            error = QStringLiteral("archive-task-index-result-contract-rejected");
    } else if (search) {
        if (!readSearch(_directory, _policy, _request, _cancelled, &result, &error) && error.isEmpty()) {
            error = QStringLiteral("archive-search-history-rejected");
        }
        if (error.isEmpty() && !searchResultMatches(result, _request, _policy)) error = QStringLiteral("archive-search-result-contract-rejected");
    } else if (offline) {
        if (!readOfflineCurrent(_directory, _policy, _request, _cancelled, &result, &error) && error.isEmpty()) {
            error = QStringLiteral("archive-offline-history-rejected");
        }
    } else if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("catalog")) {
        QList<Segment> before;
        QString beforeDirectoryIdentity;
        const qint64 maximumBytes = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
        if (!scanSegments(_directory, maximumBytes, _cancelled, &before, &beforeDirectoryIdentity, &error) && error.isEmpty()) {
            error = QStringLiteral("archive-sealed-directory-rejected");
        }
        QVariantList entries;
        for (const Segment& segment : before) {
            if (!error.isEmpty()) break;
            QVariantMap entry;
            if (_cancelled.load() || !readSegment(_directory, segment, _policy, _request, _cancelled, &entry, &error)) {
                if (error.isEmpty()) error = QStringLiteral("archive-sealed-history-rejected");
                break;
            }
            entries.append(entry);
        }
        QList<Segment> after;
        QString afterDirectoryIdentity;
        if (error.isEmpty() && (!scanSegments(_directory, maximumBytes, _cancelled, &after, &afterDirectoryIdentity, &error)
            || beforeDirectoryIdentity != afterDirectoryIdentity || !sameSegments(before, after))) {
            if (error.isEmpty()) error = QStringLiteral("archive-sealed-catalog-changed");
        }
        if (error.isEmpty()) {
            result = {{QStringLiteral("format"), QStringLiteral("merivus-fleet-event-archive-catalog")},
                {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("entries"), entries},
                {QStringLiteral("segmentCount"), entries.size()}, {QStringLiteral("maximumSegments"), kMaximumSegments},
                {QStringLiteral("directoryScanLimit"), kMaximumDirectoryEntries},
                {QStringLiteral("directoryInventoryEnumerationCompleted"), true},
                {QStringLiteral("ordering"), QStringLiteral("basename-not-flight-time")},
                {QStringLiteral("wallClockOrderingUsed"), false}, {QStringLiteral("globalRowIdAvailable"), false},
                {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
                {QStringLiteral("signatureVerified"), false}, {QStringLiteral("permissionsRestored"), false},
                {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
                {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
                {QStringLiteral("restoresAuthorization"), false}, {QStringLiteral("allRecordHashesVerified"), false},
                {QStringLiteral("catalogCompletenessVerified"), false},
                {QStringLiteral("crossSegmentAtomicSnapshot"), false}};
        }
    } else {
        Segment segment;
        if (!parseBasename(_request.value(QStringLiteral("basename")).toString(), &segment)
            || !readSegment(_directory, segment, _policy, _request, _cancelled, &result, &error)) {
            if (error.isEmpty()) error = QStringLiteral("archive-sealed-history-rejected");
        }
    }
    // Task-index may already have committed its checkpoint when cancellation
    // is first observed here. Only that actual fact survives cancellation;
    // an uncommitted open/page operation still follows the old cancel rule.
    if (_cancelled.load()) {
        if (taskIndex && taskIndexExecuted && error.isEmpty()
            && flagValue(result.value(QStringLiteral("checkpointCommitted")), true)) {
            result.insert(QStringLiteral("cancelObservedAfterCommit"), true);
            if (!FleetArchiveTaskIndex::resultMatches(result, _request, _policy))
                error = QStringLiteral("archive-task-index-result-contract-rejected");
        } else error = prefix + QStringLiteral("history-cancelled");
    }
    if (error.isEmpty() && QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size()
        > _policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB - kFacadeMetadataReserve) {
        error = prefix + QStringLiteral("result-byte-limit");
    }
    if (!error.isEmpty()) {
        setTerminal(_cancelled.load() ? QStringLiteral("cancelled") : QStringLiteral("failed"), error);
        emit failure(_queryId, error);
        return;
    }
    setTerminal(QStringLiteral("result-submitted"));
    // One bounded result only. Root keeps its single history slot occupied
    // until both this result reaches the GUI and actual finished is delivered.
    emit resultReady(_queryId, result);
}

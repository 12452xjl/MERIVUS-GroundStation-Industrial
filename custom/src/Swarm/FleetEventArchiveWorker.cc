#include "FleetEventArchiveWorker.h"
#include "FleetArchiveDirectoryInventory.h"
#include "FleetEventArchiveSql.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariantList>
#include <QtEndian>

#include <limits>

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr int kMaximumQueries = 8;

bool uuid(const QString& value)
{
    return !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces).toLower() == value;
}

bool decimal(const QString& value, qint64* number)
{
    if (value.isEmpty() || value.size() > 19 || (value.size() > 1 && value.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : value) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = value.toLongLong(&ok, 10);
    if (!ok || parsed < 0 || QString::number(parsed) != value) return false;
    if (number) *number = parsed;
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
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong:
        break;
    default: return false;
    }
    bool ok = false;
    const qint64 number = value.toLongLong(&ok);
    return ok && number >= minimum && number <= maximum;
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

qint64 recordCharge(const FleetEventArchiveCodec::Record& record)
{
    // Conservative charge includes the retained Record strings and queue node;
    // shared Qt allocations do not make a pending value free of its byte cap.
    return record.canonicalJson.size() + 1024LL + 2LL * (record.runId.size()
        + record.sequence.size() + record.contentHash.size() + record.eventType.size()
        + record.eventSource.size() + record.errorCode.size());
}

bool pragmaEquals(QSqlDatabase& database, const QString& sql, const QString& expected)
{
    QSqlQuery query(database);
    return query.exec(sql) && query.next() && query.value(0).toString().toLower() == expected
        && !query.next() && !query.lastError().isValid();
}

bool localDirectory(const QString& directory)
{
    const QString normalized = QDir::fromNativeSeparators(directory);
    const QFileInfo info(normalized);
    return !normalized.isEmpty() && QDir::isAbsolutePath(normalized)
        && !normalized.startsWith(QStringLiteral("//")) && !normalized.contains(QChar::Null)
        && QDir::cleanPath(normalized) == normalized && info.exists() && info.isDir()
        && !info.isSymLink() && QDir::fromNativeSeparators(info.canonicalFilePath()) == normalized;
}

bool safeFile(const QString& filePath, qint64 maximumBytes)
{
    const QFileInfo info(filePath);
    if (info.isSymLink() || (info.exists() && (!info.isFile() || info.size() > maximumBytes))) return false;
    if (info.exists() && QDir::fromNativeSeparators(info.canonicalFilePath()) != QDir::fromNativeSeparators(filePath)) return false;
    for (const QString& suffix : {QStringLiteral("-journal"), QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        const QFileInfo sidecar(filePath + suffix);
        if (sidecar.isSymLink() || (sidecar.exists() && !sidecar.isFile())) return false;
    }
    return true;
}

bool sidecarsAbsent(const QString& path)
{
    for (const QString& suffix : {QStringLiteral("-journal"), QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        const QFileInfo info(path + suffix);
        if (info.exists() || info.isSymLink()) return false;
    }
    return true;
}

struct ArchiveFileSnapshot {
    QByteArray header;
    QByteArray sha256;
    qint64 bytes = 0;
    bool valid = false;
};

bool sealIdentityMatches(QSqlDatabase& database, const QString& expectedEpoch, const QString& runId,
                         qint64 durable, qint64 dropped, qint64 discarded)
{
    QVariantMap actual;
    bool valid = FleetEventArchiveSql::inspect(database, &actual) && actual.value(QStringLiteral("databaseEpoch")).toString() == expectedEpoch;
    QSqlQuery run(database);
    if (valid) valid = run.prepare(QStringLiteral(
        "SELECT CASE WHEN typeof(clean_shutdown)='integer' THEN clean_shutdown END, "
        "CASE WHEN typeof(last_sequence)='integer' THEN last_sequence END, "
        "CASE WHEN typeof(dropped_count)='integer' THEN dropped_count END, "
        "CASE WHEN typeof(discarded_count)='integer' THEN discarded_count END FROM runs WHERE run_id=?"));
    if (valid) {
        run.addBindValue(runId);
        valid = run.exec() && run.next() && integerIn(run.value(0), 1, 1)
            && integerIn(run.value(1), 0, std::numeric_limits<qint64>::max()) && run.value(1).toLongLong() == durable
            && integerIn(run.value(2), 0, std::numeric_limits<qint64>::max()) && run.value(2).toLongLong() == dropped
            && integerIn(run.value(3), 0, std::numeric_limits<qint64>::max()) && run.value(3).toLongLong() == discarded
            && !run.next() && !run.lastError().isValid();
    }
    run.finish();
    return valid;
}

bool beginSealReadBarrier(QSqlDatabase& database, const QString& expectedEpoch, const QString& runId,
                          qint64 durable, qint64 dropped, qint64 discarded, QString* error)
{
    QSqlQuery barrier(database);
    if (!barrier.exec(QStringLiteral("BEGIN EXCLUSIVE"))) {
        *error = QStringLiteral("archive-seal-read-barrier-unavailable"); return false;
    }
    barrier.finish();
    const bool valid = sealIdentityMatches(database, expectedEpoch, runId, durable, dropped, discarded);
    if (!valid) {
        database.rollback(); *error = QStringLiteral("archive-seal-final-identity-rejected"); return false;
    }
    // Caller owns this read-only exclusive barrier until its bounded preclose
    // fingerprint finishes. Release it before closing the connection.
    return true;
}

bool probeCurrentSealIdentity(const QString& path, const QString& expectedEpoch, const QString& runId,
                              qint64 durable, qint64 dropped, qint64 discarded, int busyTimeout,
                              qint64 maximumBytes)
{
    if (!safeFile(path, maximumBytes) || !QFileInfo::exists(path) || !sidecarsAbsent(path)) return false;
    const QString connection = QStringLiteral("merivus-event-seal-probe-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool valid = false;
    {
        // Reopen the actual candidate path after the old handle was closed.
        // Never SQL-open a sealed destination or recover a hot journal here.
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=") + QString::number(busyTimeout));
        if (database.open() && database.transaction()) {
            valid = sealIdentityMatches(database, expectedEpoch, runId, durable, dropped, discarded);
            if (!database.rollback()) valid = false; // End this read-only snapshot.
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    return valid && safeFile(path, maximumBytes) && sidecarsAbsent(path);
}

bool fileSnapshot(const QString& path, qint64 maximumBytes, ArchiveFileSnapshot* snapshot)
{
    if (!snapshot || !safeFile(path, maximumBytes)) return false;
    const QFileInfo before(path);
    if (!before.exists() || before.size() < 512 || before.size() > maximumBytes) return false;
    QFile file(path);
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
        || before.size() % pageSize != 0 || unsigned32(60) != FleetEventArchiveSql::schemaVersion()
        || unsigned32(68) != static_cast<quint32>(FleetEventArchiveSql::applicationId())
        || unsigned32(24) != unsigned32(92) || unsigned32(28) != static_cast<quint64>(before.size() / pageSize)
        || !file.seek(0)) return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 remaining = before.size();
    while (remaining > 0) {
        const QByteArray chunk = file.read(qMin<qint64>(remaining, 64LL * 1024LL));
        if (chunk.isEmpty() || chunk.size() > remaining) return false;
        hash.addData(chunk); remaining -= chunk.size();
    }
    if (!file.read(1).isEmpty() || file.error() != QFileDevice::NoError || file.size() != before.size()) return false;
    file.close();
    const QFileInfo after(path);
    if (!safeFile(path, maximumBytes) || !after.exists() || after.size() != before.size()
        || after.lastModified() != before.lastModified()) return false;
    snapshot->header = header; snapshot->sha256 = hash.result(); snapshot->bytes = before.size(); snapshot->valid = true;
    return true;
}

bool sameSnapshot(const ArchiveFileSnapshot& first, const ArchiveFileSnapshot& second)
{
    return first.valid && second.valid && first.bytes == second.bytes
        && first.header == second.header && first.sha256 == second.sha256;
}

bool sealNamespaceHasCapacity(const QString& directory, int maximumSegments, QString* error)
{
    const QString prefix = QStringLiteral("fleet-events-sealed-");
    const QString suffix = QStringLiteral(".sqlite");
    const auto inventory = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::SealedEventArchives, maximumSegments, nullptr);
    if (!inventory.ok || !inventory.enumerationCompleted || inventory.directoryIdentity.isEmpty()) {
        *error = QStringLiteral("archive-sealed-directory-inventory-failed"); return false;
    }
    for (const auto& entry : inventory.entries) {
        const QString& name = entry.basename;
        const QFileInfo info(QDir(directory).filePath(name));
        if (!name.startsWith(prefix) || !name.endsWith(suffix)
            || name.size() != prefix.size() + 36 + 1 + 36 + suffix.size()
            || name.at(prefix.size() + 36) != QLatin1Char('-')
            || !uuid(name.mid(prefix.size(), 36)) || !uuid(name.mid(prefix.size() + 37, 36))
            || !entry.regularFile || entry.symlink || !info.isFile() || info.isSymLink()
            || QDir::fromNativeSeparators(info.canonicalFilePath()) != QDir(directory).filePath(name)) {
            *error = QStringLiteral("archive-seal-namespace-rejected"); return false;
        }
    }
    if (inventory.entries.size() >= maximumSegments) { *error = QStringLiteral("archive-seal-segment-cap-reached"); return false; }
    return true;
}

bool sealArchive(const QString& directory, const QString& currentPath, const QString& databaseEpoch,
                 const QString& runId, int maximumSegments, qint64 maximumBytes,
                 qint64 durable, qint64 dropped, qint64 discarded, int busyTimeout,
                 const ArchiveFileSnapshot& beforeClose, QString* basename,
                 bool* mayHaveMoved, QString* error)
{
    if (!uuid(databaseEpoch) || !uuid(runId) || !localDirectory(directory) || !sidecarsAbsent(currentPath)
        || !sealNamespaceHasCapacity(directory, maximumSegments, error)) {
        if (error->isEmpty()) *error = QStringLiteral("archive-seal-source-rejected");
        return false;
    }
    const QString name = QStringLiteral("fleet-events-sealed-") + databaseEpoch + QLatin1Char('-') + runId + QStringLiteral(".sqlite");
    const QString target = QDir(directory).filePath(name);
    const QFileInfo targetInfo(target);
    if (targetInfo.exists() || targetInfo.isSymLink() || !sidecarsAbsent(target)) {
        *error = QStringLiteral("archive-seal-target-exists"); return false;
    }
    ArchiveFileSnapshot afterClose;
    if (!fileSnapshot(currentPath, maximumBytes, &afterClose) || !sameSnapshot(beforeClose, afterClose)
        || !localDirectory(directory) || !sidecarsAbsent(currentPath)
        || QFileInfo::exists(target) || QFileInfo(target).isSymLink()) {
        *error = QStringLiteral("archive-seal-source-changed"); return false;
    }
    if (!probeCurrentSealIdentity(currentPath, databaseEpoch, runId, durable, dropped, discarded, busyTimeout, maximumBytes)) {
        *error = QStringLiteral("archive-seal-current-probe-rejected"); return false;
    }
    ArchiveFileSnapshot afterProbe;
    if (!fileSnapshot(currentPath, maximumBytes, &afterProbe) || !sameSnapshot(beforeClose, afterProbe)
        || !localDirectory(directory) || !sidecarsAbsent(currentPath) || !sidecarsAbsent(target)
        || QFileInfo::exists(target) || QFileInfo(target).isSymLink()) {
        *error = QStringLiteral("archive-seal-postprobe-changed"); return false;
    }
    QFile source(currentPath);
    *mayHaveMoved = true; // rename failure/partial fallback is not a rollback proof.
    if (!source.rename(target)) { *error = QStringLiteral("archive-seal-rename-failed"); return false; }
    ArchiveFileSnapshot sealed;
    if (!localDirectory(directory) || QFileInfo::exists(currentPath) || QFileInfo(currentPath).isSymLink()
        || !sidecarsAbsent(currentPath) || !sidecarsAbsent(target)
        || !fileSnapshot(target, maximumBytes, &sealed) || !sameSnapshot(beforeClose, sealed)) {
        *error = QStringLiteral("archive-seal-postcondition-rejected"); return false;
    }
    *basename = name;
    return true;
}

bool initialize(QSqlDatabase& database, const QString& path, const QVariantMap& policy,
                const QString& runId, QVariantMap* metadata, QString* error)
{
    const qint64 maximumBytes = policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
    if (!safeFile(path, maximumBytes)) { *error = QStringLiteral("archive-path-rejected"); return false; }
    const bool existed = QFileInfo::exists(path);
    const QString timeoutOption = QStringLiteral("QSQLITE_BUSY_TIMEOUT=")
        + QString::number(policy.value(QStringLiteral("busyTimeoutMs")).toInt());
    database.setDatabaseName(path);
    if (existed) {
        // Existing identity is read before any write pragma. A hot-journal
        // database that SQLite cannot read without recovery remains untouched;
        // the engineer must recover its copy offline, never delete/recreate it.
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;") + timeoutOption);
        if (!database.open() || !FleetEventArchiveSql::inspect(database, metadata)) { *error = QStringLiteral("archive-existing-identity-rejected"); return false; }
        database.close();
        if (!safeFile(path, maximumBytes)) { *error = QStringLiteral("archive-path-changed"); return false; }
        database.setConnectOptions(timeoutOption);
    } else {
        QFile reservation(path);
        if (!reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) { *error = QStringLiteral("archive-create-reservation-failed"); return false; }
        const bool permissions = reservation.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        reservation.close();
        if (!permissions) { *error = QStringLiteral("archive-file-permissions-failed"); return false; }
        database.setConnectOptions(timeoutOption);
    }
    if (!database.open() || !safeFile(path, maximumBytes)) { *error = QStringLiteral("archive-open-failed"); return false; }
    if (existed && !FleetEventArchiveSql::inspect(database, metadata)) { *error = QStringLiteral("archive-existing-identity-changed"); return false; }
    QSqlQuery settings(database);
    if (!settings.exec(QStringLiteral("PRAGMA busy_timeout=") + QString::number(policy.value(QStringLiteral("busyTimeoutMs")).toInt()))
        || !pragmaEquals(database, QStringLiteral("PRAGMA journal_mode=DELETE"), QStringLiteral("delete"))
        || !settings.exec(QStringLiteral("PRAGMA synchronous=FULL"))
        || !pragmaEquals(database, QStringLiteral("PRAGMA synchronous"), QStringLiteral("2"))
        || !settings.exec(QStringLiteral("PRAGMA foreign_keys=ON"))
        || !pragmaEquals(database, QStringLiteral("PRAGMA foreign_keys"), QStringLiteral("1"))) {
        *error = QStringLiteral("archive-durability-policy-rejected"); return false;
    }
    qint64 busy = 0, pageSize = 0, pageCount = 0, cap = 0;
    if (!FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA busy_timeout"), &busy) || busy != policy.value(QStringLiteral("busyTimeoutMs")).toLongLong()
        || !FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA page_size"), &pageSize) || pageSize < 512 || pageSize > 65536
        || !FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA page_count"), &pageCount) || pageCount < 0) {
        *error = QStringLiteral("archive-page-policy-rejected"); return false;
    }
    const qint64 maximumPages = maximumBytes / pageSize;
    if (pageCount > maximumPages
        || !FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA max_page_count=") + QString::number(maximumPages), &cap)
        || cap != maximumPages || !FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA max_page_count"), &cap) || cap != maximumPages) {
        *error = QStringLiteral("archive-page-cap-rejected"); return false;
    }
    if (!existed) {
        qint64 application = -1, version = -1, tables = -1;
        if (!FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA application_id"), &application) || application != 0
            || !FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA user_version"), &version) || version != 0
            || !FleetEventArchiveSql::scalar(database, QStringLiteral("SELECT COUNT(*) FROM sqlite_master"), &tables) || tables != 0
            || !database.transaction()) { *error = QStringLiteral("archive-new-identity-rejected"); return false; }
        QSqlQuery create(database);
        const QString epoch = QUuid::createUuid().toString(QUuid::WithoutBraces).toLower();
        bool created = create.exec(FleetEventArchiveSql::metaSchema()) && create.exec(FleetEventArchiveSql::runsSchema()) && create.exec(FleetEventArchiveSql::eventsSchema())
            && create.exec(QStringLiteral("PRAGMA application_id=") + QString::number(FleetEventArchiveSql::applicationId()))
            && create.exec(QStringLiteral("PRAGMA user_version=") + QString::number(FleetEventArchiveSql::schemaVersion()))
            && create.prepare(QStringLiteral("INSERT INTO meta(singleton,database_epoch,retired_count,created_utc_ms) VALUES(1,?,0,?)"));
        if (created) { create.addBindValue(epoch); create.addBindValue(QDateTime::currentMSecsSinceEpoch()); created = create.exec(); }
        create.finish();
        if (!created || !database.commit()) { database.rollback(); *error = QStringLiteral("archive-create-commit-failed"); return false; }
        if (!FleetEventArchiveSql::inspect(database, metadata)) { *error = QStringLiteral("archive-created-identity-rejected"); return false; }
    }
    qint64 existingEvents = 0;
    if (!FleetEventArchiveSql::scalar(database, QStringLiteral("SELECT COUNT(*) FROM events"), &existingEvents)
        || existingEvents < 0 || existingEvents > policy.value(QStringLiteral("maximumEvents")).toInt()) {
        *error = QStringLiteral("archive-existing-retention-rejected"); return false;
    }
    if (!database.transaction()) { *error = QStringLiteral("archive-run-begin-failed"); return false; }
    QSqlQuery run(database);
    bool createdRun = run.prepare(QStringLiteral("INSERT INTO runs(run_id,started_utc_ms,last_sequence,clean_shutdown,dropped_count,discarded_count) VALUES(?,?,0,0,0,0)"));
    if (createdRun) { run.addBindValue(runId); run.addBindValue(QDateTime::currentMSecsSinceEpoch()); createdRun = run.exec(); }
    run.finish();
    if (!createdRun || !database.commit()) { database.rollback(); *error = QStringLiteral("archive-run-commit-failed"); return false; }
    // Shared inspection exposes actual history bounds for readers. They are
    // not live worker counters and must not become stale startup-only status.
    metadata->remove(QStringLiteral("retainedFromRowId"));
    metadata->remove(QStringLiteral("retainedThroughRowId"));
    metadata->insert(QStringLiteral("maximumMainDatabaseBytes"), QString::number(maximumBytes));
    metadata->insert(QStringLiteral("pageSize"), pageSize);
    metadata->insert(QStringLiteral("maximumPages"), maximumPages);
    metadata->insert(QStringLiteral("rollbackJournalNeedsAdditionalSpace"), true);
    return true;
}

bool writeBatch(QSqlDatabase& database, const QList<FleetEventArchiveCodec::Record>& batch,
                const QString& runId, int maximumEvents, qint64 dropped, qint64 discarded,
                qint64* retired, QString* error)
{
    for (const auto& record : batch) if (!FleetEventArchiveSql::recordConsistent(record)) { *error = QStringLiteral("archive-record-rejected"); return false; }
    if (batch.isEmpty() || !database.transaction()) { *error = QStringLiteral("archive-batch-begin-failed"); return false; }
    QSqlQuery insert(database);
    bool ok = insert.prepare(QStringLiteral("INSERT INTO events(run_id,sequence,event_utc_ms,received_utc_ms,received_monotonic_ms,event_type,event_source,canonical_json,content_hash) VALUES(?,?,?,?,?,?,?,?,?)"));
    qint64 excess = 0, retained = 0;
    // Prune before allocation; otherwise a full page-capped database could
    // fail an INSERT even though a bounded replacement batch would fit.
    if (ok) ok = FleetEventArchiveSql::scalar(database, QStringLiteral("SELECT COUNT(*) FROM events"), &retained);
    if (ok) {
        excess = qMax<qint64>(0, retained + batch.size() - maximumEvents);
        if (excess > batch.size()) ok = false; // Do not silently adopt an over-policy existing store.
    }
    if (ok && excess > 0) {
        QSqlQuery prune(database);
        ok = prune.prepare(QStringLiteral("DELETE FROM events WHERE row_id IN (SELECT row_id FROM events ORDER BY row_id LIMIT ?)"));
        if (ok) { prune.addBindValue(excess); ok = prune.exec() && prune.numRowsAffected() == excess; }
        QSqlQuery retire(database);
        if (ok) ok = retire.prepare(QStringLiteral("UPDATE meta SET retired_count=retired_count+? WHERE singleton=1 AND retired_count<=?"));
        if (ok) {
            retire.addBindValue(excess); retire.addBindValue(std::numeric_limits<qint64>::max() - excess);
            ok = retire.exec() && retire.numRowsAffected() == 1;
        }
    }
    for (const auto& record : batch) {
        if (!ok) break;
        insert.bindValue(0, record.runId); insert.bindValue(1, record.sequenceNumber);
        insert.bindValue(2, record.eventUtcMs); insert.bindValue(3, record.receivedUtcMs);
        insert.bindValue(4, record.receivedMonotonicMs); insert.bindValue(5, record.eventType);
        insert.bindValue(6, record.eventSource); insert.bindValue(7, record.canonicalJson); insert.bindValue(8, record.contentHash);
        ok = record.runId == runId && insert.exec() && insert.numRowsAffected() == 1;
    }
    QSqlQuery update(database);
    if (ok) ok = update.prepare(QStringLiteral("UPDATE runs SET last_sequence=?,dropped_count=?,discarded_count=? WHERE run_id=? AND clean_shutdown=0 AND last_sequence<?"));
    if (ok) {
        update.addBindValue(batch.last().sequenceNumber); update.addBindValue(dropped); update.addBindValue(discarded);
        update.addBindValue(runId); update.addBindValue(batch.first().sequenceNumber);
        ok = update.exec() && update.numRowsAffected() == 1;
    }
    qint64 retiredValue = 0;
    if (ok) ok = FleetEventArchiveSql::scalar(database, QStringLiteral("SELECT CASE WHEN typeof(retired_count)='integer' THEN retired_count END FROM meta WHERE singleton=1"), &retiredValue);
    insert.finish(); update.finish();
    if (!ok || !database.commit()) { database.rollback(); *error = QStringLiteral("archive-batch-commit-failed"); return false; }
    *retired = retiredValue;
    return true;
}

bool finishRun(QSqlDatabase& database, const QString& runId, qint64 durable, qint64 dropped, qint64 discarded, bool clean)
{
    if (!database.transaction()) return false;
    QSqlQuery query(database);
    bool ok = query.prepare(QStringLiteral("UPDATE runs SET clean_shutdown=?,dropped_count=?,discarded_count=? WHERE run_id=? AND clean_shutdown=0 AND last_sequence=?"));
    if (ok) {
        query.addBindValue(clean ? 1 : 0); query.addBindValue(dropped); query.addBindValue(discarded);
        query.addBindValue(runId); query.addBindValue(durable);
        ok = query.exec() && query.numRowsAffected() == 1;
    }
    query.finish();
    if (!ok || !database.commit()) { database.rollback(); return false; }
    return true;
}
} // namespace

FleetEventArchiveWorker::FleetEventArchiveWorker(const QString& absoluteDirectory,
                                               const QVariantMap& exactPolicy, const QString& runId)
    : QThread(nullptr), _directory(absoluteDirectory), _policy(exactPolicy), _runId(runId)
{
    _configurationValid = policyValid(_policy) && uuid(_runId);
    if (_configurationValid) {
        _maximumQueueEvents = _policy.value(QStringLiteral("maximumQueueEvents")).toInt();
        _maximumQueueBytes = _policy.value(QStringLiteral("maximumQueueMiB")).toLongLong() * kMiB;
        _batchEventLimit = _policy.value(QStringLiteral("batchEventLimit")).toInt();
        _accepting = true;
    }
    _status = {{QStringLiteral("state"), QStringLiteral("created")}, {QStringLiteral("runId"), _runId},
        {QStringLiteral("errorCode"), QString()}, {QStringLiteral("databaseEpoch"), QString()},
        {QStringLiteral("acceptedCount"), QStringLiteral("0")}, {QStringLiteral("committedCount"), QStringLiteral("0")},
        {QStringLiteral("droppedCount"), QStringLiteral("0")}, {QStringLiteral("discardedCount"), QStringLiteral("0")},
        {QStringLiteral("externalGapCount"), QStringLiteral("0")}, {QStringLiteral("gapObservationCount"), QStringLiteral("0")},
        {QStringLiteral("failedPendingCount"), QStringLiteral("0")}, {QStringLiteral("durableSequence"), QStringLiteral("0")},
        {QStringLiteral("retiredCount"), QStringLiteral("0")}, {QStringLiteral("gapObserved"), false},
        {QStringLiteral("cleanShutdownConfirmed"), false}, {QStringLiteral("pendingIsDurable"), false},
        {QStringLiteral("sealRequested"), false}, {QStringLiteral("sealDecisionFinal"), false},
        {QStringLiteral("sealed"), false}, {QStringLiteral("sealedFileName"), QString()},
        {QStringLiteral("sealedDatabaseEpoch"), QString()}, {QStringLiteral("sealMayHaveMoved"), false},
        {QStringLiteral("rolloverErrorCode"), QString()},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("authenticated"), false}};
    updatePendingLocked();
}

FleetEventArchiveWorker::~FleetEventArchiveWorker()
{
    // Lifetime owner must use finished -> deleteLater; destruction cannot wait
    // for disk, terminate the thread, or pretend a pending batch was durable.
    Q_ASSERT(!isRunning());
}

bool FleetEventArchiveWorker::countLocked(const QString& key, qint64 amount)
{
    const qint64 previous = _status.value(key).toString().toLongLong();
    if (amount < 0 || previous > std::numeric_limits<qint64>::max() - amount) {
        _accepting = false; _stopRequested = true; _drainRequested = false;
        if (_sealRequested) _status.insert(QStringLiteral("rolloverErrorCode"), QStringLiteral("archive-seal-counter-exhausted"));
        _sealRequested = false;
        _status.insert(QStringLiteral("errorCode"), QStringLiteral("archive-counter-exhausted"));
        _status.insert(QStringLiteral("gapObserved"), true); _wake.wakeAll(); return false;
    }
    _status.insert(key, QString::number(previous + amount));
    return true;
}

void FleetEventArchiveWorker::updatePendingLocked()
{
    _status.insert(QStringLiteral("queuedEventCount"), _records.size());
    _status.insert(QStringLiteral("queuedBytes"), QString::number(_queuedBytes));
    _status.insert(QStringLiteral("inFlightEventCount"), _inFlightCount);
    _status.insert(QStringLiteral("inFlightBytes"), QString::number(_inFlightBytes));
    _status.insert(QStringLiteral("pendingEventCount"), _records.size() + _inFlightCount);
    _status.insert(QStringLiteral("pendingQueryCount"), _queryIds.size());
    _status.insert(QStringLiteral("accepting"), _accepting);
    _status.insert(QStringLiteral("stopRequested"), _stopRequested);
    _status.insert(QStringLiteral("stopDecisionFinal"), _stopDecisionTaken);
    _status.insert(QStringLiteral("sealRequested"), _sealRequested);
    _status.insert(QStringLiteral("sealDecisionFinal"), _stopDecisionTaken && _sealRequested);
}

bool FleetEventArchiveWorker::enqueue(const FleetEventArchiveCodec::Record& record)
{
    QMutexLocker lock(&_mutex);
    if (!_accepting || !record.ok || record.runId != _runId || record.sequenceNumber <= _lastOfferedSequence
        || record.sequenceNumber <= 0 || record.sequence != QString::number(record.sequenceNumber)
        || record.canonicalJson.isEmpty() || record.canonicalJson.size() > FleetEventArchiveSql::maximumRecordBytes()) return false;
    if (record.sequenceNumber - _lastOfferedSequence != 1) {
        countLocked(QStringLiteral("gapObservationCount")); _status.insert(QStringLiteral("gapObserved"), true);
        if (!_accepting) { updatePendingLocked(); return false; }
    }
    _lastOfferedSequence = record.sequenceNumber;
    const qint64 charge = recordCharge(record);
    if (_records.size() + _inFlightCount >= _maximumQueueEvents
        || charge > _maximumQueueBytes - _queuedBytes - _inFlightBytes) {
        countLocked(QStringLiteral("droppedCount")); countLocked(QStringLiteral("gapObservationCount"));
        _status.insert(QStringLiteral("gapObserved"), true);
        updatePendingLocked(); return false;
    }
    if (!countLocked(QStringLiteral("acceptedCount"))) { updatePendingLocked(); return false; }
    _records.enqueue(record); _queuedBytes += charge; updatePendingLocked(); _wake.wakeOne(); return true;
}

bool FleetEventArchiveWorker::noteExternalGap(quint64 lostCount)
{
    if (lostCount == 0 || lostCount > static_cast<quint64>(std::numeric_limits<qint64>::max())) return false;
    QMutexLocker lock(&_mutex);
    if (!_accepting || _stopDecisionTaken) return false;
    const qint64 lost = static_cast<qint64>(lostCount);
    const bool counted = countLocked(QStringLiteral("droppedCount"), lost)
        && countLocked(QStringLiteral("externalGapCount"), lost)
        && countLocked(QStringLiteral("gapObservationCount"));
    _status.insert(QStringLiteral("gapObserved"), true);
    updatePendingLocked(); _wake.wakeOne(); return counted;
}

bool FleetEventArchiveWorker::requestPage(const QString& queryId, const QString& runFilter,
                                        const QString& afterRowId, int limit)
{
    qint64 cursor = 0;
    if (!identifier(queryId) || (!runFilter.isEmpty() && !uuid(runFilter))
        || !decimal(afterRowId, &cursor) || limit < 1 || limit > 256) return false;
    QMutexLocker lock(&_mutex);
    if (!_accepting || _queryIds.size() >= kMaximumQueries || _queryIds.contains(queryId)) return false;
    _queryIds.insert(queryId); _queries.enqueue({queryId, runFilter, cursor, limit});
    updatePendingLocked(); _wake.wakeOne(); return true;
}

void FleetEventArchiveWorker::requestStop(bool drain)
{
    QMutexLocker lock(&_mutex);
    // Once the worker has taken the empty-queue stop decision, all accepted
    // work is already finished. A later destructor/stop call cannot race the
    // final clean-commit snapshot or change the completed drain into discard.
    if (_stopDecisionTaken) return;
    if (_sealRequested) _status.insert(QStringLiteral("rolloverErrorCode"), QStringLiteral("archive-seal-cancelled"));
    _sealRequested = false;
    _accepting = false; _stopRequested = true;
    _drainRequested = _drainRequested && drain; // A later call cannot undo a discard request.
    if (!_drainRequested) {
        const int discarded = _records.size();
        _records.clear(); _queuedBytes = 0;
        if (discarded > 0) {
            countLocked(QStringLiteral("discardedCount"), discarded); countLocked(QStringLiteral("gapObservationCount"));
            _status.insert(QStringLiteral("gapObserved"), true);
        }
        _queries.clear(); _queryIds.clear();
    }
    updatePendingLocked(); _wake.wakeAll();
}

bool FleetEventArchiveWorker::requestStopAndSeal(int maximumSealedSegments)
{
    if (maximumSealedSegments < 1 || maximumSealedSegments > 128) return false;
    QMutexLocker lock(&_mutex);
    if (!_accepting || _sealRequested || _stopRequested || _stopDecisionTaken || !_configurationValid) return false;
    _maximumSealedSegments = maximumSealedSegments; _sealRequested = true;
    _accepting = false; _stopRequested = true; _drainRequested = true;
    updatePendingLocked(); _wake.wakeAll(); return true;
}

QVariantMap FleetEventArchiveWorker::current() const
{
    QMutexLocker lock(&_mutex);
    return _status;
}

void FleetEventArchiveWorker::fail(const QString& fixedCode)
{
    {
        QMutexLocker lock(&_mutex);
        _accepting = false; _stopRequested = true; _drainRequested = false;
        if (_sealRequested) _status.insert(QStringLiteral("rolloverErrorCode"), QStringLiteral("archive-seal-storage-failed"));
        _sealRequested = false;
        countLocked(QStringLiteral("failedPendingCount"), _records.size() + _inFlightCount);
        if (!_records.isEmpty() || _inFlightCount > 0) {
            countLocked(QStringLiteral("gapObservationCount")); _status.insert(QStringLiteral("gapObserved"), true);
        }
        _records.clear(); _queuedBytes = 0; _inFlightCount = 0; _inFlightBytes = 0;
        _queries.clear(); _queryIds.clear(); _status.insert(QStringLiteral("state"), QStringLiteral("failed"));
        _status.insert(QStringLiteral("errorCode"), fixedCode); updatePendingLocked();
    }
    emit failure(_runId, fixedCode);
}

void FleetEventArchiveWorker::run()
{
    {
        QMutexLocker lock(&_mutex);
        _status.insert(QStringLiteral("state"), QStringLiteral("opening"));
    }
    if (!_configurationValid || !localDirectory(_directory)) { fail(QStringLiteral("archive-configuration-rejected")); return; }
    const QString currentPath = QDir(_directory).filePath(QStringLiteral("fleet-events-v1.sqlite"));
    const QString ownerPath = currentPath + QStringLiteral(".owner.lock");
    const QFileInfo ownerInfo(ownerPath);
    if (ownerInfo.isSymLink() || (ownerInfo.exists() && (!ownerInfo.isFile() || ownerInfo.size() > 16384
        || QDir::fromNativeSeparators(ownerInfo.canonicalFilePath()) != ownerPath))) {
        fail(QStringLiteral("archive-owner-lock-path-rejected")); return;
    }
    // Cooperative product writers hold the same non-blocking lock throughout
    // SQL open/use/close and optional seal. No age-based theft of live owners.
    QLockFile owner(ownerPath);
    owner.setStaleLockTime(0);
    if (!owner.tryLock(0)) { fail(QStringLiteral("archive-owner-lock-unavailable")); return; }
    const QString connection = QStringLiteral("merivus-event-archive-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString error, rolloverError, databaseEpoch;
    bool seal = false;
    int maximumSegments = 0;
    ArchiveFileSnapshot beforeClose;
    {
        // All query values/handles die in this scope before removeDatabase.
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        QVariantMap metadata;
        if (initialize(database, currentPath, _policy, _runId, &metadata, &error)) {
            databaseEpoch = metadata.value(QStringLiteral("databaseEpoch")).toString();
            {
                QMutexLocker lock(&_mutex);
                for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) _status.insert(it.key(), it.value());
                _status.insert(QStringLiteral("state"), QStringLiteral("recording"));
            }
            emit ready(_runId);
            bool alternate = false;
            while (error.isEmpty()) {
                QList<FleetEventArchiveCodec::Record> batch;
                PageRequest request;
                bool hasQuery = false, stop = false, drain = true;
                qint64 dropped = 0, discarded = 0;
                {
                    QMutexLocker lock(&_mutex);
                    while (_records.isEmpty() && _queries.isEmpty() && !_stopRequested) _wake.wait(&_mutex);
                    if (!_status.value(QStringLiteral("errorCode")).toString().isEmpty()) {
                        error = _status.value(QStringLiteral("errorCode")).toString(); break;
                    }
                    stop = _stopRequested; drain = _drainRequested;
                    if (stop && _records.isEmpty() && _queries.isEmpty()) {
                        _stopDecisionTaken = true; seal = _sealRequested; maximumSegments = _maximumSealedSegments;
                    }
                    // Alternate bounded work so queries cannot starve event commits,
                    // and continuous events cannot indefinitely starve history.
                    if (!_queries.isEmpty() && (_records.isEmpty() || alternate)) {
                        request = _queries.dequeue(); hasQuery = true; alternate = false;
                    } else if (!_records.isEmpty()) {
                        while (!_records.isEmpty() && batch.size() < _batchEventLimit) {
                            const auto record = _records.dequeue();
                            const qint64 charge = recordCharge(record);
                            _queuedBytes -= charge; _inFlightBytes += charge; ++_inFlightCount; batch.append(record);
                        }
                        alternate = true;
                    }
                    dropped = _status.value(QStringLiteral("droppedCount")).toString().toLongLong();
                    discarded = _status.value(QStringLiteral("discardedCount")).toString().toLongLong();
                    updatePendingLocked();
                }
                if (!batch.isEmpty()) {
                    qint64 retired = 0;
                    if (!writeBatch(database, batch, _runId, _policy.value(QStringLiteral("maximumEvents")).toInt(), dropped, discarded, &retired, &error)) break;
                    QMutexLocker lock(&_mutex);
                    _inFlightCount = 0; _inFlightBytes = 0;
                    if (!countLocked(QStringLiteral("committedCount"), batch.size())) { error = QStringLiteral("archive-counter-exhausted"); break; }
                    _status.insert(QStringLiteral("durableSequence"), batch.last().sequence);
                    _status.insert(QStringLiteral("retiredCount"), QString::number(retired)); updatePendingLocked();
                } else if (hasQuery) {
                    QVariantMap page;
                    if (!FleetEventArchiveSql::readPage(database, request.runFilter, request.afterRowId, request.limit,
                        _policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB,
                        metadata.value(QStringLiteral("databaseEpoch")).toString(), &page, &error)) break;
                    bool deliver = false;
                    {
                        QMutexLocker lock(&_mutex);
                        deliver = !(_stopRequested && !_drainRequested);
                        _queryIds.remove(request.queryId); updatePendingLocked();
                    }
                    if (deliver) emit pageReady(_runId, request.queryId, page);
                } else if (stop) {
                    const qint64 durable = current().value(QStringLiteral("durableSequence")).toString().toLongLong();
                    if (!finishRun(database, _runId, durable, dropped, discarded, drain)) error = QStringLiteral("archive-stop-summary-commit-failed");
                    else if (drain) { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("cleanShutdownConfirmed"), true); }
                    break;
                }
            }
            if (error.isEmpty() && seal) {
                {
                    QMutexLocker lock(&_mutex);
                    if (!_status.value(QStringLiteral("cleanShutdownConfirmed")).toBool()) rolloverError = QStringLiteral("archive-seal-clean-shutdown-missing");
                    else _status.insert(QStringLiteral("state"), QStringLiteral("sealing"));
                }
                const qint64 maximumBytes = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
                const QVariantMap stopped = current();
                if (rolloverError.isEmpty() && beginSealReadBarrier(database, databaseEpoch, _runId,
                    stopped.value(QStringLiteral("durableSequence")).toString().toLongLong(),
                    stopped.value(QStringLiteral("droppedCount")).toString().toLongLong(),
                    stopped.value(QStringLiteral("discardedCount")).toString().toLongLong(), &rolloverError)) {
                    if (!sidecarsAbsent(currentPath) || !fileSnapshot(currentPath, maximumBytes, &beforeClose)) {
                        rolloverError = QStringLiteral("archive-seal-preclose-fingerprint-rejected");
                    }
                    if (!database.rollback() && rolloverError.isEmpty()) rolloverError = QStringLiteral("archive-seal-read-barrier-release-failed");
                }
            }
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    if (error.isEmpty() && seal) {
        QString name;
        bool mayHaveMoved = false;
        bool sealed = false;
        if (rolloverError.isEmpty()) {
            const QVariantMap stopped = current();
            sealed = sealArchive(_directory, currentPath, databaseEpoch, _runId, maximumSegments,
                _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB,
                stopped.value(QStringLiteral("durableSequence")).toString().toLongLong(),
                stopped.value(QStringLiteral("droppedCount")).toString().toLongLong(),
                stopped.value(QStringLiteral("discardedCount")).toString().toLongLong(),
                _policy.value(QStringLiteral("busyTimeoutMs")).toInt(),
                beforeClose, &name, &mayHaveMoved, &rolloverError);
        }
        QMutexLocker lock(&_mutex);
        _status.insert(QStringLiteral("sealed"), sealed);
        _status.insert(QStringLiteral("sealMayHaveMoved"), mayHaveMoved);
        _status.insert(QStringLiteral("rolloverErrorCode"), rolloverError);
        if (sealed) {
            _status.insert(QStringLiteral("sealedFileName"), name);
            _status.insert(QStringLiteral("sealedDatabaseEpoch"), databaseEpoch);
        }
    }
    if (!error.isEmpty()) fail(error);
    else {
        QMutexLocker lock(&_mutex);
        _accepting = false; _status.insert(QStringLiteral("state"), QStringLiteral("stopped")); updatePendingLocked();
    }
}

#include "FleetEventArchiveMaintenanceWorker.h"

#include "FleetArchiveDirectoryInventory.h"
#include "FleetEventArchiveSql.h"

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
#include <QScopeGuard>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariantList>
#include <QtEndian>

#include <algorithm>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr qint64 kResultMaximum = 2LL * kMiB - 2048;
constexpr int kMaximumFiles = 128;

struct File {
    QString basename;
    QString kind;
    QString epoch;
    QString run;
    QString operation;
    qint64 bytes = 0;
    qint64 modified = 0;
};
struct Listing {
    QString identity;
    QList<File> files;
    int sealed = 0;
    int retired = 0;
};
struct Fingerprint {
    QByteArray header;
    QByteArray hash;
    qint64 bytes = 0;
    qint64 modified = 0;
};

bool text(const QVariant& value) { return value.type() == QVariant::String && !value.isNull(); }
bool bit(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}
bool uuid(const QString& value)
{
    return !QUuid(value).isNull() && QUuid(value).toString(QUuid::WithoutBraces).toLower() == value;
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
bool decimal(const QString& value, qint64 minimum = 0, qint64* number = nullptr)
{
    if (value.isEmpty() || value.size() > 19 || (value.size() > 1 && value.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : value) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = value.toLongLong(&ok);
    if (!ok || parsed < minimum || QString::number(parsed) != value) return false;
    if (number) *number = parsed;
    return true;
}
bool lowerHex(const QString& value, int length)
{
    if (value.size() != length) return false;
    for (const QChar ch : value) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
bool directoryIdentity(const QString& value)
{
    // POSIX native uintmax_t may be wider than qint64; preserve its unsigned
    // decimal object identifier rather than narrowing it to a JSON number.
    if (value.size() > 256) return false;
    const QStringList parts = value.split(QLatin1Char(':'));
    if (parts.size() == 4 && parts.at(0) == QStringLiteral("windows-volume-file-index"))
        return lowerHex(parts.at(1), 8) && lowerHex(parts.at(2), 8) && lowerHex(parts.at(3), 8);
    if (parts.size() != 3 || parts.at(0) != QStringLiteral("posix-device-inode")) return false;
    for (int index = 1; index != 3; ++index) {
        const QString& part = parts.at(index);
        if (part.isEmpty() || part.size() > 80 || (part.size() > 1 && part.at(0) == QLatin1Char('0'))) return false;
        for (const QChar ch : part) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    }
    return true;
}
bool integerIn(const QVariant& value, qint64 low, qint64 high)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false;
    }
    bool ok = false;
    const qint64 number = value.toLongLong(&ok);
    return ok && number >= low && number <= high;
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
QString prefix(const QString& kind)
{
    return kind == QStringLiteral("sealed") ? QStringLiteral("fleet-events-sealed-")
        : kind == QStringLiteral("retired") ? QStringLiteral("fleet-events-retired-") : QString();
}
bool parseName(const QString& name, File* value)
{
    if (!value) return false;
    QString kind;
    if (name.startsWith(prefix(QStringLiteral("sealed")))) kind = QStringLiteral("sealed");
    else if (name.startsWith(prefix(QStringLiteral("retired")))) kind = QStringLiteral("retired");
    else return false;
    const QString start = prefix(kind);
    if (name.size() != start.size() + 36 + 1 + 36 + 7 || !name.endsWith(QStringLiteral(".sqlite"))
        || name.at(start.size() + 36) != QLatin1Char('-')) return false;
    const QString epoch = name.mid(start.size(), 36), run = name.mid(start.size() + 37, 36);
    if (!uuid(epoch) || !uuid(run)) return false;
    value->basename = name; value->kind = kind; value->epoch = epoch; value->run = run;
    return true;
}
bool parseBackupName(const QString& name, File* value)
{
    const QString start = QStringLiteral("fleet-events-backup-");
    if (!value || !name.startsWith(start) || name.size() != start.size() + 3 * 36 + 2 + 7
        || !name.endsWith(QStringLiteral(".sqlite")) || name.at(start.size() + 36) != QLatin1Char('-')
        || name.at(start.size() + 73) != QLatin1Char('-')) return false;
    const QString epoch = name.mid(start.size(), 36), run = name.mid(start.size() + 37, 36);
    const QString operation = name.mid(start.size() + 74, 36);
    if (!uuid(epoch) || !uuid(run) || !uuid(operation)) return false;
    value->basename = name; value->kind = QStringLiteral("backup");
    value->epoch = epoch; value->run = run; value->operation = operation;
    return true;
}
QString backupName(const File& source, const QString& operation)
{
    return QStringLiteral("fleet-events-backup-") + source.epoch + QLatin1Char('-') + source.run
        + QLatin1Char('-') + operation + QStringLiteral(".sqlite");
}
QString importTargetName(const File& source)
{
    return QStringLiteral("fleet-events-retired-") + source.epoch + QLatin1Char('-') + source.run + QStringLiteral(".sqlite");
}
QString importSealedSibling(const File& source)
{
    return QStringLiteral("fleet-events-sealed-") + source.epoch + QLatin1Char('-') + source.run + QStringLiteral(".sqlite");
}
bool destinationSyntax(const QString& directory)
{
    if (directory.isEmpty() || directory.size() > 4096 || directory.trimmed() != directory
        || directory.contains(QChar::Null) || QDir::fromNativeSeparators(directory) != directory
        || !QDir::isAbsolutePath(directory) || QDir::cleanPath(directory) != directory
        || directory.startsWith(QStringLiteral("//")) || directory.contains(QLatin1Char('\\'))
        || directory == QStringLiteral("/")) return false;
    for (int index = 0; index < directory.size(); ++index) {
        const ushort unit = directory.at(index).unicode();
        if (unit >= 0xd800 && unit <= 0xdbff) {
            if (++index >= directory.size()) return false;
            const ushort low = directory.at(index).unicode();
            if (low < 0xdc00 || low > 0xdfff) return false;
        } else if (unit >= 0xdc00 && unit <= 0xdfff) return false;
    }
#if defined(Q_OS_WIN)
    if (directory.size() <= 3 || directory.at(1) != QLatin1Char(':') || directory.at(2) != QLatin1Char('/')
        || !((directory.at(0) >= QLatin1Char('A') && directory.at(0) <= QLatin1Char('Z'))
            || (directory.at(0) >= QLatin1Char('a') && directory.at(0) <= QLatin1Char('z')))
        || directory.mid(2).contains(QLatin1Char(':'))) return false;
#endif
    return true;
}
bool disjointDirectories(const QString& source, const QString& destination)
{
#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    const QString sourceChild = source + (source.endsWith(QLatin1Char('/')) ? QStringLiteral("") : QStringLiteral("/"));
    const QString destinationChild = destination + (destination.endsWith(QLatin1Char('/')) ? QStringLiteral("") : QStringLiteral("/"));
    return source.compare(destination, sensitivity) != 0
        && !destination.startsWith(sourceChild, sensitivity) && !source.startsWith(destinationChild, sensitivity);
}
bool descriptorValid(const QVariantMap& value, File* parsed = nullptr)
{
    if (value.size() != 7) return false;
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("kind"), QStringLiteral("databaseEpoch"),
        QStringLiteral("sealRunId"), QStringLiteral("fileBytes"), QStringLiteral("fileFingerprint"), QStringLiteral("modifiedUtcMs")})
        if (!text(value.value(key))) return false;
    File file;
    if (!parseName(value.value(QStringLiteral("basename")).toString(), &file)
        || file.kind != value.value(QStringLiteral("kind")).toString()
        || file.epoch != value.value(QStringLiteral("databaseEpoch")).toString()
        || file.run != value.value(QStringLiteral("sealRunId")).toString()
        || !decimal(value.value(QStringLiteral("fileBytes")).toString(), 512, &file.bytes)
        || !decimal(value.value(QStringLiteral("modifiedUtcMs")).toString(), 1, &file.modified)
        || !lowerHex(value.value(QStringLiteral("fileFingerprint")).toString(), 64)) return false;
    if (parsed) *parsed = file;
    return true;
}
bool backupDescriptorValid(const QVariantMap& value, File* parsed = nullptr)
{
    if (value.size() != 8) return false;
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("kind"), QStringLiteral("databaseEpoch"),
        QStringLiteral("sealRunId"), QStringLiteral("backupOperationId"), QStringLiteral("fileBytes"),
        QStringLiteral("fileFingerprint"), QStringLiteral("modifiedUtcMs")}) if (!text(value.value(key))) return false;
    File file;
    if (!parseBackupName(value.value(QStringLiteral("basename")).toString(), &file)
        || value.value(QStringLiteral("kind")).toString() != QStringLiteral("backup")
        || file.epoch != value.value(QStringLiteral("databaseEpoch")).toString()
        || file.run != value.value(QStringLiteral("sealRunId")).toString()
        || file.operation != value.value(QStringLiteral("backupOperationId")).toString()
        || !decimal(value.value(QStringLiteral("fileBytes")).toString(), 512, &file.bytes)
        || !decimal(value.value(QStringLiteral("modifiedUtcMs")).toString(), 1, &file.modified)
        || !lowerHex(value.value(QStringLiteral("fileFingerprint")).toString(), 64)) return false;
    if (parsed) *parsed = file;
    return true;
}
QVariantMap common()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
        {QStringLiteral("restoresAuthorization"), false}, {QStringLiteral("allRecordHashesVerified"), false},
        {QStringLiteral("catalogCompletenessVerified"), false}, {QStringLiteral("crossSegmentAtomicSnapshot"), false}};
}
bool commonMatches(const QVariantMap& value)
{
    const QVariantMap fields = common();
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) if (!bit(value.value(it.key()), it.value().toBool())) return false;
    return true;
}
bool localDirectory(const QString& directory)
{
    if (directory.isEmpty() || directory.size() > 4096 || directory.contains(QChar::Null)) return false;
    const QFileInfo info(directory);
    return QDir::isAbsolutePath(directory) && !directory.startsWith(QStringLiteral("//"))
        && QDir::cleanPath(directory) == directory && info.exists() && info.isDir() && info.isReadable()
        && !info.isSymLink() && QDir::fromNativeSeparators(info.canonicalFilePath()) == directory;
}
bool sidecarsAbsent(const QString& path)
{
    for (const QString& suffix : {QStringLiteral("-journal"), QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        const QFileInfo info(path + suffix);
        if (info.exists() || info.isSymLink()) return false;
    }
    return true;
}
bool absent(const QString& directory, const QString& basename)
{
    const QString path = QDir(directory).filePath(basename);
    const QFileInfo info(path);
    return !info.exists() && !info.isSymLink() && sidecarsAbsent(path);
}
bool safeFile(const QString& directory, const QString& basename, qint64 maximum, QFileInfo* actual = nullptr,
              bool backup = false)
{
    File parsed;
    // Backup readback is explicitly scoped. Original sealed/retired callers
    // never gain an alternate filename or permission to open an arbitrary DB.
    if (!(backup ? parseBackupName(basename, &parsed) : parseName(basename, &parsed)) || !localDirectory(directory)) return false;
    const QString path = QDir(directory).filePath(basename);
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || info.isSymLink() || !info.isReadable()
        || info.size() < 512 || info.size() > maximum || info.lastModified().toMSecsSinceEpoch() < 1
        || QDir::fromNativeSeparators(info.canonicalFilePath()) != path || !sidecarsAbsent(path)) return false;
    if (actual) *actual = info;
    return true;
}
bool listing(const QString& directory, qint64 maximum, const std::atomic_bool& cancelled, Listing* result)
{
    if (!result || cancelled.load()) return false;
    Listing value;
    for (const auto which : {FleetArchiveDirectoryInventory::Namespace::SealedEventArchives,
                            FleetArchiveDirectoryInventory::Namespace::RetiredEventArchives}) {
        const auto scan = FleetArchiveDirectoryInventory::scan(directory, which, kMaximumFiles, &cancelled);
        if (!scan.ok || !scan.enumerationCompleted || !directoryIdentity(scan.directoryIdentity)
            || (!value.identity.isEmpty() && value.identity != scan.directoryIdentity)) return false;
        value.identity = scan.directoryIdentity;
        for (const auto& entry : scan.entries) {
            File file;
            QFileInfo actual;
            if (cancelled.load() || !entry.regularFile || entry.symlink || !parseName(entry.basename, &file)
                || (which == FleetArchiveDirectoryInventory::Namespace::SealedEventArchives
                    ? file.kind != QStringLiteral("sealed") : file.kind != QStringLiteral("retired"))
                || !safeFile(directory, entry.basename, maximum, &actual)) return false;
            file.bytes = actual.size(); file.modified = actual.lastModified().toMSecsSinceEpoch();
            value.files.append(file);
            if (file.kind == QStringLiteral("sealed")) ++value.sealed; else ++value.retired;
        }
    }
    if (cancelled.load()) return false;
    *result = value;
    return true;
}
bool sameListing(const Listing& first, const Listing& second)
{
    if (first.identity != second.identity || first.sealed != second.sealed || first.retired != second.retired
        || first.files.size() != second.files.size()) return false;
    for (int index = 0; index < first.files.size(); ++index) {
        const File& a = first.files.at(index); const File& b = second.files.at(index);
        if (a.basename != b.basename || a.bytes != b.bytes || a.modified != b.modified) return false;
    }
    return true;
}
bool backupListing(const QString& directory, qint64 maximum, const std::atomic_bool& cancelled, Listing* result)
{
    if (!result || cancelled.load()) return false;
    const auto scan = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::EventBackups, kMaximumFiles, &cancelled);
    if (!scan.ok || !scan.enumerationCompleted || !directoryIdentity(scan.directoryIdentity)) return false;
    Listing value;
    value.identity = scan.directoryIdentity;
    for (const auto& entry : scan.entries) {
        File file;
        QFileInfo actual;
        // The native namespace is case-insensitive, the accepted full name is
        // exact. Case variants/sidecars/bad last rows cannot disappear from a
        // seemingly complete, successful backup catalog.
        if (cancelled.load() || !entry.regularFile || entry.symlink || !parseBackupName(entry.basename, &file)
            || !safeFile(directory, entry.basename, maximum, &actual, true)) return false;
        file.bytes = actual.size(); file.modified = actual.lastModified().toMSecsSinceEpoch();
        value.files.append(file);
    }
    if (cancelled.load()) return false;
    *result = value;
    return true;
}
bool diagnosticBasename(const QString& name)
{
    if (name.isEmpty() || name.size() > 512 || name.contains(QChar::Null) || name.contains(QLatin1Char('/'))) return false;
    for (int index = 0; index < name.size(); ++index) {
        const ushort unit = name.at(index).unicode();
        if (unit >= 0xd800 && unit <= 0xdbff) {
            if (++index >= name.size()) return false;
            const ushort low = name.at(index).unicode();
            if (low < 0xdc00 || low > 0xdfff) return false;
        } else if (unit >= 0xdc00 && unit <= 0xdfff) return false;
    }
    // A POSIX backslash is a representable raw name, not a path separator.
    // It still fails parseBackupName and must never be opened as a payload.
    return true;
}
bool diagnosticReason(const QString& code)
{
    return QStringList{QStringLiteral("archive-backup-name-invalid"), QStringLiteral("archive-backup-type-rejected"),
        QStringLiteral("archive-backup-size-rejected"), QStringLiteral("archive-backup-sidecars-present"),
        QStringLiteral("archive-backup-file-rejected"), QStringLiteral("archive-backup-structure-unconfirmed")}.contains(code);
}
bool sameBackupCandidates(const FleetArchiveDirectoryInventory::Snapshot& first,
                          const FleetArchiveDirectoryInventory::Snapshot& second)
{
    if (!first.ok || !first.enumerationCompleted || !second.ok || !second.enumerationCompleted
        || first.directoryIdentity != second.directoryIdentity || first.entries.size() != second.entries.size()) return false;
    for (int index = 0; index < first.entries.size(); ++index) {
        const auto& a = first.entries.at(index); const auto& b = second.entries.at(index);
        if (a.basename != b.basename || a.regularFile != b.regularFile || a.symlink != b.symlink) return false;
    }
    return true;
}
bool fingerprint(const QString& directory, const File& expected, qint64 maximum,
                 const std::atomic_bool& cancelled, Fingerprint* result, bool backup = false,
                 bool* fileCloseFailed = nullptr)
{
    QFileInfo before;
    if (!result || cancelled.load() || !safeFile(directory, expected.basename, maximum, &before, backup)
        || before.size() != expected.bytes || before.lastModified().toMSecsSinceEpoch() != expected.modified) return false;
    QFile file(before.filePath());
    const auto observeClose = qScopeGuard([&file, fileCloseFailed]() {
        if (file.isOpen()) {
            file.close();
            if (file.error() != QFileDevice::NoError && fileCloseFailed) *fileCloseFailed = true;
        }
    });
    if (!file.open(QIODevice::ReadOnly) || file.size() != before.size()) return false;
    const QByteArray header = file.read(100);
    if (header.size() != 100 || header.left(16) != QByteArray("SQLite format 3\0", 16)
        || static_cast<unsigned char>(header.at(18)) != 1 || static_cast<unsigned char>(header.at(19)) != 1) return false;
    const auto unsigned32 = [&header](int offset) {
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(header.constData() + offset));
    };
    const quint16 encodedPage = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(header.constData() + 16));
    const qint64 page = encodedPage == 1 ? 65536 : encodedPage;
    if (page < 512 || page > 65536 || (page & (page - 1)) != 0 || before.size() % page != 0
        || unsigned32(60) != static_cast<quint32>(FleetEventArchiveSql::schemaVersion())
        || unsigned32(68) != static_cast<quint32>(FleetEventArchiveSql::applicationId())
        || unsigned32(24) != unsigned32(92) || unsigned32(28) != static_cast<quint64>(before.size() / page)
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
    if (file.error() != QFileDevice::NoError) {
        if (fileCloseFailed) *fileCloseFailed = true;
        return false;
    }
    QFileInfo after;
    if (!safeFile(directory, expected.basename, maximum, &after, backup) || after.size() != before.size()
        || after.lastModified() != before.lastModified()) return false;
    result->header = header; result->hash = hash.result(); result->bytes = before.size();
    result->modified = before.lastModified().toMSecsSinceEpoch();
    return true;
}
bool sameFingerprint(const Fingerprint& a, const Fingerprint& b)
{
    return a.header == b.header && a.hash == b.hash && a.bytes == b.bytes && a.modified == b.modified;
}
bool settingsMatch(QSqlDatabase& database, int timeout)
{
    qint64 busy = 0;
    if (!FleetEventArchiveSql::scalar(database, QStringLiteral("PRAGMA busy_timeout"), &busy) || busy != timeout) return false;
    QSqlQuery mode(database);
    return mode.exec(QStringLiteral("PRAGMA journal_mode")) && mode.next()
        && text(mode.value(0)) && mode.value(0).toString() == QStringLiteral("delete")
        && !mode.next() && !mode.lastError().isValid();
}
bool inspectFile(const QString& directory, const File& file, const QVariantMap& policy,
                 const std::atomic_bool& cancelled, QVariantMap* descriptor, bool backup = false,
                 bool* fileCloseFailed = nullptr)
{
    const qint64 maximum = policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
    Fingerprint before;
    if (!descriptor || !fingerprint(directory, file, maximum, cancelled, &before, backup, fileCloseFailed)) return false;
    const QString connection = QStringLiteral("merivus-archive-maintenance-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool valid = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(QDir(directory).filePath(file.basename));
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=")
            + QString::number(policy.value(QStringLiteral("busyTimeoutMs")).toInt()));
        if (!cancelled.load() && database.open() && settingsMatch(database, policy.value(QStringLiteral("busyTimeoutMs")).toInt())
            && !cancelled.load() && database.transaction()) {
            // Shared native inspection validates application/schema/meta epoch
            // and the original seal run's clean flag. It is not row hash proof.
            valid = !cancelled.load() && FleetEventArchiveSql::sealedRunMatches(database, file.epoch, file.run);
            if (!database.rollback()) valid = false;
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    Fingerprint after;
    if (!valid || cancelled.load() || !fingerprint(directory, file, maximum, cancelled, &after, backup, fileCloseFailed)
        || !sameFingerprint(before, after)) return false;
    *descriptor = {{QStringLiteral("basename"), file.basename}, {QStringLiteral("kind"), file.kind},
        {QStringLiteral("databaseEpoch"), file.epoch}, {QStringLiteral("sealRunId"), file.run},
        {QStringLiteral("fileBytes"), QString::number(after.bytes)},
        {QStringLiteral("fileFingerprint"), QString::fromLatin1(after.hash.toHex())},
        {QStringLiteral("modifiedUtcMs"), QString::number(after.modified)}};
    if (!backup) return descriptorValid(*descriptor);
    File parsed;
    return file.kind == QStringLiteral("backup") && parseBackupName(file.basename, &parsed)
        && parsed.epoch == file.epoch && parsed.run == file.run && parsed.operation == file.operation;
}
QString targetName(const File& source)
{
    return prefix(source.kind == QStringLiteral("sealed") ? QStringLiteral("retired") : QStringLiteral("sealed"))
        + source.epoch + QLatin1Char('-') + source.run + QStringLiteral(".sqlite");
}
bool nativeRenameSupported()
{
#if defined(Q_OS_WIN) || (defined(Q_OS_LINUX) && defined(SYS_renameat2) && defined(RENAME_NOREPLACE))
    return true;
#else
    // Do not fall back to a rename that overwrites, link/unlink, or a Qt
    // rename implementation which may copy/remove/rollback internally.
    return false;
#endif
}
bool nativeRenameNoReplace(const QString& directory, const QString& source, const QString& target)
{
    const QString from = QDir(directory).filePath(source), to = QDir(directory).filePath(target);
#if defined(Q_OS_WIN)
    const QString nativeFrom = QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(from);
    const QString nativeTo = QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(to);
    // No MOVEFILE_REPLACE_EXISTING or MOVEFILE_COPY_ALLOWED. Both names are
    // fixed basenames in the same already inspected local directory.
    return MoveFileExW(reinterpret_cast<LPCWSTR>(nativeFrom.utf16()),
                       reinterpret_cast<LPCWSTR>(nativeTo.utf16()), 0) != 0;
#elif defined(Q_OS_LINUX) && defined(SYS_renameat2) && defined(RENAME_NOREPLACE)
    const QByteArray nativeFrom = QFile::encodeName(from), nativeTo = QFile::encodeName(to);
    if (nativeFrom.contains('\0') || nativeTo.contains('\0') || QFile::decodeName(nativeFrom) != from
        || QFile::decodeName(nativeTo) != to) return false;
    // Kernel/filesystem lack of RENAME_NOREPLACE support is a failed attempt,
    // never permission to retry with ordinary overwriting POSIX rename.
    return syscall(SYS_renameat2, AT_FDCWD, nativeFrom.constData(), AT_FDCWD, nativeTo.constData(), RENAME_NOREPLACE) == 0;
#else
    Q_UNUSED(from);
    Q_UNUSED(to);
    return false;
#endif
}
bool fixedError(const QString& code)
{
    return QStringList{QString(), QStringLiteral("archive-maintenance-configuration-rejected"),
        QStringLiteral("archive-maintenance-directory-rejected"), QStringLiteral("archive-maintenance-cancelled"),
        QStringLiteral("archive-maintenance-owner-lock-path-rejected"), QStringLiteral("archive-maintenance-owner-lock-unavailable"),
        QStringLiteral("archive-maintenance-platform-unsupported"),
        QStringLiteral("archive-maintenance-inventory-rejected"), QStringLiteral("archive-maintenance-directory-changed"),
        QStringLiteral("archive-maintenance-source-changed"), QStringLiteral("archive-maintenance-target-exists"),
        QStringLiteral("archive-maintenance-capacity-rejected"), QStringLiteral("archive-maintenance-rename-unconfirmed"),
        QStringLiteral("archive-maintenance-postcondition-unconfirmed"), QStringLiteral("archive-maintenance-result-contract-rejected")}.contains(code);
}
bool backupError(const QString& code)
{
    return fixedError(code) || QStringList{QStringLiteral("archive-backup-destination-rejected"),
        QStringLiteral("archive-backup-destination-lock-rejected"), QStringLiteral("archive-backup-destination-lock-unavailable"),
        QStringLiteral("archive-backup-destination-changed"), QStringLiteral("archive-backup-target-exists"),
        QStringLiteral("archive-backup-target-create-unconfirmed"), QStringLiteral("archive-backup-read-failed"),
        QStringLiteral("archive-backup-write-failed"), QStringLiteral("archive-backup-copy-incomplete"),
        QStringLiteral("archive-backup-flush-failed"), QStringLiteral("archive-backup-close-unconfirmed"),
        QStringLiteral("archive-backup-readback-unconfirmed"), QStringLiteral("archive-backup-source-changed")}.contains(code);
}
bool importError(const QString& code)
{
    return backupError(code) || QStringList{QStringLiteral("archive-import-source-rejected"),
        QStringLiteral("archive-import-source-changed"), QStringLiteral("archive-import-target-rejected"),
        QStringLiteral("archive-import-target-changed"), QStringLiteral("archive-import-target-exists"),
        QStringLiteral("archive-import-capacity-rejected"), QStringLiteral("archive-import-staging-exists"),
        QStringLiteral("archive-import-staging-create-unconfirmed"), QStringLiteral("archive-import-staging-unconfirmed"),
        QStringLiteral("archive-import-publish-unconfirmed"), QStringLiteral("archive-import-postcondition-unconfirmed")}.contains(code);
}
bool destinationIdentity(const QString& directory, const std::atomic_bool& cancelled, QString* identity)
{
    // This is a native EOF/error-aware directory-object observation, NOT a
    // backup catalog or validation of unrelated files. Every entry is still
    // counted by the shared scanner's 4096 bound; no backup rows are imported.
    const auto scan = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::SealedEventArchives, kMaximumFiles, &cancelled);
    if (!scan.ok || !scan.enumerationCompleted || !directoryIdentity(scan.directoryIdentity)) return false;
    *identity = scan.directoryIdentity;
    return true;
}
qint64 jsonBytes(const QVariantMap& value)
{
    return QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size();
}
bool purgeKind(const QString& kind)
{
    return kind == QStringLiteral("purge-preview") || kind == QStringLiteral("purge-retired");
}
QVariantMap backupRequest(const QVariantMap& basis)
{
    return {{QStringLiteral("kind"), QStringLiteral("backup")},
        {QStringLiteral("source"), basis.value(QStringLiteral("source"))},
        {QStringLiteral("expectedDirectoryIdentity"), basis.value(QStringLiteral("expectedDirectoryIdentity"))},
        {QStringLiteral("destinationDirectory"), basis.value(QStringLiteral("destinationDirectory"))},
        {QStringLiteral("operationId"), basis.value(QStringLiteral("operationId"))}};
}
bool purgeBasisValid(const QVariantMap& basis, const QVariantMap& policy)
{
    File source;
    return FleetEventArchiveMaintenanceWorker::resultMatches(basis, backupRequest(basis), policy)
        && bit(basis.value(QStringLiteral("backupVerified")), true)
        && descriptorValid(basis.value(QStringLiteral("source")).toMap(), &source)
        && source.kind == QStringLiteral("retired")
        && source.bytes <= policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB
        && basis.value(QStringLiteral("observedDirectoryIdentity")) == basis.value(QStringLiteral("expectedDirectoryIdentity"))
        && !basis.value(QStringLiteral("destinationDirectoryIdentity")).toString().isEmpty()
        && basis.value(QStringLiteral("observedDirectoryIdentity")) != basis.value(QStringLiteral("destinationDirectoryIdentity"));
}
bool windowsFileIdentity(const QString& value)
{
    const QStringList parts = value.split(QLatin1Char(':'));
    return parts.size() == 4 && parts.at(0) == QStringLiteral("windows-volume-file-index")
        && lowerHex(parts.at(1), 8) && lowerHex(parts.at(2), 8) && lowerHex(parts.at(3), 8);
}
bool purgeBackupValid(const QVariantMap& descriptor, const QVariantMap& basis)
{
    File backup;
    const QVariantMap source = basis.value(QStringLiteral("source")).toMap();
    if (!backupDescriptorValid(descriptor, &backup)
        || descriptor.value(QStringLiteral("basename")) != basis.value(QStringLiteral("targetBasename"))
        || descriptor.value(QStringLiteral("backupOperationId")) != basis.value(QStringLiteral("operationId"))) return false;
    for (const QString& key : {QStringLiteral("databaseEpoch"), QStringLiteral("sealRunId"),
        QStringLiteral("fileBytes"), QStringLiteral("fileFingerprint")})
        if (descriptor.value(key) != source.value(key)) return false;
    return true;
}
bool purgeProofValid(const QVariantMap& proof, const QVariantMap& basis)
{
    if (proof.size() != 3 || !text(proof.value(QStringLiteral("sourceFileIdentity")))
        || !text(proof.value(QStringLiteral("backupFileIdentity")))
        || !windowsFileIdentity(proof.value(QStringLiteral("sourceFileIdentity")).toString())
        || !windowsFileIdentity(proof.value(QStringLiteral("backupFileIdentity")).toString())
        || proof.value(QStringLiteral("sourceFileIdentity")) == proof.value(QStringLiteral("backupFileIdentity"))
        || proof.value(QStringLiteral("backupDescriptor")).type() != QVariant::Map
        || proof.value(QStringLiteral("backupDescriptor")).isNull()) return false;
    return purgeBackupValid(proof.value(QStringLiteral("backupDescriptor")).toMap(), basis);
}
QString purgeReceiptName(const QString& operation)
{
    return QStringLiteral("fleet-events-purge-") + operation + QStringLiteral(".jsonl");
}
bool purgeError(const QString& code)
{
    return fixedError(code) || QStringList{QStringLiteral("archive-purge-basis-rejected"),
        QStringLiteral("archive-purge-backup-changed"), QStringLiteral("archive-purge-proof-changed"),
        QStringLiteral("archive-purge-handle-open-unconfirmed"), QStringLiteral("archive-purge-handle-close-unconfirmed"),
        QStringLiteral("archive-purge-delete-unconfirmed"), QStringLiteral("archive-purge-backup-postcondition-unconfirmed"),
        QStringLiteral("archive-purge-receipt-create-unconfirmed"), QStringLiteral("archive-purge-receipt-write-failed"),
        QStringLiteral("archive-purge-receipt-flush-failed"), QStringLiteral("archive-purge-receipt-close-unconfirmed"),
        QStringLiteral("archive-purge-receipt-budget-rejected")}.contains(code);
}
bool purgeReceiptError(const QString& code)
{
    return QStringList{QStringLiteral(""), QStringLiteral("archive-purge-receipt-create-unconfirmed"),
        QStringLiteral("archive-purge-receipt-write-failed"), QStringLiteral("archive-purge-receipt-flush-failed"),
        QStringLiteral("archive-purge-receipt-close-unconfirmed"), QStringLiteral("archive-purge-receipt-budget-rejected")}.contains(code);
}
bool nativePurgeSupported()
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}
#if defined(Q_OS_WIN)
// Handles are never transferred to the owner. A failed close is not retried on
// an uncertain handle value, and cannot be reported as a confirmed release.
class PurgeHandle
{
public:
    ~PurgeHandle() { close(); }
    bool open(const QString& path, bool deletable)
    {
        if (_value != INVALID_HANDLE_VALUE) return false;
        const QString native = QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(path);
        _value = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()),
            GENERIC_READ | (deletable ? DELETE : 0), FILE_SHARE_READ, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        return _value != INVALID_HANDLE_VALUE;
    }
    bool valid() const { return _value != INVALID_HANDLE_VALUE; }
    HANDLE value() const { return _value; }
    bool close()
    {
        if (!valid()) return false;
        const HANDLE old = _value;
        _value = INVALID_HANDLE_VALUE;
        return CloseHandle(old) != 0;
    }
private:
    HANDLE _value = INVALID_HANDLE_VALUE;
};
QString handleIdentity(const BY_HANDLE_FILE_INFORMATION& info)
{
    return QStringLiteral("windows-volume-file-index:%1:%2:%3")
        .arg(static_cast<qulonglong>(info.dwVolumeSerialNumber), 8, 16, QLatin1Char('0'))
        .arg(static_cast<qulonglong>(info.nFileIndexHigh), 8, 16, QLatin1Char('0'))
        .arg(static_cast<qulonglong>(info.nFileIndexLow), 8, 16, QLatin1Char('0'));
}
bool handleInformation(HANDLE handle, const File& expected, BY_HANDLE_FILE_INFORMATION* result)
{
    BY_HANDLE_FILE_INFORMATION info{};
    if (!result || GetFileType(handle) != FILE_TYPE_DISK || !GetFileInformationByHandle(handle, &info)
        || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0) return false;
    const quint64 bytes = (static_cast<quint64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    const quint64 ticks = (static_cast<quint64>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime;
    constexpr quint64 unixOffset = 116444736000000000ULL;
    if (bytes != static_cast<quint64>(expected.bytes) || ticks < unixOffset
        || (ticks - unixOffset) / 10000 != static_cast<quint64>(expected.modified)) return false;
    *result = info;
    return true;
}
bool sameHandleInformation(const BY_HANDLE_FILE_INFORMATION& a, const BY_HANDLE_FILE_INFORMATION& b)
{
    return handleIdentity(a) == handleIdentity(b) && a.dwFileAttributes == b.dwFileAttributes
        && a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow == b.nFileSizeLow
        && a.nNumberOfLinks == b.nNumberOfLinks
        && a.ftCreationTime.dwHighDateTime == b.ftCreationTime.dwHighDateTime
        && a.ftCreationTime.dwLowDateTime == b.ftCreationTime.dwLowDateTime
        && a.ftLastWriteTime.dwHighDateTime == b.ftLastWriteTime.dwHighDateTime
        && a.ftLastWriteTime.dwLowDateTime == b.ftLastWriteTime.dwLowDateTime;
}
bool handleFingerprint(HANDLE handle, const File& expected, const QString& hash,
                       const std::atomic_bool& cancelled, QString* identity)
{
    BY_HANDLE_FILE_INFORMATION before{}, after{};
    LARGE_INTEGER zero{};
    if (!identity || cancelled.load() || !handleInformation(handle, expected, &before)
        || !SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) return false;
    QCryptographicHash digest(QCryptographicHash::Sha256);
    QByteArray buffer(64 * 1024, '\0');
    qint64 remaining = expected.bytes;
    while (remaining > 0) {
        DWORD read = 0;
        const DWORD wanted = static_cast<DWORD>(qMin<qint64>(remaining, buffer.size()));
        if (cancelled.load() || !ReadFile(handle, buffer.data(), wanted, &read, nullptr)
            || read == 0 || read > wanted) return false;
        digest.addData(buffer.constData(), static_cast<int>(read));
        remaining -= read;
    }
    DWORD extra = 0;
    if (cancelled.load() || !ReadFile(handle, buffer.data(), 1, &extra, nullptr) || extra != 0
        || !handleInformation(handle, expected, &after) || !sameHandleInformation(before, after)
        || QString::fromLatin1(digest.result().toHex()) != hash) return false;
    *identity = handleIdentity(after);
    return true;
}
#endif
} // namespace

bool FleetEventArchiveMaintenanceWorker::requestValid(const QVariantMap& request)
{
    if (!text(request.value(QStringLiteral("kind")))) return false;
    const QString kind = request.value(QStringLiteral("kind")).toString();
    if (purgeKind(kind)) {
        // Shape checking uses the largest existing policy domain; the worker
        // rechecks the same old backup result under its actual fixed policy.
        const QVariantMap ceiling{{QStringLiteral("maximumEvents"), 1000000},
            {QStringLiteral("maximumDatabaseMiB"), 4096}, {QStringLiteral("maximumQueueEvents"), 4096},
            {QStringLiteral("maximumQueueMiB"), 64}, {QStringLiteral("batchEventLimit"), 128},
            {QStringLiteral("busyTimeoutMs"), 1000}, {QStringLiteral("maximumHistoryMiB"), 8}};
        if (request.size() != (kind == QStringLiteral("purge-preview") ? 3 : 4)
            || !text(request.value(QStringLiteral("operationId"))) || !uuid(request.value(QStringLiteral("operationId")).toString())
            || request.value(QStringLiteral("basis")).type() != QVariant::Map || request.value(QStringLiteral("basis")).isNull()
            || !purgeBasisValid(request.value(QStringLiteral("basis")).toMap(), ceiling)) return false;
        return kind == QStringLiteral("purge-preview")
            || (request.value(QStringLiteral("proof")).type() == QVariant::Map && !request.value(QStringLiteral("proof")).isNull()
                && purgeProofValid(request.value(QStringLiteral("proof")).toMap(), request.value(QStringLiteral("basis")).toMap()));
    }
    if (kind == QStringLiteral("inventory")) return request.size() == 1;
    if (kind == QStringLiteral("backup-catalog") || kind == QStringLiteral("backup-diagnostics")) return request.size() == 2
        && text(request.value(QStringLiteral("destinationDirectory")))
        && destinationSyntax(request.value(QStringLiteral("destinationDirectory")).toString());
    if (kind == QStringLiteral("import")) return request.size() == 5
        && request.value(QStringLiteral("source")).type() == QVariant::Map && !request.value(QStringLiteral("source")).isNull()
        && backupDescriptorValid(request.value(QStringLiteral("source")).toMap())
        && text(request.value(QStringLiteral("sourceDirectory"))) && destinationSyntax(request.value(QStringLiteral("sourceDirectory")).toString())
        && text(request.value(QStringLiteral("expectedDirectoryIdentity"))) && directoryIdentity(request.value(QStringLiteral("expectedDirectoryIdentity")).toString())
        && text(request.value(QStringLiteral("operationId"))) && uuid(request.value(QStringLiteral("operationId")).toString());
    const bool backup = kind == QStringLiteral("backup");
    if (request.size() != 5 || (!backup && kind != QStringLiteral("retire") && kind != QStringLiteral("restore"))
        || request.value(QStringLiteral("source")).type() != QVariant::Map || request.value(QStringLiteral("source")).isNull()
        || !text(request.value(QStringLiteral("expectedDirectoryIdentity")))
        || !directoryIdentity(request.value(QStringLiteral("expectedDirectoryIdentity")).toString())
        || !text(request.value(QStringLiteral("operationId"))) || !uuid(request.value(QStringLiteral("operationId")).toString())) return false;
    if (backup) {
        return text(request.value(QStringLiteral("destinationDirectory")))
            && destinationSyntax(request.value(QStringLiteral("destinationDirectory")).toString())
            && descriptorValid(request.value(QStringLiteral("source")).toMap());
    }
    if (request.value(QStringLiteral("maximumSealedSegments")).type() != QVariant::Int
        || !integerIn(request.value(QStringLiteral("maximumSealedSegments")), 1, 128)) return false;
    File file;
    return descriptorValid(request.value(QStringLiteral("source")).toMap(), &file)
        && file.kind == (kind == QStringLiteral("retire") ? QStringLiteral("sealed") : QStringLiteral("retired"));
}

bool FleetEventArchiveMaintenanceWorker::resultMatches(const QVariantMap& result, const QVariantMap& request,
                                                       const QVariantMap& policy)
{
    if (!requestValid(request) || !policyValid(policy) || !commonMatches(result)
        || result.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || result.value(QStringLiteral("schemaVersion")).toInt() != 1 || !text(result.value(QStringLiteral("format")))
        || jsonBytes(result) > qMin(kResultMaximum, policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * kMiB - 2048)) return false;
    if (purgeKind(request.value(QStringLiteral("kind")).toString())) {
        const bool preview = request.value(QStringLiteral("kind")).toString() == QStringLiteral("purge-preview");
        const QVariantMap basis = request.value(QStringLiteral("basis")).toMap();
        if (result.size() != 41 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-retired-purge")
            || !purgeBasisValid(basis, policy) || result.value(QStringLiteral("basis")).type() != QVariant::Map
            || result.value(QStringLiteral("basis")).isNull() || result.value(QStringLiteral("basis")) != request.value(QStringLiteral("basis"))
            || result.value(QStringLiteral("proof")).type() != QVariant::Map || result.value(QStringLiteral("proof")).isNull()) return false;
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId")})
            if (!text(result.value(key)) || result.value(key) != request.value(key)) return false;
        for (const QString& key : {QStringLiteral("previewVerified"), QStringLiteral("nativeDeleteSupported"),
            QStringLiteral("decisionFinal"), QStringLiteral("deleteAttempted"), QStringLiteral("deleteReportedSuccess"),
            QStringLiteral("mayHaveDeleted"), QStringLiteral("sourceAbsentObserved"), QStringLiteral("backupAfterVerified"),
            QStringLiteral("sourceHandleClosed"), QStringLiteral("backupHandleClosed"), QStringLiteral("cancelObserved"),
            QStringLiteral("receiptCreateAttempted"), QStringLiteral("receiptCreated"), QStringLiteral("intentWritten"),
            QStringLiteral("resultWritten"), QStringLiteral("receiptClosed")})
            if (result.value(key).type() != QVariant::Bool || result.value(key).isNull()) return false;
        if (!bit(result.value(QStringLiteral("diskSpaceReclaimedVerified")), false)
            || !text(result.value(QStringLiteral("sourceLogicalBytes")))
            || result.value(QStringLiteral("sourceLogicalBytes")) != basis.value(QStringLiteral("source")).toMap().value(QStringLiteral("fileBytes"))
            || !text(result.value(QStringLiteral("errorCode"))) || !purgeError(result.value(QStringLiteral("errorCode")).toString())
            || !text(result.value(QStringLiteral("receiptError"))) || !purgeReceiptError(result.value(QStringLiteral("receiptError")).toString())
            || !text(result.value(QStringLiteral("receiptBasename")))) return false;
        for (const QString& key : {QStringLiteral("observedDirectoryIdentity"), QStringLiteral("destinationDirectoryIdentity")})
            if (!text(result.value(key)) || (!result.value(key).toString().isEmpty() && !directoryIdentity(result.value(key).toString()))) return false;
        const bool decision = result.value(QStringLiteral("decisionFinal")).toBool();
        const bool verified = result.value(QStringLiteral("previewVerified")).toBool();
        const bool created = result.value(QStringLiteral("receiptCreated")).toBool();
        const bool intent = result.value(QStringLiteral("intentWritten")).toBool();
        const bool proofOk = purgeProofValid(result.value(QStringLiteral("proof")).toMap(), basis);
        if (decision != result.value(QStringLiteral("deleteAttempted")).toBool()
            || decision != result.value(QStringLiteral("mayHaveDeleted")).toBool()
            || (result.value(QStringLiteral("deleteReportedSuccess")).toBool() && !decision)
            || (result.value(QStringLiteral("sourceAbsentObserved")).toBool() && !decision)
            || (result.value(QStringLiteral("backupAfterVerified")).toBool() && !decision)
            || (created && !result.value(QStringLiteral("receiptCreateAttempted")).toBool())
            || (intent && !created) || (result.value(QStringLiteral("resultWritten")).toBool() && !intent)
            || (result.value(QStringLiteral("receiptClosed")).toBool() && !created)
            || (!result.value(QStringLiteral("receiptError")).toString().isEmpty() && !result.value(QStringLiteral("receiptCreateAttempted")).toBool())
            || (decision && (!intent || !proofOk || !result.value(QStringLiteral("nativeDeleteSupported")).toBool()))
            || ((decision || verified) && (result.value(QStringLiteral("observedDirectoryIdentity")) != basis.value(QStringLiteral("expectedDirectoryIdentity"))
                || result.value(QStringLiteral("destinationDirectoryIdentity")) != basis.value(QStringLiteral("destinationDirectoryIdentity"))))
            || (verified && (!proofOk || !result.value(QStringLiteral("nativeDeleteSupported")).toBool()
                || !result.value(QStringLiteral("sourceHandleClosed")).toBool() || !result.value(QStringLiteral("backupHandleClosed")).toBool()))) return false;
        if (preview) {
            if (decision || result.value(QStringLiteral("deleteReportedSuccess")).toBool()
                || result.value(QStringLiteral("sourceAbsentObserved")).toBool() || result.value(QStringLiteral("backupAfterVerified")).toBool()
                || !result.value(QStringLiteral("receiptBasename")).toString().isEmpty()
                || result.value(QStringLiteral("receiptCreateAttempted")).toBool() || created || intent
                || result.value(QStringLiteral("resultWritten")).toBool() || result.value(QStringLiteral("receiptClosed")).toBool()
                || !result.value(QStringLiteral("receiptError")).toString().isEmpty()
                || (!verified && !result.value(QStringLiteral("proof")).toMap().isEmpty())) return false;
            return verified ? result.value(QStringLiteral("errorCode")).toString().isEmpty()
                : !result.value(QStringLiteral("errorCode")).toString().isEmpty();
        }
        if (result.value(QStringLiteral("proof")) != request.value(QStringLiteral("proof"))
            || result.value(QStringLiteral("receiptBasename")).toString() != purgeReceiptName(request.value(QStringLiteral("operationId")).toString())) return false;
        if (!result.value(QStringLiteral("errorCode")).toString().isEmpty()) return true;
        return decision && result.value(QStringLiteral("deleteReportedSuccess")).toBool()
            && result.value(QStringLiteral("sourceAbsentObserved")).toBool() && result.value(QStringLiteral("backupAfterVerified")).toBool()
            && verified && result.value(QStringLiteral("resultWritten")).toBool() && result.value(QStringLiteral("receiptClosed")).toBool()
            && result.value(QStringLiteral("receiptError")).toString().isEmpty();
    }
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("import")) {
        if (result.size() != 38 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-backup-import")) return false;
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("sourceDirectory"), QStringLiteral("expectedDirectoryIdentity")})
            if (!text(result.value(key)) || result.value(key) != request.value(key)) return false;
        File source;
        if (result.value(QStringLiteral("source")).type() != QVariant::Map || result.value(QStringLiteral("source")) != request.value(QStringLiteral("source"))
            || !backupDescriptorValid(request.value(QStringLiteral("source")).toMap(), &source)
            || source.bytes > policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB
            || !text(result.value(QStringLiteral("destinationDirectory"))) || !destinationSyntax(result.value(QStringLiteral("destinationDirectory")).toString())
            || !text(result.value(QStringLiteral("stagingBasename"))) || !text(result.value(QStringLiteral("targetBasename")))
            || result.value(QStringLiteral("stagingBasename")).toString() != backupName(source, request.value(QStringLiteral("operationId")).toString())
            || result.value(QStringLiteral("targetBasename")).toString() != importTargetName(source)
            || !text(result.value(QStringLiteral("bytesCopied"))) || !text(result.value(QStringLiteral("errorCode")))
            || !importError(result.value(QStringLiteral("errorCode")).toString())
            || result.value(QStringLiteral("targetDescriptor")).type() != QVariant::Map || result.value(QStringLiteral("targetDescriptor")).isNull()) return false;
        qint64 copied = 0;
        if (!decimal(result.value(QStringLiteral("bytesCopied")).toString(), 0, &copied) || copied > source.bytes) return false;
        for (const QString& key : {QStringLiteral("observedDirectoryIdentity"), QStringLiteral("destinationDirectoryIdentity")})
            if (!text(result.value(key)) || (!result.value(key).toString().isEmpty() && !directoryIdentity(result.value(key).toString()))) return false;
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("stagingCreateAttempted"), QStringLiteral("stagingCreated"),
            QStringLiteral("mayHaveWritten"), QStringLiteral("copyCompleted"), QStringLiteral("stagingVerified"), QStringLiteral("publishAttempted"),
            QStringLiteral("publishReportedSuccess"), QStringLiteral("mayHavePublished"), QStringLiteral("importVerified"), QStringLiteral("cancelObserved")})
            if (result.value(key).type() != QVariant::Bool || result.value(key).isNull()) return false;
        const bool decision = result.value(QStringLiteral("decisionFinal")).toBool(), created = result.value(QStringLiteral("stagingCreated")).toBool();
        const bool complete = result.value(QStringLiteral("copyCompleted")).toBool(), staged = result.value(QStringLiteral("stagingVerified")).toBool();
        const bool published = result.value(QStringLiteral("publishAttempted")).toBool(), verified = result.value(QStringLiteral("importVerified")).toBool();
        if (decision != result.value(QStringLiteral("stagingCreateAttempted")).toBool() || decision != result.value(QStringLiteral("mayHaveWritten")).toBool()
            || (created && !decision) || (copied > 0 && !created) || (complete && (!created || copied != source.bytes))
            || (staged && !complete) || published != result.value(QStringLiteral("mayHavePublished")).toBool()
            || (published && !staged) || (result.value(QStringLiteral("publishReportedSuccess")).toBool() && !published)
            || (verified && !published) || (decision && (result.value(QStringLiteral("observedDirectoryIdentity")) != request.value(QStringLiteral("expectedDirectoryIdentity"))
                || result.value(QStringLiteral("destinationDirectoryIdentity")).toString().isEmpty()))) return false;
        const QVariantMap target = result.value(QStringLiteral("targetDescriptor")).toMap();
        if (!verified) return target.isEmpty() && !result.value(QStringLiteral("errorCode")).toString().isEmpty();
        File targetFile;
        return result.value(QStringLiteral("errorCode")).toString().isEmpty() && descriptorValid(target, &targetFile)
            && targetFile.kind == QStringLiteral("retired") && targetFile.basename == importTargetName(source)
            && targetFile.epoch == source.epoch && targetFile.run == source.run && targetFile.bytes == source.bytes
            && target.value(QStringLiteral("fileFingerprint")) == request.value(QStringLiteral("source")).toMap().value(QStringLiteral("fileFingerprint"));
    }
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup-diagnostics")) {
        if (result.size() != 23 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-backup-diagnostics")
            || !text(result.value(QStringLiteral("destinationDirectory")))
            || result.value(QStringLiteral("destinationDirectory")) != request.value(QStringLiteral("destinationDirectory"))
            || !text(result.value(QStringLiteral("directoryIdentity"))) || !directoryIdentity(result.value(QStringLiteral("directoryIdentity")).toString())
            || result.value(QStringLiteral("entries")).type() != QVariant::List || result.value(QStringLiteral("entries")).isNull()
            || !bit(result.value(QStringLiteral("enumerationCompleted")), true)
            || !bit(result.value(QStringLiteral("originalSourceVerified")), false)
            || !bit(result.value(QStringLiteral("backupSetCompletenessVerified")), false)) return false;
        for (const QString& key : {QStringLiteral("candidateCount"), QStringLiteral("readableCount"), QStringLiteral("rejectedCount")})
            if (result.value(key).type() != QVariant::Int || !integerIn(result.value(key), 0, kMaximumFiles)) return false;
        const QVariantList entries = result.value(QStringLiteral("entries")).toList();
        if (entries.size() != result.value(QStringLiteral("candidateCount")).toInt() || entries.size() > kMaximumFiles) return false;
        QString previous;
        int readable = 0, rejected = 0;
        for (const QVariant& entry : entries) {
            if (entry.type() != QVariant::Map || entry.isNull()) return false;
            const QVariantMap row = entry.toMap();
            if (row.size() != 4 || !text(row.value(QStringLiteral("basename"))) || !text(row.value(QStringLiteral("status")))
                || !text(row.value(QStringLiteral("errorCode"))) || row.value(QStringLiteral("descriptor")).type() != QVariant::Map
                || row.value(QStringLiteral("descriptor")).isNull()) return false;
            const QString name = row.value(QStringLiteral("basename")).toString();
            if (!diagnosticBasename(name) || !name.startsWith(QStringLiteral("fleet-events-backup-"), Qt::CaseInsensitive)
                || (!previous.isEmpty() && previous >= name)) return false;
            previous = name;
            if (row.value(QStringLiteral("status")).toString() == QStringLiteral("readable")) {
                File file;
                if (!row.value(QStringLiteral("errorCode")).toString().isEmpty()
                    || !backupDescriptorValid(row.value(QStringLiteral("descriptor")).toMap(), &file)
                    || file.basename != name || file.bytes > policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB) return false;
                ++readable;
            } else if (row.value(QStringLiteral("status")).toString() == QStringLiteral("rejected")) {
                if (!row.value(QStringLiteral("descriptor")).toMap().isEmpty() || !diagnosticReason(row.value(QStringLiteral("errorCode")).toString())) return false;
                ++rejected;
            } else return false;
        }
        return readable == result.value(QStringLiteral("readableCount")).toInt()
            && rejected == result.value(QStringLiteral("rejectedCount")).toInt()
            && readable + rejected == entries.size();
    }
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup-catalog")) {
        if (result.size() != 21 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-backup-catalog")
            || !text(result.value(QStringLiteral("destinationDirectory")))
            || result.value(QStringLiteral("destinationDirectory")) != request.value(QStringLiteral("destinationDirectory"))
            || !text(result.value(QStringLiteral("directoryIdentity"))) || !directoryIdentity(result.value(QStringLiteral("directoryIdentity")).toString())
            || result.value(QStringLiteral("entries")).type() != QVariant::List || result.value(QStringLiteral("entries")).isNull()
            || result.value(QStringLiteral("fileCount")).type() != QVariant::Int || !integerIn(result.value(QStringLiteral("fileCount")), 0, kMaximumFiles)
            || !bit(result.value(QStringLiteral("enumerationCompleted")), true)
            || !bit(result.value(QStringLiteral("originalSourceVerified")), false)
            || !bit(result.value(QStringLiteral("backupSetCompletenessVerified")), false)) return false;
        const QVariantList entries = result.value(QStringLiteral("entries")).toList();
        if (entries.size() != result.value(QStringLiteral("fileCount")).toInt() || entries.size() > kMaximumFiles) return false;
        QString previous;
        for (const QVariant& entry : entries) {
            File file;
            if (entry.type() != QVariant::Map || entry.isNull() || !backupDescriptorValid(entry.toMap(), &file)
                || file.bytes > policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB
                || (!previous.isEmpty() && previous >= file.basename)) return false;
            previous = file.basename;
        }
        return true;
    }
    const bool inventory = request.value(QStringLiteral("kind")).toString() == QStringLiteral("inventory");
    if (inventory) {
        if (result.size() != 19 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-maintenance-inventory")
            || !text(result.value(QStringLiteral("directoryIdentity"))) || !directoryIdentity(result.value(QStringLiteral("directoryIdentity")).toString())
            || !bit(result.value(QStringLiteral("enumerationCompleted")), true) || result.value(QStringLiteral("entries")).type() != QVariant::List
            || result.value(QStringLiteral("entries")).isNull() || result.value(QStringLiteral("sealedCount")).type() != QVariant::Int
            || result.value(QStringLiteral("retiredCount")).type() != QVariant::Int
            || !integerIn(result.value(QStringLiteral("sealedCount")), 0, 128)
            || !integerIn(result.value(QStringLiteral("retiredCount")), 0, 128)) return false;
        const QVariantList entries = result.value(QStringLiteral("entries")).toList();
        if (entries.size() > 256) return false;
        QSet<QString> names;
        int sealed = 0, retired = 0;
        for (const QVariant& entry : entries) {
            File file;
            if (entry.type() != QVariant::Map || entry.isNull() || !descriptorValid(entry.toMap(), &file)
                || file.bytes > policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB
                || names.contains(file.basename)) return false;
            names.insert(file.basename);
            if (file.kind == QStringLiteral("sealed")) ++sealed; else ++retired;
        }
        return sealed == result.value(QStringLiteral("sealedCount")).toInt()
            && retired == result.value(QStringLiteral("retiredCount")).toInt();
    }
    if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup")) {
        if (result.size() != 31 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-maintenance-backup")) return false;
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("expectedDirectoryIdentity"),
            QStringLiteral("destinationDirectory")}) if (!text(result.value(key)) || result.value(key) != request.value(key)) return false;
        File file;
        if (result.value(QStringLiteral("source")).type() != QVariant::Map || result.value(QStringLiteral("source")) != request.value(QStringLiteral("source"))
            || !descriptorValid(request.value(QStringLiteral("source")).toMap(), &file)
            || !text(result.value(QStringLiteral("targetBasename")))
            || result.value(QStringLiteral("targetBasename")).toString() != backupName(file, request.value(QStringLiteral("operationId")).toString())
            || !text(result.value(QStringLiteral("bytesCopied"))) || !text(result.value(QStringLiteral("errorCode")))
            || !backupError(result.value(QStringLiteral("errorCode")).toString())) return false;
        qint64 copied = 0;
        if (!decimal(result.value(QStringLiteral("bytesCopied")).toString(), 0, &copied) || copied > file.bytes) return false;
        for (const QString& key : {QStringLiteral("observedDirectoryIdentity"), QStringLiteral("destinationDirectoryIdentity")})
            if (!text(result.value(key)) || (!result.value(key).toString().isEmpty() && !directoryIdentity(result.value(key).toString()))) return false;
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("targetCreateAttempted"), QStringLiteral("targetCreated"),
            QStringLiteral("mayHaveWritten"), QStringLiteral("copyCompleted"), QStringLiteral("backupVerified"), QStringLiteral("cancelObserved")})
            if (result.value(key).type() != QVariant::Bool || result.value(key).isNull()) return false;
        const bool decision = result.value(QStringLiteral("decisionFinal")).toBool();
        const bool created = result.value(QStringLiteral("targetCreated")).toBool();
        const bool complete = result.value(QStringLiteral("copyCompleted")).toBool();
        const bool verified = result.value(QStringLiteral("backupVerified")).toBool();
        if (decision != result.value(QStringLiteral("targetCreateAttempted")).toBool() || decision != result.value(QStringLiteral("mayHaveWritten")).toBool()
            || (created && !decision) || (copied > 0 && !created) || (complete && (!created || copied != file.bytes))
            || (verified && !complete) || (decision && (result.value(QStringLiteral("observedDirectoryIdentity")) != request.value(QStringLiteral("expectedDirectoryIdentity"))
                || result.value(QStringLiteral("destinationDirectoryIdentity")).toString().isEmpty()))) return false;
        const QString code = result.value(QStringLiteral("errorCode")).toString();
        return verified ? code.isEmpty() : !code.isEmpty();
    }
    if (result.size() != 27 || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-archive-maintenance-move")) return false;
    for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("expectedDirectoryIdentity")})
        if (!text(result.value(key)) || result.value(key) != request.value(key)) return false;
    if (result.value(QStringLiteral("source")).type() != QVariant::Map
        || result.value(QStringLiteral("source")) != request.value(QStringLiteral("source"))) return false;
    File file;
    if (!descriptorValid(request.value(QStringLiteral("source")).toMap(), &file)
        || !text(result.value(QStringLiteral("targetBasename"))) || result.value(QStringLiteral("targetBasename")).toString() != targetName(file)
        || !text(result.value(QStringLiteral("observedDirectoryIdentity")))
        || (!result.value(QStringLiteral("observedDirectoryIdentity")).toString().isEmpty()
            && !directoryIdentity(result.value(QStringLiteral("observedDirectoryIdentity")).toString()))
        || !text(result.value(QStringLiteral("errorCode"))) || !fixedError(result.value(QStringLiteral("errorCode")).toString())) return false;
    for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("renameAttempted"), QStringLiteral("renameReportedSuccess"),
        QStringLiteral("mayHaveMoved"), QStringLiteral("moveConfirmed"), QStringLiteral("cancelObserved")})
        if (result.value(key).type() != QVariant::Bool || result.value(key).isNull()) return false;
    const bool decision = result.value(QStringLiteral("decisionFinal")).toBool();
    const bool attempted = result.value(QStringLiteral("renameAttempted")).toBool();
    const bool reported = result.value(QStringLiteral("renameReportedSuccess")).toBool();
    const bool may = result.value(QStringLiteral("mayHaveMoved")).toBool();
    const bool confirmed = result.value(QStringLiteral("moveConfirmed")).toBool();
    const QString code = result.value(QStringLiteral("errorCode")).toString();
    if (decision != attempted || may != attempted || (reported && !attempted)
        || (decision && result.value(QStringLiteral("observedDirectoryIdentity")) != request.value(QStringLiteral("expectedDirectoryIdentity"))) return false;
    if (confirmed) return decision && code.isEmpty();
    return !code.isEmpty() && (!code.endsWith(QStringLiteral("cancelled")) || !decision);
}

FleetEventArchiveMaintenanceWorker::FleetEventArchiveMaintenanceWorker(const QString& directory,
    const QVariantMap& policy, const QString& requestId, const QVariantMap& request)
    : QThread(nullptr), _directory(QDir::fromNativeSeparators(directory)), _policy(policy),
      _requestId(identifier(requestId) ? requestId : QStringLiteral("")), _request(request),
      _configurationValid(policyValid(policy) && identifier(requestId) && requestValid(request))
{
    _status = common();
    _status.insert(QStringLiteral("requestId"), _requestId); _status.insert(QStringLiteral("state"), QStringLiteral("created"));
    _status.insert(QStringLiteral("errorCode"), QStringLiteral("")); _status.insert(QStringLiteral("decisionFinal"), false);
    if (purgeKind(request.value(QStringLiteral("kind")).toString())) {
        for (const QString& key : {QStringLiteral("deleteAttempted"), QStringLiteral("deleteReportedSuccess"),
            QStringLiteral("mayHaveDeleted"), QStringLiteral("sourceAbsentObserved"), QStringLiteral("backupAfterVerified"),
            QStringLiteral("sourceHandleClosed"), QStringLiteral("backupHandleClosed")}) _status.insert(key, false);
    } else if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("import")) {
        for (const QString& key : {QStringLiteral("stagingCreateAttempted"), QStringLiteral("stagingCreated"), QStringLiteral("mayHaveWritten"),
            QStringLiteral("copyCompleted"), QStringLiteral("stagingVerified"), QStringLiteral("publishAttempted"),
            QStringLiteral("publishReportedSuccess"), QStringLiteral("mayHavePublished"), QStringLiteral("importVerified")}) _status.insert(key, false);
        _status.insert(QStringLiteral("bytesCopied"), QStringLiteral("0"));
    } else if (request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup")) {
        for (const QString& key : {QStringLiteral("targetCreateAttempted"), QStringLiteral("targetCreated"), QStringLiteral("mayHaveWritten"),
            QStringLiteral("copyCompleted"), QStringLiteral("backupVerified")}) _status.insert(key, false);
        _status.insert(QStringLiteral("bytesCopied"), QStringLiteral("0"));
    } else {
        _status.insert(QStringLiteral("mayHaveMoved"), false); _status.insert(QStringLiteral("moveConfirmed"), false);
    }
    _status.insert(QStringLiteral("cancelRequested"), false);
}
FleetEventArchiveMaintenanceWorker::~FleetEventArchiveMaintenanceWorker() { Q_ASSERT(!isRunning()); }
void FleetEventArchiveMaintenanceWorker::requestCancel()
{
    _cancelled.store(true);
    QMutexLocker lock(&_mutex);
    _status.insert(QStringLiteral("cancelRequested"), true);
}
QVariantMap FleetEventArchiveMaintenanceWorker::current() const
{
    QMutexLocker lock(&_mutex);
    return _status;
}
void FleetEventArchiveMaintenanceWorker::setState(const QString& state, const QString& error)
{
    QMutexLocker lock(&_mutex);
    _status.insert(QStringLiteral("state"), state);
    _status.insert(QStringLiteral("errorCode"), error.isNull() ? QStringLiteral("") : error);
}
void FleetEventArchiveMaintenanceWorker::run()
{
    if (purgeKind(_request.value(QStringLiteral("kind")).toString())) { runPurge(); return; }
    if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("import")) { runImport(); return; }
    // Independent backup reads must remain usable when the original source
    // directory/database no longer exists. Never enter the source-lock path.
    if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup-diagnostics")) { runBackupDiagnostics(); return; }
    if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup-catalog")) { runBackupCatalog(); return; }
    if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("backup")) { runBackup(); return; }
    setState(QStringLiteral("opening"));
    const bool inventory = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("inventory");
    QString error = QStringLiteral("");
    QVariantMap result = common();
    result.insert(QStringLiteral("schemaVersion"), 1);
    File source;
    QString target;
    if (!inventory) {
        descriptorValid(_request.value(QStringLiteral("source")).toMap(), &source);
        target = targetName(source);
        result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-maintenance-move"));
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("source"), QStringLiteral("expectedDirectoryIdentity")})
            result.insert(key, _request.value(key));
        result.insert(QStringLiteral("targetBasename"), target);
        result.insert(QStringLiteral("observedDirectoryIdentity"), QStringLiteral(""));
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("renameAttempted"), QStringLiteral("renameReportedSuccess"),
            QStringLiteral("mayHaveMoved"), QStringLiteral("moveConfirmed"), QStringLiteral("cancelObserved")}) result.insert(key, false);
        result.insert(QStringLiteral("errorCode"), QStringLiteral(""));
    }
    if (!_configurationValid) error = QStringLiteral("archive-maintenance-configuration-rejected");
    else if (!localDirectory(_directory)) error = QStringLiteral("archive-maintenance-directory-rejected");
    else if (!inventory && !nativeRenameSupported()) error = QStringLiteral("archive-maintenance-platform-unsupported");
    else if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty()) {
        // The only created/released file is this existing product cooperation
        // lock. Never open/create/fix fleet-events-v1.sqlite itself.
        const QString ownerPath = QDir(_directory).filePath(QStringLiteral("fleet-events-v1.sqlite.owner.lock"));
        const QFileInfo ownerInfo(ownerPath);
        if (ownerInfo.isSymLink() || (ownerInfo.exists() && (!ownerInfo.isFile() || ownerInfo.size() > 16384
            || QDir::fromNativeSeparators(ownerInfo.canonicalFilePath()) != ownerPath))) {
            error = QStringLiteral("archive-maintenance-owner-lock-path-rejected");
        } else {
            QLockFile owner(ownerPath);
            owner.setStaleLockTime(0);
            if (!owner.tryLock(0)) error = QStringLiteral("archive-maintenance-owner-lock-unavailable");
            else {
                setState(QStringLiteral("inspecting"));
                const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
                Listing before;
                if (!listing(_directory, maximum, _cancelled, &before)) error = QStringLiteral("archive-maintenance-inventory-rejected");
                if (inventory && error.isEmpty()) {
                    QVariantList entries;
                    for (const File& file : before.files) {
                        QVariantMap entry;
                        if (!inspectFile(_directory, file, _policy, _cancelled, &entry)) {
                            error = QStringLiteral("archive-maintenance-source-changed"); break;
                        }
                        entries.append(entry);
                    }
                    Listing after;
                    if (error.isEmpty() && (!listing(_directory, maximum, _cancelled, &after) || !sameListing(before, after)))
                        error = QStringLiteral("archive-maintenance-directory-changed");
                    if (error.isEmpty()) {
                        result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-maintenance-inventory"));
                        result.insert(QStringLiteral("directoryIdentity"), before.identity); result.insert(QStringLiteral("entries"), entries);
                        result.insert(QStringLiteral("sealedCount"), before.sealed); result.insert(QStringLiteral("retiredCount"), before.retired);
                        result.insert(QStringLiteral("enumerationCompleted"), true);
                    }
                } else if (!inventory && error.isEmpty()) {
                    result.insert(QStringLiteral("observedDirectoryIdentity"), before.identity);
                    if (before.identity != _request.value(QStringLiteral("expectedDirectoryIdentity")).toString())
                        error = QStringLiteral("archive-maintenance-directory-changed");
                    bool found = false;
                    for (const File& file : before.files) if (file.basename == source.basename
                        && file.bytes == source.bytes && file.modified == source.modified) found = true;
                    if (error.isEmpty() && !found) error = QStringLiteral("archive-maintenance-source-changed");
                    if (error.isEmpty() && !absent(_directory, target)) error = QStringLiteral("archive-maintenance-target-exists");
                    const bool retire = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("retire");
                    if (error.isEmpty() && (retire ? before.retired >= kMaximumFiles
                        : before.sealed >= _request.value(QStringLiteral("maximumSealedSegments")).toInt()))
                        error = QStringLiteral("archive-maintenance-capacity-rejected");
                    QVariantMap actual;
                    if (error.isEmpty() && (!inspectFile(_directory, source, _policy, _cancelled, &actual)
                        || actual != _request.value(QStringLiteral("source")).toMap())) error = QStringLiteral("archive-maintenance-source-changed");
                    Listing finalBefore;
                    if (error.isEmpty() && (!listing(_directory, maximum, _cancelled, &finalBefore)
                        || !sameListing(before, finalBefore) || !absent(_directory, target)))
                        error = QStringLiteral("archive-maintenance-directory-changed");
                    if (error.isEmpty()) {
                        QMutexLocker lock(&_mutex);
                        if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
                        else {
                            // After this decision cancellation never short-cuts
                            // post-observation or suggests an automatic rollback.
                            result.insert(QStringLiteral("decisionFinal"), true);
                            result.insert(QStringLiteral("renameAttempted"), true);
                            result.insert(QStringLiteral("mayHaveMoved"), true);
                            _status.insert(QStringLiteral("state"), QStringLiteral("moving"));
                            _status.insert(QStringLiteral("decisionFinal"), true); _status.insert(QStringLiteral("mayHaveMoved"), true);
                        }
                    }
                    if (result.value(QStringLiteral("decisionFinal")).toBool()) {
                        // Native no-replace only: no Qt copy/remove fallback,
                        // no deletion, no automatic repair or rollback.
                        const bool reported = nativeRenameNoReplace(_directory, source.basename, target);
                        result.insert(QStringLiteral("renameReportedSuccess"), reported);
                        setState(QStringLiteral("confirming"));
                        const std::atomic_bool observeDespiteCancel{false};
                        Listing postBefore, postAfter;
                        File moved = source; moved.basename = target;
                        moved.kind = retire ? QStringLiteral("retired") : QStringLiteral("sealed");
                        QVariantMap observed, expected = _request.value(QStringLiteral("source")).toMap();
                        expected.insert(QStringLiteral("basename"), target); expected.insert(QStringLiteral("kind"), moved.kind);
                        const bool observedTarget = listing(_directory, maximum, observeDespiteCancel, &postBefore)
                            && postBefore.identity == before.identity && absent(_directory, source.basename)
                            && inspectFile(_directory, moved, _policy, observeDespiteCancel, &observed) && observed == expected
                            && listing(_directory, maximum, observeDespiteCancel, &postAfter) && sameListing(postBefore, postAfter)
                            && absent(_directory, source.basename);
                        result.insert(QStringLiteral("moveConfirmed"), observedTarget);
                        if (!observedTarget) error = reported ? QStringLiteral("archive-maintenance-postcondition-unconfirmed")
                            : QStringLiteral("archive-maintenance-rename-unconfirmed");
                    }
                }
            } // owner lock covers all enumeration/hash/SQL/move/post-observation.
        }
    }
    if (inventory) {
        if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
        if (error.isEmpty() && !resultMatches(result, _request, _policy)) error = QStringLiteral("archive-maintenance-result-contract-rejected");
        if (!error.isEmpty()) {
            setState(_cancelled.load() ? QStringLiteral("cancelled") : QStringLiteral("failed"), error);
            emit failure(_requestId, error); return;
        }
    } else {
        const bool decided = result.value(QStringLiteral("decisionFinal")).toBool();
        if (!decided && _cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
        result.insert(QStringLiteral("cancelObserved"), _cancelled.load()); result.insert(QStringLiteral("errorCode"), error);
        // A valid move request always returns the uncertainty facts, including
        // failures. Never replace a mayHaveMoved result by a failure signal.
        if (_configurationValid && !resultMatches(result, _request, _policy)) {
            error = QStringLiteral("archive-maintenance-result-contract-rejected");
            result.insert(QStringLiteral("moveConfirmed"), false); result.insert(QStringLiteral("errorCode"), error);
        }
        {
            QMutexLocker lock(&_mutex);
            _status.insert(QStringLiteral("moveConfirmed"), result.value(QStringLiteral("moveConfirmed")));
        }
    }
    setState(QStringLiteral("result-submitted"), error);
    emit resultReady(_requestId, result);
}

void FleetEventArchiveMaintenanceWorker::runPurge()
{
    setState(QStringLiteral("opening"));
    const bool preview = _request.value(QStringLiteral("kind")).toString() == QStringLiteral("purge-preview");
    const QVariantMap basis = _request.value(QStringLiteral("basis")).toMap();
    const QVariantMap sourceDescriptor = basis.value(QStringLiteral("source")).toMap();
    const QString destination = basis.value(QStringLiteral("destinationDirectory")).toString();
    const QString operation = _request.value(QStringLiteral("operationId")).toString();
    File source;
    descriptorValid(sourceDescriptor, &source);
    QVariantMap result = common();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-retired-purge"));
    result.insert(QStringLiteral("schemaVersion"), 1);
    for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("basis")}) result.insert(key, _request.value(key));
    result.insert(QStringLiteral("proof"), preview ? QVariantMap{} : _request.value(QStringLiteral("proof")).toMap());
    for (const QString& key : {QStringLiteral("previewVerified"), QStringLiteral("decisionFinal"), QStringLiteral("deleteAttempted"),
        QStringLiteral("deleteReportedSuccess"), QStringLiteral("mayHaveDeleted"), QStringLiteral("sourceAbsentObserved"),
        QStringLiteral("backupAfterVerified"), QStringLiteral("sourceHandleClosed"), QStringLiteral("backupHandleClosed"),
        QStringLiteral("cancelObserved"), QStringLiteral("diskSpaceReclaimedVerified"), QStringLiteral("receiptCreateAttempted"),
        QStringLiteral("receiptCreated"), QStringLiteral("intentWritten"), QStringLiteral("resultWritten"), QStringLiteral("receiptClosed")}) result.insert(key, false);
    result.insert(QStringLiteral("nativeDeleteSupported"), nativePurgeSupported());
    result.insert(QStringLiteral("observedDirectoryIdentity"), QStringLiteral(""));
    result.insert(QStringLiteral("destinationDirectoryIdentity"), QStringLiteral(""));
    result.insert(QStringLiteral("sourceLogicalBytes"), sourceDescriptor.value(QStringLiteral("fileBytes")));
    result.insert(QStringLiteral("receiptBasename"), preview ? QStringLiteral("") : purgeReceiptName(operation));
    result.insert(QStringLiteral("receiptError"), QStringLiteral(""));
    result.insert(QStringLiteral("errorCode"), QStringLiteral(""));
    QString error = QStringLiteral("");
    const auto publishFacts = [this, &result]() {
        QMutexLocker lock(&_mutex);
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("deleteAttempted"), QStringLiteral("deleteReportedSuccess"),
            QStringLiteral("mayHaveDeleted"), QStringLiteral("sourceAbsentObserved"), QStringLiteral("backupAfterVerified"),
            QStringLiteral("sourceHandleClosed"), QStringLiteral("backupHandleClosed")}) _status.insert(key, result.value(key));
    };
    if (!_configurationValid || !purgeBasisValid(basis, _policy)) error = QStringLiteral("archive-purge-basis-rejected");
    else if (!nativePurgeSupported()) error = QStringLiteral("archive-maintenance-platform-unsupported");
    else if (!destinationSyntax(_directory) || !localDirectory(_directory) || !localDirectory(destination)
        || !disjointDirectories(_directory, destination)) error = QStringLiteral("archive-maintenance-directory-rejected");
    else if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
#if defined(Q_OS_WIN)
    if (error.isEmpty()) {
        const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
        const auto lockPathSafe = [](const QString& path) {
            const QFileInfo info(path);
            return !info.isSymLink() && (!info.exists() || (info.isFile() && info.size() <= 16 * 1024
                && QDir::fromNativeSeparators(info.canonicalFilePath()) == path));
        };
        const QString sourceLockPath = QDir(_directory).filePath(QStringLiteral("fleet-events-v1.sqlite.owner.lock"));
        const QString backupLockPath = QDir(destination).filePath(QStringLiteral("fleet-events-backup.owner.lock"));
        QLockFile sourceLock(sourceLockPath), backupLock(backupLockPath);
        sourceLock.setStaleLockTime(0); backupLock.setStaleLockTime(0);
        if (!lockPathSafe(sourceLockPath) || !lockPathSafe(backupLockPath)) error = QStringLiteral("archive-maintenance-owner-lock-path-rejected");
        else if (!sourceLock.tryLock(0) || !backupLock.tryLock(0)) error = QStringLiteral("archive-maintenance-owner-lock-unavailable");
        Listing sourceBefore, backupBefore;
        File backup;
        QVariantMap actualSource, actualBackup;
        PurgeHandle sourceHandle, backupHandle;
        QFile receipt;
        qint64 receiptBytes = 0;
        bool probed = false;
        const auto receiptFailure = [&result, &error](const QString& code) {
            if (result.value(QStringLiteral("receiptError")).toString().isEmpty()) result.insert(QStringLiteral("receiptError"), code);
            if (error.isEmpty()) error = code;
        };
        const auto writeReceipt = [&receipt, &receiptBytes, &receiptFailure](const QVariantMap& record) {
            const QByteArray bytes = QJsonDocument(QJsonObject::fromVariantMap(record)).toJson(QJsonDocument::Compact) + '\n';
            if (bytes.size() > 64 * 1024 || receiptBytes + bytes.size() > 128 * 1024) {
                receiptFailure(QStringLiteral("archive-purge-receipt-budget-rejected")); return false;
            }
            // A short write is not retried or reported as a complete record.
            const qint64 written = receipt.write(bytes);
            if (written > 0) receiptBytes += written;
            if (written != bytes.size() || receipt.error() != QFileDevice::NoError) {
                receiptFailure(QStringLiteral("archive-purge-receipt-write-failed")); return false;
            }
            if (!receipt.flush()) { receiptFailure(QStringLiteral("archive-purge-receipt-flush-failed")); return false; }
            return true;
        };
        if (error.isEmpty()) {
            setState(QStringLiteral("inspecting"));
            if (!listing(_directory, maximum, _cancelled, &sourceBefore)
                || !backupListing(destination, maximum, _cancelled, &backupBefore)) error = QStringLiteral("archive-maintenance-inventory-rejected");
            else {
                result.insert(QStringLiteral("observedDirectoryIdentity"), sourceBefore.identity);
                result.insert(QStringLiteral("destinationDirectoryIdentity"), backupBefore.identity);
                if (sourceBefore.identity != basis.value(QStringLiteral("expectedDirectoryIdentity")).toString()
                    || backupBefore.identity != basis.value(QStringLiteral("destinationDirectoryIdentity")).toString()
                    || sourceBefore.identity == backupBefore.identity) error = QStringLiteral("archive-maintenance-directory-changed");
            }
        }
        if (error.isEmpty()) {
            bool foundSource = false, foundBackup = false;
            for (const File& entry : sourceBefore.files) if (entry.basename == source.basename)
                foundSource = entry.bytes == source.bytes && entry.modified == source.modified && entry.kind == QStringLiteral("retired");
            for (const File& entry : backupBefore.files) if (entry.basename == basis.value(QStringLiteral("targetBasename")).toString()) {
                backup = entry; foundBackup = true;
            }
            bool closeFailed = false;
            if (!foundSource || !foundBackup || !inspectFile(_directory, source, _policy, _cancelled, &actualSource, false, &closeFailed)
                || actualSource != sourceDescriptor) error = QStringLiteral("archive-maintenance-source-changed");
            else if (!inspectFile(destination, backup, _policy, _cancelled, &actualBackup, true, &closeFailed)) error = QStringLiteral("archive-purge-backup-changed");
            else {
                actualBackup.insert(QStringLiteral("backupOperationId"), backup.operation);
                if (!purgeBackupValid(actualBackup, basis)) error = QStringLiteral("archive-purge-backup-changed");
            }
            if (closeFailed && error.isEmpty()) error = QStringLiteral("archive-purge-handle-close-unconfirmed");
        }
        QString sourceIdentity, backupIdentity;
        if (error.isEmpty()) {
            // SQL and both ordinary QFile fingerprints are finished before the
            // DELETE-access handle exists. No Qt SQLite reopen under that handle.
            if (!sourceHandle.open(QDir(_directory).filePath(source.basename), !preview)
                || !backupHandle.open(QDir(destination).filePath(backup.basename), false)) error = QStringLiteral("archive-purge-handle-open-unconfirmed");
            else if (!handleFingerprint(sourceHandle.value(), source, sourceDescriptor.value(QStringLiteral("fileFingerprint")).toString(), _cancelled, &sourceIdentity)
                || !handleFingerprint(backupHandle.value(), backup, actualBackup.value(QStringLiteral("fileFingerprint")).toString(), _cancelled, &backupIdentity)
                || sourceIdentity == backupIdentity) error = QStringLiteral("archive-purge-proof-changed");
        }
        QVariantMap observedProof{{QStringLiteral("sourceFileIdentity"), sourceIdentity},
            {QStringLiteral("backupFileIdentity"), backupIdentity}, {QStringLiteral("backupDescriptor"), actualBackup}};
        if (error.isEmpty() && (!purgeProofValid(observedProof, basis)
            || (!preview && observedProof != _request.value(QStringLiteral("proof")).toMap()))) error = QStringLiteral("archive-purge-proof-changed");
        const auto pairStillCurrent = [&]() {
            Listing sourceNow, backupNow;
            QString currentSource, currentBackup;
            return listing(_directory, maximum, _cancelled, &sourceNow) && sameListing(sourceBefore, sourceNow)
                && backupListing(destination, maximum, _cancelled, &backupNow) && sameListing(backupBefore, backupNow)
                && handleFingerprint(sourceHandle.value(), source, sourceDescriptor.value(QStringLiteral("fileFingerprint")).toString(), _cancelled, &currentSource)
                && currentSource == sourceIdentity
                && handleFingerprint(backupHandle.value(), backup, actualBackup.value(QStringLiteral("fileFingerprint")).toString(), _cancelled, &currentBackup)
                && currentBackup == backupIdentity;
        };
        if (error.isEmpty() && !pairStillCurrent()) error = QStringLiteral("archive-maintenance-source-changed");
        if (error.isEmpty() && !preview) {
            receipt.setFileName(QDir(_directory).filePath(purgeReceiptName(operation)));
            // This new artifact is intentionally retained even after short
            // writes, cancellation or a failed deletion; never overwrite/retry.
            if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
            else {
                result.insert(QStringLiteral("receiptCreateAttempted"), true);
                if (!receipt.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Unbuffered))
                    receiptFailure(QStringLiteral("archive-purge-receipt-create-unconfirmed"));
            }
            if (receipt.isOpen()) {
                result.insert(QStringLiteral("receiptCreated"), true);
                const QVariantMap intent{{QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-retired-purge-receipt")},
                    {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("phase"), QStringLiteral("intent")},
                    {QStringLiteral("operationId"), operation}, {QStringLiteral("basis"), basis}, {QStringLiteral("proof"), observedProof},
                    {QStringLiteral("sourceDirectory"), _directory}, {QStringLiteral("sourcePath"), QDir(_directory).filePath(source.basename)},
                    {QStringLiteral("backupPath"), QDir(destination).filePath(backup.basename)},
                    {QStringLiteral("recordedAtUtcMs"), QString::number(QDateTime::currentMSecsSinceEpoch())},
                    {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
                    {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}};
                if (writeReceipt(intent)) result.insert(QStringLiteral("intentWritten"), true);
            }
        }
        if (error.isEmpty() && !pairStillCurrent()) error = QStringLiteral("archive-maintenance-source-changed");
        if (error.isEmpty()) {
            probed = true;
            if (!preview) {
                bool deleteNow = false;
                {
                    QMutexLocker lock(&_mutex);
                    if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
                    else {
                        result.insert(QStringLiteral("decisionFinal"), true);
                        result.insert(QStringLiteral("deleteAttempted"), true);
                        result.insert(QStringLiteral("mayHaveDeleted"), true);
                        _status.insert(QStringLiteral("decisionFinal"), true);
                        _status.insert(QStringLiteral("deleteAttempted"), true);
                        _status.insert(QStringLiteral("mayHaveDeleted"), true);
                        _status.insert(QStringLiteral("state"), QStringLiteral("deleting"));
                        deleteNow = true;
                    }
                }
                // The decision is already committed; the status/cancel mutex
                // must not be held across potentially slow native filesystem IO.
                if (deleteNow) {
                    FILE_DISPOSITION_INFO disposition{};
                    disposition.DeleteFile = TRUE;
                    const bool reported = SetFileInformationByHandle(sourceHandle.value(), FileDispositionInfo,
                        &disposition, sizeof(disposition)) != 0;
                    result.insert(QStringLiteral("deleteReportedSuccess"), reported);
                    { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("deleteReportedSuccess"), reported); }
                    if (!reported) error = QStringLiteral("archive-purge-delete-unconfirmed");
                }
            }
        }
        if (sourceHandle.valid()) {
            result.insert(QStringLiteral("sourceHandleClosed"), sourceHandle.close());
            if (!result.value(QStringLiteral("sourceHandleClosed")).toBool()) {
                probed = false;
                if (error.isEmpty()) error = QStringLiteral("archive-purge-handle-close-unconfirmed");
            }
        }
        if (result.value(QStringLiteral("deleteAttempted")).toBool()) {
            setState(QStringLiteral("confirming"));
            // Cancellation after disposition cannot erase the attempted write.
            // Keep the backup object fixed while observing normal directory EOF.
            const std::atomic_bool finishObservation{false};
            Listing expectedAfter = sourceBefore;
            for (int index = 0; index < expectedAfter.files.size(); ++index) if (expectedAfter.files.at(index).basename == source.basename) {
                expectedAfter.files.removeAt(index); --expectedAfter.retired; break;
            }
            Listing afterFirst, afterSecond, backupAfterFirst, backupAfterSecond;
            const bool sourceAbsent = result.value(QStringLiteral("sourceHandleClosed")).toBool()
                && listing(_directory, maximum, finishObservation, &afterFirst) && sameListing(expectedAfter, afterFirst)
                && absent(_directory, source.basename)
                && listing(_directory, maximum, finishObservation, &afterSecond) && sameListing(expectedAfter, afterSecond);
            result.insert(QStringLiteral("sourceAbsentObserved"), sourceAbsent);
            QString afterBackupIdentity;
            const bool backupVerified = backupListing(destination, maximum, finishObservation, &backupAfterFirst)
                && sameListing(backupBefore, backupAfterFirst) && backupHandle.valid()
                && handleFingerprint(backupHandle.value(), backup, actualBackup.value(QStringLiteral("fileFingerprint")).toString(), finishObservation, &afterBackupIdentity)
                && afterBackupIdentity == backupIdentity
                && backupListing(destination, maximum, finishObservation, &backupAfterSecond) && sameListing(backupBefore, backupAfterSecond);
            result.insert(QStringLiteral("backupAfterVerified"), backupVerified);
            if (error.isEmpty() && !sourceAbsent) error = QStringLiteral("archive-maintenance-postcondition-unconfirmed");
            if (error.isEmpty() && !backupVerified) error = QStringLiteral("archive-purge-backup-postcondition-unconfirmed");
        }
        if (backupHandle.valid()) {
            result.insert(QStringLiteral("backupHandleClosed"), backupHandle.close());
            if (!result.value(QStringLiteral("backupHandleClosed")).toBool()) {
                probed = false;
                if (error.isEmpty()) error = QStringLiteral("archive-purge-handle-close-unconfirmed");
            }
        }
        probed = probed && result.value(QStringLiteral("sourceHandleClosed")).toBool()
            && result.value(QStringLiteral("backupHandleClosed")).toBool();
        result.insert(QStringLiteral("previewVerified"), probed);
        if (preview && probed && error.isEmpty()) result.insert(QStringLiteral("proof"), observedProof);
        // A preview cancellation never publishes a usable proof, even if its
        // already-opened read handles were successfully closed.
        if (preview && (_cancelled.load() || !error.isEmpty())) {
            result.insert(QStringLiteral("previewVerified"), false);
            result.insert(QStringLiteral("proof"), QVariantMap{});
            if (error.isEmpty()) error = QStringLiteral("archive-maintenance-cancelled");
        }
        result.insert(QStringLiteral("cancelObserved"), _cancelled.load());
        result.insert(QStringLiteral("errorCode"), error);
        publishFacts();
        if (receipt.isOpen()) {
            if (result.value(QStringLiteral("intentWritten")).toBool()) {
                // This record freezes deletion/backup observations before its
                // own write/flush/close. It cannot certify its own persistence.
                const QVariantMap terminal{{QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-retired-purge-receipt")},
                    {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("phase"), QStringLiteral("result")},
                    {QStringLiteral("operationId"), operation}, {QStringLiteral("sourceDirectory"), _directory},
                    {QStringLiteral("sourcePath"), QDir(_directory).filePath(source.basename)},
                    {QStringLiteral("backupPath"), QDir(destination).filePath(backup.basename)},
                    {QStringLiteral("recordedAtUtcMs"), QString::number(QDateTime::currentMSecsSinceEpoch())},
                    {QStringLiteral("result"), result}, {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}};
                if (writeReceipt(terminal)) result.insert(QStringLiteral("resultWritten"), true);
            }
            receipt.close();
            if (receipt.error() == QFileDevice::NoError) result.insert(QStringLiteral("receiptClosed"), true);
            else receiptFailure(QStringLiteral("archive-purge-receipt-close-unconfirmed"));
        }
    }
#endif
    if (_cancelled.load() && error.isEmpty() && !result.value(QStringLiteral("decisionFinal")).toBool()) error = QStringLiteral("archive-maintenance-cancelled");
    if (preview && !error.isEmpty()) {
        result.insert(QStringLiteral("previewVerified"), false);
        result.insert(QStringLiteral("proof"), QVariantMap{});
    }
    result.insert(QStringLiteral("cancelObserved"), _cancelled.load());
    result.insert(QStringLiteral("errorCode"), error);
    publishFacts();
    // Preserve irreversible observations on every post-decision path. A pure
    // contract failure is diagnostic, never a claim that deletion did not occur.
    if (!resultMatches(result, _request, _policy)) {
        result.insert(QStringLiteral("errorCode"), QStringLiteral("archive-maintenance-result-contract-rejected"));
        if (!result.value(QStringLiteral("decisionFinal")).toBool() && !result.value(QStringLiteral("receiptCreateAttempted")).toBool()
            && !resultMatches(result, _request, _policy)) {
            setState(QStringLiteral("result-submitted"), QStringLiteral("archive-maintenance-result-contract-rejected"));
            emit failure(_requestId, QStringLiteral("archive-maintenance-result-contract-rejected")); return;
        }
    }
    setState(QStringLiteral("result-submitted"), result.value(QStringLiteral("errorCode")).toString());
    emit resultReady(_requestId, result);
}

void FleetEventArchiveMaintenanceWorker::runImport()
{
    setState(QStringLiteral("opening"));
    const QString empty = QStringLiteral("");
    const QVariantMap sourceDescriptor = _request.value(QStringLiteral("source")).toMap();
    File source;
    backupDescriptorValid(sourceDescriptor, &source);
    const QString sourceDirectory = _request.value(QStringLiteral("sourceDirectory")).toString();
    const QString staging = backupName(source, _request.value(QStringLiteral("operationId")).toString());
    const QString target = importTargetName(source), sibling = importSealedSibling(source);
    QVariantMap result = common();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-backup-import"));
    result.insert(QStringLiteral("schemaVersion"), 1);
    for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("source"),
        QStringLiteral("sourceDirectory"), QStringLiteral("expectedDirectoryIdentity")}) result.insert(key, _request.value(key));
    result.insert(QStringLiteral("observedDirectoryIdentity"), empty);
    result.insert(QStringLiteral("destinationDirectory"), _directory);
    result.insert(QStringLiteral("destinationDirectoryIdentity"), empty);
    result.insert(QStringLiteral("stagingBasename"), staging); result.insert(QStringLiteral("targetBasename"), target);
    result.insert(QStringLiteral("targetDescriptor"), QVariantMap{});
    result.insert(QStringLiteral("bytesCopied"), QStringLiteral("0"));
    result.insert(QStringLiteral("errorCode"), empty);
    for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("stagingCreateAttempted"), QStringLiteral("stagingCreated"),
        QStringLiteral("mayHaveWritten"), QStringLiteral("copyCompleted"), QStringLiteral("stagingVerified"), QStringLiteral("publishAttempted"),
        QStringLiteral("publishReportedSuccess"), QStringLiteral("mayHavePublished"), QStringLiteral("importVerified"), QStringLiteral("cancelObserved")}) result.insert(key, false);
    const auto publishFacts = [this, &result]() {
        QMutexLocker lock(&_mutex);
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("stagingCreateAttempted"), QStringLiteral("stagingCreated"),
            QStringLiteral("mayHaveWritten"), QStringLiteral("bytesCopied"), QStringLiteral("copyCompleted"), QStringLiteral("stagingVerified"),
            QStringLiteral("publishAttempted"), QStringLiteral("publishReportedSuccess"), QStringLiteral("mayHavePublished"), QStringLiteral("importVerified")})
            _status.insert(key, result.value(key));
    };
    const auto lockPathSafe = [](const QString& path) {
        const QFileInfo info(path);
        return !info.isSymLink() && (!info.exists() || (info.isFile() && info.size() <= 16384
            && QDir::fromNativeSeparators(info.canonicalFilePath()) == path));
    };
    const auto perform = [&]() -> QString {
        if (!_configurationValid) return QStringLiteral("archive-maintenance-configuration-rejected");
        if (!destinationSyntax(_directory) || !localDirectory(_directory) || QDir(_directory).isRoot()) return QStringLiteral("archive-import-target-rejected");
        if (!destinationSyntax(sourceDirectory) || !localDirectory(sourceDirectory) || QDir(sourceDirectory).isRoot()
            || !disjointDirectories(_directory, sourceDirectory)) return QStringLiteral("archive-import-source-rejected");
        if (!nativeRenameSupported()) return QStringLiteral("archive-maintenance-platform-unsupported");
        if (_cancelled.load()) return QStringLiteral("archive-maintenance-cancelled");
        const QString targetLockPath = QDir(_directory).filePath(QStringLiteral("fleet-events-v1.sqlite.owner.lock"));
        const QString sourceLockPath = QDir(sourceDirectory).filePath(QStringLiteral("fleet-events-backup.owner.lock"));
        if (!lockPathSafe(targetLockPath)) return QStringLiteral("archive-maintenance-owner-lock-path-rejected");
        QLockFile targetOwner(targetLockPath);
        targetOwner.setStaleLockTime(0);
        if (!targetOwner.tryLock(0)) return QStringLiteral("archive-maintenance-owner-lock-unavailable");
        if (!lockPathSafe(sourceLockPath)) return QStringLiteral("archive-backup-destination-lock-rejected");
        QLockFile sourceOwner(sourceLockPath);
        sourceOwner.setStaleLockTime(0);
        if (!sourceOwner.tryLock(0)) return QStringLiteral("archive-backup-destination-lock-unavailable");
        setState(QStringLiteral("inspecting"));
        const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
        if (source.bytes > maximum) return QStringLiteral("archive-import-source-rejected");
        Listing targetBefore, targetPreCopy;
        if (!listing(_directory, maximum, _cancelled, &targetBefore)) return QStringLiteral("archive-import-target-rejected");
        result.insert(QStringLiteral("destinationDirectoryIdentity"), targetBefore.identity);
        const auto sourceBefore = FleetArchiveDirectoryInventory::scan(sourceDirectory,
            FleetArchiveDirectoryInventory::Namespace::EventBackups, kMaximumFiles, &_cancelled);
        if (!sourceBefore.ok || !sourceBefore.enumerationCompleted || !directoryIdentity(sourceBefore.directoryIdentity)) return QStringLiteral("archive-import-source-rejected");
        result.insert(QStringLiteral("observedDirectoryIdentity"), sourceBefore.directoryIdentity);
        if (sourceBefore.directoryIdentity != _request.value(QStringLiteral("expectedDirectoryIdentity")).toString()
            || sourceBefore.directoryIdentity == targetBefore.identity) return QStringLiteral("archive-import-source-changed");
        bool found = false;
        for (const auto& entry : sourceBefore.entries) if (entry.basename == source.basename && entry.regularFile && !entry.symlink) found = true;
        if (!found) return QStringLiteral("archive-import-source-rejected");
        const auto sourceCurrent = [&](const std::atomic_bool& cancelled) {
            const auto scan = FleetArchiveDirectoryInventory::scan(sourceDirectory,
                FleetArchiveDirectoryInventory::Namespace::EventBackups, kMaximumFiles, &cancelled);
            if (!sameBackupCandidates(sourceBefore, scan)) return false;
            QVariantMap actual;
            if (!inspectFile(sourceDirectory, source, _policy, cancelled, &actual, true)) return false;
            actual.insert(QStringLiteral("backupOperationId"), source.operation);
            return actual == sourceDescriptor;
        };
        if (!sourceCurrent(_cancelled)) return QStringLiteral("archive-import-source-changed");
        if (targetBefore.retired >= kMaximumFiles) return QStringLiteral("archive-import-capacity-rejected");
        if (!absent(_directory, target) || !absent(_directory, sibling)) return QStringLiteral("archive-import-target-exists");
        if (!absent(_directory, staging)) return QStringLiteral("archive-import-staging-exists");
        if (!listing(_directory, maximum, _cancelled, &targetPreCopy) || !sameListing(targetBefore, targetPreCopy)
            || !absent(_directory, target) || !absent(_directory, sibling) || !absent(_directory, staging)) return QStringLiteral("archive-import-target-changed");
        if (!sourceCurrent(_cancelled)) return QStringLiteral("archive-import-source-changed");
        QFileInfo sourceInfo;
        if (!safeFile(sourceDirectory, source.basename, maximum, &sourceInfo, true)
            || sourceInfo.size() != source.bytes || sourceInfo.lastModified().toMSecsSinceEpoch() != source.modified) return QStringLiteral("archive-import-source-changed");
        QFile input(QDir(sourceDirectory).filePath(source.basename));
        if (!input.open(QIODevice::ReadOnly | QIODevice::Unbuffered) || input.size() != source.bytes) return QStringLiteral("archive-import-source-rejected");
        {
            QMutexLocker lock(&_mutex);
            if (_cancelled.load()) return QStringLiteral("archive-maintenance-cancelled");
            result.insert(QStringLiteral("decisionFinal"), true); result.insert(QStringLiteral("stagingCreateAttempted"), true);
            result.insert(QStringLiteral("mayHaveWritten"), true);
            _status.insert(QStringLiteral("state"), QStringLiteral("copying"));
            _status.insert(QStringLiteral("decisionFinal"), true); _status.insert(QStringLiteral("stagingCreateAttempted"), true);
            _status.insert(QStringLiteral("mayHaveWritten"), true);
        }
        QFile output(QDir(_directory).filePath(staging));
        QString copyError = empty;
        if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Unbuffered)) copyError = QStringLiteral("archive-import-staging-create-unconfirmed");
        else {
            result.insert(QStringLiteral("stagingCreated"), true); publishFacts();
            qint64 copied = 0;
            QCryptographicHash copyHash(QCryptographicHash::Sha256);
            while (copyError.isEmpty() && copied < source.bytes) {
                if (_cancelled.load()) { copyError = QStringLiteral("archive-maintenance-cancelled"); break; }
                const QByteArray chunk = input.read(qMin<qint64>(source.bytes - copied, 64LL * 1024LL));
                if (chunk.isEmpty() || chunk.size() > source.bytes - copied || input.error() != QFileDevice::NoError) { copyError = QStringLiteral("archive-backup-read-failed"); break; }
                qint64 written = 0;
                while (copyError.isEmpty() && written < chunk.size()) {
                    if (_cancelled.load()) { copyError = QStringLiteral("archive-maintenance-cancelled"); break; }
                    const qint64 count = output.write(chunk.constData() + written, chunk.size() - written);
                    if (count <= 0 || count > chunk.size() - written) { copyError = QStringLiteral("archive-backup-write-failed"); break; }
                    written += count; copied += count;
                    result.insert(QStringLiteral("bytesCopied"), QString::number(copied)); publishFacts();
                }
                if (copyError.isEmpty()) copyHash.addData(chunk);
            }
            if (copyError.isEmpty()) {
                if (copied != source.bytes || !input.read(1).isEmpty() || input.error() != QFileDevice::NoError
                    || input.size() != source.bytes || QString::fromLatin1(copyHash.result().toHex()) != sourceDescriptor.value(QStringLiteral("fileFingerprint")).toString())
                    copyError = QStringLiteral("archive-backup-copy-incomplete");
                else result.insert(QStringLiteral("copyCompleted"), true);
            }
            if (!output.flush() && copyError.isEmpty()) copyError = QStringLiteral("archive-backup-flush-failed");
            output.close();
            if (output.error() != QFileDevice::NoError && copyError.isEmpty()) copyError = QStringLiteral("archive-backup-close-unconfirmed");
        }
        input.close();
        if (input.error() != QFileDevice::NoError && copyError.isEmpty()) copyError = QStringLiteral("archive-backup-close-unconfirmed");
        publishFacts();
        if (!result.value(QStringLiteral("copyCompleted")).toBool()) return copyError.isEmpty() ? QStringLiteral("archive-backup-copy-incomplete") : copyError;
        // Completed writes always receive full readback, even when a late
        // cancellation/flush/close failure will prevent publication.
        setState(QStringLiteral("verifying"));
        const std::atomic_bool observeDespiteCancel{false};
        QFileInfo stageInfo;
        File stage;
        QVariantMap stageDescriptor;
        Listing targetAfterStage;
        bool stageValid = safeFile(_directory, staging, maximum, &stageInfo, true) && parseBackupName(staging, &stage);
        if (stageValid) {
            stage.bytes = source.bytes; stage.modified = stageInfo.lastModified().toMSecsSinceEpoch();
            stageValid = inspectFile(_directory, stage, _policy, observeDespiteCancel, &stageDescriptor, true)
                && stageDescriptor.value(QStringLiteral("fileBytes")) == sourceDescriptor.value(QStringLiteral("fileBytes"))
                && stageDescriptor.value(QStringLiteral("fileFingerprint")) == sourceDescriptor.value(QStringLiteral("fileFingerprint"))
                && stageDescriptor.value(QStringLiteral("databaseEpoch")) == sourceDescriptor.value(QStringLiteral("databaseEpoch"))
                && stageDescriptor.value(QStringLiteral("sealRunId")) == sourceDescriptor.value(QStringLiteral("sealRunId"));
        }
        const bool sourceStillCurrent = sourceCurrent(observeDespiteCancel);
        const bool targetStillCurrent = listing(_directory, maximum, observeDespiteCancel, &targetAfterStage)
            && sameListing(targetBefore, targetAfterStage) && absent(_directory, target) && absent(_directory, sibling);
        const bool staged = stageValid && sourceStillCurrent && targetStillCurrent && copyError.isEmpty();
        result.insert(QStringLiteral("stagingVerified"), staged); publishFacts();
        if (!staged) return !copyError.isEmpty() ? copyError : !sourceStillCurrent ? QStringLiteral("archive-import-source-changed")
            : !targetStillCurrent ? QStringLiteral("archive-import-target-changed") : QStringLiteral("archive-import-staging-unconfirmed");
        {
            QMutexLocker lock(&_mutex);
            if (_cancelled.load()) return QStringLiteral("archive-maintenance-cancelled");
            result.insert(QStringLiteral("publishAttempted"), true); result.insert(QStringLiteral("mayHavePublished"), true);
            _status.insert(QStringLiteral("state"), QStringLiteral("publishing"));
            _status.insert(QStringLiteral("publishAttempted"), true); _status.insert(QStringLiteral("mayHavePublished"), true);
        }
        const bool reported = nativeRenameNoReplace(_directory, staging, target);
        result.insert(QStringLiteral("publishReportedSuccess"), reported); publishFacts();
        QFileInfo targetInfo;
        File targetFile;
        QVariantMap finalDescriptor;
        bool verified = absent(_directory, staging) && absent(_directory, sibling)
            && safeFile(_directory, target, maximum, &targetInfo) && parseName(target, &targetFile);
        if (verified) {
            targetFile.bytes = source.bytes; targetFile.modified = targetInfo.lastModified().toMSecsSinceEpoch();
            verified = inspectFile(_directory, targetFile, _policy, observeDespiteCancel, &finalDescriptor)
                && finalDescriptor.value(QStringLiteral("fileBytes")) == sourceDescriptor.value(QStringLiteral("fileBytes"))
                && finalDescriptor.value(QStringLiteral("fileFingerprint")) == sourceDescriptor.value(QStringLiteral("fileFingerprint"))
                && finalDescriptor.value(QStringLiteral("databaseEpoch")) == sourceDescriptor.value(QStringLiteral("databaseEpoch"))
                && finalDescriptor.value(QStringLiteral("sealRunId")) == sourceDescriptor.value(QStringLiteral("sealRunId"));
        }
        Listing targetAfterPublish, expected = targetBefore;
        if (verified) {
            ++expected.retired; expected.files.append(targetFile);
            std::sort(expected.files.begin(), expected.files.end(), [](const File& a, const File& b) {
                return a.kind == b.kind ? a.basename < b.basename : a.kind == QStringLiteral("sealed");
            });
            verified = listing(_directory, maximum, observeDespiteCancel, &targetAfterPublish)
                && sameListing(expected, targetAfterPublish) && absent(_directory, staging) && absent(_directory, sibling);
        }
        const bool finalSourceCurrent = sourceCurrent(observeDespiteCancel);
        verified = verified && finalSourceCurrent;
        result.insert(QStringLiteral("importVerified"), verified);
        if (verified) { result.insert(QStringLiteral("targetDescriptor"), finalDescriptor); return empty; }
        return reported ? QStringLiteral("archive-import-postcondition-unconfirmed") : QStringLiteral("archive-import-publish-unconfirmed");
    };
    QString error = perform();
    if (!result.value(QStringLiteral("decisionFinal")).toBool() && _cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    result.insert(QStringLiteral("cancelObserved"), _cancelled.load()); result.insert(QStringLiteral("errorCode"), error);
    if (_configurationValid && !resultMatches(result, _request, _policy)) {
        result.insert(QStringLiteral("importVerified"), false); result.insert(QStringLiteral("targetDescriptor"), QVariantMap{});
        error = QStringLiteral("archive-maintenance-result-contract-rejected"); result.insert(QStringLiteral("errorCode"), error);
    }
    publishFacts(); setState(QStringLiteral("result-submitted"), error);
    emit resultReady(_requestId, result); // Never discard possible writes/publication in a failure signal.
}

void FleetEventArchiveMaintenanceWorker::runBackupDiagnostics()
{
    setState(QStringLiteral("opening"));
    const QString empty = QStringLiteral("");
    QString error = empty;
    QVariantMap result;
    const QString destination = _request.value(QStringLiteral("destinationDirectory")).toString();
    if (!_configurationValid) error = QStringLiteral("archive-maintenance-configuration-rejected");
    else if (!destinationSyntax(destination) || !localDirectory(destination) || QDir(destination).isRoot())
        error = QStringLiteral("archive-backup-destination-rejected");
    else if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty()) {
        // Only the explicitly selected destination and its cooperation lock
        // are used. Never implicitly read the constructor's original folder.
        const QString lockPath = QDir(destination).filePath(QStringLiteral("fleet-events-backup.owner.lock"));
        const QFileInfo lockInfo(lockPath);
        if (lockInfo.isSymLink() || (lockInfo.exists() && (!lockInfo.isFile() || lockInfo.size() > 16384
            || QDir::fromNativeSeparators(lockInfo.canonicalFilePath()) != lockPath)))
            error = QStringLiteral("archive-backup-destination-lock-rejected");
        else {
            QLockFile owner(lockPath);
            owner.setStaleLockTime(0);
            if (!owner.tryLock(0)) error = QStringLiteral("archive-backup-destination-lock-unavailable");
            else {
                setState(QStringLiteral("inspecting"));
                const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
                const auto before = FleetArchiveDirectoryInventory::scan(destination,
                    FleetArchiveDirectoryInventory::Namespace::EventBackups, kMaximumFiles, &_cancelled);
                if (!before.ok || !before.enumerationCompleted || !directoryIdentity(before.directoryIdentity))
                    error = QStringLiteral("archive-backup-diagnostics-inventory-rejected");
                QVariantList rows;
                int readable = 0, rejected = 0;
                for (const auto& candidate : before.entries) {
                    if (!error.isEmpty()) break;
                    if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                    const QString& name = candidate.basename;
                    if (!diagnosticBasename(name)) { error = QStringLiteral("archive-backup-diagnostics-name-unrepresentable"); break; }
                    QString reason = empty;
                    File file;
                    QVariantMap descriptor;
                    if (!parseBackupName(name, &file)) reason = QStringLiteral("archive-backup-name-invalid");
                    else if (!candidate.regularFile || candidate.symlink) reason = QStringLiteral("archive-backup-type-rejected");
                    else {
                        const QString path = QDir(destination).filePath(name);
                        QFileInfo actual(path);
                        if (!actual.exists() || !actual.isFile() || actual.isSymLink()) reason = QStringLiteral("archive-backup-file-rejected");
                        else if (actual.size() < 512 || actual.size() > maximum) reason = QStringLiteral("archive-backup-size-rejected");
                        else if (!sidecarsAbsent(path)) reason = QStringLiteral("archive-backup-sidecars-present");
                        else if (!safeFile(destination, name, maximum, &actual, true)) reason = QStringLiteral("archive-backup-file-rejected");
                        else {
                            file.bytes = actual.size(); file.modified = actual.lastModified().toMSecsSinceEpoch();
                            bool closeFailed = false;
                            const bool inspected = inspectFile(destination, file, _policy, _cancelled, &descriptor, true, &closeFailed);
                            // These are operation failures, never a single row
                            // rejected while the rest becomes a success prefix.
                            if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                            if (closeFailed) { error = QStringLiteral("archive-backup-diagnostics-close-failed"); break; }
                            if (!inspected) reason = QStringLiteral("archive-backup-structure-unconfirmed");
                            else {
                                descriptor.insert(QStringLiteral("backupOperationId"), file.operation);
                                if (!backupDescriptorValid(descriptor)) {
                                    error = QStringLiteral("archive-maintenance-result-contract-rejected"); break;
                                }
                            }
                        }
                    }
                    if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                    const bool accepted = reason.isEmpty();
                    if (accepted) ++readable; else { ++rejected; descriptor.clear(); }
                    rows.append(QVariantMap{{QStringLiteral("basename"), name},
                        {QStringLiteral("status"), accepted ? QStringLiteral("readable") : QStringLiteral("rejected")},
                        {QStringLiteral("errorCode"), reason}, {QStringLiteral("descriptor"), descriptor}});
                }
                if (error.isEmpty()) {
                    const auto after = FleetArchiveDirectoryInventory::scan(destination,
                        FleetArchiveDirectoryInventory::Namespace::EventBackups, kMaximumFiles, &_cancelled);
                    if (!sameBackupCandidates(before, after)) error = QStringLiteral("archive-backup-diagnostics-candidates-changed");
                }
                // Rejected names retain no trusted metadata. Only readable
                // rows carry measured metadata, which must still match at the
                // final tail. This is not a cross-file atomic read snapshot.
                for (const QVariant& value : rows) {
                    if (!error.isEmpty()) break;
                    if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                    const QVariantMap row = value.toMap();
                    if (row.value(QStringLiteral("status")).toString() != QStringLiteral("readable")) continue;
                    File file;
                    QFileInfo actual;
                    if (!backupDescriptorValid(row.value(QStringLiteral("descriptor")).toMap(), &file)
                        || !safeFile(destination, file.basename, maximum, &actual, true)
                        || actual.size() != file.bytes || actual.lastModified().toMSecsSinceEpoch() != file.modified)
                        error = QStringLiteral("archive-backup-diagnostics-readable-changed");
                }
                if (error.isEmpty()) {
                    result = common();
                    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-backup-diagnostics"));
                    result.insert(QStringLiteral("schemaVersion"), 1);
                    result.insert(QStringLiteral("destinationDirectory"), destination);
                    result.insert(QStringLiteral("directoryIdentity"), before.directoryIdentity);
                    result.insert(QStringLiteral("entries"), rows);
                    result.insert(QStringLiteral("candidateCount"), before.entries.size());
                    result.insert(QStringLiteral("readableCount"), readable);
                    result.insert(QStringLiteral("rejectedCount"), rejected);
                    result.insert(QStringLiteral("enumerationCompleted"), true);
                    result.insert(QStringLiteral("originalSourceVerified"), false);
                    result.insert(QStringLiteral("backupSetCompletenessVerified"), false);
                }
            }
        }
    }
    if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty() && !resultMatches(result, _request, _policy)) error = QStringLiteral("archive-maintenance-result-contract-rejected");
    if (!error.isEmpty()) {
        setState(_cancelled.load() ? QStringLiteral("cancelled") : QStringLiteral("failed"), error);
        emit failure(_requestId, error);
        return;
    }
    setState(QStringLiteral("result-submitted"));
    emit resultReady(_requestId, result);
}

void FleetEventArchiveMaintenanceWorker::runBackupCatalog()
{
    setState(QStringLiteral("opening"));
    QString error = QStringLiteral("");
    QVariantMap result;
    const QString destination = _request.value(QStringLiteral("destinationDirectory")).toString();
    if (!_configurationValid) error = QStringLiteral("archive-maintenance-configuration-rejected");
    else if (!destinationSyntax(destination) || !localDirectory(destination) || QDir(destination).isRoot())
        error = QStringLiteral("archive-backup-destination-rejected");
    else if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty()) {
        // This is the ONLY created/released file. Never implicitly select the
        // constructor's source folder or its owner lock/current DB. The user
        // may explicitly choose that same directory as the catalog destination.
        // No backup copy, SQL repair, move or recovery is involved.
        const QString lockPath = QDir(destination).filePath(QStringLiteral("fleet-events-backup.owner.lock"));
        const QFileInfo lockInfo(lockPath);
        if (lockInfo.isSymLink() || (lockInfo.exists() && (!lockInfo.isFile() || lockInfo.size() > 16384
            || QDir::fromNativeSeparators(lockInfo.canonicalFilePath()) != lockPath)))
            error = QStringLiteral("archive-backup-destination-lock-rejected");
        else {
            QLockFile owner(lockPath);
            owner.setStaleLockTime(0);
            if (!owner.tryLock(0)) error = QStringLiteral("archive-backup-destination-lock-unavailable");
            else {
                setState(QStringLiteral("inspecting"));
                const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
                Listing before, after;
                QVariantList entries;
                if (!backupListing(destination, maximum, _cancelled, &before)) error = QStringLiteral("archive-backup-catalog-rejected");
                for (const File& file : before.files) {
                    if (!error.isEmpty()) break;
                    QVariantMap descriptor;
                    // New SHA is only the bytes observed in this independent
                    // read. The filename does not restore an original source
                    // fingerprint or a successful historical backup receipt.
                    if (!inspectFile(destination, file, _policy, _cancelled, &descriptor, true)) {
                        error = QStringLiteral("archive-backup-catalog-rejected"); break;
                    }
                    descriptor.insert(QStringLiteral("backupOperationId"), file.operation);
                    if (!backupDescriptorValid(descriptor)) { error = QStringLiteral("archive-backup-catalog-rejected"); break; }
                    entries.append(descriptor);
                }
                if (error.isEmpty() && (!backupListing(destination, maximum, _cancelled, &after) || !sameListing(before, after)))
                    error = QStringLiteral("archive-backup-catalog-changed");
                if (error.isEmpty()) {
                    result = common();
                    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-backup-catalog"));
                    result.insert(QStringLiteral("schemaVersion"), 1);
                    result.insert(QStringLiteral("destinationDirectory"), destination);
                    result.insert(QStringLiteral("directoryIdentity"), before.identity);
                    result.insert(QStringLiteral("entries"), entries);
                    result.insert(QStringLiteral("fileCount"), entries.size());
                    result.insert(QStringLiteral("enumerationCompleted"), true);
                    result.insert(QStringLiteral("originalSourceVerified"), false);
                    result.insert(QStringLiteral("backupSetCompletenessVerified"), false);
                }
            } // Destination cooperation lock covers all hash/SQL/scans.
        }
    }
    if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty() && !resultMatches(result, _request, _policy)) error = QStringLiteral("archive-maintenance-result-contract-rejected");
    if (!error.isEmpty()) {
        setState(_cancelled.load() ? QStringLiteral("cancelled") : QStringLiteral("failed"), error);
        emit failure(_requestId, error); // Never publish any successful prefix.
        return;
    }
    setState(QStringLiteral("result-submitted"));
    emit resultReady(_requestId, result);
}

void FleetEventArchiveMaintenanceWorker::runBackup()
{
    setState(QStringLiteral("opening"));
    const QString empty = QStringLiteral("");
    QString error = empty;
    File source;
    descriptorValid(_request.value(QStringLiteral("source")).toMap(), &source);
    const QString destination = _request.value(QStringLiteral("destinationDirectory")).toString();
    const QString operation = _request.value(QStringLiteral("operationId")).toString();
    const QString target = backupName(source, operation);
    QVariantMap result = common();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-maintenance-backup"));
    result.insert(QStringLiteral("schemaVersion"), 1);
    for (const QString& key : {QStringLiteral("kind"), QStringLiteral("operationId"), QStringLiteral("source"),
        QStringLiteral("expectedDirectoryIdentity"), QStringLiteral("destinationDirectory")}) result.insert(key, _request.value(key));
    result.insert(QStringLiteral("targetBasename"), target);
    result.insert(QStringLiteral("observedDirectoryIdentity"), empty);
    result.insert(QStringLiteral("destinationDirectoryIdentity"), empty);
    result.insert(QStringLiteral("bytesCopied"), QStringLiteral("0"));
    result.insert(QStringLiteral("errorCode"), empty);
    for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("targetCreateAttempted"), QStringLiteral("targetCreated"),
        QStringLiteral("mayHaveWritten"), QStringLiteral("copyCompleted"), QStringLiteral("backupVerified"), QStringLiteral("cancelObserved")})
        result.insert(key, false);
    const auto publishFacts = [this, &result]() {
        QMutexLocker lock(&_mutex);
        for (const QString& key : {QStringLiteral("decisionFinal"), QStringLiteral("targetCreateAttempted"), QStringLiteral("targetCreated"),
            QStringLiteral("mayHaveWritten"), QStringLiteral("bytesCopied"), QStringLiteral("copyCompleted"), QStringLiteral("backupVerified")})
            _status.insert(key, result.value(key));
    };
    const auto lockPathSafe = [](const QString& path) {
        const QFileInfo info(path);
        return !info.isSymLink() && (!info.exists() || (info.isFile() && info.size() <= 16384
            && QDir::fromNativeSeparators(info.canonicalFilePath()) == path));
    };
    if (!_configurationValid) error = QStringLiteral("archive-maintenance-configuration-rejected");
    else if (!localDirectory(_directory)) error = QStringLiteral("archive-maintenance-directory-rejected");
    else if (!destinationSyntax(destination) || !disjointDirectories(_directory, destination)
        || !localDirectory(destination) || QDir(destination).isRoot()) error = QStringLiteral("archive-backup-destination-rejected");
    else if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
    if (error.isEmpty()) {
        const QString sourceLockPath = QDir(_directory).filePath(QStringLiteral("fleet-events-v1.sqlite.owner.lock"));
        if (!lockPathSafe(sourceLockPath)) error = QStringLiteral("archive-maintenance-owner-lock-path-rejected");
        else {
            QLockFile sourceOwner(sourceLockPath);
            sourceOwner.setStaleLockTime(0);
            if (!sourceOwner.tryLock(0)) error = QStringLiteral("archive-maintenance-owner-lock-unavailable");
            else {
                const QString destinationLockPath = QDir(destination).filePath(QStringLiteral("fleet-events-backup.owner.lock"));
                if (!lockPathSafe(destinationLockPath)) error = QStringLiteral("archive-backup-destination-lock-rejected");
                else {
                    QLockFile destinationOwner(destinationLockPath);
                    destinationOwner.setStaleLockTime(0);
                    if (!destinationOwner.tryLock(0)) error = QStringLiteral("archive-backup-destination-lock-unavailable");
                    else {
                        setState(QStringLiteral("inspecting"));
                        const qint64 maximum = _policy.value(QStringLiteral("maximumDatabaseMiB")).toLongLong() * kMiB;
                        Listing before, finalBefore;
                        QString destinationBefore = empty, destinationFinalBefore = empty;
                        QVariantMap actual;
                        if (!listing(_directory, maximum, _cancelled, &before)) error = QStringLiteral("archive-maintenance-inventory-rejected");
                        else {
                            result.insert(QStringLiteral("observedDirectoryIdentity"), before.identity);
                            if (before.identity != _request.value(QStringLiteral("expectedDirectoryIdentity")).toString())
                                error = QStringLiteral("archive-maintenance-directory-changed");
                        }
                        bool found = false;
                        for (const File& file : before.files) if (file.basename == source.basename
                            && file.bytes == source.bytes && file.modified == source.modified) found = true;
                        if (error.isEmpty() && !found) error = QStringLiteral("archive-maintenance-source-changed");
                        if (error.isEmpty() && (!destinationIdentity(destination, _cancelled, &destinationBefore)
                            || destinationBefore == before.identity)) error = QStringLiteral("archive-backup-destination-rejected");
                        if (error.isEmpty()) result.insert(QStringLiteral("destinationDirectoryIdentity"), destinationBefore);
                        if (error.isEmpty() && !absent(destination, target)) error = QStringLiteral("archive-backup-target-exists");
                        if (error.isEmpty() && (!inspectFile(_directory, source, _policy, _cancelled, &actual)
                            || actual != _request.value(QStringLiteral("source")).toMap())) error = QStringLiteral("archive-maintenance-source-changed");
                        if (error.isEmpty() && (!listing(_directory, maximum, _cancelled, &finalBefore) || !sameListing(before, finalBefore)))
                            error = QStringLiteral("archive-maintenance-directory-changed");
                        if (error.isEmpty() && (!destinationIdentity(destination, _cancelled, &destinationFinalBefore)
                            || destinationFinalBefore != destinationBefore || !absent(destination, target))) error = QStringLiteral("archive-backup-destination-changed");
                        QFile input(QDir(_directory).filePath(source.basename));
                        QFile output(QDir(destination).filePath(target));
                        if (error.isEmpty()) {
                            QFileInfo sourceInfo;
                            if (!safeFile(_directory, source.basename, maximum, &sourceInfo) || sourceInfo.size() != source.bytes
                                || sourceInfo.lastModified().toMSecsSinceEpoch() != source.modified
                                || !input.open(QIODevice::ReadOnly | QIODevice::Unbuffered) || input.size() != source.bytes)
                                error = QStringLiteral("archive-backup-source-changed");
                        }
                        if (error.isEmpty()) {
                            QMutexLocker lock(&_mutex);
                            if (_cancelled.load()) error = QStringLiteral("archive-maintenance-cancelled");
                            else {
                                result.insert(QStringLiteral("decisionFinal"), true);
                                result.insert(QStringLiteral("targetCreateAttempted"), true);
                                result.insert(QStringLiteral("mayHaveWritten"), true);
                                _status.insert(QStringLiteral("state"), QStringLiteral("copying"));
                                _status.insert(QStringLiteral("decisionFinal"), true);
                                _status.insert(QStringLiteral("targetCreateAttempted"), true);
                                _status.insert(QStringLiteral("mayHaveWritten"), true);
                            }
                        }
                        if (result.value(QStringLiteral("targetCreateAttempted")).toBool()) {
                            // Qt 5.15 native engine maps NewOnly to CREATE_NEW
                            // / O_CREAT|O_EXCL. No temp-file rename, overwrite,
                            // QFile::copy, resize, delete, or cleanup rollback.
                            if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Unbuffered))
                                error = QStringLiteral("archive-backup-target-create-unconfirmed");
                            else {
                                result.insert(QStringLiteral("targetCreated"), true);
                                publishFacts();
                                qint64 copied = 0;
                                QCryptographicHash copyHash(QCryptographicHash::Sha256);
                                while (error.isEmpty() && copied < source.bytes) {
                                    if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                                    const QByteArray chunk = input.read(qMin<qint64>(source.bytes - copied, 64LL * 1024LL));
                                    if (chunk.isEmpty() || chunk.size() > source.bytes - copied || input.error() != QFileDevice::NoError) {
                                        error = QStringLiteral("archive-backup-read-failed"); break;
                                    }
                                    qint64 written = 0;
                                    while (error.isEmpty() && written < chunk.size()) {
                                        if (_cancelled.load()) { error = QStringLiteral("archive-maintenance-cancelled"); break; }
                                        const qint64 count = output.write(chunk.constData() + written, chunk.size() - written);
                                        if (count <= 0 || count > chunk.size() - written) { error = QStringLiteral("archive-backup-write-failed"); break; }
                                        written += count; copied += count;
                                        result.insert(QStringLiteral("bytesCopied"), QString::number(copied));
                                        publishFacts();
                                    }
                                    if (error.isEmpty()) copyHash.addData(chunk);
                                }
                                if (error.isEmpty()) {
                                    if (copied != source.bytes || !input.read(1).isEmpty() || input.error() != QFileDevice::NoError
                                        || input.size() != source.bytes || QString::fromLatin1(copyHash.result().toHex())
                                            != _request.value(QStringLiteral("source")).toMap().value(QStringLiteral("fileFingerprint")).toString())
                                        error = QStringLiteral("archive-backup-copy-incomplete");
                                    else result.insert(QStringLiteral("copyCompleted"), true);
                                }
                                // Even a cancelled/failed partial file is closed
                                // and retained. close() is void; preserve earlier
                                // failures and independently check flush/error.
                                if (!output.flush() && error.isEmpty()) error = QStringLiteral("archive-backup-flush-failed");
                                output.close();
                                if (output.error() != QFileDevice::NoError && error.isEmpty()) error = QStringLiteral("archive-backup-close-unconfirmed");
                            }
                        }
                        if (input.isOpen()) {
                            input.close();
                            if (input.error() != QFileDevice::NoError && error.isEmpty())
                                error = QStringLiteral("archive-backup-close-unconfirmed");
                        }
                        publishFacts();
                        if (result.value(QStringLiteral("copyCompleted")).toBool()) {
                            setState(QStringLiteral("verifying"));
                            // A complete copy is read back even if cancellation
                            // arrives now. A late request cannot erase a file or
                            // turn completed writes into a not-backed-up claim.
                            const std::atomic_bool observeDespiteCancel{false};
                            QString targetDirectoryBefore = empty, targetDirectoryAfter = empty;
                            QFileInfo targetInfo;
                            File backup;
                            QVariantMap observedBackup, observedSource;
                            Listing sourceAfter;
                            bool verified = destinationIdentity(destination, observeDespiteCancel, &targetDirectoryBefore)
                                && targetDirectoryBefore == destinationBefore
                                && safeFile(destination, target, maximum, &targetInfo, true)
                                && parseBackupName(target, &backup);
                            if (verified) {
                                backup.bytes = source.bytes; backup.modified = targetInfo.lastModified().toMSecsSinceEpoch();
                                verified = inspectFile(destination, backup, _policy, observeDespiteCancel, &observedBackup, true)
                                    && observedBackup.value(QStringLiteral("fileBytes")) == actual.value(QStringLiteral("fileBytes"))
                                    && observedBackup.value(QStringLiteral("fileFingerprint")) == actual.value(QStringLiteral("fileFingerprint"))
                                    && observedBackup.value(QStringLiteral("databaseEpoch")) == actual.value(QStringLiteral("databaseEpoch"))
                                    && observedBackup.value(QStringLiteral("sealRunId")) == actual.value(QStringLiteral("sealRunId"))
                                    && destinationIdentity(destination, observeDespiteCancel, &targetDirectoryAfter)
                                    && targetDirectoryAfter == destinationBefore;
                            }
                            if (!verified && error.isEmpty()) error = QStringLiteral("archive-backup-readback-unconfirmed");
                            const bool sourceUnchanged = inspectFile(_directory, source, _policy, observeDespiteCancel, &observedSource)
                                && observedSource == _request.value(QStringLiteral("source")).toMap()
                                && listing(_directory, maximum, observeDespiteCancel, &sourceAfter) && sameListing(before, sourceAfter);
                            if (!sourceUnchanged && error.isEmpty()) error = QStringLiteral("archive-backup-source-changed");
                            result.insert(QStringLiteral("backupVerified"), verified && sourceUnchanged && error.isEmpty());
                        }
                    }
                }
            } // Source and destination cooperation locks released after all IO.
        }
    }
    if (!result.value(QStringLiteral("targetCreateAttempted")).toBool() && _cancelled.load())
        error = QStringLiteral("archive-maintenance-cancelled");
    if (!result.value(QStringLiteral("backupVerified")).toBool() && error.isEmpty()) error = QStringLiteral("archive-backup-copy-incomplete");
    result.insert(QStringLiteral("cancelObserved"), _cancelled.load()); result.insert(QStringLiteral("errorCode"), error);
    if (_configurationValid && !resultMatches(result, _request, _policy)) {
        result.insert(QStringLiteral("backupVerified"), false);
        error = QStringLiteral("archive-maintenance-result-contract-rejected"); result.insert(QStringLiteral("errorCode"), error);
    }
    publishFacts();
    setState(QStringLiteral("result-submitted"), error);
    // Every valid backup terminal keeps mayHaveWritten/partial facts, even
    // create failure, cancellation, flush failure or unconfirmed readback.
    emit resultReady(_requestId, result);
}

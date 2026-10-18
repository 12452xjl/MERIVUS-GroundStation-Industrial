#include "FleetEventArchiveSql.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariantList>

#include <limits>

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr int kApplicationId = 1296389702; // 0x4d455646, MEVF
constexpr int kSchemaVersion = 1;
constexpr qint64 kMaximumRecordBytes = 512LL * 1024LL;

const QString kMetaSql = QStringLiteral(
    "CREATE TABLE meta (singleton INTEGER PRIMARY KEY CHECK(singleton=1), "
    "database_epoch TEXT NOT NULL, retired_count INTEGER NOT NULL CHECK(retired_count>=0), "
    "created_utc_ms INTEGER NOT NULL CHECK(created_utc_ms>0))");
const QString kRunsSql = QStringLiteral(
    "CREATE TABLE runs (run_id TEXT PRIMARY KEY NOT NULL, started_utc_ms INTEGER NOT NULL CHECK(started_utc_ms>0), "
    "last_sequence INTEGER NOT NULL CHECK(last_sequence>=0), clean_shutdown INTEGER NOT NULL CHECK(clean_shutdown IN (0,1)), "
    "dropped_count INTEGER NOT NULL CHECK(dropped_count>=0), discarded_count INTEGER NOT NULL CHECK(discarded_count>=0))");
const QString kEventsSql = QStringLiteral(
    "CREATE TABLE events (row_id INTEGER PRIMARY KEY AUTOINCREMENT, run_id TEXT NOT NULL REFERENCES runs(run_id), "
    "sequence INTEGER NOT NULL CHECK(sequence>0), event_utc_ms INTEGER NOT NULL CHECK(event_utc_ms>0), "
    "received_utc_ms INTEGER NOT NULL CHECK(received_utc_ms>0), received_monotonic_ms INTEGER NOT NULL CHECK(received_monotonic_ms>=0), "
    "event_type TEXT NOT NULL, event_source TEXT NOT NULL, canonical_json BLOB NOT NULL, content_hash TEXT NOT NULL, "
    "UNIQUE(run_id,sequence))");

bool uuid(const QString& value)
{
    return !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces).toLower() == value;
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

bool pragmaEquals(QSqlDatabase& database, const QString& sql, const QString& expected)
{
    QSqlQuery query(database);
    return query.exec(sql) && query.next() && query.value(0).toString().toLower() == expected
        && !query.next() && !query.lastError().isValid();
}

bool columns(QSqlDatabase& database, const QString& table, const QStringList& names,
             const QStringList& types, const QList<int>& notNull, const QList<int>& primaryKey)
{
    // Table identifiers below are fixed literals, never user/query values.
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA table_info(") + table + QLatin1Char(')'))) return false;
    int index = 0;
    while (query.next()) {
        if (index >= names.size() || query.value(0).toInt() != index
            || query.value(1).toString() != names.at(index)
            || query.value(2).toString() != types.at(index)
            || query.value(3).toInt() != notNull.at(index)
            || !query.value(4).isNull() || query.value(5).toInt() != primaryKey.at(index)) return false;
        ++index;
    }
    return index == names.size() && !query.lastError().isValid();
}

bool indexMatches(QSqlDatabase& database, const QString& table, const QString& index,
                  const QStringList& expectedColumns, const QString& origin)
{
    QSqlQuery list(database);
    if (!list.exec(QStringLiteral("PRAGMA index_list(") + table + QLatin1Char(')'))) return false;
    if (!list.next() || list.value(1).toString() != index || list.value(2).toInt() != 1
        || list.value(3).toString() != origin || list.value(4).toInt() != 0
        || list.next() || list.lastError().isValid()) return false;
    QSqlQuery info(database);
    if (!info.exec(QStringLiteral("PRAGMA index_info(") + index + QLatin1Char(')'))) return false;
    int position = 0;
    while (info.next()) {
        if (position >= expectedColumns.size() || info.value(0).toInt() != position
            || info.value(2).toString() != expectedColumns.at(position)) return false;
        ++position;
    }
    return position == expectedColumns.size() && !info.lastError().isValid();
}

} // namespace

int FleetEventArchiveSql::applicationId() { return kApplicationId; }
int FleetEventArchiveSql::schemaVersion() { return kSchemaVersion; }
qint64 FleetEventArchiveSql::maximumRecordBytes() { return kMaximumRecordBytes; }
const QString& FleetEventArchiveSql::metaSchema() { return kMetaSql; }
const QString& FleetEventArchiveSql::runsSchema() { return kRunsSql; }
const QString& FleetEventArchiveSql::eventsSchema() { return kEventsSql; }

bool FleetEventArchiveSql::recordConsistent(const FleetEventArchiveCodec::Record& record)
{
    bool decoded = false;
    const QVariantMap wrapper = FleetEventArchiveCodec::decodeCanonical(record.canonicalJson, record.contentHash, &decoded);
    const QVariantMap event = wrapper.value(QStringLiteral("event")).toMap();
    return record.ok && decoded && uuid(record.runId) && record.sequenceNumber > 0
        && record.sequence == QString::number(record.sequenceNumber)
        && wrapper.value(QStringLiteral("runId")).toString() == record.runId
        && wrapper.value(QStringLiteral("sequence")).toString() == record.sequence
        && wrapper.value(QStringLiteral("receivedAtUtcMs")).toLongLong() == record.receivedUtcMs
        && wrapper.value(QStringLiteral("receivedAtMonotonicMs")).toLongLong() == record.receivedMonotonicMs
        && event.value(QStringLiteral("timestampMs")).toLongLong() == record.eventUtcMs
        && event.value(QStringLiteral("type")).toString() == record.eventType
        && event.value(QStringLiteral("source")).toString() == record.eventSource;
}

bool FleetEventArchiveSql::scalar(QSqlDatabase& database, const QString& sql, qint64* value)
{
    QSqlQuery query(database);
    if (!query.exec(sql) || !query.next() || query.value(0).isNull()) return false;
    bool ok = false;
    const qint64 result = query.value(0).toLongLong(&ok);
    if (!ok || !integerIn(query.value(0), std::numeric_limits<qint64>::min(), std::numeric_limits<qint64>::max())
        || query.next() || query.lastError().isValid()) return false;
    if (value) *value = result;
    return true;
}

bool FleetEventArchiveSql::inspect(QSqlDatabase& database, QVariantMap* metadata)
{
    qint64 application = 0, version = 0;
    if (!scalar(database, QStringLiteral("PRAGMA application_id"), &application) || application != kApplicationId
        || !scalar(database, QStringLiteral("PRAGMA user_version"), &version) || version != kSchemaVersion) return false;
    QSqlQuery schema(database);
    if (!schema.exec(QStringLiteral(
        "SELECT CASE WHEN typeof(type)='text' AND length(type)<=16 THEN type END, "
        "CASE WHEN typeof(name)='text' AND length(name)<=64 THEN name END, "
        "CASE WHEN typeof(tbl_name)='text' AND length(tbl_name)<=64 THEN tbl_name END, "
        "CASE WHEN typeof(sql)='text' AND length(sql)<=4096 THEN sql END, "
        "CASE WHEN sql IS NULL OR (typeof(sql)='text' AND length(sql)<=4096) THEN 1 ELSE 0 END "
        "FROM sqlite_master ORDER BY name"))) return false;
    QSet<QString> seen;
    const QMap<QString, QString> definitions{{QStringLiteral("meta"), kMetaSql},
        {QStringLiteral("runs"), kRunsSql}, {QStringLiteral("events"), kEventsSql}};
    while (schema.next()) {
        if (schema.value(4).toInt() != 1) return false;
        const QString name = schema.value(1).toString();
        if (seen.contains(name)) return false;
        seen.insert(name);
        if (definitions.contains(name)) {
            if (schema.value(0).toString() != QStringLiteral("table") || schema.value(2).toString() != name
                || schema.value(3).type() != QVariant::String
                || schema.value(3).toString() != definitions.value(name)) return false;
        } else if (name == QStringLiteral("sqlite_sequence")) {
            if (schema.value(0).toString() != QStringLiteral("table") || schema.value(2).toString() != name
                || schema.value(3).toString() != QStringLiteral("CREATE TABLE sqlite_sequence(name,seq)")) return false;
        } else if (name == QStringLiteral("sqlite_autoindex_runs_1") || name == QStringLiteral("sqlite_autoindex_events_1")) {
            const QString owner = name == QStringLiteral("sqlite_autoindex_runs_1") ? QStringLiteral("runs") : QStringLiteral("events");
            if (schema.value(0).toString() != QStringLiteral("index") || schema.value(2).toString() != owner
                || !schema.value(3).isNull()) return false;
        } else return false; // Includes foreign tables, triggers, views and indices.
    }
    if (schema.lastError().isValid() || seen.size() != 6
        || !columns(database, QStringLiteral("meta"), {"singleton", "database_epoch", "retired_count", "created_utc_ms"},
                    {"INTEGER", "TEXT", "INTEGER", "INTEGER"}, {0,1,1,1}, {1,0,0,0})
        || !columns(database, QStringLiteral("runs"), {"run_id", "started_utc_ms", "last_sequence", "clean_shutdown", "dropped_count", "discarded_count"},
                    {"TEXT", "INTEGER", "INTEGER", "INTEGER", "INTEGER", "INTEGER"}, {1,1,1,1,1,1}, {1,0,0,0,0,0})
        || !columns(database, QStringLiteral("events"), {"row_id", "run_id", "sequence", "event_utc_ms", "received_utc_ms", "received_monotonic_ms", "event_type", "event_source", "canonical_json", "content_hash"},
                    {"INTEGER", "TEXT", "INTEGER", "INTEGER", "INTEGER", "INTEGER", "TEXT", "TEXT", "BLOB", "TEXT"}, {0,1,1,1,1,1,1,1,1,1}, {1,0,0,0,0,0,0,0,0,0})
        || !indexMatches(database, QStringLiteral("runs"), QStringLiteral("sqlite_autoindex_runs_1"), {"run_id"}, QStringLiteral("pk"))
        || !indexMatches(database, QStringLiteral("events"), QStringLiteral("sqlite_autoindex_events_1"), {"run_id", "sequence"}, QStringLiteral("u"))) return false;
    QSqlQuery noMetaIndex(database);
    if (!noMetaIndex.exec(QStringLiteral("PRAGMA index_list(meta)")) || noMetaIndex.next() || noMetaIndex.lastError().isValid()) return false;
    qint64 invalidRuns = 0, invalidSequence = 0;
    if (!scalar(database, QStringLiteral(
        "SELECT COUNT(*) FROM runs WHERE typeof(run_id)!='text' OR length(run_id)!=36 "
        "OR run_id GLOB '*[^0-9a-f-]*' OR substr(run_id,9,1)!='-' OR substr(run_id,14,1)!='-' "
        "OR substr(run_id,19,1)!='-' OR substr(run_id,24,1)!='-' "
        "OR length(replace(run_id,'-',''))!=32 OR run_id='00000000-0000-0000-0000-000000000000' "
        "OR typeof(started_utc_ms)!='integer' OR started_utc_ms<1 OR started_utc_ms>9007199254740991 "
        "OR typeof(last_sequence)!='integer' OR last_sequence<0 "
        "OR typeof(clean_shutdown)!='integer' OR clean_shutdown NOT IN (0,1) "
        "OR typeof(dropped_count)!='integer' OR dropped_count<0 "
        "OR typeof(discarded_count)!='integer' OR discarded_count<0"), &invalidRuns) || invalidRuns != 0
        || !scalar(database, QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_sequence WHERE name!='events' OR typeof(seq)!='integer' OR seq<0"),
        &invalidSequence) || invalidSequence != 0) return false;
    QSqlQuery foreignKeys(database);
    if (!foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_check")) || foreignKeys.next()
        || foreignKeys.lastError().isValid()) return false;
    QSqlQuery meta(database);
    if (!meta.exec(QStringLiteral(
        "SELECT CASE WHEN typeof(singleton)='integer' THEN singleton END, "
        "CASE WHEN typeof(database_epoch)='text' AND length(database_epoch)=36 THEN database_epoch END, "
        "CASE WHEN typeof(retired_count)='integer' THEN retired_count END, "
        "CASE WHEN typeof(created_utc_ms)='integer' THEN created_utc_ms END FROM meta")) || !meta.next()
        || !integerIn(meta.value(0), 1, 1) || meta.value(1).type() != QVariant::String || !uuid(meta.value(1).toString())
        || !integerIn(meta.value(2), 0, std::numeric_limits<qint64>::max())
        || !integerIn(meta.value(3), 1, 9007199254740991LL)) return false;
    const QString epoch = meta.value(1).toString();
    const qint64 retired = meta.value(2).toLongLong();
    if (meta.next() || meta.lastError().isValid()
        || !pragmaEquals(database, QStringLiteral("PRAGMA quick_check"), QStringLiteral("ok"))) return false;
    qint64 first = 0, last = 0, retained = 0;
    if (!scalar(database, QStringLiteral("SELECT COALESCE(MIN(row_id),0) FROM events"), &first)
        || !scalar(database, QStringLiteral("SELECT COALESCE(MAX(row_id),0) FROM events"), &last)
        || !scalar(database, QStringLiteral("SELECT COUNT(*) FROM events"), &retained)
        || retained < 0 || first < 0 || last < first
        || (retained == 0 && (first != 0 || last != 0))
        || (retained > 0 && (first <= 0 || last - first + 1 < retained))) return false;
    if (metadata) *metadata = {{QStringLiteral("databaseEpoch"), epoch},
        {QStringLiteral("retiredCount"), QString::number(retired)},
        {QStringLiteral("retainedFromRowId"), QString::number(first)},
        {QStringLiteral("retainedThroughRowId"), QString::number(last)}};
    return true;
}

bool FleetEventArchiveSql::sealedRunMatches(QSqlDatabase& database, const QString& expectedEpoch,
                                           const QString& sealRunId)
{
    if (!uuid(expectedEpoch) || !uuid(sealRunId)) return false;
    QVariantMap actual;
    if (!inspect(database, &actual) || actual.value(QStringLiteral("databaseEpoch")).toString() != expectedEpoch) return false;
    QSqlQuery query(database);
    if (!query.prepare(QStringLiteral(
        "SELECT CASE WHEN typeof(clean_shutdown)='integer' THEN clean_shutdown END, "
        "CASE WHEN typeof(last_sequence)='integer' THEN last_sequence END, "
        "CASE WHEN typeof(dropped_count)='integer' THEN dropped_count END, "
        "CASE WHEN typeof(discarded_count)='integer' THEN discarded_count END FROM runs WHERE run_id=?"))) return false;
    query.addBindValue(sealRunId);
    return query.exec() && query.next() && integerIn(query.value(0), 1, 1)
        && integerIn(query.value(1), 0, std::numeric_limits<qint64>::max())
        && integerIn(query.value(2), 0, std::numeric_limits<qint64>::max())
        && integerIn(query.value(3), 0, std::numeric_limits<qint64>::max())
        && !query.next() && !query.lastError().isValid();
}

bool FleetEventArchiveSql::readPage(QSqlDatabase& database, const QString& runFilter, qint64 cursor, int limit,
              qint64 maximumBytes, const QString& databaseEpoch, QVariantMap* page, QString* error, const std::atomic_bool* cancelled)
{
    if (!page || !error) return false;
    if (!uuid(databaseEpoch) || (!runFilter.isEmpty() && !uuid(runFilter)) || cursor < 0
        || limit < 1 || limit > 256 || maximumBytes < kMiB - 1024LL || maximumBytes > 8LL * kMiB) {
        *error = QStringLiteral("archive-history-request-rejected"); return false;
    }
    if (cancelled && cancelled->load(std::memory_order_acquire)) {
        *error = QStringLiteral("archive-history-cancelled"); return false;
    }
    if (!database.transaction()) { *error = QStringLiteral("archive-history-begin-failed"); return false; }
    qint64 first = 0, last = 0, retired = 0;
    QVariantMap actual;
    bool ok = inspect(database, &actual) && actual.value(QStringLiteral("databaseEpoch")).toString() == databaseEpoch
        && scalar(database, QStringLiteral("SELECT COALESCE(MIN(row_id),0) FROM events"), &first)
        && scalar(database, QStringLiteral("SELECT COALESCE(MAX(row_id),0) FROM events"), &last)
        && scalar(database, QStringLiteral("SELECT CASE WHEN typeof(retired_count)='integer' THEN retired_count END FROM meta WHERE singleton=1"), &retired);
    QVariantMap result{{QStringLiteral("format"), QStringLiteral("merivus-fleet-event-history-page")},
        {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("databaseEpoch"), databaseEpoch},
        {QStringLiteral("runFilter"), runFilter}, {QStringLiteral("afterRowId"), QString::number(cursor)},
        {QStringLiteral("nextRowId"), QString::number(cursor)}, {QStringLiteral("hasMore"), false},
        {QStringLiteral("retainedFromRowId"), QString::number(first)}, {QStringLiteral("retainedThroughRowId"), QString::number(last)},
        {QStringLiteral("retiredCount"), QString::number(retired)},
        {QStringLiteral("cursorPrecedesRetainedHistory"), cursor > 0 && first > 0 && cursor < first - 1},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
        {QStringLiteral("wallClockOrderingUsed"), false}, {QStringLiteral("rows"), QVariantList{}}};
    QVariantList rows;
    QSqlQuery query(database);
    // Bound hostile/corrupt SQLite values before the driver materializes them
    // into QVariant, not only after QByteArray::size() has allocated a blob.
    const QString projection = QStringLiteral(
        "SELECT e.row_id, CASE WHEN typeof(e.run_id)='text' AND length(e.run_id)=36 THEN e.run_id END, "
        "CASE WHEN typeof(e.sequence)='integer' THEN e.sequence END, "
        "CASE WHEN typeof(e.event_utc_ms)='integer' THEN e.event_utc_ms END, "
        "CASE WHEN typeof(e.received_utc_ms)='integer' THEN e.received_utc_ms END, "
        "CASE WHEN typeof(e.received_monotonic_ms)='integer' THEN e.received_monotonic_ms END, "
        "CASE WHEN typeof(e.event_type)='text' AND length(e.event_type)<=64 THEN e.event_type END, "
        "CASE WHEN typeof(e.event_source)='text' AND length(e.event_source)<=64 THEN e.event_source END, "
        "CASE WHEN typeof(e.canonical_json)='blob' AND length(e.canonical_json)<=524288 THEN e.canonical_json END, "
        "CASE WHEN typeof(e.content_hash)='text' AND length(e.content_hash)=64 THEN e.content_hash END, "
        "CASE WHEN typeof(r.clean_shutdown)='integer' THEN r.clean_shutdown END, "
        "CASE WHEN typeof(r.last_sequence)='integer' THEN r.last_sequence END, "
        "CASE WHEN typeof(r.dropped_count)='integer' THEN r.dropped_count END, "
        "CASE WHEN typeof(r.discarded_count)='integer' THEN r.discarded_count END "
        "FROM events e JOIN runs r ON r.run_id=e.run_id ");
    const QString sql = projection + (runFilter.isEmpty()
        ? QStringLiteral("WHERE e.row_id>? ORDER BY e.row_id LIMIT ?")
        : QStringLiteral("WHERE e.run_id=? AND e.row_id>? ORDER BY e.row_id LIMIT ?"));
    if (ok) ok = query.prepare(sql);
    if (ok) {
        if (!runFilter.isEmpty()) query.addBindValue(runFilter);
        query.addBindValue(cursor); query.addBindValue(limit + 1); ok = query.exec();
    }
    qint64 next = cursor;
    bool more = false;
    while (ok) {
        if (cancelled && cancelled->load(std::memory_order_acquire)) {
            query.finish(); database.rollback(); *error = QStringLiteral("archive-history-cancelled"); return false;
        }
        if (!query.next()) break;
        if (cancelled && cancelled->load(std::memory_order_acquire)) {
            query.finish(); database.rollback(); *error = QStringLiteral("archive-history-cancelled"); return false;
        }
        if (rows.size() == limit) { more = true; break; }
        const QByteArray bytes = query.value(8).toByteArray();
        FleetEventArchiveCodec::Record record;
        record.ok = true; record.runId = query.value(1).toString();
        record.sequenceNumber = query.value(2).toLongLong(); record.sequence = QString::number(record.sequenceNumber);
        record.eventUtcMs = query.value(3).toLongLong(); record.receivedUtcMs = query.value(4).toLongLong();
        record.receivedMonotonicMs = query.value(5).toLongLong(); record.eventType = query.value(6).toString();
        record.eventSource = query.value(7).toString(); record.canonicalJson = bytes; record.contentHash = query.value(9).toString();
        const qint64 rowId = query.value(0).toLongLong(), durable = query.value(11).toLongLong();
        const int clean = query.value(10).toInt();
        if (query.value(1).type() != QVariant::String || query.value(6).type() != QVariant::String
            || query.value(7).type() != QVariant::String || query.value(9).type() != QVariant::String
            || query.value(8).type() != QVariant::ByteArray
            || !integerIn(query.value(0), 1, std::numeric_limits<qint64>::max())
            || !integerIn(query.value(2), 1, std::numeric_limits<qint64>::max())
            || !integerIn(query.value(3), 1, 9007199254740991LL) || !integerIn(query.value(4), 1, 9007199254740991LL)
            || !integerIn(query.value(5), 0, 9007199254740991LL) || !integerIn(query.value(10), 0, 1)
            || !integerIn(query.value(11), 0, std::numeric_limits<qint64>::max())
            || !integerIn(query.value(12), 0, std::numeric_limits<qint64>::max())
            || !integerIn(query.value(13), 0, std::numeric_limits<qint64>::max())
            || bytes.isEmpty() || bytes.size() > kMaximumRecordBytes || rowId <= next || record.sequenceNumber > durable
            || (clean != 0 && clean != 1)
            || !recordConsistent(record)) { ok = false; break; }
        bool decoded = false;
        const QVariantMap wrapper = FleetEventArchiveCodec::decodeCanonical(bytes, record.contentHash, &decoded);
        QVariantMap row{{QStringLiteral("rowId"), QString::number(rowId)}, {QStringLiteral("record"), wrapper},
            {QStringLiteral("contentHash"), record.contentHash}, {QStringLiteral("runCleanShutdown"), clean == 1},
            {QStringLiteral("uncleanRunMayHaveLostTail"), clean == 0},
            {QStringLiteral("runDurableSequence"), QString::number(durable)},
            {QStringLiteral("runDroppedCount"), QString::number(query.value(12).toLongLong())},
            {QStringLiteral("runDiscardedCount"), QString::number(query.value(13).toLongLong())}};
        if (!decoded) { ok = false; break; }
        rows.append(row);
        result.insert(QStringLiteral("rows"), rows);
        result.insert(QStringLiteral("nextRowId"), QString::number(rowId));
        // Bound the whole UTF-8 page envelope, not just stored JSON blobs.
        if (QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size() > maximumBytes - 256) {
            rows.removeLast(); result.insert(QStringLiteral("rows"), rows);
            result.insert(QStringLiteral("nextRowId"), QString::number(next)); more = true;
            if (rows.isEmpty()) ok = false;
            break;
        }
        next = rowId;
    }
    if (query.lastError().isValid()) ok = false;
    query.finish();
    if (cancelled && cancelled->load(std::memory_order_acquire)) {
        database.rollback(); *error = QStringLiteral("archive-history-cancelled"); return false;
    }
    result.insert(QStringLiteral("hasMore"), more);
    const int pageBytes = QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size();
    result.insert(QStringLiteral("pageBytesWithoutThisField"), pageBytes);
    if (!ok || QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size() > maximumBytes
        || !database.commit()) { database.rollback(); *error = QStringLiteral("archive-history-rejected"); return false; }
    if (cancelled && cancelled->load(std::memory_order_acquire)) {
        *error = QStringLiteral("archive-history-cancelled"); return false;
    }
    *page = result;
    return true;
}

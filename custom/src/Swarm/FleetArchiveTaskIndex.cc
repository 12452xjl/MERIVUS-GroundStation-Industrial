#include "FleetArchiveTaskIndex.h"

#include "FleetEventArchiveHistoryReader.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumBytes = 16LL * 1024LL * 1024LL;
constexpr int MaximumTasks = 32768;
constexpr int MaximumWindowBatches = 128;
constexpr int PageRows = 64;
const QString Basename = QStringLiteral("fleet-task-index-v1.json");
const QString Format = QStringLiteral("merivus-fleet-archive-task-index");

bool exact(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}
bool string(const QVariant& value) { return value.type() == QVariant::String && !value.isNull(); }
bool flag(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull(); }
bool digest(const QString& value)
{
    if (value.size() != 64) return false;
    for (const QChar ch : value) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
bool uuid(const QString& value)
{
    return !QUuid(value).isNull() && QUuid(value).toString(QUuid::WithoutBraces).toLower() == value;
}
bool counter(const QVariant& value, quint64* output = nullptr, bool positive = false)
{
    if (!string(value)) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok, 10);
    if (!ok || QString::number(number) != text || (positive && number == 0)) return false;
    if (output) *output = number;
    return true;
}
// Stored JSON numbers are decoded as Double by Qt. Only structure integers
// use this gate; policies and native result counters retain actual Int types.
bool integer(const QVariant& value, int low, int high, int* output = nullptr, bool json = false)
{
    if (value.isNull() || (value.type() != QVariant::Int && !(json && value.type() == QVariant::Double))) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < low || number > high || std::floor(number) != number) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}
bool policyValid(const QVariantMap& policy)
{
    const QStringList keys{QStringLiteral("maximumEvents"), QStringLiteral("maximumDatabaseMiB"),
        QStringLiteral("maximumQueueEvents"), QStringLiteral("maximumQueueMiB"), QStringLiteral("batchEventLimit"),
        QStringLiteral("busyTimeoutMs"), QStringLiteral("maximumHistoryMiB")};
    if (!exact(policy, keys)) return false;
    const int lows[] = {1000, 32, 16, 1, 1, 1, 1};
    const int highs[] = {1000000, 4096, 4096, 64, 128, 1000, 8};
    for (int i = 0; i < keys.size(); ++i) {
        const QVariant value = policy.value(keys.at(i));
        switch (value.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
        default: return false;
        }
        bool ok = false;
        const qint64 number = value.toLongLong(&ok);
        if (value.isNull() || !ok || number < lows[i] || number > highs[i]) return false;
    }
    return true;
}
QByteArray bytes(const QVariantMap& value) { return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact); }
QString hash(const QByteArray& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex());
}
bool same(const QVariantMap& a, const QVariantMap& b) { return bytes(a) == bytes(b); }
bool add(quint64& value, quint64 amount = 1)
{
    if (amount > std::numeric_limits<quint64>::max() - value) return false;
    value += amount;
    return true;
}
QString failure(const char* suffix) { return QStringLiteral("archive-task-index-") + QString::fromLatin1(suffix); }
bool scopeValid(const QString& scope)
{
    return scope == QStringLiteral("sealed") || scope == QStringLiteral("sealed-and-offline");
}
QVariantMap searchRequest(const QString& scope, const QVariantMap& cursor, const QString& task = QString())
{
    QVariantMap request{{QStringLiteral("kind"), QStringLiteral("search")}, {QStringLiteral("scope"), scope},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("operation"), QStringLiteral("")}, {QStringLiteral("cursor"), cursor}};
    if (!task.isEmpty()) request.insert(QStringLiteral("taskIdHash"), task);
    return request;
}
bool cursorValid(const QVariant& value, const QString& scope, bool allowEmpty)
{
    if (value.type() != QVariant::Map || value.isNull() || (!allowEmpty && value.toMap().isEmpty())) return false;
    return FleetEventArchiveHistoryReader::searchRequestValid(searchRequest(scope, value.toMap()));
}
bool directoryValid(const QString& directory)
{
    const QString path = QDir::fromNativeSeparators(directory);
    const QFileInfo info(path);
    return !path.isEmpty() && path.size() <= 4096 && !path.contains(QChar::Null)
        && QDir::isAbsolutePath(path) && !path.startsWith(QStringLiteral("//"))
        && QDir::cleanPath(path) == path && info.exists() && info.isDir() && !info.isSymLink()
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == path;
}
bool safeFile(const QString& directory, const QString& path, bool* exists = nullptr)
{
    if (!directoryValid(directory) || path != QDir(directory).filePath(Basename)) return false;
    const QFileInfo info(path);
    const bool present = info.exists();
    if (exists) *exists = present;
    return !info.isSymLink() && (!present || (info.isFile() && info.isReadable() && info.size() > 0
        && info.size() <= MaximumBytes && QDir::fromNativeSeparators(info.canonicalFilePath()) == path
        && info.lastModified().isValid()));
}
bool safeLock(const QString& directory, const QString& path)
{
    if (!directoryValid(directory) || path != QDir(directory).filePath(Basename + QStringLiteral(".lock"))) return false;
    const QFileInfo info(path);
    return !info.isSymLink() && (!info.exists() || (info.isFile() && info.size() <= 16384
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == path));
}
struct Baseline {
    bool exists = false;
    QByteArray content;
    QDateTime modified;
};
bool readFile(const QString& directory, const QString& path, const std::atomic_bool& cancelled, Baseline& result)
{
    result = Baseline{};
    bool exists = false;
    if (cancelled.load() || !safeFile(directory, path, &exists)) return false;
    if (!exists) return true;
    const QFileInfo before(path);
    // Recheck the allocation/read bound on the actual baseline observation,
    // not a QFileInfo fetched earlier by safeFile().
    if (!before.isFile() || before.isSymLink() || before.size() <= 0 || before.size() > MaximumBytes
        || !before.lastModified().isValid() || QDir::fromNativeSeparators(before.canonicalFilePath()) != path) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() != before.size()) return false;
    QByteArray content;
    content.reserve(static_cast<int>(before.size()));
    qint64 remaining = before.size();
    while (remaining > 0) {
        if (cancelled.load()) return false;
        const QByteArray chunk = file.read(qMin<qint64>(remaining, 65536));
        if (chunk.isEmpty() || chunk.size() > remaining) return false;
        content.append(chunk);
        remaining -= chunk.size();
    }
    const bool good = !cancelled.load() && file.read(1).isEmpty() && file.atEnd()
        && file.error() == QFileDevice::NoError && file.size() == before.size();
    file.close();
    const QFileInfo after(path);
    if (!good || file.error() != QFileDevice::NoError || !safeFile(directory, path, &exists) || !exists
        || after.size() != before.size() || after.lastModified() != before.lastModified()) return false;
    result.exists = true;
    result.content = content;
    result.modified = before.lastModified();
    return true;
}
bool baselineMatches(const QString& directory, const QString& path, const Baseline& original,
                     const std::atomic_bool& cancelled)
{
    Baseline current;
    return readFile(directory, path, cancelled, current) && current.exists == original.exists
        && current.content == original.content && current.modified == original.modified;
}
// Enforce bounded nesting before invoking Qt's JSON parser. Exact compact
// reserialization below rejects duplicate keys, invalid UTF-8 and trailing data.
bool jsonBudget(const QByteArray& input)
{
    int depth = 0, tokens = 0, stringBytes = 0;
    bool quoted = false, escaped = false;
    for (const char ch : input) {
        if (quoted) {
            if (++stringBytes > 8192) return false;
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') { quoted = false; stringBytes = 0; }
        } else if (ch == '"') { quoted = true; stringBytes = 0; if (++tokens > 262144) return false; }
        else if (ch == '{' || ch == '[') { if (++depth > 12 || ++tokens > 262144) return false; }
        else if (ch == '}' || ch == ']') { if (--depth < 0) return false; }
    }
    return !quoted && !escaped && depth == 0;
}

struct Checkpoint {
    QString id, scope, inventory;
    int filesTotal = 0;
    bool complete = false;
    QVariantMap cursor;
    QVariantList identities;
    QMap<QString, quint64> tasks;
    quint64 events = 0, unassigned = 0;
};
QVariantMap document(const Checkpoint& point)
{
    QVariantList tasks;
    for (auto it = point.tasks.constBegin(); it != point.tasks.constEnd(); ++it)
        tasks.append(QVariantMap{{QStringLiteral("taskIdHash"), it.key()}, {QStringLiteral("eventCount"), QString::number(it.value())}});
    return {{QStringLiteral("format"), Format}, {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("indexId"), point.id},
        {QStringLiteral("scope"), point.scope}, {QStringLiteral("inventoryFingerprint"), point.inventory},
        {QStringLiteral("filesTotal"), point.filesTotal}, {QStringLiteral("scanComplete"), point.complete},
        {QStringLiteral("cursor"), point.cursor}, {QStringLiteral("fileIdentities"), point.identities},
        {QStringLiteral("indexedEventCount"), QString::number(point.events)},
        {QStringLiteral("unassignedEventCount"), QString::number(point.unassigned)}, {QStringLiteral("tasks"), tasks}};
}
bool identityValid(const QVariantMap& identity, const QString& scope, int index, bool json = false)
{
    if (!exact(identity, {QStringLiteral("fileIndex"), QStringLiteral("basename"), QStringLiteral("databaseEpoch"),
            QStringLiteral("sealRunId"), QStringLiteral("fileFingerprint"), QStringLiteral("offlineCurrentOnly")})
        || !integer(identity.value(QStringLiteral("fileIndex")), index, index, nullptr, json)
        || !flag(identity.value(QStringLiteral("offlineCurrentOnly")))) return false;
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("databaseEpoch"), QStringLiteral("sealRunId"), QStringLiteral("fileFingerprint")})
        if (!string(identity.value(key))) return false;
    const QString basename = identity.value(QStringLiteral("basename")).toString();
    const QString epoch = identity.value(QStringLiteral("databaseEpoch")).toString();
    const QString seal = identity.value(QStringLiteral("sealRunId")).toString();
    if (!uuid(epoch) || !digest(identity.value(QStringLiteral("fileFingerprint")).toString())) return false;
    if (identity.value(QStringLiteral("offlineCurrentOnly")).toBool())
        return scope == QStringLiteral("sealed-and-offline") && basename == QStringLiteral("fleet-events-v1.sqlite") && seal.isEmpty();
    return uuid(seal) && basename == QStringLiteral("fleet-events-sealed-") + epoch + QLatin1Char('-') + seal + QStringLiteral(".sqlite");
}
bool decode(const QByteArray& input, Checkpoint& point)
{
    point = Checkpoint{};
    if (input.isEmpty() || input.size() > MaximumBytes || !jsonBudget(input)) return false;
    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(input, &parseError);
    if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) return false;
    const QVariantMap map = parsed.object().toVariantMap();
    if (bytes(map) != input || !exact(map, {QStringLiteral("format"), QStringLiteral("schemaVersion"), QStringLiteral("indexId"),
        QStringLiteral("scope"), QStringLiteral("inventoryFingerprint"), QStringLiteral("filesTotal"), QStringLiteral("scanComplete"),
        QStringLiteral("cursor"), QStringLiteral("fileIdentities"), QStringLiteral("indexedEventCount"),
        QStringLiteral("unassignedEventCount"), QStringLiteral("tasks")})
        || !integer(map.value(QStringLiteral("schemaVersion")), 1, 1, nullptr, true)
        || map.value(QStringLiteral("format")) != QVariant(Format)) return false;
    for (const QString& key : {QStringLiteral("indexId"), QStringLiteral("scope"), QStringLiteral("inventoryFingerprint")})
        if (!string(map.value(key))) return false;
    Checkpoint candidate;
    candidate.id = map.value(QStringLiteral("indexId")).toString();
    candidate.scope = map.value(QStringLiteral("scope")).toString();
    candidate.inventory = map.value(QStringLiteral("inventoryFingerprint")).toString();
    if (map.value(QStringLiteral("cursor")).type() != QVariant::Map || map.value(QStringLiteral("cursor")).isNull()) return false;
    QVariantMap normalizedCursor = map.value(QStringLiteral("cursor")).toMap();
    if (!normalizedCursor.isEmpty()) {
        int fileIndex = 0;
        if (!integer(normalizedCursor.value(QStringLiteral("fileIndex")), 0, 128, &fileIndex, true)) return false;
        normalizedCursor.insert(QStringLiteral("fileIndex"), fileIndex);
    }
    if (!uuid(candidate.id) || !scopeValid(candidate.scope) || !digest(candidate.inventory)
        || !integer(map.value(QStringLiteral("filesTotal")), 0, candidate.scope == QStringLiteral("sealed") ? 128 : 129, &candidate.filesTotal, true)
        || !flag(map.value(QStringLiteral("scanComplete"))) || !cursorValid(normalizedCursor, candidate.scope, true)
        || map.value(QStringLiteral("fileIdentities")).type() != QVariant::List || map.value(QStringLiteral("fileIdentities")).isNull()
        || map.value(QStringLiteral("tasks")).type() != QVariant::List || map.value(QStringLiteral("tasks")).isNull()
        || !counter(map.value(QStringLiteral("indexedEventCount")), &candidate.events)
        || !counter(map.value(QStringLiteral("unassignedEventCount")), &candidate.unassigned)) return false;
    candidate.complete = map.value(QStringLiteral("scanComplete")).toBool();
    candidate.cursor = normalizedCursor;
    if (candidate.complete != candidate.cursor.isEmpty()) return false;
    const QVariantList identities = map.value(QStringLiteral("fileIdentities")).toList();
    // Every checkpoint freezes the WHOLE inventory, including files whose
    // rows have not been indexed yet. Partial-index record browsing therefore
    // rechecks earlier record pages too, not only an index's scanned prefix.
    if (identities.size() != candidate.filesTotal) return false;
    for (int i = 0; i < identities.size(); ++i) {
        if (identities.at(i).type() != QVariant::Map || identities.at(i).isNull()
            || !identityValid(identities.at(i).toMap(), candidate.scope, i, true)) return false;
        QVariantMap normalized = identities.at(i).toMap();
        normalized.insert(QStringLiteral("fileIndex"), i);
        if (normalized.value(QStringLiteral("offlineCurrentOnly")).toBool() && i + 1 != candidate.filesTotal) return false;
        candidate.identities.append(normalized);
    }
    if (!candidate.complete) {
        const int fileIndex = candidate.cursor.value(QStringLiteral("fileIndex")).toInt();
        quint64 after = 0;
        if (candidate.cursor.value(QStringLiteral("inventoryFingerprint")).toString() != candidate.inventory
            || !counter(candidate.cursor.value(QStringLiteral("afterRowId")), &after)
            || fileIndex >= candidate.filesTotal) return false;
        if (after) {
            const QVariantMap active = candidate.identities.at(fileIndex).toMap();
            if (candidate.cursor.value(QStringLiteral("fileFingerprint")) != active.value(QStringLiteral("fileFingerprint"))
                || candidate.cursor.value(QStringLiteral("databaseEpoch")) != active.value(QStringLiteral("databaseEpoch"))) return false;
        }
        candidate.cursor.insert(QStringLiteral("fileIndex"), fileIndex);
    }
    const QVariantList tasks = map.value(QStringLiteral("tasks")).toList();
    if (tasks.size() > MaximumTasks) return false;
    QString previous;
    quint64 total = candidate.unassigned;
    for (const QVariant& value : tasks) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap row = value.toMap();
        quint64 count = 0;
        if (!exact(row, {QStringLiteral("taskIdHash"), QStringLiteral("eventCount")})
            || !string(row.value(QStringLiteral("taskIdHash"))) || !digest(row.value(QStringLiteral("taskIdHash")).toString())
            || row.value(QStringLiteral("taskIdHash")).toString() <= previous || !counter(row.value(QStringLiteral("eventCount")), &count, true)
            || !add(total, count)) return false;
        previous = row.value(QStringLiteral("taskIdHash")).toString();
        candidate.tasks.insert(previous, count);
    }
    if (total != candidate.events || bytes(document(candidate)) != input) return false;
    point = candidate;
    return true;
}
bool mergeIdentities(Checkpoint& point, const QVariantList& pages)
{
    const QStringList keys{QStringLiteral("fileIndex"), QStringLiteral("basename"), QStringLiteral("databaseEpoch"),
        QStringLiteral("sealRunId"), QStringLiteral("fileFingerprint"), QStringLiteral("offlineCurrentOnly")};
    for (const QVariant& page : pages) {
        const QVariantMap source = page.toMap();
        const int index = source.value(QStringLiteral("fileIndex")).toInt();
        QVariantMap identity;
        for (const QString& key : keys) identity.insert(key, source.value(key));
        if (index < 0 || index >= point.filesTotal || index > point.identities.size()
            || !identityValid(identity, point.scope, index)) return false;
        if (index == point.identities.size()) point.identities.append(identity);
        else if (!same(identity, point.identities.at(index).toMap())) return false;
    }
    return true;
}
bool scanWindow(const QString& directory, const QVariantMap& policy, const std::atomic_bool& cancelled,
                Checkpoint& point, int& batches, int& rows, QString& error)
{
    for (int i = 0; i < MaximumWindowBatches && !point.complete; ++i) {
        if (cancelled.load()) { error = failure("cancelled-before-checkpoint"); return false; }
        const QVariantMap request = searchRequest(point.scope, point.cursor);
        QVariantMap found;
        if (!FleetEventArchiveHistoryReader::searchInWorker(directory, policy, request, cancelled, &found, &error)
            || !FleetEventArchiveHistoryReader::searchResultMatches(found, request, policy)) {
            if (error.isEmpty()) error = failure("search-result-invalid");
            return false;
        }
        const QString inventory = found.value(QStringLiteral("inventoryFingerprint")).toString();
        const int files = found.value(QStringLiteral("filesTotal")).toInt();
        if (point.inventory.isEmpty()) { point.inventory = inventory; point.filesTotal = files; }
        if (point.inventory != inventory || point.filesTotal != files || !mergeIdentities(point, found.value(QStringLiteral("scannedPages")).toList())) {
            error = failure("source-identity-changed"); return false;
        }
        const QVariantList matches = found.value(QStringLiteral("rows")).toList();
        if (found.value(QStringLiteral("scannedRows")).toInt() != matches.size()) {
            error = failure("unfiltered-row-count-mismatch"); return false;
        }
        for (const QVariant& value : matches) {
            if (cancelled.load()) { error = failure("cancelled-before-checkpoint"); return false; }
            const QVariantMap payload = value.toMap().value(QStringLiteral("row")).toMap()
                .value(QStringLiteral("record")).toMap().value(QStringLiteral("event")).toMap().value(QStringLiteral("payload")).toMap();
            if (!add(point.events)) { error = failure("counter-exhausted"); return false; }
            const QVariant task = payload.value(QStringLiteral("taskIdHash"));
            // The validated v3 capture-gap projection intentionally retains
            // an explicit empty taskIdHash. It is unassigned, never hash("").
            if (!payload.contains(QStringLiteral("taskIdHash")) || (string(task) && task.toString().isEmpty())) {
                if (!add(point.unassigned)) { error = failure("counter-exhausted"); return false; }
            } else {
                if (!string(task) || !digest(task.toString())) { error = failure("row-task-hash-invalid"); return false; }
                auto it = point.tasks.find(task.toString());
                if (it == point.tasks.end()) {
                    if (point.tasks.size() >= MaximumTasks) { error = failure("task-capacity-exceeded"); return false; }
                    point.tasks.insert(task.toString(), 1);
                } else if (!add(it.value())) { error = failure("counter-exhausted"); return false; }
            }
        }
        ++batches;
        rows += matches.size();
        point.complete = found.value(QStringLiteral("scanComplete")).toBool();
        point.cursor = found.value(QStringLiteral("nextCursor")).toMap();
    }
    return true;
}
bool freezeAllSources(const QString& directory, const QVariantMap& policy, const std::atomic_bool& cancelled,
                      Checkpoint& point, QString& error)
{
    QVariantList all;
    if (!FleetEventArchiveHistoryReader::snapshotIndexSourcesInWorker(directory, policy, point.scope,
            point.inventory, cancelled, &all, &error)) return false;
    if (all.size() != point.filesTotal || point.identities.size() > all.size()) {
        error = failure("full-source-count-changed"); return false;
    }
    for (int i = 0; i < all.size(); ++i) {
        if (cancelled.load()) { error = failure("cancelled-before-checkpoint"); return false; }
        if (all.at(i).type() != QVariant::Map || all.at(i).isNull()
            || !identityValid(all.at(i).toMap(), point.scope, i)
            || (i < point.identities.size() && !same(all.at(i).toMap(), point.identities.at(i).toMap()))) {
            error = failure("consumed-source-changed-before-freeze"); return false;
        }
    }
    point.identities = all;
    return true;
}
QVariantMap metadata(const Checkpoint& point, int batches, int rows)
{
    return {{QStringLiteral("inventoryFingerprint"), point.inventory}, {QStringLiteral("scanComplete"), point.complete},
        {QStringLiteral("taskCount"), point.tasks.size()}, {QStringLiteral("indexedEventCount"), QString::number(point.events)},
        {QStringLiteral("unassignedEventCount"), QString::number(point.unassigned)}, {QStringLiteral("sourceFileCount"), point.identities.size()},
        {QStringLiteral("windowBatches"), batches}, {QStringLiteral("windowScannedRows"), rows}};
}
QVariantMap pageFor(const Checkpoint& point, const QString& after)
{
    QVariantList rows;
    auto it = after.isEmpty() ? point.tasks.constBegin() : point.tasks.upperBound(after);
    for (int i = 0; i < PageRows && it != point.tasks.constEnd(); ++i, ++it)
        rows.append(QVariantMap{{QStringLiteral("taskIdHash"), it.key()}, {QStringLiteral("eventCount"), QString::number(it.value())}});
    const bool more = it != point.tasks.constEnd();
    return {{QStringLiteral("afterTask"), after}, {QStringLiteral("nextAfterTask"), more ? rows.last().toMap().value(QStringLiteral("taskIdHash")) : QVariant(QStringLiteral(""))},
        {QStringLiteral("hasMore"), more}, {QStringLiteral("totalCount"), point.tasks.size()}, {QStringLiteral("rows"), rows}};
}
QVariantMap publicResult(const Checkpoint& point, const QString& action, int batches, int rows,
                        bool committed, bool cancelledAfterCommit, const QVariantMap& page = QVariantMap(),
                        const QVariantMap& records = QVariantMap())
{
    return {{QStringLiteral("format"), Format}, {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("action"), action}, {QStringLiteral("scope"), point.scope}, {QStringLiteral("indexId"), point.id},
        {QStringLiteral("checkpointHash"), hash(bytes(document(point)))}, {QStringLiteral("checkpointCommitted"), committed},
        {QStringLiteral("cancelObservedAfterCommit"), cancelledAfterCommit},
        {QStringLiteral("checkpoint"), metadata(point, batches, rows)}, {QStringLiteral("page"), page}, {QStringLiteral("records"), records},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
bool writeCheckpoint(const QString& directory, const QString& path, const Baseline& baseline,
                     const Checkpoint& point, const QVariantMap& policy, const std::atomic_bool& cancelled, QString& error)
{
    const QByteArray content = bytes(document(point));
    Checkpoint checked;
    if (content.isEmpty() || content.size() > MaximumBytes || !decode(content, checked)) { error = failure("checkpoint-capacity-or-structure-invalid"); return false; }
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (cancelled.load() || !file.open(QIODevice::WriteOnly)) { error = failure("checkpoint-open-failed-or-cancelled"); return false; }
    qint64 offset = 0;
    while (offset < content.size()) {
        if (cancelled.load()) { file.cancelWriting(); error = failure("cancelled-before-checkpoint"); return false; }
        const qint64 length = qMin<qint64>(65536, content.size() - offset);
        if (file.write(content.constData() + offset, length) != length) { file.cancelWriting(); error = failure("checkpoint-write-failed"); return false; }
        offset += length;
    }
    if (!file.flush() || file.error() != QFileDevice::NoError
        || !FleetEventArchiveHistoryReader::verifyIndexSourcesInWorker(directory, policy, point.scope,
            point.inventory, point.identities, cancelled, &error)
        || !baselineMatches(directory, path, baseline, cancelled) || cancelled.load()) {
        file.cancelWriting(); if (error.isEmpty()) error = failure("checkpoint-baseline-changed-or-cancelled"); return false;
    }
    if (!file.commit()) { error = failure("checkpoint-commit-failed"); return false; }
    return true; // A later cancellation cannot undo this atomic replacement.
}
} // namespace

bool FleetArchiveTaskIndex::requestValid(const QVariantMap& request)
{
    if (!exact(request, {QStringLiteral("kind"), QStringLiteral("action"), QStringLiteral("scope"), QStringLiteral("indexId"),
            QStringLiteral("checkpointHash"), QStringLiteral("afterTask"), QStringLiteral("taskIdHash"), QStringLiteral("cursor")})) return false;
    for (const QString& key : {QStringLiteral("kind"), QStringLiteral("action"), QStringLiteral("scope"), QStringLiteral("indexId"),
        QStringLiteral("checkpointHash"), QStringLiteral("afterTask"), QStringLiteral("taskIdHash")}) if (!string(request.value(key))) return false;
    if (request.value(QStringLiteral("kind")) != QVariant(QStringLiteral("task-index"))) return false;
    const QString action = request.value(QStringLiteral("action")).toString(), scope = request.value(QStringLiteral("scope")).toString();
    const QStringList actions{QStringLiteral("create"), QStringLiteral("open"), QStringLiteral("resume"), QStringLiteral("page"),
        QStringLiteral("records"), QStringLiteral("continue-records"), QStringLiteral("rebuild")};
    if (!actions.contains(action) || !scopeValid(scope) || !cursorValid(request.value(QStringLiteral("cursor")), scope, true)) return false;
    const bool first = action == QStringLiteral("create") || action == QStringLiteral("open");
    const QString id = request.value(QStringLiteral("indexId")).toString(), checkpoint = request.value(QStringLiteral("checkpointHash")).toString();
    if (first ? (!id.isEmpty() || !checkpoint.isEmpty())
        : action == QStringLiteral("rebuild") ? !((id.isEmpty() && checkpoint.isEmpty()) || (uuid(id) && digest(checkpoint)))
                                             : (!uuid(id) || !digest(checkpoint))) return false;
    const QString after = request.value(QStringLiteral("afterTask")).toString(), task = request.value(QStringLiteral("taskIdHash")).toString();
    const QVariantMap cursor = request.value(QStringLiteral("cursor")).toMap();
    if (action == QStringLiteral("records") || action == QStringLiteral("continue-records"))
        return after.isEmpty() && digest(task) && (action == QStringLiteral("records") ? cursor.isEmpty() : !cursor.isEmpty());
    return task.isEmpty() && cursor.isEmpty() && (action == QStringLiteral("page") ? (after.isEmpty() || digest(after)) : after.isEmpty());
}

bool FleetArchiveTaskIndex::execute(const QString& directorySource, const QVariantMap& policySource, const QVariantMap& requestSource,
    const std::atomic_bool& cancelled, QVariantMap* result, QString* error)
{
    // Freeze borrowed arguments before clearing possibly aliased output maps.
    const QString directory = directorySource;
    const QVariantMap policy = policySource, request = requestSource;
    if (!result || !error) return false;
    result->clear(); error->clear();
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application || QThread::currentThread() == application->thread()) { *error = failure("worker-thread-required"); return false; }
    if (!requestValid(request) || !policyValid(policy) || !directoryValid(directory)) { *error = failure("configuration-invalid"); return false; }
    const QString path = QDir(directory).filePath(Basename), lockPath = path + QStringLiteral(".lock");
    if (!safeLock(directory, lockPath) || cancelled.load()) { *error = failure("lock-path-invalid-or-cancelled"); return false; }
    QLockFile lock(lockPath);
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0) || !safeLock(directory, lockPath)) { *error = failure("cooperative-lock-unavailable"); return false; }
    Baseline baseline;
    if (!readFile(directory, path, cancelled, baseline)) { *error = failure("checkpoint-read-failed-or-cancelled"); return false; }
    const QString action = request.value(QStringLiteral("action")).toString(), scope = request.value(QStringLiteral("scope")).toString();
    const bool creating = action == QStringLiteral("create"), rebuilding = action == QStringLiteral("rebuild");
    Checkpoint point;
    if (creating) {
        if (baseline.exists) { *error = failure("checkpoint-already-exists"); return false; }
    } else {
        if (!baseline.exists || !decode(baseline.content, point)) { *error = failure("checkpoint-invalid-or-missing"); return false; }
        const bool explicitIdentity = !request.value(QStringLiteral("indexId")).toString().isEmpty();
        if ((!rebuilding && scope != point.scope) || (action != QStringLiteral("open") && (!rebuilding || explicitIdentity)
            && (request.value(QStringLiteral("indexId")).toString() != point.id
                || request.value(QStringLiteral("checkpointHash")).toString() != hash(baseline.content)))) {
            *error = failure("checkpoint-identity-changed"); return false;
        }
        if (!rebuilding && !FleetEventArchiveHistoryReader::verifyIndexSourcesInWorker(directory, policy, point.scope,
                point.inventory, point.identities, cancelled, error)) return false;
    }
    if (creating || rebuilding) {
        point = Checkpoint{};
        point.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        point.scope = scope;
    }
    int batches = 0, rows = 0;
    bool committed = false;
    QVariantMap page, records;
    if (creating || rebuilding || action == QStringLiteral("resume")) {
        if (!point.complete) {
            if (!scanWindow(directory, policy, cancelled, point, batches, rows, *error)) return false;
            if ((creating || rebuilding) && !freezeAllSources(directory, policy, cancelled, point, *error)) return false;
            // Validate the entire eventual receipt before the commit point.
            // Post-commit cancellation may only change the one factual flag.
            if (!resultMatches(publicResult(point, action, batches, rows, true, false), request, policy)) {
                *error = failure("result-contract-invalid"); return false;
            }
            if (!writeCheckpoint(directory, path, baseline, point, policy, cancelled, *error)) return false;
            committed = true;
        }
    } else if (action == QStringLiteral("page")) {
        const QString after = request.value(QStringLiteral("afterTask")).toString();
        if (!after.isEmpty() && !point.tasks.contains(after)) { *error = failure("task-page-anchor-not-in-checkpoint"); return false; }
        page = pageFor(point, after);
    } else if (action == QStringLiteral("records") || action == QStringLiteral("continue-records")) {
        const QString task = request.value(QStringLiteral("taskIdHash")).toString();
        if (!point.tasks.contains(task)) { *error = failure("task-not-in-checkpoint"); return false; }
        const QVariantMap query = searchRequest(scope, request.value(QStringLiteral("cursor")).toMap(), task);
        if (!FleetEventArchiveHistoryReader::searchInWorker(directory, policy, query, cancelled, &records, error)
            || !FleetEventArchiveHistoryReader::searchResultMatches(records, query, policy)
            || records.value(QStringLiteral("inventoryFingerprint")).toString() != point.inventory) {
            if (error->isEmpty()) *error = failure("record-result-source-invalid"); return false;
        }
    }
    // Pure browsing must also retain the exact checkpoint baseline. For a
    // committed checkpoint the new bytes/id below are the factual outcome.
    if (!committed && (!baselineMatches(directory, path, baseline, cancelled)
        || !FleetEventArchiveHistoryReader::verifyIndexSourcesInWorker(directory, policy, point.scope,
            point.inventory, point.identities, cancelled, error))) {
        if (error->isEmpty()) *error = failure("checkpoint-or-source-changed"); return false;
    }
    const QVariantMap candidate = publicResult(point, action, batches, rows, committed,
        committed && cancelled.load(), page, records);
    if (!committed && !resultMatches(candidate, request, policy)) { *error = failure("result-contract-invalid"); return false; }
    if (!committed && cancelled.load()) { *error = failure("cancelled-before-delivery"); return false; }
    *result = candidate;
    return true;
}

bool FleetArchiveTaskIndex::resultMatches(const QVariantMap& result, const QVariantMap& request, const QVariantMap& policy)
{
    if (!requestValid(request) || !policyValid(policy) || !exact(result, {QStringLiteral("format"), QStringLiteral("schemaVersion"),
        QStringLiteral("action"), QStringLiteral("scope"), QStringLiteral("indexId"), QStringLiteral("checkpointHash"),
        QStringLiteral("checkpointCommitted"), QStringLiteral("cancelObservedAfterCommit"), QStringLiteral("checkpoint"),
        QStringLiteral("page"), QStringLiteral("records"), QStringLiteral("historicalOnly"), QStringLiteral("authenticated"),
        QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !string(result.value(QStringLiteral("format"))) || result.value(QStringLiteral("format")) != QVariant(Format)
        || !integer(result.value(QStringLiteral("schemaVersion")), 1, 1)
        || !string(result.value(QStringLiteral("action"))) || !string(result.value(QStringLiteral("scope")))
        || result.value(QStringLiteral("action")) != request.value(QStringLiteral("action"))
        || result.value(QStringLiteral("scope")) != request.value(QStringLiteral("scope"))) return false;
    for (const QString& key : {QStringLiteral("indexId"), QStringLiteral("checkpointHash")}) if (!string(result.value(key))) return false;
    if (!uuid(result.value(QStringLiteral("indexId")).toString()) || !digest(result.value(QStringLiteral("checkpointHash")).toString())) return false;
    for (const QString& key : {QStringLiteral("checkpointCommitted"), QStringLiteral("cancelObservedAfterCommit"), QStringLiteral("historicalOnly"),
        QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!flag(result.value(key))) return false;
    if (!result.value(QStringLiteral("historicalOnly")).toBool() || result.value(QStringLiteral("authenticated")).toBool()
        || result.value(QStringLiteral("permissionsRestored")).toBool() || result.value(QStringLiteral("executionPermit")).toBool()
        || result.value(QStringLiteral("flightCommandReleased")).toBool()) return false;
    for (const QString& key : {QStringLiteral("checkpoint"), QStringLiteral("page"), QStringLiteral("records")})
        if (result.value(key).type() != QVariant::Map || result.value(key).isNull()) return false;
    const QString action = request.value(QStringLiteral("action")).toString();
    const bool committed = result.value(QStringLiteral("checkpointCommitted")).toBool();
    const bool writeAction = action == QStringLiteral("create") || action == QStringLiteral("rebuild") || action == QStringLiteral("resume");
    if ((!writeAction && committed) || (result.value(QStringLiteral("cancelObservedAfterCommit")).toBool() && !committed)
        || ((action == QStringLiteral("create") || action == QStringLiteral("rebuild")) && !committed)) return false;
    if (action != QStringLiteral("create") && action != QStringLiteral("open") && action != QStringLiteral("rebuild")
        && result.value(QStringLiteral("indexId")) != request.value(QStringLiteral("indexId"))) return false;
    if (!committed && action != QStringLiteral("open") && result.value(QStringLiteral("checkpointHash")) != request.value(QStringLiteral("checkpointHash"))) return false;
    if (action == QStringLiteral("rebuild") && result.value(QStringLiteral("indexId")) == request.value(QStringLiteral("indexId"))) return false;
    const QVariantMap checkpoint = result.value(QStringLiteral("checkpoint")).toMap();
    int taskCount = 0, sourceFiles = 0, window = 0, scanned = 0;
    quint64 events = 0, unassigned = 0;
    if (!exact(checkpoint, {QStringLiteral("inventoryFingerprint"), QStringLiteral("scanComplete"), QStringLiteral("taskCount"),
        QStringLiteral("indexedEventCount"), QStringLiteral("unassignedEventCount"), QStringLiteral("sourceFileCount"),
        QStringLiteral("windowBatches"), QStringLiteral("windowScannedRows")})
        || !string(checkpoint.value(QStringLiteral("inventoryFingerprint"))) || !digest(checkpoint.value(QStringLiteral("inventoryFingerprint")).toString())
        || !flag(checkpoint.value(QStringLiteral("scanComplete"))) || !integer(checkpoint.value(QStringLiteral("taskCount")), 0, MaximumTasks, &taskCount)
        || !counter(checkpoint.value(QStringLiteral("indexedEventCount")), &events) || !counter(checkpoint.value(QStringLiteral("unassignedEventCount")), &unassigned)
        || unassigned > events || quint64(taskCount) > events - unassigned || ((taskCount == 0) != (events == unassigned))
        || !integer(checkpoint.value(QStringLiteral("sourceFileCount")), 0, request.value(QStringLiteral("scope")) == QVariant(QStringLiteral("sealed")) ? 128 : 129, &sourceFiles)
        || !integer(checkpoint.value(QStringLiteral("windowBatches")), 0, MaximumWindowBatches, &window)
        || !integer(checkpoint.value(QStringLiteral("windowScannedRows")), 0, MaximumWindowBatches * PageRows, &scanned)
        || scanned > window * PageRows || quint64(scanned) > events || (committed ? window < 1 : (window != 0 || scanned != 0))
        || (sourceFiles == 0 && (events != 0 || !checkpoint.value(QStringLiteral("scanComplete")).toBool()
            || request.value(QStringLiteral("scope")) != QVariant(QStringLiteral("sealed"))))
        || (action == QStringLiteral("resume") && !committed && !checkpoint.value(QStringLiteral("scanComplete")).toBool())) return false;
    const QVariantMap page = result.value(QStringLiteral("page")).toMap(), records = result.value(QStringLiteral("records")).toMap();
    if (action == QStringLiteral("page")) {
        if (!exact(page, {QStringLiteral("afterTask"), QStringLiteral("nextAfterTask"), QStringLiteral("hasMore"), QStringLiteral("totalCount"), QStringLiteral("rows")})
            || !string(page.value(QStringLiteral("afterTask"))) || page.value(QStringLiteral("afterTask")) != request.value(QStringLiteral("afterTask"))
            || !string(page.value(QStringLiteral("nextAfterTask"))) || !flag(page.value(QStringLiteral("hasMore")))
            || !integer(page.value(QStringLiteral("totalCount")), taskCount, taskCount)
            || page.value(QStringLiteral("rows")).type() != QVariant::List || page.value(QStringLiteral("rows")).isNull()) return false;
        const QVariantList rows = page.value(QStringLiteral("rows")).toList();
        if (rows.size() > PageRows || rows.size() > taskCount) return false;
        QString previous = request.value(QStringLiteral("afterTask")).toString();
        quint64 pageEvents = 0;
        for (const QVariant& value : rows) {
            if (value.type() != QVariant::Map || value.isNull()) return false;
            const QVariantMap row = value.toMap();
            quint64 count = 0;
            if (!exact(row, {QStringLiteral("taskIdHash"), QStringLiteral("eventCount")}) || !string(row.value(QStringLiteral("taskIdHash")))
                || !digest(row.value(QStringLiteral("taskIdHash")).toString()) || row.value(QStringLiteral("taskIdHash")).toString() <= previous
                || !counter(row.value(QStringLiteral("eventCount")), &count, true) || count > events - unassigned
                || !add(pageEvents, count) || pageEvents > events - unassigned) return false;
            previous = row.value(QStringLiteral("taskIdHash")).toString();
        }
        const bool more = page.value(QStringLiteral("hasMore")).toBool();
        if (more ? (rows.size() != PageRows || taskCount <= PageRows || page.value(QStringLiteral("nextAfterTask")).toString() != previous)
                 : !page.value(QStringLiteral("nextAfterTask")).toString().isEmpty()) return false;
        if (request.value(QStringLiteral("afterTask")).toString().isEmpty() && !more
            && (rows.size() != taskCount || pageEvents != events - unassigned)) return false;
    } else if (!page.isEmpty()) return false;
    if (action == QStringLiteral("records") || action == QStringLiteral("continue-records")) {
        const QVariantMap query = searchRequest(request.value(QStringLiteral("scope")).toString(), request.value(QStringLiteral("cursor")).toMap(),
            request.value(QStringLiteral("taskIdHash")).toString());
        if (!FleetEventArchiveHistoryReader::searchResultMatches(records, query, policy)
            || records.value(QStringLiteral("inventoryFingerprint")) != checkpoint.value(QStringLiteral("inventoryFingerprint"))) return false;
    } else if (!records.isEmpty()) return false;
    const QByteArray encoded = bytes(result);
    return !encoded.isEmpty() && encoded.size() <= policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * 1024LL * 1024LL - 512;
}

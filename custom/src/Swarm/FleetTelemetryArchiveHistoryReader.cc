#include "FleetTelemetryArchiveHistoryReader.h"
#include "FleetTelemetryArchiveCodec.h"
#include "FleetArchiveDirectoryInventory.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVariantList>

#include <algorithm>
#include <limits>

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr int kMaximumJsonBytes = 4096;
constexpr int kMaximumStoredLineBytes = kMaximumJsonBytes + 1;
constexpr qint64 kFacadeReserve = 1024;
constexpr int kSearchMaximumFiles = 128;
constexpr int kSearchMaximumRows = 64;
constexpr qint64 kSearchMaximumFileBytes = 64LL * kMiB;
constexpr qint64 kSearchMaximumResultBytes = kMiB;
constexpr qint64 kSearchReadByteBudget = 130LL * kMiB;

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull() && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool identifier(const QString& value)
{
    if (value.isEmpty() || value.size() > 128) return false;
    for (const QChar ch : value) if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
        || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z')) || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || ch == QLatin1Char('_') || ch == QLatin1Char('-'))) return false;
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
    const qint64 number = value.toLongLong(&ok);
    return ok && number >= minimum && number <= maximum;
}

bool decimal(const QString& text, qint64* value)
{
    if (text.isEmpty() || text.size() > 19 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = text.toLongLong(&ok, 10);
    if (!ok || parsed < 0 || QString::number(parsed) != text) return false;
    if (value) *value = parsed;
    return true;
}

bool unsignedSequence(const QString& text, quint64* value)
{
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (const QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = text.toULongLong(&ok, 10);
    if (!ok || parsed == 0 || QString::number(parsed) != text) return false;
    if (value) *value = parsed;
    return true;
}

bool sha256(const QString& value, bool allowEmpty = false)
{
    if (allowEmpty && value.isEmpty()) return true;
    if (value.size() != 64) return false;
    for (const QChar ch : value) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool basename(const QString& name, QString* runId)
{
    const QString prefix = QStringLiteral("fleet-telemetry-");
    const QString suffix = QStringLiteral(".ndjson");
    if (!name.startsWith(prefix) || !name.endsWith(suffix) || name.size() != prefix.size() + 36 + suffix.size()) return false;
    const QString parsed = name.mid(prefix.size(), 36);
    if (!uuid(parsed)) return false;
    if (runId) *runId = parsed;
    return true;
}

bool exactKeys(const QVariantMap& value, const QStringList& keys)
{
    if (value.size() != keys.size()) return false;
    for (const QString& key : keys) if (!value.contains(key)) return false;
    return true;
}

bool actualInt(const QVariant& value, int minimum, int maximum)
{
    return value.type() == QVariant::Int && !value.isNull()
        && value.toInt() >= minimum && value.toInt() <= maximum;
}

bool actualBool(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool actualString(const QVariant& value, int maximumLength)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString().size() <= maximumLength;
}

QString ownString(const QString& value)
{
    return value.isEmpty() ? QStringLiteral("") : QString(value.constData(), value.size());
}

QString ownedSearchText(const QVariantMap& request, const QString& value)
{
    return request.value(QStringLiteral("kind")).type() == QVariant::String
        && request.value(QStringLiteral("kind")).toString() == QStringLiteral("search") ? ownString(value) : value;
}

bool directoryIdentityValid(const QVariant& value, bool allowEmpty)
{
    if (!actualString(value, 512) || (!allowEmpty && value.toString().isEmpty())) return false;
    for (QChar character : value.toString())
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool searchPolicyValid(const QVariantMap& policy)
{
    // Keep the broad native catalog/page policy. Search still never scans a
    // file above its separate 64 MiB hard cap; that row remains unavailable.
    return FleetTelemetryArchiveCodec::readPolicyValid(policy);
}

bool ownedDescriptor(const QVariant& value, QVariantMap* output)
{
    if (!output || value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    if (!exactKeys(row, {QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("fileBytes"),
            QStringLiteral("modifiedUtcMs"), QStringLiteral("headerHash"), QStringLiteral("headerValid"),
            QStringLiteral("positionTelemetryConsent"), QStringLiteral("errorCode")})) return false;
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("fileBytes"),
            QStringLiteral("modifiedUtcMs"), QStringLiteral("headerHash"), QStringLiteral("errorCode")})
        if (!actualString(row.value(key), 128)) return false;
    if (row.value(QStringLiteral("headerValid")).type() != QVariant::Bool || row.value(QStringLiteral("headerValid")).isNull()
        || row.value(QStringLiteral("positionTelemetryConsent")).type() != QVariant::Bool
        || row.value(QStringLiteral("positionTelemetryConsent")).isNull()) return false;
    QString runId;
    qint64 bytes = 0, modified = 0;
    if (!basename(row.value(QStringLiteral("basename")).toString(), &runId)
        || row.value(QStringLiteral("runId")).toString() != runId
        || !decimal(row.value(QStringLiteral("fileBytes")).toString(), &bytes)
        || !decimal(row.value(QStringLiteral("modifiedUtcMs")).toString(), &modified)) return false;
    const bool valid = row.value(QStringLiteral("headerValid")).toBool();
    const QString error = row.value(QStringLiteral("errorCode")).toString();
    if (valid ? (bytes < 1 || modified < 1 || !sha256(row.value(QStringLiteral("headerHash")).toString()) || !error.isEmpty())
              : (!row.value(QStringLiteral("headerHash")).toString().isEmpty()
                    || row.value(QStringLiteral("positionTelemetryConsent")).toBool() || !identifier(error))) return false;
    QVariantMap owned;
    for (auto it = row.constBegin(); it != row.constEnd(); ++it)
        owned.insert(ownString(it.key()), it.value().type() == QVariant::String
            ? QVariant(ownString(it.value().toString())) : it.value());
    *output = owned;
    return true;
}

QVariantMap ownedRequest(const QVariantMap& request, const QVariantMap& policy)
{
    // Preserve the old catalog/page value contract exactly. Only the new
    // search branch accepts and independently owns a nested manifest.
    if (request.value(QStringLiteral("kind")).type() != QVariant::String
        || request.value(QStringLiteral("kind")).toString() != QStringLiteral("search")) return request;
    if (!FleetTelemetryArchiveHistoryReader::searchRequestValid(request, policy)) return {};
    QVariantList manifest;
    QString digest;
    if (!FleetTelemetryArchiveHistoryReader::canonicalSearchManifest(
            request.value(QStringLiteral("manifest")).toList(), &manifest, &digest)) return {};
    QVariantMap owned;
    for (auto it = request.constBegin(); it != request.constEnd(); ++it) {
        if (it.key() == QStringLiteral("manifest")) owned.insert(it.key(), manifest);
        else owned.insert(ownString(it.key()), it.value().type() == QVariant::String
            ? QVariant(ownString(it.value().toString())) : it.value());
    }
    return owned;
}

QVariantMap ownedPolicy(const QVariantMap& request, const QVariantMap& policy)
{
    if (request.value(QStringLiteral("kind")).type() != QVariant::String
        || request.value(QStringLiteral("kind")).toString() != QStringLiteral("search")) return policy;
    if (!FleetTelemetryArchiveCodec::readPolicyValid(policy)) return {};
    return {{QStringLiteral("maximumFileMiB"), policy.value(QStringLiteral("maximumFileMiB"))},
        {QStringLiteral("maximumCatalogFiles"), policy.value(QStringLiteral("maximumCatalogFiles"))},
        {QStringLiteral("maximumPageMiB"), policy.value(QStringLiteral("maximumPageMiB"))},
        {QStringLiteral("maximumPageFrames"), policy.value(QStringLiteral("maximumPageFrames"))}};
}

bool requestValid(const QVariantMap& request, const QVariantMap& policy)
{
    const QVariant kind = request.value(QStringLiteral("kind"));
    if (kind.type() != QVariant::String || kind.isNull()) return false;
    if (kind.toString() == QStringLiteral("search"))
        return FleetTelemetryArchiveHistoryReader::searchRequestValid(request, policy);
    if (kind.toString() == QStringLiteral("catalog")) return request.size() == 1;
    if (kind.toString() != QStringLiteral("page") || request.size() != 11) return false;
    const QStringList strings{QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("fileBytes"),
        QStringLiteral("modifiedUtcMs"), QStringLiteral("headerHash"), QStringLiteral("expectedFileHash"), QStringLiteral("afterSequence")};
    for (const QString& key : strings) if (request.value(key).type() != QVariant::String || request.value(key).isNull()) return false;
    QString runId;
    qint64 bytes = 0, modified = 0, after = 0;
    if (!basename(request.value(QStringLiteral("basename")).toString(), &runId)
        || runId != request.value(QStringLiteral("runId")).toString()
        || !decimal(request.value(QStringLiteral("fileBytes")).toString(), &bytes) || bytes < 1
        || bytes > policy.value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB
        || !decimal(request.value(QStringLiteral("modifiedUtcMs")).toString(), &modified) || modified < 1
        || !sha256(request.value(QStringLiteral("headerHash")).toString())
        || !sha256(request.value(QStringLiteral("expectedFileHash")).toString(), true)
        || !decimal(request.value(QStringLiteral("afterSequence")).toString(), &after)
        || (after > 0 && request.value(QStringLiteral("expectedFileHash")).toString().isEmpty())
        || !integerIn(request.value(QStringLiteral("vehicleId")), 0, 255)
        || request.value(QStringLiteral("includePositionPayload")).type() != QVariant::Bool
        || request.value(QStringLiteral("includePositionPayload")).isNull()
        || !integerIn(request.value(QStringLiteral("limit")), 1, policy.value(QStringLiteral("maximumPageFrames")).toInt())) return false;
    return true;
}

bool directoryValid(const QString& directory)
{
    if (directory.isEmpty() || directory.size() > 4096 || directory.contains(QChar::Null)) return false;
    const QString normalized = QDir::fromNativeSeparators(directory);
    const QFileInfo info(normalized);
    return QDir::isAbsolutePath(normalized) && !normalized.startsWith(QStringLiteral("//"))
        && QDir::cleanPath(normalized) == normalized && info.exists() && info.isDir() && info.isReadable()
        && !info.isSymLink() && QDir::fromNativeSeparators(info.canonicalFilePath()) == normalized;
}

struct FileView {
    QString name;
    QString runId;
    qint64 bytes = 0;
    qint64 modifiedUtcMs = 0;
};

bool fileView(const QString& directory, const QString& name, FileView* view)
{
    QString runId;
    if (!view || !basename(name, &runId) || !directoryValid(directory)) return false;
    const QString path = QDir(directory).filePath(name);
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable() || info.isSymLink() || info.size() < 0
        || QDir::fromNativeSeparators(info.canonicalFilePath()) != path) return false;
    view->name = name; view->runId = runId; view->bytes = info.size();
    const qint64 modified = info.lastModified().toUTC().toMSecsSinceEpoch();
    view->modifiedUtcMs = modified > 0 ? modified : 0;
    return true;
}

bool sameView(const FileView& a, const FileView& b)
{
    return a.name == b.name && a.runId == b.runId && a.bytes == b.bytes && a.modifiedUtcMs == b.modifiedUtcMs;
}

bool scanDirectory(const QString& directory, int maximumFiles, const std::atomic_bool& cancelled,
                   QList<FileView>* views, QString* directoryIdentity, QString* error)
{
    if (!directoryValid(directory)) { *error = QStringLiteral("telemetry-history-directory-rejected"); return false; }
    const auto inventory = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::TelemetryJournals, maximumFiles, &cancelled);
    if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
    if (!inventory.ok || !inventory.enumerationCompleted || inventory.directoryIdentity.isEmpty()) {
        *error = QStringLiteral("telemetry-history-directory-inventory-failed"); return false;
    }
    QList<FileView> result;
    QSet<QString> names;
    for (const auto& entry : inventory.entries) {
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        const QString name = entry.basename;
        FileView view;
        if (!entry.regularFile || entry.symlink || !fileView(directory, name, &view) || names.contains(name)) {
            *error = QStringLiteral("telemetry-history-namespace-rejected"); return false;
        }
        if (result.size() >= maximumFiles) { *error = QStringLiteral("telemetry-history-catalog-cap-reached"); return false; }
        names.insert(name); result.append(view);
    }
    if (!directoryValid(directory) || cancelled.load(std::memory_order_acquire)) {
        *error = cancelled.load() ? QStringLiteral("telemetry-history-cancelled") : QStringLiteral("telemetry-history-directory-changed"); return false;
    }
    std::sort(result.begin(), result.end(), [](const FileView& a, const FileView& b) { return a.name < b.name; });
    *views = result; *directoryIdentity = inventory.directoryIdentity; return true;
}

bool sameViews(const QList<FileView>& before, const QList<FileView>& after)
{
    if (before.size() != after.size()) return false;
    for (int index = 0; index < before.size(); ++index) if (!sameView(before.at(index), after.at(index))) return false;
    return true;
}

QVariantMap claims()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("crcVerified"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("fullTelemetryCaptured"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("closureVerified"), false}, {QStringLiteral("crossFileAtomicSnapshot"), false},
        {QStringLiteral("directoryEnumerationCompletenessVerified"), false},
        {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durableVerified"), false}};
}

qint64 jsonBytes(const QVariantMap& value)
{
    return QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size();
}

QVariantMap catalogEntry(const QString& directory, const FileView& view, qint64 maximumBytes,
                         const std::atomic_bool& cancelled, bool* observedSourceChange = nullptr)
{
    if (observedSourceChange) *observedSourceChange = false;
    QVariantMap entry{{QStringLiteral("basename"), view.name}, {QStringLiteral("runId"), view.runId},
        {QStringLiteral("fileBytes"), QString::number(view.bytes)}, {QStringLiteral("modifiedUtcMs"), QString::number(view.modifiedUtcMs)},
        {QStringLiteral("headerHash"), QStringLiteral("")}, {QStringLiteral("headerValid"), false},
        {QStringLiteral("positionTelemetryConsent"), false}, {QStringLiteral("errorCode"), QStringLiteral("")}};
    QString error = QStringLiteral("");
    if (view.bytes == 0) error = QStringLiteral("telemetry-history-header-missing");
    else if (view.bytes > maximumBytes) error = QStringLiteral("telemetry-history-file-cap-rejected");
    else if (view.modifiedUtcMs < 1) error = QStringLiteral("telemetry-history-file-metadata-invalid");
    else if (cancelled.load()) error = QStringLiteral("telemetry-history-cancelled");
    else {
        QFile file(QDir(directory).filePath(view.name));
        if (!file.open(QIODevice::ReadOnly)) error = QStringLiteral("telemetry-history-open-failed");
        else {
            const bool sameSize = file.size() == view.bytes;
            const QByteArray line = sameSize ? file.readLine(kMaximumStoredLineBytes + 1) : QByteArray();
            if (observedSourceChange && line.size() > view.bytes) *observedSourceChange = true;
            if (!sameSize || file.size() != view.bytes) {
                if (observedSourceChange) *observedSourceChange = true;
                error = QStringLiteral("telemetry-history-file-changed");
            }
            else if (file.error() != QFileDevice::NoError) error = QStringLiteral("telemetry-history-read-failed");
            else if (line.isEmpty() || !line.endsWith('\n') || line.size() > kMaximumStoredLineBytes || line.size() > view.bytes) error = QStringLiteral("telemetry-history-header-incomplete");
            else {
                const auto header = FleetTelemetryArchiveCodec::inspectHeader(line.left(line.size() - 1), view.runId);
                if (!header.ok) error = header.errorCode;
                else if (view.bytes > header.value.value(QStringLiteral("policy")).toMap().value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB) error = QStringLiteral("telemetry-history-stored-file-cap-rejected");
                else {
                    entry.insert(QStringLiteral("headerHash"), QString::fromLatin1(QCryptographicHash::hash(header.canonicalJson, QCryptographicHash::Sha256).toHex()));
                    entry.insert(QStringLiteral("headerValid"), true);
                    entry.insert(QStringLiteral("positionTelemetryConsent"), header.includePositionTelemetry);
                }
            }
            file.close();
            if (file.error() != QFileDevice::NoError) error = QStringLiteral("telemetry-history-close-failed");
        }
    }
    FileView after;
    if (!fileView(directory, view.name, &after) || !sameView(view, after)) {
        if (observedSourceChange) *observedSourceChange = true;
        error = QStringLiteral("telemetry-history-file-changed");
    }
    if (!error.isEmpty()) {
        entry.insert(QStringLiteral("headerValid"), false); entry.insert(QStringLiteral("errorCode"), error);
        entry.insert(QStringLiteral("headerHash"), QStringLiteral("")); entry.insert(QStringLiteral("positionTelemetryConsent"), false);
    }
    return entry;
}

bool catalog(const QString& directory, const QVariantMap& policy, const std::atomic_bool& cancelled,
             QVariantMap* output, QString* error)
{
    QList<FileView> before, after;
    QString beforeIdentity, afterIdentity;
    if (!scanDirectory(directory, policy.value(QStringLiteral("maximumCatalogFiles")).toInt(), cancelled, &before, &beforeIdentity, error)) return false;
    QVariantList entries;
    for (const FileView& view : before) {
        if (cancelled.load()) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        entries.append(catalogEntry(directory, view, policy.value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB, cancelled));
    }
    if (!scanDirectory(directory, policy.value(QStringLiteral("maximumCatalogFiles")).toInt(), cancelled, &after, &afterIdentity, error)) return false;
    if (beforeIdentity != afterIdentity || !sameViews(before, after)) { *error = QStringLiteral("telemetry-history-catalog-changed"); return false; }
    QVariantMap result = claims();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-catalog"));
    result.insert(QStringLiteral("version"), 1); result.insert(QStringLiteral("kind"), QStringLiteral("catalog"));
    result.insert(QStringLiteral("entries"), entries); result.insert(QStringLiteral("fullRecordVerification"), false);
    result.insert(QStringLiteral("directoryInventoryEnumerationCompleted"), true);
    if (jsonBytes(result) > policy.value(QStringLiteral("maximumPageMiB")).toLongLong() * kMiB - kFacadeReserve) {
        *error = QStringLiteral("telemetry-history-result-cap-rejected"); return false;
    }
    *output = result; return true;
}

bool secondHash(const QString& directory, const FileView& before, const std::atomic_bool& cancelled,
                QString* digest, QString* error, bool* observedSourceChange = nullptr)
{
    if (observedSourceChange) *observedSourceChange = false;
    FileView opening;
    if (!fileView(directory, before.name, &opening) || !sameView(before, opening)) { *error = QStringLiteral("telemetry-history-file-changed"); return false; }
    QFile file(QDir(directory).filePath(before.name));
    if (!file.open(QIODevice::ReadOnly)) { *error = QStringLiteral("telemetry-history-reopen-failed"); return false; }
    if (file.size() != before.bytes) {
        if (observedSourceChange) *observedSourceChange = true;
        *error = QStringLiteral("telemetry-history-reopen-failed"); return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 remaining = before.bytes;
    while (remaining > 0) {
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        const QByteArray chunk = file.read(qMin<qint64>(remaining, 64LL * 1024LL));
        if (chunk.isEmpty() || chunk.size() > remaining || file.error() != QFileDevice::NoError) { *error = QStringLiteral("telemetry-history-read-failed"); return false; }
        hash.addData(chunk); remaining -= chunk.size();
    }
    if (!file.read(1).isEmpty() || file.error() != QFileDevice::NoError || file.size() != before.bytes) { *error = QStringLiteral("telemetry-history-file-changed"); return false; }
    file.close();
    FileView after;
    if (file.error() != QFileDevice::NoError || !fileView(directory, before.name, &after) || !sameView(before, after)) {
        *error = QStringLiteral("telemetry-history-file-changed"); return false;
    }
    *digest = QString::fromLatin1(hash.result().toHex()); return true;
}

bool page(const QString& directory, const QVariantMap& policy, const QVariantMap& request,
          const std::atomic_bool& cancelled, QVariantMap* output, QString* error)
{
    FileView before;
    qint64 requestedBytes = 0, requestedModified = 0, cursor = 0;
    decimal(request.value(QStringLiteral("fileBytes")).toString(), &requestedBytes);
    decimal(request.value(QStringLiteral("modifiedUtcMs")).toString(), &requestedModified);
    decimal(request.value(QStringLiteral("afterSequence")).toString(), &cursor);
    if (!fileView(directory, request.value(QStringLiteral("basename")).toString(), &before)
        || before.runId != request.value(QStringLiteral("runId")).toString() || before.bytes != requestedBytes || before.modifiedUtcMs != requestedModified) {
        *error = QStringLiteral("telemetry-history-descriptor-stale"); return false;
    }
    const qint64 budget = policy.value(QStringLiteral("maximumPageMiB")).toLongLong() * kMiB - kFacadeReserve;
    const int limit = request.value(QStringLiteral("limit")).toInt();
    const int vehicle = request.value(QStringLiteral("vehicleId")).toInt();
    const bool includePosition = request.value(QStringLiteral("includePositionPayload")).toBool();
    QFile file(QDir(directory).filePath(before.name));
    if (!file.open(QIODevice::ReadOnly) || file.size() != before.bytes) { *error = QStringLiteral("telemetry-history-open-failed"); return false; }
    const QByteArray firstLine = file.readLine(kMaximumStoredLineBytes + 1);
    if (firstLine.isEmpty() || !firstLine.endsWith('\n') || firstLine.size() > kMaximumStoredLineBytes
        || firstLine.size() > before.bytes || file.error() != QFileDevice::NoError) {
        *error = QStringLiteral("telemetry-history-header-incomplete"); return false;
    }
    const auto header = FleetTelemetryArchiveCodec::inspectHeader(firstLine.left(firstLine.size() - 1), before.runId);
    if (!header.ok) { *error = header.errorCode; return false; }
    const QString headerHash = QString::fromLatin1(QCryptographicHash::hash(header.canonicalJson, QCryptographicHash::Sha256).toHex());
    if (headerHash != request.value(QStringLiteral("headerHash")).toString()
        || before.bytes > header.value.value(QStringLiteral("policy")).toMap().value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB
        || (vehicle != 0 && !header.sourceVehicleIds.values().contains(vehicle))) {
        *error = QStringLiteral("telemetry-history-header-scope-mismatch"); return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(firstLine);
    qint64 scannedBytes = firstLine.size(), frames = 0, lastSequence = 0, nextSequence = cursor, withheld = 0;
    QMap<QString, quint64> sourceSequences;
    bool cursorSeen = cursor == 0, hasMore = false, partialTail = false, footerPresent = false, budgetStopped = false;
    QVariantMap footer;
    QVariantList rows;
    QVariantMap result = claims();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-history-page"));
    result.insert(QStringLiteral("version"), 1); result.insert(QStringLiteral("kind"), QStringLiteral("page"));
    result.insert(QStringLiteral("basename"), before.name); result.insert(QStringLiteral("runId"), before.runId);
    result.insert(QStringLiteral("headerHash"), headerHash); result.insert(QStringLiteral("fileHash"), QString(64, QLatin1Char('0')));
    result.insert(QStringLiteral("fileBytes"), QString::number(before.bytes)); result.insert(QStringLiteral("modifiedUtcMs"), QString::number(before.modifiedUtcMs));
    result.insert(QStringLiteral("sources"), header.value.value(QStringLiteral("sources")));
    result.insert(QStringLiteral("afterSequence"), QString::number(cursor));
    result.insert(QStringLiteral("completeRecordPrefixOnly"), true);
    const qint64 nonRowReserve = jsonBytes(result) + kMaximumJsonBytes + 2048;
    qint64 rowBytes = 2;
    while (scannedBytes < before.bytes) {
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        const qint64 lineOffset = scannedBytes;
        const QByteArray line = file.readLine(kMaximumStoredLineBytes + 1);
        if (line.isEmpty() || file.error() != QFileDevice::NoError || line.size() > before.bytes - scannedBytes) {
            *error = QStringLiteral("telemetry-history-read-failed"); return false;
        }
        if (line.size() > kMaximumStoredLineBytes) { *error = QStringLiteral("telemetry-history-line-cap-rejected"); return false; }
        hash.addData(line); scannedBytes += line.size();
        if (footerPresent) { *error = QStringLiteral("telemetry-history-after-footer"); return false; }
        if (!line.endsWith('\n')) {
            if (scannedBytes != before.bytes) { *error = QStringLiteral("telemetry-history-interior-line-incomplete"); return false; }
            partialTail = true; break; // Never parse or expose an unterminated tail.
        }
        const QByteArray json = line.left(line.size() - 1);
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson(json, &parse);
        if (parse.error != QJsonParseError::NoError || !document.isObject()) { *error = QStringLiteral("telemetry-history-record-invalid"); return false; }
        const QJsonObject frame = document.object();
        if (frame.value(QStringLiteral("kind")).toString() == QStringLiteral("footer")) {
            bool ok = false;
            footer = FleetTelemetryArchiveCodec::inspectFooter(json, header, frames, lineOffset, lastSequence, &ok, error);
            if (!ok) return false;
            footerPresent = true;
            if (scannedBytes != before.bytes) { *error = QStringLiteral("telemetry-history-after-footer"); return false; }
            continue;
        }
        qint64 sequence = 0;
        if (!frame.value(QStringLiteral("sequence")).isString() || !decimal(frame.value(QStringLiteral("sequence")).toString(), &sequence) || sequence <= lastSequence) {
            *error = QStringLiteral("telemetry-history-sequence-invalid"); return false;
        }
        FleetTelemetryArchiveCodec::Record record;
        record.ok = true; record.runId = before.runId; record.sequence = sequence; record.lineJson = json;
        if (!FleetTelemetryArchiveCodec::recordConsistent(record)) { *error = QStringLiteral("telemetry-history-record-invalid"); return false; }
        const QString epoch = frame.value(QStringLiteral("sourceEpoch")).toString();
        quint64 sourceSequence = 0;
        if (!header.sourceVehicleIds.contains(epoch) || frame.value(QStringLiteral("sysid")).toInt() != header.sourceVehicleIds.value(epoch)
            || frame.value(QStringLiteral("positionTelemetryConsent")).toBool() != header.includePositionTelemetry
            || !unsignedSequence(frame.value(QStringLiteral("sourceSequence")).toString(), &sourceSequence)
            || sourceSequence <= sourceSequences.value(epoch, 0)) { *error = QStringLiteral("telemetry-history-source-scope-invalid"); return false; }
        sourceSequences.insert(epoch, sourceSequence); lastSequence = sequence;
        if (frames == std::numeric_limits<qint64>::max()) { *error = QStringLiteral("telemetry-history-counter-exhausted"); return false; }
        ++frames;
        if (sequence == cursor) cursorSeen = true;
        if (sequence <= cursor || (vehicle != 0 && frame.value(QStringLiteral("sysid")).toInt() != vehicle)) continue;
        if (!includePosition && frame.value(QStringLiteral("positionTelemetry")).toBool()) { ++withheld; continue; }
        if (rows.size() >= limit || budgetStopped) { hasMore = true; continue; }
        const QVariantMap value = frame.toVariantMap();
        const qint64 charge = jsonBytes(value) + 1;
        if (rowBytes + charge > budget - nonRowReserve) {
            hasMore = true; budgetStopped = true;
            if (rows.isEmpty()) { *error = QStringLiteral("telemetry-history-result-cap-rejected"); return false; }
            continue;
        }
        rows.append(value); rowBytes += charge; nextSequence = sequence;
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
    }
    if (!file.read(1).isEmpty() || file.error() != QFileDevice::NoError || file.size() != before.bytes || !cursorSeen) {
        *error = cursorSeen ? QStringLiteral("telemetry-history-file-changed") : QStringLiteral("telemetry-history-cursor-not-observed"); return false;
    }
    file.close();
    if (file.error() != QFileDevice::NoError) { *error = QStringLiteral("telemetry-history-close-failed"); return false; }
    FileView after;
    if (!fileView(directory, before.name, &after) || !sameView(before, after)) { *error = QStringLiteral("telemetry-history-file-changed"); return false; }
    const QString digest = QString::fromLatin1(hash.result().toHex());
    QString rereadHash;
    if (!secondHash(directory, before, cancelled, &rereadHash, error)) return false;
    if (rereadHash != digest || (!request.value(QStringLiteral("expectedFileHash")).toString().isEmpty()
        && request.value(QStringLiteral("expectedFileHash")).toString() != digest)) {
        *error = QStringLiteral("telemetry-history-file-hash-changed"); return false;
    }
    result.insert(QStringLiteral("fileHash"), digest); result.insert(QStringLiteral("rows"), rows);
    result.insert(QStringLiteral("nextSequence"), QString::number(nextSequence)); result.insert(QStringLiteral("hasMore"), hasMore);
    result.insert(QStringLiteral("totalValidatedFrames"), QString::number(frames));
    result.insert(QStringLiteral("lastValidatedSequence"), QString::number(lastSequence));
    result.insert(QStringLiteral("positionWithheldFrames"), QString::number(withheld));
    result.insert(QStringLiteral("footerPresent"), footerPresent); result.insert(QStringLiteral("partialTail"), partialTail);
    result.insert(QStringLiteral("footerDiagnostic"), footer);
    if (jsonBytes(result) > budget) { *error = QStringLiteral("telemetry-history-result-cap-rejected"); return false; }
    *output = result; return true;
}

bool searchManifestSnapshot(const QString& directory, const QVariantMap& policy,
    const std::atomic_bool& cancelled, const QVariantList& expected, QString* identity, QString* error)
{
    QList<FileView> before, after;
    QString beforeIdentity, afterIdentity;
    if (!scanDirectory(directory, policy.value(QStringLiteral("maximumCatalogFiles")).toInt(),
            cancelled, &before, &beforeIdentity, error)) return false;
    QVariantList observed;
    for (const FileView& view : before) {
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        bool observedSourceChange = false;
        const QVariantMap entry = catalogEntry(directory, view,
            policy.value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB, cancelled, &observedSourceChange);
        if (observedSourceChange || entry.value(QStringLiteral("errorCode")).toString() == QStringLiteral("telemetry-history-file-changed")) {
            *error = QStringLiteral("telemetry-history-search-source-stale"); return false;
        }
        observed.append(entry);
    }
    if (!scanDirectory(directory, policy.value(QStringLiteral("maximumCatalogFiles")).toInt(),
            cancelled, &after, &afterIdentity, error)) return false;
    if (beforeIdentity != afterIdentity || !sameViews(before, after) || observed != expected) {
        *error = QStringLiteral("telemetry-history-search-source-stale"); return false;
    }
    *identity = beforeIdentity;
    return true;
}

QVariantMap unknownSearchRow(int index, const QVariantMap& descriptor)
{
    return {{QStringLiteral("fileIndex"), index}, {QStringLiteral("descriptor"), descriptor},
        {QStringLiteral("fileHash"), QStringLiteral("")}, {QStringLiteral("status"), QStringLiteral("unavailable")},
        {QStringLiteral("errorCode"), descriptor.value(QStringLiteral("errorCode"))},
        {QStringLiteral("matchingFrameCount"), QStringLiteral("")},
        {QStringLiteral("firstMatchingSequence"), QStringLiteral("")},
        {QStringLiteral("lastMatchingSequence"), QStringLiteral("")},
        {QStringLiteral("footerState"), QStringLiteral("unknown")}, {QStringLiteral("tailState"), QStringLiteral("unknown")}};
}

bool searchFile(const QString& directory, const QVariantMap& descriptor, int vehicle, int message,
    const std::atomic_bool& cancelled, QVariantMap* row, QString* error)
{
    const auto unavailable = [&](const QString& code) {
        row->insert(QStringLiteral("errorCode"), ownString(code));
        return true; // Unknown, never a successful zero-match prefix.
    };
    const auto stale = [&]() { *error = QStringLiteral("telemetry-history-search-source-stale"); return false; };
    const auto cancelledNow = [&]() {
        if (!cancelled.load(std::memory_order_acquire)) return false;
        *error = QStringLiteral("telemetry-history-cancelled"); return true;
    };
    if (cancelledNow()) return false;
    FileView before;
    qint64 bytes = 0, modified = 0;
    decimal(descriptor.value(QStringLiteral("fileBytes")).toString(), &bytes);
    decimal(descriptor.value(QStringLiteral("modifiedUtcMs")).toString(), &modified);
    if (!fileView(directory, descriptor.value(QStringLiteral("basename")).toString(), &before)
        || before.runId != descriptor.value(QStringLiteral("runId")).toString()
        || before.bytes != bytes || before.modifiedUtcMs != modified) return stale();
    if (!descriptor.value(QStringLiteral("headerValid")).toBool()) return true;
    if (before.bytes < 1) return stale();
    if (before.bytes > kSearchMaximumFileBytes)
        return unavailable(QStringLiteral("telemetry-history-search-file-cap-rejected"));
    QFile file(QDir(directory).filePath(before.name));
    if (!file.open(QIODevice::ReadOnly)) return unavailable(QStringLiteral("telemetry-history-open-failed"));
    if (file.size() != before.bytes) return stale();
    const QByteArray firstLine = file.readLine(kMaximumStoredLineBytes + 1);
    if (file.error() != QFileDevice::NoError) return unavailable(QStringLiteral("telemetry-history-read-failed"));
    if (firstLine.isEmpty() || !firstLine.endsWith('\n') || firstLine.size() > kMaximumStoredLineBytes
        || firstLine.size() > before.bytes) return stale();
    const auto header = FleetTelemetryArchiveCodec::inspectHeader(firstLine.left(firstLine.size() - 1), before.runId);
    if (!header.ok || QString::fromLatin1(QCryptographicHash::hash(header.canonicalJson, QCryptographicHash::Sha256).toHex())
            != descriptor.value(QStringLiteral("headerHash")).toString()
        || header.includePositionTelemetry != descriptor.value(QStringLiteral("positionTelemetryConsent")).toBool()
        || before.bytes > header.value.value(QStringLiteral("policy")).toMap().value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB)
        return stale();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(firstLine);
    qint64 scanned = firstLine.size(), frames = 0, lastSequence = 0, matching = 0, firstMatching = 0, lastMatching = 0;
    QMap<QString, quint64> sourceSequences;
    bool footerPresent = false, partialTail = false, readFailed = false;
    QString recordError = QStringLiteral("");
    while (scanned < before.bytes) {
        if (cancelledNow()) return false;
        const qint64 offset = scanned;
        const QByteArray line = file.readLine(kMaximumStoredLineBytes + 1);
        if (line.size() > before.bytes - scanned) return stale();
        if (line.isEmpty() || file.error() != QFileDevice::NoError) {
            readFailed = true; break;
        }
        hash.addData(line); scanned += line.size();
        // After a malformed complete record, finish the bounded byte/hash
        // pass without parsing further records or retaining a success prefix.
        if (!recordError.isEmpty()) continue;
        if (line.size() > kMaximumStoredLineBytes) { recordError = QStringLiteral("telemetry-history-line-cap-rejected"); continue; }
        if (footerPresent) { recordError = QStringLiteral("telemetry-history-after-footer"); continue; }
        if (!line.endsWith('\n')) {
            if (scanned == before.bytes) partialTail = true;
            else recordError = QStringLiteral("telemetry-history-interior-line-incomplete");
            continue;
        }
        const QByteArray json = line.left(line.size() - 1);
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson(json, &parse);
        if (parse.error != QJsonParseError::NoError || !document.isObject()) {
            recordError = QStringLiteral("telemetry-history-record-invalid"); continue;
        }
        const QJsonObject frame = document.object();
        if (frame.value(QStringLiteral("kind")).toString() == QStringLiteral("footer")) {
            bool valid = false;
            QString footerError;
            FleetTelemetryArchiveCodec::inspectFooter(json, header, frames, offset, lastSequence, &valid, &footerError);
            if (!valid) recordError = footerError.isEmpty() ? QStringLiteral("telemetry-history-footer-invalid") : footerError;
            else { footerPresent = true; if (scanned != before.bytes) recordError = QStringLiteral("telemetry-history-after-footer"); }
            continue;
        }
        qint64 sequence = 0;
        if (!frame.value(QStringLiteral("sequence")).isString()
            || !decimal(frame.value(QStringLiteral("sequence")).toString(), &sequence) || sequence <= lastSequence) {
            recordError = QStringLiteral("telemetry-history-sequence-invalid"); continue;
        }
        FleetTelemetryArchiveCodec::Record record;
        record.ok = true; record.runId = before.runId; record.sequence = sequence; record.lineJson = json;
        if (!FleetTelemetryArchiveCodec::recordConsistent(record)) { recordError = QStringLiteral("telemetry-history-record-invalid"); continue; }
        const QString epoch = frame.value(QStringLiteral("sourceEpoch")).toString();
        quint64 sourceSequence = 0;
        if (!header.sourceVehicleIds.contains(epoch) || frame.value(QStringLiteral("sysid")).toInt() != header.sourceVehicleIds.value(epoch)
            || frame.value(QStringLiteral("positionTelemetryConsent")).toBool() != header.includePositionTelemetry
            || !unsignedSequence(frame.value(QStringLiteral("sourceSequence")).toString(), &sourceSequence)
            || sourceSequence <= sourceSequences.value(epoch, 0)) {
            recordError = QStringLiteral("telemetry-history-source-scope-invalid"); continue;
        }
        sourceSequences.insert(epoch, sourceSequence); lastSequence = sequence;
        if (frames == std::numeric_limits<qint64>::max()) { recordError = QStringLiteral("telemetry-history-counter-exhausted"); continue; }
        ++frames;
        if ((vehicle == 0 || frame.value(QStringLiteral("sysid")).toInt() == vehicle)
            && (message == -1 || frame.value(QStringLiteral("msgid")).toInt() == message)) {
            if (matching == std::numeric_limits<qint64>::max()) { recordError = QStringLiteral("telemetry-history-counter-exhausted"); continue; }
            if (matching == 0) firstMatching = sequence;
            ++matching; lastMatching = sequence;
        }
    }
    if (file.size() != before.bytes) return stale();
    if (!readFailed && !file.read(1).isEmpty()) return stale();
    if (file.error() != QFileDevice::NoError || scanned != before.bytes) readFailed = true;
    file.close();
    if (file.error() != QFileDevice::NoError) readFailed = true;
    FileView after;
    if (!fileView(directory, before.name, &after) || !sameView(before, after)) return stale();
    if (cancelledNow()) return false;
    if (readFailed) return unavailable(QStringLiteral("telemetry-history-read-failed"));
    const QString digest = QString::fromLatin1(hash.result().toHex());
    QString rereadHash, hashError;
    bool observedSourceChange = false;
    if (!secondHash(directory, before, cancelled, &rereadHash, &hashError, &observedSourceChange)) {
        if (cancelledNow()) return false;
        if (observedSourceChange || hashError == QStringLiteral("telemetry-history-file-changed")) return stale();
        return unavailable(hashError.isEmpty() ? QStringLiteral("telemetry-history-read-failed") : hashError);
    }
    if (rereadHash != digest) return stale();
    if (!recordError.isEmpty()) return unavailable(recordError);
    row->insert(QStringLiteral("fileHash"), digest);
    row->insert(QStringLiteral("status"), matching > 0 ? QStringLiteral("matched") : QStringLiteral("no-match"));
    row->insert(QStringLiteral("errorCode"), QStringLiteral(""));
    row->insert(QStringLiteral("matchingFrameCount"), QString::number(matching));
    row->insert(QStringLiteral("firstMatchingSequence"), QString::number(firstMatching));
    row->insert(QStringLiteral("lastMatchingSequence"), QString::number(lastMatching));
    row->insert(QStringLiteral("footerState"), footerPresent ? QStringLiteral("present") : QStringLiteral("missing"));
    row->insert(QStringLiteral("tailState"), partialTail ? QStringLiteral("partial") : QStringLiteral("none"));
    return true;
}

bool search(const QString& directory, const QVariantMap& policy, const QVariantMap& request,
    const std::atomic_bool& cancelled, QVariantMap* output, QString* error)
{
    const QVariantList manifest = request.value(QStringLiteral("manifest")).toList();
    QString identity;
    if (!searchManifestSnapshot(directory, policy, cancelled, manifest, &identity, error)) return false;
    if (!request.value(QStringLiteral("expectedDirectoryIdentity")).toString().isEmpty()
        && identity != request.value(QStringLiteral("expectedDirectoryIdentity")).toString()) {
        *error = QStringLiteral("telemetry-history-search-source-stale"); return false;
    }
    const int after = request.value(QStringLiteral("afterFileIndex")).toInt();
    const int limit = request.value(QStringLiteral("limit")).toInt();
    int next = after, verified = 0, matched = 0, unavailable = 0;
    qint64 reserved = 2LL * manifest.size() * (kMaximumStoredLineBytes + 1);
    QVariantList rows;
    bool budgetLimited = false;
    for (; next < manifest.size(); ++next) {
        if (rows.size() >= limit) break;
        if (cancelled.load(std::memory_order_acquire)) { *error = QStringLiteral("telemetry-history-cancelled"); return false; }
        const QVariantMap descriptor = manifest.at(next).toMap();
        qint64 bytes = 0;
        decimal(descriptor.value(QStringLiteral("fileBytes")).toString(), &bytes);
        const qint64 charge = descriptor.value(QStringLiteral("headerValid")).toBool() && bytes <= kSearchMaximumFileBytes ? 2 * bytes : 0;
        if (charge > kSearchReadByteBudget - reserved) { budgetLimited = true; break; }
        reserved += charge;
        QVariantMap row = unknownSearchRow(next, descriptor);
        if (!searchFile(directory, descriptor, request.value(QStringLiteral("vehicleId")).toInt(),
                request.value(QStringLiteral("messageId")).toInt(), cancelled, &row, error)) return false;
        if (row.value(QStringLiteral("status")).toString() == QStringLiteral("unavailable")) ++unavailable;
        else { ++verified; if (row.value(QStringLiteral("status")).toString() == QStringLiteral("matched")) ++matched; }
        rows.append(row);
    }
    QString finalIdentity;
    if (!searchManifestSnapshot(directory, policy, cancelled, manifest, &finalIdentity, error)) return false;
    if (identity != finalIdentity) { *error = QStringLiteral("telemetry-history-search-source-stale"); return false; }
    QVariantMap result = claims();
    result.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-history-search"));
    result.insert(QStringLiteral("version"), 1); result.insert(QStringLiteral("kind"), QStringLiteral("search"));
    result.insert(QStringLiteral("manifestHash"), request.value(QStringLiteral("manifestHash")));
    result.insert(QStringLiteral("directoryIdentity"), identity);
    result.insert(QStringLiteral("vehicleId"), request.value(QStringLiteral("vehicleId")));
    result.insert(QStringLiteral("messageId"), request.value(QStringLiteral("messageId")));
    result.insert(QStringLiteral("afterFileIndex"), after); result.insert(QStringLiteral("nextFileIndex"), next);
    result.insert(QStringLiteral("totalFileCount"), manifest.size()); result.insert(QStringLiteral("inspectedFileCount"), rows.size());
    result.insert(QStringLiteral("verifiedFileCount"), verified); result.insert(QStringLiteral("matchedFileCount"), matched);
    result.insert(QStringLiteral("diagnosticFileCount"), unavailable); result.insert(QStringLiteral("hasMore"), next < manifest.size());
    result.insert(QStringLiteral("budgetLimited"), budgetLimited);
    result.insert(QStringLiteral("readByteBudget"), QString::number(kSearchReadByteBudget));
    result.insert(QStringLiteral("reservedReadBytes"), QString::number(reserved)); result.insert(QStringLiteral("rows"), rows);
    result.insert(QStringLiteral("readOnly"), true); result.insert(QStringLiteral("payloadPubliclyExposed"), false);
    if (!FleetTelemetryArchiveHistoryReader::searchResultMatches(result, request, policy)) {
        *error = QStringLiteral("telemetry-history-search-result-rejected"); return false;
    }
    *output = result;
    return true;
}
} // namespace

bool FleetTelemetryArchiveHistoryReader::canonicalSearchManifest(const QVariantList& descriptors,
    QVariantList* ownedDescriptors, QString* manifestHash)
{
    QVariantList staged;
    QString previous;
    bool valid = ownedDescriptors && manifestHash && descriptors.size() <= kSearchMaximumFiles;
    if (valid) for (const QVariant& descriptor : descriptors) {
        QVariantMap row;
        if (!ownedDescriptor(descriptor, &row)) { valid = false; break; }
        const QString name = row.value(QStringLiteral("basename")).toString();
        if (!previous.isEmpty() && name <= previous) { valid = false; break; }
        previous = name; staged.append(row);
    }
    QString digest;
    if (valid) {
        const QByteArray json = QJsonDocument::fromVariant(staged).toJson(QJsonDocument::Compact);
        valid = json.size() <= kSearchMaximumResultBytes - kFacadeReserve;
        if (valid) digest = QString::fromLatin1(QCryptographicHash::hash(json, QCryptographicHash::Sha256).toHex());
    }
    // Stage first so an output alias of the input cannot erase its source.
    if (!valid) {
        if (ownedDescriptors) ownedDescriptors->clear();
        if (manifestHash) manifestHash->clear();
        return false;
    }
    *ownedDescriptors = staged;
    *manifestHash = digest;
    return true;
}

bool FleetTelemetryArchiveHistoryReader::searchRequestValid(const QVariantMap& request, const QVariantMap& policy)
{
    if (!searchPolicyValid(policy)
        || !exactKeys(request, {QStringLiteral("kind"), QStringLiteral("manifest"), QStringLiteral("manifestHash"),
            QStringLiteral("expectedDirectoryIdentity"), QStringLiteral("afterFileIndex"), QStringLiteral("vehicleId"),
            QStringLiteral("messageId"), QStringLiteral("limit")})
        || !actualString(request.value(QStringLiteral("kind")), 16) || request.value(QStringLiteral("kind")).toString() != QStringLiteral("search")
        || request.value(QStringLiteral("manifest")).type() != QVariant::List || request.value(QStringLiteral("manifest")).isNull()
        || !actualString(request.value(QStringLiteral("manifestHash")), 64)
        || !sha256(request.value(QStringLiteral("manifestHash")).toString())) return false;
    QVariantList manifest;
    QString digest;
    if (!canonicalSearchManifest(request.value(QStringLiteral("manifest")).toList(), &manifest, &digest)
        || digest != request.value(QStringLiteral("manifestHash")).toString()
        || manifest.size() > policy.value(QStringLiteral("maximumCatalogFiles")).toInt()
        || !actualInt(request.value(QStringLiteral("afterFileIndex")), 0, manifest.size())
        || !actualInt(request.value(QStringLiteral("vehicleId")), 0, 255)
        || !actualInt(request.value(QStringLiteral("messageId")), -1, 16777215)
        || !actualInt(request.value(QStringLiteral("limit")), 1,
            qMin(kSearchMaximumRows, policy.value(QStringLiteral("maximumPageFrames")).toInt()))) return false;
    const bool first = request.value(QStringLiteral("afterFileIndex")).toInt() == 0;
    if (!directoryIdentityValid(request.value(QStringLiteral("expectedDirectoryIdentity")), first)
        || (first && !request.value(QStringLiteral("expectedDirectoryIdentity")).toString().isEmpty())) return false;
    const qint64 cap = policy.value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB;
    for (const QVariant& value : manifest) {
        const QVariantMap row = value.toMap();
        qint64 bytes = 0;
        if (!decimal(row.value(QStringLiteral("fileBytes")).toString(), &bytes)
            || (row.value(QStringLiteral("headerValid")).toBool() && bytes > cap)) return false;
    }
    return true;
}

bool FleetTelemetryArchiveHistoryReader::searchResultMatches(const QVariantMap& result,
    const QVariantMap& request, const QVariantMap& policy)
{
    if (!searchRequestValid(request, policy)) return false;
    QStringList keys = claims().keys();
    keys << QStringLiteral("format") << QStringLiteral("version") << QStringLiteral("kind")
        << QStringLiteral("manifestHash") << QStringLiteral("directoryIdentity") << QStringLiteral("vehicleId")
        << QStringLiteral("messageId") << QStringLiteral("afterFileIndex") << QStringLiteral("nextFileIndex")
        << QStringLiteral("totalFileCount") << QStringLiteral("inspectedFileCount") << QStringLiteral("verifiedFileCount")
        << QStringLiteral("matchedFileCount") << QStringLiteral("diagnosticFileCount") << QStringLiteral("hasMore")
        << QStringLiteral("budgetLimited") << QStringLiteral("readByteBudget") << QStringLiteral("reservedReadBytes")
        << QStringLiteral("rows") << QStringLiteral("readOnly") << QStringLiteral("payloadPubliclyExposed");
    if (!exactKeys(result, keys) || !actualString(result.value(QStringLiteral("format")), 128)
        || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-telemetry-history-search")
        || !actualInt(result.value(QStringLiteral("version")), 1, 1)
        || !actualString(result.value(QStringLiteral("kind")), 16) || result.value(QStringLiteral("kind")).toString() != QStringLiteral("search")
        || !actualString(result.value(QStringLiteral("manifestHash")), 64)
        || result.value(QStringLiteral("manifestHash")) != request.value(QStringLiteral("manifestHash"))
        || !directoryIdentityValid(result.value(QStringLiteral("directoryIdentity")), false)
        || !actualBool(result.value(QStringLiteral("readOnly")), true)
        || !actualBool(result.value(QStringLiteral("payloadPubliclyExposed")), false)) return false;
    const QVariantMap expectedClaims = claims();
    for (auto it = expectedClaims.constBegin(); it != expectedClaims.constEnd(); ++it)
        if (!actualBool(result.value(it.key()), it.value().toBool())) return false;
    const QString expectedIdentity = request.value(QStringLiteral("expectedDirectoryIdentity")).toString();
    if (!expectedIdentity.isEmpty() && result.value(QStringLiteral("directoryIdentity")).toString() != expectedIdentity) return false;
    const QVariantList manifest = request.value(QStringLiteral("manifest")).toList();
    const int after = request.value(QStringLiteral("afterFileIndex")).toInt();
    const int limit = request.value(QStringLiteral("limit")).toInt();
    if (!actualInt(result.value(QStringLiteral("vehicleId")), 0, 255)
        || result.value(QStringLiteral("vehicleId")) != request.value(QStringLiteral("vehicleId"))
        || !actualInt(result.value(QStringLiteral("messageId")), -1, 16777215)
        || result.value(QStringLiteral("messageId")) != request.value(QStringLiteral("messageId"))
        || !actualInt(result.value(QStringLiteral("afterFileIndex")), after, after)
        || !actualInt(result.value(QStringLiteral("nextFileIndex")), after, manifest.size())
        || !actualInt(result.value(QStringLiteral("totalFileCount")), manifest.size(), manifest.size())
        || result.value(QStringLiteral("rows")).type() != QVariant::List || result.value(QStringLiteral("rows")).isNull()) return false;
    const QVariantList rows = result.value(QStringLiteral("rows")).toList();
    const int next = result.value(QStringLiteral("nextFileIndex")).toInt();
    if (rows.size() > limit || next != after + rows.size() || (after < manifest.size() && rows.isEmpty())
        || !actualInt(result.value(QStringLiteral("inspectedFileCount")), rows.size(), rows.size())) return false;
    int verified = 0, matched = 0, unavailable = 0;
    qint64 reserved = 2LL * manifest.size() * (kMaximumStoredLineBytes + 1);
    for (int offset = 0; offset < rows.size(); ++offset) {
        if (rows.at(offset).type() != QVariant::Map || rows.at(offset).isNull()) return false;
        const QVariantMap row = rows.at(offset).toMap();
        const QVariantMap descriptor = manifest.at(after + offset).toMap();
        if (!exactKeys(row, {QStringLiteral("fileIndex"), QStringLiteral("descriptor"), QStringLiteral("fileHash"),
                QStringLiteral("status"), QStringLiteral("errorCode"), QStringLiteral("matchingFrameCount"),
                QStringLiteral("firstMatchingSequence"), QStringLiteral("lastMatchingSequence"),
                QStringLiteral("footerState"), QStringLiteral("tailState")})
            || !actualInt(row.value(QStringLiteral("fileIndex")), after + offset, after + offset)
            || row.value(QStringLiteral("descriptor")).type() != QVariant::Map
            || row.value(QStringLiteral("descriptor")).toMap() != descriptor) return false;
        for (const QString& key : {QStringLiteral("fileHash"), QStringLiteral("status"), QStringLiteral("errorCode"),
                QStringLiteral("matchingFrameCount"), QStringLiteral("firstMatchingSequence"), QStringLiteral("lastMatchingSequence"),
                QStringLiteral("footerState"), QStringLiteral("tailState")})
            if (!actualString(row.value(key), 128)) return false;
        qint64 fileBytes = 0;
        decimal(descriptor.value(QStringLiteral("fileBytes")).toString(), &fileBytes);
        if (descriptor.value(QStringLiteral("headerValid")).toBool() && fileBytes <= kSearchMaximumFileBytes) reserved += 2 * fileBytes;
        const QString status = row.value(QStringLiteral("status")).toString();
        if (status == QStringLiteral("unavailable")) {
            if (!identifier(row.value(QStringLiteral("errorCode")).toString())
                || !row.value(QStringLiteral("fileHash")).toString().isEmpty()
                || !row.value(QStringLiteral("matchingFrameCount")).toString().isEmpty()
                || !row.value(QStringLiteral("firstMatchingSequence")).toString().isEmpty()
                || !row.value(QStringLiteral("lastMatchingSequence")).toString().isEmpty()
                || row.value(QStringLiteral("footerState")).toString() != QStringLiteral("unknown")
                || row.value(QStringLiteral("tailState")).toString() != QStringLiteral("unknown")
                || (!descriptor.value(QStringLiteral("headerValid")).toBool()
                    && row.value(QStringLiteral("errorCode")) != descriptor.value(QStringLiteral("errorCode")))
                || (descriptor.value(QStringLiteral("headerValid")).toBool() && fileBytes > kSearchMaximumFileBytes
                    && row.value(QStringLiteral("errorCode")).toString() != QStringLiteral("telemetry-history-search-file-cap-rejected"))) return false;
            ++unavailable; continue;
        }
        if (!descriptor.value(QStringLiteral("headerValid")).toBool() || fileBytes > kSearchMaximumFileBytes
            || (status != QStringLiteral("matched") && status != QStringLiteral("no-match"))
            || !row.value(QStringLiteral("errorCode")).toString().isEmpty()
            || !sha256(row.value(QStringLiteral("fileHash")).toString())) return false;
        const QString footer = row.value(QStringLiteral("footerState")).toString(), tail = row.value(QStringLiteral("tailState")).toString();
        if ((footer != QStringLiteral("present") && footer != QStringLiteral("missing"))
            || (tail != QStringLiteral("partial") && tail != QStringLiteral("none"))
            || (footer == QStringLiteral("present") && tail == QStringLiteral("partial"))) return false;
        qint64 count = 0, first = 0, last = 0;
        if (!decimal(row.value(QStringLiteral("matchingFrameCount")).toString(), &count)
            || !decimal(row.value(QStringLiteral("firstMatchingSequence")).toString(), &first)
            || !decimal(row.value(QStringLiteral("lastMatchingSequence")).toString(), &last)) return false;
        if (status == QStringLiteral("no-match")) { if (count != 0 || first != 0 || last != 0) return false; }
        else {
            if (count < 1 || first < 1 || last < first || count > last - first + 1 || count > fileBytes) return false;
            ++matched;
        }
        ++verified;
    }
    if (reserved > kSearchReadByteBudget || !actualInt(result.value(QStringLiteral("verifiedFileCount")), verified, verified)
        || !actualInt(result.value(QStringLiteral("matchedFileCount")), matched, matched)
        || !actualInt(result.value(QStringLiteral("diagnosticFileCount")), unavailable, unavailable)
        || !actualBool(result.value(QStringLiteral("hasMore")), next < manifest.size())
        || !actualString(result.value(QStringLiteral("readByteBudget")), 19)
        || result.value(QStringLiteral("readByteBudget")).toString() != QString::number(kSearchReadByteBudget)
        || !actualString(result.value(QStringLiteral("reservedReadBytes")), 19)
        || result.value(QStringLiteral("reservedReadBytes")).toString() != QString::number(reserved)) return false;
    bool budgetLimited = false;
    if (next < manifest.size() && rows.size() < limit) {
        const QVariantMap descriptor = manifest.at(next).toMap();
        qint64 bytes = 0;
        decimal(descriptor.value(QStringLiteral("fileBytes")).toString(), &bytes);
        budgetLimited = descriptor.value(QStringLiteral("headerValid")).toBool() && bytes <= kSearchMaximumFileBytes
            && 2 * bytes > kSearchReadByteBudget - reserved;
        if (!budgetLimited) return false;
    }
    return actualBool(result.value(QStringLiteral("budgetLimited")), budgetLimited)
        && jsonBytes(result) <= qMin(kSearchMaximumResultBytes,
            policy.value(QStringLiteral("maximumPageMiB")).toLongLong() * kMiB) - kFacadeReserve;
}

FleetTelemetryArchiveHistoryReader::FleetTelemetryArchiveHistoryReader(const QString& canonicalAbsoluteDirectory,
                                                                       const QVariantMap& exactReadPolicy,
                                                                       const QString& queryId, const QVariantMap& exactRequest)
    : QThread(nullptr), _directory(ownedSearchText(exactRequest, canonicalAbsoluteDirectory)),
      _policy(ownedPolicy(exactRequest, exactReadPolicy)), _queryId(ownedSearchText(exactRequest, queryId)),
      _request(ownedRequest(exactRequest, exactReadPolicy))
{
    _configurationValid = FleetTelemetryArchiveCodec::readPolicyValid(_policy) && identifier(_queryId) && requestValid(_request, _policy);
    _status = claims(); _status.insert(QStringLiteral("queryId"), _queryId);
    _status.insert(QStringLiteral("state"), QStringLiteral("created")); _status.insert(QStringLiteral("errorCode"), QStringLiteral(""));
}

FleetTelemetryArchiveHistoryReader::~FleetTelemetryArchiveHistoryReader()
{
    Q_ASSERT(!isRunning()); // Owner must use finished -> deleteLater, never disk wait/terminate.
}

void FleetTelemetryArchiveHistoryReader::requestCancel()
{
    _cancelled.store(true, std::memory_order_release);
}

QVariantMap FleetTelemetryArchiveHistoryReader::current() const
{
    QMutexLocker lock(&_mutex);
    QVariantMap result = _status; result.insert(QStringLiteral("cancelRequested"), _cancelled.load(std::memory_order_acquire));
    return result;
}

void FleetTelemetryArchiveHistoryReader::run()
{
    { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("state"), QStringLiteral("reading")); }
    QString error = QStringLiteral("");
    QVariantMap result;
    bool ok = false;
    if (!_configurationValid) error = QStringLiteral("telemetry-history-request-rejected");
    else if (_cancelled.load()) error = QStringLiteral("telemetry-history-cancelled");
    else if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("catalog")) ok = catalog(_directory, _policy, _cancelled, &result, &error);
    else if (_request.value(QStringLiteral("kind")).toString() == QStringLiteral("search")) ok = search(_directory, _policy, _request, _cancelled, &result, &error);
    else ok = page(_directory, _policy, _request, _cancelled, &result, &error);
    if (_cancelled.load(std::memory_order_acquire)) { ok = false; error = QStringLiteral("telemetry-history-cancelled"); }
    {
        QMutexLocker lock(&_mutex);
        _status.insert(QStringLiteral("state"), ok ? QStringLiteral("finished") : (error == QStringLiteral("telemetry-history-cancelled") ? QStringLiteral("cancelled") : QStringLiteral("failed")));
        _status.insert(QStringLiteral("errorCode"), error);
    }
    if (ok && !_cancelled.load(std::memory_order_acquire)) emit resultReady(_queryId, result);
    else {
        const QString code = _cancelled.load() ? QStringLiteral("telemetry-history-cancelled") : (error.isEmpty() ? QStringLiteral("telemetry-history-read-failed") : error);
        { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("state"), code == QStringLiteral("telemetry-history-cancelled") ? QStringLiteral("cancelled") : QStringLiteral("failed")); _status.insert(QStringLiteral("errorCode"), code); }
        emit failure(_queryId, code);
    }
}

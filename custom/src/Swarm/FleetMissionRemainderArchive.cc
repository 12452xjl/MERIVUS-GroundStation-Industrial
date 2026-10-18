#include "FleetMissionRemainderArchive.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionStartupTransaction.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaType>
#include <QSaveFile>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr int MaximumDepth = 24;
constexpr int MaximumNodes = 1048576;
constexpr qlonglong MaximumExactInteger = Q_INT64_C(9007199254740991);

bool text(const QVariant& value, int maximum, bool empty = false)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    return (empty || !string.isEmpty()) && string.size() <= maximum
        && QString::fromUtf8(string.toUtf8()) == string;
}

bool integer(const QVariant& value, double minimum, double maximum)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    return !value.isNull() && std::isfinite(number) && std::floor(number) == number
        && number >= minimum && number <= maximum;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool positiveDecimal(const QVariant& value)
{
    if (!text(value, 20)) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok, 10);
    return ok && number > 0 && QString::number(number) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool exactKeys(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}

bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* scalarBytes)
{
    if (depth > MaximumDepth || ++(*nodes) > MaximumNodes) return false;
    // Native absent observations and parsed explicit JSON null are inert.
    // Typed-null QObject/custom values are still rejected by the type switch.
    if (!value.isValid() || value.type() == QVariant::Invalid || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(it.key(), 1024, true)) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > MaximumBytes || !boundedTree(it.value(), depth + 1, nodes, scalarBytes)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > 32768) return false;
        for (const QVariant& child : list) if (!boundedTree(child, depth + 1, nodes, scalarBytes)) return false;
        return true;
    }
    case QVariant::StringList: {
        const QStringList list = value.toStringList();
        if (list.size() > 32768) return false;
        for (const QString& child : list) if (!boundedTree(child, depth + 1, nodes, scalarBytes)) return false;
        return true;
    }
    case QVariant::String:
        if (!text(value, 32768, true)) return false;
        *scalarBytes += value.toString().toUtf8().size();
        return *scalarBytes <= MaximumBytes;
    case QVariant::Bool: return !value.isNull();
    case QVariant::Int: case QVariant::UInt: return !value.isNull();
    case QVariant::LongLong:
        return !value.isNull() && value.toLongLong() >= -MaximumExactInteger
            && value.toLongLong() <= MaximumExactInteger;
    case QVariant::ULongLong:
        return !value.isNull() && value.toULongLong() <= static_cast<qulonglong>(MaximumExactInteger);
    case QVariant::Double:
        return !value.isNull() && std::isfinite(value.toDouble())
            && std::abs(value.toDouble()) <= static_cast<double>(MaximumExactInteger);
    default: return false;
    }
}

QByteArray json(const QVariantMap& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

bool boundedJson(const QVariantMap& value, QByteArray* bytes = nullptr)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!boundedTree(value, 0, &nodes, &scalarBytes)) return false;
    const QByteArray encoded = json(value);
    if (encoded.isEmpty() || encoded.size() > MaximumBytes) return false;
    if (bytes) *bytes = encoded;
    return true;
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QVariantMap reply(const QString& error, const QVariantMap& archive = {})
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("archive"), archive}};
}

QVariantMap sourceReply(const QString& error, const QVariantMap& source = {})
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("source"), source}};
}

QVariantMap metadataReply(const QString& error, const QVariantMap& metadata = {})
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("metadata"), metadata}};
}

QVariantMap saveReply(const QString& error, bool committed = false)
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("committed"), committed}};
}

QVariantMap claims()
{
    return {{QStringLiteral("snapshotOnly"), true}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("planningOnly"), true}, {QStringLiteral("sourceAuthenticated"), false},
        {QStringLiteral("approvalRestored"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

bool contextValid(const QVariantMap& value)
{
    return exactKeys(value, {QStringLiteral("lifecycleEpoch"), QStringLiteral("selectionGeneration"), QStringLiteral("episodeId")})
        && uuid(value.value(QStringLiteral("lifecycleEpoch"))) && positiveDecimal(value.value(QStringLiteral("selectionGeneration")))
        && uuid(value.value(QStringLiteral("episodeId")));
}

bool checkedSource(const QVariantMap& value, bool nativeOnly, QVariantMap* source)
{
    if (nativeOnly && (value.size() != 18 || !integer(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || value.value(QStringLiteral("sourceKind")) != QVariant(QStringLiteral("native-run-planning-snapshot")))) return false;
    const QVariantMap checked = FleetMissionRemainderPlanner::validateSource(value);
    if (!exactKeys(checked, {QStringLiteral("ok"), QStringLiteral("reason"), QStringLiteral("source")})
        || !flag(checked, QStringLiteral("ok"), true) || !text(checked.value(QStringLiteral("reason")), 0, true)
        || checked.value(QStringLiteral("source")).type() != QVariant::Map || checked.value(QStringLiteral("source")).isNull()) return false;
    const QVariantMap canonical = checked.value(QStringLiteral("source")).toMap();
    // The shared validator may normalize JSON null/numeric widths, never
    // replace the complete captured source with a summary or a repaired one.
    if (canonical.isEmpty() || json(canonical) != json(value)) return false;
    if (source) *source = canonical;
    return true;
}

bool checkedCollection(const QVariantMap& value, QVariantMap* collection)
{
    const QVariantMap checked = FleetMissionStartupTransaction::validateCollection(value);
    if (!exactKeys(checked, {QStringLiteral("ok"), QStringLiteral("reason"), QStringLiteral("collection")})
        || !flag(checked, QStringLiteral("ok"), true) || !text(checked.value(QStringLiteral("reason")), 0, true)
        || checked.value(QStringLiteral("collection")).type() != QVariant::Map
        || checked.value(QStringLiteral("collection")).isNull()) return false;
    const QVariantMap canonical = checked.value(QStringLiteral("collection")).toMap();
    if (canonical.isEmpty() || json(canonical) != json(value)) return false;
    if (collection) *collection = canonical;
    return true;
}

bool inspectArchive(const QVariantMap& archive, QByteArray* bytes = nullptr, QVariantMap* nativeValue = nullptr)
{
    QByteArray encoded;
    if (!boundedJson(archive, &encoded) || !integer(archive.value(QStringLiteral("schemaVersion")), 1, 2)) return false;
    const bool collection = archive.value(QStringLiteral("schemaVersion")).toInt() == 2;
    const QString valueKey = collection ? QStringLiteral("nativeCollection") : QStringLiteral("nativeSource");
    if (!exactKeys(archive, {QStringLiteral("schemaVersion"), QStringLiteral("archiveType"), QStringLiteral("archiveId"),
            QStringLiteral("capturedAtUtc"), QStringLiteral("sourceContext"), valueKey,
            QStringLiteral("claims"), QStringLiteral("contentHash")})
        || archive.value(QStringLiteral("archiveType")) != QVariant(QStringLiteral("fleet-run-replanning-source"))
        || !uuid(archive.value(QStringLiteral("archiveId"))) || !text(archive.value(QStringLiteral("capturedAtUtc")), 32)
        || !digest(archive.value(QStringLiteral("contentHash")))) return false;
    for (const QString& key : {QStringLiteral("sourceContext"), valueKey, QStringLiteral("claims")})
        if (archive.value(key).type() != QVariant::Map || archive.value(key).isNull()) return false;
    const QString captured = archive.value(QStringLiteral("capturedAtUtc")).toString();
    const QDateTime date = QDateTime::fromString(captured, Qt::ISODateWithMs);
    if (!date.isValid() || date.toUTC().toString(Qt::ISODateWithMs) != captured
        || !contextValid(archive.value(QStringLiteral("sourceContext")).toMap())) return false;
    const QVariantMap actualClaims = archive.value(QStringLiteral("claims")).toMap();
    const QVariantMap expectedClaims = claims();
    if (actualClaims.size() != expectedClaims.size()) return false;
    for (auto it = expectedClaims.constBegin(); it != expectedClaims.constEnd(); ++it)
        if (!flag(actualClaims, it.key(), it.value().toBool())) return false;
    QVariantMap unhashed = archive;
    unhashed.remove(QStringLiteral("contentHash"));
    if (hash(json(unhashed)) != archive.value(QStringLiteral("contentHash")).toString()) return false;
    QVariantMap original;
    const QVariantMap value = archive.value(valueKey).toMap();
    if (collection ? !checkedCollection(value, &original) : !checkedSource(value, true, &original)) return false;
    if (bytes) *bytes = encoded;
    if (nativeValue) *nativeValue = original;
    return true;
}

QVariantMap historicalSource(const QVariantMap& archive, QVariantMap source)
{
    source.insert(QStringLiteral("schemaVersion"), 2);
    source.insert(QStringLiteral("sourceKind"), QStringLiteral("historical-run-planning-snapshot"));
    source.insert(QStringLiteral("sourceArchiveId"), archive.value(QStringLiteral("archiveId")));
    source.insert(QStringLiteral("sourceArchiveContentHash"), archive.value(QStringLiteral("contentHash")));
    QVariantMap canonical;
    if (!checkedSource(source, false, &canonical)) return sourceReply(QStringLiteral("remainder_archive_historical_source_invalid"));
    return sourceReply(QStringLiteral(""), canonical);
}

bool pathValid(const QString& path)
{
    if (path.isEmpty() || path.size() > 4096 || path != path.trimmed() || path.contains(QStringLiteral("://"))
        || path.startsWith(QStringLiteral(":")) || QString::fromUtf8(path.toUtf8()) != path) return false;
    for (const QChar character : path)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return QFileInfo(path).isAbsolute();
}

bool readArchive(const QString& path, QVariantMap* archive, QByteArray* originalBytes, QString* reason)
{
    const auto fail = [reason](const QString& error) { if (reason) *reason = error; return false; };
    const QFileInfo before(path);
    if (!pathValid(path) || !before.isFile() || before.isSymLink())
        return fail(QStringLiteral("remainder_archive_path_invalid"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(QStringLiteral("remainder_archive_open_failed"));
    const qint64 size = file.size();
    if (size <= 0 || size > MaximumBytes) return fail(QStringLiteral("remainder_archive_file_budget_exceeded"));
    const QByteArray bytes = file.read(MaximumBytes + 1);
    const QFileInfo after(path);
    if (file.error() != QFileDevice::NoError || bytes.size() != size || file.size() != size
        || !after.isFile() || after.isSymLink() || after.size() != size)
        return fail(QStringLiteral("remainder_archive_read_failed"));
    const QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes) return fail(QStringLiteral("remainder_archive_utf8_invalid"));
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return fail(QStringLiteral("remainder_archive_json_invalid"));
    const QVariantMap parsed = document.toVariant().toMap();
    if (!inspectArchive(parsed)) return fail(QStringLiteral("remainder_archive_invalid"));
    if (archive) *archive = parsed;
    if (originalBytes) *originalBytes = bytes;
    if (reason) *reason = QStringLiteral("");
    return true;
}
} // namespace

FleetMissionRemainderArchive::FleetMissionRemainderArchive(QObject* parent) : QObject(parent) {}

QVariantMap FleetMissionRemainderArchive::build(const QVariantMap& nativeSource, const QVariantMap& sourceContext) const
{
    if (QThread::currentThread() != thread()) return reply(QStringLiteral("remainder_archive_wrong_thread"));
    if (!boundedJson({{QStringLiteral("nativeSource"), nativeSource}, {QStringLiteral("sourceContext"), sourceContext}}))
        return reply(QStringLiteral("remainder_archive_input_budget_or_type_invalid"));
    QVariantMap original;
    if (!contextValid(sourceContext) || !checkedSource(nativeSource, true, &original))
        return reply(QStringLiteral("remainder_archive_native_source_or_context_invalid"));
    QVariantMap archive{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("archiveType"), QStringLiteral("fleet-run-replanning-source")},
        {QStringLiteral("archiveId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("capturedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("sourceContext"), sourceContext}, {QStringLiteral("nativeSource"), original},
        {QStringLiteral("claims"), claims()}};
    QByteArray bytes;
    if (!boundedJson(archive, &bytes)) return reply(QStringLiteral("remainder_archive_output_budget_exceeded"));
    // Independent JSON-owned values preserve explicit null when entering QML.
    archive = QJsonDocument::fromJson(bytes).toVariant().toMap();
    archive.insert(QStringLiteral("contentHash"), hash(json(archive)));
    if (!inspectArchive(archive)) return reply(QStringLiteral("remainder_archive_output_invalid"));
    return reply(QStringLiteral(""), archive);
}

QVariantMap FleetMissionRemainderArchive::buildCollection(const QVariantMap& nativeCollection,
    const QVariantMap& sourceContext) const
{
    if (QThread::currentThread() != thread()) return reply(QStringLiteral("remainder_archive_wrong_thread"));
    if (!boundedJson({{QStringLiteral("nativeCollection"), nativeCollection}, {QStringLiteral("sourceContext"), sourceContext}}))
        return reply(QStringLiteral("remainder_archive_input_budget_or_type_invalid"));
    QVariantMap original;
    if (!contextValid(sourceContext) || !checkedCollection(nativeCollection, &original))
        return reply(QStringLiteral("remainder_archive_native_collection_or_context_invalid"));
    QVariantMap archive{{QStringLiteral("schemaVersion"), 2},
        {QStringLiteral("archiveType"), QStringLiteral("fleet-run-replanning-source")},
        {QStringLiteral("archiveId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("capturedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("sourceContext"), sourceContext}, {QStringLiteral("nativeCollection"), original},
        {QStringLiteral("claims"), claims()}};
    QByteArray bytes;
    if (!boundedJson(archive, &bytes)) return reply(QStringLiteral("remainder_archive_output_budget_exceeded"));
    archive = QJsonDocument::fromJson(bytes).toVariant().toMap();
    archive.insert(QStringLiteral("contentHash"), hash(json(archive)));
    if (!inspectArchive(archive)) return reply(QStringLiteral("remainder_archive_output_invalid"));
    return reply(QStringLiteral(""), archive);
}

bool FleetMissionRemainderArchive::archiveValid(const QVariantMap& archive)
{
    return inspectArchive(archive);
}

QVariantMap FleetMissionRemainderArchive::metadata(const QVariantMap& archive) const
{
    if (QThread::currentThread() != thread()) return metadataReply(QStringLiteral("remainder_archive_wrong_thread"));
    QVariantMap native;
    if (!inspectArchive(archive, nullptr, &native)) return metadataReply(QStringLiteral("remainder_archive_invalid"));
    const bool collection = archive.value(QStringLiteral("schemaVersion")).toInt() == 2;
    QVariantList rows;
    const QVariantList targets = collection ? native.value(QStringLiteral("targets")).toList()
        : QVariantList{QVariantMap{{QStringLiteral("vehicleId"), native.value(QStringLiteral("sourceVehicleId"))},
            {QStringLiteral("available"), true}, {QStringLiteral("reason"), QStringLiteral("")},
            {QStringLiteral("sourceMember"), native.value(QStringLiteral("sourceMember"))},
            {QStringLiteral("progress"), native.value(QStringLiteral("progress"))}}};
    for (const QVariant& entry : targets) {
        const QVariantMap target = entry.toMap();
        const QVariantMap progress = target.value(QStringLiteral("progress")).toMap();
        QVariant began = progress.value(QStringLiteral("modeQueueBegan"));
        const QVariant committed = progress.value(QStringLiteral("modeQueueCommitted"));
        // Original v1 validation did not require a begin flag. Preserve its
        // bool or explicit unknown without inventing a false/true queue fact;
        // v1 availability retains that original single-source value contract.
        if (!collection && (began.type() != QVariant::Bool || began.isNull())) began = QVariant();
        if ((collection && (began.type() != QVariant::Bool || began.isNull()))
            || committed.type() != QVariant::Bool || committed.isNull())
            return metadataReply(QStringLiteral("remainder_archive_member_metadata_invalid"));
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("available"), target.value(QStringLiteral("available"))},
            {QStringLiteral("reason"), target.value(QStringLiteral("reason"))},
            {QStringLiteral("waypointCount"), target.value(QStringLiteral("sourceMember")).toMap()
                .value(QStringLiteral("waypoints")).toList().size()},
            {QStringLiteral("modeQueueBegan"), began}, {QStringLiteral("modeQueueCommitted"), committed}});
    }
    QVariantMap result{{QStringLiteral("schemaVersion"), archive.value(QStringLiteral("schemaVersion"))},
        {QStringLiteral("archiveId"), archive.value(QStringLiteral("archiveId"))},
        {QStringLiteral("contentHash"), archive.value(QStringLiteral("contentHash"))},
        {QStringLiteral("capturedAtUtc"), archive.value(QStringLiteral("capturedAtUtc"))},
        {QStringLiteral("sourceContext"), archive.value(QStringLiteral("sourceContext"))},
        {QStringLiteral("binding"), native.value(QStringLiteral("binding"))}, {QStringLiteral("members"), rows}};
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
        QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId")}) result.insert(key, native.value(key));
    // Small independent metadata only: never publish a payload, plan or task.
    result = QJsonDocument::fromVariant(result).toVariant().toMap();
    return metadataReply(QStringLiteral(""), result);
}

QVariantMap FleetMissionRemainderArchive::planningSource(const QVariantMap& archive) const
{
    if (QThread::currentThread() != thread()) return sourceReply(QStringLiteral("remainder_archive_wrong_thread"));
    QVariantMap source;
    if (!inspectArchive(archive, nullptr, &source)) return sourceReply(QStringLiteral("remainder_archive_invalid"));
    if (archive.value(QStringLiteral("schemaVersion")).toInt() != 1)
        return sourceReply(QStringLiteral("remainder_archive_vehicle_selection_required"));
    return historicalSource(archive, source);
}

QVariantMap FleetMissionRemainderArchive::planningSourceForVehicle(const QVariantMap& archive,
    const QVariant& sourceVehicleId) const
{
    if (QThread::currentThread() != thread()) return sourceReply(QStringLiteral("remainder_archive_wrong_thread"));
    if (!integer(sourceVehicleId, 1, 255)) return sourceReply(QStringLiteral("remainder_archive_vehicle_selection_invalid"));
    QVariantMap native;
    if (!inspectArchive(archive, nullptr, &native)) return sourceReply(QStringLiteral("remainder_archive_invalid"));
    QVariantMap source;
    if (archive.value(QStringLiteral("schemaVersion")).toInt() == 1) {
        if (native.value(QStringLiteral("sourceVehicleId")).toInt() != sourceVehicleId.toInt())
            return sourceReply(QStringLiteral("remainder_archive_vehicle_not_present"));
        source = native;
    } else {
        // The native pure selector revalidates every row, not only a chosen
        // available prefix, before projecting the original native18 source.
        const QVariantMap selected = FleetMissionStartupTransaction::sourceFromCollection(native, sourceVehicleId.toInt());
        if (!exactKeys(selected, {QStringLiteral("ok"), QStringLiteral("reason"), QStringLiteral("source")})
            || !flag(selected, QStringLiteral("ok"), true) || !text(selected.value(QStringLiteral("reason")), 0, true)
            || selected.value(QStringLiteral("source")).type() != QVariant::Map
            || selected.value(QStringLiteral("source")).isNull()
            || !checkedSource(selected.value(QStringLiteral("source")).toMap(), true, &source))
            return sourceReply(QStringLiteral("remainder_archive_vehicle_unavailable"));
    }
    return historicalSource(archive, source);
}

QVariantMap FleetMissionRemainderArchive::saveToFile(const QString& absoluteLocalPath, const QVariantMap& archive) const
{
    if (QThread::currentThread() != thread()) return saveReply(QStringLiteral("remainder_archive_wrong_thread"));
    const QString path = absoluteLocalPath;
    QByteArray bytes;
    if (!inspectArchive(archive, &bytes)) return saveReply(QStringLiteral("remainder_archive_invalid"));
    if (!pathValid(path) || QFileInfo(path).isSymLink()) return saveReply(QStringLiteral("remainder_archive_path_invalid"));
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return saveReply(QStringLiteral("remainder_archive_open_failed"));
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return saveReply(QStringLiteral("remainder_archive_write_failed"));
    }
    if (!file.commit()) return saveReply(QStringLiteral("remainder_archive_commit_failed"));
    QByteArray observedBytes;
    QString reason;
    if (!readArchive(path, nullptr, &observedBytes, &reason)) return saveReply(reason, true);
    if (observedBytes != bytes) return saveReply(QStringLiteral("remainder_archive_readback_mismatch"), true);
    return saveReply(QStringLiteral(""), true);
}

QVariantMap FleetMissionRemainderArchive::loadFromFile(const QString& absoluteLocalPath) const
{
    if (QThread::currentThread() != thread()) return reply(QStringLiteral("remainder_archive_wrong_thread"));
    QVariantMap archive;
    QString reason;
    if (!readArchive(absoluteLocalPath, &archive, nullptr, &reason)) return reply(reason);
    return reply(QStringLiteral(""), archive);
}

#include "FleetMaintenanceWorkspaceWorker.h"

#include "FleetMaintenancePassport.h"
#include "FleetSnapshotFileIo.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaType>
#include <QSet>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024LL * 1024LL;
constexpr int MaximumDetailCharacters = 64 * 1024 * 1024;
constexpr qint64 MaximumExactInteger = 9007199254740991LL;
constexpr int MaximumRecords = 2000;
constexpr int PageSize = 8;
constexpr int TextSize = 2048;
constexpr int MaximumNodes = 262144;
constexpr int MaximumDepth = 24;

bool exact(const QVariantMap& value, const QSet<QString>& keys)
{
    if (value.size() != keys.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum, bool empty = false)
{
    return value.type() == QVariant::String && !value.isNull()
        && (empty || !value.toString().isEmpty()) && value.toString().size() <= maximum;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    if (!value.isValid() || value.isNull() || value.type() == QVariant::Bool) return false;
    qint64 number = 0;
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt || value.type() == QVariant::LongLong) {
        bool ok = false;
        number = value.toLongLong(&ok);
        if (!ok) return false;
    } else if (value.type() == QVariant::ULongLong) {
        const qulonglong unsignedNumber = value.toULongLong();
        if (unsignedNumber > static_cast<qulonglong>(MaximumExactInteger)) return false;
        number = static_cast<qint64>(unsignedNumber);
    } else if (value.type() == QVariant::Double) {
        const double real = value.toDouble();
        if (!std::isfinite(real) || std::floor(real) != real
            || real < -static_cast<double>(MaximumExactInteger) || real > static_cast<double>(MaximumExactInteger)) return false;
        number = static_cast<qint64>(real);
    } else return false;
    if (number < minimum || number > maximum) return false;
    if (output) *output = number;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar c : value.toString())
        if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9')) || (c >= QLatin1Char('a') && c <= QLatin1Char('f')))) return false;
    return true;
}

bool counter(const QVariant& value)
{
    if (!text(value, 20)) return false;
    const QString raw = value.toString();
    bool ok = false;
    const qulonglong parsed = raw.toULongLong(&ok);
    return ok && QString::number(parsed) == raw;
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QByteArray json(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

// No QObject, custom converter, DateTime, byte array or typed-null container
// may cross into JSON conversion. The GUI performs the same complete freeze
// preflight incrementally; this independently validates file-derived values.
bool pureTree(const QVariant& value, int depth, int* nodes, qint64* scalarBytes,
              const std::atomic_bool* cancelled)
{
    if (cancelled->load() || depth > MaximumDepth || ++*nodes > MaximumNodes) return false;
    // Qt versions represent JSON null as Invalid or QMetaType::Nullptr.
    // Neither case admits a typed QObject* null or another custom type.
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.type() == QVariant::Map) {
        if (value.isNull()) return false;
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > MaximumBytes || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > MaximumBytes || !pureTree(it.value(), depth + 1, nodes, scalarBytes, cancelled)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        if (value.isNull()) return false;
        for (const QVariant& child : value.toList())
            if (!pureTree(child, depth + 1, nodes, scalarBytes, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        const QString raw = value.toString();
        if (raw.size() > MaximumBytes) return false;
        const QByteArray encoded = raw.toUtf8();
        if (QString::fromUtf8(encoded) != raw) return false;
        *scalarBytes += encoded.size();
    } else if (value.type() == QVariant::Bool) {
        if (value.isNull()) return false;
        *scalarBytes += 5;
    } else if (value.type() == QVariant::Int || value.type() == QVariant::UInt
               || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
               || value.type() == QVariant::Double) {
        if (value.isNull() || !std::isfinite(value.toDouble())) return false;
        if (value.type() == QVariant::LongLong
            && (value.toLongLong() < -MaximumExactInteger || value.toLongLong() > MaximumExactInteger)) return false;
        if (value.type() == QVariant::ULongLong
            && value.toULongLong() > static_cast<qulonglong>(MaximumExactInteger)) return false;
        *scalarBytes += 32;
    } else return false;
    return *scalarBytes <= MaximumBytes;
}

bool pureTree(const QVariant& value, const std::atomic_bool* cancelled)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    return pureTree(value, 0, &nodes, &scalarBytes, cancelled);
}

bool localPath(const QVariant& value)
{
    return FleetSnapshotFileIo::localJsonPath(value);
}

bool readBytes(const QString& path, QByteArray* bytes, QString* error, const std::atomic_bool* cancelled)
{
    return FleetSnapshotFileIo::readUtf8(path, MaximumBytes, *cancelled,
        QStringLiteral("maintenance-workspace"), bytes, error);
}

QString groupKey(int vehicleId, const QString& scope)
{
    return hash(json(QVariantMap{{QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("historyScopeId"), scope}}));
}

QVariantMap groupRow(const QVariantMap& assessment, int recordCount)
{
    const int vehicleId = assessment.value(QStringLiteral("vehicleId")).toInt();
    const QString scope = assessment.value(QStringLiteral("historyScopeId")).toString();
    return {{QStringLiteral("groupId"), groupKey(vehicleId, scope)}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("historyScopeId"), scope.isEmpty() ? QStringLiteral("") : scope},
        {QStringLiteral("historicalOnly"), !scope.isEmpty()}, {QStringLiteral("recordCount"), recordCount},
        {QStringLiteral("maintenanceStatus"), assessment.value(QStringLiteral("maintenanceStatus"))},
        {QStringLiteral("predictiveRisk"), assessment.value(QStringLiteral("predictiveRisk"))},
        {QStringLiteral("estimatedRiskScore"), assessment.value(QStringLiteral("estimatedRiskScore"))},
        {QStringLiteral("confidence"), assessment.value(QStringLiteral("confidence"))},
        {QStringLiteral("sourceSelectionRequired"), false}, {QStringLiteral("manualReviewRequired"), true},
        {QStringLiteral("historicalTelemetrySource"), scope.isEmpty() ? QVariantMap()
            : assessment.value(QStringLiteral("historicalTelemetrySource")).toMap()},
        {QStringLiteral("physicalIdentityVerified"), false}};
}

QVariantMap makeCache(const QByteArray& bytes, const QString& snapshotId, const QString& origin,
    const QString& epoch, const QString& revision, qint64 capturedAt,
    QString* error, const std::atomic_bool* cancelled)
{
    if (cancelled->load()) { *error = QStringLiteral("maintenance-workspace-cancelled"); return {}; }
    if (bytes.isEmpty() || bytes.size() > MaximumBytes || QString::fromUtf8(bytes).toUtf8() != bytes) {
        *error = QStringLiteral("maintenance-workspace-file-budget-or-utf8"); return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
        || !document.object().value(QStringLiteral("records")).isArray()
        || !pureTree(document.object().value(QStringLiteral("records")).toVariant(), cancelled)) {
        *error = cancelled->load() ? QStringLiteral("maintenance-workspace-cancelled")
            : QStringLiteral("maintenance-workspace-json-budget-or-type"); return {};
    }
    const QJsonValue generated = document.object().value(QStringLiteral("generatedAt"));
    // Legacy import accepts an arbitrary String here. This new small public
    // descriptor must not turn a near-file-sized root string into GUI state.
    // Reject the whole workspace open rather than truncate the source value.
    if (generated.isString() && generated.toString().size() > 128) {
        *error = QStringLiteral("maintenance-workspace-generated-at-budget"); return {};
    }
    // A worker-local object has no observers and no GUI/archive/native source.
    // Reuse the actual schema1/conditional-marker/normalization/overlap rules.
    FleetMaintenancePassport passport;
    if (!passport.importJson(QString::fromUtf8(bytes))) {
        *error = QStringLiteral("maintenance-workspace-document-invalid"); return {};
    }
    const QVariantList records = passport.records();
    if (records.size() > MaximumRecords || !pureTree(records, cancelled)) {
        *error = cancelled->load() ? QStringLiteral("maintenance-workspace-cancelled")
            : QStringLiteral("maintenance-workspace-normalized-budget-or-type"); return {};
    }
    const QVariantList assessments = passport.assessAll();
    QVariantList groups;
    for (const QVariant& value : assessments) {
        if (cancelled->load()) { *error = QStringLiteral("maintenance-workspace-cancelled"); return {}; }
        const QVariantMap assessment = value.toMap();
        const int vehicleId = assessment.value(QStringLiteral("vehicleId")).toInt();
        const QString scope = assessment.value(QStringLiteral("historyScopeId")).toString();
        QVariantList indices;
        for (int i = 0; i < records.size(); ++i) {
            if (cancelled->load()) { *error = QStringLiteral("maintenance-workspace-cancelled"); return {}; }
            const QVariantMap record = records.at(i).toMap();
            if (record.value(QStringLiteral("vehicleId")).toInt() == vehicleId
                && record.value(QStringLiteral("historyScopeId")).toString() == scope) indices.append(i);
        }
        if (indices.isEmpty()) { *error = QStringLiteral("maintenance-workspace-group-invalid"); return {}; }
        groups.append(QVariantMap{{QStringLiteral("row"), groupRow(assessment, indices.size())},
            {QStringLiteral("assessment"), assessment}, {QStringLiteral("recordIndices"), indices}});
    }
    if (cancelled->load()) { *error = QStringLiteral("maintenance-workspace-cancelled"); return {}; }
    const QString contentHash = hash(bytes);
    const QVariantMap descriptor{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("snapshotId"), snapshotId},
        {QStringLiteral("origin"), origin}, {QStringLiteral("sourceInstanceEpoch"), epoch},
        {QStringLiteral("sourceRevision"), revision}, {QStringLiteral("capturedAtUtcMs"), capturedAt},
        {QStringLiteral("documentGeneratedAt"), generated.isString() && !generated.toString().isEmpty()
            ? generated.toString() : QStringLiteral("")},
        {QStringLiteral("recordCount"), records.size()}, {QStringLiteral("groupCount"), groups.size()},
        {QStringLiteral("contentHash"), contentHash},
        {QStringLiteral("sourceFileHash"), origin == QStringLiteral("file") ? contentHash : QStringLiteral("")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("physicalIdentityVerified"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    return {{QStringLiteral("descriptor"), descriptor}, {QStringLiteral("bytes"), bytes},
        {QStringLiteral("records"), records}, {QStringLiteral("groups"), groups},
        {QStringLiteral("detailCache"), QVariantMap()}};
}

bool cacheValid(const QVariantMap& cache)
{
    if (!exact(cache, {QStringLiteral("descriptor"), QStringLiteral("bytes"), QStringLiteral("records"),
            QStringLiteral("groups"), QStringLiteral("detailCache")})
        || cache.value(QStringLiteral("descriptor")).type() != QVariant::Map
        || cache.value(QStringLiteral("bytes")).type() != QVariant::ByteArray
        || cache.value(QStringLiteral("records")).type() != QVariant::List
        || cache.value(QStringLiteral("groups")).type() != QVariant::List
        || cache.value(QStringLiteral("detailCache")).type() != QVariant::Map) return false;
    const QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
    const QByteArray bytes = cache.value(QStringLiteral("bytes")).toByteArray();
    return uuid(descriptor.value(QStringLiteral("snapshotId"))) && digest(descriptor.value(QStringLiteral("contentHash")))
        && !bytes.isEmpty() && bytes.size() <= MaximumBytes && hash(bytes) == descriptor.value(QStringLiteral("contentHash")).toString()
        && integer(descriptor.value(QStringLiteral("recordCount")), 0, MaximumRecords)
        && descriptor.value(QStringLiteral("recordCount")).toInt() == cache.value(QStringLiteral("records")).toList().size()
        && integer(descriptor.value(QStringLiteral("groupCount")), 0, MaximumRecords)
        && descriptor.value(QStringLiteral("groupCount")).toInt() == cache.value(QStringLiteral("groups")).toList().size();
}

QVariantMap findGroup(const QVariantList& groups, const QString& id)
{
    for (const QVariant& value : groups) {
        const QVariantMap group = value.toMap();
        if (group.value(QStringLiteral("row")).toMap().value(QStringLiteral("groupId")).toString() == id) return group;
    }
    return {};
}

QVariantMap recordRow(const QVariantMap& record, int recordIndex, int groupIndex)
{
    QVariantMap metrics;
    for (const QString& key : {QStringLiteral("currentAmps"), QStringLiteral("temperatureC"), QStringLiteral("vibrationRms"),
            QStringLiteral("batteryDegradationPercent"), QStringLiteral("flightCount")})
        if (record.contains(key)) metrics.insert(key, record.value(key));
    return {{QStringLiteral("recordIndex"), recordIndex}, {QStringLiteral("groupRecordIndex"), groupIndex},
        {QStringLiteral("timestamp"), record.value(QStringLiteral("timestamp"))},
        {QStringLiteral("timestampMs"), record.value(QStringLiteral("timestampMs"))},
        {QStringLiteral("fieldCount"), record.size()}, {QStringLiteral("historicalOnly"), record.contains(QStringLiteral("historicalTelemetrySource"))},
        {QStringLiteral("metrics"), metrics}, {QStringLiteral("hasModifications"), record.contains(QStringLiteral("modificationRecords"))}};
}
} // namespace

FleetMaintenanceWorkspaceWorker::FleetMaintenanceWorkspaceWorker(const QString& jobId, const QString& action,
    const QVariantMap& request, const QVariantMap& privateSnapshot, const QVariantList& freezeRecords)
    : _jobId(jobId), _action(action), _request(request), _snapshot(privateSnapshot), _records(freezeRecords)
{
}

void FleetMaintenanceWorkspaceWorker::requestCancel()
{
    _cancelled.store(true);
}

void FleetMaintenanceWorkspaceWorker::run()
{
    QVariantMap result{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("maintenance-workspace-request-invalid")},
        {QStringLiteral("action"), _action}, {QStringLiteral("cache"), QVariantMap()}, {QStringLiteral("reply"), QVariantMap()}};
    const auto finish = [this, &result](const QString& error) {
        result[QStringLiteral("error")] = error;
        emit resultReady(_jobId, result);
    };
    if (!uuid(QVariant(_jobId))) { finish(QStringLiteral("maintenance-workspace-job-invalid")); return; }
    if (_cancelled.load()) { finish(QStringLiteral("maintenance-workspace-cancelled")); return; }
    if (_action == QStringLiteral("freeze")) {
        qint64 capturedAt = 0;
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("sourceInstanceEpoch"),
                QStringLiteral("sourceRevision"), QStringLiteral("capturedAtUtcMs")})
            || !uuid(_request.value(QStringLiteral("snapshotId"))) || !uuid(_request.value(QStringLiteral("sourceInstanceEpoch")))
            || !counter(_request.value(QStringLiteral("sourceRevision")))
            || !integer(_request.value(QStringLiteral("capturedAtUtcMs")), 1, MaximumExactInteger, &capturedAt)
            || _records.size() > MaximumRecords || !pureTree(_records, &_cancelled)) {
            finish(_cancelled.load() ? QStringLiteral("maintenance-workspace-cancelled") : QStringLiteral("maintenance-workspace-freeze-invalid")); return;
        }
        const QDateTime frozenAt = QDateTime::fromMSecsSinceEpoch(capturedAt, Qt::UTC);
        if (!frozenAt.isValid()) { finish(QStringLiteral("maintenance-workspace-freeze-time-invalid")); return; }
        QVariantMap document{{QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("documentType"), QStringLiteral("merivus_fleet_maintenance_passport")},
            {QStringLiteral("riskModel"), QStringLiteral("heuristic_estimate_review_only")},
            {QStringLiteral("generatedAt"), frozenAt.toString(Qt::ISODateWithMs)}, {QStringLiteral("records"), _records}};
        for (const QVariant& value : _records) if (value.toMap().contains(QStringLiteral("historicalTelemetrySource"))) {
            document.insert(QStringLiteral("historySourceContract"), QStringLiteral("merivus-telemetry-history-source-v1")); break;
        }
        QString error;
        const QVariantMap cache = makeCache(json(document), _request.value(QStringLiteral("snapshotId")).toString(), QStringLiteral("session"),
            _request.value(QStringLiteral("sourceInstanceEpoch")).toString(), _request.value(QStringLiteral("sourceRevision")).toString(),
            capturedAt, &error, &_cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        result[QStringLiteral("cache")] = cache;
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("descriptor"), cache.value(QStringLiteral("descriptor"))}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (_action == QStringLiteral("open")) {
        if (!exact(_request, {QStringLiteral("path"), QStringLiteral("snapshotId")})
            || !localPath(_request.value(QStringLiteral("path"))) || !uuid(_request.value(QStringLiteral("snapshotId")))) {
            finish(QStringLiteral("maintenance-workspace-open-selection-invalid")); return;
        }
        const qint64 capturedAt = QDateTime::currentMSecsSinceEpoch();
        if (capturedAt <= 0 || capturedAt > MaximumExactInteger) { finish(QStringLiteral("maintenance-workspace-time-unavailable")); return; }
        QByteArray bytes;
        QString error;
        if (!readBytes(_request.value(QStringLiteral("path")).toString(), &bytes, &error, &_cancelled)) { finish(error); return; }
        const QVariantMap cache = makeCache(bytes, _request.value(QStringLiteral("snapshotId")).toString(), QStringLiteral("file"),
            QStringLiteral(""), QStringLiteral("0"), capturedAt, &error, &_cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        result[QStringLiteral("cache")] = cache;
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("descriptor"), cache.value(QStringLiteral("descriptor"))}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (!cacheValid(_snapshot) || _request.value(QStringLiteral("snapshotId"))
            != _snapshot.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId"))) {
        finish(QStringLiteral("maintenance-workspace-snapshot-unavailable")); return;
    }
    const QVariantMap descriptor = _snapshot.value(QStringLiteral("descriptor")).toMap();
    const QString snapshotId = descriptor.value(QStringLiteral("snapshotId")).toString();
    if (_action == QStringLiteral("save")) {
        QVariantMap reply{{QStringLiteral("snapshotId"), snapshotId}, {QStringLiteral("contentHash"), descriptor.value(QStringLiteral("contentHash"))},
            {QStringLiteral("path"), text(_request.value(QStringLiteral("path")), 4096) ? _request.value(QStringLiteral("path")) : QVariant(QStringLiteral(""))},
            {QStringLiteral("fileWriteAttempted"), false}, {QStringLiteral("fileWrittenKnown"), true}, {QStringLiteral("fileWritten"), false},
            {QStringLiteral("readbackVerified"), false}, {QStringLiteral("readbackError"), QStringLiteral("")},
            {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}, {QStringLiteral("authenticated"), false}};
        result[QStringLiteral("reply")] = reply;
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("contentHash"), QStringLiteral("path"), QStringLiteral("overwriteConfirmed")})
            || !localPath(_request.value(QStringLiteral("path"))) || !digest(_request.value(QStringLiteral("contentHash")))
            || _request.value(QStringLiteral("contentHash")) != descriptor.value(QStringLiteral("contentHash"))
            || _request.value(QStringLiteral("overwriteConfirmed")).type() != QVariant::Bool
            || !_request.value(QStringLiteral("overwriteConfirmed")).toBool()
            || descriptor.value(QStringLiteral("origin")).toString() != QStringLiteral("session")) {
            finish(QStringLiteral("maintenance-workspace-save-selection-invalid")); return;
        }
        const QString path = _request.value(QStringLiteral("path")).toString();
        const QByteArray bytes = _snapshot.value(QStringLiteral("bytes")).toByteArray();
        const FleetSnapshotFileIo::SaveResult saved = FleetSnapshotFileIo::saveAndReadback(
            path, bytes, MaximumBytes, _cancelled, QStringLiteral("maintenance-workspace"));
        reply[QStringLiteral("fileWriteAttempted")] = saved.fileWriteAttempted;
        reply[QStringLiteral("fileWrittenKnown")] = saved.fileWrittenKnown;
        reply[QStringLiteral("fileWritten")] = saved.fileWritten;
        reply[QStringLiteral("readbackVerified")] = saved.readbackVerified;
        reply[QStringLiteral("readbackError")] = saved.readbackError;
        result[QStringLiteral("ok")] = saved.ok;
        result[QStringLiteral("reply")] = reply;
        finish(saved.error); return;
    }
    const QVariantList groups = _snapshot.value(QStringLiteral("groups")).toList();
    qint64 after = 0;
    if (_action == QStringLiteral("groups")) {
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("afterIndex")})
            || !integer(_request.value(QStringLiteral("afterIndex")), 0, groups.size(), &after)) {
            finish(QStringLiteral("maintenance-workspace-page-invalid")); return;
        }
        QVariantList rows;
        const int next = qMin(groups.size(), static_cast<int>(after) + PageSize);
        for (int i = static_cast<int>(after); i < next; ++i) {
            if (_cancelled.load()) { finish(QStringLiteral("maintenance-workspace-cancelled")); return; }
            rows.append(groups.at(i).toMap().value(QStringLiteral("row")));
        }
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("snapshotId"), snapshotId}, {QStringLiteral("afterIndex"), after},
            {QStringLiteral("nextIndex"), next}, {QStringLiteral("hasMore"), next < groups.size()},
            {QStringLiteral("totalCount"), groups.size()}, {QStringLiteral("rows"), rows}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (!digest(_request.value(QStringLiteral("groupId")))) { finish(QStringLiteral("maintenance-workspace-group-invalid")); return; }
    const QString groupId = _request.value(QStringLiteral("groupId")).toString();
    const QVariantMap group = findGroup(groups, groupId);
    if (group.isEmpty()) { finish(QStringLiteral("maintenance-workspace-group-unavailable")); return; }
    const QVariantList indices = group.value(QStringLiteral("recordIndices")).toList();
    const QVariantList records = _snapshot.value(QStringLiteral("records")).toList();
    if (_action == QStringLiteral("records")) {
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("groupId"), QStringLiteral("afterIndex")})
            || !integer(_request.value(QStringLiteral("afterIndex")), 0, indices.size(), &after)) {
            finish(QStringLiteral("maintenance-workspace-page-invalid")); return;
        }
        QVariantList rows;
        const int next = qMin(indices.size(), static_cast<int>(after) + PageSize);
        for (int i = static_cast<int>(after); i < next; ++i) {
            if (_cancelled.load()) { finish(QStringLiteral("maintenance-workspace-cancelled")); return; }
            const int recordIndex = indices.at(i).toInt();
            if (recordIndex < 0 || recordIndex >= records.size()) { finish(QStringLiteral("maintenance-workspace-record-unavailable")); return; }
            rows.append(recordRow(records.at(recordIndex).toMap(), recordIndex, i));
        }
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("snapshotId"), snapshotId}, {QStringLiteral("groupId"), groupId},
            {QStringLiteral("afterIndex"), after}, {QStringLiteral("nextIndex"), next}, {QStringLiteral("hasMore"), next < indices.size()},
            {QStringLiteral("totalCount"), indices.size()}, {QStringLiteral("rows"), rows}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (_action != QStringLiteral("detail")) { finish(QStringLiteral("maintenance-workspace-action-invalid")); return; }
    qint64 recordIndex = -1, offset = 0;
    if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("groupId"), QStringLiteral("recordIndex"), QStringLiteral("offset")})
        || !integer(_request.value(QStringLiteral("recordIndex")), -1, records.size() - 1, &recordIndex)
        || !integer(_request.value(QStringLiteral("offset")), 0, MaximumDetailCharacters, &offset)
        || (recordIndex >= 0 && !indices.contains(QVariant(static_cast<int>(recordIndex))))) {
        finish(QStringLiteral("maintenance-workspace-detail-selection-invalid")); return;
    }
    QVariantMap detail = _snapshot.value(QStringLiteral("detailCache")).toMap();
    if (detail.value(QStringLiteral("groupId")).toString() != groupId
        || detail.value(QStringLiteral("recordIndex")).toInt() != recordIndex
        || detail.value(QStringLiteral("text")).type() != QVariant::String || !digest(detail.value(QStringLiteral("contentHash")))) {
        if (_cancelled.load()) { finish(QStringLiteral("maintenance-workspace-cancelled")); return; }
        const QByteArray bytes = json(recordIndex < 0 ? group.value(QStringLiteral("assessment")) : records.at(static_cast<int>(recordIndex)));
        const QString fullText = QString::fromUtf8(bytes);
        if (bytes.isEmpty() || bytes.size() > MaximumDetailCharacters || fullText.size() > MaximumDetailCharacters
            || fullText.toUtf8() != bytes) { finish(QStringLiteral("maintenance-workspace-detail-budget")); return; }
        detail = {{QStringLiteral("groupId"), groupId}, {QStringLiteral("recordIndex"), static_cast<int>(recordIndex)},
            {QStringLiteral("text"), fullText}, {QStringLiteral("contentHash"), hash(bytes)}};
    }
    const QString fullText = detail.value(QStringLiteral("text")).toString();
    if (offset > fullText.size() || (offset < fullText.size() && fullText.at(static_cast<int>(offset)).isLowSurrogate())) {
        finish(QStringLiteral("maintenance-workspace-detail-offset-invalid")); return;
    }
    int next = qMin(fullText.size(), static_cast<int>(offset) + TextSize);
    if (next < fullText.size() && next > offset && fullText.at(next - 1).isHighSurrogate()) --next;
    if (_cancelled.load()) { finish(QStringLiteral("maintenance-workspace-cancelled")); return; }
    QVariantMap cache = _snapshot;
    cache[QStringLiteral("detailCache")] = detail;
    const QString fragment = fullText.mid(static_cast<int>(offset), next - static_cast<int>(offset));
    result[QStringLiteral("cache")] = cache;
    result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("snapshotId"), snapshotId}, {QStringLiteral("groupId"), groupId},
        {QStringLiteral("recordIndex"), static_cast<int>(recordIndex)}, {QStringLiteral("offset"), offset}, {QStringLiteral("nextOffset"), next},
        {QStringLiteral("hasMore"), next < fullText.size()}, {QStringLiteral("totalCharacters"), fullText.size()},
        {QStringLiteral("contentHash"), detail.value(QStringLiteral("contentHash"))},
        {QStringLiteral("text"), fragment.isEmpty() ? QStringLiteral("") : fragment}, {QStringLiteral("canonicalJson"), true}};
    result[QStringLiteral("ok")] = true; finish(QStringLiteral(""));
}

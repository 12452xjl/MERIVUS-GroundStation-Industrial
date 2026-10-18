#include "FleetGnssMemoryWorkspaceWorker.h"
#include "FleetSnapshotFileIo.h"
#include "FleetTelemetryGnssReview.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QUuid>

#include <cmath>

namespace {
constexpr qint64 MaximumBytes = 4LL * 1024LL * 1024LL;
constexpr qint64 MaximumExactInteger = 9007199254740991LL;
constexpr int MaximumReports = 32;
constexpr int PageSize = 8;

bool exact(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum, bool empty = false)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString().size() <= maximum
        && (empty || !value.toString().isEmpty());
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool counter(const QVariant& value)
{
    if (!text(value, 20) || value.toString().at(0) == QLatin1Char('0')) return false;
    const QString raw = value.toString();
    for (QChar ch : raw) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = raw.toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == raw;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    if (value.type() != QVariant::Int && value.type() != QVariant::UInt && value.type() != QVariant::LongLong
        && value.type() != QVariant::ULongLong && value.type() != QVariant::Double) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<qint64>(number);
    return true;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QByteArray json(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

bool documentValid(const QVariantMap& document)
{
    if (!exact(document, {QStringLiteral("schemaVersion"), QStringLiteral("documentType"),
            QStringLiteral("sourceInstanceEpoch"), QStringLiteral("sourceRevision"), QStringLiteral("capturedAtUtcMs"),
            QStringLiteral("reports"), QStringLiteral("historicalOnly"), QStringLiteral("authenticated"),
            QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !integer(document.value(QStringLiteral("schemaVersion")), 1, 1)
        || !text(document.value(QStringLiteral("documentType")), 64)
        || document.value(QStringLiteral("documentType")).toString() != QStringLiteral("merivus-fleet-gnss-memory")
        || !uuid(document.value(QStringLiteral("sourceInstanceEpoch"))) || !counter(document.value(QStringLiteral("sourceRevision")))
        || !integer(document.value(QStringLiteral("capturedAtUtcMs")), 1, MaximumExactInteger)
        || document.value(QStringLiteral("reports")).type() != QVariant::List
        || document.value(QStringLiteral("reports")).isNull() || !flag(document.value(QStringLiteral("historicalOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!flag(document.value(key), false)) return false;
    // Shared exact33/7/3/18 validation and bounded whole-collection duplicate
    // checks. A valid value tree is still not proof of original native capture.
    return FleetTelemetryGnssReview::reportsValid(document.value(QStringLiteral("reports")).toList());
}

QVariantMap makeCache(const QByteArray& bytes, const QString& snapshotId, const QString& origin,
    QString* error, const std::atomic_bool& cancelled)
{
    if (cancelled.load()) { *error = QStringLiteral("gnss-memory-workspace-cancelled"); return {}; }
    if (bytes.isEmpty() || bytes.size() > MaximumBytes || QString::fromUtf8(bytes).toUtf8() != bytes
        || !uuid(QVariant(snapshotId)) || (origin != QStringLiteral("session") && origin != QStringLiteral("file"))) {
        *error = QStringLiteral("gnss-memory-workspace-document-budget-or-identity"); return {};
    }
    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
        *error = QStringLiteral("gnss-memory-workspace-document-invalid"); return {};
    }
    const QVariantMap document = parsed.object().toVariantMap();
    if (!documentValid(document)) { *error = QStringLiteral("gnss-memory-workspace-document-invalid"); return {}; }
    if (cancelled.load()) { *error = QStringLiteral("gnss-memory-workspace-cancelled"); return {}; }
    const QVariantList reports = document.value(QStringLiteral("reports")).toList();
    int samples = 0, located = 0;
    for (const QVariant& value : reports) {
        const QVariantMap report = value.toMap();
        samples += report.value(QStringLiteral("gnssSampleCount")).toInt();
        located += report.value(QStringLiteral("locatedSampleCount")).toInt();
    }
    const QString contentHash = hash(bytes);
    const QVariantMap descriptor{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("snapshotId"), snapshotId},
        {QStringLiteral("origin"), origin},
        {QStringLiteral("sourceInstanceEpoch"), origin == QStringLiteral("session")
            ? document.value(QStringLiteral("sourceInstanceEpoch")) : QVariant(QStringLiteral(""))},
        {QStringLiteral("sourceRevision"), origin == QStringLiteral("session")
            ? document.value(QStringLiteral("sourceRevision")) : QVariant(QStringLiteral("0"))},
        {QStringLiteral("capturedAtUtcMs"), document.value(QStringLiteral("capturedAtUtcMs"))},
        {QStringLiteral("reportCount"), reports.size()}, {QStringLiteral("sampleCount"), samples},
        {QStringLiteral("locatedSampleCount"), located}, {QStringLiteral("contentHash"), contentHash},
        {QStringLiteral("sourceFileHash"), origin == QStringLiteral("file") ? contentHash : QStringLiteral("")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    return {{QStringLiteral("descriptor"), descriptor}, {QStringLiteral("bytes"), bytes}, {QStringLiteral("reports"), reports}};
}

bool cacheValid(const QVariantMap& cache, const std::atomic_bool& cancelled)
{
    if (!exact(cache, {QStringLiteral("descriptor"), QStringLiteral("bytes"), QStringLiteral("reports")})
        || cache.value(QStringLiteral("descriptor")).type() != QVariant::Map
        || cache.value(QStringLiteral("descriptor")).isNull() || cache.value(QStringLiteral("bytes")).type() != QVariant::ByteArray
        || cache.value(QStringLiteral("bytes")).isNull() || cache.value(QStringLiteral("reports")).type() != QVariant::List
        || cache.value(QStringLiteral("reports")).isNull()) return false;
    const QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
    if (descriptor.size() != 17 || !uuid(descriptor.value(QStringLiteral("snapshotId")))
        || !digest(descriptor.value(QStringLiteral("contentHash"))) || !flag(descriptor.value(QStringLiteral("sourceCurrent")), false)
        || !text(descriptor.value(QStringLiteral("origin")), 7)) return false;
    QString error;
    // Rebuild in this worker from the actual cached bytes, including their
    // exact hash, so stale/tampered cache fields cannot lend report contents.
    const QVariantMap rebuilt = makeCache(cache.value(QStringLiteral("bytes")).toByteArray(),
        descriptor.value(QStringLiteral("snapshotId")).toString(), descriptor.value(QStringLiteral("origin")).toString(), &error, cancelled);
    return !rebuilt.isEmpty() && rebuilt == cache;
}

QVariantMap reportRow(const QVariantMap& report, int index)
{
    QVariantMap row{{QStringLiteral("reportIndex"), index}, {QStringLiteral("historicalOnly"), true}};
    for (const QString& key : {QStringLiteral("sourceIdentity"), QStringLiteral("rowCount"), QStringLiteral("gnssSampleCount"),
            QStringLiteral("locatedSampleCount"), QStringLiteral("unlocatedSampleCount"), QStringLiteral("qualityCautionSampleCount"),
            QStringLiteral("qualityUnknownSampleCount"), QStringLiteral("withinThresholdSampleCount"), QStringLiteral("groupCount")})
        row.insert(key, report.value(key));
    return row;
}
} // namespace

FleetGnssMemoryWorkspaceWorker::FleetGnssMemoryWorkspaceWorker(const QString& jobId, const QString& action,
    const QVariantMap& request, const QVariantMap& privateSnapshot, const QVariantList& freezeReports)
    : _jobId(jobId), _action(action), _request(request), _snapshot(privateSnapshot), _reports(freezeReports)
{
}

void FleetGnssMemoryWorkspaceWorker::requestCancel()
{
    _cancelled.store(true);
}

void FleetGnssMemoryWorkspaceWorker::run()
{
    QVariantMap result{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("gnss-memory-workspace-request-invalid")},
        {QStringLiteral("action"), _action}, {QStringLiteral("cache"), QVariantMap()}, {QStringLiteral("reply"), QVariantMap()}};
    const auto finish = [this, &result](const QString& error) {
        result[QStringLiteral("error")] = error;
        emit resultReady(_jobId, result);
    };
    if (!uuid(QVariant(_jobId))) { finish(QStringLiteral("gnss-memory-workspace-job-invalid")); return; }
    if (_cancelled.load()) { finish(QStringLiteral("gnss-memory-workspace-cancelled")); return; }
    if (_action == QStringLiteral("freeze")) {
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("sourceInstanceEpoch"),
                QStringLiteral("sourceRevision"), QStringLiteral("capturedAtUtcMs")})
            || !uuid(_request.value(QStringLiteral("snapshotId"))) || !uuid(_request.value(QStringLiteral("sourceInstanceEpoch")))
            || !counter(_request.value(QStringLiteral("sourceRevision")))
            || !integer(_request.value(QStringLiteral("capturedAtUtcMs")), 1, MaximumExactInteger)
            || _reports.size() > MaximumReports || !FleetTelemetryGnssReview::reportsValid(_reports)) {
            finish(QStringLiteral("gnss-memory-workspace-freeze-invalid")); return;
        }
        QVariantMap document{{QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("documentType"), QStringLiteral("merivus-fleet-gnss-memory")},
            {QStringLiteral("sourceInstanceEpoch"), _request.value(QStringLiteral("sourceInstanceEpoch"))},
            {QStringLiteral("sourceRevision"), _request.value(QStringLiteral("sourceRevision"))},
            {QStringLiteral("capturedAtUtcMs"), _request.value(QStringLiteral("capturedAtUtcMs"))},
            {QStringLiteral("reports"), _reports}, {QStringLiteral("historicalOnly"), true},
            {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
            {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
        if (_cancelled.load()) { finish(QStringLiteral("gnss-memory-workspace-cancelled")); return; }
        QString error;
        const QVariantMap cache = makeCache(json(document), _request.value(QStringLiteral("snapshotId")).toString(),
            QStringLiteral("session"), &error, _cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        result[QStringLiteral("cache")] = cache;
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("descriptor"), cache.value(QStringLiteral("descriptor"))}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (_action == QStringLiteral("open")) {
        if (!exact(_request, {QStringLiteral("path"), QStringLiteral("snapshotId")})
            || !FleetSnapshotFileIo::localJsonPath(_request.value(QStringLiteral("path"))) || !uuid(_request.value(QStringLiteral("snapshotId")))) {
            finish(QStringLiteral("gnss-memory-workspace-open-selection-invalid")); return;
        }
        QByteArray bytes;
        QString error;
        if (!FleetSnapshotFileIo::readUtf8(_request.value(QStringLiteral("path")).toString(), MaximumBytes, _cancelled,
                QStringLiteral("gnss-memory-workspace"), &bytes, &error)) { finish(error); return; }
        const QVariantMap cache = makeCache(bytes, _request.value(QStringLiteral("snapshotId")).toString(), QStringLiteral("file"), &error, _cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        result[QStringLiteral("cache")] = cache;
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("descriptor"), cache.value(QStringLiteral("descriptor"))}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (!uuid(_request.value(QStringLiteral("snapshotId"))) || !cacheValid(_snapshot, _cancelled) || _request.value(QStringLiteral("snapshotId"))
        != _snapshot.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId"))) {
        finish(_cancelled.load() ? QStringLiteral("gnss-memory-workspace-cancelled")
            : QStringLiteral("gnss-memory-workspace-snapshot-unavailable")); return;
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
            || !FleetSnapshotFileIo::localJsonPath(_request.value(QStringLiteral("path"))) || !digest(_request.value(QStringLiteral("contentHash")))
            || _request.value(QStringLiteral("contentHash")) != descriptor.value(QStringLiteral("contentHash"))
            || !flag(_request.value(QStringLiteral("overwriteConfirmed")), true)
            || descriptor.value(QStringLiteral("origin")).toString() != QStringLiteral("session")) {
            finish(QStringLiteral("gnss-memory-workspace-save-selection-invalid")); return;
        }
        const FleetSnapshotFileIo::SaveResult saved = FleetSnapshotFileIo::saveAndReadback(
            _request.value(QStringLiteral("path")).toString(), _snapshot.value(QStringLiteral("bytes")).toByteArray(),
            MaximumBytes, _cancelled, QStringLiteral("gnss-memory-workspace"));
        reply[QStringLiteral("fileWriteAttempted")] = saved.fileWriteAttempted;
        reply[QStringLiteral("fileWrittenKnown")] = saved.fileWrittenKnown;
        reply[QStringLiteral("fileWritten")] = saved.fileWritten;
        reply[QStringLiteral("readbackVerified")] = saved.readbackVerified;
        reply[QStringLiteral("readbackError")] = saved.readbackError;
        result[QStringLiteral("reply")] = reply; result[QStringLiteral("ok")] = saved.ok;
        finish(saved.error); return;
    }
    const QVariantList reports = _snapshot.value(QStringLiteral("reports")).toList();
    if (_action == QStringLiteral("reports")) {
        qint64 after = 0;
        if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("afterIndex")})
            || !integer(_request.value(QStringLiteral("afterIndex")), 0, reports.size(), &after)) {
            finish(QStringLiteral("gnss-memory-workspace-page-invalid")); return;
        }
        QVariantList rows;
        const int next = qMin(reports.size(), static_cast<int>(after) + PageSize);
        for (int i = static_cast<int>(after); i < next; ++i) {
            if (_cancelled.load()) { finish(QStringLiteral("gnss-memory-workspace-cancelled")); return; }
            rows.append(reportRow(reports.at(i).toMap(), i));
        }
        result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("snapshotId"), snapshotId}, {QStringLiteral("afterIndex"), after},
            {QStringLiteral("nextIndex"), next}, {QStringLiteral("totalCount"), reports.size()},
            {QStringLiteral("hasMore"), next < reports.size()}, {QStringLiteral("rows"), rows}};
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (_action != QStringLiteral("report")) { finish(QStringLiteral("gnss-memory-workspace-action-invalid")); return; }
    qint64 index = -1;
    if (!exact(_request, {QStringLiteral("snapshotId"), QStringLiteral("reportIndex")})
        || !integer(_request.value(QStringLiteral("reportIndex")), 0, reports.size() - 1, &index)) {
        finish(QStringLiteral("gnss-memory-workspace-report-invalid")); return;
    }
    if (_cancelled.load()) { finish(QStringLiteral("gnss-memory-workspace-cancelled")); return; }
    result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("snapshotId"), snapshotId},
        {QStringLiteral("reportIndex"), static_cast<int>(index)}, {QStringLiteral("report"), reports.at(static_cast<int>(index))}};
    result[QStringLiteral("ok")] = true; finish(QStringLiteral(""));
}

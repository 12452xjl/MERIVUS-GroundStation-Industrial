#include "FleetUserTaskTemplates.h"
#include "FleetMissionDraft.h"
#include "FleetSnapshotFileIo.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaType>
#include <QMap>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024LL * 1024LL;
constexpr int MaximumTemplates = 32;
constexpr int MaximumVersions = 64;
constexpr double MaximumExactNumber = 9007199254740991.0;
constexpr double MaximumSourceRevision = 9007199254740992.0;

QString text(const QString& value) { return value.isNull() ? QStringLiteral("") : value; }
QString uuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
bool uuidValid(const QString& value)
{
    const QUuid parsed(value);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value;
}
bool decimal(const QString& value, quint64* output = nullptr)
{
    if (value.isEmpty() || value.size() > 20 || value.at(0) == QLatin1Char('0')) return false;
    for (QChar c : value) if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = value.toULongLong(&ok);
    if (!ok || number == 0 || QString::number(number) != value) return false;
    if (output) *output = number;
    return true;
}
bool number(const QVariant& value, double* output)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::Double: break;
    case QVariant::LongLong:
        if (value.toLongLong() < 0 || value.toLongLong() > Q_INT64_C(9007199254740992)) return false;
        break;
    case QVariant::ULongLong:
        if (value.toULongLong() > Q_UINT64_C(9007199254740992)) return false;
        break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < 0.0 || candidate > MaximumSourceRevision) return false;
    if (output) *output = candidate;
    return true;
}
bool keys(const QVariantMap& map, const QSet<QString>& fields)
{
    if (map.size() != fields.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    return true;
}
bool stringValue(const QVariant& value, int limit, bool allowEmpty)
{
    return value.type() == QVariant::String && !value.isNull()
        && value.toString().size() <= limit && (allowEmpty || !value.toString().trimmed().isEmpty())
        && !value.toString().contains(QChar(0)) && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
}
bool metadataValid(const QVariantMap& metadata)
{
    return keys(metadata, {QStringLiteral("name"), QStringLiteral("category"), QStringLiteral("description")})
        && stringValue(metadata.value(QStringLiteral("name")), 128, false)
        && stringValue(metadata.value(QStringLiteral("category")), 128, true)
        && stringValue(metadata.value(QStringLiteral("description")), 4096, true);
}
// Bound the primitive tree before whole-library JSON conversion. Canonical
// draft validation supplies the semantic checks; this is only resource work.
bool bounded(const QVariant& value, int depth, int* nodes, qint64* units,
    const std::atomic_bool* cancelled = nullptr)
{
    if ((cancelled && cancelled->load()) || depth > 24 || ++*nodes > 1048576 || ++*units > MaximumBytes) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.isNull()) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            *units += it.key().size();
            if (*units > MaximumBytes || !bounded(it.value(), depth + 1, nodes, units, cancelled)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > 32768) return false;
        for (const QVariant& child : list) if (!bounded(child, depth + 1, nodes, units, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (!stringValue(value, static_cast<int>(MaximumBytes), true)) return false;
        *units += value.toString().size();
        return *units <= MaximumBytes;
    }
    if (value.type() == QVariant::Bool) return true;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        return std::isfinite(value.toDouble());
    default: return false;
    }
}
QVariantMap document(const QVariantList& records)
{
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("documentType"), QStringLiteral("merivus-user-task-templates")},
        {QStringLiteral("records"), records}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
qint64 encodedSize(qint64 recordsBytes, int count)
{
    const qint64 overhead = QJsonDocument::fromVariant(document({})).toJson(QJsonDocument::Compact).size();
    return overhead + recordsBytes + qMax(0, count - 1);
}
QVariantMap row(const QVariantMap& record)
{
    const QVariantMap source = record.value(QStringLiteral("editingSource")).toMap();
    return {{QStringLiteral("templateId"), record.value(QStringLiteral("templateId"))},
        {QStringLiteral("version"), record.value(QStringLiteral("version"))},
        {QStringLiteral("name"), record.value(QStringLiteral("name"))},
        {QStringLiteral("category"), record.value(QStringLiteral("category"))},
        {QStringLiteral("description"), record.value(QStringLiteral("description"))},
        {QStringLiteral("createdAtUtcMs"), record.value(QStringLiteral("createdAtUtcMs"))},
        {QStringLiteral("sourceRevision"), source.value(QStringLiteral("sourceRevision"))},
        {QStringLiteral("memberCount"), source.value(QStringLiteral("members")).toList().size()},
        {QStringLiteral("vertexCount"), source.value(QStringLiteral("vertices")).toList().size()}};
}
QVariantMap receipt(const QString& action, bool accepted, const QString& jobId, const QString& reason,
    bool committed, const QString& id, const QString& version, const QString& libraryRevision)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), text(action)},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("jobId"), text(jobId)},
        {QStringLiteral("reason"), text(reason)}, {QStringLiteral("committed"), committed},
        {QStringLiteral("templateId"), text(id)}, {QStringLiteral("version"), text(version)},
        {QStringLiteral("libraryRevision"), libraryRevision}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap operationResult(const QString& action, const QString& jobId, bool ok,
    const QString& error, const QVariantMap& reply, bool libraryCommitted)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("jobId"), jobId}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), text(error)},
        {QStringLiteral("reply"), reply}, {QStringLiteral("libraryCommitted"), libraryCommitted},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
bool validateDocument(const QVariantMap& input, QVariantList* output, qint64* bytes, int* recordNodes,
    const std::atomic_bool& cancelled, QString* error)
{
    const QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("documentType"), QStringLiteral("records"),
        QStringLiteral("historicalOnly"), QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")};
    double schema = 0.0;
    if (!keys(input, fields) || !number(input.value(QStringLiteral("schemaVersion")), &schema) || schema != 1.0
        || input.value(QStringLiteral("documentType")) != QVariant(QStringLiteral("merivus-user-task-templates"))
        || input.value(QStringLiteral("records")).type() != QVariant::List || input.value(QStringLiteral("records")).isNull()) {
        *error = QStringLiteral("user-template-file-schema-invalid"); return false;
    }
    for (const QString& key : {QStringLiteral("historicalOnly"), QStringLiteral("authenticated"),
            QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")}) {
        if (input.value(key).type() != QVariant::Bool || input.value(key).isNull()
            || input.value(key).toBool() != (key == QStringLiteral("historicalOnly"))) {
            *error = QStringLiteral("user-template-file-flags-invalid"); return false;
        }
    }
    int nodes = 0;
    qint64 units = 0;
    if (!bounded(input, 0, &nodes, &units, &cancelled)) {
        *error = QStringLiteral("user-template-file-budget-or-cancelled"); return false;
    }
    const QVariantList values = input.value(QStringLiteral("records")).toList();
    if (values.size() > MaximumVersions) { *error = QStringLiteral("user-template-version-limit"); return false; }
    const QSet<QString> recordFields{QStringLiteral("templateId"), QStringLiteral("version"), QStringLiteral("name"),
        QStringLiteral("category"), QStringLiteral("description"), QStringLiteral("createdAtUtcMs"), QStringLiteral("editingSource")};
    QHash<QString, quint64> latest;
    QVariantList candidate;
    qint64 recordBytes = 0;
    for (const QVariant& value : values) {
        if (cancelled.load()) { *error = QStringLiteral("user-template-cancelled"); return false; }
        if (value.type() != QVariant::Map || value.isNull()) { *error = QStringLiteral("user-template-record-invalid"); return false; }
        const QVariantMap record = value.toMap();
        const QString id = record.value(QStringLiteral("templateId")).toString();
        quint64 version = 0;
        double created = 0.0;
        const QVariantMap metadata{{QStringLiteral("name"), record.value(QStringLiteral("name"))},
            {QStringLiteral("category"), record.value(QStringLiteral("category"))},
            {QStringLiteral("description"), record.value(QStringLiteral("description"))}};
        if (!keys(record, recordFields) || record.value(QStringLiteral("templateId")).type() != QVariant::String || !uuidValid(id)
            || record.value(QStringLiteral("version")).type() != QVariant::String
            || !decimal(record.value(QStringLiteral("version")).toString(), &version)
            || latest.value(id, 0) == std::numeric_limits<quint64>::max() || version != latest.value(id, 0) + 1
            || !metadataValid(metadata) || !number(record.value(QStringLiteral("createdAtUtcMs")), &created)
            || created < 1.0 || created > MaximumExactNumber || std::floor(created) != created
            || record.value(QStringLiteral("editingSource")).type() != QVariant::Map || record.value(QStringLiteral("editingSource")).isNull()) {
            *error = QStringLiteral("user-template-record-invalid"); return false;
        }
        const QVariantMap checked = FleetMissionDraft::validateEditingSource(record.value(QStringLiteral("editingSource")).toMap());
        if (cancelled.load()) { *error = QStringLiteral("user-template-cancelled"); return false; }
        if (checked.value(QStringLiteral("ok")) != QVariant(true)
            || QJsonDocument::fromVariant(checked.value(QStringLiteral("source"))).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(record.value(QStringLiteral("editingSource"))).toJson(QJsonDocument::Compact)) {
            *error = QStringLiteral("user-template-editing-source-invalid"); return false;
        }
        latest.insert(id, version);
        if (latest.size() > MaximumTemplates) { *error = QStringLiteral("user-template-count-limit"); return false; }
        recordBytes += QJsonDocument::fromVariant(record).toJson(QJsonDocument::Compact).size();
        if (encodedSize(recordBytes, candidate.size() + 1) > MaximumBytes) {
            *error = QStringLiteral("user-template-file-budget"); return false;
        }
        candidate.append(record);
    }
    *output = candidate;
    *bytes = recordBytes;
    *recordNodes = nodes - 9; // Root map + its eight immediate fields.
    return true;
}
} // namespace

// No Q_OBJECT and no live QObject input. Results are observed only after the
// actual QThread finished edge; independent cleanup survives workspace death.
class FleetUserTaskTemplatesWorker final : public QThread
{
public:
    FleetUserTaskTemplatesWorker(const QString& actionValue, const QString& pathValue,
        const QString& idValue, const QString& expectedRevision, const QVariantList& frozen)
        : action(actionValue), path(pathValue), jobId(idValue), expectedLibraryRevision(expectedRevision), records(frozen) {}
    void cancel() { cancelled.store(true); }
    const QString action, path, jobId, expectedLibraryRevision;
    QVariantList records;
    qint64 recordBytes = 0;
    int recordNodes = 0;
    bool ok = false;
    QString error = QStringLiteral("");
    QVariantMap reply;
    bool completed = false;
    std::atomic_bool cancelled{false};
protected:
    void run() override
    {
        const auto finish = [this]() { completed = true; };
        if (action == QStringLiteral("open")) {
            QByteArray bytes;
            if (!FleetSnapshotFileIo::readUtf8(path, MaximumBytes, cancelled, QStringLiteral("user-template"), &bytes, &error)) { finish(); return; }
            QJsonParseError parseError;
            const QJsonDocument parsed = QJsonDocument::fromJson(bytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
                error = QStringLiteral("user-template-json-invalid"); finish(); return;
            }
            ok = validateDocument(parsed.toVariant().toMap(), &records, &recordBytes, &recordNodes, cancelled, &error);
            finish(); return;
        }
        const QVariantMap snapshot = document(records);
        QVariantList checked;
        if (!validateDocument(snapshot, &checked, &recordBytes, &recordNodes, cancelled, &error)) { finish(); return; }
        const QByteArray bytes = QJsonDocument::fromVariant(snapshot).toJson(QJsonDocument::Compact);
        const auto saved = FleetSnapshotFileIo::saveAndReadback(path, bytes, MaximumBytes, cancelled, QStringLiteral("user-template"));
        ok = saved.ok; error = saved.error;
        reply = {{QStringLiteral("snapshotId"), jobId},
            {QStringLiteral("contentHash"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())},
            {QStringLiteral("path"), path}, {QStringLiteral("fileWriteAttempted"), saved.fileWriteAttempted},
            {QStringLiteral("fileWrittenKnown"), saved.fileWrittenKnown}, {QStringLiteral("fileWritten"), saved.fileWritten},
            {QStringLiteral("readbackVerified"), saved.readbackVerified}, {QStringLiteral("readbackError"), text(saved.readbackError)},
            {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}, {QStringLiteral("authenticated"), false}};
        finish();
    }
};

FleetUserTaskTemplates::FleetUserTaskTemplates(QObject* parent) : QObject(parent), _instanceEpoch(uuid()) {}
FleetUserTaskTemplates::~FleetUserTaskTemplates()
{
    _destroying = true;
    if (_worker) _worker->cancel();
}
bool FleetUserTaskTemplates::available() const { return QThread::currentThread() == thread() && !_destroying; }
int FleetUserTaskTemplates::latestIndex(const QString& templateId) const
{
    for (int index = _records.size() - 1; index >= 0; --index)
        if (_records.at(index).toMap().value(QStringLiteral("templateId")) == QVariant(templateId)) return index;
    return -1;
}
int FleetUserTaskTemplates::recordIndex(const QString& templateId, const QString& version) const
{
    for (int index = 0; index < _records.size(); ++index) {
        const QVariantMap record = _records.at(index).toMap();
        if (record.value(QStringLiteral("templateId")) == QVariant(templateId)
            && record.value(QStringLiteral("version")) == QVariant(version)) return index;
    }
    return -1;
}
QVariantMap FleetUserTaskTemplates::state() const
{
    if (!available()) return {};
    QSet<QString> ids;
    for (const QVariant& value : _records) ids.insert(value.toMap().value(QStringLiteral("templateId")).toString());
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("instanceEpoch"), _instanceEpoch},
        {QStringLiteral("libraryRevision"), QString::number(_libraryRevision)}, {QStringLiteral("templateCount"), ids.size()},
        {QStringLiteral("versionCount"), _records.size()}, {QStringLiteral("busy"), _busy},
        {QStringLiteral("jobId"), _jobId}, {QStringLiteral("action"), _action}, {QStringLiteral("stage"), _stage},
        {QStringLiteral("cancelRequested"), _cancelRequested}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QVariantList FleetUserTaskTemplates::rows() const
{
    if (!available()) return {};
    QMap<QString, QVariantMap> latest;
    for (const QVariant& value : _records) {
        const QVariantMap record = value.toMap();
        latest.insert(record.value(QStringLiteral("templateId")).toString(), record);
    }
    QVariantList output;
    for (auto it = latest.constBegin(); it != latest.constEnd(); ++it) output.append(row(it.value()));
    return output;
}
QVariantList FleetUserTaskTemplates::versions(const QString& templateId) const
{
    if (!available() || !uuidValid(templateId)) return {};
    QVariantList output;
    for (const QVariant& value : _records)
        if (value.toMap().value(QStringLiteral("templateId")) == QVariant(templateId)) output.append(row(value.toMap()));
    return output;
}
QVariantMap FleetUserTaskTemplates::preview() const { return available() ? _preview : QVariantMap(); }
QVariantMap FleetUserTaskTemplates::result() const { return available() ? _result : QVariantMap(); }

QVariantMap FleetUserTaskTemplates::capture(FleetMissionDraft* actualDraft, const QVariant& expectedDraftRevision,
    const QString& templateId, const QString& expectedTemplateVersion, const QVariantMap& metadata,
    const QString& expectedLibraryRevision, bool confirmed)
{
    if (!available()) return receipt(QStringLiteral("capture"), false, QStringLiteral(""),
        QStringLiteral("user-template-capture-rejected"), false, QStringLiteral(""), QStringLiteral(""), QStringLiteral("0"));
    const QString revision = QString::number(_libraryRevision);
    const auto denied = [&]() { return receipt(QStringLiteral("capture"), false, _jobId,
        QStringLiteral("user-template-capture-rejected"), false, templateId, expectedTemplateVersion, revision); };
    if (!available() || _busy || _entryBusy || !confirmed || expectedLibraryRevision != revision
        || !actualDraft || actualDraft->thread() != thread() || !metadataValid(metadata)) return denied();
    double expected = 0.0;
    if (!number(expectedDraftRevision, &expected) || std::floor(expected) != expected || actualDraft->revision() != expected) return denied();
    const QPointer<FleetMissionDraft> draft(actualDraft);
    const QVariantMap source = actualDraft->planningSource();
    const QVariantMap checked = FleetMissionDraft::validateEditingSource(source);
    if (!draft || draft->revision() != expected || draft->planningSource() != source
        || checked.value(QStringLiteral("ok")) != QVariant(true)) return denied();
    return appendVersion(QStringLiteral("capture"), checked.value(QStringLiteral("source")).toMap(),
        templateId, expectedTemplateVersion, metadata);
}
QVariantMap FleetUserTaskTemplates::editMetadata(const QString& templateId, const QString& expectedTemplateVersion,
    const QVariantMap& metadata, const QString& expectedLibraryRevision, bool confirmed)
{
    if (!available()) return receipt(QStringLiteral("edit-metadata"), false, QStringLiteral(""),
        QStringLiteral("user-template-edit-rejected"), false, QStringLiteral(""), QStringLiteral(""), QStringLiteral("0"));
    const QString revision = QString::number(_libraryRevision);
    const int index = available() ? latestIndex(templateId) : -1;
    if (!available() || _busy || _entryBusy || !confirmed || expectedLibraryRevision != revision
        || index < 0 || !metadataValid(metadata)) return receipt(QStringLiteral("edit-metadata"), false, _jobId,
            QStringLiteral("user-template-edit-rejected"), false, templateId, expectedTemplateVersion, revision);
    return appendVersion(QStringLiteral("edit-metadata"), _records.at(index).toMap().value(QStringLiteral("editingSource")).toMap(),
        templateId, expectedTemplateVersion, metadata);
}
QVariantMap FleetUserTaskTemplates::appendVersion(const QString& action, const QVariantMap& source,
    const QString& templateId, const QString& expectedTemplateVersion, const QVariantMap& metadata)
{
    const QString revision = QString::number(_libraryRevision);
    const auto rejected = [&](const QString& reason) { return receipt(action, false, _jobId, reason, false,
        templateId, expectedTemplateVersion, revision); };
    if (_libraryRevision == std::numeric_limits<quint64>::max() || _records.size() >= MaximumVersions)
        return rejected(QStringLiteral("user-template-version-or-revision-limit"));
    const int index = latestIndex(templateId);
    QString id = templateId;
    quint64 version = 1;
    if (templateId.isEmpty()) {
        if (!expectedTemplateVersion.isEmpty() || rows().size() >= MaximumTemplates) return rejected(QStringLiteral("user-template-count-limit"));
        id = uuid();
        if (!uuidValid(id) || latestIndex(id) >= 0) return rejected(QStringLiteral("user-template-identity-unavailable"));
    } else if (!uuidValid(id) || index < 0
        || _records.at(index).toMap().value(QStringLiteral("version")) != QVariant(expectedTemplateVersion)
        || !decimal(expectedTemplateVersion, &version) || version == std::numeric_limits<quint64>::max()) {
        return rejected(QStringLiteral("user-template-version-changed"));
    } else ++version;
    const qint64 created = QDateTime::currentMSecsSinceEpoch();
    if (created < 1 || created > Q_INT64_C(9007199254740991))
        return rejected(QStringLiteral("user-template-local-clock-invalid"));
    const QVariantMap record{{QStringLiteral("templateId"), id}, {QStringLiteral("version"), QString::number(version)},
        {QStringLiteral("name"), metadata.value(QStringLiteral("name"))}, {QStringLiteral("category"), metadata.value(QStringLiteral("category"))},
        {QStringLiteral("description"), metadata.value(QStringLiteral("description"))},
        {QStringLiteral("createdAtUtcMs"), created}, {QStringLiteral("editingSource"), source}};
    int nodes = 0;
    qint64 units = 0;
    if (!bounded(record, 0, &nodes, &units) || nodes > 1048576 - 9 - _recordNodes)
        return rejected(QStringLiteral("user-template-source-budget"));
    const qint64 bytes = QJsonDocument::fromVariant(record).toJson(QJsonDocument::Compact).size();
    if (encodedSize(_recordBytes + bytes, _records.size() + 1) > MaximumBytes)
        return rejected(QStringLiteral("user-template-file-budget"));
    const QString idJob = uuid();
    if (!uuidValid(idJob) || idJob == _jobId) return rejected(QStringLiteral("user-template-job-unavailable"));
    const QPointer<FleetUserTaskTemplates> self(this);
    _entryBusy = true;
    _records.append(record); _recordBytes += bytes; _recordNodes += nodes; ++_libraryRevision;
    _preview.clear(); _previewSource.clear();
    _jobId = idJob; _action = action; _stage = QStringLiteral("complete"); _cancelRequested = false;
    const QVariantMap reply{{QStringLiteral("templateId"), id}, {QStringLiteral("version"), QString::number(version)},
        {QStringLiteral("libraryRevision"), QString::number(_libraryRevision)}};
    _result = operationResult(action, idJob, true, QStringLiteral(""), reply, true);
    const QVariantMap output = receipt(action, true, idJob, QStringLiteral(""), true, id,
        QString::number(version), QString::number(_libraryRevision));
    emit changed();
    if (self) self->_entryBusy = false;
    return output; // Frozen commit facts, never a later callback's state.
}

QVariantMap FleetUserTaskTemplates::previewTemplate(const QString& templateId, const QString& version)
{
    if (!available() || _busy || _entryBusy) return {};
    const int index = recordIndex(templateId, version);
    if (index < 0) return {};
    const QVariantMap record = _records.at(index).toMap();
    const QVariantMap small = row(record);
    const QString token = uuid();
    if (!uuidValid(token) || token == _preview.value(QStringLiteral("previewToken")).toString()) return {};
    QVariantMap output{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("previewToken"), token},
        {QStringLiteral("instanceEpoch"), _instanceEpoch}, {QStringLiteral("libraryRevision"), QString::number(_libraryRevision)},
        {QStringLiteral("templateId"), templateId}, {QStringLiteral("version"), version},
        {QStringLiteral("name"), small.value(QStringLiteral("name"))}, {QStringLiteral("category"), small.value(QStringLiteral("category"))},
        {QStringLiteral("description"), small.value(QStringLiteral("description"))},
        {QStringLiteral("sourceRevision"), small.value(QStringLiteral("sourceRevision"))},
        {QStringLiteral("memberCount"), small.value(QStringLiteral("memberCount"))}, {QStringLiteral("vertexCount"), small.value(QStringLiteral("vertexCount"))},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const QPointer<FleetUserTaskTemplates> self(this);
    _entryBusy = true;
    _preview = output; _previewSource = record.value(QStringLiteral("editingSource")).toMap();
    emit changed();
    if (self) self->_entryBusy = false;
    return output;
}
QVariantMap FleetUserTaskTemplates::restoreSource(const QString& previewToken) const
{
    const bool current = available() && !_busy && !_entryBusy && !previewToken.isEmpty()
        && _preview.value(QStringLiteral("previewToken")) == QVariant(previewToken)
        && _preview.value(QStringLiteral("instanceEpoch")) == QVariant(_instanceEpoch)
        && _preview.value(QStringLiteral("libraryRevision")) == QVariant(QString::number(_libraryRevision))
        && !_previewSource.isEmpty();
    return {{QStringLiteral("ok"), current}, {QStringLiteral("error"), current ? QStringLiteral("") : QStringLiteral("user-template-preview-stale")},
        {QStringLiteral("previewToken"), current ? previewToken : QStringLiteral("")},
        {QStringLiteral("templateId"), current ? _preview.value(QStringLiteral("templateId")) : QVariant(QStringLiteral(""))},
        {QStringLiteral("version"), current ? _preview.value(QStringLiteral("version")) : QVariant(QStringLiteral(""))},
        {QStringLiteral("source"), current ? _previewSource : QVariantMap()}, {QStringLiteral("manualConfirmationRequired"), true},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
bool FleetUserTaskTemplates::cancelPreview(const QString& previewToken)
{
    if (!available() || previewToken.isEmpty() || _preview.value(QStringLiteral("previewToken")) != QVariant(previewToken)) return false;
    _preview.clear(); _previewSource.clear(); emit changed(); return true;
}

QVariantMap FleetUserTaskTemplates::requestFile(const QString& action, const QString& path,
    const QString& expectedJobId, const QString& expectedLibraryRevision, bool overwriteConfirmed, bool confirmed)
{
    if (!available()) return receipt(action, false, QStringLiteral(""),
        QStringLiteral("user-template-file-request-rejected"), false, QStringLiteral(""), QStringLiteral(""), QStringLiteral("0"));
    const QString revision = QString::number(_libraryRevision);
    const auto denied = [&]() { return receipt(action, false, _jobId, QStringLiteral("user-template-file-request-rejected"),
        false, QStringLiteral(""), QStringLiteral(""), revision); };
    if (!available() || _busy || _entryBusy || !confirmed || expectedJobId != _jobId || expectedLibraryRevision != revision
        || (action != QStringLiteral("save") && action != QStringLiteral("open"))
        || (action == QStringLiteral("save") ? !overwriteConfirmed : overwriteConfirmed)
        || !FleetSnapshotFileIo::localJsonPath(path) || _libraryRevision == std::numeric_limits<quint64>::max()) return denied();
    const QString id = uuid();
    if (!uuidValid(id) || id == _jobId) return denied();
    const QPointer<FleetUserTaskTemplates> self(this);
    _entryBusy = true;
    const QPointer<FleetUserTaskTemplatesWorker> worker(new FleetUserTaskTemplatesWorker(action, path, id, revision,
        action == QStringLiteral("save") ? _records : QVariantList()));
    // No parent: workspace destruction requests cancellation, not a running
    // QThread deletion. The independent finished cleanup is installed too.
    connect(worker.data(), &QThread::finished, this, [self, worker, id]() {
        if (self && worker) self->finishFile(worker.data(), id);
    }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    _worker = worker; _jobId = id; _action = action; _stage = QStringLiteral("running");
    _busy = true; _cancelRequested = false; _preview.clear(); _previewSource.clear();
    // The original local generation is a typed private worker value, never
    // restored from file fields or a mutable QObject dynamic property.
    const QVariantMap output = receipt(action, true, id, QStringLiteral(""), false,
        QStringLiteral(""), QStringLiteral(""), revision);
    worker->start();
    emit changed();
    if (self) self->_entryBusy = false;
    return output;
}
bool FleetUserTaskTemplates::cancelFile(const QString& jobId)
{
    if (!available() || !_busy || !_worker || jobId.isEmpty() || jobId != _jobId) return false;
    if (_cancelRequested) return true;
    _cancelRequested = true; _stage = QStringLiteral("cancelling"); _worker->cancel(); emit changed(); return true;
}
void FleetUserTaskTemplates::finishFile(FleetUserTaskTemplatesWorker* worker, const QString& jobId)
{
    if (!available() || !_busy || _worker.data() != worker || _jobId != jobId || !worker->isFinished()
        || worker->jobId != jobId || worker->action != _action) return;
    const QPointer<FleetUserTaskTemplates> self(this);
    _entryBusy = true;
    bool ok = worker->completed && worker->ok;
    QString error = worker->completed ? worker->error : QStringLiteral("user-template-worker-result-unavailable");
    QVariantMap reply = worker->completed ? worker->reply : QVariantMap();
    bool committed = false;
    if (_action == QStringLiteral("open")) {
        if (_cancelRequested || worker->cancelled.load()) {
            ok = false; error = QStringLiteral("user-template-cancelled");
        } else if (ok) {
            if (worker->expectedLibraryRevision != QString::number(_libraryRevision)
                || _libraryRevision == std::numeric_limits<quint64>::max()) {
                ok = false; error = QStringLiteral("user-template-library-changed");
            } else {
                _records = worker->records; _recordBytes = worker->recordBytes; _recordNodes = worker->recordNodes;
                ++_libraryRevision; committed = true;
                QSet<QString> ids;
                for (const QVariant& value : _records) ids.insert(value.toMap().value(QStringLiteral("templateId")).toString());
                reply = {{QStringLiteral("templateCount"), ids.size()}, {QStringLiteral("versionCount"), _records.size()},
                    {QStringLiteral("libraryRevision"), QString::number(_libraryRevision)}};
            }
        }
    }
    _result = operationResult(_action, jobId, ok, error, reply, committed);
    _stage = ok ? QStringLiteral("complete") : (_cancelRequested || worker->cancelled.load())
        ? QStringLiteral("cancelled") : QStringLiteral("failed");
    _worker.clear(); _busy = false;
    emit changed();
    if (self) self->_entryBusy = false;
}

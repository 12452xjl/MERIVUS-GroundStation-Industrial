#include "FleetMissionDraftStore.h"
#include "FleetMissionDraft.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLockFile>
#include <QMetaType>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr int MaximumNodes = 1048576;
constexpr int MaximumDepth = 24;
constexpr double MaximumSourceRevision = 9007199254740992.0;

QString uuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool uuidValid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    const QUuid parsed(text);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == text;
}

bool revisionValue(const QVariant& value, double* revision)
{
    if (!value.isValid() || value.isNull()) return false;
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
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0.0 || number > MaximumSourceRevision
        || std::floor(number) != number) return false;
    if (revision) *revision = number;
    return true;
}

bool decimalRevision(const QVariant& value, quint64* revision)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (const QChar character : text)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    if (!ok || number == 0 || QString::number(number) != text) return false;
    if (revision) *revision = number;
    return true;
}

bool digestValid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool textValid(const QString& text, int maximum)
{
    return text.size() <= maximum && QString::fromUtf8(text.toUtf8()) == text
        && !text.contains(QChar(0));
}

bool bounded(const QVariant& value, int depth, int* nodes, qint64* scalarBytes)
{
    if (depth > MaximumDepth || ++(*nodes) > MaximumNodes) return false;
    // Do not accept a typed-null QObject/custom value as JSON null.
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!textValid(it.key(), 1024)) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > MaximumBytes || !bounded(it.value(), depth + 1, nodes, scalarBytes)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > 32768) return false;
        for (const QVariant& entry : list)
            if (!bounded(entry, depth + 1, nodes, scalarBytes)) return false;
        return true;
    }
    case QVariant::StringList: {
        const QStringList list = value.toStringList();
        if (list.size() > 32768) return false;
        for (const QString& entry : list)
            if (!bounded(entry, depth + 1, nodes, scalarBytes)) return false;
        return true;
    }
    case QVariant::String:
        if (!textValid(value.toString(), 32768)) return false;
        *scalarBytes += value.toString().toUtf8().size();
        return *scalarBytes <= MaximumBytes;
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong:
        return value.toLongLong() >= -Q_INT64_C(9007199254740992)
            && value.toLongLong() <= Q_INT64_C(9007199254740992);
    case QVariant::ULongLong: return value.toULongLong() <= Q_UINT64_C(9007199254740992);
    case QVariant::Double:
        return std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= MaximumSourceRevision;
    default: return false;
    }
}

bool encode(const QVariantMap& map, QByteArray* bytes)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!bounded(map, 0, &nodes, &scalarBytes)) return false;
    const QByteArray result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    if (result.isEmpty() || result.size() > MaximumBytes) return false;
    if (bytes) *bytes = result;
    return true;
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool canonicalSource(const QVariantMap& supplied, QVariantMap* source, QByteArray* sourceBytes)
{
    QByteArray suppliedBytes;
    if (!encode(supplied, &suppliedBytes)) return false;
    const QVariantMap checked = FleetMissionDraft::validateEditingSource(supplied);
    if (checked.size() != 4 || !flag(checked, QStringLiteral("ok"), true)
        || checked.value(QStringLiteral("source")).type() != QVariant::Map
        || checked.value(QStringLiteral("task")).type() != QVariant::Map
        || !checked.value(QStringLiteral("task")).toMap().isEmpty()
        || checked.value(QStringLiteral("error")).type() != QVariant::String
        || !checked.value(QStringLiteral("error")).toString().isEmpty()) return false;
    const QVariantMap canonical = checked.value(QStringLiteral("source")).toMap();
    QByteArray canonicalBytes;
    if (canonical.size() != 5 || !encode(canonical, &canonicalBytes)) return false;
    // A JSON round trip gives this owner independent containers/strings. It
    // is a value copy, not an authentication or a new runtime Draft identity.
    if (source) *source = QJsonDocument::fromJson(canonicalBytes).toVariant().toMap();
    if (sourceBytes) *sourceBytes = canonicalBytes;
    return true;
}

QVariantMap display(const QString& state = QStringLiteral("idle"),
                    const QString& reason = QStringLiteral(""),
                    const QString& kind = QStringLiteral("none"),
                    const QString& token = QStringLiteral(""),
                    const QVariantMap& source = QVariantMap(),
                    const QString& checkpointId = QStringLiteral(""),
                    const QString& checkpointRevision = QStringLiteral("0"),
                    bool overwrites = false)
{
    return {{QStringLiteral("state"), state}, {QStringLiteral("reason"), reason},
        {QStringLiteral("previewKind"), kind}, {QStringLiteral("previewToken"), token},
        {QStringLiteral("checkpointId"), checkpointId}, {QStringLiteral("checkpointRevision"), checkpointRevision},
        {QStringLiteral("sourceRevision"), source.isEmpty() ? QVariant(-1.0) : source.value(QStringLiteral("sourceRevision"))},
        {QStringLiteral("vertexCount"), source.value(QStringLiteral("vertices")).toList().size()},
        {QStringLiteral("memberCount"), source.value(QStringLiteral("members")).toList().size()},
        {QStringLiteral("hasClearanceInput"), source.value(QStringLiteral("settings")).toMap().contains(QStringLiteral("clearanceInput"))},
        {QStringLiteral("overwritesExisting"), overwrites}, {QStringLiteral("editingStateOnly"), true},
        {QStringLiteral("manualConfirmationRequired"), true}, {QStringLiteral("approvalRestored"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap reply(const QString& error, bool committed, bool current,
                  const QVariantMap& preview, const QVariantMap& source = QVariantMap())
{
    return {{QStringLiteral("ok"), error.isEmpty() && current}, {QStringLiteral("error"), error},
        {QStringLiteral("committed"), committed}, {QStringLiteral("current"), current},
        {QStringLiteral("preview"), preview}, {QStringLiteral("source"), source}};
}

QVariantMap rejected(const QString& error)
{
    return reply(error, false, false, display(QStringLiteral("failed"), error));
}

QVariantMap terminalDisplay(const QVariantMap& original, const QString& state, const QString& reason)
{
    QVariantMap result = original;
    result.insert(QStringLiteral("state"), state);
    result.insert(QStringLiteral("reason"), reason);
    result.insert(QStringLiteral("previewKind"), QStringLiteral("none"));
    result.insert(QStringLiteral("previewToken"), QStringLiteral(""));
    return result;
}

struct Saved {
    bool exists = false;
    QByteArray bytes;
    QVariantMap envelope;
    QVariantMap source;
    QByteArray sourceBytes;
    quint64 revision = 0;
};

bool directoryUsable(const QString& slotPath, bool* exists)
{
    const QFileInfo directory(QFileInfo(slotPath).absolutePath());
    if (directory.isSymLink()) return false;
    *exists = directory.exists();
    return !*exists || directory.isDir();
}

bool inspectBytes(const QByteArray& bytes, Saved* saved)
{
    if (bytes.isEmpty() || bytes.size() > MaximumBytes || QString::fromUtf8(bytes).toUtf8() != bytes) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QVariantMap envelope = document.toVariant().toMap();
    QByteArray normalized;
    // This format accepts only our complete compact canonical encoding. This
    // also rejects duplicate-key collapse, BOM, whitespace/tails and truncation.
    if (!encode(envelope, &normalized) || normalized != bytes) return false;
    const QSet<QString> keys{QStringLiteral("schemaVersion"), QStringLiteral("documentType"),
        QStringLiteral("checkpointId"), QStringLiteral("checkpointRevision"), QStringLiteral("savedAtUtc"),
        QStringLiteral("source"), QStringLiteral("sourceHash"), QStringLiteral("contentHash"),
        QStringLiteral("editingStateOnly"), QStringLiteral("manualConfirmationRequired"),
        QStringLiteral("approvalRestored"), QStringLiteral("flightCommandReleased")};
    if (envelope.size() != keys.size()) return false;
    for (auto it = envelope.constBegin(); it != envelope.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    double schema = 0.0;
    quint64 revision = 0;
    if (!revisionValue(envelope.value(QStringLiteral("schemaVersion")), &schema) || schema != 1.0
        || envelope.value(QStringLiteral("documentType")) != QVariant(QStringLiteral("merivus.fleet.editing-draft"))
        || !uuidValid(envelope.value(QStringLiteral("checkpointId")))
        || !decimalRevision(envelope.value(QStringLiteral("checkpointRevision")), &revision)
        || !flag(envelope, QStringLiteral("editingStateOnly"), true)
        || !flag(envelope, QStringLiteral("manualConfirmationRequired"), true)
        || !flag(envelope, QStringLiteral("approvalRestored"), false)
        || !flag(envelope, QStringLiteral("flightCommandReleased"), false)
        || !digestValid(envelope.value(QStringLiteral("sourceHash")))
        || !digestValid(envelope.value(QStringLiteral("contentHash")))
        || envelope.value(QStringLiteral("source")).type() != QVariant::Map
        || envelope.value(QStringLiteral("savedAtUtc")).type() != QVariant::String) return false;
    const QString time = envelope.value(QStringLiteral("savedAtUtc")).toString();
    const QDateTime parsedTime = QDateTime::fromString(time, Qt::ISODateWithMs);
    if (!parsedTime.isValid() || parsedTime.timeSpec() != Qt::UTC
        || parsedTime.toString(Qt::ISODateWithMs) != time) return false;
    QVariantMap source;
    QByteArray sourceBytes, suppliedSourceBytes;
    if (!canonicalSource(envelope.value(QStringLiteral("source")).toMap(), &source, &sourceBytes)
        || !encode(envelope.value(QStringLiteral("source")).toMap(), &suppliedSourceBytes)
        || sourceBytes != suppliedSourceBytes
        || hash(sourceBytes) != envelope.value(QStringLiteral("sourceHash")).toString()) return false;
    QVariantMap unhashed = envelope;
    unhashed.remove(QStringLiteral("contentHash"));
    QByteArray unhashedBytes;
    if (!encode(unhashed, &unhashedBytes)
        || hash(unhashedBytes) != envelope.value(QStringLiteral("contentHash")).toString()) return false;
    if (saved) {
        saved->exists = true;
        saved->bytes = bytes;
        saved->envelope = envelope;
        saved->source = source;
        saved->sourceBytes = sourceBytes;
        saved->revision = revision;
    }
    return true;
}

bool readSaved(const QString& slotPath, Saved* saved, QString* error)
{
    *saved = Saved();
    bool directoryExists = false;
    if (!directoryUsable(slotPath, &directoryExists)) {
        *error = QStringLiteral("draft-store-directory-rejected"); return false;
    }
    const QFileInfo before(slotPath);
    if (before.isSymLink() || (before.exists() && !before.isFile())) {
        *error = QStringLiteral("draft-store-file-type-rejected"); return false;
    }
    if (!before.exists()) return true;
    if (!directoryExists || before.size() <= 0 || before.size() > MaximumBytes) {
        *error = QStringLiteral("draft-store-file-budget-invalid"); return false;
    }
    QFile file(slotPath);
    if (!file.open(QIODevice::ReadOnly)) { *error = QStringLiteral("draft-store-read-failed"); return false; }
    const qint64 size = file.size();
    if (size <= 0 || size > MaximumBytes) { *error = QStringLiteral("draft-store-file-budget-invalid"); return false; }
    const QByteArray bytes = file.read(MaximumBytes + 1);
    const QFileInfo after(slotPath);
    if (file.error() != QFileDevice::NoError || bytes.size() != size || file.size() != size
        || !after.exists() || !after.isFile() || after.isSymLink() || after.size() != size) {
        *error = QStringLiteral("draft-store-read-failed"); return false;
    }
    if (!inspectBytes(bytes, saved)) { *error = QStringLiteral("draft-store-checkpoint-invalid"); return false; }
    return true;
}

bool draftMatches(FleetMissionDraft* draft, double revision, const QByteArray& sourceBytes)
{
    if (!draft || draft->thread() != QThread::currentThread() || draft->revision() != revision) return false;
    QVariantMap source;
    QByteArray bytes;
    return canonicalSource(draft->planningSource(), &source, &bytes) && bytes == sourceBytes;
}
} // namespace

FleetMissionDraftStore::FleetMissionDraftStore(QObject* parent)
    : QObject(parent), _current(display())
{
}

QVariantMap FleetMissionDraftStore::current() const
{
    return QThread::currentThread() == thread() ? _current
        : display(QStringLiteral("failed"), QStringLiteral("draft-store-wrong-thread"));
}

bool FleetMissionDraftStore::busy() const
{
    return QThread::currentThread() != thread() || _busy;
}

bool FleetMissionDraftStore::ensureLocation()
{
    if (!_slotPath.isEmpty()) return true;
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty() || !QFileInfo(base).isAbsolute() || !textValid(base, 4096)) return false;
    _slotPath = QDir(base).filePath(QStringLiteral("fleet-os/editing-draft-v1.json"));
    return true;
}

QString FleetMissionDraftStore::begin(const QString& state)
{
    const QString operationId = uuid();
    _operationId = operationId;
    _preview = Preview(); // Every new explicit operation retires the old token.
    _current = display(state);
    _busy = true;
    const QPointer<FleetMissionDraftStore> self(this);
    emit changed();
    return self && self->thread() == QThread::currentThread() ? operationId : QString();
}

QVariantMap FleetMissionDraftStore::finish(const QString& operationId, const QVariantMap& published,
    const QString& error, bool committed, const QVariantMap& source,
    FleetMissionDraft* expectedDraft, double expectedRevision, const QByteArray& expectedSourceBytes)
{
    const QPointer<FleetMissionDraftStore> self(this);
    const QPointer<FleetMissionDraft> draft(expectedDraft);
    _current = published;
    _busy = false;
    emit changed();
    const bool stillCurrent = error.isEmpty() && self && self->thread() == QThread::currentThread()
        && self->_operationId == operationId
        && self->_current == published
        && (expectedRevision < 0.0 || (draft && draftMatches(draft.data(), expectedRevision, expectedSourceBytes)));
    const QString finalError = !error.isEmpty() ? error : stillCurrent ? QStringLiteral("")
        : QStringLiteral("draft-store-result-superseded");
    return reply(finalError, committed, stillCurrent, published, stillCurrent ? source : QVariantMap());
}

QVariantMap FleetMissionDraftStore::prepareSave(FleetMissionDraft* actualDraft, const QVariant& expectedRevision)
{
    if (QThread::currentThread() != thread()) return rejected(QStringLiteral("draft-store-wrong-thread"));
    if (_busy) return rejected(QStringLiteral("draft-store-busy"));
    // Freeze the caller's value before begin() notifies. A native caller may
    // retain a mutable QVariant lvalue and change it in a synchronous slot.
    double revision = 0.0;
    const bool expectedRevisionValid = revisionValue(expectedRevision, &revision);
    const QPointer<FleetMissionDraftStore> self(this);
    const QPointer<FleetMissionDraft> draft(actualDraft);
    const QString operationId = begin(QStringLiteral("preparing-save"));
    if (!self || operationId.isEmpty()) return rejected(QStringLiteral("draft-store-owner-changed"));
    const auto fail = [&](const QString& error) { return finish(operationId, display(QStringLiteral("failed"), error), error); };
    if (!draft || draft->thread() != thread() || !expectedRevisionValid
        || draft->revision() != revision) return fail(QStringLiteral("draft-store-draft-revision-mismatch"));
    Preview prepared;
    if (!canonicalSource(draft->planningSource(), &prepared.source, &prepared.sourceBytes))
        return fail(QStringLiteral("draft-store-editing-source-invalid"));
    if (!ensureLocation()) return fail(QStringLiteral("draft-store-location-unavailable"));
    Saved saved;
    QString error;
    if (!readSaved(_slotPath, &saved, &error)) return fail(error);
    if (saved.revision == std::numeric_limits<quint64>::max())
        return fail(QStringLiteral("draft-store-checkpoint-revision-exhausted"));
    if (!draftMatches(draft.data(), revision, prepared.sourceBytes))
        return fail(QStringLiteral("draft-store-draft-revision-mismatch"));
    prepared.kind = QStringLiteral("save");
    prepared.token = uuid();
    prepared.draft = draft;
    prepared.revision = revision;
    prepared.baselineExists = saved.exists;
    prepared.baselineBytes = saved.bytes;
    const QString checkpointId = saved.exists ? saved.envelope.value(QStringLiteral("checkpointId")).toString() : uuid();
    const QString checkpointRevision = QString::number(saved.revision + 1);
    QVariantMap envelope{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("documentType"), QStringLiteral("merivus.fleet.editing-draft")},
        {QStringLiteral("checkpointId"), checkpointId}, {QStringLiteral("checkpointRevision"), checkpointRevision},
        {QStringLiteral("savedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("source"), prepared.source}, {QStringLiteral("sourceHash"), hash(prepared.sourceBytes)},
        {QStringLiteral("editingStateOnly"), true}, {QStringLiteral("manualConfirmationRequired"), true},
        {QStringLiteral("approvalRestored"), false}, {QStringLiteral("flightCommandReleased"), false}};
    QByteArray unhashed;
    if (!encode(envelope, &unhashed)) return fail(QStringLiteral("draft-store-checkpoint-budget-exceeded"));
    envelope.insert(QStringLiteral("contentHash"), hash(unhashed));
    if (!encode(envelope, &prepared.saveBytes) || !inspectBytes(prepared.saveBytes, nullptr))
        return fail(QStringLiteral("draft-store-checkpoint-invalid"));
    prepared.display = display(QStringLiteral("save-preview"), QStringLiteral(""), prepared.kind,
        prepared.token, prepared.source, checkpointId, checkpointRevision, saved.exists);
    _preview = prepared;
    return finish(operationId, prepared.display, QStringLiteral(""), false, {}, draft.data(), revision, prepared.sourceBytes);
}

QVariantMap FleetMissionDraftStore::inspectSaved()
{
    if (QThread::currentThread() != thread()) return rejected(QStringLiteral("draft-store-wrong-thread"));
    if (_busy) return rejected(QStringLiteral("draft-store-busy"));
    const QPointer<FleetMissionDraftStore> self(this);
    const QString operationId = begin(QStringLiteral("inspecting-saved"));
    if (!self || operationId.isEmpty()) return rejected(QStringLiteral("draft-store-owner-changed"));
    const auto fail = [&](const QString& error) { return finish(operationId, display(QStringLiteral("failed"), error), error); };
    if (!ensureLocation()) return fail(QStringLiteral("draft-store-location-unavailable"));
    Saved saved;
    QString error;
    if (!readSaved(_slotPath, &saved, &error)) return fail(error);
    if (!saved.exists) return fail(QStringLiteral("draft-store-checkpoint-absent"));
    Preview prepared;
    prepared.kind = QStringLiteral("restore");
    prepared.token = uuid();
    prepared.source = saved.source;
    prepared.sourceBytes = saved.sourceBytes;
    prepared.baselineExists = true;
    prepared.baselineBytes = saved.bytes;
    prepared.display = display(QStringLiteral("restore-preview"), QStringLiteral(""), prepared.kind,
        prepared.token, saved.source, saved.envelope.value(QStringLiteral("checkpointId")).toString(),
        saved.envelope.value(QStringLiteral("checkpointRevision")).toString());
    _preview = prepared;
    return finish(operationId, prepared.display, QStringLiteral(""));
}

QVariantMap FleetMissionDraftStore::commitSave(const QString& previewToken, bool confirmed)
{
    if (QThread::currentThread() != thread()) return rejected(QStringLiteral("draft-store-wrong-thread"));
    if (_busy) return rejected(QStringLiteral("draft-store-busy"));
    if (!confirmed || !uuidValid(previewToken) || _preview.kind != QStringLiteral("save")
        || _preview.token != previewToken) return rejected(QStringLiteral("draft-store-save-preview-required"));
    const Preview prepared = _preview;
    const QPointer<FleetMissionDraftStore> self(this);
    const QString operationId = begin(QStringLiteral("saving")); // Consume before any attempted IO.
    if (!self || operationId.isEmpty()) return rejected(QStringLiteral("draft-store-owner-changed"));
    const auto fail = [&](const QString& error, bool committed = false) {
        return finish(operationId, terminalDisplay(prepared.display, QStringLiteral("failed"), error), error, committed);
    };
    if (!draftMatches(prepared.draft.data(), prepared.revision, prepared.sourceBytes))
        return fail(QStringLiteral("draft-store-draft-revision-mismatch"));
    if (!ensureLocation()) return fail(QStringLiteral("draft-store-location-unavailable"));
    bool directoryExists = false;
    if (!directoryUsable(_slotPath, &directoryExists)) return fail(QStringLiteral("draft-store-directory-rejected"));
    const QString directory = QFileInfo(_slotPath).absolutePath();
    if ((!directoryExists && !QDir().mkpath(directory))
        || !directoryUsable(_slotPath, &directoryExists) || !directoryExists)
        return fail(QStringLiteral("draft-store-directory-create-failed"));
    QLockFile lock(_slotPath + QStringLiteral(".lock"));
    // No waiting or forced stale-lock removal in this user-confirmed action.
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) return fail(QStringLiteral("draft-store-lock-unavailable"));
    Saved saved;
    QString error;
    if (!readSaved(_slotPath, &saved, &error)) return fail(error);
    if (saved.exists != prepared.baselineExists || saved.bytes != prepared.baselineBytes)
        return fail(QStringLiteral("draft-store-checkpoint-changed"));
    if (!draftMatches(prepared.draft.data(), prepared.revision, prepared.sourceBytes))
        return fail(QStringLiteral("draft-store-draft-revision-mismatch"));
    QSaveFile file(_slotPath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return fail(QStringLiteral("draft-store-write-open-failed"));
    if (file.write(prepared.saveBytes) != prepared.saveBytes.size()) {
        file.cancelWriting();
        return fail(QStringLiteral("draft-store-write-failed"));
    }
    if (!file.commit()) return fail(QStringLiteral("draft-store-commit-unconfirmed"));
    // committed is the actual QSaveFile success fact, not future currentness,
    // authenticated origin, power-loss durability or approval restoration.
    Saved readback;
    if (!readSaved(_slotPath, &readback, &error)) return fail(error, true);
    if (!readback.exists || readback.bytes != prepared.saveBytes)
        return fail(QStringLiteral("draft-store-readback-mismatch"), true);
    lock.unlock();
    return finish(operationId, terminalDisplay(prepared.display, QStringLiteral("saved"), QStringLiteral("")),
        QStringLiteral(""), true, {}, prepared.draft.data(), prepared.revision, prepared.sourceBytes);
}

QVariantMap FleetMissionDraftStore::restoreSource(const QString& restoreToken)
{
    if (QThread::currentThread() != thread()) return rejected(QStringLiteral("draft-store-wrong-thread"));
    if (_busy) return rejected(QStringLiteral("draft-store-busy"));
    if (!uuidValid(restoreToken) || _preview.kind != QStringLiteral("restore") || _preview.token != restoreToken)
        return rejected(QStringLiteral("draft-store-restore-preview-required"));
    const Preview prepared = _preview;
    const QPointer<FleetMissionDraftStore> self(this);
    const QString operationId = begin(QStringLiteral("reading-restore-source"));
    if (!self || operationId.isEmpty()) return rejected(QStringLiteral("draft-store-owner-changed"));
    const auto fail = [&](const QString& error) {
        return finish(operationId, terminalDisplay(prepared.display, QStringLiteral("failed"), error), error);
    };
    if (!ensureLocation()) return fail(QStringLiteral("draft-store-location-unavailable"));
    Saved saved;
    QString error;
    if (!readSaved(_slotPath, &saved, &error)) return fail(error);
    if (!saved.exists || !prepared.baselineExists || saved.bytes != prepared.baselineBytes
        || saved.sourceBytes != prepared.sourceBytes) return fail(QStringLiteral("draft-store-checkpoint-changed"));
    // This is a repeated read-only source getter for the same preview, not a
    // permission consumption. Every call still rereads the complete original
    // baseline; failure/new inspection/cancellation retires the preview.
    _preview = prepared;
    return finish(operationId, prepared.display, QStringLiteral(""), false, prepared.source);
}

bool FleetMissionDraftStore::cancelPreview(const QString& token)
{
    if (QThread::currentThread() != thread() || _busy || !uuidValid(token)
        || _preview.kind.isEmpty() || _preview.token != token) return false;
    const QString operationId = uuid();
    _operationId = operationId;
    const QVariantMap published = terminalDisplay(_preview.display, QStringLiteral("cancelled"), QStringLiteral(""));
    _preview = Preview();
    _current = published;
    const QPointer<FleetMissionDraftStore> self(this);
    emit changed();
    return self && self->thread() == QThread::currentThread() && self->_operationId == operationId
        && self->_current == published && self->_preview.token.isEmpty();
}

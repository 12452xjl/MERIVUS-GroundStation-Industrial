#include "FleetEventBlackBox.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QMetaType>
#include <QSaveFile>
#include <QVariantHash>
#include <QtMath>
#include <QPointer>
#include <QScopeGuard>
#include <QThread>
#include <QUuid>

#include <limits>
#include <cstring>

namespace {
constexpr qint64 kMaxPersistenceBytes = 32LL * 1024LL * 1024LL;
constexpr qint64 kFutureEventToleranceMs = 5LL * 60LL * 1000LL;
// Event payloads can originate from adapters or imported handoff files.  The
// journal is intentionally bounded per event as well as by the whole snapshot
// so a single malformed/deep QVariant tree cannot monopolize the UI thread or
// make retention depend on payload shape.
constexpr int kMaxEventTypeLength = 128;
constexpr int kMaxEventSourceLength = 256;
constexpr int kMaxPayloadDepth = 16;
constexpr int kMaxPayloadNodes = 50000;
constexpr qint64 kMaxPayloadBytes = 512LL * 1024LL;

bool payloadWithinLimits(const QVariant& value,
                         int depth,
                         int* nodeCount,
                         qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxPayloadDepth) return false;
    if (++(*nodeCount) > kMaxPayloadNodes) return false;
    if (!value.isValid() || value.isNull()) return true;

    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            *scalarBytes += static_cast<qint64>(it.key().size()) * 2;
            if (*scalarBytes > kMaxPayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    case QVariant::Hash: {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            *scalarBytes += static_cast<qint64>(it.key().size()) * 2;
            if (*scalarBytes > kMaxPayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& item : value.toList()) {
            if (!payloadWithinLimits(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    case QVariant::String:
        *scalarBytes += static_cast<qint64>(value.toString().size()) * 2;
        return *scalarBytes <= kMaxPayloadBytes;
    case QVariant::StringList:
        for (const QString& item : value.toStringList()) {
            *scalarBytes += static_cast<qint64>(item.size()) * 2;
            if (*scalarBytes > kMaxPayloadBytes) return false;
        }
        return true;
    case QVariant::ByteArray:
        *scalarBytes += value.toByteArray().size();
        return *scalarBytes <= kMaxPayloadBytes;
    default:
        // Numeric/bool/date values are bounded by QVariant itself.
        return true;
    }
}

bool payloadWithinLimits(const QVariantMap& payload)
{
    int nodeCount = 0;
    qint64 scalarBytes = 0;
    return payloadWithinLimits(QVariant(payload), 0, &nodeCount, &scalarBytes);
}

bool sameNativeValue(const QVariant& left, const QVariant& right, int depth, int* nodes)
{
    if (!nodes || depth > 18 || ++(*nodes) > 50016
        || left.userType() != right.userType() || left.isNull() != right.isNull()) return false;
    if (!left.isValid()) return !right.isValid();
    if (left.type() == QVariant::Map || left.type() == QVariant::Hash) {
        const QVariantMap a = left.type() == QVariant::Map ? left.toMap() : QVariantMap();
        const QVariantMap b = right.type() == QVariant::Map ? right.toMap() : QVariantMap();
        if (left.type() == QVariant::Hash) {
            const QVariantHash ah = left.toHash(), bh = right.toHash();
            if (ah.size() != bh.size()) return false;
            for (auto it = ah.constBegin(); it != ah.constEnd(); ++it) {
                if (!bh.contains(it.key()) || !sameNativeValue(it.value(), bh.value(it.key()), depth + 1, nodes)) return false;
            }
            return true;
        }
        if (a.size() != b.size()) return false;
        for (auto it = a.constBegin(); it != a.constEnd(); ++it) {
            if (!b.contains(it.key()) || !sameNativeValue(it.value(), b.value(it.key()), depth + 1, nodes)) return false;
        }
        return true;
    }
    if (left.type() == QVariant::List) {
        const QVariantList a = left.toList(), b = right.toList();
        if (a.size() != b.size()) return false;
        for (int i = 0; i < a.size(); ++i) {
            if (!sameNativeValue(a.at(i), b.at(i), depth + 1, nodes)) return false;
        }
        return true;
    }
    // Receipt identity and archive support are different checks. Known fixed
    // Qt scalars can match exactly but still be rejected by the JSON codec.
    // No custom comparators and no cross-type numeric aliases are admitted.
    if (left.userType() == QMetaType::Double)
        return std::memcmp(left.constData(), right.constData(), sizeof(double)) == 0;
    if (left.userType() == QMetaType::Float)
        return std::memcmp(left.constData(), right.constData(), sizeof(float)) == 0;
    switch (left.type()) {
    case QVariant::Bool: case QVariant::String: case QVariant::Int:
    case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong:
    case QVariant::StringList: case QVariant::ByteArray: case QVariant::Date:
    case QVariant::Time: case QVariant::DateTime: return left == right;
    default: return false;
    }
}
}

FleetEventBlackBox::FleetEventBlackBox(QObject* parent) : QObject(parent),
    _nativeSourceEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

QString FleetEventBlackBox::nativeSourceEpoch() const
{
    return QThread::currentThread() == thread() ? _nativeSourceEpoch : QString();
}
QString FleetEventBlackBox::nativeReceiptSequence() const
{
    return QThread::currentThread() == thread() ? QString::number(_nativeReceiptSequence) : QString();
}
bool FleetEventBlackBox::nativeEventReceiptMatches(const QString& epoch, const QString& sequence, const QVariantMap& exactEvent) const
{
    if (QThread::currentThread() != thread() || epoch != _nativeSourceEpoch || _nativeReceiptExhausted
        || sequence.isEmpty() || sequence.size() > 20) return false;
    bool valid = false;
    const quint64 value = sequence.toULongLong(&valid);
    int nodes = 0;
    return valid && value != 0 && QString::number(value) == sequence
        && _nativePublishingFrames.contains(value)
        && sameNativeValue(_nativePublishingFrames.value(value), exactEvent, 0, &nodes);
}

void FleetEventBlackBox::setMaxEvents(int maxEvents)
{
    if (QThread::currentThread() != thread()) return;
    const int bounded = qBound(100, maxEvents, 1000000);
    if (_maxEvents == bounded) return;
    _maxEvents = bounded;
    while (_events.size() > _maxEvents) _events.removeFirst();
    const QPointer<FleetEventBlackBox> self(this);
    emit maxEventsChanged();
    if (!self) return;
    emit eventsChanged();
}

void FleetEventBlackBox::startRecording() { if (QThread::currentThread() == thread() && !_recording) { _nativePublishingFrames.clear(); _recording = true; emit recordingChanged(); } }
void FleetEventBlackBox::stopRecording() { if (QThread::currentThread() == thread() && _recording) { _nativePublishingFrames.clear(); _recording = false; emit recordingChanged(); } }
void FleetEventBlackBox::clear() { if (QThread::currentThread() == thread()) { _nativePublishingFrames.clear(); _events.clear(); emit eventsChanged(); } }
bool FleetEventBlackBox::record(const QString& type, const QVariantMap& payload, const QString& source) { return recordAt(QDateTime::currentMSecsSinceEpoch(), type, payload, source); }
bool FleetEventBlackBox::recordAt(qint64 timestampMs, const QString& type, const QVariantMap& payload, const QString& source)
{
    if (QThread::currentThread() != thread() || _nativeReceiptExhausted || _nativePublishingFrames.size() >= 32) return false;
    if (_nativeReceiptSequence == std::numeric_limits<quint64>::max()) { _nativeReceiptExhausted = true; return false; }
    // Check the raw strings before trimming as well: a large whitespace
    // prefix/suffix must not bypass the normalized length limit.
    if (type.size() > kMaxEventTypeLength || source.size() > kMaxEventSourceLength) {
        return false;
    }
    const QString normalizedType = type.trimmed();
    const QString normalizedSource = source.trimmed();
    if (!_recording || normalizedType.isEmpty()
        || normalizedType.size() > kMaxEventTypeLength
        || normalizedSource.size() > kMaxEventSourceLength
        || !payloadWithinLimits(payload)
        || timestampMs <= 0
        || timestampMs > QDateTime::currentMSecsSinceEpoch() + kFutureEventToleranceMs) {
        return false;
    }
    QVariantMap event;
    event[QStringLiteral("timestampMs")] = timestampMs;
    event[QStringLiteral("type")] = normalizedType;
    event[QStringLiteral("source")] = normalizedSource.isNull() ? QStringLiteral("") : normalizedSource;
    event[QStringLiteral("payload")] = payload;
    _events.append(event);
    while (_events.size() > _maxEvents) _events.removeFirst();
    const quint64 nativeSequence = ++_nativeReceiptSequence;
    const QString nativeEpoch = _nativeSourceEpoch;
    const QVariantMap exactEvent = event;
    _nativePublishingFrames.insert(nativeSequence, exactEvent);
    const QPointer<FleetEventBlackBox> self(this);
    const auto releaseNative = qScopeGuard([self, nativeSequence]() {
        if (self) self->_nativePublishingFrames.remove(nativeSequence);
    });
    // Native acceptance is committed and published BEFORE any legacy/QML
    // notification can clear/import or create another event. Storage still
    // must check this actual object's transient receipt, not the value alone.
    emit nativeEventAccepted(nativeEpoch, QString::number(nativeSequence), exactEvent);
    if (!self) return true; // The original diagnostic was already accepted.
    emit eventsChanged();
    if (!self) return true;
    emit eventRecorded(exactEvent);
    return true;
}

QVariantMap FleetEventBlackBox::eventAt(int index) const { return (index >= 0 && index < _events.size()) ? _events.at(index).toMap() : QVariantMap(); }

QString FleetEventBlackBox::exportJson() const
{
    QJsonArray array;
    for (const QVariant& value : _events) {
        array.append(QJsonObject::fromVariantMap(value.toMap()));
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

bool FleetEventBlackBox::importJson(const QString& json)
{
    if (QThread::currentThread() != thread()) return false;
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxPersistenceBytes) {
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) return false;
    QVariantList imported;
    for (const QJsonValue& value : document.array()) {
        if (!value.isObject()) return false;
        const QJsonObject object = value.toObject();
        for (const QString& key : object.keys()) {
            if (key != QStringLiteral("timestampMs")
                && key != QStringLiteral("type")
                && key != QStringLiteral("source")
                && key != QStringLiteral("payload")) {
                return false;
            }
        }
        // Every persisted journal item must retain the same shape produced by
        // recordAt().  In particular, do not accept an item without a
        // timestamp or payload: such an item cannot be ordered or audited and
        // would otherwise enter the in-memory journal as an unverifiable
        // record.  source may be empty, but it is still serialized as a
        // string by recordAt() and therefore remains required here.
        if (!object.contains(QStringLiteral("timestampMs"))
            || !object.contains(QStringLiteral("type"))
            || !object.contains(QStringLiteral("source"))
            || !object.contains(QStringLiteral("payload"))) {
            return false;
        }
        const QVariantMap event = object.toVariantMap();
        const QString rawType = event.value(QStringLiteral("type")).toString();
        const QString rawSource = event.value(QStringLiteral("source")).toString();
        const QString normalizedType = rawType.trimmed();
        const QString normalizedSource = rawSource.trimmed();
        if (event.value(QStringLiteral("type")).type() != QVariant::String
            || rawType.size() > kMaxEventTypeLength
            || normalizedType.isEmpty()
            || normalizedType.size() > kMaxEventTypeLength) {
            return false;
        }
        if (event.contains(QStringLiteral("source"))
            && (event.value(QStringLiteral("source")).type() != QVariant::String
                || rawSource.size() > kMaxEventSourceLength
                || normalizedSource.size() > kMaxEventSourceLength)) {
            return false;
        }
        if (event.contains(QStringLiteral("timestampMs"))) {
            const QJsonValue timestamp = object.value(QStringLiteral("timestampMs"));
            const double number = timestamp.toDouble();
            if (!timestamp.isDouble() || !qIsFinite(number)
                || qFloor(number) != number
                || number <= 0.0
                || number < static_cast<double>(std::numeric_limits<qint64>::min())
                || number >= static_cast<double>(std::numeric_limits<qint64>::max())
                || number > static_cast<double>(QDateTime::currentMSecsSinceEpoch()
                                                 + kFutureEventToleranceMs)) {
                return false;
            }
        }
        if (event.contains(QStringLiteral("payload"))
            && (event.value(QStringLiteral("payload")).type() != QVariant::Map
                || !payloadWithinLimits(event.value(QStringLiteral("payload")).toMap()))) {
            return false;
        }
        imported.append(event);
    }
    while (imported.size() > _maxEvents) imported.removeFirst();
    _nativePublishingFrames.clear(); // Import cannot publish/restore a native receipt.
    _events = imported;
    emit eventsChanged();
    return true;
}

QString FleetEventBlackBox::exportSnapshotJson() const
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("merivus.fleet.event-blackbox"));
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    root.insert(QStringLiteral("events"), QJsonDocument::fromJson(exportJson().toUtf8()).array());
    root.insert(QStringLiteral("recording"), _recording);
    root.insert(QStringLiteral("maxEvents"), _maxEvents);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool FleetEventBlackBox::importSnapshotJson(const QString& json)
{
    if (QThread::currentThread() != thread()) return false;
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxPersistenceBytes) {
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();
    for (const QString& key : root.keys()) {
        if (key != QStringLiteral("format")
            && key != QStringLiteral("schemaVersion")
            && key != QStringLiteral("events")
            && key != QStringLiteral("recording")
            && key != QStringLiteral("maxEvents")) {
            return false;
        }
    }
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("merivus.fleet.event-blackbox")
        || !schemaVersion.isDouble() || schemaVersion.toDouble() != kSchemaVersion
        || !root.value(QStringLiteral("events")).isArray()) {
        return false;
    }
    if (root.contains(QStringLiteral("recording"))
            && !root.value(QStringLiteral("recording")).isBool()) {
        return false;
    }
    int importedMaxEvents = _maxEvents;
    if (root.contains(QStringLiteral("maxEvents"))) {
        const QJsonValue maxEvents = root.value(QStringLiteral("maxEvents"));
        const double maxEventsValue = maxEvents.toDouble(-1.0);
        const int importedMax = maxEvents.toInt(-1);
        if (!maxEvents.isDouble() || maxEventsValue != importedMax
                || importedMax < 100 || importedMax > 1000000) {
            return false;
        }
        importedMaxEvents = importedMax;
    }
    const QString eventsJson = QString::fromUtf8(
        QJsonDocument(root.value(QStringLiteral("events")).toArray()).toJson(QJsonDocument::Compact));

    // Validate and bound against the snapshot's retention policy in a
    // temporary object before replacing either piece of live state.  Recording
    // state is intentionally not restored from evidence.
    FleetEventBlackBox staged;
    staged._maxEvents = importedMaxEvents;
    if (!staged.importJson(eventsJson)) {
        return false;
    }

    const bool retentionChanged = _maxEvents != importedMaxEvents;
    _nativePublishingFrames.clear();
    _events = staged._events;
    _maxEvents = importedMaxEvents;
    const QPointer<FleetEventBlackBox> self(this);
    if (retentionChanged) {
        emit maxEventsChanged();
        if (!self) return true;
    }
    emit eventsChanged();
    return true;
}

bool FleetEventBlackBox::saveSnapshotToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportSnapshotJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxPersistenceBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetEventBlackBox::loadSnapshotFromFile(const QString& filePath)
{
    if (QThread::currentThread() != thread()) return false;
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxPersistenceBytes) {
        return false;
    }
    // Read one byte beyond the limit so a file that grows after size() cannot
    // make the bounded check ineffective.
    const QByteArray bytes = file.read(kMaxPersistenceBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importSnapshotJson validates the complete document before replacing the
    // in-memory journal.  Do not clear or partially apply a failed load.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importSnapshotJson(json);
}

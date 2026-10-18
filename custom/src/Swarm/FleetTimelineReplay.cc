#include "FleetTimelineReplay.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QStringList>
#include <QVariantHash>
#include <QtMath>

#include <limits>

namespace {
constexpr qint64 kMaxPersistenceBytes = 32LL * 1024LL * 1024LL;
constexpr int kMaxEventTypeLength = 128;
constexpr int kMaxEventSourceLength = 256;
constexpr int kMaxPayloadDepth = 16;
constexpr int kMaxPayloadNodes = 50000;
constexpr qint64 kMaxPayloadBytes = 512LL * 1024LL;

bool payloadWithinLimits(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (!nodes || !bytes || depth > kMaxPayloadDepth) return false;
    if (++(*nodes) > kMaxPayloadNodes) return false;
    if (!value.isValid() || value.isNull()) return true;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            *bytes += static_cast<qint64>(it.key().size()) * 2;
            if (*bytes > kMaxPayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            *bytes += static_cast<qint64>(it.key().size()) * 2;
            if (*bytes > kMaxPayloadBytes
                || !payloadWithinLimits(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!payloadWithinLimits(item, depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        *bytes += static_cast<qint64>(value.toString().size()) * 2;
    } else if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            *bytes += static_cast<qint64>(item.size()) * 2;
        }
    } else if (value.type() == QVariant::ByteArray) {
        *bytes += value.toByteArray().size();
    }
    return *bytes <= kMaxPayloadBytes;
}

bool payloadWithinLimits(const QVariantMap& payload)
{
    int nodes = 0;
    qint64 bytes = 0;
    return payloadWithinLimits(QVariant(payload), 0, &nodes, &bytes);
}

bool strictTimestamp(const QVariant& value, qint64* timestamp)
{
    if (!timestamp) return false;
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number <= 0.0
        || number >= static_cast<double>(std::numeric_limits<qint64>::max())) return false;
    *timestamp = static_cast<qint64>(number);
    return true;
}

bool validEvent(const QVariant& value, qint64* timestamp)
{
    if (value.type() != QVariant::Map) return false;
    const QVariantMap event = value.toMap();
    const QStringList allowed = {QStringLiteral("timestampMs"), QStringLiteral("type"),
                                 QStringLiteral("source"), QStringLiteral("payload")};
    for (const QString& key : event.keys()) if (!allowed.contains(key)) return false;
    if (!event.contains(QStringLiteral("timestampMs"))
        || !event.contains(QStringLiteral("type"))
        || !event.contains(QStringLiteral("source"))
        || !event.contains(QStringLiteral("payload"))) return false;
    const QVariant type = event.value(QStringLiteral("type"));
    const QVariant source = event.value(QStringLiteral("source"));
    if (type.type() != QVariant::String || type.toString().trimmed().isEmpty()
        || type.toString().size() > kMaxEventTypeLength
        || source.type() != QVariant::String
        || source.toString().size() > kMaxEventSourceLength
        || event.value(QStringLiteral("payload")).type() != QVariant::Map
        || !payloadWithinLimits(event.value(QStringLiteral("payload")).toMap())) return false;
    return strictTimestamp(event.value(QStringLiteral("timestampMs")), timestamp);
}

bool validEventList(const QVariantList& events)
{
    qint64 previous = 0;
    for (const QVariant& value : events) {
        qint64 timestamp = 0;
        if (!validEvent(value, &timestamp) || (previous > 0 && timestamp < previous)) return false;
        previous = timestamp;
    }
    return true;
}
}

FleetTimelineReplay::FleetTimelineReplay(QObject* parent) : QObject(parent)
{
    _timer.setSingleShot(false);
    _timer.setInterval(_intervalMs);
    connect(&_timer, &QTimer::timeout, this, &FleetTimelineReplay::_tick);
}

void FleetTimelineReplay::setEvents(const QVariantList& events)
{
    if (!validEventList(events)) return;
    _events = events;
    reset();
    emit eventsChanged();
}

bool FleetTimelineReplay::replaceEvents(const QVariantList& events)
{
    // Keep the in-memory replacement path subject to the same event schema as
    // the black-box journal. A malformed row or out-of-order timestamp must
    // never partially replace a replay used for audit or incident review.
    if (!validEventList(events)) return false;
    setEvents(events);
    return true;
}
void FleetTimelineReplay::setIntervalMs(int intervalMs)
{
    const int bounded = qBound(10, intervalMs, 60000);
    if (_intervalMs == bounded) return;
    _intervalMs = bounded;
    _timer.setInterval(_intervalMs);
    emit intervalChanged();
}

void FleetTimelineReplay::reset()
{
    _position = 0;
    _timer.stop();
    if (_playing) { _playing = false; emit playingChanged(); }
    emit positionChanged();
}

QVariantMap FleetTimelineReplay::step()
{
    if (_position >= _events.size()) {
        if (_playing) { _playing = false; emit playingChanged(); }
        _timer.stop();
        emit finished();
        return QVariantMap();
    }
    QVariantMap event = _events.at(_position++).toMap();
    emit positionChanged();
    emit eventReady(event);
    if (_position >= _events.size()) {
        if (_playing) { _playing = false; emit playingChanged(); }
        _timer.stop();
        emit finished();
    }
    return event;
}

void FleetTimelineReplay::play()
{
    if (!_playing && _position < _events.size()) {
        _playing = true;
        _timer.start();
        emit playingChanged();
    }
}

void FleetTimelineReplay::pause()
{
    if (_playing) {
        _playing = false;
        _timer.stop();
        emit playingChanged();
    }
}

void FleetTimelineReplay::_tick()
{
    step();
}

QString FleetTimelineReplay::exportJson() const
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("merivus.fleet.timeline-replay"));
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("events"), QJsonValue::fromVariant(_events));
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool FleetTimelineReplay::importJson(const QString& json)
{
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxPersistenceBytes) return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QJsonObject root = document.object();
    const QStringList allowedKeys = {
        QStringLiteral("format"), QStringLiteral("schemaVersion"), QStringLiteral("events")
    };
    for (const QString& key : root.keys()) {
        if (!allowedKeys.contains(key)) {
            return false;
        }
    }
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("merivus.fleet.timeline-replay")
        || !schemaVersion.isDouble() || schemaVersion.toDouble() != 1.0
        || !root.value(QStringLiteral("events")).isArray()) {
        return false;
    }
    QVariantList imported;
    for (const QJsonValue& value : root.value(QStringLiteral("events")).toArray()) {
        if (!value.isObject()) return false;
        const QVariantMap event = value.toObject().toVariantMap();
        if (event.isEmpty()) return false;
        imported.append(event);
    }
    return replaceEvents(imported);
}

bool FleetTimelineReplay::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
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

bool FleetTimelineReplay::loadFromFile(const QString& filePath)
{
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
    const QByteArray bytes = file.read(kMaxPersistenceBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importJson validates the complete document before replacing the
    // timeline, so an invalid file cannot partially mutate replay state.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importJson(json);
}

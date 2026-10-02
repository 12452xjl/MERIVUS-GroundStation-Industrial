#include "FleetEventBlackBox.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

FleetEventBlackBox::FleetEventBlackBox(QObject* parent) : QObject(parent) {}

void FleetEventBlackBox::setMaxEvents(int maxEvents)
{
    const int bounded = qBound(100, maxEvents, 1000000);
    if (_maxEvents == bounded) return;
    _maxEvents = bounded;
    while (_events.size() > _maxEvents) _events.removeFirst();
    emit maxEventsChanged();
    emit eventsChanged();
}

void FleetEventBlackBox::startRecording() { if (!_recording) { _recording = true; emit recordingChanged(); } }
void FleetEventBlackBox::stopRecording() { if (_recording) { _recording = false; emit recordingChanged(); } }
void FleetEventBlackBox::clear() { _events.clear(); emit eventsChanged(); }
bool FleetEventBlackBox::record(const QString& type, const QVariantMap& payload, const QString& source) { return recordAt(QDateTime::currentMSecsSinceEpoch(), type, payload, source); }
bool FleetEventBlackBox::recordAt(qint64 timestampMs, const QString& type, const QVariantMap& payload, const QString& source)
{
    if (!_recording || type.trimmed().isEmpty()) return false;
    QVariantMap event;
    event[QStringLiteral("timestampMs")] = timestampMs;
    event[QStringLiteral("type")] = type.trimmed();
    event[QStringLiteral("source")] = source.trimmed();
    event[QStringLiteral("payload")] = payload;
    _events.append(event);
    while (_events.size() > _maxEvents) _events.removeFirst();
    emit eventsChanged();
    emit eventRecorded(event);
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
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) return false;
    QVariantList imported;
    for (const QJsonValue& value : document.array()) {
        if (value.isObject()) imported.append(value.toObject().toVariantMap());
    }
    while (imported.size() > _maxEvents) imported.removeFirst();
    _events = imported;
    emit eventsChanged();
    return true;
}

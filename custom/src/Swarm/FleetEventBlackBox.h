#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Transport independent append-only event journal for diagnostics and replay.
class FleetEventBlackBox : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int maxEvents READ maxEvents WRITE setMaxEvents NOTIFY maxEventsChanged)
public:
    explicit FleetEventBlackBox(QObject* parent = nullptr);
    QVariantList events() const { return _events; }
    bool recording() const { return _recording; }
    int maxEvents() const { return _maxEvents; }
    void setMaxEvents(int maxEvents);
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool record(const QString& type, const QVariantMap& payload = QVariantMap(), const QString& source = QString());
    Q_INVOKABLE bool recordAt(qint64 timestampMs, const QString& type, const QVariantMap& payload = QVariantMap(), const QString& source = QString());
    Q_INVOKABLE QVariantMap eventAt(int index) const;
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
signals:
    void eventsChanged();
    void recordingChanged();
    void maxEventsChanged();
    void eventRecorded(const QVariantMap& event);
private:
    QVariantList _events;
    bool _recording = false;
    int _maxEvents = 10000;
};

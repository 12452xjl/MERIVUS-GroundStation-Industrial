#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
class FleetTimelineReplay : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList events READ events WRITE setEvents NOTIFY eventsChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY intervalChanged)
public:
    explicit FleetTimelineReplay(QObject* parent = nullptr);
    QVariantList events() const { return _events; }
    int position() const { return _position; }
    bool playing() const { return _playing; }
    int intervalMs() const { return _intervalMs; }
    void setEvents(const QVariantList& events);
    void setIntervalMs(int intervalMs);
    Q_INVOKABLE void reset(); Q_INVOKABLE QVariantMap step(); Q_INVOKABLE void play(); Q_INVOKABLE void pause();
signals: void eventsChanged(); void positionChanged(); void playingChanged(); void intervalChanged(); void eventReady(const QVariantMap& event); void finished();
private slots: void _tick();
private: QVariantList _events; int _position = 0; bool _playing = false; int _intervalMs = 100; QTimer _timer;
};

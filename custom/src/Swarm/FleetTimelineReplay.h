#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
class FleetTimelineReplay : public QObject
{
    Q_OBJECT
    // QML may inspect the replay, but cannot replace it through an unvalidated
    // property assignment. Use replaceEvents/importJson so the controller has
    // one validation boundary for black-box data.
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
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
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE bool replaceEvents(const QVariantList& events);
    // Local-only persistence helpers.  Writes are atomic and loads preserve
    // the current timeline when validation fails.
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
signals: void eventsChanged(); void positionChanged(); void playingChanged(); void intervalChanged(); void eventReady(const QVariantMap& event); void finished();
private slots: void _tick();
private: QVariantList _events; int _position = 0; bool _playing = false; int _intervalMs = 100; QTimer _timer;
};

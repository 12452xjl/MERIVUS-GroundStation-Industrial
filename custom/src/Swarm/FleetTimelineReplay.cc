#include "FleetTimelineReplay.h"

FleetTimelineReplay::FleetTimelineReplay(QObject* parent) : QObject(parent)
{
    _timer.setSingleShot(false);
    _timer.setInterval(_intervalMs);
    connect(&_timer, &QTimer::timeout, this, &FleetTimelineReplay::_tick);
}

void FleetTimelineReplay::setEvents(const QVariantList& events) { _events = events; reset(); emit eventsChanged(); }
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

#pragma once

#include <QGeoCoordinate>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QtMath>

#include "QGCMAVLink.h"

class Vehicle;

class ReviewRangeController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Vehicle* vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)
    Q_PROPERTY(bool inspecting READ inspecting NOTIFY stateChanged)
    Q_PROPERTY(bool previewing READ previewing NOTIFY stateChanged)
    Q_PROPERTY(bool rangeAvailable READ rangeAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool energyOnly READ energyOnly NOTIFY stateChanged)
    Q_PROPERTY(QGeoCoordinate anchor READ anchor NOTIFY stateChanged)
    Q_PROPERTY(double radiusMeters READ radiusMeters NOTIFY stateChanged)
    Q_PROPERTY(double returnMarginSeconds READ returnMarginSeconds NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(int taskWorkSeconds READ taskWorkSeconds WRITE setTaskWorkSeconds NOTIFY stateChanged)

public:
    explicit ReviewRangeController(QObject* parent = nullptr);

    Vehicle* vehicle() const { return _vehicle; }
    void setVehicle(Vehicle* vehicle);
    bool inspecting() const { return _inspecting; }
    bool previewing() const { return _inspecting && !_taskStarted; }
    bool rangeAvailable() const { return previewing() && qIsFinite(_radiusMeters); }
    bool energyOnly() const { return _energyOnly; }
    QGeoCoordinate anchor() const { return _anchor; }
    double radiusMeters() const { return _radiusMeters; }
    double returnMarginSeconds() const { return _returnMarginSeconds; }
    QString statusText() const { return _statusText; }
    int taskWorkSeconds() const { return _taskWorkSeconds; }
    void setTaskWorkSeconds(int seconds);

    Q_INVOKABLE void startInspection();
    Q_INVOKABLE void stopInspection();

signals:
    void vehicleChanged();
    void stateChanged();

private:
    void _onMavlinkMessage(const mavlink_message_t& message);
    void _refresh();
    void _publish(double radius, double returnMargin, bool energyOnly, const QString& status);

    Vehicle* _vehicle = nullptr;
    QGeoCoordinate _anchor;
    QTimer _timer;
    bool _inspecting = false;
    bool _taskStarted = false;
    bool _energyOnly = true;
    double _radiusMeters = qQNaN();
    double _returnMarginSeconds = qQNaN();
    QString _statusText;
    int _taskWorkSeconds = 300;
    QHash<int, qint64> _batterySeenAt;
    qint64 _windSeenAt = 0;
    qint64 _escSeenAt = 0;
    qint64 _escInfoSeenAt = 0;
    bool _escFailure = false;
    int _motionTicks = 0;
};

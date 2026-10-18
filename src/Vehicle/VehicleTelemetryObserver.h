#pragma once

#include "QGCMAVLink.h"

#include <QObject>
#include <QString>

class Vehicle;
class LinkInterface;

// Native passive ingress only. No Qt signal, invokable, import, replay, or
// control API. The core Vehicle module does not depend on a custom recorder.
class VehicleTelemetryObserver : public QObject
{
public:
    explicit VehicleTelemetryObserver(QObject* parent = nullptr) : QObject(parent) {}
    ~VehicleTelemetryObserver() override = default;

protected:
    virtual void observeTelemetryFrame(Vehicle* vehicle, LinkInterface* link,
        const mavlink_message_t& originalMessage, const QString& sourceEpoch,
        quint64 sourceSequence, qint64 receivedMonotonicMs) = 0;
    virtual void observeTelemetrySourceRetired(Vehicle* vehicle,
        const QString& sourceEpoch, quint64 lastSourceSequence, const QString& fixedCode) = 0;

private:
    friend class Vehicle;
};

#pragma once

#include "QGCMAVLink.h"

#include <QObject>
#include <QString>

class Vehicle;
class LinkInterface;

// Independent native execution observation lane. No QObject metadata,
// invokable, public bind/import, command or telemetry-recorder ownership.
class VehicleMissionExecutionObserver : public QObject
{
public:
    explicit VehicleMissionExecutionObserver(QObject* parent = nullptr) : QObject(parent) {}
    ~VehicleMissionExecutionObserver() override = default;

    struct QueueContext {
        QString sourceEpoch;
        QString startupId;
        QString stepId;
        QString operationId;
        QString wirePayloadHash;
        int vehicleId = 0;
        int componentId = 0;
    };

protected:
    virtual bool beginExecutionQueue(Vehicle* vehicle, LinkInterface* link, const QueueContext& context) = 0;
    virtual void executionQueueCommitted(const QueueContext& context) = 0;
    virtual void observeExecutionFrame(Vehicle* vehicle, LinkInterface* link,
        const mavlink_message_t& originalMessage, const QString& sourceEpoch,
        quint64 sourceSequence, qint64 receivedMonotonicMs) = 0;
    virtual void observeExecutionSourceRetired(Vehicle* vehicle, const QString& sourceEpoch,
        quint64 lastSourceSequence, const QString& fixedCode) = 0;

private:
    friend class Vehicle;
};

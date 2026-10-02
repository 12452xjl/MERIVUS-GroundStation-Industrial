#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Selects heterogeneous vehicles for a task without depending on Vehicle or
// MAVLink.  The result is a plan for human review; it never dispatches a
// command and it treats missing safety fields as risk instead of silently
// converting them into a healthy state.
class FleetCapabilityMatcher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastPlan READ lastPlan NOTIFY planChanged)

public:
    explicit FleetCapabilityMatcher(QObject* parent = nullptr);

    QVariantMap lastPlan() const { return _lastPlan; }

    Q_INVOKABLE QVariantMap plan(const QVariantMap& task,
                                 const QVariantList& fleet);
    Q_INVOKABLE void clear();

signals:
    void planChanged();

private:
    static int vehicleId(const QVariantMap& vehicle);
    static QStringList capabilities(const QVariantMap& vehicle);
    static bool hasCapability(const QStringList& capabilities, const QString& required);
    static QStringList stringList(const QVariant& value);
    static QVariantMap rejection(int id, const QString& reason);

    QVariantMap _lastPlan;
};

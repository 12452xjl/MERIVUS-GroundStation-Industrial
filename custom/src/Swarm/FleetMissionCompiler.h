#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "FleetMissionClearanceGeometry.h"

// Compiles an intent/task description into a reviewable, transport-neutral
// mission plan. The compiler normalizes supplied routes and can generate
// geometric area-coverage proposals when explicitly requested. It does not
// select a live Vehicle, talk MAVLink, upload a mission, or release a flight
// command. Generated estimates are not authoritative waypoint timing.
// Explicit execution profiles and waypoint dwell are retained as requirements;
// normalization does not grant an execution permit or infer unsupported policy.
class FleetMissionCompiler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastPlan READ lastPlan NOTIFY planChanged)

public:
    explicit FleetMissionCompiler(QObject* parent = nullptr);

    QVariantMap lastPlan() const { return _lastPlan; }

    // task may contain intent/taskId, area/target, waypoints or route,
    // time/constraint fields, assignments/members, and reserveVehicleIds.
    // An optional coverage map invokes the bounded pure-data coverage
    // planner instead of a shared global route. Selected survey members
    // receive distinct routes; support members retain explicit own routes.
    // task.executionProfile is an optional complete default; a member's
    // executionProfile replaces it as a whole and is never partially merged.
    // With no profile, legacy geometry-only planning remains unchanged.
    // fleet is optional planning context. When non-empty it is a closed
    // membership snapshot: malformed/duplicate entries and assignments or
    // reserves absent from it block the plan. An empty fleet is explicitly
    // represented as planning-only in the returned plan.
    Q_INVOKABLE QVariantMap compile(const QVariantMap& task,
                                    const QVariantList& fleet = QVariantList());

    // Pure native worker entry: no QObject state or signals. Cancellation
    // returns exact2 {cancelled:true, reason:"mission-compile-cancelled"},
    // never a partial/blocked plan. The owning publisher must also check its
    // token and request identity after the worker has actually finished.
    static QVariantMap compileCancelable(const QVariantMap& task,
                                         const QVariantList& fleet,
                                         const FleetMissionClearanceGeometry::CancellationToken& cancellation);
    Q_INVOKABLE void clear();

signals:
    void planChanged();

private:
    QVariantMap _lastPlan;
};

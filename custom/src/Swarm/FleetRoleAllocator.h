#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Produces a reviewable role allocation for a heterogeneous fleet.  The
// allocator is deliberately transport-agnostic: it consumes normalized fleet
// snapshots and returns suggestions only.  It never mutates a task, selects a
// Vehicle, or sends a MAVLink/flight command.
class FleetRoleAllocator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastPlan READ lastPlan NOTIFY planChanged)

public:
    explicit FleetRoleAllocator(QObject* parent = nullptr);

    QVariantMap lastPlan() const { return _lastPlan; }

    // task fields include leaderRequired, requiredRoles, scoutCount, relayCount,
    // responderCount, reserveCount, minimumBatteryPercent,
    // minimumLinkQualityPercent, requireConfigurationBaseline, global
    // requiredCapabilities (or legacy capabilities) and optional
    // role-specific capability lists (leaderCapabilities, scoutCapabilities,
    // relayCapabilities, responderCapabilities).  Fleet entries use the
    // normalized registry fields (systemId/vehicleId, healthState, online,
    // capabilities, configurationState, batteryPercent, linkQualityPercent
    // and distanceMeters/distanceToTaskMeters). Reserves must satisfy the
    // global policy and explicit capabilities of at least one actually
    // assigned primary role. Both global aliases are strictly validated;
    // a present requiredCapabilities, even empty, takes precedence. Global
    // and role lists use the same bounded canonical capability parser.
    // Reserve assignments expose eligibleReplacementRoles; reserveRoleCoverage
    // distinguishes eligible candidates from selected reserves, while
    // simultaneousReserveCoverage is a one-to-one recommendation for missing
    // primary slots, not an airframe/performance guarantee or reservation.
    // feasible continues to describe primary allocation only.
    Q_INVOKABLE QVariantMap plan(const QVariantMap& task,
                                 const QVariantList& fleet);
    Q_INVOKABLE void clear();

signals:
    void planChanged();

private:
    QVariantMap _lastPlan;
};

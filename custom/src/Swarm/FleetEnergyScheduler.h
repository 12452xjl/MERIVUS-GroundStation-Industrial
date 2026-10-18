#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Produces a reviewable energy and reserve plan for a heterogeneous fleet.
// The class is deliberately transport- and Vehicle-agnostic: it performs
// pure calculations and never sends commands or mutates a mission.
class FleetEnergyScheduler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastPlan READ lastPlan NOTIFY planChanged)

public:
    explicit FleetEnergyScheduler(QObject* parent = nullptr);

    QVariantMap lastPlan() const { return _lastPlan; }

    // task fields: vehicleCount/minVehicleCount, reserveCount,
    // missionDurationSeconds, returnReserveSeconds,
    // replacementThresholdPercent and minimumBatteryPercent.
    // Optional task.candidateVehicleIds constrains selection to the compiler/
    // role allocator assignment snapshot; when present, vehicles outside it
    // are not allowed to satisfy the energy plan.
    // Fleet entries may provide batteryPercent, availableFlightSeconds or
    // consumptionPercentPerMinute (plus optional enduranceSeconds).
    // Optional memberEnergyBudgets binds every original primary and reserve:
    // each row is {vehicleId, group, missionDurationSeconds,
    // returnReserveSeconds, details}. Rows preserve input order and identity;
    // ineligible members remain visible and reserves never replace primaries.
    // A compiled reserve with explicit required/incomplete planning evidence
    // may use exact -1/-1 mission/return budgets. It is always ineligible,
    // remains unknown in the ledger, and does not erase valid primary rows.
    // Battery/link/endurance accept the Registry's exact -1 unknown sentinel.
    // Available-time estimates use the minimum of supplied valid models;
    // percentage projection needs an independent drain/endurance model.
    // Registry snapshots set nativeBatteryEvidenceRequired=true. Its bounded
    // native BATTERY_STATUS envelope and scalar aliases must agree, and the
    // uniquely identified propulsion source must be current within 3000 ms;
    // stale/ambiguous sources cannot fall back to cached percentage/endurance.
    // Native direct time is an autopilot-reported estimate, not measured
    // endurance. Each assignment includes nativeBatteryEvidence (empty when
    // no valid native envelope exists), retaining rejected members' sources.
    // Shared native battery validation accepts legacy schema1 and schema2
    // reported charge/fault fields. A current fresh row reporting unsafe
    // charge state or a nonzero fault mask makes that vehicle ineligible;
    // this includes auxiliary batteries and cannot be bypassed by a static
    // endurance estimate. Missing reports do not certify battery health.
    // Invalid task parameters make parametersValid=false and never yield a
    // feasible plan; invalid vehicle evidence is reported per vehicle.
    // Explicit actual bool requirePerformanceEvidence=true additionally needs
    // current capability-declaration/baseline evidence and positive canonical
    // full-battery endurance plus known battery %. Raw drain/direct aliases
    // cannot satisfy that policy. Current validated native direct time may
    // only further lower available time. The optional independently validated
    // nativeBatteryDrainEvidence may only lower already-known available and
    // projected values; it cannot satisfy a missing model or declaration.
    // Native decline evidence and before/after diagnostics live in the top
    // nativeBatteryDrainAdjustments list, never the legacy18/strict21 member
    // or compiled reserve budget shapes. Strict rows retain the declaration.
    // Missing decline sibling preserves the previous output shape.
    Q_INVOKABLE QVariantMap plan(const QVariantMap& task,
                                 const QVariantList& fleet);
    Q_INVOKABLE void clear();

signals:
    void planChanged();

private:
    QVariantMap _lastPlan;
};

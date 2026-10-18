#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Transport-independent continuity planner for active fleet missions.
//
// The planner consumes task summaries from SwarmMissionOrchestrator and
// normalized vehicle snapshots from FleetRegistry. It emits reviewable
// suggestions for leader faults, link loss, low battery, split/reallocation,
// recovery and mid-mission joins. It never mutates a task, owns a Vehicle,
// sends MAVLink, or releases a flight command.
class FleetMissionContinuityPlanner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastPlan READ lastPlan NOTIFY planChanged)
    Q_PROPERTY(QVariantList suggestions READ suggestions NOTIFY planChanged)

public:
    explicit FleetMissionContinuityPlanner(QObject* parent = nullptr);

    QVariantMap lastPlan() const { return _lastPlan; }
    QVariantList suggestions() const;

    // missions accepts SwarmMissionOrchestrator::taskSummaries() entries;
    // continuity suggestions retain the source taskRevision when supplied.
    // Supply the full task collection: running/paused/pending members are
    // reserved to their existing task and excluded from other candidates.
    // Malformed or conflicting ownership blocks the complete review batch.
    // Explicit metadata.roleAssignments is a closed {vehicleId,role} map for
    // all members (leader/scout/relay/responder/member); role capability
    // policies require this source, never FleetRegistry.role. A replacement
    // graph covers all affected roles at once and only a complete one-to-one
    // match produces a complete reviewed reassignment proposal. Within this
    // call, complete proposals exclude each other's replacement candidates;
    // those exclusions are not actual Swarm reservations or flight permits.
    // Optional value-only context.nativeOccupancySnapshot is validated by
    // FleetNativeOccupancyEvidence. Every held/uncertain entry excludes a
    // candidate; any assigned entry holds the whole generic task. Set strict
    // bool requireNativeOccupancyEvidence=true in the product path: missing
    // evidence then permits only hold, never a complete change proposal.
    // Default missing evidence preserves isolated planning diagnostics marked
    // requiresNativeOccupancyReview=true, not applicability or an owner grant.
    // A valid map alone does not authenticate the actual Swarm source; final
    // local reassignment must be admitted by that same pinned native owner.
    // fleet accepts FleetRegistry::vehicles() entries. Context is optional and
    // may provide lowBatteryPercent, candidateBatteryPercent,
    // linkLossQualityPercent, allowMidMissionJoin, requireBatteryEvidence,
    // recoveryReadyTaskIds, evaluatedAt, nativeOccupancySnapshot and
    // requireNativeOccupancyEvidence.
    Q_INVOKABLE QVariantMap plan(const QVariantList& missions,
                                 const QVariantList& fleet,
                                 const QVariantMap& context = QVariantMap());
    // Single-task inspection cannot establish cross-task availability. Its
    // suggestions explicitly carry ownershipEvidenceAvailable=false; use
    // plan() with the complete collection before applying a reassignment.
    Q_INVOKABLE QVariantMap planMission(const QVariantMap& mission,
                                        const QVariantList& fleet,
                                        const QVariantMap& context = QVariantMap());
    Q_INVOKABLE void clear();

signals:
    void planChanged();

private:
    QVariantMap planMissionWithOwnership(const QVariantMap& mission,
                                          const QVariantList& fleet,
                                          const QVariantMap& context,
                                          const QVariantMap& ownership);
    QVariantMap _lastPlan;
};

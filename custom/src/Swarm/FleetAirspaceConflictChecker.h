#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// Performs a transport-neutral 4D (space + time) review of fleet plans.
// The checker never changes a plan, contacts Vehicle/MAVLink, or releases a
// command.  It is intentionally conservative: missing coordinates, timing,
// or a return corridor become an explicit review item instead of being
// treated as safe.
class FleetAirspaceConflictChecker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastReport READ lastReport NOTIFY reportChanged)

public:
    explicit FleetAirspaceConflictChecker(QObject* parent = nullptr);

    QVariantMap lastReport() const { return _lastReport; }

    // missionPlans: [{vehicleId, waypoints/route, returnPath, startTime,
    //                  endTime, returnRequired, ...}].
    // vehicles: optional vehicle/home snapshots reserved for an adapter; this
    // checker does not synthesize a return path from home coordinates.
    // noFlyZonesInput: a real JSON-compatible QVariantList of zone maps. The
    // raw QVariant is checked before conversion so a scalar/null cannot be
    // silently treated as an empty zone list. Omission retains the legacy
    // explicit empty-list default; it does not prove airspace coverage.
    // options: minimumSeparationMeters, timeStepSeconds, returnRequired and
    // coordinate-system policy.  All values are review inputs only.
    Q_INVOKABLE QVariantMap check(const QVariantList& missionPlans,
                                  const QVariantList& vehicles = QVariantList(),
                                  const QVariant& noFlyZonesInput = QVariant(QVariantList()),
                                  const QVariantMap& options = QVariantMap());
    Q_INVOKABLE void clear();

    // Pure bounded input inspection only, not zone geometry or coverage
    // certification. Returns valid/state/reason/zoneCount/fingerprint; the
    // fingerprint hashes the complete compact JSON list. No state or signals.
    static QVariantMap inspectNoFlyZoneInput(const QVariant& input);

    // Independent, pure-value review of explicitly supplied nominal WGS84 /
    // AMSL startup segments. It neither joins alternative branches nor borrows
    // plan timing, a default separation, or any clearance / execution permit.
    // Does not mutate lastReport or emit reportChanged.
    static QVariantMap checkNominalStartupSegments(const QVariantList& segments,
                                                   const QVariant& minimumSeparationMeters);

    // Pure, bounded joint geometry review of nominal geodetic / AMSL segments
    // and explicitly supplied supported zones. No timing-based exclusions,
    // default separation, successful prefixes, lastReport changes or signals.
    static QVariantMap checkJointSpatialSegments(const QVariant& segmentsInput,
                                                 const QVariant& noFlyZonesInput,
                                                 const QVariant& minimumSeparationMeters);

    // Display only the original, bounded evidence from the same archived
    // plan/review record. Does not rerun a check, load zones, infer policy from
    // an old empty list, mutate lastReport, or establish any clearance.
    static QVariantMap archivedMissionSpatialEvidence(const QVariantMap& plan,
                                                      const QVariantMap& reviewEvidence);

signals:
    void reportChanged();

private:
    QVariantMap _lastReport;
};

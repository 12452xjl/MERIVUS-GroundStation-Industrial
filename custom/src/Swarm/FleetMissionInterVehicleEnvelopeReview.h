#pragma once

#include "FleetMissionClearanceGeometry.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <memory>

class FleetMissionStartupDomainReview;
class FleetMissionWaypointSwitchReview;

// Complete declared-envelope overlap calculation. A candidate is NOT an
// observed collision, a synchronized trajectory or a permission to start.
class FleetMissionInterVehicleEnvelopeReview final
{
public:
    using CancellationToken = FleetMissionClearanceGeometry::CancellationToken;

    // Only the two actual producers can create these immutable sets. No
    // public report/map can install components or drop their extra planes.
    // Matcher-only sets may validate result shape, never start a calculation.
    class Prepared final {
    public:
        ~Prepared() = default;
        Prepared(const Prepared&) = delete;
        Prepared& operator=(const Prepared&) = delete;
    private:
        friend class FleetMissionStartupDomainReview;
        friend class FleetMissionWaypointSwitchReview;
        friend class FleetMissionInterVehicleEnvelopeReview;
        struct Component {
            int vehicleId = 0;
            QString kind;
            QVariantMap reference;
            FleetMissionClearanceGeometry::Polytope envelope;
        };
        Prepared() = default;
        QString source;
        QString geometryBasis;
        QString inputFingerprint;
        QString modelsFingerprint;
        QString motionFingerprint;
        QVariantList memberIds;
        bool geometryComplete = false;
        bool calculationSource = false;
        QVector<Component> components;
    };
    using PreparedPtr = std::shared_ptr<const Prepared>;

    // Exact3 {ok,error,report}. Same-build complete geometry is compared in
    // all DD/DW/WW combinations, excluding only same-vehicle pairs. Sources
    // that are valid but geometrically incomplete produce an Unknown report
    // with no component prefix. No terrain-contact waiver applies here.
    static QVariantMap build(const PreparedPtr& domain, const PreparedPtr& waypoint,
                             const CancellationToken& cancellation = CancellationToken());

    // Source/count/detail identity checks only: never calls feasible(), SAT,
    // coverage or another intersection solver. Does not authenticate verdicts.
    static bool resultMatches(const PreparedPtr& domain, const PreparedPtr& waypoint,
                              const QVariantMap& result);

private:
    struct Inputs;
    static bool _inputs(const PreparedPtr& domain, const PreparedPtr& waypoint,
                        Inputs* output, FleetMissionClearanceGeometry::Budget* budget);
};

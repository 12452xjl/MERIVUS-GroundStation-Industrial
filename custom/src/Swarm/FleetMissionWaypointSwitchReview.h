#pragma once

#include "FleetMissionClearanceGeometry.h"
#include "FleetMissionInterVehicleEnvelopeReview.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Pure declared-model geometry for ordinary NAV acceptance regions, adjacent
// NAV connections, LAND entry/descent reference envelopes, and the conditional
// ground-branch TAKEOFF vertical/top-to-first-NAV reference envelopes. These
// keep the complete endpoint encoding regions, not an observed action,
// smoothing reachable-set, fresh ground-state proof or permit.
class FleetMissionWaypointSwitchReview final
{
public:
    using CancellationToken = FleetMissionClearanceGeometry::CancellationToken;
    using PreparedPtr = FleetMissionInterVehicleEnvelopeReview::PreparedPtr;

    // models: complete ordered row3 {vehicleId, geographicMotionModel,
    // parameterSnapshot}. The latter is the exact six-key native transport
    // codec; it does not itself authenticate receipts or their currentness.
    // Exact3 {ok,error,report}; failure has no usable report/geometry prefix.
    // Optional output seals this same full prepare (including every plane),
    // without exporting terrain contact waivers for use against another UAV.
    static QVariantMap build(const QVariant& clearanceInput, const QVariantList& models,
                             const QVariantMap& frozenJointView, const QString& geometryBasis,
                             const CancellationToken& cancellation = CancellationToken(),
                             PreparedPtr* prepared = nullptr);

    // Reconstructs complete inputs, regions, connections and report counts;
    // does not rerun environment solvers or authenticate a reported verdict.
    // Optional output is matcher-only, never an actual Inter build source.
    static bool resultMatches(const QVariant& clearanceInput, const QVariantList& models,
                              const QVariantMap& frozenJointView, const QString& geometryBasis,
                              const QVariantMap& result, PreparedPtr* prepared = nullptr);
};

#pragma once

#include <QVariantMap>

// Pure, bounded comparison of two views captured from the same native review.
// The caller owns that source binding. Values/hashes do not authenticate it,
// certify actual paths, or establish onboard contents or flight permission.
class FleetMissionJointSpatialReview
{
public:
    // exact2 {missionReview: whole-plan result3, startupView: startup view15}
    // -> exact3 {ok, error, view}. Does not rebuild or change the mission.
    static QVariantMap build(const QVariantMap& source);

    // Builds the complete mission view once from the plan/evidence, excluding
    // only its separately bounded derived planningClearanceReview field,
    // then extracts only its actual encoded-planning scope. No startup view
    // or current-position hypothesis is invented. Returns result3/scope view22.
    static QVariantMap buildPlanning(const QVariantMap& plan, const QVariantMap& reviewEvidence);

    // Builds the complete mission view once and the supplied startup source
    // exact4 {reviewKind, geometryBasis, vehicles, minimumSeparationMeters}.
    // Returns result3/scope view22 without running a spatial/environment
    // comparison. The native caller, not this value function, owns actuality.
    static QVariantMap buildStartup(const QVariantMap& plan, const QVariantMap& reviewEvidence,
                                    const QVariantMap& startupSource);

    // exact2 {missionReview: whole-plan result3, startupView: basis view14}.
    // Returns result3/scope view22; geometryBasis never means reviewFingerprint.
    static QVariantMap buildForGeometryBasis(const QVariantMap& source);
};

#pragma once

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// Pure, bounded geometry planning. This class never owns a Vehicle, loads a
// map, uploads a mission, or releases commands. All routes remain proposals.
// Optional options.vehicleEndpoints maps canonical participant IDs to explicit
// {start, returnTo} points in the declared frame/altitude datum. It adds bounded
// ingress and return proposals, not launch/landing or obstacle-clearance proof.
// Different endpoint altitudes require climbSpeedMetersPerSecond. Route section
// indices are zero-based inclusive, sharing the adjacent section's boundary.
// area.holes may contain up to eight simple disjoint vertex-list rings strictly
// inside the outer ring (128 total vertices). Only nominal interiors are
// excluded by default: boundaries may be followed and no setback is inferred.
// Optional area.boundarySetbackMeters is a real number in [0,500]. Positive
// values construct all bounded conservative inset/expanded-hole components and
// independently check complete nominal segments against every original edge.
// This is not physical clearance or certified numerical error evidence.
class FleetAreaCoveragePlanner
{
public:
    static QVariantMap generate(const QVariant& area,
                                const QVariantMap& options,
                                const QVariantList& members);

    // Reuse the same domain and nominal segment checks for explicit compiled
    // routes, including remainder tasks which no longer request coverage.
    // No/empty holes and absent/exact-zero setback is a legacy no-op. Positive
    // setback also activates full checks without holes and never permits an
    // outside-outer-ring route. Active checks require a single explicit
    // coordinate frame across every outbound/return point, bounded projection
    // and complete route checks. This does not generate or rewrite any route.
    // Exact result: {ok, reason, holesProvided, holeCount,
    //                flightCommandReleased:false}.
    static QVariantMap validateExclusionRoutes(const QVariant& area,
                                               const QVariantList& members);

    // Complete, value-only original boundary policy retained by compiled plans.
    // All three methods return exactly {ok, reason, policy}; failure policy is
    // empty. The policy is exactly {schemaVersion:1, active:bool, area:map}.
    // Inactive area is empty and preserves the legacy no-hole/zero-setback
    // no-op without imposing coverage vertex limits on ordinary metadata.
    // Active area retains canonical ORIGINAL coordinates, holes, frame and
    // nominal setback; it contains no checked/certified/permission flags.
    // Members supply an explicit frame only when the original area omitted it.
    static QVariantMap buildRouteBoundaryPolicy(const QVariant& area,
                                                const QVariantList& members);
    static QVariantMap validateRouteBoundaryPolicy(const QVariantMap& policy);
    // Recheck complete nominal route segments against the original policy,
    // including outbound-to-return joins, without generating candidate paths.
    static QVariantMap validateBoundaryPolicyRoutes(const QVariantMap& policy,
                                                    const QVariantList& members);
};

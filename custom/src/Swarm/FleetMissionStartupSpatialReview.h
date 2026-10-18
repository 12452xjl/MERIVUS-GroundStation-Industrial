#pragma once

#include <QVariantMap>

// Pure, bounded display projection. The caller binds the original native
// review; this value function does not authenticate its input or grant flight
// permission. No Vehicle, requests, persistence, callbacks or shared state.
class FleetMissionStartupSpatialReview
{
public:
    static QVariantMap build(const QVariantMap& source);

    // exact4 {reviewKind, geometryBasis, vehicles, minimumSeparationMeters}.
    // geometryBasis is a pre-artifact SHA-256 supplied by the native capture,
    // not a final review fingerprint. Returns result3 with a distinct view14.
    static QVariantMap buildForGeometryBasis(const QVariantMap& source);
};

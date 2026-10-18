#pragma once

#include <QVariantMap>
#include <QtGlobal>

// Pure value checks for an actual native ESTIMATOR_STATUS observation.
// The caller must retain the real Vehicle/source and ordered RAW journal.
// A valid or healthy value is not authentication, continuity or permission.
class FleetMissionStartupEstimator
{
public:
    // Returns the stable eleven-field reference, excluding receipt/age and
    // remote time. ok means current/fresh, well-shaped evidence, not healthy.
    // Invalid or unavailable evidence returns an empty map, without defaults.
    static QVariantMap reference(const QVariantMap& actualHealth, qint64 maximumAgeMs,
        bool* ok = nullptr);

    // Additional actual-ID pairing for callers holding a frozen native target.
    // A successful capture may still contain negative flags; check healthy.
    static bool capture(const QVariantMap& actualHealth, int vehicleId, int componentId,
        qint64 maximumAgeMs, QVariantMap* output);

    // Negative flags/clock discontinuity are valid observations, not discarded
    // as malformed. Only healthyReference applies the healthy predicate.
    static bool referenceValid(const QVariantMap& observed);
    // This mission path encodes WGS84/MSL coordinates. In addition to the
    // existing attitude/velocity/relative-position checks, require good
    // absolute horizontal/vertical position and reject constant-position mode.
    // These estimator output flags do not require a particular GNSS device or
    // certify the coordinate datum, obstacle clearance, or flight permission.
    static bool healthyReference(const QVariantMap& observed);

    // Advance one changed, healthy semantic reference in an ordered journal.
    // Revisions must increase, not necessarily by one (SYS shares the native
    // counter). On rejection output is untouched and callers must stop; this
    // function never removes the original negative/discontinuous RAW history.
    static bool advance(const QVariantMap& previous, const QVariantMap& observed,
        QVariantMap* output);
};

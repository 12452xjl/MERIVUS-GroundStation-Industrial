#pragma once

#include <QVariantMap>

// Bounded arithmetic on already observed native risk values. A true result
// is NOT a reference/origin certificate, command permission or motion proof.
// The caller must retain and verify the actual native owner and RAW history.
class FleetMissionStartupMotion
{
public:
    static bool localMatches(const QVariantMap& observed, const QVariantMap& reviewed,
        const QVariantMap& envelope);

    // In/out observed travel belongs to one actual ordered native journal.
    // On failure it is untouched; equal encoded headings do not add travel.
    static bool headingMatches(const QVariantMap& observed, const QVariantMap& reviewed,
        const QVariantMap& previous, const QVariantMap& envelope, double* observedTravelDegrees);
};

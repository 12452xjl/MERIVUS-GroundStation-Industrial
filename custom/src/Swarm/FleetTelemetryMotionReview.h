#pragma once

#include <QVariantMap>

// Bounded value-only review of one completed native telemetry-history page.
// No QObject, observer, files, transport, live vehicle or flight authority.
// Validation of a public value map does not authenticate its native owner.
class FleetTelemetryMotionReview final
{
public:
    // Requires the Archive owner's separate, current position-value consent.
    // Exact31 report; exact5 groups and exact16 segments. Only relative sample
    // statistics leave this function, never payloads, UTC, absolute positions
    // or source epochs. A metric of -1 means there were insufficient usable
    // samples for that metric (a receive gap requires at least two samples).
    static QVariantMap review(const QVariantMap& nativePage, bool positionValuesConfirmed);
};

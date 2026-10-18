#pragma once

#include <QVariantMap>
#include <QVariantList>

// Bounded, single-page interpretation of the reported GPS_RAW_INT fields.
// No live source, files, worker, commands, interpolation or flight authority.
class FleetTelemetryGnssReview final
{
public:
    // The Archive separately owns the current native page and fresh consent.
    // A supplied value map can establish consistency, never native ownership.
    // Exact33 report / exact7 identity / exact3 display policy / exact18 rows.
    static QVariantMap review(const QVariantMap& nativePage, bool positionValuesConfirmed);

    // Complete successful-report value validation for bounded historical
    // storage. JSON integral Doubles are accepted; no caller/native origin,
    // CRC, physical identity or permission is established by a value map.
    static bool reportValid(const QVariantMap& report);
    // Independent collection limits and complete-page/sample overlap rules.
    // File hashes may change as an original capture grows; they are not a
    // deduplication identity. Empty collections are valid values.
    static bool reportsValid(const QVariantList& reports);
};

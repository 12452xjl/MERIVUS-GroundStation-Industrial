#pragma once

#include <QByteArray>
#include <QVariantList>
#include <QVariantMap>

// Pure value encoding of an EXACT eleven-key conservative journal document:
// schemaVersion, format, ledgerEpoch, ledgerRevision, writerEpoch, runOpen,
// coverageComplete, everObservedVehicleIds, lastObservationIdentity,
// recoveryReviewRequired, fingerprint. No permissions, credentials, positions,
// live owners or restored QObject bindings are represented by this format.
// A matching hash is consistency only: it authenticates neither the writer nor
// the aircraft, prevents no rollback and never proves that empty IDs are free.
class FleetOccupancyJournalCodec final
{
public:
    static constexpr int MaxFileBytes = 16384;

    // Numeric schema/vehicle IDs normalize to Int; input IDs must already be
    // strictly ascending and unique. Every other field keeps its strict type.
    // A failed call clears a supplied output, including when input aliases it.
    static bool documentValid(const QVariantMap& document,
                              QVariantMap* canonical = nullptr);
    static bool identityValid(const QVariantMap& identity, bool allowEmpty = false);
    static bool vehicleIdsValid(const QVariantList& vehicleIds,
                                QVariantList* canonical = nullptr);

    // Accept EXACTLY the ten document fields other than fingerprint. Invalid
    // input returns empty bytes; unknown fields are not silently discarded.
    static QByteArray encode(const QVariantMap& withoutFingerprint);
    // Bounded JSON object decoding plus the same strict document/hash checks.
    // This does not attest file origin, durable storage or historical coverage.
    static bool decode(const QByteArray& bytes, QVariantMap* canonical);
};

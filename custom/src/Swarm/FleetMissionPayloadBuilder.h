#pragma once

#include <QVariantList>
#include <QVariantMap>

// Deterministic, bounded geometry and explicit supported-request serialization.
// Encoded commands are not execution, timing or clearance certificates.
// This helper neither
// owns a Vehicle nor uploads, starts, changes, or authorizes a mission.
// The returned payload is deliberately not an executable mission permit.
class FleetMissionPayloadBuilder
{
public:
    // Current construction requires a complete canonical routeBoundaryPolicy;
    // historical plans without it must be rebuilt from their original source
    // and reviewed again. Successful serialization is never flight authority.
    static QVariantMap build(const QVariantMap& plan);

    // Inert historical byte reproduction ONLY. With a retained policy this
    // uses build(); policy-less old plans reproduce their legacy payload shape
    // for archive/provenance comparison, not current preparation or upload.
    // Explicit current compiler/version markers cannot use this legacy path.
    static QVariantMap reproduceHistoricalPayloads(const QVariantMap& plan);

    // MISSION_ITEM_INT immutable comparison identity. Raw upload items retain
    // original degrees; readback items use decoded degrees and explicit
    // null/system-default for geographic NaN yaw. Non-coordinate speed uses
    // numeric reserved zeros and not-applicable yaw. Current-item state is
    // not identity. No explicit heading retains the exact legacy v2 domain.
    // Explicit NAV heading uses v3 tick256 semantics: requests are quantized
    // visibly before upload; actual float32 readback is retained separately
    // and may canonicalize only within the declared derived error bound.
    // This is not an upload-byte hash or proof of actual yaw attainment.
    static QVariantMap canonicalWireItems(const QVariantList& items,
                                          bool fromReadback = false);

private:
    static QVariantMap buildImpl(const QVariantMap& plan, bool legacyHistory);
};

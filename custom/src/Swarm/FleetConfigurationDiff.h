#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

/// Pure, bounded comparison of locally retained configuration values.
///
/// This is neither source authentication nor a physical-modification detector.
/// Fingerprints retain QVariant types and UTF-16 code units. Unknown null
/// metatype IDs are meaningful only in this process, not across Qt versions.
/// DateTime equality uses validity and UTC milliseconds, not its display zone.
class FleetConfigurationDiff
{
public:
    static bool valueValid(const QVariant& value);
    static QString typedFingerprint(const QVariantMap& configuration);
    static bool exactEqual(const QVariant& first, const QVariant& second);

    /// maxChanges must be 1..128. Each input independently permits depth 8,
    /// 2048 nodes and 128 KiB of descriptor scalar bytes, matching Registry.
    /// Result exact13: schemaVersion, comparisonState, comparisonKnown,
    /// complete, truncated, reason, beforeKnown, afterKnown,
    /// beforeTypedFingerprint, afterTypedFingerprint, changeCount,
    /// reportedChangeCount, changes. Unknown is never the same as equal.
    ///
    /// Each change exact6: path (JSON Pointer), kind, beforePresent,
    /// afterPresent, beforeValue, afterValue. Kinds are added, removed,
    /// type-changed, null-changed, value-changed. Arrays compare by index,
    /// never inferred moves. Added/removed/type-changed containers are one
    /// subtree row, with explicitly truncated display when members are omitted.
    ///
    /// Each safe display value exact9: type, valid, null, representation,
    /// display, displayTruncated, byteCount, elementCount, typedFingerprint.
    /// No raw QVariant/container/user value is exposed. Display <=512 UTF-16
    /// code units per value and total display+path <=64 Ki UTF-16 code units.
    /// Any omitted row/value marks the entire report truncated, never complete.
    static QVariantMap compare(const QVariantMap& before, bool beforeKnown,
                               const QVariantMap& after, bool afterKnown,
                               int maxChanges = 128);
};

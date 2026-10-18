#pragma once

#include <QVariantList>
#include <QVariantMap>

// Pure, bounded diagnostics of an already completed ordinary MissionItem
// readback. Never a source receipt, RAW INT proof, or mission permission.
class FleetMissionReadbackDiff final
{
public:
    static QVariantMap compare(const QVariantList& expectedRawItems,
                               const QVariantList& observedDecodedItems);
    static QVariantMap summary(const QVariantMap& diagnostic);
};

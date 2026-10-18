#pragma once

#include <QMap>
#include <QVariantMap>
#include <functional>

class FleetRegistry;
class FleetFirmwareAdapterCatalog;
class Vehicle;

// Native-owner correspondence only: declarations do not authenticate a datum
// or identify the estimator branch producing the current global altitude.
class FleetMissionClearanceDatum
{
public:
    // Exact5: provided, inputValid, declaredDatumCorrespondenceMatches,
    // reason, evidence. Empty source18 means the legacy absent-input path.
    // Native UTC watermark is NOT evidence/hash content. Clear output to -1
    // on entry; only present success writes its final observed UTC. A caller
    // retains the maximum across layers/calls and its original monotonic cap.
    static QVariantMap capture(const QVariantMap& originalClearanceSourceEvidence,
        FleetRegistry* actualRegistry, FleetFirmwareAdapterCatalog* actualCatalog,
        const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence = {},
        qint64 minimumObservedUtcMs = -1, qint64* observedAtUtcMs = nullptr);

    // Only the native owner's original successful evidence14/source18; never
    // a QML/import recovery. Success returns the ORIGINAL evidence unchanged.
    static QVariantMap recapture(const QVariantMap& originalDatumEvidence,
        const QVariantMap& originalClearanceSourceEvidence, FleetRegistry* actualRegistry,
        FleetFirmwareAdapterCatalog* actualCatalog, const QMap<int, Vehicle*>& actualVehicles,
        const std::function<bool()>& fence = {}, qint64 minimumObservedUtcMs = -1,
        qint64* observedAtUtcMs = nullptr);

    // No plugin, external fence, Link virtual, weak-to-strong link promotion,
    // full-catalog hash or raw message request. Origin is checked through the
    // memory-only native observation, never the SourceMatches entry point.
    // Checks local source continuity and UTC only, not remote reachability.
    // The calling review still owns its non-renewable monotonic deadline.
    static bool memoryCurrent(const QVariantMap& originalDatumEvidence,
        const QVariantMap& originalClearanceSourceEvidence, FleetRegistry* actualRegistry,
        FleetFirmwareAdapterCatalog* actualCatalog, const QMap<int, Vehicle*>& actualVehicles,
        qint64 minimumObservedUtcMs = -1, qint64* observedAtUtcMs = nullptr);
};

#pragma once

#include "FleetMissionStartupGeographicMotion.h"
#include "LinkInterface.h"

#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <QVariantMap>

#include <functional>
#include <memory>

class FleetFirmwareAdapterCatalog;
class FleetRegistry;
class Vehicle;

// Native-only shared continuity witness, not an authorization or a physical
// model certificate. It retains the original review's clock and materials;
// passing it between native owners never renews the review or its sources.
class FleetMissionStartupClearanceWitness final
    : public std::enable_shared_from_this<FleetMissionStartupClearanceWitness>
{
public:
    using Ptr = std::shared_ptr<FleetMissionStartupClearanceWitness>;

    // Original complete weak-owner map is frozen by the Review before its
    // first readiness capture. Input includes that capture's complete 29-row
    // value snapshot; neither source may be replaced or renewed here. The
    // complete result8 must include the same-input inter-vehicle envelope
    // report and pass the shared model gate before any witness is published.
    static Ptr capture(const QVariantMap& input, const QVariantMap& result,
        const FleetMissionStartupGeographicMotion::Models& originalModels,
        const QVariantMap& nativeSourceEvidence, const QVariantMap& nativeDatumEvidence,
        FleetRegistry* actualRegistry, FleetFirmwareAdapterCatalog* actualCatalog,
        const QMap<int, Vehicle*>& actualVehicles,
        const QMap<int, WeakLinkInterfacePtr>& originalPrimaryOwners, const QElapsedTimer& originalReviewClock,
        qint64 originalDeadlineMs, qint64 minimumObservedUtcMs,
        const std::function<bool()>& fence = {});

    ~FleetMissionStartupClearanceWitness();
    FleetMissionStartupClearanceWitness(const FleetMissionStartupClearanceWitness&) = delete;
    FleetMissionStartupClearanceWitness& operator=(const FleetMissionStartupClearanceWitness&) = delete;

    // Full original native source checks may call plugins/fence. A fence must
    // not recursively enter this same witness. Source/expiry failures retire
    // it permanently; destruction/cancellation of a Review alone does not.
    bool sourceCurrent(const std::function<bool()>& fence = {});

    // Pure native-memory/clock check: no solver, plugin, Link virtual,
    // weak-to-strong promotion, I/O or callback. The native watermark out is
    // cleared first and published only on success, never hashed as evidence.
    bool memoryCurrent(qint64 minimumObservedUtcMs = -1, qint64* observedAtUtcMs = nullptr);

    // Only the original Review may supply a deadline in this copied clock's
    // elapsed domain. Never pass another owner's local elapsed timestamp.
    bool shortenReviewDeadline(qint64 originalReviewDeadlineMs);

    // Returns the same immutable object created by the original Review and
    // carried through its worker, only after the native memory/time gate.
    // This is neither a map-restoration API nor a whole-domain certificate.
    std::shared_ptr<const FleetMissionStartupGeographicMotion> originalGeographicMotion(int vehicleId);

    QString inputFingerprint() const;
    bool retired() const;
    QString failureReason() const;

private:
    struct State;
    explicit FleetMissionStartupClearanceWitness(std::unique_ptr<State> state);
    std::unique_ptr<State> _state;
};

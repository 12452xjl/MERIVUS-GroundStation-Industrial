#pragma once

#include "FleetMissionStartupIntegerDomain.h"

#include <QMap>
#include <QVariantMap>
#include <memory>

// One immutable value model for the existing geographic startup predicate.
// No Vehicle, source certification, clearance proof or command permission.
class FleetMissionStartupGeographicMotion final
{
public:
    using Ptr = std::shared_ptr<const FleetMissionStartupGeographicMotion>;
    // Native-only complete group ownership. The pointers retain the original
    // cached Qt operands; a source descriptor alone cannot restore them.
    // Copy containers independently at boundaries that accept caller maps.
    using Models = QMap<int, Ptr>;

    static Ptr prepare(const QVariantMap& originalGlobalPosition9,
        const QVariantMap& actualPayload, const QVariantMap& normalizedStartupPolicy,
        double ceiling);
    // Recheck complete original arguments, including canonical item types and
    // payload hash, without recalculating Qt segment lengths or bearings.
    // Value equality does not certify native provenance or current validity.
    bool matchesSource(const QVariantMap& originalGlobalPosition9,
        const QVariantMap& actualPayload, const QVariantMap& normalizedStartupPolicy,
        double ceiling) const;
    bool contains(const QVariantMap& value9, const QVariantMap& previousHeading,
        double* observedHeadingTravel) const;
    // Present only for an explicitly selected profile startupSpatialGuard.
    // Cover/Review must retain this original object, not reconstruct it from
    // the public source map or silently upgrade a legacy source-v1 model.
    FleetMissionStartupIntegerDomain::Ptr integerDomain() const;

    // Complete bounded value descriptor, not a portable source attestation or
    // proof that the permitted geographic domain fits any checked space.
    QVariantMap source() const;
    ~FleetMissionStartupGeographicMotion();

    FleetMissionStartupGeographicMotion(const FleetMissionStartupGeographicMotion&) = delete;
    FleetMissionStartupGeographicMotion& operator=(const FleetMissionStartupGeographicMotion&) = delete;

private:
    struct State;
    explicit FleetMissionStartupGeographicMotion(std::unique_ptr<State> state);
    const std::unique_ptr<State> _state;
};

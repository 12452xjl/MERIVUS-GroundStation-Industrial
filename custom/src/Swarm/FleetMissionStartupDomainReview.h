#pragma once

#include "FleetMissionClearanceGeometry.h"
#include "FleetMissionInterVehicleEnvelopeReview.h"
#include "FleetMissionStartupGeographicMotion.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Bounded environment calculation over the complete versioned native domain.
// New guarded models enforce those same integer cells before Qt filtering;
// legacy ideal covers still do not prove Qt/libm predicate containment.
// Neither branch certifies actual paths, declarations or permission to start.
class FleetMissionStartupDomainReview final
{
public:
    using CancellationToken = FleetMissionClearanceGeometry::CancellationToken;
    using PreparedPtr = FleetMissionInterVehicleEnvelopeReview::PreparedPtr;

    // models is the complete ordered row2 {vehicleId, geographicMotionModel};
    // originalModels must retain the same native original Ptr for every row.
    // Actual calculation never reconstructs/substitutes a model from JSON.
    // Exact3 {ok,error,report}; successful construction includes incomplete or
    // violated reports. Cancellation/invalid sources return an empty report.
    // Optional native output retains this same complete prepare, not public
    // report reconstruction; an incomplete source has no component prefix.
    static QVariantMap build(const QVariant& clearanceInput, const QVariantList& models,
                             const FleetMissionStartupGeographicMotion::Models& originalModels,
                             const QVariantMap& frozenJointView, const QString& geometryBasis,
                             const CancellationToken& cancellation = CancellationToken(),
                             PreparedPtr* prepared = nullptr);

    // Successful report shape/source/count validation only. Rebuilds bounded
    // outer geometry and contact bindings using temporary value-only models,
    // never reruns environment solvers. Cannot prove original Ptr identity.
    // The guarded construction result is not a proof of legacy Qt inclusion,
    // original object identity, between-sample dynamics or execution permission.
    // Optional output is matcher-only and cannot be an actual Inter build source.
    static bool resultMatches(const QVariant& clearanceInput, const QVariantList& models,
                              const QVariantMap& frozenJointView, const QString& geometryBasis,
                              const QVariantMap& result, PreparedPtr* prepared = nullptr);
};

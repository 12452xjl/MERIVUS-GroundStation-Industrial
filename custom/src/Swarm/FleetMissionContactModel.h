#pragma once

#include "FleetMissionClearanceGeometry.h"

#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

// Pure preparation of engineer-declared geometry. This does not run a solver,
// change a wire mission, authenticate a surface or authorize a flight.
class FleetMissionContactModel
{
public:
    using Geometry = FleetMissionClearanceGeometry;
    using CancellationToken = Geometry::CancellationToken;

    struct Component {
        QVariantMap identity; // Original complete segment; never rewritten.
        QString componentId;
        QString componentKind; // protected | support
        Geometry::Polytope swept;
        QString contactId;
        QSet<QString> allowedTerrainCellIds;
        QVector<Geometry::Plane> contactVolume;
    };
    // Same-call, native-only resting-reference material from a successfully
    // bound LAND support component. This is not an observed landing state,
    // a portable contact credential, or a new terrain-contact exemption.
    struct LandingReference {
        int vehicleId = 0;
        int missionSequenceNumber = -1;
        QString segmentId;
        qint64 referenceLower[3]{0, 0, 0};
        qint64 referenceUpper[3]{0, 0, 0};
        QString contactId;
        QSet<QString> allowedTerrainCellIds;
        QVector<Geometry::Plane> contactVolume;
    };
    // Same-call material from a successfully applied vertical TAKEOFF
    // support component. The original reported starting enclosure and its
    // declared height window are retained separately; neither proves ground
    // state nor transfers contact permission to another region or model.
    struct TakeoffReference {
        int vehicleId = 0;
        int missionSequenceNumber = -1;
        QString segmentId;
        qint64 referenceLower[3]{0, 0, 0};
        qint64 referenceUpper[3]{0, 0, 0};
        qint64 minimumReferenceAltitudeMm = 0;
        qint64 maximumReferenceAltitudeMm = 0;
        QString contactId;
        QSet<QString> allowedTerrainCellIds;
        QVector<Geometry::Plane> contactVolume;
    };
    struct Result {
        bool ok = false;
        QString reason;
        bool contactContextComplete = false;
        QVector<Component> components;
        QVector<LandingReference> landingReferences;
        QVector<TakeoffReference> takeoffReferences;
        QVariantList contactApplications;
        QVariantList contactIssues;
    };

    // Callers supply already-normalized frame/polygon values and a validated
    // Joint segment. Exact E7 / original binary64 altitude enclosure in mm.
    static bool pointMillimetres(const QVariantMap& point, const QVariantMap& frame,
                                 qint64 lower[3], qint64 upper[3]);
    // Exact floor/ceil of the original binary64 altitude times 1000, without
    // first rounding a floating-point product. Supported altitude: [-1000,
    // 20000] metres. Distinct non-null outputs; failure leaves both unchanged.
    static bool altitudeMillimetreBounds(double value, qint64* lower, qint64* upper);
    // Pure projection of original integer E7 coordinates and integer AMSL mm.
    // Validates the entire exact declared affine-frame contract and its full
    // geographic extent. No E7/degrees round trip or longitude wrapping. The
    // complete result must be inside +/-50000000 mm on every axis. Callers
    // supply disjoint three-element output arrays; failure changes neither.
    static bool integerPointMillimetres(qint64 latitudeE7, qint64 longitudeE7,
                                        qint64 altitudeMm, const QVariantMap& frame,
                                        qint64 lower[3], qint64 upper[3]);
    static QVector<Geometry::Plane> polygonPlanes(const QVariantList& points);
    static void altitudePlanes(QVector<Geometry::Plane>* planes, qint64 lower, qint64 upper);

    // Schema 2 only. The caller first normalizes the input and validates the
    // entire Joint envelope/ordering/budgets. Each segment produces protected,
    // then support geometry. Missing/mismatched contact is a retained diagnostic
    // (ok=true, incomplete), not permission to omit a segment. Unsupported
    // numeric geometry or cancellation returns no component/result prefix.
    static Result prepare(const QVariantMap& normalizedInput, const QVariantMap& jointView,
                          const CancellationToken& cancellation = CancellationToken());
};

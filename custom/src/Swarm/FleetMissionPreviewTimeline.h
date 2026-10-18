#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Bounded mathematical planning estimate, never an execution/safety permit.
class FleetMissionPreviewTimeline final
{
public:
    struct Result {
        bool ok = false;
        QString error;
        QVariantMap summary;
        QVariantList events;
    };

    static Result build(const QVariantMap& plan, const QVariantMap& energyPlan);

    // Travel-only nominal geometry of one effective, already compiled member.
    // The caller resolves any permitted global route before calling. Holds,
    // real ingress, launch/terminal dynamics and battery use are not included.
    // Always returns the fixed eleven-field diagnostic, with null unknowns.
    static QVariantMap nominalRouteTiming(const QVariantMap& member);
};

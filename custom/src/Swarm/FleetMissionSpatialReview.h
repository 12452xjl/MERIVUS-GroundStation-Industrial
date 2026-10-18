#pragma once

#include <QVariantMap>

// Bounded, value-only display of one supplied compiled plan and its archived
// review evidence. Encoding is rebuilt once from the unchanged whole plan;
// this is neither an onboard read nor a combined spatial safety assessment.
class FleetMissionSpatialReview
{
public:
    static QVariantMap build(const QVariantMap& plan, const QVariantMap& reviewEvidence);
};

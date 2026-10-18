#pragma once

#include <QVariantMap>

// Pure local proposal construction. Native source ownership is established
// by the caller, not by hashes or this value-only helper. No transport or
// Vehicle dependency and no restoration of execution authority.
class FleetMissionRemainderPlanner
{
public:
    // Whole source consistency only: no native owner/authentication claim.
    // Native schema1/exact18 or historical schema2/exact20 with archive binding.
    static QVariantMap validateSource(const QVariantMap& source);
    static QVariantMap build(const QVariantMap& source, const QVariantMap& request);
    static QVariantMap validateProvenance(const QVariantMap& provenance);
    static bool matchesTask(const QVariantMap& provenance, const QVariantMap& task);
    static bool matchesPlan(const QVariantMap& provenance, const QVariantMap& plan);
};

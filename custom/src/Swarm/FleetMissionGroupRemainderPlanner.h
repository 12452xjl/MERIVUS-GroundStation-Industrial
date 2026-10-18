#pragma once

#include <QVariantMap>

// One complete historical Run -> one newly reviewed multi-member task.
// Value consistency only; no native owner, filesystem or flight authority.
// Exact3 requests retain old reserve policies (provenance schema2). Exact4
// requests explicitly replace every reserve descriptor (schema3/exact28).
// Task matching binds that new intent; compiled-plan matching independently
// reproduces the actual reserveMembers geometry/cost with the same generator.
// Neither retained performance values nor matching restore current authority.
class FleetMissionGroupRemainderPlanner final
{
public:
    static QVariantMap build(const QVariantMap& source, const QVariantMap& request);
    static QVariantMap validateProvenance(const QVariantMap& provenance);
    static bool matchesTask(const QVariantMap& provenance, const QVariantMap& task);
    static bool matchesPlan(const QVariantMap& provenance, const QVariantMap& plan);
};

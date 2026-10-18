#pragma once

#include <QVariantMap>

// Explicit independent reserve dispatch proposal, not continuation of a Run.
// Pure bounded value consistency; no native ownership, upload or permission.
class FleetReserveDispatchPlanner final
{
public:
    // source exact3 {binding,task,plan}; request exact3
    // {reserveVehicleId,executionProfile,operatorConfirmed:true}.
    // result exact7 {ok,reason,task,summary,provenance,
    //                flightCommandReleased:false,executable:false}.
    static QVariantMap build(const QVariantMap& source, const QVariantMap& request);
    static QVariantMap validateProvenance(const QVariantMap& provenance);
    static bool matchesTask(const QVariantMap& provenance, const QVariantMap& task);
    static bool matchesPlan(const QVariantMap& provenance, const QVariantMap& plan);
};

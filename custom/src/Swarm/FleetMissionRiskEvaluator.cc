#include "FleetMissionRiskEvaluator.h"

#include <QSet>
#include <QHash>
#include <QMetaType>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

QVariantList listValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) result.append(item);
        return result;
    }
    if (value.type() == QVariant::Map) return QVariantList() << value;
    return QVariantList();
}

QVariant firstValue(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (map.contains(key) && map.value(key).isValid() && !map.value(key).isNull()) {
            return map.value(key);
        }
    }
    return QVariant();
}

double finiteNumber(const QVariant& value, double fallback = -1.0)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return fallback;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && qIsFinite(number) ? number : fallback;
}

bool finiteNonNegativeInteger(const QVariant& value, int* output)
{
    const double number = finiteNumber(value, std::numeric_limits<double>::quiet_NaN());
    if (!qIsFinite(number) || number < 0.0 || qFloor(number) != number
        || number > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (output) *output = static_cast<int>(number);
    return true;
}

QString normalizedId(const QVariantMap& map)
{
    const QVariant value = firstValue(map, QStringList()
        << QStringLiteral("vehicleId") << QStringLiteral("systemId") << QStringLiteral("id"));
    if (!value.isValid() || value.isNull()) return QString();
    double numeric = 0.0;
    const double numericValue = finiteNumber(value, std::numeric_limits<double>::quiet_NaN());
    if (qIsFinite(numericValue)) {
        numeric = numericValue;
        if (numeric < 1.0 || numeric > 255.0 || qFloor(numeric) != numeric) return QString();
        return QString::number(static_cast<int>(numeric));
    }
    if (value.type() != QVariant::String && value.type() != QVariant::ByteArray) return QString();
    const QString text = value.toString().trimmed();
    if (text.isEmpty() || text.size() > 16) return QString();
    bool numericTextOk = false;
    const double numericText = text.toDouble(&numericTextOk);
    if (!numericTextOk || !qIsFinite(numericText) || numericText < 1.0
        || numericText > 255.0 || qFloor(numericText) != numericText) return QString();
    return QString::number(static_cast<int>(numericText));
}

QString severityFor(const QVariantMap& signal)
{
    const QString value = firstValue(signal, QStringList()
        << QStringLiteral("severity") << QStringLiteral("level")).toString().trimmed().toLower();
    if (value == QStringLiteral("critical") || value == QStringLiteral("fatal")
        || value == QStringLiteral("blocking") || value == QStringLiteral("error")) {
        return QStringLiteral("critical");
    }
    if (value == QStringLiteral("major") || value == QStringLiteral("high")) {
        return QStringLiteral("major");
    }
    if (value == QStringLiteral("minor") || value == QStringLiteral("info")
        || value == QStringLiteral("notice")) {
        return QStringLiteral("minor");
    }
    const double weight = finiteNumber(signal.value(QStringLiteral("weight")), 0.0);
    if (weight >= 70.0) return QStringLiteral("critical");
    if (weight >= 35.0) return QStringLiteral("major");
    return QStringLiteral("warning");
}

int severityRank(const QString& severity)
{
    if (severity == QStringLiteral("critical")) return 3;
    if (severity == QStringLiteral("major")) return 2;
    if (severity == QStringLiteral("warning")) return 1;
    return 0;
}

QString normalizedHealthState(const QString& rawState)
{
    const QString state = rawState.trimmed().toLower();
    if (state == QStringLiteral("healthy") || state == QStringLiteral("ok")
        || state == QStringLiteral("ready")) {
        return QStringLiteral("healthy");
    }
    if (state == QStringLiteral("degraded") || state == QStringLiteral("warning")
        || state == QStringLiteral("limited") || state == QStringLiteral("major")) {
        return QStringLiteral("degraded");
    }
    if (state == QStringLiteral("offline") || state == QStringLiteral("critical")
        || state == QStringLiteral("failed") || state == QStringLiteral("fault")
        || state == QStringLiteral("unhealthy") || state == QStringLiteral("emergency")
        || state == QStringLiteral("lost") || state == QStringLiteral("disconnected")
        || state == QStringLiteral("maintenance")) {
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("unknown");
}

void addRisk(QVariantList* risks, const QString& code, const QString& severity,
             const QString& detail, double weight, const QString& source,
             bool affectsCompletion = true)
{
    if (!risks) return;
    QVariantMap risk;
    risk.insert(QStringLiteral("code"), code);
    risk.insert(QStringLiteral("severity"), severity);
    risk.insert(QStringLiteral("detail"), detail);
    risk.insert(QStringLiteral("weight"), qBound(0.0, weight, 100.0));
    risk.insert(QStringLiteral("source"), source);
    risk.insert(QStringLiteral("affectsCompletion"), affectsCompletion);
    risks->append(risk);
}

void appendUnique(QStringList* values, const QString& value)
{
    if (values && !value.isEmpty() && !values->contains(value)) values->append(value);
}

QVariantList extractHealthReports(const QVariantList& input)
{
    // QML callers sometimes wrap a FleetHealthCenter summary in a one-item
    // list because this API is list-typed. Unwrap only that explicit shape;
    // ordinary per-vehicle reports remain unchanged.
    if (input.size() == 1) {
        const QVariantMap wrapper = input.first().toMap();
        if (wrapper.contains(QStringLiteral("vehicles"))) {
            return listValue(wrapper.value(QStringLiteral("vehicles")));
        }
        if (wrapper.contains(QStringLiteral("vehicleHealth"))) {
            return listValue(wrapper.value(QStringLiteral("vehicleHealth")));
        }
    }
    return input;
}

QVariantList missionMembers(const QVariantMap& plan)
{
    QVariant value = plan.value(QStringLiteral("members"));
    if (!value.isValid()) value = plan.value(QStringLiteral("assignments"));
    return listValue(value);
}

QString boundVehicleId(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QString();
    const double number = finiteNumber(value, std::numeric_limits<double>::quiet_NaN());
    return qIsFinite(number) && number >= 1.0 && number <= 255.0 && qFloor(number) == number
        ? QString::number(static_cast<int>(number)) : QString();
}

QString boundMemberId(const QVariantMap& member)
{
    QString resolved;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("systemId"),
                               QStringLiteral("id")}) {
        if (!member.contains(key)) continue;
        const QString id = boundVehicleId(member.value(key));
        if (id.isEmpty() || (!resolved.isEmpty() && resolved != id)) return QString();
        resolved = id;
    }
    return resolved;
}

bool energyIdSet(const QVariant& value, bool bound, QSet<QString>* ids)
{
    if (!ids || !value.isValid() || value.isNull()
        || (value.type() != QVariant::List && (bound || value.type() != QVariant::StringList))) return false;
    const QVariantList entries = listValue(value);
    if (entries.size() > 255) return false;
    ids->clear();
    for (const QVariant& item : entries) {
        const QString id = bound ? boundVehicleId(item)
            : normalizedId(QVariantMap{{QStringLiteral("vehicleId"), item}});
        if (id.isEmpty() || ids->contains(id)) return false;
        ids->insert(id);
    }
    return true;
}

struct BoundEnergyIdentity {
    bool valid = false;
    bool primaryComplete = false;
    bool reserveComplete = false;
    QString error;
};

BoundEnergyIdentity boundEnergyIdentity(const QVariantMap& plan, const QVariantMap& energy)
{
    BoundEnergyIdentity result;
    result.error = QStringLiteral("invalid_bound_energy_identity");
    const QVariant membersValue = plan.value(QStringLiteral("members"));
    const QVariant assignmentsValue = energy.value(QStringLiteral("assignments"));
    if (membersValue.isNull() || membersValue.type() != QVariant::List
        || assignmentsValue.isNull() || assignmentsValue.type() != QVariant::List) return result;
    const QVariantList members = membersValue.toList();
    const QVariantList assignments = assignmentsValue.toList();
    if (members.isEmpty() || members.size() > 255 || assignments.size() > 255) return result;
    QSet<QString> expectedPrimary, expectedReserve, selected, reserves;
    for (const QVariant& value : members) {
        if (value.isNull() || value.type() != QVariant::Map) return result;
        const QString id = boundMemberId(value.toMap());
        if (id.isEmpty() || expectedPrimary.contains(id)) return result;
        expectedPrimary.insert(id);
    }
    if (!energyIdSet(plan.value(QStringLiteral("reserveVehicleIds")), true, &expectedReserve)
        || !energyIdSet(energy.value(QStringLiteral("selectedVehicleIds")), true, &selected)
        || !energyIdSet(energy.value(QStringLiteral("reserveVehicleIds")), true, &reserves)) return result;
    for (const QString& id : expectedPrimary) {
        if (expectedReserve.contains(id)) return result;
    }
    QSet<QString> actualPrimary, actualReserve, eligiblePrimary, eligibleReserve, seen;
    for (const QVariant& value : assignments) {
        if (value.isNull() || value.type() != QVariant::Map) return result;
        const QVariantMap assignment = value.toMap();
        const QString id = boundMemberId(assignment);
        const QVariant groupValue = assignment.value(QStringLiteral("group"));
        const QVariant eligibleValue = assignment.value(QStringLiteral("eligible"));
        const QVariant reasonValue = assignment.value(QStringLiteral("reason"));
        if (id.isEmpty() || seen.contains(id)
            || groupValue.isNull() || groupValue.type() != QVariant::String
            || eligibleValue.isNull() || eligibleValue.type() != QVariant::Bool
            || reasonValue.isNull() || reasonValue.type() != QVariant::String) return result;
        seen.insert(id);
        const QString group = groupValue.toString();
        const bool eligible = eligibleValue.toBool();
        if ((group != QStringLiteral("primary") && group != QStringLiteral("reserve"))
            || (eligible ? !reasonValue.toString().isEmpty() : reasonValue.toString().isEmpty())) return result;
        if (group == QStringLiteral("primary")) {
            actualPrimary.insert(id);
            if (eligible) eligiblePrimary.insert(id);
        } else {
            actualReserve.insert(id);
            if (eligible) eligibleReserve.insert(id);
        }
    }
    // Rows retain ALL assigned aircraft, including ineligible ones. Neither
    // a reserve nor an unrelated eligible vehicle can replace a primary by
    // merely keeping the selected count unchanged.
    if (actualPrimary != expectedPrimary || actualReserve != expectedReserve
        || selected != eligiblePrimary || reserves != eligibleReserve) {
        result.error = QStringLiteral("energy_assignment_identity_mismatch");
        return result;
    }
    result.valid = true;
    result.primaryComplete = eligiblePrimary == expectedPrimary;
    result.reserveComplete = eligibleReserve == expectedReserve;
    result.error = QStringLiteral("");
    return result;
}

bool hasRouteEvidence(const QVariantMap& plan)
{
    int waypointCount = 0;
    if (plan.contains(QStringLiteral("waypointCount"))
        && finiteNonNegativeInteger(plan.value(QStringLiteral("waypointCount")), &waypointCount)
        && waypointCount > 0) {
        return true;
    }
    if (!listValue(plan.value(QStringLiteral("waypoints"))).isEmpty()) return true;
    const QVariantMap area = plan.value(QStringLiteral("areaSummary")).toMap();
    return !area.isEmpty();
}

QVariantMap coverageEvidence(const QVariantMap& plan)
{
    QVariantMap evidence;
    bool invalid = false;
    const QStringList countKeys = QStringList()
        << QStringLiteral("completedWaypoints") << QStringLiteral("observedWaypoints")
        << QStringLiteral("totalWaypoints") << QStringLiteral("waypointCount");
    QHash<QString, double> counts;
    for (const QString& key : countKeys) {
        if (!plan.contains(key)) continue;
        const double number = finiteNumber(plan.value(key), std::numeric_limits<double>::quiet_NaN());
        if (!qIsFinite(number) || number < 0.0 || qFloor(number) != number) {
            invalid = true;
        } else {
            counts.insert(key, number);
        }
    }
    const bool hasCompleted = counts.contains(QStringLiteral("completedWaypoints"))
        || counts.contains(QStringLiteral("observedWaypoints"));
    const bool hasTotal = counts.contains(QStringLiteral("totalWaypoints"))
        || counts.contains(QStringLiteral("waypointCount"));
    if (hasCompleted && hasTotal) {
        const double completed = counts.value(counts.contains(QStringLiteral("completedWaypoints"))
            ? QStringLiteral("completedWaypoints") : QStringLiteral("observedWaypoints"));
        const double total = counts.value(counts.contains(QStringLiteral("totalWaypoints"))
            ? QStringLiteral("totalWaypoints") : QStringLiteral("waypointCount"));
        if (total <= 0.0 || completed > total) invalid = true;
    }
    QVariant source = firstValue(plan, QStringList()
        << QStringLiteral("coverageRate") << QStringLiteral("coveragePercent")
        << QStringLiteral("coveredPercent") << QStringLiteral("coverageEvidence"));
    QVariantMap sourceMap = source.toMap();
    if (sourceMap.contains(QStringLiteral("rate"))) source = sourceMap.value(QStringLiteral("rate"));
    if (sourceMap.contains(QStringLiteral("coverageRate"))) source = sourceMap.value(QStringLiteral("coverageRate"));
    double rate = finiteNumber(source);
    invalid = invalid || (source.isValid() && rate < 0.0);
    if (rate < 0.0) {
        const double completed = finiteNumber(firstValue(plan, QStringList()
            << QStringLiteral("completedWaypoints") << QStringLiteral("observedWaypoints")));
        const double total = finiteNumber(firstValue(plan, QStringList()
            << QStringLiteral("totalWaypoints") << QStringLiteral("waypointCount")));
        if (completed >= 0.0 && total > 0.0) rate = completed / total * 100.0;
        else if (source.isValid()) invalid = true;
    } else if (rate <= 1.0) {
        rate *= 100.0;
    }
    if (source.isValid() && (rate < 0.0 || rate > 100.0)) invalid = true;
    if (!invalid && rate >= 0.0 && rate <= 100.0) {
        evidence.insert(QStringLiteral("available"), true);
        evidence.insert(QStringLiteral("ratePercent"), rate);
        evidence.insert(QStringLiteral("method"), QStringLiteral("caller_supplied_evidence"));
        evidence.insert(QStringLiteral("valid"), true);
    } else if (invalid) {
        evidence.insert(QStringLiteral("available"), false);
        evidence.insert(QStringLiteral("ratePercent"), -1.0);
        evidence.insert(QStringLiteral("method"), QStringLiteral("invalid_coverage_evidence"));
        evidence.insert(QStringLiteral("valid"), false);
    } else {
        evidence.insert(QStringLiteral("available"), false);
        evidence.insert(QStringLiteral("ratePercent"), -1.0);
        evidence.insert(QStringLiteral("method"), QStringLiteral("no_coverage_evidence"));
        evidence.insert(QStringLiteral("valid"), true);
    }
    return evidence;
}

}

FleetMissionRiskEvaluator::FleetMissionRiskEvaluator(QObject* parent)
    : QObject(parent)
{
}

bool FleetMissionRiskEvaluator::hasNoBlockingRisks(const QVariantMap& assessment)
{
    const QVariant inputValid = assessment.value(QStringLiteral("inputValid"));
    if (inputValid.userType() != QMetaType::Bool || inputValid.isNull() || !inputValid.toBool()) return false;
    const QVariant reasons = assessment.value(QStringLiteral("blockingReasons"));
    if (!reasons.isValid() || reasons.isNull()) return false;
    // Check the actual container type before reading it. Missing or malformed
    // data must not be converted into an apparently empty list.
    if (reasons.userType() == QMetaType::QStringList) return reasons.value<QStringList>().isEmpty();
    if (reasons.userType() == QMetaType::QVariantList) return reasons.value<QVariantList>().isEmpty();
    return false;
}

QVariantMap FleetMissionRiskEvaluator::evaluate(const QVariantMap& missionPlan,
                                                 const QVariantList& healthReports,
                                                 const QVariantMap& energyPlan,
                                                 const QVariantList& riskSignals)
{
    QVariantList risks;
    QStringList blockingReasons;
    QStringList alternatives;
    double probability = 1.0;

    bool inputValid = true;
    auto addInputError = [&](const QString& code, const QString& detail) {
        inputValid = false;
        addRisk(&risks, code, QStringLiteral("critical"), detail, 100.0,
                QStringLiteral("inputValidation"));
        appendUnique(&blockingReasons, code);
    };

    bool planFeasible = true;
    if (missionPlan.contains(QStringLiteral("feasible"))) {
        const QVariant value = missionPlan.value(QStringLiteral("feasible"));
        if (value.type() != QVariant::Bool) {
            addInputError(QStringLiteral("invalid_plan_feasible_flag"),
                          QStringLiteral("Mission plan feasible must be a boolean."));
        } else {
            planFeasible = value.toBool();
        }
    }
    QStringList unmet;
    if (missionPlan.contains(QStringLiteral("unmetRequirements"))) {
        const QVariant value = missionPlan.value(QStringLiteral("unmetRequirements"));
        if (value.type() == QVariant::StringList) {
            unmet = value.toStringList();
        } else if (value.type() == QVariant::List) {
            for (const QVariant& item : value.toList()) {
                if (item.type() != QVariant::String || item.toString().trimmed().isEmpty()) {
                    addInputError(QStringLiteral("invalid_unmet_requirements"),
                                  QStringLiteral("Mission plan unmetRequirements must contain non-empty strings."));
                    break;
                }
                unmet.append(item.toString().trimmed());
            }
        } else {
            addInputError(QStringLiteral("invalid_unmet_requirements"),
                          QStringLiteral("Mission plan unmetRequirements must be a string list."));
        }
    }
    if (!planFeasible || !unmet.isEmpty()) {
        addRisk(&risks, QStringLiteral("mission_plan_not_feasible"), QStringLiteral("critical"),
                QStringLiteral("The supplied mission plan has unmet requirements and is not executable as reviewed."),
                90.0, QStringLiteral("missionPlan"));
        probability *= 0.15;
        appendUnique(&blockingReasons, QStringLiteral("mission_plan_not_feasible"));
        alternatives << QStringLiteral("缩小任务范围或补齐任务计划缺项后重新评估");
    }

    const QStringList missionCountKeys = QStringList()
        << QStringLiteral("waypointCount") << QStringLiteral("completedWaypoints")
        << QStringLiteral("observedWaypoints") << QStringLiteral("totalWaypoints");
    for (const QString& key : missionCountKeys) {
        if (missionPlan.contains(key)
            && !finiteNonNegativeInteger(missionPlan.value(key), nullptr)) {
            addInputError(QStringLiteral("invalid_mission_numeric_field"),
                          QStringLiteral("Mission plan %1 must be a finite non-negative integer.").arg(key));
        }
    }

    const QVariantList members = missionMembers(missionPlan);
    QSet<QString> memberIds;
    if (!members.isEmpty()) {
        for (const QVariant& memberValue : members) {
            if (memberValue.type() != QVariant::Map) {
                addInputError(QStringLiteral("invalid_mission_member"),
                              QStringLiteral("Every mission member must be a map with a vehicle identifier."));
                continue;
            }
            const QString id = normalizedId(memberValue.toMap());
            if (id.isEmpty()) {
                addInputError(QStringLiteral("invalid_mission_member_id"),
                              QStringLiteral("Every mission member must have a valid vehicle identifier."));
            } else if (memberIds.contains(id)) {
                addInputError(QStringLiteral("duplicate_mission_member_id"),
                              QStringLiteral("Mission member identifiers must be unique."));
            } else {
                memberIds.insert(id);
            }
        }
    }
    if (members.isEmpty() || !hasRouteEvidence(missionPlan)) {
        addRisk(&risks, QStringLiteral("mission_evidence_missing"), QStringLiteral("critical"),
                QStringLiteral("The plan has no usable member assignment or route/area evidence."),
                85.0, QStringLiteral("missionPlan"));
        probability *= 0.1;
        appendUnique(&blockingReasons, QStringLiteral("mission_evidence_missing"));
        alternatives << QStringLiteral("补充成员分配、区域或航点证据");
    }

    QVariantList reports = extractHealthReports(healthReports);
    const QVariantList fleetHealth = listValue(QVariant(healthReports));
    if (reports.isEmpty() && !fleetHealth.isEmpty()) reports = fleetHealth;
    QSet<QString> healthyMembers;
    QSet<QString> plannedMembers;
    for (const QVariant& memberValue : members) {
        const QString memberId = normalizedId(memberValue.toMap());
        if (!memberId.isEmpty()) plannedMembers.insert(memberId);
    }
    QSet<QString> reportedMembers;
    QSet<QString> seenReports;
    int unknownHealth = 0;
    int unavailableHealth = 0;
    int degradedHealth = 0;
    for (const QVariant& value : reports) {
        if (value.type() != QVariant::Map) {
            addInputError(QStringLiteral("invalid_health_report"),
                          QStringLiteral("Every health report must be a map."));
            continue;
        }
        const QVariantMap report = value.toMap();
        if (report.isEmpty()) {
            addInputError(QStringLiteral("invalid_health_report"),
                          QStringLiteral("Health report maps must not be empty."));
            continue;
        }
        const QString id = normalizedId(report);
        if (id.isEmpty()) {
            addInputError(QStringLiteral("invalid_health_report_id"),
                          QStringLiteral("Every health report must have a valid vehicle identifier."));
        } else if (seenReports.contains(id)) {
            addInputError(QStringLiteral("duplicate_health_report_id"),
                          QStringLiteral("Health reports must contain at most one entry per vehicle."));
        } else {
            reportedMembers.insert(id);
            seenReports.insert(id);
        }
        const QString state = firstValue(report, QStringList()
            << QStringLiteral("healthState") << QStringLiteral("state") << QStringLiteral("status"))
            .toString().trimmed().toLower();
        bool online = true;
        bool onlineKnown = false;
        if (report.contains(QStringLiteral("online"))) {
            if (report.value(QStringLiteral("online")).type() != QVariant::Bool) {
                addInputError(QStringLiteral("invalid_health_online_flag"),
                              QStringLiteral("Health report online must be a boolean."));
            } else {
                online = report.value(QStringLiteral("online")).toBool();
                onlineKnown = true;
            }
        } else {
            addInputError(QStringLiteral("missing_health_online_flag"),
                          QStringLiteral("Health report online must be explicitly provided as a boolean."));
        }
        const QString normalizedState = normalizedHealthState(state);
        if (!onlineKnown || !online || normalizedState == QStringLiteral("unavailable")) {
            ++unavailableHealth;
            addRisk(&risks, QStringLiteral("vehicle_health_unavailable"), QStringLiteral("critical"),
                    QStringLiteral("Vehicle %1 is offline or reports a critical health state.").arg(id.isEmpty() ? QStringLiteral("unknown") : id),
                    75.0, QStringLiteral("health"));
            probability *= 0.65;
            appendUnique(&blockingReasons, QStringLiteral("vehicle_health_unavailable"));
            alternatives << QStringLiteral("移除故障机并使用备用机或拆分任务");
        } else if (normalizedState == QStringLiteral("degraded")) {
            ++degradedHealth;
            addRisk(&risks, QStringLiteral("vehicle_health_degraded"), QStringLiteral("major"),
                    QStringLiteral("Vehicle %1 is connected but its health state requires operator review.").arg(id.isEmpty() ? QStringLiteral("unknown") : id),
                    55.0, QStringLiteral("health"));
            probability *= 0.82;
            appendUnique(&blockingReasons, QStringLiteral("vehicle_health_degraded"));
            alternatives << QStringLiteral("降低任务规模或先恢复该机健康状态");
        } else if (normalizedState == QStringLiteral("unknown")
                   || state == QStringLiteral("unverified")) {
            ++unknownHealth;
            addRisk(&risks, QStringLiteral("vehicle_health_unknown"), QStringLiteral("major"),
                    QStringLiteral("Vehicle %1 has no verified health state.").arg(id.isEmpty() ? QStringLiteral("unknown") : id),
                    45.0, QStringLiteral("health"));
            probability *= 0.82;
            alternatives << QStringLiteral("先完成健康检查，再重新进行任务审阅");
        } else {
            if (!id.isEmpty()) {
                healthyMembers.insert(id);
            }
        }
    }

    if (!plannedMembers.isEmpty()) {
        QSet<QString> extraMembers = reportedMembers;
        for (const QString& plannedId : plannedMembers) extraMembers.remove(plannedId);
        if (!extraMembers.isEmpty()) {
            addInputError(QStringLiteral("health_report_extra_vehicle"),
                          QStringLiteral("Health reports must not contain vehicles outside the reviewed mission assignment."));
        }
    }

    if (!members.isEmpty() && reports.isEmpty()) {
        addRisk(&risks, QStringLiteral("health_evidence_missing"), QStringLiteral("major"),
                QStringLiteral("No per-vehicle health snapshot was supplied for the planned members."),
                55.0, QStringLiteral("health"));
        probability *= 0.55;
        appendUnique(&blockingReasons, QStringLiteral("health_evidence_missing"));
        alternatives << QStringLiteral("获取所有成员的最新健康快照");
    } else if (!plannedMembers.isEmpty()) {
        QSet<QString> missingMembers = plannedMembers;
        for (const QString& reportId : reportedMembers) missingMembers.remove(reportId);
        if (!missingMembers.isEmpty()) {
            addRisk(&risks, QStringLiteral("planned_vehicle_health_missing"), QStringLiteral("major"),
                    QStringLiteral("One or more planned members have no matching health snapshot."),
                    55.0, QStringLiteral("health"));
            probability *= 0.65;
            appendUnique(&blockingReasons, QStringLiteral("planned_vehicle_health_missing"));
            alternatives << QStringLiteral("补齐所有计划成员的健康快照或将其移入备用组");
        }
    }

    if (!energyPlan.isEmpty()) {
        bool energyFeasible = false;
        if (energyPlan.contains(QStringLiteral("feasible"))) {
            if (energyPlan.value(QStringLiteral("feasible")).type() != QVariant::Bool) {
                addInputError(QStringLiteral("invalid_energy_feasible_flag"),
                              QStringLiteral("Energy plan feasible must be a boolean."));
            } else {
                energyFeasible = energyPlan.value(QStringLiteral("feasible")).toBool();
            }
        } else {
            addInputError(QStringLiteral("missing_energy_feasible_flag"),
                          QStringLiteral("Energy plan feasible must be explicitly provided as a boolean."));
        }
        if (!energyFeasible) {
            addRisk(&risks, QStringLiteral("energy_plan_not_feasible"), QStringLiteral("critical"),
                    QStringLiteral("The supplied energy plan cannot meet mission or reserve requirements."),
                    85.0, QStringLiteral("energy"));
            probability *= 0.35;
            appendUnique(&blockingReasons, QStringLiteral("energy_plan_not_feasible"));
            alternatives << QStringLiteral("增加备用机、缩短任务或提高返航余量");
        }
        if (!energyPlan.contains(QStringLiteral("parametersValid"))) {
            addInputError(QStringLiteral("missing_energy_parameters_flag"),
                          QStringLiteral("Energy plan parametersValid must be explicitly provided as a boolean."));
        } else if (energyPlan.value(QStringLiteral("parametersValid")).type() != QVariant::Bool) {
            addInputError(QStringLiteral("invalid_energy_parameters_flag"),
                          QStringLiteral("Energy plan parametersValid must be a boolean."));
        } else if (!energyPlan.value(QStringLiteral("parametersValid")).toBool()) {
            addInputError(QStringLiteral("energy_parameters_invalid"),
                          QStringLiteral("Energy plan parameters are not valid."));
        }
        bool boundAssignments = false;
        if (energyPlan.contains(QStringLiteral("boundAssignments"))) {
            const QVariant value = energyPlan.value(QStringLiteral("boundAssignments"));
            if (value.isNull() || value.type() != QVariant::Bool) {
                addInputError(QStringLiteral("invalid_energy_binding_flag"),
                              QStringLiteral("Energy plan boundAssignments must be a boolean."));
            } else {
                boundAssignments = value.toBool();
            }
        }
        if (boundAssignments) {
            const BoundEnergyIdentity identity = boundEnergyIdentity(missionPlan, energyPlan);
            if (!identity.valid) {
                addInputError(identity.error,
                    QStringLiteral("Bound energy rows and eligible selections must preserve the exact mission primary and reserve identities."));
            } else {
                if (!identity.primaryComplete) {
                    addRisk(&risks, QStringLiteral("assigned_vehicle_energy_ineligible"), QStringLiteral("critical"),
                        QStringLiteral("One or more original mission members are not energy-eligible; eligible reserves do not replace those assignments."),
                        85.0, QStringLiteral("energy"));
                    appendUnique(&blockingReasons, QStringLiteral("assigned_vehicle_energy_ineligible"));
                    probability *= 0.35;
                    alternatives << QStringLiteral("补齐原成员能量证据、缩短对应航线，或人工重新分配后重新审阅");
                    if (energyFeasible) {
                        addInputError(QStringLiteral("energy_feasibility_identity_mismatch"),
                            QStringLiteral("A bound energy plan cannot be feasible when any original primary is ineligible."));
                    }
                }
                if (!identity.reserveComplete) {
                    addRisk(&risks, QStringLiteral("planned_reserve_energy_ineligible"), QStringLiteral("major"),
                        QStringLiteral("One or more designated reserves lack energy eligibility; other aircraft do not fill those reserved identities."),
                        55.0, QStringLiteral("energy"));
                    appendUnique(&blockingReasons, QStringLiteral("planned_reserve_energy_ineligible"));
                    probability *= 0.72;
                }
            }
        } else if (energyPlan.contains(QStringLiteral("selectedVehicleIds"))) {
            // Legacy independent scheduling remains supported. It is not a
            // bound per-member ledger, but an explicit different selection
            // must not silently stand in for this mission's original IDs.
            QSet<QString> selectedIds;
            if (!energyIdSet(energyPlan.value(QStringLiteral("selectedVehicleIds")), false, &selectedIds)) {
                addInputError(QStringLiteral("invalid_energy_selected_ids"),
                              QStringLiteral("Energy selections must contain unique valid vehicle identifiers."));
            } else if (selectedIds != memberIds) {
                addRisk(&risks, QStringLiteral("energy_selection_mission_mismatch"), QStringLiteral("critical"),
                    QStringLiteral("The independent energy selection differs from the actual mission member identities."),
                    85.0, QStringLiteral("energy"));
                appendUnique(&blockingReasons, QStringLiteral("energy_selection_mission_mismatch"));
                probability *= 0.35;
                alternatives << QStringLiteral("按已编译的原成员重新生成逐机能量账单，不以人数相同替代身份一致");
            }
        }
        bool requireEnergyEvidence = false;
        if (energyPlan.contains(QStringLiteral("requireEnergyEvidence"))) {
            if (energyPlan.value(QStringLiteral("requireEnergyEvidence")).type() != QVariant::Bool) {
                addInputError(QStringLiteral("invalid_energy_evidence_flag"),
                              QStringLiteral("Energy plan requireEnergyEvidence must be a boolean."));
            } else {
                requireEnergyEvidence = energyPlan.value(QStringLiteral("requireEnergyEvidence")).toBool();
            }
        }
        if (!requireEnergyEvidence) {
            addRisk(&risks, QStringLiteral("energy_evidence_unverified"), QStringLiteral("warning"),
                    QStringLiteral("Energy eligibility was evaluated without requiring explicit endurance evidence."),
                    20.0, QStringLiteral("energy"));
            probability *= 0.92;
        }
        int reserveCount = 0;
        if (energyPlan.contains(QStringLiteral("reserveCount"))
            && !finiteNonNegativeInteger(energyPlan.value(QStringLiteral("reserveCount")), &reserveCount)) {
            addInputError(QStringLiteral("invalid_energy_reserve_count"),
                          QStringLiteral("Energy plan reserveCount must be a finite non-negative integer."));
        }
        QVariantList reserveIds;
        if (energyPlan.contains(QStringLiteral("reserveVehicleIds"))) {
            const QVariant reserveIdsValue = energyPlan.value(QStringLiteral("reserveVehicleIds"));
            if (reserveIdsValue.type() != QVariant::List
                && reserveIdsValue.type() != QVariant::StringList) {
                addInputError(QStringLiteral("invalid_energy_reserve_ids"),
                              QStringLiteral("Energy plan reserveVehicleIds must be a list."));
            } else {
                reserveIds = listValue(reserveIdsValue);
            }
        }
        if (reserveCount > 0 && reserveIds.size() < reserveCount) {
            addRisk(&risks, QStringLiteral("reserve_shortfall"), QStringLiteral("major"),
                    QStringLiteral("The energy plan has fewer reserves than requested."),
                    55.0, QStringLiteral("energy"));
            probability *= 0.72;
            appendUnique(&blockingReasons, QStringLiteral("reserve_shortfall"));
            alternatives << QStringLiteral("降低编组规模或补充可用备用机");
        }
    } else {
        addRisk(&risks, QStringLiteral("energy_evidence_missing"), QStringLiteral("major"),
                QStringLiteral("No energy/reserve assessment was supplied; completion cannot be confirmed."),
                50.0, QStringLiteral("energy"));
        probability *= 0.65;
        appendUnique(&blockingReasons, QStringLiteral("energy_evidence_missing"));
        alternatives << QStringLiteral("提供任务时长、返航余量和每机剩余续航证据");
    }

    const QVariantList planSignals = listValue(missionPlan.value(QStringLiteral("riskSignals")));
    QVariantList allSignals = planSignals;
    allSignals += riskSignals;
    for (const QVariant& value : allSignals) {
        if (value.type() != QVariant::Map) {
            addInputError(QStringLiteral("invalid_risk_signal"),
                          QStringLiteral("Every risk signal must be a map."));
            continue;
        }
        const QVariantMap signal = value.toMap();
        if (signal.isEmpty()) {
            addInputError(QStringLiteral("invalid_risk_signal"),
                          QStringLiteral("Risk signal maps must not be empty."));
            continue;
        }
        const QString code = firstValue(signal, QStringList()
            << QStringLiteral("code") << QStringLiteral("reason") << QStringLiteral("type"))
            .toString().trimmed();
        const QString effectiveCode = code.isEmpty() ? QStringLiteral("caller_risk_signal") : code;
        if (signal.contains(QStringLiteral("severity"))
            || signal.contains(QStringLiteral("level"))) {
            const QString suppliedSeverity = firstValue(signal, QStringList()
                << QStringLiteral("severity") << QStringLiteral("level"))
                .toString().trimmed().toLower();
            if (suppliedSeverity != QStringLiteral("critical")
                && suppliedSeverity != QStringLiteral("fatal")
                && suppliedSeverity != QStringLiteral("blocking")
                && suppliedSeverity != QStringLiteral("error")
                && suppliedSeverity != QStringLiteral("major")
                && suppliedSeverity != QStringLiteral("high")
                && suppliedSeverity != QStringLiteral("minor")
                && suppliedSeverity != QStringLiteral("info")
                && suppliedSeverity != QStringLiteral("notice")
                && suppliedSeverity != QStringLiteral("warning")) {
                addInputError(QStringLiteral("invalid_risk_severity"),
                              QStringLiteral("Risk signal severity must be a recognized level."));
            }
        }
        const QString severity = severityFor(signal);
        const bool hasWeight = signal.contains(QStringLiteral("weight"));
        const double rawWeight = finiteNumber(signal.value(QStringLiteral("weight")),
                                              std::numeric_limits<double>::quiet_NaN());
        if (hasWeight && (!qIsFinite(rawWeight) || rawWeight < 0.0 || rawWeight > 100.0)) {
            addInputError(QStringLiteral("invalid_risk_weight"),
                          QStringLiteral("Risk signal weight must be finite and within 0..100."));
        }
        const double weight = finiteNumber(signal.value(QStringLiteral("weight")),
                                           severity == QStringLiteral("critical") ? 70.0
                                           : severity == QStringLiteral("major") ? 40.0 : 15.0);
        const QString detail = firstValue(signal, QStringList()
            << QStringLiteral("detail") << QStringLiteral("message") << QStringLiteral("explanation"))
            .toString().trimmed();
        addRisk(&risks, effectiveCode,
                severity, detail.isEmpty() ? QStringLiteral("Caller supplied a mission risk signal.") : detail,
                weight, QStringLiteral("riskSignal"));
        probability *= qBound(0.35, 1.0 - qBound(0.0, weight, 100.0) / 250.0, 1.0);
        if (severityRank(severity) >= 2) appendUnique(&blockingReasons, effectiveCode);
    }

    if (unavailableHealth > 0 || unknownHealth > 0 || degradedHealth > 0) {
        appendUnique(&alternatives, QStringLiteral("按健康状态重新分组，保留人工确认"));
    }
    if (alternatives.isEmpty()) alternatives << QStringLiteral("保持人工确认，先在预演或 Mock 环境核对任务结果");
    probability = qBound(0.0, probability, 1.0);

    const QVariantMap coverage = coverageEvidence(missionPlan);
    if (!coverage.value(QStringLiteral("valid"), true).toBool()) {
        addInputError(QStringLiteral("invalid_coverage_evidence"),
                      QStringLiteral("Coverage evidence must be finite and within 0..100 percent."));
    }
    int worstRank = 0;
    QString worstSeverity = QStringLiteral("info");
    QString worstOutcome = QStringLiteral("task_can_continue_with_operator_review");
    for (const QVariant& value : risks) {
        const QVariantMap risk = value.toMap();
        const QString severity = risk.value(QStringLiteral("severity")).toString();
        const int rank = severityRank(severity);
        if (rank > worstRank) {
            worstRank = rank;
            worstSeverity = severity;
            worstOutcome = rank >= 3 ? QStringLiteral("task_aborted_or_safe_hold")
                         : rank == 2 ? QStringLiteral("partial_coverage_or_reassignment")
                                     : QStringLiteral("operator_review_required");
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("evaluatorVersion"), QStringLiteral("0.1.0"));
    result.insert(QStringLiteral("missionId"), missionPlan.value(QStringLiteral("planId"),
                                                                  missionPlan.value(QStringLiteral("taskId"))));
    result.insert(QStringLiteral("worstCaseOutcome"), worstOutcome);
    result.insert(QStringLiteral("worstCaseSeverity"), worstSeverity);
    result.insert(QStringLiteral("completionProbabilityEstimate"), probability);
    result.insert(QStringLiteral("completionProbabilityPercentEstimate"), probability * 100.0);
    result.insert(QStringLiteral("completionProbabilityIsEstimate"), true);
    result.insert(QStringLiteral("inputValid"), inputValid);
    result.insert(QStringLiteral("probabilityModel"), QStringLiteral("deterministic_heuristic_not_certification"));
    result.insert(QStringLiteral("coverageRateEstimatePercent"), coverage.value(QStringLiteral("ratePercent"), -1.0));
    result.insert(QStringLiteral("coverageEvidenceAvailable"), coverage.value(QStringLiteral("available"), false));
    result.insert(QStringLiteral("coverageEvidenceMethod"), coverage.value(QStringLiteral("method")));
    result.insert(QStringLiteral("riskItems"), risks);
    result.insert(QStringLiteral("blockingReasons"), QVariant::fromValue(blockingReasons));
    result.insert(QStringLiteral("alternatives"), QVariant::fromValue(alternatives));
    result.insert(QStringLiteral("healthReportCount"), reports.size());
    result.insert(QStringLiteral("healthyVehicleCount"), healthyMembers.size());
    result.insert(QStringLiteral("unknownHealthCount"), unknownHealth);
    result.insert(QStringLiteral("unavailableHealthCount"), unavailableHealth);
    result.insert(QStringLiteral("degradedHealthCount"), degradedHealth);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("requiresOperatorReview"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("missionMutated"), false);
    result.insert(QStringLiteral("commandDispatch"), QStringLiteral("not-released"));
    result.insert(QStringLiteral("limitations"), QStringList()
        << QStringLiteral("This evaluator does not run a simulator, collision checker, or flight controller.")
        << QStringLiteral("The probability is a heuristic estimate from supplied evidence, not a guarantee.")
        << QStringLiteral("Missing coverage evidence is reported as unknown, never as complete coverage."));

    _lastAssessment = result;
    emit assessmentChanged();
    return result;
}

void FleetMissionRiskEvaluator::clear()
{
    if (_lastAssessment.isEmpty()) return;
    _lastAssessment.clear();
    emit assessmentChanged();
}

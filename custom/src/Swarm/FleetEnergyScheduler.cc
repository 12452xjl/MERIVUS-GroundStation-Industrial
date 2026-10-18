#include "FleetEnergyScheduler.h"
#include "FleetCapabilityEvidence.h"
#include "FleetNativeBatteryEvidence.h"

#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

struct Candidate {
    int id = 0;
    double batteryPercent = -1.0;
    double availableSeconds = -1.0;
    double missionSeconds = 0.0;
    double returnSeconds = 0.0;
    double requiredSeconds = 0.0;
    double marginSeconds = -1.0;
    double projectedBattery = -1.0;
    double score = -1.0;
    bool energyKnown = false;
    bool projectionKnown = false;
    bool eligible = false;
    bool replacementRecommended = false;
    QString reason;
    QString energyBasis = QStringLiteral("unknown");
    QString projectionBasis = QStringLiteral("unknown");
    QVariantMap details;
    QVariantMap nativeBatteryEvidence;
    bool requirePerformanceEvidence = false;
    QVariantMap declaredPerformanceEvidence;
};

bool numericVariant(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return true;
    default:
        return false;
    }
}

struct NumberRead {
    bool present = false;
    bool valid = true;
    double value = 0.0;
};

NumberRead optionalNumber(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        NumberRead result;
        result.present = true;
        if (!numericVariant(value) || value.isNull()) {
            result.valid = false;
            return result;
        }
        bool ok = false;
        result.value = value.toDouble(&ok);
        result.valid = ok && qIsFinite(result.value);
        return result;
    }
    return {};
}

struct IntegerRead {
    bool present = false;
    bool valid = true;
    int value = 0;
};

IntegerRead optionalInteger(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        IntegerRead result;
        result.present = true;
        if (!numericVariant(value) || value.isNull()) {
            result.valid = false;
            return result;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qFloor(number) != number
            || number < static_cast<double>(std::numeric_limits<int>::min())
            || number > static_cast<double>(std::numeric_limits<int>::max())) {
            result.valid = false;
            return result;
        }
        result.value = static_cast<int>(number);
        return result;
    }
    return {};
}

bool online(const QVariantMap& vehicle)
{
    const QVariant value = vehicle.value(QStringLiteral("online"));
    return value.type() == QVariant::Bool && value.toBool();
}

QString healthState(const QVariantMap& vehicle)
{
    return vehicle.value(QStringLiteral("healthState")).toString().trimmed().toLower();
}

QVariantMap rejection(int id, const QString& reason)
{
    QVariantMap result;
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("reason"), reason);
    return result;
}

bool readBool(const QVariantMap& map, const QString& key, bool fallback, bool* valid)
{
    if (!map.contains(key)) return fallback;
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool || value.isNull()) {
        *valid = false;
        return fallback;
    }
    return value.toBool();
}

bool boundedDetailsValue(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 8 || ++*nodes > 4096) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > 65536 || !boundedDetailsValue(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > 4096) return false;
        for (const QVariant& child : list) if (!boundedDetailsValue(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        const QString string = value.toString();
        if (string.size() > 65536 || QString::fromUtf8(string.toUtf8()) != string) return false;
        *bytes += string.toUtf8().size();
        return *bytes <= 65536;
    }
    if (value.type() == QVariant::Bool) return !value.isNull();
    return numericVariant(value) && !value.isNull() && qIsFinite(value.toDouble())
        && qAbs(value.toDouble()) <= 9007199254740991.0;
}

bool boundedDetails(const QVariant& value)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    int nodes = 0;
    qint64 bytes = 0;
    return boundedDetailsValue(value, 0, &nodes, &bytes)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= 65536;
}

bool exactBool(const QVariantMap& map, const QString& key, bool* result)
{
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool || value.isNull()) return false;
    *result = value.toBool();
    return true;
}

enum class ReserveBudgetKind { Legacy, Complete, Unknown, Invalid };

ReserveBudgetKind reserveBudgetKind(const QVariantMap& budget)
{
    const QVariantMap details = budget.value(QStringLiteral("details")).toMap();
    if (!details.contains(QStringLiteral("reserveEnergyPlanRequired"))
        && !details.contains(QStringLiteral("reserveEnergyPlanComplete"))) return ReserveBudgetKind::Legacy;
    bool required = false, complete = false;
    const QVariant reason = details.value(QStringLiteral("reason"));
    if (budget.value(QStringLiteral("group")).toString() != QStringLiteral("reserve")
        || !exactBool(details, QStringLiteral("reserveEnergyPlanRequired"), &required) || !required
        || !exactBool(details, QStringLiteral("reserveEnergyPlanComplete"), &complete)
        || details.value(QStringLiteral("basis")).type() != QVariant::String
        || details.value(QStringLiteral("basis")).toString() != QStringLiteral("compiled-reserve-route-budget")
        || reason.type() != QVariant::String || reason.isNull()
        || (complete ? !reason.toString().isEmpty() : reason.toString().isEmpty())) return ReserveBudgetKind::Invalid;
    return complete ? ReserveBudgetKind::Complete : ReserveBudgetKind::Unknown;
}

double memberRequiredSeconds(double mission, double returning)
{
    return mission < 0.0 || returning < 0.0 ? -1.0 : mission + returning;
}

QVariantMap assignmentFor(const Candidate& candidate, const QString& group)
{
    QVariantMap result{{QStringLiteral("vehicleId"), candidate.id}, {QStringLiteral("group"), group},
        {QStringLiteral("batteryPercent"), candidate.batteryPercent},
        {QStringLiteral("availableFlightSeconds"), candidate.availableSeconds},
        {QStringLiteral("requiredFlightSeconds"), candidate.requiredSeconds},
        {QStringLiteral("marginSeconds"), candidate.marginSeconds},
        {QStringLiteral("projectedBatteryPercent"), candidate.projectedBattery},
        {QStringLiteral("replacementRecommended"), candidate.replacementRecommended},
        {QStringLiteral("energyEvidenceKnown"), candidate.energyKnown},
        {QStringLiteral("eligible"), candidate.eligible}, {QStringLiteral("reason"), candidate.reason},
        {QStringLiteral("missionDurationSeconds"), candidate.missionSeconds},
        {QStringLiteral("returnReserveSeconds"), candidate.returnSeconds},
        {QStringLiteral("budgetDetails"), candidate.details}, {QStringLiteral("energyBasis"), candidate.energyBasis},
        {QStringLiteral("projectionKnown"), candidate.projectionKnown},
        {QStringLiteral("projectionBasis"), candidate.projectionBasis},
        {QStringLiteral("nativeBatteryEvidence"), candidate.nativeBatteryEvidence}};
    if (candidate.requirePerformanceEvidence) {
        result.insert(QStringLiteral("requirePerformanceEvidence"), true);
        result.insert(QStringLiteral("declaredPerformanceEvidence"), candidate.declaredPerformanceEvidence);
        result.insert(QStringLiteral("declaredPerformanceEvidenceUsable"), !candidate.declaredPerformanceEvidence.isEmpty());
    }
    return result;
}

QVariantMap drainAdjustment(int id, const FleetNativeBatteryEvidence::DrainRead& read)
{
    return {{QStringLiteral("vehicleId"), id}, {QStringLiteral("present"), read.present},
        {QStringLiteral("inputValid"), read.valid}, {QStringLiteral("usable"), read.usable},
        {QStringLiteral("reason"), read.usable ? QStringLiteral("member-not-evaluated") : read.reason},
        {QStringLiteral("evidence"), read.valid ? read.evidence : QVariantMap()},
        {QStringLiteral("availableKnownBefore"), false}, {QStringLiteral("availableFlightSecondsBefore"), -1.0},
        {QStringLiteral("availableFlightSecondsAfter"), -1.0}, {QStringLiteral("availableLowered"), false},
        {QStringLiteral("projectionKnownBefore"), false}, {QStringLiteral("projectedBatteryPercentBefore"), -1.0},
        {QStringLiteral("projectedBatteryPercentAfter"), -1.0}, {QStringLiteral("projectionLowered"), false}};
}

}

FleetEnergyScheduler::FleetEnergyScheduler(QObject* parent)
    : QObject(parent)
{
}

QVariantMap FleetEnergyScheduler::plan(const QVariantMap& task,
                                       const QVariantList& fleet)
{
    bool parametersValid = true;
    const IntegerRead requestedRead = optionalInteger(task,
        QStringList() << QStringLiteral("vehicleCount") << QStringLiteral("minVehicleCount"));
    const int requestedCount = requestedRead.present ? requestedRead.value : 1;
    parametersValid = parametersValid && requestedRead.valid && requestedCount >= 1 && requestedCount <= 255;
    const IntegerRead reserveRead = optionalInteger(task, QStringList() << QStringLiteral("reserveCount"));
    const int reserveCount = reserveRead.present ? reserveRead.value : 0;
    parametersValid = parametersValid && reserveRead.valid && reserveCount >= 0 && reserveCount <= 255;

    const NumberRead missionRead = optionalNumber(task,
        QStringList() << QStringLiteral("missionDurationSeconds")
                      << QStringLiteral("taskDurationSeconds"));
    const double missionSeconds = missionRead.present ? missionRead.value : 0.0;
    const NumberRead returnRead = optionalNumber(task,
        QStringList() << QStringLiteral("returnReserveSeconds")
                      << QStringLiteral("returnToHomeReserveSeconds"));
    const double returnReserveSeconds = returnRead.present ? returnRead.value : 120.0;
    // A mission without a positive duration has no defensible energy budget;
    // do not silently treat an omitted duration as a zero-second task.
    parametersValid = parametersValid && missionRead.present && missionRead.valid
        && returnRead.valid && missionSeconds > 0.0 && returnReserveSeconds >= 0.0;

    const NumberRead minimumRead = optionalNumber(task,
        QStringList() << QStringLiteral("minimumBatteryPercent"));
    const double minimumBattery = minimumRead.present ? minimumRead.value : 30.0;
    const NumberRead replacementRead = optionalNumber(task,
        QStringList() << QStringLiteral("replacementThresholdPercent")
                      << QStringLiteral("reserveTriggerBatteryPercent"));
    const double replacementThreshold = replacementRead.present ? replacementRead.value : 35.0;
    parametersValid = parametersValid && minimumRead.valid && replacementRead.valid
        && minimumBattery >= 0.0 && minimumBattery <= 100.0
        && replacementThreshold >= 0.0 && replacementThreshold <= 100.0;

    bool boolValid = true;
    const bool requireEnergyEvidence = readBool(task, QStringLiteral("requireEnergyEvidence"), true, &boolValid);
    parametersValid = parametersValid && boolValid;
    bool performancePolicyValid = true;
    const bool requirePerformanceEvidence = readBool(task, QStringLiteral("requirePerformanceEvidence"), false,
        &performancePolicyValid);
    parametersValid = parametersValid && performancePolicyValid;
    const double requiredSeconds = missionSeconds + returnReserveSeconds;
    parametersValid = parametersValid && qIsFinite(requiredSeconds)
        && requiredSeconds >= 0.0;

    QSet<int> authoritativeIds;
    bool authoritativeIdsValid = true;
    const bool hasAuthoritativeIds = task.contains(QStringLiteral("candidateVehicleIds"));
    if (hasAuthoritativeIds) {
        const QVariant value = task.value(QStringLiteral("candidateVehicleIds"));
        if (value.type() != QVariant::List || value.toList().isEmpty()) {
            authoritativeIdsValid = false;
        } else {
            for (const QVariant& item : value.toList()) {
                if (!numericVariant(item)) {
                    authoritativeIdsValid = false;
                    break;
                }
                bool ok = false;
                const double number = item.toDouble(&ok);
                if (!ok || !qIsFinite(number) || qFloor(number) != number
                    || number < 1.0 || number > 255.0) {
                    authoritativeIdsValid = false;
                    break;
                }
                const int id = static_cast<int>(number);
                if (authoritativeIds.contains(id)) {
                    authoritativeIdsValid = false;
                    break;
                }
                authoritativeIds.insert(id);
            }
        }
    }
    parametersValid = parametersValid && authoritativeIdsValid;

    const bool boundAssignments = task.contains(QStringLiteral("memberEnergyBudgets"));
    const QVariant budgetValue = task.value(QStringLiteral("memberEnergyBudgets"));
    QVariantList budgets;
    QMap<int, QVariantMap> budgetsById;
    bool memberBudgetsValid = true;
    if (boundAssignments) {
        memberBudgetsValid = budgetValue.type() == QVariant::List && !budgetValue.isNull();
        if (memberBudgetsValid) budgets = budgetValue.toList();
        memberBudgetsValid = memberBudgetsValid && !budgets.isEmpty() && budgets.size() <= 255
            && hasAuthoritativeIds && authoritativeIdsValid;
        QSet<int> budgetIds;
        int primaryCount = 0, actualReserveCount = 0;
        double totalBudgetSeconds = 0.0;
        if (memberBudgetsValid) for (const QVariant& item : budgets) {
            if (item.type() != QVariant::Map) { memberBudgetsValid = false; break; }
            const QVariantMap budget = item.toMap();
            const IntegerRead id = optionalInteger(budget, {QStringLiteral("vehicleId")});
            const NumberRead mission = optionalNumber(budget, {QStringLiteral("missionDurationSeconds")});
            const NumberRead reserve = optionalNumber(budget, {QStringLiteral("returnReserveSeconds")});
            const QVariant groupValue = budget.value(QStringLiteral("group"));
            const QString group = groupValue.toString();
            const ReserveBudgetKind reserveKind = reserveBudgetKind(budget);
            const bool unknownReserve = reserveKind == ReserveBudgetKind::Unknown;
            if (budget.size() != 5 || !id.present || !id.valid || id.value < 1 || id.value > 255
                || budgetIds.contains(id.value) || groupValue.type() != QVariant::String || groupValue.isNull()
                || (group != QStringLiteral("primary") && group != QStringLiteral("reserve"))
                || reserveKind == ReserveBudgetKind::Invalid
                || !mission.present || !mission.valid || (unknownReserve ? mission.value != -1.0 : mission.value <= 0.0)
                || !reserve.present || !reserve.valid || (unknownReserve ? reserve.value != -1.0 : reserve.value < 0.0)
                || !qIsFinite(mission.value + reserve.value) || !boundedDetails(budget.value(QStringLiteral("details")))) {
                memberBudgetsValid = false;
                break;
            }
            budgetIds.insert(id.value);
            budgetsById.insert(id.value, budget);
            if (!unknownReserve) totalBudgetSeconds += mission.value + reserve.value;
            if (!qIsFinite(totalBudgetSeconds)) { memberBudgetsValid = false; break; }
            primaryCount += group == QStringLiteral("primary") ? 1 : 0;
            actualReserveCount += group == QStringLiteral("reserve") ? 1 : 0;
        }
        memberBudgetsValid = memberBudgetsValid && budgetIds == authoritativeIds
            && primaryCount == requestedCount && actualReserveCount == reserveCount;
        if (!memberBudgetsValid) { budgets.clear(); budgetsById.clear(); }
        parametersValid = parametersValid && memberBudgetsValid;
    }

    QVariantList rejected;
    QList<Candidate> candidates;
    QMap<int, Candidate> inspected;
    QMap<int, QVariantMap> batterySources;
    QMap<int, QVariantMap> declaredPerformanceSources;
    QMap<int, QVariantMap> drainAdjustments;
    QList<int> drainOrder;
    QSet<int> seenIds;
    QSet<int> duplicateIds;
    for (const QVariant& value : fleet) {
        if (!performancePolicyValid || (boundAssignments && !parametersValid)) break;
        if (value.type() != QVariant::Map) {
            rejected.append(rejection(0, QStringLiteral("invalid_vehicle_entry")));
            continue;
        }
        const QVariantMap vehicle = value.toMap();
        const IntegerRead idRead = optionalInteger(vehicle,
            QStringList() << QStringLiteral("systemId") << QStringLiteral("vehicleId") << QStringLiteral("id"));
        const int id = idRead.present ? idRead.value : 0;
        if (!idRead.valid || id < 1 || id > 255) {
            rejected.append(rejection(id, QStringLiteral("invalid_vehicle_id")));
            continue;
        }
        if (seenIds.contains(id)) {
            duplicateIds.insert(id);
            if (drainAdjustments.contains(id) || vehicle.contains(QStringLiteral("nativeBatteryDrainEvidence"))) {
                FleetNativeBatteryEvidence::DrainRead duplicate;
                duplicate.present = true;
                duplicate.valid = false;
                duplicate.reason = QStringLiteral("duplicate_vehicle_id");
                if (!drainAdjustments.contains(id)) drainOrder.append(id);
                drainAdjustments.insert(id, drainAdjustment(id, duplicate));
            }
            rejected.append(rejection(id, QStringLiteral("duplicate_vehicle_id")));
            continue;
        }
        seenIds.insert(id);
        if (hasAuthoritativeIds && (!authoritativeIdsValid || !authoritativeIds.contains(id))) {
            rejected.append(rejection(id, QStringLiteral("vehicle_not_in_authoritative_assignment")));
            continue;
        }
        QVariantMap declaredPerformance;
        const bool declaredPerformanceCurrent = requirePerformanceEvidence
            && FleetCapabilityEvidence::currentDeclaredPerformance(vehicle, &declaredPerformance)
            && declaredPerformance.value(QStringLiteral("vehicleId")).toInt() == id;
        if (requirePerformanceEvidence) declaredPerformanceSources.insert(id,
            declaredPerformanceCurrent ? declaredPerformance : QVariantMap());
        const FleetNativeBatteryEvidence::Read nativeBattery = FleetNativeBatteryEvidence::inspect(vehicle, id);
        if (nativeBattery.required && nativeBattery.valid) batterySources.insert(id, nativeBattery.evidence);
        const FleetNativeBatteryEvidence::DrainRead nativeDrain = FleetNativeBatteryEvidence::inspectDrain(vehicle, id);
        if (nativeDrain.present) {
            drainOrder.append(id);
            drainAdjustments.insert(id, drainAdjustment(id, nativeDrain));
            if (!nativeDrain.valid) {
                rejected.append(rejection(id, nativeDrain.reason));
                continue; // A malformed sibling is never silently absent.
            }
        }
        if (nativeBattery.required) {
            if (!nativeBattery.valid || !nativeBattery.usable) {
                rejected.append(rejection(id, nativeBattery.reason));
                continue;
            }
        }
        if (!online(vehicle)) {
            rejected.append(rejection(id, QStringLiteral("offline_or_unknown")));
            continue;
        }
        const QString state = healthState(vehicle);
        if (state == QStringLiteral("critical") || state == QStringLiteral("offline")
            || state == QStringLiteral("failed") || state == QStringLiteral("fault")
            || state == QStringLiteral("unhealthy") || state == QStringLiteral("emergency")
            || state == QStringLiteral("lost") || state == QStringLiteral("disconnected")) {
            rejected.append(rejection(id, QStringLiteral("health_critical")));
            continue;
        }
        if (state == QStringLiteral("degraded") || state == QStringLiteral("warning")
            || state == QStringLiteral("limited")) {
            rejected.append(rejection(id, QStringLiteral("health_degraded")));
            continue;
        }
        if (state.isEmpty() || state == QStringLiteral("unknown")) {
            rejected.append(rejection(id, QStringLiteral("health_unknown")));
            continue;
        }

        Candidate candidate;
        candidate.id = id;
        candidate.requirePerformanceEvidence = requirePerformanceEvidence;
        candidate.declaredPerformanceEvidence = declaredPerformanceCurrent ? declaredPerformance : QVariantMap();
        const QVariantMap budget = budgetsById.value(id);
        candidate.missionSeconds = boundAssignments ? budget.value(QStringLiteral("missionDurationSeconds")).toDouble() : missionSeconds;
        candidate.returnSeconds = boundAssignments ? budget.value(QStringLiteral("returnReserveSeconds")).toDouble() : returnReserveSeconds;
        candidate.requiredSeconds = memberRequiredSeconds(candidate.missionSeconds, candidate.returnSeconds);
        candidate.details = budget.value(QStringLiteral("details")).toMap();
        candidate.nativeBatteryEvidence = nativeBattery.evidence;
        if (boundAssignments && reserveBudgetKind(budget) == ReserveBudgetKind::Unknown) {
            candidate.reason = QStringLiteral("reserve_energy_plan_incomplete");
            inspected.insert(id, candidate);
            rejected.append(rejection(id, candidate.reason));
            continue;
        }
        const NumberRead batteryRead = optionalNumber(vehicle,
            QStringList() << QStringLiteral("batteryPercent") << QStringLiteral("battery"));
        if (!batteryRead.valid || (batteryRead.present
            && ((batteryRead.value < 0.0 && batteryRead.value != -1.0) || batteryRead.value > 100.0))) {
            rejected.append(rejection(id, QStringLiteral("invalid_battery_evidence")));
            continue;
        }
        candidate.batteryPercent = batteryRead.present ? batteryRead.value : -1.0;

        if (requirePerformanceEvidence && (!declaredPerformanceCurrent
            || declaredPerformance.value(QStringLiteral("enduranceSeconds")).toDouble() <= 0.0
            || candidate.batteryPercent < 0.0)) {
            candidate.reason = !declaredPerformanceCurrent ? QStringLiteral("performance_declaration_not_current")
                : declaredPerformance.value(QStringLiteral("enduranceSeconds")).toDouble() <= 0.0
                    ? QStringLiteral("declared_endurance_unknown") : QStringLiteral("battery_evidence_missing");
            inspected.insert(id, candidate);
            rejected.append(rejection(id, candidate.reason));
            continue;
        }
        // Legacy estimates retain their original alias/validation contract.
        // Strict planning ignores unbound drain/direct/endurance metadata;
        // only validated CURRENT native direct time may further lower the
        // declaration-based available time, never satisfy it on its own.
        const NumberRead directRead = requirePerformanceEvidence ? NumberRead() : optionalNumber(vehicle,
            QStringList() << QStringLiteral("availableFlightSeconds")
                          << QStringLiteral("remainingFlightSeconds"));
        const NumberRead consumptionRead = requirePerformanceEvidence ? NumberRead() : optionalNumber(vehicle,
            QStringList() << QStringLiteral("consumptionPercentPerMinute")
                          << QStringLiteral("batteryConsumptionPercentPerMinute")
                          << QStringLiteral("batteryDrainPercentPerMinute"));
        const NumberRead enduranceRead = requirePerformanceEvidence
            ? optionalNumber(declaredPerformance, QStringList() << QStringLiteral("enduranceSeconds"))
            : optionalNumber(vehicle, QStringList() << QStringLiteral("enduranceSeconds") << QStringLiteral("enduranceSec"));
        if (!directRead.valid || !consumptionRead.valid || !enduranceRead.valid
            || (directRead.present && directRead.value < 0.0)
            || (consumptionRead.present && consumptionRead.value <= 0.0)
            || (enduranceRead.present && enduranceRead.value != -1.0 && enduranceRead.value <= 0.0)) {
            rejected.append(rejection(id, QStringLiteral("invalid_energy_evidence")));
            continue;
        }
        const bool strictNativeDirect = requirePerformanceEvidence && nativeBattery.required && nativeBattery.valid
            && nativeBattery.usable && nativeBattery.evidence.value(QStringLiteral("remainingSecondsKnown")).toBool();
        const double directAvailable = strictNativeDirect
            ? nativeBattery.evidence.value(QStringLiteral("remainingFlightSeconds")).toDouble()
            : directRead.present ? directRead.value : -1.0;
        const double consumptionPerMinute = consumptionRead.present ? consumptionRead.value : -1.0;
        const double endurance = enduranceRead.present ? enduranceRead.value : -1.0;
        bool estimatesValid = true;
        const auto includeEstimate = [&candidate, &estimatesValid](double seconds, const QString& basis) {
            if (!qIsFinite(seconds) || seconds < 0.0) { estimatesValid = false; return; }
            if (!candidate.energyKnown || seconds < candidate.availableSeconds) {
                candidate.availableSeconds = seconds;
                candidate.energyBasis = basis;
            }
            candidate.energyKnown = true;
        };
        if (!requirePerformanceEvidence && directAvailable >= 0.0) includeEstimate(directAvailable, nativeBattery.required
            ? QStringLiteral("autopilot-reported-estimate") : QStringLiteral("direct-remaining"));
        if (candidate.batteryPercent >= 0.0 && consumptionPerMinute > 0.0)
            includeEstimate(candidate.batteryPercent / consumptionPerMinute * 60.0, QStringLiteral("percent-drain-rate"));
        if (candidate.batteryPercent >= 0.0 && endurance > 0.0)
            includeEstimate(endurance * (candidate.batteryPercent / 100.0), requirePerformanceEvidence
                ? QStringLiteral("current-declared-full-battery-endurance") : QStringLiteral("full-battery-endurance"));
        if (strictNativeDirect) includeEstimate(directAvailable, QStringLiteral("autopilot-reported-estimate"));
        if (!estimatesValid) {
            rejected.append(rejection(id, QStringLiteral("invalid_energy_evidence")));
            continue;
        }
        if (nativeDrain.present) {
            QVariantMap adjustment = drainAdjustments.value(id);
            adjustment.insert(QStringLiteral("availableKnownBefore"), candidate.energyKnown);
            adjustment.insert(QStringLiteral("availableFlightSecondsBefore"), candidate.availableSeconds);
            // Do not call includeEstimate: that would turn unknown into known.
            if (nativeDrain.usable && candidate.energyKnown
                && nativeDrain.remainingFlightSeconds < candidate.availableSeconds) {
                candidate.availableSeconds = nativeDrain.remainingFlightSeconds;
                candidate.energyBasis = QStringLiteral("native-soc-decline-window");
                adjustment.insert(QStringLiteral("availableLowered"), true);
            }
            adjustment.insert(QStringLiteral("availableFlightSecondsAfter"), candidate.availableSeconds);
            drainAdjustments.insert(id, adjustment);
        }
        if (candidate.energyKnown) {
            candidate.marginSeconds = candidate.availableSeconds - candidate.requiredSeconds;
        }

        const NumberRead linkRead = optionalNumber(vehicle,
            QStringList() << QStringLiteral("linkQualityPercent") << QStringLiteral("linkQuality"));
        if (!linkRead.valid || (linkRead.present && ((linkRead.value < 0.0 && linkRead.value != -1.0) || linkRead.value > 100.0))) {
            rejected.append(rejection(id, QStringLiteral("invalid_link_quality")));
            continue;
        }
        const double link = linkRead.present ? linkRead.value : -1.0;
        double drainPerSecond = -1.0;
        if (consumptionPerMinute > 0.0) {
            drainPerSecond = consumptionPerMinute / 60.0;
            candidate.projectionBasis = QStringLiteral("percent-drain-rate");
        }
        if (endurance > 0.0 && 100.0 / endurance > drainPerSecond) {
            drainPerSecond = 100.0 / endurance;
            candidate.projectionBasis = requirePerformanceEvidence
                ? QStringLiteral("current-declared-full-battery-endurance") : QStringLiteral("full-battery-endurance");
        }
        if (candidate.batteryPercent >= 0.0 && drainPerSecond > 0.0) {
            // Endurance is a full-battery duration: subtract percentage
            // points consumed, not a fraction of the already remaining %.
            candidate.projectedBattery = candidate.batteryPercent - drainPerSecond * candidate.requiredSeconds;
            candidate.projectionKnown = true;
        } else candidate.projectionBasis = QStringLiteral("unknown");
        if (candidate.projectionKnown && !qIsFinite(candidate.projectedBattery)) {
            rejected.append(rejection(id, QStringLiteral("invalid_energy_projection")));
            continue;
        }
        if (candidate.projectionKnown) candidate.projectedBattery = qBound(0.0, candidate.projectedBattery, 100.0);
        if (nativeDrain.present) {
            QVariantMap adjustment = drainAdjustments.value(id);
            adjustment.insert(QStringLiteral("projectionKnownBefore"), candidate.projectionKnown);
            adjustment.insert(QStringLiteral("projectedBatteryPercentBefore"), candidate.projectedBattery);
            if (nativeDrain.usable && candidate.projectionKnown) {
                const double projected = qBound(0.0, candidate.batteryPercent - nativeDrain.percentPerSecond
                    * (candidate.requiredSeconds + static_cast<double>(nativeDrain.ageMs) / 1000.0), 100.0);
                if (projected < candidate.projectedBattery) {
                    candidate.projectedBattery = projected;
                    candidate.projectionBasis = QStringLiteral("native-soc-decline-window");
                    adjustment.insert(QStringLiteral("projectionLowered"), true);
                }
            }
            adjustment.insert(QStringLiteral("projectedBatteryPercentAfter"), candidate.projectedBattery);
            if (nativeDrain.usable) adjustment.insert(QStringLiteral("reason"),
                adjustment.value(QStringLiteral("availableLowered")).toBool()
                    || adjustment.value(QStringLiteral("projectionLowered")).toBool()
                    ? QStringLiteral("conservative-estimate-lowered")
                    : !candidate.energyKnown && !candidate.projectionKnown ? QStringLiteral("existing-estimates-unknown")
                    : QStringLiteral("not-lower-than-existing-estimates"));
            drainAdjustments.insert(id, adjustment);
        }
        candidate.replacementRecommended = candidate.projectionKnown
            && candidate.projectedBattery <= replacementThreshold;
        if (minimumBattery > 0.0 && candidate.batteryPercent < 0.0) candidate.reason = QStringLiteral("battery_evidence_missing");
        else if (candidate.batteryPercent >= 0.0 && candidate.batteryPercent < minimumBattery) candidate.reason = QStringLiteral("battery_below_minimum");
        else if (!candidate.energyKnown && requireEnergyEvidence) candidate.reason = QStringLiteral("energy_evidence_missing");
        else if (candidate.energyKnown && candidate.marginSeconds < 0.0) candidate.reason = QStringLiteral("insufficient_energy_margin");
        if (!candidate.reason.isEmpty()) {
            inspected.insert(id, candidate);
            rejected.append(rejection(id, candidate.reason));
            continue;
        }
        candidate.eligible = true;
        candidate.reason = QStringLiteral("");
        candidate.score = 50.0;
        if (state == QStringLiteral("healthy")) candidate.score += 20.0;
        if (candidate.marginSeconds >= 0.0) candidate.score += qMin(20.0, candidate.marginSeconds / 60.0);
        if (candidate.batteryPercent >= 0.0) candidate.score += qMin(8.0, candidate.batteryPercent * 0.08);
        if (link >= 0.0) candidate.score += qMin(2.0, link * 0.02);
        inspected.insert(id, candidate);
        candidates.append(candidate);
    }

    candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&duplicateIds](const Candidate& value) {
        return duplicateIds.contains(value.id);
    }), candidates.end());
    if (!boundAssignments) {
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
            if (!qFuzzyCompare(left.score + 1.0, right.score + 1.0)) return left.score > right.score;
            return left.id < right.id;
        });
    }

    QVariantList selected;
    QVariantList reserves;
    QVariantList assignments;
    if (boundAssignments && parametersValid) {
        QMap<int, QString> reasons;
        for (const QVariant& value : rejected) {
            const QVariantMap item = value.toMap();
            reasons.insert(item.value(QStringLiteral("vehicleId")).toInt(), item.value(QStringLiteral("reason")).toString());
        }
        for (const QVariant& value : budgets) {
            const QVariantMap budget = value.toMap();
            const int id = budget.value(QStringLiteral("vehicleId")).toInt();
            Candidate candidate = duplicateIds.contains(id) ? Candidate() : inspected.value(id);
            candidate.id = id;
            candidate.requirePerformanceEvidence = requirePerformanceEvidence;
            if (requirePerformanceEvidence) {
                candidate.declaredPerformanceEvidence = duplicateIds.contains(id)
                    ? QVariantMap() : declaredPerformanceSources.value(id);
                // Retain an explicit empty declaration for a missing original
                // member too, not merely the successfully inspected subset.
                declaredPerformanceSources.insert(id, candidate.declaredPerformanceEvidence);
            }
            // Preserve a structurally valid native source even when health,
            // topology or energy eligibility rejected the original member.
            // Never attribute one duplicate row's source to the other row.
            if (!duplicateIds.contains(id)) candidate.nativeBatteryEvidence = batterySources.value(id);
            candidate.missionSeconds = budget.value(QStringLiteral("missionDurationSeconds")).toDouble();
            candidate.returnSeconds = budget.value(QStringLiteral("returnReserveSeconds")).toDouble();
            candidate.requiredSeconds = memberRequiredSeconds(candidate.missionSeconds, candidate.returnSeconds);
            candidate.details = budget.value(QStringLiteral("details")).toMap();
            if (!candidate.eligible) {
                candidate.reason = duplicateIds.contains(id) ? QStringLiteral("duplicate_vehicle_id")
                    : reasons.value(id, QStringLiteral("assigned_vehicle_missing"));
                if (!reasons.contains(id)) rejected.append(rejection(id, candidate.reason));
            }
            const QString group = budget.value(QStringLiteral("group")).toString();
            if (candidate.eligible) {
                if (group == QStringLiteral("primary")) selected.append(id);
                else reserves.append(id);
            }
            assignments.append(assignmentFor(candidate, group));
        }
    } else if (!boundAssignments) {
        for (int index = 0; index < candidates.size(); ++index) {
            const Candidate& candidate = candidates.at(index);
            const bool primary = index < requestedCount;
            const bool reserve = !primary && index < requestedCount + reserveCount;
            if (!primary && !reserve) continue;
            if (primary) selected.append(candidate.id);
            else reserves.append(candidate.id);
            assignments.append(assignmentFor(candidate, primary ? QStringLiteral("primary") : QStringLiteral("reserve")));
        }
    }

    bool shortfallComplete = parametersValid && !assignments.isEmpty();
    int bottleneckVehicleId = 0, unknownEnergyVehicleCount = 0;
    double bottleneckMargin = -1.0, totalShortfall = 0.0;
    for (const QVariant& value : assignments) {
        const QVariantMap item = value.toMap();
        if (!item.value(QStringLiteral("energyEvidenceKnown")).toBool()) {
            shortfallComplete = false;
            ++unknownEnergyVehicleCount;
            continue;
        }
        const double margin = item.value(QStringLiteral("marginSeconds")).toDouble();
        if (!bottleneckVehicleId || margin < bottleneckMargin) {
            bottleneckVehicleId = item.value(QStringLiteral("vehicleId")).toInt();
            bottleneckMargin = margin;
        }
        totalShortfall += qMax(0.0, -margin);
    }
    // A minimum over only the known subset is not the group's bottleneck.
    // The shortfall sum remains a clearly labelled known lower bound.
    const bool bottleneckKnown = shortfallComplete && bottleneckVehicleId != 0;
    if (!bottleneckKnown) { bottleneckVehicleId = 0; bottleneckMargin = -1.0; }

    QVariantList riskSignals;
    QStringList explanations;
    const bool feasible = parametersValid && selected.size() >= requestedCount;
    if (!parametersValid) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 100},
                                       {QStringLiteral("reason"), QStringLiteral("invalid_task_parameters")}});
        explanations << QStringLiteral("任务参数存在类型、范围或有限性错误，已拒绝生成可执行计划");
    }
    if (hasAuthoritativeIds && !authoritativeIdsValid) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 100},
                                       {QStringLiteral("reason"), QStringLiteral("invalid_authoritative_vehicle_ids")}});
        explanations << QStringLiteral("编译/角色分配提供的候选机身份列表无效，已拒绝能量计划");
    }
    if (boundAssignments && !memberBudgetsValid) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 100},
            {QStringLiteral("reason"), QStringLiteral("invalid_member_energy_budgets")}});
        explanations << QStringLiteral("逐机预算必须完整对应原编译主机和备用机；无效预算不降级为重新选机");
    }
    if (!feasible) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 50},
                                       {QStringLiteral("reason"), QStringLiteral("insufficient_energy_eligible_vehicles")}});
        explanations << QStringLiteral("满足任务时长和返航余量的车辆数量不足");
    }
    if (reserveCount > reserves.size()) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 20},
                                       {QStringLiteral("reason"), QStringLiteral("insufficient_energy_reserves")}});
        explanations << QStringLiteral("满足能量门槛的备用机数量不足");
    }
    int replacementCount = 0;
    for (const QVariant& item : assignments) {
        if (item.toMap().value(QStringLiteral("replacementRecommended")).toBool()) ++replacementCount;
    }
    if (replacementCount > 0) {
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 15},
                                       {QStringLiteral("reason"), QStringLiteral("replacement_threshold_reached")}});
        explanations << QStringLiteral("有 %1 架候选机预计在任务末端触发替补阈值").arg(replacementCount);
    }
    if (requireEnergyEvidence) explanations << QStringLiteral("仅接受具备可解释能量证据的车辆");
    if (requirePerformanceEvidence) explanations << QStringLiteral("严格能量估算仅用当前本地工程声明的满电续航与当前电量比例；原生报告剩余时间仅可进一步降低可用时间，不认证真实续航或飞行许可");
    if (boundAssignments && parametersValid) {
        explanations << QStringLiteral("逐机预算保留原主机/备用身份，不用高电量备用机替换原任务成员");
        explanations << (shortfallComplete
            ? QStringLiteral("已知估算的累计续航缺口为 %1 秒（逐机缺口之和，不是整组延误）").arg(totalShortfall)
            : QStringLiteral("有 %1 架预算的续航未知；已知缺口至少 %2 秒，不能把未知当作零缺口").arg(unknownEnergyVehicleCount).arg(totalShortfall));
    }

    QVariantMap result;
    result.insert(QStringLiteral("feasible"), feasible);
    result.insert(QStringLiteral("parametersValid"), parametersValid);
    result.insert(QStringLiteral("requestedVehicleCount"), requestedCount);
    result.insert(QStringLiteral("reserveCount"), reserveCount);
    result.insert(QStringLiteral("missionDurationSeconds"), missionSeconds);
    result.insert(QStringLiteral("returnReserveSeconds"), returnReserveSeconds);
    result.insert(QStringLiteral("requiredFlightSeconds"), requiredSeconds);
    result.insert(QStringLiteral("minimumBatteryPercent"), minimumBattery);
    result.insert(QStringLiteral("replacementThresholdPercent"), replacementThreshold);
    result.insert(QStringLiteral("requireEnergyEvidence"), requireEnergyEvidence);
    if (requirePerformanceEvidence) {
        result.insert(QStringLiteral("requirePerformanceEvidence"), true);
        QVariantList sources;
        for (auto it = declaredPerformanceSources.constBegin(); it != declaredPerformanceSources.constEnd(); ++it)
            sources.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()},
                {QStringLiteral("evidence"), duplicateIds.contains(it.key()) ? QVariantMap() : it.value()}});
        result.insert(QStringLiteral("declaredPerformanceSources"), sources);
    }
    result.insert(QStringLiteral("boundAssignments"), boundAssignments);
    result.insert(QStringLiteral("bottleneckKnown"), bottleneckKnown);
    result.insert(QStringLiteral("bottleneckVehicleId"), bottleneckVehicleId);
    result.insert(QStringLiteral("bottleneckMarginSeconds"), bottleneckMargin);
    result.insert(QStringLiteral("totalShortfallSeconds"), totalShortfall);
    result.insert(QStringLiteral("shortfallComplete"), shortfallComplete);
    result.insert(QStringLiteral("unknownEnergyVehicleCount"), unknownEnergyVehicleCount);
    const QString status = !parametersValid ? QStringLiteral("invalid-input")
        : !shortfallComplete ? QStringLiteral("estimate-incomplete")
        : !feasible ? QStringLiteral("estimate-insufficient")
        : reserves.size() < reserveCount ? QStringLiteral("estimate-reserve-shortfall")
        : QStringLiteral("estimate-feasible");
    result.insert(QStringLiteral("status"), status);
    result.insert(QStringLiteral("reviewStatus"), status);
    if (hasAuthoritativeIds) {
        QVariantList authoritative;
        for (const int id : authoritativeIds) authoritative.append(id);
        std::sort(authoritative.begin(), authoritative.end(), [](const QVariant& left,
                                                                 const QVariant& right) {
            return left.toInt() < right.toInt();
        });
        result.insert(QStringLiteral("candidateVehicleIds"), authoritative);
    }
    result.insert(QStringLiteral("selectedVehicleIds"), selected);
    result.insert(QStringLiteral("reserveVehicleIds"), reserves);
    result.insert(QStringLiteral("assignments"), assignments);
    if (!drainAdjustments.isEmpty()) {
        QVariantList adjustments;
        QSet<int> emitted;
        // Original bound order, or selected order, is retained. Diagnostics
        // for rejected/unselected observed members follow without inventing
        // assignments or attributing one duplicate row's source to another.
        const QVariantList ordered = boundAssignments ? budgets : assignments;
        for (const QVariant& item : ordered) {
            const int id = item.toMap().value(QStringLiteral("vehicleId")).toInt();
            if (drainAdjustments.contains(id) && !emitted.contains(id)) {
                emitted.insert(id);
                adjustments.append(drainAdjustments.value(id));
            }
        }
        for (const int id : drainOrder) if (!emitted.contains(id)) {
            emitted.insert(id);
            adjustments.append(drainAdjustments.value(id));
        }
        result.insert(QStringLiteral("nativeBatteryDrainAdjustments"), adjustments);
        explanations << QStringLiteral("近期原生 SOC 下降窗仅下调已有已知续航或末端电量估算；不是实测续航保证，未知值及严格性能声明要求不由窗口补齐");
    }
    result.insert(QStringLiteral("rejectedVehicles"), rejected);
    result.insert(QStringLiteral("riskSignals"), riskSignals);
    result.insert(QStringLiteral("explanations"), explanations);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("task"), task);
    _lastPlan = result;
    emit planChanged();
    return result;
}

void FleetEnergyScheduler::clear()
{
    if (_lastPlan.isEmpty()) return;
    _lastPlan.clear();
    emit planChanged();
}

#include "FleetCapabilityMatcher.h"
#include "FleetCapabilityEvidence.h"

#include <QMap>
#include <QSet>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

constexpr int kMaxCapabilityCount = 128;
constexpr int kMaxCapabilityLength = 128;
constexpr int kMaxFleetCandidates = 2048;

struct Candidate {
    QVariantMap vehicle;
    QVariantMap declaredPerformanceEvidence;
    int id = 0;
    int score = 0;
    bool relay = false;
};

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
}

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

NumberRead readNumber(const QVariantMap& map, const QStringList& keys,
                      bool registryUnknownIsMissing = false)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        NumberRead result;
        result.present = true;
        const QVariant value = map.value(key);
        if (!value.isValid() || value.isNull() || !numericVariant(value)) {
            result.valid = false;
            return result;
        }
        bool ok = false;
        result.value = value.toDouble(&ok);
        result.valid = ok && qIsFinite(result.value);
        // Only Registry core fields use the exact numeric -1 sentinel.
        // Missing remains subject to the original evidence policy; it is
        // never a measured zero, a threshold pass or a verified capability.
        if (registryUnknownIsMissing && result.valid && result.value == -1.0) {
            result.present = false;
        }
        return result;
    }
    return {};
}

struct IntegerRead {
    bool present = false;
    bool valid = true;
    int value = 0;
};

IntegerRead readInteger(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        IntegerRead result;
        result.present = true;
        const QVariant value = map.value(key);
        if (!numericVariant(value)) {
            result.valid = false;
            return result;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qFloor(number) != number
            || number < 0.0 || number > 255.0) {
            result.valid = false;
            return result;
        }
        result.value = static_cast<int>(number);
        return result;
    }
    return {};
}

bool readBool(const QVariantMap& map, const QString& key, bool fallback = false,
              bool* valid = nullptr)
{
    if (!map.contains(key)) return fallback;
    const QVariant value = map.value(key);
    if (!value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        if (valid) *valid = false;
        return fallback;
    }
    return value.toBool();
}

bool validStringListValue(const QVariant& value)
{
    QStringList values;
    if (value.type() == QVariant::String) {
        if (value.toString().toUtf8().size() > 128LL * 1024LL) return false;
        values = value.toString().split(QChar(','), Qt::SkipEmptyParts);
    } else if (value.type() == QVariant::StringList) {
        values = value.toStringList();
    } else if (value.type() == QVariant::List) {
        if (value.toList().size() > kMaxCapabilityCount) return false;
        for (const QVariant& item : value.toList()) {
            if (item.type() != QVariant::String) return false;
            values.append(item.toString());
        }
    } else {
        return false;
    }
    if (values.size() > kMaxCapabilityCount) return false;
    for (const QString& item : values) {
        if (item.size() > kMaxCapabilityLength) return false;
    }
    return true;
}

}

FleetCapabilityMatcher::FleetCapabilityMatcher(QObject* parent)
    : QObject(parent)
{
}

QStringList FleetCapabilityMatcher::stringList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QStringList();
    }
    if (!validStringListValue(value)) {
        return QStringList();
    }
    if (value.type() == QVariant::StringList) {
        return value.toStringList();
    }
    if (value.type() == QVariant::List) {
        QStringList result;
        for (const QVariant& item : value.toList()) {
            result.append(item.toString());
        }
        return result;
    }
    QStringList result;
    for (const QString& item : value.toString().split(QChar(','), Qt::SkipEmptyParts)) {
        const QString normalizedItem = normalized(item);
        if (!normalizedItem.isEmpty()) {
            result.append(normalizedItem);
        }
    }
    return result;
}

int FleetCapabilityMatcher::vehicleId(const QVariantMap& vehicle)
{
    int resolved = 0;
    bool found = false;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!vehicle.contains(key)) {
            continue;
        }
        const QVariant value = vehicle.value(key);
        // Fleet snapshots are identity evidence, not user text.  Reject
        // numeric strings and aliases that disagree instead of silently
        // selecting the first value and planning for another vehicle.
        if (!numericVariant(value)) {
            return 0;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qFloor(number) != number
            || number < 1.0 || number > 255.0) {
            return 0;
        }
        const int candidate = static_cast<int>(number);
        if (found && candidate != resolved) {
            return 0;
        }
        found = true;
        resolved = candidate;
    }
    return found ? resolved : 0;
}

QStringList FleetCapabilityMatcher::capabilities(const QVariantMap& vehicle,
                                                 bool requireVerifiedCapabilities)
{
    QStringList result;
    QStringList reviewed;
    const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewed);
    // "Verified" here means a local engineering declaration consistent with
    // the snapshot's current review/configuration identity. This pure input
    // check does not authenticate the caller map, Registry or aircraft.
    if (requireVerifiedCapabilities) return reviewCurrent ? reviewed : result;
    const QStringList available = stringList(vehicle.value(QStringLiteral("capabilities")))
        + (reviewCurrent ? reviewed : QStringList());
    for (const QString& capability : available) {
        const QString normalizedCapability = normalized(capability);
        if (!normalizedCapability.isEmpty() && !result.contains(normalizedCapability)) {
            result.append(normalizedCapability);
        }
    }
    // Ordinary planning can use observed convenience fields plus current
    // reviewed declarations. Neither path authenticates installed hardware.
    if (readBool(vehicle, QStringLiteral("canUseGnss"))) result.append(QStringLiteral("gnss"));
    if (readBool(vehicle, QStringLiteral("canUseRtk"))) result.append(QStringLiteral("rtk"));
    if (readBool(vehicle, QStringLiteral("canUseVision"))) result.append(QStringLiteral("vision"));
    if (readBool(vehicle, QStringLiteral("canUseRangeSensor"))) result.append(QStringLiteral("range"));
    if (readBool(vehicle, QStringLiteral("canRelay"))) result.append(QStringLiteral("relay"));
    result.removeDuplicates();
    return result;
}

bool FleetCapabilityMatcher::hasCapability(const QStringList& available, const QString& required)
{
    const QString wanted = normalized(required);
    if (wanted.isEmpty()) {
        return true;
    }
    for (const QString& item : available) {
        if (normalized(item) == wanted) {
            return true;
        }
    }
    return false;
}

QVariantMap FleetCapabilityMatcher::rejection(int id, const QString& reason)
{
    QVariantMap result;
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("reason"), reason);
    return result;
}

QVariantMap FleetCapabilityMatcher::plan(const QVariantMap& task,
                                         const QVariantList& fleet)
{
    bool parametersValid = true;
    if (fleet.size() > kMaxFleetCandidates) {
        parametersValid = false;
    }
    const QVariant requiredValue = task.value(QStringLiteral("requiredCapabilities"));
    QStringList required = stringList(requiredValue);
    if (task.contains(QStringLiteral("requiredCapabilities"))
        && (!requiredValue.isValid() || requiredValue.isNull() || !validStringListValue(requiredValue))) {
        parametersValid = false;
    }
    const IntegerRead requestedRead = readInteger(task,
        QStringList() << QStringLiteral("vehicleCount") << QStringLiteral("minVehicleCount"));
    const int requestedCount = requestedRead.present ? requestedRead.value : 1;
    parametersValid = parametersValid && requestedRead.valid && requestedCount >= 1;
    const IntegerRead reserveRead = readInteger(task, QStringList() << QStringLiteral("reserveCount"));
    const int reserveCount = reserveRead.present ? reserveRead.value : 0;
    parametersValid = parametersValid && reserveRead.valid && reserveCount <= 255;
    const NumberRead minimumBatteryRead = readNumber(task,
        QStringList() << QStringLiteral("minimumBatteryPercent"));
    const double minimumBattery = minimumBatteryRead.present ? minimumBatteryRead.value : 30.0;
    parametersValid = parametersValid && minimumBatteryRead.valid
        && minimumBattery >= 0.0 && minimumBattery <= 100.0;
    const IntegerRead minimumLinkRead = readInteger(task,
        QStringList() << QStringLiteral("minimumLinkQualityPercent"));
    int minimumLink = minimumLinkRead.present ? minimumLinkRead.value : 40;
    parametersValid = parametersValid && minimumLinkRead.valid && minimumLink >= 0 && minimumLink <= 100;
    const NumberRead minimumSpeedRead = readNumber(task,
        QStringList() << QStringLiteral("minimumSpeedMps"));
    const double minimumSpeedMps = minimumSpeedRead.present ? minimumSpeedRead.value : 0.0;
    parametersValid = parametersValid && minimumSpeedRead.valid && minimumSpeedMps >= 0.0
        && minimumSpeedMps <= 9007199254740991.0;
    const NumberRead minimumEnduranceRead = readNumber(task,
        QStringList() << QStringLiteral("minimumEnduranceSeconds"));
    const double minimumEnduranceSeconds = minimumEnduranceRead.present
        ? minimumEnduranceRead.value : 0.0;
    parametersValid = parametersValid && minimumEnduranceRead.valid
        && minimumEnduranceSeconds >= 0.0 && minimumEnduranceSeconds <= 9007199254740991.0;
    bool boolValid = true;
    const bool requirePerformanceEvidence = readBool(task,
        QStringLiteral("requirePerformanceEvidence"), false, &boolValid);
    const bool performanceBoolValid = boolValid;
    boolValid = true;
    const bool leaderRequired = readBool(task, QStringLiteral("leaderRequired"), true, &boolValid);
    parametersValid = parametersValid && performanceBoolValid && boolValid;
    // Configuration evidence is optional for planning/replay.  A mission can
    // explicitly require an approved baseline when operating a safety-critical
    // fleet; this keeps existing tasks compatible while preventing a known
    // modified or unverified airframe from being selected for that mission.
    boolValid = true;
    const bool requireConfigurationBaseline = readBool(task,
        QStringLiteral("requireConfigurationBaseline"), false, &boolValid);
    parametersValid = parametersValid && boolValid;
    // Unknown health is not an implicit pass. A caller must explicitly opt in
    // for planning-only scenarios where an unverified vehicle may be shown as
    // a candidate without being treated as ready for execution.
    boolValid = true;
    const bool allowUnverified = readBool(task, QStringLiteral("allowUnverified"), false, &boolValid);
    parametersValid = parametersValid && boolValid;
    boolValid = true;
    const bool requireVerifiedCapabilities = readBool(
        task, QStringLiteral("requireVerifiedCapabilities"), false, &boolValid);
    parametersValid = parametersValid && boolValid;

    QList<Candidate> candidates;
    QMap<int, QVariantMap> declaredPerformanceSources;
    QVariantList rejected;
    QSet<int> seenIds;
    QSet<int> strictDuplicateIds;
    if (fleet.size() > kMaxFleetCandidates) {
        rejected.append(rejection(0, QStringLiteral("fleet_resource_bounds")));
    }
    const QStringList boolFields = QStringList()
        << QStringLiteral("online") << QStringLiteral("canUseGnss")
        << QStringLiteral("canUseRtk") << QStringLiteral("canUseVision")
        << QStringLiteral("canUseRangeSensor") << QStringLiteral("canRelay");
    for (int fleetIndex = 0;
         parametersValid && fleetIndex < fleet.size() && fleetIndex < kMaxFleetCandidates;
         ++fleetIndex) {
        const QVariant& value = fleet.at(fleetIndex);
        if (value.type() != QVariant::Map) {
            rejected.append(rejection(0, QStringLiteral("invalid_vehicle_entry")));
            continue;
        }
        const QVariantMap vehicle = value.toMap();
        const int id = vehicleId(vehicle);
        if (id <= 0 || seenIds.contains(id)) {
            if (requirePerformanceEvidence && id > 0 && seenIds.contains(id)) strictDuplicateIds.insert(id);
            rejected.append(rejection(id, id > 0 ? QStringLiteral("duplicate_vehicle_id")
                                                 : QStringLiteral("invalid_vehicle_id")));
            continue;
        }
        seenIds.insert(id);

        QVariantMap declaredPerformance;
        const bool declaredPerformanceCurrent = requirePerformanceEvidence
            && FleetCapabilityEvidence::currentDeclaredPerformance(vehicle, &declaredPerformance)
            && declaredPerformance.value(QStringLiteral("vehicleId")).toInt() == id;
        if (requirePerformanceEvidence) declaredPerformanceSources.insert(id,
            declaredPerformanceCurrent ? declaredPerformance : QVariantMap());

        bool vehicleBoolValid = true;
        const bool vehicleOnline = readBool(vehicle, QStringLiteral("online"), false,
                                            &vehicleBoolValid);
        for (const QString& key : boolFields) {
            if (vehicle.contains(key) && vehicle.value(key).type() != QVariant::Bool) {
                vehicleBoolValid = false;
                break;
            }
        }
        if (!vehicleBoolValid) {
            rejected.append(rejection(id, QStringLiteral("invalid_boolean_evidence")));
            continue;
        }
        const QVariantMap metadata = vehicle.value(QStringLiteral("metadata")).toMap();
        bool capabilityEvidenceValid = true;
        for (const QString& key : {QStringLiteral("capabilities"),
                                   QStringLiteral("verifiedCapabilities")}) {
            if (vehicle.contains(key)
                && !validStringListValue(vehicle.value(key))) {
                capabilityEvidenceValid = false;
            }
        }
        if (metadata.contains(QStringLiteral("verifiedCapabilities"))
            && !validStringListValue(metadata.value(QStringLiteral("verifiedCapabilities")))) {
            capabilityEvidenceValid = false;
        }
        if (!capabilityEvidenceValid) {
            rejected.append(rejection(id, QStringLiteral("invalid_capability_evidence")));
            continue;
        }
        QStringList reviewedCapabilities;
        if (requireVerifiedCapabilities
            && !FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewedCapabilities)) {
            rejected.append(rejection(id, QStringLiteral("capability_review_not_current")));
            continue;
        }
        if (!vehicleOnline) {
            rejected.append(rejection(id, QStringLiteral("offline_or_unknown")));
            continue;
        }
        const QString state = normalized(vehicle.value(QStringLiteral("healthState")).toString());
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
        if (!allowUnverified && (state.isEmpty() || state == QStringLiteral("unknown"))) {
            rejected.append(rejection(id, QStringLiteral("health_unknown")));
            continue;
        }
        if (requireConfigurationBaseline
            && !FleetCapabilityEvidence::currentConfigurationBaseline(vehicle)) {
            rejected.append(rejection(id, QStringLiteral("configuration_not_approved")));
            continue;
        }
        if (requirePerformanceEvidence && !declaredPerformanceCurrent) {
            rejected.append(rejection(id, QStringLiteral("performance_declaration_not_current")));
            continue;
        }
        // Strict planning consumes ONLY the canonical numbers bound by the
        // current engineering declaration and complete baseline. Unknown -1
        // cannot acquire a value from an unbound alias or convenience field.
        const NumberRead speedRead = readNumber(requirePerformanceEvidence ? declaredPerformance : vehicle,
            QStringList() << QStringLiteral("maxSpeedMps"), true);
        const NumberRead enduranceRead = requirePerformanceEvidence
            ? readNumber(declaredPerformance, QStringList() << QStringLiteral("enduranceSeconds"), true)
            : readNumber(vehicle, QStringList() << QStringLiteral("enduranceSeconds") << QStringLiteral("enduranceSec"), true);
        if (!speedRead.valid || !enduranceRead.valid
            || (speedRead.present && speedRead.value < 0.0)
            || (enduranceRead.present && enduranceRead.value < 0.0)) {
            rejected.append(rejection(id, QStringLiteral("invalid_performance_evidence")));
            continue;
        }
        const double speed = speedRead.present ? speedRead.value : -1.0;
        const double endurance = enduranceRead.present ? enduranceRead.value : -1.0;
        // A positive task floor is still a requirement when the operator did
        // not request source-bound engineering declarations. Legacy planning
        // descriptors may supply a value, but unknown cannot satisfy a floor.
        if ((minimumSpeedMps > 0.0 && speed < 0.0)
            || (minimumEnduranceSeconds > 0.0 && endurance < 0.0)) {
            rejected.append(rejection(id, QStringLiteral("performance_evidence_missing")));
            continue;
        }
        if (minimumSpeedMps > 0.0 && speed >= 0.0 && speed < minimumSpeedMps) {
            rejected.append(rejection(id, QStringLiteral("speed_below_task_threshold")));
            continue;
        }
        if (minimumEnduranceSeconds > 0.0 && endurance >= 0.0
            && endurance < minimumEnduranceSeconds) {
            rejected.append(rejection(id, QStringLiteral("endurance_below_task_threshold")));
            continue;
        }
        const NumberRead batteryRead = readNumber(vehicle,
            QStringList() << QStringLiteral("batteryPercent") << QStringLiteral("battery"), true);
        if (!batteryRead.valid || (batteryRead.present
            && (batteryRead.value < 0.0 || batteryRead.value > 100.0))) {
            rejected.append(rejection(id, QStringLiteral("invalid_battery_evidence")));
            continue;
        }
        const double battery = batteryRead.present ? batteryRead.value : -1.0;
        if (battery >= 0.0 && battery < minimumBattery) {
            rejected.append(rejection(id, QStringLiteral("battery_below_task_threshold")));
            continue;
        }
        const NumberRead linkRead = readNumber(vehicle,
            QStringList() << QStringLiteral("linkQualityPercent") << QStringLiteral("linkQuality"), true);
        if (!linkRead.valid || (linkRead.present && (linkRead.value < 0.0 || linkRead.value > 100.0))) {
            rejected.append(rejection(id, QStringLiteral("invalid_link_quality")));
            continue;
        }
        const int link = linkRead.present ? qRound(linkRead.value) : -1;
        if (link >= 0 && link < minimumLink) {
            rejected.append(rejection(id, QStringLiteral("link_below_task_threshold")));
            continue;
        }

        const QStringList available = capabilities(vehicle, requireVerifiedCapabilities);
        QString missing;
        for (const QString& wanted : required) {
            if (!hasCapability(available, wanted)) {
                missing = normalized(wanted);
                break;
            }
        }
        if (!missing.isEmpty()) {
            rejected.append(rejection(id, QStringLiteral("missing_capability:%1").arg(missing)));
            continue;
        }

        Candidate candidate;
        candidate.vehicle = vehicle;
        candidate.declaredPerformanceEvidence = declaredPerformance;
        candidate.id = id;
        candidate.relay = hasCapability(available, QStringLiteral("relay"));
        candidate.score = 50;
        if (state == QStringLiteral("healthy")) candidate.score += 25;
        if (battery >= 0.0) candidate.score += qRound(qMin(20.0, battery * 0.20));
        if (link >= 0) candidate.score += qRound(qMin(10.0, link * 0.10));
        if (candidate.relay) candidate.score += 8;
        candidates.append(candidate);
    }

    if (requirePerformanceEvidence) {
        // A duplicate ID cannot lend one row's declaration to another row or
        // leave its first occurrence selectable. Legacy diagnostics retain
        // their original duplicate-handling semantics when policy is false.
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&strictDuplicateIds](const Candidate& candidate) {
            return strictDuplicateIds.contains(candidate.id);
        }), candidates.end());
        for (int id : strictDuplicateIds) declaredPerformanceSources.insert(id, QVariantMap());
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.id < right.id;
    });

    QVariantList selected;
    QVariantList reserves;
    QVariantList assignments;
    const int selectedCount = qMin(requestedCount, candidates.size());
    if (parametersValid) {
        for (int i = 0; i < candidates.size(); ++i) {
            const Candidate& candidate = candidates.at(i);
            const bool isSelected = i < selectedCount;
            const bool isReserve = !isSelected && i < selectedCount + reserveCount;
            if (!isSelected && !isReserve) continue;

            const bool isFirstSelected = isSelected && selected.isEmpty();
            if (isSelected) selected.append(candidate.id);
            else reserves.append(candidate.id);

            QVariantMap assignment;
            assignment.insert(QStringLiteral("vehicleId"), candidate.id);
            assignment.insert(QStringLiteral("group"), isSelected ? QStringLiteral("primary") : QStringLiteral("reserve"));
            assignment.insert(QStringLiteral("role"),
                              isFirstSelected && leaderRequired
                                  ? QStringLiteral("leader")
                                  : (candidate.relay ? QStringLiteral("relay") : QStringLiteral("member")));
            assignment.insert(QStringLiteral("score"), candidate.score);
            if (requirePerformanceEvidence) {
                assignment.insert(QStringLiteral("requirePerformanceEvidence"), true);
                assignment.insert(QStringLiteral("declaredPerformanceEvidence"), candidate.declaredPerformanceEvidence);
                assignment.insert(QStringLiteral("declaredPerformanceEvidenceUsable"), !candidate.declaredPerformanceEvidence.isEmpty());
            }
            assignments.append(assignment);
        }
    }

    QStringList explanations;
    QVariantList riskSignals;
    if (!parametersValid) {
        explanations << QStringLiteral("任务参数存在类型、范围或有限性错误，已拒绝生成可执行候选组");
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 100},
                                       {QStringLiteral("reason"), QStringLiteral("invalid_task_parameters")}});
    }
    const bool feasible = parametersValid
        && selected.size() >= requestedCount && (!leaderRequired || !selected.isEmpty());
    if (!feasible) {
        explanations << QStringLiteral("可用车辆数量不足，无法满足任务需求");
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 45},
                                       {QStringLiteral("reason"), QStringLiteral("insufficient_eligible_vehicles")}});
    }
    if (reserveCount > reserves.size()) {
        explanations << QStringLiteral("备用机数量不足");
        riskSignals.append(QVariantMap{{QStringLiteral("weight"), 15},
                                       {QStringLiteral("reason"), QStringLiteral("insufficient_reserve_vehicles")}});
    }
    if (!required.isEmpty()) {
        explanations << QStringLiteral("已按能力标签筛选：%1").arg(required.join(QStringLiteral(", ")));
    }
    if (requireVerifiedCapabilities) {
        explanations << QStringLiteral("仅使用与当前构型修订匹配的本地工程审阅能力声明；不表示硬件或身份认证");
    }
    if (requirePerformanceEvidence) {
        explanations << QStringLiteral("性能门槛仅使用当前本地工程声明及完整构型基线所绑定的规范数值；不是实测性能认证或飞行许可");
    }
    if (!rejected.isEmpty()) {
        explanations << QStringLiteral("有 %1 架车辆未进入候选组").arg(rejected.size());
    }

    QVariantMap result;
    result.insert(QStringLiteral("feasible"), feasible);
    result.insert(QStringLiteral("parametersValid"), parametersValid);
    result.insert(QStringLiteral("requestedVehicleCount"), requestedCount);
    result.insert(QStringLiteral("selectedVehicleIds"), selected);
    result.insert(QStringLiteral("reserveVehicleIds"), reserves);
    result.insert(QStringLiteral("assignments"), assignments);
    result.insert(QStringLiteral("rejectedVehicles"), rejected);
    result.insert(QStringLiteral("requiredCapabilities"), required);
    result.insert(QStringLiteral("requireConfigurationBaseline"), requireConfigurationBaseline);
    result.insert(QStringLiteral("requireVerifiedCapabilities"), requireVerifiedCapabilities);
    result.insert(QStringLiteral("minimumSpeedMps"), minimumSpeedMps);
    result.insert(QStringLiteral("minimumEnduranceSeconds"), minimumEnduranceSeconds);
    result.insert(QStringLiteral("requirePerformanceEvidence"), requirePerformanceEvidence);
    if (requirePerformanceEvidence) {
        QVariantList sources;
        for (auto it = declaredPerformanceSources.constBegin(); it != declaredPerformanceSources.constEnd(); ++it)
            sources.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()}, {QStringLiteral("evidence"), it.value()}});
        result.insert(QStringLiteral("declaredPerformanceSources"), sources);
    }
    result.insert(QStringLiteral("riskSignals"), riskSignals);
    result.insert(QStringLiteral("explanations"), explanations);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("task"), task);
    _lastPlan = result;
    emit planChanged();
    return result;
}

void FleetCapabilityMatcher::clear()
{
    if (_lastPlan.isEmpty()) return;
    _lastPlan.clear();
    emit planChanged();
}

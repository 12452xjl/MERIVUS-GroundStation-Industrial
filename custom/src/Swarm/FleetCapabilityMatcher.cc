#include "FleetCapabilityMatcher.h"

#include <QSet>
#include <QtMath>

#include <algorithm>

namespace {

struct Candidate {
    QVariantMap vehicle;
    int id = 0;
    int score = 0;
    bool relay = false;
};

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
}

bool readBool(const QVariantMap& map, const QString& key, bool fallback = false)
{
    const QVariant value = map.value(key);
    return value.isValid() && !value.isNull() ? value.toBool() : fallback;
}

double readDouble(const QVariantMap& map, const QString& key, double fallback = -1.0)
{
    bool ok = false;
    const double result = map.value(key).toDouble(&ok);
    return ok ? result : fallback;
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
    if (value.canConvert<QStringList>()) {
        return value.toStringList();
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
    const QVariant value = vehicle.value(QStringLiteral("systemId"),
        vehicle.value(QStringLiteral("vehicleId"), vehicle.value(QStringLiteral("id"))));
    bool ok = false;
    const int id = value.toInt(&ok);
    return ok ? id : 0;
}

QStringList FleetCapabilityMatcher::capabilities(const QVariantMap& vehicle)
{
    QStringList result;
    for (const QString& capability : stringList(vehicle.value(QStringLiteral("capabilities")))) {
        const QString normalizedCapability = normalized(capability);
        if (!normalizedCapability.isEmpty() && !result.contains(normalizedCapability)) {
            result.append(normalizedCapability);
        }
    }
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
    const QStringList required = stringList(task.value(QStringLiteral("requiredCapabilities")));
    const int requestedCount = qMax(1, task.value(QStringLiteral("vehicleCount"),
                                                   task.value(QStringLiteral("minVehicleCount"), 1)).toInt());
    const int reserveCount = qMax(0, task.value(QStringLiteral("reserveCount"), 0).toInt());
    const double minimumBattery = qBound(0.0,
        readDouble(task, QStringLiteral("minimumBatteryPercent"), 30.0), 100.0);
    const int minimumLink = qBound(0,
        task.value(QStringLiteral("minimumLinkQualityPercent"), 40).toInt(), 100);
    const bool leaderRequired = task.value(QStringLiteral("leaderRequired"), true).toBool();

    QList<Candidate> candidates;
    QVariantList rejected;
    QSet<int> seenIds;
    for (const QVariant& value : fleet) {
        const QVariantMap vehicle = value.toMap();
        const int id = vehicleId(vehicle);
        if (id <= 0 || seenIds.contains(id)) {
            if (id > 0) rejected.append(rejection(id, QStringLiteral("duplicate_vehicle_id")));
            continue;
        }
        seenIds.insert(id);

        if (!readBool(vehicle, QStringLiteral("online"), false)) {
            rejected.append(rejection(id, QStringLiteral("offline_or_unknown")));
            continue;
        }
        const QString state = normalized(vehicle.value(QStringLiteral("healthState")).toString());
        if (state == QStringLiteral("critical") || state == QStringLiteral("offline")) {
            rejected.append(rejection(id, QStringLiteral("health_critical")));
            continue;
        }
        const double battery = readDouble(vehicle, QStringLiteral("batteryPercent"),
                                          readDouble(vehicle, QStringLiteral("battery"), -1.0));
        if (battery >= 0.0 && battery < minimumBattery) {
            rejected.append(rejection(id, QStringLiteral("battery_below_task_threshold")));
            continue;
        }
        const int link = vehicle.value(QStringLiteral("linkQualityPercent"),
                                       vehicle.value(QStringLiteral("linkQuality"), -1)).toInt();
        if (link >= 0 && link < minimumLink) {
            rejected.append(rejection(id, QStringLiteral("link_below_task_threshold")));
            continue;
        }

        const QStringList available = capabilities(vehicle);
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
        candidate.id = id;
        candidate.relay = hasCapability(available, QStringLiteral("relay"));
        candidate.score = 50;
        if (state == QStringLiteral("healthy")) candidate.score += 25;
        if (battery >= 0.0) candidate.score += qRound(qMin(20.0, battery * 0.20));
        if (link >= 0) candidate.score += qRound(qMin(10.0, link * 0.10));
        if (candidate.relay) candidate.score += 8;
        candidates.append(candidate);
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.id < right.id;
    });

    QVariantList selected;
    QVariantList reserves;
    QVariantList assignments;
    const int selectedCount = qMin(requestedCount, candidates.size());
    for (int i = 0; i < candidates.size(); ++i) {
        const Candidate& candidate = candidates.at(i);
        const bool isSelected = i < selectedCount;
        const bool isReserve = !isSelected && i < selectedCount + reserveCount;
        if (!isSelected && !isReserve) continue;

        if (isSelected) selected.append(candidate.id);
        else reserves.append(candidate.id);

        QVariantMap assignment;
        assignment.insert(QStringLiteral("vehicleId"), candidate.id);
        assignment.insert(QStringLiteral("group"), isSelected ? QStringLiteral("primary") : QStringLiteral("reserve"));
        assignment.insert(QStringLiteral("role"),
                          isSelected && leaderRequired && selected.isEmpty()
                              ? QStringLiteral("leader")
                              : (candidate.relay ? QStringLiteral("relay") : QStringLiteral("member")));
        assignment.insert(QStringLiteral("score"), candidate.score);
        assignments.append(assignment);
    }

    QStringList explanations;
    QVariantList riskSignals;
    const bool feasible = selected.size() >= requestedCount && (!leaderRequired || !selected.isEmpty());
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
    if (!rejected.isEmpty()) {
        explanations << QStringLiteral("有 %1 架车辆未进入候选组").arg(rejected.size());
    }

    QVariantMap result;
    result.insert(QStringLiteral("feasible"), feasible);
    result.insert(QStringLiteral("requestedVehicleCount"), requestedCount);
    result.insert(QStringLiteral("selectedVehicleIds"), selected);
    result.insert(QStringLiteral("reserveVehicleIds"), reserves);
    result.insert(QStringLiteral("assignments"), assignments);
    result.insert(QStringLiteral("rejectedVehicles"), rejected);
    result.insert(QStringLiteral("requiredCapabilities"), required);
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

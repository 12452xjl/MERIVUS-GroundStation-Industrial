#include "FleetMissionSimulator.h"

#include <QSet>
#include <QStringList>

FleetMissionSimulator::FleetMissionSimulator(QObject* parent) : QObject(parent) {}
void FleetMissionSimulator::setSteps(const QVariantList& steps) { _steps = steps; emit stepsChanged(); }
void FleetMissionSimulator::clear() { _steps.clear(); _faults.clear(); emit stepsChanged(); emit faultsChanged(); }
void FleetMissionSimulator::injectFault(const QString& kind, const QVariantMap& details)
{
    const QString normalized = kind.trimmed().toLower();
    static const QStringList supported = {
        QStringLiteral("vehicle_offline"), QStringLiteral("link_loss"),
        QStringLiteral("link_degraded"), QStringLiteral("low_battery"),
        QStringLiteral("gnss_degraded"), QStringLiteral("leader_failure"),
        QStringLiteral("communication_delay")
    };
    if (!supported.contains(normalized)) return;
    QVariantMap fault;
    fault.insert(QStringLiteral("kind"), normalized);
    fault.insert(QStringLiteral("details"), details);
    _faults.append(fault);
    emit faultsChanged();
}
QVariantList FleetMissionSimulator::preview() const
{
    QVariantList out;
    for (int i = 0; i < _steps.size(); ++i) {
        QVariantMap result = _steps.at(i).toMap();
        QStringList actions;
        QVariantList affected;
        QSet<QString> affectedKeys;
        bool requiresReview = false;
        for (const QVariant& faultValue : _faults) {
            const QVariantMap fault = faultValue.toMap();
            const QString kind = fault.value(QStringLiteral("kind")).toString().trimmed().toLower();
            const QVariantMap details = fault.value(QStringLiteral("details")).toMap();
            QVariantList ids = details.value(QStringLiteral("vehicleIds")).toList();
            if (ids.isEmpty() && details.value(QStringLiteral("vehicleId")).isValid()) {
                ids.append(details.value(QStringLiteral("vehicleId")));
            }
            for (const QVariant& id : ids) {
                const QString key = id.toString().trimmed();
                if (!key.isEmpty() && !affectedKeys.contains(key)) {
                    affectedKeys.insert(key);
                    affected.append(id);
                }
            }
            if (kind == QStringLiteral("vehicle_offline") || kind == QStringLiteral("leader_failure")) {
                actions.append(QStringLiteral("reassign_or_split"));
                requiresReview = true;
            } else if (kind == QStringLiteral("low_battery")) {
                actions.append(QStringLiteral("return_or_replace"));
                requiresReview = true;
            } else if (kind == QStringLiteral("link_degraded")) {
                actions.append(QStringLiteral("slow_down_or_hold"));
                requiresReview = true;
            } else if (kind == QStringLiteral("gnss_degraded")) {
                actions.append(QStringLiteral("pause_and_verify_positioning"));
                requiresReview = true;
            } else if (!kind.isEmpty()) {
                actions.append(QStringLiteral("operator_review"));
                requiresReview = true;
            }
        }
        actions.removeDuplicates();
        result.insert(QStringLiteral("index"), i);
        result.insert(QStringLiteral("simulated"), true);
        result.insert(QStringLiteral("faults"), _faults);
        result.insert(QStringLiteral("affectedVehicleIds"), affected);
        result.insert(QStringLiteral("degradationActions"), actions);
        result.insert(QStringLiteral("requiresReview"), requiresReview);
        result.insert(QStringLiteral("flightCommandReleased"), false);
        out.append(result);
    }
    return out;
}

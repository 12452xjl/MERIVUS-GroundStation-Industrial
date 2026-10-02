#include "FleetRiskRadar.h"

FleetRiskRadar::FleetRiskRadar(QObject* parent) : QObject(parent) {}

QVariantMap FleetRiskRadar::assess(const QVariantMap& task, const QVariantList& riskSignals)
{
    int score = 0;
    QStringList why;
    const QString intent = task.value(QStringLiteral("intent")).toString().toLower();
    const QVariantMap parameters = task.value(QStringLiteral("parameters")).toMap();
    if (intent.contains(QStringLiteral("land")) || intent.contains(QStringLiteral("return"))) { score += 35; why << QStringLiteral("flight-state transition"); }
    if (intent.contains(QStringLiteral("formation")) || intent.contains(QStringLiteral("swarm"))) { score += 20; why << QStringLiteral("multi-vehicle coordination"); }
    if (parameters.contains(QStringLiteral("geofence"))) { score += 20; why << QStringLiteral("geofence-sensitive parameters"); }
    if (parameters.contains(QStringLiteral("geofenceValidated"))
        && !parameters.value(QStringLiteral("geofenceValidated")).toBool()) {
        score += 45;
        why << QStringLiteral("geofence_not_validated");
    }
    if (parameters.contains(QStringLiteral("routeValid"))
        && !parameters.value(QStringLiteral("routeValid")).toBool()) {
        score += 45;
        why << QStringLiteral("route_invalid_or_incomplete");
    }
    if (parameters.contains(QStringLiteral("linkCoveragePercent"))) {
        const int coverage = parameters.value(QStringLiteral("linkCoveragePercent")).toInt();
        if (coverage < 40) {
            score += 30;
            why << QStringLiteral("link_coverage_low");
        }
    }
    if (parameters.contains(QStringLiteral("estimatedBatteryRemainingPercent"))) {
        const double remaining = parameters.value(QStringLiteral("estimatedBatteryRemainingPercent")).toDouble();
        const double minimum = parameters.value(QStringLiteral("minimumBatteryPercent"), 30.0).toDouble();
        if (remaining < minimum) {
            score += 40;
            why << QStringLiteral("estimated_battery_insufficient");
        }
    }
    if (parameters.contains(QStringLiteral("positioningAvailable"))
        && parameters.value(QStringLiteral("positioningAvailable")).toBool() == false) {
        score += 40;
        why << QStringLiteral("required_positioning_unavailable");
    }
    if (parameters.contains(QStringLiteral("reserveVehicleCount"))
        && parameters.contains(QStringLiteral("requiredReserveCount"))
        && parameters.value(QStringLiteral("reserveVehicleCount")).toInt()
            < parameters.value(QStringLiteral("requiredReserveCount")).toInt()) {
        score += 20;
        why << QStringLiteral("reserve_capacity_insufficient");
    }
    for (const QVariant& item : riskSignals) {
        const QVariantMap signal = item.toMap();
        score += signal.value(QStringLiteral("weight"), 0).toInt();
        if (!signal.value(QStringLiteral("reason")).toString().isEmpty()) why << signal.value(QStringLiteral("reason")).toString();
    }
    _score = qBound(0, score, 100);
    _level = _score >= 80 ? QStringLiteral("critical") : _score >= 60 ? QStringLiteral("high") : _score >= 30 ? QStringLiteral("medium") : _score > 0 ? QStringLiteral("low") : QStringLiteral("informational");
    why.removeDuplicates();
    _explanations = why;
    _assessment = {{QStringLiteral("score"), _score}, {QStringLiteral("level"), _level}, {QStringLiteral("explanations"), _explanations}, {QStringLiteral("requiresConfirmation"), _score >= 30}};
    emit assessmentChanged();
    return _assessment;
}

void FleetRiskRadar::clear()
{
    _score = 0; _level = QStringLiteral("informational"); _explanations.clear(); _assessment.clear(); emit assessmentChanged();
}

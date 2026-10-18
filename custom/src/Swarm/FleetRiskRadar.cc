#include "FleetRiskRadar.h"

#include <QtMath>

#include <limits>

namespace {

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

bool strictNumber(const QVariant& value, double* output = nullptr)
{
    if (!value.isValid() || value.isNull() || !numericVariant(value)) {
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) {
        return false;
    }
    if (output) {
        *output = number;
    }
    return true;
}

bool strictInteger(const QVariant& value, int* output = nullptr)
{
    double number = 0.0;
    if (!strictNumber(value, &number) || qFloor(number) != number
        || number < 0.0 || number > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (output) {
        *output = static_cast<int>(number);
    }
    return true;
}

bool strictBool(const QVariantMap& map, const QString& key, bool fallback,
                bool* valid = nullptr)
{
    if (!map.contains(key)) {
        return fallback;
    }
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool) {
        if (valid) {
            *valid = false;
        }
        return fallback;
    }
    return value.toBool();
}

}

FleetRiskRadar::FleetRiskRadar(QObject* parent) : QObject(parent) {}

QVariantMap FleetRiskRadar::assess(const QVariantMap& task, const QVariantList& riskSignals)
{
    int score = 0;
    QStringList why;
    bool inputValid = true;
    const QVariant intentValue = task.value(QStringLiteral("intent"));
    const QString intent = intentValue.isValid() && !intentValue.isNull()
            && intentValue.type() == QVariant::String
        ? intentValue.toString().toLower() : QString();
    if (intentValue.isValid() && !intentValue.isNull()
        && intentValue.type() != QVariant::String) {
        inputValid = false;
        why << QStringLiteral("invalid_input:intent");
    }

    const QVariant parametersValue = task.value(QStringLiteral("parameters"));
    const QVariantMap parameters = parametersValue.toMap();
    if (parametersValue.isValid() && !parametersValue.isNull()
        && parametersValue.type() != QVariant::Map) {
        inputValid = false;
        why << QStringLiteral("invalid_input:parameters");
    }
    if (intent.contains(QStringLiteral("land")) || intent.contains(QStringLiteral("return"))) { score += 35; why << QStringLiteral("flight-state transition"); }
    if (intent.contains(QStringLiteral("formation")) || intent.contains(QStringLiteral("swarm"))) { score += 20; why << QStringLiteral("multi-vehicle coordination"); }
    if (parameters.contains(QStringLiteral("geofence"))) { score += 20; why << QStringLiteral("geofence-sensitive parameters"); }
    bool boolValid = true;
    const bool geofenceValidated = strictBool(parameters,
                                              QStringLiteral("geofenceValidated"), true,
                                              &boolValid);
    if (!boolValid) {
        inputValid = false;
        why << QStringLiteral("invalid_input:geofenceValidated");
    } else if (parameters.contains(QStringLiteral("geofenceValidated"))
               && !geofenceValidated) {
        score += 45;
        why << QStringLiteral("geofence_not_validated");
    }
    boolValid = true;
    const bool routeValid = strictBool(parameters, QStringLiteral("routeValid"), true,
                                       &boolValid);
    if (!boolValid) {
        inputValid = false;
        why << QStringLiteral("invalid_input:routeValid");
    } else if (parameters.contains(QStringLiteral("routeValid")) && !routeValid) {
        score += 45;
        why << QStringLiteral("route_invalid_or_incomplete");
    }
    if (parameters.contains(QStringLiteral("linkCoveragePercent"))) {
        double coverage = 0.0;
        if (!strictNumber(parameters.value(QStringLiteral("linkCoveragePercent")), &coverage)
            || coverage < 0.0 || coverage > 100.0) {
            inputValid = false;
            why << QStringLiteral("invalid_input:linkCoveragePercent");
        } else if (coverage < 40.0) {
            score += 30;
            why << QStringLiteral("link_coverage_low");
        }
    }
    double minimum = 30.0;
    const bool minimumValid = !parameters.contains(QStringLiteral("minimumBatteryPercent"))
        || (strictNumber(parameters.value(QStringLiteral("minimumBatteryPercent")), &minimum)
            && minimum >= 0.0 && minimum <= 100.0);
    if (!minimumValid) {
        inputValid = false;
        why << QStringLiteral("invalid_input:minimumBatteryPercent");
    }
    if (parameters.contains(QStringLiteral("estimatedBatteryRemainingPercent"))) {
        double remaining = 0.0;
        const bool remainingValid = strictNumber(
            parameters.value(QStringLiteral("estimatedBatteryRemainingPercent")), &remaining)
            && remaining >= 0.0 && remaining <= 100.0;
        if (!remainingValid) {
            inputValid = false;
            why << QStringLiteral("invalid_input:estimatedBatteryRemainingPercent");
        }
        if (remainingValid && minimumValid && remaining < minimum) {
            score += 40;
            why << QStringLiteral("estimated_battery_insufficient");
        }
    }
    boolValid = true;
    const bool positioningAvailable = strictBool(parameters,
                                                 QStringLiteral("positioningAvailable"), true,
                                                 &boolValid);
    if (!boolValid) {
        inputValid = false;
        why << QStringLiteral("invalid_input:positioningAvailable");
    } else if (parameters.contains(QStringLiteral("positioningAvailable"))
               && !positioningAvailable) {
        score += 40;
        why << QStringLiteral("required_positioning_unavailable");
    }
    if (parameters.contains(QStringLiteral("reserveVehicleCount"))
        || parameters.contains(QStringLiteral("requiredReserveCount"))) {
        int reserveCount = 0;
        int requiredReserveCount = 0;
        const bool reserveValid = parameters.contains(QStringLiteral("reserveVehicleCount"))
            && strictInteger(parameters.value(QStringLiteral("reserveVehicleCount")), &reserveCount);
        const bool requiredValid = parameters.contains(QStringLiteral("requiredReserveCount"))
            && strictInteger(parameters.value(QStringLiteral("requiredReserveCount")),
                             &requiredReserveCount);
        if (!reserveValid) {
            inputValid = false;
            why << QStringLiteral("invalid_input:reserveVehicleCount");
        }
        if (!requiredValid) {
            inputValid = false;
            why << QStringLiteral("invalid_input:requiredReserveCount");
        }
        if (reserveValid && requiredValid && reserveCount < requiredReserveCount) {
            score += 20;
            why << QStringLiteral("reserve_capacity_insufficient");
        }
    }
    for (const QVariant& item : riskSignals) {
        if (item.type() != QVariant::Map) {
            inputValid = false;
            why << QStringLiteral("invalid_input:risk_signal");
            continue;
        }
        const QVariantMap signal = item.toMap();
        // Risk inputs may come from plugins or replay files. Never allow a
        // negative weight to cancel a safety signal; mitigation belongs in a
        // separate, auditable field rather than lowering the risk score.
        int weight = 0;
        if (signal.contains(QStringLiteral("weight"))
            && (!strictInteger(signal.value(QStringLiteral("weight")), &weight)
                || weight > 100)) {
            inputValid = false;
            why << QStringLiteral("invalid_input:risk_signal_weight");
        } else {
            score += qMax(0, weight);
        }
        if (signal.contains(QStringLiteral("reason"))
            && signal.value(QStringLiteral("reason")).type() != QVariant::String) {
            inputValid = false;
            why << QStringLiteral("invalid_input:risk_signal_reason");
        } else if (!signal.value(QStringLiteral("reason")).toString().isEmpty()) {
            why << signal.value(QStringLiteral("reason")).toString();
        }
    }
    if (!inputValid) {
        score = 100;
        why << QStringLiteral("invalid_input_blocks_assessment");
    }
    _score = qBound(0, score, 100);
    _level = _score >= 80 ? QStringLiteral("critical") : _score >= 60 ? QStringLiteral("high") : _score >= 30 ? QStringLiteral("medium") : _score > 0 ? QStringLiteral("low") : QStringLiteral("informational");
    why.removeDuplicates();
    _explanations = why;
    _assessment = {{QStringLiteral("score"), _score}, {QStringLiteral("level"), _level},
                   {QStringLiteral("explanations"), _explanations},
                   {QStringLiteral("inputValid"), inputValid},
                   {QStringLiteral("needsReview"), !inputValid || _score >= 30},
                   {QStringLiteral("requiresConfirmation"), _score >= 30}};
    emit assessmentChanged();
    return _assessment;
}

void FleetRiskRadar::clear()
{
    _score = 0; _level = QStringLiteral("informational"); _explanations.clear(); _assessment.clear(); emit assessmentChanged();
}

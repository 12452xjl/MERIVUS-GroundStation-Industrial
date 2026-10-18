#include "FleetMissionActionPhaseReview.h"

#include <QMap>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr double MaximumEvidenceNumber = 9007199254740991.0;

bool number(const QVariant& value, double* output)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double numeric = value.toDouble();
    if (!std::isfinite(numeric) || std::abs(numeric) > MaximumEvidenceNumber) return false;
    *output = numeric;
    return true;
}

bool exactFloat(double value)
{
    return std::abs(value) <= std::numeric_limits<float>::max()
        && static_cast<double>(static_cast<float>(value)) == value;
}

QStringList legacyNames()
{
    // This is a comparison input, not an extension of parameterProfileHash.
    return {QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("MPC_LAND_SPEED"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_YAWRAUTO_MAX"), QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("NAV_ACC_RAD")};
}

QStringList phaseNames()
{
    return {QStringLiteral("COM_OBS_AVOID"), QStringLiteral("COM_SPOOLUP_TIME"),
        QStringLiteral("MPC_ACC_DOWN_MAX"), QStringLiteral("MPC_ACC_UP_MAX"), QStringLiteral("MPC_JERK_AUTO"),
        QStringLiteral("MPC_LAND_ALT1"), QStringLiteral("MPC_LAND_ALT2"), QStringLiteral("MPC_LAND_ALT3"),
        QStringLiteral("MPC_LAND_CRWL"), QStringLiteral("MPC_LAND_RADIUS"), QStringLiteral("MPC_LAND_RC_HELP"),
        QStringLiteral("MPC_TKO_RAMP_T"), QStringLiteral("MPC_Z_VEL_P_ACC"), QStringLiteral("SYS_VEHICLE_RESP")};
}

bool phaseRange(const QString& name, double value)
{
    if (name == QStringLiteral("COM_OBS_AVOID") || name == QStringLiteral("MPC_LAND_RC_HELP"))
        return value == 0.0 || value == 1.0;
    if (name == QStringLiteral("COM_SPOOLUP_TIME")) return value >= 0.0 && value <= 30.0;
    if (name == QStringLiteral("MPC_LAND_ALT1") || name == QStringLiteral("MPC_LAND_ALT2")
        || name == QStringLiteral("MPC_LAND_ALT3")) return value >= 0.0 && value <= 122.0;
    if (name == QStringLiteral("MPC_LAND_RADIUS")) return value >= 0.0;
    if (name == QStringLiteral("MPC_TKO_RAMP_T")) return value >= 0.0 && value <= 5.0;
    if (name == QStringLiteral("SYS_VEHICLE_RESP")) return value >= -1.0 && value <= 1.0;
    // Source-derived positive operands, not metadata lower bounds: an active
    // SYS_VEHICLE_RESP can legitimately produce ACC_DOWN_MAX=0.8f at zero.
    return value > 0.0;
}

bool readRows(const QVariantList& rows, const QStringList& names, bool phase,
              QMap<QString, double>* values)
{
    if (rows.size() != names.size()) return false;
    const QSet<QString> allowed{QStringLiteral("name"), QStringLiteral("componentId"),
        QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
        QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("mavParamType"),
        QStringLiteral("contentRevision"), QStringLiteral("managerEpoch"), QStringLiteral("cachedValueMatches"),
        QStringLiteral("pendingWrite"), QStringLiteral("value"), QStringLiteral("rangeValid"), QStringLiteral("usable")};
    const QSet<QString> legacyRates{QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_VEL_MAX_UP"),
        QStringLiteral("MPC_Z_V_AUTO_DN"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_LAND_SPEED")};
    for (int index = 0; index < names.size(); ++index) {
        const QVariant& value = rows.at(index);
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap row = value.toMap();
        const QString& name = names.at(index);
        if (row.size() > allowed.size() || row.value(QStringLiteral("name")).type() != QVariant::String
            || row.value(QStringLiteral("name")).isNull() || row.value(QStringLiteral("name")).toString() != name) return false;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) if (!allowed.contains(it.key())) return false;
        double type = 0.0, numeric = 0.0;
        if (!number(row.value(QStringLiteral("mavParamType")), &type) || std::floor(type) != type || type < 1.0 || type > 10.0
            || !number(row.value(QStringLiteral("value")), &numeric)) return false;
        if (phase) {
            const bool integerSwitch = name == QStringLiteral("COM_OBS_AVOID") || name == QStringLiteral("MPC_LAND_RC_HELP");
            if (integerSwitch ? type != 6.0 : type != 9.0 || !exactFloat(numeric)) return false;
            if (!phaseRange(name, numeric)) return false;
        } else {
            // Like the separate rate-override comparison, require the actual
            // REAL32 operands for all eight rate parameters, not rounded input.
            if (legacyRates.contains(name) && (type != 9.0 || !exactFloat(numeric))) return false;
            if (name == QStringLiteral("MIS_TAKEOFF_ALT") || name == QStringLiteral("NAV_ACC_RAD")) {
                if (numeric < 0.0) return false;
            } else if (name == QStringLiteral("MPC_YAW_MODE")) {
                if (std::floor(numeric) != numeric) return false;
            } else if (numeric <= 0.0) return false;
        }
        values->insert(name, numeric);
    }
    return true;
}

float sourceLerp(float low, float high, float fraction)
{
    // Same operand order as local mathlib::lerp, with explicit float32
    // intermediates. Do not rewrite as low + (high-low)*fraction.
    const float inverse = static_cast<float>(1.f - fraction);
    const float lowTerm = static_cast<float>(inverse * low);
    const float highTerm = static_cast<float>(fraction * high);
    return static_cast<float>(lowTerm + highTerm);
}
}

QVariantMap FleetMissionActionPhaseReview::evaluate(const QVariantList& parameters,
                                                   const QVariantList& phaseParameters)
{
    const auto result = [](bool ok, const QString& reason, const QVariantList& expected = QVariantList(),
                           const QVariantMap& branches = QVariantMap(), bool takeoffCompatible = false,
                           bool landCompatible = false) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
            {QStringLiteral("model"), QStringLiteral("local-px4-action-phase-parameters-v1")},
            {QStringLiteral("expectedParameters"), expected}, {QStringLiteral("actionBranches"), branches},
            {QStringLiteral("takeoffCompatible"), takeoffCompatible}, {QStringLiteral("landCompatible"), landCompatible},
            {QStringLiteral("actualActionStateProven"), false}};
    };
    QMap<QString, double> values;
    if (!readRows(parameters, legacyNames(), false, &values)
        || !readRows(phaseParameters, phaseNames(), true, &values))
        return result(false, QStringLiteral("action-phase-complete-supported-parameter-content-required"));
    const auto value = [&values](const char* name) { return static_cast<float>(values.value(QString::fromLatin1(name))); };

    QVariantList expected;
    const float response = value("SYS_VEHICLE_RESP");
    const bool responseActive = response >= 0.f;
    if (responseActive) {
        const float squared = static_cast<float>(response * response);
        const QMap<QString, float> overrides{
            {QStringLiteral("MPC_ACC_DOWN_MAX"), sourceLerp(0.8f, 15.f, squared)},
            {QStringLiteral("MPC_ACC_UP_MAX"), sourceLerp(1.f, 15.f, squared)},
            {QStringLiteral("MPC_JERK_AUTO"), sourceLerp(1.f, 25.f, squared)}};
        for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it) {
            if (!std::isfinite(it.value()) || it.value() <= 0.f || static_cast<double>(it.value()) != values.value(it.key()))
                return result(false, QStringLiteral("action-phase-responsiveness-override-mismatch-") + it.key());
            expected.append(QVariantMap{{QStringLiteral("name"), it.key()}, {QStringLiteral("value"), static_cast<double>(it.value())}});
        }
    }

    const float rawAlt1 = value("MPC_LAND_ALT1"), alt2 = value("MPC_LAND_ALT2");
    const float alt1 = std::max(rawAlt1, alt2);
    const float ramp = value("MPC_TKO_RAMP_T"), accelerationUp = value("MPC_ACC_UP_MAX");
    const float takeoffSpeed = value("MPC_TKO_SPEED");
    const bool negativeZeroRamp = ramp == 0.f && std::signbit(ramp);
    // FlightTaskAuto divides by the signed value without a zero guard. Its
    // min(accelerationUp, takeoffSpeed / -0) selects negative infinity; do
    // not normalize that branch to the separate controller's disabled ramp.
    float nearGroundAcceleration = negativeZeroRamp ? -std::numeric_limits<float>::infinity() : accelerationUp;
    if (ramp > 0.f) {
        const float rampAcceleration = static_cast<float>(takeoffSpeed / ramp);
        // Positive finite operands can overflow to +infinity. The source's
        // min(accelerationUp, +infinity) still selects accelerationUp.
        if (std::isfinite(rampAcceleration)) nearGroundAcceleration = std::min(accelerationUp, rampAcceleration);
    }
    const bool nearGroundAccelerationFinite = std::isfinite(nearGroundAcceleration);
    // For +0 the FlightTask min selects accelerationUp; the independent
    // position-controller ramp is disabled for either signed zero. Describe
    // both explicitly instead of performing division by zero here.
    const float initialRamp = static_cast<float>(-9.80665f / std::max(value("MPC_Z_VEL_P_ACC"), 0.01f));
    const bool equalThresholds = alt1 == alt2;
    const float lowSpeed = value("MPC_LAND_SPEED"), highSpeed = value("MPC_Z_VEL_MAX_DN");
    float interpolationSlope = 0.f, interpolationIntercept = 0.f;
    if (!equalThresholds) {
        const float speedDifference = static_cast<float>(highSpeed - lowSpeed);
        const float altitudeDifference = static_cast<float>(alt1 - alt2);
        interpolationSlope = static_cast<float>(speedDifference / altitudeDifference);
        const float lowProduct = static_cast<float>(interpolationSlope * alt2);
        interpolationIntercept = static_cast<float>(lowSpeed - lowProduct);
    }
    const bool interpolationFinite = equalThresholds
        || (std::isfinite(interpolationSlope) && std::isfinite(interpolationIntercept));
    const bool interpolationMaterialized = !equalThresholds && interpolationFinite
        && std::abs(static_cast<double>(interpolationSlope)) <= MaximumEvidenceNumber
        && std::abs(static_cast<double>(interpolationIntercept)) <= MaximumEvidenceNumber;
    // The input domain and the diagnostic number domain are different. A
    // finite source coefficient outside the latter is still a valid nominal
    // branch: retain the formula and flags, never truncate/clamp or reject
    // unrelated TAKEOFF content. Materialize the complete pair or neither.

    const bool avoidance = values.value(QStringLiteral("COM_OBS_AVOID")) != 0.0;
    const bool rcHelp = values.value(QStringLiteral("MPC_LAND_RC_HELP")) != 0.0;
    const QVariantMap takeoff{
        {QStringLiteral("nearGroundDistanceThresholdMeters"), static_cast<double>(alt1)},
        {QStringLiteral("nearGroundDistanceComparison"), QStringLiteral("strictly-less")},
        {QStringLiteral("requiresTakeoffWaypointAndPriorUpwardUnsmoothedVelocity"), true},
        {QStringLiteral("nearGroundVelocityLimitMetersPerSecond"), static_cast<double>(takeoffSpeed)},
        {QStringLiteral("nearGroundAccelerationLimitMetersPerSecondSquared"), nearGroundAccelerationFinite ? QVariant(static_cast<double>(nearGroundAcceleration)) : QVariant()},
        {QStringLiteral("nearGroundAccelerationFinite"), nearGroundAccelerationFinite},
        {QStringLiteral("nearGroundRampNegativeZero"), negativeZeroRamp},
        {QStringLiteral("normalUpwardVelocityLimitMetersPerSecond"), values.value(QStringLiteral("MPC_Z_V_AUTO_UP"))},
        {QStringLiteral("normalUpwardAccelerationLimitMetersPerSecondSquared"), static_cast<double>(accelerationUp)},
        {QStringLiteral("jerkLimitMetersPerSecondCubed"), values.value(QStringLiteral("MPC_JERK_AUTO"))},
        {QStringLiteral("currentBranchKnown"), false}};
    const QVariantMap landing{
        {QStringLiteral("lowDistanceThresholdMeters"), static_cast<double>(alt2)},
        {QStringLiteral("highDistanceThresholdMeters"), static_cast<double>(alt1)},
        {QStringLiteral("altitude1RaisedLocallyToAltitude2"), rawAlt1 < alt2},
        {QStringLiteral("equalThresholdsUseStep"), equalThresholds},
        {QStringLiteral("lowBaseDownwardSpeedMetersPerSecond"), static_cast<double>(lowSpeed)},
        {QStringLiteral("highBaseDownwardSpeedMetersPerSecond"), static_cast<double>(highSpeed)},
        {QStringLiteral("interpolationSlope"), interpolationMaterialized ? QVariant(static_cast<double>(interpolationSlope)) : QVariant()},
        {QStringLiteral("interpolationIntercept"), interpolationMaterialized ? QVariant(static_cast<double>(interpolationIntercept)) : QVariant()},
        {QStringLiteral("interpolationCoefficientsMaterialized"), interpolationMaterialized},
        {QStringLiteral("interpolationCoefficientsFinite"), interpolationFinite},
        {QStringLiteral("interpolationFormula"), QStringLiteral("float32:a=(highSpeed-lowSpeed)/(highDistance-lowDistance);b=lowSpeed-a*lowDistance;speed=a*distance+b;step:distance<=lowDistance?lowSpeed:highSpeed")},
        {QStringLiteral("crawlRequiresValidFiniteBottomDistance"), true},
        {QStringLiteral("crawlBottomDistanceMaximumMeters"), values.value(QStringLiteral("MPC_LAND_ALT3"))},
        {QStringLiteral("crawlBaseDownwardSpeedMetersPerSecond"), values.value(QStringLiteral("MPC_LAND_CRWL"))},
        {QStringLiteral("rcAssistanceEnabled"), rcHelp},
        {QStringLiteral("rcThrottleMultiplierMinimum"), rcHelp ? 0.0 : 1.0},
        {QStringLiteral("rcThrottleMultiplierMaximum"), rcHelp ? 2.0 : 1.0},
        {QStringLiteral("rcCanAdjustHorizontalPositionAndYaw"), rcHelp},
        {QStringLiteral("declaredRcAdjustmentRadiusMeters"), values.value(QStringLiteral("MPC_LAND_RADIUS"))},
        {QStringLiteral("smoothingDownwardVelocityLimitMetersPerSecond"), values.value(QStringLiteral("MPC_Z_V_AUTO_DN"))},
        {QStringLiteral("downwardAccelerationLimitMetersPerSecondSquared"), values.value(QStringLiteral("MPC_ACC_DOWN_MAX"))},
        {QStringLiteral("jerkLimitMetersPerSecondCubed"), values.value(QStringLiteral("MPC_JERK_AUTO"))},
        {QStringLiteral("currentBranchKnown"), false}};
    const QVariantMap controllerRamp{
        {QStringLiteral("spoolupTimeSeconds"), values.value(QStringLiteral("COM_SPOOLUP_TIME"))},
        {QStringLiteral("rampTimeSeconds"), static_cast<double>(ramp)},
        {QStringLiteral("rampDisabled"), ramp == 0.f},
        {QStringLiteral("initialUpwardVelocityLimitReferenceMetersPerSecond"), static_cast<double>(initialRamp)},
        {QStringLiteral("stateProgressAndLoopTimeKnown"), false},
        {QStringLiteral("actualDesiredVelocityEndpointKnown"), false}};
    const QVariantMap branches{
        {QStringLiteral("responsivenessMacroActive"), responseActive},
        {QStringLiteral("takeoff"), takeoff}, {QStringLiteral("landing"), landing},
        {QStringLiteral("positionControllerRamp"), controllerRamp},
        {QStringLiteral("externalAvoidanceSetpointInjectionEnabled"), avoidance},
        {QStringLiteral("groundDistanceAndBottomValidityKnown"), false},
        {QStringLiteral("emergencyBrakingAndSmoothingStateKnown"), false},
        {QStringLiteral("actualVelocityTimingOrTrajectoryProven"), false}};
    const bool takeoffCompatible = !avoidance && nearGroundAccelerationFinite
        && nearGroundAcceleration > 0.f && std::isfinite(initialRamp);
    return result(true, QStringLiteral(""), expected, branches, takeoffCompatible, !avoidance && !rcHelp && interpolationFinite);
}

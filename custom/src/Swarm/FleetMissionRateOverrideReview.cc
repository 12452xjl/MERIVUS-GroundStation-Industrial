#include "FleetMissionRateOverrideReview.h"

#include <QMap>
#include <QSet>
#include <QStringList>

#include <cmath>
#include <limits>

namespace {
bool number(const QVariant& value, double* output)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double numeric = value.toDouble();
    if (!std::isfinite(numeric)) return false;
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
    // Keep the old twelve-name parameterProfileHash contract unchanged.
    return {QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("MPC_LAND_SPEED"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_YAWRAUTO_MAX"), QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("NAV_ACC_RAD")};
}

bool readRows(const QVariantList& rows, const QStringList& names, const QSet<QString>& rateNames,
              QMap<QString, double>* values)
{
    if (rows.size() != names.size()) return false;
    const QSet<QString> allowed{QStringLiteral("name"), QStringLiteral("componentId"),
        QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
        QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("mavParamType"),
        QStringLiteral("contentRevision"), QStringLiteral("managerEpoch"), QStringLiteral("cachedValueMatches"),
        QStringLiteral("pendingWrite"), QStringLiteral("value"), QStringLiteral("rangeValid"), QStringLiteral("usable")};
    for (int index = 0; index < names.size(); ++index) {
        const QVariant& value = rows.at(index);
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap row = value.toMap();
        if (row.size() > allowed.size() || row.value(QStringLiteral("name")).type() != QVariant::String
            || row.value(QStringLiteral("name")).isNull() || row.value(QStringLiteral("name")).toString() != names.at(index)) return false;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) if (!allowed.contains(it.key())) return false;
        double type = 0.0, numeric = 0.0;
        if (!number(row.value(QStringLiteral("mavParamType")), &type) || std::floor(type) != type || type < 1.0 || type > 10.0
            || !number(row.value(QStringLiteral("value")), &numeric)) return false;
        // The relevant local source parameters are REAL32. Accepting another
        // wire type or rounding an arbitrary double here would invent the
        // operands needed to match the firmware's float parameter operations.
        if (rateNames.contains(names.at(index)) && (type != 9.0 || !exactFloat(numeric))) return false;
        values->insert(names.at(index), numeric);
    }
    return true;
}
}

QVariantMap FleetMissionRateOverrideReview::evaluate(const QVariantList& parameters,
                                                     const QVariantList& guardParameters)
{
    const auto result = [](bool ok, const QString& reason, const QVariantList& expected = QVariantList(),
                           const QVariantMap& active = QVariantMap()) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
            {QStringLiteral("model"), QStringLiteral("local-px4-rate-overrides-v1")},
            {QStringLiteral("expectedParameters"), expected}, {QStringLiteral("macroOverridesActive"), active},
            {QStringLiteral("actualInternalValuesProven"), false}};
    };
    const QString xyAll = QStringLiteral("MPC_XY_VEL_ALL"), zAll = QStringLiteral("MPC_Z_VEL_ALL");
    const QSet<QString> rates{QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_VEL_MAX_UP"),
        QStringLiteral("MPC_Z_V_AUTO_DN"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_LAND_SPEED")};
    QMap<QString, double> observed;
    if (!readRows(parameters, legacyNames(), rates, &observed)
        || !readRows(guardParameters, QStringList{xyAll, zAll}, QSet<QString>{xyAll, zAll}, &observed))
        return result(false, QStringLiteral("rate-override-complete-real32-content-required"));

    const float xy = static_cast<float>(observed.value(xyAll));
    const float z = static_cast<float>(observed.value(zAll));
    const bool xyActive = xy >= 0.f, zActive = z >= 0.f;
    QMap<QString, double> expected;
    if (xyActive) {
        expected.insert(QStringLiteral("MPC_XY_CRUISE"), static_cast<double>(xy));
        expected.insert(QStringLiteral("MPC_XY_VEL_MAX"), static_cast<double>(xy));
    }
    if (zActive) {
        expected.insert(QStringLiteral("MPC_Z_V_AUTO_UP"), static_cast<double>(z));
        expected.insert(QStringLiteral("MPC_Z_VEL_MAX_UP"), static_cast<double>(z));
        expected.insert(QStringLiteral("MPC_Z_V_AUTO_DN"), static_cast<double>(static_cast<float>(z * 0.75f)));
        expected.insert(QStringLiteral("MPC_Z_VEL_MAX_DN"), static_cast<double>(static_cast<float>(z * 0.75f)));
        expected.insert(QStringLiteral("MPC_TKO_SPEED"), static_cast<double>(static_cast<float>(z * 0.6f)));
        expected.insert(QStringLiteral("MPC_LAND_SPEED"), static_cast<double>(static_cast<float>(z * 0.5f)));
    }
    QVariantList completeExpected;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) {
        if (!std::isfinite(it.value()) || it.value() <= 0.0 || it.value() != observed.value(it.key()))
            return result(false, QStringLiteral("rate-override-mismatch-") + it.key());
        completeExpected.append(QVariantMap{{QStringLiteral("name"), it.key()}, {QStringLiteral("value"), it.value()}});
    }
    return result(true, QStringLiteral(""), completeExpected,
        QVariantMap{{QStringLiteral("horizontal"), xyActive}, {QStringLiteral("vertical"), zActive}});
}

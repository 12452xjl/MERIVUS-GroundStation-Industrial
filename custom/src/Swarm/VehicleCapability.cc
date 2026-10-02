#include "VehicleCapability.h"

#include <QVariant>
#include <QtMath>

namespace {

int readSystemId(const QVariantMap& map, int fallback)
{
    const QVariant value = map.value(QStringLiteral("systemId"),
                                     map.value(QStringLiteral("vehicleId"),
                                               map.value(QStringLiteral("id"), fallback)));
    bool ok = false;
    const int systemId = value.toInt(&ok);
    return ok ? systemId : fallback;
}

double readDouble(const QVariantMap& map, const QString& key, double fallback)
{
    const QVariant value = map.value(key);
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const double result = value.toDouble(&ok);
    return ok ? result : fallback;
}

bool readBool(const QVariantMap& map, const QString& key, bool fallback)
{
    const QVariant value = map.value(key);
    return value.isValid() && !value.isNull() ? value.toBool() : fallback;
}

QStringList readCapabilities(const QVariantMap& map)
{
    const QVariant value = map.value(QStringLiteral("capabilities"));
    if (value.canConvert<QStringList>()) {
        return value.toStringList();
    }

    QStringList result;
    const QStringList values = value.toString().split(QChar(','), Qt::SkipEmptyParts);
    for (const QString& item : values) {
        const QString capability = item.trimmed();
        if (!capability.isEmpty()) {
            result.append(capability);
        }
    }
    return result;
}

QDateTime readDateTime(const QVariantMap& map, const QString& key)
{
    const QVariant value = map.value(key);
    if (value.canConvert<QDateTime>()) {
        const QDateTime dateTime = value.toDateTime();
        if (dateTime.isValid()) {
            return dateTime.toUTC();
        }
    }
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
    return parsed.isValid() ? parsed.toUTC() : QDateTime();
}

}

VehicleCapability VehicleCapability::fromVariantMap(const QVariantMap& descriptor,
                                                     int fallbackSystemId)
{
    VehicleCapability result;
    result.systemId = readSystemId(descriptor, fallbackSystemId);
    result.vehicleType = descriptor.value(QStringLiteral("vehicleType"),
                                          descriptor.value(QStringLiteral("type"))).toString();
    result.autopilotType = descriptor.value(QStringLiteral("autopilotType"),
                                            descriptor.value(QStringLiteral("autopilot"))).toString();
    result.firmwareVersion = descriptor.value(QStringLiteral("firmwareVersion"),
                                               descriptor.value(QStringLiteral("firmware"))).toString();
    result.capabilities = readCapabilities(descriptor);
    result.maxSpeedMps = readDouble(descriptor, QStringLiteral("maxSpeedMps"), -1.0);
    result.enduranceSeconds = readDouble(descriptor, QStringLiteral("enduranceSeconds"),
                                         readDouble(descriptor, QStringLiteral("enduranceSec"), -1.0));
    result.canUseGnss = readBool(descriptor, QStringLiteral("canUseGnss"),
                                 descriptor.value(QStringLiteral("hasGnss"), false).toBool());
    result.canUseRtk = readBool(descriptor, QStringLiteral("canUseRtk"), false);
    result.canUseVision = readBool(descriptor, QStringLiteral("canUseVision"),
                                   descriptor.value(QStringLiteral("hasCamera"), false).toBool());
    result.canUseRangeSensor = readBool(descriptor, QStringLiteral("canUseRangeSensor"),
                                        descriptor.value(QStringLiteral("hasRangeSensor"), false).toBool());
    result.canRelay = readBool(descriptor, QStringLiteral("canRelay"), false);
    result.role = descriptor.value(QStringLiteral("role")).toString();
    result.online = readBool(descriptor, QStringLiteral("online"), false);
    result.lastSeenUtc = readDateTime(descriptor, QStringLiteral("lastSeenUtc"));
    if (!result.lastSeenUtc.isValid()) {
        result.lastSeenUtc = readDateTime(descriptor, QStringLiteral("lastSeen"));
    }

    result.batteryPercent = readDouble(descriptor, QStringLiteral("batteryPercent"),
                                       readDouble(descriptor, QStringLiteral("battery"), -1.0));
    result.linkQualityPercent = qRound(readDouble(descriptor, QStringLiteral("linkQualityPercent"),
                                                 readDouble(descriptor, QStringLiteral("linkQuality"), -1.0)));
    result.estimatorHealthKnown = descriptor.contains(QStringLiteral("estimatorHealthy"));
    result.estimatorHealthy = readBool(descriptor, QStringLiteral("estimatorHealthy"), true);
    result.positionHealthKnown = descriptor.contains(QStringLiteral("positionHealthy"));
    result.positionHealthy = readBool(descriptor, QStringLiteral("positionHealthy"), true);
    result.hasFault = readBool(descriptor, QStringLiteral("hasFault"), false);
    result.faultSummary = descriptor.value(QStringLiteral("faultSummary")).toString();
    result.metadata = descriptor.value(QStringLiteral("metadata")).toMap();
    return result;
}

QVariantMap VehicleCapability::toVariantMap() const
{
    QVariantMap result;
    result[QStringLiteral("systemId")] = systemId;
    result[QStringLiteral("vehicleId")] = systemId;
    result[QStringLiteral("vehicleType")] = vehicleType;
    result[QStringLiteral("autopilotType")] = autopilotType;
    result[QStringLiteral("firmwareVersion")] = firmwareVersion;
    result[QStringLiteral("capabilities")] = capabilities;
    result[QStringLiteral("maxSpeedMps")] = maxSpeedMps;
    result[QStringLiteral("enduranceSeconds")] = enduranceSeconds;
    result[QStringLiteral("canUseGnss")] = canUseGnss;
    result[QStringLiteral("canUseRtk")] = canUseRtk;
    result[QStringLiteral("canUseVision")] = canUseVision;
    result[QStringLiteral("canUseRangeSensor")] = canUseRangeSensor;
    result[QStringLiteral("canRelay")] = canRelay;
    result[QStringLiteral("role")] = role;
    result[QStringLiteral("online")] = online;
    result[QStringLiteral("lastSeenUtc")] = lastSeenUtc.isValid()
            ? lastSeenUtc.toUTC().toString(Qt::ISODateWithMs)
            : QString();
    result[QStringLiteral("batteryPercent")] = batteryPercent;
    result[QStringLiteral("linkQualityPercent")] = linkQualityPercent;
    result[QStringLiteral("estimatorHealthy")] = estimatorHealthy;
    result[QStringLiteral("estimatorHealthKnown")] = estimatorHealthKnown;
    result[QStringLiteral("positionHealthy")] = positionHealthy;
    result[QStringLiteral("positionHealthKnown")] = positionHealthKnown;
    result[QStringLiteral("hasFault")] = hasFault;
    result[QStringLiteral("faultSummary")] = faultSummary;
    result[QStringLiteral("healthScore")] = healthScore;
    result[QStringLiteral("healthState")] = healthState;
    result[QStringLiteral("healthReason")] = healthReason;
    result[QStringLiteral("metadata")] = metadata;
    return result;
}

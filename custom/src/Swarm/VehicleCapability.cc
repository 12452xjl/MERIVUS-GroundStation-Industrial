#include "VehicleCapability.h"
#include "FleetCapabilityEvidence.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QVariantHash>
#include <QtMath>

#include <algorithm>

namespace {

QJsonValue canonicalJsonValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QJsonValue(QJsonValue::Null);
    }

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        QJsonObject object;
        QStringList keys = map.keys();
        std::sort(keys.begin(), keys.end());
        for (const QString& key : keys) {
            object.insert(key, canonicalJsonValue(map.value(key)));
        }
        return object;
    }

    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        QJsonObject object;
        QStringList keys = hash.keys();
        std::sort(keys.begin(), keys.end());
        for (const QString& key : keys) {
            object.insert(key, canonicalJsonValue(hash.value(key)));
        }
        return object;
    }

    if (value.type() == QVariant::List) {
        QJsonArray array;
        const QVariantList list = value.toList();
        for (const QVariant& item : list) {
            array.append(canonicalJsonValue(item));
        }
        return array;
    }

    if (value.type() == QVariant::StringList) {
        QJsonArray array;
        for (const QString& item : value.toStringList()) {
            array.append(item);
        }
        return array;
    }

    return QJsonValue::fromVariant(value);
}

QString trimmedString(const QVariantMap& map, const QString& key)
{
    return map.value(key).toString().trimmed();
}

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
    QStringList result;
    // Match the Registry/review token contract explicitly. In particular a
    // CSV value must not become a single QStringList element via canConvert.
    if (map.contains(QStringLiteral("capabilities")))
        FleetCapabilityEvidence::canonicalCapabilities(map.value(QStringLiteral("capabilities")), &result);
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

void preserveDiscoveryEvidence(QVariantMap& metadata, const QVariantMap& descriptor)
{
    // Discovery is produced as a top-level adapter contract, while the
    // registry deliberately keeps backend-specific fields in metadata.  Copy
    // the evidence fields during the initial parse so a newly registered
    // vehicle does not lose its verified/invalid state before the first merge.
    static const QStringList keys = {
        QStringLiteral("verifiedCapabilities"),
        QStringLiteral("declaredVerifiedCapabilities"),
        QStringLiteral("capabilityReviewContext"),
        QStringLiteral("capabilityApproval"),
        QStringLiteral("importedCapabilityApproval"),
        QStringLiteral("capabilityEvidenceState"),
        QStringLiteral("configurationEvidenceSources"),
        QStringLiteral("discoveryState"),
        QStringLiteral("discoveryErrors"),
        QStringLiteral("discoveredAtUtc"),
        QStringLiteral("positionEvidenceSource")
    };
    for (const QString& key : keys) {
        if (descriptor.contains(key)) {
            metadata.insert(key, descriptor.value(key));
        }
    }
}

void exposeDiscoveryEvidence(QVariantMap& descriptor, const QVariantMap& metadata)
{
    // Keep the public registry snapshot compatible with the discovery
    // descriptor contract as well as the metadata fallback used internally.
    // This makes a registry -> QML/backend round-trip lossless for evidence.
    static const QStringList keys = {
        QStringLiteral("verifiedCapabilities"),
        QStringLiteral("declaredVerifiedCapabilities"),
        QStringLiteral("capabilityReviewContext"),
        QStringLiteral("capabilityApproval"),
        QStringLiteral("importedCapabilityApproval"),
        QStringLiteral("capabilityEvidenceState"),
        QStringLiteral("configurationEvidenceSources"),
        QStringLiteral("discoveryState"),
        QStringLiteral("discoveryErrors"),
        QStringLiteral("discoveredAtUtc"),
        QStringLiteral("positionEvidenceSource")
    };
    for (const QString& key : keys) {
        if (metadata.contains(key)) {
            descriptor.insert(key, metadata.value(key));
        }
    }
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
    result.estimatorHealthKnown = descriptor.contains(QStringLiteral("estimatorHealthKnown"))
        ? readBool(descriptor, QStringLiteral("estimatorHealthKnown"), false)
        : descriptor.contains(QStringLiteral("estimatorHealthy"));
    result.estimatorHealthy = readBool(descriptor, QStringLiteral("estimatorHealthy"), true);
    result.positionHealthKnown = descriptor.contains(QStringLiteral("positionHealthKnown"))
        ? readBool(descriptor, QStringLiteral("positionHealthKnown"), false)
        : descriptor.contains(QStringLiteral("positionHealthy"));
    result.positionHealthy = readBool(descriptor, QStringLiteral("positionHealthy"), true);
    result.hasFault = readBool(descriptor, QStringLiteral("hasFault"), false);
    result.faultSummary = descriptor.value(QStringLiteral("faultSummary")).toString();
    result.faultHealthKnown = readBool(descriptor, QStringLiteral("faultHealthKnown"), false);
    result.faultEvidenceAgeMs = qRound(readDouble(descriptor,
                                                  QStringLiteral("faultEvidenceAgeMs"), -1.0));
    result.metadata = descriptor.value(QStringLiteral("metadata")).toMap();
    preserveDiscoveryEvidence(result.metadata, descriptor);

    // Configuration is an explicit, static evidence map.  Do not derive it
    // from the full metadata map because that map also contains changing
    // battery/link/health observations and would make the fingerprint flap
    // on every heartbeat.
    result.configuration = descriptor.value(QStringLiteral("configuration")).toMap();
    if (result.configuration.isEmpty()) {
        result.configuration = result.metadata.value(QStringLiteral("configuration")).toMap();
    }
    // Always compute the observed fingerprint locally when the evidence map is
    // present. A vehicle-supplied hash must not be able to hide a changed map.
    result.configurationFingerprint = result.configuration.isEmpty()
        ? trimmedString(descriptor, QStringLiteral("configurationFingerprint")).toLower()
        : fingerprintForConfiguration(result.configuration);
    result.baselineSource = trimmedString(descriptor, QStringLiteral("baselineSource"));
    // A baseline is an approval record. Only the explicitly trusted local
    // manifest source may establish it; telemetry with the same field is
    // treated as unverified evidence.
    if (result.baselineSource == QStringLiteral("trusted_manifest")
        || result.baselineSource == QStringLiteral("local_approval")) {
        result.baselineConfigurationFingerprint = trimmedString(
            descriptor, QStringLiteral("baselineConfigurationFingerprint")).toLower();
        if (result.baselineConfigurationFingerprint.isEmpty()) {
            const QVariantMap baseline = descriptor.value(QStringLiteral("configurationBaseline")).toMap();
            if (!baseline.isEmpty()) {
                result.baselineConfigurationFingerprint = fingerprintForConfiguration(baseline);
            }
        }
    }
    if (result.configurationFingerprint.isEmpty()) {
        result.configurationState = QStringLiteral("unknown");
    } else if (result.baselineConfigurationFingerprint.isEmpty()) {
        result.configurationState = QStringLiteral("unverified");
    } else {
        result.configurationState = result.configurationFingerprint
                == result.baselineConfigurationFingerprint
            ? QStringLiteral("baseline")
            : QStringLiteral("modified");
    }
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
    result[QStringLiteral("faultHealthKnown")] = faultHealthKnown;
    result[QStringLiteral("faultEvidenceAgeMs")] = faultEvidenceAgeMs;
    result[QStringLiteral("healthScore")] = healthScore;
    result[QStringLiteral("healthState")] = healthState;
    result[QStringLiteral("healthReason")] = healthReason;
    result[QStringLiteral("configuration")] = configuration;
    result[QStringLiteral("configurationFingerprint")] = configurationFingerprint;
    result[QStringLiteral("baselineConfigurationFingerprint")] = baselineConfigurationFingerprint;
    result[QStringLiteral("baselineSource")] = baselineSource;
    result[QStringLiteral("configurationState")] = configurationState;
    result[QStringLiteral("metadata")] = metadata;
    exposeDiscoveryEvidence(result, metadata);
    return result;
}

QString VehicleCapability::fingerprintForConfiguration(const QVariantMap& configuration)
{
    if (configuration.isEmpty()) {
        return QString();
    }

    const QJsonValue canonical = canonicalJsonValue(configuration);
    const QByteArray payload = QJsonDocument(canonical.toObject()).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

#include "FleetCapabilityDiscovery.h"

#include "VehicleCapability.h"

#include <QDateTime>
#include <QList>
#include <QStringList>
#include <QVariantHash>
#include <QtMath>

#include <limits>

namespace {

constexpr qint64 kFutureTimestampToleranceMs = 1000;
constexpr int kMaxCapabilityCount = 128;
constexpr int kMaxCapabilityLength = 128;
constexpr int kMaxEvidenceDepth = 8;
constexpr int kMaxEvidenceNodes = 2048;
constexpr qint64 kMaxEvidenceScalarBytes = 128LL * 1024LL;

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

bool strictInteger(const QVariant& value, int* result = nullptr)
{
    if (!numericVariant(value)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number < static_cast<double>(std::numeric_limits<int>::min())
        || number > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (result) *result = static_cast<int>(number);
    return true;
}

bool strictBoolean(const QVariantMap& map, const QString& key)
{
    return !map.contains(key) || map.value(key).type() == QVariant::Bool;
}

bool strictString(const QVariantMap& map, const QString& key)
{
    return !map.contains(key) || map.value(key).type() == QVariant::String;
}

bool strictNumberInRange(const QVariantMap& map, const QString& key,
                         double minimum, double maximum)
{
    if (!map.contains(key)) return true;
    const QVariant value = map.value(key);
    if (!value.isValid() || value.isNull()) return true;
    if (!numericVariant(value)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && qIsFinite(number) && number >= minimum && number <= maximum;
}

bool boundedEvidenceValue(const QVariant& value,
                          int depth,
                          int* nodeCount,
                          qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxEvidenceDepth) return false;
    if (++(*nodeCount) > kMaxEvidenceNodes) return false;
    if (!value.isValid() || value.isNull()) return true;

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > kMaxCapabilityLength) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxEvidenceScalarBytes
                || !boundedEvidenceValue(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > kMaxCapabilityLength) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxEvidenceScalarBytes
                || !boundedEvidenceValue(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!boundedEvidenceValue(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            if (++(*nodeCount) > kMaxEvidenceNodes) return false;
            *scalarBytes += item.toUtf8().size();
            if (*scalarBytes > kMaxEvidenceScalarBytes) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        *scalarBytes += value.toString().toUtf8().size();
    } else if (value.type() == QVariant::ByteArray) {
        *scalarBytes += value.toByteArray().size();
    } else if (value.type() == QVariant::DateTime) {
        *scalarBytes += value.toDateTime().toUTC().toString(Qt::ISODateWithMs).toUtf8().size();
    } else if (value.type() == QVariant::Double) {
        if (!qIsFinite(value.toDouble())) return false;
    } else if (value.type() != QVariant::Bool
               && value.type() != QVariant::Int
               && value.type() != QVariant::UInt
               && value.type() != QVariant::LongLong
               && value.type() != QVariant::ULongLong) {
        return false;
    }
    return *scalarBytes <= kMaxEvidenceScalarBytes;
}

bool boundedCapabilityList(const QVariant& value)
{
    QStringList values;
    if (value.type() == QVariant::String) {
        if (value.toString().toUtf8().size() > kMaxEvidenceScalarBytes) return false;
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

bool validTimestamp(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return false;
    QDateTime timestamp;
    if (value.type() == QVariant::DateTime) {
        timestamp = value.toDateTime();
    } else if (value.type() == QVariant::String) {
        const QString text = value.toString().trimmed();
        timestamp = QDateTime::fromString(text, Qt::ISODateWithMs);
        if (!timestamp.isValid()) timestamp = QDateTime::fromString(text, Qt::ISODate);
    } else {
        return false;
    }
    if (!timestamp.isValid()) return false;
    return timestamp.toUTC().msecsTo(QDateTime::currentDateTimeUtc())
        >= -kFutureTimestampToleranceMs;
}

bool validStringListValue(const QVariant& value)
{
    if (!boundedCapabilityList(value)) return false;
    if (value.type() == QVariant::String) return true;
    if (value.type() == QVariant::StringList) {
        return true;
    }
    if (value.type() != QVariant::List) return false;
    for (const QVariant& entry : value.toList()) {
        if (entry.type() != QVariant::String) return false;
    }
    return true;
}

void copyIfPresent(QVariantMap& target, const QVariantMap& source,
                   const QString& sourceKey, const QString& targetKey = QString())
{
    if (!source.contains(sourceKey)) {
        return;
    }
    const QVariant value = source.value(sourceKey);
    if (!value.isValid() || value.isNull()) {
        return;
    }
    target.insert(targetKey.isEmpty() ? sourceKey : targetKey, value);
}

void copyFirst(QVariantMap& target, const QList<QVariantMap>& sources,
               const QString& key, const QString& targetKey = QString())
{
    for (const QVariantMap& source : sources) {
        const QVariant value = source.value(key);
        if (source.contains(key) && value.isValid() && !value.isNull()) {
            target.insert(targetKey.isEmpty() ? key : targetKey, value);
            return;
        }
    }
}

QStringList readStringList(const QVariant& value)
{
    if (value.canConvert<QStringList>()) {
        return value.toStringList();
    }
    QStringList result;
    for (const QString& item : value.toString().split(QChar(','), Qt::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

bool appendCapabilities(QStringList& target, const QVariantMap& source)
{
    if (!source.contains(QStringLiteral("capabilities"))) return true;
    const QVariant raw = source.value(QStringLiteral("capabilities"));
    if (!boundedCapabilityList(raw)) {
        return false;
    }
    const QStringList values = readStringList(raw);
    for (const QString& value : values) {
        if (!value.trimmed().isEmpty() && !target.contains(value.trimmed(), Qt::CaseInsensitive)) {
            target.append(value.trimmed());
        }
    }
    return true;
}

void appendDiscoveryError(QVariantMap& descriptor, const QString& error)
{
    QStringList errors = descriptor.value(QStringLiteral("discoveryErrors")).toStringList();
    if (!errors.contains(error)) errors.append(error);
    descriptor.insert(QStringLiteral("discoveryErrors"), errors);
}

void addEvidenceSource(QStringList& sources, const QString& source)
{
    if (!source.isEmpty() && !sources.contains(source)) {
        sources.append(source);
    }
}

void collectIdentityCandidates(const QVariantMap& source, QList<int>& candidates,
                               bool& invalid)
{
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!source.contains(key)) {
            continue;
        }
        int candidate = 0;
        if (!strictInteger(source.value(key), &candidate)
            || candidate <= 0 || candidate > 255) {
            invalid = true;
            continue;
        }
        candidates.append(candidate);
    }
}

QString derivedConfigurationState(const QString& observed, const QString& baseline)
{
    if (observed.isEmpty()) {
        return QStringLiteral("unknown");
    }
    if (baseline.isEmpty()) {
        return QStringLiteral("unverified");
    }
    return observed == baseline ? QStringLiteral("baseline") : QStringLiteral("modified");
}

bool validFingerprint(const QString& value)
{
    if (value.size() != 64) {
        return false;
    }
    for (const QChar character : value) {
        if (!character.isDigit()
            && (character < QLatin1Char('a') || character > QLatin1Char('f'))
            && (character < QLatin1Char('A') || character > QLatin1Char('F'))) {
            return false;
        }
    }
    return true;
}

}

FleetCapabilityDiscovery::FleetCapabilityDiscovery(QObject* parent)
    : QObject(parent)
{
}

QVariantMap FleetCapabilityDiscovery::buildDescriptor(const QVariantMap& heartbeat,
                                                       const QVariantMap& autopilotVersion,
                                                       const QVariantMap& componentMetadata,
                                                       const QVariantMap& parameterSnapshot,
                                                       const QVariantMap& trustedManifest) const
{
    const QList<QVariantMap> sources = { heartbeat, autopilotVersion, componentMetadata };
    const QList<QVariantMap> staticSources = { autopilotVersion, componentMetadata, heartbeat };
    QVariantMap descriptor;

    bool discoveryInputBounded = true;
    for (const QVariantMap& source : sources) {
        int nodeCount = 0;
        qint64 scalarBytes = 0;
        if (!boundedEvidenceValue(source, 0, &nodeCount, &scalarBytes)) {
            discoveryInputBounded = false;
        }
    }
    {
        int nodeCount = 0;
        qint64 scalarBytes = 0;
        if (!boundedEvidenceValue(parameterSnapshot, 0, &nodeCount, &scalarBytes)) {
            discoveryInputBounded = false;
        }
        nodeCount = 0;
        scalarBytes = 0;
        if (!boundedEvidenceValue(trustedManifest, 0, &nodeCount, &scalarBytes)) {
            discoveryInputBounded = false;
        }
    }
    if (!discoveryInputBounded) {
        appendDiscoveryError(descriptor, QStringLiteral("discovery_input_resource_bounds"));
    }

    QList<int> identityCandidates;
    bool identityInvalid = false;
    for (const QVariantMap& source : sources) {
        collectIdentityCandidates(source, identityCandidates, identityInvalid);
    }
    bool identityConflict = identityInvalid;
    if (!identityCandidates.isEmpty()) {
        const int firstId = identityCandidates.first();
        for (const int candidate : identityCandidates) {
            if (candidate != firstId) {
                identityConflict = true;
                break;
            }
        }
    }

    if (!discoveryInputBounded) {
        // Preserve only bounded diagnostics and an unambiguous numeric ID.
        // Do not traverse, merge, or hash data already known to be too large.
        if (!identityConflict && !identityCandidates.isEmpty()) {
            descriptor.insert(QStringLiteral("systemId"), identityCandidates.first());
        } else {
            appendDiscoveryError(descriptor, QStringLiteral("system_id_invalid"));
        }
        descriptor.insert(QStringLiteral("discoveryState"), QStringLiteral("invalid"));
        descriptor.insert(QStringLiteral("configurationState"), QStringLiteral("unknown"));
        descriptor.insert(QStringLiteral("discoveredAtUtc"),
                          QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        return descriptor;
    }

    copyFirst(descriptor, sources, QStringLiteral("systemId"));
    if (!descriptor.contains(QStringLiteral("systemId"))) {
        copyFirst(descriptor, sources, QStringLiteral("vehicleId"), QStringLiteral("systemId"));
    }
    // Static capability evidence prefers protocol/metadata sources over the
    // low-trust heartbeat. Dynamic health/connection fields below retain the
    // heartbeat-first order so stale version metadata cannot overwrite live
    // telemetry and live telemetry cannot overwrite static identity claims.
    copyFirst(descriptor, staticSources, QStringLiteral("vehicleType"));
    copyFirst(descriptor, staticSources, QStringLiteral("autopilotType"));
    copyFirst(descriptor, staticSources, QStringLiteral("firmwareVersion"));
    copyFirst(descriptor, sources, QStringLiteral("online"));
    copyFirst(descriptor, sources, QStringLiteral("lastSeenUtc"));
    copyFirst(descriptor, sources, QStringLiteral("batteryPercent"));
    copyFirst(descriptor, sources, QStringLiteral("linkQualityPercent"));
    copyFirst(descriptor, sources, QStringLiteral("estimatorHealthy"));
    copyFirst(descriptor, sources, QStringLiteral("estimatorHealthKnown"));
    copyFirst(descriptor, sources, QStringLiteral("positionHealthy"));
    copyFirst(descriptor, sources, QStringLiteral("positionHealthKnown"));
    copyFirst(descriptor, sources, QStringLiteral("hasFault"));
    copyFirst(descriptor, sources, QStringLiteral("faultSummary"));
    copyFirst(descriptor, sources, QStringLiteral("faultHealthKnown"));
    copyFirst(descriptor, sources, QStringLiteral("faultEvidenceAgeMs"));
    copyFirst(descriptor, staticSources, QStringLiteral("maxSpeedMps"));
    copyFirst(descriptor, staticSources, QStringLiteral("enduranceSeconds"));
    copyFirst(descriptor, staticSources, QStringLiteral("canUseGnss"));
    copyFirst(descriptor, staticSources, QStringLiteral("canUseRtk"));
    copyFirst(descriptor, staticSources, QStringLiteral("canUseVision"));
    copyFirst(descriptor, staticSources, QStringLiteral("canUseRangeSensor"));
    copyFirst(descriptor, staticSources, QStringLiteral("canRelay"));
    copyFirst(descriptor, sources, QStringLiteral("role"));

    QStringList capabilities;
    QStringList declaredVerifiedCapabilities;
    bool capabilityEvidenceValid = true;
    for (const QVariantMap& source : sources) {
        capabilityEvidenceValid = appendCapabilities(capabilities, source)
            && capabilityEvidenceValid;
    }
    // Protocol/metadata/manifest capability strings are declarations, not a
    // transferable engineering approval or physical inspection. Only the
    // registry's explicit native revision-bound review produces effective
    // verifiedCapabilities; normalization never manufactures that record.
    for (const QVariantMap& source : {autopilotVersion, componentMetadata}) {
        capabilityEvidenceValid = appendCapabilities(declaredVerifiedCapabilities, source)
            && capabilityEvidenceValid;
    }
    for (const QVariantMap& source : {heartbeat, autopilotVersion, componentMetadata, trustedManifest}) {
        for (const QString& key : {QStringLiteral("verifiedCapabilities"), QStringLiteral("declaredVerifiedCapabilities")}) {
            if (!source.contains(key)) continue;
            const QVariantMap declaration{{QStringLiteral("capabilities"), source.value(key)}};
            capabilityEvidenceValid = appendCapabilities(declaredVerifiedCapabilities, declaration)
                && capabilityEvidenceValid;
        }
    }
    if (!capabilities.isEmpty()) {
        descriptor.insert(QStringLiteral("capabilities"), capabilities);
    }
    if (!declaredVerifiedCapabilities.isEmpty()) {
        descriptor.insert(QStringLiteral("declaredVerifiedCapabilities"), declaredVerifiedCapabilities);
        descriptor.insert(QStringLiteral("capabilityEvidenceState"), QStringLiteral("declared-unreviewed"));
    } else if (!capabilities.isEmpty()) {
        descriptor.insert(QStringLiteral("capabilityEvidenceState"), QStringLiteral("observed-unverified"));
    }
    if (!capabilityEvidenceValid) {
        appendDiscoveryError(descriptor, QStringLiteral("capabilities_type_invalid"));
    }

    QVariantMap configuration;
    QStringList evidenceSources;
    for (const QVariantMap& source : sources) {
        if (source.contains(QStringLiteral("configuration"))) {
            int nodeCount = 0;
            qint64 scalarBytes = 0;
            const QVariant configurationValue = source.value(QStringLiteral("configuration"));
            if (configurationValue.type() != QVariant::Map) {
                appendDiscoveryError(descriptor, QStringLiteral("configuration_type_invalid"));
                continue;
            }
            if (!boundedEvidenceValue(configurationValue, 0, &nodeCount, &scalarBytes)) {
                appendDiscoveryError(descriptor, QStringLiteral("configuration_resource_bounds"));
                continue;
            }
        }
        const QVariantMap nested = source.value(QStringLiteral("configuration")).toMap();
        for (auto it = nested.constBegin(); it != nested.constEnd(); ++it) {
            configuration.insert(it.key(), it.value());
        }
    }
    if (!heartbeat.isEmpty()) {
        addEvidenceSource(evidenceSources, QStringLiteral("HEARTBEAT"));
    }
    if (!autopilotVersion.isEmpty()) {
        addEvidenceSource(evidenceSources, QStringLiteral("AUTOPILOT_VERSION"));
        copyIfPresent(configuration, autopilotVersion, QStringLiteral("board"));
        copyIfPresent(configuration, autopilotVersion, QStringLiteral("boardVendor"), QStringLiteral("boardVendor"));
        copyIfPresent(configuration, autopilotVersion, QStringLiteral("boardProduct"), QStringLiteral("boardProduct"));
        copyIfPresent(configuration, autopilotVersion, QStringLiteral("gitHash"));
    }
    if (!componentMetadata.isEmpty()) {
        addEvidenceSource(evidenceSources, QStringLiteral("COMPONENT_METADATA"));
        copyIfPresent(configuration, componentMetadata, QStringLiteral("sensors"));
        copyIfPresent(configuration, componentMetadata, QStringLiteral("frame"));
        copyIfPresent(configuration, componentMetadata, QStringLiteral("esc"));
    }
    int parameterNodeCount = 0;
    qint64 parameterScalarBytes = 0;
    const bool parameterSnapshotBounded = boundedEvidenceValue(
        parameterSnapshot, 0, &parameterNodeCount, &parameterScalarBytes);
    if (!parameterSnapshot.isEmpty() && !parameterSnapshotBounded) {
        appendDiscoveryError(descriptor, QStringLiteral("parameter_snapshot_resource_bounds"));
    } else if (!parameterSnapshot.isEmpty()) {
        addEvidenceSource(evidenceSources, QStringLiteral("PARAMETER_SNAPSHOT"));
        configuration.insert(QStringLiteral("parametersHash"),
                             VehicleCapability::fingerprintForConfiguration(parameterSnapshot));
    }
    if (!configuration.isEmpty()) {
        int combinedNodes = 0;
        qint64 combinedBytes = 0;
        if (!boundedEvidenceValue(configuration, 1, &combinedNodes, &combinedBytes)) {
            // Individually bounded sources can exceed the budget when merged.
            // In that case never call the recursive canonical fingerprint.
            appendDiscoveryError(descriptor, QStringLiteral("configuration_resource_bounds"));
        } else {
            descriptor.insert(QStringLiteral("configuration"), configuration);
            descriptor.insert(QStringLiteral("configurationFingerprint"),
                              VehicleCapability::fingerprintForConfiguration(configuration));
        }
    }

    // Only a locally supplied manifest is allowed to establish the approved
    // baseline. Never copy a baseline claim from heartbeat/version/metadata.
    const QVariantMap trustedConfiguration = trustedManifest.value(QStringLiteral("configuration")).toMap();
    int trustedNodeCount = 0;
    qint64 trustedScalarBytes = 0;
    const bool trustedConfigurationBounded = boundedEvidenceValue(
        trustedConfiguration, 0, &trustedNodeCount, &trustedScalarBytes);
    if (!trustedConfiguration.isEmpty() && !trustedConfigurationBounded) {
        appendDiscoveryError(descriptor, QStringLiteral("trusted_configuration_resource_bounds"));
    }
    QString baselineFingerprint = trustedManifest.value(
        QStringLiteral("baselineConfigurationFingerprint")).toString().trimmed().toLower();
    if (baselineFingerprint.isEmpty() && !trustedConfiguration.isEmpty()
        && trustedConfigurationBounded) {
        baselineFingerprint = VehicleCapability::fingerprintForConfiguration(trustedConfiguration);
    }
    if (!baselineFingerprint.isEmpty()) {
        descriptor.insert(QStringLiteral("baselineConfigurationFingerprint"), baselineFingerprint);
        descriptor.insert(QStringLiteral("baselineSource"), QStringLiteral("trusted_manifest"));
    }

    if (!evidenceSources.isEmpty()) {
        descriptor.insert(QStringLiteral("configurationEvidenceSources"), evidenceSources);
    }
    descriptor.insert(QStringLiteral("discoveredAtUtc"),
                     QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    const QString observedFingerprint = descriptor.value(
        QStringLiteral("configurationFingerprint")).toString().trimmed();
    descriptor.insert(QStringLiteral("configurationState"),
                     derivedConfigurationState(observedFingerprint, baselineFingerprint));
    int discoveredSystemId = 0;
    if (identityConflict) {
        // Never let a source conflict be reduced to the first map's ID. An
        // invalid descriptor is intentionally rejected by FleetRegistry
        // rather than being registered under an arbitrary vehicle.
        descriptor.remove(QStringLiteral("systemId"));
        descriptor.remove(QStringLiteral("vehicleId"));
        descriptor.remove(QStringLiteral("id"));
        appendDiscoveryError(descriptor, QStringLiteral("identity_conflict"));
    }
    const bool discoveredIdValid = descriptor.contains(QStringLiteral("systemId"))
        && strictInteger(descriptor.value(QStringLiteral("systemId")), &discoveredSystemId)
        && discoveredSystemId > 0 && discoveredSystemId <= 255;
    descriptor.insert(QStringLiteral("discoveryState"),
                     !identityConflict && discoveredIdValid && capabilityEvidenceValid
                         ? QStringLiteral("partial") : QStringLiteral("invalid"));
    if (!identityConflict && discoveredIdValid && capabilityEvidenceValid
        && (!autopilotVersion.isEmpty() || !componentMetadata.isEmpty() || !parameterSnapshot.isEmpty())) {
        descriptor.insert(QStringLiteral("discoveryState"), QStringLiteral("complete"));
    }
    if (!descriptor.value(QStringLiteral("discoveryErrors")).toStringList().isEmpty()) {
        descriptor.insert(QStringLiteral("discoveryState"), QStringLiteral("invalid"));
    }

    // Keep the normalizer self-auditing.  Adapters may call buildDescriptor()
    // directly and bypass the live QGC controller, so malformed optional
    // fields must remain visible as an invalid discovery result instead of
    // looking like a trustworthy partial passport.  Do not discard the
    // descriptor here: callers still need the identity and evidence errors
    // for diagnostics and can decide whether to register it.
    const QVariantMap validation = validateDescriptor(descriptor);
    if (!validation.value(QStringLiteral("valid")).toBool()) {
        QStringList errors = descriptor.value(QStringLiteral("discoveryErrors")).toStringList();
        for (const QString& error : validation.value(QStringLiteral("errors")).toStringList()) {
            if (!errors.contains(error)) {
                errors.append(error);
            }
        }
        descriptor.insert(QStringLiteral("discoveryErrors"), errors);
        descriptor.insert(QStringLiteral("discoveryState"), QStringLiteral("invalid"));
    }
    return descriptor;
}

QVariantMap FleetCapabilityDiscovery::validateDescriptor(const QVariantMap& descriptor) const
{
    QStringList errors;
    QStringList warnings;
    int descriptorNodeCount = 0;
    qint64 descriptorScalarBytes = 0;
    if (!boundedEvidenceValue(descriptor, 0, &descriptorNodeCount, &descriptorScalarBytes)) {
        errors.append(QStringLiteral("descriptor_resource_bounds"));
        // Validation must not hash an over-depth tree after discovering the
        // resource error; callers can retain the bounded result for diagnosis.
        return QVariantMap{{QStringLiteral("valid"), false},
                           {QStringLiteral("systemId"), 0},
                           {QStringLiteral("configurationState"), QStringLiteral("unknown")},
                           {QStringLiteral("errors"), errors},
                           {QStringLiteral("warnings"), warnings}};
    }
    bool systemIdFound = false;
    bool systemIdOk = true;
    int systemId = 0;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!descriptor.contains(key)) continue;
        int candidate = 0;
        if (!strictInteger(descriptor.value(key), &candidate)
            || candidate <= 0 || candidate > 255) {
            systemIdOk = false;
            break;
        }
        if (systemIdFound && candidate != systemId) {
            systemIdOk = false;
            break;
        }
        systemIdFound = true;
        systemId = candidate;
    }
    if (!systemIdFound || !systemIdOk) {
        errors.append(QStringLiteral("system_id_invalid"));
    }

    for (const QString& key : {QStringLiteral("online"), QStringLiteral("estimatorHealthy"),
                               QStringLiteral("estimatorHealthKnown"), QStringLiteral("positionHealthy"),
                               QStringLiteral("positionHealthKnown"), QStringLiteral("hasFault"),
                               QStringLiteral("faultHealthKnown"),
                               QStringLiteral("canUseGnss"), QStringLiteral("canUseRtk"),
                               QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"),
                               QStringLiteral("canRelay"), QStringLiteral("configurationClear")}) {
        if (!strictBoolean(descriptor, key)) {
            errors.append(QStringLiteral("%1_type_invalid").arg(key));
        }
    }

    for (const QString& key : {QStringLiteral("vehicleType"), QStringLiteral("type"),
                               QStringLiteral("autopilotType"), QStringLiteral("autopilot"),
                               QStringLiteral("firmwareVersion"), QStringLiteral("firmware"),
                               QStringLiteral("role"), QStringLiteral("configurationState"),
                               QStringLiteral("discoveryState"), QStringLiteral("discoveredAtUtc"),
                               QStringLiteral("faultSummary")}) {
        if (!strictString(descriptor, key)) {
            errors.append(QStringLiteral("%1_type_invalid").arg(key));
        }
    }

    if (descriptor.contains(QStringLiteral("discoveredAtUtc"))
        && !validTimestamp(descriptor.value(QStringLiteral("discoveredAtUtc")))) {
        errors.append(QStringLiteral("discoveredAtUtc_invalid"));
    }

    for (const QString& key : {QStringLiteral("maxSpeedMps"),
                               QStringLiteral("enduranceSeconds"),
                               QStringLiteral("enduranceSec")}) {
        if (!strictNumberInRange(descriptor, key, -1.0,
                                 std::numeric_limits<double>::max())) {
            errors.append(QStringLiteral("%1_invalid").arg(key));
        }
    }
    for (const QString& key : {QStringLiteral("batteryPercent"), QStringLiteral("battery"),
                               QStringLiteral("linkQualityPercent"), QStringLiteral("linkQuality")}) {
        if (!strictNumberInRange(descriptor, key, -1.0, 100.0)) {
            errors.append(QStringLiteral("%1_invalid").arg(key));
        }
    }
    if (descriptor.contains(QStringLiteral("faultEvidenceAgeMs"))
        && !strictNumberInRange(descriptor, QStringLiteral("faultEvidenceAgeMs"),
                                -1.0, static_cast<double>(std::numeric_limits<int>::max()))) {
        errors.append(QStringLiteral("faultEvidenceAgeMs_invalid"));
    }
    const QVariant faultKnownValue = descriptor.value(QStringLiteral("faultHealthKnown"));
    const bool faultKnown = descriptor.contains(QStringLiteral("faultHealthKnown"))
        && faultKnownValue.type() == QVariant::Bool
        && faultKnownValue.toBool();
    if (descriptor.contains(QStringLiteral("faultEvidenceAgeMs"))
        && !descriptor.contains(QStringLiteral("faultHealthKnown"))) {
        errors.append(QStringLiteral("fault_evidence_pair_missing"));
    }
    if (faultKnown && (!descriptor.contains(QStringLiteral("faultEvidenceAgeMs"))
                       || !strictNumberInRange(descriptor, QStringLiteral("faultEvidenceAgeMs"),
                                               0.0, static_cast<double>(std::numeric_limits<int>::max())))) {
        errors.append(QStringLiteral("fault_evidence_freshness_missing"));
    } else if (descriptor.contains(QStringLiteral("faultEvidenceAgeMs"))
               && !faultKnown) {
        const QVariant ageValue = descriptor.value(QStringLiteral("faultEvidenceAgeMs"));
        // Null is an allowed unknown value for the range validator. Only a
        // concrete non-negative number contradicts faultHealthKnown=false;
        // avoid QVariant's permissive null/text-to-zero conversion here.
        if (ageValue.isValid() && !ageValue.isNull() && numericVariant(ageValue)) {
            bool ageOk = false;
            const double age = ageValue.toDouble(&ageOk);
            if (ageOk && qIsFinite(age) && age >= 0.0) {
                errors.append(QStringLiteral("fault_evidence_known_mismatch"));
            }
        }
    }
    for (const QString& key : {QStringLiteral("capabilities"),
                               QStringLiteral("verifiedCapabilities"), QStringLiteral("declaredVerifiedCapabilities")}) {
        if (descriptor.contains(key)
            && !validStringListValue(descriptor.value(key))) {
            errors.append(QStringLiteral("%1_type_invalid").arg(key));
        }
    }
    if (descriptor.contains(QStringLiteral("capabilityEvidenceState"))
        && descriptor.value(QStringLiteral("capabilityEvidenceState")).type() != QVariant::String) {
        errors.append(QStringLiteral("capabilityEvidenceState_type_invalid"));
    }
    if (descriptor.contains(QStringLiteral("configurationEvidenceSources"))
        && !validStringListValue(descriptor.value(QStringLiteral("configurationEvidenceSources")))) {
        errors.append(QStringLiteral("configuration_evidence_sources_type_invalid"));
    }
    for (const QString& key : {QStringLiteral("lastSeenUtc"), QStringLiteral("lastSeen")}) {
        if (descriptor.contains(key) && !validTimestamp(descriptor.value(key))) {
            errors.append(QStringLiteral("%1_invalid").arg(key));
        }
    }

    const QVariant configurationValue = descriptor.value(QStringLiteral("configuration"));
    if (descriptor.contains(QStringLiteral("configuration"))
        && configurationValue.type() != QVariant::Map) {
        errors.append(QStringLiteral("configuration_type_invalid"));
    }
    const QVariantMap configuration = configurationValue.toMap();
    const QVariant metadataValue = descriptor.value(QStringLiteral("metadata"));
    if (descriptor.contains(QStringLiteral("metadata"))
        && metadataValue.type() != QVariant::Map) {
        errors.append(QStringLiteral("metadata_type_invalid"));
    }
    const QVariant baselineValue = descriptor.value(QStringLiteral("configurationBaseline"));
    if (descriptor.contains(QStringLiteral("configurationBaseline"))
        && baselineValue.type() != QVariant::Map) {
        errors.append(QStringLiteral("baseline_configuration_type_invalid"));
    }
    const QVariantMap baselineConfiguration = baselineValue.toMap();
    if (descriptor.contains(QStringLiteral("configurationFingerprint"))
        && descriptor.value(QStringLiteral("configurationFingerprint")).type() != QVariant::String) {
        errors.append(QStringLiteral("configuration_fingerprint_type_invalid"));
    }
    const QString suppliedFingerprint = descriptor.value(
        QStringLiteral("configurationFingerprint")).toString().trimmed().toLower();
    const QString computedFingerprint = VehicleCapability::fingerprintForConfiguration(configuration);
    if (!suppliedFingerprint.isEmpty()) {
        if (!validFingerprint(suppliedFingerprint)) {
            errors.append(QStringLiteral("configuration_fingerprint_invalid"));
        } else if (!computedFingerprint.isEmpty() && suppliedFingerprint != computedFingerprint) {
            errors.append(QStringLiteral("configuration_fingerprint_mismatch"));
        }
    }

    if (descriptor.contains(QStringLiteral("baselineConfigurationFingerprint"))
        && descriptor.value(QStringLiteral("baselineConfigurationFingerprint")).type() != QVariant::String) {
        errors.append(QStringLiteral("baseline_fingerprint_type_invalid"));
    }
    if (descriptor.contains(QStringLiteral("baselineSource"))
        && descriptor.value(QStringLiteral("baselineSource")).type() != QVariant::String) {
        errors.append(QStringLiteral("baseline_source_type_invalid"));
    }
    const QString baseline = descriptor.value(
        QStringLiteral("baselineConfigurationFingerprint")).toString().trimmed().toLower();
    const QString baselineSource = descriptor.value(QStringLiteral("baselineSource")).toString().trimmed();
    if (!baseline.isEmpty()) {
        if (!validFingerprint(baseline)) {
            errors.append(QStringLiteral("baseline_fingerprint_invalid"));
        }
        if (baselineSource != QStringLiteral("trusted_manifest")
            && baselineSource != QStringLiteral("local_approval")) {
            errors.append(QStringLiteral("baseline_source_untrusted"));
        }
        if (!baselineConfiguration.isEmpty()
            && VehicleCapability::fingerprintForConfiguration(baselineConfiguration) != baseline) {
            errors.append(QStringLiteral("baseline_fingerprint_mismatch"));
        }
    } else if (!baselineSource.isEmpty() && baselineConfiguration.isEmpty()) {
        errors.append(QStringLiteral("baseline_evidence_missing"));
    } else if (!baselineConfiguration.isEmpty()) {
        warnings.append(QStringLiteral("baseline_fingerprint_missing"));
    }
    if (suppliedFingerprint.isEmpty() && configuration.isEmpty()) {
        warnings.append(QStringLiteral("configuration_evidence_missing"));
    } else if (baseline.isEmpty()) {
        warnings.append(QStringLiteral("configuration_baseline_missing"));
    }

    const QString observed = suppliedFingerprint.isEmpty() ? computedFingerprint : suppliedFingerprint;
    QVariantMap result;
    result.insert(QStringLiteral("valid"), errors.isEmpty());
    result.insert(QStringLiteral("systemId"), systemId);
    result.insert(QStringLiteral("configurationState"), derivedConfigurationState(observed, baseline));
    result.insert(QStringLiteral("errors"), errors);
    result.insert(QStringLiteral("warnings"), warnings);
    return result;
}

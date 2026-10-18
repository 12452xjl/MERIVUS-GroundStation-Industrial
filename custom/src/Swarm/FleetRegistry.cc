#include "FleetRegistry.h"
#include "FleetCapabilityEvidence.h"
#include "FleetConfigurationDiff.h"
#include "FleetNativeBatteryEvidence.h"
#include "FleetNativeConfigurationObservation.h"
#include "Fact.h"
#include "FirmwarePlugin.h"
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
#include "APM/ArduCopterFirmwarePlugin.h"
#endif
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <algorithm>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPointer>
#include <QSaveFile>
#include <QSet>
#include <QThread>
#include <QUuid>
#include <QVariantHash>
#include <QtMath>

#include <limits>
#include <initializer_list>

namespace {

constexpr qint64 kMaxCapabilityPassportBytes = 32LL * 1024LL * 1024LL;
// A small clock skew is common between a vehicle and the ground station. A
// timestamp further in the future must not keep a vehicle online forever.
constexpr qint64 kFutureLastSeenToleranceMs = 1000;
constexpr qint64 kFutureApprovalToleranceMs = 5LL * 60LL * 1000LL;
constexpr int kMaxCapabilityEvidenceCount = 128;
constexpr int kMaxCapabilityEvidenceLength = 128;
constexpr int kMaxDescriptorDepth = 8;
constexpr int kMaxDescriptorNodes = 2048;
constexpr qint64 kMaxDescriptorScalarBytes = 128LL * 1024LL;
constexpr qint64 kNativeBatteryMaximumAgeMs = 3000;

QString configurationMotorLayoutParameter(int firmwareType, int vehicleType)
{
    // Exact dependencies of the current built-in Vehicle::motorCount and
    // ArduCopterFirmwarePlugin::multiRotorXConfig recipes. Neither name has
    // the r. prefix, so ParameterManager does not remap it by firmware version.
    if (vehicleType == MAV_TYPE_SUBMARINE) return QStringLiteral("FRAME_CONFIG");
    if (firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        switch (vehicleType) {
        case MAV_TYPE_HELICOPTER: case MAV_TYPE_TRICOPTER: case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_HEXAROTOR: case MAV_TYPE_OCTOROTOR:
            return QStringLiteral("FRAME");
        default: break;
        }
    }
    return QStringLiteral("");
}

bool isSha256Fingerprint(const QString& value)
{
    const QString normalized = value.trimmed();
    if (normalized.size() != 64) {
        return false;
    }
    for (const QChar character : normalized) {
        const bool decimal = character >= QLatin1Char('0') && character <= QLatin1Char('9');
        const bool lowerHex = character >= QLatin1Char('a') && character <= QLatin1Char('f');
        const bool upperHex = character >= QLatin1Char('A') && character <= QLatin1Char('F');
        if (!decimal && !lowerHex && !upperHex) {
            return false;
        }
    }
    return true;
}

bool containsOnlyJsonKeys(const QJsonObject& object, const QStringList& allowed)
{
    for (const QString& key : object.keys()) {
        if (!allowed.contains(key)) return false;
    }
    return true;
}

bool strictNumericVariant(const QVariant& value)
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

bool strictBooleanVariant(const QVariant& value)
{
    return value.isValid() && !value.isNull() && value.type() == QVariant::Bool;
}

QString ownedPerformanceText(const QString& value)
{
    return value.isNull() ? QString() : QString(value.constData(), value.size());
}

// Only fixed, already validated context/approval/receipt DTOs reach this copy.
// Rebuild every container AND QString buffer: a caller may have retained a
// mutable nested reference or QString::data() before entering a native fence.
// This is not an arbitrary metadata/configuration parser or JSON round trip.
QVariant ownedPerformanceValue(const QVariant& value)
{
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
            result.insert(ownedPerformanceText(it.key()), ownedPerformanceValue(it.value()));
        return result;
    }
    if (value.type() == QVariant::List) {
        QVariantList result;
        const QVariantList source = value.toList();
        result.reserve(source.size());
        for (const QVariant& entry : source) result.append(ownedPerformanceValue(entry));
        return result;
    }
    if (value.type() == QVariant::StringList) {
        QStringList result;
        for (const QString& entry : value.toStringList()) result.append(ownedPerformanceText(entry));
        return result;
    }
    if (value.type() == QVariant::String) return ownedPerformanceText(value.toString());
    return value;
}

bool canonicalPerformanceProposal(const QVariantMap& source, QVariantMap* result)
{
    if (!result) return false;
    result->clear();
    if (source.size() != 2 || !source.contains(QStringLiteral("maxSpeedMps"))
        || !source.contains(QStringLiteral("enduranceSeconds"))) return false;
    double speed = -1.0, endurance = -1.0;
    for (const QString& key : {QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds")}) {
        const QVariant raw = source.value(key);
        if (raw.isNull() || !strictNumericVariant(raw)) return false;
        bool ok = false;
        const double number = raw.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qAbs(number) > 9007199254740991.0
            || (number < 0.0 && number != -1.0)
            || (key == QStringLiteral("enduranceSeconds") && number == 0.0)) return false;
        if (key == QStringLiteral("maxSpeedMps")) speed = number;
        else endurance = number;
    }
    if (speed < 0.0 && endurance < 0.0) return false;
    *result = {{QStringLiteral("maxSpeedMps"), speed}, {QStringLiteral("enduranceSeconds"), endurance}};
    return true;
}

QVariantMap ownedPerformanceContext(const QVariantMap& context)
{
    QVariantMap result = ownedPerformanceValue(context).toMap();
    // The fixed validated value contract permits an integral JSON Double ID;
    // original native contexts and both newly produced contexts keep Int.
    result.insert(QStringLiteral("vehicleId"), context.value(QStringLiteral("vehicleId")).toInt());
    return result;
}

bool strictInteger(const QVariant& value, int* result)
{
    if (!value.isValid() || value.isNull() || !strictNumericVariant(value)) {
        return false;
    }

    // QML and JSON may represent an integer as a double. Accept that form
    // only when it is finite and has no fractional component; never let
    // QVariant::toInt() silently truncate 1.9 into vehicle ID 1.
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number < static_cast<double>(std::numeric_limits<int>::min())
        || number > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (result) {
        *result = static_cast<int>(number);
    }
    return true;
}

bool boundedDescriptorValue(const QVariant& value,
                            int depth,
                            int* nodeCount,
                            qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxDescriptorDepth) return false;
    if (++(*nodeCount) > kMaxDescriptorNodes) return false;
    if (!value.isValid() || value.isNull()) return true;

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > kMaxCapabilityEvidenceLength) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxDescriptorScalarBytes
                || !boundedDescriptorValue(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > kMaxCapabilityEvidenceLength) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxDescriptorScalarBytes
                || !boundedDescriptorValue(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!boundedDescriptorValue(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            if (++(*nodeCount) > kMaxDescriptorNodes) return false;
            *scalarBytes += item.toUtf8().size();
            if (*scalarBytes > kMaxDescriptorScalarBytes) return false;
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
        // Custom QObject/user types are not a bounded descriptor contract and
        // must not be retained in the registry's metadata map.
        return false;
    }
    return *scalarBytes <= kMaxDescriptorScalarBytes;
}

bool boundedCapabilityEvidenceList(const QVariant& value)
{
    QStringList values;
    if (value.type() == QVariant::String) {
        if (value.toString().toUtf8().size() > kMaxDescriptorScalarBytes) return false;
        values = value.toString().split(QChar(','), Qt::SkipEmptyParts);
    } else if (value.type() == QVariant::StringList) {
        values = value.toStringList();
    } else if (value.type() == QVariant::List) {
        if (value.toList().size() > kMaxCapabilityEvidenceCount) return false;
        for (const QVariant& entry : value.toList()) {
            if (entry.type() != QVariant::String) return false;
            values.append(entry.toString());
        }
    } else {
        return false;
    }
    if (values.size() > kMaxCapabilityEvidenceCount) return false;
    for (const QString& valueItem : values) {
        if (valueItem.size() > kMaxCapabilityEvidenceLength) return false;
    }
    return true;
}

bool validateSystemIdAliases(const QVariantMap& descriptor, int expectedSystemId)
{
    static const QStringList aliases = {
        QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id")
    };

    bool found = false;
    int resolved = 0;
    for (const QString& alias : aliases) {
        if (!descriptor.contains(alias)) {
            continue;
        }
        int candidate = 0;
        if (!strictInteger(descriptor.value(alias), &candidate)
            || candidate <= 0 || candidate > 255) {
            return false;
        }
        if (found && candidate != resolved) {
            // Multiple identity aliases must not describe different
            // vehicles. Ambiguous identity is unsafe for partial updates.
            return false;
        }
        found = true;
        resolved = candidate;
    }

    if (!found) {
        return expectedSystemId > 0 && expectedSystemId <= 255;
    }
    return expectedSystemId <= 0 || resolved == expectedSystemId;
}

bool validateStaticTextFields(const QVariantMap& descriptor)
{
    // Validate before VehicleCapability normalizes its QString members. A
    // numeric/bool/version value must not become valid evidence by toString().
    // Empty QString remains the native unknown representation.
    const QVariantMap metadata = descriptor.value(QStringLiteral("metadata")).toMap();
    for (const QVariantMap& source : {descriptor, metadata}) {
        for (const QString& key : {QStringLiteral("vehicleType"), QStringLiteral("type"),
                QStringLiteral("autopilotType"), QStringLiteral("autopilot"),
                QStringLiteral("firmwareVersion"), QStringLiteral("firmware")}) {
            if (!source.contains(key)) continue;
            const QVariant value = source.value(key);
            if (value.type() != QVariant::String || value.toString().size() > 512
                || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        }
    }
    return true;
}

bool validateBooleanFields(const QVariantMap& descriptor)
{
    static const QStringList keys = {
        QStringLiteral("online"), QStringLiteral("estimatorHealthy"),
        QStringLiteral("estimatorHealthKnown"), QStringLiteral("positionHealthy"),
        QStringLiteral("positionHealthKnown"), QStringLiteral("hasFault"),
        QStringLiteral("faultHealthKnown"),
        QStringLiteral("canUseGnss"), QStringLiteral("hasGnss"),
        QStringLiteral("canUseRtk"), QStringLiteral("canUseVision"),
        QStringLiteral("hasCamera"), QStringLiteral("canUseRangeSensor"),
        QStringLiteral("hasRangeSensor"),
        QStringLiteral("canRelay"), QStringLiteral("configurationClear")
    };
    for (const QString& key : keys) {
        if (descriptor.contains(key)
            && descriptor.value(key).type() != QVariant::Bool) {
            return false;
        }
    }
    return true;
}

bool validateNumericFields(const QVariantMap& descriptor)
{
    struct Rule {
        const char* key;
        double minimum;
        double maximum;
    };
    static const Rule rules[] = {
        {"maxSpeedMps", -1.0, std::numeric_limits<double>::max()},
        {"enduranceSeconds", -1.0, std::numeric_limits<double>::max()},
        {"enduranceSec", -1.0, std::numeric_limits<double>::max()},
        {"batteryPercent", -1.0, 100.0},
        {"battery", -1.0, 100.0},
        {"linkQualityPercent", -1.0, 100.0},
        {"linkQuality", -1.0, 100.0},
        {"faultEvidenceAgeMs", -1.0, static_cast<double>(std::numeric_limits<int>::max())}
    };
    for (const Rule& rule : rules) {
        const QString key = QString::fromLatin1(rule.key);
        if (!descriptor.contains(key)) {
            continue;
        }
        const QVariant value = descriptor.value(key);
        // Null is the established unknown/clear representation for telemetry.
        if (!value.isValid() || value.isNull()) {
            continue;
        }
        if (!strictNumericVariant(value)) {
            return false;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || number < rule.minimum || number > rule.maximum) {
            return false;
        }
    }
    return true;
}

bool validTimestampValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return false;
    }
    QDateTime parsed;
    if (value.type() == QVariant::DateTime) {
        parsed = value.toDateTime();
    } else {
        if (value.type() != QVariant::String) {
            return false;
        }
        const QString text = value.toString().trimmed();
        if (text.isEmpty()) {
            return false;
        }
        parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
        if (!parsed.isValid()) parsed = QDateTime::fromString(text, Qt::ISODate);
    }
    if (!parsed.isValid()) return false;
    // A future last-seen value must not make an unobserved vehicle appear
    // online indefinitely. Allow a small clock-skew window for adapters.
    return parsed.toUTC().msecsTo(QDateTime::currentDateTimeUtc())
        >= -kFutureLastSeenToleranceMs;
}

bool validateTimestampFields(const QVariantMap& descriptor)
{
    for (const QString& key : {QStringLiteral("lastSeenUtc"), QStringLiteral("lastSeen")}) {
        if (descriptor.contains(key) && !validTimestampValue(descriptor.value(key))) {
            return false;
        }
    }
    return true;
}

bool validateConfigurationFields(const QVariantMap& descriptor, bool allowTrustedBaseline)
{
    const QVariant configurationValue = descriptor.value(QStringLiteral("configuration"));
    if (descriptor.contains(QStringLiteral("configuration"))
        && configurationValue.type() != QVariant::Map) {
        return false;
    }
    const QVariant metadataValue = descriptor.value(QStringLiteral("metadata"));
    if (descriptor.contains(QStringLiteral("metadata"))
        && metadataValue.type() != QVariant::Map) {
        return false;
    }
    const QVariantMap configuration = configurationValue.toMap();
    const QVariantMap metadata = metadataValue.toMap();
    if (metadata.contains(QStringLiteral("configuration"))
        && metadata.value(QStringLiteral("configuration")).type() != QVariant::Map) {
        return false;
    }
    const QVariantMap effectiveConfiguration = !configuration.isEmpty()
        ? configuration : metadata.value(QStringLiteral("configuration")).toMap();

    if (descriptor.contains(QStringLiteral("configurationFingerprint"))) {
        const QVariant value = descriptor.value(QStringLiteral("configurationFingerprint"));
        if (!value.isValid() || value.isNull() || value.type() != QVariant::String) {
            return false;
        }
        const QString fingerprint = value.toString().trimmed().toLower();
        // Empty is retained as the established "not available" value in
        // exported descriptors; a non-empty value must be a SHA-256 hex hash.
        if (!fingerprint.isEmpty() && !isSha256Fingerprint(fingerprint)) {
            return false;
        }
        if (!effectiveConfiguration.isEmpty() && !fingerprint.isEmpty()
            && VehicleCapability::fingerprintForConfiguration(effectiveConfiguration)
                != fingerprint) {
            return false;
        }
    }

    if (descriptor.contains(QStringLiteral("configurationBaseline"))) {
        const QVariant baselineValue = descriptor.value(QStringLiteral("configurationBaseline"));
        if (baselineValue.type() != QVariant::Map || !allowTrustedBaseline) {
            return false;
        }
    }

    for (const QString& key : {QStringLiteral("baselineSource"),
                               QStringLiteral("baselineConfigurationFingerprint")}) {
        if (descriptor.contains(key) && descriptor.value(key).type() != QVariant::String) {
            return false;
        }
    }

    if (!allowTrustedBaseline) {
        return true;
    }
    const QString source = descriptor.value(QStringLiteral("baselineSource"))
        .toString().trimmed();
    const QString baselineFingerprint = descriptor.value(
        QStringLiteral("baselineConfigurationFingerprint")).toString().trimmed().toLower();
    const QVariantMap baseline = descriptor.value(QStringLiteral("configurationBaseline")).toMap();
    if (!source.isEmpty() && source != QStringLiteral("trusted_manifest")
        && source != QStringLiteral("local_approval")) {
        return false;
    }
    if (!baselineFingerprint.isEmpty()
        && (!isSha256Fingerprint(baselineFingerprint) || source.isEmpty())) {
        return false;
    }
    if (!source.isEmpty() && baselineFingerprint.isEmpty() && baseline.isEmpty()) {
        return false;
    }
    if (!baseline.isEmpty() && !baselineFingerprint.isEmpty()
        && VehicleCapability::fingerprintForConfiguration(baseline) != baselineFingerprint) {
        return false;
    }
    return true;
}

bool validStringListEvidenceValue(const QVariant& value)
{
    return boundedCapabilityEvidenceList(value);
}

bool validateDiscoveryEvidenceFields(const QVariantMap& descriptor)
{
    const auto validateMap = [](const QVariantMap& evidence) {
        for (const QString& key : {QStringLiteral("capabilities"),
                                   QStringLiteral("verifiedCapabilities"),
                                   QStringLiteral("configurationEvidenceSources"),
                                   QStringLiteral("discoveryErrors")}) {
            if (evidence.contains(key)
                && !validStringListEvidenceValue(evidence.value(key))) {
                return false;
            }
        }

        for (const QString& key : {QStringLiteral("capabilityEvidenceState"),
                                   QStringLiteral("discoveryState"),
                                   QStringLiteral("positionEvidenceSource")}) {
            if (evidence.contains(key)
                && evidence.value(key).type() != QVariant::String) {
                return false;
            }
        }

        return !evidence.contains(QStringLiteral("discoveredAtUtc"))
            || validTimestampValue(evidence.value(QStringLiteral("discoveredAtUtc")));
    };

    return validateMap(descriptor)
        && validateMap(descriptor.value(QStringLiteral("metadata")).toMap());
}

bool validateIngressDescriptor(const QVariantMap& descriptor, int expectedSystemId,
                               bool allowTrustedBaseline)
{
    int descriptorNodeCount = 0;
    qint64 descriptorScalarBytes = 0;
    if (!boundedDescriptorValue(descriptor, 0, &descriptorNodeCount, &descriptorScalarBytes)) {
        return false;
    }
    const QVariant faultKnownValue = descriptor.value(QStringLiteral("faultHealthKnown"));
    if (descriptor.contains(QStringLiteral("faultHealthKnown"))
        && !strictBooleanVariant(faultKnownValue)) {
        return false;
    }
    const bool faultKnown = descriptor.contains(QStringLiteral("faultHealthKnown"))
        && faultKnownValue.toBool();
    if (descriptor.contains(QStringLiteral("faultEvidenceAgeMs"))
        && !descriptor.contains(QStringLiteral("faultHealthKnown"))) {
        // Age is meaningful only as part of the paired fault assessment;
        // accepting an age-only update would leave known=true or its old
        // anchor attached to a newly contradictory snapshot.
        return false;
    }
    if (faultKnown) {
        const QVariant ageValue = descriptor.value(QStringLiteral("faultEvidenceAgeMs"));
        if (!ageValue.isValid() || ageValue.isNull() || !strictNumericVariant(ageValue)) {
            return false;
        }
        bool ageOk = false;
        const double age = ageValue.toDouble(&ageOk);
        if (!ageOk || !qIsFinite(age) || qFloor(age) != age || age < 0.0
            || age > static_cast<double>(std::numeric_limits<int>::max())) {
            return false;
        }
    }
    if (descriptor.contains(QStringLiteral("faultEvidenceAgeMs")) && !faultKnown) {
        const QVariant ageValue = descriptor.value(QStringLiteral("faultEvidenceAgeMs"));
        // Null is the explicit unknown representation accepted by the
        // numeric-field validator. Only a real, non-negative numeric age may
        // contradict faultHealthKnown=false; do not let QVariant conversion
        // turn null or text into an accidental zero.
        if (ageValue.isValid() && !ageValue.isNull() && strictNumericVariant(ageValue)) {
            bool ageOk = false;
            const double age = ageValue.toDouble(&ageOk);
            if (ageOk && qIsFinite(age) && age >= 0.0) {
                return false;
            }
        }
    }
    return validateSystemIdAliases(descriptor, expectedSystemId)
        && validateStaticTextFields(descriptor)
        && validateBooleanFields(descriptor)
        && validateNumericFields(descriptor)
        && validateTimestampFields(descriptor)
        && validateDiscoveryEvidenceFields(descriptor)
        && validateConfigurationFields(descriptor, allowTrustedBaseline);
}

QVariantMap withoutNativeBatteryProjection(const QVariantMap& supplied)
{
    QVariantMap result = supplied;
    const auto strip = [](QVariantMap* map) {
        for (const QString& key : {QStringLiteral("nativeBatteryEvidenceRequired"),
                QStringLiteral("nativeBatteryEvidence"), QStringLiteral("nativeBatteryDrainEvidence"), QStringLiteral("remainingFlightSeconds"),
                QStringLiteral("availableFlightSeconds")}) map->remove(key);
    };
    strip(&result);
    // Only consumed root/metadata projections are reserved; do not convert
    // malformed metadata into an apparently valid empty object.
    if (result.value(QStringLiteral("metadata")).type() == QVariant::Map) {
        QVariantMap metadata = result.value(QStringLiteral("metadata")).toMap();
        strip(&metadata);
        result.insert(QStringLiteral("metadata"), metadata);
    }
    return result;
}

QVariantMap unknownNativeBatteryEvidence(int systemId, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), systemId},
        {QStringLiteral("componentId"), 0}, {QStringLiteral("sourceEpoch"), QStringLiteral("")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("rows"), QVariantList()},
        {QStringLiteral("aggregationState"), reason}, {QStringLiteral("batteryPercentKnown"), false},
        {QStringLiteral("batteryPercent"), -1.0}, {QStringLiteral("remainingSecondsKnown"), false},
        {QStringLiteral("remainingFlightSeconds"), -1.0}, {QStringLiteral("reportedRemainingSeconds"), -1.0},
        {QStringLiteral("ageMs"), static_cast<qint64>(-1)}, {QStringLiteral("batteryId"), -1},
        {QStringLiteral("receiptSequence"), QStringLiteral("0")}, {QStringLiteral("contentRevision"), QStringLiteral("0")},
        {QStringLiteral("readonly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

FleetNativeBatteryEvidence::Read nativeBatteryEnvelope(const QVariantMap& evidence, int systemId)
{
    // The common reader checks the complete native DTO and its public scalar
    // aliases. Do not use its eligibility result to discard a valid fault.
    QVariantMap projected{{QStringLiteral("systemId"), systemId},
        {QStringLiteral("nativeBatteryEvidenceRequired"), true},
        {QStringLiteral("nativeBatteryEvidence"), evidence},
        {QStringLiteral("batteryPercent"), evidence.value(QStringLiteral("batteryPercent"))}};
    if (evidence.value(QStringLiteral("remainingSecondsKnown")).type() == QVariant::Bool
        && evidence.value(QStringLiteral("remainingSecondsKnown")).toBool()) {
        projected.insert(QStringLiteral("remainingFlightSeconds"),
                         evidence.value(QStringLiteral("remainingFlightSeconds")));
    }
    return FleetNativeBatteryEvidence::inspect(projected, systemId);
}

bool optionalTelemetryInteger(const QVariant& value, double minimum, double maximum)
{
    if (!value.isValid() || value.isNull() || !strictNumericVariant(value)) return false;
    const double number = value.toDouble();
    return qIsFinite(number) && qFloor(number) == number && number >= minimum && number <= maximum;
}

bool optionalTelemetryText(const QVariant& value, bool allowEmpty = true)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > 64
        || (!allowEmpty && value.toString().isEmpty())) return false;
    for (const QChar character : value.toString())
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool optionalTelemetryExact(const QVariantMap& map, std::initializer_list<const char*> names)
{
    if (map.size() != int(names.size())) return false;
    for (const char* name : names) if (!map.contains(QString::fromLatin1(name))) return false;
    return true;
}

bool optionalTelemetryFlag(const QVariantMap& map, const char* name, bool expected)
{
    const QVariant value = map.value(QString::fromLatin1(name));
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool optionalTelemetryCounter(const QVariant& value)
{
    if (!optionalTelemetryText(value, false)) return false;
    const QString counter = value.toString();
    if (counter.size() > 20 || (counter.size() > 1 && counter.startsWith(QLatin1Char('0')))) return false;
    for (const QChar character : counter)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    return counter.size() < 20 || counter <= QStringLiteral("18446744073709551615");
}

bool optionalTelemetryObservationValid(const QVariantMap& observation, int systemId)
{
    if (!optionalTelemetryExact(observation, {"schemaVersion", "reportType", "vehicleId", "componentId", "ledgerEpoch",
            "versionEpoch", "sourceCurrent", "maximumAgeMs", "rows", "readonly", "authenticated",
            "actualTransmitRateVerified", "availableBandwidthVerified", "executionPermit", "flightCommandReleased"})
        || !optionalTelemetryInteger(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || observation.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-optional-telemetry-observations"))
        || !optionalTelemetryInteger(observation.value(QStringLiteral("vehicleId")), systemId, systemId)
        || !optionalTelemetryInteger(observation.value(QStringLiteral("componentId")), 0, 255)
        || !optionalTelemetryText(observation.value(QStringLiteral("ledgerEpoch")))
        || !optionalTelemetryText(observation.value(QStringLiteral("versionEpoch")))
        || !optionalTelemetryInteger(observation.value(QStringLiteral("maximumAgeMs")), 30000, 30000)
        || !optionalTelemetryFlag(observation, "readonly", true)
        || observation.value(QStringLiteral("sourceCurrent")).type() != QVariant::Bool
        || observation.value(QStringLiteral("sourceCurrent")).isNull()
        || observation.value(QStringLiteral("rows")).type() != QVariant::List) return false;
    for (const char* name : {"authenticated", "actualTransmitRateVerified", "availableBandwidthVerified",
            "executionPermit", "flightCommandReleased"}) if (!optionalTelemetryFlag(observation, name, false)) return false;
    const bool sourceCurrent = observation.value(QStringLiteral("sourceCurrent")).toBool();
    if (sourceCurrent && (observation.value(QStringLiteral("ledgerEpoch")).toString().isEmpty()
        || observation.value(QStringLiteral("versionEpoch")).toString().isEmpty())) return false;
    const QVariantList rows = observation.value(QStringLiteral("rows")).toList();
    const int messageIds[] = {62, 74, 83};
    if (rows.size() != 3) return false;
    for (int index = 0; index < 3; ++index) {
        if (rows.at(index).type() != QVariant::Map) return false;
        const QVariantMap row = rows.at(index).toMap();
        if (!optionalTelemetryExact(row, {"messageId", "reported", "receivedWindow"})
            || !optionalTelemetryInteger(row.value(QStringLiteral("messageId")), messageIds[index], messageIds[index])
            || row.value(QStringLiteral("reported")).type() != QVariant::Map
            || row.value(QStringLiteral("receivedWindow")).type() != QVariant::Map) return false;
        const QVariantMap reported = row.value(QStringLiteral("reported")).toMap();
        const QVariantMap window = row.value(QStringLiteral("receivedWindow")).toMap();
        if (!optionalTelemetryExact(reported, {"observed", "available", "fresh", "ageMs", "receiptSequence",
                "contentRevision", "intervalUs", "state"})
            || !optionalTelemetryInteger(reported.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
            || !optionalTelemetryCounter(reported.value(QStringLiteral("receiptSequence")))
            || !optionalTelemetryCounter(reported.value(QStringLiteral("contentRevision")))
            || !optionalTelemetryText(reported.value(QStringLiteral("state")), false)) return false;
        for (const char* name : {"observed", "available", "fresh"}) {
            const QVariant value = reported.value(QString::fromLatin1(name));
            if (value.type() != QVariant::Bool || value.isNull()) return false;
        }
        const bool observed = reported.value(QStringLiteral("observed")).toBool();
        const QVariant interval = reported.value(QStringLiteral("intervalUs"));
        if (observed ? (interval.type() != QVariant::Int || interval.isNull()) : !interval.isNull()) return false;
        const int rawInterval = interval.toInt();
        const qint64 ageMs = reported.value(QStringLiteral("ageMs")).toLongLong();
        const QString expectedState = !observed ? QStringLiteral("unobserved")
            : rawInterval == -1 ? QStringLiteral("disabled") : rawInterval == 0 ? QStringLiteral("not-available")
            : rawInterval > 0 ? QStringLiteral("positive") : QStringLiteral("invalid");
        const bool fresh = sourceCurrent && observed && rawInterval >= -1 && ageMs >= 0 && ageMs <= 30000;
        if (reported.value(QStringLiteral("state")).toString() != expectedState
            || !optionalTelemetryFlag(reported, "fresh", fresh)
            || !optionalTelemetryFlag(reported, "available", fresh && rawInterval > 0)
            || (!observed && (ageMs != -1 || reported.value(QStringLiteral("receiptSequence")).toString() != QStringLiteral("0")))
            || (observed && (reported.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0")
                || reported.value(QStringLiteral("contentRevision")).toString() == QStringLiteral("0")))) return false;
        if (!optionalTelemetryExact(window, {"durationMs", "retainedFrameCount", "complete", "saturated", "receivedFrameCount",
                "receivedMeanHz", "maximumInternalGapMs", "newestAgeMs", "receiptSequence"})
            || !optionalTelemetryInteger(window.value(QStringLiteral("durationMs")), 5000, 5000)
            || !optionalTelemetryInteger(window.value(QStringLiteral("retainedFrameCount")), 0, 512)
            || !optionalTelemetryInteger(window.value(QStringLiteral("receivedFrameCount")), -1, 512)
            || !optionalTelemetryInteger(window.value(QStringLiteral("maximumInternalGapMs")), -1, 5000)
            || !optionalTelemetryInteger(window.value(QStringLiteral("newestAgeMs")), -1, 9007199254740991.0)
            || !optionalTelemetryCounter(window.value(QStringLiteral("receiptSequence")))
            || window.value(QStringLiteral("complete")).type() != QVariant::Bool
            || window.value(QStringLiteral("complete")).isNull()
            || window.value(QStringLiteral("saturated")).type() != QVariant::Bool
            || window.value(QStringLiteral("saturated")).isNull()
            || !strictNumericVariant(window.value(QStringLiteral("receivedMeanHz")))
            || window.value(QStringLiteral("receivedMeanHz")).isNull()) return false;
        const double receivedHz = window.value(QStringLiteral("receivedMeanHz")).toDouble();
        if (!qIsFinite(receivedHz) || receivedHz < -1.0 || receivedHz > 102.4) return false;
        const bool complete = window.value(QStringLiteral("complete")).toBool();
        const int count = window.value(QStringLiteral("receivedFrameCount")).toInt();
        const qint64 maximumGap = window.value(QStringLiteral("maximumInternalGapMs")).toLongLong();
        const qint64 newestAge = window.value(QStringLiteral("newestAgeMs")).toLongLong();
        if (complete ? (!sourceCurrent || window.value(QStringLiteral("saturated")).toBool()
                || window.value(QStringLiteral("receivedFrameCount")) != window.value(QStringLiteral("retainedFrameCount"))
                || newestAge < 0 || newestAge > 30000
                || window.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0")
                || (count < 2 ? maximumGap != -1 : maximumGap < 0)
                || (count == 0 ? newestAge < 5000 : newestAge >= 5000)
                || receivedHz != count * 1000.0 / 5000.0)
            : (count != -1 || receivedHz != -1.0 || maximumGap != -1)) return false;
    }
    return true;
}

bool localReceptionInt(const QVariant& value, int minimum, int maximum)
{
    return value.type() == QVariant::Int && !value.isNull()
        && value.toInt() >= minimum && value.toInt() <= maximum;
}

bool localReceptionLong(const QVariant& value, qint64 minimum, qint64 maximum)
{
    return value.type() == QVariant::LongLong && !value.isNull()
        && value.toLongLong() >= minimum && value.toLongLong() <= maximum;
}

bool localReceptionText(const QVariant& value, int maximum, bool empty = false)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > maximum
        || (!empty && value.toString().isEmpty()) || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
    for (QChar character : value.toString())
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format) return false;
    return true;
}

bool localReceptionObservationValid(const QVariantMap& observation, int systemId)
{
    if (!optionalTelemetryExact(observation, {"schemaVersion", "reportType", "vehicleId", "componentId", "sourceEpoch",
            "receiptSequence", "sourceCurrent", "observed", "newestAgeMs", "window", "boundAgeMs", "maximumAgeMs",
            "snapshotOnly", "authenticated", "availableBandwidthVerified", "latencyVerified", "packetLossVerified",
            "remoteTransmitRateVerified", "configurationPermitted", "executionPermit", "flightCommandReleased", "reason"})
        || !localReceptionInt(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || observation.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-local-reception"))
        || !localReceptionInt(observation.value(QStringLiteral("vehicleId")), systemId, systemId)
        || !localReceptionInt(observation.value(QStringLiteral("componentId")), 0, 255)
        || !localReceptionText(observation.value(QStringLiteral("sourceEpoch")), 64, true)
        || !optionalTelemetryCounter(observation.value(QStringLiteral("receiptSequence")))
        || !localReceptionLong(observation.value(QStringLiteral("newestAgeMs")), -1, std::numeric_limits<qint64>::max())
        || !localReceptionLong(observation.value(QStringLiteral("boundAgeMs")), -1, std::numeric_limits<qint64>::max())
        || !localReceptionInt(observation.value(QStringLiteral("maximumAgeMs")), 30000, 30000)
        || !localReceptionText(observation.value(QStringLiteral("reason")), 256, true)
        || observation.value(QStringLiteral("window")).type() != QVariant::Map
        || observation.value(QStringLiteral("window")).isNull()
        || !optionalTelemetryFlag(observation, "snapshotOnly", true)) return false;
    for (const char* key : {"sourceCurrent", "observed"}) {
        const QVariant value = observation.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    for (const char* key : {"authenticated", "availableBandwidthVerified", "latencyVerified", "packetLossVerified",
            "remoteTransmitRateVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!optionalTelemetryFlag(observation, key, false)) return false;
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const bool observed = observation.value(QStringLiteral("observed")).toBool();
    const qint64 age = observation.value(QStringLiteral("newestAgeMs")).toLongLong();
    const qint64 boundAge = observation.value(QStringLiteral("boundAgeMs")).toLongLong();
    const QString receipt = observation.value(QStringLiteral("receiptSequence")).toString();
    // The receipt counter is Vehicle-global and never resets with a source.
    // A cleared/unobserved ledger can legitimately retain a nonzero counter;
    // a clock anomaly may retain observed history with an unknown age.
    if ((!observed && age != -1) || (observed && receipt == QStringLiteral("0"))
        || (current && (!observed || age < 0 || age > 30000 || boundAge < 0
            || observation.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
            || observation.value(QStringLiteral("componentId")).toInt() == 0))) return false;
    const QVariantMap window = observation.value(QStringLiteral("window")).toMap();
    if (!optionalTelemetryExact(window, {"durationMs", "maximumRetainedFrames", "retainedFrameCount", "complete", "saturated",
            "receivedFrameCount", "receivedFrameBytes", "observedReceiveKbps", "maximumInternalGapMs"})
        || !localReceptionInt(window.value(QStringLiteral("durationMs")), 5000, 5000)
        || !localReceptionInt(window.value(QStringLiteral("maximumRetainedFrames")), 4096, 4096)
        || !localReceptionInt(window.value(QStringLiteral("retainedFrameCount")), 0, 4096)
        || !localReceptionInt(window.value(QStringLiteral("receivedFrameCount")), -1, 4096)
        || !localReceptionLong(window.value(QStringLiteral("receivedFrameBytes")), -1, qint64(4096) * MAVLINK_MAX_PACKET_LEN)
        || !localReceptionLong(window.value(QStringLiteral("maximumInternalGapMs")), -1, 4999)
        || window.value(QStringLiteral("observedReceiveKbps")).type() != QVariant::Double
        || window.value(QStringLiteral("observedReceiveKbps")).isNull()
        || !qIsFinite(window.value(QStringLiteral("observedReceiveKbps")).toDouble())) return false;
    for (const char* key : {"complete", "saturated"}) {
        const QVariant value = window.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    const int retained = window.value(QStringLiteral("retainedFrameCount")).toInt();
    const int count = window.value(QStringLiteral("receivedFrameCount")).toInt();
    const qint64 bytes = window.value(QStringLiteral("receivedFrameBytes")).toLongLong();
    const qint64 gap = window.value(QStringLiteral("maximumInternalGapMs")).toLongLong();
    const double kbps = window.value(QStringLiteral("observedReceiveKbps")).toDouble();
    if (!observed && retained != 0) return false;
    if (window.value(QStringLiteral("complete")).toBool()) {
        if (!current || !observed || boundAge < 5000 || age < 0 || age > 30000
            || window.value(QStringLiteral("saturated")).toBool() || count != retained || bytes < 0
            || bytes < qint64(count) * 8 || bytes > qint64(count) * MAVLINK_MAX_PACKET_LEN
            || (count == 0 ? (bytes != 0 || age < 5000) : (bytes <= 0 || age >= 5000))
            || (count < 2 ? gap != -1 : gap < 0) || kbps != double(bytes) * 8.0 / 5000.0) return false;
    } else if (count != -1 || bytes != -1 || kbps != -1.0 || gap != -1) return false;
    return true;
}

bool cameraStreamFlagsValid(const QVariantMap& map, std::initializer_list<const char*> names)
{
    for (const char* name : names) {
        const QVariant value = map.value(QString::fromLatin1(name));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    return true;
}

bool cameraStreamCounterAtMost(const QVariant& value, const QVariant& ceiling)
{
    if (!optionalTelemetryCounter(value) || !optionalTelemetryCounter(ceiling)) return false;
    const QString number = value.toString(), maximum = ceiling.toString();
    return number.size() < maximum.size() || (number.size() == maximum.size() && number <= maximum);
}

bool cameraStreamValueValid(const QVariantMap& value, bool information, int streamId,
                           bool sourceCurrent, bool saturated, qint64 maximumAgeMs)
{
    const bool exact = information
        ? optionalTelemetryExact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision",
            "declaredCount", "type", "flags", "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"})
        : optionalTelemetryExact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision",
            "informationRevision", "matchesCurrentInformation", "flags", "framerate", "resolutionH", "resolutionV",
            "bitrate", "rotation", "hfov", "valid"});
    if (!exact || !cameraStreamFlagsValid(value, {"observed", "fresh", "valid"})
        || !localReceptionLong(value.value(QStringLiteral("ageMs")), -1, std::numeric_limits<qint64>::max())
        || !optionalTelemetryCounter(value.value(QStringLiteral("receiptSequence")))
        || !optionalTelemetryCounter(value.value(QStringLiteral("contentRevision")))
        || !localReceptionInt(value.value(QStringLiteral("flags")), -1, 65535)
        || !localReceptionInt(value.value(QStringLiteral("resolutionH")), -1, 65535)
        || !localReceptionInt(value.value(QStringLiteral("resolutionV")), -1, 65535)
        || !localReceptionLong(value.value(QStringLiteral("bitrate")), -1, 4294967295LL)
        || !localReceptionInt(value.value(QStringLiteral("rotation")), -1, 65535)
        || !localReceptionInt(value.value(QStringLiteral("hfov")), -1, 65535)
        || value.value(QStringLiteral("framerate")).type() != QVariant::Double
        || value.value(QStringLiteral("framerate")).isNull()
        || !qIsFinite(value.value(QStringLiteral("framerate")).toDouble())) return false;
    if (information) {
        if (!localReceptionInt(value.value(QStringLiteral("declaredCount")), -1, 255)
            || !localReceptionInt(value.value(QStringLiteral("type")), -1, 255)) return false;
    } else if (!optionalTelemetryCounter(value.value(QStringLiteral("informationRevision")))
               || !cameraStreamFlagsValid(value, {"matchesCurrentInformation"})) return false;
    const bool observed = value.value(QStringLiteral("observed")).toBool();
    const bool valid = value.value(QStringLiteral("valid")).toBool();
    const qint64 ageMs = value.value(QStringLiteral("ageMs")).toLongLong();
    const double framerate = value.value(QStringLiteral("framerate")).toDouble();
    const QString receipt = value.value(QStringLiteral("receiptSequence")).toString();
    const QString revision = value.value(QStringLiteral("contentRevision")).toString();
    const bool fresh = sourceCurrent && !saturated && valid && ageMs >= 0 && ageMs <= maximumAgeMs;
    if (!optionalTelemetryFlag(value, "fresh", fresh)) return false;
    if (!observed) {
        if (valid || ageMs != -1 || receipt != QStringLiteral("0") || revision != QStringLiteral("0")
            || framerate != -1.0) return false;
        for (const char* name : {"flags", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov"})
            if (value.value(QString::fromLatin1(name)).toLongLong() != -1) return false;
        return information
            ? value.value(QStringLiteral("declaredCount")).toInt() == -1 && value.value(QStringLiteral("type")).toInt() == -1
            : value.value(QStringLiteral("informationRevision")).toString() == QStringLiteral("0")
                && !value.value(QStringLiteral("matchesCurrentInformation")).toBool();
    }
    if (receipt == QStringLiteral("0") || revision == QStringLiteral("0")
        || framerate > double(std::numeric_limits<float>::max())
        || (valid ? framerate < 0.0 : framerate != -1.0)) return false;
    for (const char* name : {"flags", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov"})
        if (value.value(QString::fromLatin1(name)).toLongLong() < 0) return false;
    return !information || (value.value(QStringLiteral("declaredCount")).toInt() >= streamId
        && value.value(QStringLiteral("type")).toInt() >= 0);
}

bool cameraStreamObservationValid(const QVariantMap& observation, int systemId)
{
    if (!optionalTelemetryExact(observation, {"schemaVersion", "reportType", "vehicleId", "sourceEpoch", "transportEpoch",
            "versionEpoch", "receiptSequence", "sourceCurrent", "observed", "saturated", "maximumRows", "maximumAgeMs",
            "rows", "reason", "snapshotOnly", "authenticated", "catalogComplete", "remoteRunningVerified",
            "remoteStoppedVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        || !localReceptionInt(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || !localReceptionText(observation.value(QStringLiteral("reportType")), 64)
        || observation.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-camera-stream-observation"))
        || !localReceptionInt(observation.value(QStringLiteral("vehicleId")), systemId, systemId)
        || !localReceptionText(observation.value(QStringLiteral("sourceEpoch")), 128, true)
        || !localReceptionText(observation.value(QStringLiteral("transportEpoch")), 128, true)
        || !localReceptionText(observation.value(QStringLiteral("versionEpoch")), 128, true)
        || !optionalTelemetryCounter(observation.value(QStringLiteral("receiptSequence")))
        || !cameraStreamFlagsValid(observation, {"sourceCurrent", "observed", "saturated"})
        || !localReceptionInt(observation.value(QStringLiteral("maximumRows")), 32, 32)
        || !localReceptionLong(observation.value(QStringLiteral("maximumAgeMs")), 3000, 3000)
        || !localReceptionText(observation.value(QStringLiteral("reason")), 256, true)
        || observation.value(QStringLiteral("rows")).type() != QVariant::List
        || observation.value(QStringLiteral("rows")).isNull()
        || !optionalTelemetryFlag(observation, "snapshotOnly", true)) return false;
    for (const char* name : {"authenticated", "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified",
            "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!optionalTelemetryFlag(observation, name, false)) return false;
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const bool saturated = observation.value(QStringLiteral("saturated")).toBool();
    const QVariantList rows = observation.value(QStringLiteral("rows")).toList();
    const bool observed = observation.value(QStringLiteral("observed")).toBool();
    if (rows.size() > 32 || observed != !rows.isEmpty()
        || (observed && observation.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0"))
        || (current && (!observed || observation.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
        || observation.value(QStringLiteral("transportEpoch")).toString().isEmpty()
        || observation.value(QStringLiteral("versionEpoch")).toString().isEmpty()))) return false;
    int previousComponent = 0, previousStream = 0, knownCountComponent = 0, knownCount = 0;
    for (const QVariant& item : rows) {
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap row = item.toMap();
        if (!optionalTelemetryExact(row, {"componentId", "streamId", "information", "status"})
            || !localReceptionInt(row.value(QStringLiteral("componentId")), 100, 105)
            || !localReceptionInt(row.value(QStringLiteral("streamId")), 1, 255)
            || row.value(QStringLiteral("information")).type() != QVariant::Map || row.value(QStringLiteral("information")).isNull()
            || row.value(QStringLiteral("status")).type() != QVariant::Map || row.value(QStringLiteral("status")).isNull()) return false;
        const int component = row.value(QStringLiteral("componentId")).toInt(), stream = row.value(QStringLiteral("streamId")).toInt();
        if (component < previousComponent || (component == previousComponent && stream <= previousStream)) return false;
        previousComponent = component; previousStream = stream;
        const QVariantMap information = row.value(QStringLiteral("information")).toMap(), status = row.value(QStringLiteral("status")).toMap();
        if (!cameraStreamValueValid(information, true, stream, current, saturated, 3000)
            || !cameraStreamValueValid(status, false, stream, current, saturated, 3000)
            || !cameraStreamCounterAtMost(information.value(QStringLiteral("receiptSequence")), observation.value(QStringLiteral("receiptSequence")))
            || !cameraStreamCounterAtMost(status.value(QStringLiteral("receiptSequence")), observation.value(QStringLiteral("receiptSequence")))) return false;
        const bool infoObserved = information.value(QStringLiteral("observed")).toBool();
        const bool statusObserved = status.value(QStringLiteral("observed")).toBool();
        if ((!infoObserved && !statusObserved)
            || (statusObserved && status.value(QStringLiteral("informationRevision")).toString()
                != (infoObserved ? information.value(QStringLiteral("contentRevision")).toString() : QStringLiteral("0")))) return false;
        if (infoObserved) {
            const int count = information.value(QStringLiteral("declaredCount")).toInt();
            if (knownCountComponent == component && knownCount != count) return false;
            knownCountComponent = component; knownCount = count;
        }
        const bool matches = current && !saturated && information.value(QStringLiteral("observed")).toBool()
            && information.value(QStringLiteral("valid")).toBool() && status.value(QStringLiteral("observed")).toBool()
            && status.value(QStringLiteral("valid")).toBool()
            && status.value(QStringLiteral("informationRevision")) == information.value(QStringLiteral("contentRevision"))
            && status.value(QStringLiteral("informationRevision")).toString() != QStringLiteral("0");
        if (!optionalTelemetryFlag(status, "matchesCurrentInformation", matches)) return false;
    }
    return true;
}

QVariantMap withoutConfigurationBaselineEvidence(const QVariantMap& supplied)
{
    QVariantMap result = withoutNativeBatteryProjection(supplied);
    result.remove(QStringLiteral("configurationBaselineEvidence"));
    // Do not convert an invalid metadata type into a valid empty map. Only
    // this reserved readonly projection is stripped; all other validation
    // continues against the original remaining fields and types.
    if (result.value(QStringLiteral("metadata")).type() == QVariant::Map) {
        QVariantMap metadata = result.value(QStringLiteral("metadata")).toMap();
        metadata.remove(QStringLiteral("configurationBaselineEvidence"));
        result.insert(QStringLiteral("metadata"), metadata);
    }
    return result;
}

bool exactComparisonDto(const QVariant& first, const QVariant& second, int depth, int* remaining)
{
    // Comparison16 contains bounded display rows, not raw configuration. Its
    // wrapper can exceed the raw helper's 2048-node domain; still keep this
    // native-only equality finite and never invoke a custom QVariant operator.
    if (!remaining || depth > 12 || --(*remaining) < 0
        || first.userType() != second.userType() || first.isValid() != second.isValid()
        || first.isNull() != second.isNull()) return false;
    if (!first.isValid() || first.isNull()) return true;
    if (first.type() == QVariant::Map) {
        const QVariantMap a = first.toMap(), b = second.toMap();
        if (a.size() != b.size()) return false;
        for (auto it = a.constBegin(); it != a.constEnd(); ++it)
            if (!b.contains(it.key()) || !exactComparisonDto(it.value(), b.value(it.key()), depth + 1, remaining)) return false;
        return true;
    }
    if (first.type() == QVariant::List) {
        const QVariantList a = first.toList(), b = second.toList();
        if (a.size() != b.size()) return false;
        for (int index = 0; index < a.size(); ++index)
            if (!exactComparisonDto(a.at(index), b.at(index), depth + 1, remaining)) return false;
        return true;
    }
    switch (first.type()) {
    case QVariant::String: return first.toString() == second.toString();
    case QVariant::Bool: return first.toBool() == second.toBool();
    case QVariant::Int: return first.toInt() == second.toInt();
    case QVariant::UInt: return first.toUInt() == second.toUInt();
    case QVariant::LongLong: return first.toLongLong() == second.toLongLong();
    case QVariant::ULongLong: return first.toULongLong() == second.toULongLong();
    case QVariant::Double: return qIsFinite(first.toDouble()) && first.toDouble() == second.toDouble();
    default: return false;
    }
}

bool configurationApprovalMatches(const QVariantMap& approval, const VehicleCapability& vehicle)
{
    // Portable legacy records may omit ID/reason/time. Do not synthesize the
    // missing fields or treat configurationApproval()'s fallback as a record.
    if (approval.size() < 2 || approval.size() > 5 || vehicle.baselineSource != QStringLiteral("local_approval")
        || !isSha256Fingerprint(vehicle.baselineConfigurationFingerprint)) return false;
    for (const QString& key : approval.keys())
        if (key != QStringLiteral("systemId") && key != QStringLiteral("fingerprint") && key != QStringLiteral("source")
            && key != QStringLiteral("reason") && key != QStringLiteral("approvedAtUtc")) return false;
    if (approval.value(QStringLiteral("source")).type() != QVariant::String
        || approval.value(QStringLiteral("source")).toString().trimmed() != QStringLiteral("local_approval")
        || approval.value(QStringLiteral("fingerprint")).type() != QVariant::String
        || approval.value(QStringLiteral("fingerprint")).toString().trimmed().toLower() != vehicle.baselineConfigurationFingerprint) return false;
    int id = 0;
    if (approval.contains(QStringLiteral("systemId"))
        && (!strictInteger(approval.value(QStringLiteral("systemId")), &id) || id != vehicle.systemId)) return false;
    for (const QString& key : {QStringLiteral("reason"), QStringLiteral("approvedAtUtc")})
        if (approval.contains(key) && approval.value(key).type() != QVariant::String) return false;
    return true;
}

bool capabilityIngress(const QVariantMap& supplied, QVariantMap* output)
{
    if (!output) return false;
    QVariantMap normalized = withoutConfigurationBaselineEvidence(supplied);
    int nodes = 0; qint64 bytes = 0;
    if (!boundedDescriptorValue(normalized, 0, &nodes, &bytes)) return false;
    if (normalized.contains(QStringLiteral("metadata"))
        && normalized.value(QStringLiteral("metadata")).type() != QVariant::Map) return false;
    QVariantMap metadata = normalized.value(QStringLiteral("metadata")).toMap();
    QStringList observedTop, observedNested;
    const bool topObserved = normalized.contains(QStringLiteral("capabilities"));
    const bool nestedObserved = metadata.contains(QStringLiteral("capabilities"));
    if ((topObserved && !FleetCapabilityEvidence::canonicalCapabilities(normalized.value(QStringLiteral("capabilities")), &observedTop))
        || (nestedObserved && !FleetCapabilityEvidence::canonicalCapabilities(metadata.value(QStringLiteral("capabilities")), &observedNested))
        || (topObserved && nestedObserved && observedTop != observedNested)) return false;
    if (topObserved || nestedObserved) normalized.insert(QStringLiteral("capabilities"), topObserved ? observedTop : observedNested);
    if (nestedObserved) metadata.insert(QStringLiteral("capabilities"), observedNested);
    QStringList declarations;
    bool declarationProvided = false;
    const auto append = [&](const QVariant& raw) {
        QStringList canonical;
        if (!FleetCapabilityEvidence::canonicalCapabilities(raw, &canonical)) return false;
        declarationProvided = true;
        for (const QString& token : canonical) if (!declarations.contains(token)) declarations.append(token);
        return declarations.size() <= kMaxCapabilityEvidenceCount;
    };
    for (const QVariantMap& source : {normalized, metadata}) {
        for (const QString& key : {QStringLiteral("declaredVerifiedCapabilities"), QStringLiteral("verifiedCapabilities")})
            if (source.contains(key) && !append(source.value(key))) return false;
        if (source.contains(QStringLiteral("capabilityApproval"))) {
            const QVariant raw = source.value(QStringLiteral("capabilityApproval"));
            if (raw.type() != QVariant::Map || raw.isNull()) return false;
            const QVariantMap imported = raw.toMap();
            if (!imported.isEmpty()) {
                // Preserve one bounded imported record for review, NEVER as
                // a native approval, identity receipt or transferable grant.
                if (imported.contains(QStringLiteral("capabilities")) && !append(imported.value(QStringLiteral("capabilities")))) return false;
                if (metadata.contains(QStringLiteral("importedCapabilityApproval"))
                    && metadata.value(QStringLiteral("importedCapabilityApproval")).type() != QVariant::Map) return false;
                metadata.insert(QStringLiteral("importedCapabilityApproval"), imported);
            }
        }
    }
    for (const QString& key : {QStringLiteral("verifiedCapabilities"), QStringLiteral("capabilityReviewContext"),
            QStringLiteral("capabilityApproval"), QStringLiteral("declaredVerifiedCapabilities")}) {
        normalized.remove(key); metadata.remove(key);
    }
    if (declarationProvided) {
        std::sort(declarations.begin(), declarations.end());
        normalized.insert(QStringLiteral("declaredVerifiedCapabilities"), declarations);
    }
    if (supplied.contains(QStringLiteral("metadata")) || !metadata.isEmpty()) normalized.insert(QStringLiteral("metadata"), metadata);
    *output = normalized;
    return true;
}

QString normalizedState(int score, bool online, bool fault, bool healthKnown)
{
    if (!online) {
        return QStringLiteral("offline");
    }
    // An explicit current fault is critical even if another health stream is
    // unknown; unknown must never downgrade a positive fault assertion.
    if (fault) {
        return QStringLiteral("critical");
    }
    if (!healthKnown) {
        return QStringLiteral("unknown");
    }
    if (score < 35) {
        return QStringLiteral("critical");
    }
    if (score < 75) {
        return QStringLiteral("degraded");
    }
    return QStringLiteral("healthy");
}

}

FleetRegistry::FleetRegistry(QObject* parent)
    : QObject(parent), _nativeBatteryBindingsEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces)),
      _capabilityRegistryEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _healthTimer.setInterval(1000);
    connect(&_healthTimer, &QTimer::timeout, this, &FleetRegistry::_refreshHealthTimer);
    _healthTimer.start();
}

int FleetRegistry::onlineVehicleCount() const
{
    int result = 0;
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QVariantList snapshots = vehicles();
    if (!self) return 0;
    for (const QVariant& value : snapshots) {
        if (value.toMap().value(QStringLiteral("online")).toBool()) {
            ++result;
        }
    }
    return result;
}

int FleetRegistry::healthyVehicleCount() const
{
    int result = 0;
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QVariantList snapshots = vehicles();
    if (!self) return 0;
    for (const QVariant& value : snapshots) {
        const QVariantMap vehicle = value.toMap();
        if (vehicle.value(QStringLiteral("online")).toBool()
            && vehicle.value(QStringLiteral("healthState")).toString() == QStringLiteral("healthy")) {
            ++result;
        }
    }
    return result;
}

QVariantList FleetRegistry::vehicles() const
{
    if (thread() != QThread::currentThread()) return {};
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QString bindingsEpoch = _nativeBatteryBindingsEpoch;
    const QString registryEpoch = _capabilityRegistryEpoch;
    QVariantList result;
    QList<int> ids = _vehicles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int systemId : ids) {
        const QVariantMap snapshot = _vehicleSnapshot(systemId);
        if (!self || _nativeBatteryBindingsEpoch != bindingsEpoch
            || _capabilityRegistryEpoch != registryEpoch || snapshot.isEmpty()) return {};
        result.append(snapshot);
    }
    return result;
}

QVariantMap FleetRegistry::fleetHealthSummary() const
{
    int healthy = 0;
    int degraded = 0;
    int critical = 0;
    int offline = 0;
    int unknown = 0;
    int scoreSum = 0;
    int online = 0;
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QVariantList snapshots = vehicles();
    if (!self) return {};
    for (const QVariant& value : snapshots) {
        const QVariantMap vehicle = value.toMap();
        const QString state = vehicle.value(QStringLiteral("healthState")).toString();
        if (vehicle.value(QStringLiteral("online")).toBool()) ++online;
        if (state == QStringLiteral("healthy")) {
            ++healthy;
        } else if (state == QStringLiteral("degraded")) {
            ++degraded;
        } else if (state == QStringLiteral("critical")) {
            ++critical;
        } else if (state == QStringLiteral("unknown")) {
            ++unknown;
        } else {
            ++offline;
        }
        scoreSum += vehicle.value(QStringLiteral("healthScore")).toInt();
    }

    QVariantMap result;
    result[QStringLiteral("vehicleCount")] = snapshots.size();
    result[QStringLiteral("onlineVehicleCount")] = online;
    result[QStringLiteral("healthyVehicleCount")] = healthy;
    result[QStringLiteral("degradedVehicleCount")] = degraded;
    result[QStringLiteral("criticalVehicleCount")] = critical;
    result[QStringLiteral("offlineVehicleCount")] = offline;
    result[QStringLiteral("unknownVehicleCount")] = unknown;
    result[QStringLiteral("score")] = snapshots.isEmpty() ? 0 : qRound(static_cast<double>(scoreSum) / snapshots.size());
    result[QStringLiteral("state")] = critical > 0 ? QStringLiteral("critical")
             : (degraded > 0 || offline > 0 ? QStringLiteral("degraded")
             : (unknown > 0 ? QStringLiteral("unknown")
                            : (snapshots.isEmpty() ? QStringLiteral("unknown") : QStringLiteral("healthy"))));
    return result;
}

void FleetRegistry::setOfflineTimeoutMs(int timeoutMs)
{
    const int bounded = qBound(500, timeoutMs, 600000);
    if (_offlineTimeoutMs == bounded) {
        return;
    }
    _offlineTimeoutMs = bounded;
    const QPointer<FleetRegistry> self(this);
    emit offlineTimeoutMsChanged();
    if (!self) return;
    refreshHealth();
}

VehicleCapability* FleetRegistry::_find(int systemId)
{
    auto iterator = _vehicles.find(systemId);
    return iterator == _vehicles.end() ? nullptr : &iterator.value();
}

const VehicleCapability* FleetRegistry::_find(int systemId) const
{
    auto iterator = _vehicles.constFind(systemId);
    return iterator == _vehicles.constEnd() ? nullptr : &iterator.value();
}

void FleetRegistry::bindNativeBatterySource(int systemId, Vehicle* source)
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255) return;
    const CapabilityReviewState review = _capabilityReviews.value(systemId);
    if (!_vehicles.contains(systemId) || review.vehicleRecordEpoch.isEmpty()
        || (source && source->thread() != thread())) source = nullptr;
    const NativeBatterySource previous = _nativeBatterySources.value(systemId);
    if (!source) {
        if (!_nativeBatterySources.remove(systemId)) return;
    } else {
        if (previous.vehicle == source && previous.vehicleRecordEpoch == review.vehicleRecordEpoch) return;
        NativeBatterySource binding;
        binding.vehicle = source;
        binding.vehicleRecordEpoch = review.vehicleRecordEpoch;
        binding.bindingEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
        _nativeBatterySources.insert(systemId, binding);
    }
    _nativeBatteryBindingsEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QPointer<FleetRegistry> self(this);
    emit registryChanged();
    if (!self) return;
    emit healthChanged();
}

bool FleetRegistry::_nativeBatteryBindingCurrent(int systemId, const QString& recordEpoch,
                                                 const NativeBatterySource& binding) const
{
    if (!_vehicles.contains(systemId)
        || _capabilityReviews.value(systemId).vehicleRecordEpoch != recordEpoch) return false;
    if (binding.bindingEpoch.isEmpty()) return !_nativeBatterySources.contains(systemId);
    const NativeBatterySource current = _nativeBatterySources.value(systemId);
    return current.bindingEpoch == binding.bindingEpoch && current.vehicleRecordEpoch == recordEpoch
        && current.vehicle == binding.vehicle;
}

void FleetRegistry::_retireNativeConfigurationSource(int systemId)
{
    // Remove before disconnectNotify: a reentrant binder may install a new
    // source, and these old handles must never erase that newer binding.
    const NativeConfigurationSource retired = _nativeConfigurationSources.take(systemId);
    for (const QMetaObject::Connection& connection : retired.connections) QObject::disconnect(connection);
}

bool FleetRegistry::_nativeConfigurationBindingCurrent(int systemId, const NativeConfigurationSource& expected) const
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255 || expected.bindingEpoch.isEmpty()) return false;
    const NativeConfigurationSource current = _nativeConfigurationSources.value(systemId);
    if (current.bindingEpoch != expected.bindingEpoch || current.ready != expected.ready
        || current.vehicle != expected.vehicle || current.parameters != expected.parameters || current.links != expected.links
        || current.firmwarePlugin != expected.firmwarePlugin
        || current.manager != expected.manager || current.model != expected.model
        || current.motorLayoutParameter != expected.motorLayoutParameter || current.motorLayoutFact != expected.motorLayoutFact
        || current.primary.owner_before(expected.primary) || expected.primary.owner_before(current.primary)
        || !expected.vehicle || !expected.parameters || !expected.firmwarePlugin || !expected.links || !expected.manager || !expected.model
        || expected.vehicle->thread() != thread() || expected.parameters->thread() != thread() || expected.links->thread() != thread()
        || expected.manager->thread() != thread() || expected.model->thread() != thread()
        || expected.vehicle->id() != systemId || expected.componentId < 1 || expected.componentId > 255
        || expected.vehicle->defaultComponentId() != expected.componentId
        || static_cast<int>(expected.vehicle->vehicleType()) != expected.vehicleType
        || static_cast<int>(expected.vehicle->firmwareType()) != expected.firmwareType
        || expected.versionSourceEpoch.isEmpty()
        || expected.vehicle->autopilotVersionObservationSourceEpochMemory() != expected.versionSourceEpoch
        || expected.vehicle->isOfflineEditingVehicle() || !expected.vehicle->isInitialConnectComplete()
        || expected.vehicle->parameterManager() != expected.parameters.data() || expected.parameters->vehicle() != expected.vehicle.data()
        || expected.firmwarePlugin->thread() != thread() || expected.vehicle->firmwarePlugin() != expected.firmwarePlugin.data()
        || expected.vehicle->vehicleLinkManager() != expected.links.data() || expected.links->communicationLost() || expected.primary.expired()
        || !qgcApp() || !qgcApp()->toolbox() || qgcApp()->toolbox()->multiVehicleManager() != expected.manager.data()
        || expected.manager->vehicles() != expected.model.data() || expected.manager->getVehicleById(systemId) != expected.vehicle.data()) return false;
    const auto primary = expected.links->primaryLink();
    if (primary.expired() || primary.owner_before(expected.primary) || expected.primary.owner_before(primary)) return false;
    int matches = 0;
    if (expected.model->count() < 1) return false;
    for (int index = 0; index < expected.model->count(); ++index) {
        Vehicle* member = qobject_cast<Vehicle*>(expected.model->get(index));
        if (member && member->id() == systemId) {
            if (member != expected.vehicle.data()) return false;
            ++matches;
        }
    }
    if (matches != 1) return false;
    if (expected.motorLayoutParameter == QStringLiteral("FRAME")) {
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
        if (!expected.copterPlugin || current.copterPlugin != expected.copterPlugin
            || current.copterCoaxialMotors != expected.copterCoaxialMotors
            || expected.copterPlugin.data() != expected.firmwarePlugin.data()
            // Explicit qualified call is non-virtual. The fixed built-in
            // implementation only returns its local _coaxialMotors bool.
            || expected.copterPlugin->ArduCopterFirmwarePlugin::multiRotorCoaxialMotors(expected.vehicle.data())
                != expected.copterCoaxialMotors
            || expected.configuration.value(QStringLiteral("coaxialMotors")).type() != QVariant::Bool
            || expected.configuration.value(QStringLiteral("coaxialMotors")).toBool() != expected.copterCoaxialMotors) return false;
#else
        return false; // This compiled build has no supported fixed APM implementation.
#endif
    }
    if (expected.motorLayoutParameter != configurationMotorLayoutParameter(expected.firmwareType, expected.vehicleType)) return false;
    if (!expected.motorLayoutParameter.isEmpty()) {
        if (expected.motorLayoutFact) {
            if (expected.motorLayoutFact->thread() != thread() || expected.motorLayoutFact->componentId() != expected.componentId
                || expected.motorLayoutFact->name() != expected.motorLayoutParameter
                || !FleetConfigurationDiff::exactEqual(current.motorLayoutValue, expected.motorLayoutValue)
                || !FleetConfigurationDiff::exactEqual(expected.motorLayoutFact->rawValue(), expected.motorLayoutValue)) return false;
        } else if (expected.ready) return false;
    } else if (expected.motorLayoutFact || expected.motorLayoutValue.isValid()) return false;
    // These exact local scalar getters do not invoke a firmware plugin.
    // Motor/plugin fields are freshly checked by capture before this final
    // memory fence; this is not a journal of unreported parameter changes.
    QVariantMap scalars{{QStringLiteral("firmwareVersion"), QStringLiteral("%1.%2.%3").arg(expected.vehicle->firmwareMajorVersion())
            .arg(expected.vehicle->firmwareMinorVersion()).arg(expected.vehicle->firmwarePatchVersion())},
        {QStringLiteral("sensorsPresentBits"), expected.vehicle->sensorsPresentBits()},
        {QStringLiteral("sensorsEnabledBits"), expected.vehicle->sensorsEnabledBits()}};
    const QString gitHash = expected.vehicle->gitHash().trimmed();
    if (!gitHash.isEmpty()) scalars.insert(QStringLiteral("gitHash"), gitHash);
    if (expected.vehicle->vehicleUID() != 0) scalars.insert(QStringLiteral("vehicleUID"), expected.vehicle->vehicleUIDStr());
    QVariantMap expectedScalars = expected.configuration;
    // Display strings use tr()/QTranslator. Do not re-enter translation from
    // this memory-only fence: the original native enum values were checked above.
    expectedScalars.remove(QStringLiteral("vehicleType")); expectedScalars.remove(QStringLiteral("firmwareType"));
    expectedScalars.remove(QStringLiteral("motorCount")); expectedScalars.remove(QStringLiteral("coaxialMotors"));
    expectedScalars.remove(QStringLiteral("xConfigMotors"));
    if (!FleetConfigurationDiff::exactEqual(scalars, expectedScalars)) return false;
    const auto vehicle = _vehicles.constFind(systemId);
    const auto review = _capabilityReviews.constFind(systemId);
    const auto history = _configurationHistories.constFind(systemId);
    return vehicle != _vehicles.constEnd() && review != _capabilityReviews.constEnd() && history != _configurationHistories.constEnd()
        && !review->exhausted && !history->exhausted && history->currentFullKnown && history->lastFullKnown
        && review->vehicleRecordEpoch == expected.vehicleRecordEpoch && history->vehicleRecordEpoch == expected.vehicleRecordEpoch
        && history->observationEpoch == expected.observationEpoch && history->revision == expected.observationRevision
        && _capabilityContext(*vehicle, *review) == expected.registryContext
        && !expected.configuration.isEmpty() && FleetConfigurationDiff::exactEqual(vehicle->configuration, expected.configuration)
        && FleetConfigurationDiff::exactEqual(history->lastFull, expected.configuration);
}

void FleetRegistry::bindNativeConfigurationSource(int systemId, Vehicle* source, const QVariantMap& configuration)
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255) return;
    const QPointer<FleetRegistry> self(this);
    const QPointer<Vehicle> actual(source);
    NativeConfigurationSource candidate;
    if (actual && actual->thread() == thread() && actual->id() == systemId
        && !configuration.isEmpty() && FleetConfigurationDiff::valueValid(configuration)
        && _vehicles.contains(systemId) && _capabilityReviews.contains(systemId) && _configurationHistories.contains(systemId)
        && qgcApp() && qgcApp()->toolbox()) {
        const CapabilityReviewState review = _capabilityReviews.value(systemId);
        const ConfigurationHistory history = _configurationHistories.value(systemId);
        candidate.vehicle = actual; candidate.parameters = actual->parameterManager(); candidate.links = actual->vehicleLinkManager();
        candidate.firmwarePlugin = actual->firmwarePlugin();
        candidate.componentId = actual->defaultComponentId();
        candidate.vehicleType = static_cast<int>(actual->vehicleType());
        candidate.firmwareType = static_cast<int>(actual->firmwareType());
        candidate.versionSourceEpoch = actual->autopilotVersionObservationSourceEpochMemory();
        candidate.motorLayoutParameter = configurationMotorLayoutParameter(candidate.firmwareType, candidate.vehicleType);
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
        if (candidate.motorLayoutParameter == QStringLiteral("FRAME")) {
            candidate.copterPlugin = qobject_cast<ArduCopterFirmwarePlugin*>(candidate.firmwarePlugin.data());
            if (candidate.copterPlugin)
                candidate.copterCoaxialMotors = candidate.copterPlugin->ArduCopterFirmwarePlugin::multiRotorCoaxialMotors(actual.data());
        }
#endif
        candidate.manager = qgcApp()->toolbox()->multiVehicleManager();
        candidate.model = candidate.manager ? candidate.manager->vehicles() : nullptr;
        if (candidate.links) candidate.primary = candidate.links->primaryLink();
        candidate.vehicleRecordEpoch = review.vehicleRecordEpoch;
        candidate.registryContext = _capabilityContext(_vehicles.value(systemId), review);
        candidate.observationEpoch = history.observationEpoch; candidate.observationRevision = history.revision;
        candidate.configuration = configuration;
    }
    const NativeConfigurationSource previous = _nativeConfigurationSources.value(systemId);
    if (previous.ready && previous.vehicle == actual && _nativeConfigurationBindingCurrent(systemId, previous)
        && FleetConfigurationDiff::exactEqual(previous.configuration, configuration)) return;
    _retireNativeConfigurationSource(systemId);
    if (!self || _nativeConfigurationSources.contains(systemId)) return; // A disconnect callback installed its own newer source.
    if (!candidate.vehicle || candidate.registryContext.isEmpty() || candidate.observationEpoch.isEmpty()) return;
    candidate.bindingEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _nativeConfigurationSources.insert(systemId, candidate); // Not readable until all guarded hooks exist.
    const QString epoch = candidate.bindingEpoch;
    const auto pendingCurrent = [&]() {
        return self && self->_nativeConfigurationBindingCurrent(systemId, candidate);
    };
    const auto retire = [self, systemId, epoch]() {
        if (self && self->_nativeConfigurationSources.value(systemId).bindingEpoch == epoch)
            self->_retireNativeConfigurationSource(systemId);
    };
    const auto keep = [&](const QMetaObject::Connection& connection) {
        if (!connection || !pendingCurrent()) {
            QObject::disconnect(connection);
            retire();
            return false;
        }
        self->_nativeConfigurationSources[systemId].connections.append(connection);
        return true;
    };
    if (!pendingCurrent()) { retire(); return; }
    if (!candidate.motorLayoutParameter.isEmpty()) {
        // Only an existing, exact fixed Fact may bind. Never request a missing
        // parameter or accept getParameter's default/error placeholder.
        const bool exists = candidate.parameters->parameterExists(candidate.componentId, candidate.motorLayoutParameter);
        if (!pendingCurrent() || !exists) { retire(); return; }
        const QPointer<Fact> fact(candidate.parameters->getParameter(candidate.componentId, candidate.motorLayoutParameter));
        if (!pendingCurrent() || !fact || fact->thread() != thread() || fact->componentId() != candidate.componentId
            || fact->name() != candidate.motorLayoutParameter) { retire(); return; }
        const QVariant rawValue = fact->rawValue();
        if (!rawValue.isValid() || rawValue.isNull() || !FleetConfigurationDiff::valueValid(rawValue)) { retire(); return; }
        candidate.motorLayoutFact = fact; candidate.motorLayoutValue = rawValue;
        self->_nativeConfigurationSources[systemId].motorLayoutFact = fact;
        self->_nativeConfigurationSources[systemId].motorLayoutValue = rawValue;
        if (!pendingCurrent()) { retire(); return; }
        if (!keep(connect(fact.data(), &QObject::destroyed, this, retire))
            || !keep(connect(fact.data(), &Fact::rawValueChanged, this, retire))
            || !keep(connect(candidate.parameters.data(), &ParameterManager::factAdded, this,
                [retire, component = candidate.componentId](int actualComponent, Fact*) {
                    // Do not dereference a signal's raw Fact pointer: an
                    // earlier receiver may have destroyed it. Any addition
                    // in this exact component conservatively retires the bind.
                    if (actualComponent == component) retire();
                }))) return;
    }
    if (!keep(connect(candidate.vehicle.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.parameters.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.firmwarePlugin.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.links.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.manager.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.model.data(), &QObject::destroyed, this, retire))
        || !keep(connect(candidate.links.data(), &VehicleLinkManager::primaryLinkChanged, this, retire))
        || !keep(connect(candidate.links.data(), &VehicleLinkManager::communicationLostChanged, this, retire))
        || !keep(connect(candidate.manager.data(), &MultiVehicleManager::vehicleRemoved, this,
            [retire, actual](Vehicle* removed) { if (!actual || removed == actual.data()) retire(); }))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::firmwareTypeChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::vehicleTypeChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::firmwareVersionChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::gitHashChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::vehicleUIDChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::sensorsPresentBitsChanged, this, retire))
        || !keep(connect(candidate.vehicle.data(), &Vehicle::sensorsEnabledBitsChanged, this, retire))) return;
    if (!pendingCurrent()) { retire(); return; }
    // The fixed Fact hooks now retire A->B->A mutations during plugin calls.
    // Re-observe after capturing those dependencies, never bind a newer Fact
    // to a stale descriptor merely because both use the same system id.
    const QVariantMap verified = FleetNativeConfigurationObservation::capture(actual.data(), pendingCurrent);
    if (!pendingCurrent() || verified.value(QStringLiteral("ok")).type() != QVariant::Bool
        || !verified.value(QStringLiteral("ok")).toBool()
        || !FleetConfigurationDiff::exactEqual(verified.value(QStringLiteral("configuration")), candidate.configuration)) {
        retire(); return;
    }
    _nativeConfigurationSources[systemId].ready = true;
}

QVariantMap FleetRegistry::_nativeConfigurationEvidenceValue(int systemId, const NativeConfigurationSource& binding) const
{
    if (!binding.ready || !_nativeConfigurationBindingCurrent(systemId, binding)) return {};
    QVariantMap snapshot = _vehicles.value(systemId).toVariantMap();
    snapshot.insert(QStringLiteral("configurationBaselineEvidence"), _configurationBaselineEvidence(systemId));
    QVariantMap baseline;
    if (!FleetCapabilityEvidence::currentConfigurationBaseline(snapshot, &baseline)) return {};
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("native-current-configuration-observation")}, {QStringLiteral("vehicleId"), systemId},
        {QStringLiteral("bindingEpoch"), binding.bindingEpoch}, {QStringLiteral("registryContext"), binding.registryContext},
        {QStringLiteral("configurationBaselineEvidence"), baseline},
        {QStringLiteral("currentTypedFingerprint"), baseline.value(QStringLiteral("currentTypedFingerprint"))},
        {QStringLiteral("configurationFingerprint"), binding.registryContext.value(QStringLiteral("configurationFingerprint"))},
        {QStringLiteral("bindingMatchesCurrentObservation"), true}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("physicalEnvelopeCertified"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetRegistry::nativeConfigurationEvidence(Vehicle* actualVehicle) const
{
    const auto reject = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
            {QStringLiteral("evidence"), QVariantMap()}};
    };
    const QPointer<Vehicle> actual(actualVehicle);
    if (thread() != QThread::currentThread() || !actual || actual->thread() != thread())
        return reject(QStringLiteral("native-configuration-owner-thread-or-vehicle-invalid"));
    if (_nativeConfigurationObservationActive) return reject(QStringLiteral("native-configuration-observation-reentrant"));
    const int id = actual->id();
    const NativeConfigurationSource binding = _nativeConfigurationSources.value(id);
    if (!binding.ready || binding.vehicle != actual || !_nativeConfigurationBindingCurrent(id, binding))
        return reject(QStringLiteral("native-configuration-binding-not-current"));
    const QVariantMap baseline = _configurationBaselineEvidence(id);
    QVariantMap snapshot = _vehicles.value(id).toVariantMap();
    snapshot.insert(QStringLiteral("configurationBaselineEvidence"), baseline);
    QVariantMap canonicalBaseline;
    if (!FleetCapabilityEvidence::currentConfigurationBaseline(snapshot, &canonicalBaseline))
        return reject(QStringLiteral("native-configuration-current-local-baseline-required"));
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const auto current = [&]() {
        return self && actual && binding.vehicle == actual && self->_nativeConfigurationBindingCurrent(id, binding)
            && FleetConfigurationDiff::exactEqual(self->_configurationBaselineEvidence(id), baseline);
    };
    struct ObservationGuard {
        QPointer<FleetRegistry> owner;
        ~ObservationGuard() { if (owner) owner->_nativeConfigurationObservationActive = false; }
    };
    _nativeConfigurationObservationActive = true;
    const ObservationGuard guard{self};
    const QVariantMap observed = FleetNativeConfigurationObservation::capture(actual.data(), current);
    if (!current() || observed.size() != 3 || observed.value(QStringLiteral("ok")).type() != QVariant::Bool
        || !observed.value(QStringLiteral("ok")).toBool() || observed.value(QStringLiteral("configuration")).type() != QVariant::Map
        || !FleetConfigurationDiff::exactEqual(observed.value(QStringLiteral("configuration")), binding.configuration)) {
        // Once this actual observation failed or differed, returning later
        // to the same value must not revive the original binding. Removal is
        // runtime provenance retirement only, not a configuration/approval edit.
        if (self && self->_nativeConfigurationSources.value(id).bindingEpoch == binding.bindingEpoch)
            self->_retireNativeConfigurationSource(id);
        return reject(QStringLiteral("native-configuration-observation-no-longer-matches-binding"));
    }
    const QVariantMap evidence = _nativeConfigurationEvidenceValue(id, binding);
    if (!current() || evidence.size() != 13) return reject(QStringLiteral("native-configuration-source-changed-at-return"));
    return {{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("evidence"), evidence}};
}

QVariantMap FleetRegistry::nativeConfigurationEvidenceCurrent(Vehicle* actualVehicle, const QVariantMap& expectedEvidence) const
{
    const QVariantMap expected = expectedEvidence;
    const auto reject = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
            {QStringLiteral("evidence"), QVariantMap()}};
    };
    if (expected.size() != 13 || !FleetConfigurationDiff::valueValid(expected))
        return reject(QStringLiteral("native-configuration-expected-evidence-invalid"));
    const QVariantMap current = nativeConfigurationEvidence(actualVehicle);
    if (current.value(QStringLiteral("ok")).type() != QVariant::Bool || !current.value(QStringLiteral("ok")).toBool()) return current;
    if (!FleetConfigurationDiff::exactEqual(current.value(QStringLiteral("evidence")), expected))
        return reject(QStringLiteral("native-configuration-evidence-changed"));
    return current;
}

bool FleetRegistry::nativeConfigurationEvidenceMemoryCurrent(Vehicle* actualVehicle, const QVariantMap& expectedEvidence) const
{
    if (thread() != QThread::currentThread() || !actualVehicle || actualVehicle->thread() != thread()
        || expectedEvidence.size() != 13 || !FleetConfigurationDiff::valueValid(expectedEvidence)) return false;
    const int id = actualVehicle->id();
    const NativeConfigurationSource binding = _nativeConfigurationSources.value(id);
    if (binding.vehicle.data() != actualVehicle) return false;
    const QVariantMap actualEvidence = _nativeConfigurationEvidenceValue(id, binding);
    return actualEvidence.size() == 13 && FleetConfigurationDiff::exactEqual(actualEvidence, expectedEvidence);
}

QVariantMap FleetRegistry::_vehicleSnapshot(int systemId) const
{
    if (thread() != QThread::currentThread() || !_vehicles.contains(systemId)) return {};
    // Copy before invoking Vehicle. The current native getter is readonly
    // memory projection; keep ownership/reentry guards if it later changes.
    // No live hash pointer/iterator crosses that call.
    VehicleCapability vehicle = _vehicles.value(systemId);
    const QString recordEpoch = _capabilityReviews.value(systemId).vehicleRecordEpoch;
    const NativeBatterySource binding = _nativeBatterySources.value(systemId);
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QPointer<Vehicle> source = binding.vehicle;
    QVariantMap evidence = unknownNativeBatteryEvidence(systemId, QStringLiteral("registry-native-source-unbound"));
    QVariantMap drainEvidence = FleetNativeBatteryEvidence::unknownDrainEvidence(
        systemId, QStringLiteral("registry-native-source-unbound"));
    if (_nativeBatteryProjectionActive) {
        evidence = unknownNativeBatteryEvidence(systemId, QStringLiteral("registry-native-read-reentrant"));
        drainEvidence = FleetNativeBatteryEvidence::unknownDrainEvidence(systemId, QStringLiteral("registry-native-read-reentrant"));
    } else if (source && !recordEpoch.isEmpty() && binding.vehicleRecordEpoch == recordEpoch) {
        struct ProjectionGuard {
            QPointer<FleetRegistry> owner;
            ~ProjectionGuard() { if (owner) owner->_nativeBatteryProjectionActive = false; }
        };
        {
            _nativeBatteryProjectionActive = true;
            const ProjectionGuard guard{self};
            evidence = source->fleetBatteryTelemetryObservation(kNativeBatteryMaximumAgeMs);
            if (!self || !source || !_nativeBatteryBindingCurrent(systemId, recordEpoch, binding)) return {};
            drainEvidence = source->fleetBatteryDrainObservation(kNativeBatteryMaximumAgeMs);
        }
        if (!self || !source || !_nativeBatteryBindingCurrent(systemId, recordEpoch, binding)) return {};
        if (!nativeBatteryEnvelope(evidence, systemId).valid) {
            evidence = unknownNativeBatteryEvidence(systemId, QStringLiteral("registry-native-observation-invalid"));
        }
    } else if (!binding.bindingEpoch.isEmpty()) {
        evidence = unknownNativeBatteryEvidence(systemId, QStringLiteral("registry-native-source-unavailable"));
        drainEvidence = FleetNativeBatteryEvidence::unknownDrainEvidence(systemId, QStringLiteral("registry-native-source-unavailable"));
    }
    if (!self || !_nativeBatteryBindingCurrent(systemId, recordEpoch, binding)) return {};
    // A synchronous callback may have updated the same record. Take its
    // current static/health values only after the owner check, then project
    // the native battery onto the copy without changing persisted state.
    vehicle = _vehicles.value(systemId);
    vehicle.batteryPercent = evidence.value(QStringLiteral("batteryPercentKnown")).toBool()
        ? evidence.value(QStringLiteral("batteryPercent")).toDouble() : -1.0;
    const FleetNativeBatteryEvidence::Read battery = nativeBatteryEnvelope(evidence, systemId);
    if (battery.valid && battery.reportedUnsafe) {
        // This is only the returned copy. Never persist a transient battery
        // assertion or replace another source's fault/known/age evidence.
        vehicle.hasFault = true;
        if (!vehicle.faultSummary.isEmpty()) vehicle.faultSummary += QStringLiteral("; ");
        vehicle.faultSummary += battery.reason;
    }
    _recomputeHealthValue(vehicle, QDateTime::currentDateTimeUtc(), _offlineTimeoutMs);
    if (!battery.valid || !battery.usable) {
        if (!(battery.valid && battery.reportedUnsafe) && vehicle.healthState != QStringLiteral("critical")
            && vehicle.healthState != QStringLiteral("offline")) {
            vehicle.healthState = QStringLiteral("unknown");
            vehicle.healthScore = 0;
        }
        if (!vehicle.healthReason.isEmpty()) vehicle.healthReason += QStringLiteral("; ");
        vehicle.healthReason += battery.reason.isEmpty()
            ? QStringLiteral("native_battery_evidence_unavailable") : battery.reason;
    }
    QVariantMap snapshot = withoutNativeBatteryProjection(withoutConfigurationBaselineEvidence(vehicle.toVariantMap()));
    snapshot.insert(QStringLiteral("configurationBaselineEvidence"), _configurationBaselineEvidence(systemId));
    snapshot.insert(QStringLiteral("nativeBatteryEvidenceRequired"), true);
    snapshot.insert(QStringLiteral("nativeBatteryEvidence"), evidence);
    snapshot.insert(QStringLiteral("nativeBatteryDrainEvidence"), drainEvidence);
    if (evidence.value(QStringLiteral("remainingSecondsKnown")).toBool())
        snapshot.insert(QStringLiteral("remainingFlightSeconds"), evidence.value(QStringLiteral("remainingFlightSeconds")));
    // An invalid actual producer result must remain invalid to consumers,
    // rather than becoming a legitimate unknown that silently falls back to
    // a more optimistic static budget. The value reader authenticates nothing.
    if (!FleetNativeBatteryEvidence::inspectDrain(snapshot, systemId).valid)
        snapshot.insert(QStringLiteral("nativeBatteryDrainEvidence"), QVariantMap());
    return snapshot;
}

QVariantMap FleetRegistry::_configurationBaselineEvidence(int systemId) const
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255) return {};
    const auto vehicle = _vehicles.constFind(systemId);
    const auto review = _capabilityReviews.constFind(systemId);
    const auto history = _configurationHistories.constFind(systemId);
    if (vehicle == _vehicles.constEnd() || review == _capabilityReviews.constEnd()
        || history == _configurationHistories.constEnd() || history->exhausted || review->exhausted
        || !history->revision || history->observationEpoch.isEmpty()
        || history->vehicleRecordEpoch != review->vehicleRecordEpoch) return {};
    // Every full/configuration/baseline mutation stages this private history
    // together with the actual row. A getter does not rediscover a baseline
    // from matching caller hashes or rehash a 2048-node configuration.
    if (history->currentFullKnown && (!history->lastFullKnown || vehicle->configuration.isEmpty()
        || history->lastFingerprint != vehicle->configurationFingerprint
        || history->currentTypedFingerprint.isEmpty())) return {};
    if (history->baselineKnown && (vehicle->baselineSource != QStringLiteral("local_approval")
        || vehicle->baselineConfigurationFingerprint != history->baselineFingerprint
        || history->baselineTypedFingerprint.isEmpty() || history->baselineApprovalHash.isEmpty()
        || !FleetConfigurationDiff::exactEqual(_configurationApprovals.value(systemId), history->baselineApproval))) return {};
    const QString currentHash = history->currentFullKnown ? history->currentTypedFingerprint : QString();
    const QString baselineHash = history->baselineKnown ? history->baselineTypedFingerprint : QString();
    const bool matches = history->currentFullKnown && history->baselineKnown && currentHash == baselineHash;
    if (matches != history->baselineMatches) return {};
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("native-local-configuration-baseline")},
        {QStringLiteral("registryContext"), _capabilityContext(*vehicle, *review)},
        {QStringLiteral("observationEpoch"), history->observationEpoch},
        {QStringLiteral("observationRevision"), QString::number(history->revision)},
        {QStringLiteral("currentFullKnown"), history->currentFullKnown},
        {QStringLiteral("baselineFullKnown"), history->baselineKnown},
        {QStringLiteral("currentTypedFingerprint"), currentHash},
        {QStringLiteral("baselineTypedFingerprint"), baselineHash},
        {QStringLiteral("baselineApprovalHash"), history->baselineKnown ? history->baselineApprovalHash : QString()},
        {QStringLiteral("baselineMatches"), matches},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetRegistry::_capabilityContext(const VehicleCapability& vehicle, const CapabilityReviewState& state) const
{
    if (state.exhausted || !state.revision || !isSha256Fingerprint(vehicle.configurationFingerprint)
        || state.identityFingerprint != FleetCapabilityEvidence::identityFingerprint(vehicle.toVariantMap())) return {};
    if (!vehicle.configuration.isEmpty()
        && VehicleCapability::fingerprintForConfiguration(vehicle.configuration) != vehicle.configurationFingerprint) return {};
    const QVariantMap context{{QStringLiteral("registryEpoch"), _capabilityRegistryEpoch},
        {QStringLiteral("vehicleRecordEpoch"), state.vehicleRecordEpoch}, {QStringLiteral("vehicleId"), vehicle.systemId},
        {QStringLiteral("capabilityRevision"), QString::number(state.revision)},
        {QStringLiteral("identityFingerprint"), state.identityFingerprint},
        {QStringLiteral("configurationFingerprint"), vehicle.configurationFingerprint}};
    return FleetCapabilityEvidence::contextValid(context) ? context : QVariantMap();
}

void FleetRegistry::_projectCapabilityReview(VehicleCapability& vehicle, const CapabilityReviewState& state) const
{
    // These three keys are produced ONLY from this registry's private native
    // record. Telemetry/import values have already become declarations.
    vehicle.metadata.remove(QStringLiteral("verifiedCapabilities"));
    vehicle.metadata.remove(QStringLiteral("capabilityReviewContext"));
    vehicle.metadata.remove(QStringLiteral("capabilityApproval"));
    vehicle.metadata.remove(QStringLiteral("configurationBaselineEvidence"));
    const QVariantMap context = _capabilityContext(vehicle, state);
    QVariantMap projected = vehicle.toVariantMap();
    projected.insert(QStringLiteral("capabilityReviewContext"), context);
    projected.insert(QStringLiteral("capabilityApproval"), state.approval);
    QStringList effective;
    FleetCapabilityEvidence::currentCapabilities(projected, &effective);
    vehicle.metadata.insert(QStringLiteral("capabilityReviewContext"), context);
    vehicle.metadata.insert(QStringLiteral("capabilityApproval"), state.approval);
    vehicle.metadata.insert(QStringLiteral("verifiedCapabilities"), effective);
    QString evidenceState = QStringLiteral("unknown");
    if (!effective.isEmpty()) evidenceState = QStringLiteral("local-engineering-attested");
    else if (state.exhausted) evidenceState = QStringLiteral("review-revision-exhausted");
    else if (!state.approval.isEmpty()) evidenceState = QStringLiteral("review-") + state.approval.value(QStringLiteral("state")).toString();
    else if (!vehicle.metadata.value(QStringLiteral("declaredVerifiedCapabilities")).toStringList().isEmpty()) evidenceState = QStringLiteral("declared-unreviewed");
    else if (!vehicle.capabilities.isEmpty()) evidenceState = QStringLiteral("observed-unverified");
    vehicle.metadata.insert(QStringLiteral("capabilityEvidenceState"), evidenceState);
}

bool FleetRegistry::_synchronizeCapabilityReview(VehicleCapability& vehicle, CapabilityReviewState& state) const
{
    const QString identity = FleetCapabilityEvidence::identityFingerprint(vehicle.toVariantMap());
    if (!isSha256Fingerprint(identity)) return false;
    if (state.vehicleRecordEpoch.isEmpty()) {
        state.vehicleRecordEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
        state.revision = 1; state.identityFingerprint = identity; state.approval.clear(); state.exhausted = false;
    } else if (state.identityFingerprint != identity) {
        state.identityFingerprint = identity;
        if (!state.approval.isEmpty() && state.approval.value(QStringLiteral("state")).toString() == QStringLiteral("current"))
            state.approval.insert(QStringLiteral("state"), QStringLiteral("superseded"));
        if (state.revision == std::numeric_limits<quint64>::max()) state.exhausted = true;
        else if (!state.exhausted) ++state.revision;
    }
    _projectCapabilityReview(vehicle, state);
    return true;
}

bool FleetRegistry::_advanceConfigurationHistory(ConfigurationHistory& history) const
{
    if (history.exhausted || history.revision == std::numeric_limits<quint64>::max()) {
        history.exhausted = true;
        history.comparison.clear();
        history.currentTypedFingerprint.clear(); history.baselineTypedFingerprint.clear();
        history.baselineApprovalHash.clear(); history.baselineMatches = false;
        return false;
    }
    ++history.revision;
    return true;
}

void FleetRegistry::_cacheConfigurationComparison(ConfigurationHistory& history) const
{
    if (history.exhausted) {
        history.comparison.clear();
        history.currentTypedFingerprint.clear(); history.baselineTypedFingerprint.clear();
        history.baselineApprovalHash.clear(); history.baselineMatches = false;
        return;
    }
    history.basis = history.baselineKnown ? QStringLiteral("local-approved-baseline")
        : history.previousKnown ? QStringLiteral("previous-different-observed") : QStringLiteral("unknown");
    const QVariantMap before = history.baselineKnown ? history.baseline : history.previousDifferent;
    history.comparison = FleetConfigurationDiff::compare(before, history.baselineKnown || history.previousKnown,
        history.lastFull, history.currentFullKnown);
    // These fingerprints belong to the original private typed values, not
    // their JSON/display form or an imported metadata claim. Cache only at
    // actual history mutation; ordinary reads/health packets do not rehash.
    history.currentTypedFingerprint = history.currentFullKnown
        ? history.comparison.value(QStringLiteral("afterTypedFingerprint")).toString() : QString();
    history.baselineTypedFingerprint = history.baselineKnown
        ? history.comparison.value(QStringLiteral("beforeTypedFingerprint")).toString() : QString();
    history.baselineApprovalHash = history.baselineKnown
        ? FleetConfigurationDiff::typedFingerprint(history.baselineApproval) : QString();
    history.baselineMatches = history.currentFullKnown && history.baselineKnown
        && !history.currentTypedFingerprint.isEmpty() && !history.baselineTypedFingerprint.isEmpty()
        && !history.baselineApprovalHash.isEmpty()
        && history.currentTypedFingerprint == history.baselineTypedFingerprint
        && FleetConfigurationDiff::exactEqual(history.lastFull, history.baseline);
}

bool FleetRegistry::_observeConfiguration(ConfigurationHistory& history, const VehicleCapability& vehicle,
    const CapabilityReviewState& review, const QVariantMap& update, const VehicleCapability& parsed) const
{
    const bool fresh = history.observationEpoch.isEmpty() || history.vehicleRecordEpoch != review.vehicleRecordEpoch;
    if (fresh) {
        history = ConfigurationHistory();
        history.vehicleRecordEpoch = review.vehicleRecordEpoch;
        history.observationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    const QVariantMap metadata = update.value(QStringLiteral("metadata")).toMap();
    const bool configurationPresent = update.contains(QStringLiteral("configuration"))
        || metadata.contains(QStringLiteral("configuration"));
    const bool observationPresent = configurationPresent || update.contains(QStringLiteral("configurationFingerprint"))
        || update.value(QStringLiteral("configurationClear"), false).toBool();
    bool changed = fresh;
    bool continuityChanged = false;
    if (observationPresent && !history.exhausted) {
        // Inspect the actual input, never the partial-merge retained map. An
        // empty/cleared/hash-only input explicitly loses full-current proof.
        const bool full = configurationPresent && !parsed.configuration.isEmpty()
            && !update.value(QStringLiteral("configurationClear"), false).toBool()
            && FleetConfigurationDiff::valueValid(parsed.configuration);
        if (full) {
            const bool different = history.lastFullKnown
                && !FleetConfigurationDiff::exactEqual(history.lastFull, parsed.configuration);
            continuityChanged = different || !history.currentFullKnown;
            if (!history.lastFullKnown || different) {
                if (history.lastFullKnown) {
                    history.previousDifferent = history.lastFull;
                    history.previousFingerprint = history.lastFingerprint;
                    history.previousKnown = true;
                }
                history.lastFull = parsed.configuration;
                history.lastFingerprint = vehicle.configurationFingerprint;
                history.lastFullKnown = true;
                changed = true;
            }
            if (history.lastFingerprint != vehicle.configurationFingerprint) {
                // A legacy serialization may distinguish two values that the
                // typed policy regards as equivalent (e.g. DateTime zones).
                // Refresh the actual full observation without rotating the
                // previous DIFFERENT observation merely for presentation.
                history.lastFull = parsed.configuration;
                history.lastFingerprint = vehicle.configurationFingerprint;
                changed = true;
            }
            changed = changed || !history.currentFullKnown;
            history.currentFullKnown = true;
        } else {
            continuityChanged = history.currentFullKnown;
            changed = changed || history.currentFullKnown;
            history.currentFullKnown = false;
        }
        changed = changed || history.reportedFingerprint != vehicle.configurationFingerprint;
        history.reportedFingerprint = vehicle.configurationFingerprint;
    }
    if (changed) {
        if (!fresh) _advanceConfigurationHistory(history);
        _cacheConfigurationComparison(history);
    }
    return continuityChanged;
}

bool FleetRegistry::_configurationHistoryCurrent(int systemId, const ConfigurationHistory& expected) const
{
    const auto actual = _configurationHistories.constFind(systemId);
    return actual != _configurationHistories.constEnd()
        && actual->vehicleRecordEpoch == expected.vehicleRecordEpoch
        && actual->observationEpoch == expected.observationEpoch && actual->revision == expected.revision
        && actual->exhausted == expected.exhausted;
}

void FleetRegistry::_removeCapabilityEcho(const VehicleCapability& vehicle, QVariantMap* updates) const
{
    if (!updates) return;
    const auto state = _capabilityReviews.constFind(vehicle.systemId);
    if (state == _capabilityReviews.constEnd()) return;
    const QVariantMap context = _capabilityContext(vehicle, *state);
    const QVariantMap approval = state->approval;
    QStringList effective;
    FleetCapabilityEvidence::currentCapabilities(vehicle.toVariantMap(), &effective);
    const auto removeOwned = [&](QVariantMap& source) {
        const QVariant rawContext = source.value(QStringLiteral("capabilityReviewContext"));
        const QVariant rawApproval = source.value(QStringLiteral("capabilityApproval"));
        QVariantMap repeatedContext = rawContext.toMap(), repeatedApproval = rawApproval.toMap();
        QStringList repeated;
        if (rawContext.type() != QVariant::Map || rawApproval.type() != QVariant::Map
            || !source.contains(QStringLiteral("verifiedCapabilities"))
            || (!repeatedContext.isEmpty() && !FleetCapabilityEvidence::contextValid(repeatedContext))
            || (!repeatedApproval.isEmpty() && !FleetCapabilityEvidence::approvalValid(repeatedApproval))
            || !FleetCapabilityEvidence::canonicalCapabilities(source.value(QStringLiteral("verifiedCapabilities")), &repeated)
            || repeated != effective) return;
        // QML containers may return integral IDs/schema as Double and caps as
        // QVariantList. Normalize only those already-validated value fields;
        // UUIDs, revisions, hashes and every other record field remain exact.
        if (!repeatedContext.isEmpty()) repeatedContext.insert(QStringLiteral("vehicleId"), repeatedContext.value(QStringLiteral("vehicleId")).toInt());
        if (!repeatedApproval.isEmpty()) {
            QStringList approvalCapabilities;
            if (!FleetCapabilityEvidence::canonicalCapabilities(repeatedApproval.value(QStringLiteral("capabilities")), &approvalCapabilities)) return;
            repeatedApproval.insert(QStringLiteral("schemaVersion"), repeatedApproval.value(QStringLiteral("schemaVersion")).toInt());
            repeatedApproval.insert(QStringLiteral("capabilities"), approvalCapabilities);
        }
        if (repeatedContext != context || repeatedApproval != approval) return;
        // This is a repetition of THIS actual record's readonly projection,
        // not a new capability declaration. Other incoming claims still go
        // through the ordinary declaration-only path below.
        source.remove(QStringLiteral("capabilityReviewContext"));
        source.remove(QStringLiteral("capabilityApproval"));
        source.remove(QStringLiteral("verifiedCapabilities"));
    };
    removeOwned(*updates);
    if (updates->value(QStringLiteral("metadata")).type() == QVariant::Map) {
        QVariantMap metadata = updates->value(QStringLiteral("metadata")).toMap();
        removeOwned(metadata);
        updates->insert(QStringLiteral("metadata"), metadata);
    }
}

bool FleetRegistry::_mergeDescriptor(VehicleCapability& liveTarget, const QVariantMap& updates,
                                     bool allowTrustedBaseline)
{
    if (thread() != QThread::currentThread()) return false;
    // Stage the entire merge. Repeated individually bounded partial updates
    // must not grow retained metadata past the budget or partially change
    // health/approval state when the combined descriptor is rejected.
    VehicleCapability target = liveTarget;
    QVariantMap incoming = updates;
    if (!allowTrustedBaseline) _removeCapabilityEcho(liveTarget, &incoming);
    QVariantMap normalized;
    if (!capabilityIngress(incoming, &normalized)) return false;
    if (!allowTrustedBaseline) {
        // Baseline claims are not accepted from ordinary telemetry. Strip
        // them before validation and parsing, preserving the established API
        // contract that this path cannot change local approval state.
        normalized.remove(QStringLiteral("baselineConfigurationFingerprint"));
        normalized.remove(QStringLiteral("configurationBaseline"));
        normalized.remove(QStringLiteral("baselineSource"));
    }
    if (!validateIngressDescriptor(normalized, target.systemId, allowTrustedBaseline)) {
        return false;
    }

    // Resolve contradictory configuration operations before touching any
    // runtime fields. This keeps the rejected update atomic even when the
    // same map also carries health telemetry.
    const QVariantMap preflightMetadata = normalized.value(QStringLiteral("metadata")).toMap();
    const bool preflightHasConfiguration = normalized.contains(QStringLiteral("configuration"))
        || preflightMetadata.contains(QStringLiteral("configuration"));
    const bool preflightHasObservedFingerprint = normalized.contains(
        QStringLiteral("configurationFingerprint"));
    const bool preflightClearConfiguration = normalized.value(
        QStringLiteral("configurationClear"), false).toBool();
    if (preflightClearConfiguration
        && (preflightHasConfiguration || preflightHasObservedFingerprint)) {
        return false;
    }

    const bool wasOnline = target.online;
    const VehicleCapability parsed = VehicleCapability::fromVariantMap(normalized, target.systemId);
    const auto has = [&normalized](const QString& key) { return normalized.contains(key); };

    if (has(QStringLiteral("vehicleType")) || has(QStringLiteral("type"))) target.vehicleType = parsed.vehicleType;
    if (has(QStringLiteral("autopilotType")) || has(QStringLiteral("autopilot"))) target.autopilotType = parsed.autopilotType;
    if (has(QStringLiteral("firmwareVersion")) || has(QStringLiteral("firmware"))) target.firmwareVersion = parsed.firmwareVersion;
    if (has(QStringLiteral("capabilities"))) target.capabilities = parsed.capabilities;
    if (has(QStringLiteral("maxSpeedMps"))) target.maxSpeedMps = parsed.maxSpeedMps;
    if (has(QStringLiteral("enduranceSeconds")) || has(QStringLiteral("enduranceSec"))) target.enduranceSeconds = parsed.enduranceSeconds;
    if (has(QStringLiteral("canUseGnss")) || has(QStringLiteral("hasGnss"))) target.canUseGnss = parsed.canUseGnss;
    if (has(QStringLiteral("canUseRtk"))) target.canUseRtk = parsed.canUseRtk;
    if (has(QStringLiteral("canUseVision")) || has(QStringLiteral("hasCamera"))) target.canUseVision = parsed.canUseVision;
    if (has(QStringLiteral("canUseRangeSensor")) || has(QStringLiteral("hasRangeSensor"))) target.canUseRangeSensor = parsed.canUseRangeSensor;
    if (has(QStringLiteral("canRelay"))) target.canRelay = parsed.canRelay;
    if (has(QStringLiteral("role"))) target.role = parsed.role;
    if (has(QStringLiteral("online"))) target.online = parsed.online;
    if (has(QStringLiteral("lastSeenUtc")) || has(QStringLiteral("lastSeen"))) target.lastSeenUtc = parsed.lastSeenUtc;
    if (has(QStringLiteral("batteryPercent")) || has(QStringLiteral("battery"))) target.batteryPercent = parsed.batteryPercent;
    if (has(QStringLiteral("linkQualityPercent")) || has(QStringLiteral("linkQuality"))) target.linkQualityPercent = parsed.linkQualityPercent;
    if (has(QStringLiteral("estimatorHealthKnown")) || has(QStringLiteral("estimatorHealthy"))) {
        target.estimatorHealthy = parsed.estimatorHealthy;
        target.estimatorHealthKnown = has(QStringLiteral("estimatorHealthKnown"))
            ? parsed.estimatorHealthKnown
            : true;
    }
    if (has(QStringLiteral("positionHealthKnown")) || has(QStringLiteral("positionHealthy"))) {
        target.positionHealthy = parsed.positionHealthy;
        target.positionHealthKnown = has(QStringLiteral("positionHealthKnown"))
            ? parsed.positionHealthKnown
            : true;
    }
    if (has(QStringLiteral("hasFault"))) target.hasFault = parsed.hasFault;
    if (has(QStringLiteral("faultSummary"))) target.faultSummary = parsed.faultSummary;
    if (has(QStringLiteral("faultHealthKnown"))) {
        target.faultHealthKnown = parsed.faultHealthKnown;
        if (target.faultHealthKnown && has(QStringLiteral("faultEvidenceAgeMs"))) {
            target.faultEvidenceObservedUtc = parsed.lastSeenUtc.isValid()
                ? parsed.lastSeenUtc : QDateTime::currentDateTimeUtc();
        } else if (!target.faultHealthKnown) {
            target.faultEvidenceObservedUtc = QDateTime();
        }
        if (!target.faultHealthKnown && !has(QStringLiteral("faultEvidenceAgeMs"))) {
            // Do not leave an old age attached to an explicitly cleared
            // assessment.  It is not safety-significant while known=false,
            // but retaining it makes later partial merges ambiguous.
            target.faultEvidenceAgeMs = -1;
        }
    }
    if (has(QStringLiteral("faultEvidenceAgeMs"))) target.faultEvidenceAgeMs = parsed.faultEvidenceAgeMs;

    if (!wasOnline && target.online) {
        // A bare reconnect heartbeat is proof of liveness, not proof that the
        // pre-disconnect health samples are still current. Clear each missing
        // evidence channel independently; retain an explicit hasFault=true
        // assertion conservatively until a fresh fault assessment clears it.
        if (!has(QStringLiteral("batteryPercent")) && !has(QStringLiteral("battery"))) {
            target.batteryPercent = -1.0;
        }
        if (!has(QStringLiteral("linkQualityPercent")) && !has(QStringLiteral("linkQuality"))) {
            target.linkQualityPercent = -1;
        }
        if (!has(QStringLiteral("estimatorHealthKnown"))
            && !has(QStringLiteral("estimatorHealthy"))) {
            target.estimatorHealthKnown = false;
        }
        if (!has(QStringLiteral("positionHealthKnown"))
            && !has(QStringLiteral("positionHealthy"))) {
            target.positionHealthKnown = false;
        }
        if (!has(QStringLiteral("faultHealthKnown"))
            && !has(QStringLiteral("faultEvidenceAgeMs"))) {
            target.faultHealthKnown = false;
            target.faultEvidenceAgeMs = -1;
            target.faultEvidenceObservedUtc = QDateTime();
        }
    }
    const QVariantMap metadataUpdate = normalized.value(QStringLiteral("metadata")).toMap();
    const bool hasMetadataConfiguration = metadataUpdate.contains(QStringLiteral("configuration"));
    // VehicleCapability accepts metadata.configuration as a legacy/transport
    // alias. Treat it as real configuration evidence here as well; otherwise
    // a modified nested configuration would be stored only as metadata while
    // the old approved fingerprint remained visible.
    const bool hasConfiguration = has(QStringLiteral("configuration"))
        || hasMetadataConfiguration;
    const bool hasObservedFingerprint = has(QStringLiteral("configurationFingerprint"));
    const bool hasBaselineSource = has(QStringLiteral("baselineSource"));
    const bool trustedBaseline = allowTrustedBaseline && hasBaselineSource
        && (parsed.baselineSource == QStringLiteral("trusted_manifest")
            || parsed.baselineSource == QStringLiteral("local_approval"));
    const bool clearBaseline = allowTrustedBaseline && hasBaselineSource
        && parsed.baselineSource.isEmpty();
    const bool hasBaseline = trustedBaseline || clearBaseline;
    const bool clearConfiguration = preflightClearConfiguration;
    // An explicit clear is itself configuration evidence.  It must work as a
    // standalone update even when the transport does not repeat the previous
    // configuration map; otherwise stale configuration/baseline state would
    // survive a deliberate "configurationClear=true" update.
    const bool hasConfigurationEvidence = clearConfiguration
        || (hasConfiguration && !parsed.configuration.isEmpty());
    // A standalone observed fingerprint is useful telemetry evidence even
    // when the full map is unavailable.  Accept only a well-formed hash, and
    // never treat it as an approved baseline; the baseline path above remains
    // explicitly trusted/local only.
    const bool hasValidObservedFingerprint = hasObservedFingerprint
        && isSha256Fingerprint(parsed.configurationFingerprint);
    if (hasConfigurationEvidence || hasValidObservedFingerprint || hasBaseline) {
        if (hasConfigurationEvidence) {
            if (clearConfiguration) {
                target.configuration.clear();
                target.configurationFingerprint.clear();
                target.metadata.remove(QStringLiteral("configuration"));
            } else {
                target.configuration = parsed.configuration;
            }
        }
        if ((hasConfigurationEvidence && !clearConfiguration)
            || hasValidObservedFingerprint) {
            // A hash-only observation may legitimately arrive without the
            // full map. If it differs from the retained map, discard that
            // stale map so the exported passport cannot contain contradictory
            // configuration and fingerprint fields.
            if (!hasConfigurationEvidence && hasValidObservedFingerprint
                && !target.configuration.isEmpty()
                && VehicleCapability::fingerprintForConfiguration(target.configuration)
                    != parsed.configurationFingerprint) {
                target.configuration.clear();
                target.metadata.remove(QStringLiteral("configuration"));
            }
            target.configurationFingerprint = parsed.configurationFingerprint;
        }
        if (hasBaseline) {
            target.baselineConfigurationFingerprint = parsed.baselineConfigurationFingerprint;
            target.baselineSource = parsed.baselineSource;
        }
        if (target.configurationFingerprint.isEmpty()) {
            target.configurationState = QStringLiteral("unknown");
        } else if (target.baselineConfigurationFingerprint.isEmpty()) {
            target.configurationState = QStringLiteral("unverified");
        } else {
            target.configurationState = target.configurationFingerprint
                    == target.baselineConfigurationFingerprint
                ? QStringLiteral("baseline")
                : QStringLiteral("modified");
        }
    }
    // The canonical struct owns these fields. Do not let a retained legacy
    // alias turn a legitimate partial change into a rejected update that
    // leaves the OLD approval effective. Only old aliases are removed here;
    // this update's explicitly supplied metadata is merged afterwards and
    // still has to agree with the canonical fields in the shared helper.
    for (const QString& key : {QStringLiteral("vehicleType"), QStringLiteral("autopilotType"), QStringLiteral("firmwareVersion"),
            QStringLiteral("configurationFingerprint"), QStringLiteral("baselineConfigurationFingerprint"),
            QStringLiteral("baselineSource"), QStringLiteral("configurationState"), QStringLiteral("maxSpeedMps"),
            QStringLiteral("enduranceSeconds"), QStringLiteral("canUseGnss"), QStringLiteral("canUseRtk"),
            QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"), QStringLiteral("canRelay"), QStringLiteral("capabilities")})
        target.metadata.remove(key);
    if (has(QStringLiteral("metadata"))) target.metadata.unite(parsed.metadata);
    if (has(QStringLiteral("capabilities")) && target.metadata.contains(QStringLiteral("capabilities")))
        target.metadata.insert(QStringLiteral("capabilities"), target.capabilities);
    if (hasConfigurationEvidence) {
        if (clearConfiguration) {
            target.metadata.remove(QStringLiteral("configuration"));
        } else {
            // Keep the legacy alias synchronized with the canonical map when
            // both forms are supplied in the same update.
            target.metadata.insert(QStringLiteral("configuration"), target.configuration);
        }
    }

    // Any unrecognised fields survive in metadata for future capability fields.
    static const QStringList knownKeys = {
        QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id"),
        QStringLiteral("vehicleType"), QStringLiteral("type"), QStringLiteral("autopilotType"),
        QStringLiteral("autopilot"), QStringLiteral("firmwareVersion"), QStringLiteral("firmware"),
        QStringLiteral("capabilities"), QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"),
        QStringLiteral("enduranceSec"), QStringLiteral("canUseGnss"), QStringLiteral("hasGnss"),
        QStringLiteral("canUseRtk"), QStringLiteral("canUseVision"), QStringLiteral("hasCamera"),
        QStringLiteral("canUseRangeSensor"), QStringLiteral("hasRangeSensor"), QStringLiteral("canRelay"),
        QStringLiteral("role"), QStringLiteral("online"), QStringLiteral("lastSeenUtc"),
        QStringLiteral("lastSeen"), QStringLiteral("batteryPercent"), QStringLiteral("battery"),
        QStringLiteral("linkQualityPercent"), QStringLiteral("linkQuality"), QStringLiteral("estimatorHealthy"),
        QStringLiteral("positionHealthy"), QStringLiteral("estimatorHealthKnown"),
        QStringLiteral("positionHealthKnown"), QStringLiteral("hasFault"), QStringLiteral("faultSummary"),
        QStringLiteral("faultHealthKnown"), QStringLiteral("faultEvidenceAgeMs"),
        QStringLiteral("configuration"), QStringLiteral("configurationFingerprint"),
        QStringLiteral("configurationBaseline"), QStringLiteral("baselineConfigurationFingerprint"),
        QStringLiteral("configurationState"), QStringLiteral("configurationClear"),
        QStringLiteral("baselineSource"),
        QStringLiteral("metadata")
    };
    for (auto iterator = normalized.constBegin(); iterator != normalized.constEnd(); ++iterator) {
        if (!knownKeys.contains(iterator.key())) {
            target.metadata.insert(iterator.key(), iterator.value());
        }
    }
    CapabilityReviewState review = _capabilityReviews.value(target.systemId);
    const CapabilityReviewState previousReview = review;
    if (!_synchronizeCapabilityReview(target, review)) return false;
    ConfigurationHistory history = _configurationHistories.value(target.systemId);
    const bool configurationContinuityChanged = _observeConfiguration(history, target, review, normalized, parsed);
    // The legacy JSON hash intentionally remains compatible. A typed-only
    // change (for example Int -> Double), or loss/recovery of a full observation,
    // must supersede native capability review once even when the old hash and
    // identity remain unchanged. Recovery cannot revive the former approval.
    if (configurationContinuityChanged && previousReview.vehicleRecordEpoch == review.vehicleRecordEpoch
        && previousReview.revision == review.revision && previousReview.exhausted == review.exhausted) {
        if (!review.approval.isEmpty() && review.approval.value(QStringLiteral("state")).toString() == QStringLiteral("current"))
            review.approval.insert(QStringLiteral("state"), QStringLiteral("superseded"));
        if (review.revision == std::numeric_limits<quint64>::max()) review.exhausted = true;
        else if (!review.exhausted) ++review.revision;
        _projectCapabilityReview(target, review);
    }
    if (history.exhausted && !review.exhausted) {
        review.exhausted = true;
        if (review.approval.value(QStringLiteral("state")).toString() == QStringLiteral("current"))
            review.approval.insert(QStringLiteral("state"), QStringLiteral("superseded"));
        _projectCapabilityReview(target, review);
    }
    int mergedNodes = 0;
    qint64 mergedBytes = 0;
    if (!boundedDescriptorValue(target.toVariantMap(), 0, &mergedNodes, &mergedBytes)) {
        return false;
    }
    liveTarget = target;
    _capabilityReviews.insert(target.systemId, review);
    _configurationHistories.insert(target.systemId, history);
    return true;
}

bool FleetRegistry::registerVehicle(const QVariantMap& descriptor)
{
    return _registerVehicle(descriptor, false);
}

bool FleetRegistry::_registerVehicle(const QVariantMap& descriptor, bool allowTrustedBaseline)
{
    if (thread() != QThread::currentThread()) return false;
    QVariantMap incoming = withoutConfigurationBaselineEvidence(descriptor);
    int inputNodes = 0; qint64 inputBytes = 0;
    if (!boundedDescriptorValue(incoming, 0, &inputNodes, &inputBytes)) return false;
    if (!allowTrustedBaseline) {
        const int possibleId = VehicleCapability::fromVariantMap(descriptor).systemId;
        const auto existing = _vehicles.constFind(possibleId);
        if (existing != _vehicles.constEnd()) _removeCapabilityEcho(*existing, &incoming);
    }
    QVariantMap normalized;
    if (!capabilityIngress(incoming, &normalized)) return false;
    if (!allowTrustedBaseline) {
        // Baseline claims are not accepted from the normal discovery/telemetry
        // path. They must come from the explicit passport import or local
        // approval API below.
        normalized.remove(QStringLiteral("baselineConfigurationFingerprint"));
        normalized.remove(QStringLiteral("configurationBaseline"));
        normalized.remove(QStringLiteral("baselineSource"));
    }
    if (!validateIngressDescriptor(normalized, 0, allowTrustedBaseline)) {
        return false;
    }
    const QVariantMap preflightMetadata = normalized.value(QStringLiteral("metadata")).toMap();
    const bool preflightHasConfiguration = normalized.contains(QStringLiteral("configuration"))
        || preflightMetadata.contains(QStringLiteral("configuration"));
    const bool preflightHasObservedFingerprint = normalized.contains(
        QStringLiteral("configurationFingerprint"));
    const bool preflightClearConfiguration = normalized.value(
        QStringLiteral("configurationClear"), false).toBool();
    if (preflightClearConfiguration
        && (preflightHasConfiguration || preflightHasObservedFingerprint)) {
        return false;
    }
    const VehicleCapability parsed = VehicleCapability::fromVariantMap(normalized);
    if (parsed.systemId <= 0 || parsed.systemId > 255) {
        return false;
    }
    int storedNodes = 0;
    qint64 storedBytes = 0;
    if (!boundedDescriptorValue(parsed.toVariantMap(), 0, &storedNodes, &storedBytes)) {
        return false;
    }

    const bool existed = _vehicles.contains(parsed.systemId);
    VehicleCapability target = _vehicles.value(parsed.systemId);
    const QString oldHealthState = target.healthState;
    const int oldHealthScore = target.healthScore;
    if (!existed) {
        target = parsed;
        // A historical last-seen timestamp is evidence of observation, not
        // proof that the vehicle is online now.  Preserve an explicit
        // offline snapshot instead of promoting it to online merely because
        // it contains a valid old timestamp.  Only synthesize a heartbeat
        // time for a descriptor that explicitly declares itself online.
        if (target.online && !target.lastSeenUtc.isValid()) {
            target.lastSeenUtc = QDateTime::currentDateTimeUtc();
        }
        if (target.faultHealthKnown && target.faultEvidenceAgeMs >= 0) {
            target.faultEvidenceObservedUtc = target.lastSeenUtc.isValid()
                ? target.lastSeenUtc : QDateTime::currentDateTimeUtc();
        }
        CapabilityReviewState review;
        if (!_synchronizeCapabilityReview(target, review)) return false;
        ConfigurationHistory history;
        _observeConfiguration(history, target, review, normalized, parsed);
        int nodes = 0; qint64 bytes = 0;
        if (!boundedDescriptorValue(target.toVariantMap(), 0, &nodes, &bytes)) return false;
        _capabilityReviews.insert(parsed.systemId, review);
        _configurationHistories.insert(parsed.systemId, history);
    } else {
        if (!_mergeDescriptor(target, normalized, allowTrustedBaseline)) {
            return false;
        }
    }
    _recomputeHealth(target, QDateTime::currentDateTimeUtc(), false);
    _vehicles.insert(parsed.systemId, target);
    const QPointer<FleetRegistry> self(this);
    if (existed) emit vehicleUpdated(parsed.systemId); else emit vehicleAdded(parsed.systemId);
    if (!self) return true;
    if (oldHealthState != target.healthState || oldHealthScore != target.healthScore)
        emit vehicleHealthChanged(parsed.systemId, target.healthState, target.healthScore);
    if (!self) return true;
    emit registryChanged();
    if (!self) return true;
    emit healthChanged();
    return true;
}

bool FleetRegistry::updateVehicle(int systemId, const QVariantMap& updates)
{
    if (thread() != QThread::currentThread()) return false;
    VehicleCapability* target = _find(systemId);
    if (!target || systemId <= 0 || systemId > 255) {
        return false;
    }
    VehicleCapability staged = *target;
    const QString oldHealthState = staged.healthState;
    const int oldHealthScore = staged.healthScore;
    if (!_mergeDescriptor(staged, updates, false)) {
        return false;
    }
    _recomputeHealth(staged, QDateTime::currentDateTimeUtc(), false);
    _vehicles.insert(systemId, staged);
    const QPointer<FleetRegistry> self(this);
    emit vehicleUpdated(systemId);
    if (!self) return true;
    if (oldHealthState != staged.healthState || oldHealthScore != staged.healthScore)
        emit vehicleHealthChanged(systemId, staged.healthState, staged.healthScore);
    if (!self) return true;
    emit registryChanged();
    if (!self) return true;
    emit healthChanged();
    return true;
}

bool FleetRegistry::unregisterVehicle(int systemId)
{
    if (thread() != QThread::currentThread()) return false;
    if (_vehicles.remove(systemId) == 0) {
        return false;
    }
    _configurationApprovals.remove(systemId);
    _capabilityReviews.remove(systemId);
    _configurationHistories.remove(systemId);
    _nativeBatterySources.remove(systemId);
    _nativeBatteryBindingsEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QPointer<FleetRegistry> self(this);
    _retireNativeConfigurationSource(systemId);
    if (!self) return true;
    emit vehicleRemoved(systemId);
    if (!self) return true;
    emit registryChanged();
    if (!self) return true;
    emit healthChanged();
    return true;
}

bool FleetRegistry::containsVehicle(int systemId) const
{
    return _vehicles.contains(systemId);
}

QVariantMap FleetRegistry::vehicle(int systemId) const
{
    if (thread() != QThread::currentThread()) return {};
    return _vehicleSnapshot(systemId);
}

QVariantMap FleetRegistry::optionalTelemetryRateReview(int systemId, const QVariant& requestedIntervalUs) const
{
    const auto failure = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("error"), reason},
            {QStringLiteral("review"), QVariantMap()}};
    };
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255)
        return failure(QStringLiteral("optional-telemetry-review-invalid-thread-or-vehicle"));
    bool proposal = false;
    int target = 0;
    if (requestedIntervalUs.isValid() && !requestedIntervalUs.isNull()) {
        if (!optionalTelemetryInteger(requestedIntervalUs, 0, std::numeric_limits<int>::max()))
            return failure(QStringLiteral("optional-telemetry-review-positive-integer-interval-required"));
        target = requestedIntervalUs.toInt(); proposal = target > 0;
        if (proposal && double(float(target)) != double(target))
            return failure(QStringLiteral("optional-telemetry-review-interval-not-exact-REAL32"));
    }
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const NativeBatterySource binding = _nativeBatterySources.value(systemId);
    const QString recordEpoch = _capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> source = binding.vehicle;
    if (!source || recordEpoch.isEmpty() || binding.vehicleRecordEpoch != recordEpoch
        || source->thread() != thread() || source->id() != systemId
        || !_nativeBatteryBindingCurrent(systemId, recordEpoch, binding))
        return failure(QStringLiteral("optional-telemetry-review-native-source-unbound"));
    // Read THIS actual bound Vehicle once. Metadata, imported passports and
    // portable caller observations do not participate in this baseline.
    const QVariantMap observation = source->optionalTelemetryObservations(30000);
    if (!self || !source || !_nativeBatteryBindingCurrent(systemId, recordEpoch, binding))
        return failure(QStringLiteral("optional-telemetry-review-native-source-changed"));
    if (!optionalTelemetryObservationValid(observation, systemId))
        return failure(QStringLiteral("optional-telemetry-review-native-observation-invalid"));
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    QVariantList comparisons;
    QStringList reasons;
    bool allKnown = proposal && current, allNonIncreasing = proposal && current;
    if (!current) reasons.append(QStringLiteral("optional-telemetry-source-unavailable"));
    if (!proposal) reasons.append(QStringLiteral("optional-telemetry-no-interval-proposal"));
    for (const QVariant& value : observation.value(QStringLiteral("rows")).toList()) {
        const QVariantMap row = value.toMap();
        const QVariantMap reported = row.value(QStringLiteral("reported")).toMap();
        const int baseline = reported.value(QStringLiteral("intervalUs")).toInt();
        const bool known = current && reported.value(QStringLiteral("observed")).toBool()
            && reported.value(QStringLiteral("available")).toBool() && reported.value(QStringLiteral("fresh")).toBool()
            && baseline > 0;
        const bool nonIncreasing = proposal && known && target >= baseline;
        QString reason = !known ? QStringLiteral("positive-current-reported-baseline-unavailable")
            : !proposal ? QStringLiteral("no-interval-proposal")
            : nonIncreasing ? QStringLiteral("reported-rate-not-increased-numerically") : QStringLiteral("proposal-would-increase-reported-rate");
        comparisons.append(QVariantMap{{QStringLiteral("messageId"), row.value(QStringLiteral("messageId"))},
            {QStringLiteral("baselineKnown"), known},
            {QStringLiteral("baselineUs"), known ? QVariant(baseline) : QVariant::fromValue(nullptr)},
            {QStringLiteral("baselineExactlyEncodable"), known && double(float(baseline)) == double(baseline)},
            {QStringLiteral("nonIncreasingReportedRate"), nonIncreasing}, {QStringLiteral("reason"), reason}});
        allKnown = allKnown && known; allNonIncreasing = allNonIncreasing && nonIncreasing;
        if (!known && !reasons.contains(QStringLiteral("optional-telemetry-positive-baselines-incomplete")))
            reasons.append(QStringLiteral("optional-telemetry-positive-baselines-incomplete"));
        if (proposal && known && !nonIncreasing && !reasons.contains(QStringLiteral("optional-telemetry-proposal-increases-rate")))
            reasons.append(QStringLiteral("optional-telemetry-proposal-increases-rate"));
    }
    const QVariantMap review{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("reportType"), QStringLiteral("fleet-optional-telemetry-rate-review")},
        {QStringLiteral("registryRecordEpoch"), recordEpoch}, {QStringLiteral("bindingEpoch"), binding.bindingEpoch},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("ledgerEpoch"), observation.value(QStringLiteral("ledgerEpoch"))},
        {QStringLiteral("versionEpoch"), observation.value(QStringLiteral("versionEpoch"))}, {QStringLiteral("sourceCurrent"), current},
        {QStringLiteral("requestedIntervalUs"), proposal ? QVariant(target) : QVariant::fromValue(nullptr)},
        {QStringLiteral("proposalProvided"), proposal}, {QStringLiteral("observation"), observation},
        {QStringLiteral("comparisons"), comparisons}, {QStringLiteral("numericComparisonComplete"), allKnown},
        {QStringLiteral("nonIncreasingReportedRate"), allNonIncreasing}, {QStringLiteral("reasonCodes"), reasons},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("rateAppliedProven"), false}, {QStringLiteral("wireCausalityProven"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    // None of these numerical/value conclusions can authorize a later write.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("review"), review}};
}

QVariantMap FleetRegistry::nativeLocalReceptionReview(int systemId) const
{
    const auto failure = [systemId](const QString& error) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("error"), ownedPerformanceText(error)},
            {QStringLiteral("vehicleId"), systemId >= 1 && systemId <= 255 ? systemId : 0},
            {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")},
            {QStringLiteral("observation"), QVariantMap()}, {QStringLiteral("snapshotOnly"), true}};
    };
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255)
        return failure(QStringLiteral("local-reception-review-invalid-thread-or-vehicle"));
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const NativeBatterySource binding = _nativeBatterySources.value(systemId);
    const QString recordEpoch = _capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> source(binding.vehicle);
    const auto same = [&]() {
        return self && source && source->thread() == thread() && source->id() == systemId
            && !recordEpoch.isEmpty() && binding.vehicleRecordEpoch == recordEpoch && !binding.bindingEpoch.isEmpty()
            && _nativeBatteryBindingCurrent(systemId, recordEpoch, binding);
    };
    if (!same()) return failure(QStringLiteral("local-reception-review-native-source-unbound"));
    // One getter on THIS actual native binding. No metadata/imported map or
    // caller-supplied observation is eligible to establish a local window.
    const QVariantMap observation = source->fleetLocalReceptionObservation();
    if (!same()) return failure(QStringLiteral("local-reception-review-native-source-changed"));
    if (!localReceptionObservationValid(observation, systemId))
        return failure(QStringLiteral("local-reception-review-native-observation-invalid"));
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("vehicleRecordEpoch"), ownedPerformanceText(recordEpoch)},
        {QStringLiteral("bindingEpoch"), ownedPerformanceText(binding.bindingEpoch)},
        {QStringLiteral("observation"), ownedPerformanceValue(observation)}, {QStringLiteral("snapshotOnly"), true}};
    if (!same()) return failure(QStringLiteral("local-reception-review-native-source-changed"));
    // ok is only a successful native read/shape check, not sourceCurrent,
    // completeness, network capacity, authentication or execution permission.
    return result;
}

QVariantMap FleetRegistry::nativeCameraStreamReview(int systemId) const
{
    const auto failure = [systemId](const QString& error) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("error"), ownedPerformanceText(error)},
            {QStringLiteral("vehicleId"), systemId >= 1 && systemId <= 255 ? systemId : 0},
            {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")},
            {QStringLiteral("observation"), QVariantMap()}, {QStringLiteral("snapshotOnly"), true}};
    };
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255)
        return failure(QStringLiteral("camera-stream-review-invalid-thread-or-vehicle"));
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const NativeBatterySource binding = _nativeBatterySources.value(systemId);
    const QString recordEpoch = _capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> source(binding.vehicle);
    const auto same = [&]() {
        return self && source && source->thread() == thread() && source->id() == systemId
            && !recordEpoch.isEmpty() && binding.vehicleRecordEpoch == recordEpoch && !binding.bindingEpoch.isEmpty()
            && _nativeBatteryBindingCurrent(systemId, recordEpoch, binding);
    };
    if (!same()) return failure(QStringLiteral("camera-stream-review-native-source-unbound"));
    // Read this actual bound Vehicle exactly once. The passive native ledger,
    // not adjusted CameraManager caches or imported maps, supplies all rows.
    const QVariantMap observation = source->fleetCameraStreamObservation(3000);
    if (!same()) return failure(QStringLiteral("camera-stream-review-native-source-changed"));
    if (!cameraStreamObservationValid(observation, systemId))
        return failure(QStringLiteral("camera-stream-review-native-observation-invalid"));
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("vehicleRecordEpoch"), ownedPerformanceText(recordEpoch)},
        {QStringLiteral("bindingEpoch"), ownedPerformanceText(binding.bindingEpoch)},
        {QStringLiteral("observation"), ownedPerformanceValue(observation)}, {QStringLiteral("snapshotOnly"), true}};
    if (!same()) return failure(QStringLiteral("camera-stream-review-native-source-changed"));
    // ok records only a successful bound read/closed native value check.
    // Historical, saturated and individually stale reports grant no control.
    return result;
}

QVariantList FleetRegistry::onlineVehicles() const
{
    if (thread() != QThread::currentThread()) return {};
    QVariantList result;
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QVariantList snapshots = vehicles();
    if (!self) return {};
    for (const QVariant& value : snapshots) {
        if (value.toMap().value(QStringLiteral("online")).toBool()) {
            result.append(value);
        }
    }
    return result;
}

QVariantList FleetRegistry::vehiclesWithCapability(const QString& capability,
                                                   bool requireVerified) const
{
    if (thread() != QThread::currentThread()) return {};
    QVariantList result;
    const QString requested = capability.trimmed();
    if (requested.isEmpty()) {
        return result;
    }
    const QPointer<FleetRegistry> self(const_cast<FleetRegistry*>(this));
    const QVariantList snapshots = vehicles();
    if (!self) return {};
    for (const QVariant& value : snapshots) {
        const QVariantMap vehicle = value.toMap();
        QStringList available = vehicle.value(QStringLiteral("capabilities")).toStringList();
        QStringList reviewed;
        const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewed);
        if (requireVerified) {
            if (!reviewCurrent) continue;
            available = reviewed;
        } else if (reviewCurrent) for (const QString& token : reviewed)
            if (!available.contains(token, Qt::CaseInsensitive)) available.append(token);
        for (const QString& advertised : available) {
            if (advertised.trimmed().compare(requested, Qt::CaseInsensitive) == 0) {
                result.append(vehicle);
                break;
            }
        }
    }
    return result;
}

void FleetRegistry::markSeen(int systemId, const QVariantMap& telemetry)
{
    if (thread() != QThread::currentThread()) return;
    VehicleCapability* target = _find(systemId);
    if (!target) {
        QVariantMap descriptor = telemetry;
        descriptor.insert(QStringLiteral("systemId"), systemId);
        descriptor.insert(QStringLiteral("online"), true);
        descriptor.insert(QStringLiteral("lastSeenUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        registerVehicle(descriptor);
        return;
    }

    const QString oldState = target->healthState;
    const int oldScore = target->healthScore;
    const bool oldOnline = target->online;
    VehicleCapability staged = *target;
    QVariantMap updates = telemetry;
    updates.insert(QStringLiteral("online"), true);
    updates.insert(QStringLiteral("lastSeenUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!_mergeDescriptor(staged, updates, false)) {
        // markSeen() is intentionally void for API compatibility.  Invalid
        // telemetry must nevertheless be ignored atomically, rather than
        // updating the heartbeat/health timestamps around a partial merge.
        return;
    }
    _recomputeHealth(staged, QDateTime::currentDateTimeUtc(), false);
    _vehicles.insert(systemId, staged);

    // Heartbeats are expected once per second.  Avoid forcing every QML
    // binding to rebuild on an unchanged heartbeat while still notifying
    // callers when health or connectivity actually changes.
    bool hasNonHeartbeatTelemetry = false;
    for (auto iterator = telemetry.constBegin(); iterator != telemetry.constEnd(); ++iterator) {
        if (iterator.key() != QStringLiteral("online") && iterator.key() != QStringLiteral("lastSeenUtc")
            && iterator.key() != QStringLiteral("lastSeen")) {
            hasNonHeartbeatTelemetry = true;
            break;
        }
    }
    const bool healthChangedNow = oldState != staged.healthState || oldScore != staged.healthScore;
    const bool connectionChanged = oldOnline != staged.online;
    const QPointer<FleetRegistry> self(this);
    if (healthChangedNow) emit vehicleHealthChanged(systemId, staged.healthState, staged.healthScore);
    if (!self) return;
    if (hasNonHeartbeatTelemetry || connectionChanged) {
        emit vehicleUpdated(systemId);
        if (!self) return;
        emit registryChanged();
        if (!self) return;
    }
    if (healthChangedNow || connectionChanged) {
        emit healthChanged();
    }
}

QString FleetRegistry::configurationFingerprint(const QVariantMap& configuration) const
{
    return VehicleCapability::fingerprintForConfiguration(configuration);
}

QVariantMap FleetRegistry::capabilityReviewContext(int systemId) const
{
    if (thread() != QThread::currentThread()) return {};
    const auto vehicle = _vehicles.constFind(systemId);
    const auto review = _capabilityReviews.constFind(systemId);
    return vehicle != _vehicles.constEnd() && review != _capabilityReviews.constEnd()
        ? _capabilityContext(*vehicle, *review) : QVariantMap();
}

QVariantMap FleetRegistry::configurationComparison(int systemId) const
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255) return {};
    const auto vehicle = _vehicles.constFind(systemId);
    const auto review = _capabilityReviews.constFind(systemId);
    const auto history = _configurationHistories.constFind(systemId);
    if (vehicle == _vehicles.constEnd() || review == _capabilityReviews.constEnd()
        || history == _configurationHistories.constEnd() || history->exhausted || !history->revision
        || history->observationEpoch.isEmpty() || history->vehicleRecordEpoch != review->vehicleRecordEpoch
        || history->comparison.isEmpty()) return {};
    if (history->baselineKnown && (vehicle->baselineSource != QStringLiteral("local_approval")
        || vehicle->baselineConfigurationFingerprint != history->baselineFingerprint
        || !FleetConfigurationDiff::exactEqual(_configurationApprovals.value(systemId), history->baselineApproval))) return {};
    // The expensive typed diff is cached only at actual history mutations.
    // No source, plugin, QObject callback, imported value or wall clock query.
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("systemId"), systemId},
        {QStringLiteral("registryContext"), _capabilityContext(*vehicle, *review)},
        {QStringLiteral("observationEpoch"), history->observationEpoch},
        {QStringLiteral("comparisonRevision"), QString::number(history->revision)},
        {QStringLiteral("basis"), history->basis}, {QStringLiteral("localBaselineKnown"), history->baselineKnown},
        {QStringLiteral("previousObservationKnown"), history->previousKnown},
        {QStringLiteral("currentFullKnown"), history->currentFullKnown},
        {QStringLiteral("baselineConfigurationFingerprint"), vehicle->baselineConfigurationFingerprint},
        {QStringLiteral("previousConfigurationFingerprint"), history->previousFingerprint},
        {QStringLiteral("currentConfigurationFingerprint"), vehicle->configurationFingerprint},
        {QStringLiteral("comparison"), history->comparison},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

void FleetRegistry::_notifyCapabilityMutation(int systemId)
{
    const QPointer<FleetRegistry> self(this);
    emit vehicleUpdated(systemId);
    if (!self) return;
    emit registryChanged();
    if (!self) return;
    emit healthChanged();
}

bool FleetRegistry::_stagePerformanceDeclaration(int systemId, const QVariantMap& expectedContext,
    const QVariantMap& proposedPerformance, PerformanceDeclarationStage* output, QString* reason) const
{
    if (!output || !reason) return false;
    *output = PerformanceDeclarationStage(); reason->clear();
    const auto reject = [&](const QString& problem) { *reason = problem; return false; };
    if (thread() != QThread::currentThread()) return reject(QStringLiteral("performance-registry-owner-thread-required"));
    if (systemId < 1 || systemId > 255 || !FleetCapabilityEvidence::contextValid(expectedContext))
        return reject(QStringLiteral("performance-review-context-invalid"));
    PerformanceDeclarationStage stage;
    if (!canonicalPerformanceProposal(proposedPerformance, &stage.performance))
        return reject(QStringLiteral("performance-declaration-invalid"));
    const auto vehicle = _vehicles.constFind(systemId);
    const auto review = _capabilityReviews.constFind(systemId);
    const auto history = _configurationHistories.constFind(systemId);
    if (vehicle == _vehicles.constEnd() || review == _capabilityReviews.constEnd()
        || history == _configurationHistories.constEnd()) return reject(QStringLiteral("performance-review-record-unavailable"));
    const QVariantMap currentContext = _capabilityContext(*vehicle, *review);
    if (!FleetCapabilityEvidence::contextValid(currentContext) || currentContext != expectedContext)
        return reject(QStringLiteral("performance-review-context-conflict"));
    if (review->exhausted || history->exhausted || review->revision == std::numeric_limits<quint64>::max())
        return reject(QStringLiteral("performance-review-revision-exhausted"));
    if (!review->approval.isEmpty() && !FleetCapabilityEvidence::approvalValid(review->approval))
        return reject(QStringLiteral("performance-review-approval-invalid"));
    // Only actual stored memory is read here. _vehicleSnapshot would project
    // native battery/health and could enter callbacks; it is NOT a pure stage.
    QVariantMap snapshot = vehicle->toVariantMap();
    snapshot.insert(QStringLiteral("configurationBaselineEvidence"), _configurationBaselineEvidence(systemId));
    QVariantMap baseline;
    if (!FleetCapabilityEvidence::currentConfigurationBaseline(snapshot, &baseline)
        || baseline.value(QStringLiteral("registryContext")).toMap() != currentContext)
        return reject(QStringLiteral("performance-current-complete-baseline-required"));
    stage.originalVehicle = *vehicle; stage.originalReview = *review; stage.originalHistory = *history;
    stage.registryEpoch = ownedPerformanceText(_capabilityRegistryEpoch);
    stage.originalContext = ownedPerformanceContext(currentContext);
    stage.configurationApprovalPresent = _configurationApprovals.contains(systemId);
    stage.configurationApproval = _configurationApprovals.value(systemId);
    stage.originalRecordFingerprint = FleetConfigurationDiff::typedFingerprint(stage.originalVehicle.toVariantMap());
    stage.configurationApprovalFingerprint = FleetConfigurationDiff::typedFingerprint(stage.configurationApproval);
    stage.baselineFingerprint = FleetConfigurationDiff::typedFingerprint(stage.originalHistory.baseline);
    if (stage.originalRecordFingerprint.isEmpty() || stage.configurationApprovalFingerprint.isEmpty()
        || stage.baselineFingerprint.isEmpty()) return reject(QStringLiteral("performance-original-source-value-invalid"));
    stage.vehicle = stage.originalVehicle; stage.review = stage.originalReview;
    stage.vehicle.maxSpeedMps = stage.performance.value(QStringLiteral("maxSpeedMps")).toDouble();
    stage.vehicle.enduranceSeconds = stage.performance.value(QStringLiteral("enduranceSeconds")).toDouble();
    // This declaration REPLACES both canonical numeric fields. Retained
    // metadata aliases cannot contradict it or refill an explicit unknown.
    for (const QString& key : {QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"), QStringLiteral("enduranceSec")})
        stage.vehicle.metadata.remove(key);
    ++stage.review.revision; // Exactly one unpublished next incarnation.
    stage.review.identityFingerprint = FleetCapabilityEvidence::identityFingerprint(stage.vehicle.toVariantMap());
    if (!isSha256Fingerprint(stage.review.identityFingerprint))
        return reject(QStringLiteral("performance-proposed-identity-invalid"));
    stage.context = _capabilityContext(stage.vehicle, stage.review);
    if (!FleetCapabilityEvidence::contextValid(stage.context))
        return reject(QStringLiteral("performance-proposed-context-invalid"));
    *output = stage;
    return true;
}

QVariantMap FleetRegistry::previewPerformanceDeclaration(int systemId, const QVariantMap& expectedContext,
    const QVariantMap& proposedPerformance) const
{
    const auto response = [](bool ok, const QString& reason, const QVariantMap& performance, const QVariantMap& context) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
            {QStringLiteral("proposedPerformance"), performance}, {QStringLiteral("proposedContext"), context}};
    };
    // Validate the fixed small inputs before owning their text/containers.
    // No caller-held mutable alias reaches the returned new context.
    if (!FleetCapabilityEvidence::contextValid(expectedContext))
        return response(false, QStringLiteral("performance-review-context-invalid"), {}, {});
    QVariantMap performance;
    if (!canonicalPerformanceProposal(proposedPerformance, &performance))
        return response(false, QStringLiteral("performance-declaration-invalid"), {}, {});
    const QVariantMap expected = ownedPerformanceContext(expectedContext);
    PerformanceDeclarationStage stage;
    QString problem;
    if (!_stagePerformanceDeclaration(systemId, expected, performance, &stage, &problem))
        return response(false, problem, {}, {});
    return response(true, QStringLiteral(""), stage.performance, ownedPerformanceContext(stage.context));
}

QVariantMap FleetRegistry::approveDeclaredPerformance(int systemId, const QVariantMap& suppliedContext,
    const QVariantMap& suppliedPerformance, const QVariantMap& suppliedProposedContext,
    const QStringList& suppliedCapabilities, const QString& suppliedReason,
    const std::function<bool()>& finalCommitFence)
{
    QVariantMap receipt{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("action"), QStringLiteral("approve-performance")},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("mutationCommitted"), false},
        {QStringLiteral("previousContext"), QVariantMap()}, {QStringLiteral("context"), QVariantMap()},
        {QStringLiteral("approval"), QVariantMap()}, {QStringLiteral("effectiveCapabilities"), QStringList()},
        {QStringLiteral("stillCurrent"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const auto reject = [&](const QString& problem) {
        QVariantMap result = receipt; result.insert(QStringLiteral("reason"), problem); return result;
    };
    if (thread() != QThread::currentThread()) return reject(QStringLiteral("performance-registry-owner-thread-required"));
    if (!FleetCapabilityEvidence::contextValid(suppliedContext) || !FleetCapabilityEvidence::contextValid(suppliedProposedContext))
        return reject(QStringLiteral("performance-review-context-invalid"));
    QVariantMap performance;
    if (!canonicalPerformanceProposal(suppliedPerformance, &performance)) return reject(QStringLiteral("performance-declaration-invalid"));
    QStringList capabilities;
    if (!FleetCapabilityEvidence::canonicalCapabilities(suppliedCapabilities, &capabilities)
        || capabilities.isEmpty() || capabilities != suppliedCapabilities)
        return reject(QStringLiteral("performance-capabilities-invalid"));
    if (suppliedReason.isEmpty() || suppliedReason.size() > 1024 || suppliedReason != suppliedReason.trimmed()
        || QString::fromUtf8(suppliedReason.toUtf8()) != suppliedReason)
        return reject(QStringLiteral("performance-engineering-reason-invalid"));
    for (QChar character : suppliedReason)
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format)
            return reject(QStringLiteral("performance-engineering-reason-invalid"));
    const QVariantMap expected = ownedPerformanceContext(suppliedContext);
    const QVariantMap expectedProposed = ownedPerformanceContext(suppliedProposedContext);
    capabilities = ownedPerformanceValue(capabilities).toStringList();
    const QString reason = ownedPerformanceText(suppliedReason);
    if (!finalCommitFence) return reject(QStringLiteral("performance-final-commit-fence-required"));
    const QPointer<FleetRegistry> self(this);
    const std::function<bool()> ownedFence = finalCommitFence;
    if (!self) return reject(QStringLiteral("performance-registry-owner-destroyed"));
    PerformanceDeclarationStage stage;
    QString problem;
    if (!_stagePerformanceDeclaration(systemId, expected, performance, &stage, &problem)) return reject(problem);
    if (stage.context != expectedProposed) return reject(QStringLiteral("performance-proposed-context-conflict"));
    receipt.insert(QStringLiteral("previousContext"), ownedPerformanceContext(stage.originalContext));
    receipt.insert(QStringLiteral("context"), ownedPerformanceContext(stage.originalContext));
    receipt.insert(QStringLiteral("approval"), ownedPerformanceValue(stage.originalReview.approval));
    QStringList previousEffective;
    FleetCapabilityEvidence::currentCapabilities(stage.originalVehicle.toVariantMap(), &previousEffective);
    receipt.insert(QStringLiteral("effectiveCapabilities"), ownedPerformanceValue(previousEffective));
    stage.review.approval = {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("approvalId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("registryEpoch"), stage.registryEpoch},
        {QStringLiteral("vehicleRecordEpoch"), stage.review.vehicleRecordEpoch},
        {QStringLiteral("capabilityRevision"), QString::number(stage.review.revision)},
        {QStringLiteral("identityFingerprint"), stage.review.identityFingerprint},
        {QStringLiteral("configurationFingerprint"), stage.vehicle.configurationFingerprint},
        {QStringLiteral("capabilities"), capabilities}, {QStringLiteral("source"), QStringLiteral("local-engineering-attestation")},
        {QStringLiteral("reason"), reason}, {QStringLiteral("approvedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("state"), QStringLiteral("current")}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (!FleetCapabilityEvidence::approvalValid(stage.review.approval)) return reject(QStringLiteral("performance-approval-contract-invalid"));
    _projectCapabilityReview(stage.vehicle, stage.review);
    int nodes = 0; qint64 bytes = 0;
    if (!boundedDescriptorValue(stage.vehicle.toVariantMap(), 0, &nodes, &bytes))
        return reject(QStringLiteral("performance-review-descriptor-limit"));
    QStringList effective;
    if (_capabilityContext(stage.vehicle, stage.review) != expectedProposed
        || !FleetCapabilityEvidence::currentCapabilities(stage.vehicle.toVariantMap(), &effective) || effective != capabilities)
        return reject(QStringLiteral("performance-approval-projection-invalid"));
    const QString committedRecordFingerprint = FleetConfigurationDiff::typedFingerprint(stage.vehicle.toVariantMap());
    if (committedRecordFingerprint.isEmpty()) return reject(QStringLiteral("performance-committed-source-value-invalid"));
    // Freeze a wholly independent receipt BEFORE the callback/commit/public
    // notifications. A failure still returns the uncommitted receipt above.
    QVariantMap committedReceipt = receipt;
    committedReceipt.insert(QStringLiteral("ok"), true);
    committedReceipt.insert(QStringLiteral("mutationCommitted"), true);
    committedReceipt.insert(QStringLiteral("context"), stage.context);
    committedReceipt.insert(QStringLiteral("approval"), stage.review.approval);
    committedReceipt.insert(QStringLiteral("effectiveCapabilities"), effective);
    committedReceipt = ownedPerformanceValue(committedReceipt).toMap();
    const auto basisCurrent = [&]() {
        if (!self || !self->_configurationHistoryCurrent(systemId, stage.originalHistory)) return false;
        const auto history = self->_configurationHistories.constFind(systemId);
        return history != self->_configurationHistories.constEnd()
            && FleetConfigurationDiff::typedFingerprint(history->baseline) == stage.baselineFingerprint
            && FleetConfigurationDiff::typedFingerprint(self->_configurationApprovals.value(systemId)) == stage.configurationApprovalFingerprint;
    };
    const auto originalCurrent = [&]() {
        return self && self->_capabilityMutationStateCurrent(systemId, stage.registryEpoch, stage.originalVehicle,
                stage.originalReview, stage.configurationApprovalPresent, stage.configurationApproval)
            && FleetConfigurationDiff::typedFingerprint(self->_vehicles.value(systemId).toVariantMap()) == stage.originalRecordFingerprint
            && basisCurrent();
    };
    if (!originalCurrent()) return reject(QStringLiteral("performance-source-changed-before-fence"));
    const bool allowed = ownedFence();
    if (!originalCurrent()) return reject(QStringLiteral("performance-source-changed-before-commit"));
    if (!allowed) return reject(QStringLiteral("performance-final-commit-fence-rejected"));
    // One source CAS, then the complete numeric row AND new approval/revision
    // commit before any notification. Baseline/history/source bindings remain
    // the original values; do not invent a new full observation or reapprove.
    _capabilityReviews.insert(systemId, stage.review);
    _vehicles.insert(systemId, stage.vehicle);
    _notifyCapabilityMutation(systemId);
    committedReceipt.insert(QStringLiteral("stillCurrent"), self
        && self->_capabilityMutationStateCurrent(systemId, stage.registryEpoch, stage.vehicle, stage.review,
            stage.configurationApprovalPresent, stage.configurationApproval)
        && FleetConfigurationDiff::typedFingerprint(self->_vehicles.value(systemId).toVariantMap()) == committedRecordFingerprint
        && basisCurrent());
    return committedReceipt;
}

bool FleetRegistry::approveCurrentCapabilities(int systemId, const QVariantMap& suppliedContext,
    const QStringList& suppliedCapabilities, const QString& suppliedReason)
{
    return _mutateCapabilityApproval(systemId, suppliedContext, suppliedCapabilities, suppliedReason,
        false, std::function<bool()>(), false).value(QStringLiteral("ok")).toBool();
}

QVariantMap FleetRegistry::approveCurrentCapabilities(int systemId, const QVariantMap& expectedContext,
    const QStringList& capabilities, const QString& reason, const std::function<bool()>& finalCommitFence)
{
    return _mutateCapabilityApproval(systemId, expectedContext, capabilities, reason, false, finalCommitFence, true);
}

bool FleetRegistry::revokeCapabilityApproval(int systemId, const QVariantMap& suppliedContext, const QString& suppliedReason)
{
    // Legacy revocation reports true when the actual revocation committed,
    // including its original exhausted-revision conservative transition.
    return _mutateCapabilityApproval(systemId, suppliedContext, QStringList(), suppliedReason,
        true, std::function<bool()>(), false).value(QStringLiteral("mutationCommitted")).toBool();
}

QVariantMap FleetRegistry::revokeCapabilityApproval(int systemId, const QVariantMap& expectedContext,
    const QString& reason, const std::function<bool()>& finalCommitFence)
{
    return _mutateCapabilityApproval(systemId, expectedContext, QStringList(), reason, true, finalCommitFence, true);
}

bool FleetRegistry::_capabilityMutationStateCurrent(int systemId, const QString& registryEpoch,
    const VehicleCapability& vehicle, const CapabilityReviewState& review,
    bool configurationApprovalPresent, const QVariantMap& configurationApproval) const
{
    if (thread() != QThread::currentThread() || _capabilityRegistryEpoch != registryEpoch
        || _configurationApprovals.contains(systemId) != configurationApprovalPresent
        || _configurationApprovals.value(systemId) != configurationApproval) return false;
    const auto actualVehicle = _vehicles.constFind(systemId);
    const auto actualReview = _capabilityReviews.constFind(systemId);
    return actualVehicle != _vehicles.constEnd() && actualReview != _capabilityReviews.constEnd()
        && actualReview->vehicleRecordEpoch == review.vehicleRecordEpoch && actualReview->revision == review.revision
        && actualReview->identityFingerprint == review.identityFingerprint && actualReview->approval == review.approval
        && actualReview->exhausted == review.exhausted
        // The private fault anchor is not exposed by toVariantMap(). Do not
        // overwrite any changed runtime row with the pre-fence staged copy.
        && actualVehicle->faultEvidenceObservedUtc == vehicle.faultEvidenceObservedUtc
        && actualVehicle->lastSeenUtc == vehicle.lastSeenUtc
        && FleetConfigurationDiff::exactEqual(actualVehicle->toVariantMap(), vehicle.toVariantMap());
}

QVariantMap FleetRegistry::_mutateCapabilityApproval(int systemId, const QVariantMap& suppliedContext,
    const QStringList& suppliedCapabilities, const QString& suppliedReason, bool revoke,
    const std::function<bool()>& finalCommitFence, bool requireFence)
{
    // Own all caller inputs before a copied native callback can re-enter or
    // destroy either its owner or this registry. These maps never authorize.
    const QVariantMap expected = suppliedContext;
    const QString reason = suppliedReason.trimmed();
    const QStringList requestedCapabilities = suppliedCapabilities;
    QVariantMap receipt{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("reason"), QString()}, {QStringLiteral("action"), revoke ? QStringLiteral("revoke") : QStringLiteral("approve")},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("mutationCommitted"), false},
        {QStringLiteral("previousContext"), QVariantMap()}, {QStringLiteral("context"), QVariantMap()},
        {QStringLiteral("approval"), QVariantMap()}, {QStringLiteral("effectiveCapabilities"), QStringList()},
        {QStringLiteral("stillCurrent"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const auto reject = [&](const QString& problem) {
        QVariantMap rejected = receipt;
        rejected.insert(QStringLiteral("reason"), problem);
        return rejected;
    };
    if (thread() != QThread::currentThread()) return reject(QStringLiteral("capability-registry-owner-thread-required"));
    if (systemId < 1 || systemId > 255 || reason.isEmpty() || reason.size() > 1024
        || !FleetCapabilityEvidence::contextValid(expected)) return reject(QStringLiteral("capability-review-input-invalid"));
    if (requireFence && !finalCommitFence) return reject(QStringLiteral("capability-final-commit-fence-required"));
    QStringList capabilities;
    if (!revoke && (!FleetCapabilityEvidence::canonicalCapabilities(requestedCapabilities, &capabilities) || capabilities.isEmpty()))
        return reject(QStringLiteral("capability-review-capabilities-invalid"));
    const QVariantMap previousContext = capabilityReviewContext(systemId);
    if (previousContext != expected) return reject(QStringLiteral("capability-review-context-conflict"));
    VehicleCapability target = _vehicles.value(systemId);
    CapabilityReviewState review = _capabilityReviews.value(systemId);
    const VehicleCapability originalVehicle = target;
    const CapabilityReviewState originalReview = review;
    const ConfigurationHistory originalHistory = _configurationHistories.value(systemId);
    const QString originalRegistryEpoch = _capabilityRegistryEpoch;
    const bool configurationApprovalPresent = _configurationApprovals.contains(systemId);
    const QVariantMap configurationApproval = _configurationApprovals.value(systemId);
    receipt.insert(QStringLiteral("previousContext"), previousContext);
    receipt.insert(QStringLiteral("context"), previousContext);
    receipt.insert(QStringLiteral("approval"), review.approval);
    QStringList previousEffective;
    FleetCapabilityEvidence::currentCapabilities(target.toVariantMap(), &previousEffective);
    receipt.insert(QStringLiteral("effectiveCapabilities"), previousEffective);
    if (review.exhausted || originalHistory.exhausted) return reject(QStringLiteral("capability-review-revision-exhausted"));
    if (revoke && (!FleetCapabilityEvidence::approvalValid(review.approval)
        || review.approval.value(QStringLiteral("state")).toString() == QStringLiteral("revoked")))
        return reject(QStringLiteral("capability-approval-not-revocable"));
    const bool exhausted = review.revision == std::numeric_limits<quint64>::max();
    if (exhausted) review.exhausted = true;
    else ++review.revision;
    if (revoke) {
        review.approval.insert(QStringLiteral("state"), QStringLiteral("revoked"));
        review.approval.insert(QStringLiteral("capabilityRevision"), QString::number(review.revision));
        review.approval.insert(QStringLiteral("reason"), reason);
    } else if (exhausted) {
        if (!review.approval.isEmpty() && review.approval.value(QStringLiteral("state")).toString() == QStringLiteral("current"))
            review.approval.insert(QStringLiteral("state"), QStringLiteral("superseded"));
    } else {
        const QVariantMap context = _capabilityContext(target, review);
        if (!FleetCapabilityEvidence::contextValid(context)) return reject(QStringLiteral("capability-review-next-context-invalid"));
        review.approval = {{QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("approvalId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("registryEpoch"), originalRegistryEpoch},
            {QStringLiteral("vehicleRecordEpoch"), review.vehicleRecordEpoch},
            {QStringLiteral("capabilityRevision"), QString::number(review.revision)},
            {QStringLiteral("identityFingerprint"), review.identityFingerprint},
            {QStringLiteral("configurationFingerprint"), target.configurationFingerprint},
            {QStringLiteral("capabilities"), capabilities},
            {QStringLiteral("source"), QStringLiteral("local-engineering-attestation")},
            {QStringLiteral("reason"), reason},
            {QStringLiteral("approvedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
            {QStringLiteral("state"), QStringLiteral("current")},
            {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("flightCommandReleased"), false}};
        if (!FleetCapabilityEvidence::approvalValid(review.approval)) return reject(QStringLiteral("capability-approval-contract-invalid"));
    }
    _projectCapabilityReview(target, review);
    const QVariantMap nextContext = _capabilityContext(target, review);
    int nodes = 0; qint64 bytes = 0;
    QStringList effective;
    const bool effectiveCurrent = FleetCapabilityEvidence::currentCapabilities(target.toVariantMap(), &effective);
    // Preserve the original approve-at-overflow conservative invalidation;
    // all ordinary approve/revoke stages retain their bounded-row checks.
    if ((!exhausted || revoke) && !boundedDescriptorValue(target.toVariantMap(), 0, &nodes, &bytes))
        return reject(QStringLiteral("capability-review-descriptor-limit"));
    if (!revoke && !exhausted && (!effectiveCurrent || effective != capabilities))
        return reject(QStringLiteral("capability-review-projection-invalid"));
    QPointer<FleetRegistry> self(this);
    const std::function<bool()> ownedFence = finalCommitFence;
    const auto originalCurrent = [&]() {
        return self && self->_capabilityMutationStateCurrent(systemId, originalRegistryEpoch, originalVehicle,
            originalReview, configurationApprovalPresent, configurationApproval)
            && self->_configurationHistoryCurrent(systemId, originalHistory);
    };
    if (!originalCurrent()) return reject(QStringLiteral("capability-review-source-changed-before-fence"));
    if (requireFence) {
        const bool allowed = ownedFence();
        if (!originalCurrent()) return reject(QStringLiteral("capability-review-source-changed-before-commit"));
        if (!allowed) return reject(QStringLiteral("capability-final-commit-fence-rejected"));
    }
    // No source/callback query follows the last pure CAS before this commit.
    // Freeze the actual result before public notifications can replace or
    // destroy the source; failed post-observation never rolls it back.
    _capabilityReviews.insert(systemId, review); _vehicles.insert(systemId, target);
    receipt.insert(QStringLiteral("ok"), !exhausted);
    receipt.insert(QStringLiteral("reason"), exhausted ? QStringLiteral("capability-review-revision-exhausted") : QString());
    receipt.insert(QStringLiteral("mutationCommitted"), true);
    receipt.insert(QStringLiteral("context"), nextContext);
    receipt.insert(QStringLiteral("approval"), review.approval);
    receipt.insert(QStringLiteral("effectiveCapabilities"), effective);
    _notifyCapabilityMutation(systemId);
    receipt.insert(QStringLiteral("stillCurrent"), self && self->_capabilityMutationStateCurrent(systemId,
        originalRegistryEpoch, target, review, configurationApprovalPresent, configurationApproval)
        && self->_configurationHistoryCurrent(systemId, originalHistory));
    return receipt;
}

bool FleetRegistry::approveCurrentConfiguration(int systemId, const QString& reason)
{
    // Trusted native compatibility entry: keep the default empty reason and
    // actual-commit bool semantics, including conservative overflow commits.
    return _mutateConfigurationBaseline(systemId, QVariantMap(), reason, false,
        std::function<bool()>(), false).value(QStringLiteral("mutationCommitted")).toBool();
}

QVariantMap FleetRegistry::approveCurrentConfiguration(int systemId, const QVariantMap& expectedComparison,
    const QString& reason, const std::function<bool()>& finalCommitFence)
{
    return _mutateConfigurationBaseline(systemId, expectedComparison, reason, false, finalCommitFence, true);
}

bool FleetRegistry::rollbackConfigurationBaseline(int systemId)
{
    return _mutateConfigurationBaseline(systemId, QVariantMap(), QString(), true,
        std::function<bool()>(), false).value(QStringLiteral("mutationCommitted")).toBool();
}

QVariantMap FleetRegistry::rollbackConfigurationBaseline(int systemId, const QVariantMap& expectedComparison,
    const QString& reason, const std::function<bool()>& finalCommitFence)
{
    return _mutateConfigurationBaseline(systemId, expectedComparison, reason, true, finalCommitFence, true);
}

QVariantMap FleetRegistry::_mutateConfigurationBaseline(int systemId, const QVariantMap& suppliedComparison,
    const QString& suppliedReason, bool revoke, const std::function<bool()>& finalCommitFence, bool requireFence)
{
    const QVariantMap expectedComparison = suppliedComparison;
    const QString reason = suppliedReason.trimmed();
    QVariantMap receipt{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("reason"), QString()},
        {QStringLiteral("action"), revoke ? QStringLiteral("revoke-baseline") : QStringLiteral("approve-baseline")},
        {QStringLiteral("vehicleId"), systemId}, {QStringLiteral("mutationCommitted"), false},
        {QStringLiteral("previousComparison"), QVariantMap()}, {QStringLiteral("comparison"), QVariantMap()},
        {QStringLiteral("approval"), QVariantMap()}, {QStringLiteral("baselineEvidence"), QVariantMap()},
        {QStringLiteral("stillCurrent"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const auto reject = [&](const QString& problem) {
        QVariantMap rejected = receipt; rejected.insert(QStringLiteral("reason"), problem); return rejected;
    };
    if (thread() != QThread::currentThread()) return reject(QStringLiteral("configuration-registry-owner-thread-required"));
    if (systemId < 1 || systemId > 255 || (requireFence && (reason.isEmpty() || suppliedReason.size() > 1024)))
        return reject(QStringLiteral("configuration-baseline-input-invalid"));
    if (requireFence && !finalCommitFence) return reject(QStringLiteral("configuration-baseline-final-fence-required"));
    if (!_vehicles.contains(systemId) || !_capabilityReviews.contains(systemId) || !_configurationHistories.contains(systemId))
        return reject(QStringLiteral("configuration-baseline-native-record-unavailable"));
    const QVariantMap previousComparison = configurationComparison(systemId);
    if (requireFence) {
        int remaining = 8192;
        if (previousComparison.size() != 16 || expectedComparison.size() != 16
            || !exactComparisonDto(previousComparison, expectedComparison, 0, &remaining))
            return reject(QStringLiteral("configuration-baseline-comparison-conflict"));
    }
    VehicleCapability target = _vehicles.value(systemId);
    CapabilityReviewState review = _capabilityReviews.value(systemId);
    ConfigurationHistory history = _configurationHistories.value(systemId);
    const VehicleCapability originalVehicle = target;
    const CapabilityReviewState originalReview = review;
    const ConfigurationHistory originalHistory = history;
    const QString originalRegistryEpoch = _capabilityRegistryEpoch;
    const bool originalApprovalPresent = _configurationApprovals.contains(systemId);
    const QVariantMap originalApproval = _configurationApprovals.value(systemId);
    receipt.insert(QStringLiteral("previousComparison"), previousComparison);
    receipt.insert(QStringLiteral("comparison"), previousComparison);
    receipt.insert(QStringLiteral("approval"), originalApproval);
    receipt.insert(QStringLiteral("baselineEvidence"), _configurationBaselineEvidence(systemId));
    if (history.observationEpoch.isEmpty() || history.vehicleRecordEpoch.isEmpty()
        || history.vehicleRecordEpoch != review.vehicleRecordEpoch)
        return reject(QStringLiteral("configuration-baseline-original-history-invalid"));
    if (revoke) {
        // An actual imported local approval may be hash-only/unknown. It may
        // be revoked, never upgraded into a complete typed baseline here.
        if (!originalApprovalPresent || !configurationApprovalMatches(originalApproval, target))
            return reject(QStringLiteral("configuration-baseline-not-revocable"));
    } else if (history.exhausted || !history.currentFullKnown || !history.lastFullKnown
        || target.configuration.isEmpty() || !isSha256Fingerprint(target.configurationFingerprint)
        || history.lastFingerprint != target.configurationFingerprint
        || !FleetConfigurationDiff::exactEqual(history.lastFull, target.configuration)
        || VehicleCapability::fingerprintForConfiguration(history.lastFull) != target.configurationFingerprint) {
        return reject(QStringLiteral("configuration-baseline-current-full-observation-required"));
    }

    QVariantMap approval;
    if (revoke) {
        target.baselineConfigurationFingerprint.clear(); target.baselineSource.clear();
        target.configurationState = target.configurationFingerprint.isEmpty() ? QStringLiteral("unknown") : QStringLiteral("unverified");
        history.baselineKnown = false; history.baseline.clear();
        history.baselineFingerprint.clear(); history.baselineApproval.clear();
    } else {
        target.baselineConfigurationFingerprint = target.configurationFingerprint.toLower();
        target.baselineSource = QStringLiteral("local_approval"); target.configurationState = QStringLiteral("baseline");
        approval = {{QStringLiteral("systemId"), systemId}, {QStringLiteral("fingerprint"), target.baselineConfigurationFingerprint},
            {QStringLiteral("source"), target.baselineSource}, {QStringLiteral("reason"), reason},
            {QStringLiteral("approvedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}};
        if (!FleetConfigurationDiff::valueValid(approval)) return reject(QStringLiteral("configuration-baseline-approval-limit"));
        history.baselineKnown = true; history.baseline = history.lastFull;
        history.baselineFingerprint = target.configurationFingerprint; history.baselineApproval = approval;
    }
    for (const QString& key : {QStringLiteral("baselineConfigurationFingerprint"), QStringLiteral("baselineSource"), QStringLiteral("configurationState")})
        target.metadata.remove(key);
    if (!_synchronizeCapabilityReview(target, review)) return reject(QStringLiteral("configuration-baseline-next-review-invalid"));
    // Every actual baseline approval/revocation is a new capability-review
    // basis, including same-hash reapproval. Do not double-increment when the
    // changed legacy identity already advanced it during synchronization.
    if (review.revision == originalReview.revision && review.exhausted == originalReview.exhausted) {
        if (review.exhausted || review.revision == std::numeric_limits<quint64>::max()) review.exhausted = true;
        else ++review.revision;
    }
    if (review.approval.value(QStringLiteral("state")).toString() == QStringLiteral("current"))
        review.approval.insert(QStringLiteral("state"), QStringLiteral("superseded"));
    _advanceConfigurationHistory(history);
    _cacheConfigurationComparison(history);
    if (history.exhausted) review.exhausted = true;
    _projectCapabilityReview(target, review);
    int nodes = 0; qint64 bytes = 0;
    if (!boundedDescriptorValue(target.toVariantMap(), 0, &nodes, &bytes))
        return reject(QStringLiteral("configuration-baseline-descriptor-limit"));
    const bool exhausted = history.exhausted || review.exhausted;
    QPointer<FleetRegistry> self(this);
    const std::function<bool()> ownedFence = finalCommitFence;
    const auto originalCurrent = [&]() {
        return self && self->_capabilityMutationStateCurrent(systemId, originalRegistryEpoch, originalVehicle,
            originalReview, originalApprovalPresent, originalApproval)
            && self->_configurationHistoryCurrent(systemId, originalHistory);
    };
    if (!originalCurrent()) return reject(QStringLiteral("configuration-baseline-source-changed-before-fence"));
    if (requireFence) {
        const bool allowed = ownedFence();
        if (!originalCurrent()) return reject(QStringLiteral("configuration-baseline-source-changed-before-commit"));
        if (!allowed) return reject(QStringLiteral("configuration-baseline-final-fence-rejected"));
    }
    // No callbacks follow the last pure CAS until all four actual records
    // commit. Build the immutable result before any public notification.
    _vehicles.insert(systemId, target); _capabilityReviews.insert(systemId, review);
    _configurationHistories.insert(systemId, history);
    if (revoke) _configurationApprovals.remove(systemId); else _configurationApprovals.insert(systemId, approval);
    receipt.insert(QStringLiteral("ok"), !exhausted);
    receipt.insert(QStringLiteral("reason"), exhausted ? QStringLiteral("configuration-baseline-revision-exhausted") : QString());
    receipt.insert(QStringLiteral("mutationCommitted"), true);
    receipt.insert(QStringLiteral("comparison"), configurationComparison(systemId));
    receipt.insert(QStringLiteral("approval"), approval);
    receipt.insert(QStringLiteral("baselineEvidence"), _configurationBaselineEvidence(systemId));
    _notifyCapabilityMutation(systemId);
    receipt.insert(QStringLiteral("stillCurrent"), !exhausted && self
        && self->_capabilityMutationStateCurrent(systemId, originalRegistryEpoch, target, review, !revoke, approval)
        && self->_configurationHistoryCurrent(systemId, history));
    return receipt;
}

QVariantMap FleetRegistry::configurationApprovalRecord(int systemId) const
{
    if (thread() != QThread::currentThread() || systemId < 1 || systemId > 255) return {};
    return _configurationApprovals.value(systemId);
}

QVariantMap FleetRegistry::configurationApproval(int systemId) const
{
    const auto approval = _configurationApprovals.constFind(systemId);
    if (approval != _configurationApprovals.constEnd()) {
        return approval.value();
    }
    const VehicleCapability* target = _find(systemId);
    if (!target || target->baselineConfigurationFingerprint.isEmpty()) {
        return QVariantMap();
    }
    return QVariantMap{{QStringLiteral("systemId"), systemId},
                       {QStringLiteral("fingerprint"), target->baselineConfigurationFingerprint},
                       {QStringLiteral("source"), target->baselineSource}};
}

void FleetRegistry::_recomputeHealth(VehicleCapability& vehicle, const QDateTime& nowUtc, bool notify)
{
    const QString oldState = vehicle.healthState;
    const int oldScore = vehicle.healthScore;
    _recomputeHealthValue(vehicle, nowUtc, _offlineTimeoutMs);
    if (notify && (oldState != vehicle.healthState || oldScore != vehicle.healthScore)) {
        emit vehicleHealthChanged(vehicle.systemId, vehicle.healthState, vehicle.healthScore);
    }
}

void FleetRegistry::_recomputeHealthValue(VehicleCapability& vehicle, const QDateTime& nowUtc,
                                         int offlineTimeoutMs)
{
    if (vehicle.lastSeenUtc.isValid()) {
        const qint64 ageMs = vehicle.lastSeenUtc.msecsTo(nowUtc);
        // Require a non-negative age (apart from a tiny clock-skew window).
        // Without this lower bound, a forged/faulty future timestamp produces
        // a negative age and bypasses the offline timeout indefinitely.
        vehicle.online = vehicle.online
            && ageMs >= -kFutureLastSeenToleranceMs
            && ageMs <= offlineTimeoutMs;
    } else if (vehicle.online) {
        // An online record without a valid last-seen time cannot be trusted
        // as a live vehicle. This is also a guard for old snapshots created
        // before timestamp validation was introduced.
        vehicle.online = false;
    }

    if (vehicle.faultHealthKnown && vehicle.faultEvidenceAgeMs >= 0
        && vehicle.faultEvidenceObservedUtc.isValid()) {
        const qint64 evidenceElapsedMs = vehicle.faultEvidenceObservedUtc.msecsTo(nowUtc);
        if (evidenceElapsedMs >= 0) {
            const qint64 effectiveAge = static_cast<qint64>(vehicle.faultEvidenceAgeMs)
                + evidenceElapsedMs;
            vehicle.faultEvidenceAgeMs = static_cast<int>(qMin<qint64>(
                std::numeric_limits<int>::max(), effectiveAge));
            // Re-anchor after advancing so repeated timer refreshes do not
            // add the full lifetime since the original observation twice.
            vehicle.faultEvidenceObservedUtc = nowUtc;
        }
    }

    int score = vehicle.online ? 100 : 0;
    QStringList reasons;
    if (!vehicle.online) {
        reasons << tr("心跳超时");
    }
    if (vehicle.batteryPercent >= 0.0) {
        if (vehicle.batteryPercent < 15.0) {
            score -= 40;
            reasons << tr("电量极低");
        } else if (vehicle.batteryPercent < 30.0) {
            score -= 25;
            reasons << tr("电量偏低");
        } else if (vehicle.batteryPercent < 50.0) {
            score -= 10;
            reasons << tr("电量下降");
        }
    }
    if (vehicle.linkQualityPercent >= 0 && vehicle.linkQualityPercent < 30) {
        score -= 30;
        reasons << tr("链路质量差");
    } else if (vehicle.linkQualityPercent >= 0 && vehicle.linkQualityPercent < 60) {
        score -= 15;
        reasons << tr("链路质量下降");
    }
    if (vehicle.estimatorHealthKnown && !vehicle.estimatorHealthy) {
        score -= 40;
        reasons << tr("估计器异常");
    }
    if (vehicle.positionHealthKnown && !vehicle.positionHealthy) {
        score -= 20;
        reasons << tr("定位不健康");
    }
    if (vehicle.hasFault) {
        score -= 50;
        reasons << (vehicle.faultSummary.isEmpty() ? tr("飞行器报告故障") : vehicle.faultSummary);
    }
    // An online vehicle with no valid health telemetry is not healthy. Keep
    // this explicit so capability matching and task planning cannot silently
    // treat a newly connected or partially decoded vehicle as ready.
    const bool healthSignalsKnown = vehicle.batteryPercent >= 0.0
        || vehicle.linkQualityPercent >= 0
        || vehicle.estimatorHealthKnown
        || vehicle.positionHealthKnown;
    // A fault-free boolean is not proof that no fault exists when the
    // diagnostic stream has gone quiet. Require an explicit fault assessment
    // before advertising a healthy online vehicle.
    const bool faultEvidenceFresh = vehicle.faultHealthKnown
        && vehicle.faultEvidenceAgeMs >= 0
        && vehicle.faultEvidenceAgeMs <= offlineTimeoutMs;
    const bool healthKnown = healthSignalsKnown && faultEvidenceFresh;
    if (!healthSignalsKnown && vehicle.online) {
        reasons << tr("等待健康遥测");
    } else if (!faultEvidenceFresh && vehicle.online) {
        reasons << tr("等待故障评估遥测");
    }
    // Do not expose a nominal 100 score while the state is unknown. A zero
    // score makes the summary conservative and prevents an unverified vehicle
    // from inflating fleet readiness metrics.
    vehicle.healthScore = healthKnown ? qBound(0, score, 100) : 0;
    vehicle.healthState = normalizedState(vehicle.healthScore, vehicle.online,
                                          vehicle.hasFault, healthKnown);
    vehicle.healthReason = reasons.join(QStringLiteral("；"));
}

void FleetRegistry::refreshHealth()
{
    if (thread() != QThread::currentThread()) return;
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    bool changed = false;
    QVariantList notifications;
    for (auto iterator = _vehicles.begin(); iterator != _vehicles.end(); ++iterator) {
        const QString oldState = iterator.value().healthState;
        const int oldScore = iterator.value().healthScore;
        const bool oldOnline = iterator.value().online;
        const QString oldReason = iterator.value().healthReason;
        const int oldFaultAge = iterator.value().faultEvidenceAgeMs;
        _recomputeHealth(iterator.value(), nowUtc, false);
        if (oldState != iterator.value().healthState || oldScore != iterator.value().healthScore)
            notifications.append(QVariantMap{{QStringLiteral("vehicleId"), iterator.key()},
                {QStringLiteral("state"), iterator.value().healthState}, {QStringLiteral("score"), iterator.value().healthScore}});
        changed = changed || oldState != iterator.value().healthState
            || oldScore != iterator.value().healthScore
            || oldReason != iterator.value().healthReason
            || oldOnline != iterator.value().online
            || oldFaultAge != iterator.value().faultEvidenceAgeMs;
    }
    const QPointer<FleetRegistry> self(this);
    for (const QVariant& notification : notifications) {
        const QVariantMap row = notification.toMap();
        emit vehicleHealthChanged(row.value(QStringLiteral("vehicleId")).toInt(), row.value(QStringLiteral("state")).toString(), row.value(QStringLiteral("score")).toInt());
        if (!self) return;
    }
    if (changed) {
        emit registryChanged();
        if (!self) return;
        emit healthChanged();
    }
}

QString FleetRegistry::exportCapabilityPassport() const
{
    if (thread() != QThread::currentThread()) return QString();
    QJsonObject root;
    root.insert(QStringLiteral("kind"), QStringLiteral("merivus.fleet-capability-passport"));
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("exportedAtUtc"),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));

    QJsonArray vehiclesJson;
    QList<int> ids = _vehicles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int systemId : ids)
        vehiclesJson.append(QJsonObject::fromVariantMap(
            withoutConfigurationBaselineEvidence(_vehicles.value(systemId).toVariantMap())));
    root.insert(QStringLiteral("vehicles"), vehiclesJson);

    QJsonObject approvalsJson;
    for (auto iterator = _configurationApprovals.constBegin();
         iterator != _configurationApprovals.constEnd(); ++iterator) {
        QVariantMap approval = iterator.value();
        approval.insert(QStringLiteral("systemId"), iterator.key());
        approvalsJson.insert(QString::number(iterator.key()),
                             QJsonObject::fromVariantMap(approval));
    }
    root.insert(QStringLiteral("configurationApprovals"), approvalsJson);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool FleetRegistry::importCapabilityPassport(const QString& json, bool replaceExisting)
{
    if (thread() != QThread::currentThread()) return false;
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxCapabilityPassportBytes) {
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    if (!containsOnlyJsonKeys(root, QStringList()
        << QStringLiteral("kind") << QStringLiteral("schemaVersion")
        << QStringLiteral("exportedAtUtc") << QStringLiteral("vehicles")
        << QStringLiteral("configurationApprovals"))) {
        return false;
    }
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("kind")).toString()
            != QStringLiteral("merivus.fleet-capability-passport")
        // QJsonValue::toInt() truncates fractional numbers on some Qt
        // versions; compare the numeric value directly so 1.5 is not treated
        // as schema version 1.
        || !schemaVersion.isDouble() || schemaVersion.toDouble() != 1.0
        || !root.value(QStringLiteral("vehicles")).isArray()
        || (root.contains(QStringLiteral("configurationApprovals"))
            && !root.value(QStringLiteral("configurationApprovals")).isObject())) {
        return false;
    }
    if (root.contains(QStringLiteral("exportedAtUtc"))
        && (!root.value(QStringLiteral("exportedAtUtc")).isString()
            || !validTimestampValue(root.value(QStringLiteral("exportedAtUtc")).toString()))) {
        return false;
    }

    const QJsonArray vehiclesJson = root.value(QStringLiteral("vehicles")).toArray();
    QList<QVariantMap> descriptors;
    QSet<int> importedIds;
    descriptors.reserve(vehiclesJson.size());
    for (const QJsonValue& value : vehiclesJson) {
        if (!value.isObject()) {
            return false;
        }
        const QVariantMap descriptor = withoutConfigurationBaselineEvidence(value.toObject().toVariantMap());
        const VehicleCapability parsed = VehicleCapability::fromVariantMap(descriptor);
        if (parsed.systemId <= 0 || parsed.systemId > 255) {
            return false;
        }
        const QString suppliedFingerprint = descriptor.value(
            QStringLiteral("configurationFingerprint")).toString().trimmed().toLower();
        if (!suppliedFingerprint.isEmpty() && !isSha256Fingerprint(suppliedFingerprint)) {
            return false;
        }
        if (!parsed.configuration.isEmpty() && !suppliedFingerprint.isEmpty()
            && VehicleCapability::fingerprintForConfiguration(parsed.configuration)
                != suppliedFingerprint) {
            return false;
        }
        const QString baselineSource = descriptor.value(
            QStringLiteral("baselineSource")).toString().trimmed();
        const QString baselineFingerprint = descriptor.value(
            QStringLiteral("baselineConfigurationFingerprint")).toString().trimmed().toLower();
        const QVariantMap baselineMap = descriptor.value(
            QStringLiteral("configurationBaseline")).toMap();
        const bool hasBaselineMap = !baselineMap.isEmpty();
        if (!baselineSource.isEmpty()
            && baselineSource != QStringLiteral("trusted_manifest")
            && baselineSource != QStringLiteral("local_approval")) {
            return false;
        }
        if (!baselineSource.isEmpty() && baselineFingerprint.isEmpty() && !hasBaselineMap) {
            return false;
        }
        if (!baselineFingerprint.isEmpty()
            && (!isSha256Fingerprint(baselineFingerprint)
                || (baselineSource != QStringLiteral("trusted_manifest")
                    && baselineSource != QStringLiteral("local_approval")))) {
            return false;
        }
        if (hasBaselineMap && !baselineFingerprint.isEmpty()
            && VehicleCapability::fingerprintForConfiguration(baselineMap)
                != baselineFingerprint) {
            return false;
        }
        if (importedIds.contains(parsed.systemId)) {
            return false;
        }
        importedIds.insert(parsed.systemId);
        descriptors.append(descriptor);
    }

    const QJsonObject approvalsJson = root.value(QStringLiteral("configurationApprovals"))
        .toObject();
    QSet<int> touchedIds = importedIds;
    for (auto iterator = approvalsJson.constBegin(); iterator != approvalsJson.constEnd(); ++iterator) {
        bool idOk = false;
        const int systemId = iterator.key().toInt(&idOk);
        if (!idOk || systemId < 1 || systemId > 255) return false;
        touchedIds.insert(systemId);
    }

    // Build the candidate registry off to the side.  A malformed descriptor
    // must never leave the live fleet half-cleared or half-imported.
    FleetRegistry staged;
    staged._healthTimer.stop();
    staged._offlineTimeoutMs = _offlineTimeoutMs;
    staged._capabilityRegistryEpoch = _capabilityRegistryEpoch;
    if (!replaceExisting) {
        staged._vehicles = _vehicles;
        staged._configurationApprovals = _configurationApprovals;
        staged._capabilityReviews = _capabilityReviews;
        staged._configurationHistories = _configurationHistories;
    }
    // Even an approval-only import retires the touched row's old observation
    // incarnation. JSON can introduce a NEW decoded full observation, never
    // resurrect a process-local typed baseline or an earlier typed snapshot.
    for (const int touchedId : touchedIds) {
        staged._configurationHistories.remove(touchedId);
        staged._capabilityReviews.remove(touchedId);
    }
    for (const QVariantMap& descriptor : descriptors) {
        const int importedId = VehicleCapability::fromVariantMap(descriptor).systemId;
        // A portable snapshot may retain declarations and one historical
        // record, but cannot transfer an active native approval/CAS identity.
        // Only explicitly touched vehicles get a new record incarnation.
        staged._capabilityReviews.remove(importedId);
        if (!staged._registerVehicle(descriptor, true)) {
            return false;
        }
        // An explicit imported baseline declaration replaces the descriptor's
        // baseline state. Do not leave an unrelated local approval record
        // attached when the descriptor clears or switches to a trusted
        // manifest baseline; approvals are only valid for local_approval.
        if (descriptor.contains(QStringLiteral("baselineSource"))) {
            const QString source = descriptor.value(QStringLiteral("baselineSource"))
                .toString().trimmed();
            if (source != QStringLiteral("local_approval")) {
                bool idOk = false;
                const int systemId = descriptor.value(QStringLiteral("systemId"))
                    .toInt(&idOk);
                if (idOk) {
                    staged._configurationApprovals.remove(systemId);
                }
            }
        }
    }

    QHash<int, QVariantMap> importedApprovals;
    for (auto iterator = approvalsJson.constBegin(); iterator != approvalsJson.constEnd();
         ++iterator) {
        bool idOk = false;
        const int systemId = iterator.key().toInt(&idOk);
        if (!idOk || systemId <= 0 || systemId > 255 || !iterator.value().isObject()
            || !staged._vehicles.contains(systemId)) {
            return false;
        }
        const QVariantMap approval = iterator.value().toObject().toVariantMap();
        const QJsonObject approvalObject = iterator.value().toObject();
        if (!containsOnlyJsonKeys(approvalObject, QStringList()
            << QStringLiteral("systemId") << QStringLiteral("fingerprint")
            << QStringLiteral("source") << QStringLiteral("reason")
            << QStringLiteral("approvedAtUtc"))) {
            return false;
        }
        if ((approvalObject.contains(QStringLiteral("reason"))
             && !approvalObject.value(QStringLiteral("reason")).isString())
             || (approvalObject.contains(QStringLiteral("approvedAtUtc"))
                && !approvalObject.value(QStringLiteral("approvedAtUtc")).isString())) {
            return false;
        }
        int approvalSystemId = systemId;
        if (approvalObject.contains(QStringLiteral("systemId"))) {
            const QJsonValue approvalId = approvalObject.value(QStringLiteral("systemId"));
            const double numericId = approvalId.isDouble() ? approvalId.toDouble() : 0.0;
            if (!approvalId.isDouble() || !qIsFinite(numericId)
                || qFloor(numericId) != numericId || numericId != systemId) {
                return false;
            }
            approvalSystemId = static_cast<int>(numericId);
        }
        idOk = approvalSystemId > 0;
        const QString fingerprint = approval.value(QStringLiteral("fingerprint"))
            .toString().trimmed().toLower();
        const QString source = approval.value(QStringLiteral("source"))
            .toString().trimmed();
        if (!idOk || approvalSystemId != systemId || !isSha256Fingerprint(fingerprint)
            || source != QStringLiteral("local_approval")
            || staged._vehicles.value(systemId).baselineSource
                != QStringLiteral("local_approval")
            || staged._vehicles.value(systemId).baselineConfigurationFingerprint != fingerprint) {
            return false;
        }
        const QVariant approvedAt = approval.value(QStringLiteral("approvedAtUtc"));
        if (approvedAt.isValid()) {
            const QDateTime parsedApproval = QDateTime::fromString(
                approvedAt.toString(), Qt::ISODateWithMs);
            if (!parsedApproval.isValid()
                || parsedApproval.toUTC().msecsTo(QDateTime::currentDateTimeUtc())
                    < -kFutureApprovalToleranceMs) {
                return false;
            }
        }
        importedApprovals.insert(systemId, approval);
    }
    // Incremental imports merge approval records instead of discarding local
    // approvals for vehicles not present in this file. Full replacement still
    // starts from an empty staged map, so the same insert path is atomic.
    for (auto iterator = importedApprovals.constBegin();
         iterator != importedApprovals.constEnd(); ++iterator) {
        staged._configurationApprovals.insert(iterator.key(), iterator.value());
    }
    for (const int touchedId : touchedIds) {
        if (!staged._vehicles.contains(touchedId)) return false;
        if (!staged._capabilityReviews.contains(touchedId)) {
            VehicleCapability vehicle = staged._vehicles.value(touchedId);
            CapabilityReviewState review;
            if (!staged._synchronizeCapabilityReview(vehicle, review)) return false;
            int nodes = 0; qint64 bytes = 0;
            if (!boundedDescriptorValue(vehicle.toVariantMap(), 0, &nodes, &bytes)) return false;
            staged._vehicles.insert(touchedId, vehicle);
            staged._capabilityReviews.insert(touchedId, review);
        }
        if (!staged._configurationHistories.contains(touchedId)) {
            ConfigurationHistory history;
            staged._observeConfiguration(history, staged._vehicles.value(touchedId),
                staged._capabilityReviews.value(touchedId), QVariantMap(), VehicleCapability());
            staged._configurationHistories.insert(touchedId, history);
        }
    }

    // A local_approval baseline is meaningful only with a matching local
    // approval record. This prevents an external passport from forging a
    // baseline by putting the claim only on the vehicle descriptor.
    for (const QVariantMap& descriptor : descriptors) {
        const QString source = descriptor.value(QStringLiteral("baselineSource"))
            .toString().trimmed();
        if (source != QStringLiteral("local_approval")) {
            continue;
        }
        bool idOk = false;
        const int systemId = descriptor.value(QStringLiteral("systemId"))
            .toInt(&idOk);
        if (!idOk || !staged._vehicles.contains(systemId)) {
            return false;
        }
        const VehicleCapability& vehicle = staged._vehicles.value(systemId);
        const QVariantMap approval = staged._configurationApprovals.value(systemId);
        if (approval.value(QStringLiteral("source")).toString().trimmed()
                != QStringLiteral("local_approval")
            || approval.value(QStringLiteral("fingerprint")).toString().trimmed().toLower()
                != vehicle.baselineConfigurationFingerprint) {
            return false;
        }
    }

    QSet<int> previousIds;
    for (const int systemId : _vehicles.keys()) {
        previousIds.insert(systemId);
    }
    QSet<int> importedIdsSet;
    for (const int systemId : staged._vehicles.keys()) {
        importedIdsSet.insert(systemId);
    }
    QList<QMetaObject::Connection> retiredConfigurationConnections;
    const QList<int> configurationIds = _nativeConfigurationSources.keys();
    for (const int systemId : configurationIds) {
        if (replaceExisting || touchedIds.contains(systemId))
            retiredConfigurationConnections.append(_nativeConfigurationSources.take(systemId).connections);
    }
    _vehicles = staged._vehicles;
    _configurationApprovals = staged._configurationApprovals;
    _capabilityReviews = staged._capabilityReviews;
    _configurationHistories = staged._configurationHistories;
    if (replaceExisting) _nativeBatterySources.clear();
    else for (const int systemId : touchedIds) _nativeBatterySources.remove(systemId);
    if (replaceExisting || !touchedIds.isEmpty())
        _nativeBatteryBindingsEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QPointer<FleetRegistry> self(this);
    for (const QMetaObject::Connection& connection : retiredConfigurationConnections) {
        QObject::disconnect(connection);
        if (!self) return true;
    }
    for (const int systemId : previousIds) {
        if (!importedIdsSet.contains(systemId)) {
            emit vehicleRemoved(systemId);
        } else {
            // A replace import can change capability/configuration fields for
            // an existing system ID.  Keep the per-vehicle notification in
            // sync with the atomic registry swap; consumers should not have
            // to infer updates from registryChanged() alone.
            emit vehicleUpdated(systemId);
        }
        if (!self) return true;
    }
    for (const int systemId : importedIdsSet) {
        if (!previousIds.contains(systemId)) {
            emit vehicleAdded(systemId);
            if (!self) return true;
        }
        const VehicleCapability* importedVehicle = _find(systemId);
        if (importedVehicle) {
            const QString state = importedVehicle->healthState;
            const int score = importedVehicle->healthScore;
            emit vehicleHealthChanged(systemId, state, score);
            if (!self) return true;
        }
    }
    emit registryChanged();
    if (!self) return true;
    emit healthChanged();

    // An empty but structurally valid passport is still valid input. This is
    // useful when an operator intentionally clears a saved fleet snapshot.
    return true;
}

bool FleetRegistry::saveCapabilityPassportToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportCapabilityPassport().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxCapabilityPassportBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetRegistry::loadCapabilityPassportFromFile(const QString& filePath,
                                                   bool replaceExisting)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxCapabilityPassportBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxCapabilityPassportBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importCapabilityPassport performs a complete staged validation before
    // swapping the live registry, so a bad file leaves the current fleet intact.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importCapabilityPassport(json, replaceExisting);
}

void FleetRegistry::_refreshHealthTimer()
{
    refreshHealth();
}

void FleetRegistry::clear()
{
    if (thread() != QThread::currentThread()) return;
    const QPointer<FleetRegistry> self(this);
    const QHash<int, NativeConfigurationSource> retiredConfigurationSources = _nativeConfigurationSources;
    _nativeConfigurationSources.clear();
    const QList<int> ids = _vehicles.keys();
    _capabilityRegistryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _capabilityReviews.clear();
    _configurationHistories.clear();
    _nativeBatterySources.clear();
    _nativeBatteryBindingsEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _vehicles.clear();
    _configurationApprovals.clear();
    // All original state has committed before any disconnectNotify callback.
    // A callback-created row/binding is not removed by this old clear.
    for (const NativeConfigurationSource& binding : retiredConfigurationSources) {
        for (const QMetaObject::Connection& connection : binding.connections) {
            QObject::disconnect(connection);
            if (!self) return;
        }
    }
    if (ids.isEmpty()) return;
    for (const int systemId : ids) {
        emit vehicleRemoved(systemId);
        if (!self) return;
    }
    emit registryChanged();
    if (!self) return;
    emit healthChanged();
}

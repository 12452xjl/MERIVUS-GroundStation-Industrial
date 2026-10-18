#include "FleetCapabilityEvidence.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace {

bool text(const QVariant& value, int maximum, bool allowEmpty = false)
{
    return value.type() == QVariant::String && (allowEmpty || !value.isNull())
        && value.toString().size() <= maximum
        && (allowEmpty || !value.toString().isEmpty())
        && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool sha256(const QVariant& value, bool allowEmpty = false)
{
    if (!text(value, 64, allowEmpty)) return false;
    const QString string = value.toString();
    if (string.isEmpty()) return allowEmpty;
    if (string.size() != 64) return false;
    for (const QChar character : string)
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
              || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool decimal(const QVariant& value, bool allowZero = false, bool allowEmpty = false)
{
    if (!text(value, 20, allowEmpty)) return false;
    const QString string = value.toString();
    if (string.isEmpty()) return allowEmpty;
    bool ok = false;
    const quint64 number = string.toULongLong(&ok);
    return ok && (allowZero || number != 0) && QString::number(number) == string;
}

bool falseFlag(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull() && !value.toBool();
}

bool boundedInteger(const QVariant& value, int minimum, int maximum, int* result = nullptr)
{
    if (value.isNull() || !(value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
        || value.type() == QVariant::Double)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !std::isfinite(number) || number < minimum || number > maximum
        || std::floor(number) != number) return false;
    if (result) *result = static_cast<int>(number);
    return true;
}

bool metadataFor(const QVariantMap& snapshot, QVariantMap* metadata)
{
    if (!metadata) return false;
    metadata->clear();
    if (!snapshot.contains(QStringLiteral("metadata"))) return true;
    const QVariant value = snapshot.value(QStringLiteral("metadata"));
    if (value.type() != QVariant::Map || value.isNull()) return false;
    *metadata = value.toMap();
    return true;
}

// Never resolve an ID by string conversion, and never let a second alias name
// select another record. Double is accepted only for an integral QML number.
bool vehicleId(const QVariantMap& snapshot, int* result)
{
    int found = 0;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id")}) {
        if (!snapshot.contains(key)) continue;
        const QVariant value = snapshot.value(key);
        if (value.isNull() || !(value.type() == QVariant::Int || value.type() == QVariant::UInt
            || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
            || value.type() == QVariant::Double)) return false;
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !std::isfinite(number) || number < 1 || number > 255 || std::floor(number) != number) return false;
        const int candidate = static_cast<int>(number);
        if (found && found != candidate) return false;
        found = candidate;
    }
    if (!found) return false;
    if (result) *result = found;
    return true;
}

bool snapshotVehicleId(const QVariantMap& snapshot, const QVariantMap& metadata, int* result)
{
    int actual = 0;
    if (!vehicleId(snapshot, &actual)) return false;
    if (metadata.contains(QStringLiteral("systemId")) || metadata.contains(QStringLiteral("vehicleId"))
        || metadata.contains(QStringLiteral("id"))) {
        int nested = 0;
        if (!vehicleId(metadata, &nested) || nested != actual) return false;
    }
    if (result) *result = actual;
    return true;
}

bool selected(const QVariantMap& snapshot, const QVariantMap& metadata,
              const QString& key, QVariant* result, bool* present = nullptr)
{
    const bool top = snapshot.contains(key), nested = metadata.contains(key);
    if (present) *present = top || nested;
    if (top && nested && (snapshot.value(key).userType() != metadata.value(key).userType()
                         || snapshot.value(key) != metadata.value(key))) return false;
    if (result) *result = top ? snapshot.value(key) : metadata.value(key);
    return true;
}

bool selectedCapabilities(const QVariantMap& snapshot, const QVariantMap& metadata,
                          const QString& key, QStringList* result)
{
    QStringList top, nested;
    if (snapshot.contains(key)
        && !FleetCapabilityEvidence::canonicalCapabilities(snapshot.value(key), &top)) return false;
    if (metadata.contains(key)
        && !FleetCapabilityEvidence::canonicalCapabilities(metadata.value(key), &nested)) return false;
    if (snapshot.contains(key) && metadata.contains(key) && top != nested) return false;
    if (result) *result = snapshot.contains(key) ? top : nested;
    return true;
}

bool canonicalApprovalCapabilities(const QVariant& value, QStringList* result)
{
    if (value.type() != QVariant::StringList && value.type() != QVariant::List) return false;
    QStringList canonical;
    if (!FleetCapabilityEvidence::canonicalCapabilities(value, &canonical) || canonical.isEmpty()) return false;
    QStringList original;
    if (value.type() == QVariant::StringList) original = value.toStringList();
    else for (const QVariant& item : value.toList()) original.append(item.toString());
    if (original != canonical) return false;
    if (result) *result = canonical;
    return true;
}

bool selectedRecord(const QVariantMap& snapshot, const QVariantMap& metadata,
                    const QString& key, bool approval, QVariantMap* result)
{
    if (!result) return false;
    result->clear();
    QVariantMap first;
    bool found = false;
    for (const QVariantMap& source : {snapshot, metadata}) {
        if (!source.contains(key)) continue;
        const QVariant raw = source.value(key);
        if (raw.type() != QVariant::Map || raw.isNull()) return false;
        QVariantMap record = raw.toMap();
        if (!record.isEmpty()) {
            if (approval ? !FleetCapabilityEvidence::approvalValid(record)
                         : !FleetCapabilityEvidence::contextValid(record)) return false;
            if (approval) {
                QStringList capabilities;
                if (!canonicalApprovalCapabilities(record.value(QStringLiteral("capabilities")), &capabilities)) return false;
                record.insert(QStringLiteral("capabilities"), capabilities);
                record.insert(QStringLiteral("schemaVersion"), 1);
            } else record.insert(QStringLiteral("vehicleId"), record.value(QStringLiteral("vehicleId")).toInt());
        }
        if (found && first != record) return false;
        first = record;
        found = true;
    }
    *result = first;
    return true;
}

bool booleanValue(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull();
}

bool baselineEvidenceShape(const QVariantMap& supplied, QVariantMap* canonical)
{
    if (!canonical) return false;
    canonical->clear();
    const QVariant rawContext = supplied.value(QStringLiteral("registryContext"));
    if (supplied.size() != 14 || !boundedInteger(supplied.value(QStringLiteral("schemaVersion")), 1, 1)
        || !text(supplied.value(QStringLiteral("source")), 64)
        || supplied.value(QStringLiteral("source")).toString() != QStringLiteral("native-local-configuration-baseline")
        || rawContext.type() != QVariant::Map || rawContext.isNull()
        || !uuid(supplied.value(QStringLiteral("observationEpoch")))
        || !decimal(supplied.value(QStringLiteral("observationRevision")))
        || !booleanValue(supplied.value(QStringLiteral("currentFullKnown")))
        || !booleanValue(supplied.value(QStringLiteral("baselineFullKnown")))
        || !booleanValue(supplied.value(QStringLiteral("baselineMatches")))
        || !falseFlag(supplied.value(QStringLiteral("hardwareIdentityAuthenticated")))
        || !falseFlag(supplied.value(QStringLiteral("performanceCertified")))
        || !falseFlag(supplied.value(QStringLiteral("flightCommandReleased")))) return false;
    QVariantMap context = rawContext.toMap();
    if (!context.isEmpty()) {
        if (!FleetCapabilityEvidence::contextValid(context)) return false;
        context.insert(QStringLiteral("vehicleId"), context.value(QStringLiteral("vehicleId")).toInt());
    }
    const bool currentKnown = supplied.value(QStringLiteral("currentFullKnown")).toBool();
    const bool baselineKnown = supplied.value(QStringLiteral("baselineFullKnown")).toBool();
    const QVariant currentHash = supplied.value(QStringLiteral("currentTypedFingerprint"));
    const QVariant baselineHash = supplied.value(QStringLiteral("baselineTypedFingerprint"));
    const QVariant approvalHash = supplied.value(QStringLiteral("baselineApprovalHash"));
    if (!sha256(currentHash, !currentKnown) || (!currentKnown && !currentHash.toString().isEmpty())
        || !sha256(baselineHash, !baselineKnown) || (!baselineKnown && !baselineHash.toString().isEmpty())
        || !sha256(approvalHash, !baselineKnown) || (!baselineKnown && !approvalHash.toString().isEmpty())) return false;
    const bool matches = currentKnown && baselineKnown && currentHash.toString() == baselineHash.toString();
    // A contradictory false cannot hide a positive hash pair, and a true
    // cannot manufacture a complete observation or a missing approval hash.
    if (supplied.value(QStringLiteral("baselineMatches")).toBool() != matches) return false;
    *canonical = supplied;
    canonical->insert(QStringLiteral("schemaVersion"), 1);
    canonical->insert(QStringLiteral("registryContext"), context);
    return true;
}

enum class BaselineRecordState { Missing, Invalid, Available };

BaselineRecordState baselineRecord(const QVariantMap& snapshot, QVariantMap* canonical)
{
    if (!canonical) return BaselineRecordState::Invalid;
    canonical->clear();
    QVariantMap metadata;
    if (!metadataFor(snapshot, &metadata)) return BaselineRecordState::Invalid;
    const QString key = QStringLiteral("configurationBaselineEvidence");
    // This field is made only by the actual Registry's descriptor getters.
    // An imported/telemetry metadata claim is not an alternate source, even
    // when it happens to repeat the same values as a top-level claim.
    if (metadata.contains(key)) return BaselineRecordState::Invalid;
    if (!snapshot.contains(key)) return BaselineRecordState::Missing;
    const QVariant raw = snapshot.value(key);
    if (raw.type() != QVariant::Map || raw.isNull()
        || !baselineEvidenceShape(raw.toMap(), canonical)) return BaselineRecordState::Invalid;
    int id = 0;
    QVariantMap actualContext;
    if (!snapshotVehicleId(snapshot, metadata, &id)
        || !selectedRecord(snapshot, metadata, QStringLiteral("capabilityReviewContext"), false, &actualContext)) {
        canonical->clear(); return BaselineRecordState::Invalid;
    }
    const QVariantMap evidenceContext = canonical->value(QStringLiteral("registryContext")).toMap();
    const QString actualIdentity = FleetCapabilityEvidence::identityFingerprint(snapshot);
    if (!sha256(actualIdentity) || actualContext != evidenceContext) {
        canonical->clear(); return BaselineRecordState::Invalid;
    }
    if (!actualContext.isEmpty()) {
        QVariant configuration;
        if (!FleetCapabilityEvidence::contextValid(actualContext)
            || actualContext.value(QStringLiteral("vehicleId")).toInt() != id
            || actualContext.value(QStringLiteral("identityFingerprint")).toString() != actualIdentity
            || !selected(snapshot, metadata, QStringLiteral("configurationFingerprint"), &configuration)
            || !sha256(configuration) || configuration != actualContext.value(QStringLiteral("configurationFingerprint"))) {
            canonical->clear(); return BaselineRecordState::Invalid;
        }
    }
    return BaselineRecordState::Available;
}

// Only called for the fixed, fully validated review/baseline identities.
// Rebuild containers and text buffers rather than sharing Qt storage with a
// caller that may already hold mutable references or QString::data(). Never
// traverse arbitrary configuration/metadata or serialize away QVariant types.
QVariant ownedPerformanceIdentity(const QVariant& value)
{
    const auto ownText = [](const QString& textValue) {
        return textValue.isNull() ? QString() : QString(textValue.constData(), textValue.size());
    };
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
            result.insert(ownText(it.key()), ownedPerformanceIdentity(it.value()));
        return result;
    }
    if (value.type() == QVariant::List) {
        QVariantList result;
        const QVariantList source = value.toList();
        result.reserve(source.size());
        for (const QVariant& entry : source) result.append(ownedPerformanceIdentity(entry));
        return result;
    }
    if (value.type() == QVariant::StringList) {
        QStringList result;
        for (const QString& entry : value.toStringList()) result.append(ownText(entry));
        return result;
    }
    if (value.type() == QVariant::String) return ownText(value.toString());
    return value;
}

} // namespace

bool FleetCapabilityEvidence::canonicalCapabilities(const QVariant& value, QStringList* result)
{
    if (result) result->clear();
    if (!value.isValid() || value.isNull()) return false;
    QVariantList tokens;
    if (value.type() == QVariant::StringList) {
        for (const QString& token : value.toStringList()) tokens.append(token);
    } else if (value.type() == QVariant::List) tokens = value.toList();
    else if (value.type() == QVariant::String) {
        if (value.toString().size() > 128 * 129 || !text(value, 128 * 129, true)) return false;
        if (!value.toString().trimmed().isEmpty())
            for (const QString& token : value.toString().split(QLatin1Char(','), Qt::KeepEmptyParts)) tokens.append(token);
    } else return false;
    if (tokens.size() > 128) return false;
    QStringList canonical;
    for (const QVariant& token : tokens) {
        if (!text(token, 128) || token.toString().trimmed().isEmpty()) return false;
        const QString normalized = token.toString().trimmed().toLower();
        if (normalized.size() > 128) return false;
        canonical.append(normalized);
    }
    canonical.removeDuplicates();
    std::sort(canonical.begin(), canonical.end());
    if (result) *result = canonical;
    return true;
}

QString FleetCapabilityEvidence::identityFingerprint(const QVariantMap& snapshot)
{
    QVariantMap metadata;
    int id = 0;
    if (!metadataFor(snapshot, &metadata) || !snapshotVehicleId(snapshot, metadata, &id)) return {};
    QVariantMap basis{{QStringLiteral("vehicleId"), id}};
    for (const QString& key : {QStringLiteral("vehicleType"), QStringLiteral("autopilotType"),
            QStringLiteral("firmwareVersion"), QStringLiteral("baselineSource"), QStringLiteral("configurationState")}) {
        QVariant value;
        bool present = false;
        if (!selected(snapshot, metadata, key, &value, &present)
            || (present && !text(value, 512, true))) return {};
        basis.insert(key, present ? value.toString() : QString());
    }
    for (const QString& key : {QStringLiteral("configurationFingerprint"), QStringLiteral("baselineConfigurationFingerprint")}) {
        QVariant value;
        bool present = false;
        if (!selected(snapshot, metadata, key, &value, &present) || (present && !sha256(value, true))) return {};
        basis.insert(key, present ? value.toString() : QString());
    }
    for (const QString& key : {QStringLiteral("capabilities"), QStringLiteral("declaredVerifiedCapabilities")}) {
        QStringList capabilities;
        if (!selectedCapabilities(snapshot, metadata, key, &capabilities)) return {};
        basis.insert(key, capabilities);
    }
    for (const QString& key : {QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds")}) {
        QVariant value;
        bool present = false;
        if (!selected(snapshot, metadata, key, &value, &present)) return {};
        double number = -1;
        if (present) {
            if (value.isNull() || !(value.type() == QVariant::Double || value.type() == QVariant::Int
                || value.type() == QVariant::UInt || value.type() == QVariant::LongLong
                || value.type() == QVariant::ULongLong)) return {};
            bool ok = false;
            number = value.toDouble(&ok);
            if (!ok || !std::isfinite(number) || (number < 0 && number != -1)) return {};
        }
        basis.insert(key, number);
    }
    for (const QString& key : {QStringLiteral("canUseGnss"), QStringLiteral("canUseRtk"),
            QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"), QStringLiteral("canRelay")}) {
        QVariant value;
        bool present = false;
        if (!selected(snapshot, metadata, key, &value, &present)
            || (present && (value.type() != QVariant::Bool || value.isNull()))) return {};
        basis.insert(key, present && value.toBool());
    }
    QVariantMap uid;
    for (const QString& key : {QStringLiteral("vehicleUID"), QStringLiteral("uidDecimal"), QStringLiteral("uid2Hex"),
            QStringLiteral("uidKnown"), QStringLiteral("uid2Known")}) {
        QVariant value;
        bool present = false;
        if (!selected(snapshot, metadata, key, &value, &present)) return {};
        if (!present) continue;
        if (key == QStringLiteral("uidKnown") || key == QStringLiteral("uid2Known")) {
            if (value.type() != QVariant::Bool || value.isNull()) return {};
        } else if (key == QStringLiteral("uidDecimal")) {
            if (!decimal(value, true, true)) return {};
        } else {
            if (!text(value, 128, true)) return {};
            if (key == QStringLiteral("uid2Hex")) {
                const QString string = value.toString();
                if (string.size() % 2) return {};
                for (const QChar character : string)
                    if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
                          || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return {};
            }
        }
        uid.insert(key, value);
    }
    basis.insert(QStringLiteral("providedUidEvidence"), uid);
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(basis).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

bool FleetCapabilityEvidence::contextValid(const QVariantMap& context)
{
    const QVariant id = context.value(QStringLiteral("vehicleId"));
    return context.size() == 6 && uuid(context.value(QStringLiteral("registryEpoch")))
        && uuid(context.value(QStringLiteral("vehicleRecordEpoch")))
        && boundedInteger(id, 1, 255)
        && decimal(context.value(QStringLiteral("capabilityRevision")))
        && sha256(context.value(QStringLiteral("identityFingerprint")))
        && sha256(context.value(QStringLiteral("configurationFingerprint")));
}

bool FleetCapabilityEvidence::approvalValid(const QVariantMap& approval)
{
    const QVariant version = approval.value(QStringLiteral("schemaVersion"));
    const QVariant time = approval.value(QStringLiteral("approvedAtUtc"));
    if (approval.size() != 14 || !boundedInteger(version, 1, 1)
        || !uuid(approval.value(QStringLiteral("approvalId"))) || !uuid(approval.value(QStringLiteral("registryEpoch")))
        || !uuid(approval.value(QStringLiteral("vehicleRecordEpoch")))
        || !decimal(approval.value(QStringLiteral("capabilityRevision")))
        || !sha256(approval.value(QStringLiteral("identityFingerprint")))
        || !sha256(approval.value(QStringLiteral("configurationFingerprint")))
        || !canonicalApprovalCapabilities(approval.value(QStringLiteral("capabilities")), nullptr)
        || !text(approval.value(QStringLiteral("source")), 64)
        || approval.value(QStringLiteral("source")).toString() != QStringLiteral("local-engineering-attestation")
        || !text(approval.value(QStringLiteral("reason")), 1024)
        || approval.value(QStringLiteral("reason")).toString().trimmed().isEmpty()
        || !text(time, 32) || !text(approval.value(QStringLiteral("state")), 16)
        || !falseFlag(approval.value(QStringLiteral("hardwareIdentityAuthenticated")))
        || !falseFlag(approval.value(QStringLiteral("flightCommandReleased")))) return false;
    const QString state = approval.value(QStringLiteral("state")).toString();
    if (state != QStringLiteral("current") && state != QStringLiteral("superseded") && state != QStringLiteral("revoked")) return false;
    const QDateTime parsed = QDateTime::fromString(time.toString(), Qt::ISODateWithMs);
    return parsed.isValid() && parsed.toUTC().toString(Qt::ISODateWithMs) == time.toString();
}

bool FleetCapabilityEvidence::currentCapabilities(const QVariantMap& snapshot, QStringList* result, QVariantMap* identity)
{
    if (result) result->clear();
    if (identity) identity->clear();
    QVariantMap metadata, context, approval;
    int id = 0;
    if (!metadataFor(snapshot, &metadata) || !snapshotVehicleId(snapshot, metadata, &id)
        || !selectedRecord(snapshot, metadata, QStringLiteral("capabilityReviewContext"), false, &context)
        || !selectedRecord(snapshot, metadata, QStringLiteral("capabilityApproval"), true, &approval)
        || !contextValid(context) || !approvalValid(approval)
        || context.value(QStringLiteral("vehicleId")).toInt() != id
        || approval.value(QStringLiteral("state")).toString() != QStringLiteral("current")) return false;
    for (const QString& key : {QStringLiteral("registryEpoch"), QStringLiteral("vehicleRecordEpoch"),
            QStringLiteral("capabilityRevision"), QStringLiteral("identityFingerprint"), QStringLiteral("configurationFingerprint")})
        if (approval.value(key).userType() != context.value(key).userType() || approval.value(key) != context.value(key)) return false;
    QVariant configuration;
    if (!selected(snapshot, metadata, QStringLiteral("configurationFingerprint"), &configuration)
        || !sha256(configuration) || configuration != context.value(QStringLiteral("configurationFingerprint"))
        || identityFingerprint(snapshot) != context.value(QStringLiteral("identityFingerprint")).toString()) return false;
    QStringList capabilities;
    if (!canonicalApprovalCapabilities(approval.value(QStringLiteral("capabilities")), &capabilities)) return false;
    if (result) *result = capabilities;
    if (identity) *identity = stableReviewIdentity(snapshot);
    return true;
}

bool FleetCapabilityEvidence::currentDeclaredPerformance(const QVariantMap& snapshot, QVariantMap* result)
{
    if (!result) return false;
    const QVariantMap actualSnapshot = snapshot;
    QVariantMap evidence;
    // Consume and independently own ALL input-derived output before clearing
    // the destination: it may be the source map or a pre-held nested map.
    const bool valid = [&]() {
        QVariantMap metadata, reviewIdentity, baselineIdentity;
        QStringList capabilities;
        int id = 0;
        if (!metadataFor(actualSnapshot, &metadata) || !snapshotVehicleId(actualSnapshot, metadata, &id)
            || actualSnapshot.contains(QStringLiteral("enduranceSec")) || metadata.contains(QStringLiteral("enduranceSec"))
            || !currentCapabilities(actualSnapshot, &capabilities, &reviewIdentity)
            || !currentConfigurationBaseline(actualSnapshot, &baselineIdentity)
            || reviewIdentity.value(QStringLiteral("capabilityReviewContext"))
                != baselineIdentity.value(QStringLiteral("registryContext"))) return false;
        const auto canonicalNumber = [&](const QString& key, bool endurance, double* output) {
            QVariant value;
            bool present = false;
            if (!selected(actualSnapshot, metadata, key, &value, &present)) return false;
            if (!present) { *output = -1.0; return true; }
            if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
                && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
                && value.type() != QVariant::Double)) return false;
            bool ok = false;
            const double number = value.toDouble(&ok);
            if (!ok || !std::isfinite(number) || std::abs(number) > 9007199254740991.0
                || (number < 0.0 && number != -1.0) || (endurance && number == 0.0)) return false;
            *output = number;
            return true;
        };
        double speed = -1.0, endurance = -1.0;
        if (!canonicalNumber(QStringLiteral("maxSpeedMps"), false, &speed)
            || !canonicalNumber(QStringLiteral("enduranceSeconds"), true, &endurance)
            || (speed < 0.0 && endurance < 0.0)) return false;
        evidence = {{QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("source"), QStringLiteral("current-local-engineering-declared-performance")},
            {QStringLiteral("vehicleId"), id}, {QStringLiteral("maxSpeedMps"), speed},
            {QStringLiteral("enduranceSeconds"), endurance},
            {QStringLiteral("capabilityReviewIdentity"), ownedPerformanceIdentity(reviewIdentity)},
            {QStringLiteral("configurationBaselineIdentity"), ownedPerformanceIdentity(baselineIdentity)},
            {QStringLiteral("performanceCertified"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
            {QStringLiteral("executionPermitted"), false}, {QStringLiteral("flightCommandReleased"), false}};
        return true;
    }();
    result->clear();
    if (!valid) return false;
    *result = evidence;
    return true;
}

QVariantMap FleetCapabilityEvidence::stableReviewIdentity(const QVariantMap& snapshot)
{
    QVariantMap metadata, context, approval;
    const bool valid = metadataFor(snapshot, &metadata)
        && selectedRecord(snapshot, metadata, QStringLiteral("capabilityReviewContext"), false, &context)
        && selectedRecord(snapshot, metadata, QStringLiteral("capabilityApproval"), true, &approval);
    if (!valid) { context.clear(); approval.clear(); }
    // Keep superseded/revoked IDs and revisions for ABA detection. UTC/reason
    // text is not an authorization identity and cannot make this hash churn.
    approval.remove(QStringLiteral("reason"));
    approval.remove(QStringLiteral("approvedAtUtc"));
    return {{QStringLiteral("capabilityReviewContext"), context},
            {QStringLiteral("capabilityApproval"), approval}};
}

bool FleetCapabilityEvidence::currentConfigurationBaseline(const QVariantMap& snapshot, QVariantMap* identity)
{
    // Copy first so an output alias cannot erase the input before validation.
    const QVariantMap actualSnapshot = snapshot;
    if (identity) identity->clear();
    QVariantMap evidence;
    if (baselineRecord(actualSnapshot, &evidence) != BaselineRecordState::Available
        || !contextValid(evidence.value(QStringLiteral("registryContext")).toMap())
        || !evidence.value(QStringLiteral("currentFullKnown")).toBool()
        || !evidence.value(QStringLiteral("baselineFullKnown")).toBool()
        || !evidence.value(QStringLiteral("baselineMatches")).toBool()) return false;
    // Do not re-hash the raw configuration after a JS/JSON round trip: it may
    // legitimately have lost native QVariant integer/bytes/date/null types.
    // The actual native producer and final owner fences verify original data.
    if (identity) *identity = evidence;
    return true;
}

QVariantMap FleetCapabilityEvidence::stableConfigurationBaselineIdentity(const QVariantMap& snapshot)
{
    QVariantMap evidence;
    const BaselineRecordState state = baselineRecord(snapshot, &evidence);
    const QString label = state == BaselineRecordState::Missing ? QStringLiteral("missing")
        : state == BaselineRecordState::Available ? QStringLiteral("available") : QStringLiteral("invalid");
    if (state != BaselineRecordState::Available) evidence.clear();
    // Valid negative values retain original incarnation/revision and hashes;
    // missing/malformed values are fixed, small identities, never raw input.
    return {{QStringLiteral("state"), label}, {QStringLiteral("evidence"), evidence}};
}

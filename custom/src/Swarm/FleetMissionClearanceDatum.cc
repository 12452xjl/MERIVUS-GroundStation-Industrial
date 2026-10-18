#include "FleetMissionClearanceDatum.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionClearanceSource.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetRegistry.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QMetaType>
#include <QPointer>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <initializer_list>

namespace {
constexpr qint64 MaximumExactInteger = Q_INT64_C(9007199254740991);
constexpr qint64 MaximumBytes = 512 * 1024;
constexpr int MaximumNodes = 16000;
constexpr qint64 VersionAgeMs = 30000;

bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}
bool mapValue(const QVariant& value) { return value.userType() == QMetaType::QVariantMap && !value.isNull(); }
bool listValue(const QVariant& value) { return value.userType() == QMetaType::QVariantList && !value.isNull(); }
bool flag(const QVariantMap& map, const char* key, bool expected)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.userType() == QMetaType::Bool && !value.isNull() && value.toBool() == expected;
}
bool nativeInt(const QVariant& value, int expected)
{
    return value.userType() == QMetaType::Int && !value.isNull() && value.toInt() == expected;
}
bool nativeUInt(const QVariant& value, uint expected)
{
    return value.userType() == QMetaType::UInt && !value.isNull() && value.toUInt() == expected;
}
bool text(const QVariant& value, int maximum = 128)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    return true;
}
bool literal(const QVariant& value, const char* expected)
{
    return value.userType() == QMetaType::QString && !value.isNull() && value.toString() == QString::fromLatin1(expected);
}
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool decimal(const QVariant& value, bool positive)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    for (QChar ch : string) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 encoded = string.toULongLong(&ok);
    return ok && (!positive || encoded != 0) && QString::number(encoded) == string;
}
bool utc(const QVariant& value, qint64* output)
{
    if (!text(value, 24) || value.toString().size() != 24 || !value.toString().endsWith(QLatin1Char('Z'))) return false;
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!parsed.isValid() || parsed.toUTC().toString(Qt::ISODateWithMs) != value.toString()) return false;
    const qint64 number = parsed.toMSecsSinceEpoch();
    if (number < -MaximumExactInteger || number > MaximumExactInteger) return false;
    *output = number;
    return true;
}
// This is a small retained native envelope, not an arbitrary evidence importer.
// Keep type/null identity; in particular Catalog's original empty errorCode
// may be a null QString. Never round-trip the original through JSON here.
bool tree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 12 || ++*nodes > MaximumNodes) return false;
    if (value.userType() == QMetaType::QVariantMap && !value.isNull()) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 128) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumBytes || !tree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.userType() == QMetaType::QVariantList && !value.isNull()) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes - *nodes) return false;
        for (const QVariant& item : list) if (!tree(item, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.userType() == QMetaType::QString) {
        const QString string = value.toString();
        if (string.size() > MaximumBytes - *bytes) return false;
        *bytes += string.toUtf8().size();
        return *bytes <= MaximumBytes;
    }
    if (value.isNull()) return false;
    if (value.userType() == QMetaType::Bool || value.userType() == QMetaType::Int || value.userType() == QMetaType::UInt) return true;
    if (value.userType() == QMetaType::LongLong) return value.toLongLong() >= -MaximumExactInteger && value.toLongLong() <= MaximumExactInteger;
    if (value.userType() == QMetaType::ULongLong) return value.toULongLong() <= quint64(MaximumExactInteger);
    if (value.userType() == QMetaType::Double) return std::isfinite(value.toDouble()) && std::fabs(value.toDouble()) <= double(MaximumExactInteger);
    return false;
}
bool bounded(const QVariantMap& value)
{
    int nodes = 0; qint64 bytes = 0;
    return tree(value, 0, &nodes, &bytes);
}
bool sameTyped(const QVariant& first, const QVariant& second)
{
    if (first.userType() != second.userType() || first.isNull() != second.isNull()) return false;
    if (first.userType() == QMetaType::QVariantMap) {
        const QVariantMap a = first.toMap(), b = second.toMap();
        if (a.size() != b.size()) return false;
        for (auto it = a.constBegin(); it != a.constEnd(); ++it)
            if (!b.contains(it.key()) || !sameTyped(it.value(), b.value(it.key()))) return false;
        return true;
    }
    if (first.userType() == QMetaType::QVariantList) {
        const QVariantList a = first.toList(), b = second.toList();
        if (a.size() != b.size()) return false;
        for (int index = 0; index < a.size(); ++index) if (!sameTyped(a[index], b[index])) return false;
        return true;
    }
    return first == second;
}
QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}
QVariantMap response(bool provided, bool valid, const QString& reason, const QVariantMap& evidence = {})
{
    return {{QStringLiteral("provided"), provided}, {QStringLiteral("inputValid"), valid},
        {QStringLiteral("declaredDatumCorrespondenceMatches"), !evidence.isEmpty()},
        {QStringLiteral("reason"), reason}, {QStringLiteral("evidence"), evidence}};
}

struct Source {
    QVariantList rows;
    QMap<int, QVariantMap> configurations;
    qint64 acquired = 0;
    qint64 until = 0;
};
bool sourceKnown(const QVariantMap& value, Source* output)
{
    if (!bounded(value) || !exact(value, {"schemaVersion", "source", "inputFingerprint", "sceneId", "sceneRevision",
            "sceneSourceDocumentHash", "acquiredAtUtc", "validUntilUtc", "horizontalDatum", "verticalDatumId", "members",
            "snapshotOnly", "physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated",
            "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        || !nativeInt(value.value(QStringLiteral("schemaVersion")), 1)
        || !literal(value.value(QStringLiteral("source")), "native-local-configuration-and-time-window")
        || !digest(value.value(QStringLiteral("inputFingerprint"))) || !text(value.value(QStringLiteral("sceneId")))
        || !decimal(value.value(QStringLiteral("sceneRevision")), true) || !digest(value.value(QStringLiteral("sceneSourceDocumentHash")))
        || !literal(value.value(QStringLiteral("horizontalDatum")), "wgs84") || !text(value.value(QStringLiteral("verticalDatumId")))
        || !flag(value, "snapshotOnly", true) || !listValue(value.value(QStringLiteral("members")))) return false;
    for (const char* key : {"physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated",
            "performanceCertified", "startupPermitGranted", "flightCommandReleased"}) if (!flag(value, key, false)) return false;
    Source parsed;
    if (!utc(value.value(QStringLiteral("acquiredAtUtc")), &parsed.acquired)
        || !utc(value.value(QStringLiteral("validUntilUtc")), &parsed.until) || parsed.until <= parsed.acquired) return false;
    parsed.rows = value.value(QStringLiteral("members")).toList();
    if (parsed.rows.isEmpty() || parsed.rows.size() > 32) return false;
    for (const QVariant& item : parsed.rows) {
        if (!mapValue(item)) return false;
        const QVariantMap row = item.toMap();
        const QVariant id = row.value(QStringLiteral("vehicleId"));
        if (!exact(row, {"vehicleId", "configurationEvidence"}) || id.userType() != QMetaType::Int || id.isNull()
            || id.toInt() < 1 || id.toInt() > 255 || parsed.configurations.contains(id.toInt())
            || !mapValue(row.value(QStringLiteral("configurationEvidence")))) return false;
        const QVariantMap config = row.value(QStringLiteral("configurationEvidence")).toMap();
        if (!exact(config, {"schemaVersion", "source", "vehicleId", "bindingEpoch", "registryContext", "configurationBaselineEvidence",
                "currentTypedFingerprint", "configurationFingerprint", "bindingMatchesCurrentObservation", "hardwareIdentityAuthenticated",
                "physicalEnvelopeCertified", "performanceCertified", "flightCommandReleased"})
            || !nativeInt(config.value(QStringLiteral("schemaVersion")), 1) || !nativeInt(config.value(QStringLiteral("vehicleId")), id.toInt())
            || !literal(config.value(QStringLiteral("source")), "native-current-configuration-observation")
            || !uuid(config.value(QStringLiteral("bindingEpoch"))) || !digest(config.value(QStringLiteral("currentTypedFingerprint")))
            || !digest(config.value(QStringLiteral("configurationFingerprint"))) || !flag(config, "bindingMatchesCurrentObservation", true)
            || !mapValue(config.value(QStringLiteral("registryContext"))) || config.value(QStringLiteral("registryContext")).toMap().size() != 6
            || !mapValue(config.value(QStringLiteral("configurationBaselineEvidence")))
            || config.value(QStringLiteral("configurationBaselineEvidence")).toMap().size() != 14) return false;
        for (const char* key : {"hardwareIdentityAuthenticated", "physicalEnvelopeCertified", "performanceCertified", "flightCommandReleased"})
            if (!flag(config, key, false)) return false;
        parsed.configurations.insert(id.toInt(), config);
    }
    *output = parsed;
    return true;
}
bool timeCurrent(const Source& source, qint64 now)
{
    return now > 0 && now <= MaximumExactInteger && now >= source.acquired && now < source.until;
}
bool utcFloorValid(qint64 minimum)
{
    return minimum == -1 || (minimum > 0 && minimum <= MaximumExactInteger);
}
bool catalogKnown(const QVariantMap& value)
{
    return exact(value, {"catalogEpoch", "revision", "catalogFingerprint"})
        && uuid(value.value(QStringLiteral("catalogEpoch"))) && decimal(value.value(QStringLiteral("revision")), false)
        && digest(value.value(QStringLiteral("catalogFingerprint")));
}
QVariantMap queryFor(const QVariantMap& source, const QVariantMap& configuration, const QVariantMap& origin)
{
    return {{QStringLiteral("sceneId"), source.value(QStringLiteral("sceneId"))},
        {QStringLiteral("sceneRevision"), source.value(QStringLiteral("sceneRevision"))},
        {QStringLiteral("sceneSourceDocumentHash"), source.value(QStringLiteral("sceneSourceDocumentHash"))},
        {QStringLiteral("sceneVerticalDatumId"), source.value(QStringLiteral("verticalDatumId"))},
        {QStringLiteral("configurationFingerprint"), configuration.value(QStringLiteral("configurationFingerprint"))},
        {QStringLiteral("globalOriginReferenceHash"), origin.value(QStringLiteral("referenceHash"))}};
}
bool observedOrigin(const QVariantMap& value)
{
    return flag(value, "observed", true) && flag(value, "available", true)
        && FleetMissionStartupGlobalOrigin::admissible(value);
}

struct Owner {
    QPointer<Vehicle> vehicle;
    QPointer<ParameterManager> parameters;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    int componentId = 0;
};
bool ownerCurrent(int id, const Owner& owner)
{
    if (!owner.vehicle || !owner.parameters || !owner.links || !owner.primary
        || owner.vehicle->thread() != QThread::currentThread() || owner.parameters->thread() != QThread::currentThread()
        || owner.links->thread() != QThread::currentThread() || owner.vehicle->id() != id
        || owner.vehicle->defaultComponentId() != owner.componentId || owner.componentId < 1 || owner.componentId > 255
        || owner.vehicle->parameterManager() != owner.parameters.data() || owner.vehicle->vehicleLinkManager() != owner.links.data()) return false;
    const WeakLinkInterfacePtr current = owner.links->primaryLink();
    return !current.expired() && !owner.primaryOwner.expired()
        && !current.owner_before(owner.primaryOwner) && !owner.primaryOwner.owner_before(current);
}
bool ownersFor(const Source& source, const QMap<int, Vehicle*>& actualVehicles, QMap<int, Owner>* output)
{
    if (actualVehicles.size() != source.rows.size()) return false;
    QMap<int, Owner> owners;
    for (auto it = actualVehicles.constBegin(); it != actualVehicles.constEnd(); ++it) {
        if (!it.value() || !source.configurations.contains(it.key())) return false;
        Owner owner;
        owner.vehicle = it.value();
        if (owner.vehicle->thread() != QThread::currentThread() || owner.vehicle->id() != it.key()) return false;
        owner.parameters = owner.vehicle->parameterManager(); owner.links = owner.vehicle->vehicleLinkManager();
        owner.componentId = owner.vehicle->defaultComponentId();
        if (!owner.links || owner.links->thread() != QThread::currentThread()) return false;
        owner.primaryOwner = owner.links->primaryLink();
        // Resolve the actual VERSION-bound PRIMARY without promoting a weak
        // owner: releasing a temporary strong link must not be a hidden
        // destruction/callback boundary in a supposedly memory-only gate.
        owner.primary = owner.vehicle->autopilotVersionObservationPrimaryLinkMemory();
        if (!ownerCurrent(it.key(), owner)) return false;
        owners.insert(it.key(), owner);
    }
    *output = owners;
    return true;
}
bool ownersCurrent(const QPointer<FleetRegistry>& registry, const QPointer<FleetFirmwareAdapterCatalog>& catalog,
    const QMap<int, Owner>& owners)
{
    if (!registry || !catalog || registry->thread() != QThread::currentThread() || catalog->thread() != QThread::currentThread()) return false;
    for (auto it = owners.constBegin(); it != owners.constEnd(); ++it) if (!ownerCurrent(it.key(), it.value())) return false;
    return true;
}

bool correspondenceKnown(const QVariantMap& value, const QVariantMap& query, const QVariantMap& catalogBinding,
    int id, int componentId, QVariantMap* observationBinding)
{
    if (!exact(value, {"schemaVersion", "source", "firmwareBinding", "relation", "relationFingerprint",
            "declaredCorrespondenceMatches", "physicalDatumAlignmentCertified", "firmwareAdapterCertified", "startupPermitGranted", "flightCommandReleased"})
        || !nativeInt(value.value(QStringLiteral("schemaVersion")), 1)
        || !literal(value.value(QStringLiteral("source")), "native-local-declared-datum-correspondence")
        || !flag(value, "declaredCorrespondenceMatches", true) || !digest(value.value(QStringLiteral("relationFingerprint")))
        || !mapValue(value.value(QStringLiteral("firmwareBinding"))) || !mapValue(value.value(QStringLiteral("relation")))) return false;
    for (const char* key : {"physicalDatumAlignmentCertified", "firmwareAdapterCertified", "startupPermitGranted", "flightCommandReleased"})
        if (!flag(value, key, false)) return false;
    const QVariantMap relation = value.value(QStringLiteral("relation")).toMap();
    if (!exact(relation, {"relationId", "sceneId", "sceneRevision", "sceneSourceDocumentHash", "sceneVerticalDatumId",
            "configurationFingerprint", "globalOriginReferenceHash", "reportedAltitudeSource", "reportedAltitudeDatumId",
            "offsetMm", "relationModel", "engineeringReview"})
        || !text(relation.value(QStringLiteral("relationId")))
        || !literal(relation.value(QStringLiteral("reportedAltitudeSource")), "global-position-int-alt")
        || !literal(relation.value(QStringLiteral("relationModel")), "engineer-declared-same-datum-zero-offset")
        || !nativeInt(relation.value(QStringLiteral("offsetMm")), 0)
        || !sameTyped(relation.value(QStringLiteral("reportedAltitudeDatumId")), query.value(QStringLiteral("sceneVerticalDatumId")))
        || !mapValue(relation.value(QStringLiteral("engineeringReview"))) || relation.value(QStringLiteral("engineeringReview")).toMap().size() != 4
        || value.value(QStringLiteral("relationFingerprint")).toString() != hash(relation)) return false;
    for (auto it = query.constBegin(); it != query.constEnd(); ++it) if (!sameTyped(relation.value(it.key()), it.value())) return false;
    const QVariantMap firmware = value.value(QStringLiteral("firmwareBinding")).toMap();
    if (!exact(firmware, {"ok", "errorCode", "firmwareAdapterCertified", "signatureVerified", "authenticated",
            "executionPermitted", "startPermitted", "executable", "flightCommandReleased", "executionVerified", "sourceCommitProven",
            "hardwareIdentityCertified", "matched", "source", "descriptorId", "descriptorHash", "commandApprovalRestored",
            "catalogEpoch", "revision", "catalogFingerprint", "observationBinding", "adapterId", "sourceCommit", "sourceRepository",
            "engineeringReview", "declarationReview", "restoredFromArchive", "customVersionEncoding", "encodedGitPrefix", "vendorVersion",
            "matchMeaning", "reasons", "fingerprint"})
        || !flag(firmware, "ok", true) || !flag(firmware, "matched", true)
        || !literal(firmware.value(QStringLiteral("source")), "local-engineering-declaration")
        || firmware.value(QStringLiteral("errorCode")).userType() != QMetaType::QString
        || !firmware.value(QStringLiteral("errorCode")).toString().isEmpty()
        || !listValue(firmware.value(QStringLiteral("reasons"))) || !firmware.value(QStringLiteral("reasons")).toList().isEmpty()
        || !mapValue(firmware.value(QStringLiteral("observationBinding"))) || !digest(firmware.value(QStringLiteral("fingerprint")))) return false;
    for (auto it = catalogBinding.constBegin(); it != catalogBinding.constEnd(); ++it)
        if (!sameTyped(firmware.value(it.key()), it.value())) return false;
    for (const char* key : {"firmwareAdapterCertified", "signatureVerified", "authenticated", "executionPermitted", "startPermitted",
            "executable", "flightCommandReleased", "executionVerified", "sourceCommitProven", "hardwareIdentityCertified", "commandApprovalRestored"})
        if (!flag(firmware, key, false)) return false;
    QVariantMap unhashed = firmware; unhashed.remove(QStringLiteral("fingerprint"));
    if (firmware.value(QStringLiteral("fingerprint")).toString() != hash(unhashed)) return false;
    const QVariantMap observed = firmware.value(QStringLiteral("observationBinding")).toMap();
    if (!exact(observed, {"vehicleId", "componentId", "firmwareType", "vehicleType", "source", "sourceEpoch", "contentRevision", "contentHash",
            "uidKnown", "uid2Known", "missionIntReported", "observedFirmware"})
        || !nativeUInt(observed.value(QStringLiteral("vehicleId")), uint(id))
        || !nativeUInt(observed.value(QStringLiteral("componentId")), uint(componentId))
        || !literal(observed.value(QStringLiteral("source")), "live-autopilot-version")
        || !uuid(observed.value(QStringLiteral("sourceEpoch"))) || !decimal(observed.value(QStringLiteral("contentRevision")), true)
        || !digest(observed.value(QStringLiteral("contentHash"))) || !flag(observed, "missionIntReported", true)
        || !mapValue(observed.value(QStringLiteral("observedFirmware"))) || observed.value(QStringLiteral("observedFirmware")).toMap().size() != 6) return false;
    *observationBinding = observed;
    return true;
}

bool datumKnown(const QVariantMap& value, const QVariantMap& sourceValue, const Source& source,
    const QMap<int, Owner>& owners)
{
    if (!bounded(value) || !exact(value, {"schemaVersion", "source", "inputFingerprint", "catalogBinding", "members",
            "declaredDatumCorrespondenceMatches", "sameDatumZeroOffsetDeclared", "physicalDatumAlignmentCertified", "currentGlobalAltitudeBranchProven",
            "sameEstimatorSampleProven", "hardwareIdentityAuthenticated", "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        || !nativeInt(value.value(QStringLiteral("schemaVersion")), 1)
        || !literal(value.value(QStringLiteral("source")), "native-local-declared-clearance-datum")
        || !sameTyped(value.value(QStringLiteral("inputFingerprint")), sourceValue.value(QStringLiteral("inputFingerprint")))
        || !mapValue(value.value(QStringLiteral("catalogBinding"))) || !catalogKnown(value.value(QStringLiteral("catalogBinding")).toMap())
        || !listValue(value.value(QStringLiteral("members"))) || !flag(value, "declaredDatumCorrespondenceMatches", true)
        || !flag(value, "sameDatumZeroOffsetDeclared", true)) return false;
    for (const char* key : {"physicalDatumAlignmentCertified", "currentGlobalAltitudeBranchProven", "sameEstimatorSampleProven",
            "hardwareIdentityAuthenticated", "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        if (!flag(value, key, false)) return false;
    const QVariantList rows = value.value(QStringLiteral("members")).toList();
    if (rows.size() != source.rows.size() || owners.size() != rows.size()) return false;
    const QVariantMap catalogBinding = value.value(QStringLiteral("catalogBinding")).toMap();
    for (int index = 0; index < rows.size(); ++index) {
        if (!mapValue(rows[index])) return false;
        const int id = source.rows[index].toMap().value(QStringLiteral("vehicleId")).toInt();
        const QVariantMap row = rows[index].toMap();
        if (!exact(row, {"vehicleId", "configurationEvidence", "originReference", "datumCorrespondence"})
            || !nativeInt(row.value(QStringLiteral("vehicleId")), id) || !owners.contains(id)
            || !sameTyped(row.value(QStringLiteral("configurationEvidence")), source.configurations.value(id))
            || !mapValue(row.value(QStringLiteral("originReference"))) || !mapValue(row.value(QStringLiteral("datumCorrespondence")))) return false;
        const QVariantMap origin = row.value(QStringLiteral("originReference")).toMap();
        if (!observedOrigin(origin) || !nativeInt(origin.value(QStringLiteral("vehicleId")), id)
            || !nativeInt(origin.value(QStringLiteral("componentId")), owners.value(id).componentId)) return false;
        QVariantMap observed;
        if (!correspondenceKnown(row.value(QStringLiteral("datumCorrespondence")).toMap(), queryFor(sourceValue, source.configurations.value(id), origin),
                catalogBinding, id, owners.value(id).componentId, &observed)) return false;
    }
    return true;
}

bool memoryPass(const QVariantMap& datum, const Source& source, const QPointer<FleetRegistry>& registry,
    const QPointer<FleetFirmwareAdapterCatalog>& catalog, const QMap<int, Owner>& owners)
{
    if (!ownersCurrent(registry, catalog, owners)
        || !sameTyped(catalog->datumBindingMemory(), datum.value(QStringLiteral("catalogBinding")))) return false;
    const QVariantList rows = datum.value(QStringLiteral("members")).toList();
    for (const QVariant& rowValue : rows) {
        const QVariantMap row = rowValue.toMap();
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        const Owner owner = owners.value(id);
        if (!ownerCurrent(id, owner) || !registry->nativeConfigurationEvidenceMemoryCurrent(owner.vehicle.data(), source.configurations.value(id))) return false;
        const QVariantMap observed = row.value(QStringLiteral("datumCorrespondence")).toMap()
            .value(QStringLiteral("firmwareBinding")).toMap().value(QStringLiteral("observationBinding")).toMap();
        if (!owner.vehicle->autopilotVersionObservationMemoryCurrent(owner.primary.data(), observed.value(QStringLiteral("sourceEpoch")).toString(),
                observed.value(QStringLiteral("contentRevision")).toString(), observed.value(QStringLiteral("contentHash")).toString(), VersionAgeMs)) return false;
        // available is computed by the native getter's memory-only PRIMARY,
        // VERSION/flight/epoch check. Do not call SourceMatches here, so this
        // last group pass does not depend on its callback policy.
        QVariantMap origin;
        if (!FleetMissionStartupGlobalOrigin::capture(owner.vehicle->startupGlobalOriginObservation(), id, owner.componentId, &origin)
            || !observedOrigin(origin) || !sameTyped(row.value(QStringLiteral("originReference")), origin)
            || !FleetMissionStartupGlobalOrigin::same(row.value(QStringLiteral("originReference")).toMap(), origin)) return false;
    }
    return ownersCurrent(registry, catalog, owners);
}
} // namespace

QVariantMap FleetMissionClearanceDatum::capture(const QVariantMap& originalClearanceSourceEvidence,
    FleetRegistry* actualRegistry, FleetFirmwareAdapterCatalog* actualCatalog,
    const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence,
    qint64 minimumObservedUtcMs, qint64* observedAtUtcMs)
{
    if (observedAtUtcMs) *observedAtUtcMs = -1;
    const QVariantMap original = originalClearanceSourceEvidence;
    if (original.isEmpty()) return response(false, true, QStringLiteral(""));
    if (!utcFloorValid(minimumObservedUtcMs)) return response(true, false, QStringLiteral("clearance-datum-utc-floor-invalid"));
    const QMap<int, Vehicle*> suppliedVehicles = actualVehicles;
    Source source;
    if (!sourceKnown(original, &source)) return response(true, false, QStringLiteral("clearance-datum-source-evidence-invalid"));
    const auto reject = [](const QString& reason) { return response(true, true, reason); };
    const QPointer<FleetRegistry> registry(actualRegistry);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(actualCatalog);
    const std::function<bool()> sourceFence = fence;
    QMap<int, Owner> owners;
    if (!ownersFor(source, suppliedVehicles, &owners) || !ownersCurrent(registry, catalog, owners))
        return reject(QStringLiteral("clearance-datum-native-owners-unavailable"));
    const QVariantMap catalogBinding = catalog->datumBindingMemory();
    if (!catalogKnown(catalogBinding)) return reject(QStringLiteral("clearance-datum-catalog-binding-unavailable"));
    const auto fenced = [&]() {
        if (!ownersCurrent(registry, catalog, owners)) return false;
        if (sourceFence && !sourceFence()) return false;
        return ownersCurrent(registry, catalog, owners) && sameTyped(catalog->datumBindingMemory(), catalogBinding);
    };
    const qint64 beforeUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (beforeUtc < minimumObservedUtcMs || !timeCurrent(source, beforeUtc) || !fenced())
        return reject(QStringLiteral("clearance-datum-source-or-time-unavailable"));
    // Original source18, not a newly sampled replacement of its configuration.
    const QVariantMap configuration = FleetMissionClearanceSource::recapture(original, registry.data(), suppliedVehicles, fenced);
    if (!fenced()) return reject(QStringLiteral("clearance-datum-source-changed"));
    if (flag(configuration, "inputValid", false))
        return response(true, false, QStringLiteral("clearance-datum-source-evidence-invalid"));
    if (!exact(configuration, {"provided", "inputValid", "configurationAndTimeCurrent", "reason", "evidence", "remainingValidityMs", "observedAtUtcMs"})
        || !flag(configuration, "provided", true) || !flag(configuration, "inputValid", true)
        || !flag(configuration, "configurationAndTimeCurrent", true) || !sameTyped(configuration.value(QStringLiteral("evidence")), original)
        || configuration.value(QStringLiteral("observedAtUtcMs")).userType() != QMetaType::LongLong
        || configuration.value(QStringLiteral("observedAtUtcMs")).isNull())
        return reject(QStringLiteral("clearance-datum-original-configuration-or-time-unavailable"));
    const qint64 sourceObservedUtc = configuration.value(QStringLiteral("observedAtUtcMs")).toLongLong();
    if (sourceObservedUtc < beforeUtc || !timeCurrent(source, sourceObservedUtc))
        return reject(QStringLiteral("clearance-datum-utc-changed-or-expired"));
    QVariantList members;
    for (const QVariant& sourceRow : source.rows) {
        const int id = sourceRow.toMap().value(QStringLiteral("vehicleId")).toInt();
        if (!fenced()) return reject(QStringLiteral("clearance-datum-source-changed"));
        const Owner owner = owners.value(id);
        const QVariantMap version = owner.vehicle->autopilotVersionObservation(VersionAgeMs);
        if (!fenced()) return reject(QStringLiteral("clearance-datum-version-source-changed"));
        const bool versionSourceMatches = owner.vehicle->autopilotVersionObservationSourceMatches(owner.primary.data());
        if (!fenced() || !versionSourceMatches) return reject(QStringLiteral("clearance-datum-version-source-unavailable"));
        // Treat the source-check API as a callback boundary regardless of
        // its current implementation; never dereference Vehicle afterwards
        // without first checking every frozen owner and the caller's fence.
        const bool originSourceMatches = owner.vehicle->startupGlobalOriginObservationSourceMatches(owner.primary.data());
        if (!fenced() || !originSourceMatches) return reject(QStringLiteral("clearance-datum-observed-global-origin-unavailable"));
        const QVariantMap originObservation = owner.vehicle->startupGlobalOriginObservation();
        if (!fenced()) return reject(QStringLiteral("clearance-datum-origin-source-changed"));
        QVariantMap origin;
        if (!FleetMissionStartupGlobalOrigin::capture(originObservation, id, owner.componentId, &origin)
            || !observedOrigin(origin)) return reject(QStringLiteral("clearance-datum-observed-global-origin-unavailable"));
        const QVariantMap query = queryFor(original, source.configurations.value(id), origin);
        const QVariantMap matched = catalog->matchDatumRelation(version, query);
        if (!fenced()) return reject(QStringLiteral("clearance-datum-catalog-or-source-changed"));
        if (!exact(matched, {"ok", "reason", "evidence"}) || !flag(matched, "ok", true)
            || !literal(matched.value(QStringLiteral("reason")), "") || !mapValue(matched.value(QStringLiteral("evidence"))))
            return reject(QStringLiteral("clearance-datum-no-matching-engineering-declaration"));
        const QVariantMap correspondence = matched.value(QStringLiteral("evidence")).toMap();
        QVariantMap observationBinding;
        if (!bounded(correspondence) || !correspondenceKnown(correspondence, query, catalogBinding, id, owner.componentId, &observationBinding))
            return reject(QStringLiteral("clearance-datum-native-correspondence-invalid"));
        members.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("configurationEvidence"), source.configurations.value(id)},
            {QStringLiteral("originReference"), origin}, {QStringLiteral("datumCorrespondence"), correspondence}});
    }
    const QVariantMap evidence{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("source"), QStringLiteral("native-local-declared-clearance-datum")},
        {QStringLiteral("inputFingerprint"), original.value(QStringLiteral("inputFingerprint"))}, {QStringLiteral("catalogBinding"), catalogBinding},
        {QStringLiteral("members"), members}, {QStringLiteral("declaredDatumCorrespondenceMatches"), true}, {QStringLiteral("sameDatumZeroOffsetDeclared"), true},
        {QStringLiteral("physicalDatumAlignmentCertified"), false}, {QStringLiteral("currentGlobalAltitudeBranchProven"), false},
        {QStringLiteral("sameEstimatorSampleProven"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (!datumKnown(evidence, original, source, owners)) return reject(QStringLiteral("clearance-datum-evidence-shape-invalid"));
    // Finish all externally callable fences BEFORE the entire group memory
    // pass: a later vehicle's callback cannot leave an earlier stale prefix.
    if (!fenced() || !memoryPass(evidence, source, registry, catalog, owners))
        return reject(QStringLiteral("clearance-datum-native-source-changed-at-tail"));
    const qint64 afterUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (afterUtc < sourceObservedUtc || !timeCurrent(source, afterUtc)) return reject(QStringLiteral("clearance-datum-utc-changed-or-expired"));
    if (observedAtUtcMs) *observedAtUtcMs = afterUtc;
    return response(true, true, QStringLiteral(""), evidence);
}

QVariantMap FleetMissionClearanceDatum::recapture(const QVariantMap& originalDatumEvidence,
    const QVariantMap& originalClearanceSourceEvidence, FleetRegistry* actualRegistry,
    FleetFirmwareAdapterCatalog* actualCatalog, const QMap<int, Vehicle*>& actualVehicles,
    const std::function<bool()>& fence, qint64 minimumObservedUtcMs, qint64* observedAtUtcMs)
{
    if (observedAtUtcMs) *observedAtUtcMs = -1;
    const QVariantMap original = originalDatumEvidence;
    const QVariantMap source = originalClearanceSourceEvidence;
    if (source.isEmpty()) return original.isEmpty() ? response(false, true, QStringLiteral(""))
        : response(true, false, QStringLiteral("clearance-datum-unexpected-evidence-without-source"));
    if (!utcFloorValid(minimumObservedUtcMs)) return response(true, false, QStringLiteral("clearance-datum-utc-floor-invalid"));
    const QMap<int, Vehicle*> suppliedVehicles = actualVehicles;
    Source parsed;
    QMap<int, Owner> owners;
    if (!sourceKnown(source, &parsed) || !ownersFor(parsed, suppliedVehicles, &owners)
        || !datumKnown(original, source, parsed, owners)) return response(true, false, QStringLiteral("clearance-datum-original-evidence-invalid"));
    qint64 currentObservedUtc = -1;
    const QVariantMap current = capture(source, actualRegistry, actualCatalog, suppliedVehicles, fence,
        minimumObservedUtcMs, &currentObservedUtc);
    if (!flag(current, "declaredDatumCorrespondenceMatches", true)) return current;
    if (!sameTyped(current.value(QStringLiteral("evidence")), original))
        return response(true, true, QStringLiteral("clearance-datum-original-correspondence-changed"));
    if (observedAtUtcMs) *observedAtUtcMs = currentObservedUtc;
    return response(true, true, QStringLiteral(""), original);
}

bool FleetMissionClearanceDatum::memoryCurrent(const QVariantMap& originalDatumEvidence,
    const QVariantMap& originalClearanceSourceEvidence, FleetRegistry* actualRegistry,
    FleetFirmwareAdapterCatalog* actualCatalog, const QMap<int, Vehicle*>& actualVehicles,
    qint64 minimumObservedUtcMs, qint64* observedAtUtcMs)
{
    if (observedAtUtcMs) *observedAtUtcMs = -1;
    if (originalClearanceSourceEvidence.isEmpty()) return originalDatumEvidence.isEmpty();
    if (!utcFloorValid(minimumObservedUtcMs)) return false;
    Source source;
    QMap<int, Owner> owners;
    const QPointer<FleetRegistry> registry(actualRegistry);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(actualCatalog);
    if (!sourceKnown(originalClearanceSourceEvidence, &source) || !ownersFor(source, actualVehicles, &owners)
        || !ownersCurrent(registry, catalog, owners)
        || !datumKnown(originalDatumEvidence, originalClearanceSourceEvidence, source, owners)) return false;
    const qint64 beforeUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (beforeUtc < minimumObservedUtcMs || !timeCurrent(source, beforeUtc)
        || !memoryPass(originalDatumEvidence, source, registry, catalog, owners)) return false;
    const qint64 afterUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (afterUtc < beforeUtc || !timeCurrent(source, afterUtc)) return false;
    if (observedAtUtcMs) *observedAtUtcMs = afterUtc;
    return true;
}

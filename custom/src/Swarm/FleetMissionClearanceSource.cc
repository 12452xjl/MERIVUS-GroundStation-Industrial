#include "FleetMissionClearanceSource.h"

#include "FleetMissionClearanceInput.h"
#include "FleetRegistry.h"
#include "Vehicle.h"

#include <QDateTime>
#include <QMetaType>
#include <QPointer>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <initializer_list>

namespace {
constexpr qint64 MaximumExactInteger = Q_INT64_C(9007199254740991);

bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool mapValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool flag(const QVariantMap& value, const char* key, bool expected)
{
    const QVariant field = value.value(QString::fromLatin1(key));
    return field.userType() == QMetaType::Bool && !field.isNull() && field.toBool() == expected;
}

bool text(const QVariant& value, int maximum = 128)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed()) return false;
    for (QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control
        || ch.category() == QChar::Other_Format) return false;
    return true;
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

bool positiveRevision(const QVariant& value)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    for (QChar ch : string) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 revision = string.toULongLong(&ok);
    return ok && revision != 0 && QString::number(revision) == string;
}

bool nativeInt(const QVariant& value, int expected)
{
    return value.userType() == QMetaType::Int && !value.isNull() && value.toInt() == expected;
}

bool nativeEvidence(const QVariantMap& response, int vehicleId, QVariantMap* output)
{
    if (!exact(response, {"ok", "reason", "evidence"}) || !flag(response, "ok", true)
        || response.value(QStringLiteral("reason")).userType() != QMetaType::QString
        || response.value(QStringLiteral("reason")).isNull()
        || !response.value(QStringLiteral("reason")).toString().isEmpty()
        || !mapValue(response.value(QStringLiteral("evidence")))) return false;
    const QVariantMap value = response.value(QStringLiteral("evidence")).toMap();
    if (!exact(value, {"schemaVersion", "source", "vehicleId", "bindingEpoch", "registryContext",
            "configurationBaselineEvidence", "currentTypedFingerprint", "configurationFingerprint",
            "bindingMatchesCurrentObservation", "hardwareIdentityAuthenticated", "physicalEnvelopeCertified",
            "performanceCertified", "flightCommandReleased"})
        || !nativeInt(value.value(QStringLiteral("schemaVersion")), 1)
        || !nativeInt(value.value(QStringLiteral("vehicleId")), vehicleId)
        || !text(value.value(QStringLiteral("source")))
        || value.value(QStringLiteral("source")).toString() != QStringLiteral("native-current-configuration-observation")
        || !uuid(value.value(QStringLiteral("bindingEpoch")))
        || !digest(value.value(QStringLiteral("currentTypedFingerprint")))
        || !digest(value.value(QStringLiteral("configurationFingerprint")))
        || !mapValue(value.value(QStringLiteral("registryContext")))
        || !mapValue(value.value(QStringLiteral("configurationBaselineEvidence")))
        || !flag(value, "bindingMatchesCurrentObservation", true)) return false;
    for (const char* key : {"hardwareIdentityAuthenticated", "physicalEnvelopeCertified", "performanceCertified", "flightCommandReleased"})
        if (!flag(value, key, false)) return false;
    const QVariantMap context = value.value(QStringLiteral("registryContext")).toMap();
    if (!exact(context, {"registryEpoch", "vehicleRecordEpoch", "vehicleId", "capabilityRevision", "identityFingerprint", "configurationFingerprint"})
        || !uuid(context.value(QStringLiteral("registryEpoch"))) || !uuid(context.value(QStringLiteral("vehicleRecordEpoch")))
        || !nativeInt(context.value(QStringLiteral("vehicleId")), vehicleId)
        || !positiveRevision(context.value(QStringLiteral("capabilityRevision")))
        || !digest(context.value(QStringLiteral("identityFingerprint")))
        || context.value(QStringLiteral("configurationFingerprint")) != value.value(QStringLiteral("configurationFingerprint"))) return false;
    const QVariantMap baseline = value.value(QStringLiteral("configurationBaselineEvidence")).toMap();
    if (!exact(baseline, {"schemaVersion", "source", "registryContext", "observationEpoch", "observationRevision",
            "currentFullKnown", "baselineFullKnown", "currentTypedFingerprint", "baselineTypedFingerprint", "baselineApprovalHash",
            "baselineMatches", "hardwareIdentityAuthenticated", "performanceCertified", "flightCommandReleased"})
        || !nativeInt(baseline.value(QStringLiteral("schemaVersion")), 1)
        || !text(baseline.value(QStringLiteral("source")))
        || baseline.value(QStringLiteral("source")).toString() != QStringLiteral("native-local-configuration-baseline")
        || !mapValue(baseline.value(QStringLiteral("registryContext"))) || baseline.value(QStringLiteral("registryContext")).toMap() != context
        || !uuid(baseline.value(QStringLiteral("observationEpoch"))) || !positiveRevision(baseline.value(QStringLiteral("observationRevision")))
        || !flag(baseline, "currentFullKnown", true) || !flag(baseline, "baselineFullKnown", true) || !flag(baseline, "baselineMatches", true)
        || baseline.value(QStringLiteral("currentTypedFingerprint")) != value.value(QStringLiteral("currentTypedFingerprint"))
        || baseline.value(QStringLiteral("baselineTypedFingerprint")) != value.value(QStringLiteral("currentTypedFingerprint"))
        || !digest(baseline.value(QStringLiteral("baselineApprovalHash")))) return false;
    for (const char* key : {"hardwareIdentityAuthenticated", "performanceCertified", "flightCommandReleased"})
        if (!flag(baseline, key, false)) return false;
    *output = value;
    return true;
}

QVariantMap result(bool provided, bool inputValid, const QString& reason)
{
    return {{QStringLiteral("provided"), provided}, {QStringLiteral("inputValid"), inputValid},
        {QStringLiteral("configurationAndTimeCurrent"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("evidence"), QVariantMap()}, {QStringLiteral("remainingValidityMs"), qint64(-1)},
        {QStringLiteral("observedAtUtcMs"), qint64(-1)}};
}

bool utc(const QVariant& value, qint64* milliseconds)
{
    if (!text(value, 24) || value.toString().size() != 24 || !value.toString().endsWith(QLatin1Char('Z'))) return false;
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!parsed.isValid() || parsed.toUTC().toString(Qt::ISODateWithMs) != value.toString()) return false;
    const qint64 encoded = parsed.toMSecsSinceEpoch();
    if (encoded < -MaximumExactInteger || encoded > MaximumExactInteger) return false;
    *milliseconds = encoded;
    return true;
}

bool sceneText(const QVariant& value)
{
    // Keep the accepted scene label domain identical to ClearanceInput.
    // Native UUID/hash fields retain their separate stricter validators.
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > 128 || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    return true;
}
} // namespace

QVariantMap FleetMissionClearanceSource::capture(const QVariantMap& plan, FleetRegistry* actualRegistry,
    const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence)
{
    // Value inputs and the callable are frozen before any source callback.
    const QVariantMap sourcePlan = plan;
    const QMap<int, Vehicle*> suppliedVehicles = actualVehicles;
    const std::function<bool()> sourceFence = fence;
    const QVariantMap inspected = FleetMissionClearanceInput::inspectForPlan(sourcePlan);
    const bool provided = flag(inspected, "provided", true);
    const bool valid = flag(inspected, "inputValid", true);
    if (!valid) return result(provided, false, inspected.value(QStringLiteral("reason")).toString());
    if (!provided) return result(false, true, QStringLiteral(""));
    const auto reject = [](const QString& reason) { return result(true, true, reason); };
    if (!flag(inspected, "memberIdsMatch", true) || !mapValue(inspected.value(QStringLiteral("input")))
        || !digest(inspected.value(QStringLiteral("fingerprint"))))
        return result(true, false, QStringLiteral("clearance-source-input-inspection-invalid"));
    const QVariantMap input = inspected.value(QStringLiteral("input")).toMap();
    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap();
    const QVariantList members = input.value(QStringLiteral("vehicles")).toList();
    if (members.isEmpty() || members.size() > 32 || suppliedVehicles.size() != members.size())
        return reject(QStringLiteral("clearance-source-actual-member-set-mismatch"));

    QPointer<FleetRegistry> registry(actualRegistry);
    QMap<int, QPointer<Vehicle>> vehicles;
    for (auto it = suppliedVehicles.constBegin(); it != suppliedVehicles.constEnd(); ++it) {
        if (it.key() < 1 || it.key() > 255 || !it.value()) return reject(QStringLiteral("clearance-source-actual-member-set-mismatch"));
        vehicles.insert(it.key(), QPointer<Vehicle>(it.value()));
    }
    QSet<int> expectedIds;
    for (const QVariant& member : members) {
        const int id = member.toMap().value(QStringLiteral("vehicleId")).toInt();
        if (expectedIds.contains(id) || !vehicles.contains(id)) return reject(QStringLiteral("clearance-source-actual-member-set-mismatch"));
        expectedIds.insert(id);
    }
    const auto ownersCurrent = [&]() {
        if (!registry || registry->thread() != QThread::currentThread()) return false;
        for (auto it = vehicles.constBegin(); it != vehicles.constEnd(); ++it)
            if (!it.value() || it.value()->thread() != QThread::currentThread() || it.value()->id() != it.key()) return false;
        return true;
    };
    const auto fenced = [&]() {
        if (!ownersCurrent()) return false;
        if (sourceFence && !sourceFence()) return false;
        return ownersCurrent();
    };
    qint64 acquired = 0, until = 0;
    if (!utc(scene.value(QStringLiteral("acquiredAtUtc")), &acquired)
        || !utc(scene.value(QStringLiteral("validUntilUtc")), &until) || until <= acquired)
        return result(true, false, QStringLiteral("clearance-source-scene-utc-invalid"));
    const qint64 beforeUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (beforeUtc <= 0 || beforeUtc > MaximumExactInteger || beforeUtc < acquired || beforeUtc >= until)
        return reject(QStringLiteral("clearance-source-scene-outside-current-utc-window"));
    if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-unavailable"));

    QVariantList rows;
    QMap<int, QVariantMap> originals;
    for (const QVariant& member : members) {
        if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
        const QVariantMap declaration = member.toMap();
        const int id = declaration.value(QStringLiteral("vehicleId")).toInt();
        const QPointer<Vehicle> vehicle = vehicles.value(id);
        const QVariantMap response = registry->nativeConfigurationEvidence(vehicle.data());
        if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
        QVariantMap evidence;
        if (!nativeEvidence(response, id, &evidence))
            return reject(QStringLiteral("clearance-source-vehicle-%1-native-configuration-unavailable").arg(id));
        if (evidence.value(QStringLiteral("configurationFingerprint")) != declaration.value(QStringLiteral("configurationFingerprint")))
            return reject(QStringLiteral("clearance-source-vehicle-%1-configuration-fingerprint-mismatch").arg(id));
        originals.insert(id, evidence);
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("configurationEvidence"), evidence}});
    }
    if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
    // Final whole-group checks use the original evidence and actual objects,
    // never a fresh map that silently replaces the first member's basis.
    for (const QVariant& member : members) {
        if (!ownersCurrent()) return reject(QStringLiteral("clearance-source-native-owner-changed"));
        const int id = member.toMap().value(QStringLiteral("vehicleId")).toInt();
        const QVariantMap response = registry->nativeConfigurationEvidenceCurrent(vehicles.value(id).data(), originals.value(id));
        if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
        QVariantMap current;
        if (!nativeEvidence(response, id, &current) || current != originals.value(id))
            return reject(QStringLiteral("clearance-source-vehicle-%1-original-configuration-changed").arg(id));
    }
    // Current() may capture through a motor plugin. Finish EVERY callback,
    // including the caller's final fence, before this whole-group memory-only
    // pass: a later member's callback must not leave an earlier stale prefix.
    if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
    for (const QVariant& member : members) {
        if (!ownersCurrent()) return reject(QStringLiteral("clearance-source-native-owner-changed"));
        const int id = member.toMap().value(QStringLiteral("vehicleId")).toInt();
        if (!registry->nativeConfigurationEvidenceMemoryCurrent(vehicles.value(id).data(), originals.value(id)))
            return reject(QStringLiteral("clearance-source-vehicle-%1-native-source-changed-at-tail").arg(id));
    }
    const qint64 afterUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (afterUtc < beforeUtc) return reject(QStringLiteral("clearance-source-utc-rolled-back-during-capture"));
    if (afterUtc <= 0 || afterUtc > MaximumExactInteger || afterUtc < acquired || afterUtc >= until)
        return reject(QStringLiteral("clearance-source-scene-outside-current-utc-window"));
    if (!ownersCurrent()) return reject(QStringLiteral("clearance-source-native-owner-changed"));
    const qint64 remaining = until - afterUtc;
    if (remaining <= 0 || remaining > MaximumExactInteger)
        return reject(QStringLiteral("clearance-source-remaining-validity-outside-supported-domain"));
    const QVariantMap evidence{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("native-local-configuration-and-time-window")},
        {QStringLiteral("inputFingerprint"), inspected.value(QStringLiteral("fingerprint"))},
        {QStringLiteral("sceneId"), scene.value(QStringLiteral("sceneId"))},
        {QStringLiteral("sceneRevision"), scene.value(QStringLiteral("revision"))},
        {QStringLiteral("sceneSourceDocumentHash"), scene.value(QStringLiteral("sourceDocumentHash"))},
        {QStringLiteral("acquiredAtUtc"), scene.value(QStringLiteral("acquiredAtUtc"))},
        {QStringLiteral("validUntilUtc"), scene.value(QStringLiteral("validUntilUtc"))},
        {QStringLiteral("horizontalDatum"), scene.value(QStringLiteral("horizontalDatum"))},
        {QStringLiteral("verticalDatumId"), scene.value(QStringLiteral("verticalDatumId"))}, {QStringLiteral("members"), rows},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("physicalEnvelopeCertified"), false},
        {QStringLiteral("datumRelationEstablished"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    return {{QStringLiteral("provided"), true}, {QStringLiteral("inputValid"), true},
        {QStringLiteral("configurationAndTimeCurrent"), true}, {QStringLiteral("reason"), QStringLiteral("")},
        {QStringLiteral("evidence"), evidence}, {QStringLiteral("remainingValidityMs"), remaining},
        {QStringLiteral("observedAtUtcMs"), afterUtc}};
}

QVariantMap FleetMissionClearanceSource::recapture(const QVariantMap& frozenEvidence, FleetRegistry* actualRegistry,
    const QMap<int, Vehicle*>& actualVehicles, const std::function<bool()>& fence)
{
    const QVariantMap original = frozenEvidence;
    const QMap<int, Vehicle*> suppliedVehicles = actualVehicles;
    const std::function<bool()> sourceFence = fence;
    const auto malformed = []() { return result(true, false, QStringLiteral("clearance-source-original-evidence-invalid")); };
    const auto reject = [](const QString& reason) { return result(true, true, reason); };
    if (!exact(original, {"schemaVersion", "source", "inputFingerprint", "sceneId", "sceneRevision",
            "sceneSourceDocumentHash", "acquiredAtUtc", "validUntilUtc", "horizontalDatum", "verticalDatumId", "members",
            "snapshotOnly", "physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated",
            "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        || !nativeInt(original.value(QStringLiteral("schemaVersion")), 1)
        || !text(original.value(QStringLiteral("source")))
        || original.value(QStringLiteral("source")).toString() != QStringLiteral("native-local-configuration-and-time-window")
        || !digest(original.value(QStringLiteral("inputFingerprint")))
        || !sceneText(original.value(QStringLiteral("sceneId")))
        || !positiveRevision(original.value(QStringLiteral("sceneRevision")))
        || !digest(original.value(QStringLiteral("sceneSourceDocumentHash")))
        || !text(original.value(QStringLiteral("horizontalDatum")))
        || original.value(QStringLiteral("horizontalDatum")).toString() != QStringLiteral("wgs84")
        || !sceneText(original.value(QStringLiteral("verticalDatumId")))
        || original.value(QStringLiteral("members")).userType() != QMetaType::QVariantList
        || original.value(QStringLiteral("members")).isNull() || !flag(original, "snapshotOnly", true)) return malformed();
    for (const char* key : {"physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated",
            "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        if (!flag(original, key, false)) return malformed();
    qint64 acquired = 0, until = 0;
    if (!utc(original.value(QStringLiteral("acquiredAtUtc")), &acquired)
        || !utc(original.value(QStringLiteral("validUntilUtc")), &until) || until <= acquired) return malformed();
    const QVariantList rows = original.value(QStringLiteral("members")).toList();
    if (rows.isEmpty() || rows.size() > 32) return malformed();
    QMap<int, QVariantMap> originals;
    for (const QVariant& rowValue : rows) {
        if (!mapValue(rowValue)) return malformed();
        const QVariantMap row = rowValue.toMap();
        const QVariant rawId = row.value(QStringLiteral("vehicleId"));
        if (!exact(row, {"vehicleId", "configurationEvidence"}) || rawId.userType() != QMetaType::Int
            || rawId.isNull() || rawId.toInt() < 1 || rawId.toInt() > 255
            || originals.contains(rawId.toInt()) || !mapValue(row.value(QStringLiteral("configurationEvidence")))) return malformed();
        QVariantMap checked;
        const QVariantMap response{{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")},
            {QStringLiteral("evidence"), row.value(QStringLiteral("configurationEvidence"))}};
        if (!nativeEvidence(response, rawId.toInt(), &checked)) return malformed();
        originals.insert(rawId.toInt(), checked);
    }
    if (suppliedVehicles.size() != rows.size()) return reject(QStringLiteral("clearance-source-actual-member-set-mismatch"));
    const QPointer<FleetRegistry> registry(actualRegistry);
    QMap<int, QPointer<Vehicle>> vehicles;
    for (auto it = suppliedVehicles.constBegin(); it != suppliedVehicles.constEnd(); ++it) {
        if (!originals.contains(it.key()) || !it.value()) return reject(QStringLiteral("clearance-source-actual-member-set-mismatch"));
        vehicles.insert(it.key(), QPointer<Vehicle>(it.value()));
    }
    const auto ownersCurrent = [&]() {
        if (!registry || registry->thread() != QThread::currentThread()) return false;
        for (auto it = vehicles.constBegin(); it != vehicles.constEnd(); ++it)
            if (!it.value() || it.value()->thread() != QThread::currentThread() || it.value()->id() != it.key()) return false;
        return true;
    };
    const auto fenced = [&]() {
        if (!ownersCurrent()) return false;
        if (sourceFence && !sourceFence()) return false;
        return ownersCurrent();
    };
    const qint64 beforeUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (beforeUtc <= 0 || beforeUtc > MaximumExactInteger || beforeUtc < acquired || beforeUtc >= until)
        return reject(QStringLiteral("clearance-source-scene-outside-current-utc-window"));
    if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-unavailable"));
    for (const QVariant& rowValue : rows) {
        if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
        const int id = rowValue.toMap().value(QStringLiteral("vehicleId")).toInt();
        const QVariantMap response = registry->nativeConfigurationEvidenceCurrent(vehicles.value(id).data(), originals.value(id));
        if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
        QVariantMap current;
        if (!nativeEvidence(response, id, &current) || current != originals.value(id))
            return reject(QStringLiteral("clearance-source-vehicle-%1-original-configuration-changed").arg(id));
    }
    if (!fenced()) return reject(QStringLiteral("clearance-source-owner-or-fence-changed"));
    // No caller, plugin, capture, or other callback after this group pass.
    for (const QVariant& rowValue : rows) {
        if (!ownersCurrent()) return reject(QStringLiteral("clearance-source-native-owner-changed"));
        const int id = rowValue.toMap().value(QStringLiteral("vehicleId")).toInt();
        if (!registry->nativeConfigurationEvidenceMemoryCurrent(vehicles.value(id).data(), originals.value(id)))
            return reject(QStringLiteral("clearance-source-vehicle-%1-native-source-changed-at-tail").arg(id));
    }
    const qint64 afterUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (afterUtc < beforeUtc) return reject(QStringLiteral("clearance-source-utc-rolled-back-during-capture"));
    if (afterUtc <= 0 || afterUtc > MaximumExactInteger || afterUtc < acquired || afterUtc >= until)
        return reject(QStringLiteral("clearance-source-scene-outside-current-utc-window"));
    if (!ownersCurrent()) return reject(QStringLiteral("clearance-source-native-owner-changed"));
    const qint64 remaining = until - afterUtc;
    if (remaining <= 0 || remaining > MaximumExactInteger)
        return reject(QStringLiteral("clearance-source-remaining-validity-outside-supported-domain"));
    return {{QStringLiteral("provided"), true}, {QStringLiteral("inputValid"), true},
        {QStringLiteral("configurationAndTimeCurrent"), true}, {QStringLiteral("reason"), QStringLiteral("")},
        {QStringLiteral("evidence"), original}, {QStringLiteral("remainingValidityMs"), remaining},
        {QStringLiteral("observedAtUtcMs"), afterUtc}};
}

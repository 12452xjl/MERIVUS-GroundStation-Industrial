#include "FleetMissionStartupClearanceWorker.h"

#include "FleetMissionClearanceReview.h"
#include "FleetMissionJointSpatialReview.h"
#include "FleetMissionPlanningWorker.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetMissionStartupDomainReview.h"
#include "FleetMissionWaypointSwitchReview.h"
#include "FleetMissionInterVehicleEnvelopeReview.h"
#include "ParameterManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QUuid>

#include <cmath>
#include <initializer_list>

namespace {
using Motion = FleetMissionStartupGeographicMotion;

bool exact(const QVariantMap& map, std::initializer_list<const char*> keys)
{
    if (map.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!map.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool mapValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool listValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantList && !value.isNull();
}

bool flag(const QVariantMap& map, const char* key, bool expected)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.userType() == QMetaType::Bool && !value.isNull() && value.toBool() == expected;
}

bool integer(const QVariant& value, int minimum, int maximum)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: case QMetaType::Float: break;
    default: return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= minimum && number <= maximum && std::floor(number) == number;
}

bool number(const QVariant& value, double minimum, double maximum, double* output)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: case QMetaType::Float: break;
    default: return false;
    }
    const double result = value.toDouble();
    if (!std::isfinite(result) || result < minimum || result > maximum) return false;
    *output = result;
    return true;
}

bool text(const QVariant& value, int maximum, bool empty = false)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString string = value.toString();
    if ((!empty && string.isEmpty()) || string.size() > maximum) return false;
    for (QChar character : string)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool revision(const QVariant& value, bool positive = true)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    bool ok = false;
    const quint64 number = string.toULongLong(&ok);
    return ok && (!positive || number > 0) && QString::number(number) == string;
}

QByteArray json(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

QString fingerprint(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}

bool sourceKnown(const QVariantMap& source, const QVariantMap& datum)
{
    if (!exact(source, {"schemaVersion", "source", "inputFingerprint", "sceneId", "sceneRevision", "sceneSourceDocumentHash",
            "acquiredAtUtc", "validUntilUtc", "horizontalDatum", "verticalDatumId", "members", "snapshotOnly",
            "physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated", "performanceCertified",
            "startupPermitGranted", "flightCommandReleased"})
        || !integer(source.value(QStringLiteral("schemaVersion")), 1, 1)
        || source.value(QStringLiteral("source")) != QVariant(QStringLiteral("native-local-configuration-and-time-window"))
        || !digest(source.value(QStringLiteral("inputFingerprint")))
        || !text(source.value(QStringLiteral("sceneId")), 128)
        || !revision(source.value(QStringLiteral("sceneRevision")))
        || !digest(source.value(QStringLiteral("sceneSourceDocumentHash")))
        || !text(source.value(QStringLiteral("acquiredAtUtc")), 24)
        || !text(source.value(QStringLiteral("validUntilUtc")), 24)
        || source.value(QStringLiteral("horizontalDatum")) != QVariant(QStringLiteral("wgs84"))
        || !text(source.value(QStringLiteral("verticalDatumId")), 128)
        || !listValue(source.value(QStringLiteral("members"))) || !flag(source, "snapshotOnly", true)) return false;
    for (const char* key : {"physicalEnvelopeCertified", "datumRelationEstablished", "hardwareIdentityAuthenticated",
            "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        if (!flag(source, key, false)) return false;
    if (!exact(datum, {"schemaVersion", "source", "inputFingerprint", "catalogBinding", "members", "declaredDatumCorrespondenceMatches",
            "sameDatumZeroOffsetDeclared", "physicalDatumAlignmentCertified", "currentGlobalAltitudeBranchProven", "sameEstimatorSampleProven",
            "hardwareIdentityAuthenticated", "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        || !integer(datum.value(QStringLiteral("schemaVersion")), 1, 1)
        || datum.value(QStringLiteral("source")) != QVariant(QStringLiteral("native-local-declared-clearance-datum"))
        || datum.value(QStringLiteral("inputFingerprint")) != source.value(QStringLiteral("inputFingerprint"))
        || !mapValue(datum.value(QStringLiteral("catalogBinding"))) || !listValue(datum.value(QStringLiteral("members")))
        || !flag(datum, "declaredDatumCorrespondenceMatches", true) || !flag(datum, "sameDatumZeroOffsetDeclared", true)) return false;
    for (const char* key : {"physicalDatumAlignmentCertified", "currentGlobalAltitudeBranchProven", "sameEstimatorSampleProven",
            "hardwareIdentityAuthenticated", "performanceCertified", "startupPermitGranted", "flightCommandReleased"})
        if (!flag(datum, key, false)) return false;
    const QVariantMap catalog = datum.value(QStringLiteral("catalogBinding")).toMap();
    return exact(catalog, {"catalogEpoch", "revision", "catalogFingerprint"})
        && uuid(catalog.value(QStringLiteral("catalogEpoch"))) && revision(catalog.value(QStringLiteral("revision")), false)
        && digest(catalog.value(QStringLiteral("catalogFingerprint")));
}

Motion::Models copyNativeModels(const Motion::Models& source)
{
    // Build distinct map storage instead of sharing a caller's outer QMap;
    // only the factory-created immutable value models themselves are shared.
    Motion::Models result;
    if (source.isEmpty() || source.size() > 32) return result;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) result.insert(it.key(), it.value());
    return result;
}

bool executionSnapshotKnown(const QVariant& value)
{
    if (!mapValue(value)) return false;
    QVariantList legacy, rate, phase, arrival;
    // This only restores the fixed, bounded value contract. The native
    // owner must still compare the ORIGINAL capture against its original
    // manager/PRIMARY control block and age after all queued work.
    return ParameterManager::executionParameterSnapshotRows(value.toMap(), &legacy, &rate, &phase, &arrival);
}

bool nativeModelsMatch(const QVariantMap& frozenInput, const Motion::Models& originalModels)
{
    if (!exact(frozenInput, {"schemaVersion", "captureIdentity", "plan", "reviewEvidence", "vehicles", "minimumSeparationMeters",
            "clearanceSourceEvidence", "clearanceDatumEvidence"})
        || !integer(frozenInput.value(QStringLiteral("schemaVersion")), 3, 3)
        || !listValue(frozenInput.value(QStringLiteral("vehicles")))) return false;
    const QVariantList rows = frozenInput.value(QStringLiteral("vehicles")).toList();
    if (rows.isEmpty() || rows.size() > 32 || originalModels.size() != rows.size()) return false;
    QSet<int> seen;
    for (const QVariant& value : rows) {
        if (!mapValue(value)) return false;
        const QVariantMap row = value.toMap();
        if (!exact(row, {"vehicleId", "readinessSummary", "geographicMotionModel", "parameterSnapshot"})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255)
            || !mapValue(row.value(QStringLiteral("readinessSummary")))
            || !mapValue(row.value(QStringLiteral("geographicMotionModel")))
            || !executionSnapshotKnown(row.value(QStringLiteral("parameterSnapshot")))) return false;
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        const Motion::Ptr original = originalModels.value(id);
        if (seen.contains(id) || !original || json(original->source()) != json(row.value(QStringLiteral("geographicMotionModel")))) return false;
        seen.insert(id);
    }
    return true;
}

bool inputKnown(const QVariantMap& input, const Motion::Models* originalModels = nullptr)
{
    if (!exact(input, {"schemaVersion", "captureIdentity", "plan", "reviewEvidence", "vehicles", "minimumSeparationMeters",
            "clearanceSourceEvidence", "clearanceDatumEvidence"})
        || !integer(input.value(QStringLiteral("schemaVersion")), 3, 3)
        || !mapValue(input.value(QStringLiteral("captureIdentity"))) || !mapValue(input.value(QStringLiteral("plan")))
        || !mapValue(input.value(QStringLiteral("reviewEvidence"))) || !listValue(input.value(QStringLiteral("vehicles")))
        || !mapValue(input.value(QStringLiteral("clearanceSourceEvidence"))) || !mapValue(input.value(QStringLiteral("clearanceDatumEvidence")))) return false;
    const QVariantMap identity = input.value(QStringLiteral("captureIdentity")).toMap();
    if (!exact(identity, {"reviewKind", "reviewEpoch", "reviewGeneration", "reviewId"})
        || !text(identity.value(QStringLiteral("reviewKind")), 6)
        || (identity.value(QStringLiteral("reviewKind")).toString() != QStringLiteral("normal")
            && identity.value(QStringLiteral("reviewKind")).toString() != QStringLiteral("ground"))
        || !uuid(identity.value(QStringLiteral("reviewEpoch"))) || !uuid(identity.value(QStringLiteral("reviewId")))
        || !revision(identity.value(QStringLiteral("reviewGeneration")))) return false;
    const QVariantMap plan = input.value(QStringLiteral("plan")).toMap();
    if (!listValue(plan.value(QStringLiteral("members"))) || !mapValue(plan.value(QStringLiteral("constraints")))) return false;
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    if (!mapValue(constraints.value(QStringLiteral("clearanceInput")))) return false;
    double ceiling = 0.0;
    if (!number(constraints.value(QStringLiteral("maxAltitudeMeters")), -1000.0, 20000.0, &ceiling)) return false;
    const QVariantMap source = input.value(QStringLiteral("clearanceSourceEvidence")).toMap();
    const QVariantMap datum = input.value(QStringLiteral("clearanceDatumEvidence")).toMap();
    if (!sourceKnown(source, datum)) return false;
    const QVariantList rows = input.value(QStringLiteral("vehicles")).toList();
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    const QVariantList sources = source.value(QStringLiteral("members")).toList();
    const QVariantList datums = datum.value(QStringLiteral("members")).toList();
    if (rows.isEmpty() || rows.size() > 32 || rows.size() != members.size()
        || rows.size() != sources.size() || rows.size() != datums.size()) return false;
    if (originalModels && !nativeModelsMatch(input, *originalModels)) return false;
    QSet<int> seen;
    int canonicalItemCount = 0;
    for (int index = 0; index < rows.size(); ++index) {
        if (!mapValue(rows.at(index)) || !mapValue(members.at(index))
            || !mapValue(sources.at(index)) || !mapValue(datums.at(index))) return false;
        const QVariantMap row = rows.at(index).toMap(), member = members.at(index).toMap();
        const QVariantMap sourceRow = sources.at(index).toMap(), datumRow = datums.at(index).toMap();
        if (!exact(row, {"vehicleId", "readinessSummary", "geographicMotionModel", "parameterSnapshot"})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255)
            || !mapValue(row.value(QStringLiteral("readinessSummary")))
            || !mapValue(row.value(QStringLiteral("geographicMotionModel")))
            || !executionSnapshotKnown(row.value(QStringLiteral("parameterSnapshot")))) return false;
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        if (seen.contains(id) || !integer(member.value(QStringLiteral("vehicleId")), id, id)
            || !exact(sourceRow, {"vehicleId", "configurationEvidence"}) || !integer(sourceRow.value(QStringLiteral("vehicleId")), id, id)
            || !exact(datumRow, {"vehicleId", "configurationEvidence", "originReference", "datumCorrespondence"})
            || !integer(datumRow.value(QStringLiteral("vehicleId")), id, id)
            || !mapValue(sourceRow.value(QStringLiteral("configurationEvidence")))
            || sourceRow.value(QStringLiteral("configurationEvidence")).toMap().size() != 13
            || !mapValue(datumRow.value(QStringLiteral("configurationEvidence")))
            || json(sourceRow.value(QStringLiteral("configurationEvidence"))) != json(datumRow.value(QStringLiteral("configurationEvidence")))
            || !mapValue(datumRow.value(QStringLiteral("originReference"))) || datumRow.value(QStringLiteral("originReference")).toMap().size() != 18
            || !mapValue(datumRow.value(QStringLiteral("datumCorrespondence"))) || datumRow.value(QStringLiteral("datumCorrespondence")).toMap().size() != 10) return false;
        const QVariantMap readiness = row.value(QStringLiteral("readinessSummary")).toMap();
        // The owner explicitly selects these geometry values. Do not silently
        // remove a diagnostic, live ALTITUDE sample, or any other extra key.
        if (!exact(readiness, {"launchType", "currentPositionAltitudeSummary", "takeoffDerivation", "currentPositionToFirstWaypoint"})) return false;
        const QVariantMap motion = row.value(QStringLiteral("geographicMotionModel")).toMap();
        const QVariantMap profile = member.value(QStringLiteral("executionProfile")).toMap();
        const QVariantMap policy = profile.value(QStringLiteral("startupTransitionPolicy")).toMap();
        if (profile.contains(QStringLiteral("startupSpatialGuard")) != motion.contains(QStringLiteral("startupSpatialGuard"))
            || (profile.contains(QStringLiteral("startupSpatialGuard"))
                && (profile.value(QStringLiteral("startupSpatialGuard")).userType() != QMetaType::QString
                    || profile.value(QStringLiteral("startupSpatialGuard")) != QVariant(QStringLiteral("integer-e7-amsl-mm-v1"))
                    || profile.value(QStringLiteral("startupSpatialGuard")) != motion.value(QStringLiteral("startupSpatialGuard"))))) return false;
        double motionCeiling = 0.0;
        if (!mapValue(motion.value(QStringLiteral("baselineGlobalPosition")))
            || !listValue(motion.value(QStringLiteral("canonicalWireItems")))
            || !mapValue(motion.value(QStringLiteral("startupTransitionPolicy")))
            || json(motion.value(QStringLiteral("startupTransitionPolicy"))) != json(policy)
            || !number(motion.value(QStringLiteral("maximumAltitudeAmslMeters")), -1000.0, 20000.0, &motionCeiling)
            || motionCeiling != ceiling) return false;
        const QVariantList wire = motion.value(QStringLiteral("canonicalWireItems")).toList();
        if (wire.isEmpty() || wire.size() > 2048 || canonicalItemCount > 8192 - wire.size()) return false;
        canonicalItemCount += wire.size();
        if (!originalModels) {
            // Value-only snapshot/matcher validation may reconstruct a
            // temporary descriptor. Never substitute it for an actual run's
            // original immutable model or its cached Qt numeric operands.
            const auto prepared = Motion::prepare(motion.value(QStringLiteral("baselineGlobalPosition")).toMap(),
                {{QStringLiteral("canonicalWireItems"), wire}, {QStringLiteral("contentHash"), motion.value(QStringLiteral("payloadContentHash"))},
                    {QStringLiteral("executionProfile"), profile}},
                policy, ceiling);
            if (!prepared || json(prepared->source()) != json(motion)) return false;
        }
        const QVariantMap baseline = motion.value(QStringLiteral("baselineGlobalPosition")).toMap();
        const QVariantMap altitude = readiness.value(QStringLiteral("currentPositionAltitudeSummary")).toMap();
        const QVariantMap first = readiness.value(QStringLiteral("currentPositionToFirstWaypoint")).toMap();
        double reportedAltitude = 0.0;
        if (!mapValue(readiness.value(QStringLiteral("currentPositionAltitudeSummary")))
            || !mapValue(readiness.value(QStringLiteral("currentPositionToFirstWaypoint")))
            || !integer(baseline.value(QStringLiteral("altitudeAmslMm")), -1000000, 20000000)
            || !number(altitude.value(QStringLiteral("reportedAltitudeAmslMeters")), -1000.0, 20000.0, &reportedAltitude)
            || reportedAltitude != baseline.value(QStringLiteral("altitudeAmslMm")).toDouble() / 1000.0
            || !integer(first.value(QStringLiteral("fromLatitudeE7")), -800000000, 800000000)
            || !integer(first.value(QStringLiteral("fromLongitudeE7")), -1800000000, 1800000000)
            || first.value(QStringLiteral("fromLatitudeE7")) != baseline.value(QStringLiteral("latitudeE7"))
            || first.value(QStringLiteral("fromLongitudeE7")) != baseline.value(QStringLiteral("longitudeE7"))) return false;
        seen.insert(id);
    }
    return true;
}

QVariantMap detachedNativeInput(const QVariantMap& input, const Motion::Models& originalModels)
{
    QVariantMap result;
    if (!FleetMissionPlanningWorker::snapshot(input, &result) || !inputKnown(result, &originalModels)) return {};
    return result;
}

QVariantMap failure(const QString& inputFingerprint, const QString& error)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error},
        {QStringLiteral("inputFingerprint"), inputFingerprint}, {QStringLiteral("jointView"), QVariantMap()},
        {QStringLiteral("clearanceReview"), QVariantMap()}, {QStringLiteral("startupDomainReview"), QVariantMap()},
        {QStringLiteral("waypointSwitchReview"), QVariantMap()},
        {QStringLiteral("interVehicleEnvelopeReview"), QVariantMap()}};
}

QVariantMap jointFor(const QVariantMap& input, const QString& inputFingerprint)
{
    const QVariantMap identity = input.value(QStringLiteral("captureIdentity")).toMap();
    // The full original motion descriptor stays in inputFingerprint. Joint's
    // independent nominal geometry contract remains exactly its row2, never
    // treating this descriptor as another nominal/startup segment.
    QVariantList geometryRows;
    for (const QVariant& value : input.value(QStringLiteral("vehicles")).toList()) {
        const QVariantMap row = value.toMap();
        geometryRows.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("readinessSummary"), row.value(QStringLiteral("readinessSummary"))}});
    }
    return FleetMissionJointSpatialReview::buildStartup(input.value(QStringLiteral("plan")).toMap(),
        input.value(QStringLiteral("reviewEvidence")).toMap(),
        {{QStringLiteral("reviewKind"), identity.value(QStringLiteral("reviewKind"))},
            {QStringLiteral("geometryBasis"), inputFingerprint}, {QStringLiteral("vehicles"), geometryRows},
            {QStringLiteral("minimumSeparationMeters"), input.value(QStringLiteral("minimumSeparationMeters"))}});
}

bool motionPayloadsMatch(const QVariantMap& input, const QVariantMap& joint)
{
    const QVariantList rows = input.value(QStringLiteral("vehicles")).toList();
    const QVariantList members = joint.value(QStringLiteral("memberCoverage")).toList();
    if (rows.size() != members.size()) return false;
    for (int index = 0; index < rows.size(); ++index) {
        const QVariantMap row = rows.at(index).toMap(), member = members.at(index).toMap();
        if (row.value(QStringLiteral("vehicleId")) != member.value(QStringLiteral("vehicleId"))
            || row.value(QStringLiteral("geographicMotionModel")).toMap().value(QStringLiteral("payloadContentHash"))
                != member.value(QStringLiteral("payloadHash"))) return false;
    }
    return true;
}

QVariantList motionModels(const QVariantMap& input)
{
    QVariantList models;
    for (const QVariant& value : input.value(QStringLiteral("vehicles")).toList()) {
        const QVariantMap row = value.toMap();
        models.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("geographicMotionModel"), row.value(QStringLiteral("geographicMotionModel"))}});
    }
    return models;
}

QVariantList waypointModels(const QVariantMap& input)
{
    QVariantList models;
    for (const QVariant& value : input.value(QStringLiteral("vehicles")).toList()) {
        const QVariantMap row = value.toMap();
        models.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("geographicMotionModel"), row.value(QStringLiteral("geographicMotionModel"))},
            {QStringLiteral("parameterSnapshot"), row.value(QStringLiteral("parameterSnapshot"))}});
    }
    return models;
}

bool built(const QVariantMap& result, const char* valueKey)
{
    const QString key = QString::fromLatin1(valueKey);
    return result.size() == 3 && result.contains(key) && flag(result, "ok", true)
        && text(result.value(QStringLiteral("error")), 256, true) && result.value(QStringLiteral("error")).toString().isEmpty()
        && mapValue(result.value(key)) && !result.value(key).toMap().isEmpty();
}

QString resultError(const QVariantMap& result, const QString& fallback)
{
    return text(result.value(QStringLiteral("error")), 256) ? result.value(QStringLiteral("error")).toString() : fallback;
}

QVariant clearanceInput(const QVariantMap& input)
{
    return input.value(QStringLiteral("plan")).toMap().value(QStringLiteral("constraints")).toMap().value(QStringLiteral("clearanceInput"));
}

bool clearanceSourceMatches(const QVariantMap& input, const QVariantMap& clearance)
{
    return built(clearance, "report")
        && clearance.value(QStringLiteral("report")).toMap().value(QStringLiteral("clearanceInputFingerprint"))
            == input.value(QStringLiteral("clearanceSourceEvidence")).toMap().value(QStringLiteral("inputFingerprint"));
}
} // namespace

FleetMissionStartupClearanceWorker::FleetMissionStartupClearanceWorker(const QVariantMap& input,
                                                                     const Motion::Models& originalModels,
                                                                     const Cancellation& cancellation)
    : QThread(nullptr), _originalModels(copyNativeModels(originalModels)),
      _input(detachedNativeInput(input, _originalModels)), _cancellation(cancellation)
{
}

bool FleetMissionStartupClearanceWorker::nativeModelsMatchInput(const QVariantMap& frozenInput,
    const Motion::Models& originalModels)
{
    return nativeModelsMatch(frozenInput, originalModels);
}

Motion::Models FleetMissionStartupClearanceWorker::originalGeographicModels() const
{
    return copyNativeModels(_originalModels);
}

bool FleetMissionStartupClearanceWorker::snapshot(const QVariantMap& input, QVariantMap* output)
{
    if (!output) return false;
    const QVariantMap source = input;
    output->clear();
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(source, &detached) || !inputKnown(detached)) return false;
    *output = detached;
    return true;
}

bool FleetMissionStartupClearanceWorker::resultMatches(const QVariantMap& input, const QVariantMap& result)
{
    QVariantMap detached, observed;
    if (!snapshot(input, &detached) || !FleetMissionPlanningWorker::snapshot(result, &observed)
        || !exact(observed, {"ok", "error", "inputFingerprint", "jointView", "clearanceReview", "startupDomainReview", "waypointSwitchReview", "interVehicleEnvelopeReview"})
        || !text(observed.value(QStringLiteral("error")), 256, true)
        || !digest(observed.value(QStringLiteral("inputFingerprint")))
        || observed.value(QStringLiteral("inputFingerprint")).toString() != fingerprint(detached)
        || !mapValue(observed.value(QStringLiteral("jointView"))) || !mapValue(observed.value(QStringLiteral("clearanceReview")))
        || !mapValue(observed.value(QStringLiteral("startupDomainReview")))
        || !mapValue(observed.value(QStringLiteral("waypointSwitchReview")))
        || !mapValue(observed.value(QStringLiteral("interVehicleEnvelopeReview")))) return false;
    if (flag(observed, "ok", false))
        return !observed.value(QStringLiteral("error")).toString().isEmpty()
            && observed.value(QStringLiteral("jointView")).toMap().isEmpty() && observed.value(QStringLiteral("clearanceReview")).toMap().isEmpty()
            && observed.value(QStringLiteral("startupDomainReview")).toMap().isEmpty()
            && observed.value(QStringLiteral("waypointSwitchReview")).toMap().isEmpty()
            && observed.value(QStringLiteral("interVehicleEnvelopeReview")).toMap().isEmpty();
    if (!flag(observed, "ok", true) || !observed.value(QStringLiteral("error")).toString().isEmpty()) return false;
    const QVariantMap joint = jointFor(detached, fingerprint(detached));
    if (!built(joint, "view") || !motionPayloadsMatch(detached, joint.value(QStringLiteral("view")).toMap())
        || json(joint.value(QStringLiteral("view"))) != json(observed.value(QStringLiteral("jointView")))) return false;
    const QVariantMap clearance = observed.value(QStringLiteral("clearanceReview")).toMap();
    FleetMissionInterVehicleEnvelopeReview::PreparedPtr domainPrepared, waypointPrepared;
    return clearanceSourceMatches(detached, clearance)
        && FleetMissionClearanceReview::geometryResultMatches(clearanceInput(detached),
            observed.value(QStringLiteral("jointView")).toMap(), clearance)
        && FleetMissionStartupDomainReview::resultMatches(clearanceInput(detached), motionModels(detached),
            observed.value(QStringLiteral("jointView")).toMap(), fingerprint(detached),
            observed.value(QStringLiteral("startupDomainReview")).toMap(), &domainPrepared)
        && FleetMissionWaypointSwitchReview::resultMatches(clearanceInput(detached), waypointModels(detached),
            observed.value(QStringLiteral("jointView")).toMap(), fingerprint(detached),
            observed.value(QStringLiteral("waypointSwitchReview")).toMap(), &waypointPrepared)
        && FleetMissionInterVehicleEnvelopeReview::resultMatches(domainPrepared, waypointPrepared,
            observed.value(QStringLiteral("interVehicleEnvelopeReview")).toMap());
}

bool FleetMissionStartupClearanceWorker::startupModelSatisfied(const QVariantMap& result)
{
    if (!exact(result, {"ok", "error", "inputFingerprint", "jointView", "clearanceReview", "startupDomainReview", "waypointSwitchReview", "interVehicleEnvelopeReview"})
        || !flag(result, "ok", true) || !text(result.value(QStringLiteral("error")), 256, true)
        || !result.value(QStringLiteral("error")).toString().isEmpty()
        || !digest(result.value(QStringLiteral("inputFingerprint")))
        || !mapValue(result.value(QStringLiteral("jointView")))
        || !mapValue(result.value(QStringLiteral("clearanceReview")))
        || !mapValue(result.value(QStringLiteral("startupDomainReview")))
        || !mapValue(result.value(QStringLiteral("waypointSwitchReview")))
        || !mapValue(result.value(QStringLiteral("interVehicleEnvelopeReview")))) return false;
    const QVariantMap clearance = result.value(QStringLiteral("clearanceReview")).toMap();
    const QVariantMap domain = result.value(QStringLiteral("startupDomainReview")).toMap();
    const QVariantMap waypoint = result.value(QStringLiteral("waypointSwitchReview")).toMap();
    const QVariantMap interVehicle = result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap();
    if (!built(clearance, "report") || !built(domain, "report") || !built(waypoint, "report")
        || !built(interVehicle, "report")) return false;
    const QVariantMap nominal = clearance.value(QStringLiteral("report")).toMap();
    const QVariantMap permitted = domain.value(QStringLiteral("report")).toMap();
    const QVariantMap switching = waypoint.value(QStringLiteral("report")).toMap();
    const QVariantMap envelopes = interVehicle.value(QStringLiteral("report")).toMap();
    // Evaluate the new complete comparison independently of the existing
    // currently-false actual-path certificate. It is a mandatory model input,
    // not a decorative report hidden behind that older blocking condition.
    const bool interVehicleSatisfied = flag(envelopes, "geometryComplete", true)
        && flag(envelopes, "comparisonComplete", true) && flag(envelopes, "declaredModelSatisfied", true)
        && integer(envelopes.value(QStringLiteral("conflictCount")), 0, 0);
    return interVehicleSatisfied
        && flag(nominal, "scopeCoverageComplete", true) && flag(nominal, "comparisonComplete", true)
        && flag(nominal, "contactContextComplete", true) && flag(nominal, "declaredModelSatisfied", true)
        && integer(nominal.value(QStringLiteral("violationCount")), 0, 0)
        && flag(permitted, "outerCoverComplete", true) && flag(permitted, "comparisonComplete", true)
        && flag(permitted, "contactContextComplete", true) && flag(permitted, "declaredModelSatisfied", true)
        && flag(permitted, "numericContainmentProven", true)
        // Construction containment of accepted telemetry samples does not
        // establish action dynamics or motion between those samples.
        && flag(permitted, "actualPathCertified", true)
        && integer(permitted.value(QStringLiteral("violationCount")), 0, 0)
        && flag(switching, "modelComplete", true) && flag(switching, "comparisonComplete", true)
        && flag(switching, "declaredModelSatisfied", true)
        && integer(switching.value(QStringLiteral("violationCount")), 0, 0);
}

void FleetMissionStartupClearanceWorker::run()
{
    const QString inputFingerprint = _input.isEmpty() ? QStringLiteral("") : fingerprint(_input);
    const auto cancelled = [this]() { return !_cancellation || _cancellation->load(std::memory_order_acquire); };
    const auto reject = [this, &inputFingerprint](const QString& error) { _result = failure(inputFingerprint, error); };
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (_input.isEmpty()) { reject(QStringLiteral("startup-clearance-input-resource-or-value-invalid")); return; }
    if (!nativeModelsMatchInput(_input, _originalModels)) {
        reject(QStringLiteral("startup-clearance-original-geographic-models-mismatch")); return;
    }
    const QVariantMap joint = jointFor(_input, inputFingerprint);
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (!built(joint, "view")) { reject(resultError(joint, QStringLiteral("startup-clearance-joint-invalid"))); return; }
    const QVariantMap view = joint.value(QStringLiteral("view")).toMap();
    if (!motionPayloadsMatch(_input, view)) { reject(QStringLiteral("startup-motion-model-original-plan-payload-mismatch")); return; }
    const QVariantMap clearance = FleetMissionClearanceReview::buildForGeometryBasisCancelable(clearanceInput(_input), view, _cancellation);
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (!clearanceSourceMatches(_input, clearance)) {
        reject(resultError(clearance, QStringLiteral("startup-clearance-report-or-source-invalid"))); return;
    }
    FleetMissionInterVehicleEnvelopeReview::PreparedPtr domainPrepared, waypointPrepared;
    const QVariantMap domain = FleetMissionStartupDomainReview::build(clearanceInput(_input), motionModels(_input),
        _originalModels, view, inputFingerprint, _cancellation, &domainPrepared);
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (!built(domain, "report")) {
        reject(resultError(domain, QStringLiteral("startup-domain-report-invalid"))); return;
    }
    const QVariantMap waypoint = FleetMissionWaypointSwitchReview::build(clearanceInput(_input), waypointModels(_input),
        view, inputFingerprint, _cancellation, &waypointPrepared);
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (!built(waypoint, "report")) {
        reject(resultError(waypoint, QStringLiteral("waypoint-switch-report-invalid"))); return;
    }
    // These seals retain the complete native components produced above, not
    // a reconstruction from public summaries or a nominal route subset.
    const QVariantMap interVehicle = FleetMissionInterVehicleEnvelopeReview::build(
        domainPrepared, waypointPrepared, _cancellation);
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    if (!built(interVehicle, "report")) {
        reject(resultError(interVehicle, QStringLiteral("inter-vehicle-envelope-report-invalid"))); return;
    }
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("inputFingerprint"), inputFingerprint}, {QStringLiteral("jointView"), view},
        {QStringLiteral("clearanceReview"), clearance}, {QStringLiteral("startupDomainReview"), domain},
        {QStringLiteral("waypointSwitchReview"), waypoint},
        {QStringLiteral("interVehicleEnvelopeReview"), interVehicle}};
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(result, &detached)) {
        reject(QStringLiteral("startup-clearance-result-resource-or-value-invalid")); return;
    }
    if (cancelled()) { reject(QStringLiteral("startup-clearance-cancelled")); return; }
    _result = detached;
}

#include "FleetReserveDispatchPlanner.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetCapabilityEvidence.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetReserveEnergyPlan.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <cmath>

namespace {
constexpr double MaximumNumber = 9007199254740991.0;
constexpr qint64 MaximumSourceBytes = 32LL * 1024 * 1024;
constexpr qint64 MaximumTaskBytes = 16LL * 1024 * 1024;
const QString SourceKey = QStringLiteral("reserveDispatchPlanningSource");
const QString SourceKind = QStringLiteral("reserve-plan-dispatch-proposal");

bool number(const QVariant& value, double low, double high, double* output = nullptr)
{
    if (value.isNull() || !value.isValid()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double parsed = value.toDouble();
    if (!std::isfinite(parsed) || parsed < low || parsed > high) return false;
    if (output) *output = parsed;
    return true;
}
bool integer(const QVariant& value, int low, int high, int* output = nullptr)
{
    double parsed = 0.0;
    if (!number(value, low, high, &parsed) || std::floor(parsed) != parsed) return false;
    if (output) *output = int(parsed);
    return true;
}
bool assignmentId(const QVariantMap& map, int* output)
{
    int selected = 0;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("systemId"), QStringLiteral("id")}) {
        if (!map.contains(key)) continue;
        int candidate = 0;
        if (!integer(map.value(key), 1, 255, &candidate) || (selected != 0 && candidate != selected)) return false;
        selected = candidate;
    }
    if (selected == 0) return false;
    *output = selected; return true;
}
bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}
bool exact(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    return true;
}
QByteArray json(const QVariant& value) { return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact); }
bool same(const QVariant& left, const QVariant& right) { return json(QVariantList{left}) == json(QVariantList{right}); }
QString hash(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}
bool tree(const QVariant& value, int depth, int* nodes, qint64* bytes, qint64 limit)
{
    if (depth > 24 || ++(*nodes) > 1048576) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map:
        if (value.isNull() || value.toMap().size() > 1024) return false;
        {
            const QVariantMap map = value.toMap();
            for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                *bytes += it.key().size() * qint64(2);
                if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()
                    || *bytes > limit || !tree(it.value(), depth + 1, nodes, bytes, limit)) return false;
            }
        }
        return true;
    case QVariant::List:
        if (value.isNull() || value.toList().size() > 32768) return false;
        for (const QVariant& entry : value.toList()) if (!tree(entry, depth + 1, nodes, bytes, limit)) return false;
        return true;
    case QVariant::StringList:
        if (value.isNull() || value.toStringList().size() > 32768) return false;
        for (const QString& entry : value.toStringList()) if (!tree(entry, depth + 1, nodes, bytes, limit)) return false;
        return true;
    case QVariant::String:
        *bytes += value.toString().size() * qint64(2);
        return value.toString().size() <= 32768 && *bytes <= limit && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: return !value.isNull();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        return number(value, -MaximumNumber, MaximumNumber);
    default: return false;
    }
}
bool bounded(const QVariantMap& map, qint64 limit)
{
    int nodes = 0; qint64 bytes = 0;
    return tree(map, 0, &nodes, &bytes, limit) && !json(map).isEmpty() && json(map).size() <= limit;
}
bool text(const QVariant& value, int maximum)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().isEmpty()
        || value.toString().size() > maximum || value.toString() != value.toString().trimmed()) return false;
    for (const QChar c : value.toString()) if (c.isNull() || c.category() == QChar::Other_Control) return false;
    return true;
}
bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar c : value.toString()) if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        || (c >= QLatin1Char('a') && c <= QLatin1Char('f')))) return false;
    return true;
}
bool uuid(const QVariant& value)
{
    if (!text(value, 36)) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool bindingValid(const QVariantMap& binding)
{
    QSet<QString> fields{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (binding.size() == 9) fields.unite({QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")});
    if (!exact(binding, fields) || !text(binding.value(QStringLiteral("sourceTaskId")), 128)
        || !text(binding.value(QStringLiteral("recordId")), 128) || !uuid(binding.value(QStringLiteral("storeEpoch")))) return false;
    for (const QString& key : {QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (!digest(binding.value(key))) return false;
    for (const QString& key : {QStringLiteral("recordRevision"), QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")}) {
        if (!binding.contains(key)) continue;
        double value = 0.0;
        if (!number(binding.value(key), key == QStringLiteral("recordRevision") ? 1.0 : 0.0, MaximumNumber, &value)
            || std::floor(value) != value) return false;
    }
    return true;
}
bool estimateOnly(const QVariantMap& map)
{
    for (const QString& key : {QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"),
            QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"), QStringLiteral("timeSeconds"), QStringLiteral("time"),
            QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds")})
        if (map.contains(key)) return false;
    for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")})
        if (map.contains(key) && (!text(map.value(key), 64) || !QStringList{QStringLiteral("estimated"), QStringLiteral("unknown"),
            QStringLiteral("estimate"), QStringLiteral("constant-speed-estimate")}.contains(map.value(key).toString()))) return false;
    for (const QString& key : {QStringLiteral("timeReference"), QStringLiteral("returnTimeReference")})
        if (map.contains(key) && (!text(map.value(key), 64) || !QStringList{QStringLiteral("relative"), QStringLiteral("relative-estimate"),
            QStringLiteral("unknown")}.contains(map.value(key).toString()))) return false;
    return map.value(QStringLiteral("executionProfile")).toMap().value(QStringLiteral("timingPolicy")).toString() != QStringLiteral("absolute-required");
}
bool point(const QVariant& value, bool endpoint, int sequence, QVariantMap* result)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    QSet<QString> fields{QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")};
    if (!endpoint) fields.unite({QStringLiteral("sequence"), QStringLiteral("heading"), QStringLiteral("holdSeconds"),
        QStringLiteral("estimatedElapsedSeconds"), QStringLiteral("timingEvidence"), QStringLiteral("timeReference")});
    if ((endpoint && !exact(map, fields)) || map.size() > fields.size() || !estimateOnly(map)) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    if (!text(map.value(QStringLiteral("coordinateSystem")), 16) || map.value(QStringLiteral("coordinateSystem")).toString() != QStringLiteral("geodetic")
        || !text(map.value(QStringLiteral("altitudeReference")), 16) || map.value(QStringLiteral("altitudeReference")).toString() != QStringLiteral("amsl")
        || !number(map.value(QStringLiteral("latitude")), -80.0, 80.0, &latitude)
        || !number(map.value(QStringLiteral("longitude")), -180.0, 180.0, &longitude)
        || !number(map.value(QStringLiteral("altitude")), -1000.0, 20000.0, &altitude)
        || (!endpoint && !integer(map.value(QStringLiteral("sequence")), sequence, sequence))) return false;
    *result = {{QStringLiteral("sequence"), sequence}, {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")}, {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude}};
    for (const QString& key : {QStringLiteral("heading"), QStringLiteral("holdSeconds"), QStringLiteral("estimatedElapsedSeconds")}) {
        if (!map.contains(key)) continue;
        double value = 0.0;
        if (!number(map.value(key), key == QStringLiteral("heading") ? -1e15 : 0.0,
                key == QStringLiteral("holdSeconds") ? 86400.0 : 1e15, &value)) return false;
        if (key != QStringLiteral("estimatedElapsedSeconds")) result->insert(key, value);
    }
    return true;
}
QVariantMap taskProjection(QVariantMap task)
{
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("intent"), QStringLiteral("taskId"), QStringLiteral("templateValidation"), SourceKey}) task.remove(key);
    return task;
}
QVariantMap memberProjection(QVariantMap member)
{
    for (const QString& key : {QStringLiteral("memberIndex"), QStringLiteral("waypointCount"),
        QStringLiteral("manualConfirmationRequired"), QStringLiteral("nominalRouteTiming")}) member.remove(key);
    return member;
}
bool strictPolicy(const QVariantMap& map, bool* result)
{
    const QVariant value = map.value(QStringLiteral("requirePerformanceEvidence"), false);
    if (value.type() != QVariant::Bool || value.isNull()) return false;
    *result = value.toBool(); return true;
}
QVariantMap routeProjection(const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& boundary, bool strict)
{
    return {{QStringLiteral("member"), memberProjection(member)}, {QStringLiteral("constraints"), constraints},
        {QStringLiteral("routeBoundaryPolicy"), boundary}, {QStringLiteral("requirePerformanceEvidence"), strict}};
}
QVariantMap policies(QVariantMap task)
{
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("healthReports"), QStringLiteral("nativeVehicleEvidence"),
        QStringLiteral("capabilityReviewEvidence"), QStringLiteral("templateValidation"), QStringLiteral("intent"), QStringLiteral("taskId"), QStringLiteral("id"), QStringLiteral("sourceTaskId"),
        QStringLiteral("coverage"), QStringLiteral("target"), QStringLiteral("planningSource"), QStringLiteral("remainderPlanningSource"), QStringLiteral("groupRemainderPlanningSource"), SourceKey,
        QStringLiteral("waypoints"), QStringLiteral("route"), QStringLiteral("routeTiming"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"), QStringLiteral("returnPaths"),
        QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members"), QStringLiteral("vehicleIds"), QStringLiteral("targetVehicleIds"), QStringLiteral("candidateVehicleIds"),
        QStringLiteral("selectedVehicleIds"), QStringLiteral("vehicleCount"), QStringLiteral("minVehicleCount"), QStringLiteral("reserve"), QStringLiteral("reserveVehicleIds"), QStringLiteral("reserveMembers"),
        QStringLiteral("reservePlans"), QStringLiteral("reserveCount"), QStringLiteral("requiredRoles"), QStringLiteral("leaderRequired"), QStringLiteral("leaderCount"), QStringLiteral("scoutCount"),
        QStringLiteral("relayCount"), QStringLiteral("responderCount"), QStringLiteral("leaderVehicleId"), QStringLiteral("leaderId"), QStringLiteral("roleAssignments"), QStringLiteral("rolePlan"),
        QStringLiteral("memberEnergyBudgets"), QStringLiteral("memberEnergyBudgetsDerivedFromPlan"), QStringLiteral("executionProfile"), QStringLiteral("routeBoundaryPolicy"), QStringLiteral("runId"),
        QStringLiteral("startupId"), QStringLiteral("deploymentId"), QStringLiteral("binding"), QStringLiteral("sourceEpoch"), QStringLiteral("approval"), QStringLiteral("approvalId"), QStringLiteral("approvalState"),
        QStringLiteral("executionPermit"), QStringLiteral("startupPermitGranted"), QStringLiteral("dispatch"), QStringLiteral("executable"), QStringLiteral("flightCommandReleased")}) task.remove(key);
    return task;
}
QVariantMap memberPolicies(QVariantMap member)
{
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("systemId"), QStringLiteral("id"), QStringLiteral("assignmentRole"), QStringLiteral("role"),
        QStringLiteral("memberIndex"), QStringLiteral("waypointCount"), QStringLiteral("manualConfirmationRequired"), QStringLiteral("flightCommandReleased"), QStringLiteral("coverageLegStart"),
        QStringLiteral("coverageLegCount"), QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("waypoints"), QStringLiteral("route"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"),
        QStringLiteral("executionProfile"), QStringLiteral("nominalRouteTiming"), QStringLiteral("estimatedDurationSeconds"), QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("pathLengthMeters"), QStringLiteral("returnPathLengthMeters"),
        QStringLiteral("routeSource"), QStringLiteral("returnRouteSource"), QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("ingressWaypoints"), QStringLiteral("surveyWaypoints"),
        QStringLiteral("estimatedIngressDurationSeconds"), QStringLiteral("ingressPathLengthMeters"), QStringLiteral("estimatedSurveyDurationSeconds"), QStringLiteral("surveyPathLengthMeters"),
        QStringLiteral("routeSections"), QStringLiteral("routeSectionIndexConvention"), QStringLiteral("launchLandingInferred"), QStringLiteral("ingressGenerated"), QStringLiteral("returnGenerated"),
        QStringLiteral("ingressOutsideArea"), QStringLiteral("returnOutsideArea"), QStringLiteral("endpointPathsClearanceVerified"), QStringLiteral("verticalSequencing"), QStringLiteral("durationSeconds"),
        QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"), QStringLiteral("returnDurationSeconds"), QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds"),
        QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence"), QStringLiteral("timeReference"), QStringLiteral("returnTimeReference")}) member.remove(key);
    return member;
}
bool floor(const QVariantMap& map, const QStringList& keys, double* maximum, bool positive = false)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        double value = 0.0;
        if (!number(map.value(key), 0.0, MaximumNumber, &value) || (positive && value == 0.0)) return false;
        *maximum = qMax(*maximum, value);
    }
    return true;
}
bool budgetMatches(const QVariantMap& expected, const QVariantMap& actual, bool strict)
{
    QSet<QString> fields;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) fields.insert(it.key());
    if (strict) fields.unite({QStringLiteral("requirePerformanceEvidence"), QStringLiteral("declaredPerformanceEvidence"), QStringLiteral("declaredPerformanceEvidenceUsable")});
    if (!exact(actual, fields) || !flag(actual, QStringLiteral("complete"), true)) return false;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) {
        if (it.key() == QStringLiteral("speedCapabilityKnown") || it.key() == QStringLiteral("declaredMaxSpeedMetersPerSecond")) continue;
        if (!same(actual.value(it.key()), it.value()) || (it.value().type() == QVariant::Bool && !flag(actual, it.key(), it.value().toBool()))
            || (it.value().type() == QVariant::String && (actual.value(it.key()).type() != QVariant::String || actual.value(it.key()).isNull()))) return false;
    }
    const QVariant known = actual.value(QStringLiteral("speedCapabilityKnown"));
    double speed = 0.0;
    if (known.type() != QVariant::Bool || known.isNull()
        || !number(actual.value(QStringLiteral("declaredMaxSpeedMetersPerSecond")), -1.0, strict ? MaximumNumber : 1000.0, &speed)
        || (known.toBool() ? speed < 0.0 : speed != -1.0)
        || (known.toBool() && speed < expected.value(QStringLiteral("policy")).toMap().value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble())) return false;
    if (!strict) return true;
    if (!known.toBool() || !flag(actual, QStringLiteral("requirePerformanceEvidence"), true)
        || !flag(actual, QStringLiteral("declaredPerformanceEvidenceUsable"), true)
        || actual.value(QStringLiteral("declaredPerformanceEvidence")).type() != QVariant::Map) return false;
    const QVariantMap evidence = actual.value(QStringLiteral("declaredPerformanceEvidence")).toMap();
    if (!exact(evidence, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("vehicleId"), QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"),
        QStringLiteral("capabilityReviewIdentity"), QStringLiteral("configurationBaselineIdentity"), QStringLiteral("performanceCertified"), QStringLiteral("hardwareIdentityAuthenticated"),
        QStringLiteral("executionPermitted"), QStringLiteral("flightCommandReleased")}) || !integer(evidence.value(QStringLiteral("schemaVersion")), 1, 1)
        || !text(evidence.value(QStringLiteral("source")), 64) || evidence.value(QStringLiteral("source")).toString() != QStringLiteral("current-local-engineering-declared-performance")
        || !same(evidence.value(QStringLiteral("vehicleId")), expected.value(QStringLiteral("vehicleId")))
        || !number(evidence.value(QStringLiteral("maxSpeedMps")), 0.0, MaximumNumber) || evidence.value(QStringLiteral("maxSpeedMps")).toDouble() != speed
        || !number(evidence.value(QStringLiteral("enduranceSeconds")), -1.0, MaximumNumber)
        || (evidence.value(QStringLiteral("enduranceSeconds")).toDouble() <= 0.0 && evidence.value(QStringLiteral("enduranceSeconds")).toDouble() != -1.0)
        || evidence.value(QStringLiteral("capabilityReviewIdentity")).type() != QVariant::Map || evidence.value(QStringLiteral("configurationBaselineIdentity")).type() != QVariant::Map) return false;
    for (const QString& key : {QStringLiteral("performanceCertified"), QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("executionPermitted"), QStringLiteral("flightCommandReleased")})
        if (!flag(evidence, key, false)) return false;
    const QVariantMap review = evidence.value(QStringLiteral("capabilityReviewIdentity")).toMap();
    const QVariantMap baseline = evidence.value(QStringLiteral("configurationBaselineIdentity")).toMap();
    if (!exact(review, {QStringLiteral("capabilityReviewContext"), QStringLiteral("capabilityApproval")})
        || review.value(QStringLiteral("capabilityReviewContext")).type() != QVariant::Map
        || !FleetCapabilityEvidence::contextValid(review.value(QStringLiteral("capabilityReviewContext")).toMap())
        || review.value(QStringLiteral("capabilityApproval")).type() != QVariant::Map) return false;
    const QVariantMap context = review.value(QStringLiteral("capabilityReviewContext")).toMap();
    const QVariantMap approval = review.value(QStringLiteral("capabilityApproval")).toMap();
    if (!integer(context.value(QStringLiteral("vehicleId")), expected.value(QStringLiteral("vehicleId")).toInt(), expected.value(QStringLiteral("vehicleId")).toInt())
        || !exact(approval, {QStringLiteral("schemaVersion"), QStringLiteral("approvalId"), QStringLiteral("registryEpoch"), QStringLiteral("vehicleRecordEpoch"),
            QStringLiteral("capabilityRevision"), QStringLiteral("identityFingerprint"), QStringLiteral("configurationFingerprint"), QStringLiteral("capabilities"),
            QStringLiteral("source"), QStringLiteral("state"), QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("flightCommandReleased")})
        || !integer(approval.value(QStringLiteral("schemaVersion")), 1, 1) || !uuid(approval.value(QStringLiteral("approvalId")))
        || !text(approval.value(QStringLiteral("source")), 64) || approval.value(QStringLiteral("source")).toString() != QStringLiteral("local-engineering-attestation")
        || !text(approval.value(QStringLiteral("state")), 16) || approval.value(QStringLiteral("state")).toString() != QStringLiteral("current")
        || !flag(approval, QStringLiteral("hardwareIdentityAuthenticated"), false) || !flag(approval, QStringLiteral("flightCommandReleased"), false)) return false;
    for (const QString& key : {QStringLiteral("registryEpoch"), QStringLiteral("vehicleRecordEpoch"), QStringLiteral("capabilityRevision"),
            QStringLiteral("identityFingerprint"), QStringLiteral("configurationFingerprint")}) if (!same(approval.value(key), context.value(key))) return false;
    QStringList capabilities;
    if (!FleetCapabilityEvidence::canonicalCapabilities(approval.value(QStringLiteral("capabilities")), &capabilities)
        || capabilities.isEmpty() || !same(approval.value(QStringLiteral("capabilities")), capabilities)) return false;
    if (!exact(baseline, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("registryContext"), QStringLiteral("observationEpoch"),
            QStringLiteral("observationRevision"), QStringLiteral("currentFullKnown"), QStringLiteral("baselineFullKnown"), QStringLiteral("baselineMatches"),
            QStringLiteral("currentTypedFingerprint"), QStringLiteral("baselineTypedFingerprint"), QStringLiteral("baselineApprovalHash"),
            QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("performanceCertified"), QStringLiteral("flightCommandReleased")})
        || !integer(baseline.value(QStringLiteral("schemaVersion")), 1, 1) || !text(baseline.value(QStringLiteral("source")), 64)
        || baseline.value(QStringLiteral("source")).toString() != QStringLiteral("native-local-configuration-baseline")
        || baseline.value(QStringLiteral("registryContext")).type() != QVariant::Map || !same(baseline.value(QStringLiteral("registryContext")), context)
        || !uuid(baseline.value(QStringLiteral("observationEpoch"))) || !text(baseline.value(QStringLiteral("observationRevision")), 20)
        || !digest(baseline.value(QStringLiteral("currentTypedFingerprint"))) || !digest(baseline.value(QStringLiteral("baselineTypedFingerprint")))
        || !same(baseline.value(QStringLiteral("currentTypedFingerprint")), baseline.value(QStringLiteral("baselineTypedFingerprint")))
        || !digest(baseline.value(QStringLiteral("baselineApprovalHash")))) return false;
    bool revisionOk = false;
    const quint64 revision = baseline.value(QStringLiteral("observationRevision")).toString().toULongLong(&revisionOk);
    if (!revisionOk || revision == 0 || QString::number(revision) != baseline.value(QStringLiteral("observationRevision")).toString()) return false;
    for (const QString& key : {QStringLiteral("currentFullKnown"), QStringLiteral("baselineFullKnown"), QStringLiteral("baselineMatches")})
        if (!flag(baseline, key, true)) return false;
    for (const QString& key : {QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("performanceCertified"), QStringLiteral("flightCommandReleased")})
        if (!flag(baseline, key, false)) return false;
    // Retained shape only: current native declarations are re-captured by the
    // ordinary new review, never reconstructed from these historical maps.
    return true;
}
QVariantMap rejected(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("task"), QVariantMap()},
        {QStringLiteral("summary"), QVariantMap()}, {QStringLiteral("provenance"), QVariantMap()},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}
bool matchingMember(const QVariantMap& provenance, const QVariantMap& container, const QVariantList& members, const QVariantMap& boundary)
{
    if (members.size() != 1 || members.first().type() != QVariant::Map || container.value(QStringLiteral("constraints")).type() != QVariant::Map) return false;
    bool strict = false;
    const QVariantMap member = members.first().toMap();
    return strictPolicy(container, &strict) && integer(member.value(QStringLiteral("vehicleId")), provenance.value(QStringLiteral("reserveVehicleId")).toInt(), provenance.value(QStringLiteral("reserveVehicleId")).toInt())
        && hash(routeProjection(member, container.value(QStringLiteral("constraints")).toMap(), boundary, strict)) == provenance.value(QStringLiteral("newRouteHash")).toString();
}
bool mixedSource(const QVariantMap& map)
{
    return map.contains(QStringLiteral("planningSource")) || map.contains(QStringLiteral("remainderPlanningSource")) || map.contains(QStringLiteral("groupRemainderPlanningSource"));
}
} // namespace

QVariantMap FleetReserveDispatchPlanner::validateProvenance(const QVariantMap& value)
{
    const auto result = [&](bool ok) { return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), ok ? QStringLiteral("") : QStringLiteral("reserve-dispatch-provenance-invalid")},
        {QStringLiteral("provenance"), ok ? value : QVariantMap()}}; };
    if (!bounded(value, 64 * 1024) || !exact(value, {QStringLiteral("schemaVersion"), QStringLiteral("sourceKind"), QStringLiteral("sourceBinding"),
        QStringLiteral("sourceTaskHash"), QStringLiteral("sourcePlanHash"), QStringLiteral("sourceDescriptorHash"), QStringLiteral("sourcePrimaryRouteHash"), QStringLiteral("sourceReserveBudgetHash"),
        QStringLiteral("reserveVehicleId"), QStringLiteral("replacementVehicleId"), QStringLiteral("derivedTaskHash"), QStringLiteral("newRouteHash"), QStringLiteral("routeBoundaryPolicy"),
        QStringLiteral("historicalOnly"), QStringLiteral("authorityRestored"), QStringLiteral("flightCommandReleased"), QStringLiteral("executable")})
        || !integer(value.value(QStringLiteral("schemaVersion")), 1, 1) || !text(value.value(QStringLiteral("sourceKind")), 64) || value.value(QStringLiteral("sourceKind")).toString() != SourceKind
        || value.value(QStringLiteral("sourceBinding")).type() != QVariant::Map || !bindingValid(value.value(QStringLiteral("sourceBinding")).toMap())
        || !integer(value.value(QStringLiteral("reserveVehicleId")), 1, 255) || !integer(value.value(QStringLiteral("replacementVehicleId")), 1, 255)
        || same(value.value(QStringLiteral("reserveVehicleId")), value.value(QStringLiteral("replacementVehicleId")))
        || !flag(value, QStringLiteral("historicalOnly"), true) || !flag(value, QStringLiteral("authorityRestored"), false)
        || !flag(value, QStringLiteral("flightCommandReleased"), false) || !flag(value, QStringLiteral("executable"), false)
        || value.value(QStringLiteral("routeBoundaryPolicy")).type() != QVariant::Map
        || !flag(FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(value.value(QStringLiteral("routeBoundaryPolicy")).toMap()), QStringLiteral("ok"), true)) return result(false);
    for (const QString& key : {QStringLiteral("sourceTaskHash"), QStringLiteral("sourcePlanHash"), QStringLiteral("sourceDescriptorHash"), QStringLiteral("sourcePrimaryRouteHash"),
        QStringLiteral("sourceReserveBudgetHash"), QStringLiteral("derivedTaskHash"), QStringLiteral("newRouteHash")}) if (!digest(value.value(key))) return result(false);
    return result(same(value.value(QStringLiteral("sourcePlanHash")), value.value(QStringLiteral("sourceBinding")).toMap().value(QStringLiteral("contentHash"))));
}

bool FleetReserveDispatchPlanner::matchesTask(const QVariantMap& provenance, const QVariantMap& task)
{
    if (!flag(validateProvenance(provenance), QStringLiteral("ok"), true) || !bounded(task, MaximumTaskBytes) || mixedSource(task)
        || (task.contains(SourceKey) && !same(task.value(SourceKey), provenance)) || hash(taskProjection(task)) != provenance.value(QStringLiteral("derivedTaskHash")).toString()
        || task.value(QStringLiteral("assignments")).type() != QVariant::List || task.value(QStringLiteral("assignments")).isNull()) return false;
    if (!integer(task.value(QStringLiteral("vehicleCount")), 1, 1) || !integer(task.value(QStringLiteral("reserveCount")), 0, 0)
        || task.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List || task.value(QStringLiteral("reserveVehicleIds")).isNull()
        || !task.value(QStringLiteral("reserveVehicleIds")).toList().isEmpty()
        || !flag(task, QStringLiteral("flightCommandReleased"), false) || !flag(task, QStringLiteral("executable"), false)) return false;
    const QVariantList members = task.value(QStringLiteral("assignments")).toList();
    const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(task.value(QStringLiteral("area")), members);
    return flag(boundary, QStringLiteral("ok"), true) && same(boundary.value(QStringLiteral("policy")), provenance.value(QStringLiteral("routeBoundaryPolicy")))
        && matchingMember(provenance, task, members, boundary.value(QStringLiteral("policy")).toMap());
}

bool FleetReserveDispatchPlanner::matchesPlan(const QVariantMap& provenance, const QVariantMap& plan)
{
    if (!flag(validateProvenance(provenance), QStringLiteral("ok"), true) || !bounded(plan, MaximumTaskBytes) || mixedSource(plan)
        || !integer(plan.value(QStringLiteral("schemaVersion")), 1, 1)
        || (plan.contains(SourceKey) && !same(plan.value(SourceKey), provenance)) || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("assignments")).type() != QVariant::List || !same(plan.value(QStringLiteral("members")), plan.value(QStringLiteral("assignments")))
        || plan.value(QStringLiteral("routeBoundaryPolicy")).type() != QVariant::Map || !same(plan.value(QStringLiteral("routeBoundaryPolicy")), provenance.value(QStringLiteral("routeBoundaryPolicy")))
        || (plan.contains(QStringLiteral("coveragePlan")) && (plan.value(QStringLiteral("coveragePlan")).type() != QVariant::Map || !plan.value(QStringLiteral("coveragePlan")).toMap().isEmpty()))) return false;
    for (const QString& key : {QStringLiteral("reserveVehicleIds"), QStringLiteral("reserveMembers")})
        if (plan.value(key).type() != QVariant::List || plan.value(key).isNull() || !plan.value(key).toList().isEmpty()) return false;
    return matchingMember(provenance, plan, plan.value(QStringLiteral("members")).toList(), plan.value(QStringLiteral("routeBoundaryPolicy")).toMap());
}

QVariantMap FleetReserveDispatchPlanner::build(const QVariantMap& source, const QVariantMap& request)
{
    if (!bounded(source, MaximumSourceBytes) || !exact(source, {QStringLiteral("binding"), QStringLiteral("task"), QStringLiteral("plan")})
        || !bounded(request, 128 * 1024) || !exact(request, {QStringLiteral("reserveVehicleId"), QStringLiteral("executionProfile"), QStringLiteral("operatorConfirmed")})
        || !flag(request, QStringLiteral("operatorConfirmed"), true)) return rejected(QStringLiteral("reserve-dispatch-complete-source-request-required"));
    for (const QString& key : {QStringLiteral("binding"), QStringLiteral("task"), QStringLiteral("plan")})
        if (source.value(key).type() != QVariant::Map || source.value(key).isNull()) return rejected(QStringLiteral("reserve-dispatch-source-map-required"));
    const QVariantMap binding = source.value(QStringLiteral("binding")).toMap(), originalTask = source.value(QStringLiteral("task")).toMap(), plan = source.value(QStringLiteral("plan")).toMap();
    bool strict = false, taskStrict = false;
    int reserveId = 0;
    if (!bindingValid(binding) || !integer(plan.value(QStringLiteral("schemaVersion")), 1, 1)
        || !same(plan.value(QStringLiteral("planId")), binding.value(QStringLiteral("planId")))
        || !same(plan.value(QStringLiteral("sourceTaskId")), binding.value(QStringLiteral("sourceTaskId")))
        || !same(originalTask.value(QStringLiteral("taskId")), binding.value(QStringLiteral("sourceTaskId")))
        || hash(plan) != binding.value(QStringLiteral("contentHash")).toString()
        || !strictPolicy(plan, &strict) || !strictPolicy(originalTask, &taskStrict) || strict != taskStrict
        || !integer(request.value(QStringLiteral("reserveVehicleId")), 1, 255, &reserveId)
        || !estimateOnly(originalTask) || !estimateOnly(plan.value(QStringLiteral("routeTiming")).toMap())
        || plan.value(QStringLiteral("constraints")).type() != QVariant::Map || plan.value(QStringLiteral("constraints")).isNull())
        return rejected(QStringLiteral("reserve-dispatch-source-binding-policy-invalid"));
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    if (originalTask.contains(QStringLiteral("constraints"))
        && (originalTask.value(QStringLiteral("constraints")).type() != QVariant::Map || originalTask.value(QStringLiteral("constraints")).isNull()))
        return rejected(QStringLiteral("reserve-dispatch-original-constraints-mismatch"));
    // Reproduce the Compiler's actual closed alias merge, preserving every
    // unknown key in the original map and filling only absent known keys.
    QVariantMap originalConstraints = originalTask.value(QStringLiteral("constraints")).toMap();
    for (const QString& key : {QStringLiteral("timeLimitSeconds"), QStringLiteral("deadline"), QStringLiteral("startAt"),
            QStringLiteral("minimumSeparationMeters"), QStringLiteral("maxAltitudeMeters"), QStringLiteral("maxAltitudeReference"),
            QStringLiteral("geofenceRequired"), QStringLiteral("returnReserveSeconds")})
        if (!originalConstraints.contains(key) && originalTask.contains(key)) originalConstraints.insert(key, originalTask.value(key));
    if (!same(originalConstraints, constraints)) return rejected(QStringLiteral("reserve-dispatch-original-constraints-mismatch"));
    if (!estimateOnly(constraints)) return rejected(QStringLiteral("reserve-dispatch-absolute-timing-replan-required"));
    if (!number(constraints.value(QStringLiteral("maxAltitudeMeters")), -1000.0, 20000.0)
        || !text(constraints.value(QStringLiteral("maxAltitudeReference")), 16)
        || constraints.value(QStringLiteral("maxAltitudeReference")).toString() != QStringLiteral("amsl"))
        return rejected(QStringLiteral("reserve-dispatch-source-amsl-ceiling-required"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList(), reserveIds = plan.value(QStringLiteral("reserveVehicleIds")).toList(), reserveRows = plan.value(QStringLiteral("reserveMembers")).toList();
    if (plan.value(QStringLiteral("members")).type() != QVariant::List || members.isEmpty() || members.size() > 32
        || plan.value(QStringLiteral("assignments")).type() != QVariant::List || !same(plan.value(QStringLiteral("assignments")), members)
        || plan.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List || reserveIds.isEmpty() || reserveIds.size() > 32
        || plan.value(QStringLiteral("reserveMembers")).type() != QVariant::List || reserveRows.size() != reserveIds.size()
        || originalTask.value(QStringLiteral("reservePlans")).type() != QVariant::List || originalTask.value(QStringLiteral("reservePlans")).toList().size() != reserveIds.size())
        return rejected(QStringLiteral("reserve-dispatch-complete-members-reserves-required"));
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariantMap originalBoundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(originalTask.value(QStringLiteral("area")), members);
        if (plan.value(QStringLiteral("routeBoundaryPolicy")).type() != QVariant::Map || plan.value(QStringLiteral("routeBoundaryPolicy")).isNull()
            || !flag(originalBoundary, QStringLiteral("ok"), true) || !same(originalBoundary.value(QStringLiteral("policy")), plan.value(QStringLiteral("routeBoundaryPolicy"))))
            return rejected(QStringLiteral("reserve-dispatch-original-boundary-mismatch"));
    }
    QSet<int> activeSet, reserveSet; QVariantMap descriptor, budget, primary;
    int originalPointCount = 0;
    for (const QVariant& value : members) {
        int id = 0;
        if (value.type() != QVariant::Map || !integer(value.toMap().value(QStringLiteral("vehicleId")), 1, 255, &id) || activeSet.contains(id))
            return rejected(QStringLiteral("reserve-dispatch-source-members-invalid"));
        int memberPoints = 0;
        for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            const QVariant route = value.toMap().value(key);
            if (route.type() != QVariant::List || route.isNull() || route.toList().size() < 2 || route.toList().size() > 2048 - memberPoints)
                return rejected(QStringLiteral("reserve-dispatch-source-route-budget-invalid"));
            memberPoints += route.toList().size();
        }
        if (originalPointCount > 8192 - memberPoints) return rejected(QStringLiteral("reserve-dispatch-source-route-budget-invalid"));
        originalPointCount += memberPoints;
        activeSet.insert(id);
    }
    for (int index = 0; index < reserveIds.size(); ++index) {
        int id = 0;
        const QVariantMap row = reserveRows.at(index).toMap();
        if (!integer(reserveIds.at(index), 1, 255, &id) || reserveSet.contains(id) || activeSet.contains(id)
            || reserveRows.at(index).type() != QVariant::Map
            || !exact(row, {QStringLiteral("vehicleId"), QStringLiteral("role"), QStringLiteral("manualConfirmationRequired"), QStringLiteral("energyPlan")})
            || !integer(row.value(QStringLiteral("vehicleId")), id, id)
            || !text(row.value(QStringLiteral("role")), 16) || row.value(QStringLiteral("role")).toString() != QStringLiteral("reserve")
            || !flag(row, QStringLiteral("manualConfirmationRequired"), true) || row.value(QStringLiteral("energyPlan")).type() != QVariant::Map)
            return rejected(QStringLiteral("reserve-dispatch-source-reserves-invalid"));
        reserveSet.insert(id); if (id == reserveId) budget = row.value(QStringLiteral("energyPlan")).toMap();
    }
    QSet<int> descriptorIds;
    for (const QVariant& value : originalTask.value(QStringLiteral("reservePlans")).toList()) {
        int id = 0;
        const QVariantMap row = value.toMap();
        if (value.type() != QVariant::Map || !exact(row, {QStringLiteral("vehicleId"), QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("reservePlan")})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255, &id) || !reserveSet.contains(id) || descriptorIds.contains(id)
            || row.value(QStringLiteral("reservePlan")).type() != QVariant::Map) return rejected(QStringLiteral("reserve-dispatch-descriptors-invalid"));
        QVariantMap start, returning;
        const QVariantMap normalized = FleetReserveEnergyPlan::normalize(row.value(QStringLiteral("reservePlan")).toMap());
        if (!flag(normalized, QStringLiteral("ok"), true) || !activeSet.contains(normalized.value(QStringLiteral("policy")).toMap().value(QStringLiteral("replacementVehicleId")).toInt())
            || !point(row.value(QStringLiteral("start")), true, 0, &start) || !point(row.value(QStringLiteral("returnTo")), true, 1, &returning)) return rejected(QStringLiteral("reserve-dispatch-descriptors-invalid"));
        descriptorIds.insert(id); if (id == reserveId) descriptor = row;
    }
    if (descriptor.isEmpty() || budget.isEmpty()) return rejected(QStringLiteral("reserve-dispatch-selected-reserve-not-found"));
    const QVariantMap policy = FleetReserveEnergyPlan::normalize(descriptor.value(QStringLiteral("reservePlan")).toMap()).value(QStringLiteral("policy")).toMap();
    const int replacementId = policy.value(QStringLiteral("replacementVehicleId")).toInt();
    for (const QVariant& value : members) if (value.toMap().value(QStringLiteral("vehicleId")).toInt() == replacementId) primary = value.toMap();
    const QVariantMap rebuilt = FleetReserveEnergyPlan::build(descriptor, members, constraints, QVariantMap(), QVariant(false));
    if (!flag(rebuilt, QStringLiteral("complete"), true) || !budgetMatches(rebuilt, budget, strict)) return rejected(QStringLiteral("reserve-dispatch-original-budget-mismatch"));
    const QString role = primary.value(QStringLiteral("role")).toString();
    if (!text(primary.value(QStringLiteral("role")), 16) || !QStringList{QStringLiteral("leader"), QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")}.contains(role)
        || !estimateOnly(primary) || primary.value(QStringLiteral("waypoints")).type() != QVariant::List) return rejected(QStringLiteral("reserve-dispatch-primary-policy-unsupported"));
    QVariant rawAssignments; bool found = false; QVariantMap raw;
    for (const QString& key : {QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members")}) {
        if (!originalTask.contains(key)) continue;
        if (found && !same(rawAssignments, originalTask.value(key))) return rejected(QStringLiteral("reserve-dispatch-assignment-alias-conflict"));
        rawAssignments = originalTask.value(key); found = true;
    }
    if (found) {
        if (rawAssignments.type() != QVariant::List || rawAssignments.toList().size() > 32) return rejected(QStringLiteral("reserve-dispatch-assignment-policy-invalid"));
        QSet<int> seen;
        for (const QVariant& value : rawAssignments.toList()) {
            int id = 0;
            if (value.type() != QVariant::Map || !assignmentId(value.toMap(), &id) || seen.contains(id) || !activeSet.contains(id))
                return rejected(QStringLiteral("reserve-dispatch-assignment-policy-invalid"));
            seen.insert(id); if (id == replacementId) raw = value.toMap();
        }
    }
    if (!estimateOnly(raw)) return rejected(QStringLiteral("reserve-dispatch-primary-absolute-timing-unsupported"));
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("id")})
        if (raw.contains(key) && !integer(raw.value(key), replacementId, replacementId)) return rejected(QStringLiteral("reserve-dispatch-primary-alias-conflict"));
    for (const QString& key : {QStringLiteral("role"), QStringLiteral("assignmentRole")})
        if (raw.contains(key) && (!text(raw.value(key), 16) || raw.value(key).toString().trimmed().toLower() != role)) return rejected(QStringLiteral("reserve-dispatch-primary-alias-conflict"));
    if (request.value(QStringLiteral("executionProfile")).type() != QVariant::Map || request.value(QStringLiteral("executionProfile")).toMap().isEmpty()) return rejected(QStringLiteral("reserve-dispatch-new-profile-required"));
    const QVariantMap profileResult = FleetMissionExecutionProfile::normalize(request.value(QStringLiteral("executionProfile")).toMap());
    if (!flag(profileResult, QStringLiteral("ok"), true)) return rejected(QStringLiteral("reserve-dispatch-new-profile-invalid"));
    const QVariantMap profile = profileResult.value(QStringLiteral("profile")).toMap(), vertical = profile.value(QStringLiteral("verticalPolicy")).toMap();
    const double cruise = policy.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble();
    if (profile.value(QStringLiteral("timingPolicy")).toString() != QStringLiteral("relative-dwell")
        || profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble() != cruise
        || profile.value(QStringLiteral("returnSpeedMetersPerSecond"), cruise).toDouble() != cruise
        || !same(vertical.value(QStringLiteral("climbSpeedMetersPerSecond")), policy.value(QStringLiteral("climbSpeedMetersPerSecond")))
        || !same(vertical.value(QStringLiteral("descentSpeedMetersPerSecond")), policy.value(QStringLiteral("descentSpeedMetersPerSecond")))) return rejected(QStringLiteral("reserve-dispatch-profile-budget-speed-conflict"));
    const QVariantList originalRoute = primary.value(QStringLiteral("waypoints")).toList();
    const int actions = 1 + (profile.contains(QStringLiteral("returnSpeedMetersPerSecond")) ? 1 : 0)
        + (profile.value(QStringLiteral("launchAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("takeoff") ? 1 : 0)
        + (QStringList{QStringLiteral("hold"), QStringLiteral("land")}.contains(profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString()) ? 1 : 0);
    if (originalRoute.isEmpty() || originalRoute.size() + 3 + actions > 2048) return rejected(QStringLiteral("reserve-dispatch-item-budget-exceeded"));
    QVariantMap start, returning;
    if (!point(descriptor.value(QStringLiteral("start")), true, 0, &start) || !point(descriptor.value(QStringLiteral("returnTo")), true, 1, &returning)) return rejected(QStringLiteral("reserve-dispatch-endpoint-invalid"));
    start.insert(QStringLiteral("holdSeconds"), 0.0); returning.insert(QStringLiteral("holdSeconds"), 0.0);
    QVariantList outbound{start};
    for (int index = 0; index < originalRoute.size(); ++index) {
        QVariantMap current;
        if (!point(originalRoute.at(index), false, index, &current)) return rejected(QStringLiteral("reserve-dispatch-full-original-route-invalid"));
        current.insert(QStringLiteral("sequence"), index + 1); outbound.append(current);
    }
    QVariantMap returnStart = outbound.last().toMap();
    returnStart.insert(QStringLiteral("sequence"), 0); returnStart.insert(QStringLiteral("holdSeconds"), 0.0); returnStart.remove(QStringLiteral("heading"));
    QVariantMap retained = raw;
    for (auto it = primary.constBegin(); it != primary.constEnd(); ++it) if (!retained.contains(it.key())) retained.insert(it.key(), it.value());
    QVariantMap member = memberPolicies(retained), task = policies(originalTask);
    double durationFloor = 0.0, returnFloor = 0.0, minimumSpeed = cruise;
    bool returnSupplied = false;
    for (const QVariantMap& scope : {originalTask, constraints})
        for (const QString& key : {QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds")}) {
            if (scope.contains(key)) returnSupplied = true;
            if (!floor(scope, {key}, &returnFloor)) return rejected(QStringLiteral("reserve-dispatch-global-floor-invalid"));
        }
    if (!returnSupplied) returnFloor = 120.0;
    for (const QVariantMap& scope : {primary, raw}) {
        if (!floor(scope, {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"), QStringLiteral("timeLimitSeconds")}, &durationFloor, true)
            || !floor(scope, {QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds")}, &returnFloor)
            || !floor(scope, {QStringLiteral("returnDurationSeconds")}, &returnFloor, true)
            || !floor(scope, {QStringLiteral("minimumSpeedMps")}, &minimumSpeed)) return rejected(QStringLiteral("reserve-dispatch-member-floor-invalid"));
    }
    if (!floor(originalTask, {QStringLiteral("minimumSpeedMps")}, &minimumSpeed)) return rejected(QStringLiteral("reserve-dispatch-global-floor-invalid"));
    member.insert(QStringLiteral("vehicleId"), reserveId); member.insert(QStringLiteral("role"), role);
    member.insert(QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")); member.insert(QStringLiteral("altitudeReference"), QStringLiteral("amsl"));
    member.insert(QStringLiteral("returnRequired"), true); member.insert(QStringLiteral("waypoints"), outbound); member.insert(QStringLiteral("returnPath"), QVariantList{returnStart, returning});
    member.insert(QStringLiteral("executionProfile"), profile); member.insert(QStringLiteral("minimumSpeedMps"), minimumSpeed); member.insert(QStringLiteral("returnReserveSeconds"), returnFloor);
    if (durationFloor > 0.0) member.insert(QStringLiteral("durationSeconds"), durationFloor);
    const double ingress = rebuilt.value(QStringLiteral("estimatedIngressSeconds")).toDouble(), work = rebuilt.value(QStringLiteral("estimatedTaskSeconds")).toDouble();
    member.insert(QStringLiteral("estimatedDurationSeconds"), ingress + work); member.insert(QStringLiteral("estimatedReturnDurationSeconds"), rebuilt.value(QStringLiteral("estimatedReturnSeconds")));
    member.insert(QStringLiteral("pathLengthMeters"), rebuilt.value(QStringLiteral("outboundDistanceMeters"))); member.insert(QStringLiteral("returnPathLengthMeters"), rebuilt.value(QStringLiteral("returnDistanceMeters")));
    member.insert(QStringLiteral("timingEvidence"), QStringLiteral("constant-speed-estimate")); member.insert(QStringLiteral("returnTimingEvidence"), QStringLiteral("constant-speed-estimate"));
    member.insert(QStringLiteral("routeSource"), QStringLiteral("explicit-reserve-dispatch")); member.insert(QStringLiteral("returnRouteSource"), QStringLiteral("explicit-reserve-dispatch-return"));
    task.insert(QStringLiteral("constraints"), constraints); task.insert(QStringLiteral("assignments"), QVariantList{member});
    task.insert(QStringLiteral("vehicleCount"), 1); task.insert(QStringLiteral("minVehicleCount"), 1); task.insert(QStringLiteral("candidateVehicleIds"), QVariantList{reserveId});
    task.insert(QStringLiteral("reserveCount"), 0); task.insert(QStringLiteral("reserveVehicleIds"), QVariantList()); task.insert(QStringLiteral("leaderRequired"), role == QStringLiteral("leader"));
    task.insert(QStringLiteral("requiredRoles"), QStringList{role});
    for (const QString& name : {QStringLiteral("leader"), QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")}) task.insert(name + QStringLiteral("Count"), name == role ? 1 : 0);
    task.insert(QStringLiteral("target"), QVariantMap{{QStringLiteral("kind"), SourceKind}, {QStringLiteral("reserveVehicleId"), reserveId}, {QStringLiteral("replacementVehicleId"), replacementId}});
    task.insert(QStringLiteral("flightCommandReleased"), false); task.insert(QStringLiteral("executable"), false);
    const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(task.value(QStringLiteral("area")), QVariantList{member});
    if (!flag(boundary, QStringLiteral("ok"), true)) return rejected(QStringLiteral("reserve-dispatch-boundary-policy-invalid"));
    QVariantMap provenance{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("sourceKind"), SourceKind}, {QStringLiteral("sourceBinding"), binding},
        {QStringLiteral("sourceTaskHash"), hash(originalTask)}, {QStringLiteral("sourcePlanHash"), hash(plan)}, {QStringLiteral("sourceDescriptorHash"), hash(descriptor)},
        {QStringLiteral("sourcePrimaryRouteHash"), hash(originalRoute)}, {QStringLiteral("sourceReserveBudgetHash"), hash(budget)},
        {QStringLiteral("reserveVehicleId"), reserveId}, {QStringLiteral("replacementVehicleId"), replacementId}, {QStringLiteral("derivedTaskHash"), hash(taskProjection(task))},
        {QStringLiteral("newRouteHash"), hash(routeProjection(member, constraints, boundary.value(QStringLiteral("policy")).toMap(), strict))},
        {QStringLiteral("routeBoundaryPolicy"), boundary.value(QStringLiteral("policy"))}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authorityRestored"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    task.insert(SourceKey, provenance);
    if (!matchesTask(provenance, task)) return rejected(QStringLiteral("reserve-dispatch-task-budget-or-binding-invalid"));
    const QVariantMap summary{{QStringLiteral("reserveVehicleId"), reserveId}, {QStringLiteral("replacementVehicleId"), replacementId}, {QStringLiteral("role"), role},
        {QStringLiteral("outboundPointCount"), outbound.size()}, {QStringLiteral("returnPointCount"), 2}, {QStringLiteral("estimatedIngressSeconds"), ingress},
        {QStringLiteral("estimatedTaskSeconds"), work}, {QStringLiteral("retainedHoldSeconds"), rebuilt.value(QStringLiteral("targetHoldSeconds"))},
        {QStringLiteral("estimatedOutboundSeconds"), ingress + work + rebuilt.value(QStringLiteral("targetHoldSeconds")).toDouble()},
        {QStringLiteral("estimatedReturnSeconds"), rebuilt.value(QStringLiteral("estimatedReturnSeconds"))}, {QStringLiteral("standbyReplayed"), false},
        {QStringLiteral("routeClearanceVerified"), false}, {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("task"), task}, {QStringLiteral("summary"), summary},
        {QStringLiteral("provenance"), provenance}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    return bounded(result, MaximumSourceBytes) ? result : rejected(QStringLiteral("reserve-dispatch-result-budget-exceeded"));
}

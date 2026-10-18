#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetMissionRemainderArchive.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetReserveEnergyPlan.h"
#include "FleetCapabilityEvidence.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <cmath>

namespace {
constexpr double MaximumNumber = 9007199254740991.0;
constexpr qint64 MaximumSourceBytes = 32LL * 1024 * 1024;
constexpr qint64 MaximumTaskBytes = 16LL * 1024 * 1024;
const QString Kind = QStringLiteral("historical-run-group-planning");
const QString Selection = QStringLiteral("inclusive-waypoint-full-dwell-replay");
const QString Timing = QStringLiteral("sequential-horizontal-climb-descent-estimate");

bool number(const QVariant& value, double low, double high, double* out = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double parsed = value.toDouble();
    if (!std::isfinite(parsed) || parsed < low || parsed > high) return false;
    if (out) *out = parsed;
    return true;
}

bool integer(const QVariant& value, int low, int high, int* out = nullptr)
{
    double parsed = 0.0;
    if (!number(value, low, high, &parsed) || std::floor(parsed) != parsed) return false;
    if (out) *out = int(parsed);
    return true;
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
bool same(const QVariant& left, const QVariant& right) { return json(left) == json(right); }
bool sameValue(const QVariant& left, const QVariant& right)
{
    // QJsonDocument::fromVariant only has a document for map/list roots.
    // Wrap scalar fields so two different scalar values cannot both become
    // an empty document. Keep the old container-only helper unchanged.
    return json(QVariantList{left}) == json(QVariantList{right});
}
QString hash(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}

bool tree(const QVariant& value, int depth, int* nodes, qint64* amount, qint64 limit)
{
    if (depth > 24 || ++(*nodes) > 1048576) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += it.key().size() * qint64(2);
            if (*amount > limit || !tree(it.value(), depth + 1, nodes, amount, limit)) return false;
        }
        return true;
    }
    case QVariant::List:
        if (value.toList().size() > 32768) return false;
        for (const QVariant& entry : value.toList()) if (!tree(entry, depth + 1, nodes, amount, limit)) return false;
        return true;
    case QVariant::StringList:
        if (value.toStringList().size() > 32768) return false;
        for (const QString& entry : value.toStringList()) if (!tree(entry, depth + 1, nodes, amount, limit)) return false;
        return true;
    case QVariant::String:
        *amount += value.toString().size() * qint64(2);
        return value.toString().size() <= 32768 && *amount <= limit
            && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: return !value.isNull();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        return number(value, -MaximumNumber, MaximumNumber);
    default: return false;
    }
}

bool bounded(const QVariantMap& value, qint64 limit)
{
    int nodes = 0; qint64 amount = 0;
    if (!tree(value, 0, &nodes, &amount, limit)) return false;
    const QByteArray bytes = json(value);
    return !bytes.isEmpty() && bytes.size() <= limit;
}

bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool role(const QVariant& value)
{
    return value.type() == QVariant::String && (value.toString() == QStringLiteral("leader")
        || value.toString() == QStringLiteral("scout") || value.toString() == QStringLiteral("relay")
        || value.toString() == QStringLiteral("responder"));
}

QVariantMap rejected(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("task"), QVariantMap()},
        {QStringLiteral("summary"), QVariantMap()}, {QStringLiteral("provenance"), QVariantMap()},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

QVariantMap provenanceResult(bool ok, const QVariantMap& value, const QString& reason)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason}, {QStringLiteral("provenance"), value}};
}

QVariantMap taskProjection(QVariantMap task)
{
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("intent"), QStringLiteral("taskId"),
            QStringLiteral("templateValidation"), QStringLiteral("groupRemainderPlanningSource")}) task.remove(key);
    return task;
}

QVariantMap memberProjection(QVariantMap member)
{
    // These are Compiler-added bookkeeping, not policy. All other retained
    // member fields participate: dropping an unsupported policy blocks match.
    for (const QString& key : {QStringLiteral("memberIndex"), QStringLiteral("waypointCount"),
            QStringLiteral("manualConfirmationRequired"), QStringLiteral("nominalRouteTiming")})
        member.remove(key);
    return member;
}

QVariantMap routeProjection(const QVariantMap& member, const QVariantMap& constraints)
{
    return {{QStringLiteral("member"), memberProjection(member)}, {QStringLiteral("constraints"), constraints}};
}

QVariantMap routesProjection(const QVariantList& members, const QVariantMap& constraints, const QVariantList& reserves,
    const QVariantList* reservePlans = nullptr, bool requirePerformanceEvidence = false)
{
    QVariantList projected;
    for (const QVariant& value : members) projected.append(memberProjection(value.toMap()));
    QVariantMap result{{QStringLiteral("members"), projected}, {QStringLiteral("constraints"), constraints},
        {QStringLiteral("reserveVehicleIds"), reserves}};
    if (reservePlans) {
        result.insert(QStringLiteral("reservePlans"), *reservePlans);
        result.insert(QStringLiteral("requirePerformanceEvidence"), requirePerformanceEvidence);
    }
    return result;
}

bool reserveEndpoint(const QVariant& value, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap point = value.toMap();
    if (!exact(point, {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
            QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")})
        || point.value(QStringLiteral("coordinateSystem")).type() != QVariant::String
        || point.value(QStringLiteral("coordinateSystem")).isNull()
        || point.value(QStringLiteral("coordinateSystem")).toString() != QStringLiteral("geodetic")
        || point.value(QStringLiteral("altitudeReference")).type() != QVariant::String
        || point.value(QStringLiteral("altitudeReference")).isNull()
        || point.value(QStringLiteral("altitudeReference")).toString() != QStringLiteral("amsl")) return false;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    if (!number(point.value(QStringLiteral("latitude")), -90.0, 90.0, &latitude)
        || !number(point.value(QStringLiteral("longitude")), -180.0, 180.0, &longitude)
        || !number(point.value(QStringLiteral("altitude")), -1000.0, 20000.0, &altitude)) return false;
    *output = {{QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")}, {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude}};
    return true;
}

bool normalizeReservePlans(const QVariant& supplied, const QVariantList& reserves, const QVariantList& targets,
    QVariantList* output)
{
    if (supplied.type() != QVariant::List || supplied.isNull() || reserves.size() > 32
        || supplied.toList().size() != reserves.size()) return false;
    QSet<int> reserveIds, targetIds;
    for (const QVariant& value : reserves) {
        int id = 0;
        if (!integer(value, 1, 255, &id) || reserveIds.contains(id)) return false;
        reserveIds.insert(id);
    }
    for (const QVariant& value : targets) {
        int id = 0;
        if (!integer(value, 1, 255, &id) || targetIds.contains(id) || reserveIds.contains(id)) return false;
        targetIds.insert(id);
    }
    QMap<int, QVariantMap> descriptors;
    for (const QVariant& value : supplied.toList()) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap descriptor = value.toMap();
        int id = 0;
        if (!exact(descriptor, {QStringLiteral("vehicleId"), QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("reservePlan")})
            || !integer(descriptor.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !reserveIds.contains(id) || descriptors.contains(id)
            || descriptor.value(QStringLiteral("reservePlan")).type() != QVariant::Map
            || descriptor.value(QStringLiteral("reservePlan")).isNull()) return false;
        const QVariantMap checked = FleetReserveEnergyPlan::normalize(descriptor.value(QStringLiteral("reservePlan")).toMap());
        if (!flag(checked, QStringLiteral("ok"), true)) return false;
        const QVariantMap policy = checked.value(QStringLiteral("policy")).toMap();
        int replacementId = 0;
        QVariantMap start, returnTo;
        if (!integer(policy.value(QStringLiteral("replacementVehicleId")), 1, 255, &replacementId)
            || !targetIds.contains(replacementId) || replacementId == id
            || !reserveEndpoint(descriptor.value(QStringLiteral("start")), &start)
            || !reserveEndpoint(descriptor.value(QStringLiteral("returnTo")), &returnTo)) return false;
        descriptors.insert(id, {{QStringLiteral("vehicleId"), id}, {QStringLiteral("start"), start},
            {QStringLiteral("returnTo"), returnTo}, {QStringLiteral("reservePlan"), policy}});
    }
    QVariantList canonical;
    for (const QVariant& value : reserves) {
        if (!descriptors.contains(value.toInt())) return false;
        canonical.append(descriptors.value(value.toInt()));
    }
    *output = canonical;
    return true;
}

QVariantMap commonPolicies(QVariantMap task)
{
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("healthReports"), QStringLiteral("nativeVehicleEvidence"),
            QStringLiteral("capabilityReviewEvidence"), QStringLiteral("templateValidation"), QStringLiteral("intent"),
            QStringLiteral("taskId"), QStringLiteral("id"), QStringLiteral("sourceTaskId"), QStringLiteral("coverage"),
            QStringLiteral("target"), QStringLiteral("planningSource"), QStringLiteral("remainderPlanningSource"),
            QStringLiteral("groupRemainderPlanningSource"), QStringLiteral("reserveDispatchPlanningSource"),
            QStringLiteral("waypoints"), QStringLiteral("route"),
            QStringLiteral("routeTiming"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"), QStringLiteral("returnPaths"),
            QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members"), QStringLiteral("vehicleIds"),
            QStringLiteral("targetVehicleIds"), QStringLiteral("candidateVehicleIds"), QStringLiteral("selectedVehicleIds"),
            QStringLiteral("reserve"), QStringLiteral("reserveVehicleIds"), QStringLiteral("reserveMembers"),
            QStringLiteral("roleAssignments"), QStringLiteral("rolePlan"), QStringLiteral("leaderVehicleId"), QStringLiteral("leaderId"),
            QStringLiteral("memberEnergyBudgets"), QStringLiteral("memberEnergyBudgetsDerivedFromPlan"), QStringLiteral("executionProfile"),
            QStringLiteral("routeBoundaryPolicy"), QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("deploymentId"), QStringLiteral("binding"),
            QStringLiteral("sourceEpoch"), QStringLiteral("approval"), QStringLiteral("approvalId"), QStringLiteral("approvalState"),
            QStringLiteral("executionPermit"), QStringLiteral("startupPermitGranted"), QStringLiteral("dispatch"),
            QStringLiteral("executable"), QStringLiteral("flightCommandReleased")}) task.remove(key);
    // In particular reservePlans, unknown bounded policies, all capability,
    // safety, template, no-fly, energy and time-limit policies are NOT erased.
    return task;
}

QVariantMap memberPolicies(QVariantMap member)
{
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("systemId"), QStringLiteral("id"),
            QStringLiteral("assignmentRole"), QStringLiteral("role"), QStringLiteral("memberIndex"), QStringLiteral("waypointCount"),
            QStringLiteral("manualConfirmationRequired"), QStringLiteral("flightCommandReleased"),
            QStringLiteral("coverageLegStart"), QStringLiteral("coverageLegCount"), QStringLiteral("start"), QStringLiteral("returnTo"),
            QStringLiteral("waypoints"), QStringLiteral("route"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"),
            QStringLiteral("executionProfile"), QStringLiteral("nominalRouteTiming"),
            QStringLiteral("estimatedDurationSeconds"), QStringLiteral("estimatedReturnDurationSeconds"),
            QStringLiteral("pathLengthMeters"), QStringLiteral("returnPathLengthMeters"), QStringLiteral("routeSource"),
            QStringLiteral("returnRouteSource"), QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
            QStringLiteral("ingressWaypoints"), QStringLiteral("surveyWaypoints"), QStringLiteral("estimatedIngressDurationSeconds"),
            QStringLiteral("ingressPathLengthMeters"), QStringLiteral("estimatedSurveyDurationSeconds"), QStringLiteral("surveyPathLengthMeters"),
            QStringLiteral("routeSections"), QStringLiteral("routeSectionIndexConvention"), QStringLiteral("launchLandingInferred"),
            QStringLiteral("ingressGenerated"), QStringLiteral("returnGenerated"), QStringLiteral("ingressOutsideArea"),
            QStringLiteral("returnOutsideArea"), QStringLiteral("endpointPathsClearanceVerified"), QStringLiteral("verticalSequencing"),
            QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
            QStringLiteral("returnDurationSeconds"), QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds"),
            QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence"), QStringLiteral("timeReference"),
            QStringLiteral("returnTimeReference")}) member.remove(key);
    return member;
}

bool includeFloor(const QVariantMap& source, const QStringList& keys, double low, double* maximum,
    bool* supplied = nullptr, bool positive = false)
{
    for (const QString& key : keys) {
        if (!source.contains(key)) continue;
        double value = 0.0;
        if (!number(source.value(key), low, MaximumNumber, &value) || (positive && value <= 0.0)) return false;
        *maximum = qMax(*maximum, value);
        if (supplied) *supplied = true;
    }
    return true;
}

bool estimatedTimingOnly(const QVariantMap& value)
{
    for (const QString& key : {QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"),
            QStringLiteral("deadline"), QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"),
            QStringLiteral("timeSeconds"), QStringLiteral("time"), QStringLiteral("timestampSeconds"),
            QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds")}) if (value.contains(key)) return false;
    for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")}) {
        if (!value.contains(key)) continue;
        if (value.value(key).type() != QVariant::String || !QStringList{QStringLiteral("estimated"), QStringLiteral("unknown"),
                QStringLiteral("estimate"), QStringLiteral("constant-speed-estimate")}.contains(value.value(key).toString())) return false;
    }
    for (const QString& key : {QStringLiteral("timeReference"), QStringLiteral("returnTimeReference")}) {
        if (!value.contains(key)) continue;
        if (value.value(key).type() != QVariant::String || !QStringList{QStringLiteral("relative"), QStringLiteral("relative-estimate"),
                QStringLiteral("unknown")}.contains(value.value(key).toString())) return false;
    }
    return value.value(QStringLiteral("executionProfile")).toMap().value(QStringLiteral("timingPolicy")).toString()
        != QStringLiteral("absolute-required");
}

bool originalRawMembers(const QVariantMap& task, const QVariantList& sourceIds, QMap<int, QVariantMap>* output)
{
    QVariant supplied;
    bool found = false;
    for (const QString& key : {QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members")}) {
        if (!task.contains(key)) continue;
        if (found && !same(supplied, task.value(key))) return false;
        supplied = task.value(key); found = true;
    }
    if (!found) return true;
    if (supplied.type() != QVariant::List || supplied.toList().size() > 32) return false;
    QSet<int> allowed;
    for (const QVariant& id : sourceIds) allowed.insert(id.toInt());
    for (const QVariant& entry : supplied.toList()) {
        int id = 0;
        if (entry.type() != QVariant::Map || !integer(entry.toMap().value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !allowed.contains(id) || output->contains(id)) return false;
        output->insert(id, entry.toMap());
    }
    return true;
}

int actionCount(const QVariantMap& profile)
{
    if (profile.isEmpty()) return 0;
    return 1 + (profile.contains(QStringLiteral("returnSpeedMetersPerSecond")) ? 1 : 0)
        + (profile.value(QStringLiteral("launchAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("takeoff") ? 1 : 0)
        + (QStringList{QStringLiteral("hold"), QStringLiteral("land")}.contains(
            profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString()) ? 1 : 0);
}

bool decimalCounter(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > 20) return false;
    bool ok = false;
    const quint64 parsed = value.toString().toULongLong(&ok);
    return ok && parsed != 0 && QString::number(parsed) == value.toString();
}

bool declaredPerformanceShape(const QVariantMap& evidence, int vehicleId, double maximumSpeed)
{
    if (!exact(evidence, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("vehicleId"),
            QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"), QStringLiteral("capabilityReviewIdentity"),
            QStringLiteral("configurationBaselineIdentity"), QStringLiteral("performanceCertified"),
            QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("executionPermitted"), QStringLiteral("flightCommandReleased")})
        || !integer(evidence.value(QStringLiteral("schemaVersion")), 1, 1)
        || evidence.value(QStringLiteral("source")).type() != QVariant::String || evidence.value(QStringLiteral("source")).isNull()
        || evidence.value(QStringLiteral("source")).toString() != QStringLiteral("current-local-engineering-declared-performance")
        || !integer(evidence.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !number(evidence.value(QStringLiteral("maxSpeedMps")), 0.0, MaximumNumber)
        || evidence.value(QStringLiteral("maxSpeedMps")).toDouble() != maximumSpeed
        || !number(evidence.value(QStringLiteral("enduranceSeconds")), -1.0, MaximumNumber)
        || (evidence.value(QStringLiteral("enduranceSeconds")).toDouble() <= 0.0
            && evidence.value(QStringLiteral("enduranceSeconds")).toDouble() != -1.0)
        || evidence.value(QStringLiteral("capabilityReviewIdentity")).type() != QVariant::Map
        || evidence.value(QStringLiteral("capabilityReviewIdentity")).isNull()
        || evidence.value(QStringLiteral("configurationBaselineIdentity")).type() != QVariant::Map
        || evidence.value(QStringLiteral("configurationBaselineIdentity")).isNull()) return false;
    for (const QString& key : {QStringLiteral("performanceCertified"), QStringLiteral("hardwareIdentityAuthenticated"),
            QStringLiteral("executionPermitted"), QStringLiteral("flightCommandReleased")}) if (!flag(evidence, key, false)) return false;
    const QVariantMap review = evidence.value(QStringLiteral("capabilityReviewIdentity")).toMap();
    if (!exact(review, {QStringLiteral("capabilityReviewContext"), QStringLiteral("capabilityApproval")})
        || review.value(QStringLiteral("capabilityReviewContext")).type() != QVariant::Map
        || review.value(QStringLiteral("capabilityApproval")).type() != QVariant::Map) return false;
    const QVariantMap context = review.value(QStringLiteral("capabilityReviewContext")).toMap();
    const QVariantMap approval = review.value(QStringLiteral("capabilityApproval")).toMap();
    if (!FleetCapabilityEvidence::contextValid(context)
        || !integer(context.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !exact(approval, {QStringLiteral("schemaVersion"), QStringLiteral("approvalId"), QStringLiteral("registryEpoch"),
            QStringLiteral("vehicleRecordEpoch"), QStringLiteral("capabilityRevision"), QStringLiteral("identityFingerprint"),
            QStringLiteral("configurationFingerprint"), QStringLiteral("capabilities"), QStringLiteral("source"),
            QStringLiteral("state"), QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("flightCommandReleased")})
        || !integer(approval.value(QStringLiteral("schemaVersion")), 1, 1) || !uuid(approval.value(QStringLiteral("approvalId")))
        || approval.value(QStringLiteral("source")).type() != QVariant::String || approval.value(QStringLiteral("source")).isNull()
        || approval.value(QStringLiteral("source")).toString() != QStringLiteral("local-engineering-attestation")
        || approval.value(QStringLiteral("state")).type() != QVariant::String || approval.value(QStringLiteral("state")).isNull()
        || approval.value(QStringLiteral("state")).toString() != QStringLiteral("current")
        || !flag(approval, QStringLiteral("hardwareIdentityAuthenticated"), false)
        || !flag(approval, QStringLiteral("flightCommandReleased"), false)) return false;
    for (const QString& key : {QStringLiteral("registryEpoch"), QStringLiteral("vehicleRecordEpoch"),
            QStringLiteral("capabilityRevision"), QStringLiteral("identityFingerprint"), QStringLiteral("configurationFingerprint")})
        if (!sameValue(approval.value(key), context.value(key))) return false;
    QStringList capabilities;
    if (!FleetCapabilityEvidence::canonicalCapabilities(approval.value(QStringLiteral("capabilities")), &capabilities)
        || capabilities.isEmpty() || !same(approval.value(QStringLiteral("capabilities")), capabilities)) return false;
    const QVariantMap baseline = evidence.value(QStringLiteral("configurationBaselineIdentity")).toMap();
    if (!exact(baseline, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("registryContext"),
            QStringLiteral("observationEpoch"), QStringLiteral("observationRevision"), QStringLiteral("currentFullKnown"),
            QStringLiteral("baselineFullKnown"), QStringLiteral("baselineMatches"), QStringLiteral("currentTypedFingerprint"),
            QStringLiteral("baselineTypedFingerprint"), QStringLiteral("baselineApprovalHash"), QStringLiteral("hardwareIdentityAuthenticated"),
            QStringLiteral("performanceCertified"), QStringLiteral("flightCommandReleased")})
        || !integer(baseline.value(QStringLiteral("schemaVersion")), 1, 1)
        || baseline.value(QStringLiteral("source")).type() != QVariant::String || baseline.value(QStringLiteral("source")).isNull()
        || baseline.value(QStringLiteral("source")).toString() != QStringLiteral("native-local-configuration-baseline")
        || baseline.value(QStringLiteral("registryContext")).type() != QVariant::Map
        || !same(baseline.value(QStringLiteral("registryContext")), context)
        || !uuid(baseline.value(QStringLiteral("observationEpoch")))
        || !decimalCounter(baseline.value(QStringLiteral("observationRevision")))
        || !digest(baseline.value(QStringLiteral("currentTypedFingerprint")))
        || !digest(baseline.value(QStringLiteral("baselineTypedFingerprint")))
        || !sameValue(baseline.value(QStringLiteral("currentTypedFingerprint")), baseline.value(QStringLiteral("baselineTypedFingerprint")))
        || !digest(baseline.value(QStringLiteral("baselineApprovalHash")))) return false;
    for (const QString& key : {QStringLiteral("currentFullKnown"), QStringLiteral("baselineFullKnown"), QStringLiteral("baselineMatches")})
        if (!flag(baseline, key, true)) return false;
    for (const QString& key : {QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("performanceCertified"), QStringLiteral("flightCommandReleased")})
        if (!flag(baseline, key, false)) return false;
    // This is only the shape/consistency of retained value evidence. The
    // actual current capability/Registry/Vehicle fences are not recreated.
    return true;
}

bool compiledReservePlansMatch(const QVariantMap& provenance, const QVariantMap& plan)
{
    const QVariantList descriptors = provenance.value(QStringLiteral("reservePlans")).toList();
    const QVariant rawRows = plan.value(QStringLiteral("reserveMembers"));
    if (rawRows.type() != QVariant::List || rawRows.isNull() || rawRows.toList().size() != descriptors.size()) return false;
    const QVariantList rows = rawRows.toList(), members = plan.value(QStringLiteral("members")).toList();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    const QVariant performancePolicy = plan.value(QStringLiteral("requirePerformanceEvidence"), false);
    if (performancePolicy.type() != QVariant::Bool || performancePolicy.isNull()) return false;
    const bool requireDeclaration = performancePolicy.toBool();
    int generatedPoints = 0;
    for (int index = 0; index < rows.size(); ++index) {
        if (rows.at(index).type() != QVariant::Map || rows.at(index).isNull()) return false;
        const QVariantMap row = rows.at(index).toMap(), descriptor = descriptors.at(index).toMap();
        const int id = descriptor.value(QStringLiteral("vehicleId")).toInt();
        if (!exact(row, {QStringLiteral("vehicleId"), QStringLiteral("role"), QStringLiteral("manualConfirmationRequired"), QStringLiteral("energyPlan")})
            || !integer(row.value(QStringLiteral("vehicleId")), id, id)
            || row.value(QStringLiteral("role")).type() != QVariant::String || row.value(QStringLiteral("role")).isNull()
            || row.value(QStringLiteral("role")).toString() != QStringLiteral("reserve")
            || !flag(row, QStringLiteral("manualConfirmationRequired"), true)
            || row.value(QStringLiteral("energyPlan")).type() != QVariant::Map || row.value(QStringLiteral("energyPlan")).isNull()) return false;
        // Rebuild with the SAME generator from the actual compiled member
        // routes. Empty fleet evidence is deliberate: it cannot authenticate
        // current performance, but can reproduce all geometry/cost fields.
        const QVariantMap expected = FleetReserveEnergyPlan::build(descriptor, members, constraints, QVariantMap(), QVariant(false));
        if (!flag(expected, QStringLiteral("complete"), true)) return false;
        const int outbound = expected.value(QStringLiteral("outboundPath")).toList().size();
        const int returning = expected.value(QStringLiteral("returnPath")).toList().size();
        if (outbound > 4096 || returning > 4096 || generatedPoints > 32768 - outbound - returning) return false;
        generatedPoints += outbound + returning;
        const QVariantMap actual = row.value(QStringLiteral("energyPlan")).toMap();
        QSet<QString> actualFields;
        for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) actualFields.insert(it.key());
        const bool hasDeclaration = actual.contains(QStringLiteral("requirePerformanceEvidence"));
        if (hasDeclaration != requireDeclaration) return false;
        if (hasDeclaration) actualFields.unite({QStringLiteral("requirePerformanceEvidence"),
            QStringLiteral("declaredPerformanceEvidence"), QStringLiteral("declaredPerformanceEvidenceUsable")});
        if (!exact(actual, actualFields)) return false;
        for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) {
            if (it.key() == QStringLiteral("speedCapabilityKnown") || it.key() == QStringLiteral("declaredMaxSpeedMetersPerSecond")) continue;
            if (!sameValue(actual.value(it.key()), it.value())) return false;
            if (it.value().type() == QVariant::Bool && !flag(actual, it.key(), it.value().toBool())) return false;
            if (it.value().type() == QVariant::String && (actual.value(it.key()).type() != QVariant::String
                || actual.value(it.key()).isNull())) return false;
        }
        const QVariant known = actual.value(QStringLiteral("speedCapabilityKnown"));
        double maximumSpeed = 0.0;
        if (known.type() != QVariant::Bool || known.isNull()
            || !number(actual.value(QStringLiteral("declaredMaxSpeedMetersPerSecond")), -1.0,
                hasDeclaration ? MaximumNumber : 1000.0, &maximumSpeed)
            || (known.toBool() ? maximumSpeed < 0.0 : maximumSpeed != -1.0)
            || (known.toBool() && descriptor.value(QStringLiteral("reservePlan")).toMap()
                .value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble() > maximumSpeed)) return false;
        if (hasDeclaration && (!flag(actual, QStringLiteral("requirePerformanceEvidence"), true)
            || !flag(actual, QStringLiteral("declaredPerformanceEvidenceUsable"), true) || !known.toBool()
            || actual.value(QStringLiteral("declaredPerformanceEvidence")).type() != QVariant::Map
            || !declaredPerformanceShape(actual.value(QStringLiteral("declaredPerformanceEvidence")).toMap(), id, maximumSpeed))) return false;
    }
    return true;
}

bool geometryMatches(const QVariantMap& provenance, const QVariantMap& plan)
{
    if (plan.contains(QStringLiteral("planningSource")) || plan.contains(QStringLiteral("remainderPlanningSource"))
        || plan.contains(QStringLiteral("reserveDispatchPlanningSource"))
        || (plan.contains(QStringLiteral("groupRemainderPlanningSource")) && !same(plan.value(QStringLiteral("groupRemainderPlanningSource")), provenance))
        || plan.value(QStringLiteral("members")).type() != QVariant::List || plan.value(QStringLiteral("constraints")).type() != QVariant::Map
        || plan.value(QStringLiteral("assignments")).type() != QVariant::List
        || !same(plan.value(QStringLiteral("assignments")), plan.value(QStringLiteral("members")))
        || plan.contains(QStringLiteral("routeBoundaryPolicy")) != provenance.contains(QStringLiteral("routeBoundaryPolicy"))
        || (provenance.contains(QStringLiteral("routeBoundaryPolicy"))
            && !same(plan.value(QStringLiteral("routeBoundaryPolicy")), provenance.value(QStringLiteral("routeBoundaryPolicy"))))
        || plan.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List
        || !same(plan.value(QStringLiteral("reserveVehicleIds")), provenance.value(QStringLiteral("reserveVehicleIds")))
        || (plan.contains(QStringLiteral("coveragePlan")) && (plan.value(QStringLiteral("coveragePlan")).type() != QVariant::Map
            || !plan.value(QStringLiteral("coveragePlan")).toMap().isEmpty()))) return false;
    const QVariantList members = plan.value(QStringLiteral("members")).toList(), rows = provenance.value(QStringLiteral("members")).toList();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    if (members.size() != rows.size()) return false;
    for (int index = 0; index < rows.size(); ++index) {
        if (members.at(index).type() != QVariant::Map) return false;
        const QVariantMap member = members.at(index).toMap(), row = rows.at(index).toMap();
        if (!integer(member.value(QStringLiteral("vehicleId")), row.value(QStringLiteral("targetVehicleId")).toInt(), row.value(QStringLiteral("targetVehicleId")).toInt())
            || member.value(QStringLiteral("role")) != row.value(QStringLiteral("targetRole"))
            || member.value(QStringLiteral("waypoints")).type() != QVariant::List
            || member.value(QStringLiteral("waypoints")).toList().size() != row.value(QStringLiteral("outboundPointCount")).toInt()
            || member.value(QStringLiteral("returnPath")).type() != QVariant::List || member.value(QStringLiteral("returnPath")).toList().size() != 2
            || !number(member.value(QStringLiteral("minimumSpeedMps")), 0.0, MaximumNumber)
            || member.value(QStringLiteral("minimumSpeedMps")).toDouble() != row.value(QStringLiteral("minimumSpeedMps")).toDouble()
            || !number(member.value(QStringLiteral("returnReserveSeconds")), 0.0, MaximumNumber)
            || member.value(QStringLiteral("returnReserveSeconds")).toDouble() != row.value(QStringLiteral("returnReserveFloorSeconds")).toDouble()
            || (row.value(QStringLiteral("missionDurationFloorSeconds")).toDouble() > 0.0
                ? (!number(member.value(QStringLiteral("durationSeconds")), 0.0, MaximumNumber)
                    || member.value(QStringLiteral("durationSeconds")).toDouble() != row.value(QStringLiteral("missionDurationFloorSeconds")).toDouble())
                : member.contains(QStringLiteral("durationSeconds")))
            || hash(routeProjection(member, constraints)) != row.value(QStringLiteral("newRouteHash")).toString()) return false;
    }
    const QVariantList reservePlans = provenance.value(QStringLiteral("reservePlans")).toList();
    const bool explicitReserves = provenance.value(QStringLiteral("schemaVersion")).toInt() == 3;
    const QVariant performancePolicy = plan.value(QStringLiteral("requirePerformanceEvidence"), false);
    if (explicitReserves && (performancePolicy.type() != QVariant::Bool || performancePolicy.isNull())) return false;
    return hash(routesProjection(members, constraints, plan.value(QStringLiteral("reserveVehicleIds")).toList(),
        explicitReserves ? &reservePlans : nullptr, explicitReserves && performancePolicy.toBool()))
        == provenance.value(QStringLiteral("newRoutesHash")).toString();
}
} // namespace

QVariantMap FleetMissionGroupRemainderPlanner::validateProvenance(const QVariantMap& value)
{
    const auto reject = [] { return provenanceResult(false, {}, QStringLiteral("group-remainder-provenance-invalid")); };
    QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("sourceKind"), QStringLiteral("sourceArchiveId"),
        QStringLiteral("sourceArchiveContentHash"), QStringLiteral("runId"), QStringLiteral("startupId"),
        QStringLiteral("transactionEpoch"), QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"),
        QStringLiteral("binding"), QStringLiteral("sourceVehicleIds"), QStringLiteral("targetVehicleIds"), QStringLiteral("reserveVehicleIds"),
        QStringLiteral("memberCount"), QStringLiteral("members"), QStringLiteral("sourceTaskPolicyHash"),
        QStringLiteral("derivedTaskHash"), QStringLiteral("newRoutesHash"), QStringLiteral("selectionPolicy"), QStringLiteral("timingBasis"),
        QStringLiteral("historicalOnly"), QStringLiteral("progressUsedToSkip"), QStringLiteral("completionProven"),
        QStringLiteral("authorityRestored"), QStringLiteral("flightCommandReleased"), QStringLiteral("executable")};
    int schema = 0;
    if (!integer(value.value(QStringLiteral("schemaVersion")), 1, 3, &schema)) return reject();
    if (schema >= 2) fields.insert(QStringLiteral("routeBoundaryPolicy"));
    if (schema == 3) fields.insert(QStringLiteral("reservePlans"));
    if (!exact(value, fields) || !bounded(value, 64 * 1024)
        || value.value(QStringLiteral("sourceKind")) != QVariant(Kind)
        || value.value(QStringLiteral("selectionPolicy")) != QVariant(Selection) || value.value(QStringLiteral("timingBasis")) != QVariant(Timing)
        || !flag(value, QStringLiteral("historicalOnly"), true)) return reject();
    if (schema >= 2) {
        const QVariant policy = value.value(QStringLiteral("routeBoundaryPolicy"));
        if (policy.type() != QVariant::Map || policy.isNull()
            || !flag(FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(policy.toMap()), QStringLiteral("ok"), true)) return reject();
    }
    for (const QString& key : {QStringLiteral("sourceArchiveId"), QStringLiteral("runId"), QStringLiteral("startupId"),
            QStringLiteral("transactionEpoch"), QStringLiteral("deploymentId")}) if (!uuid(value.value(key))) return reject();
    for (const QString& key : {QStringLiteral("sourceArchiveContentHash"), QStringLiteral("sourceTaskPolicyHash"),
            QStringLiteral("derivedTaskHash"), QStringLiteral("newRoutesHash")}) if (!digest(value.value(key))) return reject();
    for (const QString& key : {QStringLiteral("progressUsedToSkip"), QStringLiteral("completionProven"),
            QStringLiteral("authorityRestored"), QStringLiteral("flightCommandReleased"), QStringLiteral("executable")})
        if (!flag(value, key, false)) return reject();
    for (const QString& key : {QStringLiteral("sourceVehicleIds"), QStringLiteral("targetVehicleIds"),
            QStringLiteral("reserveVehicleIds"), QStringLiteral("members")})
        if (value.value(key).type() != QVariant::List || (schema == 3 && value.value(key).isNull())) return reject();
    int count = 0;
    const QVariantList sources = value.value(QStringLiteral("sourceVehicleIds")).toList(), targets = value.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList members = value.value(QStringLiteral("members")).toList(), reserves = value.value(QStringLiteral("reserveVehicleIds")).toList();
    if (!integer(value.value(QStringLiteral("memberCount")), 1, 32, &count) || sources.size() != count || targets.size() != count
        || members.size() != count || reserves.size() > 32 || value.value(QStringLiteral("binding")).type() != QVariant::Map) return reject();
    // Reuse the existing complete binding/UUID/generation rules without a
    // fabricated planning request or source: provenance schema1 is pure value.
    const QString generation = value.value(QStringLiteral("transactionGeneration")).toString();
    bool generationOk = false;
    const quint64 parsedGeneration = generation.toULongLong(&generationOk);
    if (value.value(QStringLiteral("transactionGeneration")).type() != QVariant::String || !generationOk || parsedGeneration == 0
        || QString::number(parsedGeneration) != generation) return reject();
    const QVariantMap binding = value.value(QStringLiteral("binding")).toMap();
    QSet<QString> bindingFields{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (binding.size() == 9) bindingFields.unite({QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")});
    if (!exact(binding, bindingFields) || !uuid(binding.value(QStringLiteral("storeEpoch")))) return reject();
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId")}) {
        const QVariant field = binding.value(key);
        if (field.type() != QVariant::String || field.toString().isEmpty() || field.toString().size() > 128
            || field.toString() != field.toString().trimmed()) return reject();
        for (const QChar ch : field.toString()) if (ch.isNull() || ch.category() == QChar::Other_Control) return reject();
    }
    for (const QString& key : {QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (!digest(binding.value(key))) return reject();
    for (const QString& key : {QStringLiteral("recordRevision"), QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")}) {
        if (!binding.contains(key)) continue;
        double parsed = 0.0;
        if (!number(binding.value(key), key == QStringLiteral("recordRevision") ? 1.0 : 0.0, MaximumNumber, &parsed)
            || std::floor(parsed) != parsed) return reject();
    }
    const QSet<QString> rowFields{QStringLiteral("sourceVehicleId"), QStringLiteral("targetVehicleId"), QStringLiteral("targetRole"),
        QStringLiteral("sourcePayloadHash"), QStringLiteral("originalRouteHash"), QStringLiteral("sourceMemberHash"),
        QStringLiteral("sourceWaypointIndex"), QStringLiteral("sourceWaypointCount"), QStringLiteral("outboundPointCount"),
        QStringLiteral("returnPointCount"), QStringLiteral("newRouteHash"), QStringLiteral("missionDurationFloorSeconds"),
        QStringLiteral("returnReserveFloorSeconds"), QStringLiteral("minimumSpeedMps")};
    QSet<int> sourceSet, targetSet, reserveSet;
    int leaders = 0, totalPoints = 0;
    for (int index = 0; index < count; ++index) {
        int sourceId = 0, targetId = 0, first = 0, originalCount = 0, outboundCount = 0;
        if (!integer(sources.at(index), 1, 255, &sourceId) || !integer(targets.at(index), 1, 255, &targetId)
            || sourceSet.contains(sourceId) || targetSet.contains(targetId) || members.at(index).type() != QVariant::Map) return reject();
        sourceSet.insert(sourceId); targetSet.insert(targetId);
        const QVariantMap row = members.at(index).toMap();
        if (!exact(row, rowFields) || !integer(row.value(QStringLiteral("sourceVehicleId")), sourceId, sourceId)
            || !integer(row.value(QStringLiteral("targetVehicleId")), targetId, targetId) || !role(row.value(QStringLiteral("targetRole")))
            || !integer(row.value(QStringLiteral("sourceWaypointCount")), 1, 2048, &originalCount)
            || !integer(row.value(QStringLiteral("sourceWaypointIndex")), 0, originalCount - 1, &first)
            || !integer(row.value(QStringLiteral("outboundPointCount")), 2, 2046, &outboundCount)
            || outboundCount != originalCount - first + 1 || !integer(row.value(QStringLiteral("returnPointCount")), 2, 2)
            || !number(row.value(QStringLiteral("missionDurationFloorSeconds")), 0.0, MaximumNumber)
            || !number(row.value(QStringLiteral("returnReserveFloorSeconds")), 0.0, MaximumNumber)
            || !number(row.value(QStringLiteral("minimumSpeedMps")), 0.0, MaximumNumber)
            || row.value(QStringLiteral("minimumSpeedMps")).toDouble() <= 0.0) return reject();
        for (const QString& key : {QStringLiteral("sourcePayloadHash"), QStringLiteral("originalRouteHash"),
                QStringLiteral("sourceMemberHash"), QStringLiteral("newRouteHash")}) if (!digest(row.value(key))) return reject();
        leaders += row.value(QStringLiteral("targetRole")).toString() == QStringLiteral("leader") ? 1 : 0;
        totalPoints += outboundCount + 2;
    }
    for (const QVariant& reserve : reserves) {
        int id = 0;
        if (!integer(reserve, 1, 255, &id) || targetSet.contains(id) || reserveSet.contains(id)) return reject();
        reserveSet.insert(id);
    }
    if (schema == 3) {
        QVariantList canonical;
        if (!normalizeReservePlans(value.value(QStringLiteral("reservePlans")), reserves, targets, &canonical)
            || !same(value.value(QStringLiteral("reservePlans")), canonical)) return reject();
    }
    if (leaders > 1 || totalPoints > 8192) return reject();
    return provenanceResult(true, value, QStringLiteral(""));
}

bool FleetMissionGroupRemainderPlanner::matchesTask(const QVariantMap& provenance, const QVariantMap& task)
{
    if (!flag(validateProvenance(provenance), QStringLiteral("ok"), true) || !bounded(task, MaximumTaskBytes)
        || task.contains(QStringLiteral("planningSource")) || task.contains(QStringLiteral("remainderPlanningSource"))
        || task.contains(QStringLiteral("reserveDispatchPlanningSource"))
        || (task.contains(QStringLiteral("groupRemainderPlanningSource")) && !same(task.value(QStringLiteral("groupRemainderPlanningSource")), provenance))
        || task.value(QStringLiteral("assignments")).type() != QVariant::List
        || hash(taskProjection(task)) != provenance.value(QStringLiteral("derivedTaskHash")).toString()) return false;
    const bool explicitReserves = provenance.value(QStringLiteral("schemaVersion")).toInt() == 3;
    if (explicitReserves && (task.value(QStringLiteral("reservePlans")).type() != QVariant::List
        || task.value(QStringLiteral("reservePlans")).isNull()
        || !same(task.value(QStringLiteral("reservePlans")), provenance.value(QStringLiteral("reservePlans"))))) return false;
    // This is explicitly the task path. Do not route it through the compiled
    // reserveMembers predicate or let a plan's optional reservePlans bypass it.
    QVariantMap projection = task;
    projection.insert(QStringLiteral("members"), task.value(QStringLiteral("assignments")));
    if (provenance.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
            task.value(QStringLiteral("area")), task.value(QStringLiteral("assignments")).toList());
        if (!flag(boundary, QStringLiteral("ok"), true)
            || !same(boundary.value(QStringLiteral("policy")), provenance.value(QStringLiteral("routeBoundaryPolicy")))) return false;
        projection.insert(QStringLiteral("routeBoundaryPolicy"), boundary.value(QStringLiteral("policy")));
    }
    return geometryMatches(provenance, projection);
}

bool FleetMissionGroupRemainderPlanner::matchesPlan(const QVariantMap& provenance, const QVariantMap& plan)
{
    if (!flag(validateProvenance(provenance), QStringLiteral("ok"), true) || !bounded(plan, MaximumTaskBytes)
        || !geometryMatches(provenance, plan)) return false;
    return provenance.value(QStringLiteral("schemaVersion")).toInt() != 3 || compiledReservePlansMatch(provenance, plan);
}

QVariantMap FleetMissionGroupRemainderPlanner::build(const QVariantMap& source, const QVariantMap& request)
{
    if (!bounded(source, MaximumSourceBytes) || !integer(source.value(QStringLiteral("schemaVersion")), 2, 2)
        || !FleetMissionRemainderArchive::archiveValid(source)) return rejected(QStringLiteral("group-remainder-complete-archive-v2-required"));
    const QVariantMap checked = FleetMissionStartupTransaction::validateCollection(source.value(QStringLiteral("nativeCollection")).toMap());
    if (!flag(checked, QStringLiteral("ok"), true)) return rejected(checked.value(QStringLiteral("reason")).toString());
    const QVariantMap collection = checked.value(QStringLiteral("collection")).toMap();
    const QVariantList sourceIds = collection.value(QStringLiteral("vehicleIds")).toList(), originalRows = collection.value(QStringLiteral("targets")).toList();
    const bool explicitReserves = request.contains(QStringLiteral("reservePlans"));
    QSet<QString> requestKeys{QStringLiteral("members"), QStringLiteral("reserveVehicleIds"), QStringLiteral("operatorConfirmed")};
    if (explicitReserves) requestKeys.insert(QStringLiteral("reservePlans"));
    if (!exact(request, requestKeys)
        || !bounded(request, 4LL * 1024 * 1024) || !flag(request, QStringLiteral("operatorConfirmed"), true)
        || request.value(QStringLiteral("members")).type() != QVariant::List
        || request.value(QStringLiteral("members")).toList().size() != sourceIds.size()
        || request.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List
        || (explicitReserves && (request.value(QStringLiteral("members")).isNull()
            || request.value(QStringLiteral("reserveVehicleIds")).isNull()))
        || request.value(QStringLiteral("reserveVehicleIds")).toList().size() > 32)
        return rejected(QStringLiteral("group-remainder-complete-request-required"));
    const QVariantMap originalTask = collection.value(QStringLiteral("sourceTask")).toMap();
    const QVariantMap originalPlan = collection.value(QStringLiteral("sourcePlan")).toMap();
    if (originalPlan.value(QStringLiteral("constraints")).type() != QVariant::Map)
        return rejected(QStringLiteral("group-remainder-constraints-required"));
    const QVariantMap constraints = originalPlan.value(QStringLiteral("constraints")).toMap();
    QMap<int, QVariantMap> rawMembers;
    if (!originalRawMembers(originalTask, sourceIds, &rawMembers))
        return rejected(QStringLiteral("group-remainder-member-policy-scope-unsupported"));
    QVariantMap task = commonPolicies(originalTask);
    const QString policyHash = hash(task);
    const QVariant performancePolicy = task.value(QStringLiteral("requirePerformanceEvidence"), false);
    if (explicitReserves && (performancePolicy.type() != QVariant::Bool || performancePolicy.isNull()))
        return rejected(QStringLiteral("group-remainder-performance-policy-invalid"));
    const QVariantList requests = request.value(QStringLiteral("members")).toList(), reserves = request.value(QStringLiteral("reserveVehicleIds")).toList();
    QSet<int> targetSet, reserveSet;
    QVariantList assignments, rows, summaries, targetIds;
    QMap<QString, int> roleCounts;
    double sharedReturn = 0.0;
    bool suppliedReturn = false;
    for (const QVariantMap& scope : {originalTask, constraints})
        if (!includeFloor(scope, {QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds")},
                0.0, &sharedReturn, &suppliedReturn)) return rejected(QStringLiteral("group-remainder-return-policy-invalid"));
    if (!suppliedReturn) sharedReturn = 120.0;
    double maximumOutbound = 0.0, maximumReturn = 0.0, totalHolds = 0.0;
    int totalPoints = 0, totalItems = 0;
    const QSet<QString> requestFields{QStringLiteral("sourceVehicleId"), QStringLiteral("sourceWaypointIndex"),
        QStringLiteral("targetVehicleId"), QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("cruiseSpeedMetersPerSecond"),
        QStringLiteral("climbSpeedMetersPerSecond"), QStringLiteral("descentSpeedMetersPerSecond"), QStringLiteral("executionProfile"),
        QStringLiteral("operatorConfirmed")};
    for (int index = 0; index < sourceIds.size(); ++index) {
        const int sourceId = sourceIds.at(index).toInt();
        const QVariantMap originalRow = originalRows.at(index).toMap(), originalMember = originalRow.value(QStringLiteral("sourceMember")).toMap();
        if (!flag(originalRow, QStringLiteral("available"), true)) return rejected(QStringLiteral("group-remainder-all-members-must-be-committed"));
        if (!role(originalMember.value(QStringLiteral("role")))) return rejected(QStringLiteral("group-remainder-member-role-unsupported"));
        if (requests.at(index).type() != QVariant::Map) return rejected(QStringLiteral("group-remainder-complete-request-required"));
        QVariantMap memberRequest = requests.at(index).toMap();
        int targetId = 0;
        if (!exact(memberRequest, requestFields) || !integer(memberRequest.value(QStringLiteral("sourceVehicleId")), sourceId, sourceId)
            || !integer(memberRequest.value(QStringLiteral("targetVehicleId")), 1, 255, &targetId) || targetSet.contains(targetId))
            return rejected(QStringLiteral("group-remainder-source-target-mapping-invalid"));
        targetSet.insert(targetId); targetIds.append(targetId); memberRequest.remove(QStringLiteral("sourceVehicleId"));
        // Reuse the complete single-member geometry/timing checks with an
        // explicitly HISTORICAL source. No native label/owner is fabricated.
        QVariantMap historical{{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("ok"), true},
            {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("sourceKind"), QStringLiteral("historical-run-planning-snapshot")},
            {QStringLiteral("sourceArchiveId"), source.value(QStringLiteral("archiveId"))},
            {QStringLiteral("sourceArchiveContentHash"), source.value(QStringLiteral("contentHash"))},
            {QStringLiteral("sourceVehicleId"), sourceId}, {QStringLiteral("sourceMember"), originalMember},
            {QStringLiteral("sourcePayload"), originalRow.value(QStringLiteral("sourcePayload"))}, {QStringLiteral("progress"), originalRow.value(QStringLiteral("progress"))},
            {QStringLiteral("readonly"), true}, {QStringLiteral("flightCommandReleased"), false}};
        for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
                QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding"),
                QStringLiteral("sourceTask"), QStringLiteral("sourcePlan")}) historical.insert(key, collection.value(key));
        const QVariantMap child = FleetMissionRemainderPlanner::build(historical, memberRequest);
        if (!flag(child, QStringLiteral("ok"), true)) return rejected(child.value(QStringLiteral("reason")).toString());
        const QVariantMap childTask = child.value(QStringLiteral("task")).toMap(), childSummary = child.value(QStringLiteral("summary")).toMap();
        const QVariantMap childProvenance = child.value(QStringLiteral("provenance")).toMap();
        const QVariantMap geometry = childTask.value(QStringLiteral("assignments")).toList().first().toMap();
        const QVariantMap raw = rawMembers.value(sourceId);
        for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("id")})
            if (raw.contains(key) && !integer(raw.value(key), sourceId, sourceId))
                return rejected(QStringLiteral("group-remainder-member-policy-scope-unsupported"));
        for (const QString& key : {QStringLiteral("role"), QStringLiteral("assignmentRole")})
            if (raw.contains(key) && (raw.value(key).type() != QVariant::String || raw.value(key).isNull()
                || raw.value(key).toString().trimmed().toLower() != originalMember.value(QStringLiteral("role")).toString()))
                return rejected(QStringLiteral("group-remainder-original-role-policy-not-preservable"));
        if (!estimatedTimingOnly(raw)) return rejected(QStringLiteral("group-remainder-explicit-timing-replan-required"));
        QVariantMap retained = raw;
        for (auto it = originalMember.constBegin(); it != originalMember.constEnd(); ++it)
            if (!retained.contains(it.key())) retained.insert(it.key(), it.value());
        QVariantMap member = memberPolicies(retained);
        for (auto it = geometry.constBegin(); it != geometry.constEnd(); ++it) member.insert(it.key(), it.value());
        double missionFloor = 0.0, returnFloor = sharedReturn, minimumSpeed = 0.0;
        for (const QVariantMap& scope : {originalMember, raw}) {
            if (!includeFloor(scope, {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                    QStringLiteral("taskDurationSeconds"), QStringLiteral("timeLimitSeconds")}, 0.0, &missionFloor, nullptr, true)
                || !includeFloor(scope, {QStringLiteral("returnDurationSeconds")}, 0.0, &returnFloor, nullptr, true)
                || !includeFloor(scope, {QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds")}, 0.0, &returnFloor)
                || !includeFloor(scope, {QStringLiteral("minimumSpeedMps")}, 0.0, &minimumSpeed))
                return rejected(QStringLiteral("group-remainder-member-budget-policy-invalid"));
        }
        if (!includeFloor(originalTask, {QStringLiteral("minimumSpeedMps")}, 0.0, &minimumSpeed))
            return rejected(QStringLiteral("group-remainder-speed-policy-invalid"));
        minimumSpeed = qMax(minimumSpeed, childTask.value(QStringLiteral("minimumSpeedMps")).toDouble());
        if (missionFloor > 0.0) member.insert(QStringLiteral("durationSeconds"), missionFloor);
        member.insert(QStringLiteral("returnReserveSeconds"), returnFloor);
        member.insert(QStringLiteral("minimumSpeedMps"), minimumSpeed);
        const int points = member.value(QStringLiteral("waypoints")).toList().size() + member.value(QStringLiteral("returnPath")).toList().size();
        const int items = points + actionCount(member.value(QStringLiteral("executionProfile")).toMap());
        if (items > 2048 || totalItems > 8192 - items) return rejected(QStringLiteral("group-remainder-item-budget-exceeded"));
        totalPoints += points; totalItems += items;
        const QString memberRole = member.value(QStringLiteral("role")).toString();
        roleCounts[memberRole] += 1;
        assignments.append(member);
        QVariantMap row{{QStringLiteral("sourceVehicleId"), sourceId}, {QStringLiteral("targetVehicleId"), targetId},
            {QStringLiteral("targetRole"), memberRole}, {QStringLiteral("sourcePayloadHash"), childProvenance.value(QStringLiteral("sourcePayloadHash"))},
            {QStringLiteral("originalRouteHash"), childProvenance.value(QStringLiteral("originalRouteHash"))}, {QStringLiteral("sourceMemberHash"), hash(originalMember)},
            {QStringLiteral("sourceWaypointIndex"), childProvenance.value(QStringLiteral("sourceWaypointIndex"))},
            {QStringLiteral("sourceWaypointCount"), childProvenance.value(QStringLiteral("sourceWaypointCount"))},
            {QStringLiteral("outboundPointCount"), childProvenance.value(QStringLiteral("outboundPointCount"))}, {QStringLiteral("returnPointCount"), 2},
            {QStringLiteral("newRouteHash"), hash(routeProjection(member, constraints))}, {QStringLiteral("missionDurationFloorSeconds"), missionFloor},
            {QStringLiteral("returnReserveFloorSeconds"), returnFloor}, {QStringLiteral("minimumSpeedMps"), minimumSpeed}};
        rows.append(row);
        QVariantMap summary;
        for (const QString& key : {QStringLiteral("sourceVehicleId"), QStringLiteral("targetVehicleId"), QStringLiteral("sourceWaypointIndex"),
                QStringLiteral("sourceWaypointCount"), QStringLiteral("retainedWaypointCount"), QStringLiteral("outboundPointCount"),
                QStringLiteral("returnPointCount"), QStringLiteral("estimatedIngressSeconds"), QStringLiteral("estimatedTaskSeconds"),
                QStringLiteral("retainedHoldSeconds"), QStringLiteral("estimatedOutboundSeconds"), QStringLiteral("estimatedReturnSeconds"),
                QStringLiteral("executionProfileProvided")}) summary.insert(key, childSummary.value(key));
        summary.insert(QStringLiteral("targetRole"), memberRole); summary.insert(QStringLiteral("missionDurationFloorSeconds"), missionFloor);
        summary.insert(QStringLiteral("returnReserveFloorSeconds"), returnFloor); summary.insert(QStringLiteral("minimumSpeedMps"), minimumSpeed);
        summaries.append(summary);
        maximumOutbound = qMax(maximumOutbound, childSummary.value(QStringLiteral("estimatedOutboundSeconds")).toDouble());
        maximumReturn = qMax(maximumReturn, childSummary.value(QStringLiteral("estimatedReturnSeconds")).toDouble());
        totalHolds += childSummary.value(QStringLiteral("retainedHoldSeconds")).toDouble();
    }
    if (roleCounts.value(QStringLiteral("leader")) > 1 || !std::isfinite(totalHolds)) return rejected(QStringLiteral("group-remainder-role-or-estimate-invalid"));
    for (const QVariant& reserve : reserves) {
        int id = 0;
        if (!integer(reserve, 1, 255, &id) || reserveSet.contains(id) || targetSet.contains(id))
            return rejected(QStringLiteral("group-remainder-reserve-ids-invalid"));
        reserveSet.insert(id);
    }
    int requiredReserves = originalPlan.value(QStringLiteral("reserveVehicleIds")).toList().size();
    int declaredReserves = 0;
    if (originalTask.contains(QStringLiteral("reserveCount"))) {
        if (!integer(originalTask.value(QStringLiteral("reserveCount")), 0, 255, &declaredReserves))
            return rejected(QStringLiteral("group-remainder-reserve-policy-invalid"));
        requiredReserves = qMax(requiredReserves, declaredReserves);
    }
    if (reserves.size() < requiredReserves) return rejected(QStringLiteral("group-remainder-original-reserve-policy-required"));
    QVariantList reservePlans;
    if (explicitReserves) {
        if (!normalizeReservePlans(request.value(QStringLiteral("reservePlans")), reserves, targetIds, &reservePlans))
            return rejected(QStringLiteral("group-remainder-explicit-reserve-plans-invalid"));
        // Only this explicit new configuration replaces the retained old
        // descriptors. policyHash above still binds the COMPLETE old policies.
        task.insert(QStringLiteral("reservePlans"), reservePlans);
    }
    for (const QString& memberRole : {QStringLiteral("leader"), QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")}) {
        const QString key = memberRole + QStringLiteral("Count");
        int declared = 0;
        if (originalTask.contains(key) && (!integer(originalTask.value(key), 0, 32, &declared) || declared != roleCounts.value(memberRole)))
            return rejected(QStringLiteral("group-remainder-original-role-policy-not-preservable"));
        task.insert(key, roleCounts.value(memberRole));
    }
    if (originalTask.contains(QStringLiteral("leaderRequired"))) {
        const QVariant required = originalTask.value(QStringLiteral("leaderRequired"));
        if (required.type() != QVariant::Bool || required.isNull()
            || (required.toBool() && roleCounts.value(QStringLiteral("leader")) == 0))
            return rejected(QStringLiteral("group-remainder-original-role-policy-not-preservable"));
        // Not requiring a leader is not a prohibition on an existing one.
        // commonPolicies already retained the original explicit boolean.
    } else task.insert(QStringLiteral("leaderRequired"), roleCounts.value(QStringLiteral("leader")) > 0);
    if (!task.contains(QStringLiteral("requiredRoles"))) {
        QStringList roles;
        for (const QString& memberRole : {QStringLiteral("leader"), QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")})
            if (roleCounts.value(memberRole) > 0) roles.append(memberRole);
        task.insert(QStringLiteral("requiredRoles"), roles);
    }
    task.insert(QStringLiteral("constraints"), constraints); task.insert(QStringLiteral("assignments"), assignments);
    task.insert(QStringLiteral("vehicleCount"), assignments.size()); task.insert(QStringLiteral("candidateVehicleIds"), targetIds);
    task.insert(QStringLiteral("reserveCount"), reserves.size()); task.insert(QStringLiteral("reserveVehicleIds"), reserves);
    task.insert(QStringLiteral("target"), QVariantMap{{QStringLiteral("kind"), QStringLiteral("inclusive-run-group-remainder")}, {QStringLiteral("sourceVehicleIds"), sourceIds}});
    task.insert(QStringLiteral("flightCommandReleased"), false); task.insert(QStringLiteral("executable"), false);
    const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(task.value(QStringLiteral("area")), assignments);
    if (!flag(boundary, QStringLiteral("ok"), true)) return rejected(QStringLiteral("group-remainder-boundary-policy-invalid"));
    QVariantMap provenance{{QStringLiteral("schemaVersion"), explicitReserves ? 3 : 2}, {QStringLiteral("sourceKind"), Kind},
        {QStringLiteral("routeBoundaryPolicy"), boundary.value(QStringLiteral("policy"))},
        {QStringLiteral("sourceArchiveId"), source.value(QStringLiteral("archiveId"))}, {QStringLiteral("sourceArchiveContentHash"), source.value(QStringLiteral("contentHash"))},
        {QStringLiteral("sourceVehicleIds"), sourceIds}, {QStringLiteral("targetVehicleIds"), targetIds}, {QStringLiteral("reserveVehicleIds"), reserves},
        {QStringLiteral("memberCount"), assignments.size()}, {QStringLiteral("members"), rows}, {QStringLiteral("sourceTaskPolicyHash"), policyHash},
        {QStringLiteral("derivedTaskHash"), hash(taskProjection(task))},
        {QStringLiteral("newRoutesHash"), hash(routesProjection(assignments, constraints, reserves,
            explicitReserves ? &reservePlans : nullptr, explicitReserves && performancePolicy.toBool()))},
        {QStringLiteral("selectionPolicy"), Selection}, {QStringLiteral("timingBasis"), Timing}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("progressUsedToSkip"), false}, {QStringLiteral("completionProven"), false}, {QStringLiteral("authorityRestored"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    if (explicitReserves) provenance.insert(QStringLiteral("reservePlans"), reservePlans);
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
            QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding")}) provenance.insert(key, collection.value(key));
    task.insert(QStringLiteral("groupRemainderPlanningSource"), provenance);
    if (!matchesTask(provenance, task)) return rejected(QStringLiteral("group-remainder-proposal-budget-or-identity-invalid"));
    QVariantMap summary{{QStringLiteral("sourceKind"), Kind}, {QStringLiteral("sourceArchiveId"), source.value(QStringLiteral("archiveId"))},
        {QStringLiteral("sourceArchiveContentHash"), source.value(QStringLiteral("contentHash"))}, {QStringLiteral("memberCount"), assignments.size()},
        {QStringLiteral("sourceVehicleIds"), sourceIds}, {QStringLiteral("targetVehicleIds"), targetIds}, {QStringLiteral("reserveVehicleIds"), reserves},
        {QStringLiteral("members"), summaries}, {QStringLiteral("estimatedMaximumOutboundSeconds"), maximumOutbound},
        {QStringLiteral("estimatedMaximumReturnSeconds"), maximumReturn}, {QStringLiteral("totalRetainedHoldSeconds"), totalHolds},
        {QStringLiteral("totalPointCount"), totalPoints}, {QStringLiteral("selectionPolicy"), Selection}, {QStringLiteral("timingBasis"), Timing},
        {QStringLiteral("routeClearanceVerified"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("missionCompleted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("task"), task},
        {QStringLiteral("summary"), summary}, {QStringLiteral("provenance"), provenance}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    return bounded(result, MaximumSourceBytes) ? result : rejected(QStringLiteral("group-remainder-result-budget-exceeded"));
}

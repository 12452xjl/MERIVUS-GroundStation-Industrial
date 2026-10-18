#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetAreaCoveragePlanner.h"

#include <QCryptographicHash>
#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QtMath>

#include <cmath>

namespace {
constexpr int MaximumItems = 2048;
constexpr qint64 MaximumSourceBytes = 32LL * 1024 * 1024;
constexpr qint64 MaximumTaskBytes = 16LL * 1024 * 1024;
const QString SelectionPolicy = QStringLiteral("inclusive-waypoint-full-dwell-replay");
const QString TimingBasis = QStringLiteral("sequential-horizontal-climb-descent-estimate");
const QString NativeSourceKind = QStringLiteral("native-run-planning-snapshot");
const QString HistoricalSourceKind = QStringLiteral("historical-run-planning-snapshot");

bool numeric(const QVariant& value, double low, double high, double* result = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!qIsFinite(number) || number < low || number > high) return false;
    if (result) *result = number;
    return true;
}

bool integer(const QVariant& value, int low, int high, int* result = nullptr)
{
    double number = 0.0;
    if (!numeric(value, low, high, &number) || std::floor(number) != number) return false;
    if (result) *result = int(number);
    return true;
}

bool boolean(const QVariantMap& value, const QString& key, bool expected)
{
    const QVariant field = value.value(key);
    return field.type() == QVariant::Bool && !field.isNull() && field.toBool() == expected;
}

bool exact(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum, bool empty = false)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    if ((!empty && string.isEmpty()) || string.size() > maximum || string != string.trimmed()) return false;
    for (const QChar character : string)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
              || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool generation(const QVariant& value)
{
    if (!text(value, 20) || value.toString().startsWith(QLatin1Char('0'))) return false;
    for (const QChar character : value.toString())
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const qulonglong number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}

bool bindingValid(const QVariantMap& binding)
{
    QSet<QString> keys{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (binding.size() == 9) keys.unite({QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")});
    if (!exact(binding, keys) || !text(binding.value(QStringLiteral("sourceTaskId")), 128)
        || !text(binding.value(QStringLiteral("recordId")), 128) || !uuid(binding.value(QStringLiteral("storeEpoch")))) return false;
    for (const QString& key : {QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (!digest(binding.value(key))) return false;
    for (const QString& key : {QStringLiteral("recordRevision"), QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")}) {
        if (!binding.contains(key)) continue;
        double number = 0.0;
        if (!numeric(binding.value(key), key == QStringLiteral("recordRevision") ? 1.0 : 0.0,
                     9007199254740991.0, &number) || std::floor(number) != number) return false;
    }
    return true;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes, qint64 limit)
{
    if (depth > 24 || ++(*nodes) > 1048576) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += it.key().size() * qint64(2);
            if (*bytes > limit || !boundedTree(it.value(), depth + 1, nodes, bytes, limit)) return false;
        }
        return true;
    }
    case QVariant::List:
        if (value.toList().size() > 32768) return false;
        for (const QVariant& item : value.toList()) if (!boundedTree(item, depth + 1, nodes, bytes, limit)) return false;
        return true;
    case QVariant::StringList:
        if (value.toStringList().size() > 32768) return false;
        for (const QString& item : value.toStringList()) if (!boundedTree(item, depth + 1, nodes, bytes, limit)) return false;
        return true;
    case QVariant::String:
        *bytes += value.toString().size() * qint64(2);
        return value.toString().size() <= 32768 && *bytes <= limit
            && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: return !value.isNull();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double:
        return numeric(value, -9007199254740991.0, 9007199254740991.0);
    default: return false;
    }
}

QByteArray json(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

bool bounded(const QVariant& value, qint64 limit)
{
    int nodes = 0;
    qint64 bytes = 0;
    if (!boundedTree(value, 0, &nodes, &bytes, limit)) return false;
    const QByteArray encoded = json(value);
    return !encoded.isEmpty() && encoded.size() <= limit;
}

QString hash(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}

bool same(const QVariant& left, const QVariant& right) { return json(left) == json(right); }

QVariantMap rejected(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("task"), QVariantMap()},
        {QStringLiteral("summary"), QVariantMap()}, {QStringLiteral("provenance"), QVariantMap()},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

QVariantMap provenanceResult(bool ok, const QVariantMap& value, const QString& reason)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("provenance"), value}, {QStringLiteral("reason"), reason}};
}

QVariantMap sourceResult(bool ok, const QVariantMap& source, const QString& reason)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason}, {QStringLiteral("source"), source}};
}

// Recognized old task scope and previously calculated/runtime values must not
// become a second dispatch scope. All other bounded policies remain intact.
QVariantMap policies(const QVariantMap& source)
{
    QVariantMap result = source;
    const QStringList replaced{
        QStringLiteral("fleet"), QStringLiteral("healthReports"), QStringLiteral("nativeVehicleEvidence"),
        QStringLiteral("capabilityReviewEvidence"), QStringLiteral("templateValidation"),
        QStringLiteral("intent"), QStringLiteral("taskId"), QStringLiteral("id"), QStringLiteral("sourceTaskId"),
        QStringLiteral("coverage"), QStringLiteral("target"), QStringLiteral("planningSource"),
        QStringLiteral("remainderPlanningSource"), QStringLiteral("groupRemainderPlanningSource"),
        QStringLiteral("reserveDispatchPlanningSource"), QStringLiteral("waypoints"), QStringLiteral("route"),
        QStringLiteral("routeTiming"), QStringLiteral("returnPath"), QStringLiteral("returnRoute"), QStringLiteral("returnPaths"),
        QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members"),
        QStringLiteral("vehicleIds"), QStringLiteral("targetVehicleIds"), QStringLiteral("candidateVehicleIds"),
        QStringLiteral("selectedVehicleIds"), QStringLiteral("vehicleCount"), QStringLiteral("minVehicleCount"),
        QStringLiteral("reserve"), QStringLiteral("reserveVehicleIds"), QStringLiteral("reserveMembers"),
        QStringLiteral("reservePlans"), QStringLiteral("reserveCount"), QStringLiteral("requiredRoles"),
        QStringLiteral("leaderRequired"), QStringLiteral("leaderCount"), QStringLiteral("scoutCount"),
        QStringLiteral("relayCount"), QStringLiteral("responderCount"), QStringLiteral("leaderVehicleId"),
        QStringLiteral("leaderId"), QStringLiteral("roleAssignments"), QStringLiteral("rolePlan"),
        QStringLiteral("memberEnergyBudgets"), QStringLiteral("memberEnergyBudgetsDerivedFromPlan"),
        QStringLiteral("executionProfile"), QStringLiteral("routeBoundaryPolicy"), QStringLiteral("runId"), QStringLiteral("startupId"),
        QStringLiteral("deploymentId"), QStringLiteral("binding"), QStringLiteral("sourceEpoch"),
        QStringLiteral("approval"), QStringLiteral("approvalId"), QStringLiteral("approvalState"),
        QStringLiteral("executionPermit"), QStringLiteral("startupPermitGranted"), QStringLiteral("dispatch"),
        QStringLiteral("executable"), QStringLiteral("flightCommandReleased")};
    for (const QString& key : replaced) result.remove(key);
    return result;
}

bool estimateOnly(const QVariantMap& map)
{
    for (const QString& key : {QStringLiteral("startTime"), QStringLiteral("startAt"), QStringLiteral("endTime"),
            QStringLiteral("deadline"), QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"),
            QStringLiteral("timeSeconds"), QStringLiteral("time"), QStringLiteral("timestampSeconds"),
            QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds")})
        if (map.contains(key)) return false;
    for (const QString& key : {QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")}) {
        if (!map.contains(key)) continue;
        const QString value = map.value(key).toString();
        if (map.value(key).type() != QVariant::String || (value != QStringLiteral("estimated")
            && value != QStringLiteral("unknown") && value != QStringLiteral("estimate")
            && value != QStringLiteral("constant-speed-estimate"))) return false;
    }
    for (const QString& key : {QStringLiteral("timeReference"), QStringLiteral("returnTimeReference")}) {
        if (!map.contains(key)) continue;
        const QString value = map.value(key).toString();
        if (map.value(key).type() != QVariant::String || (value != QStringLiteral("relative")
            && value != QStringLiteral("relative-estimate") && value != QStringLiteral("unknown"))) return false;
    }
    return true;
}

bool point(const QVariant& value, bool endpoint, int index, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap input = value.toMap();
    QSet<QString> allowed{QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")};
    if (!endpoint) allowed.unite({QStringLiteral("sequence"), QStringLiteral("heading"), QStringLiteral("holdSeconds"),
        QStringLiteral("estimatedElapsedSeconds"), QStringLiteral("timingEvidence"), QStringLiteral("timeReference")});
    if ((endpoint && input.size() != 5) || input.size() > allowed.size()) return false;
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) if (!allowed.contains(it.key())) return false;
    if (input.value(QStringLiteral("coordinateSystem")) != QVariant(QStringLiteral("geodetic"))
        || input.value(QStringLiteral("altitudeReference")) != QVariant(QStringLiteral("amsl"))
        || (!endpoint && (!integer(input.value(QStringLiteral("sequence")), index, index) || !estimateOnly(input)))) return false;
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    if (!numeric(input.value(QStringLiteral("latitude")), -80, 80, &latitude)
        || !numeric(input.value(QStringLiteral("longitude")), -180, 180, &longitude)
        || !numeric(input.value(QStringLiteral("altitude")), -1000, 20000, &altitude)) return false;
    *output = {{QStringLiteral("sequence"), index}, {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")}, {QStringLiteral("latitude"), latitude},
        {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude}};
    for (const QString& key : {QStringLiteral("holdSeconds"), QStringLiteral("heading"), QStringLiteral("estimatedElapsedSeconds")}) {
        if (!input.contains(key)) continue;
        double number = 0.0;
        if (!numeric(input.value(key), key == QStringLiteral("heading") ? -1e15 : 0.0,
                     key == QStringLiteral("holdSeconds") ? 86400.0 : 1e15, &number)) return false;
        if (key != QStringLiteral("estimatedElapsedSeconds")) output->insert(key, number);
    }
    return true;
}

bool leg(const QVariantMap& from, const QVariantMap& to, double cruise, double climb, double descent,
         double* duration, double* distance)
{
    const QGeoCoordinate first(from.value(QStringLiteral("latitude")).toDouble(), from.value(QStringLiteral("longitude")).toDouble());
    const QGeoCoordinate second(to.value(QStringLiteral("latitude")).toDouble(), to.value(QStringLiteral("longitude")).toDouble());
    const double horizontal = first.distanceTo(second);
    const double vertical = to.value(QStringLiteral("altitude")).toDouble() - from.value(QStringLiteral("altitude")).toDouble();
    *duration = horizontal / cruise + std::abs(vertical) / (vertical >= 0.0 ? climb : descent);
    *distance = std::hypot(horizontal, vertical);
    return qIsFinite(*duration) && *duration >= 0.0 && qIsFinite(*distance) && *distance >= 0.0;
}

QVariantMap routeProjection(const QVariantMap& member, const QVariantMap& constraints)
{
    QVariantMap projection;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("role"), QStringLiteral("coordinateSystem"),
            QStringLiteral("altitudeReference"), QStringLiteral("returnRequired"), QStringLiteral("waypoints"),
            QStringLiteral("returnPath"), QStringLiteral("executionProfile"), QStringLiteral("estimatedDurationSeconds"),
            QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("pathLengthMeters"), QStringLiteral("returnPathLengthMeters")})
        if (member.contains(key)) projection.insert(key, member.value(key));
    projection.insert(QStringLiteral("constraints"), constraints);
    return projection;
}

QVariantMap taskProjection(QVariantMap task)
{
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("intent"), QStringLiteral("taskId"),
                              QStringLiteral("remainderPlanningSource"), QStringLiteral("templateValidation")}) task.remove(key);
    return task;
}

bool progressMatches(const QVariantMap& progress, const QVariantMap& payload, int vehicleId)
{
    if (!integer(progress.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !boolean(progress, QStringLiteral("modeQueueCommitted"), true)
        || progress.value(QStringLiteral("payloadContentHash")) != payload.value(QStringLiteral("contentHash"))) return false;
    const QVariantList mapping = payload.value(QStringLiteral("sourceItemMap")).toList();
    for (const QString& prefix : {QStringLiteral("current"), QStringLiteral("reached")}) {
        const QString sequence = prefix + QStringLiteral("Sequence"), item = prefix + QStringLiteral("MissionItem");
        if (progress.contains(sequence) != progress.contains(item)) return false;
        if (!progress.contains(sequence)) continue;
        int index = 0;
        if (!integer(progress.value(sequence), 0, mapping.size() - 1, &index)
            || !same(progress.value(item), mapping.at(index))) return false;
    }
    return true;
}
} // namespace

QVariantMap FleetMissionRemainderPlanner::validateProvenance(const QVariantMap& value)
{
    const auto reject = [] { return provenanceResult(false, {}, QStringLiteral("remainder-provenance-invalid")); };
    QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("sourceKind"), QStringLiteral("runId"),
        QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("transactionGeneration"),
        QStringLiteral("deploymentId"), QStringLiteral("binding"), QStringLiteral("sourceVehicleId"),
        QStringLiteral("sourcePayloadHash"), QStringLiteral("originalRouteHash"), QStringLiteral("sourceWaypointIndex"),
        QStringLiteral("sourceWaypointCount"), QStringLiteral("targetVehicleId"), QStringLiteral("targetRole"),
        QStringLiteral("outboundPointCount"), QStringLiteral("returnPointCount"), QStringLiteral("sourceTaskPolicyHash"),
        QStringLiteral("derivedTaskHash"), QStringLiteral("newRouteHash"), QStringLiteral("selectionPolicy"),
        QStringLiteral("timingBasis"), QStringLiteral("historicalOnly"), QStringLiteral("progressUsedToSkip"),
        QStringLiteral("completionProven"), QStringLiteral("authorityRestored"), QStringLiteral("flightCommandReleased"), QStringLiteral("executable")};
    int schema = 0;
    if (!integer(value.value(QStringLiteral("schemaVersion")), 1, 4, &schema)) return reject();
    const bool historical = schema == 2 || schema == 4;
    const bool boundaryBound = schema >= 3;
    if (historical) fields.unite({QStringLiteral("sourceArchiveId"), QStringLiteral("sourceArchiveContentHash")});
    if (boundaryBound) fields.insert(QStringLiteral("routeBoundaryPolicy"));
    if (!exact(value, fields) || !bounded(value, 64 * 1024)
        || value.value(QStringLiteral("sourceKind")) != QVariant(historical ? HistoricalSourceKind : NativeSourceKind)
        || (historical && (!uuid(value.value(QStringLiteral("sourceArchiveId")))
            || !digest(value.value(QStringLiteral("sourceArchiveContentHash")))))
        || value.value(QStringLiteral("selectionPolicy")) != QVariant(SelectionPolicy)
        || value.value(QStringLiteral("timingBasis")) != QVariant(TimingBasis)
        || !generation(value.value(QStringLiteral("transactionGeneration"))) return reject();
    if (boundaryBound) {
        const QVariant policy = value.value(QStringLiteral("routeBoundaryPolicy"));
        if (policy.type() != QVariant::Map || policy.isNull()
            || !boolean(FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(policy.toMap()), QStringLiteral("ok"), true)) return reject();
    }
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("deploymentId")})
        if (!uuid(value.value(key))) return reject();
    if (value.value(QStringLiteral("binding")).type() != QVariant::Map || !bindingValid(value.value(QStringLiteral("binding")).toMap())) return reject();
    for (const QString& key : {QStringLiteral("sourcePayloadHash"), QStringLiteral("originalRouteHash"), QStringLiteral("sourceTaskPolicyHash"),
                              QStringLiteral("derivedTaskHash"), QStringLiteral("newRouteHash")}) if (!digest(value.value(key))) return reject();
    for (const QString& key : {QStringLiteral("sourceVehicleId"), QStringLiteral("targetVehicleId")})
        if (!integer(value.value(key), 1, 255)) return reject();
    const QString role = value.value(QStringLiteral("targetRole")).toString();
    if (!text(value.value(QStringLiteral("targetRole")), 16) || (role != QStringLiteral("leader") && role != QStringLiteral("scout")
        && role != QStringLiteral("relay") && role != QStringLiteral("responder"))) return reject();
    int count = 0, first = 0, outbound = 0;
    if (!integer(value.value(QStringLiteral("sourceWaypointCount")), 1, MaximumItems, &count)
        || !integer(value.value(QStringLiteral("sourceWaypointIndex")), 0, count - 1, &first)
        || !integer(value.value(QStringLiteral("outboundPointCount")), 2, MaximumItems - 2, &outbound)
        || outbound != 1 + count - first || !integer(value.value(QStringLiteral("returnPointCount")), 2, 2)
        || !boolean(value, QStringLiteral("historicalOnly"), true)) return reject();
    for (const QString& key : {QStringLiteral("progressUsedToSkip"), QStringLiteral("completionProven"),
        QStringLiteral("authorityRestored"), QStringLiteral("flightCommandReleased"), QStringLiteral("executable")})
        if (!boolean(value, key, false)) return reject();
    return provenanceResult(true, value, QStringLiteral(""));
}

bool FleetMissionRemainderPlanner::matchesTask(const QVariantMap& provenance, const QVariantMap& task)
{
    if (!validateProvenance(provenance).value(QStringLiteral("ok")).toBool()
        || !bounded(task, MaximumTaskBytes)
        || task.contains(QStringLiteral("planningSource")) || task.contains(QStringLiteral("groupRemainderPlanningSource"))
        || task.contains(QStringLiteral("reserveDispatchPlanningSource"))
        || (task.contains(QStringLiteral("remainderPlanningSource")) && !same(task.value(QStringLiteral("remainderPlanningSource")), provenance))
        || hash(taskProjection(task)) != provenance.value(QStringLiteral("derivedTaskHash")).toString()) return false;
    if (provenance.contains(QStringLiteral("routeBoundaryPolicy"))) {
        if (task.value(QStringLiteral("assignments")).type() != QVariant::List) return false;
        const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
            task.value(QStringLiteral("area")), task.value(QStringLiteral("assignments")).toList());
        if (!boolean(boundary, QStringLiteral("ok"), true)
            || !same(boundary.value(QStringLiteral("policy")), provenance.value(QStringLiteral("routeBoundaryPolicy")))) return false;
    }
    return true;
}

bool FleetMissionRemainderPlanner::matchesPlan(const QVariantMap& provenance, const QVariantMap& plan)
{
    if (!validateProvenance(provenance).value(QStringLiteral("ok")).toBool() || !bounded(plan, MaximumTaskBytes)
        || plan.contains(QStringLiteral("planningSource")) || plan.contains(QStringLiteral("groupRemainderPlanningSource"))
        || plan.contains(QStringLiteral("reserveDispatchPlanningSource"))
        || (plan.contains(QStringLiteral("remainderPlanningSource")) && !same(plan.value(QStringLiteral("remainderPlanningSource")), provenance))
        || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("members")).toList().size() != 1
        || plan.value(QStringLiteral("constraints")).type() != QVariant::Map
        || plan.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List
        || !plan.value(QStringLiteral("reserveVehicleIds")).toList().isEmpty()
        || plan.value(QStringLiteral("reserveMembers")).type() != QVariant::List
        || !plan.value(QStringLiteral("reserveMembers")).toList().isEmpty()
        || plan.value(QStringLiteral("assignments")).type() != QVariant::List
        || !same(plan.value(QStringLiteral("assignments")), plan.value(QStringLiteral("members")))
        || plan.contains(QStringLiteral("routeBoundaryPolicy")) != provenance.contains(QStringLiteral("routeBoundaryPolicy"))
        || (provenance.contains(QStringLiteral("routeBoundaryPolicy"))
            && !same(plan.value(QStringLiteral("routeBoundaryPolicy")), provenance.value(QStringLiteral("routeBoundaryPolicy"))))
        || (plan.contains(QStringLiteral("coveragePlan")) && !plan.value(QStringLiteral("coveragePlan")).toMap().isEmpty())) return false;
    const QVariant memberValue = plan.value(QStringLiteral("members")).toList().first();
    if (memberValue.type() != QVariant::Map) return false;
    const QVariantMap member = memberValue.toMap();
    return member.value(QStringLiteral("vehicleId")) == provenance.value(QStringLiteral("targetVehicleId"))
        && member.value(QStringLiteral("role")) == provenance.value(QStringLiteral("targetRole"))
        && member.value(QStringLiteral("waypoints")).type() == QVariant::List
        && member.value(QStringLiteral("waypoints")).toList().size() == provenance.value(QStringLiteral("outboundPointCount")).toInt()
        && member.value(QStringLiteral("returnPath")).type() == QVariant::List
        && member.value(QStringLiteral("returnPath")).toList().size() == 2
        && hash(routeProjection(member, plan.value(QStringLiteral("constraints")).toMap())) == provenance.value(QStringLiteral("newRouteHash")).toString();
}

QVariantMap FleetMissionRemainderPlanner::validateSource(const QVariantMap& source)
{
    const auto reject = [](const QString& reason) { return sourceResult(false, {}, reason); };
    QSet<QString> sourceFields{QStringLiteral("schemaVersion"), QStringLiteral("ok"), QStringLiteral("reason"),
        QStringLiteral("sourceKind"), QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
        QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding"),
        QStringLiteral("sourceVehicleId"), QStringLiteral("sourceTask"), QStringLiteral("sourcePlan"),
        QStringLiteral("sourceMember"), QStringLiteral("sourcePayload"), QStringLiteral("progress"),
        QStringLiteral("readonly"), QStringLiteral("flightCommandReleased")};
    int schema = 0;
    if (!integer(source.value(QStringLiteral("schemaVersion")), 1, 2, &schema))
        return reject(QStringLiteral("remainder-source-invalid"));
    const bool historical = schema == 2;
    if (historical) sourceFields.unite({QStringLiteral("sourceArchiveId"), QStringLiteral("sourceArchiveContentHash")});
    if (!exact(source, sourceFields) || !bounded(source, MaximumSourceBytes)
        || !boolean(source, QStringLiteral("ok"), true) || !boolean(source, QStringLiteral("readonly"), true)
        || !boolean(source, QStringLiteral("flightCommandReleased"), false)
        || source.value(QStringLiteral("reason")).type() != QVariant::String || !source.value(QStringLiteral("reason")).toString().isEmpty()
        || source.value(QStringLiteral("sourceKind")) != QVariant(historical ? HistoricalSourceKind : NativeSourceKind)
        || (historical && (!uuid(source.value(QStringLiteral("sourceArchiveId")))
            || !digest(source.value(QStringLiteral("sourceArchiveContentHash")))))
        || !generation(source.value(QStringLiteral("transactionGeneration")))) return reject(QStringLiteral("remainder-source-invalid"));
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("deploymentId")})
        if (!uuid(source.value(key))) return reject(QStringLiteral("remainder-source-identity-invalid"));
    for (const QString& key : {QStringLiteral("binding"), QStringLiteral("sourceTask"), QStringLiteral("sourcePlan"),
                              QStringLiteral("sourceMember"), QStringLiteral("sourcePayload"), QStringLiteral("progress")})
        if (source.value(key).type() != QVariant::Map || source.value(key).toMap().isEmpty())
            return reject(QStringLiteral("remainder-source-map-missing"));
    const QVariantMap binding = source.value(QStringLiteral("binding")).toMap();
    const QVariantMap plan = source.value(QStringLiteral("sourcePlan")).toMap();
    const QVariantMap originalMember = source.value(QStringLiteral("sourceMember")).toMap();
    const QVariantMap payload = source.value(QStringLiteral("sourcePayload")).toMap();
    const QVariantMap progress = source.value(QStringLiteral("progress")).toMap();
    int sourceId = 0;
    if (!bindingValid(binding) || !integer(source.value(QStringLiteral("sourceVehicleId")), 1, 255, &sourceId)
        || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId"))
        || plan.value(QStringLiteral("sourceTaskId")) != binding.value(QStringLiteral("sourceTaskId"))
        || hash(plan) != binding.value(QStringLiteral("contentHash")).toString()
        || !integer(originalMember.value(QStringLiteral("vehicleId")), sourceId, sourceId)
        || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("members")).toList().isEmpty() || plan.value(QStringLiteral("members")).toList().size() > 32)
        return reject(QStringLiteral("remainder-source-binding-mismatch"));
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
            source.value(QStringLiteral("sourceTask")).toMap().value(QStringLiteral("area")),
            plan.value(QStringLiteral("members")).toList());
        if (!boolean(boundary, QStringLiteral("ok"), true)
            || !same(boundary.value(QStringLiteral("policy")), plan.value(QStringLiteral("routeBoundaryPolicy"))))
            return reject(QStringLiteral("remainder-source-boundary-policy-mismatch"));
    }
    int matchingMembers = 0;
    for (const QVariant& value : plan.value(QStringLiteral("members")).toList()) {
        if (value.type() != QVariant::Map) return reject(QStringLiteral("remainder-source-member-invalid"));
        if (integer(value.toMap().value(QStringLiteral("vehicleId")), sourceId, sourceId)) {
            ++matchingMembers;
            if (!same(value, originalMember)) return reject(QStringLiteral("remainder-source-member-mismatch"));
        }
    }
    if (matchingMembers != 1) return reject(QStringLiteral("remainder-source-member-mismatch"));
    // Replay the actual encoder contract, not an assumed one-item-per-point
    // relationship. This is local deterministic serialization, never READ.
    const QVariantMap encoded = FleetMissionPayloadBuilder::reproduceHistoricalPayloads(plan);
    if (!boolean(encoded, QStringLiteral("ok"), true)) return reject(QStringLiteral("remainder-source-payload-unreproducible"));
    int matchingPayloads = 0;
    for (const QVariant& value : encoded.value(QStringLiteral("payloads")).toList()) {
        if (integer(value.toMap().value(QStringLiteral("vehicleId")), sourceId, sourceId)) {
            ++matchingPayloads;
            if (!same(value, payload)) return reject(QStringLiteral("remainder-source-payload-mismatch"));
        }
    }
    if (matchingPayloads != 1 || !digest(payload.value(QStringLiteral("contentHash")))
        || !progressMatches(progress, payload, sourceId)) return reject(QStringLiteral("remainder-source-progress-mismatch"));
    // Independently owned JSON values retain explicit null and every source
    // field. This proves consistency, not native ownership, a live aircraft
    // source or that an archive hash authenticates its captured statements.
    const QByteArray encodedSource = json(source);
    const QJsonDocument document = QJsonDocument::fromJson(encodedSource);
    if (!document.isObject()) return reject(QStringLiteral("remainder-source-snapshot-invalid"));
    const QVariantMap owned = document.toVariant().toMap();
    if (json(owned) != encodedSource) return reject(QStringLiteral("remainder-source-snapshot-invalid"));
    return sourceResult(true, owned, QStringLiteral(""));
}

QVariantMap FleetMissionRemainderPlanner::build(const QVariantMap& suppliedSource, const QVariantMap& request)
{
    const QVariantMap checked = validateSource(suppliedSource);
    if (!boolean(checked, QStringLiteral("ok"), true)) return rejected(checked.value(QStringLiteral("reason")).toString());
    const QVariantMap source = checked.value(QStringLiteral("source")).toMap();
    const QVariantMap binding = source.value(QStringLiteral("binding")).toMap();
    const QVariantMap originalTask = source.value(QStringLiteral("sourceTask")).toMap();
    const QVariantMap plan = source.value(QStringLiteral("sourcePlan")).toMap();
    const QVariantMap originalMember = source.value(QStringLiteral("sourceMember")).toMap();
    const QVariantMap payload = source.value(QStringLiteral("sourcePayload")).toMap();
    const QVariantMap progress = source.value(QStringLiteral("progress")).toMap();
    const int sourceId = source.value(QStringLiteral("sourceVehicleId")).toInt();
    const bool historical = source.value(QStringLiteral("schemaVersion")).toInt() == 2;
    const QSet<QString> requestFields{QStringLiteral("sourceWaypointIndex"), QStringLiteral("targetVehicleId"),
        QStringLiteral("start"), QStringLiteral("returnTo"), QStringLiteral("cruiseSpeedMetersPerSecond"),
        QStringLiteral("climbSpeedMetersPerSecond"), QStringLiteral("descentSpeedMetersPerSecond"),
        QStringLiteral("executionProfile"), QStringLiteral("operatorConfirmed")};
    if (!exact(request, requestFields) || !bounded(request, 128 * 1024)
        || !boolean(request, QStringLiteral("operatorConfirmed"), true)) return rejected(QStringLiteral("remainder-explicit-request-required"));
    int targetId = 0, first = 0;
    if (!integer(request.value(QStringLiteral("targetVehicleId")), 1, 255, &targetId)) return rejected(QStringLiteral("remainder-target-invalid"));
    const QVariant routeValue = originalMember.value(QStringLiteral("waypoints"));
    if (routeValue.type() != QVariant::List || routeValue.toList().isEmpty() || routeValue.toList().size() > MaximumItems)
        return rejected(QStringLiteral("remainder-original-route-missing"));
    const QVariantList originalRoute = routeValue.toList();
    if (!integer(request.value(QStringLiteral("sourceWaypointIndex")), 0, originalRoute.size() - 1, &first))
        return rejected(QStringLiteral("remainder-waypoint-index-invalid"));
    const QString role = originalMember.value(QStringLiteral("role")).toString();
    if (role != QStringLiteral("leader") && role != QStringLiteral("scout") && role != QStringLiteral("relay") && role != QStringLiteral("responder"))
        return rejected(QStringLiteral("remainder-original-role-unsupported"));
    if (plan.value(QStringLiteral("constraints")).type() != QVariant::Map) return rejected(QStringLiteral("remainder-constraints-missing"));
    QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    // An old absolute/authoritative schedule is a requirement, not an
    // estimate that this local geometry feature may silently erase.
    if (!estimateOnly(originalTask) || !estimateOnly(originalMember) || !estimateOnly(constraints)
        || (plan.contains(QStringLiteral("routeTiming")) && (plan.value(QStringLiteral("routeTiming")).type() != QVariant::Map
            || !estimateOnly(plan.value(QStringLiteral("routeTiming")).toMap())))
        return rejected(QStringLiteral("remainder-explicit-timing-replan-required"));
    const QVariantMap oldProfile = originalMember.value(QStringLiteral("executionProfile")).toMap();
    if (oldProfile.value(QStringLiteral("timingPolicy")).toString() == QStringLiteral("absolute-required"))
        return rejected(QStringLiteral("remainder-explicit-timing-replan-required"));
    QVariantMap start, returning;
    if (!point(request.value(QStringLiteral("start")), true, 0, &start)
        || !point(request.value(QStringLiteral("returnTo")), true, 1, &returning))
        return rejected(QStringLiteral("remainder-endpoints-invalid"));
    double cruise = 0.0, climb = 0.0, descent = 0.0;
    if (!numeric(request.value(QStringLiteral("cruiseSpeedMetersPerSecond")), 0, 1000, &cruise) || cruise <= 0.0
        || !numeric(request.value(QStringLiteral("climbSpeedMetersPerSecond")), 0, 1000, &climb) || climb <= 0.0
        || !numeric(request.value(QStringLiteral("descentSpeedMetersPerSecond")), 0, 1000, &descent) || descent <= 0.0)
        return rejected(QStringLiteral("remainder-speed-invalid"));
    if (request.value(QStringLiteral("executionProfile")).type() != QVariant::Map || request.value(QStringLiteral("executionProfile")).isNull())
        return rejected(QStringLiteral("remainder-profile-invalid"));
    QVariantMap profile = request.value(QStringLiteral("executionProfile")).toMap();
    double returnCruise = cruise;
    int actionItems = 0;
    if (!profile.isEmpty()) {
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(profile);
        if (!boolean(normalized, QStringLiteral("ok"), true)) return rejected(QStringLiteral("remainder-profile-invalid"));
        profile = normalized.value(QStringLiteral("profile")).toMap();
        if (profile.value(QStringLiteral("timingPolicy")).toString() != QStringLiteral("relative-dwell"))
            return rejected(QStringLiteral("remainder-explicit-timing-replan-required"));
        if (profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")).toDouble() != cruise)
            return rejected(QStringLiteral("remainder-profile-speed-conflict"));
        const QVariantMap vertical = profile.value(QStringLiteral("verticalPolicy")).toMap();
        if ((vertical.contains(QStringLiteral("climbSpeedMetersPerSecond")) && vertical.value(QStringLiteral("climbSpeedMetersPerSecond")).toDouble() != climb)
            || (vertical.contains(QStringLiteral("descentSpeedMetersPerSecond")) && vertical.value(QStringLiteral("descentSpeedMetersPerSecond")).toDouble() != descent))
            return rejected(QStringLiteral("remainder-profile-speed-conflict"));
        actionItems = 1;
        if (profile.contains(QStringLiteral("returnSpeedMetersPerSecond"))) {
            returnCruise = profile.value(QStringLiteral("returnSpeedMetersPerSecond")).toDouble();
            ++actionItems;
        }
        if (profile.value(QStringLiteral("launchAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("takeoff")) ++actionItems;
        const QString end = profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString();
        if (end == QStringLiteral("hold") || end == QStringLiteral("land")) ++actionItems;
    }
    const int retainedCount = originalRoute.size() - first;
    if (retainedCount + 3 + actionItems > MaximumItems) return rejected(QStringLiteral("remainder-item-budget-exceeded"));
    double ceiling = 0.0;
    if (!numeric(constraints.value(QStringLiteral("maxAltitudeMeters")), -1000, 20000, &ceiling)
        || constraints.value(QStringLiteral("maxAltitudeReference")) != QVariant(QStringLiteral("amsl"))
        || start.value(QStringLiteral("altitude")).toDouble() > ceiling || returning.value(QStringLiteral("altitude")).toDouble() > ceiling)
        return rejected(QStringLiteral("remainder-altitude-constraint-invalid"));
    QVariantList suffix;
    // Validate the complete original route before selecting. No skipped
    // malformed point is reinterpreted as proof that it was completed.
    for (int index = 0; index < originalRoute.size(); ++index) {
        QVariantMap canonical;
        if (!estimateOnly(originalRoute.at(index).toMap())) return rejected(QStringLiteral("remainder-explicit-timing-replan-required"));
        if (!point(originalRoute.at(index), false, index, &canonical)
            || canonical.value(QStringLiteral("altitude")).toDouble() > ceiling) return rejected(QStringLiteral("remainder-original-point-invalid"));
        if (index >= first) suffix.append(canonical);
    }
    double ingress = 0.0, ingressDistance = 0.0, work = 0.0, workDistance = 0.0, holds = 0.0;
    double returnSeconds = 0.0, returnDistance = 0.0;
    if (!leg(start, suffix.first().toMap(), cruise, climb, descent, &ingress, &ingressDistance)
        || !leg(suffix.last().toMap(), returning, returnCruise, climb, descent, &returnSeconds, &returnDistance))
        return rejected(QStringLiteral("remainder-estimate-nonfinite"));
    start.insert(QStringLiteral("holdSeconds"), 0.0);
    start.insert(QStringLiteral("estimatedElapsedSeconds"), 0.0);
    QVariantList outbound{start};
    double elapsed = ingress;
    for (int index = 0; index < suffix.size(); ++index) {
        QVariantMap current = suffix.at(index).toMap();
        if (index > 0) {
            double seconds = 0.0, distance = 0.0;
            if (!leg(suffix.at(index - 1).toMap(), current, cruise, climb, descent, &seconds, &distance))
                return rejected(QStringLiteral("remainder-estimate-nonfinite"));
            work += seconds; workDistance += distance; elapsed += seconds;
        }
        current.insert(QStringLiteral("sequence"), index + 1);
        current.insert(QStringLiteral("estimatedElapsedSeconds"), elapsed);
        const double hold = current.value(QStringLiteral("holdSeconds"), 0.0).toDouble();
        holds += hold;
        outbound.append(current);
    }
    const double duration = ingress + work + holds, distance = ingressDistance + workDistance;
    if (!qIsFinite(duration) || !qIsFinite(duration + returnSeconds) || !qIsFinite(distance)) return rejected(QStringLiteral("remainder-estimate-nonfinite"));
    QVariantMap returnStart = outbound.last().toMap();
    returnStart.insert(QStringLiteral("sequence"), 0);
    returnStart.insert(QStringLiteral("holdSeconds"), 0.0);
    returnStart.insert(QStringLiteral("estimatedElapsedSeconds"), 0.0);
    // Heading remains a requirement on the retained outbound point only;
    // the independently generated return connector carries no invented yaw.
    returnStart.remove(QStringLiteral("heading"));
    returning.insert(QStringLiteral("holdSeconds"), 0.0);
    returning.insert(QStringLiteral("estimatedElapsedSeconds"), returnSeconds);
    const QVariantList returnPath{returnStart, returning};
    QVariantMap member{{QStringLiteral("vehicleId"), targetId}, {QStringLiteral("role"), role},
        {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")}, {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
        {QStringLiteral("returnRequired"), true}, {QStringLiteral("waypoints"), outbound}, {QStringLiteral("returnPath"), returnPath},
        // Coordinator adds explicit holds to movement estimates exactly once.
        {QStringLiteral("estimatedDurationSeconds"), ingress + work}, {QStringLiteral("estimatedReturnDurationSeconds"), returnSeconds},
        {QStringLiteral("pathLengthMeters"), distance}, {QStringLiteral("returnPathLengthMeters"), returnDistance},
        {QStringLiteral("timingEvidence"), QStringLiteral("constant-speed-estimate")},
        {QStringLiteral("returnTimingEvidence"), QStringLiteral("constant-speed-estimate")},
        {QStringLiteral("routeSource"), QStringLiteral("operator-inclusive-remainder")},
        {QStringLiteral("returnRouteSource"), QStringLiteral("operator-explicit-remainder-return")}};
    if (!profile.isEmpty()) member.insert(QStringLiteral("executionProfile"), profile);
    QVariantMap task = policies(originalTask);
    const QString policyHash = hash(task);
    task.insert(QStringLiteral("constraints"), constraints);
    // Explicit old member duration floors are policy, unlike elapsed route
    // estimates. Preserve them when replacing the original assignment scope.
    double memberDurationFloor = 0.0;
    for (const QString& key : {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                              QStringLiteral("taskDurationSeconds"), QStringLiteral("timeLimitSeconds")}) {
        if (!originalMember.contains(key)) continue;
        double oldDuration = 0.0;
        if (!numeric(originalMember.value(key), 0.0, 9007199254740991.0, &oldDuration) || oldDuration <= 0.0)
            return rejected(QStringLiteral("remainder-original-duration-policy-invalid"));
        memberDurationFloor = qMax(memberDurationFloor, oldDuration);
    }
    if (memberDurationFloor > 0.0) {
        double taskDuration = 0.0;
        if (task.contains(QStringLiteral("missionDurationSeconds"))
            && (!numeric(task.value(QStringLiteral("missionDurationSeconds")), 0.0, 9007199254740991.0, &taskDuration) || taskDuration <= 0.0))
            return rejected(QStringLiteral("remainder-original-duration-policy-invalid"));
        task.insert(QStringLiteral("missionDurationSeconds"), qMax(memberDurationFloor, taskDuration));
    }
    if (originalMember.contains(QStringLiteral("returnDurationSeconds"))) {
        double oldReturn = 0.0, suppliedFloor = 0.0;
        bool supplied = false;
        if (!numeric(originalMember.value(QStringLiteral("returnDurationSeconds")), 0.0, 9007199254740991.0, &oldReturn) || oldReturn <= 0.0)
            return rejected(QStringLiteral("remainder-original-duration-policy-invalid"));
        for (const QVariantMap& scope : {task, constraints}) {
            for (const QString& key : {QStringLiteral("returnReserveSeconds"), QStringLiteral("returnToHomeReserveSeconds")}) {
                if (!scope.contains(key)) continue;
                double value = 0.0;
                if (!numeric(scope.value(key), 0.0, 9007199254740991.0, &value))
                    return rejected(QStringLiteral("remainder-original-duration-policy-invalid"));
                supplied = true;
                suppliedFloor = qMax(suppliedFloor, value);
            }
        }
        // The ordinary Coordinator's missing return floor is 120 seconds.
        const double floor = qMax(oldReturn, supplied ? suppliedFloor : 120.0);
        task.insert(QStringLiteral("returnReserveSeconds"), floor);
        constraints.insert(QStringLiteral("returnReserveSeconds"), floor);
        task.insert(QStringLiteral("constraints"), constraints);
    }
    double originalMinimumSpeed = 0.0;
    if (task.contains(QStringLiteral("minimumSpeedMps"))
        && !numeric(task.value(QStringLiteral("minimumSpeedMps")), 0.0, 9007199254740991.0, &originalMinimumSpeed))
        return rejected(QStringLiteral("remainder-original-speed-policy-invalid"));
    // Make the newly requested horizontal speeds visible to the existing
    // capability review; calculating faster geometry is not speed evidence.
    task.insert(QStringLiteral("minimumSpeedMps"), qMax(originalMinimumSpeed, qMax(cruise, returnCruise)));
    // Re-scoping one selected source slot is explicit; retain all capability,
    // safety, energy, template and no-fly policies rather than choosing a new
    // candidate through the matcher. Other roles' counts are not this task.
    task.insert(QStringLiteral("assignments"), QVariantList{member});
    task.insert(QStringLiteral("vehicleCount"), 1);
    task.insert(QStringLiteral("candidateVehicleIds"), QVariantList{targetId});
    task.insert(QStringLiteral("reserveCount"), 0);
    task.insert(QStringLiteral("reserveVehicleIds"), QVariantList());
    task.insert(QStringLiteral("leaderRequired"), role == QStringLiteral("leader"));
    task.insert(QStringLiteral("requiredRoles"), QStringList{role});
    for (const QString& name : {QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")})
        task.insert(name + QStringLiteral("Count"), name == role ? 1 : 0);
    task.insert(QStringLiteral("target"), QVariantMap{{QStringLiteral("kind"), QStringLiteral("inclusive-route-remainder")},
        {QStringLiteral("sourceVehicleId"), sourceId}, {QStringLiteral("sourceWaypointIndex"), first}});
    task.insert(QStringLiteral("flightCommandReleased"), false);
    task.insert(QStringLiteral("executable"), false);
    const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
        task.value(QStringLiteral("area")), QVariantList{member});
    if (!boolean(boundary, QStringLiteral("ok"), true)) return rejected(QStringLiteral("remainder-boundary-policy-invalid"));
    const QString routeHash = hash(routeProjection(member, constraints));
    QVariantMap provenance{{QStringLiteral("schemaVersion"), historical ? 4 : 3}, {QStringLiteral("sourceKind"), source.value(QStringLiteral("sourceKind"))},
        {QStringLiteral("routeBoundaryPolicy"), boundary.value(QStringLiteral("policy"))},
        {QStringLiteral("runId"), source.value(QStringLiteral("runId"))}, {QStringLiteral("startupId"), source.value(QStringLiteral("startupId"))},
        {QStringLiteral("transactionEpoch"), source.value(QStringLiteral("transactionEpoch"))},
        {QStringLiteral("transactionGeneration"), source.value(QStringLiteral("transactionGeneration"))},
        {QStringLiteral("deploymentId"), source.value(QStringLiteral("deploymentId"))}, {QStringLiteral("binding"), binding},
        {QStringLiteral("sourceVehicleId"), sourceId}, {QStringLiteral("sourcePayloadHash"), payload.value(QStringLiteral("contentHash"))},
        {QStringLiteral("originalRouteHash"), hash(originalRoute)}, {QStringLiteral("sourceWaypointIndex"), first},
        {QStringLiteral("sourceWaypointCount"), originalRoute.size()}, {QStringLiteral("targetVehicleId"), targetId},
        {QStringLiteral("targetRole"), role}, {QStringLiteral("outboundPointCount"), outbound.size()},
        {QStringLiteral("returnPointCount"), 2}, {QStringLiteral("sourceTaskPolicyHash"), policyHash},
        {QStringLiteral("derivedTaskHash"), hash(taskProjection(task))}, {QStringLiteral("newRouteHash"), routeHash},
        {QStringLiteral("selectionPolicy"), SelectionPolicy}, {QStringLiteral("timingBasis"), TimingBasis},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("progressUsedToSkip"), false},
        {QStringLiteral("completionProven"), false}, {QStringLiteral("authorityRestored"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    if (historical) {
        provenance.insert(QStringLiteral("sourceArchiveId"), source.value(QStringLiteral("sourceArchiveId")));
        provenance.insert(QStringLiteral("sourceArchiveContentHash"), source.value(QStringLiteral("sourceArchiveContentHash")));
    }
    task.insert(QStringLiteral("remainderPlanningSource"), provenance);
    if (!bounded(task, MaximumTaskBytes) || !matchesTask(provenance, task)) return rejected(QStringLiteral("remainder-proposal-budget-or-identity-invalid"));
    QVariantMap summary{{QStringLiteral("sourceKind"), source.value(QStringLiteral("sourceKind"))},
        {QStringLiteral("sourceVehicleId"), sourceId}, {QStringLiteral("targetVehicleId"), targetId},
        {QStringLiteral("sourceWaypointIndex"), first}, {QStringLiteral("sourceWaypointCount"), originalRoute.size()},
        {QStringLiteral("retainedWaypointCount"), retainedCount}, {QStringLiteral("outboundPointCount"), outbound.size()},
        {QStringLiteral("returnPointCount"), 2}, {QStringLiteral("estimatedIngressSeconds"), ingress},
        {QStringLiteral("estimatedTaskSeconds"), work}, {QStringLiteral("retainedHoldSeconds"), holds},
        {QStringLiteral("estimatedOutboundSeconds"), duration}, {QStringLiteral("estimatedReturnSeconds"), returnSeconds},
        {QStringLiteral("sourcePayloadHash"), payload.value(QStringLiteral("contentHash"))},
        {QStringLiteral("originalRouteHash"), hash(originalRoute)}, {QStringLiteral("newRouteHash"), routeHash},
        {QStringLiteral("executionProfileProvided"), !profile.isEmpty()}, {QStringLiteral("sourceProgress"), progress},
        {QStringLiteral("selectionPolicy"), SelectionPolicy}, {QStringLiteral("routeClearanceVerified"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("missionCompleted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (historical) {
        summary.insert(QStringLiteral("sourceArchiveId"), source.value(QStringLiteral("sourceArchiveId")));
        summary.insert(QStringLiteral("sourceArchiveContentHash"), source.value(QStringLiteral("sourceArchiveContentHash")));
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("task"), task},
        {QStringLiteral("summary"), summary}, {QStringLiteral("provenance"), provenance},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

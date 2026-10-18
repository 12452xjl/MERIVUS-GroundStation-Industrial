#include "FleetMissionPlanStore.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetMissionClearanceInput.h"
#include "FleetMissionClearanceReview.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionDraft.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetReserveDispatchPlanner.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLockFile>
#include <QMetaType>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QtMath>

#include <cmath>

namespace {
constexpr qint64 MaxArchiveBytes = 32LL * 1024 * 1024;
constexpr qint64 MaxSnapshotBytes = 16LL * 1024 * 1024;
constexpr int MaxSnapshotNodes = 512000;
constexpr int MaxTasks = 256;
constexpr int MaxVersionsPerTask = 128;
constexpr int MaxRecords = 2048;
constexpr qint64 MaxHistoryDirectoryBytes = 256LL * 1024;
constexpr double MaxExactRevision = 9007199254740991.0;

bool integer(const QVariant& value, qint64* output)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!qIsFinite(number) || number < 0.0 || number > MaxExactRevision || std::floor(number) != number) return false;
    *output = static_cast<qint64>(number);
    return true;
}

bool identifier(const QVariant& value, QString* output)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > 128) return false;
    const QString text = value.toString();
    if (text != text.trimmed()) return false;
    for (const QChar character : text) {
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    }
    *output = text;
    return true;
}

bool digestText(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (const QChar character : value.toString()) {
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
              || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    }
    return true;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 16 || ++(*nodes) > MaxSnapshotNodes) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += static_cast<qint64>(it.key().size()) * 2;
            if (*bytes > MaxSnapshotBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& item : value.toList()) {
            if (!boundedTree(item, depth + 1, nodes, bytes)) return false;
        }
        return true;
    case QVariant::StringList:
        for (const QString& item : value.toStringList()) {
            if (!boundedTree(item, depth + 1, nodes, bytes)) return false;
        }
        return true;
    case QVariant::String:
        *bytes += static_cast<qint64>(value.toString().size()) * 2;
        return *bytes <= MaxSnapshotBytes
            && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong:
        return std::abs(value.toDouble()) <= MaxExactRevision;
    case QVariant::Double: return qIsFinite(value.toDouble());
    default: return false;
    }
}

bool snapshot(const QVariantMap& value, QByteArray* encoded)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!boundedTree(value, 0, &nodes, &scalarBytes)) return false;
    *encoded = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    return !encoded->isEmpty() && encoded->size() <= MaxSnapshotBytes;
}

QString hash(const QByteArray& encoded)
{
    return QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex());
}

bool falseField(const QVariantMap& map, const QString& key)
{
    return map.value(key).type() == QVariant::Bool && !map.value(key).toBool();
}

bool trueField(const QVariantMap& map, const QString& key)
{
    return map.value(key).type() == QVariant::Bool && map.value(key).toBool();
}

bool performanceFloor(const QVariant& value, double* output)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!qIsFinite(number) || number < 0.0 || number > MaxExactRevision) return false;
    if (output) *output = number;
    return true;
}

bool validPlan(const QString& taskId, const QVariantMap& plan, QByteArray* encoded)
{
    if (!snapshot(plan, encoded)) return false;
    const QSet<QString> keys = QSet<QString>() << QStringLiteral("schemaVersion") << QStringLiteral("planVersion")
        << QStringLiteral("compilerVersion") << QStringLiteral("planId") << QStringLiteral("sourceTaskId")
        << QStringLiteral("intent") << QStringLiteral("areaSummary") << QStringLiteral("routeBoundaryPolicy") << QStringLiteral("target")
        << QStringLiteral("waypoints") << QStringLiteral("waypointCount") << QStringLiteral("routeTiming")
        << QStringLiteral("coveragePlan") << QStringLiteral("members") << QStringLiteral("assignments")
        << QStringLiteral("fleetProvided") << QStringLiteral("planningOnly") << QStringLiteral("reserveVehicleIds")
        << QStringLiteral("reserveMembers") << QStringLiteral("constraints") << QStringLiteral("risks")
        << QStringLiteral("riskSignals") << QStringLiteral("unmetRequirements") << QStringLiteral("feasible")
        << QStringLiteral("reviewStatus") << QStringLiteral("manualConfirmationRequired")
        << QStringLiteral("requiresOperatorReview") << QStringLiteral("flightCommandReleased")
        << QStringLiteral("dispatch") << QStringLiteral("compilerLimitations")
        << QStringLiteral("roleAllocationApplied") << QStringLiteral("rolePlan") << QStringLiteral("templateValidation")
        << QStringLiteral("executionProfile") << QStringLiteral("planningSource") << QStringLiteral("remainderPlanningSource")
        << QStringLiteral("groupRemainderPlanningSource") << QStringLiteral("reserveDispatchPlanningSource")
        << QStringLiteral("planningClearanceReview") << QStringLiteral("requirePerformanceEvidence")
        << QStringLiteral("minimumSpeedMps") << QStringLiteral("minimumEnduranceSeconds")
        // The compiler retains these original task-root requirements when it
        // materializes an ordinary global route. Keep diagnostic values too:
        // this store snapshots content, it does not repeat the compiler or
        // turn an invalid/unencoded requirement into execution permission.
        << QStringLiteral("timeSeconds") << QStringLiteral("time") << QStringLiteral("timestampSeconds")
        << QStringLiteral("timestamp") << QStringLiteral("t") << QStringLiteral("etaSeconds")
        << QStringLiteral("startTime") << QStringLiteral("startAt") << QStringLiteral("endTime")
        << QStringLiteral("deadline") << QStringLiteral("durationSeconds")
        << QStringLiteral("missionDurationSeconds") << QStringLiteral("taskDurationSeconds")
        << QStringLiteral("returnStartTime") << QStringLiteral("returnEndTime")
        << QStringLiteral("returnDurationSeconds") << QStringLiteral("timeLimitSeconds")
        << QStringLiteral("returnReserveSeconds") << QStringLiteral("timeReference")
        << QStringLiteral("returnTimeReference") << QStringLiteral("timingEvidence")
        << QStringLiteral("returnTimingEvidence") << QStringLiteral("coordinateSystem")
        << QStringLiteral("altitudeReference") << QStringLiteral("localFrameId");
    for (auto it = plan.constBegin(); it != plan.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    if (plan.contains(QStringLiteral("requirePerformanceEvidence"))
        && (plan.value(QStringLiteral("requirePerformanceEvidence")).type() != QVariant::Bool
            || plan.value(QStringLiteral("requirePerformanceEvidence")).isNull())) return false;
    for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")})
        if (plan.contains(key) && !performanceFloor(plan.value(key), nullptr)) return false;
    // The strict compiler generation cannot be stripped into a legacy plan.
    // Historical generations remain inert records; they do not acquire the
    // current source-qualified performance policy merely from raw numbers.
    if (plan.value(QStringLiteral("compilerVersion")) == QVariant(QStringLiteral("0.8.0"))
        && !trueField(plan, QStringLiteral("requirePerformanceEvidence"))) return false;
    // Historical plans without the field remain inert archive content. A new
    // compiler marker cannot claim the complete source contract while omitting
    // it; deployment independently requires the field for every plan version.
    if ((plan.value(QStringLiteral("planVersion")) == QVariant(QStringLiteral("1.1"))
            || plan.value(QStringLiteral("compilerVersion")) == QVariant(QStringLiteral("0.7.0")))
        && !plan.contains(QStringLiteral("routeBoundaryPolicy"))) return false;
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariant value = plan.value(QStringLiteral("routeBoundaryPolicy"));
        if (value.type() != QVariant::Map || value.isNull()
            || !trueField(FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(value.toMap()), QStringLiteral("ok"))) return false;
        if (plan.contains(QStringLiteral("coveragePlan"))) {
            const QVariant coverageValue = plan.value(QStringLiteral("coveragePlan"));
            if (coverageValue.type() != QVariant::Map || coverageValue.isNull()) return false;
            const QVariantMap coverage = coverageValue.toMap(), policy = value.toMap();
            const bool declarationPresent = coverage.contains(QStringLiteral("holes"))
                || coverage.contains(QStringLiteral("holeCount")) || coverage.contains(QStringLiteral("boundarySetbackMeters"));
            const bool generated = trueField(coverage, QStringLiteral("generated"));
            if (declarationPresent || (trueField(policy, QStringLiteral("active")) && generated)) {
                const QVariant holes = coverage.value(QStringLiteral("holes"));
                qint64 holeCount = 0;
                if (!trueField(policy, QStringLiteral("active")) || holes.type() != QVariant::List || holes.isNull()
                    || holes.toList().size() > 8 || coverage.value(QStringLiteral("holeCount")).isNull()
                    || !integer(coverage.value(QStringLiteral("holeCount")), &holeCount)
                    || holeCount != holes.toList().size() || !coverage.contains(QStringLiteral("boundarySetbackMeters"))) return false;
                // Coverage summaries are not the source. Rebuild their full
                // declaration in the retained original outer/frame and reject
                // a changed, removed or partial hole/setback declaration.
                QVariantMap mergedArea = policy.value(QStringLiteral("area")).toMap();
                mergedArea.insert(QStringLiteral("holes"), holes);
                mergedArea.insert(QStringLiteral("boundarySetbackMeters"), coverage.value(QStringLiteral("boundarySetbackMeters")));
                const QVariantMap rebuilt = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
                    mergedArea, plan.value(QStringLiteral("members")).toList());
                if (!trueField(rebuilt, QStringLiteral("ok"))
                    || QJsonDocument::fromVariant(policy).toJson(QJsonDocument::Compact)
                        != QJsonDocument::fromVariant(rebuilt.value(QStringLiteral("policy"))).toJson(QJsonDocument::Compact)) return false;
            }
        }
    }
    if (int(plan.contains(QStringLiteral("planningSource")))
        + int(plan.contains(QStringLiteral("remainderPlanningSource")))
        + int(plan.contains(QStringLiteral("groupRemainderPlanningSource")))
        + int(plan.contains(QStringLiteral("reserveDispatchPlanningSource"))) > 1) return false;
    const QVariant constraintsValue = plan.value(QStringLiteral("constraints"));
    // This optional schema does not reinterpret historical constraints. Once
    // present, however, the entire declaration and its exact active-member
    // binding must survive canonical JSON unchanged; do not save a prefix.
    if (constraintsValue.type() == QVariant::Map && !constraintsValue.isNull()
        && constraintsValue.toMap().contains(QStringLiteral("clearanceInput"))) {
        const QVariant input = constraintsValue.toMap().value(QStringLiteral("clearanceInput"));
        // inspectForPlan already normalizes this complete declaration. Do
        // not run the same bounded polygon/tree pass a second time here.
        const QVariantMap inspected = FleetMissionClearanceInput::inspectForPlan(plan);
        if (!trueField(inspected, QStringLiteral("provided"))
            || !trueField(inspected, QStringLiteral("inputValid"))
            || !trueField(inspected, QStringLiteral("memberIdsMatch"))
            || inspected.value(QStringLiteral("input")).type() != QVariant::Map
            || inspected.value(QStringLiteral("input")).isNull()
            || QJsonDocument::fromVariant(input).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(inspected.value(QStringLiteral("input"))).toJson(QJsonDocument::Compact)) return false;
    }
    if (plan.contains(QStringLiteral("planningClearanceReview"))) {
        if (constraintsValue.type() != QVariant::Map || constraintsValue.isNull()
            || !constraintsValue.toMap().contains(QStringLiteral("clearanceInput"))) return false;
        const QVariant value = plan.value(QStringLiteral("planningClearanceReview"));
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap result = value.toMap();
        const QVariant ok = result.value(QStringLiteral("ok"));
        if (result.size() != 3 || !result.contains(QStringLiteral("ok")) || !result.contains(QStringLiteral("error"))
            || !result.contains(QStringLiteral("report")) || ok.type() != QVariant::Bool || ok.isNull()) return false;
        if (ok.toBool()) {
            // Rebuild only the bounded complete SOURCE projection, not the
            // expensive geometry calculation. This is content consistency,
            // not mathematical certification or restored flight permission.
            if (!FleetMissionClearanceReview::planningResultMatches(plan, result)) return false;
        } else {
            QString reason;
            if (!identifier(result.value(QStringLiteral("error")), &reason)
                || result.value(QStringLiteral("report")).type() != QVariant::Map
                || result.value(QStringLiteral("report")).isNull()
                || !result.value(QStringLiteral("report")).toMap().isEmpty()) return false;
        }
    }
    if (plan.contains(QStringLiteral("remainderPlanningSource"))) {
        const QVariant value = plan.value(QStringLiteral("remainderPlanningSource"));
        if (plan.contains(QStringLiteral("planningSource")) || value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap checked = FleetMissionRemainderPlanner::validateProvenance(value.toMap());
        if (!trueField(checked, QStringLiteral("ok"))
            || QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(checked.value(QStringLiteral("provenance"))).toJson(QJsonDocument::Compact)
            || !FleetMissionRemainderPlanner::matchesPlan(value.toMap(), plan)) return false;
        // These hashes preserve local planning consistency, not the native
        // Run's ownership, completion, approval, or flight authority.
    }
    if (plan.contains(QStringLiteral("groupRemainderPlanningSource"))) {
        const QVariant value = plan.value(QStringLiteral("groupRemainderPlanningSource"));
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap checked = FleetMissionGroupRemainderPlanner::validateProvenance(value.toMap());
        if (!trueField(checked, QStringLiteral("ok"))
            || QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(checked.value(QStringLiteral("provenance"))).toJson(QJsonDocument::Compact)
            || !FleetMissionGroupRemainderPlanner::matchesPlan(value.toMap(), plan)) return false;
        // Group provenance remains inert history, never a restored Run,
        // ownership, approval or an observed completion prefix.
    }
    if (plan.contains(QStringLiteral("reserveDispatchPlanningSource"))) {
        const QVariant value = plan.value(QStringLiteral("reserveDispatchPlanningSource"));
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap checked = FleetReserveDispatchPlanner::validateProvenance(value.toMap());
        if (!trueField(checked, QStringLiteral("ok"))
            || checked.value(QStringLiteral("provenance")).type() != QVariant::Map
            || checked.value(QStringLiteral("provenance")).isNull()
            || QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(checked.value(QStringLiteral("provenance"))).toJson(QJsonDocument::Compact)
            || !FleetReserveDispatchPlanner::matchesPlan(value.toMap(), plan)) return false;
        // The source binds retained values to this new single-member plan.
        // It never restores an old Run, approval, occupancy or flight permit.
    }
    if (plan.contains(QStringLiteral("planningSource"))) {
        const QVariant value = plan.value(QStringLiteral("planningSource"));
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap checked = FleetMissionDraft::validatePlanningSource(value.toMap());
        // Keep archived bytes/identity intact: validate, do not silently
        // repair an imported source. JSON normalizes native numeric widths.
        if (!trueField(checked, QStringLiteral("ok"))
            || QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(checked.value(QStringLiteral("source"))).toJson(QJsonDocument::Compact)) return false;
        if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
            const QVariantMap rebuilt = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
                checked.value(QStringLiteral("task")).toMap().value(QStringLiteral("area")),
                plan.value(QStringLiteral("members")).toList());
            if (!trueField(rebuilt, QStringLiteral("ok"))
                || QJsonDocument::fromVariant(plan.value(QStringLiteral("routeBoundaryPolicy"))).toJson(QJsonDocument::Compact)
                    != QJsonDocument::fromVariant(rebuilt.value(QStringLiteral("policy"))).toJson(QJsonDocument::Compact)) return false;
        }
        const QVariantMap sourceSettings = checked.value(QStringLiteral("source")).toMap()
            .value(QStringLiteral("settings")).toMap();
        const QVariantMap sourceTask = checked.value(QStringLiteral("task")).toMap();
        const bool sourcePerformanceProvided = sourceTask.contains(QStringLiteral("requirePerformanceEvidence"));
        const bool planPerformanceProvided = plan.contains(QStringLiteral("requirePerformanceEvidence"));
        if (sourcePerformanceProvided != planPerformanceProvided) return false;
        if (sourcePerformanceProvided) {
            const QVariant sourcePolicy = sourceTask.value(QStringLiteral("requirePerformanceEvidence"));
            const QVariant planPolicy = plan.value(QStringLiteral("requirePerformanceEvidence"));
            if (sourcePolicy.type() != QVariant::Bool || sourcePolicy.isNull()
                || planPolicy.type() != QVariant::Bool || planPolicy.isNull() || sourcePolicy != planPolicy) return false;
        }
        for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
            if (sourceTask.contains(key) != plan.contains(key)) return false;
            if (!sourceTask.contains(key)) continue;
            double sourceFloor = 0.0, planFloor = 0.0;
            if (!performanceFloor(sourceTask.value(key), &sourceFloor)
                || !performanceFloor(plan.value(key), &planFloor) || sourceFloor != planFloor) return false;
        }
        const bool sourceClearanceProvided = sourceSettings.contains(QStringLiteral("clearanceInput"));
        const bool planClearanceProvided = constraintsValue.type() == QVariant::Map && !constraintsValue.isNull()
            && constraintsValue.toMap().contains(QStringLiteral("clearanceInput"));
        if (sourceClearanceProvided || planClearanceProvided) {
            // A restored editor must not switch the archived engineering
            // scene/envelopes, even when both standalone trees are valid.
            if (sourceClearanceProvided != planClearanceProvided
                || QJsonDocument::fromVariant(sourceSettings.value(QStringLiteral("clearanceInput"))).toJson(QJsonDocument::Compact)
                    != QJsonDocument::fromVariant(constraintsValue.toMap().value(QStringLiteral("clearanceInput"))).toJson(QJsonDocument::Compact)) return false;
        }
    }
    const auto profileValid = [](const QVariantMap& source) {
        if (!source.contains(QStringLiteral("executionProfile"))) return true;
        const QVariant value = source.value(QStringLiteral("executionProfile"));
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(value.toMap());
        return trueField(normalized, QStringLiteral("ok"))
            && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact)
                == QJsonDocument::fromVariant(normalized.value(QStringLiteral("profile"))).toJson(QJsonDocument::Compact);
    };
    if (!profileValid(plan)) return false;
    for (const QString& key : {QStringLiteral("members"), QStringLiteral("assignments")}) {
        if (!plan.contains(key)) continue;
        if (plan.value(key).type() != QVariant::List) return false;
        for (const QVariant& member : plan.value(key).toList()) {
            if (member.type() != QVariant::Map || !profileValid(member.toMap())) return false;
        }
    }
    qint64 schema = 0;
    QString subject;
    return integer(plan.value(QStringLiteral("schemaVersion")), &schema) && schema == 1
        && identifier(plan.value(QStringLiteral("sourceTaskId")), &subject) && subject == taskId
        && digestText(plan.value(QStringLiteral("planId")))
        && plan.value(QStringLiteral("feasible")).type() == QVariant::Bool
        && trueField(plan, QStringLiteral("manualConfirmationRequired"))
        && trueField(plan, QStringLiteral("requiresOperatorReview"))
        && falseField(plan, QStringLiteral("flightCommandReleased"))
        && plan.value(QStringLiteral("dispatch")).type() == QVariant::String
        && plan.value(QStringLiteral("dispatch")).toString() == QStringLiteral("not-released");
}

bool airspaceInputEvidenceValid(const QVariantMap& evidence)
{
    // Optional for historical compatibility. Absence does not establish that
    // an old empty zone list was explicitly provided or loaded completely.
    if (!evidence.contains(QStringLiteral("airspaceInputEvidence"))) return true;
    const QVariant value = evidence.value(QStringLiteral("airspaceInputEvidence"));
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap input = value.toMap();
    const QStringList keys{QStringLiteral("schemaVersion"), QStringLiteral("noFlyZonesProvided"),
        QStringLiteral("noFlyZonesListValid"), QStringLiteral("noFlyZonesCount")};
    if (input.size() != keys.size()) return false;
    for (const QString& key : keys) if (!input.contains(key)) return false;
    qint64 schema = 0;
    if (!integer(input.value(QStringLiteral("schemaVersion")), &schema) || schema != 1
        || input.value(QStringLiteral("noFlyZonesProvided")).type() != QVariant::Bool
        || input.value(QStringLiteral("noFlyZonesProvided")).isNull()
        || input.value(QStringLiteral("noFlyZonesListValid")).type() != QVariant::Bool
        || input.value(QStringLiteral("noFlyZonesListValid")).isNull()) return false;
    const bool provided = input.value(QStringLiteral("noFlyZonesProvided")).toBool();
    const bool listValid = input.value(QStringLiteral("noFlyZonesListValid")).toBool();
    const QVariant countValue = input.value(QStringLiteral("noFlyZonesCount"));
    if (listValid) {
        qint64 count = 0;
        return provided && !countValue.isNull() && integer(countValue, &count)
            && evidence.value(QStringLiteral("noFlyZones")).type() == QVariant::List
            && count == evidence.value(QStringLiteral("noFlyZones")).toList().size();
    }
    if (countValue.isValid() && countValue.userType() != QMetaType::Nullptr) return false;
    if (provided) return true; // Explicitly invalid original input, not a valid empty list.
    return !evidence.contains(QStringLiteral("noFlyZones"))
        || (evidence.value(QStringLiteral("noFlyZones")).type() == QVariant::List
            && evidence.value(QStringLiteral("noFlyZones")).toList().isEmpty());
}

bool validEvidence(const QVariantMap& evidence, QByteArray* encoded)
{
    return snapshot(evidence, encoded)
        && airspaceInputEvidenceValid(evidence)
        && evidence.value(QStringLiteral("needsReview")).type() == QVariant::Bool
        && evidence.value(QStringLiteral("reviewStatus")).type() == QVariant::String
        && trueField(evidence, QStringLiteral("manualConfirmationRequired"))
        && falseField(evidence, QStringLiteral("flightCommandReleased"));
}

QVariantMap historyReadRejected(const QString& error)
{
    // Deliberately not FleetMissionPlanStore::reject(): a read must not
    // modify lastError, emit archiveChanged, or clear a write quarantine.
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error},
            {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

bool historicalRecordBinding(const QVariantMap& record, const QString& task,
                             const QString& epoch, QVariantMap* binding)
{
    QString actualTask, recordId;
    qint64 revision = 0;
    const QVariant timestamp = record.value(QStringLiteral("recordedAtUtc"));
    if (record.size() != 9 || !identifier(record.value(QStringLiteral("sourceTaskId")), &actualTask)
        || actualTask != task || !identifier(record.value(QStringLiteral("recordId")), &recordId)
        || QString::fromUtf8(actualTask.toUtf8()) != actualTask || QString::fromUtf8(recordId.toUtf8()) != recordId
        || !integer(record.value(QStringLiteral("recordRevision")), &revision) || revision <= 0
        || !digestText(record.value(QStringLiteral("planId")))
        || !digestText(record.value(QStringLiteral("contentHash")))
        || !digestText(record.value(QStringLiteral("reviewEvidenceHash")))
        || timestamp.type() != QVariant::String || timestamp.toString().size() > 32
        || record.value(QStringLiteral("plan")).type() != QVariant::Map
        || record.value(QStringLiteral("reviewEvidence")).type() != QVariant::Map) return false;
    const QDateTime recorded = QDateTime::fromString(timestamp.toString(), Qt::ISODateWithMs);
    if (!recorded.isValid() || recorded.toUTC().toString(Qt::ISODateWithMs) != timestamp.toString()) return false;
    *binding = {{QStringLiteral("storeEpoch"), epoch}, {QStringLiteral("sourceTaskId"), actualTask},
                {QStringLiteral("recordId"), recordId}, {QStringLiteral("recordRevision"), revision},
                {QStringLiteral("planId"), record.value(QStringLiteral("planId"))},
                {QStringLiteral("contentHash"), record.value(QStringLiteral("contentHash"))},
                {QStringLiteral("reviewEvidenceHash"), record.value(QStringLiteral("reviewEvidenceHash"))}};
    return true;
}

bool sameFilePath(const QString& left, const QString& right)
{
    if (left.isEmpty() || right.isEmpty()) return false;
    const QFileInfo a(left), b(right);
    const QString first = a.canonicalFilePath().isEmpty() ? a.absoluteFilePath() : a.canonicalFilePath();
    const QString second = b.canonicalFilePath().isEmpty() ? b.absoluteFilePath() : b.canonicalFilePath();
#ifdef Q_OS_WIN
    return QDir::cleanPath(first).compare(QDir::cleanPath(second), Qt::CaseInsensitive) == 0;
#else
    return QDir::cleanPath(first) == QDir::cleanPath(second);
#endif
}

} // namespace

FleetMissionPlanStore::FleetMissionPlanStore(QObject* parent)
    : QObject(parent), _storeEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!base.isEmpty()) _archivePath = QDir(base).filePath(QStringLiteral("fleet-os/mission-plans-v1.json"));
    if (!_archivePath.isEmpty() && QFileInfo::exists(_archivePath)) loadFromFile(_archivePath);
}

QVariantMap FleetMissionPlanStore::summary(const QVariantMap& recordValue) const
{
    QVariantMap result = recordValue;
    result.remove(QStringLiteral("plan"));
    result.remove(QStringLiteral("reviewEvidence"));
    const QString id = recordValue.value(QStringLiteral("recordId")).toString();
    result.insert(QStringLiteral("restoredFromArchive"), _restoredRecordIds.contains(id));
    result.insert(QStringLiteral("liveReview"), _liveReviewIds.value(recordValue.value(QStringLiteral("sourceTaskId")).toString()) == id);
    result.insert(QStringLiteral("storeEpoch"), _storeEpoch);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("localReviewOnly"), true);
    return result;
}

QVariantList FleetMissionPlanStore::summaries() const
{
    QVariantList result;
    for (auto it = _versions.constBegin(); it != _versions.constEnd(); ++it) {
        if (!it.value().isEmpty()) result.append(summary(it.value().last().toMap()));
    }
    return result;
}

QVariantMap FleetMissionPlanStore::current(const QString& sourceTaskId) const
{
    const QVariantList versions = _versions.value(sourceTaskId);
    return versions.isEmpty() ? QVariantMap() : summary(versions.last().toMap());
}

QVariantMap FleetMissionPlanStore::record(const QString& sourceTaskId, const QVariant& revision) const
{
    qint64 requested = 0;
    if (!integer(revision, &requested)) return QVariantMap();
    const QVariantList versions = _versions.value(sourceTaskId);
    for (int i = versions.size() - 1; i >= 0; --i) {
        const QVariantMap value = versions.at(i).toMap();
        if (requested != 0 && value.value(QStringLiteral("recordRevision")).toLongLong() != requested) continue;
        QVariantMap result = value;
        const QVariantMap flags = summary(value);
        for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) result.insert(it.key(), it.value());
        return result;
    }
    return QVariantMap();
}

QVariantMap FleetMissionPlanStore::historyVersions(const QString& sourceTaskId,
                                                  const QString& expectedStoreEpoch) const
{
    if (QThread::currentThread() != thread()) return historyReadRejected(QStringLiteral("history_wrong_thread"));
    QString task;
    if (!identifier(sourceTaskId, &task)) return historyReadRejected(QStringLiteral("history_invalid_selection"));
    if (expectedStoreEpoch.isEmpty() || expectedStoreEpoch != _storeEpoch)
        return historyReadRejected(QStringLiteral("history_stale_store_epoch"));
    const auto found = _versions.constFind(task);
    if (found == _versions.constEnd() || found.value().isEmpty())
        return historyReadRejected(QStringLiteral("history_source_not_found"));
    const QVariantList& versions = found.value();
    if (versions.size() > MaxVersionsPerTask)
        return historyReadRejected(QStringLiteral("history_directory_budget_exceeded"));
    QVariantList rows;
    QVariantMap result = {{QStringLiteral("ok"), true}, {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("storeEpoch"), _storeEpoch}, {QStringLiteral("sourceTaskId"), task},
        {QStringLiteral("versions"), rows}, {QStringLiteral("versionCount"), versions.size()},
        {QStringLiteral("allVersionsForSource"), true}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    // Only bounded scalar metadata is serialized. Count the exact compact
    // envelope plus each validated row before appending; never return a
    // partial version list or serialize all historical plans for a catalog.
    qint64 bytes = QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact).size();
    for (int i = versions.size() - 1; i >= 0; --i) {
        if (versions.at(i).type() != QVariant::Map)
            return historyReadRejected(QStringLiteral("history_invalid_record"));
        const QVariantMap value = versions.at(i).toMap();
        QVariantMap row;
        if (!historicalRecordBinding(value, task, _storeEpoch, &row))
            return historyReadRejected(QStringLiteral("history_invalid_record"));
        row.insert(QStringLiteral("recordedAtUtc"), value.value(QStringLiteral("recordedAtUtc")));
        row.insert(QStringLiteral("restoredFromArchive"), _restoredRecordIds.contains(row.value(QStringLiteral("recordId")).toString()));
        // Presence is not validity or readiness; the source getter performs
        // the full plan/evidence/source checks on the exact selected record.
        row.insert(QStringLiteral("planningSourcePresent"), value.value(QStringLiteral("plan")).toMap().contains(QStringLiteral("planningSource")));
        const qint64 rowBytes = QJsonDocument::fromVariant(row).toJson(QJsonDocument::Compact).size();
        bytes += rowBytes + (rows.isEmpty() ? 0 : 1);
        if (bytes > MaxHistoryDirectoryBytes)
            return historyReadRejected(QStringLiteral("history_directory_budget_exceeded"));
        rows.append(row);
    }
    result.insert(QStringLiteral("versions"), rows);
    return result;
}

QVariantMap FleetMissionPlanStore::historicalDraftSource(const QVariantMap& expectedBinding) const
{
    if (QThread::currentThread() != thread()) return historyReadRejected(QStringLiteral("history_wrong_thread"));
    QString task, recordId;
    qint64 revision = 0;
    if (expectedBinding.size() != 7
        || !identifier(expectedBinding.value(QStringLiteral("sourceTaskId")), &task)
        || !identifier(expectedBinding.value(QStringLiteral("recordId")), &recordId)
        || !integer(expectedBinding.value(QStringLiteral("recordRevision")), &revision) || revision <= 0
        || expectedBinding.value(QStringLiteral("storeEpoch")).type() != QVariant::String
        || !digestText(expectedBinding.value(QStringLiteral("planId")))
        || !digestText(expectedBinding.value(QStringLiteral("contentHash")))
        || !digestText(expectedBinding.value(QStringLiteral("reviewEvidenceHash"))))
        return historyReadRejected(QStringLiteral("history_invalid_selection"));
    if (expectedBinding.value(QStringLiteral("storeEpoch")).toString() != _storeEpoch)
        return historyReadRejected(QStringLiteral("history_stale_store_epoch"));
    const auto found = _versions.constFind(task);
    if (found == _versions.constEnd()) return historyReadRejected(QStringLiteral("history_source_not_found"));
    if (found.value().size() > MaxVersionsPerTask)
        return historyReadRejected(QStringLiteral("history_directory_budget_exceeded"));
    for (const QVariant& entry : found.value()) {
        if (entry.type() != QVariant::Map) return historyReadRejected(QStringLiteral("history_invalid_record"));
        const QVariantMap value = entry.toMap();
        QVariantMap binding;
        if (!historicalRecordBinding(value, task, _storeEpoch, &binding))
            return historyReadRejected(QStringLiteral("history_invalid_record"));
        if (binding.value(QStringLiteral("recordRevision")).toLongLong() != revision) continue;
        for (const QString& key : {QStringLiteral("recordId"), QStringLiteral("planId"), QStringLiteral("contentHash"),
                                  QStringLiteral("reviewEvidenceHash")}) {
            if (expectedBinding.value(key) != binding.value(key))
                return historyReadRejected(QStringLiteral("history_stale_record_binding"));
        }
        const QVariantMap plan = value.value(QStringLiteral("plan")).toMap();
        if (!plan.contains(QStringLiteral("planningSource")))
            return historyReadRejected(QStringLiteral("history_planning_source_not_recorded"));
        QByteArray encodedPlan, encodedEvidence;
        if (!validPlan(task, plan, &encodedPlan)
            || !validEvidence(value.value(QStringLiteral("reviewEvidence")).toMap(), &encodedEvidence)
            || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId"))
            || hash(encodedPlan) != binding.value(QStringLiteral("contentHash")).toString()
            || hash(encodedEvidence) != binding.value(QStringLiteral("reviewEvidenceHash")).toString())
            return historyReadRejected(QStringLiteral("history_invalid_record_integrity"));
        const QVariantMap checked = FleetMissionDraft::validatePlanningSource(plan.value(QStringLiteral("planningSource")).toMap());
        if (!trueField(checked, QStringLiteral("ok"))
            || checked.value(QStringLiteral("source")).type() != QVariant::Map)
            return historyReadRejected(QStringLiteral("history_invalid_planning_source"));
        // This is a value copy for a new draft, never a new live review or
        // current-onboard proof. Appending other versions does not invalidate
        // this immutable record; replacing/importing the store changes epoch.
        return {{QStringLiteral("ok"), true}, {QStringLiteral("schemaVersion"), 1},
                {QStringLiteral("binding"), binding}, {QStringLiteral("planningSource"), checked.value(QStringLiteral("source"))},
                {QStringLiteral("restoredFromArchive"), _restoredRecordIds.contains(recordId)},
                {QStringLiteral("historicalOnly"), true}, {QStringLiteral("manualConfirmationRequired"), true},
                {QStringLiteral("approvalRestored"), false}, {QStringLiteral("flightCommandReleased"), false},
                {QStringLiteral("executable"), false}};
    }
    return historyReadRejected(QStringLiteral("history_record_not_found"));
}

QVariantMap FleetMissionPlanStore::reject(const QString& reason)
{
    _lastError = reason;
    emit archiveChanged();
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("error"), reason);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    return result;
}

QByteArray FleetMissionPlanStore::encode(const Versions& versions)
{
    QVariantList records;
    for (auto it = versions.constBegin(); it != versions.constEnd(); ++it) records += it.value();
    QVariantMap root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("kind"), QStringLiteral("local-mission-review-history"));
    root.insert(QStringLiteral("records"), records);
    return QJsonDocument::fromVariant(root).toJson(QJsonDocument::Compact);
}

bool FleetMissionPlanStore::writeArchive(const QString& filePath, const QByteArray& bytes)
{
    const QFileInfo info(filePath);
    if (filePath.trimmed().isEmpty() || !info.isAbsolute() || bytes.isEmpty() || bytes.size() > MaxArchiveBytes) return false;
    const QDir parent(info.absolutePath());
    if (!parent.exists() && !QDir().mkpath(parent.absolutePath())) return false;
    QSaveFile file(filePath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) return false;
    return file.commit();
}

bool FleetMissionPlanStore::persistDefault(const QByteArray& bytes, QString* error)
{
    const auto fail = [this, error](const QString& code) {
        if (code == QStringLiteral("archive_conflict_reload_required")) _archiveWriteQuarantined = true;
        if (error) *error = code;
        return false;
    };
    if (writeBlocked()) return fail(QStringLiteral("archive_quarantined_or_storage_unavailable"));
    const QDir parent(QFileInfo(_archivePath).absolutePath());
    if (!parent.exists() && !QDir().mkpath(parent.absolutePath())) return fail(QStringLiteral("archive_write_failed"));
    QLockFile lock(_archivePath + QStringLiteral(".lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) return fail(QStringLiteral("archive_busy"));
    // All cooperating local writers compare the exact baseline under the
    // same lock. Atomic replacement alone would silently lose another
    // controller's updates; this is not a distributed control lease.
    const bool exists = QFileInfo::exists(_archivePath);
    if (exists != _archiveBaselineExists) return fail(QStringLiteral("archive_conflict_reload_required"));
    if (exists) {
        QFile disk(_archivePath);
        if (!disk.open(QIODevice::ReadOnly)) return fail(QStringLiteral("archive_read_failed"));
        const qint64 size = disk.size();
        if (size < 0 || size > MaxArchiveBytes) return fail(QStringLiteral("archive_conflict_reload_required"));
        const QByteArray currentBytes = disk.read(MaxArchiveBytes + 1);
        if (disk.error() != QFileDevice::NoError || currentBytes.size() != size || disk.size() != size
            || hash(currentBytes) != _archiveBaselineHash) return fail(QStringLiteral("archive_conflict_reload_required"));
    }
    if (!writeArchive(_archivePath, bytes)) return fail(QStringLiteral("archive_write_failed"));
    _archiveBaselineExists = true;
    _archiveBaselineHash = hash(bytes);
    return true;
}

QVariantMap FleetMissionPlanStore::recordReview(const QString& sourceTaskId, const QVariantMap& plan,
                                                const QVariantMap& reviewEvidence, const QVariant& expectedRevision)
{
    QString task;
    qint64 expected = 0;
    QByteArray encodedPlan, encodedEvidence;
    if (!identifier(sourceTaskId, &task) || !integer(expectedRevision, &expected)
        || !validPlan(task, plan, &encodedPlan) || !validEvidence(reviewEvidence, &encodedEvidence)) {
        return reject(QStringLiteral("invalid_plan_or_review"));
    }
    const QVariantList previous = _versions.value(task);
    const qint64 actual = previous.isEmpty() ? 0 : previous.last().toMap().value(QStringLiteral("recordRevision")).toLongLong();
    if (actual != expected || actual >= static_cast<qint64>(MaxExactRevision)) return reject(QStringLiteral("stale_record_revision"));
    int recordCount = 0;
    for (auto it = _versions.constBegin(); it != _versions.constEnd(); ++it) recordCount += it.value().size();
    if ((!_versions.contains(task) && _versions.size() >= MaxTasks)
        || previous.size() >= MaxVersionsPerTask || recordCount >= MaxRecords) return reject(QStringLiteral("archive_record_budget_exceeded"));
    QVariantMap value;
    value.insert(QStringLiteral("sourceTaskId"), task);
    value.insert(QStringLiteral("planId"), plan.value(QStringLiteral("planId")));
    value.insert(QStringLiteral("recordId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    value.insert(QStringLiteral("recordRevision"), actual + 1);
    value.insert(QStringLiteral("recordedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    value.insert(QStringLiteral("contentHash"), hash(encodedPlan));
    value.insert(QStringLiteral("reviewEvidenceHash"), hash(encodedEvidence));
    value.insert(QStringLiteral("plan"), plan);
    value.insert(QStringLiteral("reviewEvidence"), reviewEvidence);
    Versions staged = _versions;
    staged[task].append(value);
    const QByteArray archive = encode(staged);
    if (archive.size() > MaxArchiveBytes) return reject(QStringLiteral("archive_byte_budget_exceeded"));
    // Persistence succeeds before the live model/binding is changed.
    QString persistError;
    if (!persistDefault(archive, &persistError)) return reject(persistError);
    _versions = staged;
    _liveReviewIds.insert(task, value.value(QStringLiteral("recordId")).toString());
    _lastError.clear();
    emit archiveChanged();
    QVariantMap result = summary(value);
    result.insert(QStringLiteral("ok"), true);
    return result;
}

bool FleetMissionPlanStore::matchesBinding(const QVariantMap& binding) const
{
    QString task;
    qint64 expected = 0;
    if (!identifier(binding.value(QStringLiteral("sourceTaskId")), &task)
        || !integer(binding.value(QStringLiteral("recordRevision")), &expected) || expected == 0) return false;
    const QVariantMap actual = current(task);
    if (actual.isEmpty() || !trueField(actual, QStringLiteral("liveReview"))
        || !falseField(actual, QStringLiteral("restoredFromArchive"))) return false;
    for (const QString& key : {QStringLiteral("recordId"), QStringLiteral("planId"), QStringLiteral("contentHash"),
                              QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")}) {
        if (binding.value(key).type() != QVariant::String || binding.value(key) != actual.value(key)) return false;
    }
    return expected == actual.value(QStringLiteral("recordRevision")).toLongLong();
}

bool FleetMissionPlanStore::revokeReview(const QVariantMap& binding)
{
    // Revision alone can recur after restoring an older history. Revoke the
    // exact record and live epoch, never a numerically matching replacement.
    if (!matchesBinding(binding)) return false;
    const QString sourceTaskId = binding.value(QStringLiteral("sourceTaskId")).toString();
    _liveReviewIds.remove(sourceTaskId);
    emit archiveChanged();
    return true;
}

QString FleetMissionPlanStore::exportJson() const { return QString::fromUtf8(encode(_versions)); }

bool FleetMissionPlanStore::importJson(const QString& json)
{
    return importInternal(json, false, QByteArray());
}

bool FleetMissionPlanStore::importInternal(const QString& json, bool defaultArchive, const QByteArray& defaultBytes)
{
    const auto fail = [this](const QString& error) { reject(error); return false; };
    if (json.size() > MaxArchiveBytes || json.toUtf8().size() > MaxArchiveBytes) return fail(QStringLiteral("archive_byte_budget_exceeded"));
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return fail(QStringLiteral("invalid_archive_json"));
    const QVariantMap root = document.toVariant().toMap();
    qint64 schema = 0;
    if (root.size() != 3 || !integer(root.value(QStringLiteral("schemaVersion")), &schema) || schema != 1
        || root.value(QStringLiteral("kind")).type() != QVariant::String
        || root.value(QStringLiteral("kind")).toString() != QStringLiteral("local-mission-review-history")
        || root.value(QStringLiteral("records")).type() != QVariant::List) return fail(QStringLiteral("invalid_archive_schema"));
    const QVariantList rows = root.value(QStringLiteral("records")).toList();
    if (rows.size() > MaxRecords) return fail(QStringLiteral("archive_record_budget_exceeded"));
    Versions staged;
    QSet<QString> recordIds;
    for (const QVariant& row : rows) {
        if (row.type() != QVariant::Map) return fail(QStringLiteral("invalid_archive_record"));
        const QVariantMap value = row.toMap();
        QString task, recordId;
        qint64 revision = 0;
        QByteArray encodedPlan, encodedEvidence;
        const QDateTime recorded = QDateTime::fromString(value.value(QStringLiteral("recordedAtUtc")).toString(), Qt::ISODateWithMs);
        if (value.size() != 9 || !identifier(value.value(QStringLiteral("sourceTaskId")), &task)
            || !identifier(value.value(QStringLiteral("recordId")), &recordId) || recordIds.contains(recordId)
            || !integer(value.value(QStringLiteral("recordRevision")), &revision) || revision <= 0
            || value.value(QStringLiteral("recordedAtUtc")).type() != QVariant::String || !recorded.isValid()
            || recorded.toUTC().toString(Qt::ISODateWithMs) != value.value(QStringLiteral("recordedAtUtc")).toString()
            || value.value(QStringLiteral("plan")).type() != QVariant::Map
            || value.value(QStringLiteral("reviewEvidence")).type() != QVariant::Map
            || !validPlan(task, value.value(QStringLiteral("plan")).toMap(), &encodedPlan)
            || !validEvidence(value.value(QStringLiteral("reviewEvidence")).toMap(), &encodedEvidence)
            || !digestText(value.value(QStringLiteral("contentHash"))) || !digestText(value.value(QStringLiteral("reviewEvidenceHash")))
            || value.value(QStringLiteral("contentHash")).toString() != hash(encodedPlan)
            || value.value(QStringLiteral("reviewEvidenceHash")).toString() != hash(encodedEvidence)
            || value.value(QStringLiteral("planId")) != value.value(QStringLiteral("plan")).toMap().value(QStringLiteral("planId"))) {
            return fail(QStringLiteral("invalid_archive_record"));
        }
        if ((!staged.contains(task) && staged.size() >= MaxTasks) || staged.value(task).size() >= MaxVersionsPerTask) return fail(QStringLiteral("archive_record_budget_exceeded"));
        const qint64 previous = staged.value(task).isEmpty() ? 0 : staged.value(task).last().toMap().value(QStringLiteral("recordRevision")).toLongLong();
        if (revision != previous + 1) return fail(QStringLiteral("archive_revision_sequence_invalid"));
        staged[task].append(value);
        recordIds.insert(recordId);
    }
    _versions = staged;
    _liveReviewIds.clear();
    _restoredRecordIds = recordIds;
    _storeEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Arbitrary backend imports are history views, not authorization to
    // overwrite the default archive. Reload that exact archive before write.
    _archiveWriteQuarantined = !defaultArchive;
    if (defaultArchive) {
        _archiveBaselineExists = true;
        _archiveBaselineHash = hash(defaultBytes);
    }
    _lastError.clear();
    emit archiveChanged();
    return true;
}

bool FleetMissionPlanStore::saveToFile(const QString& filePath) const
{
    // Export must not bypass the default archive's lock/baseline checks.
    if (sameFilePath(filePath, _archivePath)) return false;
    return writeArchive(filePath, encode(_versions));
}

bool FleetMissionPlanStore::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    const bool defaultArchive = sameFilePath(filePath, _archivePath);
    if (filePath.trimmed().isEmpty() || !QFileInfo(filePath).isAbsolute() || !file.open(QIODevice::ReadOnly)
        || file.size() < 0 || file.size() > MaxArchiveBytes) {
        if (defaultArchive) _archiveWriteQuarantined = true;
        reject(QStringLiteral("archive_read_failed")); return false;
    }
    const qint64 expectedSize = file.size();
    const QByteArray bytes = file.read(MaxArchiveBytes + 1);
    const QString decoded = QString::fromUtf8(bytes);
    if (file.error() != QFileDevice::NoError || bytes.size() > MaxArchiveBytes
        || file.size() != expectedSize || bytes.size() != expectedSize || decoded.toUtf8() != bytes) {
        if (defaultArchive) _archiveWriteQuarantined = true;
        reject(QStringLiteral("archive_read_failed")); return false;
    }
    if (defaultArchive) _archiveWriteQuarantined = true;
    // Commit model, epoch, quarantine and baseline together before notifying
    // direct slots; an outer load must not overwrite a nested import's state.
    return importInternal(decoded, defaultArchive, bytes);
}

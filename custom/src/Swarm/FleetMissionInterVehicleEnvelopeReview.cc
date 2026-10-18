#include "FleetMissionInterVehicleEnvelopeReview.h"

#include "FleetMissionPlanningWorker.h"

#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Verdict = Geometry::Verdict;
constexpr int MaximumComponents = 131072;
constexpr int MaximumWork = 2000000;
constexpr int MaximumFragments = 2048;
constexpr int MaximumDetails = 512;
constexpr int MaximumOutputBytes = 2 * 1024 * 1024;
constexpr qint64 CoordinateBound = 51000000;
constexpr qint64 NormalBound = 100000000;
constexpr qint64 LimitBound = Q_INT64_C(20000000000000000);
const QString ResourceReason = QStringLiteral("inter-vehicle-envelope-comparison-resource-limit");
const QString UnknownReason = QStringLiteral("inter-vehicle-envelope-comparison-unknown");

bool cancelled(const Geometry::CancellationToken& value)
{
    return value && value->load(std::memory_order_acquire);
}
bool charge(Geometry::Budget* budget)
{
    if (!budget) return true; // Structural matcher: bounded traversal, no solver.
    if (budget->cancelled || cancelled(budget->cancellation)) { budget->cancelled = true; return false; }
    if (budget->exhausted || budget->remainingWork <= 0) { budget->exhausted = true; return false; }
    --budget->remainingWork;
    return true;
}
bool mapValue(const QVariant& value) { return value.userType() == QMetaType::QVariantMap && !value.isNull(); }
bool listValue(const QVariant& value) { return value.userType() == QMetaType::QVariantList && !value.isNull(); }
bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}
bool integer(const QVariant& value, qint64 minimum, qint64 maximum)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= double(minimum) && number <= double(maximum) && std::floor(number) == number;
}
bool flag(const QVariantMap& value, const char* key, bool expected)
{
    const QVariant item = value.value(QString::fromLatin1(key));
    return item.userType() == QMetaType::Bool && !item.isNull() && item.toBool() == expected;
}
bool boolean(const QVariantMap& value, const char* key)
{
    const QVariant item = value.value(QString::fromLatin1(key));
    return item.userType() == QMetaType::Bool && !item.isNull();
}
bool digest(const QString& value)
{
    if (value.size() != 64) return false;
    for (QChar ch : value) if (!(ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        && !(ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))) return false;
    return true;
}
QByteArray json(const QVariant& value) { return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact); }
QVariantMap failure(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reason}, {QStringLiteral("report"), QVariantMap()}};
}
QVariantMap limits()
{
    return {{QStringLiteral("maximumComponents"), MaximumComponents}, {QStringLiteral("maximumWorkUnits"), MaximumWork},
        {QStringLiteral("maximumFragments"), MaximumFragments}, {QStringLiteral("maximumConflictDetails"), MaximumDetails},
        {QStringLiteral("maximumOutputBytes"), MaximumOutputBytes}};
}
QStringList reasons(bool geometryComplete, bool comparisonComplete, bool resource, qint64 conflicts)
{
    QStringList output{QStringLiteral("unsynchronized-all-space-declared-envelope-comparison-not-actual-path-proof")};
    if (!geometryComplete) output.append(QStringLiteral("inter-vehicle-envelope-geometry-incomplete"));
    else if (!comparisonComplete) output.append(resource ? ResourceReason : UnknownReason);
    if (conflicts) output.append(QStringLiteral("inter-vehicle-envelope-overlap-candidates"));
    if (conflicts > MaximumDetails) output.append(QStringLiteral("inter-vehicle-envelope-conflict-details-truncated"));
    return output;
}
bool inRange(qint64 value, qint64 bound) { return value >= -bound && value <= bound; }
bool polytopeValid(const Geometry::Polytope& value, Geometry::Budget* budget)
{
    if (!charge(budget)) return false;
    const auto& b = value.bounds;
    if (!inRange(b.minimumEast, CoordinateBound) || !inRange(b.maximumEast, CoordinateBound)
        || !inRange(b.minimumNorth, CoordinateBound) || !inRange(b.maximumNorth, CoordinateBound)
        || !inRange(b.minimumAltitude, CoordinateBound) || !inRange(b.maximumAltitude, CoordinateBound)
        || b.minimumEast > b.maximumEast || b.minimumNorth > b.maximumNorth || b.minimumAltitude > b.maximumAltitude
        || value.planes.size() > 64) return false;
    for (const auto& plane : value.planes) {
        if (!charge(budget) || (!plane.east && !plane.north && !plane.altitude)
            || !inRange(plane.east, NormalBound) || !inRange(plane.north, NormalBound)
            || !inRange(plane.altitude, NormalBound) || !inRange(plane.limit, LimitBound)) return false;
    }
    return true;
}
struct ComponentView {
    int vehicleId = 0, member = 0, position = 0, index = 0;
    QString source, kind;
    const QVariantMap* reference = nullptr;
    const Geometry::Polytope* envelope = nullptr;
};
QVariantMap detail(const ComponentView& a, const ComponentView& b)
{
    return {{QStringLiteral("vehicleA"), a.vehicleId}, {QStringLiteral("sourceA"), a.source},
        {QStringLiteral("componentIndexA"), a.index}, {QStringLiteral("componentKindA"), a.kind}, {QStringLiteral("referenceA"), *a.reference},
        {QStringLiteral("vehicleB"), b.vehicleId}, {QStringLiteral("sourceB"), b.source},
        {QStringLiteral("componentIndexB"), b.index}, {QStringLiteral("componentKindB"), b.kind}, {QStringLiteral("referenceB"), *b.reference}};
}
Verdict intersects(const Geometry::Polytope& a, const Geometry::Polytope& b, Geometry::Budget* budget)
{
    if (!charge(budget)) return Verdict::Unknown;
    Geometry::Polytope intersection;
    auto& bounds = intersection.bounds;
    bounds.minimumEast = std::max(a.bounds.minimumEast, b.bounds.minimumEast);
    bounds.maximumEast = std::min(a.bounds.maximumEast, b.bounds.maximumEast);
    bounds.minimumNorth = std::max(a.bounds.minimumNorth, b.bounds.minimumNorth);
    bounds.maximumNorth = std::min(a.bounds.maximumNorth, b.bounds.maximumNorth);
    bounds.minimumAltitude = std::max(a.bounds.minimumAltitude, b.bounds.minimumAltitude);
    bounds.maximumAltitude = std::min(a.bounds.maximumAltitude, b.bounds.maximumAltitude);
    // STRICT separation only. Equality retains closed point/line/face contact.
    if (bounds.minimumEast > bounds.maximumEast || bounds.minimumNorth > bounds.maximumNorth
        || bounds.minimumAltitude > bounds.maximumAltitude) return Verdict::ProvenFalse;
    if (a.planes.size() + b.planes.size() > 64) return Verdict::Unknown;
    intersection.planes.reserve(a.planes.size() + b.planes.size());
    for (const auto* source : {&a, &b}) {
        for (const auto& plane : source->planes) {
            if (!charge(budget)) return Verdict::Unknown;
            intersection.planes.append(plane); // Preserve normal, limit AND strictness.
        }
    }
    // This is the sole exact narrow phase. Never replace an overlapping
    // AABB by a positive verdict or discard either operand's extra planes.
    return Geometry::feasible(intersection, budget);
}
template<class Input>
QVariantMap makeReport(const Input& input, bool comparisonComplete, qint64 processed, qint64 conflicts,
                       const QVariantList& details, int work, bool resource)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-inter-vehicle-envelope-review")},
        {QStringLiteral("geometryBasis"), input.geometryBasis}, {QStringLiteral("clearanceInputFingerprint"), input.inputFingerprint},
        {QStringLiteral("domainModelsFingerprint"), input.domainFingerprint}, {QStringLiteral("waypointModelsFingerprint"), input.waypointFingerprint},
        {QStringLiteral("memberIds"), input.memberIds}, {QStringLiteral("memberCount"), input.memberIds.size()},
        {QStringLiteral("domainComponentCount"), input.domainCount}, {QStringLiteral("waypointComponentCount"), input.waypointCount},
        {QStringLiteral("componentCount"), input.components.size()}, {QStringLiteral("geometryComplete"), input.geometryComplete},
        {QStringLiteral("pairCount"), input.pairs}, {QStringLiteral("processedPairCount"), processed},
        {QStringLiteral("comparisonComplete"), comparisonComplete},
        {QStringLiteral("declaredModelSatisfied"), input.geometryComplete && comparisonComplete && !conflicts},
        {QStringLiteral("conflictCount"), conflicts}, {QStringLiteral("conflicts"), details},
        {QStringLiteral("conflictsTruncated"), conflicts > MaximumDetails},
        {QStringLiteral("reasonCodes"), reasons(input.geometryComplete, comparisonComplete, resource, conflicts)},
        {QStringLiteral("workUnits"), work}, {QStringLiteral("limits"), limits()}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("timingKnown"), false}, {QStringLiteral("actualPathCertified"), false}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap success(const QVariantMap& report, const Geometry::CancellationToken& cancellation)
{
    if (cancelled(cancellation)) return failure(QStringLiteral("inter-vehicle-envelope-review-cancelled"));
    QVariantMap detached;
    const QVariantMap value{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("report"), report}};
    if (!FleetMissionPlanningWorker::snapshot(value, &detached) || json(detached).size() > MaximumOutputBytes)
        return failure(QStringLiteral("inter-vehicle-envelope-output-resource-or-value-invalid"));
    return cancelled(cancellation) ? failure(QStringLiteral("inter-vehicle-envelope-review-cancelled")) : detached;
}
template<class Input>
qint64 pairOrdinal(const Input& input, const ComponentView& a, const ComponentView& b)
{
    qint64 ordinal = 0;
    for (int first = 0; first < input.byMember.size(); ++first) {
        for (int second = first + 1; second < input.byMember.size(); ++second) {
            if (first == a.member && second == b.member)
                return ordinal + qint64(a.position) * qint64(input.byMember.at(second).size()) + b.position;
            ordinal += qint64(input.byMember.at(first).size()) * qint64(input.byMember.at(second).size());
        }
    }
    return -1;
}
} // namespace

struct FleetMissionInterVehicleEnvelopeReview::Inputs {
    QString geometryBasis, inputFingerprint, domainFingerprint, waypointFingerprint;
    QVariantList memberIds;
    int domainCount = 0, waypointCount = 0;
    bool geometryComplete = false;
    qint64 pairs = 0;
    qint64 validationWork = 0;
    QVector<ComponentView> components;
    QVector<QVector<int>> byMember;
};

bool FleetMissionInterVehicleEnvelopeReview::_inputs(const PreparedPtr& domain, const PreparedPtr& waypoint,
    Inputs* output, Geometry::Budget* budget)
{
    if (!output) return false;
    *output = Inputs();
    if (!domain || !waypoint || domain->source != QStringLiteral("domain") || waypoint->source != QStringLiteral("waypoint")
        || !digest(domain->geometryBasis) || domain->geometryBasis != waypoint->geometryBasis
        || !digest(domain->inputFingerprint) || domain->inputFingerprint != waypoint->inputFingerprint
        || !digest(domain->modelsFingerprint) || !digest(waypoint->modelsFingerprint)
        || domain->motionFingerprint != domain->modelsFingerprint || waypoint->motionFingerprint != domain->modelsFingerprint
        || domain->memberIds.isEmpty() || domain->memberIds.size() > 32 || domain->memberIds != waypoint->memberIds
        || domain->components.size() > 65536 || waypoint->components.size() > 65536
        || (!domain->geometryComplete && !domain->components.isEmpty())
        || (!waypoint->geometryComplete && !waypoint->components.isEmpty())) return false;
    Inputs result;
    result.geometryBasis = domain->geometryBasis; result.inputFingerprint = domain->inputFingerprint;
    result.domainFingerprint = domain->modelsFingerprint; result.waypointFingerprint = waypoint->modelsFingerprint;
    result.memberIds = domain->memberIds; result.geometryComplete = domain->geometryComplete && waypoint->geometryComplete;
    QMap<int, int> members;
    for (const QVariant& value : result.memberIds) {
        if (!charge(budget) || !integer(value, 1, 255) || members.contains(value.toInt())) return false;
        members.insert(value.toInt(), members.size());
        result.byMember.append(QVector<int>());
        ++result.validationWork;
    }
    result.domainCount = domain->components.size(); result.waypointCount = waypoint->components.size();
    result.components.reserve(result.domainCount + result.waypointCount);
    const QSet<QString> waypointKinds{QStringLiteral("waypoint-acceptance"), QStringLiteral("nav-transition"),
        QStringLiteral("land-entry-connection"), QStringLiteral("land-descent-reference"),
        QStringLiteral("takeoff-vertical-reference"), QStringLiteral("takeoff-nav-reference-connection")};
    for (const auto& source : {domain, waypoint}) {
        for (int index = 0; index < source->components.size(); ++index) {
            const auto& component = source->components.at(index);
            const auto member = members.constFind(component.vehicleId);
            if (!charge(budget) || member == members.constEnd()
                || (component.kind != QStringLiteral("protected") && component.kind != QStringLiteral("support"))) return false;
            const QVariantMap& reference = component.reference;
            if (source == domain) {
                if (!exact(reference, {"segmentIndex", "endpointIndex"}) || !integer(reference.value(QStringLiteral("segmentIndex")), 0, 2047)
                    || !integer(reference.value(QStringLiteral("endpointIndex")), 0, 1)) return false;
            } else if (!exact(reference, {"fromSequence", "toSequence", "regionKind"})
                || !integer(reference.value(QStringLiteral("fromSequence")), 0, 2047)
                || !integer(reference.value(QStringLiteral("toSequence")), 0, 2047)
                || !waypointKinds.contains(reference.value(QStringLiteral("regionKind")).toString())) return false;
            if (!polytopeValid(component.envelope, budget)) return false;
            result.validationWork += 2 + component.envelope.planes.size();
            ComponentView view;
            view.vehicleId = component.vehicleId; view.source = source->source; view.index = index; view.kind = component.kind;
            view.reference = &component.reference; view.envelope = &component.envelope;
            view.member = member.value(); view.position = result.byMember.at(view.member).size();
            result.byMember[view.member].append(result.components.size());
            result.components.append(view);
        }
    }
    if (result.geometryComplete) {
        for (int first = 0; first < result.byMember.size(); ++first)
            for (int second = first + 1; second < result.byMember.size(); ++second) {
                if (!charge(budget)) return false;
                ++result.validationWork;
                result.pairs += qint64(result.byMember.at(first).size()) * qint64(result.byMember.at(second).size());
            }
    }
    *output = result;
    return true;
}

QVariantMap FleetMissionInterVehicleEnvelopeReview::build(const PreparedPtr& originalDomain, const PreparedPtr& originalWaypoint,
    const CancellationToken& cancellation)
{
    // Retain original immutable sets even if a caller aliases its Ptr slots.
    const PreparedPtr domain = originalDomain, waypoint = originalWaypoint;
    if (cancelled(cancellation)) return failure(QStringLiteral("inter-vehicle-envelope-review-cancelled"));
    if (!domain || !waypoint || !domain->calculationSource || !waypoint->calculationSource)
        return failure(QStringLiteral("inter-vehicle-envelope-same-build-native-sources-required"));
    Geometry::Budget budget;
    budget.remainingWork = MaximumWork; budget.remainingFragments = MaximumFragments; budget.cancellation = cancellation;
    Inputs input;
    if (!_inputs(domain, waypoint, &input, &budget))
        return failure(budget.cancelled || cancelled(cancellation) ? QStringLiteral("inter-vehicle-envelope-review-cancelled")
            : budget.exhausted ? QStringLiteral("inter-vehicle-envelope-source-validation-resource-limit")
            : QStringLiteral("inter-vehicle-envelope-source-or-complete-geometry-invalid"));
    if (!input.geometryComplete) return success(makeReport(input, false, 0, 0, {}, MaximumWork - int(budget.remainingWork), false), cancellation);
    qint64 processed = 0, conflicts = 0;
    QVariantList details;
    bool complete = true;
    for (int first = 0; first < input.byMember.size() && complete; ++first) {
        for (int second = first + 1; second < input.byMember.size() && complete; ++second) {
            for (int a : input.byMember.at(first)) {
                if (!complete) break;
                for (int b : input.byMember.at(second)) {
                    if (!charge(&budget)) { complete = false; break; }
                    const auto& left = input.components.at(a); const auto& right = input.components.at(b);
                    const Verdict verdict = intersects(*left.envelope, *right.envelope, &budget);
                    if (verdict == Verdict::Unknown) { complete = false; break; }
                    ++processed;
                    if (verdict == Verdict::ProvenTrue) {
                        ++conflicts;
                        if (details.size() < MaximumDetails) details.append(detail(left, right));
                    }
                }
            }
        }
    }
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("inter-vehicle-envelope-review-cancelled"));
    return success(makeReport(input, complete, processed, conflicts, details,
        MaximumWork - int(budget.remainingWork), budget.exhausted), cancellation);
}

bool FleetMissionInterVehicleEnvelopeReview::resultMatches(const PreparedPtr& originalDomain, const PreparedPtr& originalWaypoint,
    const QVariantMap& result)
{
    const PreparedPtr domain = originalDomain, waypoint = originalWaypoint;
    Inputs input;
    if (!_inputs(domain, waypoint, &input, nullptr)) return false;
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(result, &detached) || json(detached).size() > MaximumOutputBytes
        || !exact(detached, {"ok", "error", "report"}) || !flag(detached, "ok", true)
        || detached.value(QStringLiteral("error")) != QVariant(QStringLiteral("")) || !mapValue(detached.value(QStringLiteral("report")))) return false;
    const QVariantMap report = detached.value(QStringLiteral("report")).toMap();
    if (!exact(report, {"schemaVersion", "reportType", "geometryBasis", "clearanceInputFingerprint", "domainModelsFingerprint", "waypointModelsFingerprint",
            "memberIds", "memberCount", "domainComponentCount", "waypointComponentCount", "componentCount", "geometryComplete", "pairCount",
            "processedPairCount", "comparisonComplete", "declaredModelSatisfied", "conflictCount", "conflicts", "conflictsTruncated", "reasonCodes", "workUnits",
            "limits", "snapshotOnly", "timingKnown", "actualPathCertified", "clearanceCertified", "executionPermit", "flightCommandReleased"})
        || !integer(report.value(QStringLiteral("schemaVersion")), 1, 1)
        || report.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-inter-vehicle-envelope-review"))
        || report.value(QStringLiteral("geometryBasis")) != QVariant(input.geometryBasis)
        || report.value(QStringLiteral("clearanceInputFingerprint")) != QVariant(input.inputFingerprint)
        || report.value(QStringLiteral("domainModelsFingerprint")) != QVariant(input.domainFingerprint)
        || report.value(QStringLiteral("waypointModelsFingerprint")) != QVariant(input.waypointFingerprint)
        || !listValue(report.value(QStringLiteral("memberIds"))) || json(report.value(QStringLiteral("memberIds"))) != json(input.memberIds)
        || !integer(report.value(QStringLiteral("memberCount")), input.memberIds.size(), input.memberIds.size())
        || !integer(report.value(QStringLiteral("domainComponentCount")), input.domainCount, input.domainCount)
        || !integer(report.value(QStringLiteral("waypointComponentCount")), input.waypointCount, input.waypointCount)
        || !integer(report.value(QStringLiteral("componentCount")), input.components.size(), input.components.size())
        || !flag(report, "geometryComplete", input.geometryComplete) || !integer(report.value(QStringLiteral("pairCount")), input.pairs, input.pairs)
        || !integer(report.value(QStringLiteral("processedPairCount")), 0, input.pairs) || !integer(report.value(QStringLiteral("workUnits")), 0, MaximumWork)
        || !boolean(report, "comparisonComplete") || !boolean(report, "declaredModelSatisfied") || !boolean(report, "conflictsTruncated")
        || !listValue(report.value(QStringLiteral("conflicts"))) || !listValue(report.value(QStringLiteral("reasonCodes")))
        || !mapValue(report.value(QStringLiteral("limits"))) || json(report.value(QStringLiteral("limits"))) != json(limits())
        || !flag(report, "snapshotOnly", true) || !flag(report, "timingKnown", false) || !flag(report, "actualPathCertified", false)
        || !flag(report, "clearanceCertified", false) || !flag(report, "executionPermit", false) || !flag(report, "flightCommandReleased", false)) return false;
    const qint64 processed = report.value(QStringLiteral("processedPairCount")).toLongLong();
    if (!integer(report.value(QStringLiteral("conflictCount")), 0, processed)) return false;
    const qint64 conflicts = report.value(QStringLiteral("conflictCount")).toLongLong();
    const bool complete = report.value(QStringLiteral("comparisonComplete")).toBool();
    const QVariantList details = report.value(QStringLiteral("conflicts")).toList();
    const QVariantList codes = report.value(QStringLiteral("reasonCodes")).toList();
    const int work = report.value(QStringLiteral("workUnits")).toInt();
    const bool resource = codes.contains(ResourceReason);
    if (details.size() != std::min<qint64>(conflicts, MaximumDetails) || !flag(report, "conflictsTruncated", conflicts > MaximumDetails)
        || !flag(report, "declaredModelSatisfied", input.geometryComplete && complete && !conflicts)
        || (input.geometryComplete ? (complete ? processed != input.pairs : processed >= input.pairs) : (complete || processed || conflicts))
        || qint64(work) < input.validationWork + 2 * processed
        // feasible() does not consume fragments. This implementation can set
        // exhausted only after all work is used; Unknown for >64 planes or
        // arithmetic support is separate and need not consume that budget.
        || (resource && work != MaximumWork)
        || json(codes) != json(reasons(input.geometryComplete, complete, resource, conflicts))) return false;
    qint64 previous = -1;
    for (const QVariant& value : details) {
        if (!mapValue(value)) return false;
        const QVariantMap row = value.toMap();
        if (!exact(row, {"vehicleA", "sourceA", "componentIndexA", "componentKindA", "referenceA", "vehicleB", "sourceB", "componentIndexB", "componentKindB", "referenceB"})) return false;
        const auto component = [&](const char* sourceKey, const char* indexKey) -> const ComponentView* {
            const QVariant source = row.value(QString::fromLatin1(sourceKey));
            const bool isDomain = source == QVariant(QStringLiteral("domain"));
            if (!isDomain && source != QVariant(QStringLiteral("waypoint"))) return nullptr;
            const QVariant index = row.value(QString::fromLatin1(indexKey));
            if (!integer(index, 0, (isDomain ? input.domainCount : input.waypointCount) - 1)) return nullptr;
            return &input.components.at(index.toInt() + (isDomain ? 0 : input.domainCount));
        };
        const ComponentView* a = component("sourceA", "componentIndexA");
        const ComponentView* b = component("sourceB", "componentIndexB");
        if (!a || !b || a->member >= b->member || json(row) != json(detail(*a, *b))) return false;
        const qint64 ordinal = pairOrdinal(input, *a, *b);
        if (ordinal <= previous || ordinal >= processed) return false;
        previous = ordinal;
    }
    return true;
}

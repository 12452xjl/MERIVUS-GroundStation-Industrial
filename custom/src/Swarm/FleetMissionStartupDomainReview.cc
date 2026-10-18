#include "FleetMissionStartupDomainReview.h"

#include "FleetMissionClearanceReview.h"
#include "FleetMissionContactModel.h"
#include "FleetMissionPlanningWorker.h"
#include "FleetMissionStartupDomainCover.h"
#include "FleetMissionStartupGeographicMotion.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Polytope = Geometry::Polytope;
using Verdict = Geometry::Verdict;
using CancellationToken = Geometry::CancellationToken;
using Motion = FleetMissionStartupGeographicMotion;
using Cover = FleetMissionStartupDomainCover;
constexpr int MaximumModels = 32;
constexpr int MaximumItems = 8192;
constexpr int MaximumCells = 32768;
constexpr int MaximumComponents = 65536;
constexpr int MaximumDetails = 512;
constexpr int MaximumWork = 2000000;
constexpr int MaximumFragments = 2048;
constexpr int MaximumOutputBytes = 2 * 1024 * 1024;
constexpr qint64 CoordinateBound = 51000000;
const QString NumericUnproven = QStringLiteral("startup-domain-numeric-containment-unproven");
const QString ModelName = QStringLiteral("versioned-startup-domain-cover-v3");
const QString ResourceReason = QStringLiteral("startup-domain-geometry-resource-limit");
const QString UnknownReason = QStringLiteral("startup-domain-geometry-unknown");

bool cancelled(const CancellationToken& token)
{
    return token && token->load(std::memory_order_acquire);
}

bool mapValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool listValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantList && !value.isNull();
}

bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double result = value.toDouble();
    if (!std::isfinite(result) || result < minimum || result > maximum) return false;
    if (output) *output = result;
    return true;
}

bool integer(const QVariant& value, int minimum, int maximum)
{
    double result = 0.0;
    return number(value, minimum, maximum, &result) && result == std::floor(result);
}

bool flag(const QVariantMap& map, const char* key, bool expected)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.userType() == QMetaType::Bool && !value.isNull() && value.toBool() == expected;
}

bool boolean(const QVariantMap& map, const char* key)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.userType() == QMetaType::Bool && !value.isNull();
}

bool text(const QVariant& value, int maximum = 256, bool empty = false)
{
    if (value.userType() != QMetaType::QString) return false;
    const QString string = value.toString();
    if ((!empty && string.isEmpty()) || string.size() > maximum || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    return true;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!(ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        && !(ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))) return false;
    return true;
}

QByteArray json(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

QString fingerprint(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(json(value), QCryptographicHash::Sha256).toHex());
}

QVariantMap failure(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reason}, {QStringLiteral("report"), QVariantMap()}};
}

void addReason(QStringList* reasons, const QString& reason)
{
    if (!reasons->contains(reason)) reasons->append(reason);
}

QVariantMap limits()
{
    return {{QStringLiteral("maximumModels"), MaximumModels}, {QStringLiteral("maximumCanonicalItems"), MaximumItems},
        {QStringLiteral("maximumCoverCells"), MaximumCells}, {QStringLiteral("maximumComponents"), MaximumComponents},
        {QStringLiteral("maximumWorkUnits"), MaximumWork}, {QStringLiteral("maximumFragments"), MaximumFragments},
        {QStringLiteral("maximumViolationDetails"), MaximumDetails}, {QStringLiteral("maximumOutputBytes"), MaximumOutputBytes}};
}

bool limitsMatch(const QVariantMap& value)
{
    const QVariantMap expected = limits();
    if (value.size() != expected.size()) return false;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it)
        if (!integer(value.value(it.key()), it.value().toInt(), it.value().toInt())) return false;
    return true;
}

struct Model {
    int vehicleId = 0;
    int segmentCount = 0;
    Motion::Ptr original;
    QVariantMap declaration;
};

struct Component {
    int vehicleId = 0;
    int segmentIndex = 0;
    int endpointIndex = 0;
    QString kind;
    Polytope envelope;
};

struct Prepared {
    bool ok = false;
    QString error;
    QVariantMap input;
    QString inputFingerprint;
    QString modelsFingerprint;
    QVariantList memberIds;
    int segmentCount = 0;
    int cellCount = 0;
    bool outerComplete = false;
    bool contactComplete = true;
    bool numericComplete = true;
    QStringList reasons{QStringLiteral("startup-action-dynamics-and-between-sample-path-unproven")};
    QVector<Component> components;
};

bool expanded(const Polytope& reference, const QVariantMap& vehicle, const QVariantMap& policy,
              bool support, Polytope* result)
{
    if (!result || !reference.planes.isEmpty()) return false;
    const qint64 horizontal = qint64(vehicle.value(support ? QStringLiteral("supportHorizontalRadiusMm")
            : QStringLiteral("bodyHorizontalRadiusMm")).toDouble())
        + qint64(vehicle.value(QStringLiteral("horizontalTrackingErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalSceneErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalProjectionErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("horizontalSafetyMarginMm")).toDouble());
    const qint64 verticalError = qint64(vehicle.value(QStringLiteral("verticalTrackingErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalSceneErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalDatumAlignmentErrorMm")).toDouble())
        + qint64(policy.value(QStringLiteral("verticalSafetyMarginMm")).toDouble());
    const qint64 below = verticalError + qint64(vehicle.value(support ? QStringLiteral("supportBelowReferenceMm")
        : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
    const qint64 above = verticalError + qint64(vehicle.value(support ? QStringLiteral("supportAboveReferenceMm")
        : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
    Polytope output = reference;
    output.bounds.minimumEast -= horizontal; output.bounds.maximumEast += horizontal;
    output.bounds.minimumNorth -= horizontal; output.bounds.maximumNorth += horizontal;
    output.bounds.minimumAltitude -= below; output.bounds.maximumAltitude += above;
    const auto& b = output.bounds;
    if (b.minimumEast < -CoordinateBound || b.maximumEast > CoordinateBound || b.minimumEast > b.maximumEast
        || b.minimumNorth < -CoordinateBound || b.maximumNorth > CoordinateBound || b.minimumNorth > b.maximumNorth
        || b.minimumAltitude < -CoordinateBound || b.maximumAltitude > CoordinateBound || b.minimumAltitude > b.maximumAltitude) return false;
    *result = output;
    return true;
}

Prepared prepare(const QVariant& clearanceInput, const QVariantList& sourceModels,
                 const Motion::Models* originalModels, const QVariantMap& joint,
                 const QString& geometryBasis, const CancellationToken& token)
{
    Prepared result;
    const auto fail = [&](const QString& reason) {
        result.ok = false;
        result.error = cancelled(token) ? QStringLiteral("startup-domain-review-cancelled") : reason;
        result.components.clear();
        return result;
    };
    if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
    // The established transport caps the ENTIRE copied source, not each row
    // separately. The clearance/Joint helper retains its smaller own budgets.
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot({{QStringLiteral("clearanceInput"), clearanceInput},
            {QStringLiteral("models"), sourceModels}, {QStringLiteral("jointView"), joint},
            {QStringLiteral("geometryBasis"), geometryBasis}}, &detached))
        return fail(QStringLiteral("startup-domain-source-value-or-resource-invalid"));
    if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
    if (!digest(geometryBasis) || joint.value(QStringLiteral("geometryBasis")) != QVariant(geometryBasis)
        || !FleetMissionClearanceReview::geometrySourceValid(clearanceInput, joint, &result.input, token)
        || joint.value(QStringLiteral("scope")) != QVariant(QStringLiteral("startup-and-encoded-planning"))
        || !flag(joint, "startupIncluded", true)) return fail(QStringLiteral("startup-domain-joint-source-invalid"));
    result.inputFingerprint = fingerprint(result.input);
    result.modelsFingerprint = fingerprint(sourceModels);
    const QVariantList declarations = result.input.value(QStringLiteral("vehicles")).toList();
    const QVariantList coverage = joint.value(QStringLiteral("memberCoverage")).toList();
    if (sourceModels.isEmpty() || sourceModels.size() > MaximumModels || sourceModels.size() != declarations.size()
        || sourceModels.size() != coverage.size()) return fail(QStringLiteral("startup-domain-member-count-invalid"));
    if (originalModels && originalModels->size() != sourceModels.size())
        return fail(QStringLiteral("startup-domain-original-model-set-mismatch"));
    QVector<Model> models;
    QSet<int> seen;
    int itemCount = 0;
    for (int index = 0; index < sourceModels.size(); ++index) {
        if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
        if (!mapValue(sourceModels.at(index))) return fail(QStringLiteral("startup-domain-model-row-invalid"));
        const QVariantMap row = sourceModels.at(index).toMap();
        if (!exact(row, {"vehicleId", "geographicMotionModel"}) || !integer(row.value(QStringLiteral("vehicleId")), 1, 255)
            || !mapValue(row.value(QStringLiteral("geographicMotionModel")))) return fail(QStringLiteral("startup-domain-model-row-invalid"));
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        const QVariantMap declaration = declarations.at(index).toMap(), member = coverage.at(index).toMap();
        if (seen.contains(id) || !integer(declaration.value(QStringLiteral("vehicleId")), id, id)
            || !integer(member.value(QStringLiteral("vehicleId")), id, id) || !flag(member, "encodingAvailable", true))
            return fail(QStringLiteral("startup-domain-member-order-or-encoding-invalid"));
        seen.insert(id);
        const QVariantMap source = row.value(QStringLiteral("geographicMotionModel")).toMap();
        if (!mapValue(source.value(QStringLiteral("baselineGlobalPosition")))
            || !listValue(source.value(QStringLiteral("canonicalWireItems")))
            || !mapValue(source.value(QStringLiteral("startupTransitionPolicy")))) return fail(QStringLiteral("startup-domain-motion-source-invalid"));
        const QVariantList wire = source.value(QStringLiteral("canonicalWireItems")).toList();
        double ceiling = 0.0;
        if (wire.isEmpty() || wire.size() > 2048 || itemCount > MaximumItems - wire.size()
            || !number(source.value(QStringLiteral("maximumAltitudeAmslMeters")), -1000.0, 20000.0, &ceiling)
            || source.value(QStringLiteral("payloadContentHash")) != member.value(QStringLiteral("payloadHash")))
            return fail(QStringLiteral("startup-domain-motion-budget-or-payload-invalid"));
        itemCount += wire.size();
        Model model;
        model.vehicleId = id; model.declaration = declaration;
        if (originalModels) {
            // The actual worker owns a distinct outer map containing these
            // exact original immutable objects. Same JSON is not permission
            // to recompute/substitute their cached Qt distance/bearing values.
            model.original = originalModels->value(id);
        } else {
            // ONLY the public value-only result matcher takes this branch.
            // Its temporary model establishes shape/counts, not native source
            // continuity or the actual calculation's numeric operand identity.
            QVariantMap payload{{QStringLiteral("canonicalWireItems"), wire},
                {QStringLiteral("contentHash"), source.value(QStringLiteral("payloadContentHash"))}};
            if (source.contains(QStringLiteral("startupSpatialGuard")))
                payload.insert(QStringLiteral("executionProfile"), QVariantMap{
                    {QStringLiteral("startupSpatialGuard"), source.value(QStringLiteral("startupSpatialGuard"))},
                    {QStringLiteral("startupTransitionPolicy"), source.value(QStringLiteral("startupTransitionPolicy"))}});
            model.original = Motion::prepare(source.value(QStringLiteral("baselineGlobalPosition")).toMap(), payload,
                source.value(QStringLiteral("startupTransitionPolicy")).toMap(), ceiling);
        }
        if (!model.original || json(model.original->source()) != json(source))
            return fail(QStringLiteral("startup-domain-motion-source-invalid"));
        for (const QVariant& item : wire) {
            // Motion::prepare validated the complete command/frame/value set;
            // speed is the only accepted non-geographic command (178).
            if (item.toMap().value(QStringLiteral("command")).toInt() != 178) ++model.segmentCount;
        }
        result.segmentCount += model.segmentCount;
        result.memberIds.append(id);
        models.append(model);
    }
    if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
    const bool supportModel = result.input.value(QStringLiteral("schemaVersion")).toInt() == 2;
    if (supportModel) {
        const auto contact = FleetMissionContactModel::prepare(result.input, joint, token);
        if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
        result.contactComplete = contact.ok && contact.contactContextComplete;
        // These pads are bound to individual nominal TAKEOFF/LAND components.
        // The original runtime predicate accepts a union of every geographic
        // leg and has no action/phase binding. Neither a cover-cell leg index
        // nor a shared vehicle/terrain ID extends that contact permission to
        // this whole independent-altitude domain. Keep the context diagnosis,
        // but never transfer the nominal action's terrain-contact waiver.
        if (!result.contactComplete) addReason(&result.reasons, QStringLiteral("startup-domain-contact-context-incomplete"));
    }
    // Prepare ALL original models before starting the environment solver.
    // A later unsupported model discards every earlier usable cell/component.
    QVector<Component> components;
    int cellCount = 0;
    bool complete = true;
    const QVariantMap policy = result.input.value(QStringLiteral("policy")).toMap();
    for (const Model& model : models) {
        if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
        const auto cover = Cover::prepare(model.original, result.input, token);
        if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
        result.numericComplete = result.numericComplete && cover.nativeIntegerDomainCovered;
        if (!cover.ok) {
            complete = false;
            addReason(&result.reasons, QStringLiteral("startup-domain-outer-cover-unsupported"));
            // Only the native fixed-error Cover producer supplies this value;
            // retaining it distinguishes frame/slab/cell-capacity rejection.
            if (!cover.error.isEmpty()) addReason(&result.reasons, cover.error);
            continue;
        }
        if (cover.segmentCount != model.segmentCount || cover.cellCount != cover.cells.size() || cover.cells.isEmpty()) {
            complete = false;
            addReason(&result.reasons, QStringLiteral("startup-domain-outer-cover-identity-invalid"));
            continue;
        }
        if (cellCount > MaximumCells - cover.cells.size()) {
            complete = false;
            addReason(&result.reasons, QStringLiteral("startup-domain-outer-cover-capacity-exceeded"));
            continue;
        }
        cellCount += cover.cells.size();
        for (const auto& cell : cover.cells) {
            if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
            if (cell.segmentIndex < 0 || cell.segmentIndex >= model.segmentCount || cell.endpointIndex < 0 || cell.endpointIndex > 1) {
                complete = false;
                addReason(&result.reasons, QStringLiteral("startup-domain-outer-cover-identity-invalid"));
                continue;
            }
            for (int part = 0; part < (supportModel ? 2 : 1); ++part) {
                Component component;
                component.vehicleId = model.vehicleId; component.segmentIndex = cell.segmentIndex; component.endpointIndex = cell.endpointIndex;
                component.kind = part ? QStringLiteral("support") : QStringLiteral("protected");
                if (!expanded(cell.reference, model.declaration, policy, part != 0, &component.envelope)) {
                    complete = false;
                    addReason(&result.reasons, QStringLiteral("startup-domain-expanded-envelope-domain-unsupported"));
                    continue;
                }
                components.append(component);
            }
        }
    }
    if (cancelled(token)) return fail(QStringLiteral("startup-domain-review-cancelled"));
    if (complete && components.size() == cellCount * (supportModel ? 2 : 1) && components.size() <= MaximumComponents) {
        result.components = components;
        result.cellCount = cellCount;
        result.outerComplete = true;
    }
    if (!result.outerComplete || !result.numericComplete) addReason(&result.reasons, NumericUnproven);
    result.ok = true;
    return result;
}

bool prepareCoverageIndex(const QVariantList& rows, bool horizontalOnly,
                          Geometry::PreparedPrismIndex* index, Geometry::Budget* budget,
                          const CancellationToken& token)
{
    if (!index || !budget) return false;
    QVector<Geometry::PreparedPrism> prisms;
    prisms.reserve(rows.size());
    for (const QVariant& value : rows) {
        if (cancelled(token)) return false;
        const QVariantMap row = value.toMap();
        QVector<Geometry::Point2> footprint;
        const QVariantList polygon = row.value(QStringLiteral("polygon")).toList();
        footprint.reserve(polygon.size());
        for (const QVariant& pointValue : polygon) {
            const QVariantMap point = pointValue.toMap();
            Geometry::Point2 vertex;
            vertex.east = qint64(point.value(QStringLiteral("eastMm")).toDouble());
            vertex.north = qint64(point.value(QStringLiteral("northMm")).toDouble());
            footprint.append(vertex);
        }
        // Terrain coverage is purely horizontal over the COMPLETE query
        // domain. Its independent index must not use the terrain contact top.
        const qint64 lower = horizontalOnly ? -CoordinateBound
            : qint64(row.value(QStringLiteral("minimumAmslMm")).toDouble());
        const qint64 upper = horizontalOnly ? CoordinateBound
            : qint64(row.value(QStringLiteral("maximumAmslMm")).toDouble());
        Geometry::PreparedPrism prism;
        if (!Geometry::prepareConvexPrism(footprint, lower, upper, &prism, budget)) return false;
        prisms.append(prism);
    }
    return Geometry::preparePrismIndex(prisms, index, budget);
}

QString componentKey(int vehicleId, int segmentIndex, int endpointIndex, const QString& kind)
{
    return QString::number(vehicleId) + QLatin1Char(':') + QString::number(segmentIndex) + QLatin1Char(':')
        + QString::number(endpointIndex) + QLatin1Char(':') + kind;
}

QStringList reasonsFor(const Prepared& prepared, bool comparisonComplete, bool resourceLimit, int violationCount)
{
    QStringList reasons = prepared.reasons;
    if (prepared.outerComplete && !comparisonComplete) addReason(&reasons, resourceLimit ? ResourceReason : UnknownReason);
    if (violationCount) addReason(&reasons, QStringLiteral("startup-domain-envelope-candidates-observed"));
    if (violationCount > MaximumDetails) addReason(&reasons, QStringLiteral("startup-domain-violation-details-truncated"));
    return reasons;
}

QVariantMap makeReport(const Prepared& prepared, const QString& basis, bool comparisonComplete, int processed,
                       int violationCount, const QVariantList& violations, int workUnits, bool resourceLimit)
{
    return {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("reportType"), QStringLiteral("fleet-startup-domain-review")},
        {QStringLiteral("geometryBasis"), basis}, {QStringLiteral("clearanceInputFingerprint"), prepared.inputFingerprint},
        {QStringLiteral("motionModelsFingerprint"), prepared.modelsFingerprint}, {QStringLiteral("memberIds"), prepared.memberIds},
        {QStringLiteral("memberCount"), prepared.memberIds.size()}, {QStringLiteral("segmentCount"), prepared.segmentCount},
        {QStringLiteral("coverCellCount"), prepared.cellCount}, {QStringLiteral("componentCount"), prepared.components.size()},
        {QStringLiteral("processedComponentCount"), processed}, {QStringLiteral("outerCoverComplete"), prepared.outerComplete},
        {QStringLiteral("comparisonComplete"), comparisonComplete},
        {QStringLiteral("declaredModelSatisfied"), prepared.outerComplete && comparisonComplete && prepared.contactComplete && !violationCount},
        {QStringLiteral("contactContextComplete"), prepared.contactComplete},
        {QStringLiteral("numericContainmentProven"), prepared.outerComplete && prepared.numericComplete},
        {QStringLiteral("violationCount"), violationCount}, {QStringLiteral("violations"), violations},
        {QStringLiteral("violationsTruncated"), violationCount > MaximumDetails},
        {QStringLiteral("reasonCodes"), reasonsFor(prepared, comparisonComplete, resourceLimit, violationCount)},
        {QStringLiteral("workUnits"), workUnits}, {QStringLiteral("limits"), limits()}, {QStringLiteral("modelName"), ModelName},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("actualPathCertified"), false}};
}

QVariantMap success(const QVariantMap& report, const CancellationToken& token)
{
    if (cancelled(token)) return failure(QStringLiteral("startup-domain-review-cancelled"));
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("report"), report}};
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(result, &detached) || json(detached).size() > MaximumOutputBytes)
        return failure(cancelled(token) ? QStringLiteral("startup-domain-review-cancelled") : QStringLiteral("startup-domain-review-output-resource-invalid"));
    return cancelled(token) ? failure(QStringLiteral("startup-domain-review-cancelled")) : detached;
}
} // namespace

QVariantMap FleetMissionStartupDomainReview::build(const QVariant& clearanceInput, const QVariantList& models,
    const Motion::Models& originalModels, const QVariantMap& frozenJointView, const QString& geometryBasis,
    const CancellationToken& cancellation, PreparedPtr* exported)
{
    if (exported) exported->reset();
    const Prepared prepared = prepare(clearanceInput, models, &originalModels, frozenJointView, geometryBasis, cancellation);
    if (!prepared.ok) return failure(prepared.error);
    const auto finish = [&](const QVariantMap& result) {
        if (exported && flag(result, "ok", true) && !cancelled(cancellation)) {
            using Envelope = FleetMissionInterVehicleEnvelopeReview::Prepared;
            const auto value = std::shared_ptr<Envelope>(new Envelope);
            value->source = QStringLiteral("domain"); value->geometryBasis = geometryBasis;
            value->inputFingerprint = prepared.inputFingerprint; value->modelsFingerprint = prepared.modelsFingerprint;
            value->motionFingerprint = prepared.modelsFingerprint;
            value->memberIds = prepared.memberIds; value->geometryComplete = prepared.outerComplete;
            value->calculationSource = true;
            if (prepared.outerComplete) {
                value->components.reserve(prepared.components.size());
                for (const Component& component : prepared.components) {
                    if (cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
                    Envelope::Component copy;
                    copy.vehicleId = component.vehicleId; copy.kind = component.kind;
                    copy.reference = {{QStringLiteral("segmentIndex"), component.segmentIndex},
                        {QStringLiteral("endpointIndex"), component.endpointIndex}};
                    copy.envelope = component.envelope;
                    value->components.append(copy); // Includes EVERY extra plane.
                }
            }
            *exported = value;
        }
        if (cancelled(cancellation)) {
            if (exported) exported->reset();
            return failure(QStringLiteral("startup-domain-review-cancelled"));
        }
        return result;
    };
    if (!prepared.outerComplete) return finish(success(makeReport(prepared, geometryBasis, false, 0, 0, {}, 0, false), cancellation));

    const QVariantMap scene = prepared.input.value(QStringLiteral("scene")).toMap();
    const QVariantList terrainRows = scene.value(QStringLiteral("terrainCells")).toList();
    const QVariantList obstacleRows = scene.value(QStringLiteral("obstacleVolumes")).toList();
    Geometry::Budget budget;
    budget.remainingWork = MaximumWork;
    budget.remainingFragments = MaximumFragments;
    budget.cancellation = cancellation;
    QVector<Geometry::PreparedPrism> terrainPrisms, obstaclePrisms;
    const auto preparationFailure = [&](const QString& invalidReason) {
        if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
        if (budget.exhausted) return finish(success(makeReport(prepared, geometryBasis, false, 0, 0, {},
            MaximumWork - int(budget.remainingWork), true), cancellation));
        return failure(invalidReason);
    };
    // Prepare EVERY normalized scene prism before any environment judgment.
    // The same calculation budget pays for this complete preparation and all
    // later unions/SAT queries. No failed/default or usable prefix is queried.
    for (int collection = 0; collection < 2; ++collection) {
        const QVariantList& rows = collection == 0 ? terrainRows : obstacleRows;
        QVector<Geometry::PreparedPrism>& prisms = collection == 0 ? terrainPrisms : obstaclePrisms;
        prisms.reserve(rows.size());
        for (const QVariant& value : rows) {
            if (cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
            const QVariantMap row = value.toMap();
            QVector<Geometry::Point2> footprint;
            const QVariantList polygon = row.value(QStringLiteral("polygon")).toList();
            footprint.reserve(polygon.size());
            for (const QVariant& pointValue : polygon) {
                const QVariantMap point = pointValue.toMap();
                Geometry::Point2 point2;
                point2.east = qint64(point.value(QStringLiteral("eastMm")).toDouble());
                point2.north = qint64(point.value(QStringLiteral("northMm")).toDouble());
                footprint.append(point2);
            }
            // Queries already lie in the full +/-CoordinateBound support
            // domain, so truncating terrain below that bound is exact here;
            // the real terrain top and every obstacle's Z limits are kept.
            const qint64 lower = collection == 0 ? -CoordinateBound
                : qint64(row.value(QStringLiteral("minimumAmslMm")).toDouble());
            const qint64 upper = qint64(row.value(collection == 0 ? QStringLiteral("maximumTerrainAmslMm")
                : QStringLiteral("maximumAmslMm")).toDouble());
            Geometry::PreparedPrism prism;
            if (!Geometry::prepareConvexPrism(footprint, lower, upper, &prism, &budget))
                return preparationFailure(QStringLiteral("startup-domain-scene-prism-invalid"));
            prisms.append(prism);
        }
    }
    // Both complete indices own their original prism rows. Neither a usable
    // first index nor partially enumerated candidates is a complete scene.
    Geometry::PreparedPrismIndex terrainIndex, obstacleIndex;
    if (!Geometry::preparePrismIndex(terrainPrisms, &terrainIndex, &budget)
        || !Geometry::preparePrismIndex(obstaclePrisms, &obstacleIndex, &budget))
        return preparationFailure(QStringLiteral("startup-domain-scene-prism-index-invalid"));
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
    Geometry::PreparedPrismIndex coverageIndex, corridorIndex, terrainHorizontalIndex;
    // Complete each union under the SAME group budget before any query.
    // Empty unions remain valid empty indices, never a partial scene prefix.
    if (!prepareCoverageIndex(scene.value(QStringLiteral("coverageVolumes")).toList(), false, &coverageIndex, &budget, cancellation)
        || !prepareCoverageIndex(scene.value(QStringLiteral("corridorVolumes")).toList(), false, &corridorIndex, &budget, cancellation)
        || !prepareCoverageIndex(terrainRows, true, &terrainHorizontalIndex, &budget, cancellation))
        return preparationFailure(QStringLiteral("startup-domain-coverage-prism-index-invalid"));
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
    int processed = 0, violationCount = 0;
    QVariantList violations;
    bool comparisonComplete = true;
    const auto consume = [&](Verdict verdict) {
        if (verdict != Verdict::Unknown) return true;
        comparisonComplete = false;
        return false;
    };
    const auto violate = [&](const Component& component, const QString& code, const QString& volumeId) {
        ++violationCount;
        if (violations.size() < MaximumDetails) violations.append(QVariantMap{
            {QStringLiteral("vehicleId"), component.vehicleId}, {QStringLiteral("segmentIndex"), component.segmentIndex},
            {QStringLiteral("endpointIndex"), component.endpointIndex}, {QStringLiteral("componentKind"), component.kind},
            {QStringLiteral("code"), code}, {QStringLiteral("volumeId"), volumeId}});
    };
    for (const Component& component : prepared.components) {
        if (cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
        Verdict verdict = Geometry::coveredByIndexedPrisms(component.envelope, coverageIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(component, QStringLiteral("domain-envelope-not-covered"), QStringLiteral(""));
        verdict = Geometry::coveredByIndexedPrisms(component.envelope, corridorIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(component, QStringLiteral("domain-envelope-outside-corridor-union"), QStringLiteral(""));
        // Full-domain terrain prisms preserve the old XY-only coverage test.
        // The separate terrain-contact index below still keeps every top.
        verdict = Geometry::coveredByIndexedPrisms(component.envelope, terrainHorizontalIndex, &budget);
        if (!consume(verdict)) break;
        if (verdict == Verdict::ProvenFalse) violate(component, QStringLiteral("domain-envelope-terrain-coverage-incomplete"), QStringLiteral(""));
        QVector<int> terrainCandidates;
        if (!Geometry::prismCandidates(component.envelope, terrainIndex, &terrainCandidates, &budget)) {
            consume(Verdict::Unknown);
            break; // Never consume an incomplete candidate-output prefix.
        }
        for (int index : terrainCandidates) {
            if (cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
            verdict = Geometry::axisAlignedBoxIntersectsIndexedPrism(component.envelope, terrainIndex, index, &budget);
            if (!consume(verdict)) break;
            if (verdict == Verdict::ProvenTrue)
                violate(component, QStringLiteral("domain-envelope-terrain-contact-candidate"),
                    terrainRows.at(index).toMap().value(QStringLiteral("cellId")).toString());
        }
        if (!comparisonComplete) break;
        QVector<int> obstacleCandidates;
        if (!Geometry::prismCandidates(component.envelope, obstacleIndex, &obstacleCandidates, &budget)) {
            consume(Verdict::Unknown);
            break; // Complete earlier findings remain diagnostic only.
        }
        for (int index : obstacleCandidates) {
            if (cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
            verdict = Geometry::axisAlignedBoxIntersectsIndexedPrism(component.envelope, obstacleIndex, index, &budget);
            if (!consume(verdict)) break;
            if (verdict == Verdict::ProvenTrue) violate(component, QStringLiteral("domain-envelope-obstacle-contact-candidate"),
                obstacleRows.at(index).toMap().value(QStringLiteral("volumeId")).toString());
        }
        if (!comparisonComplete) break;
        ++processed;
    }
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("startup-domain-review-cancelled"));
    return finish(success(makeReport(prepared, geometryBasis, comparisonComplete, processed, violationCount, violations,
        MaximumWork - int(budget.remainingWork), budget.exhausted), cancellation));
}

bool FleetMissionStartupDomainReview::resultMatches(const QVariant& clearanceInput, const QVariantList& models,
    const QVariantMap& frozenJointView, const QString& geometryBasis, const QVariantMap& result, PreparedPtr* exported)
{
    if (exported) exported->reset();
    QVariantMap detached;
    if (!FleetMissionPlanningWorker::snapshot(result, &detached) || json(result).size() > MaximumOutputBytes
        || !exact(result, {"ok", "error", "report"}) || !flag(result, "ok", true)
        || !text(result.value(QStringLiteral("error")), 256, true) || !result.value(QStringLiteral("error")).toString().isEmpty()
        || !mapValue(result.value(QStringLiteral("report")))) return false;
    const QVariantMap report = result.value(QStringLiteral("report")).toMap();
    if (!exact(report, {"schemaVersion", "reportType", "geometryBasis", "clearanceInputFingerprint", "motionModelsFingerprint",
            "memberIds", "memberCount", "segmentCount", "coverCellCount", "componentCount", "processedComponentCount",
            "outerCoverComplete", "comparisonComplete", "declaredModelSatisfied", "contactContextComplete", "numericContainmentProven",
            "violationCount", "violations", "violationsTruncated", "reasonCodes", "workUnits", "limits", "modelName", "snapshotOnly",
            "clearanceCertified", "executionPermit", "flightCommandReleased", "actualPathCertified"})
        || !integer(report.value(QStringLiteral("schemaVersion")), 2, 2)
        || report.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-startup-domain-review"))
        || report.value(QStringLiteral("modelName")) != QVariant(ModelName)
        || !boolean(report, "numericContainmentProven") || !flag(report, "snapshotOnly", true)
        || !flag(report, "clearanceCertified", false) || !flag(report, "executionPermit", false)
        || !flag(report, "flightCommandReleased", false) || !flag(report, "actualPathCertified", false)
        || !boolean(report, "outerCoverComplete") || !boolean(report, "comparisonComplete")
        || !boolean(report, "declaredModelSatisfied") || !boolean(report, "contactContextComplete")
        || !boolean(report, "violationsTruncated") || !listValue(report.value(QStringLiteral("memberIds")))
        || !listValue(report.value(QStringLiteral("violations"))) || !listValue(report.value(QStringLiteral("reasonCodes")))
        || !mapValue(report.value(QStringLiteral("limits"))) || !limitsMatch(report.value(QStringLiteral("limits")).toMap())) return false;
    const Prepared prepared = prepare(clearanceInput, models, nullptr, frozenJointView, geometryBasis, {});
    if (!prepared.ok || report.value(QStringLiteral("geometryBasis")) != QVariant(geometryBasis)
        || report.value(QStringLiteral("clearanceInputFingerprint")) != QVariant(prepared.inputFingerprint)
        || report.value(QStringLiteral("motionModelsFingerprint")) != QVariant(prepared.modelsFingerprint)
        || json(report.value(QStringLiteral("memberIds"))) != json(prepared.memberIds)
        || !integer(report.value(QStringLiteral("memberCount")), prepared.memberIds.size(), prepared.memberIds.size())
        || !integer(report.value(QStringLiteral("segmentCount")), prepared.segmentCount, prepared.segmentCount)
        || !integer(report.value(QStringLiteral("coverCellCount")), prepared.cellCount, prepared.cellCount)
        || !integer(report.value(QStringLiteral("componentCount")), prepared.components.size(), prepared.components.size())
        || !flag(report, "outerCoverComplete", prepared.outerComplete) || !flag(report, "contactContextComplete", prepared.contactComplete)
        || !flag(report, "numericContainmentProven", prepared.outerComplete && prepared.numericComplete)
        || !integer(report.value(QStringLiteral("processedComponentCount")), 0, prepared.components.size())
        || !integer(report.value(QStringLiteral("workUnits")), 0, MaximumWork)) return false;
    const bool complete = report.value(QStringLiteral("comparisonComplete")).toBool();
    const int processed = report.value(QStringLiteral("processedComponentCount")).toInt();
    const int components = prepared.components.size();
    const QVariantMap scene = prepared.input.value(QStringLiteral("scene")).toMap();
    const int checksPerComponent = 3 + scene.value(QStringLiteral("terrainCells")).toList().size()
        + scene.value(QStringLiteral("obstacleVolumes")).toList().size();
    if (!integer(report.value(QStringLiteral("violationCount")), 0, (complete ? components : std::min(components, processed + 1)) * checksPerComponent)) return false;
    const int violationCount = report.value(QStringLiteral("violationCount")).toInt();
    const QVariantList violations = report.value(QStringLiteral("violations")).toList();
    if (violations.size() != std::min(violationCount, MaximumDetails)
        || !flag(report, "violationsTruncated", violationCount > MaximumDetails)
        || !flag(report, "declaredModelSatisfied", prepared.outerComplete && complete && prepared.contactComplete && !violationCount)
        || (prepared.outerComplete ? (complete ? processed != components : processed >= components)
            : (complete || processed != 0 || violationCount != 0 || report.value(QStringLiteral("workUnits")).toInt() != 0))) return false;
    const QVariantList reasons = report.value(QStringLiteral("reasonCodes")).toList();
    // Resource exhaustion may be caused by fragments before all work is used;
    // the shape matcher must not rerun or invent the environment conclusion.
    const bool resource = reasons.contains(ResourceReason);
    if (json(report.value(QStringLiteral("reasonCodes"))) != json(reasonsFor(prepared, complete, resource, violationCount))) return false;
    QMap<QString, int> earliestComponentIndex;
    QSet<QString> terrainIds, obstacleIds;
    for (int index = 0; index < prepared.components.size(); ++index) {
        const Component& component = prepared.components.at(index);
        const QString key = componentKey(component.vehicleId, component.segmentIndex, component.endpointIndex, component.kind);
        // An antimeridian split may produce several cells with the same row6
        // identity. Keep its FIRST possible occurrence, not a fake unique ID
        // or the last split's index that could reject an earlier real detail.
        if (!earliestComponentIndex.contains(key)) earliestComponentIndex.insert(key, index);
    }
    for (const QVariant& row : scene.value(QStringLiteral("terrainCells")).toList()) terrainIds.insert(row.toMap().value(QStringLiteral("cellId")).toString());
    for (const QVariant& row : scene.value(QStringLiteral("obstacleVolumes")).toList()) obstacleIds.insert(row.toMap().value(QStringLiteral("volumeId")).toString());
    const QSet<QString> emptyVolumeCodes{QStringLiteral("domain-envelope-not-covered"), QStringLiteral("domain-envelope-outside-corridor-union"),
        QStringLiteral("domain-envelope-terrain-coverage-incomplete")};
    for (const QVariant& value : violations) {
        if (!mapValue(value)) return false;
        const QVariantMap row = value.toMap();
        if (!exact(row, {"vehicleId", "segmentIndex", "endpointIndex", "componentKind", "code", "volumeId"})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255) || !integer(row.value(QStringLiteral("segmentIndex")), 0, 2047)
            || !integer(row.value(QStringLiteral("endpointIndex")), 0, 1) || !text(row.value(QStringLiteral("componentKind")), 16)
            || !text(row.value(QStringLiteral("code"))) || !text(row.value(QStringLiteral("volumeId")), 128, true)) return false;
        const QString key = componentKey(row.value(QStringLiteral("vehicleId")).toInt(), row.value(QStringLiteral("segmentIndex")).toInt(),
            row.value(QStringLiteral("endpointIndex")).toInt(), row.value(QStringLiteral("componentKind")).toString());
        const auto first = earliestComponentIndex.constFind(key);
        // The producer processes complete components [0,processed) and can
        // retain partial findings only from the current component processed.
        // It cannot have visited a later-only identity after Unknown stopped
        // the loop. Complete reports are not restricted by this prefix rule.
        if (first == earliestComponentIndex.constEnd() || (!complete && first.value() > processed)) return false;
        const QString code = row.value(QStringLiteral("code")).toString(), id = row.value(QStringLiteral("volumeId")).toString();
        if (emptyVolumeCodes.contains(code)) { if (!id.isEmpty()) return false; }
        else if (code == QStringLiteral("domain-envelope-terrain-contact-candidate")) { if (!terrainIds.contains(id)) return false; }
        else if (code == QStringLiteral("domain-envelope-obstacle-contact-candidate")) { if (!obstacleIds.contains(id)) return false; }
        else return false;
    }
    if (exported) {
        using Envelope = FleetMissionInterVehicleEnvelopeReview::Prepared;
        const auto value = std::shared_ptr<Envelope>(new Envelope);
        value->source = QStringLiteral("domain"); value->geometryBasis = geometryBasis;
        value->inputFingerprint = prepared.inputFingerprint; value->modelsFingerprint = prepared.modelsFingerprint;
        value->motionFingerprint = prepared.modelsFingerprint;
        value->memberIds = prepared.memberIds; value->geometryComplete = prepared.outerComplete;
        value->calculationSource = false;
        if (prepared.outerComplete) {
            value->components.reserve(prepared.components.size());
            for (const Component& component : prepared.components) {
                Envelope::Component copy;
                copy.vehicleId = component.vehicleId; copy.kind = component.kind;
                copy.reference = {{QStringLiteral("segmentIndex"), component.segmentIndex},
                    {QStringLiteral("endpointIndex"), component.endpointIndex}};
                copy.envelope = component.envelope;
                value->components.append(copy);
            }
        }
        *exported = value; // Value-only reconstruction is NOT a calculation source.
    }
    return true;
}

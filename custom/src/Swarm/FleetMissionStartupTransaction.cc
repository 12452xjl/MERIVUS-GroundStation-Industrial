#include "FleetMissionStartupTransaction.h"
#include "FleetMissionStartupClearanceWitness.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetStartupMemorySeal.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionStartReadiness.h"
#include "FaultToleranceManager.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionExecutionObservation.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionStartupAuthority.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionStartupTransition.h"
#include "FleetMissionStartupEstimator.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "QGCMAVLink.h"
#include "px4_custom_mode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>
#include <initializer_list>

namespace {
bool outboundGroup(const QObject* owner, std::initializer_list<const QObject*> peers)
{
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)) return false;
    for (const QObject* peer : peers)
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, peer)) return false;
    return true;
}
constexpr qint64 MaximumMs = 30000;
constexpr qint64 StageMs = 6000;
constexpr int PaceMs = 100;
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr double ExactMaximum = 9007199254740991.0;
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool integer(const QVariant& value, double minimum, double maximum, double* number = nullptr)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum || std::floor(candidate) != candidate) return false;
    if (number) *number = candidate;
    return true;
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
    for (QChar character : value.toString()) if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
        || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}
bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > 20) return false;
    bool valid = false;
    const quint64 number = value.toString().toULongLong(&valid);
    return valid && number > 0 && QString::number(number) == value.toString();
}
bool stepReceipt(const QVariantMap& map, const QString& startupId, const QString& stepId, int step, int vehicleId, int componentId)
{
    double schema = 0, actualVehicle = 0, actualComponent = 0;
    // Schema 3 adds the actual estimator queue basis to the dedicated native
    // lane. A schema-2 receipt cannot claim this continuous EST coverage.
    return map.size() == 33 && integer(map.value(QStringLiteral("schemaVersion")), 3, 3, &schema)
        && map.value(QStringLiteral("startupId")).type() == QVariant::String && map.value(QStringLiteral("startupId")).toString() == startupId
        && map.value(QStringLiteral("stepId")).type() == QVariant::String && map.value(QStringLiteral("stepId")).toString() == stepId
        && map.value(QStringLiteral("step")).type() == QVariant::String
        && map.value(QStringLiteral("step")).toString() == (step == 0 ? QStringLiteral("Seq0") : step == 1 ? QStringLiteral("NormalArm") : QStringLiteral("MissionModeOnly"))
        && integer(map.value(QStringLiteral("vehicleId")), vehicleId, vehicleId, &actualVehicle)
        && integer(map.value(QStringLiteral("componentId")), componentId, componentId, &actualComponent)
        && decimal(map.value(QStringLiteral("operationId"))) && uuid(map.value(QStringLiteral("sourceEpoch")))
        && uuid(map.value(QStringLiteral("flightSourceEpoch"))) && digest(map.value(QStringLiteral("wirePayloadHash")))
        && isTrue(map.value(QStringLiteral("requestIssued")))
        && (step == 0 ? isFalse(map.value(QStringLiteral("flightCommandReleased"))) : isTrue(map.value(QStringLiteral("flightCommandReleased"))))
        && isFalse(map.value(QStringLiteral("requestCausalityVerified"))) && isFalse(map.value(QStringLiteral("authenticated")))
        && isFalse(map.value(QStringLiteral("signatureVerified"))) && isFalse(map.value(QStringLiteral("executionVerified")))
        && isFalse(map.value(QStringLiteral("journalExhausted"))) && map.value(QStringLiteral("riskObservationsJournal")).type() == QVariant::List
        && map.value(QStringLiteral("queueEstimatorObservation")).type() == QVariant::Map
        && FleetMissionStartupEstimator::healthyReference(map.value(QStringLiteral("queueEstimatorObservation")).toMap());
}
bool tree(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 20 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += static_cast<qint64>(it.key().size()) * 2;
            if (*amount > MaximumBytes || !tree(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!tree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList()) if (!tree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::String:
        *amount += static_cast<qint64>(value.toString().size()) * 2;
        return *amount <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool encode(const QVariantMap& map, QByteArray* result)
{
    int nodes = 0; qint64 amount = 0;
    if (map.isEmpty() || !tree(map, 0, &nodes, &amount)) return false;
    *result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !result->isEmpty() && result->size() <= MaximumBytes;
}

bool planningSnapshotTree(const QVariant& value, int depth, int* nodes, qint64* amount, bool exactNumbers = false)
{
    if (depth > 24 || ++(*nodes) > 1048576) return false;
    if (!value.isValid() || (value.userType() == QMetaType::Nullptr && value.isNull())) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += static_cast<qint64>(it.key().size()) * 2;
            if (*amount > MaximumBytes || !planningSnapshotTree(it.value(), depth + 1, nodes, amount, exactNumbers)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > 32768) return false;
        for (const QVariant& entry : list) if (!planningSnapshotTree(entry, depth + 1, nodes, amount, exactNumbers)) return false;
        return true;
    }
    case QVariant::StringList: {
        const QStringList list = value.toStringList();
        if (list.size() > 32768) return false;
        for (const QString& entry : list) if (!planningSnapshotTree(entry, depth + 1, nodes, amount, exactNumbers)) return false;
        return true;
    }
    case QVariant::String:
        *amount += static_cast<qint64>(value.toString().size()) * 2;
        return value.toString().size() <= 32768 && *amount <= MaximumBytes
            && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong:
        return value.toLongLong() >= -9007199254740991LL && value.toLongLong() <= 9007199254740991LL;
    case QVariant::ULongLong: return value.toULongLong() <= 9007199254740991ULL;
    case QVariant::Double:
        return std::isfinite(value.toDouble()) && (!exactNumbers || std::abs(value.toDouble()) <= ExactMaximum);
    default: return false;
    }
}

bool planningSnapshotBounded(const QVariantMap& map, bool exactNumbers = false)
{
    int nodes = 0; qint64 amount = 0;
    if (map.isEmpty() || !planningSnapshotTree(map, 0, &nodes, &amount, exactNumbers)) return false;
    const QByteArray bytes = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !bytes.isEmpty() && bytes.size() <= MaximumBytes;
}

QVariantMap remainderSourceRejected(const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("reason"), reason}, {QStringLiteral("sourceKind"), QStringLiteral("native-run-planning-snapshot")},
        {QStringLiteral("runId"), QStringLiteral("")}, {QStringLiteral("startupId"), QStringLiteral("")},
        {QStringLiteral("transactionEpoch"), QStringLiteral("")}, {QStringLiteral("transactionGeneration"), QStringLiteral("")},
        {QStringLiteral("deploymentId"), QStringLiteral("")}, {QStringLiteral("binding"), QVariantMap()},
        {QStringLiteral("sourceVehicleId"), 0}, {QStringLiteral("sourceTask"), QVariantMap()},
        {QStringLiteral("sourcePlan"), QVariantMap()}, {QStringLiteral("sourceMember"), QVariantMap()},
        {QStringLiteral("sourcePayload"), QVariantMap()}, {QStringLiteral("progress"), QVariantMap()},
        {QStringLiteral("readonly"), true}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap remainderCollectionRejected(const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("reason"), reason}, {QStringLiteral("sourceKind"), QStringLiteral("native-run-planning-collection")},
        {QStringLiteral("runId"), QStringLiteral("")}, {QStringLiteral("startupId"), QStringLiteral("")},
        {QStringLiteral("transactionEpoch"), QStringLiteral("")}, {QStringLiteral("transactionGeneration"), QStringLiteral("")},
        {QStringLiteral("deploymentId"), QStringLiteral("")}, {QStringLiteral("binding"), QVariantMap()},
        {QStringLiteral("sourceTask"), QVariantMap()}, {QStringLiteral("sourcePlan"), QVariantMap()},
        {QStringLiteral("vehicleIds"), QVariantList()}, {QStringLiteral("targets"), QVariantList()},
        {QStringLiteral("readonly"), true}, {QStringLiteral("flightCommandReleased"), false}};
}

bool exactPlanningFields(const QVariantMap& map, const QSet<QString>& fields)
{
    if (map.size() != fields.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    return true;
}

bool planningBool(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant field = map.value(key);
    return field.type() == QVariant::Bool && !field.isNull() && field.toBool() == expected;
}

QByteArray planningJson(const QVariant& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

bool samePlanningValue(const QVariant& left, const QVariant& right)
{
    return planningJson(left) == planningJson(right);
}

bool planningBindingValid(const QVariantMap& binding)
{
    QSet<QString> keys{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (binding.size() == 9) keys.unite({QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")});
    if (!exactPlanningFields(binding, keys) || !uuid(binding.value(QStringLiteral("storeEpoch")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId")}) {
        const QVariant field = binding.value(key);
        const QString text = field.toString();
        if (field.type() != QVariant::String || text.isEmpty() || text.size() > 128 || text != text.trimmed()) return false;
        for (const QChar character : text)
            if (character.isNull() || character.category() == QChar::Other_Control) return false;
    }
    for (const QString& key : {QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (!digest(binding.value(key))) return false;
    for (const QString& key : {QStringLiteral("recordRevision"), QStringLiteral("inputGeneration"), QStringLiteral("reviewGeneration")})
        if (binding.contains(key) && !integer(binding.value(key), key == QStringLiteral("recordRevision") ? 1.0 : 0.0, ExactMaximum)) return false;
    return true;
}

QVariantMap collectionResult(bool ok, const QString& reason, const QVariantMap& value)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason}, {QStringLiteral("collection"), value}};
}

QVariantMap collectionSource(const QVariantMap& collection, const QVariantMap& target)
{
    QVariantMap source = remainderSourceRejected(QStringLiteral(""));
    source.insert(QStringLiteral("ok"), true);
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
            QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding"),
            QStringLiteral("sourceTask"), QStringLiteral("sourcePlan")}) source.insert(key, collection.value(key));
    source.insert(QStringLiteral("sourceVehicleId"), target.value(QStringLiteral("vehicleId")));
    for (const QString& key : {QStringLiteral("sourceMember"), QStringLiteral("sourcePayload"), QStringLiteral("progress")})
        source.insert(key, target.value(key));
    return source;
}

bool collectionProgressMatches(const QVariantMap& progress, const QVariantMap& payload, int vehicleId,
    bool* available)
{
    const QVariant began = progress.value(QStringLiteral("modeQueueBegan"));
    const QVariant committed = progress.value(QStringLiteral("modeQueueCommitted"));
    const QVariantList items = payload.value(QStringLiteral("items")).toList();
    const QVariantList mapping = payload.value(QStringLiteral("sourceItemMap")).toList();
    if (began.type() != QVariant::Bool || began.isNull() || committed.type() != QVariant::Bool || committed.isNull()
        || (committed.toBool() && !began.toBool()) || !integer(progress.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(progress.value(QStringLiteral("componentId")), 1, 255)
        || !integer(progress.value(QStringLiteral("itemCount")), items.size(), items.size())
        || !uuid(progress.value(QStringLiteral("modeStepId")))
        || progress.value(QStringLiteral("payloadContentHash")) != payload.value(QStringLiteral("contentHash"))
        || progress.value(QStringLiteral("sourceCurrent")).type() != QVariant::Bool
        || progress.value(QStringLiteral("sourceCurrent")).isNull()
        || !planningBool(progress, QStringLiteral("progressObservationsPostCommitOnly"), true)
        || !planningBool(progress, QStringLiteral("unknownTail"), true)
        || !planningBool(progress, QStringLiteral("currentValuesFreshnessVerified"), false)
        || !planningBool(progress, QStringLiteral("valuesAreLastObservedOnly"), true)) return false;
    if (began.toBool()) {
        if (!uuid(progress.value(QStringLiteral("sourceEpoch"))) || !decimal(progress.value(QStringLiteral("modeQueueOperationId")))
            || !digest(progress.value(QStringLiteral("modeWirePayloadHash")))) return false;
    } else {
        for (const QString& key : {QStringLiteral("sourceEpoch"), QStringLiteral("modeQueueOperationId"), QStringLiteral("modeWirePayloadHash")})
            if (progress.value(key).type() != QVariant::String || !progress.value(key).toString().isEmpty()) return false;
    }
    for (const QString& prefix : {QStringLiteral("current"), QStringLiteral("reached")}) {
        const QString sequenceKey = prefix + QStringLiteral("Sequence"), itemKey = prefix + QStringLiteral("MissionItem");
        if (progress.contains(sequenceKey) != progress.contains(itemKey)
            || (!committed.toBool() && progress.contains(sequenceKey))) return false;
        if (!progress.contains(sequenceKey)) continue;
        double sequence = 0.0;
        if (!integer(progress.value(sequenceKey), 0, mapping.size() - 1, &sequence)
            || progress.value(itemKey).type() != QVariant::Map
            || !samePlanningValue(progress.value(itemKey), mapping.at(int(sequence)))) return false;
    }
    *available = began.toBool() && committed.toBool();
    return true;
}
QVariantMap nativeAuthorityReference(const QVariantMap& actualContext, bool groundRestart)
{
    // Only the factory's freshly fetched, fully checked actual authority
    // context enters here. The summary is historical provenance, not an
    // authentication proof; never reconstruct it from a terminal caller map.
    double schema = 0;
    if (actualContext.size() != (groundRestart ? 27 : 25)
        || !integer(actualContext.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || !uuid(actualContext.value(QStringLiteral("authorityEpoch")))
        || !digest(actualContext.value(QStringLiteral("sourceHash")))) return {};
    QVariantMap sourceContext = actualContext;
    sourceContext.remove(QStringLiteral("remainingLifetimeMs"));
    QByteArray bytes;
    if (!encode(sourceContext, &bytes)) return {};
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("authorityEpoch"), actualContext.value(QStringLiteral("authorityEpoch"))},
        {QStringLiteral("sourceHash"), actualContext.value(QStringLiteral("sourceHash"))},
        {QStringLiteral("sourceContextHash"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())},
        {QStringLiteral("scope"), groundRestart ? QStringLiteral("ground-reassessment-authority-reference-not-permission")
            : QStringLiteral("pre-arm-authority-reference-not-permission")},
        {QStringLiteral("previousAuthorizationRestored"), false}};
}
bool nativeAuthorityReferenceShape(const QVariantMap& reference, bool groundRestart)
{
    double schema = 0;
    return reference.size() == 6 && integer(reference.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        && uuid(reference.value(QStringLiteral("authorityEpoch"))) && digest(reference.value(QStringLiteral("sourceHash")))
        && digest(reference.value(QStringLiteral("sourceContextHash")))
        && reference.value(QStringLiteral("scope")).type() == QVariant::String
        && reference.value(QStringLiteral("scope")).toString() == (groundRestart
            ? QStringLiteral("ground-reassessment-authority-reference-not-permission")
            : QStringLiteral("pre-arm-authority-reference-not-permission"))
        && isFalse(reference.value(QStringLiteral("previousAuthorizationRestored")));
}
QObject* actualObject(const QVariant& value) { return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr; }
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result)
{
    const QString task = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(task), record = store->record(task, binding.value(QStringLiteral("recordRevision")));
    if (!isFalse(latest.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (!binding.contains(key) || binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return encode(record, result);
}
bool validRole(const QVariantMap& identity, const QVariantMap& role)
{
    const QString primary = identity.value(QStringLiteral("role")).toString();
    return (primary == QStringLiteral("operator") || primary == QStringLiteral("commander") || primary == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && isTrue(role.value(QStringLiteral("allowed"))) && isTrue(role.value(QStringLiteral("requiresSecondApproval")))
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")))
        && isTrue(identity.value(QStringLiteral("authenticated")));
}
QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executionVerified"), false}, {QStringLiteral("rollbackPerformed"), false}};
}
const QStringList& stepNames()
{
    static const QStringList names = {QStringLiteral("Seq0"), QStringLiteral("NormalArm"), QStringLiteral("MissionModeOnly")};
    return names;
}
} // namespace

FleetMissionStartupTransaction::FleetMissionStartupTransaction(QObject* parent)
    : QObject(parent), _transactionEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(PaceMs);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionStartupTransaction::_tick);
}
FleetMissionStartupTransaction::~FleetMissionStartupTransaction()
{
    _active = false; _state = QStringLiteral("cancelled");
    _retirePhaseMemorySeals();
    _disconnectSources(); _detachDrain();
}
bool FleetMissionStartupTransaction::_live(quint64 generation) const
{
    return generation != 0 && generation == _generation && _clock.elapsed() < _expiresAtMs
        && (_state == QStringLiteral("preparing") || (_active && _state == QStringLiteral("running")));
}
bool FleetMissionStartupTransaction::_same(quint64 generation) const
{
    return _live(generation) && outboundGroup(this, {_session.data(), _deployment.data(), _groundRestart
        ? static_cast<QObject*>(_groundAuthority.data()) : static_cast<QObject*>(_authority.data())})
        && _session && _roles && _store
        && (_groundRestart ? bool(_groundAuthority) && !_authority : bool(_authority) && !_groundAuthority)
        && _deployment && _swarm && _health && _authorization && _coordinator && _catalog;
}
QVariantMap FleetMissionStartupTransaction::_bundle() const
{
    const auto object = [](QObject* source) { return QVariant::fromValue(source); };
    return {{QStringLiteral("session"), object(_session.data())}, {QStringLiteral("roles"), object(_roles.data())},
        {QStringLiteral("store"), object(_store.data())}, {QStringLiteral("authority"), object(_groundRestart
            ? static_cast<QObject*>(_groundAuthority.data()) : static_cast<QObject*>(_authority.data()))},
        {QStringLiteral("deployment"), object(_deployment.data())}, {QStringLiteral("swarm"), object(_swarm.data())},
        {QStringLiteral("health"), object(_health.data())}, {QStringLiteral("authorizationOwner"), object(_authorization.data())}};
}
QVariantMap FleetMissionStartupTransaction::current() const
{
    QVariantList targets;
    bool anyQueued = false;
    for (const Target& target : _targets) {
        QVariantList steps;
        for (int step = 0; step < 3; ++step) {
            anyQueued = anyQueued || target.queued[step];
            steps.append(QVariantMap{{QStringLiteral("step"), stepNames().at(step)}, {QStringLiteral("stepId"), target.stepIds.value(step)},
                {QStringLiteral("skippedReviewedAlreadyArmed"), step == 1 && _groundRestart && target.reviewedArmed},
                {QStringLiteral("requestIssued"), target.queued[step]}, {QStringLiteral("rawStateObserved"), target.observed[step]},
                {QStringLiteral("mayHaveCommitted"), target.uncertain[step] || target.queued[step]},
                {QStringLiteral("requestCausalityVerified"), false}});
        }
        targets.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("state"), target.state},
            {QStringLiteral("reason"), target.reason}, {QStringLiteral("steps"), steps}, {QStringLiteral("observations"), target.stepResults}});
    }
    return {{QStringLiteral("ok"), (_active && _live(_generation) && _reservationHeld) || (_state == QStringLiteral("started-observed") && !_reservationHeld && !_transportDraining)}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("deploymentId"), _authorityContext.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("transactionEpoch"), _transactionEpoch}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("sourceHash"), _sourceHash}, {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason},
        {QStringLiteral("groundReassessmentStartup"), _groundRestart}, {QStringLiteral("previousAuthorizationRestored"), false},
        {QStringLiteral("active"), _active}, {QStringLiteral("reservationHeld"), _reservationHeld}, {QStringLiteral("transportDraining"), _transportDraining},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), targets},
        {QStringLiteral("stage"), _stage >= 0 && _stage < 3 ? stepNames().at(_stage) : QStringLiteral("terminal")},
        {QStringLiteral("remainingLifetimeMs"), static_cast<double>(qMax<qint64>(0, _expiresAtMs - _clock.elapsed()))},
        {QStringLiteral("commandsMayHaveCommitted"), anyQueued}, {QStringLiteral("missionStateMayHaveChanged"), anyQueued},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("scopedDispatchAuthorizationHeld"), _active && _live(_generation) && _reservationHeld},
        {QStringLiteral("flightCommandReleased"), _anyFlightCommandQueued}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("missionCompleted"), false}, {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("authenticatedAircraft"), false},
        {QStringLiteral("rollbackPerformed"), false}, {QStringLiteral("automaticDisarmPerformed"), false},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("verificationScope"), QStringLiteral("new-raw-seq0-armed-ground-posctl-and-armed-auto-mission-mode-not-execution")}};
}
QVariantMap FleetMissionStartupTransaction::executionObservation() const
{
    return _executionObservation ? _executionObservation->current() : QVariantMap();
}

QVariantMap FleetMissionStartupTransaction::validateCollection(const QVariantMap& collection)
{
    const auto reject = [](const QString& reason) { return collectionResult(false, reason, {}); };
    const QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("ok"), QStringLiteral("reason"),
        QStringLiteral("sourceKind"), QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
        QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding"),
        QStringLiteral("sourceTask"), QStringLiteral("sourcePlan"), QStringLiteral("vehicleIds"), QStringLiteral("targets"),
        QStringLiteral("readonly"), QStringLiteral("flightCommandReleased")};
    if (!exactPlanningFields(collection, fields) || !planningSnapshotBounded(collection, true)
        || !integer(collection.value(QStringLiteral("schemaVersion")), 1, 1)
        || !planningBool(collection, QStringLiteral("ok"), true)
        || !planningBool(collection, QStringLiteral("readonly"), true)
        || !planningBool(collection, QStringLiteral("flightCommandReleased"), false)
        || collection.value(QStringLiteral("reason")).type() != QVariant::String
        || !collection.value(QStringLiteral("reason")).toString().isEmpty()
        || collection.value(QStringLiteral("sourceKind")) != QVariant(QStringLiteral("native-run-planning-collection"))
        || !decimal(collection.value(QStringLiteral("transactionGeneration"))))
        return reject(QStringLiteral("planning-collection-invalid"));
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("deploymentId")})
        if (!uuid(collection.value(key))) return reject(QStringLiteral("planning-collection-identity-invalid"));
    for (const QString& key : {QStringLiteral("binding"), QStringLiteral("sourceTask"), QStringLiteral("sourcePlan")})
        if (collection.value(key).type() != QVariant::Map || collection.value(key).toMap().isEmpty())
            return reject(QStringLiteral("planning-collection-source-missing"));
    if (collection.value(QStringLiteral("vehicleIds")).type() != QVariant::List
        || collection.value(QStringLiteral("targets")).type() != QVariant::List)
        return reject(QStringLiteral("planning-collection-targets-invalid"));
    const QVariantList ids = collection.value(QStringLiteral("vehicleIds")).toList();
    const QVariantList targets = collection.value(QStringLiteral("targets")).toList();
    const QVariantMap binding = collection.value(QStringLiteral("binding")).toMap();
    const QVariantMap plan = collection.value(QStringLiteral("sourcePlan")).toMap();
    const QString planHash = QString::fromLatin1(QCryptographicHash::hash(planningJson(plan), QCryptographicHash::Sha256).toHex());
    if (!planningBindingValid(binding) || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId"))
        || plan.value(QStringLiteral("sourceTaskId")) != binding.value(QStringLiteral("sourceTaskId"))
        || planHash != binding.value(QStringLiteral("contentHash")).toString()
        || plan.value(QStringLiteral("members")).type() != QVariant::List)
        return reject(QStringLiteral("planning-collection-binding-mismatch"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (ids.isEmpty() || ids.size() > 32 || targets.size() != ids.size() || members.size() != ids.size())
        return reject(QStringLiteral("planning-collection-targets-invalid"));
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(
            collection.value(QStringLiteral("sourceTask")).toMap().value(QStringLiteral("area")), members);
        if (!planningBool(boundary, QStringLiteral("ok"), true)
            || !samePlanningValue(boundary.value(QStringLiteral("policy")), plan.value(QStringLiteral("routeBoundaryPolicy"))))
            return reject(QStringLiteral("planning-collection-boundary-policy-mismatch"));
    }
    // One whole-plan encoding for the whole group; never N single-source
    // reconstructions or a successful-prefix collection.
    const QVariantMap rebuilt = FleetMissionPayloadBuilder::reproduceHistoricalPayloads(plan);
    if (!planningBool(rebuilt, QStringLiteral("ok"), true) || rebuilt.value(QStringLiteral("payloads")).type() != QVariant::List)
        return reject(QStringLiteral("planning-collection-payload-rebuild-unavailable"));
    const QVariantList payloads = rebuilt.value(QStringLiteral("payloads")).toList();
    if (payloads.size() != ids.size()) return reject(QStringLiteral("planning-collection-payload-mismatch"));
    QMap<int, QVariantMap> membersById, payloadsById;
    const auto collectById = [](const QVariantList& values, QMap<int, QVariantMap>* result) {
        for (const QVariant& value : values) {
            double number = 0.0;
            if (value.type() != QVariant::Map || !integer(value.toMap().value(QStringLiteral("vehicleId")), 1, 255, &number)
                || result->contains(int(number))) return false;
            result->insert(int(number), value.toMap());
        }
        return true;
    };
    if (!collectById(members, &membersById) || !collectById(payloads, &payloadsById))
        return reject(QStringLiteral("planning-collection-payload-mismatch"));
    const QSet<QString> rowFields{QStringLiteral("vehicleId"), QStringLiteral("available"), QStringLiteral("reason"),
        QStringLiteral("sourceMember"), QStringLiteral("sourcePayload"), QStringLiteral("progress")};
    QSet<int> seen;
    int totalItems = 0;
    for (int index = 0; index < ids.size(); ++index) {
        double numericId = 0.0;
        if (!integer(ids.at(index), 1, 255, &numericId) || seen.contains(int(numericId))
            || targets.at(index).type() != QVariant::Map) return reject(QStringLiteral("planning-collection-targets-invalid"));
        const int id = int(numericId);
        seen.insert(id);
        const QVariantMap target = targets.at(index).toMap();
        if (!exactPlanningFields(target, rowFields) || !integer(target.value(QStringLiteral("vehicleId")), id, id)
            || target.value(QStringLiteral("available")).type() != QVariant::Bool || target.value(QStringLiteral("available")).isNull()
            || target.value(QStringLiteral("reason")).type() != QVariant::String
            || target.value(QStringLiteral("sourceMember")).type() != QVariant::Map
            || target.value(QStringLiteral("sourcePayload")).type() != QVariant::Map
            || target.value(QStringLiteral("progress")).type() != QVariant::Map
            || !membersById.contains(id) || !payloadsById.contains(id)
            || !samePlanningValue(target.value(QStringLiteral("sourceMember")), membersById.value(id))
            || !samePlanningValue(target.value(QStringLiteral("sourcePayload")), payloadsById.value(id)))
            return reject(QStringLiteral("planning-collection-payload-mismatch"));
        const QVariantMap payload = target.value(QStringLiteral("sourcePayload")).toMap();
        const QVariantList items = payload.value(QStringLiteral("items")).toList();
        if (payload.value(QStringLiteral("items")).type() != QVariant::List || items.isEmpty() || items.size() > 2048
            || payload.value(QStringLiteral("sourceItemMap")).type() != QVariant::List
            || payload.value(QStringLiteral("sourceItemMap")).toList().size() != items.size()
            || !digest(payload.value(QStringLiteral("contentHash")))
            || totalItems > 8192 - items.size()) return reject(QStringLiteral("planning-collection-item-budget-invalid"));
        totalItems += items.size();
        bool available = false;
        if (!collectionProgressMatches(target.value(QStringLiteral("progress")).toMap(), payload, id, &available)
            || target.value(QStringLiteral("available")).toBool() != available
            || target.value(QStringLiteral("reason")).toString() != (available ? QStringLiteral("")
                : QStringLiteral("original-target-mode-not-committed")))
            return reject(QStringLiteral("planning-collection-progress-mismatch"));
    }
    const QByteArray bytes = planningJson(collection);
    const QJsonDocument document = QJsonDocument::fromJson(bytes);
    const QVariantMap owned = document.toVariant().toMap();
    if (!document.isObject() || planningJson(owned) != bytes)
        return reject(QStringLiteral("planning-collection-snapshot-invalid"));
    return collectionResult(true, QStringLiteral(""), owned);
}

QVariantMap FleetMissionStartupTransaction::sourceFromCollection(const QVariantMap& collection, int sourceVehicleId)
{
    const auto reject = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("source"), QVariantMap()}};
    };
    const QVariantMap checked = validateCollection(collection);
    if (!planningBool(checked, QStringLiteral("ok"), true)) return reject(checked.value(QStringLiteral("reason")).toString());
    if (sourceVehicleId < 1 || sourceVehicleId > 255) return reject(QStringLiteral("planning-collection-source-vehicle-invalid"));
    const QVariantMap canonical = checked.value(QStringLiteral("collection")).toMap();
    for (const QVariant& value : canonical.value(QStringLiteral("targets")).toList()) {
        const QVariantMap target = value.toMap();
        if (!integer(target.value(QStringLiteral("vehicleId")), sourceVehicleId, sourceVehicleId)) continue;
        if (!planningBool(target, QStringLiteral("available"), true)) return reject(target.value(QStringLiteral("reason")).toString());
        return FleetMissionRemainderPlanner::validateSource(collectionSource(canonical, target));
    }
    return reject(QStringLiteral("planning-collection-source-vehicle-not-found"));
}

QVariantMap FleetMissionStartupTransaction::remainderPlanningSource(const QString& expectedRunId,
    int sourceVehicleId) const
{
    if (QThread::currentThread() != thread() || _remainderSourceInProgress
        || !uuid(expectedRunId) || sourceVehicleId < 1 || sourceVehicleId > 255)
        return remainderSourceRejected(QStringLiteral("original-run-and-source-vehicle-required"));
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    _remainderSourceInProgress = true;
    FlagGuard reading{QPointer<QObject>(self.data()), &_remainderSourceInProgress};
    const QPointer<FleetMissionExecutionObservation> observer(_executionObservation);
    const quint64 generation = _generation;
    const QString startupId = _startupId, epoch = _transactionEpoch;
    const QVariantMap binding = _binding, sourceTask = _planningTask, deploymentContext = _deploymentContext;
    const QVariantList vehicleIds = _vehicleIds;
    const QVariantMap plan = deploymentContext.value(QStringLiteral("plan")).toMap();
    const QString deploymentId = _authorityContext.value(QStringLiteral("deploymentId")).toString();
    if (!observer || observer->thread() != thread() || !observer->_prepared || observer->_ownerDestroying
        || observer->_startup != self || observer->_runId != expectedRunId
        || observer->_startupId != startupId || observer->_startupEpoch != epoch
        || observer->_startupGeneration != generation || observer->_binding != binding
        || observer->_vehicleIds != vehicleIds || observer->_deploymentId != deploymentId
        || generation == 0 || !uuid(startupId) || !uuid(epoch) || !uuid(deploymentId)
        || plan.isEmpty() || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId")))
        return remainderSourceRejected(QStringLiteral("actual-prepared-run-identity-required"));
    if (sourceTask.isEmpty())
        return remainderSourceRejected(QStringLiteral("original-planning-task-unavailable"));

    Target target;
    FleetMissionExecutionObservation::Target observedTarget;
    int targetMatches = 0, observedMatches = 0;
    for (const Target& row : _targets) if (row.vehicleId == sourceVehicleId) { target = row; ++targetMatches; }
    for (const auto& row : observer->_targets) if (row.vehicleId == sourceVehicleId) { observedTarget = row; ++observedMatches; }
    const QVariantList items = target.payload.value(QStringLiteral("items")).toList();
    const QVariantList sourceItems = target.payload.value(QStringLiteral("sourceItemMap")).toList();
    if (targetMatches != 1 || observedMatches != 1 || !observedTarget.queueBegan || !observedTarget.queueCommitted
        || target.componentId != observedTarget.componentId || target.stepIds.value(2).toString() != observedTarget.modeStepId
        || target.readReceipt != observedTarget.readReceipt || items.isEmpty() || items.size() > 2048
        || sourceItems.size() != items.size() || observedTarget.itemCount != items.size()
        || observedTarget.sources != sourceItems || !digest(target.payload.value(QStringLiteral("contentHash")))
        || observedTarget.payloadHash != target.payload.value(QStringLiteral("contentHash")).toString()
        || !uuid(observedTarget.queue.sourceEpoch) || !decimal(observedTarget.queue.operationId))
        return remainderSourceRejected(QStringLiteral("actual-committed-target-payload-required"));

    // Only immutable identities are compared after external native getters.
    // Retired/lost links and advancing observation counters are historical
    // planning facts, not reasons to invent a new source or renew authority.
    const auto unchanged = [&]() {
        if (!self || !observer || _executionObservation != observer || _generation != generation
            || _startupId != startupId || _transactionEpoch != epoch || _binding != binding
            || _planningTask != sourceTask || _deploymentContext != deploymentContext || _vehicleIds != vehicleIds
            || _authorityContext.value(QStringLiteral("deploymentId")).toString() != deploymentId
            || observer->_startup != self || !observer->_prepared || observer->_ownerDestroying
            || observer->_runId != expectedRunId || observer->_startupId != startupId || observer->_startupEpoch != epoch
            || observer->_startupGeneration != generation || observer->_binding != binding
            || observer->_vehicleIds != vehicleIds || observer->_deploymentId != deploymentId) return false;
        int ownMatches = 0, runMatches = 0;
        for (const Target& row : _targets) if (row.vehicleId == sourceVehicleId) {
            ++ownMatches;
            if (row.componentId != target.componentId || row.member != target.member || row.payload != target.payload
                || row.readReceipt != target.readReceipt || row.stepIds != target.stepIds) return false;
        }
        for (const auto& row : observer->_targets) if (row.vehicleId == sourceVehicleId) {
            ++runMatches;
            if (!row.queueBegan || !row.queueCommitted || row.componentId != observedTarget.componentId
                || row.itemCount != observedTarget.itemCount || row.sources != sourceItems || row.payloadHash != observedTarget.payloadHash
                || row.readReceipt != observedTarget.readReceipt
                || row.modeStepId != observedTarget.modeStepId || row.queue.sourceEpoch != observedTarget.queue.sourceEpoch
                || row.queue.operationId != observedTarget.queue.operationId || row.queue.wirePayloadHash != observedTarget.queue.wirePayloadHash)
                return false;
        }
        return ownMatches == 1 && runMatches == 1;
    };
    QVariantMap value = remainderSourceRejected(QStringLiteral(""));
    value.insert(QStringLiteral("runId"), expectedRunId); value.insert(QStringLiteral("startupId"), startupId);
    value.insert(QStringLiteral("transactionEpoch"), epoch); value.insert(QStringLiteral("transactionGeneration"), QString::number(generation));
    value.insert(QStringLiteral("deploymentId"), deploymentId); value.insert(QStringLiteral("binding"), binding);
    value.insert(QStringLiteral("sourceVehicleId"), sourceVehicleId); value.insert(QStringLiteral("sourceTask"), sourceTask);
    value.insert(QStringLiteral("sourcePlan"), plan); value.insert(QStringLiteral("sourceMember"), target.member);
    value.insert(QStringLiteral("sourcePayload"), target.payload);
    if (!planningSnapshotBounded(value))
        return remainderSourceRejected(QStringLiteral("planning-source-not-bounded-pure-values"));

    int memberMatches = 0;
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (plan.value(QStringLiteral("members")).type() != QVariant::List || members.isEmpty() || members.size() > 32)
        return remainderSourceRejected(QStringLiteral("original-compiled-members-required"));
    if (plan.contains(QStringLiteral("routeBoundaryPolicy"))) {
        const QVariantMap boundary = FleetAreaCoveragePlanner::buildRouteBoundaryPolicy(sourceTask.value(QStringLiteral("area")), members);
        if (!isTrue(boundary.value(QStringLiteral("ok")))
            || !samePlanningValue(boundary.value(QStringLiteral("policy")), plan.value(QStringLiteral("routeBoundaryPolicy"))))
            return remainderSourceRejected(QStringLiteral("original-planning-boundary-policy-mismatch"));
    }
    for (const QVariant& entry : members) {
        if (entry.type() != QVariant::Map) return remainderSourceRejected(QStringLiteral("original-compiled-members-required"));
        const QVariantMap member = entry.toMap();
        if (integer(member.value(QStringLiteral("vehicleId")), sourceVehicleId, sourceVehicleId)) {
            if (member != target.member) return remainderSourceRejected(QStringLiteral("original-member-payload-mismatch"));
            ++memberMatches;
        }
    }
    if (memberMatches != 1) return remainderSourceRejected(QStringLiteral("unique-original-compiled-member-required"));
    // Pure re-encoding proves only that these frozen values still describe
    // the originally built payload; it does not re-read an aircraft mission.
    const QVariantMap rebuilt = FleetMissionPayloadBuilder::reproduceHistoricalPayloads(plan);
    int payloadMatches = 0;
    if (!isTrue(rebuilt.value(QStringLiteral("ok"))))
        return remainderSourceRejected(QStringLiteral("original-plan-payload-rebuild-unavailable"));
    for (const QVariant& entry : rebuilt.value(QStringLiteral("payloads")).toList()) {
        const QVariantMap payload = entry.toMap();
        if (integer(payload.value(QStringLiteral("vehicleId")), sourceVehicleId, sourceVehicleId)) {
            if (payload != target.payload) return remainderSourceRejected(QStringLiteral("original-plan-payload-mismatch"));
            ++payloadMatches;
        }
    }
    if (payloadMatches != 1 || !unchanged())
        return remainderSourceRejected(QStringLiteral("original-planning-source-changed"));

    // current() may consult native source objects. Do not retain references
    // into either owner's target list across that call; tail-check ownership.
    const QVariantMap run = observer->current();
    if (!unchanged() || !isTrue(run.value(QStringLiteral("prepared")))
        || run.value(QStringLiteral("runId")) != expectedRunId || run.value(QStringLiteral("startupId")) != startupId
        || run.value(QStringLiteral("transactionEpoch")) != epoch
        || run.value(QStringLiteral("transactionGeneration")) != QString::number(generation)
        || run.value(QStringLiteral("binding")).toMap() != binding || run.value(QStringLiteral("deploymentId")) != deploymentId)
        return remainderSourceRejected(QStringLiteral("original-run-changed-during-planning-capture"));
    QVariantMap progress;
    int progressMatches = 0;
    for (const QVariant& entry : run.value(QStringLiteral("vehicles")).toList()) {
        const QVariantMap row = entry.toMap();
        if (integer(row.value(QStringLiteral("vehicleId")), sourceVehicleId, sourceVehicleId)) { progress = row; ++progressMatches; }
    }
    if (progressMatches != 1 || !isTrue(progress.value(QStringLiteral("modeQueueBegan")))
        || !isTrue(progress.value(QStringLiteral("modeQueueCommitted")))
        || !integer(progress.value(QStringLiteral("componentId")), target.componentId, target.componentId)
        || !integer(progress.value(QStringLiteral("itemCount")), items.size(), items.size())
        || progress.value(QStringLiteral("payloadContentHash")) != target.payload.value(QStringLiteral("contentHash"))
        || progress.value(QStringLiteral("sourceEpoch")) != observedTarget.queue.sourceEpoch
        || progress.value(QStringLiteral("modeQueueOperationId")) != observedTarget.queue.operationId
        || progress.value(QStringLiteral("modeStepId")) != observedTarget.modeStepId)
        return remainderSourceRejected(QStringLiteral("actual-progress-payload-identity-mismatch"));
    for (const QString& prefix : {QStringLiteral("current"), QStringLiteral("reached")}) {
        const QString sequenceKey = prefix + QStringLiteral("Sequence"), itemKey = prefix + QStringLiteral("MissionItem");
        if (progress.contains(sequenceKey) != progress.contains(itemKey))
            return remainderSourceRejected(QStringLiteral("actual-progress-source-mapping-incomplete"));
        if (!progress.contains(sequenceKey)) continue;
        double sequence = 0;
        if (!integer(progress.value(sequenceKey), 0, sourceItems.size() - 1, &sequence)
            || progress.value(itemKey).type() != QVariant::Map || progress.value(itemKey) != sourceItems.at(int(sequence)))
            return remainderSourceRejected(QStringLiteral("actual-progress-source-mapping-mismatch"));
    }
    value.insert(QStringLiteral("progress"), progress);
    value.insert(QStringLiteral("ok"), true);
    if (!planningSnapshotBounded(value) || !unchanged())
        return remainderSourceRejected(QStringLiteral("planning-source-resource-or-owner-changed"));
    return value;
}

QVariantMap FleetMissionStartupTransaction::remainderPlanningCollection(const QString& expectedRunId) const
{
    // Freeze the caller's string before any native observation getter.
    const QString runId = expectedRunId;
    if (QThread::currentThread() != thread() || _remainderSourceInProgress || !uuid(runId))
        return remainderCollectionRejected(QStringLiteral("original-run-required"));
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    _remainderSourceInProgress = true;
    FlagGuard reading{QPointer<QObject>(self.data()), &_remainderSourceInProgress};
    const QPointer<FleetMissionExecutionObservation> observer(_executionObservation);
    const quint64 generation = _generation;
    const QString startupId = _startupId, epoch = _transactionEpoch;
    const QVariantMap binding = _binding, sourceTask = _planningTask, deploymentContext = _deploymentContext;
    const QVariantList vehicleIds = _vehicleIds;
    const QVariantMap plan = deploymentContext.value(QStringLiteral("plan")).toMap();
    const QString deploymentId = _authorityContext.value(QStringLiteral("deploymentId")).toString();
    if (!observer || observer->thread() != thread() || !observer->_prepared || observer->_ownerDestroying
        || observer->_startup != self || observer->_runId != runId || observer->_startupId != startupId
        || observer->_startupEpoch != epoch || observer->_startupGeneration != generation
        || observer->_binding != binding || observer->_vehicleIds != vehicleIds || observer->_deploymentId != deploymentId
        || generation == 0 || !uuid(startupId) || !uuid(epoch) || !uuid(deploymentId)
        || vehicleIds.isEmpty() || vehicleIds.size() > 32 || _targets.size() != vehicleIds.size()
        || observer->_targets.size() != vehicleIds.size() || plan.isEmpty()
        || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId")))
        return remainderCollectionRejected(QStringLiteral("actual-prepared-run-identity-required"));
    if (sourceTask.isEmpty()) return remainderCollectionRejected(QStringLiteral("original-planning-task-unavailable"));

    // Local copies, never references into a native target container across
    // observer.current(). All queue identities/flags belong to this freeze.
    const QList<Target> originalTargets = _targets;
    const QList<FleetMissionExecutionObservation::Target> originalObservedTargets = observer->_targets;
    QVariantList rows;
    QSet<int> seen;
    int totalItems = 0;
    for (int index = 0; index < vehicleIds.size(); ++index) {
        const Target& target = originalTargets.at(index);
        const auto& observed = originalObservedTargets.at(index);
        const QVariantList items = target.payload.value(QStringLiteral("items")).toList();
        const QVariantList mapping = target.payload.value(QStringLiteral("sourceItemMap")).toList();
        if (!integer(vehicleIds.at(index), target.vehicleId, target.vehicleId) || target.vehicleId < 1 || target.vehicleId > 255
            || seen.contains(target.vehicleId) || observed.vehicleId != target.vehicleId
            || target.componentId != observed.componentId || target.componentId < 1 || target.componentId > 255
            || target.stepIds.value(2).toString() != observed.modeStepId || !uuid(observed.modeStepId)
            || target.readReceipt != observed.readReceipt || items.isEmpty() || items.size() > 2048
            || totalItems > 8192 - items.size() || mapping.size() != items.size() || observed.itemCount != items.size()
            || observed.sources != mapping || !digest(target.payload.value(QStringLiteral("contentHash")))
            || observed.payloadHash != target.payload.value(QStringLiteral("contentHash")).toString()
            || (observed.queueCommitted && !observed.queueBegan)
            || (observed.queueBegan && (!uuid(observed.queue.sourceEpoch) || !decimal(observed.queue.operationId)
                || !digest(observed.queue.wirePayloadHash))))
            return remainderCollectionRejected(QStringLiteral("original-target-payload-required"));
        seen.insert(target.vehicleId);
        totalItems += items.size();
        const bool available = observed.queueBegan && observed.queueCommitted;
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("available"), available},
            {QStringLiteral("reason"), available ? QStringLiteral("") : QStringLiteral("original-target-mode-not-committed")},
            {QStringLiteral("sourceMember"), target.member}, {QStringLiteral("sourcePayload"), target.payload},
            {QStringLiteral("progress"), QVariantMap()}});
    }
    const auto unchanged = [&]() {
        if (!self || !observer || self->thread() != QThread::currentThread() || observer->thread() != QThread::currentThread()
            || _executionObservation != observer || _generation != generation || _startupId != startupId || _transactionEpoch != epoch
            || _binding != binding || _planningTask != sourceTask || _deploymentContext != deploymentContext || _vehicleIds != vehicleIds
            || _authorityContext.value(QStringLiteral("deploymentId")).toString() != deploymentId
            || _targets.size() != originalTargets.size() || observer->_targets.size() != originalObservedTargets.size()
            || observer->_startup != self || !observer->_prepared || observer->_ownerDestroying
            || observer->_runId != runId || observer->_startupId != startupId || observer->_startupEpoch != epoch
            || observer->_startupGeneration != generation || observer->_binding != binding
            || observer->_vehicleIds != vehicleIds || observer->_deploymentId != deploymentId) return false;
        for (int index = 0; index < originalTargets.size(); ++index) {
            const Target& original = originalTargets.at(index);
            const Target& actual = _targets.at(index);
            const auto& saved = originalObservedTargets.at(index);
            const auto& actualObserved = observer->_targets.at(index);
            if (actual.vehicleId != original.vehicleId || actual.componentId != original.componentId
                || actual.member != original.member || actual.payload != original.payload
                || actual.readReceipt != original.readReceipt || actual.stepIds != original.stepIds
                || actualObserved.vehicleId != saved.vehicleId || actualObserved.componentId != saved.componentId
                || actualObserved.itemCount != saved.itemCount || actualObserved.sources != saved.sources
                || actualObserved.payloadHash != saved.payloadHash || actualObserved.readReceipt != saved.readReceipt
                || actualObserved.modeStepId != saved.modeStepId || actualObserved.queueBegan != saved.queueBegan
                || actualObserved.queueCommitted != saved.queueCommitted || actualObserved.queue.sourceEpoch != saved.queue.sourceEpoch
                || actualObserved.queue.operationId != saved.queue.operationId || actualObserved.queue.wirePayloadHash != saved.queue.wirePayloadHash)
                return false;
        }
        return true;
    };
    QVariantMap collection = remainderCollectionRejected(QStringLiteral(""));
    collection.insert(QStringLiteral("ok"), true);
    collection.insert(QStringLiteral("runId"), runId); collection.insert(QStringLiteral("startupId"), startupId);
    collection.insert(QStringLiteral("transactionEpoch"), epoch); collection.insert(QStringLiteral("transactionGeneration"), QString::number(generation));
    collection.insert(QStringLiteral("deploymentId"), deploymentId); collection.insert(QStringLiteral("binding"), binding);
    collection.insert(QStringLiteral("sourceTask"), sourceTask); collection.insert(QStringLiteral("sourcePlan"), plan);
    collection.insert(QStringLiteral("vehicleIds"), vehicleIds); collection.insert(QStringLiteral("targets"), rows);
    if (!planningSnapshotBounded(collection, true) || !unchanged())
        return remainderCollectionRejected(QStringLiteral("planning-collection-source-not-bounded-or-changed"));

    // Exactly one observation of this complete native Run, not one per row.
    const QVariantMap run = observer->current();
    if (!unchanged() || !isTrue(run.value(QStringLiteral("prepared")))
        || run.value(QStringLiteral("runId")) != runId || run.value(QStringLiteral("startupId")) != startupId
        || run.value(QStringLiteral("transactionEpoch")) != epoch
        || run.value(QStringLiteral("transactionGeneration")) != QString::number(generation)
        || run.value(QStringLiteral("binding")).toMap() != binding || run.value(QStringLiteral("deploymentId")) != deploymentId
        || !samePlanningValue(run.value(QStringLiteral("targetVehicleIds")), vehicleIds)
        || run.value(QStringLiteral("vehicles")).type() != QVariant::List
        || run.value(QStringLiteral("vehicles")).toList().size() != rows.size())
        return remainderCollectionRejected(QStringLiteral("original-run-changed-during-planning-capture"));
    const QVariantList progressRows = run.value(QStringLiteral("vehicles")).toList();
    for (int index = 0; index < rows.size(); ++index) {
        const auto& original = originalObservedTargets.at(index);
        if (progressRows.at(index).type() != QVariant::Map)
            return remainderCollectionRejected(QStringLiteral("actual-progress-payload-identity-mismatch"));
        const QVariantMap progress = progressRows.at(index).toMap();
        if (!integer(progress.value(QStringLiteral("vehicleId")), original.vehicleId, original.vehicleId)
            || !integer(progress.value(QStringLiteral("componentId")), original.componentId, original.componentId)
            || !integer(progress.value(QStringLiteral("itemCount")), original.itemCount, original.itemCount)
            || !planningBool(progress, QStringLiteral("modeQueueBegan"), original.queueBegan)
            || !planningBool(progress, QStringLiteral("modeQueueCommitted"), original.queueCommitted)
            || progress.value(QStringLiteral("payloadContentHash")) != original.payloadHash
            || progress.value(QStringLiteral("sourceEpoch")) != original.queue.sourceEpoch
            || progress.value(QStringLiteral("modeQueueOperationId")) != original.queue.operationId
            || progress.value(QStringLiteral("modeWirePayloadHash")) != original.queue.wirePayloadHash
            || progress.value(QStringLiteral("modeStepId")) != original.modeStepId)
            return remainderCollectionRejected(QStringLiteral("actual-progress-payload-identity-mismatch"));
        QVariantMap row = rows.at(index).toMap();
        row.insert(QStringLiteral("progress"), progress);
        rows[index] = row;
    }
    collection.insert(QStringLiteral("targets"), rows);
    const QVariantMap checked = validateCollection(collection);
    if (!planningBool(checked, QStringLiteral("ok"), true))
        return remainderCollectionRejected(checked.value(QStringLiteral("reason")).toString());
    if (!unchanged()) return remainderCollectionRejected(QStringLiteral("planning-collection-source-changed"));
    return checked.value(QStringLiteral("collection")).toMap();
}

VehicleMissionExecutionObserver* FleetMissionStartupTransaction::executionObserverForQueue(
    QObject* actualVehicle, QObject* actualPrimary, const QString& suppliedStartupId, const QString& suppliedModeStepId) const
{
    const QString startupId = suppliedStartupId, modeStepId = suppliedModeStepId;
    if (!_executionObservation || _executionObservationRetired
        || !commandContextCurrent(actualVehicle, actualPrimary, startupId, modeStepId, 2)) return nullptr;
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    const QPointer<FleetMissionExecutionObservation> observer(_executionObservation);
    const QPointer<QObject> vehicle(actualVehicle), primary(actualPrimary);
    const quint64 generation = _generation;
    const QString epoch = _transactionEpoch;
    const int sendingIndex = _sendingIndex;
    const qint64 expires = _expiresAtMs, stageDeadline = _stageDeadlineMs;
    const QList<Target> targets = _targets;
    const auto same = [&]() {
        return self && observer && vehicle && primary && self->_executionObservation == observer && !self->_executionObservationRetired
            && self->_generation == generation && self->_transactionEpoch == epoch && self->_startupId == startupId
            && self->_sendingIndex == sendingIndex && self->_expiresAtMs == expires && self->_stageDeadlineMs == stageDeadline
            && self->commandContextCurrent(vehicle.data(), primary.data(), startupId, modeStepId, 2);
    };
    for (const Target& target : targets) {
        if (target.vehicle.data() != actualVehicle || target.primary.data() != actualPrimary) continue;
        const bool ready = observer->readyForQueue(target.vehicle.data(), target.primary.data(), startupId, modeStepId);
        if (!same() || !ready) return nullptr;
        return observer.data();
    }
    return nullptr;
}

QVariantMap FleetMissionStartupTransaction::observationCloseContext(const QString& suppliedStartupId) const
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    const QPointer<FleetMissionExecutionObservation> run(_executionObservation);
    if (thread() != QThread::currentThread() || startupId != _startupId || _generation == 0
        || _active || _busy || _checking || _sendingIndex >= 0 || _state == QStringLiteral("empty")
        || _state == QStringLiteral("preparing") || _state == QStringLiteral("running")
        || !run || run->thread() != thread() || !run->_prepared || run->_retired || !run->_startupRetired
        || run->_startup.data() != this || run->_startupId != _startupId || run->_startupEpoch != _transactionEpoch
        || run->_startupGeneration != _generation || run->_binding != _binding || run->_vehicleIds != _vehicleIds
        || run->_runId == _startupId
        || !_swarm || _swarm->thread() != thread() || run->_swarm != _swarm
        || !run->_taskOrchestrator || run->_taskOrchestrator->thread() != thread()
        || _swarm->executionTaskOrchestratorBackend() != run->_taskOrchestrator.data()
        || _targets.size() != run->_targets.size() || _targets.isEmpty() || _targets.size() > 32) return {};
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<SwarmMissionOrchestrator> orchestrator(run->_taskOrchestrator);
    const auto runContext = run->readContext();
    const quint64 generation = _generation;
    const QString epoch = _transactionEpoch, state = _state;
    const QVariantMap binding = _binding;
    const QVariantList vehicleIds = _vehicleIds;
    const QList<Target> originalTargets = _targets;
    const auto observedTargets = run->_targets;
    const bool observerRetired = _executionObservationRetired;
    const auto same = [&]() {
        if (!self || !run || !swarm || !orchestrator || self->_executionObservation != run
            || self->_swarm != swarm || run->_taskOrchestrator != orchestrator
            || !run->readContextCurrent(runContext) || self->_generation != generation || self->_startupId != startupId
            || self->_transactionEpoch != epoch || self->_state != state || self->_binding != binding || self->_vehicleIds != vehicleIds
            || self->_active || self->_busy || self->_checking || self->_sendingIndex >= 0
            || self->_executionObservationRetired != observerRetired || self->_targets.size() != originalTargets.size()) return false;
        for (int index = 0; index < originalTargets.size(); ++index) {
            const Target& original = originalTargets.at(index);
            const Target& actual = self->_targets.at(index);
            if (actual.vehicleId != original.vehicleId || actual.componentId != original.componentId
                || actual.vehicle != original.vehicle || actual.mission != original.mission || actual.primary != original.primary
                || actual.stepIds != original.stepIds) return false;
            for (int step = 0; step < 3; ++step)
                if (actual.queued[step] != original.queued[step] || actual.uncertain[step] != original.uncertain[step]) return false;
        }
        return true;
    };
    const bool registered = orchestrator->executionObservationRetirementMatches(run.data(), runContext.taskModelId);
    if (!same() || !registered) return {};
    QVariantList targets;
    for (int index = 0; index < originalTargets.size(); ++index) {
        const Target& actual = originalTargets.at(index);
        const auto& observed = observedTargets.at(index);
        if (actual.vehicleId != observed.vehicleId || actual.componentId != observed.componentId
            || actual.vehicle != observed.vehicle || actual.mission != observed.mission
            || actual.primary != observed.primary || actual.stepIds.value(2).toString() != observed.modeStepId
            || (actual.vehicle && !actual.vehicle->fleetStartupTransportQuiescent())
            || (actual.mission && actual.mission->transportCallInProgress())) return {};
        targets.append(QVariantMap{{QStringLiteral("vehicleId"), actual.vehicleId}, {QStringLiteral("componentId"), actual.componentId},
            {QStringLiteral("modeStepId"), observed.modeStepId}, {QStringLiteral("modeQueueBegan"), observed.queueBegan},
            {QStringLiteral("modeQueueCommitted"), observed.queueCommitted}, {QStringLiteral("modeIssuedOrUncertain"), actual.queued[2] || actual.uncertain[2]},
            {QStringLiteral("continuityUnknown"), observed.continuityUnknown},
            {QStringLiteral("sourceBindingActiveAtLastObservation"), observed.sourceActive},
            {QStringLiteral("trackingAtLastObservation"), observed.tracking},
            {QStringLiteral("vehicleObjectRetained"), bool(actual.vehicle)}, {QStringLiteral("missionObjectRetained"), bool(actual.mission)}});
    }
    const QVariantMap runReservation = swarm->_missionDeploymentReservationState(runContext.runId, runContext.vehicleIds);
    const QVariantMap startupReservation = swarm->_missionDeploymentReservationState(startupId, runContext.vehicleIds);
    if (!same() || runReservation.isEmpty() || startupReservation.isEmpty()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("transactionEpoch"), _transactionEpoch}, {QStringLiteral("transactionGeneration"), QString::number(_generation)},
        {QStringLiteral("deploymentId"), run->_deploymentId}, {QStringLiteral("runId"), run->_runId},
        {QStringLiteral("executionObservationTaskId"), run->_taskModelId}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("targetVehicleIds"), run->_vehicleIds}, {QStringLiteral("startupState"), _state},
        {QStringLiteral("runReservationCurrentlyHeld"), !runReservation.value(QStringLiteral("occupiedVehicleIds")).toList().isEmpty()},
        {QStringLiteral("startupReservationCurrentlyHeld"), !startupReservation.value(QStringLiteral("occupiedVehicleIds")).toList().isEmpty()},
        {QStringLiteral("localReservationState"), QVariantMap{{QStringLiteral("run"), runReservation}, {QStringLiteral("startup"), startupReservation}}},
        {QStringLiteral("reservationDisposition"), QStringLiteral("end-local-observation-and-release-only-owned-local-reservations")},
        {QStringLiteral("flightMayContinue"), true}, {QStringLiteral("missionResultVerified"), false},
        {QStringLiteral("sourcesMayBeUnavailable"), true}, {QStringLiteral("targets"), targets}};
}

bool FleetMissionStartupTransaction::observationCloseContextCurrent(const QString& startupId, const QVariantMap& context) const
{
    const QString originalStartupId = startupId;
    const QVariantMap originalContext = context;
    return originalContext.size() == 18 && observationCloseContext(originalStartupId) == originalContext;
}

QVariantMap FleetMissionStartupTransaction::observationCloseSourceObjects(const QString& startupId, const QVariantMap& context) const
{
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    if (!observationCloseContextCurrent(startupId, context) || !self || !_executionObservation || !_swarm
        || !_executionObservation->_taskOrchestrator) return {};
    return {{QStringLiteral("run"), QVariant::fromValue(static_cast<QObject*>(_executionObservation.data()))},
        {QStringLiteral("swarm"), QVariant::fromValue(static_cast<QObject*>(_swarm.data()))},
        {QStringLiteral("orchestrator"), QVariant::fromValue(static_cast<QObject*>(_executionObservation->_taskOrchestrator.data()))}};
}

bool FleetMissionStartupTransaction::commitObservationClose(const QString& startupId, const QVariantMap& context)
{
    const QString originalStartupId = startupId;
    const QPointer<FleetMissionStartupTransaction> self(this);
    if (!observationCloseContextCurrent(originalStartupId, context) || !self) return false;
    const bool retired = retireExecutionObservation(originalStartupId, true);
    if (self && retired) self->_independentObservationCloseCommitted = true;
    return retired;
}

QVariantMap FleetMissionStartupTransaction::observationCloseDisposition(const QString& startupId, const QVariantMap& context) const
{
    const QPointer<FleetMissionExecutionObservation> run(_executionObservation);
    if (thread() != QThread::currentThread() || startupId != _startupId || context.size() != 18
        || !run || !run->_retired || !_executionObservationRetired || !_swarm || run->_swarm != _swarm
        || _swarm->thread() != thread() || run->_startup.data() != this || run->_startupId != _startupId
        || context.value(QStringLiteral("startupId")) != _startupId || context.value(QStringLiteral("runId")) != run->_runId
        || context.value(QStringLiteral("transactionEpoch")) != _transactionEpoch
        || context.value(QStringLiteral("transactionGeneration")) != QString::number(_generation)
        || context.value(QStringLiteral("deploymentId")) != run->_deploymentId
        || context.value(QStringLiteral("executionObservationTaskId")) != run->_taskModelId
        || context.value(QStringLiteral("binding")).toMap() != _binding
        || context.value(QStringLiteral("targetVehicleIds")).toList() != run->_vehicleIds) return {};
    const QVariantMap runReservation = _swarm->_missionDeploymentReservationState(run->_runId, run->_vehicleIds);
    const QVariantMap startupReservation = _swarm->_missionDeploymentReservationState(_startupId, run->_vehicleIds);
    if (runReservation.isEmpty() || startupReservation.isEmpty()) return {};
    const bool runHeld = !runReservation.value(QStringLiteral("occupiedVehicleIds")).toList().isEmpty();
    const bool startupHeld = !startupReservation.value(QStringLiteral("occupiedVehicleIds")).toList().isEmpty();
    return {{QStringLiteral("monitoringRetired"), true}, {QStringLiteral("runReservationCurrentlyHeld"), runHeld},
        {QStringLiteral("startupReservationCurrentlyHeld"), startupHeld}, {QStringLiteral("localReservationsReleasedVerified"), !runHeld && !startupHeld}};
}

bool FleetMissionStartupTransaction::retireExecutionObservation(const QString& suppliedStartupId, bool operatorConfirmed)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || !operatorConfirmed || startupId != _startupId
        || !_executionObservation || _active || _busy || _checking || _sendingIndex >= 0) return false;
    for (const Target& target : _targets)
        if ((target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())
            || (target.mission && target.mission->transportCallInProgress())) return false;
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QPointer<FleetMissionExecutionObservation> observer(_executionObservation);
    const quint64 generation = _generation;
    const QString epoch = _transactionEpoch;
    _executionObservationRetired = true;
    const bool retired = observer->retireObservation(true);
    if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch
        || _executionObservation != observer) return retired;
    if (!retired) { _executionObservationRetired = false; return false; }
    // An explicitly retired monitor does not keep an old startup transport
    // reservation solely because its observation handoff had failed.
    _drain();
    return true;
}

bool FleetMissionStartupTransaction::commandContextCurrent(QObject* actualVehicle, QObject* actualPrimary,
    const QString& suppliedStartupId, const QString& suppliedStepId, int step) const
{
    const QString startupId = suppliedStartupId, stepId = suppliedStepId;
    if (thread() != QThread::currentThread() || !_active || !_same(_generation) || !_reservationHeld || step != _stage
        || step < 0 || step > 2 || startupId != _startupId || _clock.elapsed() >= _stageDeadlineMs
        || !_phaseMemoryCurrent(_generation)) return false;
    // This actual native transaction, not a caller map or a retired Review,
    // holds the original witness. It checks the WHOLE original source group
    // without callbacks/solver immediately in the Vehicle ownership lane.
    if (_startupClearanceRequired ? !_startupClearanceWitness || !_startupClearanceWitness->memoryCurrent()
        : bool(_startupClearanceWitness)) return false;
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle && target.primary.data() == actualPrimary)
        return target.vehicle && target.primary && target.stepIds.value(step).toString() == stepId
            && !(step == 1 && _groundRestart && target.reviewedArmed)
            && (target.queued[step] || (_sendingIndex >= 0 && _sendingIndex < _targets.size()
                && _targets.at(_sendingIndex).vehicleId == target.vehicleId));
    return false;
}

bool FleetMissionStartupTransaction::reviewedArmedGroundBasisCurrent(QObject* actualVehicle,
    const QString& startupId, const QVariantMap& exactReadiness) const
{
    if (thread() != QThread::currentThread() || !_groundRestart || !_same(_generation)
        || startupId != _startupId || (!_active && _state != QStringLiteral("preparing"))) return false;
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle)
        return target.vehicle && target.reviewedArmed && target.beforeArm == exactReadiness;
    return false;
}

QVariantList FleetMissionStartupTransaction::_orderedSteps(const Target& target, int stage, bool issued) const
{
    QVariantList ordered;
    for (int index = 0; index < stage + (issued ? 1 : 0); ++index)
        if (!(index == 1 && _groundRestart && target.reviewedArmed)) ordered.append(target.stepIds.value(index));
    return ordered;
}

bool FleetMissionStartupTransaction::odometryContinuityRequired(QObject* actualVehicle, const QString& startupId) const
{
    if (thread() != QThread::currentThread() || !_same(_generation) || startupId != _startupId
        || (!_active && _state != QStringLiteral("preparing"))) return false;
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle) {
        const QVariantMap policy = target.beforeArm.value(QStringLiteral("startupTransitionPolicy")).toMap();
        const bool motionPolicyShape = (policy.size() == 8 && integer(policy.value(QStringLiteral("schemaVersion")), 2, 2))
            || (policy.size() == 9 && integer(policy.value(QStringLiteral("schemaVersion")), 3, 4));
        return target.vehicle && target.primary && motionPolicyShape
            && target.member.value(QStringLiteral("executionProfile")).toMap().value(QStringLiteral("startupTransitionPolicy")).toMap() == policy
            && isTrue(target.beforeArm.value(QStringLiteral("startupMotionReferenceRequired")))
            && isTrue(target.beforeArm.value(QStringLiteral("startupMotionReferenceMatches")));
    }
    return false;
}

qint64 FleetMissionStartupTransaction::estimatorContinuityMaximumGapMs(QObject* actualVehicle, const QString& startupId) const
{
    if (thread() != QThread::currentThread() || !_same(_generation) || startupId != _startupId
        || (!_active && _state != QStringLiteral("preparing"))) return -1;
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle) {
        const QVariantMap profile = target.member.value(QStringLiteral("executionProfile")).toMap();
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(profile);
        const QVariantMap policy = target.beforeArm.value(QStringLiteral("startupTransitionPolicy")).toMap();
        double gap = 0;
        if (!target.vehicle || !target.primary || !isTrue(normalized.value(QStringLiteral("ok")))
            || normalized.value(QStringLiteral("profile")).toMap() != profile
            || profile.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map
            || profile.value(QStringLiteral("startupTransitionPolicy")).toMap() != policy
            || !integer(policy.value(QStringLiteral("maximumRawGapMs")), 100, 3000, &gap)
            || !isTrue(target.beforeArm.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
            || !isTrue(target.beforeArm.value(QStringLiteral("startupEstimatorEvidenceMatches")))
            || !FleetMissionStartupEstimator::healthyReference(target.beforeArm.value(QStringLiteral("startupEstimatorObservation")).toMap())) return -1;
        return static_cast<qint64>(gap);
    }
    return -1;
}

QVariantMap FleetMissionStartupTransaction::reviewedAltitudeContext(QObject* actualVehicle,
    QObject* actualPrimary, const QString& startupId) const
{
    // Retained terminal lookup is historical only. Native sending separately
    // requires commandContextCurrent/_same and the mandatory full fence.
    if (thread() != QThread::currentThread() || _generation == 0 || startupId != _startupId
        || !uuid(startupId) || !uuid(_transactionEpoch)) return {};
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle && target.primary.data() == actualPrimary) {
        const QVariantMap profile = target.member.value(QStringLiteral("executionProfile")).toMap();
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(profile);
        const QVariantMap policy = target.beforeArm.value(QStringLiteral("startupTransitionPolicy")).toMap();
        if (!target.vehicle || !target.primary || target.vehicle->thread() != thread()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || !isTrue(normalized.value(QStringLiteral("ok")))
            || normalized.value(QStringLiteral("profile")).toMap() != profile
            || profile.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map
            || profile.value(QStringLiteral("startupTransitionPolicy")).toMap() != policy) return {};
        const bool required = integer(policy.value(QStringLiteral("schemaVersion")), 4, 4);
        QVariantMap reference, sample;
        if (required) {
            reference = target.beforeArm.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
            sample = target.beforeArm.value(QStringLiteral("startupAltitudeReferenceSample")).toMap();
            if (!isTrue(target.beforeArm.value(QStringLiteral("startupAltitudeReferenceRequired")))
                || !isTrue(target.beforeArm.value(QStringLiteral("startupAltitudeReferenceCaptured")))
                || !isTrue(target.beforeArm.value(QStringLiteral("startupAltitudeReferenceMatches")))
                || reference.size() != 20 || sample.size() != 20
                || QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(reference).toJson(QJsonDocument::Compact),
                    QCryptographicHash::Sha256).toHex()) != target.beforeArm.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
                || !FleetMissionStartupAltitude::sampleWithinBaseline(sample, sample,
                    policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap())) return {};
        }
        return {{QStringLiteral("required"), required}, {QStringLiteral("policy"), policy},
            {QStringLiteral("reference"), reference}, {QStringLiteral("sample"), sample}};
    }
    return {};
}

std::shared_ptr<const FleetMissionStartupGeographicMotion>
FleetMissionStartupTransaction::startupGeographicMotionBasis(QObject* actualVehicle,
    const QString& startupId, const QVariantMap& exactReadiness) const
{
    if (thread() != QThread::currentThread() || !_same(_generation) || startupId != _startupId) return {};
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle) {
        if (target.vehicle && target.primary && target.beforeArm == exactReadiness && target.geographicMotion)
            return target.geographicMotion;
        return {};
    }
    return {};
}

bool FleetMissionStartupTransaction::phaseBasisCurrent(QObject* actualVehicle, const QString& startupId,
    const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& payload,
    const QVariantMap& exactReadiness, const QVariantList& orderedStepIds, int suppliedPhase) const
{
    if (thread() != QThread::currentThread() || !_same(_generation) || startupId != _startupId
        || constraints != _constraints) return false;
    using Phase = FleetMissionStartupTransition::Phase;
    int stage = -1; bool issued = false;
    switch (static_cast<Phase>(suppliedPhase)) {
    case Phase::BeforeSeq0: stage = 0; break;
    case Phase::AwaitSeq0: stage = 0; issued = true; break;
    case Phase::ReadyToArm: stage = 1; break;
    case Phase::AwaitArm: stage = 1; issued = true; break;
    case Phase::ReadyToMode: stage = 2; break;
    case Phase::AwaitMode: case Phase::ModeObserved: stage = 2; issued = true; break;
    default: return false;
    }
    if (_stage != stage) return false;
    for (const Target& target : _targets) if (target.vehicle.data() == actualVehicle)
        return target.vehicle && target.primary && target.geographicMotion && target.member == member && target.payload == payload
            && target.beforeArm == exactReadiness && !(stage == 1 && issued && _groundRestart && target.reviewedArmed)
            && orderedStepIds == _orderedSteps(target, stage, issued);
    return false;
}

bool FleetMissionStartupTransaction::_publish(quint64 generation)
{
    const QPointer<FleetMissionStartupTransaction> self(this);
    if (generation != _generation) return false;
    emit changed();
    return self && _generation == generation;
}
void FleetMissionStartupTransaction::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionStartupTransaction::_connectSources(quint64 generation)
{
    // Expected ARM/mode cache notifications occur inside the native receive
    // stack. They are NOT phase proof and must never advance a barrier here.
    const auto notice = [this, generation]() {
        if (_generation != generation) return;
        if (_active || _state == QStringLiteral("preparing")) _timer.start();
    };
    const auto destroyed = [this, generation]() { _stop(generation, QStringLiteral("stale"), QStringLiteral("startup-actual-source-destroyed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_authority.data()), static_cast<QObject*>(_groundAuthority.data()),
        static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()),
        static_cast<QObject*>(_health.data()), _authorization.data(), static_cast<QObject*>(_coordinator.data()),
        static_cast<QObject*>(_catalog.data()), static_cast<QObject*>(_registry.data()), static_cast<QObject*>(_faultPolicy.data())})
        if (object) _connections.append(connect(object, &QObject::destroyed, this, destroyed));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, notice));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, notice));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, notice));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, notice));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, notice));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, notice));
    _connections.append(connect(_coordinator.data(), &FleetMissionReviewCoordinator::firmwareAdapterCatalogChanged, this, notice));
    _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, notice));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, notice));
    _connections.append(connect(_health.data(), &FleetHealthCenter::healthChanged, this, notice));
    _connections.append(connect(_swarm.data(), &SwarmController::formationActiveChanged, this, notice));
    _connections.append(connect(_swarm.data(), &SwarmController::formationBusyChanged, this, notice));
    for (const Target& target : _targets) {
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(object, &QObject::destroyed, this, destroyed));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, notice));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, notice));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, notice));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, notice));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, notice));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, notice));
        _connections.append(connect(target.vehicle.data(), &Vehicle::fleetStartupStepObservationChanged, this, notice));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, notice));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this, notice));
    }
}

QVariantMap FleetMissionStartupTransaction::requestStartup(const QVariantMap& suppliedSources, const QString& suppliedStartupId, bool operatorConfirmed)
{
    return _requestStartup(suppliedSources, suppliedStartupId, operatorConfirmed, false);
}

QVariantMap FleetMissionStartupTransaction::requestGroundStartup(const QVariantMap& suppliedSources, const QString& suppliedStartupId, bool operatorConfirmed)
{
    return _requestStartup(suppliedSources, suppliedStartupId, operatorConfirmed, true);
}

QVariantMap FleetMissionStartupTransaction::_requestStartup(const QVariantMap& suppliedSources, const QString& suppliedStartupId,
    bool operatorConfirmed, bool groundRestart)
{
    const QVariantMap sources = suppliedSources;
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _busy || _checking || _active || _reservationHeld || _transportDraining
        || _handoffPending || _generation != 0 || !_startupId.isEmpty() || _executionObservation
        || !operatorConfirmed || !uuid(startupId)) return rejection(QStringLiteral("explicit-confirmed-unused-native-startup-required"));
    _busy = true;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QSet<QString> names{QStringLiteral("session"), QStringLiteral("roles"), QStringLiteral("store"), QStringLiteral("authority"),
        QStringLiteral("deployment"), QStringLiteral("swarm"), QStringLiteral("health"), QStringLiteral("authorizationOwner")};
    if (sources.size() != 8) return rejection(QStringLiteral("exact-eight-native-startup-sources-required"));
    for (auto it = sources.constBegin(); it != sources.constEnd(); ++it)
        if (!names.contains(it.key()) || !actualObject(it.value())) return rejection(QStringLiteral("actual-native-object-not-json-required"));
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualObject(sources.value(QStringLiteral("session")))));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualObject(sources.value(QStringLiteral("roles")))));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(actualObject(sources.value(QStringLiteral("store")))));
    const QPointer<FleetMissionStartupAuthority> authority(qobject_cast<FleetMissionStartupAuthority*>(actualObject(sources.value(QStringLiteral("authority")))));
    const QPointer<FleetMissionGroundStartupAuthority> groundAuthority(qobject_cast<FleetMissionGroundStartupAuthority*>(actualObject(sources.value(QStringLiteral("authority")))));
    const QPointer<FleetMissionDeployment> deployment(qobject_cast<FleetMissionDeployment*>(actualObject(sources.value(QStringLiteral("deployment")))));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(actualObject(sources.value(QStringLiteral("swarm")))));
    const QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(actualObject(sources.value(QStringLiteral("health")))));
    const QPointer<QObject> owner(actualObject(sources.value(QStringLiteral("authorizationOwner"))));
    const auto alive = [&]() { return self && session && roles && store
        && (groundRestart ? bool(groundAuthority) && !authority : bool(authority) && !groundAuthority)
        && deployment && swarm && health && owner
        && outboundGroup(self.data(), {session.data(), deployment.data(), groundRestart
            ? static_cast<QObject*>(groundAuthority.data()) : static_cast<QObject*>(authority.data())}); };
    if (!alive()) return rejection(QStringLiteral("typed-native-startup-sources-required"));
    for (QObject* object : {static_cast<QObject*>(session.data()), static_cast<QObject*>(roles.data()), static_cast<QObject*>(store.data()),
        groundRestart ? static_cast<QObject*>(groundAuthority.data()) : static_cast<QObject*>(authority.data()),
        static_cast<QObject*>(deployment.data()), static_cast<QObject*>(swarm.data()),
        static_cast<QObject*>(health.data()), owner.data()})
        if (object->thread() != thread()) return rejection(QStringLiteral("same-owner-thread-required"));
    // Reject a wrong actual instance BEFORE replacing its retained terminal
    // generation/targets. A later full gate is too late to protect that source.
    QVariant actualConsumer;
    const bool consumerInvoked = QMetaObject::invokeMethod(owner.data(), groundRestart
        ? "fleetGroundStartupTransactionSourcesMatch" : "fleetStartupTransactionSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, actualConsumer), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(sources)));
    if (!alive() || !consumerInvoked || !isTrue(actualConsumer)) return rejection(QStringLiteral("actual-startup-consumer-instance-required-before-state-replacement"));
    const qint64 beforeContext = _clock.elapsed();
    const QVariantMap context = groundRestart ? groundAuthority->authorityContext(startupId) : authority->authorityContext(startupId);
    double remaining = 0, schema = 0;
    if (!alive() || context.size() != (groundRestart ? 27 : 25) || !integer(context.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || !isTrue(context.value(QStringLiteral("ok"))) || (!groundRestart && !isTrue(context.value(QStringLiteral("authorityAccepted"))))
        || context.value(QStringLiteral("startupId")) != startupId || !uuid(context.value(QStringLiteral("authorityEpoch")))
        || !digest(context.value(QStringLiteral("sourceHash"))) || (!groundRestart && !isTrue(context.value(QStringLiteral("preARMOnly"))))
        || (groundRestart && (context.value(QStringLiteral("scope")).toString() != QStringLiteral("new-ground-reassessment-startup-basis-not-dispatch")
            || !isTrue(context.value(QStringLiteral("originalRisksPreserved"))) || !isFalse(context.value(QStringLiteral("previousAuthorizationRestored")))))
        || !isFalse(context.value(QStringLiteral("startupPermitGranted"))) || !isFalse(context.value(QStringLiteral("executable")))
        || !isFalse(context.value(QStringLiteral("flightCommandReleased")))
        || !integer(context.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumMs, &remaining))
        return rejection(QStringLiteral("actual-current-pre-arm-authority-required"));
    const QVariantMap actualSources = groundRestart ? groundAuthority->actualSourcesForStartupTransaction(startupId) : authority->actualSourcesForStartupTransaction(startupId);
    if (!alive() || actualSources.size() != 8) return rejection(QStringLiteral("actual-authority-source-bundle-required"));
    for (const QString& key : names) if (actualObject(actualSources.value(key)) != actualObject(sources.value(key)))
        return rejection(QStringLiteral("authority-source-substitution-denied"));
    const QPointer<FleetFirmwareAdapterCatalog> catalog(groundRestart ? groundAuthority->nativeFirmwareCatalog(startupId) : authority->nativeFirmwareCatalog(startupId));
    if (!alive() || !catalog) return rejection(QStringLiteral("actual-firmware-catalog-required"));
    const QPointer<FleetMissionReviewCoordinator> coordinator(groundRestart ? groundAuthority->nativeReviewCoordinator(startupId) : authority->nativeReviewCoordinator(startupId));
    if (!alive() || !catalog || !coordinator) return rejection(QStringLiteral("actual-review-coordinator-required"));
    const QPointer<FleetRegistry> registry(groundRestart ? groundAuthority->nativeRegistry(startupId) : authority->nativeRegistry(startupId));
    if (!alive() || !catalog || !coordinator) return rejection(QStringLiteral("authority-provider-changed"));
    const QPointer<FaultToleranceManager> faults(groundRestart ? groundAuthority->nativeFaultPolicy(startupId) : authority->nativeFaultPolicy(startupId));
    if (!alive() || !catalog || !coordinator || catalog->thread() != thread() || coordinator->thread() != thread())
        return rejection(QStringLiteral("actual-owner-thread-providers-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!alive() || !catalog || !coordinator) return rejection(QStringLiteral("primary-identity-source-destroyed"));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!alive() || !catalog || !coordinator || !identityCurrent) return rejection(QStringLiteral("current-actual-primary-operator-required"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start"));
    if (!alive() || !validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return rejection(QStringLiteral("current-two-account-start-role-required"));
    const QPointer<FleetMissionArmedGroundReview> groundReview(groundRestart ? groundAuthority->nativeGroundReview(startupId) : nullptr);
    if (!alive() || (groundRestart && (!groundReview || !registry || !faults
        || !outboundGroup(self.data(), {groundReview.data()})))) return rejection(QStringLiteral("actual-new-ground-review-providers-required"));
    const QVariantMap reviewArtifact = context.value(groundRestart ? QStringLiteral("newReviewContext") : QStringLiteral("reviewContext")).toMap().value(QStringLiteral("reviewArtifact")).toMap();
    const QVariantMap frozenDeployment = groundRestart ? reviewArtifact.value(QStringLiteral("previousReference")).toMap().value(QStringLiteral("deploymentContext")).toMap()
        : context.value(QStringLiteral("deploymentContext")).toMap();
    const QVariantMap plan = frozenDeployment.value(QStringLiteral("plan")).toMap();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    const bool clearanceRequired = constraints.contains(QStringLiteral("clearanceInput"));
    const auto clearanceWitness = groundRestart ? groundAuthority->startupClearanceWitness(startupId)
        : authority->startupClearanceWitness(startupId);
    const QString clearanceFingerprint = reviewArtifact.value(QStringLiteral("startupClearanceReview")).toMap()
        .value(QStringLiteral("inputFingerprint")).toString();
    if (!alive() || (clearanceRequired ? !clearanceWitness || !digest(clearanceFingerprint)
            || clearanceWitness->inputFingerprint() != clearanceFingerprint || !clearanceWitness->memoryCurrent()
        : bool(clearanceWitness))) return rejection(QStringLiteral("same-native-reviewed-startup-clearance-witness-required"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    const QVariantList ids = context.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList physical = context.value(QStringLiteral("physicalTargets")).toList();
    QByteArray record;
    if (!currentRecord(store.data(), binding, &record) || !alive() || ids.isEmpty() || ids.size() > 32 || physical.size() != ids.size())
        return rejection(QStringLiteral("whole-latest-nonrestored-record-and-targets-required"));
    // Supplementary history, not a new startup admission requirement. The
    // exact original deployment may lack this field (older retained Run or
    // unbounded input); never borrow a task from the latest store or QML.
    const QVariantMap deploymentSnapshot = deployment->current();
    const QVariantMap planningTask = deploymentSnapshot.value(QStringLiteral("deploymentId")) == context.value(QStringLiteral("deploymentId"))
        && deploymentSnapshot.value(QStringLiteral("binding")).toMap() == binding
        ? deployment->planningTaskForRun(binding) : QVariantMap();
    QList<Target> targets;
    QSet<int> unique;
    int items = 0;
    const QString deploymentId = context.value(QStringLiteral("deploymentId")).toString();
    for (int index = 0; index < ids.size(); ++index) {
        double id = 0;
        if (!integer(ids[index], 1, 255, &id) || unique.contains(int(id)) || physical[index].type() != QVariant::Map)
            return rejection(QStringLiteral("ordered-unique-native-targets-required"));
        unique.insert(int(id));
        const QVariantMap basis = physical[index].toMap();
        Target target; target.vehicleId = int(id);
        if (!integer(basis.value(QStringLiteral("vehicleId")), id, id, &schema)) return rejection(QStringLiteral("physical-target-order-mismatch"));
        target.member = basis.value(QStringLiteral("member")).toMap(); target.payload = basis.value(QStringLiteral("payload")).toMap();
        target.beforeArm = basis.value(groundRestart ? QStringLiteral("currentRisk") : QStringLiteral("beforeArmReadiness")).toMap();
        target.readReceipt = basis.value(groundRestart ? QStringLiteral("newReadReceipt") : QStringLiteral("readReceipt")).toMap();
        if (groundRestart) {
            if (basis.value(QStringLiteral("frozenArmed")).type() != QVariant::Bool || target.readReceipt.size() != 33)
                return rejection(QStringLiteral("new-ground-frozen-armed-and-exact-read-basis-required"));
            target.reviewedArmed = basis.value(QStringLiteral("frozenArmed")).toBool();
            if (target.beforeArm.value(QStringLiteral("startupPhase")).toString() != (target.reviewedArmed
                    ? QStringLiteral("armed-ground-before-mission-mode") : QStringLiteral("before-normal-arm")))
                return rejection(QStringLiteral("new-ground-reviewed-typed-phase-required"));
        }
        // Missing policy is NOT filled in from defaults. It requires a new
        // plan, raw content check, review and independent-account approval.
        const QVariantMap profileSource = target.member.value(QStringLiteral("executionProfile")).toMap();
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(profileSource);
        const QVariantMap policy = profileSource.value(QStringLiteral("startupTransitionPolicy")).toMap();
        const bool policyShape = (policy.size() == 6 && integer(policy.value(QStringLiteral("schemaVersion")), 1, 1))
            || (policy.size() == 8 && integer(policy.value(QStringLiteral("schemaVersion")), 2, 2))
            || (policy.size() == 9 && integer(policy.value(QStringLiteral("schemaVersion")), 3, 4));
        if (profileSource.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map || !policyShape
            || !isTrue(normalized.value(QStringLiteral("ok"))) || normalized.value(QStringLiteral("profile")).toMap() != profileSource
            || target.beforeArm.value(QStringLiteral("startupTransitionPolicy")).toMap() != policy)
            return rejection(QStringLiteral("new-review-with-explicit-startup-transition-policy-required"));
        // A clearance task retains the original Review/worker/Witness object
        // and verifies its complete source without rebuilding Qt operands.
        // A legacy task without clearance prepares its own model once.
        const QVariant flightStateValue = target.beforeArm.value(QStringLiteral("flightStateSummary"));
        const QVariant channelsValue = flightStateValue.toMap().value(QStringLiteral("channels"));
        const QVariant globalValue = channelsValue.toMap().value(QStringLiteral("globalPosition"));
        const QVariant positionValue = globalValue.toMap().value(QStringLiteral("value"));
        const QVariant ceilingValue = constraints.value(QStringLiteral("maxAltitudeMeters"));
        const bool numericCeiling = ceilingValue.type() == QVariant::Int || ceilingValue.type() == QVariant::UInt
            || ceilingValue.type() == QVariant::LongLong || ceilingValue.type() == QVariant::ULongLong
            || ceilingValue.type() == QVariant::Double;
        if (flightStateValue.type() != QVariant::Map || flightStateValue.isNull()
            || channelsValue.type() != QVariant::Map || channelsValue.isNull()
            || globalValue.type() != QVariant::Map || globalValue.isNull()
            || positionValue.type() != QVariant::Map || positionValue.isNull()
            || !numericCeiling || ceilingValue.isNull() || !std::isfinite(ceilingValue.toDouble()))
            return rejection(QStringLiteral("original-reviewed-geographic-motion-source-required"));
        if (clearanceRequired) {
            const auto originalMotion = clearanceWitness->originalGeographicMotion(target.vehicleId);
            if (!alive() || !originalMotion || !originalMotion->matchesSource(positionValue.toMap(),
                    target.payload, policy, ceilingValue.toDouble()))
                return rejection(QStringLiteral("same-native-reviewed-geographic-motion-model-required"));
            target.geographicMotion = originalMotion;
        } else {
            const auto preparedMotion = FleetMissionStartupGeographicMotion::prepare(positionValue.toMap(),
                target.payload, policy, ceilingValue.toDouble());
            if (!preparedMotion) return rejection(QStringLiteral("original-reviewed-geographic-motion-model-required"));
            target.geographicMotion = preparedMotion;
        }
        const int count = target.payload.value(QStringLiteral("items")).toList().size(); items += count;
        if (count < 1 || count > 2048 || items > 8192 || target.readReceipt.size() != (groundRestart ? 33 : 34))
            return rejection(QStringLiteral("bounded-complete-raw-mission-basis-required"));
        target.vehicle = groundRestart ? groundAuthority->authorityVehicle(startupId, target.vehicleId) : authority->authorityVehicle(startupId, target.vehicleId);
        if (!alive() || !target.vehicle) return rejection(QStringLiteral("actual-authority-vehicle-required"));
        target.mission = groundRestart ? groundAuthority->authorityManager(startupId, target.vehicleId) : authority->authorityManager(startupId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission) return rejection(QStringLiteral("actual-authority-manager-required"));
        target.primary = groundRestart ? groundAuthority->authorityPrimaryLink(startupId, target.vehicleId) : authority->authorityPrimaryLink(startupId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission || !target.primary) return rejection(QStringLiteral("actual-authority-primary-required"));
        target.parameters = target.vehicle->parameterManager();
        if (!alive() || !target.vehicle || !target.parameters) return rejection(QStringLiteral("actual-parameter-manager-required"));
        target.links = target.vehicle->vehicleLinkManager();
        if (!alive() || !target.vehicle || !target.parameters || !target.links) return rejection(QStringLiteral("actual-link-manager-required"));
        target.componentId = target.vehicle->defaultComponentId();
        // Pin this transaction's actual PRIMARY control block before later
        // per-target callbacks. Keep the original reviewed twenty-nine rows;
        // neither a new parameter read nor a later stage may replace them.
        target.primaryOwner = target.links->primaryLink();
        const QVariant parameterSummary = target.beforeArm.value(QStringLiteral("parameterSummary"));
        const QVariant originalParameters = parameterSummary.toMap().value(QStringLiteral("originalExecutionParameters"));
        if (parameterSummary.type() != QVariant::Map || parameterSummary.isNull()
            || originalParameters.type() != QVariant::Map || originalParameters.isNull()
            || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId,
                target.primary.data(), target.primaryOwner, originalParameters.toMap(), MaximumMs))
            return rejection(QStringLiteral("original-reviewed-execution-parameters-and-primary-owner-required"));
        if (groundRestart) {
            const QVariantMap actualRead = target.mission->fleetMissionArmedGroundReadReceipt(MaximumMs, groundReview.data());
            QVariantMap stableRead = actualRead; stableRead.remove(QStringLiteral("ageMs"));
            if (!alive() || !groundReview || !target.vehicle || !target.mission || actualRead.size() != 34
                || !isTrue(actualRead.value(QStringLiteral("available"))) || !isTrue(actualRead.value(QStringLiteral("fresh")))
                || stableRead != target.readReceipt || target.vehicle->armed() != target.reviewedArmed)
                return rejection(QStringLiteral("actual-current-new-complete-ground-read-required"));
            target.readReceipt = actualRead;
        }
        for (int step = 0; step < 3; ++step) target.stepIds.append(QUuid::createUuid().toString(QUuid::WithoutBraces));
        targets.append(target);
    }
    const QVariantMap leader = reviewArtifact.value(QStringLiteral("leaderBinding")).toMap();
    const QVariantMap reviewedHealth = context.value(QStringLiteral("healthGate")).toMap();
    double plannedLeader = 0;
    if (groundRestart ? (!isTrue(reviewedHealth.value(QStringLiteral("allowed")))
            || reviewedHealth.value(QStringLiteral("requireLeader")).type() != QVariant::Bool
            || context.value(QStringLiteral("primaryIdentity")).toMap() != identity
            || context.value(QStringLiteral("roleSnapshot")).toMap() != role)
        : (leader.value(QStringLiteral("requiredByFrozenRequest")).type() != QVariant::Bool
        || !integer(leader.value(QStringLiteral("plannedLeaderVehicleId")), 0, 255, &plannedLeader)
        || ((isTrue(leader.value(QStringLiteral("requiredByFrozenRequest"))) || plannedLeader > 0)
            && !isTrue(leader.value(QStringLiteral("currentLeaderBindingMatches"))))))
        return rejection(QStringLiteral("explicit-current-reviewed-leader-binding-required"));
    const bool basisCurrent = alive() && (groundRestart ? groundAuthority->authorityContextCurrent(startupId, context) : authority->authorityContextCurrent(startupId, context));
    if (!alive() || !basisCurrent
        || _generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("authority-changed-before-transaction-handoff"));
    // Freeze a bounded historical summary while the complete actual basis is
    // still current. Reject encoding failure BEFORE replacing old terminal
    // state. Subsequent captures never call the retired authority or renew
    // this digest from caller-supplied JSON.
    const QVariantMap authorityReference = nativeAuthorityReference(context, groundRestart);
    if (!nativeAuthorityReferenceShape(authorityReference, groundRestart))
        return rejection(QStringLiteral("native-authority-reference-resource-or-shape-limit"));
    _disconnectSources(); ++_generation;
    _retirePhaseMemorySeals();
    _reassessmentReferenceContext.clear();
    const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _groundRestart = groundRestart;
    _authority = authority; _groundAuthority = groundAuthority; _deployment = deployment; _swarm = swarm; _health = health;
    _authorization = owner; _coordinator = coordinator; _catalog = catalog; _registry = registry; _faultPolicy = faults;
    _authorityContext = context; _nativeAuthorityReference = authorityReference;
    _startupClearanceRequired = clearanceRequired; _startupClearanceWitness = clearanceWitness;
    _identityReceipt = identity; _roleReceipt = role; _binding = binding; _constraints = constraints;
    _recordBytes = record; _deploymentContext = frozenDeployment; _healthGate = context.value(QStringLiteral("healthGate")).toMap();
    _planningTask = planningTask;
    _catalogSnapshot = context.value(QStringLiteral("catalogSnapshot")).toMap(); _vehicleIds = ids; _targets = targets;
    _requireLeader = isTrue(groundRestart ? reviewedHealth.value(QStringLiteral("requireLeader")) : leader.value(QStringLiteral("requiredByFrozenRequest")));
    _leaderId = faults ? faults->leaderVehicleId() : 0; _leaderRegistered = registry && _leaderId > 0 && registry->containsVehicle(_leaderId);
    _startupId = startupId; _sourceHash = context.value(QStringLiteral("sourceHash")).toString(); _authenticationReceipt.clear();
    _expiresAtMs = beforeContext + qint64(remaining); _stageDeadlineMs = _expiresAtMs; _lastQueueAtMs = -1;
    _stage = 0; _sendingIndex = -1; _state = QStringLiteral("preparing"); _reason.clear();
    _active = false; _reservationHeld = false; _handoffPending = false; _transportDraining = false; _anyFlightCommandQueued = false;
    _connectSources(generation);
    if (!_gate(generation, false) || !self) {
        if (self) _stop(generation, QStringLiteral("failed"), QStringLiteral("startup-pre-handoff-full-source-gate-denied"));
        return rejection(QStringLiteral("startup-pre-handoff-full-source-gate-denied"));
    }
    _handoffPending = true;
    const qint64 beforeTransfer = _clock.elapsed();
    const qint64 maximumTransfer = qMin<qint64>(MaximumMs, _expiresAtMs - beforeTransfer);
    const QVariantMap moved = groundRestart ? groundAuthority->transferToStartupTransaction(self.data(), context, startupId, _sourceHash, maximumTransfer)
        : authority->transferToStartupTransaction(self.data(), context, startupId, _sourceHash, maximumTransfer);
    if (!self) return rejection(QStringLiteral("startup-transaction-destroyed-at-handoff"));
    double handedRemaining = 0;
    if (!_same(generation) || !isTrue(moved.value(QStringLiteral("ok"))) || moved.size() != 10
        || !isTrue(moved.value(QStringLiteral("reservationTransferred"))) || moved.value(QStringLiteral("startupId")) != startupId
        || moved.value(QStringLiteral("transactionEpoch")) != _transactionEpoch || moved.value(QStringLiteral("sourceHash")) != _sourceHash
        || moved.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map || moved.value(QStringLiteral("authenticationReceipt")).toMap().size() != (groundRestart ? 30 : 24)
        || !isFalse(moved.value(QStringLiteral("startupPermitGranted"))) || !isFalse(moved.value(QStringLiteral("executable")))
        || !isFalse(moved.value(QStringLiteral("flightCommandReleased"))) || !integer(moved.value(QStringLiteral("remainingMs")), 1, MaximumMs, &handedRemaining)) {
        _stop(generation, QStringLiteral("failed"), QStringLiteral("actual-once-only-authentication-reservation-handoff-failed"));
        return rejection(QStringLiteral("actual-once-only-authentication-reservation-handoff-failed"));
    }
    _authenticationReceipt = moved.value(QStringLiteral("authenticationReceipt")).toMap();
    _expiresAtMs = qMin(_expiresAtMs, beforeTransfer + qint64(handedRemaining));
    _reservationHeld = true; _handoffPending = false; _state = QStringLiteral("running"); _active = true;
    _stageDeadlineMs = qMin(_expiresAtMs, _clock.elapsed() + StageMs);
    if (!_gate(generation, true) || !self) {
        if (self) _stop(generation, QStringLiteral("stale"), QStringLiteral("startup-post-handoff-full-source-gate-denied"));
        return rejection(QStringLiteral("startup-post-handoff-full-source-gate-denied"));
    }
    _timer.start();
    const bool published = _publish(generation);
    if (!self) return rejection(QStringLiteral("startup-destroyed-at-publication"));
    if (!published || !_same(generation)) {
        _stop(generation, QStringLiteral("failed"), QStringLiteral("startup-cancelled-or-expired-at-publication"));
        return rejection(QStringLiteral("startup-cancelled-or-expired-at-publication"));
    }
    QVariantMap accepted = current();
    accepted[QStringLiteral("ok")] = true;
    accepted[QStringLiteral("requestAccepted")] = true;
    return accepted; // Admission only; no packet from the factory/transfer stack.
}

bool FleetMissionStartupTransaction::authenticationTransferContextCurrent(const QString& suppliedStartupId, const QString& suppliedSourceHash,
    QObject* actualAuthority, const QVariantMap& suppliedContext)
{
    const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QVariantMap context = suppliedContext;
    QVariantMap comparableContext = context, comparableBasis = _authorityContext;
    if (_groundRestart) {
        comparableContext.remove(QStringLiteral("remainingLifetimeMs"));
        comparableBasis.remove(QStringLiteral("remainingLifetimeMs"));
    }
    if (thread() != QThread::currentThread() || _checking || !_busy || _active || _sendingIndex >= 0
        || _state != QStringLiteral("preparing") || !_handoffPending
        || !_same(_generation) || actualAuthority != (_groundRestart
            ? static_cast<QObject*>(_groundAuthority.data()) : static_cast<QObject*>(_authority.data())) || startupId != _startupId || sourceHash != _sourceHash
        || !outboundGroup(this, {actualAuthority}) || comparableContext != comparableBasis
        || !_authenticationReceipt.isEmpty() || _reservationHeld || !uuid(_transactionEpoch)
        || !uuid(context.value(QStringLiteral("authorityEpoch"))) || context.value(QStringLiteral("sourceHash")) != _sourceHash
        || context.value(QStringLiteral("startupId")) != _startupId
        || context.value(QStringLiteral("deploymentId")) != _deploymentContext.value(QStringLiteral("deploymentId"))
        || context.value(QStringLiteral("binding")).toMap() != _binding
        || !integer(context.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumMs)) return false;
    // current() on these two native owners is a passive value snapshot. Match
    // the actual generation as well as its stable constructor epoch; never
    // follow a new scope created later on the same Authority QObject.
    const QVariantMap actualAuthorityState = _groundRestart ? _groundAuthority->current() : _authority->current();
    if (actualAuthorityState.value(QStringLiteral("startupId")) != _startupId
        || actualAuthorityState.value(QStringLiteral("authorityEpoch")) != context.value(QStringLiteral("authorityEpoch"))
        || actualAuthorityState.value(QStringLiteral("generation")) != context.value(_groundRestart
            ? QStringLiteral("generation") : QStringLiteral("authorityGeneration"))
        || actualAuthorityState.value(QStringLiteral("sourceHash")) != _sourceHash
        || actualAuthorityState.value(QStringLiteral("deploymentId")) != context.value(QStringLiteral("deploymentId"))
        || actualAuthorityState.value(QStringLiteral("binding")).toMap() != _binding
        || actualAuthorityState.value(QStringLiteral("targetVehicleIds")).toList() != _vehicleIds) return false;
    // Pure native ownership/context comparison for BOTH transfer purposes.
    // Request and Authority wrappers retain their full source/OS gates before
    // and after Session's move; recursing into _gate here would reenter them.
    return context.size() == (_groundRestart ? 27 : 25)
        && (_groundRestart ? _groundAuthority && !_authority
            && context.value(QStringLiteral("authorityEpoch")).toString() == _groundAuthority->authorityEpoch()
            : _authority && !_groundAuthority
            && context.value(QStringLiteral("authorityEpoch")).toString() == _authority->authorityEpoch())
        && _clock.elapsed() < _stageDeadlineMs && _clock.elapsed() < _expiresAtMs;
}

void FleetMissionStartupTransaction::_retirePhaseMemorySeals()
{
    _phaseMemorySeals.clear(); _phaseMemoryGeneration = 0; _phaseMemoryStage = -1; _phaseMemoryStartupId.clear();
}

bool FleetMissionStartupTransaction::_phaseMemoryCurrent(quint64 generation) const
{
    if (thread() != QThread::currentThread() || !_same(generation) || _phaseMemoryGeneration != generation
        || _phaseMemoryStage != _stage || _phaseMemoryStartupId != _startupId || _stage < 0 || _stage > 2
        || _clock.elapsed() >= _stageDeadlineMs || _targets.isEmpty() || _targets.size() > 32
        || _phaseMemorySeals.size() != _targets.size()) return false;
    for (int index = 0; index < _targets.size(); ++index) {
        const Target& target = _targets.at(index);
        const PhaseMemorySeal& captured = _phaseMemorySeals.at(index);
        if (!target.vehicle || !target.primary || captured.vehicleId != target.vehicleId
            || captured.vehicle != target.vehicle || captured.primary != target.primary
            || !captured.seal || !captured.seal->current()) return false;
    }
    return true; // No new capture, callback, source substitution or renewal.
}

bool FleetMissionStartupTransaction::_targetsCurrent(quint64 generation, bool* allObserved, QString* reason,
    QList<PhaseMemorySeal>* memorySeals)
{
    if (allObserved) *allObserved = false;
    if (memorySeals) memorySeals->clear();
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QString startupId = _startupId, deploymentId = _authorityContext.value(QStringLiteral("deploymentId")).toString();
    const QList<Target> targets = _targets;
    const QVariantMap constraints = _constraints;
    const int stage = _stage;
    QList<PhaseMemorySeal> capturedSeals;
    bool everyObserved = true;
    const auto same = [&]() { return self && _same(generation) && _stage == stage && deployment && catalog
        && _deployment == deployment && _catalog == catalog && _startupId == startupId && _constraints == constraints
        && _clock.elapsed() < _stageDeadlineMs; };
    const auto fail = [&](const QString& why) { if (reason) *reason = why; return false; };
    if (!memorySeals || !same() || targets.isEmpty() || targets.size() > 32 || stage < 0 || stage > 2)
        return fail(QStringLiteral("whole-actual-target-scope-required"));
    for (const Target& target : targets) {
        const auto alive = [&]() { return same() && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
        if (!alive()) return fail(QStringLiteral("actual-startup-target-destroyed"));
        // Freeze before ANY per-target callback-capable validation. Taking a
        // later token would silently accept state changed after phase capture.
        const auto seal = FleetStartupMemorySeal::capture(target.vehicle.data(), target.primary.data(),
            target.parameters.data(), target.primaryOwner,
            target.beforeArm.value(QStringLiteral("parameterSummary")).toMap()
                .value(QStringLiteral("originalExecutionParameters")).toMap(),
            target.beforeArm.value(QStringLiteral("startupTransitionPolicy")).toMap());
        if (!alive() || !seal) return fail(QStringLiteral("native-startup-memory-seal-unavailable"));
        const QPointer<Vehicle> actualVehicle(deployment->startReviewVehicle(deploymentId, target.vehicleId));
        if (!alive()) return fail(QStringLiteral("target-source-changed-at-vehicle-getter"));
        const QPointer<MissionManager> actualManager(deployment->startReviewManager(deploymentId, target.vehicleId));
        if (!alive() || actualVehicle != target.vehicle || actualManager != target.mission) return fail(QStringLiteral("deployment-target-source-substitution"));
        const auto shared = target.links->primaryLink().lock();
        if (!alive() || !shared || shared.get() != target.primary.data() || target.links->communicationLost()) return fail(QStringLiteral("original-primary-link-changed"));
        // Read this BEFORE passive MM/RAW validators. Those correctly reject
        // receive-stack depth, which is a bounded defer rather than a stale
        // source verdict for this explicitly owned transition.
        const QVariantMap receipt = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, target.stepIds.value(stage).toString());
        if (!alive()) return fail(QStringLiteral("target-deleted-at-native-step-receipt"));
        if (isFalse(receipt.value(QStringLiteral("receiveStackQuiescent"))) || (receipt.isEmpty() && !target.vehicle->fleetStartupTransportQuiescent())) {
            _deferredSource = true; return fail(QStringLiteral("native-receive-stack-awaiting-unwind"));
        }
        if (target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.componentId < 1 || target.componentId > 255 || target.vehicle->parameterManager() != target.parameters.data()
            || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->missionManager() != target.mission.data()
            || target.vehicle->vehicleLinkManager() != target.links.data() || target.vehicle->thread() != thread()
            || target.mission->thread() != thread() || target.parameters->thread() != thread() || target.links->thread() != thread()
            || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || !target.vehicle->isInitialConnectComplete()
            || target.vehicle->isOfflineEditingVehicle() || target.mission->inProgress() || target.mission->transportCallInProgress()
            || !target.parameters->parametersReady() || target.parameters->missingParameters()) return fail(QStringLiteral("native-target-or-foreign-mission-context-changed"));
        const bool pending = target.parameters->pendingWrites();
        if (!alive() || pending) return fail(QStringLiteral("parameter-write-or-target-source-changed"));
        const bool connected = target.primary->isConnected();
        if (!alive() || !connected) return fail(QStringLiteral("original-primary-disconnected"));
        const bool mock = target.primary->isMockLink();
        if (!alive() || mock) return fail(QStringLiteral("mock-primary-forbidden"));
        const bool replay = target.primary->isLogReplay();
        if (!alive() || replay) return fail(QStringLiteral("replay-primary-forbidden"));
        const bool retained = target.mission->fleetMissionReadReceiptRetainedSourceMatches(target.primary.data(), target.readReceipt);
        if (!alive() || !retained) return fail(QStringLiteral("original-complete-raw-read-cache-or-source-changed"));
        // A prepared adapter slot is not a queued step. Native receipt fields
        // commit actual queue accounting before outward notifications, so a
        // reentrant post-queue fence cannot treat expected ARM as unowned.
        const bool issued = isTrue(receipt.value(QStringLiteral("requestIssued")));
        if (target.queued[stage] && !issued) return fail(QStringLiteral("queued-native-step-receipt-lost"));
        if (issued) {
            if (!stepReceipt(receipt, startupId, target.stepIds.value(stage).toString(), stage, target.vehicleId, target.componentId))
                return fail(QStringLiteral("exact-native-queued-step-metadata-required"));
            const bool source = target.vehicle->fleetStartupStepReceiptSourceMatches(self.data(), target.primary.data(), startupId, target.stepIds.value(stage).toString());
            if (!alive() || !source) return fail(QStringLiteral("native-queued-step-source-not-current"));
        }
        const bool skipArm = stage == 1 && _groundRestart && target.reviewedArmed;
        if (skipArm && (issued || target.queued[1])) return fail(QStringLiteral("reviewed-already-armed-target-must-not-have-new-arm-request"));
        const QVariantList ordered = _orderedSteps(target, stage, issued);
        using Phase = FleetMissionStartupTransition::Phase;
        const Phase phase = stage == 0 ? (issued ? Phase::AwaitSeq0 : Phase::BeforeSeq0)
            : stage == 1 ? (issued ? Phase::AwaitArm : Phase::ReadyToArm) : (issued ? Phase::AwaitMode : Phase::ReadyToMode);
        const QVariantMap transition = FleetMissionStartupTransition::capture(target.vehicle.data(), self.data(), startupId,
            target.member, constraints, target.payload, target.beforeArm, ordered, phase, catalog.data());
        if (!alive()) return fail(QStringLiteral("actual-target-or-context-changed-during-phase-capture"));
        if (!isTrue(transition.value(QStringLiteral("match")))) {
            if (isTrue(transition.value(QStringLiteral("waiting")))) _deferredSource = true;
            return fail(transition.value(QStringLiteral("reason")).toString().left(256));
        }
        if (transition.value(QStringLiteral("waiting")).type() != QVariant::Bool || !digest(transition.value(QStringLiteral("phaseFingerprint"))))
            return fail(QStringLiteral("typed-native-phase-result-required"));
        everyObserved = everyObserved && (issued || skipArm) && isFalse(transition.value(QStringLiteral("waiting")));
        const bool readAfter = target.mission->fleetMissionReadReceiptRetainedSourceMatches(target.primary.data(), target.readReceipt);
        if (!alive() || !readAfter) return fail(QStringLiteral("read-proof-changed-after-physical-phase-check"));
        if (!seal->current()) return fail(QStringLiteral("native-startup-state-changed-during-phase-check"));
        PhaseMemorySeal captured;
        captured.vehicleId = target.vehicleId; captured.vehicle = target.vehicle; captured.primary = target.primary; captured.seal = seal;
        capturedSeals.append(captured);
    }
    // A later vehicle's plugin/Link callback may change an earlier vehicle.
    // Do not publish a prefix or recapture that earlier seal without replay.
    if (!same() || capturedSeals.size() != targets.size()) return fail(QStringLiteral("whole-native-phase-memory-set-required"));
    for (const PhaseMemorySeal& captured : capturedSeals)
        if (!captured.vehicle || !captured.primary || !captured.seal || !captured.seal->current())
            return fail(QStringLiteral("whole-native-phase-memory-changed-after-group-check"));
    if (!same()) return fail(QStringLiteral("whole-native-phase-owner-changed"));
    if (allObserved) *allObserved = everyObserved;
    *memorySeals = capturedSeals;
    return true;
}

void FleetMissionStartupTransaction::_tick()
{
    if (_transportDraining) { _drain(); return; }
    if (!_active || _busy || _checking || _sendingIndex >= 0) return;
    _busy = true;
    FlagGuard ticking{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupTransaction> self(this);
    const quint64 generation = _generation;
    const QString startupId = _startupId, epoch = _transactionEpoch;
    const int stage = _stage;
    if (!_live(generation) || _clock.elapsed() >= _stageDeadlineMs) {
        _stop(generation, QStringLiteral("expired"), QStringLiteral("original-startup-or-bounded-stage-deadline-expired")); return;
    }
    bool allObserved = false;
    if (!_gate(generation, true, &allObserved)) {
        if (!self) return;
        if (_deferredSource && _same(generation) && _clock.elapsed() < _stageDeadlineMs) return;
        _stop(generation, QStringLiteral("stale"), _gateReason.isEmpty()
            ? QStringLiteral("startup-authentication-source-risk-or-phase-gate-rejected") : _gateReason); return;
    }
    if (!self || !_same(generation) || _stage != stage) return;
    // Save bounded diagnostic facts only after the full group gate. They are
    // never fed back as permission or used instead of native retained records.
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets[index];
        if (stage == 1 && _groundRestart && target.reviewedArmed) {
            // The full-group typed gate above checked current armed/POSCTL
            // and the continuous Seq0 journal. No fake receipt or ACK.
            _targets[index].observed[stage] = true;
            while (_targets[index].stepResults.size() <= stage) _targets[index].stepResults.append(QVariantMap());
            _targets[index].stepResults[stage] = QVariantMap{{QStringLiteral("step"), stepNames().at(stage)},
                {QStringLiteral("stepSkipped"), true}, {QStringLiteral("reason"), QStringLiteral("reviewed-already-armed-state-preserved")},
                {QStringLiteral("requestIssued"), false}, {QStringLiteral("rawStateObserved"), true},
                {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("executionVerified"), false}};
            _targets[index].state = QStringLiteral("reviewed-arm-retained");
            continue;
        }
        if (!target.queued[stage] || !target.vehicle) continue;
        const QVariantMap receipt = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, target.stepIds[stage].toString());
        if (!self || !_same(generation) || _stage != stage || !target.vehicle) return;
        const QVariantList ordered = _orderedSteps(target, stage, true);
        using Phase = FleetMissionStartupTransition::Phase;
        const QVariantMap phase = FleetMissionStartupTransition::capture(target.vehicle.data(), self.data(), startupId,
            target.member, _constraints, target.payload, target.beforeArm, ordered,
            stage == 0 ? Phase::AwaitSeq0 : stage == 1 ? Phase::AwaitArm : Phase::AwaitMode, _catalog.data());
        if (!self || !_same(generation) || _stage != stage || !target.vehicle || _clock.elapsed() >= _stageDeadlineMs) return;
        if (!isTrue(phase.value(QStringLiteral("match")))) {
            if (isTrue(phase.value(QStringLiteral("waiting")))) return;
            _stop(generation, QStringLiteral("failed"), phase.value(QStringLiteral("reason")).toString()); return;
        }
        _targets[index].observed[stage] = isFalse(phase.value(QStringLiteral("waiting")));
        if (_targets[index].observed[stage]) _targets[index].uncertain[stage] = false;
        const QVariantMap diagnostic{{QStringLiteral("step"), stepNames().at(stage)}, {QStringLiteral("stepId"), target.stepIds[stage]},
            {QStringLiteral("operationId"), receipt.value(QStringLiteral("operationId"))}, {QStringLiteral("requestIssued"), true},
            {QStringLiteral("rawStateObserved"), _targets[index].observed[stage]}, {QStringLiteral("phase"), phase},
            {QStringLiteral("ack"), receipt.value(QStringLiteral("ack"))}, {QStringLiteral("heartbeat"), receipt.value(QStringLiteral("heartbeat"))},
            {QStringLiteral("missionCurrent"), receipt.value(QStringLiteral("missionCurrent"))},
            {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("executionVerified"), false}};
        while (_targets[index].stepResults.size() <= stage) _targets[index].stepResults.append(QVariantMap());
        _targets[index].stepResults[stage] = diagnostic;
        _targets[index].state = _targets[index].observed[stage] ? QStringLiteral("phase-observed") : QStringLiteral("awaiting-new-raw-phase");
    }
    // Diagnostics above contain native callbacks too. Their later captures
    // cannot leave the earlier allObserved decision or the whole-group seal
    // eligible for a barrier/send without a fresh complete phase review.
    if (!_gate(generation, true, &allObserved)) {
        if (!self) return;
        if (_deferredSource && _same(generation) && _clock.elapsed() < _stageDeadlineMs) return;
        _stop(generation, QStringLiteral("stale"), _gateReason.isEmpty()
            ? QStringLiteral("startup-post-diagnostic-whole-group-gate-rejected") : _gateReason); return;
    }
    if (!self || !_same(generation) || _stage != stage) return;
    if (allObserved) {
        // Whole-group barrier; no next-stage packet from this tick/receive
        // stack, and the successful closed lanes keep journaling risks.
        if (stage == 2) { _stop(generation, QStringLiteral("started-observed"), QStringLiteral("new-raw-armed-auto-mission-mode-observed-not-execution-or-completion-proof")); return; }
        ++_stage; _retirePhaseMemorySeals(); _stageDeadlineMs = qMin(_expiresAtMs, _clock.elapsed() + StageMs);
        _publish(generation); return;
    }
    if (_lastQueueAtMs >= 0 && _clock.elapsed() - _lastQueueAtMs < PaceMs) return;
    int index = -1;
    for (int candidate = 0; candidate < _targets.size(); ++candidate)
        if (!_targets[candidate].queued[stage] && !(stage == 1 && _groundRestart && _targets[candidate].reviewedArmed)) { index = candidate; break; }
    if (index < 0) return; // All queued; await genuine RAW, never retry.
    const Target target = _targets[index];
    if (!target.vehicle || !target.primary) { _stop(generation, QStringLiteral("stale"), QStringLiteral("next-target-source-destroyed")); return; }
    if (stage == 2 && !_executionObservation) {
        FleetMissionExecutionObservation* observer = new FleetMissionExecutionObservation(this);
        _executionObservation = observer; _executionObservationRetired = false;
        connect(observer, &FleetMissionExecutionObservation::changed, this, [this, generation]() {
            if (_generation == generation) emit executionObservationChanged();
        });
        if (!observer->prepareFromStartup(this)) {
            if (self) _stop(generation, QStringLiteral("failed"), QStringLiteral("actual-execution-observation-basis-required-before-mode"));
            return;
        }
        if (!self || !_same(generation) || _stage != stage) return;
    }
    // Preparing the actual execution observer may invoke callbacks. Seal the
    // complete current group again before entering Vehicle's native lane.
    if (!_gate(generation, true)) {
        if (!self) return;
        if (_deferredSource && _same(generation) && _clock.elapsed() < _stageDeadlineMs) return;
        _stop(generation, QStringLiteral("stale"), _gateReason.isEmpty()
            ? QStringLiteral("startup-before-send-whole-group-gate-rejected") : _gateReason); return;
    }
    if (!self || !_same(generation) || _stage != stage) return;
    const QString stepId = target.stepIds[stage].toString();
    const qint64 stageDeadline = _stageDeadlineMs, totalDeadline = _expiresAtMs;
    _sendingIndex = index;
    // By-value native fence is rechecked before/after outgoing-plugin work
    // and immediately before real queueing. A callback cancellation/logout
    // cannot authorize a new packet just because transport pointers survive.
    const auto fence = [self, generation, startupId, epoch, stage, index, stepId, target, stageDeadline, totalDeadline]() {
        const auto same = [&]() {
            return self && self->_same(generation) && self->_active && self->_state == QStringLiteral("running")
                && outboundGroup(self.data(), {self->_session.data(), self->_deployment.data(), self->_groundRestart
                    ? static_cast<QObject*>(self->_groundAuthority.data()) : static_cast<QObject*>(self->_authority.data())})
                && self->_startupId == startupId && self->_transactionEpoch == epoch && self->_stage == stage && self->_sendingIndex == index
                && self->_stageDeadlineMs == stageDeadline && self->_expiresAtMs == totalDeadline && self->_clock.elapsed() < stageDeadline
                && self->_clock.elapsed() < totalDeadline && target.vehicle && target.primary && index < self->_targets.size()
                && self->_targets[index].vehicle == target.vehicle && self->_targets[index].primary == target.primary
                && self->_targets[index].stepIds.value(stage).toString() == stepId;
        };
        if (!same()) return false;
        const bool allowed = self->_gate(generation, true);
        return same() && allowed;
    };
    const Vehicle::FleetStartupStep command = stage == 0 ? Vehicle::FleetStartupStep::Seq0
        : stage == 1 ? Vehicle::FleetStartupStep::NormalArm : Vehicle::FleetStartupStep::MissionModeOnly;
    const QVariantMap returned = target.vehicle->sendFleetStartupStep(self.data(), target.primary.data(), startupId, stepId, command, fence);
    if (!self) return; // Destructor/native owner-loss cleanup owns drain.
    if (_generation != generation || _startupId != startupId || _transactionEpoch != epoch) return;
    _sendingIndex = -1;
    QVariantMap actual;
    if (target.vehicle) actual = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, stepId);
    if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch) return;
    // Retain actual partial commit even if cancellation happened inside the
    // queue notification or the return map reports a later gate failure.
    const bool issued = isTrue(actual.value(QStringLiteral("requestIssued"))) || isTrue(returned.value(QStringLiteral("requestIssued")));
    if (issued && index < _targets.size() && _targets[index].vehicle == target.vehicle) {
        _targets[index].queued[stage] = true; _targets[index].uncertain[stage] = true;
        _targets[index].operationId = actual.value(QStringLiteral("operationId")).toString();
        if (_active) _targets[index].state = QStringLiteral("awaiting-new-raw-phase");
        _lastQueueAtMs = _clock.elapsed();
        if (stage > 0) _anyFlightCommandQueued = true;
    }
    if (!_active || !_same(generation)) { _publish(generation); return; }
    if (!issued || !target.vehicle || !stepReceipt(actual, startupId, stepId, stage, target.vehicleId, target.componentId)
        || !isTrue(actual.value(QStringLiteral("ok")))) {
        _stop(generation, QStringLiteral("failed"), QStringLiteral("native-startup-step-not-queued-or-post-queue-context-rejected")); return;
    }
    const bool source = target.vehicle->fleetStartupStepReceiptSourceMatches(self.data(), target.primary.data(), startupId, stepId);
    if (!self || !_same(generation)) return;
    if (!source) { _stop(generation, QStringLiteral("stale"), QStringLiteral("actual-queued-step-source-lost")); return; }
    _publish(generation); // Next fanout/step only on a future paced tick.
}

void FleetMissionStartupTransaction::_stop(quint64 generation, const QString& suppliedState, const QString& suppliedReason)
{
    const QString state = suppliedState, reason = suppliedReason.left(256);
    if (generation == 0 || generation != _generation || (!_active && _state != QStringLiteral("preparing"))) return;
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm);
    const QList<Target> targets = _targets;
    const QString startupId = _startupId, epoch = _transactionEpoch;
    const QVariantList ids = _vehicleIds;
    const bool pending = _handoffPending;
    _active = false; _state = state; _reason = reason; _handoffPending = false;
    _retirePhaseMemorySeals();
    _timer.stop(); _disconnectSources();
    if (_executionObservation) _executionObservation->startupRetired(state, reason);
    if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch || _state != state) return;
    // If destruction/cancel occurs while the native handoff is unwinding,
    // this exact UUID may already have moved despite no wrapper commit yet.
    if (pending && swarm && swarm->missionDeploymentOwns(startupId, ids)) _reservationHeld = true;
    if (session) {
        if (_groundRestart) session->revokeGroundStartupTransactionAuthentication(self.data(), startupId, epoch);
        else session->revokeStartupTransactionAuthentication(self.data(), startupId, epoch);
    }
    if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch || _state != state) return;
    for (int index = 0; index < targets.size(); ++index) {
        const Target target = targets[index];
        if (!target.vehicle) continue;
        for (int step = 0; step < 3; ++step) {
            const QVariantMap receipt = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, target.stepIds.value(step).toString());
            if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch) return;
            if (isTrue(receipt.value(QStringLiteral("requestIssued"))) && index < _targets.size()) {
                _targets[index].queued[step] = true;
                _targets[index].uncertain[step] = !_targets[index].observed[step];
                if (step > 0) _anyFlightCommandQueued = true;
                while (_targets[index].stepResults.size() <= step) _targets[index].stepResults.append(QVariantMap());
                QVariantMap diagnostic = _targets[index].stepResults[step].toMap();
                diagnostic[QStringLiteral("step")] = stepNames()[step];
                diagnostic[QStringLiteral("stepId")] = target.stepIds.value(step);
                diagnostic[QStringLiteral("operationId")] = receipt.value(QStringLiteral("operationId"));
                diagnostic[QStringLiteral("terminalRawFacts")] = QVariantMap{
                    {QStringLiteral("ack"), receipt.value(QStringLiteral("ack"))}, {QStringLiteral("heartbeat"), receipt.value(QStringLiteral("heartbeat"))},
                    {QStringLiteral("missionCurrent"), receipt.value(QStringLiteral("missionCurrent"))},
                    {QStringLiteral("receiveStackQuiescent"), receipt.value(QStringLiteral("receiveStackQuiescent"))},
                    {QStringLiteral("nativeSourceCurrentAtCapture"), receipt.value(QStringLiteral("ok"))},
                    {QStringLiteral("typedPhaseRevalidatedAtTerminal"), false}, {QStringLiteral("requestCausalityVerified"), false}};
                _targets[index].stepResults[step] = diagnostic;
            }
            if (target.vehicle) target.vehicle->cancelFleetStartupStep(self.data(), startupId, target.stepIds.value(step).toString());
            if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch) return;
        }
        if (target.vehicle) target.vehicle->retireFleetStartupAltitudeMonitor(self.data(), startupId);
        if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch) return;
        if (index < _targets.size()) { _targets[index].state = state; _targets[index].reason = reason; }
    }
    _transportDraining = _reservationHeld;
    _drain();
    if (self && _generation == generation && _startupId == startupId && _transactionEpoch == epoch && _state == state) _publish(generation);
}

void FleetMissionStartupTransaction::_drain()
{
    if (!_reservationHeld || _active || _state == QStringLiteral("preparing")) { _transportDraining = false; return; }
    // A terminal callback may occur inside the sender/fence. Wait until that
    // actual call has returned before deciding whether MODE was unissued.
    if (_busy || _checking || _sendingIndex >= 0) { _transportDraining = true; _timer.start(); return; }
    const QPointer<FleetMissionStartupTransaction> self(this);
    const quint64 generation = _generation;
    const QString startupId = _startupId, epoch = _transactionEpoch, state = _state;
    const QPointer<SwarmController> swarm(_swarm);
    const QList<Target> targets = _targets;
    for (const Target& target : targets) {
        if ((target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())
            || (target.vehicle && !target.vehicle->fleetStartupAltitudeTransportQuiescent(startupId, epoch))
            || (target.mission && target.mission->transportCallInProgress())) {
            _transportDraining = true; _timer.start(); return;
        }
    }
    const QPointer<FleetMissionExecutionObservation> observer(_executionObservation);
    if (observer && !_executionObservationRetired && !observer->hasCommittedMode()) {
        const bool retired = observer->retireUnissuedPreparation(this);
        if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch || _state != state
            || _executionObservation != observer) return;
        if (retired) _executionObservationRetired = true;
    }
    if (_executionObservation && !_executionObservationRetired && _executionObservation->hasCommittedMode()) {
        const bool adopted = _executionObservation->adoptReservation(swarm.data(), startupId, _vehicleIds);
        if (!self || _generation != generation || _startupId != startupId || _transactionEpoch != epoch || _state != state) return;
        if (!adopted && swarm && swarm->missionDeploymentOwns(startupId, _vehicleIds)) {
            // Keep the actual old whole-group reservation when an observation
            // handoff is not established. Never create a release/reserve gap.
            // Explicit native monitoring retirement can drain it later.
            _transportDraining = true; _timer.start(); return;
        }
        // An already revoked/lost old reservation is not reconstructed. The
        // Run keeps its queue/history facts without claiming a live lease.
    }
    _transportDraining = false; _reservationHeld = false; _timer.stop();
    if (swarm) swarm->releaseMissionDeployment(startupId);
    if (self && _generation == generation && _startupId == startupId && _transactionEpoch == epoch && _state == state) _publish(generation);
}

void FleetMissionStartupTransaction::_detachDrain()
{
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm);
    const QString startupId = _startupId, epoch = _transactionEpoch;
    const QVariantList ids = _vehicleIds;
    const QList<Target> targets = _targets;
    const bool held = _reservationHeld || (_handoffPending && swarm && swarm->missionDeploymentOwns(startupId, ids));
    const bool groundRestart = _groundRestart;
    _timer.stop();
    // Close ownership before any native notification; the captured old UUID
    // and targets below survive replacement without touching its new fields.
    _reservationHeld = false; _handoffPending = false; _transportDraining = false;
    if (session && !startupId.isEmpty()) {
        if (groundRestart) session->revokeGroundStartupTransactionAuthentication(this, startupId, epoch);
        else session->revokeStartupTransactionAuthentication(this, startupId, epoch);
    }
    // Destruction calls exact cancellation while this native owner is still
    // alive. The detached drain NEVER retains/calls a dangling owner pointer.
    for (const Target& target : targets) if (target.vehicle) {
        for (const QVariant& step : target.stepIds) target.vehicle->cancelFleetStartupStep(this, startupId, step.toString());
        if (target.vehicle) target.vehicle->retireFleetStartupAltitudeMonitor(this, startupId);
    }
    if (!held || !swarm || !uuid(startupId)) return;
    QTimer* timer = new QTimer(swarm.data()); timer->setInterval(PaceMs);
    const QPointer<QTimer> weak(timer);
    const auto release = [weak, swarm, startupId, epoch, targets]() {
        if (!weak) return;
        if (!swarm) { weak->deleteLater(); return; }
        for (const Target& target : targets)
            if ((target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())
                || (target.vehicle && !target.vehicle->fleetStartupAltitudeTransportQuiescent(startupId, epoch))
                || (target.mission && target.mission->transportCallInProgress())) return;
        weak->stop(); swarm->releaseMissionDeployment(startupId);
        if (weak) weak->deleteLater();
    };
    connect(timer, &QTimer::timeout, timer, release);
    timer->start(); // Passive draining only. Never dispatches/retries/rolls back.
}

bool FleetMissionStartupTransaction::cancel(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || startupId != _startupId || (!_active && _state != QStringLiteral("preparing"))) return false;
    _stop(_generation, QStringLiteral("cancelled"), QStringLiteral("operator-cancelled-no-disarm-or-rollback"));
    return true;
}
bool FleetMissionStartupTransaction::revokeStartup(const QString& startupId) { return cancel(startupId); }

bool FleetMissionStartupTransaction::_gate(quint64 generation, bool requireAuthentication, bool* allObserved)
{
    if (allObserved) *allObserved = false;
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    if (!outboundGroup(this, {_session.data(), _deployment.data(), _groundRestart
        ? static_cast<QObject*>(_groundAuthority.data()) : static_cast<QObject*>(_authority.data())})) return false;
    _checking = true; _reentered = false; _deferredSource = false; _gateReason.clear();
    // An unsuccessful/reentered gate cannot leave a previous check usable by
    // commandContextCurrent. Only the final complete, pure tail publishes.
    _retirePhaseMemorySeals();
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionStartupAuthority> authority(_authority);
    const QPointer<FleetMissionGroundStartupAuthority> groundAuthority(_groundAuthority);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<QObject> owner(_authorization);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FaultToleranceManager> faults(_faultPolicy);
    const auto clearanceWitness = _startupClearanceWitness;
    const bool clearanceRequired = _startupClearanceRequired;
    const bool hadRegistry = bool(registry), hadFaults = bool(faults), held = _reservationHeld, pending = _handoffPending, groundRestart = _groundRestart;
    const QVariantMap identity = _identityReceipt, role = _roleReceipt, authentication = _authenticationReceipt;
    const QVariantMap binding = _binding, healthGate = _healthGate, catalogSnapshot = _catalogSnapshot, frozenDeployment = _deploymentContext;
    const QVariantList ids = _vehicleIds;
    const QList<Target> targets = _targets;
    const QByteArray bytes = _recordBytes;
    const QString startupId = _startupId, sourceHash = _sourceHash, epoch = _transactionEpoch, state = _state;
    const int stage = _stage, leader = _leaderId;
    const bool requireLeader = _requireLeader, registered = _leaderRegistered;
    const auto same = [&]() {
        return self && !_reentered && _same(generation) && _clock.elapsed() < _stageDeadlineMs && _state == state && _stage == stage
            && session && roles && store && (groundRestart ? bool(groundAuthority) && !authority : bool(authority) && !groundAuthority)
            && deployment && swarm && health && owner && coordinator && catalog
            && _session == session && _roles == roles && _store == store && _authority == authority && _groundAuthority == groundAuthority
            && _groundRestart == groundRestart && _deployment == deployment
            && _swarm == swarm && _health == health && _authorization == owner && _coordinator == coordinator && _catalog == catalog
            && _registry == registry && _faultPolicy == faults && (!hadRegistry || registry) && (!hadFaults || faults)
            && _identityReceipt == identity && _roleReceipt == role && _authenticationReceipt == authentication && _binding == binding
            && _healthGate == healthGate && _catalogSnapshot == catalogSnapshot && _deploymentContext == frozenDeployment && _recordBytes == bytes
            && _vehicleIds == ids && _startupId == startupId && _sourceHash == sourceHash && _transactionEpoch == epoch
            && _reservationHeld == held && _handoffPending == pending && _leaderId == leader && _requireLeader == requireLeader && _leaderRegistered == registered
            && _startupClearanceWitness == clearanceWitness && _startupClearanceRequired == clearanceRequired;
    };
    bool observed = false;
    QList<PhaseMemorySeal> candidateSeals;
    const auto native = [&]() {
        // This full source pass may enter plugin/fence callbacks. Run it
        // BEFORE the existing identity/authentication and target gates;
        // the callback-free witness tail below is the final consumer.
        if (!same()) return false;
        if (clearanceRequired ? !clearanceWitness || !clearanceWitness->sourceCurrent(same) : bool(clearanceWitness)) {
            if (same()) _gateReason = QStringLiteral("original-startup-clearance-witness-source-or-deadline-changed");
            return false;
        }
        if (!same() || !validRole(identity, role) || !roles->actionSnapshotCurrent(role)) return false;
        const bool actor = session->identitySnapshotCurrent(identity);
        if (!same() || !actor) return false;
        if (requireAuthentication) {
            const bool proof = groundRestart ? session->groundStartupTransactionAuthenticationCurrent(authentication, self.data(), startupId, epoch, sourceHash)
                : session->startupTransactionAuthenticationCurrent(authentication, self.data(), startupId, epoch, sourceHash);
            if (!same() || !proof) return false;
        }
        for (const Target& target : targets) {
            if (!same() || !target.vehicle) return false;
            const QVariantMap step = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, target.stepIds.value(stage).toString());
            if (!same() || !target.vehicle) return false;
            if (isFalse(step.value(QStringLiteral("receiveStackQuiescent"))) || (step.isEmpty() && !target.vehicle->fleetStartupTransportQuiescent())) {
                _deferredSource = true; return false;
            }
        }
        // The old Authority already owns this exact UUID during preparation;
        // after native move only the transaction holds it, without a gap.
        if (!swarm->missionDeploymentOwns(startupId, ids) || !same() || swarm->formationActive() || swarm->formationBusy()
            || swarm->hasActiveTemporaryMission(ids) || !same()) return false;
        if (health->fleetRegistry() != registry.data() || health->faultToleranceManager() != faults.data()) return false;
        const QVariantMap actualHealth = health->evaluateTaskGate(ids, requireLeader);
        if (!same() || actualHealth != healthGate || !isTrue(actualHealth.value(QStringLiteral("allowed")))
            || (faults && faults->leaderVehicleId() != leader) || (registry && leader > 0 && registry->containsVehicle(leader) != registered)) return false;
        if (coordinator->firmwareAdapterCatalogBackend() != catalog.data()) return false;
        const QVariantMap actualCatalog = catalog->snapshot();
        if (!same() || actualCatalog != catalogSnapshot) return false;
        QByteArray actualRecord;
        if (!currentRecord(store.data(), binding, &actualRecord) || !same() || actualRecord != bytes) return false;
        const QVariantMap actualDeployment = deployment->startReviewContext(frozenDeployment.value(QStringLiteral("deploymentId")).toString());
        if (!same() || actualDeployment != frozenDeployment) return false;
        QString targetReason;
        const bool targetsMatch = _targetsCurrent(generation, &observed, &targetReason, &candidateSeals);
        if (same() && !targetsMatch) _gateReason = targetReason.left(256);
        if (!targetsMatch || !same()) return false;
        return targetsMatch && same();
    };
    if (!native()) return false;
    QVariant matched;
    const QVariantMap bundle = _bundle();
    const bool sourcesInvoked = QMetaObject::invokeMethod(owner.data(), groundRestart ? "fleetGroundStartupTransactionSourcesMatch" : "fleetStartupTransactionSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!same() || !sourcesInvoked || !isTrue(matched) || !native()) return false;
    QVariant allowed;
    const bool contextInvoked = QMetaObject::invokeMethod(owner.data(), groundRestart ? "fleetGroundStartupTransactionContextAllowed" : "fleetStartupTransactionContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(startupId)), Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(ids)));
    if (!same() || !contextInvoked || !isTrue(allowed) || !native()) return false;
    // All callbacks have ended. Check the original complete source group a
    // final time without consulting the retired Review or rerunning solver.
    if (clearanceRequired ? !clearanceWitness || !clearanceWitness->memoryCurrent() : bool(clearanceWitness)) return false;
    if (!same() || candidateSeals.size() != targets.size()) return false;
    for (const PhaseMemorySeal& captured : candidateSeals)
        if (!captured.vehicle || !captured.primary || !captured.seal || !captured.seal->current()) return false;
    if (!same()) return false;
    _phaseMemorySeals = candidateSeals; _phaseMemoryGeneration = generation;
    _phaseMemoryStage = stage; _phaseMemoryStartupId = startupId;
    if (!_phaseMemoryCurrent(generation) || !same()) { _retirePhaseMemorySeals(); return false; }
    if (allObserved) *allObserved = observed;
    return true;
}

bool FleetMissionStartupTransaction::_terminalReassessmentEligible(const QString& startupId, bool allowOwnedRead,
    QObject* reviewObject) const
{
    const QPointer<FleetMissionStartupTransaction> self(const_cast<FleetMissionStartupTransaction*>(this));
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(reviewObject));
    const quint64 generation = _generation;
    const auto same = [&]() {
        return self && thread() == QThread::currentThread() && generation != 0 && _generation == generation
            && startupId == _startupId && !_active && !_busy && !_checking && _sendingIndex < 0
            && !_reservationHeld && !_handoffPending && !_transportDraining
            && _state != QStringLiteral("empty") && _state != QStringLiteral("preparing") && _state != QStringLiteral("running")
            && _session && _roles && _store && _deployment && _swarm && _health && _authorization && _catalog && _coordinator
            && outboundGroup(self.data(), {_session.data(), _deployment.data()});
    };
    if (!same() || (_executionObservation && !_executionObservationRetired) || _targets.isEmpty() || _targets.size() > 32
        || !nativeAuthorityReferenceShape(_nativeAuthorityReference, _groundRestart)
        || _nativeAuthorityReference.value(QStringLiteral("authorityEpoch")) != _authorityContext.value(QStringLiteral("authorityEpoch"))
        || _nativeAuthorityReference.value(QStringLiteral("sourceHash")) != _sourceHash
        || (allowOwnedRead && (!review || review->thread() != thread() || !outboundGroup(this, {review.data()})))) return false;
    QByteArray actualRecord;
    if (!currentRecord(_store.data(), _binding, &actualRecord) || !same() || actualRecord != _recordBytes
        || _catalog->snapshot() != _catalogSnapshot || !same()
        || _coordinator->firmwareAdapterCatalogBackend() != _catalog.data()
        || _health->fleetRegistry() != _registry.data() || _health->faultToleranceManager() != _faultPolicy.data()) return false;
    const QList<Target> retainedTargets = _targets;
    for (const Target& target : retainedTargets) {
        const auto targetAlive = [&]() { return same() && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
        if (!same() || !target.vehicle || !target.mission || !target.parameters || !target.links || !target.primary
            || target.queued[2] || target.uncertain[2]
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.links->communicationLost() || target.links->primaryLink().lock().get() != target.primary.data()
            || !target.vehicle->isInitialConnectComplete() || target.vehicle->isOfflineEditingVehicle()
            || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || target.vehicle->flying()) return false;
        const bool pending = target.parameters->pendingWrites();
        if (!targetAlive() || pending || !target.parameters->parametersReady() || target.parameters->missingParameters()) return false;
        // A local terminal field alone cannot hide a packet queued during a
        // callback. The original actual native lane is consulted as well.
        const QVariantMap mode = target.vehicle->fleetStartupStepReceipt(self.data(), startupId, target.stepIds.value(2).toString());
        if (!targetAlive() || isTrue(mode.value(QStringLiteral("requestIssued")))) return false;
        const QString scope = allowOwnedRead ? review->readScopeForTarget(target.vehicle.data()) : QString();
        const bool missionBusy = target.mission->inProgress() || target.mission->transportCallInProgress();
        const bool ownedRead = allowOwnedRead && review && !scope.isEmpty() && missionBusy
            && (target.mission->ownsFleetMissionRead(scope, target.primary.data(), 0)
                || target.mission->ownsFleetMissionRead(scope, target.primary.data(), target.mission->activeOperationId()));
        if (!targetAlive() || (missionBusy && !ownedRead)) return false;
        // New INT reply processing sends its next request synchronously. The
        // original terminal admission still requires ALL stacks quiescent;
        // retained sources may tolerate only this exact owner's current READ
        // receive stack, never an ARM/flight/version/coordinate receive stack.
        if (!target.vehicle->fleetStartupTransportQuiescent()
            && (!ownedRead || !target.vehicle->fleetArmedGroundReviewSourceMatches(target.primary.data()))) return false;
        if (_deployment->startReviewVehicle(_authorityContext.value(QStringLiteral("deploymentId")).toString(), target.vehicleId) != target.vehicle.data()
            || _deployment->startReviewManager(_authorityContext.value(QStringLiteral("deploymentId")).toString(), target.vehicleId) != target.mission.data()
            || !targetAlive()) return false;
        const QVariantMap raw = target.vehicle->flightStateRiskObservation();
        const QVariantMap rows = raw.value(QStringLiteral("channels")).toMap();
        const QVariantMap heartbeatRow = rows.value(QStringLiteral("heartbeat")).toMap(), landedRow = rows.value(QStringLiteral("landedState")).toMap();
        const QVariantMap positionRow = rows.value(QStringLiteral("globalPosition")).toMap();
        const QVariantMap hb = heartbeatRow.value(QStringLiteral("value")).toMap(), landed = landedRow.value(QStringLiteral("value")).toMap();
        const QVariantMap healthTelemetry = target.vehicle->fleetHealthTelemetryObservation();
        QVariantMap estimator;
        if (!targetAlive() || !FleetMissionStartupEstimator::capture(healthTelemetry,
                target.vehicleId, target.componentId, 3000, &estimator)
            || !FleetMissionStartupEstimator::healthyReference(estimator)) return false;
        px4_custom_mode observed{}; observed.data = hb.value(QStringLiteral("customMode")).toUInt();
        const uint base = hb.value(QStringLiteral("baseMode")).toUInt();
        if (!isTrue(heartbeatRow.value(QStringLiteral("available"))) || !isTrue(heartbeatRow.value(QStringLiteral("fresh")))
            || !isTrue(landedRow.value(QStringLiteral("available"))) || !isTrue(landedRow.value(QStringLiteral("fresh")))
            || !isTrue(positionRow.value(QStringLiteral("available"))) || !isTrue(positionRow.value(QStringLiteral("fresh")))
            || hb.value(QStringLiteral("armed")).type() != QVariant::Bool || hb.value(QStringLiteral("armed")).toBool() != target.vehicle->armed()
            || (base & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) == 0 || (base & (MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_HIL_ENABLED)) != 0
            || observed.reserved != 0 || observed.main_mode != PX4_CUSTOM_MAIN_MODE_POSCTL || observed.sub_mode != PX4_CUSTOM_SUB_MODE_POSCTL_POSCTL
            || !isTrue(landed.value(QStringLiteral("landedStateKnown"))) || landed.value(QStringLiteral("landedState")).toInt() != MAV_LANDED_STATE_ON_GROUND
            || !targetAlive()) return false;
    }
    return same();
}

namespace {
QVariantMap terminalRiskProjection(QVariantMap context)
{
    QVariantList physical;
    for (const QVariant& entry : context.value(QStringLiteral("physicalTargets")).toList()) {
        QVariantMap row = entry.toMap(), raw = row.value(QStringLiteral("currentFlightObservation")).toMap();
        QVariantMap channels = raw.value(QStringLiteral("channels")).toMap();
        for (auto it = channels.begin(); it != channels.end(); ++it) {
            QVariantMap channel = it.value().toMap(), value = channel.value(QStringLiteral("value")).toMap();
            if (it.key() == QStringLiteral("globalPosition")) value.remove(QStringLiteral("timeBootMs"));
            channel.insert(QStringLiteral("value"), value);
            for (const QString& key : {QStringLiteral("ageMs"), QStringLiteral("receiptSequence"), QStringLiteral("contentHash"), QStringLiteral("contentRevision")}) channel.remove(key);
            it.value() = channel;
        }
        raw.insert(QStringLiteral("channels"), channels); row.insert(QStringLiteral("currentFlightObservation"), raw); physical.append(row);
    }
    context.insert(QStringLiteral("physicalTargets"), physical); context.remove(QStringLiteral("phaseFingerprint"));
    return context;
}
}

QVariantMap FleetMissionStartupTransaction::terminalReassessmentContext(const QString& suppliedStartupId)
{
    return _captureTerminalReassessmentContext(suppliedStartupId, true);
}
QVariantMap FleetMissionStartupTransaction::_captureTerminalReassessmentContext(const QString& suppliedStartupId, bool retainReference)
{
    const QString startupId = suppliedStartupId;
    const QPointer<FleetMissionStartupTransaction> self(this);
    if (!_terminalReassessmentEligible(startupId)) return rejection(QStringLiteral("terminal-drained-never-mode-queued-ground-sources-required"));
    const quint64 generation = _generation;
    const QList<Target> targets = _targets;
    QVariantList physical;
    for (const Target& target : targets) {
        if (!self || _generation != generation || !target.vehicle)
            return rejection(QStringLiteral("terminal-target-retired-before-new-review-capture"));
        const bool armed = target.vehicle->armed();
        const QVariantMap readiness = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, _constraints, target.payload,
            armed ? FleetMissionStartReadiness::Phase::ArmedGroundReady : FleetMissionStartReadiness::Phase::BeforeArm, _catalog.data());
        if (!self || _generation != generation || !_terminalReassessmentEligible(startupId)
            || !isTrue(readiness.value(QStringLiteral("startupPhasePreconditionsMatch")))) return rejection(QStringLiteral("fresh-current-ground-phase-or-configuration-unavailable"));
        QVariantList steps;
        for (int step = 0; step < 3; ++step) steps.append(QVariantMap{{QStringLiteral("step"), stepNames().at(step)},
            {QStringLiteral("stepId"), target.stepIds.value(step)}, {QStringLiteral("requestIssued"), target.queued[step]},
            {QStringLiteral("mayHaveCommitted"), target.queued[step] || target.uncertain[step]},
            {QStringLiteral("rawStateObserved"), target.observed[step]}, {QStringLiteral("requestCausalityVerified"), false}});
        const QVariantMap estimator = readiness.value(QStringLiteral("startupEstimatorObservation")).toMap();
        if (readiness.value(QStringLiteral("startupEstimatorObservation")).type() != QVariant::Map
            || !isTrue(readiness.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
            || !isTrue(readiness.value(QStringLiteral("startupEstimatorEvidenceMatches")))
            || !FleetMissionStartupEstimator::healthyReference(estimator))
            return rejection(QStringLiteral("fresh-native-estimator-ground-review-basis-required"));
        const QVariantMap origin = readiness.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap();
        const QString originFingerprint = QString::fromLatin1(QCryptographicHash::hash(
            QJsonDocument::fromVariant(origin).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
        if (readiness.value(QStringLiteral("startupGlobalOriginReferenceObservation")).type() != QVariant::Map
            || !isTrue(readiness.value(QStringLiteral("startupGlobalOriginReferenceCaptured")))
            || !isTrue(readiness.value(QStringLiteral("startupGlobalOriginReferenceMatches")))
            || !FleetMissionStartupGlobalOrigin::admissible(origin)
            || !digest(readiness.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")))
            || originFingerprint != readiness.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")).toString())
            return rejection(QStringLiteral("fresh-native-global-origin-ground-review-basis-required"));
        const QVariantMap flight = target.vehicle->flightStateRiskObservation();
        if (!self || _generation != generation || !target.vehicle || !_terminalReassessmentEligible(startupId))
            return rejection(QStringLiteral("terminal-source-retired-during-new-review-flight-capture"));
        physical.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("componentId"), target.componentId},
            {QStringLiteral("member"), target.member}, {QStringLiteral("payload"), target.payload},
            {QStringLiteral("previousReadReceipt"), target.readReceipt}, {QStringLiteral("previousSteps"), steps},
            {QStringLiteral("currentFlightObservation"), flight},
            // Stable native source/revision/flags only. Receive age and the
            // advancing sensor time do not churn this new-review identity.
            {QStringLiteral("startupEstimatorObservation"), estimator},
            // Preserve the initial event reference, including its observed
            // epoch/revision; later callbacks cannot silently rebase it.
            {QStringLiteral("startupGlobalOriginReferenceObservation"), origin},
            {QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"), originFingerprint},
            {QStringLiteral("startupGlobalOriginReferenceCaptured"), true},
            {QStringLiteral("startupGlobalOriginReferenceMatches"), true},
            {QStringLiteral("phase"), armed ? QStringLiteral("armed-ground-posctl") : QStringLiteral("disarmed-ground-posctl")}});
    }
    if (!self || _generation != generation || !_health)
        return rejection(QStringLiteral("terminal-health-owner-retired-before-new-review-gate"));
    const QVariantMap health = _health->evaluateTaskGate(_vehicleIds, _requireLeader);
    if (!self || _generation != generation || !_terminalReassessmentEligible(startupId) || !isTrue(health.value(QStringLiteral("allowed"))))
        return rejection(QStringLiteral("terminal-current-ground-health-or-source-changed"));
    QVariantMap context{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true}, {QStringLiteral("startupId"), startupId},
        {QStringLiteral("transactionEpoch"), _transactionEpoch}, {QStringLiteral("generation"), QString::number(generation)},
        {QStringLiteral("previousSourceHash"), _sourceHash}, {QStringLiteral("deploymentId"), _authorityContext.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("targetVehicleIds"), _vehicleIds},
        {QStringLiteral("deploymentContext"), _deploymentContext}, {QStringLiteral("previousAuthorityContext"), _nativeAuthorityReference},
        {QStringLiteral("physicalTargets"), physical}, {QStringLiteral("currentHealthGate"), health}, {QStringLiteral("catalogSnapshot"), _catalogSnapshot},
        {QStringLiteral("phaseFingerprint"), QString()}, {QStringLiteral("purpose"), QStringLiteral("new-ground-read-required-old-authority-not-restored")},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("previousAuthorizationRestored"), false}};
    QByteArray bytes;
    if (!encode(terminalRiskProjection(context), &bytes)) return rejection(QStringLiteral("terminal-context-resource-limit"));
    context[QStringLiteral("phaseFingerprint")] = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    if (!self || _generation != generation || !_terminalReassessmentEligible(startupId)
        || !self || _generation != generation) return rejection(QStringLiteral("terminal-source-changed-before-reference-capture"));
    // All callback-capable flight/health/eligibility reads have finished. The
    // following native origin getter and source matcher are memory-only.
    // Do not replace the initial stable18 in this capture with a newer basis.
    for (int index = 0; index < targets.size(); ++index) {
        const Target& target = targets.at(index);
        if (!self || _generation != generation || !target.vehicle || !target.links || !target.primary
            || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.links->primaryLink().lock().get() != target.primary.data())
            return rejection(QStringLiteral("terminal-global-origin-source-retired-before-reference-capture"));
        const QVariantMap origin = physical.at(index).toMap().value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap();
        QVariantMap currentOrigin;
        const QVariantMap rawOrigin = target.vehicle->startupGlobalOriginObservation();
        if (!self || _generation != generation || !target.vehicle || !target.links || !target.primary
            || !FleetMissionStartupGlobalOrigin::capture(rawOrigin, target.vehicleId, target.componentId, &currentOrigin)
            || !FleetMissionStartupGlobalOrigin::same(origin, currentOrigin)
            || !target.vehicle->startupGlobalOriginObservationSourceMatches(target.primary.data()))
            return rejection(QStringLiteral("terminal-global-origin-reference-changed-before-reference-capture"));
    }
    // Explicit new native admission captures can replace the retained RAW
    // reference for a NEW review. Comparison-only captures never replace it;
    // neither path reconstructs permission or an old authentication budget.
    if (retainReference) _reassessmentReferenceContext = context;
    return context;
}

bool FleetMissionStartupTransaction::terminalReassessmentContextCurrent(const QString& startupId, const QVariantMap& expected)
{
    if (thread() != QThread::currentThread() || expected.size() != 20 || expected != _reassessmentReferenceContext
        || !digest(expected.value(QStringLiteral("phaseFingerprint")))) return false;
    const QPointer<FleetMissionStartupTransaction> self(this);
    const QVariantMap actual = _captureTerminalReassessmentContext(startupId, false);
    QByteArray expectedBytes, actualBytes;
    return self && isTrue(actual.value(QStringLiteral("ok"))) && encode(terminalRiskProjection(expected), &expectedBytes)
        && encode(terminalRiskProjection(actual), &actualBytes) && expectedBytes == actualBytes
        && expected.value(QStringLiteral("phaseFingerprint")) == actual.value(QStringLiteral("phaseFingerprint"));
}

QVariantMap FleetMissionStartupTransaction::actualSourcesForReassessment(const QString& startupId)
{
    if (!_terminalReassessmentEligible(startupId)) return {};
    QVariantMap sources = _bundle(); sources.remove(QStringLiteral("authority"));
    sources.insert(QStringLiteral("previousStartup"), QVariant::fromValue(static_cast<QObject*>(this)));
    return sources;
}

bool FleetMissionStartupTransaction::terminalReassessmentSourcesRetained(QObject* reviewObject, const QString& startupId, const QVariantMap& context) const
{
    if (thread() != QThread::currentThread()) return false;
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(reviewObject));
    if (!review || !outboundGroup(this, {review.data()}) || context.size() != 20 || context != _reassessmentReferenceContext
        || context.value(QStringLiteral("startupId")) != _startupId
        || context.value(QStringLiteral("transactionEpoch")) != _transactionEpoch
        || context.value(QStringLiteral("generation")).toString() != QString::number(_generation)
        || context.value(QStringLiteral("previousSourceHash")) != _sourceHash || context.value(QStringLiteral("binding")).toMap() != _binding
        || context.value(QStringLiteral("targetVehicleIds")).toList() != _vehicleIds || context.value(QStringLiteral("deploymentContext")).toMap() != _deploymentContext
        || context.value(QStringLiteral("previousAuthorityContext")).toMap() != _nativeAuthorityReference
        || !review->retainedPreviousSourceContextMatches(const_cast<FleetMissionStartupTransaction*>(this), startupId, context)) return false;
    // Physical new-review facts are checked by its actual owner. This passive
    // old-source check deliberately permits only that owner's new READ.
    return _terminalReassessmentEligible(startupId, true, review.data());
}

Vehicle* FleetMissionStartupTransaction::terminalReassessmentVehicle(const QString& startupId, int vehicleId) const
{
    if (!_terminalReassessmentEligible(startupId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.vehicle.data();
    return nullptr;
}
MissionManager* FleetMissionStartupTransaction::terminalReassessmentManager(const QString& startupId, int vehicleId) const
{
    if (!terminalReassessmentVehicle(startupId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.mission.data();
    return nullptr;
}
LinkInterface* FleetMissionStartupTransaction::terminalReassessmentPrimaryLink(const QString& startupId, int vehicleId) const
{
    if (!terminalReassessmentVehicle(startupId, vehicleId)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.primary.data();
    return nullptr;
}
FleetFirmwareAdapterCatalog* FleetMissionStartupTransaction::nativeReassessmentFirmwareCatalog(const QString& startupId) const
{
    return _terminalReassessmentEligible(startupId) ? _catalog.data() : nullptr;
}
FleetMissionReviewCoordinator* FleetMissionStartupTransaction::nativeReassessmentReviewCoordinator(const QString& startupId) const
{
    return _terminalReassessmentEligible(startupId) ? _coordinator.data() : nullptr;
}
FleetRegistry* FleetMissionStartupTransaction::nativeReassessmentRegistry(const QString& startupId) const
{
    return _terminalReassessmentEligible(startupId) ? _registry.data() : nullptr;
}
FaultToleranceManager* FleetMissionStartupTransaction::nativeReassessmentFaultPolicy(const QString& startupId) const
{
    return _terminalReassessmentEligible(startupId) ? _faultPolicy.data() : nullptr;
}

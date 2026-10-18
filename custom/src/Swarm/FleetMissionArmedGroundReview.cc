#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionStartupEstimator.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionClearanceSource.h"
#include "FleetMissionClearanceDatum.h"
#include "FleetMissionStartupClearanceWorker.h"
#include "FleetMissionStartupClearanceWitness.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetAirspaceConflictChecker.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetRegistry.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionRiskEvaluator.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "QGCMAVLink.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QMetaType>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
bool outboundSources(const QObject* owner, const FleetOperatorSession* session,
    const FleetMissionStartupTransaction* previous, const FleetMissionDeployment* deployment)
{
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, session)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, previous)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, deployment);
}
constexpr qint64 BatchLifetimeMs = 120000;
constexpr qint64 ReviewLifetimeMs = 30000;
constexpr int PollMs = 100;
constexpr double ExactMaximum = 9007199254740991.0;
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
struct FlagGuard {
    QPointer<QObject> object;
    bool* flag;
    ~FlagGuard() { if (object) *flag = false; }
};
FleetMissionStartupGeographicMotion::Models copyModels(const FleetMissionStartupGeographicMotion::Models& source)
{
    FleetMissionStartupGeographicMotion::Models result;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) result.insert(it.key(), it.value());
    return result;
}
bool witnessModelsMatch(const std::shared_ptr<FleetMissionStartupClearanceWitness>& witness,
                        const FleetMissionStartupGeographicMotion::Models& models)
{
    if (!witness || models.isEmpty()) return false;
    for (auto it = models.constBegin(); it != models.constEnd(); ++it)
        if (!it.value() || witness->originalGeographicMotion(it.key()) != it.value()) return false;
    return true;
}
bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool text(const QVariant& value, int maximum = 128)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string)
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
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}
bool integer(const QVariant& value, double minimum, double maximum, double* number)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum || std::floor(candidate) != candidate) return false;
    *number = candidate;
    return true;
}
bool finiteFloat(const QVariant& value, double* number)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || std::abs(candidate) > std::numeric_limits<float>::max()
        || static_cast<double>(static_cast<float>(candidate)) != candidate) return false;
    *number = candidate;
    return true;
}
bool decimal(const QVariant& value, quint64* number)
{
    if (!text(value, 20)) return false;
    bool valid = false;
    const quint64 candidate = value.toString().toULongLong(&valid);
    if (!valid || QString::number(candidate) != value.toString()) return false;
    *number = candidate;
    return true;
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 20 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += static_cast<qint64>(it.key().size()) * 2;
            if (*amount > MaximumBytes || !boundedTree(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!boundedTree(entry, depth + 1, nodes, amount)) return false;
        return true;
    case QVariant::StringList:
        for (const QString& entry : value.toStringList()) if (!boundedTree(entry, depth + 1, nodes, amount)) return false;
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
    int nodes = 0;
    qint64 amount = 0;
    if (map.isEmpty() || !boundedTree(map, 0, &nodes, &amount)) return false;
    *result = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !result->isEmpty() && result->size() <= MaximumBytes;
}
QString hash(const QVariantMap& value)
{
    QByteArray encoded;
    if (!encode(value, &encoded)) return {};
    return QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex());
}
QVariantMap riskFields(const QVariantMap& source, const QStringList& fields)
{
    QVariantMap result;
    for (const QString& field : fields)
        if (source.contains(field)) result.insert(field, source.value(field));
    return result;
}
// This smaller boundary is exclusively for the public risk display. Do not
// serialize the private artifact and then remove a few authentication fields.
bool riskDisplayTree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 12 || ++(*nodes) > 32768) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 256) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(it.key(), 128)) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > 2 * 1024 * 1024 || !riskDisplayTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > 4096) return false;
        for (const QVariant& entry : list) if (!riskDisplayTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    }
    case QVariant::StringList: {
        const QStringList list = value.toStringList();
        if (list.size() > 4096) return false;
        for (const QString& entry : list) if (!riskDisplayTree(entry, depth + 1, nodes, bytes)) return false;
        return true;
    }
    case QVariant::String: {
        const QString string = value.toString();
        if (string.size() > 8192 || QString::fromUtf8(string.toUtf8()) != string || string.contains(QChar(0))) return false;
        *bytes += string.toUtf8().size();
        return *bytes <= 2 * 1024 * 1024;
    }
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false; // including typed-null QObject/custom pointers
    }
}
bool riskCodes(const QVariant& value, QVariantList* result)
{
    QVariantList entries;
    if (value.type() == QVariant::List) entries = value.toList();
    else if (value.type() == QVariant::StringList)
        for (const QString& entry : value.toStringList()) entries.append(entry);
    else return false;
    if (entries.size() > 256) return false;
    for (const QVariant& entry : entries) if (!text(entry, 256)) return false;
    *result = entries;
    return true;
}
bool bindingShape(const QVariantMap& binding)
{
    const QSet<QString> strings = {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if ((binding.size() != 7 && binding.size() != 9)
        || binding.contains(QStringLiteral("reviewGeneration")) != binding.contains(QStringLiteral("inputGeneration"))) return false;
    for (auto it = binding.constBegin(); it != binding.constEnd(); ++it) {
        if (strings.contains(it.key())) { if (!text(it.value())) return false; }
        else if (it.key() == QStringLiteral("recordRevision") || it.key() == QStringLiteral("reviewGeneration")
            || it.key() == QStringLiteral("inputGeneration")) {
            double number = 0;
            if (!integer(it.value(), it.key() == QStringLiteral("recordRevision") ? 1 : 0, ExactMaximum, &number)) return false;
        } else return false;
    }
    for (const QString& key : strings) if (!binding.contains(key)) return false;
    return binding.contains(QStringLiteral("recordRevision")) && digest(binding.value(QStringLiteral("planId")))
        && digest(binding.value(QStringLiteral("contentHash"))) && digest(binding.value(QStringLiteral("reviewEvidenceHash")));
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result,
    QVariantMap* capturedRecord = nullptr)
{
    const QString task = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(task);
    const QVariantMap record = store->record(task, binding.value(QStringLiteral("recordRevision")));
    if (!isFalse(latest.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    QByteArray encodedRecord;
    if (!encode(record, &encodedRecord)) return false;
    const QVariant evidenceValue = record.value(QStringLiteral("reviewEvidence"));
    if (evidenceValue.type() != QVariant::Map || evidenceValue.isNull()) return false;
    const QVariantMap evidence = evidenceValue.toMap();
    if (hash(evidence) != binding.value(QStringLiteral("reviewEvidenceHash")).toString()) return false;
    const QVariant needsReview = evidence.value(QStringLiteral("needsReview"));
    const QVariant reviewStatus = evidence.value(QStringLiteral("reviewStatus"));
    const QVariant risk = evidence.value(QStringLiteral("riskAssessment"));
    // A fresh ground read does not erase blocking risks in its bound planning
    // record, nor may a later coordinator review silently replace that record.
    if (needsReview.type() != QVariant::Bool || needsReview.isNull() || needsReview.toBool()
        || reviewStatus.type() != QVariant::String || reviewStatus.isNull()
        || reviewStatus.toString() != QStringLiteral("reviewable")
        || risk.type() != QVariant::Map || risk.isNull()
        || !FleetMissionRiskEvaluator::hasNoBlockingRisks(risk.toMap())) return false;
    *result = encodedRecord;
    if (capturedRecord) *capturedRecord = record;
    return true;
}
bool validRole(const QVariantMap& identity, const QVariantMap& role)
{
    return role.size() == 9 && text(role.value(QStringLiteral("actorId"))) && text(role.value(QStringLiteral("role")))
        && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start.ground.reassess")
        && isTrue(role.value(QStringLiteral("allowed"))) && isTrue(identity.value(QStringLiteral("authenticated")))
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")));
}
QVariantMap stableReceipt(QVariantMap value) { value.remove(QStringLiteral("ageMs")); return value; }
QVariantMap stableContext(QVariantMap value) { value.remove(QStringLiteral("remainingLifetimeMs")); return value; }
QVariantMap rejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

// Decode the retained ORIGINAL MISSION_ITEM_INT evidence, never MissionItem
// display/cache values. The sole frame translation is AMSL GLOBAL_INT ->
// canonical AMSL GLOBAL. Current is retained but not immutable plan identity.
QVariantMap canonicalRaw(const QVariantMap& receipt, int expectedCount)
{
    if (receipt.value(QStringLiteral("rawItems")).type() != QVariant::List) return {};
    const QVariantList items = receipt.value(QStringLiteral("rawItems")).toList();
    if (items.size() != expectedCount || expectedCount < 1 || expectedCount > 2048) return {};
    QVariantList normalized;
    for (int index = 0; index < items.size(); ++index) {
        if (items.at(index).type() != QVariant::Map) return {};
        const QVariantMap raw = items.at(index).toMap();
        double seq = 0, command = 0, frame = 0, x = 0, y = 0, current = 0, automatic = 0, missionType = 0, system = 0, component = 0;
        if (raw.size() != 16 || !integer(raw.value(QStringLiteral("sequenceNumber")), index, index, &seq)
            || !integer(raw.value(QStringLiteral("command")), 0, 65535, &command)
            || !integer(raw.value(QStringLiteral("frame")), 0, 255, &frame)
            || !integer(raw.value(QStringLiteral("x")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &x)
            || !integer(raw.value(QStringLiteral("y")), std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::max(), &y)
            || !integer(raw.value(QStringLiteral("current")), 0, 1, &current)
            || !integer(raw.value(QStringLiteral("autocontinue")), 0, 1, &automatic)
            || !integer(raw.value(QStringLiteral("missionType")), MAV_MISSION_TYPE_MISSION, MAV_MISSION_TYPE_MISSION, &missionType)
            || !integer(raw.value(QStringLiteral("targetSystem")), 0, 255, &system)
            || !integer(raw.value(QStringLiteral("targetComponent")), 0, 255, &component)
            || !digest(raw.value(QStringLiteral("rawPayloadHash")))) return {};
        const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
        const bool geographic = command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_TAKEOFF
            || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
        if ((!speed && !geographic) || frame != (speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL_INT)) return {};
        double p1 = 0, p2 = 0, p3 = 0, z = 0, p4 = 0;
        if (!finiteFloat(raw.value(QStringLiteral("param1")), &p1) || !finiteFloat(raw.value(QStringLiteral("param2")), &p2)
            || !finiteFloat(raw.value(QStringLiteral("param3")), &p3) || !finiteFloat(raw.value(QStringLiteral("z")), &z)
            || !raw.contains(QStringLiteral("param4"))
            || (raw.value(QStringLiteral("param4")).isValid() && !finiteFloat(raw.value(QStringLiteral("param4")), &p4))
            || (speed && (x != 0 || y != 0))) return {};
        QVariant parameter4;
        QString yawMode, headingError;
        if (!FleetMissionHeadingEncoding::decodeParam4(static_cast<int>(command),
                raw.value(QStringLiteral("param4")).isValid() ? float(p4) : std::numeric_limits<float>::quiet_NaN(),
                parameter4, yawMode, headingError)) return {};
        normalized.append(QVariantMap{{QStringLiteral("sequenceNumber"), index}, {QStringLiteral("command"), static_cast<int>(command)},
            {QStringLiteral("frame"), static_cast<int>(speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL)},
            {QStringLiteral("param1"), p1}, {QStringLiteral("param2"), p2}, {QStringLiteral("param3"), p3},
            {QStringLiteral("param4"), parameter4}, {QStringLiteral("param5"), speed ? 0.0 : x / 1e7},
            {QStringLiteral("param6"), speed ? 0.0 : y / 1e7}, {QStringLiteral("param7"), z},
            {QStringLiteral("autoContinue"), automatic == 1}, {QStringLiteral("isCurrentItem"), current == 1},
            {QStringLiteral("yawMode"), yawMode}});
    }
    return FleetMissionPayloadBuilder::canonicalWireItems(normalized, true);
}
} // namespace

namespace {
QVariantMap stableReadiness(QVariantMap value)
{
    value.remove(QStringLiteral("startupCoordinateObservation"));
    value.remove(QStringLiteral("startupAltitudeDiagnostic"));
    value.remove(QStringLiteral("startupAltitudeReferenceSample"));
    return value;
}
bool altitudeMatches(const QVariantMap& current, const QVariantMap& original, const QVariantMap& policy)
{
    if (policy.value(QStringLiteral("schemaVersion")).toInt() != 4) return true;
    const QVariantMap reference = original.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
    return isTrue(current.value(QStringLiteral("startupAltitudeReferenceRequired"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceRequired")))
        && isTrue(current.value(QStringLiteral("startupAltitudeReferenceCaptured"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceCaptured")))
        && isTrue(current.value(QStringLiteral("startupAltitudeReferenceMatches"))) && isTrue(original.value(QStringLiteral("startupAltitudeReferenceMatches")))
        && reference.size() == 20 && reference == current.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
        && digest(original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
        && hash(reference) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
        && current.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint"))
        && FleetMissionStartupAltitude::sampleWithinBaseline(current.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
            original.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
}
QVariantMap physicalReadProjection(const QVariantMap& readiness)
{
    // A READ can reveal previously unknown CURRENT/REACHED data. Do not freeze
    // those observations before collection; the complete final risk includes
    // them. Physical channels, their ABA revisions, and configuration do not
    // acquire this exception, and are never silently rebased during the READ.
    QVariantMap flight = readiness.value(QStringLiteral("flightStateSummary")).toMap();
    QVariantMap channels = flight.value(QStringLiteral("channels")).toMap();
    channels.remove(QStringLiteral("missionCurrent"));
    channels.remove(QStringLiteral("missionReached"));
    flight.insert(QStringLiteral("channels"), channels);
    QVariantMap result{{QStringLiteral("flightStateSummary"), flight}};
    for (const QString& key : {QStringLiteral("startupConfigurationFingerprint"), QStringLiteral("inputConsistencyFingerprint"),
            QStringLiteral("payloadRequirementsFingerprint"), QStringLiteral("firmwareAdapterCatalogBinding"),
            QStringLiteral("parameterSummary"), QStringLiteral("takeoffDerivation"), QStringLiteral("startupPhase"),
            QStringLiteral("startupPhasePreconditionsMatch"), QStringLiteral("startupTransitionPolicy"),
            QStringLiteral("startupTransitionPolicyAvailable"), QStringLiteral("startupCoordinateRiskFingerprint"),
            QStringLiteral("startupOdometryReferenceObservation"), QStringLiteral("startupOdometryReferenceRiskFingerprint"),
            QStringLiteral("startupOdometryReferenceCaptured"), QStringLiteral("startupOdometryReferenceMatches"),
            QStringLiteral("startupGlobalOriginReferenceObservation"), QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"),
            QStringLiteral("startupGlobalOriginReferenceCaptured"), QStringLiteral("startupGlobalOriginReferenceMatches"),
            QStringLiteral("startupMotionReferenceRequired"), QStringLiteral("startupMotionReferenceMatches"),
            QStringLiteral("startupEstimatorObservation"), QStringLiteral("startupEstimatorEvidenceCaptured"),
            QStringLiteral("startupEstimatorEvidenceMatches"), QStringLiteral("startupEstimatorRiskFingerprint")})
        result.insert(key, readiness.value(key));
    for (const QString& key : {QStringLiteral("startupAltitudeReferenceObservation"), QStringLiteral("startupAltitudeReferenceSample"),
            QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("startupAltitudeReferenceRequired"),
            QStringLiteral("startupAltitudeReferenceCaptured"), QStringLiteral("startupAltitudeReferenceMatches")})
        if (readiness.contains(key)) result.insert(key, readiness.value(key));
    return result;
}
bool physicalReadMatches(const QVariantMap& current, const QVariantMap& original)
{
    return stableReadiness(current) == stableReadiness(original)
        && altitudeMatches(current, original, original.value(QStringLiteral("startupTransitionPolicy")).toMap());
}
}

FleetMissionArmedGroundReview::FleetMissionArmedGroundReview(QObject* parent)
    : QObject(parent), _reviewEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(PollMs);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionArmedGroundReview::_tick);
}
FleetMissionArmedGroundReview::~FleetMissionArmedGroundReview()
{
    _retireClearanceCalculation();
    if (_clearanceWorker) disconnect(_clearanceWorker.data(), nullptr, this, nullptr);
    _active = false; _disconnectSources(); _detachDrain();
}
bool FleetMissionArmedGroundReview::_live(quint64 generation) const
{
    return generation != 0 && generation == _generation && !_clearanceSourceInvalidated
        && (!_clearanceSourceProvided || (_clearanceDeadlineMs >= 0 && _clock.elapsed() < _clearanceDeadlineMs))
        && (_active ? _expiresAtMs >= 0 && _clock.elapsed() < _expiresAtMs
                && (_reviewExpiresAtMs < 0 || _clock.elapsed() < _reviewExpiresAtMs)
            : (_state == QStringLiteral("content-ready") || _state == QStringLiteral("acknowledged"))
                && _reviewExpiresAtMs >= 0 && _clock.elapsed() < _reviewExpiresAtMs);
}
bool FleetMissionArmedGroundReview::_same(quint64 generation) const
{
    return _live(generation) && _session && _roles && _store && _previous && _deployment && _swarm
        && _health && _authorization && _catalog && _coordinator && _registry && _faults;
}
QVariantMap FleetMissionArmedGroundReview::_bundle() const
{
    const auto object = [](QObject* value) { return QVariant::fromValue(value); };
    return {{QStringLiteral("session"), object(_session.data())}, {QStringLiteral("roles"), object(_roles.data())},
        {QStringLiteral("store"), object(_store.data())}, {QStringLiteral("previousStartup"), object(_previous.data())},
        {QStringLiteral("deployment"), object(_deployment.data())}, {QStringLiteral("swarm"), object(_swarm.data())},
        {QStringLiteral("health"), object(_health.data())}, {QStringLiteral("authorizationOwner"), object(_authorization.data())}};
}
QVariantMap FleetMissionArmedGroundReview::current() const
{
    const qint64 deadline = _active
        ? (_reviewExpiresAtMs >= 0 ? qMin(_expiresAtMs, _reviewExpiresAtMs) : _expiresAtMs)
        : _reviewExpiresAtMs;
    QVariantList rows;
    for (const Target& target : _targets) rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("componentId"), target.componentId}, {QStringLiteral("frozenArmed"), target.armed},
        {QStringLiteral("state"), target.state}, {QStringLiteral("reason"), target.reason},
        {QStringLiteral("itemCount"), target.payload.value(QStringLiteral("itemCount"))},
        {QStringLiteral("contentHash"), target.canonical.value(QStringLiteral("contentHash"))}});
    // No full identity, old authorization, raw aircraft evidence, or private
    // risk artifact is exposed to QML/logging through the public snapshot.
    return {{QStringLiteral("ok"), _live(_generation) && !_active && !_reservationHeld && !_transportDraining},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("reviewEpoch"), _reviewEpoch},
        {QStringLiteral("previousStartupId"), _previousStartupId}, {QStringLiteral("state"), _state},
        {QStringLiteral("reason"), _reason}, {QStringLiteral("active"), _active},
        {QStringLiteral("reservationHeld"), _reservationHeld}, {QStringLiteral("transportDraining"), _transportDraining},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), rows},
        {QStringLiteral("sourceFingerprint"), _sourceFingerprint}, {QStringLiteral("reviewAcknowledged"), _state == QStringLiteral("acknowledged")},
        {QStringLiteral("remainingMs"), static_cast<double>(qMax<qint64>(0, deadline - _clock.elapsed()))},
        {QStringLiteral("readOnlyRequestsMayRemainInflight"), _issuedAnyRequest}, {QStringLiteral("previousAuthorizationRestored"), false},
        {QStringLiteral("remoteAtomicSnapshotVerified"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap FleetMissionArmedGroundReview::riskSummary() const
{
    const bool ownerThread = thread() == QThread::currentThread();
    const bool expired = ownerThread && _reviewExpiresAtMs >= 0 && _clock.elapsed() >= _reviewExpiresAtMs;
    QVariantMap unknown{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("reviewId"), ownerThread ? _reviewId : QStringLiteral("")},
        {QStringLiteral("reviewEpoch"), ownerThread ? _reviewEpoch : QStringLiteral("")},
        {QStringLiteral("previousStartupId"), ownerThread ? _previousStartupId : QStringLiteral("")},
        {QStringLiteral("sourceFingerprint"), ownerThread ? _sourceFingerprint : QStringLiteral("")},
        {QStringLiteral("state"), ownerThread ? _state : QStringLiteral("unknown")},
        {QStringLiteral("expired"), expired}, {QStringLiteral("available"), false},
        {QStringLiteral("reason"), QStringLiteral("complete-current-risk-artifact-unavailable")},
        {QStringLiteral("targetVehicleIds"), QVariantList()}, {QStringLiteral("constraints"), QVariantMap()},
        {QStringLiteral("healthGate"), QVariantMap()}, {QStringLiteral("blockedReasons"), QVariantList()},
        {QStringLiteral("unverifiedRequirements"), QVariantList()}, {QStringLiteral("reasonsTruncated"), false},
        {QStringLiteral("vehicles"), QVariantList()},
        {QStringLiteral("limitations"), QStringList{
            QStringLiteral("frozen-risk-display-not-live-telemetry-or-authorization"),
            QStringLiteral("conditional-acknowledgement-does-not-prove-final-start-permit"),
            QStringLiteral("constraint-and-readiness-whitelists-not-original-plan-or-wire-payload")}},
        {QStringLiteral("allNewRawReadsMatched"), false}, {QStringLiteral("readonly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    const auto unavailable = [&](const QString& reason) {
        QVariantMap result = unknown;
        result.insert(QStringLiteral("reason"), reason);
        return result;
    };
    if (!ownerThread) return unavailable(QStringLiteral("risk-display-owner-thread-required"));
    if (expired) return unavailable(QStringLiteral("risk-display-original-read-deadline-expired"));
    // No _gate(), external getters, WTS/role authentication, READ, or signals.
    // The retained artifact is the only data source; no recapture can silently
    // renew the reviewed deadline or substitute a new vehicle/physical basis.
    const bool blockedDiagnostic = _state == QStringLiteral("clearance-blocked");
    if ((!blockedDiagnostic && !_same(_generation)) || _active || _reservationHeld || _transportDraining
        || (_state != QStringLiteral("content-ready") && _state != QStringLiteral("acknowledged") && !blockedDiagnostic)
        || _artifact.isEmpty()) return unknown;
    if (_artifact.value(QStringLiteral("reviewId")).toString() != _reviewId
        || _artifact.value(QStringLiteral("reviewEpoch")).toString() != _reviewEpoch
        || _artifact.value(QStringLiteral("previousStartupId")).toString() != _previousStartupId
        || _artifact.value(QStringLiteral("generation")).toString() != QString::number(_generation)
        || !digest(_sourceFingerprint) || !isTrue(_artifact.value(QStringLiteral("allNewRawReadsMatched"))))
        return unavailable(QStringLiteral("risk-display-artifact-identity-invalid"));
    const QVariantMap plan = _artifact.value(QStringLiteral("previousReference")).toMap()
        .value(QStringLiteral("deploymentContext")).toMap().value(QStringLiteral("plan")).toMap();
    const QVariantMap health = _artifact.value(QStringLiteral("currentHealthGate")).toMap();
    const QVariantList ids = _artifact.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList physical = _artifact.value(QStringLiteral("physicalTargets")).toList();
    if (plan.value(QStringLiteral("constraints")).type() != QVariant::Map || health.isEmpty()
        || ids.isEmpty() || ids.size() > 32 || physical.size() != ids.size() || ids != _vehicleIds)
        return unavailable(QStringLiteral("risk-display-whole-group-missing"));
    QVariantList blocked, unverified, rows, healthReasons, altitudeDiagnostics;
    if (blockedDiagnostic) {
        const QVariantMap result = _artifact.value(QStringLiteral("startupClearanceReview")).toMap();
        const QVariantMap clearance = result.value(QStringLiteral("clearanceReview")).toMap();
        const QVariantMap report = clearance.value(QStringLiteral("report")).toMap();
        const QVariantMap domain = result.value(QStringLiteral("startupDomainReview")).toMap();
        const QVariantMap domainReport = domain.value(QStringLiteral("report")).toMap();
        const QVariantMap waypoint = result.value(QStringLiteral("waypointSwitchReview")).toMap();
        const QVariantMap waypointReport = waypoint.value(QStringLiteral("report")).toMap();
        const QVariantMap interVehicle = result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap();
        const QVariantMap interVehicleReport = interVehicle.value(QStringLiteral("report")).toMap();
        QVariantList reasons, domainReasons, waypointReasons, interVehicleReasons;
        if (!isTrue(result.value(QStringLiteral("ok"))) || !isTrue(clearance.value(QStringLiteral("ok")))
            || !isTrue(domain.value(QStringLiteral("ok"))) || domainReport.isEmpty()
            || !isTrue(waypoint.value(QStringLiteral("ok"))) || waypointReport.isEmpty()
            || !isTrue(interVehicle.value(QStringLiteral("ok"))) || interVehicleReport.isEmpty()
            || report.isEmpty() || !riskCodes(report.value(QStringLiteral("reasonCodes")), &reasons)
            || !riskCodes(domainReport.value(QStringLiteral("reasonCodes")), &domainReasons)
            || !riskCodes(waypointReport.value(QStringLiteral("reasonCodes")), &waypointReasons)
            || !riskCodes(interVehicleReport.value(QStringLiteral("reasonCodes")), &interVehicleReasons))
            return unavailable(QStringLiteral("risk-display-clearance-blocked-report-unavailable"));
        blocked.append(QVariantMap{{QStringLiteral("vehicleId"), 0},
            {QStringLiteral("code"), QStringLiteral("startup-clearance-declared-model-not-satisfied")}});
        for (const QVariant& reason : reasons)
            blocked.append(QVariantMap{{QStringLiteral("vehicleId"), 0}, {QStringLiteral("code"), reason}});
        for (const QVariant& reason : domainReasons)
            blocked.append(QVariantMap{{QStringLiteral("vehicleId"), 0}, {QStringLiteral("code"), reason}});
        for (const QVariant& reason : waypointReasons)
            blocked.append(QVariantMap{{QStringLiteral("vehicleId"), 0}, {QStringLiteral("code"), reason}});
        for (const QVariant& reason : interVehicleReasons)
            blocked.append(QVariantMap{{QStringLiteral("vehicleId"), 0}, {QStringLiteral("code"), reason}});
    }
    const QStringList unprovenFields{QStringLiteral("previousAuthorizationRestored"),
        QStringLiteral("homeReferenceBranchValidityProven"), QStringLiteral("localReferenceResetCertified"),
        QStringLiteral("pilotInputsNeutralProven"), QStringLiteral("groundAndObstacleClearanceCertified"),
        QStringLiteral("hardwareIdentityAuthenticated"), QStringLiteral("remoteAtomicSnapshotVerified"),
        QStringLiteral("requestAttributionVerified"), QStringLiteral("startupPermitGranted"),
        QStringLiteral("executable"), QStringLiteral("flightCommandReleased")};
    for (const QString& field : unprovenFields) {
        if (!isFalse(_artifact.value(field))) return unavailable(QStringLiteral("risk-display-boundary-invalid"));
        unverified.append(QVariantMap{{QStringLiteral("vehicleId"), 0}, {QStringLiteral("code"), field + QStringLiteral("-not-proven-or-granted")}});
    }
    if (health.value(QStringLiteral("reasons")).type() != QVariant::List
        || health.value(QStringLiteral("reasons")).toList().size() > 128)
        return unavailable(QStringLiteral("risk-display-health-reasons-invalid"));
    for (const QVariant& value : health.value(QStringLiteral("reasons")).toList()) {
        const QVariantMap entry = value.toMap();
        double id = 0;
        if (value.type() != QVariant::Map || !integer(entry.value(QStringLiteral("vehicleId")), -1, 255, &id)
            || !text(entry.value(QStringLiteral("code")), 256) || !text(entry.value(QStringLiteral("severity")), 32)
            || !text(entry.value(QStringLiteral("message")), 8192))
            return unavailable(QStringLiteral("risk-display-health-reasons-invalid"));
        healthReasons.append(riskFields(entry, {QStringLiteral("vehicleId"), QStringLiteral("code"), QStringLiteral("severity"), QStringLiteral("message")}));
        if (entry.value(QStringLiteral("severity")).toString() == QStringLiteral("block"))
            blocked.append(QVariantMap{{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("code"), entry.value(QStringLiteral("code"))}});
    }
    QSet<int> seen;
    for (int index = 0; index < physical.size(); ++index) {
        const QVariantMap target = physical.at(index).toMap();
        const QVariantMap risk = target.value(QStringLiteral("currentRisk")).toMap();
        double id = 0, component = 0;
        QVariantList rowBlocked, rowUnverified, unencoded;
        if (!integer(target.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !integer(target.value(QStringLiteral("componentId")), 1, 255, &component)
            || target.value(QStringLiteral("frozenArmed")).type() != QVariant::Bool
            || target.value(QStringLiteral("vehicleId")) != ids.at(index) || seen.contains(int(id))
            || risk.value(QStringLiteral("vehicleId")) != target.value(QStringLiteral("vehicleId"))
            || risk.value(QStringLiteral("componentId")) != target.value(QStringLiteral("componentId"))
            || !isFalse(risk.value(QStringLiteral("reasonsTruncated")))
            || !riskCodes(risk.value(QStringLiteral("blockedReasons")), &rowBlocked)
            || !riskCodes(risk.value(QStringLiteral("unverifiedRequirements")), &rowUnverified)
            || !riskCodes(risk.value(QStringLiteral("sourceUnencodedRequirements")), &unencoded)
            || !isTrue(risk.value(QStringLiteral("startupTransitionPolicyAvailable")))
            || risk.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map)
            return unavailable(QStringLiteral("risk-display-complete-vehicle-risk-unavailable"));
        seen.insert(int(id));
        for (const QVariant& code : rowBlocked) blocked.append(QVariantMap{{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("code"), code}});
        for (const QVariant& code : rowUnverified) unverified.append(QVariantMap{{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("code"), code}});
        QVariantMap summary = riskFields(risk, {
            QStringLiteral("sourceEvidenceComplete"), QStringLiteral("sourcesStillCurrent"), QStringLiteral("preconditionsMatch"),
            QStringLiteral("controlledArmPreconditionsMatch"), QStringLiteral("positionModeGroundAdmissionMatches"),
            QStringLiteral("needSeparateManualArming"), QStringLiteral("launchType"), QStringLiteral("endType"),
            QStringLiteral("startupPhase"), QStringLiteral("startupPhasePreconditionsMatch"), QStringLiteral("startupPhaseEvidenceScope"),
            QStringLiteral("deferredStartupAction"), QStringLiteral("phaseDoesNotProveNeutralPilotInputs"),
            QStringLiteral("phaseDoesNotCertifyMotorOrGroundClearance"), QStringLiteral("phaseDoesNotCertifyLocalReferenceValidity"),
            QStringLiteral("phaseDoesNotProveCrossMessageSameSample"), QStringLiteral("takeoffDerivation"),
            QStringLiteral("currentPositionAltitudeSummary"), QStringLiteral("currentPositionToFirstWaypoint"),
            QStringLiteral("parameterSummary"), QStringLiteral("flightStateSummary"), QStringLiteral("startupEstimatorObservation"),
            QStringLiteral("startupEstimatorEvidenceCaptured"), QStringLiteral("startupEstimatorEvidenceMatches"),
            QStringLiteral("startupCoordinateObservation"), QStringLiteral("startupOdometryReferenceObservation"),
            QStringLiteral("startupOdometryReferenceCaptured"), QStringLiteral("startupOdometryReferenceMatches"),
            QStringLiteral("startupGlobalOriginReferenceObservation"), QStringLiteral("startupGlobalOriginReferenceCaptured"),
            QStringLiteral("startupGlobalOriginReferenceMatches"),
            QStringLiteral("startupMotionReferenceRequired"), QStringLiteral("startupMotionReferenceMatches"),
            QStringLiteral("startupPermitGranted"), QStringLiteral("executable"), QStringLiteral("flightCommandReleased"),
            QStringLiteral("executionVerified"), QStringLiteral("executionPermitted"), QStringLiteral("signatureVerified"),
            QStringLiteral("authenticated"), QStringLiteral("firmwareAdapterCertified"), QStringLiteral("actualTakeoffPathCertified"),
            QStringLiteral("landingClearanceCertified"), QStringLiteral("missionCompleteVerified")});
        summary.insert(QStringLiteral("sourceUnencodedRequirements"), unencoded);
        for (const QString& key : {QStringLiteral("startupAltitudeReferenceObservation"), QStringLiteral("startupAltitudeReferenceSample"),
                QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("startupAltitudeReferenceRequired"),
                QStringLiteral("startupAltitudeReferenceCaptured"), QStringLiteral("startupAltitudeReferenceMatches")})
            if (risk.contains(key)) summary.insert(key, risk.value(key));
        altitudeDiagnostics.append(risk.value(QStringLiteral("startupAltitudeDiagnostic")));
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("componentId"), int(component)},
            {QStringLiteral("frozenArmed"), target.value(QStringLiteral("frozenArmed"))},
            {QStringLiteral("blockedReasons"), rowBlocked}, {QStringLiteral("unverifiedRequirements"), rowUnverified},
            {QStringLiteral("reasonsTruncated"), false}, {QStringLiteral("readinessSummary"), summary},
            {QStringLiteral("startupTransitionPolicy"), risk.value(QStringLiteral("startupTransitionPolicy"))}});
    }
    QVariantMap result = unknown;
    result.insert(QStringLiteral("targetVehicleIds"), ids);
    result.insert(QStringLiteral("constraints"), riskFields(plan.value(QStringLiteral("constraints")).toMap(), {
        QStringLiteral("timeLimitSeconds"), QStringLiteral("deadline"), QStringLiteral("startAt"),
        QStringLiteral("minimumSeparationMeters"), QStringLiteral("maxAltitudeMeters"), QStringLiteral("maxAltitudeReference"),
        QStringLiteral("geofenceRequired"), QStringLiteral("returnReserveSeconds"), QStringLiteral("requireLeader"),
        QStringLiteral("synchronizedStart"), QStringLiteral("synchronizationRequired")}));
    QVariantMap publicHealth = riskFields(health, {QStringLiteral("allowed"), QStringLiteral("state"),
        QStringLiteral("selectedVehicleIds"), QStringLiteral("blockingVehicleIds"), QStringLiteral("warningVehicleIds"),
        QStringLiteral("invalidSelection"), QStringLiteral("globalBlocking"), QStringLiteral("requireLeader"), QStringLiteral("source")});
    publicHealth.insert(QStringLiteral("reasons"), healthReasons);
    result.insert(QStringLiteral("healthGate"), publicHealth);
    result.insert(QStringLiteral("blockedReasons"), blocked);
    result.insert(QStringLiteral("unverifiedRequirements"), unverified);
    result.insert(QStringLiteral("vehicles"), rows);
    result.insert(QStringLiteral("allNewRawReadsMatched"), true);
    result.insert(QStringLiteral("available"), true);
    result.insert(QStringLiteral("reason"), QStringLiteral(""));
    int nodes = 0; qint64 bytes = 0;
    if (!riskDisplayTree(result, 0, &nodes, &bytes)) return unavailable(QStringLiteral("risk-display-resource-or-type-limit"));
    const QVariantMap originalRiskDisplay = result;
    qint64 encodedBytes = QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact).size();
    if (encodedBytes > 2 * 1024 * 1024)
        return unavailable(QStringLiteral("risk-display-byte-limit"));
    // The old complete risk display owns the budget first. A supplementary
    // altitude sample is optional, all-or-nothing, and cannot hide a target or
    // turn a previously displayable risk into an unavailable whole report.
    const QString diagnosticKey = QStringLiteral("startupAltitudeDiagnostic");
    for (int index = 0; index < rows.size(); ++index) {
        const QVariant value = altitudeDiagnostics.at(index);
        if (value.type() != QVariant::Map || value.toMap().isEmpty()) continue;
        const QVariantMap diagnostic = value.toMap();
        QVariantMap row = rows.at(index).toMap(), summary = row.value(QStringLiteral("readinessSummary")).toMap();
        const qint64 extraBytes = QJsonDocument::fromVariant(diagnostic).toJson(QJsonDocument::Compact).size()
            + diagnosticKey.toUtf8().size() + (summary.isEmpty() ? 3 : 4);
        int candidateNodes = nodes; qint64 candidateBytes = bytes + diagnosticKey.toUtf8().size();
        if (encodedBytes + extraBytes > 2 * 1024 * 1024
            || !riskDisplayTree(diagnostic, 4, &candidateNodes, &candidateBytes)) continue;
        summary.insert(diagnosticKey, diagnostic); row.insert(QStringLiteral("readinessSummary"), summary); rows[index] = row;
        nodes = candidateNodes; bytes = candidateBytes; encodedBytes += extraBytes;
    }
    result.insert(QStringLiteral("vehicles"), rows);
    nodes = 0; bytes = 0;
    if (!riskDisplayTree(result, 0, &nodes, &bytes)
        || QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact).size() > 2 * 1024 * 1024)
        result = originalRiskDisplay;
    const QJsonDocument document = QJsonDocument::fromVariant(result);
    // Known-missing physical scalars stay explicit JSON null in QML; no
    // undefined key loss and no conversion to zero, false, or a passing flag.
    return document.toVariant().toMap();
}
bool FleetMissionArmedGroundReview::retainedPreviousSourceContextMatches(QObject* previousObject,
    const QString& previousStartupId, const QVariantMap& exactContext) const
{
    return thread() == QThread::currentThread() && _same(_generation) && _previous.data() == previousObject
        && previousStartupId == _previousStartupId && exactContext == _previousContext;
}
QString FleetMissionArmedGroundReview::readScopeForTarget(Vehicle* vehicle) const
{
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        || !_same(_generation)) return {};
    for (const Target& target : _targets) if (target.vehicle.data() == vehicle) return target.readScopeId;
    return {};
}
bool FleetMissionArmedGroundReview::nativeReadAdmissionCurrent(QObject* vehicleObject, QObject* managerObject,
    QObject* primaryObject, const QString& scope)
{
    // PURE retained ownership fence: no Session/WTS call, full gate, previous
    // transaction lookup or PlanManager::ownsFleetMissionRead recursion here.
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        || !_same(_generation) || !_active || !_reservationHeld
        || _state != QStringLiteral("reading-content") || _targetIndex < 0 || _targetIndex >= _targets.size()) return false;
    const Target target = _targets.at(_targetIndex);
    return target.vehicle && target.mission && target.primary && target.links && target.parameters
        && target.vehicle.data() == vehicleObject && target.mission.data() == managerObject && target.primary.data() == primaryObject
        && scope == target.readScopeId && uuid(scope) && target.state == QStringLiteral("reading-content")
        && target.vehicle->armed() == target.armed && !target.vehicle->flying()
        && target.vehicle->id() == target.vehicleId && target.vehicle->defaultComponentId() == target.componentId
        && target.vehicle->missionManager() == target.mission.data() && target.vehicle->parameterManager() == target.parameters.data()
        && target.vehicle->vehicleLinkManager() == target.links.data() && !target.links->communicationLost()
        && target.links->primaryLink().lock().get() == target.primary.data()
        && _clock.elapsed() < _targetDeadlineMs && _swarm->missionDeploymentOwns(_reviewId, _vehicleIds);
}
bool FleetMissionArmedGroundReview::_targetCurrent(const Target& supplied, bool allowOwnedRead, bool captureRisk) const
{
    const Target target = supplied;
    const QPointer<FleetMissionArmedGroundReview> self(const_cast<FleetMissionArmedGroundReview*>(this));
    const quint64 generation = _generation;
    const auto alive = [&]() { return self && outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        && _same(generation) && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
    if (!alive() || target.vehicle->thread() != thread() || target.mission->thread() != thread() || target.parameters->thread() != thread()
        || target.links->thread() != thread()
        || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
        || target.vehicle->missionManager() != target.mission.data() || target.vehicle->parameterManager() != target.parameters.data()
        || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->vehicleLinkManager() != target.links.data()
        || target.links->communicationLost() || target.links->primaryLink().lock().get() != target.primary.data()
        || target.vehicle->isOfflineEditingVehicle() || !target.vehicle->isInitialConnectComplete()
        || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || target.vehicle->armed() != target.armed || target.vehicle->flying()) return false;
    if (!target.primary->isConnected() || !alive() || target.primary->isMockLink() || !alive()
        || target.primary->isLogReplay() || !alive()) return false;
    const bool pending = target.parameters->pendingWrites();
    if (!alive() || pending || !target.parameters->parametersReady() || target.parameters->missingParameters()) return false;
    const QVariantMap version = target.vehicle->autopilotVersionObservation(30000);
    if (!alive() || version.value(QStringLiteral("sourceEpoch")) != target.versionEpoch) return false;
    const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MIS_TAKEOFF_ALT"), 30000);
    if (!alive() || parameter.value(QStringLiteral("managerEpoch")) != target.parameterEpoch) return false;
    if (target.mission->inProgress() || target.mission->transportCallInProgress()) {
        const quint64 operation = target.operationId ? target.operationId : target.mission->activeOperationId();
        const bool own = allowOwnedRead && !target.readScopeId.isEmpty()
            && (target.mission->ownsFleetMissionRead(target.readScopeId, target.primary.data(), operation)
                || (target.operationId == 0 && target.mission->ownsFleetMissionRead(target.readScopeId, target.primary.data(), 0)));
        if (!alive() || !own) return false;
    }
    if (captureRisk) {
        const bool phaseSource = target.vehicle->fleetArmedGroundReviewSourceMatches(target.primary.data());
        if (!alive() || !phaseSource) return false;
        const bool coordinateSource = target.vehicle->startupCoordinateObservationSourceMatches(target.primary.data());
        if (!alive() || !coordinateSource) return false;
        const QVariantMap actual = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, _constraints, target.payload,
            target.armed ? FleetMissionStartReadiness::Phase::ArmedGroundReady : FleetMissionStartReadiness::Phase::BeforeArm, _catalog.data());
        if (!alive() || !isTrue(actual.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !isTrue(actual.value(QStringLiteral("startupTransitionPolicyAvailable")))
            || !physicalReadMatches(physicalReadProjection(actual), target.physicalBaseline)) return false;
        if (!target.reviewReadiness.isEmpty() && (stableReadiness(actual) != stableReadiness(target.reviewReadiness)
            || !altitudeMatches(actual, target.reviewReadiness, target.reviewReadiness.value(QStringLiteral("startupTransitionPolicy")).toMap()))) return false;
    }
    return alive() && target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
        target.primaryOwner, target.parameterSnapshot, ReviewLifetimeMs);
}

bool FleetMissionArmedGroundReview::_receiptCurrent(const Target& supplied, quint64 generation, QVariantMap* output) const
{
    const Target target = supplied;
    const QPointer<FleetMissionArmedGroundReview> self(const_cast<FleetMissionArmedGroundReview*>(this));
    const auto alive = [&]() { return self && outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        && _live(generation) && target.mission && target.primary && target.vehicle; };
    if (!alive() || !target.operationId || target.readScopeId.isEmpty() || !_targetCurrent(target, false, false)) return false;
    const bool before = target.mission->fleetMissionArmedGroundReadReceiptSourceMatches(target.primary.data(), QString::number(target.operationId), target.readScopeId, self.data());
    if (!alive()) return false;
    const QVariantMap receipt = target.mission->fleetMissionArmedGroundReadReceipt(_active ? BatchLifetimeMs : ReviewLifetimeMs, self.data());
    if (!alive()) return false;
    const bool after = target.mission->fleetMissionArmedGroundReadReceiptSourceMatches(target.primary.data(), QString::number(target.operationId), target.readScopeId, self.data());
    if (!alive() || !before || !after) return false;
    double schema = 0, age = 0, vehicle = 0, component = 0, count = 0, started = 0, countAt = 0, firstAt = 0, lastAt = 0, complete = 0, duration = 0;
    quint64 sequence = 0, baseline = 0;
    const int expectedCount = target.payload.value(QStringLiteral("items")).toList().size();
    if (receipt.size() != 34 || !integer(receipt.value(QStringLiteral("schemaVersion")), 1, 1, &schema) || !isTrue(receipt.value(QStringLiteral("observed")))
        || !isTrue(receipt.value(QStringLiteral("available"))) || !isTrue(receipt.value(QStringLiteral("fresh")))
        || !isTrue(receipt.value(QStringLiteral("requestIssued"))) || !isTrue(receipt.value(QStringLiteral("countObserved")))
        || !isTrue(receipt.value(QStringLiteral("allItemsInt"))) || !isTrue(receipt.value(QStringLiteral("readEndAckQueued")))
        || !integer(receipt.value(QStringLiteral("ageMs")), 0, _active ? BatchLifetimeMs : ReviewLifetimeMs, &age)
        || !integer(receipt.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId, &vehicle)
        || !integer(receipt.value(QStringLiteral("componentId")), target.componentId, target.componentId, &component)
        || !integer(receipt.value(QStringLiteral("itemCount")), expectedCount, expectedCount, &count)
        || receipt.value(QStringLiteral("operationId")) != QString::number(target.operationId)
        || receipt.value(QStringLiteral("readScopeId")) != target.readScopeId
        || receipt.value(QStringLiteral("sourceEpoch")) != target.managerEpoch
        || receipt.value(QStringLiteral("vehicleSourceEpoch")) != target.versionEpoch
        || !decimal(receipt.value(QStringLiteral("receiptSequence")), &sequence)
        || !decimal(target.baselineSequence, &baseline) || sequence <= baseline
        || !digest(receipt.value(QStringLiteral("cacheFingerprint"))) || !digest(receipt.value(QStringLiteral("countPayloadHash")))
        || !isFalse(receipt.value(QStringLiteral("requestCausalityVerified"))) || !isFalse(receipt.value(QStringLiteral("authenticated")))
        || !isFalse(receipt.value(QStringLiteral("atomicSnapshotVerified"))) || !isFalse(receipt.value(QStringLiteral("currentOnboardContentsProven")))
        || !isFalse(receipt.value(QStringLiteral("signatureVerified"))) || !isFalse(receipt.value(QStringLiteral("startupPermitGranted")))
        || !isFalse(receipt.value(QStringLiteral("executionVerified"))) || !isFalse(receipt.value(QStringLiteral("flightCommandReleased")))
        || !integer(receipt.value(QStringLiteral("readStartedAtMonotonicMs")), 0, ExactMaximum, &started)
        || !integer(receipt.value(QStringLiteral("countReceivedAtMonotonicMs")), started, ExactMaximum, &countAt)
        || !integer(receipt.value(QStringLiteral("firstItemReceivedAtMonotonicMs")), countAt, ExactMaximum, &firstAt)
        || !integer(receipt.value(QStringLiteral("lastItemReceivedAtMonotonicMs")), firstAt, ExactMaximum, &lastAt)
        || !integer(receipt.value(QStringLiteral("completedAtMonotonicMs")), lastAt, ExactMaximum, &complete)
        || !integer(receipt.value(QStringLiteral("durationMs")), 0, BatchLifetimeMs, &duration) || duration != complete - started) return false;
    const QVariantMap canonical = canonicalRaw(receipt, expectedCount);
    if (!isTrue(canonical.value(QStringLiteral("ok"))) || canonical.value(QStringLiteral("wireSchema")) != target.payload.value(QStringLiteral("wireSchema"))
        || canonical.value(QStringLiteral("contentHash")) != target.payload.value(QStringLiteral("contentHash"))
        || canonical.value(QStringLiteral("items")) != target.payload.value(QStringLiteral("canonicalWireItems"))) return false;
    if (!target.receipt.isEmpty() && stableReceipt(receipt) != stableReceipt(target.receipt)) return false;
    *output = receipt;
    return alive();
}

bool FleetMissionArmedGroundReview::_captureClearanceSource(quint64 generation)
{
    if (thread() != QThread::currentThread() || _clearanceSourceChecking || !_same(generation)) return false;
    _clearanceSourceChecking = true;
    FlagGuard checking{QPointer<QObject>(this), &_clearanceSourceChecking};
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QVariantMap plan = _clearanceSourcePlan;
    const QList<Target> targets = _targets;
    const auto current = [&]() {
        if (!self || !_same(generation) || _reentered || !registry || !health || _registry != registry || _health != health
            || !catalog || !coordinator || _catalog != catalog || _coordinator != coordinator
            || coordinator->firmwareAdapterCatalogBackend() != catalog.data()
            || health->fleetRegistry() != registry.data() || _clearanceSourcePlan != plan
            || !outboundSources(self.data(), _session.data(), _previous.data(), _deployment.data())
            || targets.size() != _targets.size()) return false;
        for (int index = 0; index < targets.size(); ++index) {
            const Target& target = targets.at(index);
            const Target& actual = _targets.at(index);
            if (!target.vehicle || !target.mission || !target.parameters || !target.links || !target.primary
                || target.vehicle != actual.vehicle || target.mission != actual.mission || target.parameters != actual.parameters
                || target.links != actual.links || target.primary != actual.primary || target.vehicleId != actual.vehicleId
                || target.vehicle->id() != target.vehicleId || target.vehicle->missionManager() != target.mission.data()
                || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
                || target.vehicle->vehicleLinkManager() != target.links.data()) return false;
        }
        return true;
    };
    // Later gates recheck the small first evidence against actual native
    // sources; they do not repeatedly normalize/hash the complete scene.
    if (_clearanceSourceFrozen && !_clearanceSourceProvided) return current();
    QMap<int, Vehicle*> vehicles;
    for (const Target& target : targets) vehicles.insert(target.vehicleId, target.vehicle.data());
    const qint64 before = _clock.elapsed();
    const QVariantMap captured = _clearanceSourceFrozen
        ? FleetMissionClearanceSource::recapture(_clearanceSourceEvidence, registry.data(), vehicles, current)
        : FleetMissionClearanceSource::capture(plan, registry.data(), vehicles, current);
    const auto reject = [&]() {
        if (self && _generation == generation) _clearanceSourceInvalidated = true;
        return false;
    };
    if (!current() || captured.size() != 7 || captured.value(QStringLiteral("provided")).type() != QVariant::Bool
        || !isTrue(captured.value(QStringLiteral("inputValid")))
        || captured.value(QStringLiteral("configurationAndTimeCurrent")).type() != QVariant::Bool
        || captured.value(QStringLiteral("reason")).type() != QVariant::String || captured.value(QStringLiteral("reason")).isNull()
        || captured.value(QStringLiteral("evidence")).type() != QVariant::Map || captured.value(QStringLiteral("evidence")).isNull()) return reject();
    const bool provided = isTrue(captured.value(QStringLiteral("provided")));
    const QVariantMap evidence = captured.value(QStringLiteral("evidence")).toMap();
    double remaining = -1, utc = -1;
    if (!integer(captured.value(QStringLiteral("remainingValidityMs")), provided ? 1 : -1, provided ? 9007199254740991.0 : -1, &remaining)
        || !integer(captured.value(QStringLiteral("observedAtUtcMs")), provided ? 1 : -1, provided ? 9007199254740991.0 : -1, &utc)
        || (provided ? !isTrue(captured.value(QStringLiteral("configurationAndTimeCurrent"))) || evidence.isEmpty() || !digest(hash(evidence))
            : !isFalse(captured.value(QStringLiteral("configurationAndTimeCurrent"))) || !evidence.isEmpty())
        || (_clearanceSourceFrozen && (_clearanceSourceProvided != provided || _clearanceSourceEvidence != evidence))
        || (provided && (qint64(remaining) > Q_INT64_C(9007199254740991) - qint64(utc)
            || (_clearanceLastUtcMs >= 0 && qint64(utc) < _clearanceLastUtcMs)
            || (_clearanceSourceFrozen && _clearanceValidUntilUtcMs != qint64(utc) + qint64(remaining))))) return reject();
    // Freeze the declared datum correspondence BEFORE this new batch's first
    // READ. Subsequent gates must recapture the original evidence unchanged.
    const qint64 datumUtcFloor = provided ? qMax(qint64(utc), _clearanceLastUtcMs) : -1;
    qint64 datumUtc = -1;
    const QVariantMap datum = _clearanceSourceFrozen
        ? FleetMissionClearanceDatum::recapture(_clearanceDatumEvidence, evidence, registry.data(), catalog.data(), vehicles, current, datumUtcFloor, &datumUtc)
        : FleetMissionClearanceDatum::capture(evidence, registry.data(), catalog.data(), vehicles, current, datumUtcFloor, &datumUtc);
    if (!current() || datum.size() != 5 || datum.value(QStringLiteral("provided")).type() != QVariant::Bool
        || isTrue(datum.value(QStringLiteral("provided"))) != provided
        || !isTrue(datum.value(QStringLiteral("inputValid")))
        || datum.value(QStringLiteral("declaredDatumCorrespondenceMatches")).type() != QVariant::Bool
        || isTrue(datum.value(QStringLiteral("declaredDatumCorrespondenceMatches"))) != provided
        || datum.value(QStringLiteral("reason")).type() != QVariant::String || datum.value(QStringLiteral("reason")).isNull()
        || !datum.value(QStringLiteral("reason")).toString().isEmpty()
        || datum.value(QStringLiteral("evidence")).type() != QVariant::Map || datum.value(QStringLiteral("evidence")).isNull()) return reject();
    const QVariantMap datumEvidence = datum.value(QStringLiteral("evidence")).toMap();
    if ((provided ? datumEvidence.isEmpty() || !digest(hash(datumEvidence)) : !datumEvidence.isEmpty())
        || (_clearanceSourceFrozen && _clearanceDatumEvidence != datumEvidence)
        || (provided ? datumUtc < datumUtcFloor || datumUtc >= qint64(utc) + qint64(remaining) : datumUtc != -1)) return reject();
    if (provided) {
        const qint64 deadline = before + qMin(qint64(remaining), BatchLifetimeMs);
        _clearanceDeadlineMs = _clearanceDeadlineMs < 0 ? deadline : qMin(_clearanceDeadlineMs, deadline);
        _expiresAtMs = qMin(_expiresAtMs, _clearanceDeadlineMs);
        if (_reviewExpiresAtMs >= 0) _reviewExpiresAtMs = qMin(_reviewExpiresAtMs, _clearanceDeadlineMs);
        if (!current() || _clock.elapsed() >= _clearanceDeadlineMs) return reject();
    }
    _clearanceSourceEvidence = evidence; _clearanceDatumEvidence = datumEvidence;
    _clearanceSourceProvided = provided; _clearanceSourceFrozen = true;
    // Datum's later observation advances only the private UTC watermark,
    // never the original source window or the monotonic batch/review cap.
    if (provided) { _clearanceLastUtcMs = datumUtc; _clearanceValidUntilUtcMs = qint64(utc) + qint64(remaining); }
    return current() && _clearanceSourceMemoryCurrent(generation);
}

bool FleetMissionArmedGroundReview::_clearanceSourceMemoryCurrent(quint64 generation)
{
    if (!_same(generation) || !_clearanceSourceFrozen) return false;
    if (!_clearanceSourceProvided) return true;
    const auto reject = [&]() { _clearanceSourceInvalidated = true; return false; };
    const QVariantList rows = _clearanceSourceEvidence.value(QStringLiteral("members")).toList();
    if (!_registry || !_health || _health->fleetRegistry() != _registry.data() || !_catalog || !_coordinator
        || _coordinator->firmwareAdapterCatalogBackend() != _catalog.data()
        || rows.isEmpty() || rows.size() != _targets.size()) return reject();
    QMap<int, Vehicle*> vehicles;
    for (int index = 0; index < _targets.size(); ++index) {
        const Target& target = _targets.at(index);
        const QVariantMap row = rows.at(index).toMap();
        double id = 0;
        if (!target.vehicle || !target.mission || !target.parameters || !target.links || !target.primary
            || row.size() != 2 || !integer(row.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId, &id)
            || row.value(QStringLiteral("configurationEvidence")).type() != QVariant::Map
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->parameterManager() != target.parameters.data()
            || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || !_registry->nativeConfigurationEvidenceMemoryCurrent(target.vehicle.data(), row.value(QStringLiteral("configurationEvidence")).toMap())) return reject();
        vehicles.insert(target.vehicleId, target.vehicle.data());
    }
    qint64 datumUtc = -1;
    if (!FleetMissionClearanceDatum::memoryCurrent(_clearanceDatumEvidence, _clearanceSourceEvidence,
            _registry.data(), _catalog.data(), vehicles, _clearanceLastUtcMs, &datumUtc)
        || datumUtc < _clearanceLastUtcMs || datumUtc >= _clearanceValidUntilUtcMs) return reject();
    const qint64 before = _clock.elapsed();
    const qint64 utc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (utc <= 0 || utc < _clearanceLastUtcMs || utc < datumUtc || utc >= _clearanceValidUntilUtcMs) return reject();
    _clearanceDeadlineMs = qMin(_clearanceDeadlineMs, before + qMin(_clearanceValidUntilUtcMs - utc, BatchLifetimeMs));
    _expiresAtMs = qMin(_expiresAtMs, _clearanceDeadlineMs);
    if (_reviewExpiresAtMs >= 0) _reviewExpiresAtMs = qMin(_reviewExpiresAtMs, _clearanceDeadlineMs);
    _clearanceLastUtcMs = utc;
    return _same(generation) && _executionParametersMemoryCurrent() ? true : reject();
}

bool FleetMissionArmedGroundReview::_executionParametersMemoryCurrent() const
{
    if (thread() != QThread::currentThread() || _targets.isEmpty() || _targets.size() > 32) return false;
    QSet<int> ids;
    for (const Target& target : _targets) {
        if (ids.contains(target.vehicleId) || !target.vehicle || !target.parameters || !target.links || !target.primary
            || target.vehicle->thread() != thread() || target.parameters->thread() != thread() || target.links->thread() != thread()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.vehicle->vehicleLinkManager() != target.links.data() || target.parameterSnapshot.isEmpty()
            || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
                target.primaryOwner, target.parameterSnapshot, ReviewLifetimeMs)) return false;
        ids.insert(target.vehicleId);
    }
    return true;
}

bool FleetMissionArmedGroundReview::_gate(quint64 generation, bool requireComplete)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    _checking = true; _reentered = false;
    FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QVariantMap bundle = _bundle(), identity = _identityReceipt, role = _roleReceipt;
    const QVariantMap binding = _binding, previousContext = _previousContext, health = _healthGate, catalog = _catalogSnapshot;
    const QByteArray record = _recordBytes;
    const QString reviewId = _reviewId, previousId = _previousStartupId;
    const QVariantList ids = _vehicleIds;
    const QList<Target> targets = _targets;
    const QPointer<QObject> owner(_authorization);
    const bool active = _active, held = _reservationHeld;
    const auto same = [&]() { return self && outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        && !_reentered && _same(generation) && _bundle() == bundle && owner && _authorization == owner
        && _identityReceipt == identity && _roleReceipt == role && _binding == binding && _previousContext == previousContext
        && _healthGate == health && _catalogSnapshot == catalog && _recordBytes == record && _reviewId == reviewId
        && _previousStartupId == previousId && _vehicleIds == ids && _active == active && _reservationHeld == held; };
    const auto native = [&]() {
        if (!same() || !validRole(identity, role) || !_roles->actionSnapshotCurrent(role)) return false;
        // A separate approver's real credential modal must not revoke the
        // already completed risk review merely because prompting is active.
        // The native snapshot still checks SID/role/WTS/original login expiry.
        // No ongoing READ gets this exception and no proof is consumed here.
        const bool actor = active ? _session->identitySnapshotCurrent(identity) : _session->identitySnapshot() == identity;
        if (!same() || !actor || !same()) return false;
        if ((held && !_swarm->missionDeploymentOwns(reviewId, ids)) || (!active && held && !_transportDraining)
            || _swarm->formationActive() || _swarm->formationBusy() || _swarm->hasActiveTemporaryMission(ids) || !same()) return false;
        if (_health->fleetRegistry() != _registry.data() || _health->faultToleranceManager() != _faults.data()
            || _coordinator->firmwareAdapterCatalogBackend() != _catalog.data()) return false;
        const QVariantMap currentHealth = _health->evaluateTaskGate(ids, _requireLeader);
        if (!same() || currentHealth != health || !isTrue(currentHealth.value(QStringLiteral("allowed")))
            || _faults->leaderVehicleId() != _leaderId || (_leaderId > 0 && _registry->containsVehicle(_leaderId) != _leaderRegistered)
            || _catalog->snapshot() != catalog || !same()) return false;
        QByteArray actualRecord;
        if (!currentRecord(_store.data(), binding, &actualRecord) || !same() || actualRecord != record) return false;
        const bool retained = _previous->terminalReassessmentSourcesRetained(self.data(), previousId, previousContext);
        if (!same() || !retained) return false;
        if (!_captureClearanceSource(generation) || !same()) return false;
        for (const Target& target : targets) {
            if (!_targetCurrent(target, active, true) || !same()) return false;
            if (requireComplete || !target.receipt.isEmpty()) {
                QVariantMap receipt;
                if (!_receiptCurrent(target, generation, &receipt) || !same()) return false;
            }
        }
        return same();
    };
    if (!native()) return false;
    QVariant matched;
    const bool sourceInvoked = QMetaObject::invokeMethod(owner.data(), "fleetArmedGroundReviewSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!same() || !sourceInvoked || !isTrue(matched) || !native()) return false;
    QVariant allowed;
    const bool allowedInvoked = QMetaObject::invokeMethod(owner.data(), "fleetArmedGroundReviewContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(reviewId)), Q_ARG(QVariant, QVariant(previousId)),
        Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(ids)));
    return same() && allowedInvoked && isTrue(allowed) && native()
        && _clearanceSourceMemoryCurrent(generation) && same();
}
void FleetMissionArmedGroundReview::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionArmedGroundReview::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _finish(generation, QStringLiteral("stale"), QStringLiteral("new-ground-review-source-changed")); };
    for (QObject* object : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
            static_cast<QObject*>(_previous.data()), static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()),
            static_cast<QObject*>(_health.data()), static_cast<QObject*>(_catalog.data()), static_cast<QObject*>(_coordinator.data()),
            static_cast<QObject*>(_registry.data()), static_cast<QObject*>(_faults.data()), _authorization.data()})
        _connections.append(connect(object, &QObject::destroyed, this, stale));
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        const QPointer<FleetMissionArmedGroundReview> self(this);
        const QPointer<FleetOperatorSession> session(_session);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QVariantMap identity = _identityReceipt, role = _roleReceipt;
        if (!self || !_same(generation) || !session || !roles) return;
        const bool actor = _active ? session->identitySnapshotCurrent(identity) : session->identitySnapshot() == identity;
        if (!self || !_same(generation)) return;
        if (!actor || _session != session || _roles != roles || _identityReceipt != identity || _roleReceipt != role
            || !roles->actionSnapshotCurrent(role)) _finish(generation, QStringLiteral("stale"), QStringLiteral("new-review-actor-or-role-revoked"));
        // Expiring an unrelated OLD approval is not a change to this review's
        // actual logged-in identity, nor permission to extend its deadline.
    }));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_previous.data(), &FleetMissionStartupTransaction::changed, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, stale));
    _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, stale));
    _connections.append(connect(_registry.data(), &FleetRegistry::registryChanged, this, [this, generation]() {
        if (!_same(generation) || !_clearanceSourceProvided || _clearanceSourceChecking) return;
        const QPointer<FleetMissionArmedGroundReview> self(this);
        const bool matches = _clearanceSourceMemoryCurrent(generation);
        if (self && !matches) _finish(generation, QStringLiteral("stale"), QStringLiteral("clearance-native-configuration-source-changed"));
    }));
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index);
        for (QObject* object : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
                static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(object, &QObject::destroyed, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, [stale](bool pending) { if (pending) stale(); }));
        _connections.append(connect(target.parameters.data(), &ParameterManager::parametersReadyChanged, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::missingParametersChanged, this, stale));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this,
            [this, generation, index](quint64 operation, int transaction) {
                if (!_same(generation) || index >= _targets.size()) return;
                const Target current = _targets.at(index);
                const QPointer<FleetMissionArmedGroundReview> self(this);
                const bool own = _active && index == _targetIndex && transaction == PlanManager::TransactionRead
                    && current.mission && current.primary && !current.readScopeId.isEmpty()
                    && current.mission->ownsFleetMissionRead(current.readScopeId, current.primary.data(), operation);
                if (!self || !_same(generation)) return;
                if (!own) { _finish(generation, QStringLiteral("stale"), QStringLiteral("foreign-mission-operation-during-new-review")); return; }
                _targets[index].operationId = operation;
            }));
    }
    // RAW routine notifications are checked by the timer/full mandatory fence,
    // not interpreted synchronously on a mission/flight receive stack.
}
bool FleetMissionArmedGroundReview::_publish(quint64 generation)
{
    const QPointer<FleetMissionArmedGroundReview> self(this);
    emit changed(); return self && _live(generation);
}
QVariantMap FleetMissionArmedGroundReview::requestReview(const QVariantMap& suppliedSources,
    const QString& suppliedPreviousId, bool operatorConfirmed)
{
    const QVariantMap sources = suppliedSources;
    const QString previousId = suppliedPreviousId;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _busy || _checking || _clearanceSourceChecking || _active || _reservationHeld || _transportDraining
        || _clearanceWorkerActive || _clearanceCompletionPending
        || !operatorConfirmed || !uuid(previousId) || sources.size() != 8) return rejection(QStringLiteral("confirmed-idle-new-ground-review-required"));
    _busy = true; FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(sources.value(QStringLiteral("session")).value<QObject*>()));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(sources.value(QStringLiteral("roles")).value<QObject*>()));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(sources.value(QStringLiteral("store")).value<QObject*>()));
    const QPointer<FleetMissionStartupTransaction> previous(qobject_cast<FleetMissionStartupTransaction*>(sources.value(QStringLiteral("previousStartup")).value<QObject*>()));
    const QPointer<FleetMissionDeployment> deployment(qobject_cast<FleetMissionDeployment*>(sources.value(QStringLiteral("deployment")).value<QObject*>()));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(sources.value(QStringLiteral("swarm")).value<QObject*>()));
    const QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(sources.value(QStringLiteral("health")).value<QObject*>()));
    const QPointer<QObject> owner(sources.value(QStringLiteral("authorizationOwner")).value<QObject*>());
    const auto alive = [&]() { return self && session && roles && store && previous && deployment && swarm && health && owner
        && outboundSources(this, session.data(), previous.data(), deployment.data()); };
    if (!alive()) return rejection(QStringLiteral("actual-eight-native-review-sources-required"));
    for (QObject* object : {static_cast<QObject*>(session.data()), static_cast<QObject*>(roles.data()), static_cast<QObject*>(store.data()),
            static_cast<QObject*>(previous.data()), static_cast<QObject*>(deployment.data()), static_cast<QObject*>(swarm.data()),
            static_cast<QObject*>(health.data()), owner.data()}) if (object->thread() != thread()) return rejection(QStringLiteral("same-owner-thread-required"));
    if (previous->actualSourcesForReassessment(previousId) != sources || !alive()) return rejection(QStringLiteral("previous-actual-source-bundle-mismatch"));
    const QVariantMap identity = session->identitySnapshot();
    if (!alive()) return rejection(QStringLiteral("actor-source-lost"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start.ground.reassess"));
    if (!validRole(identity, role) || isTrue(role.value(QStringLiteral("requiresSecondApproval"))) || !session->identitySnapshotCurrent(identity) || !alive())
        return rejection(QStringLiteral("current-read-only-reassessment-role-and-authenticated-actor-required"));
    const QVariantMap context = previous->terminalReassessmentContext(previousId);
    if (!alive() || context.size() != 20 || !isTrue(context.value(QStringLiteral("ok"))) || !digest(context.value(QStringLiteral("phaseFingerprint"))))
        return rejection(QStringLiteral("terminal-drained-never-mode-queued-current-ground-context-required"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    QByteArray record;
    QVariantMap capturedRecord;
    if (!bindingShape(binding) || !currentRecord(store.data(), binding, &record, &capturedRecord) || !alive()) return rejection(QStringLiteral("complete-latest-nonrestored-record-required"));
    const QVariantMap plan = context.value(QStringLiteral("deploymentContext")).toMap().value(QStringLiteral("plan")).toMap();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    const QVariantList ids = context.value(QStringLiteral("targetVehicleIds")).toList(), physical = context.value(QStringLiteral("physicalTargets")).toList();
    const QPointer<FleetFirmwareAdapterCatalog> catalog(previous->nativeReassessmentFirmwareCatalog(previousId));
    if (!alive() || !catalog) return rejection(QStringLiteral("actual-firmware-catalog-required"));
    const QPointer<FleetMissionReviewCoordinator> coordinator(previous->nativeReassessmentReviewCoordinator(previousId));
    if (!alive() || !coordinator) return rejection(QStringLiteral("actual-review-coordinator-required"));
    const QPointer<FleetRegistry> registry(previous->nativeReassessmentRegistry(previousId));
    if (!alive() || !registry) return rejection(QStringLiteral("actual-health-registry-required"));
    const QPointer<FaultToleranceManager> faults(previous->nativeReassessmentFaultPolicy(previousId));
    if (!alive() || !faults || ids.isEmpty() || ids.size() > 32 || physical.size() != ids.size()) return rejection(QStringLiteral("whole-original-group-required"));
    QList<Target> targets; QSet<int> seen; int totalItems = 0;
    for (int index = 0; index < ids.size(); ++index) {
        const QVariantMap row = physical.at(index).toMap(), payload = row.value(QStringLiteral("payload")).toMap();
        double id = 0, count = 0;
        if (!integer(ids.at(index), 1, 255, &id) || row.size() != 13 || row.value(QStringLiteral("vehicleId")) != ids.at(index)
            || row.value(QStringLiteral("startupEstimatorObservation")).type() != QVariant::Map
            || !FleetMissionStartupEstimator::healthyReference(row.value(QStringLiteral("startupEstimatorObservation")).toMap())
            || row.value(QStringLiteral("startupGlobalOriginReferenceObservation")).type() != QVariant::Map
            || !FleetMissionStartupGlobalOrigin::admissible(row.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap())
            || !isTrue(row.value(QStringLiteral("startupGlobalOriginReferenceCaptured")))
            || !isTrue(row.value(QStringLiteral("startupGlobalOriginReferenceMatches")))
            || !digest(row.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")))
            || seen.contains(static_cast<int>(id)) || !integer(payload.value(QStringLiteral("itemCount")), 1, 2048, &count)
            || !isTrue(payload.value(QStringLiteral("ok"))) || payload.value(QStringLiteral("items")).toList().size() != count
            || !digest(payload.value(QStringLiteral("contentHash"))) || (totalItems += static_cast<int>(count)) > 8192)
            return rejection(QStringLiteral("bounded-original-complete-payload-and-target-order-required"));
        Target target; target.vehicleId = static_cast<int>(id); target.member = row.value(QStringLiteral("member")).toMap(); target.payload = payload;
        target.vehicle = previous->terminalReassessmentVehicle(previousId, target.vehicleId);
        if (!alive() || !target.vehicle) return rejection(QStringLiteral("actual-original-vehicle-required"));
        target.mission = previous->terminalReassessmentManager(previousId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission) return rejection(QStringLiteral("actual-original-manager-required"));
        target.primary = previous->terminalReassessmentPrimaryLink(previousId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission || !target.primary) return rejection(QStringLiteral("actual-original-primary-required"));
        target.componentId = target.vehicle->defaultComponentId(); target.armed = target.vehicle->armed();
        target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager();
        if (!target.parameters || !target.links) return rejection(QStringLiteral("actual-original-parameter-link-sources-required"));
        if (target.links->thread() != QThread::currentThread())
            return rejection(QStringLiteral("original-primary-owner-thread-required"));
        target.primaryOwner = target.links->primaryLink();
        if (target.primaryOwner.expired()) return rejection(QStringLiteral("original-primary-owner-required-before-readiness"));
        const QVariantMap version = target.vehicle->autopilotVersionObservation(30000);
        if (!alive() || !target.vehicle || !target.parameters) return rejection(QStringLiteral("target-destroyed-during-version-capture"));
        const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MIS_TAKEOFF_ALT"), 30000);
        if (!alive() || !target.vehicle || !target.parameters || !uuid(version.value(QStringLiteral("sourceEpoch")))
            || !uuid(parameter.value(QStringLiteral("managerEpoch")))) return rejection(QStringLiteral("native-current-version-and-parameter-epochs-required"));
        target.versionEpoch = version.value(QStringLiteral("sourceEpoch")).toString(); target.parameterEpoch = parameter.value(QStringLiteral("managerEpoch")).toString();
        QVariantMap readiness = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, constraints, target.payload,
            target.armed ? FleetMissionStartReadiness::Phase::ArmedGroundReady : FleetMissionStartReadiness::Phase::BeforeArm, catalog.data());
        target.parameterSnapshot = readiness.value(QStringLiteral("parameterSummary")).toMap()
            .value(QStringLiteral("originalExecutionParameters")).toMap();
        if (!alive() || !target.vehicle || !isTrue(readiness.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !isTrue(readiness.value(QStringLiteral("startupTransitionPolicyAvailable")))
            || !isTrue(readiness.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
            || !isTrue(readiness.value(QStringLiteral("startupEstimatorEvidenceMatches")))
            || readiness.value(QStringLiteral("startupEstimatorObservation")) != row.value(QStringLiteral("startupEstimatorObservation"))
            || !isTrue(readiness.value(QStringLiteral("startupGlobalOriginReferenceCaptured")))
            || !isTrue(readiness.value(QStringLiteral("startupGlobalOriginReferenceMatches")))
            || !FleetMissionStartupGlobalOrigin::same(row.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap(),
                readiness.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap())
            || readiness.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"))
                != row.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"))
            || !target.vehicle->startupCoordinateObservationSourceMatches(target.primary.data())
            || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
                target.primaryOwner, target.parameterSnapshot, ReviewLifetimeMs)) return rejection(QStringLiteral("current-explicit-physical-phase-and-policy-required"));
        // This explicitly requested NEW read/review owns a new first basis.
        // Never revive the retired previous scope or borrow its approval;
        // once admitted below, keep this sample through READ and approval.
        target.physicalBaseline = physicalReadProjection(readiness); seen.insert(target.vehicleId); targets.append(target);
    }
    const QVariantMap currentHealth = context.value(QStringLiteral("currentHealthGate")).toMap();
    if (!alive() || !catalog || !coordinator || !registry || !faults || !isTrue(currentHealth.value(QStringLiteral("allowed")))
        || !previous->terminalReassessmentContextCurrent(previousId, context) || !alive()
        || !session->identitySnapshotCurrent(identity) || !alive() || !roles->actionSnapshotCurrent(role)
        || _generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("fresh-ground-context-changed-before-admission"));
    for (const Target& target : targets)
        if (!target.vehicle || !target.parameters || !target.links || !target.primary
            || target.vehicle->parameterManager() != target.parameters.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
                target.primaryOwner, target.parameterSnapshot, ReviewLifetimeMs))
            return rejection(QStringLiteral("original-execution-parameters-changed-before-new-review-admission"));
    _disconnectSources(); ++_generation; const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _previous = previous; _deployment = deployment; _swarm = swarm; _health = health; _authorization = owner;
    _catalog = catalog; _coordinator = coordinator; _registry = registry; _faults = faults;
    _previousContext = context; _identityReceipt = identity; _roleReceipt = role; _binding = binding; _constraints = constraints; _recordBytes = record;
    _recordReviewEvidence = capturedRecord.value(QStringLiteral("reviewEvidence")).toMap();
    _clearanceWorkerInput.clear(); _clearanceCompletedResult.clear(); _clearanceCompletionPending = false;
    _clearanceGeographicModels.clear(); _clearanceCompletedModels.clear();
    _startupClearanceWitness.reset();
    _clearanceCancellation.reset(); _clearanceWorkerFinished.reset();
    _clearanceSourcePlan = plan; _clearanceSourceEvidence.clear(); _clearanceDatumEvidence.clear(); _clearanceLastUtcMs = -1;
    _clearanceValidUntilUtcMs = -1; _clearanceDeadlineMs = -1;
    _clearanceSourceFrozen = false; _clearanceSourceProvided = false; _clearanceSourceInvalidated = false;
    _reentered = false;
    _healthGate = currentHealth; _catalogSnapshot = context.value(QStringLiteral("catalogSnapshot")).toMap();
    _vehicleIds = ids; _targets = targets; _previousStartupId = previousId; _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _state = QStringLiteral("reading-content"); _reason.clear(); _artifact.clear(); _sourceFingerprint.clear(); _active = true;
    _requireLeader = isTrue(currentHealth.value(QStringLiteral("requireLeader"))); _leaderId = faults->leaderVehicleId(); _leaderRegistered = _leaderId > 0 && registry->containsVehicle(_leaderId);
    _expiresAtMs = _clock.elapsed() + BatchLifetimeMs; _reviewExpiresAtMs = -1; _targetIndex = 0; _lastRequestAtMs = -1; _issuedAnyRequest = false;
    _targetDeadlineMs = qMin(_expiresAtMs, _clock.elapsed() + 4000 + targets.first().payload.value(QStringLiteral("itemCount")).toInt() * 250LL);
    if (!_captureClearanceSource(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("clearance-source-configuration-time-unavailable-before-read"));
        return rejection(QStringLiteral("clearance-source-configuration-time-unavailable-before-read"));
    }
    _targetDeadlineMs = qMin(_targetDeadlineMs, _expiresAtMs);
    _connectSources(generation);
    if (!_gate(generation) || !self) { if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("initial-new-review-context-denied")); return rejection(QStringLiteral("initial-new-review-context-denied")); }
    const QString reviewId = _reviewId;
    const bool reserved = swarm->reserveMissionDeployment(reviewId, ids);
    if (!self || !_live(generation) || !alive() || !reserved) {
        if (reserved && swarm) swarm->releaseMissionDeployment(reviewId);
        if (self) _finish(generation, QStringLiteral("failed"), QStringLiteral("whole-group-new-read-reservation-denied"));
        return rejection(QStringLiteral("whole-group-new-read-reservation-denied"));
    }
    _reservationHeld = true;
    if (!_gate(generation) || !self) { if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("reserved-new-review-context-denied")); return rejection(QStringLiteral("reserved-new-review-context-denied")); }
    _timer.start();
    if (!_publish(generation) || !self) return rejection(QStringLiteral("new-review-revoked-at-publication"));
    if (!outboundSources(this, session.data(), previous.data(), deployment.data())) {
        _finish(generation, QStringLiteral("stale"), QStringLiteral("native-episode-changed-at-new-review-publication"));
        return rejection(QStringLiteral("new-review-revoked-at-publication"));
    }
    QVariantMap result = current(); result.insert(QStringLiteral("ok"), true); result.insert(QStringLiteral("requestAccepted"), true); return result;
}

void FleetMissionArmedGroundReview::_finish(quint64 generation, const QString& state, const QString& reason)
{
    if (generation == 0 || generation != _generation || (!_active && _state != QStringLiteral("content-ready") && _state != QStringLiteral("acknowledged"))) return;
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QList<Target> targets = _targets;
    _active = false; _state = state; _reason = reason;
    if (state != QStringLiteral("content-ready") && state != QStringLiteral("acknowledged")) {
        _retireClearanceCalculation();
        // A completed but unsatisfied calculation is retained only for the
        // existing read-only risk display. It is not a live review/context.
        const bool blockedDiagnostic = state == QStringLiteral("clearance-blocked")
            && _artifact.value(QStringLiteral("startupClearanceReview")).type() == QVariant::Map
            && digest(_sourceFingerprint);
        const bool failedDiagnostic = state == QStringLiteral("failed")
            && _artifact.value(QStringLiteral("startupClearanceReview")).type() == QVariant::Map
            && isFalse(_artifact.value(QStringLiteral("startupClearanceReview")).toMap().value(QStringLiteral("ok")))
            && digest(_sourceFingerprint);
        if (!blockedDiagnostic && !failedDiagnostic) { _reviewExpiresAtMs = -1; _artifact.clear(); _sourceFingerprint.clear(); }
        _disconnectSources();
        if (_targetIndex < _targets.size() && _targets[_targetIndex].state != QStringLiteral("content-matched")) {
            _targets[_targetIndex].state = state; _targets[_targetIndex].reason = reason;
        }
        // Stop future requests BEFORE native cleanup. There is deliberately no
        // DISARM, rollback, parameter write, mission delete or mode command.
        for (const Target& target : targets) {
            if (target.mission && !target.readScopeId.isEmpty()) target.mission->cancelFleetMissionRead(target.readScopeId);
            if (!self || generation != _generation) return;
        }
    }
    _transportDraining = _reservationHeld; _drain();
    if (!self || generation != _generation) return;
    if (_transportDraining || _live(generation)) _timer.start(); else _timer.stop();
    emit changed();
}
void FleetMissionArmedGroundReview::_drain()
{
    if (_active || !_reservationHeld) return;
    if (_clearanceWorkerActive) { _transportDraining = true; return; }
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QPointer<SwarmController> swarm(_swarm);
    const QString reviewId = _reviewId; const quint64 generation = _generation;
    for (const Target& target : _targets) {
        if ((target.mission && (target.mission->inProgress() || target.mission->transportCallInProgress()))
            || (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())) { _transportDraining = true; return; }
    }
    if (!self || generation != _generation || reviewId != _reviewId) return;
    _reservationHeld = false; _transportDraining = false;
    if (swarm) swarm->releaseMissionDeployment(reviewId);
}
void FleetMissionArmedGroundReview::_detachDrain()
{
    const QPointer<SwarmController> swarm(_swarm);
    // The detached timer must not retain original payloads/scene/artifact.
    // The worker's independent finished marker outlives this review owner.
    struct DrainTarget { QPointer<Vehicle> vehicle; QPointer<MissionManager> mission; QString readScopeId; };
    QList<DrainTarget> targets;
    for (const Target& target : _targets) targets.append({target.vehicle, target.mission, target.readScopeId});
    const std::shared_ptr<std::atomic_bool> workerFinished = _clearanceWorkerFinished;
    const bool waitForWorker = _clearanceWorkerActive;
    const QString reviewId = _reviewId; const bool held = _reservationHeld;
    for (const DrainTarget& target : targets) if (target.mission && !target.readScopeId.isEmpty()) target.mission->cancelFleetMissionRead(target.readScopeId);
    if (!held || !swarm) return;
    QTimer* cleanup = new QTimer(swarm.data()); cleanup->setInterval(PollMs);
    connect(cleanup, &QTimer::timeout, cleanup, [cleanup, swarm, targets, reviewId, waitForWorker, workerFinished]() {
        if (!swarm) { cleanup->deleteLater(); return; }
        if (waitForWorker && (!workerFinished || !workerFinished->load(std::memory_order_acquire))) return;
        for (const DrainTarget& target : targets)
            if ((target.mission && (target.mission->inProgress() || target.mission->transportCallInProgress()))
                || (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())) return;
        const QPointer<QTimer> guarded(cleanup); cleanup->stop(); swarm->releaseMissionDeployment(reviewId);
        if (guarded) cleanup->deleteLater();
    });
    cleanup->start();
}
bool FleetMissionArmedGroundReview::_captureRisk(quint64 generation)
{
    const QPointer<FleetMissionArmedGroundReview> self(this);
    if (!_active || !_gate(generation, true) || !self) return false;
    QVariantList rows, altitudeDiagnostics; qint64 expires = qMin(_expiresAtMs, _clock.elapsed() + ReviewLifetimeMs);
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index); const qint64 before = _clock.elapsed();
        QVariantMap receipt;
        if (!_receiptCurrent(target, generation, &receipt) || !self || !_same(generation)) return false;
        double age = 0;
        if (!integer(receipt.value(QStringLiteral("ageMs")), 0, BatchLifetimeMs, &age)) return false;
        // Cross-clock comparison uses elapsed duration, never raw timestamps.
        // Completion, clicking acknowledge and future login cannot restart it.
        expires = qMin(expires, before + ReviewLifetimeMs - static_cast<qint64>(age));
        QVariantMap risk = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, _constraints, target.payload,
            target.armed ? FleetMissionStartReadiness::Phase::ArmedGroundReady : FleetMissionStartReadiness::Phase::BeforeArm, _catalog.data());
        if (!self || !_same(generation) || !target.vehicle || !isTrue(risk.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !physicalReadMatches(physicalReadProjection(risk), target.physicalBaseline)
            || risk.value(QStringLiteral("parameterSummary")).toMap().value(QStringLiteral("originalExecutionParameters")).toMap()
                != target.parameterSnapshot) return false;
        if (target.physicalBaseline.contains(QStringLiteral("startupAltitudeReferenceSample")))
            risk.insert(QStringLiteral("startupAltitudeReferenceSample"), target.physicalBaseline.value(QStringLiteral("startupAltitudeReferenceSample")));
        _targets[index].reviewReadiness = risk;
        QVariantMap riskWithoutDiagnostic = risk;
        altitudeDiagnostics.append(riskWithoutDiagnostic.take(QStringLiteral("startupAltitudeDiagnostic")));
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("componentId"), target.componentId},
            {QStringLiteral("frozenArmed"), target.armed}, {QStringLiteral("member"), target.member}, {QStringLiteral("payload"), target.payload},
            {QStringLiteral("newReadReceipt"), stableReceipt(receipt)}, {QStringLiteral("canonicalWireItems"), target.canonical.value(QStringLiteral("items"))},
            {QStringLiteral("currentRisk"), riskWithoutDiagnostic}, {QStringLiteral("actualFlightObservation"), target.vehicle->flightStateRiskObservation()}});
    }
    if (!self || !_gate(generation, true) || !self || _clock.elapsed() >= expires) return false;
    QVariantMap artifact{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("reviewEpoch"), _reviewEpoch},
        {QStringLiteral("generation"), QString::number(generation)}, {QStringLiteral("previousStartupId"), _previousStartupId},
        {QStringLiteral("previousTransactionEpoch"), _previousContext.value(QStringLiteral("transactionEpoch"))},
        {QStringLiteral("previousReference"), _previousContext}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("recordHash"), QString::fromLatin1(QCryptographicHash::hash(_recordBytes, QCryptographicHash::Sha256).toHex())},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("physicalTargets"), rows},
        {QStringLiteral("primaryIdentity"), _identityReceipt}, {QStringLiteral("roleSnapshot"), _roleReceipt},
        {QStringLiteral("currentHealthGate"), _healthGate}, {QStringLiteral("catalogSnapshot"), _catalogSnapshot},
        {QStringLiteral("allNewRawReadsMatched"), true}, {QStringLiteral("previousAuthorizationRestored"), false},
        {QStringLiteral("homeReferenceBranchValidityProven"), false}, {QStringLiteral("localReferenceResetCertified"), false},
        {QStringLiteral("pilotInputsNeutralProven"), false}, {QStringLiteral("groundAndObstacleClearanceCertified"), false},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("remoteAtomicSnapshotVerified"), false},
        {QStringLiteral("requestAttributionVerified"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (_clearanceSourceProvided) {
        artifact.insert(QStringLiteral("clearanceSourceEvidence"), _clearanceSourceEvidence);
        artifact.insert(QStringLiteral("clearanceDatumEvidence"), _clearanceDatumEvidence);
    }
    const QVariantMap originalRiskArtifact = artifact;
    QString fingerprint = hash(artifact);
    if (!digest(fingerprint)) return false;
    int diagnosticNodes = 0; qint64 diagnosticBytes = 0;
    boundedTree(artifact, 0, &diagnosticNodes, &diagnosticBytes);
    qint64 encodedBytes = QJsonDocument::fromVariant(artifact).toJson(QJsonDocument::Compact).size();
    const QString diagnosticKey = QStringLiteral("startupAltitudeDiagnostic");
    for (int index = 0; index < rows.size(); ++index) {
        const QVariant value = altitudeDiagnostics.at(index);
        if (value.type() != QVariant::Map || value.toMap().isEmpty()) continue;
        const QVariantMap diagnostic = value.toMap();
        QVariantMap row = rows.at(index).toMap(), risk = row.value(QStringLiteral("currentRisk")).toMap();
        const qint64 extraBytes = QJsonDocument::fromVariant(diagnostic).toJson(QJsonDocument::Compact).size()
            + diagnosticKey.toUtf8().size() + (risk.isEmpty() ? 3 : 4);
        int nodes = diagnosticNodes; qint64 amount = diagnosticBytes + qint64(diagnosticKey.size()) * 2;
        if (encodedBytes + extraBytes > MaximumBytes || !boundedTree(diagnostic, 4, &nodes, &amount)) continue;
        risk.insert(diagnosticKey, diagnostic); row.insert(QStringLiteral("currentRisk"), risk); rows[index] = row;
        diagnosticNodes = nodes; diagnosticBytes = amount; encodedBytes += extraBytes;
    }
    artifact.insert(QStringLiteral("physicalTargets"), rows);
    const QString withDiagnostics = hash(artifact);
    if (digest(withDiagnostics)) fingerprint = withDiagnostics;
    else artifact = originalRiskArtifact; // Drop only optional diagnostics.
    if (!digest(fingerprint) || !_same(generation) || _clock.elapsed() >= expires) return false;
    if (!_executionParametersMemoryCurrent()) return false;
    _artifact = artifact; _reviewExpiresAtMs = qMin(expires, _expiresAtMs);
    if (!_clearanceSourceProvided) { _sourceFingerprint = fingerprint; return true; }
    // The first pose/readiness and its original READ-start deadline are now
    // frozen. Do not publish a final hash until the worker result is bound.
    _sourceFingerprint.clear();
    return _beginClearanceCalculation(generation);
}
bool FleetMissionArmedGroundReview::_beginClearanceCalculation(quint64 generation)
{
    const QPointer<FleetMissionArmedGroundReview> self(this);
    if (!_same(generation) || !_active || !_clearanceSourceProvided || !_reservationHeld
        || _state != QStringLiteral("reading-content") || _clearanceWorkerActive || _clearanceCompletionPending
        || !_clearanceGeographicModels.isEmpty() || !_clearanceCompletedModels.isEmpty()
        || _targetIndex != _targets.size() || _artifact.isEmpty()
        || _reviewExpiresAtMs < 0 || _clock.elapsed() >= _reviewExpiresAtMs || !_executionParametersMemoryCurrent()) return false;
    const QVariantMap artifact = _artifact;
    const auto initialModels = copyModels(_clearanceGeographicModels);
    const QString reviewId = _reviewId, reviewEpoch = _reviewEpoch;
    const auto same = [&]() {
        return self && _same(generation) && _active && _reservationHeld
            && _state == QStringLiteral("reading-content") && _reviewId == reviewId && _reviewEpoch == reviewEpoch
            && _artifact == artifact && !_clearanceWorkerActive && !_clearanceCompletionPending
            && _clearanceGeographicModels == initialModels && _clearanceCompletedModels.isEmpty()
            && _reviewExpiresAtMs >= 0 && _clock.elapsed() < _reviewExpiresAtMs;
    };
    QVariantList vehicles;
    FleetMissionStartupGeographicMotion::Models originalModels;
    const QVariantList physical = artifact.value(QStringLiteral("physicalTargets")).toList();
    if (physical.size() != _targets.size()) return false;
    for (int index = 0; index < physical.size(); ++index) {
        const QVariantMap row = physical.at(index).toMap();
        const Target& target = _targets.at(index);
        if (row.value(QStringLiteral("vehicleId")) != _vehicleIds.at(index)
            || row.value(QStringLiteral("vehicleId")).toInt() != target.vehicleId
            || row.value(QStringLiteral("currentRisk")).type() != QVariant::Map
            || row.value(QStringLiteral("payload")).type() != QVariant::Map
            || row.value(QStringLiteral("payload")).toMap() != target.payload) return false;
        // Use the original completed-READ risk, not another live pose capture
        // or the optional diagnostic projection. The payload is the same
        // retained native target payload used for that first risk capture.
        const QVariantMap originalRisk = row.value(QStringLiteral("currentRisk")).toMap();
        const QVariantMap flight = originalRisk.value(QStringLiteral("flightStateSummary")).toMap();
        const QVariantMap channels = flight.value(QStringLiteral("channels")).toMap();
        const QVariantMap position = channels.value(QStringLiteral("globalPosition")).toMap();
        const QVariant ceiling = _clearanceSourcePlan.value(QStringLiteral("constraints")).toMap().value(QStringLiteral("maxAltitudeMeters"));
        const auto ceilingType = ceiling.type();
        if (originalRisk.value(QStringLiteral("flightStateSummary")).type() != QVariant::Map
            || flight.value(QStringLiteral("channels")).type() != QVariant::Map
            || channels.value(QStringLiteral("globalPosition")).type() != QVariant::Map
            || position.value(QStringLiteral("value")).type() != QVariant::Map
            || originalRisk.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map
            || ceiling.isNull() || (ceilingType != QVariant::Int && ceilingType != QVariant::UInt
                && ceilingType != QVariant::LongLong && ceilingType != QVariant::ULongLong && ceilingType != QVariant::Double)) return false;
        const auto motion = FleetMissionStartupGeographicMotion::prepare(position.value(QStringLiteral("value")).toMap(),
            target.payload, originalRisk.value(QStringLiteral("startupTransitionPolicy")).toMap(), ceiling.toDouble());
        if (!motion || originalModels.contains(target.vehicleId)) return false;
        originalModels.insert(target.vehicleId, motion);
        const QVariantMap readiness = riskFields(originalRisk, {
            QStringLiteral("launchType"), QStringLiteral("currentPositionAltitudeSummary"),
            QStringLiteral("takeoffDerivation"), QStringLiteral("currentPositionToFirstWaypoint")});
        if (readiness.size() != 4) return false;
        if (originalRisk.value(QStringLiteral("parameterSummary")).toMap().value(QStringLiteral("originalExecutionParameters")).toMap()
            != target.parameterSnapshot) return false;
        vehicles.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("readinessSummary"), readiness}, {QStringLiteral("geographicMotionModel"), motion->source()},
            {QStringLiteral("parameterSnapshot"), target.parameterSnapshot}});
    }
    // Use the applied value in this exact archived review. Missing/invalid
    // evidence remains null, never a plan/default/latest-map replacement.
    const QVariantMap airspace = FleetAirspaceConflictChecker::archivedMissionSpatialEvidence(
        _clearanceSourcePlan, _recordReviewEvidence);
    const QVariantMap rawInput{{QStringLiteral("schemaVersion"), 3},
        {QStringLiteral("captureIdentity"), QVariantMap{{QStringLiteral("reviewKind"), QStringLiteral("ground")},
            {QStringLiteral("reviewEpoch"), reviewEpoch}, {QStringLiteral("reviewGeneration"), QString::number(generation)},
            {QStringLiteral("reviewId"), reviewId}}},
        {QStringLiteral("plan"), _clearanceSourcePlan}, {QStringLiteral("reviewEvidence"), _recordReviewEvidence},
        {QStringLiteral("vehicles"), vehicles}, {QStringLiteral("minimumSeparationMeters"), airspace.value(QStringLiteral("minimumSeparationMeters"))},
        {QStringLiteral("clearanceSourceEvidence"), _clearanceSourceEvidence},
        {QStringLiteral("clearanceDatumEvidence"), _clearanceDatumEvidence}};
    QVariantMap input;
    if (!FleetMissionStartupClearanceWorker::snapshot(rawInput, &input)
        || !FleetMissionStartupClearanceWorker::nativeModelsMatchInput(input, originalModels)
        || !same() || !_gate(generation, true) || !same() || !_executionParametersMemoryCurrent()) return false;
    _clearanceWorkerInput = input;
    _clearanceGeographicModels = copyModels(originalModels);
    _clearanceCompletedResult.clear(); _clearanceCompletionPending = false;
    _clearanceCancellation = std::make_shared<std::atomic_bool>(false);
    const std::shared_ptr<std::atomic_bool> cancellation = _clearanceCancellation;
    _clearanceWorkerFinished = std::make_shared<std::atomic_bool>(false);
    const std::shared_ptr<std::atomic_bool> finished = _clearanceWorkerFinished;
    auto* worker = new FleetMissionStartupClearanceWorker(input, originalModels, cancellation);
    _clearanceWorker = worker; _clearanceWorkerActive = true;
    _state = QStringLiteral("clearance-computing");
    _reason = QStringLiteral("frozen-new-read-startup-clearance-calculating-no-startup-authority");
    connect(worker, &QThread::finished, worker, [finished]() {
        finished->store(true, std::memory_order_release);
    }, Qt::DirectConnection);
    connect(worker, &QThread::finished, this, [this, worker, generation, reviewId, reviewEpoch, cancellation]() {
        _clearanceCalculationFinished(worker, generation, reviewId, reviewEpoch, cancellation);
    }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater, Qt::QueuedConnection);
    // A notification may cancel/destroy this owner or enter a nested loop.
    // Always start the independently owned, possibly cancelled worker after
    // notification so actual finished/deferred cleanup still occur.
    _publish(generation);
    worker->start();
    if (!self || !_same(generation) || !_active || _state != QStringLiteral("clearance-computing")
        || _clearanceGeographicModels != originalModels || _clearanceCancellation != cancellation
        || _reviewId != reviewId || _reviewEpoch != reviewEpoch || _clearanceWorker.data() != worker) return false;
    if (!_executionParametersMemoryCurrent()) {
        _finish(generation, QStringLiteral("stale"), QStringLiteral("original-execution-parameters-changed-at-clearance-queue-publication"));
        return false;
    }
    return true;
}
void FleetMissionArmedGroundReview::_retireClearanceCalculation()
{
    if (_clearanceCancellation) _clearanceCancellation->store(true, std::memory_order_release);
    _clearanceCompletionPending = false; _clearanceCompletedResult.clear(); _clearanceWorkerInput.clear();
    _clearanceGeographicModels.clear(); _clearanceCompletedModels.clear(); _startupClearanceWitness.reset();
    // The worker slot and whole-group reservation survive until actual
    // finished and the original native transport are both drained.
}
void FleetMissionArmedGroundReview::_clearanceCalculationFinished(FleetMissionStartupClearanceWorker* worker,
    quint64 generation, const QString& reviewId, const QString& reviewEpoch,
    const std::shared_ptr<std::atomic_bool>& cancellation)
{
    if (_clearanceWorker.data() != worker) return;
    const auto originalModels = worker->originalGeographicModels();
    const QVariantMap result = worker->finishedResult();
    _clearanceWorker.clear(); _clearanceWorkerActive = false;
    if (generation == _generation && _reviewId == reviewId && _reviewEpoch == reviewEpoch
        && _active && _state == QStringLiteral("clearance-computing") && cancellation
        && _clearanceCancellation == cancellation && !cancellation->load(std::memory_order_acquire)
        && originalModels == _clearanceGeographicModels
        && FleetMissionStartupClearanceWorker::nativeModelsMatchInput(_clearanceWorkerInput, originalModels)) {
        _clearanceCompletedModels = copyModels(originalModels);
        _clearanceCompletedResult = result; _clearanceCompletionPending = true;
    } else {
        _clearanceCompletedResult.clear(); _clearanceCompletionPending = false; _clearanceWorkerInput.clear();
        _clearanceCompletedModels.clear(); _clearanceGeographicModels.clear();
        if (generation == _generation && _reviewId == reviewId && _reviewEpoch == reviewEpoch
            && _active && _state == QStringLiteral("clearance-computing")) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("startup-clearance-original-model-ownership-or-cancellation-changed"));
            return;
        }
    }
    // A finished signal can be delivered by a nested event loop while an
    // old gate is still on the stack. Defer admission to the ordinary tick;
    // the timer retains the result without recursively entering that gate.
    _timer.start();
    _tick();
}
void FleetMissionArmedGroundReview::_acceptClearanceCalculation(quint64 generation)
{
    if (!_clearanceCompletionPending || _clearanceWorkerActive) return;
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const QVariantMap input = _clearanceWorkerInput, result = _clearanceCompletedResult, originalArtifact = _artifact;
    const auto originalModels = copyModels(_clearanceCompletedModels);
    const std::shared_ptr<std::atomic_bool> cancellation = _clearanceCancellation;
    const QString reviewId = _reviewId, reviewEpoch = _reviewEpoch;
    _clearanceCompletionPending = false; _clearanceCompletedResult.clear();
    _clearanceCompletedModels.clear();
    const auto same = [&]() {
        return self && _same(generation) && _active && _reservationHeld
            && _state == QStringLiteral("clearance-computing") && _reviewId == reviewId && _reviewEpoch == reviewEpoch
            && !_clearanceWorkerActive && !_clearanceCompletionPending && _clearanceCancellation == cancellation
            && cancellation && !cancellation->load(std::memory_order_acquire)
            && _clearanceWorkerInput == input && _artifact == originalArtifact
            && _clearanceCompletedModels.isEmpty() && _clearanceGeographicModels == originalModels
            && _reviewExpiresAtMs >= 0 && _clock.elapsed() < _reviewExpiresAtMs
            && _executionParametersMemoryCurrent();
    };
    if (!same() || !FleetMissionStartupClearanceWorker::nativeModelsMatchInput(input, originalModels)
        || !FleetMissionStartupClearanceWorker::resultMatches(input, result)) {
        if (self && generation == _generation && _active)
            _finish(generation, QStringLiteral("failed"), QStringLiteral("frozen-startup-clearance-result-unavailable-or-invalid"));
        return;
    }
    // Full native rechecks validate, but never replace, the captured pose,
    // original RAW read, configuration/datum basis and non-renewable TTL.
    if (!_gate(generation, true) || !same()) {
        if (self && generation == _generation && _active)
            _finish(generation, QStringLiteral("stale"), QStringLiteral("startup-clearance-original-source-or-deadline-changed"));
        return;
    }
    QVariantMap artifact = originalArtifact;
    artifact.insert(QStringLiteral("startupClearanceReview"), result);
    QString fingerprint = hash(artifact);
    if (!digest(fingerprint)) {
        // Optional display samples must not crowd out the mandatory complete
        // calculation. Never remove v4 reference/sample or trim the report.
        QVariantList physical = artifact.value(QStringLiteral("physicalTargets")).toList();
        bool removed = false;
        for (int index = 0; index < physical.size(); ++index) {
            QVariantMap row = physical.at(index).toMap();
            QVariantMap risk = row.value(QStringLiteral("currentRisk")).toMap();
            if (risk.remove(QStringLiteral("startupAltitudeDiagnostic")) > 0) {
                row.insert(QStringLiteral("currentRisk"), risk); physical[index] = row; removed = true;
            }
        }
        if (removed) {
            artifact.insert(QStringLiteral("physicalTargets"), physical);
            fingerprint = hash(artifact);
        }
    }
    const bool constructed = isTrue(result.value(QStringLiteral("ok")));
    const bool satisfied = FleetMissionStartupClearanceWorker::startupModelSatisfied(result);
    if (!digest(fingerprint) || !same() || !_clearanceSourceMemoryCurrent(generation) || !same()
        || !_executionParametersMemoryCurrent()) {
        if (self && generation == _generation && _active)
            _finish(generation, QStringLiteral("failed"), QStringLiteral("complete-startup-clearance-artifact-or-final-source-invalid"));
        return;
    }
    if (satisfied) {
        QMap<int, Vehicle*> vehicles;
        QMap<int, WeakLinkInterfacePtr> primaryOwners;
        for (const Target& target : _targets) {
            vehicles.insert(target.vehicleId, target.vehicle.data());
            primaryOwners.insert(target.vehicleId, target.primaryOwner);
        }
        const auto witness = FleetMissionStartupClearanceWitness::capture(input, result, originalModels, _clearanceSourceEvidence,
            _clearanceDatumEvidence, _registry.data(), _catalog.data(), vehicles, primaryOwners, _clock,
            qMin(_reviewExpiresAtMs, _expiresAtMs), _clearanceLastUtcMs, same);
        if (!self) return;
        qint64 observedUtc = -1;
        if (!same() || !witness || !witness->shortenReviewDeadline(qMin(_reviewExpiresAtMs, _expiresAtMs))
            || !witness->memoryCurrent(_clearanceLastUtcMs, &observedUtc)
            || !witnessModelsMatch(witness, originalModels) || !same()) {
            if (generation == _generation && _active)
                _finish(generation, QStringLiteral("failed"), QStringLiteral("startup-clearance-native-witness-unavailable-or-source-changed"));
            return;
        }
        _clearanceLastUtcMs = observedUtc;
        _startupClearanceWitness = witness;
    }
    _artifact = artifact; _sourceFingerprint = fingerprint; _clearanceWorkerInput.clear();
    if (!constructed) {
        const QString error = result.value(QStringLiteral("error")).toString();
        _finish(generation, QStringLiteral("failed"), error.isEmpty()
            ? QStringLiteral("startup-clearance-calculation-failed") : error.left(256));
    } else {
        const QVariantMap domainReport = result.value(QStringLiteral("startupDomainReview")).toMap().value(QStringLiteral("report")).toMap();
        const QVariantMap interVehicle = result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap()
            .value(QStringLiteral("report")).toMap();
        const QString blockedReason = !isTrue(interVehicle.value(QStringLiteral("geometryComplete")))
                || !isTrue(interVehicle.value(QStringLiteral("comparisonComplete")))
            ? QStringLiteral("startup-clearance-inter-vehicle-envelope-comparison-incomplete")
            : interVehicle.value(QStringLiteral("conflictCount")).toLongLong() > 0
                ? QStringLiteral("startup-clearance-inter-vehicle-envelope-conflict")
                : !isTrue(interVehicle.value(QStringLiteral("declaredModelSatisfied")))
                    ? QStringLiteral("startup-clearance-inter-vehicle-envelope-model-not-satisfied")
                    : isFalse(domainReport.value(QStringLiteral("numericContainmentProven")))
                        ? QStringLiteral("startup-clearance-original-qt-domain-containment-unproven")
                        : QStringLiteral("startup-clearance-declared-model-incomplete-or-violated");
        _finish(generation, satisfied ? QStringLiteral("content-ready") : QStringLiteral("clearance-blocked"),
            satisfied ? QStringLiteral("new-risk-review-required-no-startup-authority")
                : blockedReason);
    }
}
void FleetMissionArmedGroundReview::_tick()
{
    if (_busy || _checking) return;
    _busy = true; FlagGuard ticking{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionArmedGroundReview> self(this); const quint64 generation = _generation;
    if (!_active) {
        const bool draining = _transportDraining; _drain();
        if (!self || _generation != generation) return;
        if (_state == QStringLiteral("content-ready") || _state == QStringLiteral("acknowledged")) {
            if (!_live(generation) || !_gate(generation, true) || !self) {
                if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("new-review-risk-source-or-original-read-start-deadline-changed"));
                return;
            }
        } else if (!_transportDraining) _timer.stop();
        if (draining != _transportDraining) emit changed();
        return;
    }
    if (!_live(generation) || !_gate(generation) || !self) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("new-read-original-group-phase-or-source-invalidated")); return;
    }
    if (_state == QStringLiteral("clearance-computing")) {
        if (_clearanceCompletionPending) _acceptClearanceCalculation(generation);
        return;
    }
    if (_targetIndex >= _targets.size()) {
        if (!_captureRisk(generation) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("whole-new-read-risk-unavailable-or-first-read-start-too-old")); return;
        }
        if (!_clearanceSourceProvided)
            _finish(generation, QStringLiteral("content-ready"), QStringLiteral("new-risk-review-required-no-startup-authority"));
        return;
    }
    if (_clock.elapsed() >= _targetDeadlineMs) { _finish(generation, QStringLiteral("failed"), QStringLiteral("bounded-new-read-timeout")); return; }
    const int index = _targetIndex; const Target target = _targets.at(index);
    if (target.state == QStringLiteral("queued")) {
        const qint64 now = _clock.elapsed(); if (_lastRequestAtMs >= 0 && now - _lastRequestAtMs < PollMs) return;
        const QString scope = QUuid::createUuid().toString(QUuid::WithoutBraces); const qint64 deadline = _targetDeadlineMs;
        const QVariantMap sources = _bundle(), identity = _identityReceipt, role = _roleReceipt;
        const QString reviewId = _reviewId, previousId = _previousStartupId;
        _targets[index].readScopeId = scope; _targets[index].state = QStringLiteral("reading-content"); _targets[index].readStartedAtMs = now;
        const auto fence = [self, generation, index, target, scope, deadline, sources, identity, role, reviewId, previousId]() {
            const auto same = [&]() {
                if (!self || !outboundSources(self.data(), self->_session.data(), self->_previous.data(), self->_deployment.data())
                    || !self->_active || !self->_same(generation) || self->_clock.elapsed() >= deadline || self->_targetDeadlineMs != deadline
                    || self->_targetIndex != index || index < 0 || index >= self->_targets.size() || self->_bundle() != sources
                    || self->_identityReceipt != identity || self->_roleReceipt != role || self->_reviewId != reviewId || self->_previousStartupId != previousId) return false;
                const Target current = self->_targets.at(index);
                return target.vehicle && target.mission && target.parameters && target.links && target.primary
                    && current.vehicle == target.vehicle && current.mission == target.mission && current.parameters == target.parameters
                    && current.links == target.links && current.primary == target.primary && current.armed == target.armed && current.member == target.member
                    && current.payload == target.payload && current.physicalBaseline == target.physicalBaseline && current.versionEpoch == target.versionEpoch
                    && current.parameterEpoch == target.parameterEpoch && current.readScopeId == scope && current.state == QStringLiteral("reading-content");
            };
            return same() && self->_gate(generation) && same();
        };
        _lastRequestAtMs = now; _issuedAnyRequest = true;
        const QVariantMap issued = target.mission->loadFleetMissionForArmedGroundReview(target.primary.data(), scope, self.data(), fence);
        if (!self || !_same(generation) || !_active) return;
        if (_targetIndex != index || _targets.at(index).readScopeId != scope || !_gate(generation) || !self) {
            if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("new-native-read-owner-changed-before-return")); return;
        }
        quint64 operation = 0, baseline = 0; double id = 0, component = 0;
        if (issued.size() != 9 || _clock.elapsed() >= deadline || !isTrue(issued.value(QStringLiteral("ok"))) || !isTrue(issued.value(QStringLiteral("requestIssued")))
            || !decimal(issued.value(QStringLiteral("operationId")), &operation) || operation == 0
            || !decimal(issued.value(QStringLiteral("baselineReceiptSequence")), &baseline) || issued.value(QStringLiteral("readScopeId")) != scope
            || !uuid(issued.value(QStringLiteral("sourceEpoch"))) || issued.value(QStringLiteral("vehicleSourceEpoch")) != target.versionEpoch
            || !integer(issued.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId, &id)
            || !integer(issued.value(QStringLiteral("componentId")), target.componentId, target.componentId, &component)
            || (_targets[index].operationId != 0 && _targets[index].operationId != operation)) {
            _finish(generation, QStringLiteral("failed"), QStringLiteral("complete-scoped-read-not-issued-or-native-receipt-invalid")); return;
        }
        _targets[index].operationId = operation; _targets[index].baselineSequence = issued.value(QStringLiteral("baselineReceiptSequence")).toString();
        _targets[index].managerEpoch = issued.value(QStringLiteral("sourceEpoch")).toString(); _publish(generation); return;
    }
    if (target.mission->inProgress() || target.mission->transportCallInProgress()) return;
    QVariantMap receipt;
    if (!_receiptCurrent(target, generation, &receipt) || !self) {
        if (self) _finish(generation, QStringLiteral("failed"), QStringLiteral("new-complete-raw-int-payload-not-matched")); return;
    }
    if (!_gate(generation) || !self || _clock.elapsed() >= _targetDeadlineMs) {
        if (self) _finish(generation, QStringLiteral("stale"), QStringLiteral("new-read-source-changed-at-capture")); return;
    }
    _targets[index].receipt = receipt; _targets[index].canonical = canonicalRaw(receipt, target.payload.value(QStringLiteral("itemCount")).toInt());
    _targets[index].state = QStringLiteral("content-matched"); ++_targetIndex;
    if (_targetIndex < _targets.size()) _targetDeadlineMs = qMin(_expiresAtMs,
        _clock.elapsed() + 4000 + _targets.at(_targetIndex).payload.value(QStringLiteral("itemCount")).toInt() * 250LL);
    _publish(generation);
}
bool FleetMissionArmedGroundReview::cancel(const QString& reviewId)
{
    if (thread() != QThread::currentThread() || reviewId != _reviewId
        || (!_active && _state != QStringLiteral("content-ready") && _state != QStringLiteral("acknowledged"))) return false;
    _finish(_generation, QStringLiteral("canceled"), QStringLiteral("operator-canceled-new-read-risk-review")); return true;
}
bool FleetMissionArmedGroundReview::acknowledgeReview(const QString& reviewId, bool operatorConfirmed)
{
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        || _busy || _checking || !operatorConfirmed || reviewId != _reviewId
        || _active || _reservationHeld || _transportDraining || _state != QStringLiteral("content-ready")) return false;
    _busy = true; FlagGuard acknowledging{QPointer<QObject>(this), &_busy}; const QPointer<FleetMissionArmedGroundReview> self(this);
    const quint64 generation = _generation;
    if (!_gate(generation, true) || !self || !_same(generation) || hash(_artifact) != _sourceFingerprint) return false;
    _state = QStringLiteral("acknowledged"); _reason = QStringLiteral("new-risk-acknowledged-fresh-independent-authority-still-required");
    // This is an acknowledgement, not consumption of the old SID proof and
    // not a command lease. The first READ START deadline does not move.
    const bool published = _publish(generation);
    return self && published && outboundSources(this, _session.data(), _previous.data(), _deployment.data());
}
QVariantMap FleetMissionArmedGroundReview::reviewContext(const QString& reviewId)
{
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        || _busy || _checking || reviewId != _reviewId || _active
        || _reservationHeld || _transportDraining || !_live(_generation)
        || (_state != QStringLiteral("content-ready") && _state != QStringLiteral("acknowledged"))) return rejection(QStringLiteral("fresh-new-risk-review-context-unavailable"));
    _busy = true; FlagGuard querying{QPointer<QObject>(this), &_busy}; const QPointer<FleetMissionArmedGroundReview> self(this);
    if (!_gate(_generation, true) || !self || hash(_artifact) != _sourceFingerprint) return rejection(QStringLiteral("new-risk-review-current-source-denied"));
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true}, {QStringLiteral("reviewId"), _reviewId},
        {QStringLiteral("reviewEpoch"), _reviewEpoch}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("previousStartupId"), _previousStartupId}, {QStringLiteral("previousTransactionEpoch"), _previousContext.value(QStringLiteral("transactionEpoch"))},
        {QStringLiteral("deploymentId"), _previousContext.value(QStringLiteral("deploymentId"))}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("sourceFingerprint"), _sourceFingerprint},
        {QStringLiteral("reviewArtifact"), _artifact}, {QStringLiteral("reviewAcknowledged"), _state == QStringLiteral("acknowledged")},
        {QStringLiteral("allNewRawReadsMatched"), true}, {QStringLiteral("remainingLifetimeMs"), static_cast<double>(_reviewExpiresAtMs - _clock.elapsed())},
        {QStringLiteral("previousAuthorizationRestored"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("scope"), QStringLiteral("new-risk-review-only-not-new-startup-authority")}};
}
bool FleetMissionArmedGroundReview::reviewContextCurrent(const QString& reviewId, const QVariantMap& supplied)
{
    const QVariantMap expected = supplied; double lifetime = 0;
    if (expected.size() != 20 || !isTrue(expected.value(QStringLiteral("ok")))
        || !integer(expected.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewLifetimeMs, &lifetime)) return false;
    const QPointer<FleetMissionArmedGroundReview> self(this); const QVariantMap actual = reviewContext(reviewId);
    QByteArray expectedBytes, actualBytes;
    return self && isTrue(actual.value(QStringLiteral("ok"))) && encode(stableContext(expected), &expectedBytes)
        && encode(stableContext(actual), &actualBytes) && expectedBytes == actualBytes;
}

QVariantMap FleetMissionArmedGroundReview::actualSourcesForStartApproval(const QString& reviewId)
{
    if (thread() != QThread::currentThread() || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())) return {};
    const QPointer<FleetMissionArmedGroundReview> self(this);
    const quint64 generation = _generation;
    const QVariantMap context = reviewContext(reviewId);
    if (!self || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        || !_same(generation) || reviewId != _reviewId || _state != QStringLiteral("acknowledged")
        || !isTrue(context.value(QStringLiteral("reviewAcknowledged"))) || !isTrue(context.value(QStringLiteral("ok")))) return {};
    QVariantMap sources = _bundle();
    sources.insert(QStringLiteral("groundReview"), QVariant::fromValue(static_cast<QObject*>(this)));
    return sources;
}
Vehicle* FleetMissionArmedGroundReview::reviewVehicle(const QString& reviewId, int vehicleId) const
{
    if (thread() != QThread::currentThread() || _busy || _checking || !_same(_generation) || _active
        || _reservationHeld || _transportDraining || _state != QStringLiteral("acknowledged") || reviewId != _reviewId) return nullptr;
    const QPointer<FleetMissionArmedGroundReview> self(const_cast<FleetMissionArmedGroundReview*>(this));
    const quint64 generation = _generation;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) {
        const Target frozen = target;
        QVariantMap receipt;
        const bool source = _targetCurrent(frozen, false, true) && _receiptCurrent(frozen, generation, &receipt);
        return self && _same(generation) && source ? frozen.vehicle.data() : nullptr;
    }
    return nullptr;
}
MissionManager* FleetMissionArmedGroundReview::reviewManager(const QString& reviewId, int vehicleId) const
{
    const QPointer<FleetMissionArmedGroundReview> self(const_cast<FleetMissionArmedGroundReview*>(this));
    const QPointer<Vehicle> vehicle(reviewVehicle(reviewId, vehicleId));
    if (!self || !vehicle) return nullptr;
    for (const Target& target : _targets) if (target.vehicle == vehicle) return target.mission.data();
    return nullptr;
}
LinkInterface* FleetMissionArmedGroundReview::reviewPrimaryLink(const QString& reviewId, int vehicleId) const
{
    const QPointer<FleetMissionArmedGroundReview> self(const_cast<FleetMissionArmedGroundReview*>(this));
    const QPointer<Vehicle> vehicle(reviewVehicle(reviewId, vehicleId));
    if (!self || !vehicle) return nullptr;
    for (const Target& target : _targets) if (target.vehicle == vehicle) return target.primary.data();
    return nullptr;
}
FleetFirmwareAdapterCatalog* FleetMissionArmedGroundReview::nativeReviewFirmwareCatalog(const QString& reviewId) const
{
    return thread() == QThread::currentThread() && outboundSources(this, _session.data(), _previous.data(), _deployment.data())
        && _same(_generation) && !_active && !_reservationHeld && !_transportDraining
        && _state == QStringLiteral("acknowledged") && reviewId == _reviewId
        && _coordinator->firmwareAdapterCatalogBackend() == _catalog.data() ? _catalog.data() : nullptr;
}
FleetMissionReviewCoordinator* FleetMissionArmedGroundReview::nativeReviewCoordinator(const QString& reviewId) const
{
    return nativeReviewFirmwareCatalog(reviewId) ? _coordinator.data() : nullptr;
}
FleetRegistry* FleetMissionArmedGroundReview::nativeReviewRegistry(const QString& reviewId) const
{
    return nativeReviewFirmwareCatalog(reviewId) && _health->fleetRegistry() == _registry.data() ? _registry.data() : nullptr;
}
FaultToleranceManager* FleetMissionArmedGroundReview::nativeReviewFaultPolicy(const QString& reviewId) const
{
    return nativeReviewFirmwareCatalog(reviewId) && _health->faultToleranceManager() == _faults.data() ? _faults.data() : nullptr;
}

std::shared_ptr<FleetMissionStartupClearanceWitness> FleetMissionArmedGroundReview::startupClearanceWitness(const QString& suppliedReviewId)
{
    const QString reviewId = suppliedReviewId;
    if (thread() != QThread::currentThread() || _busy || _checking || _clearanceSourceChecking
        || !_same(_generation) || _active || _reservationHeld || _transportDraining
        || _state != QStringLiteral("acknowledged") || reviewId != _reviewId || !_clearanceSourceProvided
        || !outboundSources(this, _session.data(), _previous.data(), _deployment.data())) return {};
    const auto witness = _startupClearanceWitness;
    const auto originalModels = copyModels(_clearanceGeographicModels);
    const QVariantMap result = _artifact.value(QStringLiteral("startupClearanceReview")).toMap();
    qint64 observedUtc = -1;
    if (!witness || witness->inputFingerprint() != result.value(QStringLiteral("inputFingerprint")).toString()
        || !_clearanceSourceMemoryCurrent(_generation)
        || !witness->shortenReviewDeadline(qMin(_reviewExpiresAtMs, _expiresAtMs))
        || !witness->memoryCurrent(_clearanceLastUtcMs, &observedUtc)
        || !witnessModelsMatch(witness, originalModels) || _clearanceGeographicModels != originalModels) return {};
    _clearanceLastUtcMs = observedUtc;
    return witness;
}

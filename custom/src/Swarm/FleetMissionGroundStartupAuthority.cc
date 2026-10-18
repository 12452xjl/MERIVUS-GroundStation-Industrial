#include "FleetMissionGroundStartupAuthority.h"
#include "FaultToleranceManager.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionGroundStartApproval.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionStartupClearanceWitness.h"
#include "FleetMissionStartupClearanceWorker.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
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
constexpr qint64 MaximumMs = 30000, MaximumBytes = 32LL * 1024 * 1024;
constexpr double ExactMaximum = 9007199254740991.0;

const QString Purpose = QStringLiteral("new-ground-reassessment-startup-basis-not-dispatch");
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    switch (value.type()) { case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break; default: return false; }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < minimum || number > maximum || std::floor(number) != number) return false;
    if (output) *output = number; return true;
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
    for (QChar c : value.toString()) if (!(c >= QLatin1Char('0') && c <= QLatin1Char('9')) && !(c >= QLatin1Char('a') && c <= QLatin1Char('f'))) return false;
    return true;
}
bool clearanceBindingMatches(bool required, const FleetMissionStartupClearanceWitness::Ptr& witness,
    const QVariantMap& artifact)
{
    if (!required) return !witness;
    const QVariant value = artifact.value(QStringLiteral("startupClearanceReview"));
    if (!witness || value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap result = value.toMap();
    return result.size() == 8 && yes(result.value(QStringLiteral("ok")))
        && result.value(QStringLiteral("startupDomainReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("startupDomainReview")).isNull()
        && !result.value(QStringLiteral("startupDomainReview")).toMap().isEmpty()
        && result.value(QStringLiteral("waypointSwitchReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("waypointSwitchReview")).isNull()
        && !result.value(QStringLiteral("waypointSwitchReview")).toMap().isEmpty()
        && result.value(QStringLiteral("interVehicleEnvelopeReview")).type() == QVariant::Map
        && !result.value(QStringLiteral("interVehicleEnvelopeReview")).isNull()
        && !result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap().isEmpty()
        // Retain the original native Witness and the shared full conclusion;
        // a ground reassessment cannot waive another vehicle's envelope.
        && FleetMissionStartupClearanceWorker::startupModelSatisfied(result)
        && digest(result.value(QStringLiteral("inputFingerprint")))
        && witness->inputFingerprint() == result.value(QStringLiteral("inputFingerprint")).toString();
}
bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > 20) return false;
    bool ok = false; const quint64 number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}
bool bounded(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 24 || ++(*nodes) > 2000000) return false;
    if (!value.isValid() || value.isNull()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += qint64(it.key().size()) * 2;
            if (*amount > MaximumBytes || !bounded(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List: for (const QVariant& item : value.toList()) if (!bounded(item, depth + 1, nodes, amount)) return false; return true;
    case QVariant::StringList: for (const QString& item : value.toStringList()) if (!bounded(item, depth + 1, nodes, amount)) return false; return true;
    case QVariant::String: *amount += qint64(value.toString().size()) * 2; return *amount <= MaximumBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= ExactMaximum;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool bytes(const QVariantMap& map, QByteArray* output)
{
    int nodes = 0; qint64 amount = 0;
    if (map.isEmpty() || !bounded(map, 0, &nodes, &amount)) return false;
    *output = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return !output->isEmpty() && output->size() <= MaximumBytes;
}
bool sameMap(const QVariantMap& left, const QVariantMap& right)
{
    QByteArray first, second;
    return bytes(left, &first) && bytes(right, &second) && first == second;
}
QString hash(const QVariantMap& map)
{
    QByteArray encoded;
    return bytes(map, &encoded) ? QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex()) : QString();
}
QVariantMap stable(QVariantMap map) { map.remove(QStringLiteral("remainingLifetimeMs")); return map; }
QObject* object(const QVariant& value) { return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr; }
bool ids(const QVariant& value)
{
    if (value.type() != QVariant::List) return false;
    const QVariantList list = value.toList(); if (list.isEmpty() || list.size() > 32) return false;
    QSet<int> seen;
    for (const QVariant& id : list) { double n = 0; if (!integer(id, 1, 255, &n) || seen.contains(int(n))) return false; seen.insert(int(n)); }
    return true;
}
bool currentRecord(FleetMissionPlanStore* store, const QVariantMap& binding, QByteArray* result)
{
    const QVariantMap latest = store->current(binding.value(QStringLiteral("sourceTaskId")).toString());
    const QVariantMap record = store->record(binding.value(QStringLiteral("sourceTaskId")).toString(), binding.value(QStringLiteral("recordRevision")));
    if (!no(latest.value(QStringLiteral("restoredFromArchive"))) || !no(record.value(QStringLiteral("restoredFromArchive")))) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"), QStringLiteral("planId"),
        QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (!binding.contains(key) || binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    return bytes(record, result);
}
bool contextShape(const QVariantMap& map)
{
    if (map.size() != 20 || !integer(map.value(QStringLiteral("schemaVersion")), 1, 1) || !yes(map.value(QStringLiteral("ok")))
        || !uuid(map.value(QStringLiteral("reviewId"))) || !uuid(map.value(QStringLiteral("reviewEpoch"))) || !decimal(map.value(QStringLiteral("generation")))
        || !uuid(map.value(QStringLiteral("previousStartupId"))) || !uuid(map.value(QStringLiteral("previousTransactionEpoch")))
        || !uuid(map.value(QStringLiteral("deploymentId"))) || map.value(QStringLiteral("binding")).type() != QVariant::Map
        || !ids(map.value(QStringLiteral("targetVehicleIds"))) || !digest(map.value(QStringLiteral("sourceFingerprint")))
        || map.value(QStringLiteral("reviewArtifact")).type() != QVariant::Map || !yes(map.value(QStringLiteral("reviewAcknowledged")))
        || !yes(map.value(QStringLiteral("allNewRawReadsMatched"))) || !integer(map.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumMs)
        || !no(map.value(QStringLiteral("previousAuthorizationRestored"))) || !no(map.value(QStringLiteral("startupPermitGranted")))
        || !no(map.value(QStringLiteral("executable"))) || !no(map.value(QStringLiteral("flightCommandReleased")))
        || map.value(QStringLiteral("scope")).type() != QVariant::String
        || map.value(QStringLiteral("scope")).toString() != QStringLiteral("new-risk-review-only-not-new-startup-authority")) return false;
    const QVariantMap artifact = map.value(QStringLiteral("reviewArtifact")).toMap();
    return hash(artifact) == map.value(QStringLiteral("sourceFingerprint")).toString() && artifact.value(QStringLiteral("targetVehicleIds")) == map.value(QStringLiteral("targetVehicleIds"))
        && artifact.value(QStringLiteral("binding")) == map.value(QStringLiteral("binding")) && no(artifact.value(QStringLiteral("previousAuthorizationRestored")))
        && no(artifact.value(QStringLiteral("startupPermitGranted"))) && no(artifact.value(QStringLiteral("executable"))) && no(artifact.value(QStringLiteral("flightCommandReleased")));
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    const QString principal = identity.value(QStringLiteral("role")).toString();
    return identity.size() == 11 && yes(identity.value(QStringLiteral("authenticated")))
        && (principal == QStringLiteral("operator") || principal == QStringLiteral("commander") || principal == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role")) && role.value(QStringLiteral("action")).type() == QVariant::String
        && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start") && yes(role.value(QStringLiteral("allowed")))
        && yes(role.value(QStringLiteral("requiresSecondApproval"))) && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}

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
    return yes(current.value(QStringLiteral("startupAltitudeReferenceRequired"))) && yes(original.value(QStringLiteral("startupAltitudeReferenceRequired")))
        && yes(current.value(QStringLiteral("startupAltitudeReferenceCaptured"))) && yes(original.value(QStringLiteral("startupAltitudeReferenceCaptured")))
        && yes(current.value(QStringLiteral("startupAltitudeReferenceMatches"))) && yes(original.value(QStringLiteral("startupAltitudeReferenceMatches")))
        && reference.size() == 20 && reference == current.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
        && digest(original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
        && hash(reference) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
        && current.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")) == original.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint"))
        && FleetMissionStartupAltitude::sampleWithinBaseline(current.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
            original.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
}
QVariantMap stableReceipt(QVariantMap value) { value.remove(QStringLiteral("ageMs")); return value; }
QVariantMap sourceContext(QVariantMap value) { value.remove(QStringLiteral("remainingLifetimeMs")); value.remove(QStringLiteral("sourceHash")); return value; }
QVariantMap denied(const QString& reason) {
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason.left(256)},
        {QStringLiteral("authorityAccepted"), false}, {QStringLiteral("previousAuthorizationRestored"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

FleetMissionGroundStartupAuthority::FleetMissionGroundStartupAuthority(QObject* parent)
    : QObject(parent), _authorityEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start(); _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (!_active || _busy || _checking || _transferInProgress) return;
        const QPointer<FleetMissionGroundStartupAuthority> self(this); const quint64 generation = _generation;
        if (!_live(generation) || !_gate(generation, true) || !self) {
            if (self) _invalidate(generation, QStringLiteral("ground-startup-native-authentication-source-risk-or-budget-changed"));
        }
    });
}
FleetMissionGroundStartupAuthority::~FleetMissionGroundStartupAuthority()
{
    const QPointer<FleetOperatorSession> session(_session); const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetMissionStartupTransaction> pending(_pendingTransaction), transaction(_transferredTransaction);
    const QString startupId = _startupId, epoch = _authorityEpoch, sourceHash = _sourceHash;
    const QString pendingEpoch = _pendingTransactionEpoch, transactionEpoch = _transferredTransactionEpoch;
    const bool held = _reservationHeld, transferred = _reservationTransferred;
    _active = false; _reservationHeld = false; _state = QStringLiteral("destroying"); _disconnectSources();
    if (session && !startupId.isEmpty()) session->revokeGroundStartupAuthentication(this, startupId, epoch);
    if (transferred) {
        // Actual Transaction owns its proof, cancellation and passive drain.
        // Never directly revoke it or free its reservation from old watchers.
        if (transaction && transaction->thread() == QThread::currentThread() && transaction->transactionEpoch() == transactionEpoch) {
            const QVariantMap status = transaction->current();
            if (transaction && status.value(QStringLiteral("startupId")).toString() == startupId
                && status.value(QStringLiteral("transactionEpoch")).toString() == transactionEpoch
                && status.value(QStringLiteral("sourceHash")).toString() == sourceHash) transaction->revokeStartup(startupId);
        }
    } else {
        // A Session callback may destroy this old owner while the new proof
        // exists but before the wrapper commits. No action is issued yet.
        if (session && pending && !pendingEpoch.isEmpty()) session->revokeGroundStartupTransactionAuthentication(pending.data(), startupId, pendingEpoch);
        if (held && swarm) swarm->releaseMissionDeployment(startupId);
    }
}
bool FleetMissionGroundStartupAuthority::_live(quint64 generation) const
{
    return thread() == QThread::currentThread() && generation != 0 && generation == _generation
        && _expiresAtMs >= 0 && _clock.elapsed() < _expiresAtMs
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("accepted"));
}
bool FleetMissionGroundStartupAuthority::_same(quint64 generation) const
{
    return _live(generation) && outboundGroup(this, {_session.data(), _review.data(), _approval.data(), _previous.data(), _deployment.data()})
        && _session && _roles && _store && _review && _approval && _previous && _deployment && _swarm && _health
        && _authorization && _catalog && _coordinator && _registry && _faults;
}
QVariantMap FleetMissionGroundStartupAuthority::_bundle() const
{
    const auto actual = [](QObject* pointer) { return QVariant::fromValue(pointer); };
    return {{QStringLiteral("session"), actual(_session.data())}, {QStringLiteral("roles"), actual(_roles.data())},
        {QStringLiteral("store"), actual(_store.data())}, {QStringLiteral("groundReview"), actual(_review.data())},
        {QStringLiteral("groundApproval"), actual(_approval.data())}, {QStringLiteral("previousStartup"), actual(_previous.data())},
        {QStringLiteral("deployment"), actual(_deployment.data())}, {QStringLiteral("swarm"), actual(_swarm.data())},
        {QStringLiteral("health"), actual(_health.data())}, {QStringLiteral("authorizationOwner"), actual(_authorization.data())}};
}
QVariantMap FleetMissionGroundStartupAuthority::current() const
{
    QVariantList rows;
    for (const Target& target : _targets) rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("componentId"), target.componentId}, {QStringLiteral("frozenArmed"), target.armed},
        {QStringLiteral("readScopeId"), target.readReceipt.value(QStringLiteral("readScopeId"))},
        {QStringLiteral("payloadContentHash"), target.payload.value(QStringLiteral("contentHash"))}});
    const bool accepted = _active && _state == QStringLiteral("accepted") && _reservationHeld && _live(_generation);
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("authorityAccepted"), accepted}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("authorityEpoch"), _authorityEpoch}, {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("reviewId"), _reviewContext.value(QStringLiteral("reviewId"))},
        {QStringLiteral("previousStartupId"), _reviewContext.value(QStringLiteral("previousStartupId"))},
        {QStringLiteral("deploymentId"), _reviewContext.value(QStringLiteral("deploymentId"))}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("vehicles"), rows}, {QStringLiteral("sourceHash"), _sourceHash},
        {QStringLiteral("active"), _active}, {QStringLiteral("reservationHeld"), _reservationHeld}, {QStringLiteral("transportDraining"), _transportDraining},
        {QStringLiteral("reservationTransferred"), _reservationTransferred}, {QStringLiteral("transactionEpoch"), _transferredTransactionEpoch},
        {QStringLiteral("remainingMs"), double(qMax<qint64>(0, _expiresAtMs - _clock.elapsed()))}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("previousAuthorizationRestored"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap FleetMissionGroundStartupAuthority::_context() const
{
    QVariantList physical;
    for (const Target& target : _targets) physical.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("componentId"), target.componentId}, {QStringLiteral("frozenArmed"), target.armed},
        {QStringLiteral("member"), target.member}, {QStringLiteral("payload"), target.payload},
        {QStringLiteral("newReadReceipt"), target.readReceipt}, {QStringLiteral("currentRisk"), target.readiness},
        {QStringLiteral("versionEpoch"), target.versionEpoch}, {QStringLiteral("parameterEpoch"), target.parameterEpoch}, {QStringLiteral("flightEpoch"), target.flightEpoch}});
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("authorityEpoch"), _authorityEpoch}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("reviewId"), _reviewContext.value(QStringLiteral("reviewId"))}, {QStringLiteral("reviewEpoch"), _reviewContext.value(QStringLiteral("reviewEpoch"))},
        {QStringLiteral("reviewGeneration"), _reviewContext.value(QStringLiteral("generation"))},
        {QStringLiteral("previousStartupId"), _reviewContext.value(QStringLiteral("previousStartupId"))},
        {QStringLiteral("previousTransactionEpoch"), _reviewContext.value(QStringLiteral("previousTransactionEpoch"))},
        {QStringLiteral("deploymentId"), _reviewContext.value(QStringLiteral("deploymentId"))}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("sourceHash"), _sourceHash}, {QStringLiteral("targetVehicleIds"), _vehicleIds}, {QStringLiteral("physicalTargets"), physical},
        {QStringLiteral("newReviewContext"), _reviewContext}, {QStringLiteral("primaryIdentity"), _identity}, {QStringLiteral("roleSnapshot"), _role},
        {QStringLiteral("catalogSnapshot"), _catalogSnapshot}, {QStringLiteral("healthGate"), _healthGate}, {QStringLiteral("scope"), Purpose},
        {QStringLiteral("originalRisksPreserved"), true}, {QStringLiteral("previousAuthorizationRestored"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("remainingLifetimeMs"), double(qMax<qint64>(0, _expiresAtMs - _clock.elapsed()))}};
}
bool FleetMissionGroundStartupAuthority::_targetsCurrent(quint64 generation) const
{
    const QPointer<FleetMissionGroundStartupAuthority> self(const_cast<FleetMissionGroundStartupAuthority*>(this));
    const QList<Target> targets = _targets; const QPointer<FleetMissionArmedGroundReview> review(_review);
    const QVariantMap constraints = _constraints; const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const auto same = [&]() { return self && _same(generation) && review && catalog && _review == review && _catalog == catalog && _constraints == constraints; };
    if (!same() || targets.isEmpty() || targets.size() > 32) return false;
    for (const Target& target : targets) {
        const auto alive = [&]() { return same() && target.vehicle && target.mission && target.parameters && target.links && target.primary; };
        if (!alive() || target.vehicle->thread() != thread() || target.mission->thread() != thread() || target.parameters->thread() != thread()
            || target.links->thread() != thread()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->parameterManager() != target.parameters.data()
            || target.parameters->vehicle() != target.vehicle.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.vehicle->armed() != target.armed || target.vehicle->flying() || target.links->communicationLost()
            || target.links->primaryLink().lock().get() != target.primary.data() || target.mission->inProgress() || target.mission->transportCallInProgress()) return false;
        const bool writes = target.parameters->pendingWrites();
        if (!alive() || writes || !target.parameters->parametersReady() || target.parameters->missingParameters()) return false;
        const bool physicalSource = target.vehicle->fleetArmedGroundReviewSourceMatches(target.primary.data())
            && target.vehicle->startupCoordinateObservationSourceMatches(target.primary.data()) && target.vehicle->fleetStartupTransportQuiescent();
        if (!alive() || !physicalSource) return false;
        const QVariantMap version = target.vehicle->autopilotVersionObservation(MaximumMs);
        if (!alive() || version.value(QStringLiteral("sourceEpoch")) != target.versionEpoch) return false;
        const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MIS_TAKEOFF_ALT"), MaximumMs);
        if (!alive() || parameter.value(QStringLiteral("managerEpoch")) != target.parameterEpoch) return false;
        const QVariantMap flight = target.vehicle->flightStateRiskObservation();
        if (!alive() || flight.value(QStringLiteral("sourceEpoch")) != target.flightEpoch) return false;
        const bool before = target.mission->fleetMissionArmedGroundReadReceiptSourceMatches(target.primary.data(),
            target.readReceipt.value(QStringLiteral("operationId")).toString(), target.readReceipt.value(QStringLiteral("readScopeId")).toString(), review.data());
        if (!alive() || !before) return false;
        const QVariantMap receipt = target.mission->fleetMissionArmedGroundReadReceipt(MaximumMs, review.data());
        if (!alive() || receipt.size() != 34 || !yes(receipt.value(QStringLiteral("available"))) || !yes(receipt.value(QStringLiteral("fresh")))
            || !sameMap(stableReceipt(receipt), target.readReceipt)) return false;
        const bool after = target.mission->fleetMissionArmedGroundReadReceiptSourceMatches(target.primary.data(),
            target.readReceipt.value(QStringLiteral("operationId")).toString(), target.readReceipt.value(QStringLiteral("readScopeId")).toString(), review.data());
        if (!alive() || !after) return false;
        const QVariantMap readiness = FleetMissionStartReadiness::captureForStartup(target.vehicle.data(), target.member, constraints, target.payload,
            target.armed ? FleetMissionStartReadiness::Phase::ArmedGroundReady : FleetMissionStartReadiness::Phase::BeforeArm, catalog.data());
        if (!alive() || !yes(readiness.value(QStringLiteral("startupPhasePreconditionsMatch")))
            || !sameMap(stableReadiness(readiness), stableReadiness(target.readiness))
            || !altitudeMatches(readiness, target.readiness, target.readiness.value(QStringLiteral("startupTransitionPolicy")).toMap())) return false;
    }
    return same();
}

bool FleetMissionGroundStartupAuthority::_gate(quint64 generation, bool requireAuthentication)
{
    if (thread() != QThread::currentThread() || _checking) { _reentered = true; return false; }
    if (!outboundGroup(this, {_session.data(), _review.data(), _approval.data(), _previous.data(), _deployment.data()})) return false;
    _checking = true; _reentered = false; FlagGuard checking{QPointer<QObject>(this), &_checking};
    const QPointer<FleetMissionGroundStartupAuthority> self(this); const QPointer<QObject> owner(_authorization);
    const QVariantMap bundle = _bundle(), identity = _identity, role = _role, binding = _binding, reviewContext = _reviewContext;
    const QVariantMap authentication = _authentication, grant = _grant, health = _healthGate, catalog = _catalogSnapshot;
    const QVariantList vehicleIds = _vehicleIds; const QByteArray record = _recordBytes;
    const QString startupId = _startupId, epoch = _authorityEpoch, sourceHash = _sourceHash;
    const auto clearance = _clearanceWitness;
    const bool clearanceRequired = _clearanceRequired;
    const bool held = _reservationHeld, active = _active;
    const auto same = [&]() { return self && !_reentered && _same(generation) && owner && _authorization == owner && _bundle() == bundle
        && _identity == identity && _role == role && _binding == binding && _reviewContext == reviewContext && _authentication == authentication
        && _grant == grant && _healthGate == health && _catalogSnapshot == catalog && _recordBytes == record && _vehicleIds == vehicleIds
        && _startupId == startupId && _authorityEpoch == epoch && _sourceHash == sourceHash && _reservationHeld == held && _active == active
        && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired; };
    const auto native = [&]() {
        if (!same() || !clearanceBindingMatches(clearanceRequired, clearance,
                reviewContext.value(QStringLiteral("reviewArtifact")).toMap())) return false;
        if (clearanceRequired && (!clearance->sourceCurrent(same) || !same())) return false;
        if (!same() || !roleMatches(identity, role) || !_roles->actionSnapshotCurrent(role)) return false;
        const bool actor = _session->identitySnapshotCurrent(identity);
        if (!same() || !actor) return false;
        if (requireAuthentication) {
            const bool current = _session->groundStartupAuthenticationCurrent(authentication, self.data(), startupId, epoch, sourceHash);
            if (!same() || !current) return false;
        } else {
            // Preparation may use the old NEW-review ledger only until its
            // one-time move. After that the independent owner revalidates its
            // own full native evidence, never an old approval.valid scalar.
            const bool current = _review->reviewContextCurrent(reviewContext.value(QStringLiteral("reviewId")).toString(), reviewContext);
            if (!same() || !current) return false;
            if (!grant.isEmpty()) {
                const bool exactGrant = _approval->grantCurrent(grant, startupId);
                if (!same() || !exactGrant) return false;
            }
        }
        if ((held && !_swarm->missionDeploymentOwns(startupId, vehicleIds)) || !same() || _swarm->formationActive() || _swarm->formationBusy()
            || _swarm->hasActiveTemporaryMission(vehicleIds) || !same()) return false;
        if (_health->fleetRegistry() != _registry.data() || _health->faultToleranceManager() != _faults.data()
            || _coordinator->firmwareAdapterCatalogBackend() != _catalog.data()) return false;
        const QVariantMap actualHealth = _health->evaluateTaskGate(vehicleIds, _requireLeader);
        if (!same() || actualHealth != health || !yes(actualHealth.value(QStringLiteral("allowed"))) || _faults->leaderVehicleId() != _leaderId
            || (_leaderId > 0 && _registry->containsVehicle(_leaderId) != _leaderRegistered) || _catalog->snapshot() != catalog || !same()) return false;
        QByteArray actualRecord;
        if (!currentRecord(_store.data(), binding, &actualRecord) || !same() || actualRecord != record
            || !_targetsCurrent(generation) || !same()
            || !clearanceBindingMatches(clearanceRequired, clearance, reviewContext.value(QStringLiteral("reviewArtifact")).toMap())) return false;
        if (clearanceRequired && !clearance->memoryCurrent()) return false;
        return same();
    };
    if (!native()) return false;
    QVariant matched;
    const bool sources = QMetaObject::invokeMethod(owner.data(), "fleetGroundStartupAuthoritySourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))), Q_ARG(QVariant, QVariant(bundle)));
    if (!same() || !sources || !yes(matched) || !native()) return false;
    QVariant allowed;
    const bool context = QMetaObject::invokeMethod(owner.data(), "fleetGroundStartupAuthorityContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, allowed), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant(startupId)), Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(vehicleIds)));
    return same() && context && yes(allowed) && native() && same()
        && (!clearanceRequired || clearance->memoryCurrent());
}
bool FleetMissionGroundStartupAuthority::authenticationTransferContextCurrent(const QString& startupId, const QString& sourceHash,
    QObject* approvalObject, const QVariantMap& suppliedBasis)
{
    const QVariantMap basis = suppliedBasis;
    // PURE ownership memory: the Session performs current native identities;
    // caller completes full risk/source gates before and AFTER the move.
    return thread() == QThread::currentThread() && _same(_generation) && _state == QStringLiteral("preparing")
        && _busy && _transferInProgress && !_checking && !_active && !_reservationHeld
        && _approval.data() == approvalObject && outboundGroup(this, {approvalObject})
        && startupId == _startupId && sourceHash == _sourceHash && digest(sourceHash)
        && _grant.size() == 14 && _grant.value(QStringLiteral("consumerId")).toString() == startupId
        && basis.size() == 22 && sameMap(basis, _grant.value(QStringLiteral("authenticationReceipt")).toMap())
        && hash(sourceContext(_context())) == _sourceHash;
}
void FleetMissionGroundStartupAuthority::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionGroundStartupAuthority::_invalidate(quint64 generation, const QString& suppliedReason, const QString& state)
{
    const QString reason = suppliedReason.left(256);
    if (!generation || generation != _generation || (_state != QStringLiteral("preparing") && _state != QStringLiteral("accepted"))) return;
    const QPointer<FleetMissionGroundStartupAuthority> self(this); const QPointer<FleetOperatorSession> session(_session);
    const QPointer<SwarmController> swarm(_swarm); const QString startupId = _startupId, epoch = _authorityEpoch;
    const QPointer<FleetMissionStartupTransaction> pending(_pendingTransaction); const QString pendingEpoch = _pendingTransactionEpoch;
    const bool held = _reservationHeld;
    _active = false; _state = state; _reason = reason; _transferInProgress = false; _reservationHeld = false;
    _pendingTransaction.clear(); _pendingTransactionEpoch.clear(); _transferContext.clear();
    _authentication.clear(); _timer.stop(); _disconnectSources();
    if (session) session->revokeGroundStartupAuthentication(self.data(), startupId, epoch);
    if (session && pending && !pendingEpoch.isEmpty()) session->revokeGroundStartupTransactionAuthentication(pending.data(), startupId, pendingEpoch);
    // Revoke emits native notifications that may delete/replace this owner.
    // Release the captured OLD UUID even then; never use replacement fields.
    if (held && swarm) swarm->releaseMissionDeployment(startupId);
    if (self && generation == _generation && _startupId == startupId && _state == state) emit changed();
}
void FleetMissionGroundStartupAuthority::_connectSources(quint64 generation)
{
    const auto stale = [this, generation]() { _invalidate(generation, QStringLiteral("ground-startup-actual-source-changed")); };
    for (QObject* source : {static_cast<QObject*>(_session.data()), static_cast<QObject*>(_roles.data()), static_cast<QObject*>(_store.data()),
        static_cast<QObject*>(_review.data()), static_cast<QObject*>(_approval.data()), static_cast<QObject*>(_previous.data()),
        static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_swarm.data()), static_cast<QObject*>(_health.data()), _authorization.data(),
        static_cast<QObject*>(_catalog.data()), static_cast<QObject*>(_coordinator.data()), static_cast<QObject*>(_registry.data()), static_cast<QObject*>(_faults.data())})
        _connections.append(connect(source, &QObject::destroyed, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, stale));
    _connections.append(connect(_previous.data(), &FleetMissionStartupTransaction::changed, this, stale));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, stale));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, stale));
    _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, stale));
    _connections.append(connect(_coordinator.data(), &FleetMissionReviewCoordinator::firmwareAdapterCatalogChanged, this, stale));
    // Old Review/Approval natural retirement is not itself an authentication
    // source change. Their actual QQ owners still must live; new evidence is
    // re-read by this owner, and explicit old-scope revoke forwards exactly.
    _connections.append(connect(_session.data(), &FleetOperatorSession::changed, this, [this, generation]() {
        if (_busy || _checking || _transferInProgress) return;
        const QPointer<FleetMissionGroundStartupAuthority> self(this);
        if (!_gate(generation, !_authentication.isEmpty()) && self) _invalidate(generation, QStringLiteral("ground-startup-current-account-revoked"));
    }));
    for (const Target& target : _targets) {
        for (QObject* source : {static_cast<QObject*>(target.vehicle.data()), static_cast<QObject*>(target.mission.data()),
            static_cast<QObject*>(target.parameters.data()), static_cast<QObject*>(target.links.data()), static_cast<QObject*>(target.primary.data())})
            _connections.append(connect(source, &QObject::destroyed, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, stale));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale));
        _connections.append(connect(target.links.data(), &VehicleLinkManager::communicationLostChanged, this, stale));
        _connections.append(connect(target.primary.data(), &LinkInterface::disconnected, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, [stale](bool value) { if (value) stale(); }));
        _connections.append(connect(target.parameters.data(), &ParameterManager::parametersReadyChanged, this, stale));
        _connections.append(connect(target.parameters.data(), &ParameterManager::missingParametersChanged, this, stale));
        _connections.append(connect(target.mission.data(), &MissionManager::operationStarted, this, stale));
    }
}

QVariantMap FleetMissionGroundStartupAuthority::requestAuthority(const QVariantMap& suppliedSources, const QString& suppliedReviewId, bool operatorConfirmed)
{
    const QVariantMap sources = suppliedSources; const QString reviewId = suppliedReviewId;
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _busy || _checking || _active || _reservationHeld || _reservationTransferred || _transferInProgress
        || _pendingTransaction || _state == QStringLiteral("transferred")
        || !operatorConfirmed || !uuid(reviewId) || sources.size() != 10) return denied(QStringLiteral("confirmed-idle-new-ground-startup-basis-required"));
    _busy = true; FlagGuard requesting{QPointer<QObject>(this), &_busy}; const QPointer<FleetMissionGroundStartupAuthority> self(this);
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(object(sources.value(QStringLiteral("session")))));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(object(sources.value(QStringLiteral("roles")))));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(object(sources.value(QStringLiteral("store")))));
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(object(sources.value(QStringLiteral("groundReview")))));
    const QPointer<FleetMissionGroundStartApproval> approval(qobject_cast<FleetMissionGroundStartApproval*>(object(sources.value(QStringLiteral("groundApproval")))));
    const QPointer<FleetMissionStartupTransaction> previous(qobject_cast<FleetMissionStartupTransaction*>(object(sources.value(QStringLiteral("previousStartup")))));
    const QPointer<FleetMissionDeployment> deployment(qobject_cast<FleetMissionDeployment*>(object(sources.value(QStringLiteral("deployment")))));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(object(sources.value(QStringLiteral("swarm")))));
    const QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(object(sources.value(QStringLiteral("health")))));
    const QPointer<QObject> owner(object(sources.value(QStringLiteral("authorizationOwner"))));
    const auto alive = [&]() { return self && session && roles && store && review && approval && previous && deployment && swarm && health && owner
        && outboundGroup(self.data(), {session.data(), review.data(), approval.data(), previous.data(), deployment.data()}); };
    if (!alive()) return denied(QStringLiteral("actual-ten-native-sources-required"));
    for (QObject* source : {static_cast<QObject*>(session.data()), static_cast<QObject*>(roles.data()), static_cast<QObject*>(store.data()),
        static_cast<QObject*>(review.data()), static_cast<QObject*>(approval.data()), static_cast<QObject*>(previous.data()),
        static_cast<QObject*>(deployment.data()), static_cast<QObject*>(swarm.data()), static_cast<QObject*>(health.data()), owner.data()})
        if (source->thread() != thread()) return denied(QStringLiteral("same-native-owner-thread-required"));
    const qint64 beforeContext = _clock.elapsed(); const QVariantMap context = review->reviewContext(reviewId);
    if (!alive() || !contextShape(context) || context.value(QStringLiteral("reviewId")).toString() != reviewId) return denied(QStringLiteral("fresh-acknowledged-new-review-required"));
    const QVariantMap actualSources = review->actualSourcesForStartApproval(reviewId);
    if (!alive() || actualSources.size() != 9) return denied(QStringLiteral("actual-new-review-source-bundle-required"));
    const QStringList originalKeys{QStringLiteral("session"), QStringLiteral("roles"), QStringLiteral("store"), QStringLiteral("groundReview"),
        QStringLiteral("previousStartup"), QStringLiteral("deployment"), QStringLiteral("swarm"), QStringLiteral("health"), QStringLiteral("authorizationOwner")};
    for (const QString& key : originalKeys) if (object(actualSources.value(key)) != object(sources.value(key))) return denied(QStringLiteral("new-review-source-substitution-denied"));
    const QPointer<FleetFirmwareAdapterCatalog> catalog(review->nativeReviewFirmwareCatalog(reviewId));
    if (!alive() || !catalog) return denied(QStringLiteral("actual-catalog-required"));
    const QPointer<FleetMissionReviewCoordinator> coordinator(review->nativeReviewCoordinator(reviewId));
    if (!alive() || !catalog || !coordinator) return denied(QStringLiteral("actual-coordinator-required"));
    const QPointer<FleetRegistry> registry(review->nativeReviewRegistry(reviewId));
    if (!alive() || !registry) return denied(QStringLiteral("actual-registry-required"));
    const QPointer<FaultToleranceManager> faults(review->nativeReviewFaultPolicy(reviewId));
    if (!alive() || !faults) return denied(QStringLiteral("actual-fault-policy-required"));
    const QVariantMap identity = session->identitySnapshot();
    if (!alive()) return denied(QStringLiteral("identity-source-destroyed"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start")), artifact = context.value(QStringLiteral("reviewArtifact")).toMap();
    if (!alive() || !roleMatches(identity, role) || !sameMap(identity, artifact.value(QStringLiteral("primaryIdentity")).toMap())
        || !session->identitySnapshotCurrent(identity) || !alive() || !roles->actionSnapshotCurrent(role)) return denied(QStringLiteral("same-current-native-primary-and-role-required"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap(); QByteArray record;
    if (!currentRecord(store.data(), binding, &record) || !alive()
        || QString::fromLatin1(QCryptographicHash::hash(record, QCryptographicHash::Sha256).toHex()) != artifact.value(QStringLiteral("recordHash")).toString())
        return denied(QStringLiteral("full-latest-nonrestored-record-required"));
    const QVariantMap plan = artifact.value(QStringLiteral("previousReference")).toMap().value(QStringLiteral("deploymentContext")).toMap().value(QStringLiteral("plan")).toMap();
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    const bool clearanceRequired = constraints.contains(QStringLiteral("clearanceInput"));
    const auto clearance = clearanceRequired ? review->startupClearanceWitness(reviewId) : FleetMissionStartupClearanceWitness::Ptr();
    if (!alive() || !clearanceBindingMatches(clearanceRequired, clearance, artifact)
        || (clearanceRequired && (!clearance->sourceCurrent(alive) || !alive())))
        return denied(QStringLiteral("original-native-ground-clearance-witness-required"));
    const QVariantList vehicleIds = context.value(QStringLiteral("targetVehicleIds")).toList(), rows = artifact.value(QStringLiteral("physicalTargets")).toList();
    if (!ids(vehicleIds) || rows.size() != vehicleIds.size()) return denied(QStringLiteral("exact-whole-reviewed-group-required"));
    QList<Target> targets;
    for (int index = 0; index < rows.size(); ++index) {
        const QVariantMap row = rows[index].toMap(); double id = 0, component = 0;
        if (!integer(vehicleIds[index], 1, 255, &id) || !integer(row.value(QStringLiteral("vehicleId")), id, id)
            || !integer(row.value(QStringLiteral("componentId")), 1, 255, &component) || row.value(QStringLiteral("frozenArmed")).type() != QVariant::Bool)
            return denied(QStringLiteral("actual-target-and-armed-phase-required"));
        Target target; target.vehicleId = int(id); target.componentId = int(component); target.armed = row.value(QStringLiteral("frozenArmed")).toBool();
        target.member = row.value(QStringLiteral("member")).toMap(); target.payload = row.value(QStringLiteral("payload")).toMap();
        target.readReceipt = row.value(QStringLiteral("newReadReceipt")).toMap(); target.readiness = row.value(QStringLiteral("currentRisk")).toMap();
        target.vehicle = review->reviewVehicle(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle) return denied(QStringLiteral("actual-reviewed-vehicle-required"));
        target.mission = review->reviewManager(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.mission) return denied(QStringLiteral("actual-reviewed-manager-required"));
        target.primary = review->reviewPrimaryLink(reviewId, target.vehicleId);
        if (!alive() || !target.vehicle || !target.primary) return denied(QStringLiteral("actual-reviewed-primary-required"));
        target.parameters = target.vehicle->parameterManager(); target.links = target.vehicle->vehicleLinkManager();
        if (!target.parameters || !target.links) return denied(QStringLiteral("actual-reviewed-parameter-link-sources-required"));
        const QVariantMap version = target.vehicle->autopilotVersionObservation(MaximumMs);
        if (!alive() || !target.vehicle || !target.parameters) return denied(QStringLiteral("source-destroyed-at-version-capture"));
        const QVariantMap parameter = target.parameters->parameterObservation(target.componentId, QStringLiteral("MIS_TAKEOFF_ALT"), MaximumMs);
        if (!alive() || !target.vehicle || !target.parameters) return denied(QStringLiteral("source-destroyed-at-parameter-capture"));
        const QVariantMap flight = target.vehicle->flightStateRiskObservation();
        if (!alive() || !uuid(version.value(QStringLiteral("sourceEpoch"))) || !uuid(parameter.value(QStringLiteral("managerEpoch"))) || !uuid(flight.value(QStringLiteral("sourceEpoch"))))
            return denied(QStringLiteral("native-version-parameter-flight-epochs-required"));
        target.versionEpoch = version.value(QStringLiteral("sourceEpoch")).toString(); target.parameterEpoch = parameter.value(QStringLiteral("managerEpoch")).toString();
        target.flightEpoch = flight.value(QStringLiteral("sourceEpoch")).toString(); targets.append(target);
    }
    const QVariantMap approvalView = approval->current(); const QVariantMap approvalReceipt = approvalView.value(QStringLiteral("approvalReceipt")).toMap();
    if (!alive() || approvalView.value(QStringLiteral("state")).toString() != QStringLiteral("approved") || approvalReceipt.size() != 27
        || approvalReceipt.value(QStringLiteral("reviewId")).toString() != reviewId || approvalReceipt.value(QStringLiteral("sourceFingerprint")) != context.value(QStringLiteral("sourceFingerprint"))
        || !review->reviewContextCurrent(reviewId, context) || !alive() || _generation == std::numeric_limits<quint64>::max())
        return denied(QStringLiteral("exact-current-new-conditional-approval-required"));
    const QVariantMap healthGate = artifact.value(QStringLiteral("currentHealthGate")).toMap();
    if (!yes(healthGate.value(QStringLiteral("allowed"))) || healthGate.value(QStringLiteral("requireLeader")).type() != QVariant::Bool)
        return denied(QStringLiteral("actual-current-health-required"));
    if (clearanceRequired) {
        const auto currentWitness = review->startupClearanceWitness(reviewId);
        if (!alive() || currentWitness != clearance || !clearance->memoryCurrent())
            return denied(QStringLiteral("original-native-ground-clearance-witness-changed"));
    }
    _disconnectSources(); ++_generation; const quint64 generation = _generation;
    _session = session; _roles = roles; _store = store; _review = review; _approval = approval; _previous = previous; _deployment = deployment;
    _swarm = swarm; _health = health; _authorization = owner; _catalog = catalog; _coordinator = coordinator; _registry = registry; _faults = faults;
    _targets = targets; _vehicleIds = vehicleIds; _binding = binding; _constraints = constraints; _recordBytes = record; _reviewContext = context;
    _clearanceRequired = clearanceRequired; _clearanceWitness = clearance;
    _identity = identity; _role = role; _healthGate = healthGate; _catalogSnapshot = artifact.value(QStringLiteral("catalogSnapshot")).toMap();
    _requireLeader = yes(healthGate.value(QStringLiteral("requireLeader"))); _leaderId = faults->leaderVehicleId(); _leaderRegistered = _leaderId > 0 && registry->containsVehicle(_leaderId);
    _startupId = QUuid::createUuid().toString(QUuid::WithoutBraces); _state = QStringLiteral("preparing"); _reason.clear();
    _expiresAtMs = beforeContext + qint64(context.value(QStringLiteral("remainingLifetimeMs")).toDouble());
    _active = false; _reservationHeld = false; _reservationTransferred = false; _transportDraining = false;
    _pendingTransaction.clear(); _transferredTransaction.clear(); _pendingTransactionEpoch.clear(); _transferredTransactionEpoch.clear();
    _transferContext.clear(); _grant.clear(); _authentication.clear(); _sourceHash.clear();
    _sourceHash = hash(sourceContext(_context())); _connectSources(generation);
    if (!digest(_sourceHash) || !_gate(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("ground-startup-pre-consumption-source-denied")); return denied(QStringLiteral("ground-startup-pre-consumption-source-denied"));
    }
    const QString startupId = _startupId, epoch = _authorityEpoch, sourceHash = _sourceHash;
    const QVariantMap grant = approval->consumeApproval(actualSources, approvalReceipt, startupId);
    if (!self) return denied(QStringLiteral("ground-startup-owner-destroyed-at-consumption"));
    if (!_same(generation) || grant.size() != 14 || grant.value(QStringLiteral("consumerId")).toString() != startupId) {
        _invalidate(generation, QStringLiteral("ground-startup-conditional-consumption-denied")); return denied(QStringLiteral("ground-startup-conditional-consumption-denied"));
    }
    _grant = grant;
    if (!_gate(generation, false) || !self) {
        if (self) _invalidate(generation, QStringLiteral("ground-startup-conditional-source-changed")); return denied(QStringLiteral("ground-startup-conditional-source-changed"));
    }
    const qint64 beforeTransfer = _clock.elapsed(); const qint64 remaining = qMin<qint64>(MaximumMs, _expiresAtMs - beforeTransfer);
    if (remaining < 1) { _invalidate(generation, QStringLiteral("original-read-start-deadline-expired-before-transfer")); return denied(QStringLiteral("original-read-start-deadline-expired-before-transfer")); }
    _transferInProgress = true;
    const QVariantMap moved = approval->transferToGroundStartup(self.data(), grant, startupId, sourceHash, remaining);
    if (!self) return denied(QStringLiteral("ground-startup-owner-destroyed-during-transfer"));
    _transferInProgress = false;
    const QVariantMap authentication = moved.value(QStringLiteral("authenticationReceipt")).toMap(); double actualRemaining = 0;
    if (!_same(generation) || moved.size() != 10 || !yes(moved.value(QStringLiteral("ok"))) || authentication.size() != 28
        || moved.value(QStringLiteral("startupId")).toString() != startupId || moved.value(QStringLiteral("authorityEpoch")).toString() != epoch
        || moved.value(QStringLiteral("sourceHash")).toString() != sourceHash || !integer(moved.value(QStringLiteral("remainingMs")), 1, remaining, &actualRemaining)
        || !no(moved.value(QStringLiteral("startupPermitGranted"))) || !no(moved.value(QStringLiteral("executable")))
        || !no(moved.value(QStringLiteral("flightCommandReleased"))) || !no(moved.value(QStringLiteral("previousAuthorizationRestored")))) {
        _invalidate(generation, QStringLiteral("ground-startup-once-native-transfer-not-current")); return denied(QStringLiteral("ground-startup-once-native-transfer-not-current"));
    }
    _authentication = authentication; _expiresAtMs = qMin(_expiresAtMs, beforeTransfer + qint64(actualRemaining));
    if (!_gate(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("new-ground-startup-owned-source-denied-after-move")); return denied(QStringLiteral("new-ground-startup-owned-source-denied-after-move"));
    }
    if (!_same(generation)) return denied(QStringLiteral("current-episode-required-before-ground-reservation"));
    const bool reserved = swarm->reserveMissionDeployment(startupId, vehicleIds);
    if (!self || !_same(generation) || !reserved) {
        if (reserved && swarm) swarm->releaseMissionDeployment(startupId);
        if (self) _invalidate(generation, QStringLiteral("whole-group-new-authority-reservation-denied")); return denied(QStringLiteral("whole-group-new-authority-reservation-denied"));
    }
    _reservationHeld = true; _active = true; _state = QStringLiteral("accepted");
    if (!_gate(generation, true) || !self) {
        if (self) _invalidate(generation, QStringLiteral("new-ground-startup-reserved-current-source-denied")); return denied(QStringLiteral("new-ground-startup-reserved-current-source-denied"));
    }
    _timer.start(); emit changed();
    if (!self || !_same(generation) || !_gate(generation, true)) {
        if (self) _invalidate(generation, QStringLiteral("new-ground-startup-basis-revoked-at-publication")); return denied(QStringLiteral("new-ground-startup-basis-revoked-at-publication"));
    }
    return current();
}
bool FleetMissionGroundStartupAuthority::revokeAuthority(const QString& suppliedId)
{
    const QString startupId = suppliedId;
    if (thread() != QThread::currentThread() || startupId != _startupId) return false;
    if (_state == QStringLiteral("transferred")) {
        if (_busy || _checking || _transferInProgress) return false;
        _busy = true; FlagGuard forwarding{QPointer<QObject>(this), &_busy};
        const QPointer<FleetMissionGroundStartupAuthority> self(this);
        const QPointer<FleetMissionStartupTransaction> transaction(_transferredTransaction);
        const quint64 generation = _generation; const QString epoch = _transferredTransactionEpoch, sourceHash = _sourceHash;
        const auto same = [&]() {
            return self && _generation == generation && _state == QStringLiteral("transferred") && _startupId == startupId
                && _sourceHash == sourceHash && _transferredTransactionEpoch == epoch && _transferredTransaction == transaction
                && _reservationTransferred && !_active && !_reservationHeld && _authentication.isEmpty();
        };
        const auto exact = [&](const QVariantMap& status) {
            return same() && transaction && transaction->thread() == thread() && transaction->transactionEpoch() == epoch
                && status.value(QStringLiteral("startupId")).type() == QVariant::String && status.value(QStringLiteral("startupId")).toString() == startupId
                && status.value(QStringLiteral("transactionEpoch")).type() == QVariant::String && status.value(QStringLiteral("transactionEpoch")).toString() == epoch
                && status.value(QStringLiteral("sourceHash")).type() == QVariant::String && status.value(QStringLiteral("sourceHash")).toString() == sourceHash;
        };
        const auto terminal = [&](const QVariantMap& status) {
            const QString state = status.value(QStringLiteral("state")).toString();
            return exact(status) && status.value(QStringLiteral("state")).type() == QVariant::String
                && (state == QStringLiteral("cancelled") || state == QStringLiteral("failed") || state == QStringLiteral("expired")
                    || state == QStringLiteral("stale") || state == QStringLiteral("started-observed"))
                && no(status.value(QStringLiteral("active"))) && no(status.value(QStringLiteral("reservationHeld")))
                && no(status.value(QStringLiteral("transportDraining")));
        };
        bool retired = !transaction;
        if (transaction) {
            const QVariantMap before = transaction->current();
            if (!same()) return false;
            if (exact(before)) {
                if (terminal(before)) retired = true;
                else {
                    transaction->revokeStartup(startupId); // Explicit old-ledger revoke propagates through exact native ownership.
                    if (!same()) return false;
                    retired = !transaction || terminal(transaction->current());
                }
            }
        }
        if (!same()) return false;
        if (!retired) { _reason = QStringLiteral("new-ground-transaction-cancellation-or-drain-required"); emit changed(); return false; }
        _state = QStringLiteral("revoked"); _reason = QStringLiteral("transferred-new-ground-transaction-retired");
        _reservationTransferred = false; _transferredTransaction.clear(); _transferredTransactionEpoch.clear();
        // Only actual Transaction revokes/releases its scope after drain.
        // Retiring this chain never clears a replacement proof or reservation.
        emit changed(); return true;
    }
    if (_state != QStringLiteral("preparing") && _state != QStringLiteral("accepted")) return false;
    _invalidate(_generation, QStringLiteral("explicit-new-ground-startup-basis-revoked"), QStringLiteral("revoked")); return true;
}
QVariantMap FleetMissionGroundStartupAuthority::authorityContext(const QString& suppliedId)
{
    const QString startupId = suppliedId;
    if (thread() != QThread::currentThread() || _busy || _checking || !_active || !_reservationHeld || _state != QStringLiteral("accepted") || startupId != _startupId)
        return denied(QStringLiteral("current-new-ground-startup-basis-unavailable"));
    _busy = true; FlagGuard querying{QPointer<QObject>(this), &_busy}; const QPointer<FleetMissionGroundStartupAuthority> self(this);
    if (!_gate(_generation, true) || !self || hash(sourceContext(_context())) != _sourceHash) return denied(QStringLiteral("new-ground-startup-context-source-denied"));
    return _context();
}
bool FleetMissionGroundStartupAuthority::authorityContextCurrent(const QString& startupId, const QVariantMap& supplied)
{
    const QVariantMap expected = supplied;
    if (expected.size() != 27 || !yes(expected.value(QStringLiteral("ok"))) || !digest(expected.value(QStringLiteral("sourceHash")))
        || !integer(expected.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumMs)) return false;
    const QPointer<FleetMissionGroundStartupAuthority> self(this); const QVariantMap actual = authorityContext(startupId);
    return self && yes(actual.value(QStringLiteral("ok"))) && sameMap(stable(expected), stable(actual));
}
std::shared_ptr<FleetMissionStartupClearanceWitness>
FleetMissionGroundStartupAuthority::startupClearanceWitness(const QString& suppliedStartupId)
{
    const QString startupId = suppliedStartupId;
    if (thread() != QThread::currentThread() || _busy || _checking || _transferInProgress || !_active
        || !_reservationHeld || _reservationTransferred || _state != QStringLiteral("accepted")
        || startupId != _startupId || !_same(_generation)) return {};
    _busy = true;
    FlagGuard reading{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    const quint64 generation = _generation;
    const auto witness = _clearanceWitness;
    const bool required = _clearanceRequired;
    if (!_gate(generation, true) || !self || !_same(generation) || !_active || !_reservationHeld || _reservationTransferred
        || _state != QStringLiteral("accepted") || startupId != _startupId
        || _clearanceRequired != required || _clearanceWitness != witness
        || !clearanceBindingMatches(required, witness, _reviewContext.value(QStringLiteral("reviewArtifact")).toMap())
        || (required && !witness->memoryCurrent())) return {};
    return witness;
}

QVariantMap FleetMissionGroundStartupAuthority::actualSourcesForStartupTransaction(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    if (thread() != QThread::currentThread() || _busy || _checking || !_active || !_reservationHeld || _reservationTransferred
        || _state != QStringLiteral("accepted") || startupId != _startupId || !_same(_generation)) return {};
    _busy = true; FlagGuard reading{QPointer<QObject>(this), &_busy};
    if (!_gate(_generation, true) || !self) return {};
    const auto actual = [](QObject* pointer) { return QVariant::fromValue(pointer); };
    return {{QStringLiteral("session"), actual(_session.data())}, {QStringLiteral("roles"), actual(_roles.data())},
        {QStringLiteral("store"), actual(_store.data())}, {QStringLiteral("authority"), actual(self.data())},
        {QStringLiteral("deployment"), actual(_deployment.data())}, {QStringLiteral("swarm"), actual(_swarm.data())},
        {QStringLiteral("health"), actual(_health.data())}, {QStringLiteral("authorizationOwner"), actual(_authorization.data())}};
}
FleetFirmwareAdapterCatalog* FleetMissionGroundStartupAuthority::nativeFirmwareCatalog(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    if (thread() != QThread::currentThread() || _busy || _checking || !_active || !_reservationHeld || _reservationTransferred
        || _state != QStringLiteral("accepted") || startupId != _startupId || !_same(_generation)) return nullptr;
    _busy = true; FlagGuard reading{QPointer<QObject>(this), &_busy};
    if (!_gate(_generation, true) || !self) return nullptr;
    return _catalog.data();
}
FleetMissionReviewCoordinator* FleetMissionGroundStartupAuthority::nativeReviewCoordinator(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    return nativeFirmwareCatalog(startupId) && self ? _coordinator.data() : nullptr;
}
FleetRegistry* FleetMissionGroundStartupAuthority::nativeRegistry(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    return nativeFirmwareCatalog(startupId) && self ? _registry.data() : nullptr;
}
FaultToleranceManager* FleetMissionGroundStartupAuthority::nativeFaultPolicy(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    return nativeFirmwareCatalog(startupId) && self ? _faults.data() : nullptr;
}
FleetMissionArmedGroundReview* FleetMissionGroundStartupAuthority::nativeGroundReview(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    return nativeFirmwareCatalog(startupId) && self ? _review.data() : nullptr;
}
FleetMissionStartupTransaction* FleetMissionGroundStartupAuthority::nativePreviousStartup(const QString& startupId)
{
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    return nativeFirmwareCatalog(startupId) && self ? _previous.data() : nullptr;
}
QVariantMap FleetMissionGroundStartupAuthority::pendingStartupTransactionContext(QObject* actualTransaction,
    const QString& startupId, const QString& transactionEpoch, const QString& sourceHash) const
{
    // PURE actual ownership snapshot only while this native move is pending.
    // Session separately checks both identities and original native deadlines;
    // Transaction performs its full source gate AFTER the wrapper commits.
    if (thread() != QThread::currentThread() || !_busy || !_transferInProgress || _checking || !_active || !_reservationHeld
        || _reservationTransferred || _state != QStringLiteral("accepted") || !_same(_generation)
        || !_pendingTransaction || _pendingTransaction.data() != actualTransaction || _pendingTransaction->thread() != thread()
        || !outboundGroup(this, {_pendingTransaction.data()})
        || _pendingTransaction->transactionEpoch() != transactionEpoch || _pendingTransactionEpoch != transactionEpoch
        || startupId != _startupId || sourceHash != _sourceHash || !uuid(startupId) || !uuid(transactionEpoch) || !digest(sourceHash)
        || _authentication.size() != 28 || _transferContext.size() != 27
        || !sameMap(stable(_transferContext), stable(_context())) || hash(sourceContext(_context())) != sourceHash) return {};
    return _transferContext;
}
QVariantMap FleetMissionGroundStartupAuthority::transferToStartupTransaction(QObject* actualTransaction,
    const QVariantMap& suppliedContext, const QString& suppliedStartupId, const QString& suppliedSourceHash,
    qint64 maximumRemainingMs)
{
    const QVariantMap context = suppliedContext; const QString startupId = suppliedStartupId, sourceHash = suppliedSourceHash;
    const QPointer<FleetMissionGroundStartupAuthority> self(this);
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(actualTransaction));
    if (thread() != QThread::currentThread() || _busy || _checking || _transferInProgress || _state != QStringLiteral("accepted")
        || !_active || !_reservationHeld || _reservationTransferred || _pendingTransaction || !transaction || transaction == _previous
        || transaction->thread() != thread() || !outboundGroup(this, {transaction.data()})
        || startupId != _startupId || sourceHash != _sourceHash || !uuid(startupId) || !digest(sourceHash)
        || maximumRemainingMs < 1 || maximumRemainingMs > MaximumMs) return denied(QStringLiteral("actual-current-new-ground-authority-and-distinct-transaction-required"));
    const QString transactionEpoch = transaction->transactionEpoch(), epoch = _authorityEpoch;
    if (!uuid(transactionEpoch)) return denied(QStringLiteral("actual-ground-startup-transaction-epoch-required"));
    const quint64 generation = _generation; const QPointer<FleetOperatorSession> session(_session); const QPointer<SwarmController> swarm(_swarm);
    const QVariantMap authentication = _authentication; const QVariantList vehicleIds = _vehicleIds;
    const auto clearance = _clearanceWitness;
    const bool clearanceRequired = _clearanceRequired;
    const qint64 entered = _clock.elapsed(), deadline = qMin(_expiresAtMs, entered + maximumRemainingMs);
    if (!authorityContextCurrent(startupId, context) || !self || !session || !swarm || !transaction)
        return denied(QStringLiteral("new-ground-authority-context-not-current-before-transaction"));
    _busy = true; FlagGuard transferring{QPointer<QObject>(this), &_busy};
    const auto same = [&]() {
        return self && session && swarm && transaction && _same(generation)
            && outboundGroup(self.data(), {session.data(), transaction.data()}) && _state == QStringLiteral("accepted")
            && _session == session && _swarm == swarm && _startupId == startupId && _sourceHash == sourceHash && _authorityEpoch == epoch
            && _authentication == authentication && _vehicleIds == vehicleIds && _active && _reservationHeld && !_reservationTransferred
            && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired
            && transaction->thread() == thread() && transaction->transactionEpoch() == transactionEpoch && _clock.elapsed() < deadline;
    };
    if (!same() || !_gate(generation, true) || !same()
        || !transaction->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), context) || !same()
        || !_gate(generation, true) || !same()) return denied(QStringLiteral("new-ground-transaction-pre-transfer-full-source-denied"));
    _pendingTransaction = transaction; _pendingTransactionEpoch = transactionEpoch; _transferContext = _context();
    _transferInProgress = true; FlagGuard transferringContext{QPointer<QObject>(this), &_transferInProgress};
    const qint64 beforeNative = _clock.elapsed(), remainingBefore = deadline - beforeNative;
    if (remainingBefore < 1 || pendingStartupTransactionContext(transaction.data(), startupId, transactionEpoch, sourceHash).size() != 27
        || (clearanceRequired && (!clearance || !clearance->memoryCurrent()))) {
        _invalidate(generation, QStringLiteral("original-new-ground-budget-or-actual-pending-context-denied"), QStringLiteral("failed"));
        return denied(QStringLiteral("original-new-ground-budget-or-actual-pending-context-denied"));
    }
    const QVariantMap movedResult = session->transferGroundStartupAuthenticationToTransaction(authentication, self.data(), transaction.data(),
        startupId, epoch, transactionEpoch, sourceHash, int(remainingBefore));
    const bool moved = yes(movedResult.value(QStringLiteral("ok")));
    const QVariantMap receipt = movedResult.value(QStringLiteral("authenticationReceipt")).toMap(); double reportedRemaining = 0;
    const auto revokeMoved = [&]() {
        if (moved && session && transaction) session->revokeGroundStartupTransactionAuthentication(transaction.data(), startupId, transactionEpoch);
    };
    if (!same() || !moved || movedResult.size() != 6 || movedResult.value(QStringLiteral("authenticationReceipt")).type() != QVariant::Map
        || receipt.size() != 30 || !integer(movedResult.value(QStringLiteral("remainingMs")), 1, remainingBefore, &reportedRemaining)
        || !no(movedResult.value(QStringLiteral("startupPermitGranted"))) || !no(movedResult.value(QStringLiteral("executable")))
        || !no(movedResult.value(QStringLiteral("flightCommandReleased")))
        || (clearanceRequired && (!clearance || !clearance->sourceCurrent(same) || !same() || !clearance->memoryCurrent()))) {
        revokeMoved();
        if (self && _generation == generation && _state == QStringLiteral("accepted"))
            _invalidate(generation, QStringLiteral("new-ground-transaction-authentication-move-not-committed"), QStringLiteral("failed"));
        return denied(QStringLiteral("new-ground-transaction-authentication-move-not-committed"));
    }
    // The same exact startup UUID retains its whole-group Swarm reservation.
    // Retire old 28/pre-ARM watchers BEFORE checking independently owned 30.
    _state = QStringLiteral("transferred"); _reason.clear(); _active = false; _timer.stop(); _disconnectSources();
    _authentication.clear(); _grant.clear(); _reservationHeld = false; _reservationTransferred = true;
    _transferredTransaction = transaction; _transferredTransactionEpoch = transactionEpoch;
    _pendingTransaction.clear(); _pendingTransactionEpoch.clear(); _transferContext.clear();
    // Reported remaining is AFTER Session's queries, NOT old durationMs.
    // Backdating it to call entry is deliberately conservative, never a renewal.
    _expiresAtMs = qMin(deadline, beforeNative + qint64(reportedRemaining));
    const auto transferred = [&]() {
        return self && session && swarm && transaction && outboundGroup(self.data(), {session.data(), transaction.data()})
            && _generation == generation && _state == QStringLiteral("transferred")
            && _session == session && _swarm == swarm && _startupId == startupId && _sourceHash == sourceHash && _authorityEpoch == epoch
            && _transferredTransaction == transaction && _transferredTransactionEpoch == transactionEpoch && _vehicleIds == vehicleIds
            && _clearanceWitness == clearance && _clearanceRequired == clearanceRequired
            && !_active && !_reservationHeld && _reservationTransferred && _authentication.isEmpty()
            && transaction->thread() == thread() && transaction->transactionEpoch() == transactionEpoch && _clock.elapsed() < _expiresAtMs;
    };
    const bool clearanceCurrent = transferred() && (!clearanceRequired
        || (clearance && clearance->sourceCurrent(transferred) && transferred()));
    const bool nativeCurrent = clearanceCurrent && session->groundStartupTransactionAuthenticationCurrent(receipt, transaction.data(), startupId, transactionEpoch, sourceHash);
    const bool ownerCurrent = transferred() && nativeCurrent && swarm->missionDeploymentOwns(startupId, vehicleIds)
        && transferred() && transaction->authenticationTransferContextCurrent(startupId, sourceHash, self.data(), context) && transferred()
        && (!clearanceRequired || clearance->memoryCurrent());
    const qint64 remaining = self ? _expiresAtMs - _clock.elapsed() : 0;
    if (!ownerCurrent || !transferred() || remaining < 1) {
        revokeMoved();
        // No Transaction command was possible before its wrapper commits.
        // Release only captured exact old UUID, never replacement scope fields.
        if (swarm && swarm->missionDeploymentOwns(startupId, vehicleIds)) swarm->releaseMissionDeployment(startupId);
        if (self && _generation == generation && _startupId == startupId && _state == QStringLiteral("transferred")) {
            _reservationTransferred = false; _state = QStringLiteral("failed"); _reason = QStringLiteral("new-ground-transaction-post-transfer-context-denied");
            _transferredTransaction.clear(); _transferredTransactionEpoch.clear();
            QTimer::singleShot(0, this, [self, generation, startupId]() {
                if (self && self->_generation == generation && self->_startupId == startupId && self->_state == QStringLiteral("failed")) emit self->changed();
            });
        }
        return denied(QStringLiteral("new-ground-transaction-post-transfer-context-denied"));
    }
    QTimer::singleShot(0, this, [self, generation, startupId, transactionEpoch]() {
        if (self && self->_generation == generation && self->_startupId == startupId && self->_state == QStringLiteral("transferred")
            && self->_transferredTransactionEpoch == transactionEpoch) emit self->changed();
    });
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), receipt}, {QStringLiteral("startupId"), startupId},
        {QStringLiteral("transactionEpoch"), transactionEpoch}, {QStringLiteral("sourceHash"), sourceHash}, {QStringLiteral("remainingMs"), double(remaining)},
        {QStringLiteral("reservationTransferred"), true}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
Vehicle* FleetMissionGroundStartupAuthority::authorityVehicle(const QString& startupId, int vehicleId) const
{
    if (thread() != QThread::currentThread() || !_active || !_reservationHeld || _state != QStringLiteral("accepted") || startupId != _startupId || !_same(_generation)) return nullptr;
    const QPointer<FleetMissionGroundStartupAuthority> self(const_cast<FleetMissionGroundStartupAuthority*>(this));
    const quint64 generation = _generation; const bool current = _targetsCurrent(generation);
    if (!self || !current || !_same(generation)) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.vehicle.data();
    return nullptr;
}
MissionManager* FleetMissionGroundStartupAuthority::authorityManager(const QString& startupId, int vehicleId) const
{
    const QPointer<FleetMissionGroundStartupAuthority> self(const_cast<FleetMissionGroundStartupAuthority*>(this));
    const QPointer<Vehicle> vehicle(authorityVehicle(startupId, vehicleId)); if (!self || !vehicle) return nullptr;
    for (const Target& target : _targets) if (target.vehicle == vehicle) return target.mission.data();
    return nullptr;
}
LinkInterface* FleetMissionGroundStartupAuthority::authorityPrimaryLink(const QString& startupId, int vehicleId) const
{
    const QPointer<FleetMissionGroundStartupAuthority> self(const_cast<FleetMissionGroundStartupAuthority*>(this));
    const QPointer<Vehicle> vehicle(authorityVehicle(startupId, vehicleId)); if (!self || !vehicle) return nullptr;
    for (const Target& target : _targets) if (target.vehicle == vehicle) return target.primary.data();
    return nullptr;
}

#include "FleetMissionExecutionObservation.h"

#include "FleetMissionProgressObservation.h"
#include "FleetMissionStartupTransaction.h"
#include "LinkInterface.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "px4_custom_mode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <cstring>
#include <limits>

namespace {
bool yes(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool();
}

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool digest(const QString& value)
{
    if (value.size() != 64) return false;
    for (QChar ch : value) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool integer(const QVariant& value, int minimum, int maximum, int* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}

bool queueContextValid(const VehicleMissionExecutionObserver::QueueContext& context)
{
    bool ok = false;
    const quint64 operation = context.operationId.toULongLong(&ok, 10);
    return uuid(context.sourceEpoch) && uuid(context.startupId) && uuid(context.stepId)
        && digest(context.wirePayloadHash) && ok && operation > 0 && QString::number(operation) == context.operationId
        && context.vehicleId >= 1 && context.vehicleId <= 255 && context.componentId >= 1 && context.componentId <= 255;
}

bool sameQueue(const VehicleMissionExecutionObserver::QueueContext& first,
               const VehicleMissionExecutionObserver::QueueContext& second)
{
    return first.sourceEpoch == second.sourceEpoch && first.startupId == second.startupId
        && first.stepId == second.stepId && first.operationId == second.operationId
        && first.wirePayloadHash == second.wirePayloadHash && first.vehicleId == second.vehicleId
        && first.componentId == second.componentId;
}

bool coordinates(int latitude, int longitude)
{
    return latitude >= -900000000 && latitude <= 900000000
        && longitude >= -1800000000 && longitude <= 1800000000;
}

QString periodicChannel(quint32 id)
{
    switch (id) {
    case MAVLINK_MSG_ID_HEARTBEAT: return QStringLiteral("heartbeat");
    case MAVLINK_MSG_ID_MISSION_CURRENT: return QStringLiteral("missionCurrent");
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE: return QStringLiteral("landedState");
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: return QStringLiteral("globalPosition");
    default: return {};
    }
}

bool frameLength(const mavlink_message_t& message, bool* relevant)
{
    int maximum = 0, minimum = 0;
    *relevant = true;
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: maximum = MAVLINK_MSG_ID_HEARTBEAT_LEN; minimum = MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN; break;
    case MAVLINK_MSG_ID_MISSION_CURRENT: maximum = MAVLINK_MSG_ID_MISSION_CURRENT_LEN; minimum = MAVLINK_MSG_ID_MISSION_CURRENT_MIN_LEN; break;
    case MAVLINK_MSG_ID_MISSION_ITEM_REACHED: maximum = MAVLINK_MSG_ID_MISSION_ITEM_REACHED_LEN; minimum = MAVLINK_MSG_ID_MISSION_ITEM_REACHED_MIN_LEN; break;
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE: maximum = MAVLINK_MSG_ID_EXTENDED_SYS_STATE_LEN; minimum = MAVLINK_MSG_ID_EXTENDED_SYS_STATE_MIN_LEN; break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: maximum = MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN; minimum = MAVLINK_MSG_ID_GLOBAL_POSITION_INT_MIN_LEN; break;
    case MAVLINK_MSG_ID_HOME_POSITION: maximum = MAVLINK_MSG_ID_HOME_POSITION_LEN; minimum = MAVLINK_MSG_ID_HOME_POSITION_MIN_LEN; break;
    default: *relevant = false; return true;
    }
    return (message.magic == MAVLINK_STX || message.magic == MAVLINK_STX_MAVLINK1)
        && message.len >= 1 && message.len <= maximum
        && (message.magic != MAVLINK_STX_MAVLINK1 || message.len == minimum);
}

bool knownSourceRetirementCode(const QString& code)
{
    // Only fixed codes emitted by Vehicle's actual execution-observer lane.
    // Do not publish arbitrary future/caller text as a native-known reason.
    const QStringList known{QStringLiteral("execution-vehicle-closing"), QStringLiteral("execution-vehicle-removing"),
        QStringLiteral("execution-primary-changed"), QStringLiteral("execution-communication-lost"),
        QStringLiteral("execution-source-unavailable"), QStringLiteral("execution-links-removed"),
        QStringLiteral("execution-vehicle-removed"), QStringLiteral("execution-vehicle-model-changed"),
        QStringLiteral("execution-version-source-changed"), QStringLiteral("execution-link-disconnected"),
        QStringLiteral("execution-link-destroyed"), QStringLiteral("execution-protocol-destroyed"),
        QStringLiteral("execution-observer-destroyed"), QStringLiteral("execution-native-source-invalid"),
        QStringLiteral("execution-native-sequence-exhausted-or-reentered"), QStringLiteral("execution-native-clock-invalid"),
        QStringLiteral("execution-version-source-invalidated"), QStringLiteral("execution-before-queue-rejected")};
    return known.contains(code);
}

QVariantMap claims()
{
    return {{QStringLiteral("readOnly"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("observationOnly"), true}, {QStringLiteral("localRunAttributionOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("authenticatedAircraft"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("crcVerified"), false},
        {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("executionAttributionVerified"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("fullTelemetryCaptured"), false},
        {QStringLiteral("currentValuesFreshnessVerified"), false},
        {QStringLiteral("localReferenceValidityCertified"), false}, {QStringLiteral("tailObservationComplete"), false}};
}
} // namespace

FleetMissionExecutionObservation::FleetMissionExecutionObservation(QObject* parent)
    : VehicleMissionExecutionObserver(parent), _runId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _application = qgcApp();
    if (_application) _toolbox = _application->toolbox();
    if (_toolbox) _vehicles = _toolbox->multiVehicleManager();
    _clock.start(); _timer.setParent(this); _timer.setInterval(250);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionExecutionObservation::poll);
}

FleetMissionExecutionObservation::~FleetMissionExecutionObservation()
{
    const QPointer<FleetMissionExecutionObservation> self(this);
    noteMutation();
    _ownerDestroying = true;
    _retired = true; _timer.stop(); detachBindings();
    if (!self) return;
    releaseOwnReservation();
    if (!self) return;
    updateTaskModel(); // Preserve final local facts before QObject::destroyed.
    if (!self) return;
    const QList<QMetaObject::Connection> connections = _connections;
    _connections.clear();
    for (const QMetaObject::Connection& connection : connections) {
        disconnect(connection);
        if (!self) return;
    }
}

FleetMissionExecutionObservation::ReadContext FleetMissionExecutionObservation::readContext() const
{
    ReadContext result;
    result.serial = _mutationSerial; result.exhausted = _mutationSerialExhausted;
    result.startup = _startup; result.swarm = _swarm; result.orchestrator = _taskOrchestrator;
    result.hadStartup = bool(_startup); result.hadSwarm = bool(_swarm); result.hadOrchestrator = bool(_taskOrchestrator);
    result.runId = _runId; result.startupId = _startupId; result.startupEpoch = _startupEpoch;
    result.deploymentId = _deploymentId; result.taskModelId = _taskModelId;
    result.startupGeneration = _startupGeneration; result.binding = _binding; result.vehicleIds = _vehicleIds;
    result.prepared = _prepared; result.retired = _retired; result.startupRetired = _startupRetired;
    result.ownerDestroying = _ownerDestroying; result.targetCount = _targets.size();
    return result;
}

bool FleetMissionExecutionObservation::readContextCurrent(const ReadContext& context) const
{
    return QThread::currentThread() == thread() && _mutationSerial == context.serial
        && _mutationSerialExhausted == context.exhausted
        && (!context.hadStartup || context.startup) && (!context.hadSwarm || context.swarm)
        && (!context.hadOrchestrator || context.orchestrator)
        && _startup == context.startup && _swarm == context.swarm && _taskOrchestrator == context.orchestrator
        && _runId == context.runId && _startupId == context.startupId && _startupEpoch == context.startupEpoch
        && _deploymentId == context.deploymentId && _taskModelId == context.taskModelId
        && _startupGeneration == context.startupGeneration && _binding == context.binding && _vehicleIds == context.vehicleIds
        && _prepared == context.prepared && _retired == context.retired && _startupRetired == context.startupRetired
        && _ownerDestroying == context.ownerDestroying && _targets.size() == context.targetCount;
}

void FleetMissionExecutionObservation::noteMutation()
{
    if (_mutationSerialExhausted) return;
    if (_mutationSerial < std::numeric_limits<quint64>::max()) { ++_mutationSerial; return; }
    _mutationSerialExhausted = true;
    // Exhaustion ends live probes/advancement, never already captured write
    // facts or historical candidates, and never releases a reservation.
    for (Target& target : _targets) {
        target.continuityUnknown = true; target.tracking = false;
        if (target.reason.isEmpty()) target.reason = QStringLiteral("execution-observer-mutation-serial-exhausted");
    }
}

int FleetMissionExecutionObservation::targetIndex(const Target& original) const
{
    for (int index = 0; index < _targets.size(); ++index) {
        const Target& target = _targets.at(index);
        if (target.vehicleId == original.vehicleId && target.componentId == original.componentId
            && target.vehicle == original.vehicle && target.mission == original.mission && target.links == original.links
            && target.primary == original.primary && target.modeStepId == original.modeStepId
            && target.itemCount == original.itemCount && target.payloadHash == original.payloadHash
            && sameQueue(target.queue, original.queue)) return index;
    }
    return -1;
}

bool FleetMissionExecutionObservation::taskModelEvidence(SwarmMissionOrchestrator* actualOrchestrator,
    TaskModelEvidence* evidence) const
{
    if (!evidence || !_prepared || QThread::currentThread() != thread() || !actualOrchestrator
        || actualOrchestrator != _taskOrchestrator.data() || actualOrchestrator->thread() != thread()
        || !uuid(_runId) || !uuid(_startupId) || !uuid(_startupEpoch) || !uuid(_deploymentId)
        || _startupGeneration == 0 || _targets.isEmpty() || _targets.size() > 32) return false;
    const QPointer<FleetMissionExecutionObservation> self(const_cast<FleetMissionExecutionObservation*>(this));
    const QPointer<SwarmMissionOrchestrator> orchestrator(actualOrchestrator);
    const ReadContext context = readContext();
    const QList<Target> targets = _targets;
    *evidence = TaskModelEvidence();
    TaskModelEvidence result;
    result.identity = {{QStringLiteral("runId"), _runId}, {QStringLiteral("startupId"), _startupId},
        {QStringLiteral("transactionEpoch"), _startupEpoch}, {QStringLiteral("transactionGeneration"), QString::number(_startupGeneration)},
        {QStringLiteral("deploymentId"), _deploymentId}, {QStringLiteral("binding"), _binding}};
    result.targetVehicleIds = _vehicleIds;
    result.modeCommitted = false;
    for (const Target& target : targets) result.modeCommitted = result.modeCommitted || target.queueCommitted;
    result.busy = _startupRetired && result.modeCommitted && !_retired;
    result.closed = _retired;
    QVariantList rows;
    int committed = 0, active = 0;
    for (const Target& target : targets) {
        bool currentSource = false;
        if (!context.retired && !context.exhausted && target.tracking && target.sourceActive) {
            const bool native = nativeTargetCurrent(target);
            if (!self || !orchestrator || !self->readContextCurrent(context)) return false;
            if (native) {
                const bool binding = nativeBindingCurrent(target);
                if (!self || !orchestrator || !self->readContextCurrent(context)) return false;
                currentSource = binding;
            }
        }
        committed += target.queueCommitted ? 1 : 0; active += currentSource ? 1 : 0;
        rows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("componentId"), target.componentId},
            {QStringLiteral("modeStepId"), target.modeStepId}, {QStringLiteral("sourceEpoch"), target.queue.sourceEpoch},
            {QStringLiteral("modeQueueOperationId"), target.queue.operationId}, {QStringLiteral("payloadContentHash"), target.payloadHash},
            {QStringLiteral("itemCount"), target.itemCount}, {QStringLiteral("modeQueueBegan"), target.queueBegan},
            {QStringLiteral("modeQueueCommitted"), target.queueCommitted}, {QStringLiteral("sourceCurrent"), currentSource},
            {QStringLiteral("continuityUnknown"), target.continuityUnknown}, {QStringLiteral("reason"), target.reason},
            {QStringLiteral("currentSequence"), target.currentSequence >= 0 ? QVariant(target.currentSequence) : QVariant()},
            {QStringLiteral("reachedSequence"), target.reachedSequence >= 0 ? QVariant(target.reachedSequence) : QVariant()},
            {QStringLiteral("distinctCurrentSequencesObserved"), target.currentSeen.count(true)},
            {QStringLiteral("distinctReachedSequencesObserved"), target.reachedSeen.count(true)},
            {QStringLiteral("duplicateReachedObservations"), QString::number(target.duplicateReached)},
            {QStringLiteral("currentSequenceRegressions"), QString::number(target.currentRegressions)},
            {QStringLiteral("currentSequenceJumps"), QString::number(target.currentJumps)},
            {QStringLiteral("missionModeDepartures"), QString::number(target.modeDepartures)},
            {QStringLiteral("homePayloadChanges"), QString::number(target.homeChanges)},
            {QStringLiteral("landedAfterTerminalObserved"), receiptObserved(target, target.landedAfterTerminalReceipt)},
            {QStringLiteral("observedOutcome"), targetOutcome(target, currentSource, context.retired)},
            {QStringLiteral("currentValuesFreshnessVerified"), false}, {QStringLiteral("unknownTail"), true}});
    }
    result.observation = {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("prepared"), true},
        {QStringLiteral("observerRetired"), _retired}, {QStringLiteral("startupRetired"), _startupRetired},
        {QStringLiteral("observationRetirementExplicitlyRequested"), _explicitRetirementRequested},
        {QStringLiteral("observerOwnerDestroying"), _ownerDestroying},
        {QStringLiteral("unissuedModePreparationClosed"), _unissuedPreparationClosed},
        {QStringLiteral("startupTerminalState"), _startupState}, {QStringLiteral("startupTerminalReason"), _startupReason},
        {QStringLiteral("modeCommittedVehicleCount"), committed}, {QStringLiteral("currentSourceVehicleCount"), active},
        {QStringLiteral("reservationWasAdopted"), _reservationAdopted},
        {QStringLiteral("reservationReleasedByObserver"), _reservationReleased},
        {QStringLiteral("progressMeasure"), QStringLiteral("observed-wire-sequences-not-success-distance-time-or-area")},
        {QStringLiteral("remoteMissionInstanceVerified"), false}, {QStringLiteral("missionCompleteVerified"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("unknownTail"), true},
        {QStringLiteral("observedOutcome"), outcomeSummary(targets, context.retired)}, {QStringLiteral("vehicles"), rows}};
    if (!self || !orchestrator || !self->readContextCurrent(context)) return false;
    *evidence = result;
    return true;
}

bool FleetMissionExecutionObservation::updateTaskModel()
{
    if (!_prepared || _taskModelId.isEmpty() || QThread::currentThread() != thread()) return false;
    const QPointer<FleetMissionExecutionObservation> self(this);
    const QPointer<SwarmMissionOrchestrator> orchestrator(_taskOrchestrator);
    const ReadContext context = readContext();
    const bool updated = orchestrator && orchestrator->updateExecutionObservation(this, context.taskModelId);
    if (!self || !self->readContextCurrent(context)) return false;
    if (!updated && _taskModelReason != QStringLiteral("native-task-model-source-or-binding-lost-history-retained")) {
        noteMutation();
        _taskModelReason = QStringLiteral("native-task-model-source-or-binding-lost-history-retained");
    }
    return updated && self && self->readContextCurrent(context);
}

bool FleetMissionExecutionObservation::receiptObserved(const Target& target, const ReceiptRef& receipt) const
{
    return target.queueCommitted && uuid(target.queue.sourceEpoch)
        && receipt.sourceSequence > target.commitBaselineSequence
        && receipt.sourceSequence <= target.lastSourceSequence && receipt.receivedMonotonicMs >= 0
        && receipt.receivedMonotonicMs <= target.lastRawReceivedAt;
}

QVariant FleetMissionExecutionObservation::receiptReference(const Target& target, const ReceiptRef& receipt) const
{
    if (!receiptObserved(target, receipt)) return QVariant();
    return QVariantMap{{QStringLiteral("sourceEpoch"), target.queue.sourceEpoch},
        {QStringLiteral("sourceSequence"), QString::number(receipt.sourceSequence)},
        {QStringLiteral("receivedMonotonicMs"), QString::number(receipt.receivedMonotonicMs)}};
}

QVariantMap FleetMissionExecutionObservation::missionModeExitObservation(const Target& target) const
{
    const auto exitValue = [this, &target](const ModeExit& value) -> QVariant {
        if (!receiptObserved(target, value.priorMissionReceipt) || !receiptObserved(target, value.exitReceipt))
            return QVariant();
        return QVariantMap{{QStringLiteral("customMode"), QVariant::fromValue(value.customMode)},
            {QStringLiteral("baseMode"), static_cast<int>(value.baseMode)},
            {QStringLiteral("systemStatus"), static_cast<int>(value.systemStatus)},
            {QStringLiteral("priorMissionReceipt"), receiptReference(target, value.priorMissionReceipt)},
            {QStringLiteral("exitReceipt"), receiptReference(target, value.exitReceipt)},
            {QStringLiteral("continuityUnknownAtExit"), value.continuityUnknownAtExit}};
    };
    const bool observed = target.modeDepartures > 0;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("observed"), observed},
        {QStringLiteral("count"), QString::number(target.modeDepartures)},
        {QStringLiteral("candidate"), observed ? QStringLiteral("mission-mode-exit-observed-not-outcome")
                                               : QStringLiteral("not-observed")},
        {QStringLiteral("first"), observed ? exitValue(target.firstModeExit) : QVariant()},
        {QStringLiteral("last"), observed ? exitValue(target.lastModeExit) : QVariant()},
        {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("remoteOutcomeVerified"), false},
        {QStringLiteral("automaticActionsIssued"), false}};
}

QString FleetMissionExecutionObservation::outcomeCandidate(const Target& target) const
{
    if (receiptObserved(target, target.landedAfterTerminalReceipt))
        return QStringLiteral("landed-after-terminal-observed-not-completion");
    if (receiptObserved(target, target.terminalCurrentReceipt))
        return target.lastCommand == MAV_CMD_NAV_LOITER_UNLIM ? QStringLiteral("indefinite-terminal-hold-observed")
            : QStringLiteral("terminal-item-observed-not-completion");
    return QStringLiteral("no-terminal-candidate-observed");
}

QVariantMap FleetMissionExecutionObservation::targetOutcome(const Target& target, bool sourceCurrent, bool retired) const
{
    const bool reported = receiptObserved(target, target.missionCurrentReceipt);
    const bool noMission = reported && (target.reportedMissionTotal == std::numeric_limits<quint16>::max()
        || target.reportedMissionState == MISSION_STATE_NO_MISSION);
    const bool totalKnown = reported && target.reportedMissionTotal != 0
        && target.reportedMissionTotal != std::numeric_limits<quint16>::max();
    const QString window = retired ? QStringLiteral("observation-retired")
        : !target.queueBegan ? QStringLiteral("awaiting-native-mode-queue")
        : !sourceCurrent ? QStringLiteral("source-ended-or-unknown")
        : !target.queueCommitted ? QStringLiteral("provisional-native-queue-window")
        : QStringLiteral("committed-native-source-bound");
    const QVariantMap reportedMission{{QStringLiteral("observed"), reported},
        {QStringLiteral("sequence"), reported ? QVariant(target.reportedCurrentSequence) : QVariant()},
        {QStringLiteral("total"), reported ? QVariant(target.reportedMissionTotal) : QVariant()},
        {QStringLiteral("missionState"), reported ? QVariant(target.reportedMissionState) : QVariant()},
        {QStringLiteral("missionMode"), reported ? QVariant(target.reportedMissionMode) : QVariant()},
        {QStringLiteral("totalKnown"), totalKnown},
        {QStringLiteral("missionStateKnown"), reported && target.reportedMissionState != MISSION_STATE_UNKNOWN},
        {QStringLiteral("missionModeKnown"), reported && target.reportedMissionMode != 0},
        {QStringLiteral("noMissionReported"), noMission},
        {QStringLiteral("totalMatchesExpected"), totalKnown ? QVariant(target.reportedMissionTotal == target.itemCount) : QVariant()},
        {QStringLiteral("completeStateReportedNotVerified"), reported && target.reportedMissionState == MISSION_STATE_COMPLETE}};
    const QVariantMap references{{QStringLiteral("missionMode"), receiptReference(target, target.missionModeReceipt)},
        {QStringLiteral("missionCurrent"), receiptReference(target, target.missionCurrentReceipt)},
        {QStringLiteral("missionItemReached"), receiptReference(target, target.missionReachedReceipt)},
        {QStringLiteral("terminalCurrent"), receiptReference(target, target.terminalCurrentReceipt)},
        {QStringLiteral("terminalAfterMissionMode"), receiptReference(target, target.terminalAfterMissionModeReceipt)},
        {QStringLiteral("terminalReached"), receiptReference(target, target.terminalReachedReceipt)},
        {QStringLiteral("terminalReachedAfterCurrent"), receiptReference(target, target.terminalReachedAfterCurrentReceipt)},
        {QStringLiteral("airborneState"), receiptReference(target, target.airborneReceipt)},
        {QStringLiteral("terminalAfterAirborne"), receiptReference(target, target.terminalAfterAirborneReceipt)},
        {QStringLiteral("landingAfterTerminal"), receiptReference(target, target.landingAfterTerminalReceipt)},
        {QStringLiteral("landedAfterTerminal"), receiptReference(target, target.landedAfterTerminalReceipt)}};
    const bool retiredSource = !target.sourceRetirementCode.isEmpty();
    const QVariantMap retirement{{QStringLiteral("observed"), retiredSource},
        {QStringLiteral("code"), target.sourceRetirementCode},
        {QStringLiteral("codeOrigin"), retiredSource ? target.sourceRetirementCodeOrigin : QStringLiteral("not-observed")},
        {QStringLiteral("sourceEpoch"), retiredSource ? QVariant(target.queue.sourceEpoch) : QVariant()},
        {QStringLiteral("lastSourceSequence"), retiredSource ? QVariant(QString::number(target.retiredSourceSequence)) : QVariant()}};
    return {{QStringLiteral("schemaVersion"), 2},
        {QStringLiteral("basis"), QStringLiteral("native-local-run-post-commit-reception-order-only")},
        {QStringLiteral("outcomeCandidate"), outcomeCandidate(target)}, {QStringLiteral("observationWindow"), window},
        {QStringLiteral("modeQueueBegan"), target.queueBegan}, {QStringLiteral("modeQueueCommitted"), target.queueCommitted},
        {QStringLiteral("sourceCurrent"), sourceCurrent}, {QStringLiteral("continuityUnknown"), target.continuityUnknown},
        {QStringLiteral("lastCommand"), target.lastCommand},
        {QStringLiteral("missionModeBeforeTerminalObserved"), receiptObserved(target, target.terminalAfterMissionModeReceipt)},
        {QStringLiteral("terminalCurrentObserved"), receiptObserved(target, target.terminalCurrentReceipt)},
        {QStringLiteral("terminalReachedObserved"), receiptObserved(target, target.terminalReachedReceipt)},
        {QStringLiteral("terminalReachedAfterCurrentObserved"), receiptObserved(target, target.terminalReachedAfterCurrentReceipt)},
        {QStringLiteral("nonGroundBeforeTerminalObserved"), receiptObserved(target, target.terminalAfterAirborneReceipt)},
        {QStringLiteral("landingAfterTerminalObserved"), receiptObserved(target, target.landingAfterTerminalReceipt)},
        {QStringLiteral("landedAfterTerminalObserved"), receiptObserved(target, target.landedAfterTerminalReceipt)},
        {QStringLiteral("reportedMission"), reportedMission}, {QStringLiteral("receiptReferences"), references},
        {QStringLiteral("sourceRetirement"), retirement},
        {QStringLiteral("missionModeExitObservation"), missionModeExitObservation(target)},
        {QStringLiteral("candidateIsHistoricalObservation"), true},
        {QStringLiteral("currentValuesFreshnessVerified"), false}, {QStringLiteral("remoteMissionInstanceVerified"), false},
        {QStringLiteral("resultVerified"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("automaticOutcomeActionsIssued"), false}};
}

QVariantMap FleetMissionExecutionObservation::outcomeSummary(const QList<Target>& targets, bool retired) const
{
    int began = 0, committed = 0, noCandidate = 0, terminal = 0, hold = 0, landed = 0;
    int missionBeforeTerminal = 0, reachedAfterCurrent = 0;
    for (const Target& target : targets) {
        began += target.queueBegan ? 1 : 0; committed += target.queueCommitted ? 1 : 0;
        const QString candidate = outcomeCandidate(target);
        if (candidate == QStringLiteral("landed-after-terminal-observed-not-completion")) ++landed;
        else if (candidate == QStringLiteral("indefinite-terminal-hold-observed")) ++hold;
        else if (candidate == QStringLiteral("terminal-item-observed-not-completion")) ++terminal;
        else ++noCandidate;
        missionBeforeTerminal += receiptObserved(target, target.terminalAfterMissionModeReceipt) ? 1 : 0;
        reachedAfterCurrent += receiptObserved(target, target.terminalReachedAfterCurrentReceipt) ? 1 : 0;
    }
    const QString coverage = targets.isEmpty() ? QStringLiteral("no-targets-prepared")
        : committed == 0 ? QStringLiteral("none-committed")
        : committed == targets.size() ? QStringLiteral("all-committed") : QStringLiteral("partial-committed");
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("basis"), QStringLiteral("native-local-run-post-commit-reception-order-only")},
        {QStringLiteral("modeQueueCoverage"), coverage}, {QStringLiteral("targetVehicleCount"), targets.size()},
        {QStringLiteral("modeQueueBeganVehicleCount"), began}, {QStringLiteral("modeCommittedVehicleCount"), committed},
        {QStringLiteral("uncommittedVehicleCount"), targets.size() - committed},
        {QStringLiteral("noTerminalCandidateVehicleCount"), noCandidate}, {QStringLiteral("terminalItemCandidateVehicleCount"), terminal},
        {QStringLiteral("indefiniteHoldCandidateVehicleCount"), hold}, {QStringLiteral("landedAfterTerminalCandidateVehicleCount"), landed},
        {QStringLiteral("missionModeBeforeTerminalVehicleCount"), missionBeforeTerminal},
        {QStringLiteral("terminalReachedAfterCurrentVehicleCount"), reachedAfterCurrent},
        {QStringLiteral("observerRetired"), retired}, {QStringLiteral("unknownTail"), true},
        {QStringLiteral("remoteMissionInstanceVerified"), false}, {QStringLiteral("resultVerified"), false},
        {QStringLiteral("missionCompleted"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("automaticOutcomeActionsIssued"), false}};
}

bool FleetMissionExecutionObservation::nativeTargetCurrent(const Target& supplied) const
{
    const Target target = supplied; // Never retain an internal row reference across a Link virtual.
    const QPointer<FleetMissionExecutionObservation> self(const_cast<FleetMissionExecutionObservation*>(this));
    if (QThread::currentThread() != thread() || _liveProbeBusy || _mutationSerialExhausted
        || !_application || !_toolbox || !_vehicles || qgcApp() != _application.data()
        || _application->thread() != thread() || _toolbox->thread() != thread() || _application->toolbox() != _toolbox.data()
        || _toolbox->multiVehicleManager() != _vehicles.data()) return false;
    const QPointer<QGCApplication> application(_application);
    const QPointer<QGCToolbox> toolbox(_toolbox);
    const QPointer<MultiVehicleManager> vehicles(_vehicles);
    const ReadContext context = readContext();
    const auto same = [&]() {
        if (!self || !self->readContextCurrent(context) || !application || !toolbox || !vehicles
            || self->_application != application || self->_toolbox != toolbox || self->_vehicles != vehicles
            || qgcApp() != application.data() || application->thread() != self->thread() || toolbox->thread() != self->thread()
            || application->toolbox() != toolbox.data()
            || toolbox->multiVehicleManager() != vehicles.data() || vehicles->thread() != self->thread()
            || !target.vehicle || !target.mission || !target.links || !target.primary || target.primaryOwner.expired()
            || target.vehicle->thread() != self->thread() || target.mission->thread() != self->thread()
            || target.links->thread() != self->thread() || vehicles->getVehicleById(target.vehicleId) != target.vehicle.data()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->missionManager() != target.mission.data() || target.vehicle->vehicleLinkManager() != target.links.data()
            || target.vehicle->isOfflineEditingVehicle() || !target.vehicle->isInitialConnectComplete()
            || !target.vehicle->px4Firmware() || !target.vehicle->multiRotor() || target.links->communicationLost()
            || !target.links->containsLink(target.primary.data())) return false;
        const WeakLinkInterfacePtr current = target.links->primaryLink();
        return !current.expired() && !current.owner_before(target.primaryOwner) && !target.primaryOwner.owner_before(current)
            && current.lock().get() == target.primary.data();
    };
    if (!same()) return false;
    _liveProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_liveProbeBusy = false; });
    const bool connected = target.primary->isConnected();
    if (!same() || !connected) return false;
    const bool mock = target.primary->isMockLink();
    if (!same() || mock) return false;
    const bool replay = target.primary->isLogReplay();
    return same() && !replay;
}

bool FleetMissionExecutionObservation::nativeBindingCurrent(const Target& supplied) const
{
    const Target target = supplied;
    const QPointer<FleetMissionExecutionObservation> self(const_cast<FleetMissionExecutionObservation*>(this));
    if (QThread::currentThread() != thread() || _liveProbeBusy || _mutationSerialExhausted
        || !target.vehicle || !target.primary) return false;
    const ReadContext context = readContext();
    _liveProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_liveProbeBusy = false; });
    const bool binding = target.vehicle->fleetExecutionBindingMatches(self.data(), target.queue.sourceEpoch);
    return self && self->readContextCurrent(context) && target.vehicle && target.primary && binding;
}

bool FleetMissionExecutionObservation::prepareFromStartup(FleetMissionStartupTransaction* transaction)
{
    const QPointer<FleetMissionExecutionObservation> self(this);
    const QPointer<FleetMissionStartupTransaction> owner(transaction);
    if (_prepared || _retired || _mutationSerialExhausted || !owner || QThread::currentThread() != thread() || owner->thread() != thread()
        || owner->_executionObservation != this || !owner->_active || owner->_stage != 2 || !owner->_reservationHeld
        || !owner->_same(owner->_generation) || !uuid(owner->_startupId) || !uuid(owner->_transactionEpoch)
        || owner->_targets.isEmpty() || owner->_targets.size() > 32 || !owner->_swarm
        || !owner->_swarm->executionTaskOrchestratorBackend()) return false;
    const quint64 generation = owner->_generation;
    const QString startupId = owner->_startupId, epoch = owner->_transactionEpoch;
    const QVariantMap binding = owner->_binding;
    const ReadContext original = readContext();
    const auto sourceTargets = owner->_targets;
    const QVariantList originalIds = owner->_vehicleIds;
    const auto unchanged = [&]() { return self && owner && !_retired && owner->_executionObservation == self
        && self->readContextCurrent(original)
        && owner->_generation == generation && owner->_startupId == startupId && owner->_transactionEpoch == epoch
        && owner->_active && owner->_stage == 2 && owner->_reservationHeld && owner->_binding == binding
        && owner->_vehicleIds == originalIds && owner->_targets.size() == sourceTargets.size() && owner->_same(generation); };
    QList<Target> targets;
    QVariantList ids;
    QSet<int> seen;
    int totalItems = 0;
    for (const auto& source : sourceTargets) {
        Target target;
        target.vehicleId = source.vehicleId; target.componentId = source.componentId;
        target.vehicle = source.vehicle; target.mission = source.mission; target.links = source.links; target.primary = source.primary;
        target.primaryOwner = source.primaryOwner;
        target.readReceipt = source.readReceipt;
        target.modeStepId = source.stepIds.value(2).toString(); target.payloadHash = source.payload.value(QStringLiteral("contentHash")).toString();
        const QVariantList items = source.payload.value(QStringLiteral("items")).toList();
        target.sources = source.payload.value(QStringLiteral("sourceItemMap")).toList(); target.itemCount = items.size();
        totalItems += target.itemCount;
        const auto targetAlive = [&]() { return unchanged() && target.vehicle && target.mission && target.links && target.primary; };
        if (!targetAlive() || seen.contains(target.vehicleId) || !uuid(target.modeStepId) || !digest(target.payloadHash) || target.itemCount < 1 || target.itemCount > 2048
            || totalItems > 8192 || target.sources.size() != target.itemCount
            || !target.mission) return false;
        const bool native = nativeTargetCurrent(target);
        if (!targetAlive() || !native || target.mission->inProgress() || target.mission->transportCallInProgress()) return false;
        const bool read = target.mission->fleetMissionReadReceiptRetainedSourceMatches(target.primary.data(), target.readReceipt);
        if (!targetAlive() || !read) return false;
        const QVariantMap observation = FleetMissionProgressObservation::capture(target.vehicle.data(), target.mission.data(), source.payload);
        if (!targetAlive() || !yes(observation.value(QStringLiteral("sourceCurrent")))
            || !yes(observation.value(QStringLiteral("cachedMissionContentMatches")))
            || !integer(items.last().toMap().value(QStringLiteral("command")), 0, 65535, &target.lastCommand)) return false;
        target.cacheFingerprint = PlanManager::missionItemsContentFingerprint(target.mission->missionItems());
        if (!targetAlive() || !digest(target.cacheFingerprint)) return false;
        const bool finalNative = nativeTargetCurrent(target);
        if (!targetAlive() || !finalNative) return false;
        target.currentSeen.resize(target.itemCount); target.reachedSeen.resize(target.itemCount);
        for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("missionCurrent"),
            QStringLiteral("landedState"), QStringLiteral("globalPosition")}) {
            target.lastRawAt.insert(channel, -1); target.lastHostAt.insert(channel, -1); target.maximumGap.insert(channel, 0);
        }
        ids.append(target.vehicleId); seen.insert(target.vehicleId); targets.append(target);
    }
    if (!unchanged() || ids.size() != originalIds.size()
        || !owner->_swarm->missionDeploymentOwns(startupId, ids)) return false;
    for (int index = 0; index < ids.size(); ++index)
        if (!integer(originalIds.at(index), ids.at(index).toInt(), ids.at(index).toInt())) return false;
    for (const Target& target : targets) {
        const bool native = nativeTargetCurrent(target);
        if (!unchanged() || !native) return false;
    }
    if (!unchanged()) return false;
    noteMutation();
    if (_mutationSerialExhausted) return false;
    _startup = owner; _swarm = owner->_swarm; _startupGeneration = generation; _startupEpoch = epoch;
    _startupId = startupId; _deploymentId = owner->_authorityContext.value(QStringLiteral("deploymentId")).toString();
    if (!uuid(_deploymentId)) return false;
    _binding = binding; _vehicleIds = ids; _targets = targets; _prepared = true;
    _taskOrchestrator = _swarm->executionTaskOrchestratorBackend();
    const QPointer<SwarmMissionOrchestrator> orchestrator(_taskOrchestrator);
    const ReadContext committed = readContext();
    const QString taskModelId = orchestrator ? orchestrator->registerExecutionObservation(this) : QString();
    if (!self || !owner || !self->readContextCurrent(committed)) return false;
    noteMutation();
    _taskModelId = taskModelId;
    if (_mutationSerialExhausted) return false;
    if (_taskModelId.isEmpty()) {
        _taskModelReason = QStringLiteral("actual-protected-task-model-registration-required-before-mode");
        return false;
    }
    const QList<Target> installedTargets = _targets;
    const ReadContext wired = readContext();
    // Sender connectNotify may reenter the native queue path. Do not permit
    // a live-source admission until this whole connection set is installed.
    _liveProbeBusy = true;
    const auto releaseWiringProbe = qScopeGuard([self]() { if (self) self->_liveProbeBusy = false; });
    QList<QMetaObject::Connection> stagedConnections;
    bool connectionsInstalled = false;
    const auto discardConnections = qScopeGuard([&]() {
        if (!connectionsInstalled)
            for (const QMetaObject::Connection& connection : stagedConnections) QObject::disconnect(connection);
    });
    for (const Target& target : installedTargets) {
        const int id = target.vehicleId;
        if (!self || !self->readContextCurrent(wired) || !target.vehicle || !target.mission) return false;
        stagedConnections.append(connect(target.vehicle.data(), &QObject::destroyed, self.data(), [this, id]() {
            for (Target& row : _targets) if (row.vehicleId == id) invalidate(row, QStringLiteral("execution-vehicle-destroyed"));
        }));
        if (!self || !self->readContextCurrent(wired) || !target.vehicle || !target.mission || !stagedConnections.last()) return false;
        stagedConnections.append(connect(target.mission.data(), &QObject::destroyed, self.data(), [this, id]() {
            for (Target& row : _targets) if (row.vehicleId == id) invalidate(row, QStringLiteral("execution-mission-manager-destroyed"));
        }));
        if (!self || !self->readContextCurrent(wired) || !target.vehicle || !target.mission || !stagedConnections.last()) return false;
        stagedConnections.append(connect(target.mission.data(), &MissionManager::operationStarted, self.data(),
            [this, id](quint64, int) {
                if (QThread::currentThread() != thread()) return;
                // Retain a known operation/ABA even if it starts and finishes
                // between timer ticks and restores identical cache bytes.
                for (Target& row : _targets) if (row.vehicleId == id)
                    invalidate(row, QStringLiteral("execution-foreign-mission-operation-observed"));
            }, Qt::DirectConnection));
        if (!self || !self->readContextCurrent(wired) || !target.vehicle || !target.mission || !stagedConnections.last()) return false;
    }
    _connections = stagedConnections;
    connectionsInstalled = true;
    _timer.start();
    return true;
}

bool FleetMissionExecutionObservation::readyForQueue(Vehicle* vehicle, LinkInterface* primary,
    const QString& suppliedStartupId, const QString& suppliedModeStepId) const
{
    const QString startupId = suppliedStartupId, modeStepId = suppliedModeStepId;
    if (!_prepared || _retired || _startupRetired || _mutationSerialExhausted || !_startup || QThread::currentThread() != thread()
        || _startup->_executionObservation != this || _startup->_generation != _startupGeneration
        || _startup->_transactionEpoch != _startupEpoch || _startup->_startupId != _startupId || startupId != _startupId
        || !_startup->_active || _startup->_stage != 2 || !_startup->_reservationHeld
        || !_startup->_same(_startupGeneration) || !_taskOrchestrator || _taskModelId.isEmpty()) return false;
    const QPointer<FleetMissionExecutionObservation> self(const_cast<FleetMissionExecutionObservation*>(this));
    const QPointer<SwarmMissionOrchestrator> orchestrator(_taskOrchestrator);
    const ReadContext context = readContext();
    const QList<Target> targets = _targets;
    const bool registered = orchestrator->executionObservationMatches(self.data(), context.taskModelId);
    if (!self || !self->readContextCurrent(context) || !registered) return false;
    for (const Target& target : targets) if (target.vehicle.data() == vehicle && target.primary.data() == primary) {
        if ((target.queueBegan && (!target.tracking || !target.sourceActive || target.queueCommitted))
            || target.continuityUnknown || target.modeStepId != modeStepId || !target.cacheMatches) return false;
        const bool native = nativeTargetCurrent(target);
        return self && self->readContextCurrent(context) && native && self->targetIndex(target) >= 0;
    }
    return false;
}

bool FleetMissionExecutionObservation::beginExecutionQueue(Vehicle* vehicle, LinkInterface* link, const QueueContext& supplied)
{
    const QueueContext context = supplied;
    const QPointer<FleetMissionExecutionObservation> self(this);
    const ReadContext original = readContext();
    if (!queueContextValid(context) || !readyForQueue(vehicle, link, context.startupId, context.stepId)) return false;
    if (!self || !self->readContextCurrent(original)) return false;
    Target installed;
    bool began = false;
    {
        for (Target& target : _targets) if (target.vehicle.data() == vehicle && target.primary.data() == link) {
            if (target.queueBegan || context.vehicleId != target.vehicleId || context.componentId != target.componentId) return false;
            noteMutation();
            if (_mutationSerialExhausted) return false;
            target.queue = context; target.queueBegan = true; target.sourceActive = true; target.tracking = true;
            installed = target;
            began = true;
            break;
        }
    }
    if (!began) return false;
    // Publish the actual installed-begin fact before Vehicle can enter its
    // write. Full native evidence probes Link virtuals: no Target reference
    // survives this call, and a changed/deleted original context cannot return
    // true. Vehicle still performs its independent live-source send checks.
    // Failure never resets the installed queue or any later actual commit.
    const ReadContext installedContext = readContext();
    const bool modelUpdated = updateTaskModel();
    if (!self || !self->readContextCurrent(installedContext) || !modelUpdated
        || !installed.vehicle || !installed.mission || !installed.links || !installed.primary) return false;
    const int index = self->targetIndex(installed);
    if (index < 0) return false;
    const Target current = self->_targets.at(index);
    return !self->_retired && !self->_startupRetired && !self->_mutationSerialExhausted
        && current.queueBegan && !current.queueCommitted && current.tracking && current.sourceActive
        && !current.continuityUnknown && sameQueue(current.queue, context)
        && current.vehicle == installed.vehicle && current.mission == installed.mission
        && current.links == installed.links && current.primary == installed.primary;
}

void FleetMissionExecutionObservation::executionQueueCommitted(const QueueContext& context)
{
    if (QThread::currentThread() != thread() || !queueContextValid(context)) return;
    for (Target& target : _targets) if (target.queueBegan && sameQueue(target.queue, context)) {
        noteMutation();
        // The captured queue fact survives source/owner retirement, including
        // cancellation/destruction nested in writeBytesThreadSafe itself.
        if (!target.queueCommitted) target.commitBaselineSequence = target.lastSourceSequence;
        target.queueCommitted = true;
        updateTaskModel(); // Internal facts only; task notifications are queued.
        return;
    }
}

bool FleetMissionExecutionObservation::hasCommittedMode() const
{
    for (const Target& target : _targets) if (target.queueCommitted) return true;
    return false;
}

bool FleetMissionExecutionObservation::advance(Target& target, quint64* counter, quint64 amount)
{
    if (!counter || amount > std::numeric_limits<quint64>::max() - *counter) {
        invalidate(target, QStringLiteral("execution-observation-counter-exhausted")); return false;
    }
    *counter += amount; return true;
}

void FleetMissionExecutionObservation::invalidate(Target& target, const QString& fixedCode)
{
    noteMutation();
    target.continuityUnknown = true; target.tracking = false;
    if (target.reason.isEmpty()) target.reason = fixedCode;
}

void FleetMissionExecutionObservation::observeExecutionFrame(Vehicle* vehicle, LinkInterface* link,
    const mavlink_message_t& original, const QString& sourceEpoch, quint64 sequence, qint64 receivedAt)
{
    if (!_prepared || _retired || _mutationSerialExhausted || QThread::currentThread() != thread()) return;
    int selected = -1;
    for (int index = 0; index < _targets.size(); ++index) {
        const Target& target = _targets.at(index);
        if (target.queueBegan && target.queue.sourceEpoch == sourceEpoch
            && target.vehicle.data() == vehicle && target.primary.data() == link) { selected = index; break; }
    }
    if (selected < 0 || !_targets.at(selected).tracking || !_targets.at(selected).sourceActive) return;
    const Target frozen = _targets.at(selected);
    const QPointer<FleetMissionExecutionObservation> self(this);
    noteMutation();
    if (_mutationSerialExhausted) return;
    const ReadContext context = readContext();
    const bool native = nativeTargetCurrent(frozen);
    if (!self || !self->readContextCurrent(context)) return;
    bool binding = false;
    if (native) {
        binding = nativeBindingCurrent(frozen);
        if (!self || !self->readContextCurrent(context)) return;
    }
    selected = targetIndex(frozen);
    if (selected < 0) return;
    Target& target = _targets[selected];
    if (!native || !binding || original.sysid != target.vehicleId || original.compid != target.componentId
        || sequence == 0 || sequence <= target.lastSourceSequence
        || receivedAt < 0 || (target.lastRawReceivedAt >= 0 && receivedAt < target.lastRawReceivedAt)) {
        invalidate(target, QStringLiteral("execution-native-source-or-order-changed")); return;
    }
    if (sequence - target.lastSourceSequence > 1) {
        if (!advance(target, &target.sourceMissingFrames, sequence - target.lastSourceSequence - 1)) return;
        target.continuityUnknown = true;
    }
    target.lastSourceSequence = sequence; target.lastRawReceivedAt = receivedAt;
    if (!advance(target, &target.frames) || (!target.queueCommitted && !advance(target, &target.provisionalFrames))) return;
    bool relevant = false;
    if (!frameLength(original, &relevant)) {
        advance(target, &target.invalidFrames); invalidate(target, QStringLiteral("execution-raw-frame-length-invalid")); return;
    }
    if (!relevant) { advance(target, &target.ignoredFrames); return; }
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    bool valid = true;
    if (!target.queueCommitted) {
        // These frames can arrive before the actual write or nested inside
        // it. Keep a separate finite diagnostic prefix; a later commit must
        // NEVER reclassify them as post-commit progress/terminal evidence.
        switch (original.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t value{}; mavlink_msg_heartbeat_decode(&normalized, &value);
            valid = value.mavlink_version == 3 && value.autopilot == MAV_AUTOPILOT_PX4
                && value.type == vehicle->vehicleType() && value.system_status < MAV_STATE_ENUM_END;
            px4_custom_mode mode{}; mode.data = value.custom_mode;
            if (valid) target.provisionalMissionModeObserved = target.provisionalMissionModeObserved
                || ((value.base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) && mode.reserved == 0
                    && mode.main_mode == PX4_CUSTOM_MAIN_MODE_AUTO && mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION);
            break;
        }
        case MAVLINK_MSG_ID_MISSION_CURRENT: {
            mavlink_mission_current_t value{}; mavlink_msg_mission_current_decode(&normalized, &value);
            valid = value.mission_state < MISSION_STATE_ENUM_END && value.mission_mode <= 2
                && (value.total == 0 || value.total == target.itemCount) && value.seq < target.itemCount;
            if (valid) target.provisionalCurrentSequence = value.seq;
            break;
        }
        case MAVLINK_MSG_ID_MISSION_ITEM_REACHED: {
            mavlink_mission_item_reached_t value{}; mavlink_msg_mission_item_reached_decode(&normalized, &value);
            valid = value.seq < target.itemCount;
            if (valid) target.provisionalReachedSequence = value.seq;
            break;
        }
        case MAVLINK_MSG_ID_EXTENDED_SYS_STATE: {
            mavlink_extended_sys_state_t value{}; mavlink_msg_extended_sys_state_decode(&normalized, &value);
            valid = value.landed_state < MAV_LANDED_STATE_ENUM_END && value.vtol_state < MAV_VTOL_STATE_ENUM_END;
            if (valid && value.landed_state != MAV_LANDED_STATE_UNDEFINED) {
                target.provisionalOnGroundObserved = target.provisionalOnGroundObserved || value.landed_state == MAV_LANDED_STATE_ON_GROUND;
                target.provisionalAirborneObserved = target.provisionalAirborneObserved || value.landed_state != MAV_LANDED_STATE_ON_GROUND;
            }
            break;
        }
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
            mavlink_global_position_int_t value{}; mavlink_msg_global_position_int_decode(&normalized, &value);
            valid = coordinates(value.lat, value.lon) && (value.hdg <= 35999 || value.hdg == std::numeric_limits<quint16>::max());
            break;
        }
        case MAVLINK_MSG_ID_HOME_POSITION: {
            mavlink_home_position_t value{}; mavlink_msg_home_position_decode(&normalized, &value);
            valid = coordinates(value.latitude, value.longitude);
            break;
        }
        default: break;
        }
        if (!valid) {
            advance(target, &target.invalidFrames); invalidate(target, QStringLiteral("execution-provisional-message-invalid"));
        }
        return;
    }
    ReceiptRef receipt;
    receipt.sourceSequence = sequence; receipt.receivedMonotonicMs = receivedAt;
    const auto retainFirst = [&receipt](ReceiptRef& destination) {
        if (destination.sourceSequence == 0) destination = receipt;
    };
    QString invalidCode;
    switch (original.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t value{}; mavlink_msg_heartbeat_decode(&normalized, &value);
        valid = value.mavlink_version == 3 && value.autopilot == MAV_AUTOPILOT_PX4
            && value.type == vehicle->vehicleType() && value.system_status < MAV_STATE_ENUM_END;
        if (!valid) break;
        px4_custom_mode mode{}; mode.data = value.custom_mode;
        const bool missionMode = (value.base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED)
            && mode.reserved == 0 && mode.main_mode == PX4_CUSTOM_MAIN_MODE_AUTO && mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION;
        if (target.missionModeKnown && target.missionMode && !missionMode) {
            // Only two valid post-commit observations in this same actual
            // source form a sampled departure; provisional/initial non-mission
            // heartbeats never supply the prior mission receipt.
            if (!receiptObserved(target, target.lastMissionModeReceipt)
                || target.lastMissionModeReceipt.sourceSequence >= receipt.sourceSequence
                || target.lastMissionModeReceipt.receivedMonotonicMs > receipt.receivedMonotonicMs) {
                invalidate(target, QStringLiteral("execution-mode-exit-prior-receipt-invalid"));
                return;
            }
            if (!advance(target, &target.modeDepartures)) return;
            ModeExit exit;
            exit.customMode = value.custom_mode;
            exit.baseMode = value.base_mode;
            exit.systemStatus = value.system_status;
            exit.priorMissionReceipt = target.lastMissionModeReceipt;
            exit.exitReceipt = receipt;
            exit.continuityUnknownAtExit = target.continuityUnknown;
            if (target.modeDepartures == 1) target.firstModeExit = exit;
            target.lastModeExit = exit;
        }
        target.armedKnown = true; target.armed = (value.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
        target.missionModeKnown = true; target.missionMode = missionMode;
        target.missionModeEverObserved = target.missionModeEverObserved || missionMode;
        if (missionMode) {
            retainFirst(target.missionModeReceipt);
            target.lastMissionModeReceipt = receipt;
        }
        break;
    }
    case MAVLINK_MSG_ID_MISSION_CURRENT: {
        mavlink_mission_current_t value{}; mavlink_msg_mission_current_decode(&normalized, &value);
        valid = value.mission_state < MISSION_STATE_ENUM_END && value.mission_mode <= 2;
        if (!valid) break;
        // Preserve valid negative reports before ending correlation. Zero
        // extensions stay unknown; neither COMPLETE nor a matching total
        // authenticates this remote mission instance or a result.
        target.missionCurrentReceipt = receipt;
        target.reportedCurrentSequence = value.seq; target.reportedMissionTotal = value.total;
        target.reportedMissionState = value.mission_state; target.reportedMissionMode = value.mission_mode;
        const bool noMission = value.total == std::numeric_limits<quint16>::max() || value.mission_state == MISSION_STATE_NO_MISSION;
        const bool totalMatches = value.total == 0 || value.total == target.itemCount;
        valid = !noMission && totalMatches && value.seq < target.itemCount;
        if (!valid) invalidCode = noMission ? QStringLiteral("execution-reported-no-mission")
            : !totalMatches ? QStringLiteral("execution-reported-mission-total-mismatch")
            : QStringLiteral("execution-reported-mission-sequence-out-of-range");
        if (!valid) break;
        if (target.currentSequence >= 0 && value.seq < target.currentSequence) {
            if (!advance(target, &target.currentRegressions)) return;
            // The valid report/receipt above and earlier observations remain
            // historical facts. This controlled payload has no jump command:
            // a regressed sequence ends this target's correlation window,
            // whether caused by reordering, another mission or a restart.
            // Never combine later REACHED/landed frames with the retained
            // pre-regression terminal/airborne references. No control action,
            // result promotion or reservation release follows invalidation.
            invalidate(target, QStringLiteral("execution-mission-current-regressed-correlation-ended"));
            return;
        }
        if (target.currentSequence >= 0 && value.seq > target.currentSequence + 1 && !advance(target, &target.currentJumps)) return;
        target.currentSequence = value.seq; target.currentSeen.setBit(value.seq);
        if (value.seq == target.itemCount - 1) {
            retainFirst(target.terminalCurrentReceipt);
            if (target.missionModeReceipt.sourceSequence > 0 && sequence > target.missionModeReceipt.sourceSequence)
                retainFirst(target.terminalAfterMissionModeReceipt);
            if (target.airborneReceipt.sourceSequence > 0 && sequence > target.airborneReceipt.sourceSequence)
                retainFirst(target.terminalAfterAirborneReceipt);
        }
        break;
    }
    case MAVLINK_MSG_ID_MISSION_ITEM_REACHED: {
        mavlink_mission_item_reached_t value{}; mavlink_msg_mission_item_reached_decode(&normalized, &value);
        valid = value.seq < target.itemCount;
        if (!valid) break;
        target.missionReachedReceipt = receipt;
        if (target.reachedSeen.testBit(value.seq) && !advance(target, &target.duplicateReached)) return;
        target.reachedSeen.setBit(value.seq); target.reachedSequence = value.seq;
        if (value.seq == target.itemCount - 1) {
            retainFirst(target.terminalReachedReceipt);
            if (target.terminalCurrentReceipt.sourceSequence > 0 && sequence > target.terminalCurrentReceipt.sourceSequence)
                retainFirst(target.terminalReachedAfterCurrentReceipt);
        }
        break; // Repetition is not a new waypoint success or a completion.
    }
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE: {
        mavlink_extended_sys_state_t value{}; mavlink_msg_extended_sys_state_decode(&normalized, &value);
        valid = value.landed_state < MAV_LANDED_STATE_ENUM_END && value.vtol_state < MAV_VTOL_STATE_ENUM_END;
        if (!valid) break;
        const bool previousAirborne = target.landedKnown && target.landedState != MAV_LANDED_STATE_ON_GROUND;
        target.landedKnown = value.landed_state != MAV_LANDED_STATE_UNDEFINED; target.landedState = value.landed_state;
        if (target.landedKnown && value.landed_state != MAV_LANDED_STATE_ON_GROUND)
            retainFirst(target.airborneReceipt);
        if (target.lastCommand == MAV_CMD_NAV_LAND
            && target.terminalAfterAirborneReceipt.sourceSequence > target.airborneReceipt.sourceSequence
            && sequence > target.terminalAfterAirborneReceipt.sourceSequence && value.landed_state == MAV_LANDED_STATE_LANDING)
            retainFirst(target.landingAfterTerminalReceipt);
        if (target.lastCommand == MAV_CMD_NAV_LAND && previousAirborne && target.airborneReceipt.sourceSequence > 0
            && target.terminalAfterAirborneReceipt.sourceSequence > target.airborneReceipt.sourceSequence
            && sequence > target.terminalAfterAirborneReceipt.sourceSequence && value.landed_state == MAV_LANDED_STATE_ON_GROUND)
            retainFirst(target.landedAfterTerminalReceipt);
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        mavlink_global_position_int_t value{}; mavlink_msg_global_position_int_decode(&normalized, &value);
        valid = coordinates(value.lat, value.lon) && (value.hdg <= 35999 || value.hdg == std::numeric_limits<quint16>::max());
        if (!valid) break;
        if (target.bootKnown && quint32(value.time_boot_ms - target.lastBootMs) > 0x80000000U) {
            target.bootRegressionObserved = true; target.continuityUnknown = true;
        }
        target.bootKnown = true; target.lastBootMs = value.time_boot_ms;
        break; // Do not retain/expose coordinates, trajectory or payload.
    }
    case MAVLINK_MSG_ID_HOME_POSITION: {
        mavlink_home_position_t value{}; mavlink_msg_home_position_decode(&normalized, &value);
        valid = coordinates(value.latitude, value.longitude);
        if (!valid) break;
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(QByteArray(_MAV_PAYLOAD(&original), original.len),
            QCryptographicHash::Sha256).toHex());
        if (!target.homeHash.isEmpty() && target.homeHash != hash && !advance(target, &target.homeChanges)) return;
        target.homeHash = hash;
        break; // Changed bytes do not certify HOME validity/origin/branch.
    }
    default: break;
    }
    if (!valid) {
        advance(target, &target.invalidFrames);
        invalidate(target, invalidCode.isEmpty() ? QStringLiteral("execution-raw-message-value-invalid") : invalidCode); return;
    }
    const QString channel = periodicChannel(original.msgid);
    if (!channel.isEmpty()) {
        const qint64 previous = target.lastRawAt.value(channel, -1);
        const qint64 gap = previous >= 0 ? receivedAt - previous : receivedAt;
        if (gap < 0) { invalidate(target, QStringLiteral("execution-periodic-clock-regressed")); return; }
        target.maximumGap[channel] = qMax(target.maximumGap.value(channel), gap);
        target.lastRawAt[channel] = receivedAt; target.lastHostAt[channel] = _clock.elapsed();
    }
}

void FleetMissionExecutionObservation::observeExecutionSourceRetired(Vehicle*, const QString& epoch,
    quint64 lastSequence, const QString& fixedCode)
{
    const QPointer<FleetMissionExecutionObservation> self(this);
    if (QThread::currentThread() != thread()) return;
    for (Target& target : _targets) if (target.queueBegan && target.queue.sourceEpoch == epoch) {
        noteMutation();
        if (target.sourceRetirementCode.isEmpty()) {
            const bool known = knownSourceRetirementCode(fixedCode);
            target.sourceRetirementCode = known ? fixedCode : QStringLiteral("execution-source-retired-code-unrecognized");
            target.sourceRetirementCodeOrigin = known ? QStringLiteral("vehicle-native-fixed-code") : QStringLiteral("unknown-native-code");
        }
        target.retiredSourceSequence = qMax(target.retiredSourceSequence, lastSequence);
        if (lastSequence > target.lastSourceSequence)
            advance(target, &target.sourceMissingFrames, lastSequence - target.lastSourceSequence);
        target.lastSourceSequence = qMax(lastSequence, target.lastSourceSequence);
        target.sourceActive = false;
        invalidate(target, QStringLiteral("execution-native-source-retired-tail-unknown"));
        // A deferred old-epoch retirement may arrive after explicit monitor
        // closure stopped the timer. Preserve that actual final code in the
        // same record now; this bridge only queues public notifications.
        if (self && target.queueBegan && target.queue.sourceEpoch == epoch) updateTaskModel();
        return;
    }
}

void FleetMissionExecutionObservation::detachBindings()
{
    const QPointer<FleetMissionExecutionObservation> self(this);
    const ReadContext original = readContext();
    const QList<Target> targets = _targets;
    // Cleanup is tied to these original Run/queue identities, not to the live
    // evidence serial: a late actual sameQueue commit must not strand later
    // old Vehicle slots, and its recorded facts must never be overwritten.
    const auto sameOwner = [&]() {
        return self && self->_runId == original.runId && self->_startupId == original.startupId
            && self->_startupEpoch == original.startupEpoch && self->_deploymentId == original.deploymentId
            && self->_startupGeneration == original.startupGeneration && self->_binding == original.binding
            && self->_vehicleIds == original.vehicleIds && self->_targets.size() == original.targetCount;
    };
    for (const Target& target : targets) {
        if (!sameOwner() || targetIndex(target) < 0) return;
        if (target.queueBegan && target.vehicle) {
            target.vehicle->unbindFleetExecutionObserver(this, target.queue.sourceEpoch);
            if (!sameOwner()) return;
        }
        const int index = targetIndex(target);
        if (index < 0) return;
        noteMutation();
        _targets[index].sourceActive = false; _targets[index].tracking = false;
    }
}

void FleetMissionExecutionObservation::startupRetired(const QString& state, const QString& reason)
{
    if (QThread::currentThread() != thread()) return;
    const QPointer<FleetMissionExecutionObservation> self(this);
    noteMutation();
    if (!_startupRetired) {
        _startupState = state.left(64); _startupReason = reason.left(256);
    }
    _startupRetired = true;
    const ReadContext context = readContext();
    updateTaskModel();
    if (!self || !self->readContextCurrent(context)) return;
    // Do not retire the independent stream or erase provisional queue facts.
    // A captured commit may still arrive as the actual write stack unwinds.
    if (_prepared && !_retired) _timer.start();
}

bool FleetMissionExecutionObservation::adoptReservation(SwarmController* swarm, const QString& previousOwner,
    const QVariantList& ids)
{
    if (!_prepared || _retired || !hasCommittedMode() || !swarm || swarm != _swarm.data()
        || QThread::currentThread() != thread() || swarm->thread() != thread() || previousOwner != _startupId
        || ids.size() != _vehicleIds.size()) return false;
    for (int index = 0; index < ids.size(); ++index)
        if (!integer(ids.at(index), _vehicleIds.at(index).toInt(), _vehicleIds.at(index).toInt())) return false;
    if (_reservationAdopted) return !_reservationReleased && swarm->missionDeploymentOwns(_runId, _vehicleIds);
    const QPointer<FleetMissionExecutionObservation> self(this);
    const QPointer<SwarmController> originalSwarm(swarm);
    const QString runId = _runId, startupId = _startupId;
    const QVariantList vehicleIds = _vehicleIds;
    // Only actual native int IDs frozen from Vehicle::id() reach the atomic
    // transfer. Legacy JSON doubles are compared, not passed to its strict API.
    const bool transferred = originalSwarm->transferMissionDeployment(previousOwner, runId, vehicleIds);
    if (!self || !originalSwarm || _swarm != originalSwarm || _runId != runId || _startupId != startupId) return false;
    if (!transferred) return false;
    noteMutation();
    _reservationAdopted = true;
    const ReadContext context = readContext();
    updateTaskModel();
    return self && originalSwarm && self->readContextCurrent(context)
        && originalSwarm->missionDeploymentOwns(runId, vehicleIds);
}

void FleetMissionExecutionObservation::releaseOwnReservation()
{
    if (_reservationAdopted && !_reservationReleased && _swarm && _swarm->thread() == QThread::currentThread()) {
        const QPointer<FleetMissionExecutionObservation> self(this);
        const QPointer<SwarmController> swarm(_swarm);
        const QString runId = _runId;
        swarm->releaseMissionDeployment(runId);
        if (!self || _swarm != swarm || _runId != runId) return;
        noteMutation();
        _reservationReleased = true;
    }
}

bool FleetMissionExecutionObservation::retireObservation(bool operatorConfirmed)
{
    if (!operatorConfirmed || QThread::currentThread() != thread()) return false;
    if (_retired) return true;
    const QPointer<FleetMissionExecutionObservation> self(this);
    noteMutation();
    _explicitRetirementRequested = true; // A native request fact, not account authentication.
    _retired = true; _timer.stop();
    for (Target& target : _targets) target.continuityUnknown = true;
    detachBindings();
    if (!self) return true;
    releaseOwnReservation();
    if (!self) return true;
    const ReadContext context = readContext();
    updateTaskModel();
    if (!self || !self->readContextCurrent(context)) return true;
    // No synchronous GUI notification from native retirement/cleanup stacks.
    QTimer::singleShot(0, this, [this]() { emit changed(); });
    return true;
}

bool FleetMissionExecutionObservation::retireUnissuedPreparation(FleetMissionStartupTransaction* actualStartup)
{
    const QPointer<FleetMissionExecutionObservation> self(this);
    const QPointer<FleetMissionStartupTransaction> owner(actualStartup);
    const ReadContext context = readContext();
    const auto same = [&]() {
        return self && owner && self->readContextCurrent(context) && QThread::currentThread() == thread() && owner->thread() == thread()
            && _prepared && !_retired && _startupRetired && owner == _startup && owner->_executionObservation == self
            && owner->_generation == _startupGeneration && owner->_startupId == _startupId
            && owner->_transactionEpoch == _startupEpoch && owner->_binding == _binding
            && !owner->_active && !owner->_busy && !owner->_checking && owner->_sendingIndex < 0
            && owner->_state != QStringLiteral("preparing") && !hasCommittedMode();
    };
    if (!same() || owner->_targets.size() != _targets.size()) return false;
    // A registered begin remains a real preparation fact even when the later
    // Vehicle fence rejects before write. Keep that monitor available to the
    // independent operator Close; auto-retiring it here would make neither
    // that Close nor the strict never-began archive exception reachable.
    for (const Target& target : _targets) if (target.queueBegan) return false;
    for (int index = 0; index < _targets.size(); ++index) {
        const Target target = _targets.at(index);
        const auto source = owner->_targets.at(index);
        if (!same() || !target.vehicle || !target.mission || !target.primary
            || source.vehicle != target.vehicle || source.mission != target.mission || source.primary != target.primary
            || source.vehicleId != target.vehicleId || source.stepIds.value(2).toString() != target.modeStepId
            || source.queued[2] || source.uncertain[2] || target.queueBegan || target.queueCommitted
            || !target.vehicle->fleetStartupTransportQuiescent() || target.mission->transportCallInProgress()) return false;
        const QVariantMap receipt = target.vehicle->fleetStartupStepReceipt(owner.data(), _startupId, target.modeStepId);
        if (!same()) return false;
        if (receipt.isEmpty()) {
            // No native attempt is allowed to disappear after begin. An
            // absent lane is acceptable only if this Run never began it.
            if (target.queueBegan) return false;
        } else if (receipt.size() != 33 || receipt.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
            || receipt.value(QStringLiteral("schemaVersion")).isNull() || receipt.value(QStringLiteral("schemaVersion")).toInt() != 3
            || receipt.value(QStringLiteral("requestIssued")).type() != QVariant::Bool
            || receipt.value(QStringLiteral("requestIssued")).isNull() || receipt.value(QStringLiteral("requestIssued")).toBool()
            || receipt.value(QStringLiteral("startupId")) != _startupId || receipt.value(QStringLiteral("stepId")) != target.modeStepId
            || receipt.value(QStringLiteral("step")).toString() != QStringLiteral("MissionModeOnly")
            || !yes(receipt.value(QStringLiteral("receiveStackQuiescent")))) return false;
    }
    if (!same()) return false;
    // Close only this unused monitoring preparation. No operator action,
    // physical aircraft state, rollback or execution result is inferred.
    noteMutation();
    _unissuedPreparationClosed = true; _retired = true; _timer.stop();
    detachBindings();
    if (!self) return true;
    releaseOwnReservation();
    if (!self) return true;
    const ReadContext retiredContext = readContext();
    updateTaskModel();
    if (!self || !self->readContextCurrent(retiredContext)) return true;
    QTimer::singleShot(0, this, [this]() { emit changed(); });
    return true;
}

void FleetMissionExecutionObservation::poll()
{
    if (_polling || !_prepared || _retired) return;
    const QPointer<FleetMissionExecutionObservation> self(this);
    noteMutation();
    _polling = true;
    const auto release = qScopeGuard([self]() { if (self) self->_polling = false; });
    const QList<Target> originalTargets = _targets;
    for (const Target& original : originalTargets) {
        int index = targetIndex(original);
        if (index < 0) return;
        const Target frozen = _targets.at(index);
        if (!frozen.queueBegan) continue;
        const ReadContext context = readContext();
        QString reason;
        bool cacheMatches = frozen.cacheMatches;
        if (frozen.tracking && !context.exhausted) {
            const bool native = nativeTargetCurrent(frozen);
            if (!self || !self->readContextCurrent(context)) return;
            bool binding = false;
            if (native) {
                binding = nativeBindingCurrent(frozen);
                if (!self || !self->readContextCurrent(context)) return;
            }
            if (!native || !binding) reason = QStringLiteral("execution-polled-source-lost-tail-unknown");
            else if (frozen.mission->inProgress() || frozen.mission->transportCallInProgress())
                reason = QStringLiteral("execution-foreign-mission-transport-observed");
            else {
                // READ age remains an admission bound, not a renewed lease.
                cacheMatches = frozen.mission->missionItems().size() == frozen.itemCount
                    && PlanManager::missionItemsContentFingerprint(frozen.mission->missionItems()) == frozen.cacheFingerprint;
                if (!self || !self->readContextCurrent(context)) return;
                if (!cacheMatches) reason = QStringLiteral("execution-mission-cache-changed");
            }
        }
        index = targetIndex(frozen);
        if (index < 0) return;
        noteMutation();
        _targets[index].cacheMatches = cacheMatches;
        if (!reason.isEmpty()) invalidate(_targets[index], reason);
        if (!_targets.at(index).tracking && _targets.at(index).sourceActive) {
            _targets[index].sourceActive = false;
            const QPointer<Vehicle> vehicle(_targets.at(index).vehicle);
            const QString epoch = _targets.at(index).queue.sourceEpoch;
            const ReadContext detached = readContext();
            if (vehicle) vehicle->unbindFleetExecutionObserver(this, epoch);
            if (!self || !self->readContextCurrent(detached)) return;
        }
    }
    _polling = false;
    const ReadContext context = readContext();
    updateTaskModel();
    if (!self || !self->readContextCurrent(context)) return;
    emit changed(); // All frame/queue/retirement changes coalesced to 250ms.
}

QVariantMap FleetMissionExecutionObservation::current() const
{
    if (QThread::currentThread() != thread()) return {};
    const QPointer<FleetMissionExecutionObservation> self(const_cast<FleetMissionExecutionObservation*>(this));
    const ReadContext context = readContext();
    const QList<Target> targets = _targets;
    QVariantMap result = claims();
    result.insert(QStringLiteral("schemaVersion"), 2); result.insert(QStringLiteral("runId"), _runId);
    result.insert(QStringLiteral("startupId"), _startupId); result.insert(QStringLiteral("transactionEpoch"), _startupEpoch);
    result.insert(QStringLiteral("transactionGeneration"), QString::number(_startupGeneration));
    result.insert(QStringLiteral("deploymentId"), _deploymentId); result.insert(QStringLiteral("binding"), _binding);
    result.insert(QStringLiteral("executionObservationTaskId"), _taskModelId);
    result.insert(QStringLiteral("taskModelReason"), _taskModelReason);
    result.insert(QStringLiteral("taskModelIsLocalObservationOnly"), true);
    result.insert(QStringLiteral("prepared"), _prepared); result.insert(QStringLiteral("retired"), _retired);
    result.insert(QStringLiteral("observationRetirementExplicitlyRequested"), _explicitRetirementRequested);
    result.insert(QStringLiteral("observerOwnerDestroying"), _ownerDestroying);
    result.insert(QStringLiteral("unissuedModePreparationClosed"), _unissuedPreparationClosed);
    result.insert(QStringLiteral("startupRetired"), _startupRetired); result.insert(QStringLiteral("startupTerminalState"), _startupState);
    result.insert(QStringLiteral("startupTerminalReason"), _startupReason);
    result.insert(QStringLiteral("observationReservationCurrentlyHeld"), _reservationAdopted && !_reservationReleased && _swarm
        && _swarm->thread() == thread() && _swarm->missionDeploymentOwns(_runId, _vehicleIds));
    result.insert(QStringLiteral("reservationWasAdopted"), _reservationAdopted);
    result.insert(QStringLiteral("reservationReleasedByObserver"), _reservationReleased);
    result.insert(QStringLiteral("targetVehicleIds"), _vehicleIds);
    result.insert(QStringLiteral("progressMeasure"), QStringLiteral("observed-wire-sequences-not-success-distance-time-or-area"));
    QVariantList rows;
    int committed = 0, live = 0;
    for (const Target& target : targets) {
        bool sourceCurrent = false;
        if (!context.retired && !context.exhausted && target.tracking && target.sourceActive) {
            const bool native = nativeTargetCurrent(target);
            if (!self || !self->readContextCurrent(context)) return {};
            if (native) {
                const bool binding = nativeBindingCurrent(target);
                if (!self || !self->readContextCurrent(context)) return {};
                sourceCurrent = binding;
            }
        }
        committed += target.queueCommitted ? 1 : 0; live += sourceCurrent ? 1 : 0;
        QString phase = !target.queueBegan ? QStringLiteral("awaiting-native-mode-queue")
            : !target.queueCommitted ? QStringLiteral("provisional-native-queue-window")
            : !sourceCurrent ? QStringLiteral("observation-ended-or-source-unknown")
            : receiptObserved(target, target.landedAfterTerminalReceipt) ? QStringLiteral("landed-after-terminal-observed-not-completion")
            : receiptObserved(target, target.landingAfterTerminalReceipt) ? QStringLiteral("landing-after-terminal-observed")
            : receiptObserved(target, target.terminalCurrentReceipt) && target.lastCommand == MAV_CMD_NAV_LOITER_UNLIM
                ? QStringLiteral("indefinite-terminal-hold-observed")
            : receiptObserved(target, target.terminalCurrentReceipt) ? QStringLiteral("terminal-item-observed-not-completion")
            : target.currentSequence >= 0 ? QStringLiteral("mission-sequence-observed") : QStringLiteral("awaiting-native-mission-sequence");
        QVariantMap row{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("componentId"), target.componentId},
            {QStringLiteral("sourceEpoch"), target.queue.sourceEpoch}, {QStringLiteral("modeStepId"), target.modeStepId},
            {QStringLiteral("modeQueueOperationId"), target.queue.operationId}, {QStringLiteral("modeWirePayloadHash"), target.queue.wirePayloadHash},
            {QStringLiteral("payloadContentHash"), target.payloadHash}, {QStringLiteral("itemCount"), target.itemCount},
            {QStringLiteral("phase"), phase}, {QStringLiteral("reason"), target.reason},
            {QStringLiteral("modeQueueBegan"), target.queueBegan}, {QStringLiteral("modeQueueCommitted"), target.queueCommitted},
            {QStringLiteral("progressObservationsPostCommitOnly"), true},
            {QStringLiteral("commitBaselineSourceSequence"), QString::number(target.commitBaselineSequence)},
            {QStringLiteral("sourceCurrent"), sourceCurrent}, {QStringLiteral("cacheMatchesAtLastPoll"), target.cacheMatches},
            {QStringLiteral("continuityUnknown"), target.continuityUnknown}, {QStringLiteral("unknownTail"), true},
            {QStringLiteral("lastSourceSequence"), QString::number(target.lastSourceSequence)},
            {QStringLiteral("observedFrames"), QString::number(target.frames)}, {QStringLiteral("provisionalFrames"), QString::number(target.provisionalFrames)},
            {QStringLiteral("ignoredFrames"), QString::number(target.ignoredFrames)}, {QStringLiteral("invalidFrames"), QString::number(target.invalidFrames)},
            {QStringLiteral("sourceMissingFrames"), QString::number(target.sourceMissingFrames)},
            {QStringLiteral("distinctCurrentSequencesObserved"), target.currentSeen.count(true)},
            {QStringLiteral("distinctReachedSequencesObserved"), target.reachedSeen.count(true)},
            {QStringLiteral("duplicateReachedObservations"), QString::number(target.duplicateReached)},
            {QStringLiteral("currentSequenceRegressions"), QString::number(target.currentRegressions)},
            {QStringLiteral("currentSequenceJumps"), QString::number(target.currentJumps)},
            {QStringLiteral("missionModeDepartures"), QString::number(target.modeDepartures)},
            {QStringLiteral("currentValuesFreshnessVerified"), false}, {QStringLiteral("valuesAreLastObservedOnly"), true},
            {QStringLiteral("lastObservedMissionModeKnown"), target.missionModeKnown}, {QStringLiteral("lastObservedMissionMode"), target.missionMode},
            {QStringLiteral("lastObservedArmedStateKnown"), target.armedKnown}, {QStringLiteral("lastObservedArmed"), target.armed},
            {QStringLiteral("lastObservedLandedStateKnown"), target.landedKnown}, {QStringLiteral("lastObservedLandedState"), target.landedState},
            {QStringLiteral("airborneObserved"), receiptObserved(target, target.airborneReceipt)},
            {QStringLiteral("terminalAfterAirborneObserved"), receiptObserved(target, target.terminalAfterAirborneReceipt)},
            {QStringLiteral("landingAfterTerminalObserved"), receiptObserved(target, target.landingAfterTerminalReceipt)},
            {QStringLiteral("landedAfterTerminalObserved"), receiptObserved(target, target.landedAfterTerminalReceipt)},
            {QStringLiteral("homePayloadObserved"), !target.homeHash.isEmpty()}, {QStringLiteral("homePayloadChanges"), QString::number(target.homeChanges)},
            {QStringLiteral("bootTimeRegressionObserved"), target.bootRegressionObserved},
            {QStringLiteral("observedOutcome"), targetOutcome(target, sourceCurrent, context.retired)}};
        row.insert(QStringLiteral("provisionalObservations"), QVariantMap{
            {QStringLiteral("currentSequence"), target.provisionalCurrentSequence >= 0 ? QVariant(target.provisionalCurrentSequence) : QVariant()},
            {QStringLiteral("reachedSequence"), target.provisionalReachedSequence >= 0 ? QVariant(target.provisionalReachedSequence) : QVariant()},
            {QStringLiteral("missionModeObserved"), target.provisionalMissionModeObserved},
            {QStringLiteral("airborneObserved"), target.provisionalAirborneObserved},
            {QStringLiteral("onGroundObserved"), target.provisionalOnGroundObserved},
            {QStringLiteral("attributedToActualQueue"), false}});
        if (target.currentSequence >= 0) {
            row.insert(QStringLiteral("currentSequence"), target.currentSequence);
            row.insert(QStringLiteral("currentMissionItem"), target.sources.at(target.currentSequence));
        }
        if (target.reachedSequence >= 0) {
            row.insert(QStringLiteral("reachedSequence"), target.reachedSequence);
            row.insert(QStringLiteral("reachedMissionItem"), target.sources.at(target.reachedSequence));
        }
        QVariantMap gaps;
        for (auto it = target.lastRawAt.constBegin(); it != target.lastRawAt.constEnd(); ++it) {
            const qint64 hostSeen = target.lastHostAt.value(it.key(), -1);
            gaps.insert(it.key(), QVariantMap{{QStringLiteral("observed"), it.value() >= 0},
                {QStringLiteral("maximumObservedRawGapMs"), QString::number(target.maximumGap.value(it.key()))},
                {QStringLiteral("hostQuietMs"), hostSeen >= 0 ? QVariant(QString::number(qMax<qint64>(0, _clock.elapsed() - hostSeen))) : QVariant()},
                {QStringLiteral("periodicContinuityCertified"), false}});
        }
        row.insert(QStringLiteral("periodicChannels"), gaps); rows.append(row);
    }
    result.insert(QStringLiteral("modeCommittedVehicleCount"), committed); result.insert(QStringLiteral("currentSourceVehicleCount"), live);
    result.insert(QStringLiteral("observedOutcome"), outcomeSummary(targets, context.retired));
    result.insert(QStringLiteral("vehicles"), rows);
    if (!self || !self->readContextCurrent(context)) return {};
    return result;
}

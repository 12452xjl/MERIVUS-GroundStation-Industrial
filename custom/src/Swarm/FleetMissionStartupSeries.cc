#include "FleetMissionStartupSeries.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetMissionDeploymentEpisode.h"

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
constexpr int MaximumGroundConsumers = 8;
constexpr qint64 MaximumBasisMs = 30000;
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}
bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
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
bool integer(const QVariant& value, double minimum, double maximum)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= minimum && number <= maximum && std::floor(number) == number;
}
bool ids(const QVariant& value)
{
    if (value.type() != QVariant::List) return false;
    const QVariantList entries = value.toList();
    if (entries.isEmpty() || entries.size() > 32) return false;
    QSet<int> seen;
    for (const QVariant& entry : entries) {
        if (!integer(entry, 1, 255) || seen.contains(entry.toInt())) return false;
        seen.insert(entry.toInt());
    }
    return true;
}
bool bindingShape(const QVariant& value)
{
    if (value.type() != QVariant::Map) return false;
    const QVariantMap map = value.toMap();
    // Match the actual Deployment binding, not a new incompatible schema:
    // task identifier text, digest plan ID, numeric archive revision, and an
    // optional pair of numeric input/review generations (7 or 9 fields).
    const QVariant task = map.value(QStringLiteral("sourceTaskId"));
    if ((map.size() != 7 && map.size() != 9) || task.type() != QVariant::String
        || task.toString().isEmpty() || task.toString().size() > 128 || task.toString() != task.toString().trimmed()
        || !uuid(map.value(QStringLiteral("recordId"))) || !integer(map.value(QStringLiteral("recordRevision")), 1, 9007199254740991.0)
        || !digest(map.value(QStringLiteral("planId"))) || !digest(map.value(QStringLiteral("contentHash")))
        || !digest(map.value(QStringLiteral("reviewEvidenceHash"))) || !uuid(map.value(QStringLiteral("storeEpoch")))) return false;
    for (QChar character : task.toString())
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return map.size() == 7 ? !map.contains(QStringLiteral("reviewGeneration")) && !map.contains(QStringLiteral("inputGeneration"))
        : integer(map.value(QStringLiteral("reviewGeneration")), 0, 9007199254740991.0)
            && integer(map.value(QStringLiteral("inputGeneration")), 0, 9007199254740991.0);
}
QVariantMap stableBasis(QVariantMap map)
{
    map.remove(QStringLiteral("remainingLifetimeMs"));
    return map;
}
bool basisShape(const QVariantMap& map, const QString& startupId)
{
    return map.size() == 27 && integer(map.value(QStringLiteral("schemaVersion")), 1, 1)
        && yes(map.value(QStringLiteral("ok"))) && uuid(map.value(QStringLiteral("startupId")))
        && map.value(QStringLiteral("startupId")).toString() == startupId
        && uuid(map.value(QStringLiteral("authorityEpoch"))) && decimal(map.value(QStringLiteral("generation")))
        && uuid(map.value(QStringLiteral("previousStartupId"))) && uuid(map.value(QStringLiteral("previousTransactionEpoch")))
        && uuid(map.value(QStringLiteral("deploymentId"))) && bindingShape(map.value(QStringLiteral("binding")))
        && ids(map.value(QStringLiteral("targetVehicleIds"))) && digest(map.value(QStringLiteral("sourceHash")))
        && map.value(QStringLiteral("scope")).type() == QVariant::String
        && map.value(QStringLiteral("scope")).toString() == QStringLiteral("new-ground-reassessment-startup-basis-not-dispatch")
        && yes(map.value(QStringLiteral("originalRisksPreserved"))) && no(map.value(QStringLiteral("previousAuthorizationRestored")))
        && no(map.value(QStringLiteral("startupPermitGranted"))) && no(map.value(QStringLiteral("executable")))
        && no(map.value(QStringLiteral("flightCommandReleased")))
        && integer(map.value(QStringLiteral("remainingLifetimeMs")), 1, MaximumBasisMs);
}
bool terminal(const QVariantMap& map)
{
    const QString state = map.value(QStringLiteral("state")).toString();
    return no(map.value(QStringLiteral("active"))) && no(map.value(QStringLiteral("reservationHeld")))
        && no(map.value(QStringLiteral("transportDraining")))
        && (state == QStringLiteral("failed") || state == QStringLiteral("stale") || state == QStringLiteral("cancelled")
            || state == QStringLiteral("expired") || state == QStringLiteral("started-observed"));
}
bool empty(const QVariantMap& map)
{
    return map.value(QStringLiteral("startupId")).type() == QVariant::String
        && map.value(QStringLiteral("startupId")).toString().isEmpty()
        && map.value(QStringLiteral("state")).toString() == QStringLiteral("empty")
        && no(map.value(QStringLiteral("active"))) && no(map.value(QStringLiteral("reservationHeld")))
        && no(map.value(QStringLiteral("transportDraining")));
}
bool modeIssued(const QVariantMap& map)
{
    for (const QVariant& vehicle : map.value(QStringLiteral("vehicles")).toList())
        for (const QVariant& step : vehicle.toMap().value(QStringLiteral("steps")).toList()) {
            const QVariantMap row = step.toMap();
            if (row.value(QStringLiteral("step")).toString() == QStringLiteral("MissionModeOnly")
                && (yes(row.value(QStringLiteral("requestIssued"))) || yes(row.value(QStringLiteral("mayHaveCommitted"))))) return true;
        }
    return false;
}
QVariantMap redacted(const QVariantMap& map)
{
    QVariantMap result;
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("generation"),
        QStringLiteral("sourceHash"), QStringLiteral("deploymentId"), QStringLiteral("binding"), QStringLiteral("targetVehicleIds"),
        QStringLiteral("state"), QStringLiteral("reason"), QStringLiteral("active"), QStringLiteral("reservationHeld"),
        QStringLiteral("transportDraining"), QStringLiteral("stage"), QStringLiteral("groundReassessmentStartup"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("commandsMayHaveCommitted"), QStringLiteral("missionStateMayHaveChanged")})
        result.insert(key, map.value(key));
    result.insert(QStringLiteral("modeRequestIssuedOrUncertain"), modeIssued(map));
    result.insert(QStringLiteral("snapshotOnly"), true);
    result.insert(QStringLiteral("startupPermitGranted"), false);
    result.insert(QStringLiteral("executionVerified"), false);
    result.insert(QStringLiteral("missionCompleted"), false);
    return result;
}
}

FleetMissionStartupSeries::FleetMissionStartupSeries(QObject* parent) : QObject(parent) { _clock.start(); }
FleetMissionStartupSeries::~FleetMissionStartupSeries()
{
    _destroying = true;
    for (const auto& connection : _connections) disconnect(connection);
    // QObject owns the ground children; their own destructors cancel/drain the
    // exact scopes. Never release their reservations here or erase old facts.
}
QObject* FleetMissionStartupSeries::initialTransaction() const { return _initial.data(); }
QObject* FleetMissionStartupSeries::consumer() const
{
    return !_evidenceLost && _owners.size() > 1 && _owners.last().transaction
        && _owners.last().transaction->thread() == thread() ? _owners.last().transaction.data() : nullptr;
}
QObject* FleetMissionStartupSeries::previousStartup() const
{
    if (_evidenceLost || !_initialAssigned || !_initial) return nullptr;
    // Passive native snapshots only; no gate/OS/packet/source capture. Inspect
    // the real owners, including a newly initialized one before publication.
    QObject* previous = _initial.data();
    for (const Owner& owner : _owners) {
        if (!owner.transaction || owner.lost || owner.transaction->thread() != thread()) return nullptr;
        const QVariantMap snapshot = owner.transaction->current();
        const QString id = snapshot.value(QStringLiteral("startupId")).toString();
        if (!id.isEmpty()) {
            if (!uuid(snapshot.value(QStringLiteral("startupId")))
                || (owner.initialized && (id != owner.startupId || snapshot.value(QStringLiteral("transactionEpoch")) != owner.epoch
                    || snapshot.value(QStringLiteral("generation")) != owner.generation || snapshot.value(QStringLiteral("sourceHash")) != owner.sourceHash))) return nullptr;
            previous = owner.transaction.data();
        }
        else if (owner.initialized) return nullptr;
    }
    return previous;
}
bool FleetMissionStartupSeries::consumerIsCurrent(QObject* actualTransaction) const
{
    return thread() == QThread::currentThread() && actualTransaction && !_destroying && !_evidenceLost
        && _owners.size() > 1 && _owners.last().transaction.data() == actualTransaction
        && actualTransaction->thread() == thread();
}
bool FleetMissionStartupSeries::consumerBasisCurrent(QObject* actualTransaction, QObject* actualAuthority,
    const QString& startupId) const
{
    if (!consumerIsCurrent(actualTransaction) || !actualAuthority
        || !outboundGroup(this, {actualTransaction, actualAuthority})) return false;
    const Owner& owner = _owners.last();
    if (owner.authority.data() != actualAuthority || actualAuthority->thread() != thread() || owner.expectedStartupId != startupId
        || owner.basis.value(QStringLiteral("startupId")) != startupId
        || owner.basis.value(QStringLiteral("authorityEpoch")) != owner.authority->authorityEpoch()) return false;
    // PURE native memory snapshots, not authorityContext/full gate/OS. Keep
    // this binding through Authority->Transaction transfer, but never across
    // another generation on the same real Authority object.
    const QVariantMap basis = owner.authority->current();
    if (basis.value(QStringLiteral("startupId")) != startupId
        || basis.value(QStringLiteral("authorityEpoch")) != owner.basis.value(QStringLiteral("authorityEpoch"))
        || basis.value(QStringLiteral("generation")) != owner.basis.value(QStringLiteral("generation"))
        || basis.value(QStringLiteral("sourceHash")) != owner.basis.value(QStringLiteral("sourceHash"))) return false;
    const QVariantMap snapshot = owner.transaction->current();
    if (snapshot.value(QStringLiteral("transactionEpoch")) != owner.epoch) return false;
    if (empty(snapshot)) return !owner.initialized && snapshot.value(QStringLiteral("generation")).toString() == QStringLiteral("0");
    const QString state = snapshot.value(QStringLiteral("state")).toString();
    // Initialization precedes the first changed publication. Admit only this
    // planned UUID and constructor epoch with the actual first generation.
    return (state == QStringLiteral("preparing") || state == QStringLiteral("running"))
        && yes(snapshot.value(QStringLiteral("groundReassessmentStartup")))
        && snapshot.value(QStringLiteral("startupId")) == startupId
        && snapshot.value(QStringLiteral("sourceHash")) == owner.basis.value(QStringLiteral("sourceHash"))
        && snapshot.value(QStringLiteral("generation")).toString() == QStringLiteral("1")
        && (!owner.initialized || (owner.startupId == startupId && owner.generation == QStringLiteral("1")));
}
QVariantMap FleetMissionStartupSeries::current() const
{
    QVariantList rows;
    for (int index = 0; index < _owners.size(); ++index) {
        const Owner& owner = _owners.at(index);
        const QVariantMap snapshot = owner.transaction && owner.transaction->thread() == thread() && !_evidenceLost
            ? redacted(owner.transaction->current()) : owner.snapshot;
        rows.append(QVariantMap{{QStringLiteral("round"), index}, {QStringLiteral("sourceRetained"), bool(owner.transaction)},
            {QStringLiteral("evidenceLost"), owner.lost}, {QStringLiteral("snapshot"), snapshot}});
    }
    const QObject* previous = previousStartup();
    const auto* actualPrevious = qobject_cast<const FleetMissionStartupTransaction*>(previous);
    const QVariantMap previousSnapshot = actualPrevious ? actualPrevious->current() : QVariantMap();
    const QVariantMap consumerSnapshot = _owners.size() > 1 ? (_owners.last().transaction && _owners.last().transaction->thread() == thread() && !_evidenceLost
        ? redacted(_owners.last().transaction->current()) : _owners.last().snapshot) : QVariantMap();
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("scope"), QStringLiteral("bounded-native-startup-owner-series-not-permission")},
        {QStringLiteral("initialAssigned"), _initialAssigned}, {QStringLiteral("anchorFrozen"), _anchorFrozen},
        {QStringLiteral("groundConsumerCount"), qMax(0, _owners.size() - 1)}, {QStringLiteral("capacity"), MaximumGroundConsumers},
        {QStringLiteral("capacityRemaining"), qMax(0, MaximumGroundConsumers - qMax(0, _owners.size() - 1))},
        {QStringLiteral("capacityExhausted"), _owners.size() > MaximumGroundConsumers},
        {QStringLiteral("evidenceLost"), _evidenceLost}, {QStringLiteral("lastError"), _lastError},
        {QStringLiteral("historicalSnapshotsOnly"), _evidenceLost},
        {QStringLiteral("sourceLossMayHideCommittedCommands"), _evidenceLost},
        {QStringLiteral("previousStartupId"), previousSnapshot.value(QStringLiteral("startupId"))},
        {QStringLiteral("previousTransactionEpoch"), previousSnapshot.value(QStringLiteral("transactionEpoch"))},
        {QStringLiteral("consumerSnapshot"), consumerSnapshot}, {QStringLiteral("owners"), rows},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("automaticCommandsSent"), false}, {QStringLiteral("previousAuthorizationRestored"), false}};
}
void FleetMissionStartupSeries::_loseEvidence(const QString& reason)
{
    if (_evidenceLost) return;
    _evidenceLost = true;
    _lastError = reason;
    if (_revision != std::numeric_limits<quint64>::max()) ++_revision;
}
void FleetMissionStartupSeries::_observeOwners()
{
    if (_destroying || _evidenceLost) return;
    for (int index = 0; index < _owners.size(); ++index) {
        Owner& owner = _owners[index];
        if (!owner.transaction || owner.lost) { _loseEvidence(QStringLiteral("native-owner-source-lost-no-earlier-owner-fallback")); return; }
        if (owner.transaction->thread() != thread()) { _loseEvidence(QStringLiteral("native-owner-thread-changed-no-cross-thread-snapshot")); return; }
        const QVariantMap snapshot = owner.transaction->current();
        if (empty(snapshot)) {
            if (owner.initialized) { _loseEvidence(QStringLiteral("initialized-native-owner-reset-no-history-overwrite")); return; }
            owner.snapshot = redacted(snapshot);
            continue;
        }
        if (!uuid(snapshot.value(QStringLiteral("startupId"))) || !uuid(snapshot.value(QStringLiteral("transactionEpoch")))
            || !decimal(snapshot.value(QStringLiteral("generation"))) || snapshot.value(QStringLiteral("transactionEpoch")) != owner.epoch
            || !digest(snapshot.value(QStringLiteral("sourceHash")))
            || !uuid(snapshot.value(QStringLiteral("deploymentId"))) || !bindingShape(snapshot.value(QStringLiteral("binding")))
            || !ids(snapshot.value(QStringLiteral("targetVehicleIds")))
            || (index > 0 && (!yes(snapshot.value(QStringLiteral("groundReassessmentStartup")))
                || snapshot.value(QStringLiteral("startupId")) != owner.expectedStartupId
                || snapshot.value(QStringLiteral("sourceHash")) != owner.basis.value(QStringLiteral("sourceHash"))
                || snapshot.value(QStringLiteral("generation")).toString() != QStringLiteral("1")))) {
            _loseEvidence(QStringLiteral("native-owner-round-provenance-mismatch")); return;
        }
        if (!owner.initialized) {
            owner.initialized = true;
            owner.startupId = snapshot.value(QStringLiteral("startupId")).toString();
            owner.epoch = snapshot.value(QStringLiteral("transactionEpoch")).toString();
            owner.generation = snapshot.value(QStringLiteral("generation")).toString();
            owner.sourceHash = snapshot.value(QStringLiteral("sourceHash")).toString();
            if (_revision == std::numeric_limits<quint64>::max()) { _loseEvidence(QStringLiteral("series-revision-exhausted")); return; }
            ++_revision;
        } else if (owner.startupId != snapshot.value(QStringLiteral("startupId"))
            || owner.epoch != snapshot.value(QStringLiteral("transactionEpoch"))
            || owner.generation != snapshot.value(QStringLiteral("generation"))
            || owner.sourceHash != snapshot.value(QStringLiteral("sourceHash"))) {
            _loseEvidence(QStringLiteral("native-owner-reused-old-round-evidence-overwritten")); return;
        }
        if (_anchorFrozen && (snapshot.value(QStringLiteral("binding")).toMap() != _anchorBinding
            || snapshot.value(QStringLiteral("targetVehicleIds")).toList() != _anchorIds
            || snapshot.value(QStringLiteral("deploymentId")).toString() != _anchorDeploymentId)) {
            _loseEvidence(QStringLiteral("native-owner-original-series-binding-changed")); return;
        }
        owner.snapshot = redacted(snapshot);
    }
}
void FleetMissionStartupSeries::_ownerChanged(int index)
{
    if (_destroying || index < 0 || index >= _owners.size()) return;
    _observeOwners();
    emit changed(); // Snapshot notification only, no approval/read/dispatch.
}
void FleetMissionStartupSeries::_ownerDestroyed(int index)
{
    if (_destroying || index < 0 || index >= _owners.size()) return;
    _owners[index].lost = true;
    // Also stop on loss of an apparently empty candidate: destruction can
    // follow initialization before its first changed publication reaches us.
    _loseEvidence(QStringLiteral("actual-native-owner-destroyed-no-fallback-or-reconstruction"));
    emit changed();
}
void FleetMissionStartupSeries::_connectOwner(int index)
{
    FleetMissionStartupTransaction* actual = _owners.at(index).transaction.data();
    _connections.append(connect(actual, &FleetMissionStartupTransaction::changed, this, [this, index]() { _ownerChanged(index); }));
    _connections.append(connect(actual, &QObject::destroyed, this, [this, index]() { _ownerDestroyed(index); }));
}
void FleetMissionStartupSeries::setInitialTransaction(QObject* actualObject)
{
    if (_destroying || thread() != QThread::currentThread() || _busy || _initialAssigned || _evidenceLost) return;
    const QPointer<FleetMissionStartupTransaction> actual(qobject_cast<FleetMissionStartupTransaction*>(actualObject));
    if (!actual || actual->thread() != thread()) { _lastError = QStringLiteral("one-actual-same-thread-initial-transaction-required"); emit changed(); return; }
    if (_deploymentEpisodeManaged || actual->_deploymentEpisodeManaged) {
        // Constructor-only bootstrap: the real Episode cannot yet pass its
        // published/current gate until this one-time Series binding exists.
        const QPointer<FleetMissionDeploymentEpisode> episode(_deploymentEpisodeOwner);
        if (!_deploymentEpisodeManaged || !actual->_deploymentEpisodeManaged || !episode
            || actual->_deploymentEpisodeOwner != episode || episode->thread() != thread()
            || parent() != episode.data() || actual->parent() != episode.data()
            || episode->startupSeries() != this || episode->initialTransaction() != actual.data()
            || _revision != 0 || !_owners.isEmpty() || _initial || _anchorFrozen
            || actual->_generation != 0 || !actual->_startupId.isEmpty() || actual->_state != QStringLiteral("empty")
            || actual->_active || actual->_busy || actual->_checking || actual->_sendingIndex >= 0
            || actual->_reservationHeld || actual->_handoffPending || actual->_transportDraining || actual->_executionObservation) {
            _lastError = QStringLiteral("exact-pristine-owned-episode-bootstrap-required"); emit changed(); return;
        }
    } else if (!outboundGroup(this, {actual.data()})) {
        _lastError = QStringLiteral("legacy-initial-sources-must-both-be-unmanaged"); emit changed(); return;
    }
    const QPointer<FleetMissionStartupSeries> self(this);
    _initialAssigned = true;
    _initial = actual;
    Owner owner; owner.transaction = actual; owner.epoch = actual->transactionEpoch();
    _owners.append(owner); ++_revision;
    _connectOwner(0);
    if (!self) return;
    _observeOwners();
    emit changed(); // Fully committed before any reentrant notification.
}
QObject* FleetMissionStartupSeries::_deny(const QString& reason)
{
    _lastError = reason;
    // Acquire remains busy during publication: a nested acquisition is denied
    // and the outer one must recheck its actual sources after publication.
    emit changed();
    return nullptr;
}
bool FleetMissionStartupSeries::_allOwnersIdle() const
{
    if (_evidenceLost || !_initial || _owners.isEmpty()) return false;
    for (const Owner& owner : _owners) {
        if (!owner.transaction || owner.lost || owner.transaction->thread() != thread()) return false;
        const QVariantMap snapshot = owner.transaction->current();
        if (owner.initialized ? !terminal(snapshot) : !empty(snapshot)) return false;
    }
    return true;
}
bool FleetMissionStartupSeries::_sameOwners(quint64 revision, QObject* previous, int size) const
{
    return !_destroying && !_evidenceLost && !_reentered && _revision == revision
        && outboundGroup(this, {previous, _initial.data()})
        && _owners.size() == size && previousStartup() == previous && _allOwnersIdle();
}
QObject* FleetMissionStartupSeries::acquireConsumer(QObject* actualObject, const QString& suppliedId, bool operatorConfirmed)
{
    const QString startupId = suppliedId;
    if (thread() != QThread::currentThread() || _destroying || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return nullptr;
    if (_busy) { _reentered = true; return nullptr; }
    _busy = true; _reentered = false;
    FlagGuard requesting{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartupSeries> self(this);
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(actualObject));
    _observeOwners();
    if (_evidenceLost || !_initialAssigned || !_initial || _owners.isEmpty() || !operatorConfirmed
        || !uuid(startupId) || !authority || authority->thread() != thread()
        || !outboundGroup(this, {authority.data(), _initial.data()}) || !_allOwnersIdle()
        || !_owners.first().initialized) return _deny(QStringLiteral("explicit-confirmed-current-terminal-native-series-required"));
    const QPointer<FleetMissionStartupTransaction> previous(qobject_cast<FleetMissionStartupTransaction*>(previousStartup()));
    if (!previous) return _deny(QStringLiteral("latest-native-owner-required-no-earlier-round-fallback"));
    const QVariantMap previousSnapshot = previous->current();
    if (modeIssued(previousSnapshot)) return _deny(QStringLiteral("latest-owner-has-mode-queued-or-uncertain-reassessment-denied"));
    const quint64 revision = _revision;
    const int size = _owners.size();
    const auto same = [&]() { return self && authority && previous
        && outboundGroup(self.data(), {authority.data(), previous.data()}) && _sameOwners(revision, previous.data(), size); };
    const qint64 beforeContext = _clock.elapsed();
    const QVariantMap context = authority->authorityContext(startupId); // Only explicit factory invokes full gate/OS.
    if (!same() || !basisShape(context, startupId)
        || context.value(QStringLiteral("previousStartupId")) != previousSnapshot.value(QStringLiteral("startupId"))
        || context.value(QStringLiteral("previousTransactionEpoch")) != previousSnapshot.value(QStringLiteral("transactionEpoch")))
        return self ? _deny(QStringLiteral("actual-current-ground-authority-and-latest-terminal-reference-required")) : nullptr;
    const QPointer<FleetMissionStartupTransaction> actualPrevious(authority->nativePreviousStartup(startupId));
    if (!same() || actualPrevious != previous) return self ? _deny(QStringLiteral("authority-previous-object-substitution-denied")) : nullptr;
    const QVariantMap baseSnapshot = _initial->current();
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    const QVariantList targetIds = context.value(QStringLiteral("targetVehicleIds")).toList();
    const QString deploymentId = context.value(QStringLiteral("deploymentId")).toString();
    if (!same() || baseSnapshot.value(QStringLiteral("binding")).toMap() != binding
        || baseSnapshot.value(QStringLiteral("targetVehicleIds")).toList() != targetIds
        || baseSnapshot.value(QStringLiteral("deploymentId")).toString() != deploymentId
        || (_anchorFrozen && (_anchorBinding != binding || _anchorIds != targetIds || _anchorDeploymentId != deploymentId)))
        return self ? _deny(QStringLiteral("same-original-ordered-group-deployment-and-record-binding-required")) : nullptr;
    const qint64 deadline = beforeContext + context.value(QStringLiteral("remainingLifetimeMs")).toLongLong();
    if (!authority->authorityContextCurrent(startupId, context) || !same() || _clock.elapsed() >= deadline)
        return self ? _deny(QStringLiteral("original-two-identity-budget-or-full-source-gate-expired")) : nullptr;
    // An unused allocated child may be reused ONLY for the same real authority
    // and full stable context. A genuinely new validated basis receives a NEW
    // child, retaining this unused old owner and charging it against capacity.
    // Never change an existing child's identities/basis/deadline in place.
    if (size > 1 && !_owners.last().initialized) {
        Owner& latest = _owners.last();
        if (latest.expectedStartupId == startupId) {
            if (latest.authority != authority || stableBasis(latest.basis) != stableBasis(context)
                || !latest.transaction || !outboundGroup(this, {authority.data(), latest.transaction.data()})
                || !empty(latest.transaction->current()) || _clock.elapsed() >= latest.allocationDeadlineMs || !same())
                return _deny(QStringLiteral("unused-consumer-bound-to-exact-original-basis-no-replacement"));
            latest.allocationDeadlineMs = qMin(latest.allocationDeadlineMs, deadline);
            _lastError.clear();
            return latest.transaction.data();
        }
    }
    if (size - 1 >= MaximumGroundConsumers) return _deny(QStringLiteral("bounded-eight-ground-consumer-capacity-exhausted-no-eviction"));
    for (const Owner& owner : _owners)
        if (owner.startupId == startupId || owner.expectedStartupId == startupId)
            return _deny(QStringLiteral("startup-id-already-belongs-to-retained-round"));
    if (_revision == std::numeric_limits<quint64>::max()) return _deny(QStringLiteral("series-revision-exhausted"));
    // No signal/packet/API occurs while these fields commit. The new child is
    // distinct from every previous actual owner and has a native empty scope.
    Owner allocated;
    allocated.transaction = new FleetMissionStartupTransaction(this);
    if (!self) return nullptr;
    if (!allocated.transaction) { _loseEvidence(QStringLiteral("new-native-consumer-source-lost")); return _deny(_lastError); }
    allocated.transaction->_deploymentEpisodeManaged = _deploymentEpisodeManaged;
    allocated.transaction->_deploymentEpisodeOwner = _deploymentEpisodeOwner;
    allocated.epoch = allocated.transaction->transactionEpoch();
    allocated.authority = authority;
    allocated.basis = context;
    allocated.expectedStartupId = startupId;
    allocated.allocationDeadlineMs = deadline;
    allocated.snapshot = redacted(allocated.transaction->current());
    _owners.append(allocated);
    _anchorBinding = binding; _anchorIds = targetIds; _anchorDeploymentId = deploymentId; _anchorFrozen = true;
    ++_revision; _lastError.clear();
    _connectOwner(size);
    const QPointer<FleetMissionStartupTransaction> consumerObject(allocated.transaction);
    const quint64 committedRevision = _revision;
    if (!outboundGroup(this, {authority.data(), previous.data(), consumerObject.data()}))
        return _deny(QStringLiteral("allocated-consumer-episode-provenance-not-current"));
    emit changed();
    if (!self) return nullptr;
    const auto committed = [&]() {
        return self && authority && previous && consumerObject && !_reentered && !_evidenceLost
            && outboundGroup(self.data(), {authority.data(), previous.data(), consumerObject.data()})
            && _revision == committedRevision && _owners.size() == size + 1
            && _owners.last().transaction == consumerObject && _owners.last().authority == authority
            && _owners.last().expectedStartupId == startupId && !_owners.last().initialized
            && previousStartup() == previous.data() && _allOwnersIdle() && _clock.elapsed() < deadline;
    };
    if (!committed()) { _lastError = QStringLiteral("consumer-publication-changed-retained-without-dispatch"); return nullptr; }
    const bool stillCurrent = authority->authorityContextCurrent(startupId, context);
    if (!committed() || !stillCurrent) {
        if (self) _lastError = QStringLiteral("authority-changed-after-consumer-publication-no-dispatch");
        return nullptr;
    }
    return consumerObject.data();
}

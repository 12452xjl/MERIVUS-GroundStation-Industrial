#include "FleetFormationQueue.h"

#include "FleetNativeOccupancyEvidence.h"
#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"

#include <QDateTime>
#include <QMetaType>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QtCore/qscopeguard.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr int QueueLimit = 32;
constexpr qint64 PendingLifetimeMs = 30LL * 60LL * 1000LL;
constexpr qint64 ReviewLifetimeMs = 60LL * 1000LL;

bool vehicleId(const QVariant& value, int maximum, int& id)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt:
    case QMetaType::LongLong: case QMetaType::ULongLong: case QMetaType::Double:
        break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 1 || number > maximum || std::floor(number) != number) return false;
    id = static_cast<int>(number);
    return true;
}

bool boundedText(const QString& value, int maximum)
{
    return value.size() <= maximum && !value.contains(QChar(0));
}
}

FleetFormationQueue::FleetFormationQueue(SwarmController* actualSwarm, QObject* parent)
    : QObject(parent), _swarm(actualSwarm)
{
    // Swarm constructs this child before its own constructor has completed.
    // Do not inspect the owner until bind, a later API call or the timer.
    _clock.start();
    QTimer* timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &FleetFormationQueue::_poll);
    timer->start();
}

void FleetFormationQueue::bindOrchestrator(SwarmMissionOrchestrator* actual)
{
    if (_bound || _busy || QThread::currentThread() != thread() || !_swarm || !actual
        || _swarm->thread() != thread() || actual->thread() != thread()
        || _swarm->executionTaskOrchestratorBackend() != actual) return;
    const QPointer<FleetFormationQueue> self(this);
    const QPointer<SwarmController> originalSwarm(_swarm);
    const QPointer<SwarmMissionOrchestrator> originalOrchestrator(actual);
    _busy = true;
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _bound = true;
    _orchestrator = actual;
    const auto current = [self, originalSwarm, originalOrchestrator]() {
        return self && originalSwarm && originalOrchestrator
            && self->_swarm == originalSwarm && self->_orchestrator == originalOrchestrator
            && originalSwarm->thread() == self->thread() && originalOrchestrator->thread() == self->thread()
            && originalSwarm->executionTaskOrchestratorBackend() == originalOrchestrator.data();
    };
    // connectNotify is a synchronous external boundary. Never use a later
    // source or a destroyed receiver for the next connection in this batch.
    connect(originalOrchestrator.data(), &SwarmMissionOrchestrator::taskCollectionChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalOrchestrator.data(), &SwarmMissionOrchestrator::executionObservationTasksChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalOrchestrator.data(), &QObject::destroyed, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &SwarmController::formationActiveChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &SwarmController::formationBusyChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &SwarmController::transactionsChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &SwarmController::continuityOccupancyPersistenceChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &SwarmController::executionTaskOrchestratorChanged, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    connect(originalSwarm.data(), &QObject::destroyed, this, &FleetFormationQueue::_poll);
    if (!current()) return;
    // Binding itself does not synchronously publish from Swarm's source setter.
    QTimer::singleShot(0, this, [self]() { if (self) self->_poll(); });
}

bool FleetFormationQueue::_terminal(const QString& state)
{
    return state == QStringLiteral("submitted") || state == QStringLiteral("rejected")
        || state == QStringLiteral("cancelled") || state == QStringLiteral("expired");
}

bool FleetFormationQueue::_vehicleIds(const QVariantList& input, QVariantList& canonical)
{
    canonical.clear();
    if (input.isEmpty() || input.size() > 6) return false;
    QList<int> ids;
    for (const QVariant& value : input) {
        int id = 0;
        if (!vehicleId(value, 6, id) || ids.contains(id)) return false;
        ids.append(id);
    }
    if (!ids.contains(1)) return false;
    std::sort(ids.begin(), ids.end());
    for (int id : ids) canonical.append(id);
    return true;
}

QVariantList FleetFormationQueue::rows() const
{
    if (thread() != QThread::currentThread()) return {};
    QVariantList result;
    for (const Entry& entry : _entries) {
        const bool pending = !_terminal(entry.state) && entry.state != QStringLiteral("dispatching");
        result.append(QVariantMap{
            {QStringLiteral("id"), entry.id}, {QStringLiteral("revision"), QString::number(entry.revision)},
            {QStringLiteral("vehicleIds"), entry.vehicleIds}, {QStringLiteral("state"), entry.state},
            {QStringLiteral("reason"), entry.reason}, {QStringLiteral("createdAt"), entry.createdAt},
            {QStringLiteral("expiresAt"), entry.expiresAt},
            {QStringLiteral("canReview"), !_revisionExhausted && _sourceCurrent
                && (entry.state == QStringLiteral("ready-to-review") || entry.state == QStringLiteral("awaiting-confirmation"))},
            {QStringLiteral("canCancel"), !_revisionExhausted && pending},
            {QStringLiteral("canRemove"), !_revisionExhausted && _terminal(entry.state)},
            {QStringLiteral("taskId"), entry.taskId}});
    }
    return result;
}

QVariantMap FleetFormationQueue::status() const
{
    if (thread() != QThread::currentThread()) return {};
    return {{QStringLiteral("count"), _entries.size()}, {QStringLiteral("limit"), QueueLimit},
        {QStringLiteral("sourceCurrent"), _sourceCurrent && !_revisionExhausted},
        {QStringLiteral("automaticDispatch"), false}, {QStringLiteral("persistent"), false}};
}

bool FleetFormationQueue::_enter()
{
    if (thread() != QThread::currentThread() || _busy) return false;
    _busy = true;
    return true;
}

void FleetFormationQueue::_leave()
{
    _busy = false;
    if (!_refreshRequested) return;
    _refreshRequested = false;
    const QPointer<FleetFormationQueue> self(this);
    QTimer::singleShot(0, this, [self]() { if (self) self->_poll(); });
}

void FleetFormationQueue::_poll()
{
    if (thread() != QThread::currentThread()) return;
    if (_busy) { _refreshRequested = true; return; }
    const QPointer<FleetFormationQueue> self(this);
    _busy = true;
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (self) _publish();
}

FleetFormationQueue::Availability FleetFormationQueue::_readAvailability() const
{
    Availability result;
    result.error = QStringLiteral("source_unavailable");
    const QPointer<FleetFormationQueue> self(const_cast<FleetFormationQueue*>(this));
    const QPointer<SwarmController> originalSwarm(_swarm);
    const QPointer<SwarmMissionOrchestrator> originalOrchestrator(_orchestrator);
    const auto current = [self, originalSwarm, originalOrchestrator]() {
        return self && self->_bound && originalSwarm && originalOrchestrator
            && self->_swarm == originalSwarm && self->_orchestrator == originalOrchestrator
            && originalSwarm->thread() == self->thread() && originalOrchestrator->thread() == self->thread()
            && originalSwarm->executionTaskOrchestratorBackend() == originalOrchestrator.data();
    };
    if (!current()) return result;
    // This call may refresh native recovery bindings and synchronously notify.
    // Never continue through a destroyed or replaced owner after that boundary.
    const QVariantMap snapshot = originalSwarm->continuityOccupancySnapshot();
    if (!current()) return result;
    QVariantMap canonical;
    if (!FleetNativeOccupancyEvidence::snapshotValid(snapshot, &canonical)) {
        result.error = QStringLiteral("native_occupancy_unavailable");
        return result;
    }
    result.identity = FleetNativeOccupancyEvidence::stableIdentity(canonical);
    if (result.identity.isEmpty()) return result;
    for (const QVariant& value : canonical.value(QStringLiteral("entries")).toList())
        result.nativeHeld.insert(value.toMap().value(QStringLiteral("vehicleId")).toInt());

    // Actual private TaskRecords, projected by the original native owner. No
    // metadata/observation copies and no caller-supplied availability maps.
    const QVariantMap tasks = originalOrchestrator->formationQueueAssignmentSnapshot();
    if (!current()) return Availability();
    if (tasks.size() != 2 || tasks.value(QStringLiteral("sourceCurrent")).userType() != QMetaType::Bool
        || !tasks.value(QStringLiteral("sourceCurrent")).toBool()
        || tasks.value(QStringLiteral("entries")).userType() != QMetaType::QVariantList
        || tasks.value(QStringLiteral("entries")).toList().size() > 255) {
        result.error = QStringLiteral("native_task_occupancy_unavailable");
        return result;
    }
    for (const QVariant& value : tasks.value(QStringLiteral("entries")).toList()) {
        if (value.userType() != QMetaType::QVariantMap) return Availability();
        const QVariantMap row = value.toMap();
        int id = 0;
        if (row.size() != 3 || !vehicleId(row.value(QStringLiteral("vehicleId")), 255, id)
            || result.taskHeld.contains(id)
            || row.value(QStringLiteral("taskId")).userType() != QMetaType::QString
            || row.value(QStringLiteral("taskId")).toString().isEmpty()
            || !boundedText(row.value(QStringLiteral("taskId")).toString(), 128)
            || row.value(QStringLiteral("taskPhase")).userType() != QMetaType::QString
            || row.value(QStringLiteral("taskPhase")).toString().isEmpty()) return Availability();
        result.taskHeld.insert(id);
        result.taskOwner.insert(id, row.value(QStringLiteral("taskId")).toString());
    }
    result.formationBlocked = originalSwarm->formationActive() || originalSwarm->formationBusy();
    if (!current()) return Availability();
    result.current = true;
    result.error.clear();
    return result;
}

int FleetFormationQueue::_find(const QString& id) const
{
    for (int index = 0; index < _entries.size(); ++index)
        if (_entries.at(index).id == id) return index;
    return -1;
}

int FleetFormationQueue::_head() const
{
    for (int index = 0; index < _entries.size(); ++index)
        if (!_terminal(_entries.at(index).state)) return index;
    return -1;
}

QString FleetFormationQueue::_blocker(const Entry& entry) const
{
    if (!_availability.current) return _availability.error.isEmpty()
        ? QStringLiteral("source_unavailable") : _availability.error;
    if (_availability.formationBlocked) return QStringLiteral("formation_active_or_busy");
    for (const QVariant& value : entry.vehicleIds) {
        const int id = value.toInt();
        if (_availability.nativeHeld.contains(id))
            return QStringLiteral("vehicle_native_occupancy_held:") + QString::number(id);
        if (_availability.taskHeld.contains(id))
            return QStringLiteral("vehicle_reserved_by_task:") + QString::number(id)
                + QLatin1Char(':') + _availability.taskOwner.value(id);
    }
    return {};
}

bool FleetFormationQueue::_advance(Entry& entry)
{
    if (_revisionExhausted || entry.revision == std::numeric_limits<quint64>::max()) {
        _revisionExhausted = true;
        _dirty = true;
        return false;
    }
    ++entry.revision;
    _dirty = true;
    return true;
}

bool FleetFormationQueue::_transition(Entry& entry, const QString& state, const QString& reason)
{
    if (entry.state == state && entry.reason == reason) return true;
    if (!_advance(entry)) return false;
    entry.state = state;
    entry.reason = reason;
    entry.reviewToken.clear();
    entry.reviewDeadline = 0;
    entry.reviewedOccupancy.clear();
    return true;
}

void FleetFormationQueue::_refresh()
{
    const QPointer<FleetFormationQueue> self(this);
    const Availability actual = _readAvailability();
    if (!self) return;
    _availability = actual;
    if (_sourceCurrent != actual.current) { _sourceCurrent = actual.current; _dirty = true; }
    _reconcile();
}

void FleetFormationQueue::_reconcile()
{
    if (_revisionExhausted) return;
    const qint64 now = _clock.elapsed();
    for (Entry& entry : _entries) {
        if (!_terminal(entry.state) && entry.state != QStringLiteral("dispatching") && now >= entry.deadline)
            if (!_transition(entry, QStringLiteral("expired"), QStringLiteral("pending_lifetime_expired"))) return;
    }
    const int head = _head();
    for (int index = 0; index < _entries.size(); ++index) {
        Entry& entry = _entries[index];
        if (_terminal(entry.state) || entry.state == QStringLiteral("dispatching")) continue;
        const QString blocker = index == head ? _blocker(entry) : QStringLiteral("queue_predecessor_pending");
        if (index == head && blocker.isEmpty() && entry.state == QStringLiteral("awaiting-confirmation")
            && now < entry.reviewDeadline && entry.reviewedOccupancy == _availability.identity) continue;
        const QString state = blocker.isEmpty() ? QStringLiteral("ready-to-review") : QStringLiteral("waiting");
        const QString reason = blocker.isEmpty() ? QStringLiteral("fresh_review_required") : blocker;
        if (!_transition(entry, state, reason)) return;
    }
}

void FleetFormationQueue::_publish()
{
    if (!_dirty) return;
    _dirty = false;
    emit changed(); // No member access after a public notification.
}

QVariantMap FleetFormationQueue::_unavailable(bool withToken)
{
    QVariantMap result{{QStringLiteral("ok"), false}, {QStringLiteral("queueId"), QStringLiteral("")},
        {QStringLiteral("revision"), QStringLiteral("")}, {QStringLiteral("vehicleIds"), QVariantList()},
        {QStringLiteral("reason"), QStringLiteral("queue_busy_or_wrong_thread")}};
    if (withToken) result.insert(QStringLiteral("reviewToken"), QStringLiteral(""));
    return result;
}

QVariantMap FleetFormationQueue::_reply(bool ok, int index, const QString& reason, bool withToken) const
{
    const bool found = index >= 0 && index < _entries.size();
    QVariantMap result{{QStringLiteral("ok"), ok},
        {QStringLiteral("queueId"), found ? _entries.at(index).id : QStringLiteral("")},
        {QStringLiteral("revision"), found ? QString::number(_entries.at(index).revision) : QStringLiteral("")},
        {QStringLiteral("vehicleIds"), found ? _entries.at(index).vehicleIds : QVariantList()},
        {QStringLiteral("reason"), reason}};
    if (withToken) result.insert(QStringLiteral("reviewToken"), ok && found ? _entries.at(index).reviewToken : QStringLiteral(""));
    return result;
}

QVariantMap FleetFormationQueue::enqueue(const QVariantList& ids)
{
    if (!_enter()) return _unavailable();
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable();
    QVariantList canonical;
    QVariantMap result;
    if (_revisionExhausted) result = _reply(false, -1, QStringLiteral("queue_revision_exhausted"));
    else if (!_vehicleIds(ids, canonical)) result = _reply(false, -1, QStringLiteral("formation_vehicle_ids_invalid"));
    else if (_entries.size() >= QueueLimit) result = _reply(false, -1, QStringLiteral("queue_limit_reached"));
    else {
        Entry entry;
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (_find(entry.id) >= 0) result = _reply(false, -1, QStringLiteral("queue_id_collision"));
        else {
            entry.vehicleIds = canonical;
            entry.state = QStringLiteral("waiting");
            entry.reason = QStringLiteral("fresh_review_required");
            const QDateTime now = QDateTime::currentDateTimeUtc();
            entry.createdAt = now.toString(Qt::ISODateWithMs);
            entry.expiresAt = now.addMSecs(PendingLifetimeMs).toString(Qt::ISODateWithMs);
            entry.deadline = _clock.elapsed() + PendingLifetimeMs;
            _entries.append(entry);
            _dirty = true;
            _reconcile();
            result = _reply(true, _entries.size() - 1, QStringLiteral("intention_enqueued_not_approved"));
        }
    }
    _publish();
    return result;
}

QVariantMap FleetFormationQueue::beginReview(const QString& id, const QString& expectedRevision)
{
    if (!_enter()) return _unavailable(true);
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable(true);
    const int index = _find(id);
    QVariantMap result;
    if (_revisionExhausted || index < 0 || QString::number(_entries.at(index).revision) != expectedRevision)
        result = _reply(false, index, QStringLiteral("queue_entry_stale_or_missing"), true);
    else if (index != _head() || !_blocker(_entries.at(index)).isEmpty()
        || (_entries.at(index).state != QStringLiteral("ready-to-review")
            && _entries.at(index).state != QStringLiteral("awaiting-confirmation")))
        result = _reply(false, index, QStringLiteral("queue_entry_not_reviewable"), true);
    else if (!_advance(_entries[index])) result = _reply(false, index, QStringLiteral("queue_revision_exhausted"), true);
    else {
        Entry& entry = _entries[index];
        entry.state = QStringLiteral("awaiting-confirmation");
        entry.reason = QStringLiteral("review_confirmation_required");
        entry.reviewToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.reviewDeadline = _clock.elapsed() + ReviewLifetimeMs;
        entry.reviewedOccupancy = _availability.identity;
        result = _reply(true, index, QStringLiteral("review_created_not_approved"), true);
    }
    _publish();
    return result;
}

QVariantMap FleetFormationQueue::consumeReview(const QString& id, const QString& expectedRevision, const QString& token)
{
    if (!_enter()) return _unavailable();
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable();
    const int index = _find(id);
    QVariantMap result;
    if (_revisionExhausted || index < 0 || QString::number(_entries.at(index).revision) != expectedRevision)
        result = _reply(false, index, QStringLiteral("queue_entry_stale_or_missing"));
    else if (index != _head() || _entries.at(index).state != QStringLiteral("awaiting-confirmation")
        || token.isEmpty() || _entries.at(index).reviewToken != token || !_blocker(_entries.at(index)).isEmpty()
        || _entries.at(index).reviewedOccupancy != _availability.identity)
        result = _reply(false, index, QStringLiteral("queue_review_invalid_or_expired"));
    else if (!_transition(_entries[index], QStringLiteral("dispatching"), QStringLiteral("dispatch_submission_result_unknown")))
        result = _reply(false, index, QStringLiteral("queue_revision_exhausted"));
    else result = _reply(true, index, QStringLiteral("review_consumed_not_command_acknowledged"));
    _publish();
    return result;
}

QVariantMap FleetFormationQueue::finishDispatch(const QString& id, const QString& expectedRevision,
                                               const QString& taskId, bool submitted, const QString& reason)
{
    if (!_enter()) return _unavailable();
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable();
    const int index = _find(id);
    QVariantMap result;
    if (_revisionExhausted || index < 0 || QString::number(_entries.at(index).revision) != expectedRevision
        || _entries.at(index).state != QStringLiteral("dispatching"))
        result = _reply(false, index, QStringLiteral("queue_dispatch_stale_or_missing"));
    else if (!boundedText(taskId, 128) || (submitted && taskId.isEmpty()) || !boundedText(reason, 4096))
        result = _reply(false, index, QStringLiteral("queue_dispatch_result_invalid"));
    else if (!_transition(_entries[index], submitted ? QStringLiteral("submitted") : QStringLiteral("rejected"),
        reason.isEmpty() ? (submitted ? QStringLiteral("local_request_submitted") : QStringLiteral("local_request_rejected")) : reason))
        result = _reply(false, index, QStringLiteral("queue_revision_exhausted"));
    else {
        _entries[index].taskId = taskId.isEmpty() ? QStringLiteral("") : taskId;
        _reconcile();
        result = _reply(true, index, QStringLiteral("local_dispatch_result_recorded_not_remote_outcome"));
    }
    _publish();
    return result;
}

QVariantMap FleetFormationQueue::cancel(const QString& id, const QString& expectedRevision)
{
    if (!_enter()) return _unavailable();
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable();
    const int index = _find(id);
    QVariantMap result;
    if (_revisionExhausted || index < 0 || QString::number(_entries.at(index).revision) != expectedRevision)
        result = _reply(false, index, QStringLiteral("queue_entry_stale_or_missing"));
    else if (_terminal(_entries.at(index).state) || _entries.at(index).state == QStringLiteral("dispatching"))
        result = _reply(false, index, QStringLiteral("queue_entry_not_cancellable"));
    else if (!_transition(_entries[index], QStringLiteral("cancelled"), QStringLiteral("intention_cancelled_not_aircraft_command")))
        result = _reply(false, index, QStringLiteral("queue_revision_exhausted"));
    else {
        _reconcile();
        result = _reply(true, index, QStringLiteral("intention_cancelled_not_aircraft_command"));
    }
    _publish();
    return result;
}

QVariantMap FleetFormationQueue::remove(const QString& id, const QString& expectedRevision)
{
    if (!_enter()) return _unavailable();
    const QPointer<FleetFormationQueue> self(this);
    auto release = qScopeGuard([self]() { if (self) self->_leave(); });
    _refresh();
    if (!self) return _unavailable();
    const int index = _find(id);
    QVariantMap result;
    if (_revisionExhausted || index < 0 || QString::number(_entries.at(index).revision) != expectedRevision)
        result = _reply(false, index, QStringLiteral("queue_entry_stale_or_missing"));
    else if (!_terminal(_entries.at(index).state)) result = _reply(false, index, QStringLiteral("queue_entry_not_removable"));
    else {
        result = _reply(true, index, QStringLiteral("terminal_local_intention_removed"));
        _entries.removeAt(index);
        _dirty = true;
        _reconcile();
    }
    _publish();
    return result;
}

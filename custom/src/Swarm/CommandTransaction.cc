#include "CommandTransaction.h"

#include <cmath>
#include <limits>
#include <QPointer>
#include <QUuid>

namespace {
const QString kPending = QStringLiteral("pending");
const QString kSucceeded = QStringLiteral("succeeded");
const QString kFailed = QStringLiteral("failed");
const QString kTimedOut = QStringLiteral("timed_out");
const QString kCancelled = QStringLiteral("cancelled");
}

CommandTransaction::CommandTransaction(const QString& command,
                                       const QVariantList& targetVehicleIds,
                                       QObject* parent)
    : QObject(parent)
    , _transactionId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , _command(command.trimmed())
{
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, &CommandTransaction::_handleTimeout);

    // Preserve the original request for diagnostics, but never silently
    // shrink a target set.  A malformed or duplicate protocol identity makes
    // the whole transaction invalid; start() will fail before any dispatch.
    _targetVehicleIds = targetVehicleIds;
    for (const QVariant& vehicleId : targetVehicleIds) {
        const QString key = _vehicleKey(vehicleId);
        if (key.isEmpty() || _vehicles.contains(key)) {
            _targetInputValid = false;
            continue;
        }
        VehicleState vehicle;
        vehicle.id = vehicleId;
        vehicle.state = kPending;
        _vehicles.insert(key, vehicle);
        _vehicleOrder.append(key);
    }
}

double CommandTransaction::completionRatio() const
{
    if (_vehicleOrder.isEmpty()) {
        return 0.0;
    }
    int completed = 0;
    for (const QString& key : _vehicleOrder) {
        const QString state = _vehicles.value(key).state;
        if (state == kSucceeded || state == kFailed || state == kTimedOut || state == kCancelled) {
            ++completed;
        }
    }
    return static_cast<double>(completed) / static_cast<double>(_vehicleOrder.size());
}

bool CommandTransaction::isTerminal() const
{
    return _status == Status::Succeeded || _status == Status::PartiallySucceeded ||
           _status == Status::Failed || _status == Status::TimedOut || _status == Status::Cancelled;
}

bool CommandTransaction::canRetry() const
{
    // A validation failure before the first attempt (for example an empty
    // command) is terminal, but it is not a retryable transport attempt.  Do
    // not let beginRetry() turn that failed validation into a Running
    // transaction with no command to send.
    return _attempt > 0 && _attempt < std::numeric_limits<int>::max()
           && _retryCount < std::numeric_limits<int>::max()
           && !_command.isEmpty() && !_vehicleOrder.isEmpty()
           && isTerminal() && _retryCount < _maxRetries &&
           (_countForState(kFailed) > 0 || _countForState(kTimedOut) > 0 || _countForState(kPending) > 0);
}

void CommandTransaction::setTimeoutMs(int timeoutMs)
{
    const int normalized = qMax(0, timeoutMs);
    if (_timeoutMs == normalized) {
        return;
    }
    _timeoutMs = normalized;
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    const Status originalStatus = _status;
    emit timeoutMsChanged();
    if (!self || _attempt != originalAttempt || _status != originalStatus
        || _timeoutMs != normalized) return;
    // Keep the QTimer and the exposed timeout property in sync when a policy
    // changes during an active attempt. Otherwise the UI would display the
    // new deadline while the old timer still fired in the background.
    if (_status == Status::Running) {
        if (_timeoutMs <= 0 || !_startedAt.isValid()) {
            _stopTimer();
        } else {
            const qint64 elapsedMs = _startedAt.msecsTo(QDateTime::currentDateTimeUtc());
            const int remainingMs = elapsedMs >= _timeoutMs
                ? 1
                : qMax(1, _timeoutMs - static_cast<int>(elapsedMs));
            _timeoutTimer.start(remainingMs);
        }
    }
    emit resultChanged();
}

void CommandTransaction::setMaxRetries(int maxRetries)
{
    const int normalized = qMax(0, maxRetries);
    if (_maxRetries == normalized) {
        return;
    }
    _maxRetries = normalized;
    const QPointer<CommandTransaction> self(this);
    emit maxRetriesChanged();
    if (!self) return;
    emit retryAvailableChanged();
    if (!self) return;
    emit resultChanged();
}

bool CommandTransaction::start()
{
    if (_attemptTransitionActive || _status != Status::Pending) {
        return false;
    }
    const QPointer<CommandTransaction> self(this);
    if (_command.isEmpty()) {
        _message = QStringLiteral("Command is empty");
        _finishedAt = QDateTime::currentDateTimeUtc();
        _setStatus(Status::Failed);
        if (self) _emitResultChanges();
        return false;
    }
    if (!_targetInputValid) {
        _message = QStringLiteral("Target vehicle identity list is invalid or contains duplicates");
        _finishedAt = QDateTime::currentDateTimeUtc();
        _setStatus(Status::Failed);
        if (self) _emitResultChanges();
        return false;
    }
    if (_vehicleOrder.isEmpty()) {
        _message = QStringLiteral("No target vehicles");
        _finishedAt = QDateTime::currentDateTimeUtc();
        _setStatus(Status::Failed);
        if (self) _emitResultChanges();
        return false;
    }

    _attemptTransitionActive = true;
    _startAttempt(false);
    if (self) _attemptTransitionActive = false;
    // Accepted local transition, not continued ownership or command success.
    return true;
}

bool CommandTransaction::beginRetry()
{
    if (_attemptTransitionActive || !canRetry()) {
        return false;
    }
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    const Status originalStatus = _status;
    _attemptTransitionActive = true;
    ++_retryCount;
    emit retryCountChanged();
    if (!self) return true; // The accepted retry-count fact is not rolled back.
    if (_attempt != originalAttempt || _status != originalStatus
        || _retryCount > _maxRetries) {
        _attemptTransitionActive = false;
        return true;
    }
    _startAttempt(true);
    if (self) _attemptTransitionActive = false;
    return true;
}

bool CommandTransaction::markVehicleSucceeded(const QVariant& vehicleId, const QVariantMap& details)
{
    return _setVehicleState(vehicleId, kSucceeded, QString(), details);
}

bool CommandTransaction::markVehicleFailed(const QVariant& vehicleId, const QString& reason,
                                            const QVariantMap& details)
{
    return _setVehicleState(vehicleId, kFailed, reason, details);
}

bool CommandTransaction::markVehicleTimedOut(const QVariant& vehicleId, const QString& reason)
{
    return _setVehicleState(vehicleId, kTimedOut, reason, QVariantMap());
}

bool CommandTransaction::cancel(const QString& reason)
{
    if (isTerminal()) {
        return false;
    }
    for (const QString& key : _vehicleOrder) {
        VehicleState& vehicle = _vehicles[key];
        if (vehicle.state == kPending) {
            vehicle.state = kCancelled;
            vehicle.reason = reason;
        }
    }
    _message = reason.trimmed().isEmpty() ? QStringLiteral("Cancelled") : reason.trimmed();
    _stopTimer();
    _finishedAt = QDateTime::currentDateTimeUtc();
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    _setStatus(Status::Cancelled);
    if (self && _attempt == originalAttempt && _status == Status::Cancelled) _emitResultChanges();
    return true;
}

bool CommandTransaction::checkTimeout()
{
    if (_status != Status::Running || !_timeoutTimer.isActive() || _timeoutMs <= 0 || !_startedAt.isValid()) {
        return false;
    }
    if (_startedAt.msecsTo(QDateTime::currentDateTimeUtc()) < _timeoutMs) {
        return false;
    }
    _handleTimeout();
    return true;
}

QVariantMap CommandTransaction::vehicleResult(const QVariant& vehicleId) const
{
    const QString key = _vehicleKey(vehicleId);
    if (key.isEmpty() || !_vehicles.contains(key)) {
        return QVariantMap();
    }
    const VehicleState& vehicle = _vehicles.value(key);
    QVariantMap result;
    result.insert(QStringLiteral("vehicleId"), vehicle.id);
    result.insert(QStringLiteral("state"), vehicle.state);
    result.insert(QStringLiteral("reason"), vehicle.reason);
    result.insert(QStringLiteral("details"), vehicle.details);
    result.insert(QStringLiteral("attempt"), _attempt);
    return result;
}

QString CommandTransaction::statusToString(Status status)
{
    switch (status) {
    case Status::Pending: return QStringLiteral("pending");
    case Status::Running: return QStringLiteral("running");
    case Status::Succeeded: return QStringLiteral("succeeded");
    case Status::PartiallySucceeded: return QStringLiteral("partially_succeeded");
    case Status::Failed: return QStringLiteral("failed");
    case Status::TimedOut: return QStringLiteral("timed_out");
    case Status::Cancelled: return QStringLiteral("cancelled");
    }
    return QStringLiteral("failed");
}

void CommandTransaction::_handleTimeout()
{
    // QTimer::stop()/start() can leave an already queued timeout callback in
    // the event queue.  Re-check the current deadline here so a stale event
    // cannot time out a transaction after the timeout policy was disabled or
    // extended while the attempt was running.
    if (_status != Status::Running || _timeoutMs <= 0 || !_startedAt.isValid()) {
        return;
    }
    const qint64 elapsedMs = _startedAt.msecsTo(QDateTime::currentDateTimeUtc());
    if (elapsedMs < _timeoutMs) {
        _timeoutTimer.start(qMax<qint64>(1, _timeoutMs - elapsedMs));
        return;
    }
    for (const QString& key : _vehicleOrder) {
        VehicleState& vehicle = _vehicles[key];
        if (vehicle.state == kPending) {
            vehicle.state = kTimedOut;
            vehicle.reason = QStringLiteral("Transaction timeout");
        }
    }
    _message = QStringLiteral("Transaction timeout");
    _finishedAt = QDateTime::currentDateTimeUtc();
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    const int originalRetryCount = _retryCount;
    // Preserve a partial-success status when some vehicles completed before
    // the deadline. The per-vehicle timed_outIds still make the timeout
    // explicit to callers.
    _finishCurrentAttempt();
    if (!self || _attempt != originalAttempt || _retryCount != originalRetryCount
        || !isTerminal() || _status == Status::Cancelled) return;
    const Status finishedStatus = _status;
    if (_status != Status::Succeeded) {
        _message = QStringLiteral("Transaction timeout");
        _emitResultChanges();
        if (!self || _attempt != originalAttempt || _retryCount != originalRetryCount
            || _status != finishedStatus) return;
    }
    emit timeoutReached();
    if (!self || _attempt != originalAttempt || _retryCount != originalRetryCount
        || _status != finishedStatus) return;
    if (canRetry()) {
        const int nextAttempt = originalAttempt + 1; // canRetry excludes INT_MAX.
        emit retryRequested(nextAttempt);
    }
}

QString CommandTransaction::_vehicleKey(const QVariant& vehicleId) const
{
    if (!vehicleId.isValid() || vehicleId.isNull()) {
        return QString();
    }

    // Vehicle identity is a protocol address, not an arbitrary display
    // string.  Keep the transaction key aligned with Registry/Health/mission
    // identity rules so values such as "01", true, or "vehicle-1" cannot
    // create an alias that later ACKs resolve to a different target.
    bool numeric = false;
    double value = 0.0;
    switch (vehicleId.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        value = vehicleId.toDouble(&numeric);
        break;
    default:
        break;
    }
    if (!numeric || !std::isfinite(value) || std::floor(value) != value
            || value < 1.0 || value > 255.0) {
        return QString();
    }
    return QString::number(static_cast<int>(value));
}

bool CommandTransaction::_setVehicleState(const QVariant& vehicleId, const QString& state,
                                          const QString& reason, const QVariantMap& details)
{
    if (_status != Status::Running) {
        return false;
    }
    const QString key = _vehicleKey(vehicleId);
    if (key.isEmpty() || !_vehicles.contains(key)) {
        return false;
    }
    VehicleState& vehicle = _vehicles[key];
    if (vehicle.state == kSucceeded || vehicle.state == kFailed || vehicle.state == kTimedOut ||
        vehicle.state == kCancelled) {
        return false;
    }
    vehicle.state = state;
    vehicle.reason = reason.trimmed();
    vehicle.details = details;
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    _emitResultChanges();
    if (!self || _attempt != originalAttempt || _status != Status::Running) return true;
    _finishCurrentAttempt();
    return true;
}

void CommandTransaction::_startAttempt(bool resetRetryableVehicles)
{
    // A synchronous observer may cancel, finish, or destroy this attempt.
    // The original stack must not resurrect it or arm a newer attempt's timer.
    const QPointer<CommandTransaction> self(this);
    const Status previousStatus = _status;
    const int originalRetryCount = _retryCount;
    if (_attempt == std::numeric_limits<int>::max()) return;
    if (resetRetryableVehicles) {
        for (const QString& key : _vehicleOrder) {
            VehicleState& vehicle = _vehicles[key];
            if (vehicle.state == kFailed || vehicle.state == kTimedOut || vehicle.state == kPending) {
                vehicle.state = kPending;
                vehicle.reason.clear();
                vehicle.details.clear();
            }
        }
    }
    const int originalAttempt = ++_attempt;
    _startedAt = QDateTime::currentDateTimeUtc();
    _message.clear();
    _finishedAt = QDateTime();
    emit attemptChanged();
    if (!self || _attempt != originalAttempt || _retryCount != originalRetryCount
        || _status != previousStatus) return;
    _setStatus(Status::Running);
    if (!self || _attempt != originalAttempt || _retryCount != originalRetryCount
        || _status != Status::Running) return;
    if (_timeoutMs > 0) {
        _timeoutTimer.start(_timeoutMs);
    }
    _emitResultChanges();
}

void CommandTransaction::_stopTimer()
{
    if (_timeoutTimer.isActive()) {
        _timeoutTimer.stop();
    }
}

void CommandTransaction::_finishCurrentAttempt()
{
    if (_status != Status::Running || _countForState(kPending) > 0) {
        return;
    }
    _stopTimer();
    const int successes = _countForState(kSucceeded);
    const int failures = _countForState(kFailed);
    const int timeouts = _countForState(kTimedOut);
    _finishedAt = QDateTime::currentDateTimeUtc();
    Status finishedStatus;
    if (successes == _vehicleOrder.size()) {
        _message = QStringLiteral("All target vehicles succeeded");
        finishedStatus = Status::Succeeded;
    } else if (successes > 0) {
        _message = QStringLiteral("Some target vehicles succeeded");
        finishedStatus = Status::PartiallySucceeded;
    } else if (timeouts > 0 && failures == 0) {
        _message = QStringLiteral("All target vehicles timed out");
        finishedStatus = Status::TimedOut;
    } else {
        _message = QStringLiteral("All target vehicles failed");
        finishedStatus = Status::Failed;
    }
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    _setStatus(finishedStatus);
    if (!self || _attempt != originalAttempt || _status != finishedStatus) return;
    _emitResultChanges();
}

void CommandTransaction::_setStatus(Status status)
{
    if (_status == status) {
        return;
    }
    _status = status;
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    emit statusChanged();
    if (!self || _attempt != originalAttempt || _status != status) return;
    emit retryAvailableChanged();
}

void CommandTransaction::_emitResultChanges()
{
    const QPointer<CommandTransaction> self(this);
    const int originalAttempt = _attempt;
    const Status originalStatus = _status;
    emit progressChanged();
    if (!self || _attempt != originalAttempt || _status != originalStatus) return;
    emit resultChanged();
}

QVariantList CommandTransaction::_idsForState(const QString& state) const
{
    QVariantList ids;
    for (const QString& key : _vehicleOrder) {
        const VehicleState& vehicle = _vehicles.value(key);
        if (vehicle.state == state) {
            ids.append(vehicle.id);
        }
    }
    return ids;
}

int CommandTransaction::_countForState(const QString& state) const
{
    int count = 0;
    for (const QString& key : _vehicleOrder) {
        if (_vehicles.value(key).state == state) {
            ++count;
        }
    }
    return count;
}

QString CommandTransaction::_summaryMessage() const
{
    if (!_message.isEmpty()) {
        return _message;
    }
    return _status == Status::Pending ? QStringLiteral("Not started") : QString();
}

QVariantMap CommandTransaction::resultSummary() const
{
    QVariantMap summary;
    summary.insert(QStringLiteral("transactionId"), _transactionId);
    summary.insert(QStringLiteral("command"), _command);
    summary.insert(QStringLiteral("status"), status());
    // Keep the serialized transaction contract aligned with the Q_PROPERTY
    // and with SwarmController's delayed-callback guards. Temporary Mission
    // callbacks consume summaries after a transaction may have been archived;
    // without this explicit field an archived failed/timeout transaction was
    // indistinguishable from a still-running one and its onboard mission was
    // not cleared deterministically.
    summary.insert(QStringLiteral("terminal"), isTerminal());
    summary.insert(QStringLiteral("attempt"), _attempt);
    summary.insert(QStringLiteral("retryCount"), _retryCount);
    summary.insert(QStringLiteral("maxRetries"), _maxRetries);
    summary.insert(QStringLiteral("timeoutMs"), _timeoutMs);
    summary.insert(QStringLiteral("targetInputValid"), _targetInputValid);
    summary.insert(QStringLiteral("requestedTargetCount"), _targetVehicleIds.size());
    summary.insert(QStringLiteral("targetCount"), _vehicleOrder.size());
    summary.insert(QStringLiteral("successCount"), _countForState(kSucceeded));
    summary.insert(QStringLiteral("failedCount"), _countForState(kFailed));
    summary.insert(QStringLiteral("timedOutCount"), _countForState(kTimedOut));
    summary.insert(QStringLiteral("cancelledCount"), _countForState(kCancelled));
    summary.insert(QStringLiteral("pendingCount"), _countForState(kPending));
    summary.insert(QStringLiteral("completedCount"), _vehicleOrder.size() - _countForState(kPending));
    summary.insert(QStringLiteral("completionRatio"), completionRatio());
    summary.insert(QStringLiteral("successIds"), _idsForState(kSucceeded));
    summary.insert(QStringLiteral("failedIds"), _idsForState(kFailed));
    summary.insert(QStringLiteral("timedOutIds"), _idsForState(kTimedOut));
    summary.insert(QStringLiteral("cancelledIds"), _idsForState(kCancelled));
    summary.insert(QStringLiteral("pendingIds"), _idsForState(kPending));
    summary.insert(QStringLiteral("canRetry"), canRetry());
    summary.insert(QStringLiteral("message"), _summaryMessage());
    summary.insert(QStringLiteral("startedAt"), _startedAt.isValid() ? _startedAt.toString(Qt::ISODateWithMs) : QString());
    summary.insert(QStringLiteral("finishedAt"), _finishedAt.isValid() ? _finishedAt.toString(Qt::ISODateWithMs) : QString());

    QVariantList vehicleResults;
    for (const QString& key : _vehicleOrder) {
        vehicleResults.append(vehicleResult(_vehicles.value(key).id));
    }
    summary.insert(QStringLiteral("vehicleResults"), vehicleResults);
    return summary;
}

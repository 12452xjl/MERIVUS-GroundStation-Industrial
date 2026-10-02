#include "CommandTransaction.h"

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

    for (const QVariant& vehicleId : targetVehicleIds) {
        const QString key = _vehicleKey(vehicleId);
        if (key.isEmpty() || _vehicles.contains(key)) {
            continue;
        }
        VehicleState vehicle;
        vehicle.id = vehicleId;
        vehicle.state = kPending;
        _vehicles.insert(key, vehicle);
        _vehicleOrder.append(key);
        _targetVehicleIds.append(vehicleId);
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
    return isTerminal() && _retryCount < _maxRetries &&
           (_countForState(kFailed) > 0 || _countForState(kTimedOut) > 0 || _countForState(kPending) > 0);
}

void CommandTransaction::setTimeoutMs(int timeoutMs)
{
    const int normalized = qMax(0, timeoutMs);
    if (_timeoutMs == normalized) {
        return;
    }
    _timeoutMs = normalized;
    emit timeoutMsChanged();
    emit resultChanged();
}

void CommandTransaction::setMaxRetries(int maxRetries)
{
    const int normalized = qMax(0, maxRetries);
    if (_maxRetries == normalized) {
        return;
    }
    _maxRetries = normalized;
    emit maxRetriesChanged();
    emit retryAvailableChanged();
    emit resultChanged();
}

bool CommandTransaction::start()
{
    if (_status != Status::Pending) {
        return false;
    }
    if (_command.isEmpty()) {
        _message = QStringLiteral("Command is empty");
        _finishedAt = QDateTime::currentDateTimeUtc();
        _setStatus(Status::Failed);
        _emitResultChanges();
        return false;
    }
    if (_vehicleOrder.isEmpty()) {
        _message = QStringLiteral("No target vehicles");
        _finishedAt = QDateTime::currentDateTimeUtc();
        _setStatus(Status::Failed);
        _emitResultChanges();
        return false;
    }

    _startedAt = QDateTime::currentDateTimeUtc();
    _startAttempt(false);
    return true;
}

bool CommandTransaction::beginRetry()
{
    if (!canRetry()) {
        return false;
    }
    ++_retryCount;
    emit retryCountChanged();
    _startAttempt(true);
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
    _setStatus(Status::Cancelled);
    _emitResultChanges();
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
    if (_status != Status::Running) {
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
    // Preserve a partial-success status when some vehicles completed before
    // the deadline. The per-vehicle timed_outIds still make the timeout
    // explicit to callers.
    _finishCurrentAttempt();
    if (_status != Status::Succeeded) {
        _message = QStringLiteral("Transaction timeout");
        _emitResultChanges();
    }
    emit timeoutReached();
    if (canRetry()) {
        emit retryRequested(_attempt + 1);
    }
}

QString CommandTransaction::_vehicleKey(const QVariant& vehicleId) const
{
    if (!vehicleId.isValid() || vehicleId.isNull()) {
        return QString();
    }
    return vehicleId.toString().trimmed();
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
    _emitResultChanges();
    _finishCurrentAttempt();
    return true;
}

void CommandTransaction::_startAttempt(bool resetRetryableVehicles)
{
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
    ++_attempt;
    emit attemptChanged();
    _startedAt = QDateTime::currentDateTimeUtc();
    _message.clear();
    _finishedAt = QDateTime();
    _setStatus(Status::Running);
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
    if (successes == _vehicleOrder.size()) {
        _message = QStringLiteral("All target vehicles succeeded");
        _setStatus(Status::Succeeded);
    } else if (successes > 0) {
        _message = QStringLiteral("Some target vehicles succeeded");
        _setStatus(Status::PartiallySucceeded);
    } else if (timeouts > 0 && failures == 0) {
        _message = QStringLiteral("All target vehicles timed out");
        _setStatus(Status::TimedOut);
    } else {
        _message = QStringLiteral("All target vehicles failed");
        _setStatus(Status::Failed);
    }
    _emitResultChanges();
}

void CommandTransaction::_setStatus(Status status)
{
    if (_status == status) {
        return;
    }
    _status = status;
    emit statusChanged();
    emit retryAvailableChanged();
}

void CommandTransaction::_emitResultChanges()
{
    emit progressChanged();
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
    summary.insert(QStringLiteral("attempt"), _attempt);
    summary.insert(QStringLiteral("retryCount"), _retryCount);
    summary.insert(QStringLiteral("maxRetries"), _maxRetries);
    summary.insert(QStringLiteral("timeoutMs"), _timeoutMs);
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

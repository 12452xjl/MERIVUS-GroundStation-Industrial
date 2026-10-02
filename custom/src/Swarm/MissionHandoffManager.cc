#include "MissionHandoffManager.h"

namespace {

QString normalize(const QString& value)
{
    return value.trimmed();
}

}

MissionHandoffManager::MissionHandoffManager(QObject* parent)
    : QObject(parent)
{
    _expiryTimer.setInterval(1000);
    connect(&_expiryTimer, &QTimer::timeout, this, &MissionHandoffManager::_expireDueHandoffs);
    _expiryTimer.start();
}

void MissionHandoffManager::_expireDueHandoffs()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QStringList ids = _order;
    for (const QString& id : ids) {
        const Record* record = find(id);
        if (record && record->status == Status::Pending && record->expiresAt.isValid()
                && record->expiresAt <= now) {
            expireHandoff(id, QStringLiteral("handoff request expired"));
        }
    }
}

QString MissionHandoffManager::statusToString(Status status)
{
    switch (status) {
    case Status::Pending: return QStringLiteral("pending");
    case Status::Accepted: return QStringLiteral("accepted");
    case Status::Rejected: return QStringLiteral("rejected");
    case Status::Cancelled: return QStringLiteral("cancelled");
    case Status::Expired: return QStringLiteral("expired");
    }
    return QStringLiteral("pending");
}

QString MissionHandoffManager::timestamp(const QDateTime& value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODateWithMs) : QString();
}

QVariantMap MissionHandoffManager::toSummary(const Record& record) const
{
    QVariantMap result;
    result[QStringLiteral("handoffId")] = record.id;
    result[QStringLiteral("taskId")] = record.taskId;
    result[QStringLiteral("fromOperator")] = record.fromOperator;
    result[QStringLiteral("toOperator")] = record.toOperator;
    result[QStringLiteral("reason")] = record.reason;
    result[QStringLiteral("note")] = record.note;
    result[QStringLiteral("metadata")] = record.metadata;
    result[QStringLiteral("status")] = statusToString(record.status);
    result[QStringLiteral("requiresSecondApproval")] = record.requiresSecondApproval;
    result[QStringLiteral("createdAt")] = timestamp(record.createdAt);
    result[QStringLiteral("expiresAt")] = timestamp(record.expiresAt);
    result[QStringLiteral("resolvedAt")] = timestamp(record.resolvedAt);
    result[QStringLiteral("resolvedBy")] = record.resolvedBy;
    // This explicit field prevents a UI handoff acknowledgement from being
    // misinterpreted as a MAVLink command acknowledgement.
    result[QStringLiteral("flightCommandReleased")] = false;
    return result;
}

MissionHandoffManager::Record* MissionHandoffManager::find(const QString& handoffId)
{
    const auto it = _records.find(handoffId.trimmed());
    return it == _records.end() ? nullptr : &it.value();
}

const MissionHandoffManager::Record* MissionHandoffManager::find(const QString& handoffId) const
{
    const auto it = _records.constFind(handoffId.trimmed());
    return it == _records.constEnd() ? nullptr : &it.value();
}

int MissionHandoffManager::pendingCount() const
{
    int count = 0;
    for (const Record& record : _records) {
        if (record.status == Status::Pending) {
            ++count;
        }
    }
    return count;
}

QVariantList MissionHandoffManager::handoffs() const
{
    QVariantList result;
    for (const QString& id : _order) {
        const Record* record = find(id);
        if (record) {
            result.append(toSummary(*record));
        }
    }
    return result;
}

QString MissionHandoffManager::requestHandoff(const QString& taskId,
                                              const QString& fromOperator,
                                              const QString& toOperator,
                                              const QString& reason,
                                              const QVariantMap& metadata,
                                              bool requiresSecondApproval,
                                              int expirySeconds)
{
    const QString task = normalize(taskId);
    const QString from = normalize(fromOperator);
    const QString to = normalize(toOperator);
    if (task.isEmpty() || from.isEmpty() || to.isEmpty() || from == to) {
        return QString();
    }

    Record record;
    record.id = QStringLiteral("handoff-%1-%2")
            .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddhhmmsszzz")))
            .arg(++_sequence);
    record.taskId = task;
    record.fromOperator = from;
    record.toOperator = to;
    record.reason = normalize(reason);
    record.metadata = metadata;
    record.status = Status::Pending;
    record.requiresSecondApproval = requiresSecondApproval;
    record.createdAt = QDateTime::currentDateTimeUtc();
    record.expiresAt = record.createdAt.addSecs(qMax(1, expirySeconds));
    _records.insert(record.id, record);
    _order.append(record.id);
    emit handoffRequested(record.id, toSummary(record));
    emit handoffCollectionChanged();
    return record.id;
}

bool MissionHandoffManager::resolve(const QString& handoffId, Status status,
                                    const QString& operatorId, const QString& note)
{
    Record* record = find(handoffId);
    if (!record || record->status != Status::Pending) {
        return false;
    }
    if (status != Status::Expired && normalize(operatorId).isEmpty()) {
        return false;
    }
    record->status = status;
    record->resolvedBy = normalize(operatorId);
    record->note = normalize(note);
    record->resolvedAt = QDateTime::currentDateTimeUtc();
    const QVariantMap summary = toSummary(*record);
    switch (status) {
    case Status::Accepted: emit handoffAccepted(record->id, summary); break;
    case Status::Rejected: emit handoffRejected(record->id, summary); break;
    case Status::Cancelled: emit handoffCancelled(record->id, summary); break;
    case Status::Expired: emit handoffExpired(record->id, summary); break;
    case Status::Pending: break;
    }
    emit handoffCollectionChanged();
    return true;
}

bool MissionHandoffManager::acceptHandoff(const QString& handoffId,
                                          const QString& acceptingOperator,
                                          const QString& note)
{
    Record* record = find(handoffId);
    if (!record || normalize(acceptingOperator) != record->toOperator) {
        return false;
    }
    return resolve(handoffId, Status::Accepted, acceptingOperator, note);
}

bool MissionHandoffManager::rejectHandoff(const QString& handoffId,
                                          const QString& rejectingOperator,
                                          const QString& note)
{
    Record* record = find(handoffId);
    if (!record || normalize(rejectingOperator) != record->toOperator) {
        return false;
    }
    return resolve(handoffId, Status::Rejected, rejectingOperator, note);
}

bool MissionHandoffManager::cancelHandoff(const QString& handoffId,
                                          const QString& cancellingOperator,
                                          const QString& note)
{
    Record* record = find(handoffId);
    if (!record || normalize(cancellingOperator) != record->fromOperator) {
        return false;
    }
    return resolve(handoffId, Status::Cancelled, cancellingOperator, note);
}

bool MissionHandoffManager::expireHandoff(const QString& handoffId, const QString& note)
{
    Record* record = find(handoffId);
    if (!record || record->status != Status::Pending) {
        return false;
    }
    return resolve(handoffId, Status::Expired, QString(), note);
}

QVariantMap MissionHandoffManager::handoff(const QString& handoffId) const
{
    const Record* record = find(handoffId);
    return record ? toSummary(*record) : QVariantMap();
}

bool MissionHandoffManager::hasHandoff(const QString& handoffId) const
{
    return find(handoffId) != nullptr;
}

bool MissionHandoffManager::isPending(const QString& handoffId) const
{
    const Record* record = find(handoffId);
    return record && record->status == Status::Pending;
}

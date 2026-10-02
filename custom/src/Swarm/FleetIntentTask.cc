#include "FleetIntentTask.h"

#include <QUuid>

#include "FleetCapabilityMatcher.h"

FleetIntentTask::FleetIntentTask(QObject* parent)
    : QObject(parent), _taskId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

void FleetIntentTask::setIntent(const QString& value)
{
    if (_intent == value) return;
    _intent = value.trimmed();
    emit intentChanged();
}

void FleetIntentTask::setParameters(const QVariantMap& value)
{
    if (_parameters == value) return;
    _parameters = value;
    emit parametersChanged();
}

void FleetIntentTask::preparePreview()
{
    FleetCapabilityMatcher matcher;
    const QVariantList fleet = _parameters.value(QStringLiteral("fleet")).toList();
    const QVariantMap matchingPlan = matcher.plan(_parameters, fleet);
    _preview = matchingPlan;
    _preview.insert(QStringLiteral("taskId"), _taskId);
    _preview.insert(QStringLiteral("intent"), _intent);
    _preview.insert(QStringLiteral("parameters"), _parameters);
    _preview.insert(QStringLiteral("execution"), QStringLiteral("manual-approval-required"));
    _preview.insert(QStringLiteral("dispatch"), QStringLiteral("not-released"));
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("not-requested"));
    _state = QStringLiteral("preview");
    _approvalReason.clear();
    emit previewChanged();
    emit stateChanged();
}

bool FleetIntentTask::requestApproval()
{
    if (_state != QStringLiteral("preview") || _intent.isEmpty()) return false;
    _state = QStringLiteral("awaiting-approval");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("awaiting-approval"));
    emit previewChanged();
    emit stateChanged();
    return true;
}

bool FleetIntentTask::approve()
{
    if (_state != QStringLiteral("awaiting-approval")) return false;
    _state = QStringLiteral("approved");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("approved"));
    _preview.insert(QStringLiteral("flightCommandReleased"), false);
    emit previewChanged();
    emit stateChanged();
    return true;
}

bool FleetIntentTask::reject(const QString& reason)
{
    if (_state != QStringLiteral("awaiting-approval")) return false;
    _approvalReason = reason.trimmed();
    _state = QStringLiteral("rejected");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("rejected"));
    _preview.insert(QStringLiteral("approvalReason"), _approvalReason);
    emit previewChanged();
    emit stateChanged();
    return true;
}

void FleetIntentTask::cancel()
{
    _state = QStringLiteral("cancelled");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("cancelled"));
    _preview.insert(QStringLiteral("flightCommandReleased"), false);
    emit previewChanged();
    emit stateChanged();
}

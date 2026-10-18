#include "FleetIntentTask.h"

#include <QJsonValue>
#include <QPointer>
#include <QStringList>
#include <QUuid>

#include <limits>

#include "FleetAirspaceConflictChecker.h"
#include "FleetCapabilityMatcher.h"
#include "FleetRiskRadar.h"

namespace {

bool strictBool(const QVariant& value, bool* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        return false;
    }
    *output = value.toBool();
    return true;
}

QVariantMap noFlyZoneInputCheck(const QVariantMap& parameters)
{
    if (parameters.contains(QStringLiteral("noFlyZones")))
        return FleetAirspaceConflictChecker::inspectNoFlyZoneInput(parameters.value(QStringLiteral("noFlyZones")));
    // Missing input retains the legacy planning workflow, not a claim that
    // zones were loaded, checked, empty, current, or complete.
    return {{QStringLiteral("valid"), true}, {QStringLiteral("state"), QStringLiteral("not-provided")},
        {QStringLiteral("reason"), QStringLiteral("")},
        {QStringLiteral("zoneCount"), QJsonValue(QJsonValue::Null).toVariant()},
        {QStringLiteral("fingerprint"), QStringLiteral("")}};
}

bool noFlyZoneInputValid(const QVariantMap& parameters)
{
    const QVariantMap checked = noFlyZoneInputCheck(parameters);
    bool valid = false;
    return strictBool(checked.value(QStringLiteral("valid")), &valid) && valid;
}

}

FleetIntentTask::FleetIntentTask(QObject* parent)
    : QObject(parent), _taskId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

void FleetIntentTask::setIntent(const QString& value)
{
    const QString normalized = value.trimmed();
    if (_intent == normalized || _revision == std::numeric_limits<quint64>::max()) return;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    const bool invalidateWorkflow = _state != QStringLiteral("draft") || !_preview.isEmpty();
    _intent = normalized;
    // Commit the whole input transition before notifications. A callback
    // must never observe a new input paired with the previous approval.
    if (invalidateWorkflow) {
        _state = QStringLiteral("draft");
        _preview.clear();
        _approvalReason.clear();
    }
    emit intentChanged();
    if (!self || self->_revision != revision) return;
    if (invalidateWorkflow) {
        emit previewChanged();
        if (!self || self->_revision != revision) return;
        emit stateChanged();
    }
}

void FleetIntentTask::setParameters(const QVariantMap& value)
{
    if (_parameters == value || _revision == std::numeric_limits<quint64>::max()) return;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    const bool invalidateWorkflow = _state != QStringLiteral("draft") || !_preview.isEmpty();
    _parameters = value;
    if (invalidateWorkflow) {
        _state = QStringLiteral("draft");
        _preview.clear();
        _approvalReason.clear();
    }
    emit parametersChanged();
    if (!self || self->_revision != revision) return;
    if (invalidateWorkflow) {
        emit previewChanged();
        if (!self || self->_revision != revision) return;
        emit stateChanged();
    }
}

void FleetIntentTask::preparePreview()
{
    if (_revision == std::numeric_limits<quint64>::max()) return;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    const QVariantMap parameters = _parameters;
    const QString intent = _intent;
    FleetCapabilityMatcher matcher;
    const QVariantList fleet = parameters.value(QStringLiteral("fleet")).toList();
    QVariantMap matchingPlan = matcher.plan(parameters, fleet);
    const QVariantMap airspaceInput = noFlyZoneInputCheck(parameters);
    bool zonesValid = false;
    const bool zonesAccepted = strictBool(airspaceInput.value(QStringLiteral("valid")), &zonesValid) && zonesValid;
    QVariantList risks = matchingPlan.value(QStringLiteral("riskSignals")).toList();
    if (!zonesAccepted) {
        risks.append(QVariantMap{{QStringLiteral("code"), QStringLiteral("invalid_no_fly_zones_input")},
            {QStringLiteral("reason"), airspaceInput.value(QStringLiteral("reason"))},
            {QStringLiteral("severity"), QStringLiteral("blocking")}, {QStringLiteral("weight"), 100}});
        matchingPlan.insert(QStringLiteral("feasible"), false);
        matchingPlan.insert(QStringLiteral("parametersValid"), false);
        matchingPlan.insert(QStringLiteral("needsReview"), true);
        matchingPlan.insert(QStringLiteral("reviewStatus"), QStringLiteral("blocked-invalid-airspace-input"));
        matchingPlan.insert(QStringLiteral("riskSignals"), risks);
        QStringList explanations = matchingPlan.value(QStringLiteral("explanations")).toStringList();
        explanations.append(QStringLiteral("invalid_no_fly_zones_input: ")
            + airspaceInput.value(QStringLiteral("reason")).toString());
        matchingPlan.insert(QStringLiteral("explanations"), explanations);
    }
    FleetRiskRadar riskRadar;
    QVariantMap riskAssessment = riskRadar.assess(parameters, risks);
    if (!zonesAccepted) {
        riskAssessment.insert(QStringLiteral("inputValid"), false);
        riskAssessment.insert(QStringLiteral("needsReview"), true);
        riskAssessment.insert(QStringLiteral("requiresConfirmation"), true);
    }
    if (!self || self->_revision != revision) return;
    _preview = matchingPlan;
    _preview.insert(QStringLiteral("taskId"), _taskId);
    _preview.insert(QStringLiteral("intent"), intent);
    _preview.insert(QStringLiteral("parameters"), parameters);
    _preview.insert(QStringLiteral("riskAssessment"), riskAssessment);
    _preview.insert(QStringLiteral("noFlyZonesInputCheck"), airspaceInput);
    _preview.insert(QStringLiteral("execution"), QStringLiteral("manual-approval-required"));
    _preview.insert(QStringLiteral("dispatch"), QStringLiteral("not-released"));
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("not-requested"));
    _state = QStringLiteral("preview");
    _approvalReason.clear();
    emit previewChanged();
    if (!self || self->_revision != revision) return;
    emit stateChanged();
}

bool FleetIntentTask::requestApproval()
{
    if (_revision == std::numeric_limits<quint64>::max()
        || _state != QStringLiteral("preview") || _intent.isEmpty() || !noFlyZoneInputValid(_parameters)) return false;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    _state = QStringLiteral("awaiting-approval");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("awaiting-approval"));
    emit previewChanged();
    if (!self || self->_revision != revision) return false;
    emit stateChanged();
    return self && self->_revision == revision && self->_state == QStringLiteral("awaiting-approval");
}

bool FleetIntentTask::approve()
{
    const QVariantMap riskAssessment = _preview.value(QStringLiteral("riskAssessment")).toMap();
    const QVariant riskInputValid = riskAssessment.value(QStringLiteral("inputValid"));
    const bool riskEvidenceValid = !riskInputValid.isValid()
        || (riskInputValid.type() == QVariant::Bool && riskInputValid.toBool());
    const QVariant feasibleValue = _preview.value(QStringLiteral("feasible"));
    bool feasible = false;
    const bool feasibleEvidenceValid = strictBool(feasibleValue, &feasible) && feasible;
    if (_revision == std::numeric_limits<quint64>::max()
        || _state != QStringLiteral("awaiting-approval")
        || !feasibleEvidenceValid
        || !riskEvidenceValid
        || !noFlyZoneInputValid(_parameters)) {
        return false;
    }
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    _state = QStringLiteral("approved");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("approved"));
    _preview.insert(QStringLiteral("flightCommandReleased"), false);
    emit previewChanged();
    if (!self || self->_revision != revision) return false;
    emit stateChanged();
    return self && self->_revision == revision && self->_state == QStringLiteral("approved");
}

bool FleetIntentTask::reject(const QString& reason)
{
    if (_state != QStringLiteral("awaiting-approval") || _revision == std::numeric_limits<quint64>::max()) return false;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    _approvalReason = reason.trimmed();
    _state = QStringLiteral("rejected");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("rejected"));
    _preview.insert(QStringLiteral("approvalReason"), _approvalReason);
    emit previewChanged();
    if (!self || self->_revision != revision) return false;
    emit stateChanged();
    return self && self->_revision == revision && self->_state == QStringLiteral("rejected");
}

void FleetIntentTask::cancel()
{
    if (_revision == std::numeric_limits<quint64>::max()) return;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    _state = QStringLiteral("cancelled");
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("cancelled"));
    _preview.insert(QStringLiteral("flightCommandReleased"), false);
    emit previewChanged();
    if (!self || self->_revision != revision) return;
    emit stateChanged();
}

bool FleetIntentTask::invalidateReview(const QString& reason)
{
    if (_revision == std::numeric_limits<quint64>::max()
        || (_state != QStringLiteral("awaiting-approval") && _state != QStringLiteral("approved"))) return false;
    const QPointer<FleetIntentTask> self(this);
    const quint64 revision = ++_revision;
    _state = QStringLiteral("preview");
    _approvalReason = reason.left(1024).trimmed();
    _preview.insert(QStringLiteral("approvalState"), QStringLiteral("invalidated"));
    _preview.insert(QStringLiteral("approvalReason"), _approvalReason);
    _preview.insert(QStringLiteral("flightCommandReleased"), false);
    emit previewChanged();
    if (!self || self->_revision != revision) return false;
    emit stateChanged();
    return self && self->_revision == revision && self->_state == QStringLiteral("preview");
}

bool FleetIntentTask::matchesInputs(const QString& intent, const QVariantMap& parameters) const
{
    return _intent == intent.trimmed() && _parameters == parameters;
}

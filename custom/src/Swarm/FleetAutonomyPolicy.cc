#include "FleetAutonomyPolicy.h"

FleetAutonomyPolicy::FleetAutonomyPolicy(QObject* parent)
    : QObject(parent)
{
    StageDefinition manual;
    manual.id = QStringLiteral("manual");
    manual.displayName = QStringLiteral("人工");
    manual.authority = QStringLiteral("human_operator");
    manual.permissions = {QStringLiteral("view.*"), QStringLiteral("task.preview"),
                          QStringLiteral("vehicle.command")};
    manual.allowedRoles = {QStringLiteral("operator"), QStringLiteral("commander"),
                           QStringLiteral("administrator")};
    manual.requiredApproval = QStringLiteral("operator_confirmation");
    manual.commandReleaseMode = QStringLiteral("human_ui_only");
    manual.humanApprovalRequired = true;
    _stages.append(manual);

    StageDefinition assisted;
    assisted.id = QStringLiteral("assisted");
    assisted.displayName = QStringLiteral("辅助");
    assisted.authority = QStringLiteral("ai_proposal_human_decision");
    assisted.permissions = {QStringLiteral("view.*"), QStringLiteral("task.preview"),
                            QStringLiteral("proposal.present"), QStringLiteral("risk.explain")};
    assisted.allowedRoles = {QStringLiteral("planner"), QStringLiteral("operator"),
                             QStringLiteral("commander"), QStringLiteral("administrator")};
    assisted.requiredApproval = QStringLiteral("human_confirmation");
    assisted.commandReleaseMode = QStringLiteral("proposal_only");
    assisted.humanApprovalRequired = true;
    assisted.sandboxRequired = true;
    _stages.append(assisted);

    StageDefinition supervised;
    supervised.id = QStringLiteral("supervised");
    supervised.displayName = QStringLiteral("监督");
    supervised.authority = QStringLiteral("bounded_automation_human_supervision");
    supervised.permissions = {QStringLiteral("view.*"), QStringLiteral("task.preview"),
                              QStringLiteral("proposal.present"), QStringLiteral("risk.explain"),
                              QStringLiteral("task.approve"), QStringLiteral("automation.supervise")};
    supervised.allowedRoles = {QStringLiteral("commander"), QStringLiteral("safety_officer"),
                               QStringLiteral("administrator")};
    supervised.requiredApproval = QStringLiteral("two_person_approval");
    supervised.commandReleaseMode = QStringLiteral("bounded_review_only");
    supervised.automationAllowed = true;
    supervised.humanApprovalRequired = true;
    supervised.twoPersonApprovalRequired = true;
    supervised.signatureRequired = true;
    supervised.sandboxRequired = true;
    _stages.append(supervised);

    StageDefinition automated;
    automated.id = QStringLiteral("automated");
    automated.displayName = QStringLiteral("自动化");
    automated.authority = QStringLiteral("bounded_policy_automation");
    automated.permissions = {QStringLiteral("view.*"), QStringLiteral("task.preview"),
                             QStringLiteral("proposal.present"), QStringLiteral("risk.explain"),
                             QStringLiteral("task.approve"), QStringLiteral("automation.run")};
    automated.allowedRoles = {QStringLiteral("administrator")};
    automated.requiredApproval = QStringLiteral("two_person_approval_and_signed_adapter");
    automated.commandReleaseMode = QStringLiteral("disabled_in_current_build");
    automated.automationAllowed = true;
    automated.humanApprovalRequired = true;
    automated.twoPersonApprovalRequired = true;
    automated.signatureRequired = true;
    automated.sandboxRequired = true;
    _stages.append(automated);

    AdapterDefinition px4;
    px4.id = QStringLiteral("px4");
    px4.displayName = QStringLiteral("PX4 飞控适配器");
    px4.flightController = QStringLiteral("px4");
    px4.extensionType = QStringLiteral("flight_controller_adapter");
    px4.capabilities = {QStringLiteral("telemetry"), QStringLiteral("health.snapshot"),
                        QStringLiteral("mission.preview"), QStringLiteral("configuration.read"),
                        QStringLiteral("command.preview")};
    px4.permissionScopes = {QStringLiteral("view.telemetry"), QStringLiteral("view.capabilities"),
                            QStringLiteral("task.preview")};
    px4.signatureStatus = QStringLiteral("unverified");
    px4.sandboxProfile = QStringLiteral("process_isolated_pending");
    _adapters.append(px4);

    AdapterDefinition ardupilot;
    ardupilot.id = QStringLiteral("ardupilot");
    ardupilot.displayName = QStringLiteral("ArduPilot 飞控适配器");
    ardupilot.flightController = QStringLiteral("ardupilot");
    ardupilot.extensionType = QStringLiteral("flight_controller_adapter");
    ardupilot.capabilities = {QStringLiteral("telemetry"), QStringLiteral("health.snapshot"),
                              QStringLiteral("mission.preview"), QStringLiteral("configuration.read"),
                              QStringLiteral("parameter.metadata")};
    ardupilot.permissionScopes = {QStringLiteral("view.telemetry"), QStringLiteral("view.capabilities"),
                                  QStringLiteral("task.preview")};
    ardupilot.signatureStatus = QStringLiteral("unverified");
    ardupilot.sandboxProfile = QStringLiteral("process_isolated_pending");
    _adapters.append(ardupilot);

    AdapterDefinition vision;
    vision.id = QStringLiteral("companion.vision");
    vision.displayName = QStringLiteral("伴随计算机视觉插件");
    vision.flightController = QStringLiteral("agnostic");
    vision.extensionType = QStringLiteral("companion_plugin");
    vision.capabilities = {QStringLiteral("vision.detections"), QStringLiteral("telemetry.annotation"),
                           QStringLiteral("mission.context")};
    vision.permissionScopes = {QStringLiteral("view.telemetry"), QStringLiteral("view.vision"),
                               QStringLiteral("task.preview")};
    vision.signatureStatus = QStringLiteral("unverified");
    vision.sandboxProfile = QStringLiteral("restricted_process_pending");
    _adapters.append(vision);
}

QString FleetAutonomyPolicy::normalize(const QString& value)
{
    return value.trimmed().toLower();
}

bool FleetAutonomyPolicy::permissionMatches(const QStringList& permissions, const QString& action)
{
    const QString normalizedAction = normalize(action);
    if (normalizedAction.isEmpty()) {
        return false;
    }
    for (const QString& permission : permissions) {
        const QString normalizedPermission = normalize(permission);
        if (normalizedPermission == QStringLiteral("*") || normalizedPermission == normalizedAction) {
            return true;
        }
        if (normalizedPermission.endsWith(QStringLiteral(".*")) &&
            normalizedAction.startsWith(normalizedPermission.left(normalizedPermission.size() - 1))) {
            return true;
        }
    }
    return false;
}

QString FleetAutonomyPolicy::requiredCapability(const QString& action)
{
    const QString normalizedAction = normalize(action);
    if (normalizedAction.startsWith(QStringLiteral("view."))) {
        return QStringLiteral("telemetry");
    }
    if (normalizedAction == QStringLiteral("task.preview") ||
        normalizedAction == QStringLiteral("proposal.present")) {
        return QStringLiteral("mission.preview");
    }
    if (normalizedAction.startsWith(QStringLiteral("vision."))) {
        return QStringLiteral("vision.detections");
    }
    return QString();
}

QVariantMap FleetAutonomyPolicy::toVariantMap(const StageDefinition& definition) const
{
    QVariantMap result;
    result[QStringLiteral("stageId")] = definition.id;
    result[QStringLiteral("displayName")] = definition.displayName;
    result[QStringLiteral("authority")] = definition.authority;
    result[QStringLiteral("permissions")] = definition.permissions;
    result[QStringLiteral("allowedRoles")] = definition.allowedRoles;
    result[QStringLiteral("requiredApproval")] = definition.requiredApproval;
    result[QStringLiteral("commandReleaseMode")] = definition.commandReleaseMode;
    result[QStringLiteral("automationAllowed")] = definition.automationAllowed;
    result[QStringLiteral("humanApprovalRequired")] = definition.humanApprovalRequired;
    result[QStringLiteral("twoPersonApprovalRequired")] = definition.twoPersonApprovalRequired;
    result[QStringLiteral("signatureRequired")] = definition.signatureRequired;
    result[QStringLiteral("sandboxRequired")] = definition.sandboxRequired;
    result[QStringLiteral("flightCommandReleased")] = false;
    return result;
}

QVariantMap FleetAutonomyPolicy::toVariantMap(const AdapterDefinition& definition) const
{
    QVariantMap result;
    result[QStringLiteral("adapterId")] = definition.id;
    result[QStringLiteral("displayName")] = definition.displayName;
    result[QStringLiteral("flightController")] = definition.flightController;
    result[QStringLiteral("extensionType")] = definition.extensionType;
    result[QStringLiteral("capabilities")] = definition.capabilities;
    result[QStringLiteral("permissionScopes")] = definition.permissionScopes;
    result[QStringLiteral("signatureStatus")] = definition.signatureStatus;
    result[QStringLiteral("signatureVerified")] = definition.signatureStatus == QStringLiteral("verified");
    result[QStringLiteral("sandboxProfile")] = definition.sandboxProfile;
    result[QStringLiteral("sandboxReady")] = false;
    result[QStringLiteral("enabled")] = definition.enabled;
    result[QStringLiteral("signatureRequired")] = definition.signatureRequired;
    result[QStringLiteral("sandboxRequired")] = definition.sandboxRequired;
    result[QStringLiteral("commandRelease")] = false;
    result[QStringLiteral("flightCommandReleased")] = false;
    return result;
}

const FleetAutonomyPolicy::StageDefinition* FleetAutonomyPolicy::findStage(const QString& stage) const
{
    const QString id = normalize(stage);
    for (const StageDefinition& definition : _stages) {
        if (definition.id == id) {
            return &definition;
        }
    }
    return nullptr;
}

const FleetAutonomyPolicy::AdapterDefinition* FleetAutonomyPolicy::findAdapter(const QString& adapterId) const
{
    const QString id = normalize(adapterId);
    for (const AdapterDefinition& definition : _adapters) {
        if (definition.id == id) {
            return &definition;
        }
    }
    return nullptr;
}

QVariantList FleetAutonomyPolicy::stages() const
{
    QVariantList result;
    for (const StageDefinition& definition : _stages) {
        result.append(toVariantMap(definition));
    }
    return result;
}

QVariantList FleetAutonomyPolicy::platformAdapters() const
{
    QVariantList result;
    for (const AdapterDefinition& definition : _adapters) {
        result.append(toVariantMap(definition));
    }
    return result;
}

QVariantMap FleetAutonomyPolicy::stagePolicy(const QString& stage) const
{
    const StageDefinition* definition = findStage(stage);
    return definition ? toVariantMap(*definition) : QVariantMap();
}

QVariantMap FleetAutonomyPolicy::platformAdapter(const QString& adapterId) const
{
    const AdapterDefinition* definition = findAdapter(adapterId);
    return definition ? toVariantMap(*definition) : QVariantMap();
}

QVariantList FleetAutonomyPolicy::adaptersForFlightController(const QString& flightController) const
{
    const QString controller = normalize(flightController);
    QVariantList result;
    for (const AdapterDefinition& definition : _adapters) {
        if (definition.flightController == controller || definition.flightController == QStringLiteral("agnostic")) {
            result.append(toVariantMap(definition));
        }
    }
    return result;
}

QVariantMap FleetAutonomyPolicy::evaluate(const QString& stage,
                                          const QString& action,
                                          const QString& role,
                                          const QString& adapterId) const
{
    const QString normalizedStage = normalize(stage);
    const QString normalizedAction = normalize(action);
    const QString normalizedRole = normalize(role);
    const QString normalizedAdapter = normalize(adapterId);
    const StageDefinition* stageDefinition = findStage(normalizedStage);
    const AdapterDefinition* adapterDefinition = normalizedAdapter.isEmpty()
            ? nullptr : findAdapter(normalizedAdapter);

    QVariantMap result;
    result[QStringLiteral("policyVersion")] = policyVersion();
    result[QStringLiteral("stage")] = normalizedStage;
    result[QStringLiteral("action")] = normalizedAction;
    result[QStringLiteral("role")] = normalizedRole;
    result[QStringLiteral("adapterId")] = normalizedAdapter;
    result[QStringLiteral("knownStage")] = stageDefinition != nullptr;
    result[QStringLiteral("knownAdapter")] = normalizedAdapter.isEmpty() || adapterDefinition != nullptr;
    result[QStringLiteral("allowedByStage")] = stageDefinition && permissionMatches(stageDefinition->permissions, normalizedAction);

    bool allowedByRole = true;
    bool roleCheckRequired = false;
    if (stageDefinition) {
        if (normalizedRole.isEmpty()) {
            roleCheckRequired = true;
        } else {
            allowedByRole = stageDefinition->allowedRoles.contains(normalizedRole);
        }
    } else {
        allowedByRole = false;
    }
    result[QStringLiteral("allowedByRole")] = allowedByRole;
    result[QStringLiteral("roleCheckRequired")] = roleCheckRequired;

    const QString capability = requiredCapability(normalizedAction);
    const bool adapterRequired = stageDefinition &&
                                 (stageDefinition->signatureRequired || !normalizedAdapter.isEmpty());
    bool adapterReady = !adapterRequired;
    bool capabilitySatisfied = capability.isEmpty();
    if (adapterRequired) {
        // Signature and sandbox provisioning are intentionally not implemented
        // by this read-only declaration layer.
        adapterReady = false;
    }
    if (adapterDefinition && !capability.isEmpty()) {
        capabilitySatisfied = adapterDefinition->capabilities.contains(capability);
    }
    result[QStringLiteral("requiredCapability")] = capability;
    result[QStringLiteral("adapterRequired")] = adapterRequired;
    result[QStringLiteral("adapterReady")] = adapterReady;
    result[QStringLiteral("capabilitySatisfied")] = capabilitySatisfied;
    result[QStringLiteral("signatureCheck")] = adapterRequired ? QStringLiteral("external_verification_required")
                                                                  : QStringLiteral("not_required_for_read_only_review");
    result[QStringLiteral("sandboxCheck")] = adapterRequired ? QStringLiteral("external_sandbox_required")
                                                               : QStringLiteral("not_required_for_policy_read");
    result[QStringLiteral("allowed")] = result.value(QStringLiteral("knownStage")).toBool() &&
                                         result.value(QStringLiteral("knownAdapter")).toBool() &&
                                         result.value(QStringLiteral("allowedByStage")).toBool() &&
                                         !roleCheckRequired && allowedByRole && adapterReady && capabilitySatisfied;
    // `allowed` is a policy-review result only.  It must never be consumed as
    // a flight authorization; execution remains a separate safety-gated path.
    result[QStringLiteral("reviewOnly")] = true;
    result[QStringLiteral("executionAllowed")] = false;
    result[QStringLiteral("commandDispatch")] = QStringLiteral("not-released");
    result[QStringLiteral("manualConfirmationRequired")] = stageDefinition ? stageDefinition->humanApprovalRequired : true;
    result[QStringLiteral("twoPersonApprovalRequired")] = stageDefinition ? stageDefinition->twoPersonApprovalRequired : true;
    result[QStringLiteral("flightCommandReleased")] = false;
    if (!stageDefinition) {
        result[QStringLiteral("reason")] = QStringLiteral("unknown autonomy stage");
    } else if (normalizedAction.isEmpty()) {
        result[QStringLiteral("reason")] = QStringLiteral("action is required");
    } else if (!result.value(QStringLiteral("allowedByStage")).toBool()) {
        result[QStringLiteral("reason")] = QStringLiteral("action is outside the stage permission set");
    } else if (roleCheckRequired) {
        result[QStringLiteral("reason")] = QStringLiteral("role is required; resolve it through FleetRolePolicy");
    } else if (!allowedByRole) {
        result[QStringLiteral("reason")] = QStringLiteral("role is not allowed for this autonomy stage");
    } else if (!result.value(QStringLiteral("knownAdapter")).toBool()) {
        result[QStringLiteral("reason")] = QStringLiteral("unknown platform adapter");
    } else if (!adapterReady) {
        result[QStringLiteral("reason")] = QStringLiteral("signed adapter and sandbox approval are unresolved");
    } else if (!capabilitySatisfied) {
        result[QStringLiteral("reason")] = QStringLiteral("adapter does not declare the required capability");
    } else {
        result[QStringLiteral("reason")] = QStringLiteral("policy result is review-only; host must apply FleetRolePolicy and safety gates");
    }
    return result;
}

#include "FleetRolePolicy.h"

#include <QHashIterator>
#include <QSet>
#include <QPointer>
#include <QUuid>
#include <limits>

namespace {

QString normalize(const QString& value)
{
    return value.trimmed().toLower();
}

}

FleetRolePolicy::FleetRolePolicy(QObject* parent)
    : QObject(parent), _policyEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    defineRole(QStringLiteral("observer"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit")},
               false, QStringLiteral("观察员"));
    defineRole(QStringLiteral("planner"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.create"), QStringLiteral("task.edit"), QStringLiteral("task.preview"),
                QStringLiteral("mission.evidence.refresh"), QStringLiteral("mission.start.content.read"),
                QStringLiteral("mission.reconcile.read"), QStringLiteral("fleet.telemetry.interval.capture"),
                QStringLiteral("fleet.telemetry.rtt.capture"), QStringLiteral("fleet.video.stream.capture")},
               false, QStringLiteral("任务规划员"));
    defineRole(QStringLiteral("operator"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.start"), QStringLiteral("task.pause"), QStringLiteral("task.resume"),
                QStringLiteral("task.handoff"), QStringLiteral("task.continuity.apply"), QStringLiteral("task.observation.close"),
                QStringLiteral("fleet.occupancy.recover"),
                QStringLiteral("mission.upload"), QStringLiteral("mission.start.review"), QStringLiteral("mission.start"),
                QStringLiteral("mission.evidence.refresh"), QStringLiteral("mission.start.content.read"),
                QStringLiteral("mission.reconcile.read"), QStringLiteral("fleet.telemetry.interval.capture"),
                QStringLiteral("fleet.telemetry.rtt.capture"), QStringLiteral("fleet.video.stream.capture"),
                QStringLiteral("fleet.telemetry.interval.configure"),
                QStringLiteral("fleet.video.stream.control"),
                QStringLiteral("mission.start.ground.reassess"),
                QStringLiteral("vehicle.command")},
               true, QStringLiteral("飞行操作员"));
    defineRole(QStringLiteral("safety_officer"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.approve"), QStringLiteral("task.abort"), QStringLiteral("task.handoff"),
                QStringLiteral("task.continuity.apply"), QStringLiteral("task.observation.close"), QStringLiteral("mission.start.review"),
                QStringLiteral("fleet.occupancy.recover"),
                QStringLiteral("mission.start.ground.reassess"),
                QStringLiteral("mission.evidence.refresh"), QStringLiteral("mission.start.content.read"),
                QStringLiteral("mission.reconcile.read"), QStringLiteral("fleet.telemetry.interval.capture"),
                QStringLiteral("fleet.telemetry.rtt.capture"), QStringLiteral("fleet.video.stream.capture"),
                QStringLiteral("safety.override")},
               true, QStringLiteral("安全员"));
    defineRole(QStringLiteral("maintenance"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("maintenance.inspect"), QStringLiteral("maintenance.configure"),
                QStringLiteral("firmware.catalog.reload"),
                QStringLiteral("fleet.capability.review"),
                QStringLiteral("fleet.configuration.review"),
                QStringLiteral("fleet.telemetry.interval.configure"),
                QStringLiteral("fleet.video.stream.control"),
                QStringLiteral("mission.evidence.refresh"), QStringLiteral("mission.start.content.read"),
                QStringLiteral("mission.reconcile.read"), QStringLiteral("fleet.telemetry.interval.capture"),
                QStringLiteral("fleet.telemetry.rtt.capture"), QStringLiteral("fleet.video.stream.capture")},
               false, QStringLiteral("维修人员"));
    defineRole(QStringLiteral("commander"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.create"), QStringLiteral("task.edit"), QStringLiteral("task.preview"),
                QStringLiteral("task.start"), QStringLiteral("task.pause"), QStringLiteral("task.resume"),
                QStringLiteral("task.approve"), QStringLiteral("task.abort"), QStringLiteral("task.handoff"),
                QStringLiteral("task.continuity.apply"), QStringLiteral("task.observation.close"), QStringLiteral("mission.upload"),
                QStringLiteral("fleet.occupancy.recover"),
                QStringLiteral("mission.start.review"), QStringLiteral("mission.start"), QStringLiteral("mission.evidence.refresh"),
                QStringLiteral("mission.start.content.read"),
                QStringLiteral("mission.reconcile.read"), QStringLiteral("fleet.telemetry.interval.capture"),
                QStringLiteral("fleet.telemetry.rtt.capture"), QStringLiteral("fleet.video.stream.capture"),
                QStringLiteral("fleet.telemetry.interval.configure"),
                QStringLiteral("fleet.video.stream.control"),
                QStringLiteral("mission.start.ground.reassess"),
                QStringLiteral("vehicle.command")},
               true, QStringLiteral("总指挥"));
    defineRole(QStringLiteral("administrator"),
               {QStringLiteral("*")}, true, QStringLiteral("管理员"));

    _currentRole = QStringLiteral("observer");
}

QString FleetRolePolicy::roleToString(Role role)
{
    switch (role) {
    case Role::Observer: return QStringLiteral("observer");
    case Role::Planner: return QStringLiteral("planner");
    case Role::Operator: return QStringLiteral("operator");
    case Role::SafetyOfficer: return QStringLiteral("safety_officer");
    case Role::Maintenance: return QStringLiteral("maintenance");
    case Role::Commander: return QStringLiteral("commander");
    case Role::Administrator: return QStringLiteral("administrator");
    }
    return QStringLiteral("observer");
}

QString FleetRolePolicy::normalizedRole(const QString& role) const
{
    const QString value = normalize(role);
    return value.isEmpty() ? _currentRole : value;
}

QString FleetRolePolicy::normalizedAction(const QString& action) const
{
    return normalize(action);
}

const FleetRolePolicy::RoleDefinition* FleetRolePolicy::findRole(const QString& role) const
{
    const auto it = _roles.constFind(normalizedRole(role));
    return it == _roles.constEnd() ? nullptr : &it.value();
}

FleetRolePolicy::RoleDefinition* FleetRolePolicy::findRole(const QString& role)
{
    const auto it = _roles.find(normalizedRole(role));
    return it == _roles.end() ? nullptr : &it.value();
}

QVariantMap FleetRolePolicy::roleToVariant(const RoleDefinition& role) const
{
    QVariantMap result;
    result[QStringLiteral("name")] = role.name;
    result[QStringLiteral("displayName")] = role.displayName;
    result[QStringLiteral("permissions")] = role.permissions;
    result[QStringLiteral("requiresSecondApproval")] = role.requiresSecondApproval;
    return result;
}

void FleetRolePolicy::setCurrentUserId(const QString& userId)
{
    const QString normalized = userId.trimmed();
    if (_currentUserId == normalized) {
        return;
    }
    if (!advanceRevision()) return;
    QPointer<FleetRolePolicy> self(this);
    _currentUserId = normalized;
    // A new/unassigned actor and logout must not inherit the previous actor's
    // elevated role. During this transition snapshots also require the actual
    // per-user assignment to agree, so an intermediate callback fails closed.
    setCurrentRole(_userRoles.value(_currentUserId, QStringLiteral("observer")));
    if (!self || _currentUserId != normalized) return;
    emit currentUserChanged();
}

void FleetRolePolicy::setCurrentRole(const QString& role)
{
    const QString normalized = normalize(role);
    if (normalized.isEmpty() || !_roles.contains(normalized) || _currentRole == normalized) {
        return;
    }
    if (!advanceRevision()) return;
    _currentRole = normalized;
    if (!_currentUserId.isEmpty()) {
        _userRoles.insert(_currentUserId, _currentRole);
    }
    emit currentRoleChanged();
}

bool FleetRolePolicy::defineRole(const QString& role, const QStringList& permissions,
                                 bool requiresSecondApproval, const QString& displayName)
{
    const QString normalized = normalize(role);
    if (normalized.isEmpty()) {
        return false;
    }
    RoleDefinition definition;
    definition.name = normalized;
    definition.displayName = displayName.trimmed().isEmpty() ? normalized : displayName.trimmed();
    QSet<QString> unique;
    for (const QString& permission : permissions) {
        const QString value = normalize(permission);
        if (!value.isEmpty() && !unique.contains(value)) {
            unique.insert(value);
            definition.permissions.append(value);
        }
    }
    definition.requiresSecondApproval = requiresSecondApproval;
    if (!advanceRevision()) return false;
    _roles.insert(normalized, definition);
    emit policyChanged();
    return true;
}

bool FleetRolePolicy::removeRole(const QString& role)
{
    const QString normalized = normalize(role);
    if (normalized.isEmpty() || normalized == QStringLiteral("observer") || !_roles.contains(normalized)) {
        return false;
    }
    if (!advanceRevision()) return false;
    QPointer<FleetRolePolicy> self(this);
    _roles.remove(normalized);
    for (auto it = _userRoles.begin(); it != _userRoles.end();) {
        if (it.value() == normalized) {
            it = _userRoles.erase(it);
        } else {
            ++it;
        }
    }
    if (_currentRole == normalized) {
        _currentRole = QStringLiteral("observer");
        emit currentRoleChanged();
    }
    if (!self) return false;
    emit policyChanged();
    return true;
}

bool FleetRolePolicy::assignUserRole(const QString& userId, const QString& role)
{
    const QString user = userId.trimmed();
    const QString normalized = normalize(role);
    if (user.isEmpty() || !_roles.contains(normalized)) {
        return false;
    }
    if (!advanceRevision()) return false;
    QPointer<FleetRolePolicy> self(this);
    _userRoles.insert(user, normalized);
    if (user == _currentUserId) {
        setCurrentRole(normalized);
    }
    if (!self) return false;
    emit policyChanged();
    return true;
}

QString FleetRolePolicy::roleForUser(const QString& userId) const
{
    return _userRoles.value(userId.trimmed(), QStringLiteral("observer"));
}

bool FleetRolePolicy::advanceRevision()
{
    if (_policyRevision == std::numeric_limits<quint64>::max()) return false;
    ++_policyRevision;
    return true;
}

QVariantMap FleetRolePolicy::actionSnapshot(const QString& action) const
{
    const QString requested = normalizedAction(action);
    const auto bounded = [](const QString& value, int limit) {
        if (value.isEmpty() || value.size() > limit || value != value.trimmed()
            || QString::fromUtf8(value.toUtf8()) != value) return false;
        for (QChar character : value) if (character.category() == QChar::Other_Control) return false;
        return true;
    };
    const bool identity = bounded(_currentUserId, 128) && bounded(_currentRole, 64)
        && bounded(requested, 64) && _policyRevision != 0
        && _policyRevision != std::numeric_limits<quint64>::max() && findRole(_currentRole)
        && _userRoles.contains(_currentUserId) && _userRoles.value(_currentUserId) == _currentRole;
    return {{QStringLiteral("actorId"), _currentUserId.left(128)}, {QStringLiteral("role"), _currentRole.left(64)},
        {QStringLiteral("action"), requested.left(64)}, {QStringLiteral("allowed"), identity && canPerform(requested)},
        {QStringLiteral("requiresSecondApproval"), identity && requiresSecondApproval(requested)},
        {QStringLiteral("policyEpoch"), _policyEpoch}, {QStringLiteral("policyRevision"), QString::number(_policyRevision)},
        {QStringLiteral("actorAuthenticated"), false}, {QStringLiteral("executionPermitted"), false}};
}

bool FleetRolePolicy::actionSnapshotCurrent(const QVariantMap& receipt) const
{
    if (receipt.size() != 9) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
            QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")})
        if (receipt.value(key).type() != QVariant::String) return false;
    for (const QString& key : {QStringLiteral("allowed"), QStringLiteral("requiresSecondApproval"),
            QStringLiteral("actorAuthenticated"), QStringLiteral("executionPermitted")})
        if (receipt.value(key).type() != QVariant::Bool) return false;
    if (receipt.value(QStringLiteral("actorAuthenticated")).toBool()
        || receipt.value(QStringLiteral("executionPermitted")).toBool()) return false;
    const QVariant action = receipt.value(QStringLiteral("action"));
    return !action.toString().isEmpty() && action.toString().size() <= 64
        && receipt.value(QStringLiteral("allowed")).toBool() && actionSnapshot(action.toString()) == receipt;
}

bool FleetRolePolicy::canPerform(const QString& action, const QString& role) const
{
    const RoleDefinition* definition = findRole(role);
    if (!definition) {
        return false;
    }
    const QString requested = normalizedAction(action);
    if (requested.isEmpty()) {
        return false;
    }
    for (const QString& permission : definition->permissions) {
        if (permission == QStringLiteral("*") || permission == requested) {
            return true;
        }
        // A parent scope such as task.* grants task.start/task.pause, but
        // never bypasses the adapter's flight-safety checks.
        if (permission.endsWith(QStringLiteral(".*")) && requested.startsWith(permission.left(permission.size() - 1))) {
            return true;
        }
    }
    return false;
}

bool FleetRolePolicy::requiresSecondApproval(const QString& action, const QString& role) const
{
    // Local restart/history-hold retirement and engineering declaration changes
    // always need an independent account, even if an editable role definition
    // disables its default flag. These do not grant flight permission.
    if (normalizedAction(action) == QStringLiteral("fleet.occupancy.recover")
        || normalizedAction(action) == QStringLiteral("firmware.catalog.reload")
        || normalizedAction(action) == QStringLiteral("fleet.capability.review")
        || normalizedAction(action) == QStringLiteral("fleet.telemetry.interval.configure")
        || normalizedAction(action) == QStringLiteral("fleet.video.stream.control")
        || normalizedAction(action) == QStringLiteral("fleet.configuration.review")) return canPerform(action, role);
    // These exact scopes submit bounded version/parameter/Mission/interval READ
    // requests, one targeted RTT sample, or one camera Info/Status read pair.
    // It never writes a parameter, approves a plan, or releases flight control.
    if (normalizedAction(action) == QStringLiteral("mission.evidence.refresh")
        || normalizedAction(action) == QStringLiteral("fleet.telemetry.interval.capture")
        || normalizedAction(action) == QStringLiteral("fleet.telemetry.rtt.capture")
        || normalizedAction(action) == QStringLiteral("fleet.video.stream.capture")
        || normalizedAction(action) == QStringLiteral("mission.start.content.read")
        || normalizedAction(action) == QStringLiteral("mission.reconcile.read")
        || normalizedAction(action) == QStringLiteral("mission.start.ground.reassess")) return false;
    return canPerform(action, role) && findRole(role)->requiresSecondApproval;
}

QString FleetRolePolicy::denialReason(const QString& action, const QString& role) const
{
    const QString effectiveRole = normalizedRole(role);
    if (!findRole(effectiveRole)) {
        return QStringLiteral("unknown role: %1").arg(effectiveRole);
    }
    if (canPerform(action, effectiveRole)) {
        return QString();
    }
    return QStringLiteral("role %1 is not permitted to perform %2")
            .arg(effectiveRole, normalizedAction(action));
}

QVariantMap FleetRolePolicy::evaluate(const QString& action, const QString& role) const
{
    const QString effectiveRole = normalizedRole(role);
    QVariantMap result;
    result[QStringLiteral("action")] = normalizedAction(action);
    result[QStringLiteral("role")] = effectiveRole;
    result[QStringLiteral("allowed")] = canPerform(action, effectiveRole);
    result[QStringLiteral("requiresSecondApproval")] = requiresSecondApproval(action, effectiveRole);
    result[QStringLiteral("reason")] = denialReason(action, effectiveRole);
    // Role policy is an authorization hint for the host UI, not an execution
    // token. Vehicle health, operator authentication and command safety gates
    // remain separate and are intentionally absent from this class.
    result[QStringLiteral("reviewOnly")] = true;
    result[QStringLiteral("executionAllowed")] = false;
    result[QStringLiteral("commandDispatch")] = QStringLiteral("not-released");
    result[QStringLiteral("flightCommandReleased")] = false;
    return result;
}

QVariantList FleetRolePolicy::roleDefinitions() const
{
    QVariantList result;
    for (auto it = _roles.constBegin(); it != _roles.constEnd(); ++it) {
        result.append(roleToVariant(it.value()));
    }
    return result;
}

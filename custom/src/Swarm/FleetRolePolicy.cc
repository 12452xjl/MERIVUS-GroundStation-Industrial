#include "FleetRolePolicy.h"

#include <QHashIterator>
#include <QSet>

namespace {

QString normalize(const QString& value)
{
    return value.trimmed().toLower();
}

}

FleetRolePolicy::FleetRolePolicy(QObject* parent)
    : QObject(parent)
{
    defineRole(QStringLiteral("observer"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit")},
               false, QStringLiteral("观察员"));
    defineRole(QStringLiteral("planner"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.create"), QStringLiteral("task.edit"), QStringLiteral("task.preview")},
               false, QStringLiteral("任务规划员"));
    defineRole(QStringLiteral("operator"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.start"), QStringLiteral("task.pause"), QStringLiteral("task.resume"),
                QStringLiteral("task.handoff"), QStringLiteral("vehicle.command")},
               true, QStringLiteral("飞行操作员"));
    defineRole(QStringLiteral("safety_officer"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.approve"), QStringLiteral("task.abort"), QStringLiteral("task.handoff"),
                QStringLiteral("safety.override")},
               true, QStringLiteral("安全员"));
    defineRole(QStringLiteral("maintenance"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("maintenance.inspect"), QStringLiteral("maintenance.configure")},
               false, QStringLiteral("维修人员"));
    defineRole(QStringLiteral("commander"),
               {QStringLiteral("view.fleet"), QStringLiteral("view.telemetry"), QStringLiteral("view.audit"),
                QStringLiteral("task.create"), QStringLiteral("task.edit"), QStringLiteral("task.preview"),
                QStringLiteral("task.start"), QStringLiteral("task.pause"), QStringLiteral("task.resume"),
                QStringLiteral("task.approve"), QStringLiteral("task.abort"), QStringLiteral("task.handoff"),
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
    _currentUserId = normalized;
    if (!_currentUserId.isEmpty() && _userRoles.contains(_currentUserId)) {
        setCurrentRole(_userRoles.value(_currentUserId));
    }
    emit currentUserChanged();
}

void FleetRolePolicy::setCurrentRole(const QString& role)
{
    const QString normalized = normalize(role);
    if (normalized.isEmpty() || !_roles.contains(normalized) || _currentRole == normalized) {
        return;
    }
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
    _userRoles.insert(user, normalized);
    if (user == _currentUserId) {
        setCurrentRole(normalized);
    }
    emit policyChanged();
    return true;
}

QString FleetRolePolicy::roleForUser(const QString& userId) const
{
    return _userRoles.value(userId.trimmed(), QStringLiteral("observer"));
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

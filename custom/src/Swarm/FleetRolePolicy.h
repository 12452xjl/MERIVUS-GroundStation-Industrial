#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Human-operation authorization model for Fleet OS.
//
// This class answers whether an operator may perform a named UI/task action.
// It intentionally has no Vehicle/MAVLink dependency and never sends or
// releases a flight command.  A command adapter must still perform its own
// vehicle-health, arming and safety checks before dispatch.
class FleetRolePolicy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentUserId READ currentUserId WRITE setCurrentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentRole READ currentRole WRITE setCurrentRole NOTIFY currentRoleChanged)
    Q_PROPERTY(QVariantList roleDefinitions READ roleDefinitions NOTIFY policyChanged)

public:
    enum class Role {
        Observer,
        Planner,
        Operator,
        SafetyOfficer,
        Maintenance,
        Commander,
        Administrator,
    };
    Q_ENUM(Role)

    explicit FleetRolePolicy(QObject* parent = nullptr);

    QString currentUserId() const { return _currentUserId; }
    void setCurrentUserId(const QString& userId);
    QString currentRole() const { return _currentRole; }
    void setCurrentRole(const QString& role);
    QVariantList roleDefinitions() const;

    Q_INVOKABLE bool defineRole(const QString& role,
                                const QStringList& permissions,
                                bool requiresSecondApproval = false,
                                const QString& displayName = QString());
    Q_INVOKABLE bool removeRole(const QString& role);
    Q_INVOKABLE bool assignUserRole(const QString& userId, const QString& role);
    Q_INVOKABLE QString roleForUser(const QString& userId) const;
    Q_INVOKABLE bool canPerform(const QString& action,
                                const QString& role = QString()) const;
    Q_INVOKABLE bool requiresSecondApproval(const QString& action,
                                            const QString& role = QString()) const;
    Q_INVOKABLE QString denialReason(const QString& action,
                                     const QString& role = QString()) const;
    Q_INVOKABLE QVariantMap evaluate(const QString& action,
                                     const QString& role = QString()) const;

    static QString roleToString(Role role);

signals:
    void currentUserChanged();
    void currentRoleChanged();
    void policyChanged();

private:
    struct RoleDefinition {
        QString name;
        QString displayName;
        QStringList permissions;
        bool requiresSecondApproval = false;
    };

    QString normalizedRole(const QString& role) const;
    QString normalizedAction(const QString& action) const;
    const RoleDefinition* findRole(const QString& role) const;
    RoleDefinition* findRole(const QString& role);
    QVariantMap roleToVariant(const RoleDefinition& role) const;

    QHash<QString, RoleDefinition> _roles;
    QHash<QString, QString> _userRoles;
    QString _currentUserId;
    QString _currentRole;
};

#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Read-only policy and capability declarations for progressive autonomy.
//
// This class describes the human, assisted, supervised and automated stages
// together with the platform adapters that may participate in each stage. It
// does not load a plugin, verify a signature, create a sandbox, call Vehicle,
// send MAVLink or release a flight command. Those checks remain host-product
// responsibilities and are represented here as explicit, unverified gates.
class FleetAutonomyPolicy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString policyVersion READ policyVersion CONSTANT)
    Q_PROPERTY(QVariantList stages READ stages CONSTANT)
    Q_PROPERTY(QVariantList platformAdapters READ platformAdapters CONSTANT)
    Q_PROPERTY(bool flightCommandReleased READ flightCommandReleased CONSTANT)

public:
    enum class Stage {
        Manual,
        Assisted,
        Supervised,
        Automated,
    };
    Q_ENUM(Stage)

    explicit FleetAutonomyPolicy(QObject* parent = nullptr);

    QString policyVersion() const { return QStringLiteral("1.0"); }
    QVariantList stages() const;
    QVariantList platformAdapters() const;
    bool flightCommandReleased() const { return false; }

    // Returns a reviewable decision. A true value never means a command was
    // executed: flightCommandReleased is always false in this build.
    Q_INVOKABLE QVariantMap evaluate(const QString& stage,
                                     const QString& action,
                                     const QString& role = QString(),
                                     const QString& adapterId = QString()) const;
    Q_INVOKABLE QVariantMap stagePolicy(const QString& stage) const;
    Q_INVOKABLE QVariantMap platformAdapter(const QString& adapterId) const;
    Q_INVOKABLE QVariantList adaptersForFlightController(const QString& flightController) const;

private:
    struct StageDefinition {
        QString id;
        QString displayName;
        QString authority;
        QStringList permissions;
        QStringList allowedRoles;
        QString requiredApproval;
        QString commandReleaseMode;
        bool automationAllowed = false;
        bool humanApprovalRequired = true;
        bool twoPersonApprovalRequired = false;
        bool signatureRequired = false;
        bool sandboxRequired = false;
    };

    struct AdapterDefinition {
        QString id;
        QString displayName;
        QString flightController;
        QString extensionType;
        QStringList capabilities;
        QStringList permissionScopes;
        QString signatureStatus;
        QString sandboxProfile;
        bool enabled = false;
        bool signatureRequired = true;
        bool sandboxRequired = true;
        bool commandRelease = false;
    };

    static QString normalize(const QString& value);
    static bool permissionMatches(const QStringList& permissions, const QString& action);
    static QString requiredCapability(const QString& action);
    QVariantMap toVariantMap(const StageDefinition& definition) const;
    QVariantMap toVariantMap(const AdapterDefinition& definition) const;
    const StageDefinition* findStage(const QString& stage) const;
    const AdapterDefinition* findAdapter(const QString& adapterId) const;

    QList<StageDefinition> _stages;
    QList<AdapterDefinition> _adapters;
};

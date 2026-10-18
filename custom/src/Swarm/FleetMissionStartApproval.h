#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetMissionStartReview;
class FleetMissionStartupAuthority;
class FleetOperatorSession;
class FleetRolePolicy;
class LinkInterface;
class MissionManager;
class ParameterManager;
class Vehicle;
class VehicleLinkManager;

// Actual two-account approval of one CONDITIONAL start intent. No dispatcher,
// ARM, mode change, upload lease, startup permit or execution authorization.
class FleetMissionStartApproval : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionStartApproval(QObject* parent = nullptr);
    ~FleetMissionStartApproval() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestStartApproval(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* reviewObject, QObject* deploymentObject,
        QObject* authorizationOwner, const QString& reviewId);
    Q_INVOKABLE bool revokeCurrent(const QString& approvalId);

    // Only an independent native startup workflow may consume this intent.
    // The returned conditional grant STILL does not authorize dispatch.
    QVariantMap consumeStartApproval(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* reviewObject, QObject* deploymentObject,
        QObject* authorizationOwner, const QVariantMap& approvalReceipt, const QString& startupId);
    bool grantCurrent(const QVariantMap& grant, const QString& startupId);
    // Moves the exact conditional authentication once to an actual Authority.
    // This retires the old grant; it still does NOT create a StartPermit.
    QVariantMap transferToStartup(QObject* authorityObject, const QVariantMap& conditionalGrant,
        const QString& startupId, const QString& sourceHash, qint64 maximumRemainingMs);

signals:
    void changed();

private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        bool hadPrimary = false;
    };
    bool _expired() const;
    bool _same(quint64 generation, const QString& state) const;
    bool _sourcesCurrent(quint64 generation, bool requireApprover);
    bool _targetsCurrent(quint64 generation, const QList<Target>& targets) const;
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _invalidate(quint64 generation, const QString& reason,
        const QString& state = QStringLiteral("stale"), bool deferNotification = false);
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartReview> _review;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<QObject> _authorization;
    QList<QMetaObject::Connection> _connections;
    QList<Target> _targets;
    QVariantMap _context;
    QVariantMap _binding;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QVariantMap _approverReceipt;
    QVariantMap _authenticationReceipt;
    QVariantMap _approvalReceipt;
    QVariantMap _grant;
    QPointer<FleetMissionStartupAuthority> _transferredAuthority;
    QString _transferredStartupId;
    QString _transferredAuthorityEpoch;
    QString _transferredSourceHash;
    QByteArray _recordBytes;
    QString _contextHash;
    QString _bindingHash;
    QString _approvalId;
    QString _approvalEpoch;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _createdAtMs = -1;
    qint64 _expiresAtMs = -1;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

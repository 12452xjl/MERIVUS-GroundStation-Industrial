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
class FleetFirmwareAdapterCatalog;
class FleetHealthCenter;
class FleetMissionArmedGroundReview;
class FleetMissionDeployment;
class FleetMissionGroundStartupAuthority;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
class FleetMissionStartupTransaction;
class FleetOperatorSession;
class FleetRegistry;
class FleetRolePolicy;
class FaultToleranceManager;
class LinkInterface;
class MissionManager;
class ParameterManager;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// A NEW independent-account approval of one acknowledged ground reassessment.
// No old authority/identity is restored. No command, permit or upload lease.
class FleetMissionGroundStartApproval : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionGroundStartApproval(QObject* parent = nullptr);
    ~FleetMissionGroundStartApproval() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestApproval(const QVariantMap& actualSources,
        const QString& reviewId, bool operatorConfirmed);
    Q_INVOKABLE bool revokeApproval(const QString& approvalId);
    QString approvalEpoch() const { return _approvalEpoch; }

    // Native conditional consumer only; the grant STILL cannot dispatch.
    QVariantMap consumeApproval(const QVariantMap& actualSources,
        const QVariantMap& exactApprovalReceipt, const QString& consumerId);
    bool grantCurrent(const QVariantMap& exactGrant, const QString& consumerId);
    // Retires the conditional basis once into a strong native independent
    // owner. It still creates no permit, command or fresh lifetime.
    QVariantMap transferToGroundStartup(QObject* actualGroundAuthority,
        const QVariantMap& exactGrant, const QString& startupId,
        const QString& sourceHash, qint64 maximumRemainingMs);
    bool authenticationConsumeContextCurrent(const QString& approvalId, const QString& approvalEpoch,
        const QString& bindingHash, const QString& reviewId, const QString& reviewEpoch,
        const QVariantMap& primary, const QVariantMap& approver, int maximumRemainingMs);
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        bool armed = false;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
    };
    bool _expired() const;
    bool _same(quint64 generation, const QString& state) const;
    bool _sourcesCurrent(quint64 generation, bool requireApprover, bool requireProof = false);
    bool _targetsCurrent(quint64 generation) const;
    QVariantMap _bundle() const;
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _invalidate(quint64 generation, const QString& reason,
        const QString& state = QStringLiteral("stale"), bool deferNotification = false);
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartupTransaction> _previous;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<SwarmController> _swarm;
    QPointer<FleetHealthCenter> _health;
    QPointer<QObject> _authorization;
    QPointer<FleetMissionArmedGroundReview> _review;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetRegistry> _registry;
    QPointer<FaultToleranceManager> _faults;
    QPointer<FleetMissionGroundStartupAuthority> _transferredAuthority;
    QString _transferredStartupId;
    QString _transferredAuthorityEpoch;
    QString _transferredSourceHash;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _context;
    QVariantMap _identity;
    QVariantMap _role;
    QVariantMap _approver;
    QVariantMap _authentication;
    QVariantMap _receipt;
    QVariantMap _grant;
    QVariantMap _binding;
    QByteArray _recordBytes;
    QString _contextHash;
    QString _bindingHash;
    QString _approvalId;
    QString _approvalEpoch;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _expiresAtMs = -1;
    qint64 _approvedAtMs = -1;
    int _consumeMaximumMs = 0;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _prompt = false;
    bool _consumeInProgress = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

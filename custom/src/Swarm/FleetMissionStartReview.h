#pragma once

#include "FleetMissionStartupGeographicMotion.h"
#include "LinkInterface.h"

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <atomic>
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetFirmwareAdapterCatalog;
class FleetHealthCenter;
class FleetMissionDeployment;
class FleetMissionPlanStore;
class FleetMissionStartContentCheck;
class FleetMissionReviewCoordinator;
class FleetMissionStartupClearanceWorker;
class FleetMissionStartupClearanceWitness;
class FleetRolePolicy;
class FleetRegistry;
class FaultToleranceManager;
class MissionManager;
class LinkInterface;
class ParameterManager;
class Vehicle;
class VehicleLinkManager;

// A short-lived local acknowledgement of one immutable risk review. This
// class never authenticates an actor, grants a start permit, or dispatches.
class FleetMissionStartReview : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionStartReview(QObject* parent = nullptr);
    ~FleetMissionStartReview() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(QObject* deploymentObject, QObject* storeObject,
        QObject* reviewCoordinatorObject, QObject* healthObject, QObject* rolePolicyObject,
        QObject* contentCheckObject, QObject* authorizationOwner,
        const QString& deploymentId, const QString& contentCheckId);
    Q_INVOKABLE QVariantMap acknowledgeReview(const QString& reviewId, bool operatorConfirmed);
    Q_INVOKABLE bool revokeReview(const QString& reviewId);

    // Native-only immutable input for a SEPARATE conditional human approval.
    // It is not a start permit; all technical risks and the original TTL remain.
    QVariantMap approvalContext(const QString& reviewId);
    bool approvalContextCurrent(const QString& reviewId, const QVariantMap& context);
    FleetFirmwareAdapterCatalog* nativeFirmwareCatalog() const;
    FleetMissionReviewCoordinator* nativeReviewCoordinator() const;
    // Same native witness, never restored from the portable risk artifact.
    std::shared_ptr<FleetMissionStartupClearanceWitness> startupClearanceWitness(const QString& reviewId);

signals:
    void changed();

private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> manager;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        int componentId = 0;
        QVariantMap parameterSnapshot;
        QString readScopeId;
        QString readOperationId;
    };
    bool _live(quint64 generation) const;
    bool _expired() const;
    bool _capture(quint64 generation, QVariantMap* captured, QString* reason);
    bool _captureClearanceSource(quint64 generation, const QVariantMap& plan,
        const QList<Target>& targets, FleetRegistry* registry);
    bool _clearanceSourceMemoryCurrent(quint64 generation, const QList<Target>& targets, FleetRegistry* registry);
    bool _executionParametersMemoryCurrent(const QList<Target>& targets) const;
    bool _beginStartupClearance(quint64 generation);
    void _startupClearanceFinished(FleetMissionStartupClearanceWorker* worker, quint64 generation,
        const std::shared_ptr<std::atomic_bool>& cancellation);
    bool _healthSourceCurrent(quint64 generation);
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _invalidate(quint64 generation, const QString& reason, const QString& state = QStringLiteral("stale"));
    QVariantMap _receipt(bool ok, const QString& reason = QString()) const;

    QPointer<FleetMissionDeployment> _deployment;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartContentCheck> _contentCheck;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetHealthCenter> _health;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _authorization;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetRegistry> _registry;
    QPointer<FaultToleranceManager> _faultPolicy;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _artifact;
    QVariantMap _sourceCapture;
    QVariantMap _clearanceSourcePlan;
    QVariantMap _clearanceSourceEvidence;
    QVariantMap _clearanceDatumEvidence;
    QPointer<FleetMissionStartupClearanceWorker> _startupClearanceWorker;
    std::shared_ptr<std::atomic_bool> _startupClearanceCancellation;
    QVariantMap _startupClearanceInput;
    FleetMissionStartupGeographicMotion::Models _startupGeographicModels;
    QVariantMap _startupClearanceResult;
    std::shared_ptr<FleetMissionStartupClearanceWitness> _startupClearanceWitness;
    qint64 _clearanceLastUtcMs = -1;
    qint64 _clearanceValidUntilUtcMs = -1;
    bool _clearanceSourceFrozen = false;
    bool _clearanceSourceProvided = false;
    bool _clearanceSourceInvalidated = false;
    bool _clearanceSourceChecking = false;
    QString _reviewId;
    QString _reviewEpoch;
    QString _deploymentId;
    QString _contentCheckId;
    QString _state;
    QString _reason;
    quint64 _generation = 0;
    qint64 _capturedAtMs = -1;
    qint64 _expiresAtMs = -1;
    bool _busy = false;
    bool _reentered = false;
    bool _healthChecking = false;
    QElapsedTimer _clock;
    QTimer _expiryTimer;
};

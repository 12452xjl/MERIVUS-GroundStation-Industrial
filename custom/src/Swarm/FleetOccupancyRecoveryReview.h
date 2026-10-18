#pragma once

#include "FleetOccupancyRecoveryEvidence.h"

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>

class FleetOperatorSession;
class FleetRolePolicy;
class SwarmController;

// One actual Swarm's local restart/history review. It cannot clear a current
// uncertain command outcome, an owner, a task, or an onboard control lease.
// Ground inspection is an explicit human declaration, not sensor attestation.
class FleetOccupancyRecoveryReview final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetOccupancyRecoveryReview() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(QObject* actualSession, QObject* actualRoles,
        const QVariantList& selectedVehicleIds, const QString& dispositionReference, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap requestReviewForController(QObject* actualSession, QObject* actualRoles,
        QObject* actualAuthorizationOwner, const QVariantList& selectedVehicleIds,
        const QString& dispositionReference, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap approveAndResolve(const QString& reviewId,
        bool groundInspectionConfirmed, bool localReuseOnlyConfirmed);
    Q_INVOKABLE bool cancelReview(const QString& reviewId);
signals:
    void changed();
private:
    friend class SwarmController;
    friend class FleetOperatorSession;
    explicit FleetOccupancyRecoveryReview(SwarmController* actualSwarm);
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    QVariantMap requestReviewImpl(QObject* actualSession, QObject* actualRoles, QObject* actualAuthorizationOwner,
        const QVariantList& selectedVehicleIds, const QString& dispositionReference,
        bool operatorConfirmed, bool controllerManaged);
    bool memoryCurrent(quint64 generation) const;
    bool authorizationSourcesCurrent(quint64 generation) const;
    bool sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover);
    bool dispositionSourcesCurrent() const;
    bool dispositionFinalSourcesCurrent() const;
    bool mutationBindingCurrent(SwarmController* actualSwarm, const QVariantMap& context) const;
    void refreshResolvedBindings();
    void retainResolvedTargets();
    void disconnectReviewSources();
    void watchReviewSources(quint64 generation);
    void invalidate(quint64 generation, const QString& reason);
    QVariantMap result(bool ok, const QString& reason) const;

    struct ResolvedTarget {
        FleetOccupancyRecoveryEvidence::Target source;
        QVariantMap identity;
    };
    QPointer<SwarmController> _swarm;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _authorization;
    QList<FleetOccupancyRecoveryEvidence::Target> _targets;
    QVariantList _targetIdentities;
    QVariantList _vehicleIds;
    QVariantMap _context, _identity, _role, _approver;
    QList<ResolvedTarget> _resolvedTargets;
    QList<QMetaObject::Connection> _reviewConnections;
    QList<QMetaObject::Connection> _resolvedConnections;
    QString _workflowEpoch, _reviewId, _bindingHash, _dispositionReference;
    QString _state = QStringLiteral("empty"), _reason;
    quint64 _generation = 0;
    qint64 _deadlineMs = -1;
    bool _busy = false, _prompt = false, _consuming = false, _authenticationConsumed = false;
    bool _groundInspectionConfirmed = false, _localReuseOnlyConfirmed = false;
    bool _localHoldsReleased = false;
    bool _controllerManaged = false;
    mutable bool _authorizationChecking = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

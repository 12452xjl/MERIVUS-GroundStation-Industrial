#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

class FleetOperatorSession;
class FleetRegistry;
class FleetRolePolicy;
class SwarmController;

// One actual Swarm/Registry's local engineering-attestation review. Neither
// account approval nor a Registry fingerprint authenticates physical hardware,
// detects modifications, certifies performance, or releases flight commands.
class FleetCapabilityReview final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetCapabilityReview() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, int vehicleId, const QString& operation,
        const QStringList& capabilities, const QString& reason, bool confirmed);
    // Separate entry point/purpose for a proposed canonical numeric declaration.
    // Neither this request nor the resulting local approval certifies performance.
    Q_INVOKABLE QVariantMap requestPerformanceReview(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, int vehicleId,
        const QStringList& capabilities, const QVariantMap& proposedPerformance,
        const QString& reason, bool confirmed);
    Q_INVOKABLE QVariantMap approveAndApply(const QString& reviewId, bool operationConfirmed);
    Q_INVOKABLE bool cancel(const QString& reviewId);
    // Pure comparison of the frozen account/policy basis, not authentication.
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
signals:
    void changed();
private:
    friend class SwarmController;
    friend class FleetOperatorSession;
    FleetCapabilityReview(SwarmController* actualSwarm, FleetRegistry* actualRegistry);
    QVariantMap request(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
        const QString& scopeToken, int vehicleId, const QString& operation,
        const QStringList& capabilities, const QVariantMap& proposedPerformance,
        const QString& reason, bool confirmed);
    bool ownersCurrent(quint64 generation) const;
    bool recordCurrent() const;
    bool memoryCurrent(quint64 generation) const;
    bool rootSourcesCurrent(quint64 generation) const;
    bool sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover);
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    // Closed purpose selected from THIS actual frozen operation, never from a
    // caller-supplied action. Session consumes/tail-checks that exact purpose.
    QString authenticationPurpose() const;
    bool commitBindingCurrent(quint64 generation) const;
    QVariantMap result(bool ok, const QString& reason) const;
    void disconnectSources();
    void watchSources(quint64 generation);
    void invalidate(quint64 generation, const QString& reason);

    QPointer<SwarmController> _swarm;
    QPointer<FleetRegistry> _registry;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    QString _workflowEpoch, _reviewId, _scopeToken, _bindingHash, _recordIdentityFingerprint;
    QString _state = QStringLiteral("empty"), _reason;
    int _vehicleId = 0;
    QString _operation, _engineeringReason;
    QStringList _capabilities;
    QVariantMap _registryContext, _previousApproval, _previousReviewIdentity, _configurationApproval, _configuration, _preview;
    QVariantMap _configurationComparison;
    QVariantMap _proposedPerformance, _proposedContext, _performanceBaselineIdentity;
    QVariantMap _identity, _role, _approver, _commitReceipt;
    quint64 _generation = 0;
    qint64 _deadlineMs = -1;
    bool _busy = false, _prompt = false, _consuming = false, _applying = false;
    bool _authenticationConsumed = false, _operationConfirmed = false, _mutationCommitted = false;
    mutable bool _rootChecking = false;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};

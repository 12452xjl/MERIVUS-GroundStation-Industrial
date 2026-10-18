#pragma once

#include <QByteArray>
#include <QList>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetMissionReconciliation;
class FleetHealthCenter;
class FleetIntentTask;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
class FleetRolePolicy;
class FleetOperatorSession;
class FleetOperationApproval;
class LinkInterface;
class MissionManager;
class PlanManager;
class SwarmController;
class Vehicle;

// Native, grounded mission staging. Verification means a fresh onboard
// content readback matched the frozen payload, NEVER permission to fly.
// Native observation bindings must still be current at every continuation.
class FleetMissionDeployment : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap reconciliation READ reconciliation NOTIFY reconciliationChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(QVariantMap startReadiness READ startReadiness NOTIFY inspectionChanged)
    Q_PROPERTY(QVariantMap progressObservation READ progressObservation NOTIFY inspectionChanged)
public:
    explicit FleetMissionDeployment(QObject* parent = nullptr);
    ~FleetMissionDeployment() override;
    QVariantMap current() const;
    QVariantMap reconciliation() const;
    Q_INVOKABLE QVariantMap requestReconciliation(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* swarmObject, QObject* authorizationOwner,
        const QString& expectedDeploymentId, bool operatorConfirmed);
    Q_INVOKABLE bool cancelReconciliation(const QString& requestId);
    // Value-only copy of the original archived planning input after this
    // deployment's independent diagnostic read has ended and drained. This
    // neither edits a draft nor restores a review/upload/start authorization.
    Q_INVOKABLE QVariantMap repairDraftSource(const QString& expectedDeploymentId,
        const QString& expectedReconciliationRequestId) const;
    QVariantList history() const { return _history; }
    QVariantMap startReadiness() const;
    QVariantMap progressObservation() const;

    // Explicit read-only inspection of this native staging instance. Caller
    // cannot supply telemetry, payload, approval or a substituted Vehicle.
    Q_INVOKABLE QVariantMap inspectExecutionState(const QString& deploymentId);

    Q_INVOKABLE QVariantMap beginUpload(QObject* storeObject, QObject* intentObject,
        QObject* reviewObject, QObject* healthObject, QObject* swarmObject,
        QObject* rolePolicyObject, QObject* sessionObject, QObject* approvalObject,
        QObject* authorizationOwner, const QVariantMap& binding,
        const QVariantMap& authorizationReceipt, const QVariantMap& identityReceipt,
        const QVariantMap& approvalReceipt, bool operatorConfirmed);
    Q_INVOKABLE bool cancel(const QString& deploymentId, const QString& reason = QString());

    // Native-only historical content provenance for a NEW independent review.
    // None of these getters renews live staging/intent/command authorization.
    QVariantMap startReviewContext(const QString& deploymentId) const;
    // Original value-only operator input for planning a NEW task. Exact
    // frozen binding only; never a latest-record lookup or flight authority.
    QVariantMap planningTaskForRun(const QVariantMap& binding) const;
    // Revoke old live upload eligibility BEFORE a separate fresh READ begins.
    // Retains only immutable upload/readback history; no task write or rollback.
    bool prepareStartContentCheck(const QString& deploymentId);
    Vehicle* startReviewVehicle(const QString& deploymentId, int vehicleId) const;
    MissionManager* startReviewManager(const QString& deploymentId, int vehicleId) const;

signals:
    void changed();
    void reconciliationChanged();
    void inspectionChanged();

private:
    friend class FleetMissionDeploymentEpisode;
    friend class FleetMissionReconciliation;
    friend class PlanManager;
    friend class Vehicle;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Target {
        int vehicleId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> manager;
        // Original primary for this actual scoped transport operation. A lost
        // captured source remains lost, never a same-ID replacement lookup.
        QPointer<LinkInterface> transportPrimary;
        std::weak_ptr<LinkInterface> transportPrimaryOwner;
        bool transportPrimaryCaptured = false;
        QVariantMap payload;
        QString state = QStringLiteral("waiting");
        quint64 operationId = 0;
        quint64 writeOperationId = 0;
        quint64 readOperationId = 0;
        int awaitingType = 0;
        qint64 deadlineMs = 0;
        bool uploadAcknowledged = false;
        bool readbackVerified = false;
        // Historical decoded-item diagnostics only. No observation is {}, not
        // a match; terminal history removes per-field rows from this value.
        QVariantMap readbackDiagnostic;
        QVariantMap historicalReadbackReceipt;
        bool operationError = false;
        bool writeMayHaveCommitted = false;
        QString error;
    };
    bool _publish(quint64 generation);
    bool _reconciliationBusy() const;
    bool _reconciliationSourcesMatch(FleetOperatorSession* session, FleetRolePolicy* roles,
        FleetMissionPlanStore* store, SwarmController* swarm, QObject* authorization);
    bool _reconciliationContextAllowed(QObject* authorization, const QString& requestId,
        const QVariantMap& binding, const QVariantList& vehicleIds);
    bool _same(quint64 generation) const;
    bool _active() const;
    bool _stackBusy() const;
    bool _gate(quint64 generation, bool requireReservation, QString* reason);
    bool nativeTransportContextCurrent(PlanManager* actualManager, Vehicle* actualVehicle,
        LinkInterface* actualPrimary, const QString& deploymentId, quint64 generation,
        int transactionType, quint64 operationId) const;
    bool _vehicleReady(const Target& target, bool requireIdle, QString* reason) const;
    void _connectContext(quint64 generation);
    void _connectTarget(int index, quint64 generation);
    void _disconnectAll();
    void _uploadNext(quint64 generation);
    void _readback(int index, quint64 generation);
    void _resumeDeferred(quint64 generation);
    void _started(int index, quint64 generation, quint64 operationId, int type);
    void _sent(int index, quint64 generation, quint64 operationId, bool error);
    void _read(int index, quint64 generation, quint64 operationId, bool removeAll);
    void _error(int index, quint64 generation, quint64 operationId, const QString& message);
    void _fail(quint64 generation, const QString& reason);
    void _tick();
    bool _drain();
    void _archive();
    QVariantMap _inspectionSnapshot(const QVariantMap& captured) const;

    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionReconciliation> _reconciliation;
    QPointer<FleetIntentTask> _intent;
    QPointer<FleetMissionReviewCoordinator> _review;
    QPointer<FleetHealthCenter> _health;
    QPointer<SwarmController> _swarm;
    QPointer<FleetRolePolicy> _rolePolicy;
    QVariantMap _uploadRoleReceipt;
    QPointer<FleetOperatorSession> _operatorSession;
    QPointer<FleetOperationApproval> _operationApproval;
    QVariantMap _uploadIdentityReceipt;
    QVariantMap _uploadApprovalGrant;
    QPointer<QObject> _authorization;
    QList<QMetaObject::Connection> _connections;
    QList<Target> _targets;
    QVariantMap _binding;
    QVariantMap _recordSnapshot;
    QVariantMap _planningTask;
    QByteArray _planningParametersFingerprint;
    QString _planningTaskUnavailableReason;
    QVariantMap _reviewSnapshot;
    QVariantMap _payloadBuild;
    QVariantList _vehicleIds;
    QVariantList _history;
    QString _deploymentId;
    QString _state;
    QString _reason;
    QString _createdAt;
    quint64 _generation = 0;
    int _nextTarget = 0;
    bool _beginInProgress = false;
    bool _gateInProgress = false;
    bool _gateReentered = false;
    bool _dispatching = false;
    bool _inspectionInProgress = false;
    QVariantMap _startReadinessSnapshot;
    QVariantMap _progressObservationSnapshot;
    qint64 _inspectionCapturedMs = -1;
    quint64 _inspectionGeneration = 0;
    QString _inspectionState;
    bool _reservationHeld = false;
    bool _transportDraining = false;
    int _deferredStep = 0; // 1: next upload, 2: current readback
    int _deferredTarget = -1;
    qint64 _deferredDeadlineMs = 0;
    QTimer _timer;
    QElapsedTimer _clock;
};

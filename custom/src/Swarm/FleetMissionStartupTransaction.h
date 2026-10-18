#pragma once

#include "LinkInterface.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetFirmwareAdapterCatalog;
class FleetHealthCenter;
class FleetMissionDeployment;
class FleetMissionExecutionObservation;
class FleetMissionObservationClose;
class FleetMissionPlanStore;
class FleetMissionReviewCoordinator;
class FleetMissionStartupClearanceWitness;
class FleetMissionStartupGeographicMotion;
class FleetMissionStartupAuthority;
class FleetMissionGroundStartupAuthority;
class FleetStartupMemorySeal;
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
class VehicleMissionExecutionObserver;

// Native scoped command transaction. No arbitrary command/force-arm input,
// implicit mission-start adapter, auto retry, disarm or rollback.
class FleetMissionStartupTransaction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantMap executionObservation READ executionObservation NOTIFY executionObservationChanged)
public:
    explicit FleetMissionStartupTransaction(QObject* parent = nullptr);
    ~FleetMissionStartupTransaction() override;
    QVariantMap current() const;
    QVariantMap executionObservation() const;
    // Historical planning values from this instance's real prepared Run.
    // No caller telemetry, imported report, command or renewed permission.
    Q_INVOKABLE QVariantMap remainderPlanningSource(const QString& expectedRunId,
        int sourceVehicleId) const;
    // One complete Run snapshot, including explicitly unavailable uncommitted
    // members. Does not loop the single-member getter or restore authority.
    Q_INVOKABLE QVariantMap remainderPlanningCollection(const QString& expectedRunId) const;
    // Pure value consistency for Archive. No QObject/native authentication.
    // exact3 {ok,reason,collection}; failure collection is empty.
    static QVariantMap validateCollection(const QVariantMap& collection);
    // Always validates the whole collection before projecting an available
    // member's original native18; exact3 {ok,reason,source}.
    static QVariantMap sourceFromCollection(const QVariantMap& collection, int sourceVehicleId);
    // Native queue boundary lookup only. No caller-provided observer or data
    // may replace this generation's actual prepared execution observer.
    VehicleMissionExecutionObserver* executionObserverForQueue(QObject* actualVehicle,
        QObject* actualPrimary, const QString& startupId, const QString& modeStepId) const;
    // Ends monitoring/local reservation only, never the onboard task. Not an
    // invokable flight action or a claim that the aircraft has stopped.
    bool retireExecutionObservation(const QString& startupId, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap requestStartup(const QVariantMap& actualSources,
        const QString& startupId, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap requestGroundStartup(const QVariantMap& actualSources,
        const QString& startupId, bool operatorConfirmed);
    Q_INVOKABLE bool cancel(const QString& startupId);
    bool revokeStartup(const QString& startupId);
    QString transactionEpoch() const { return _transactionEpoch; }
    bool authenticationTransferContextCurrent(const QString& startupId, const QString& sourceHash,
        QObject* actualAuthority, const QVariantMap& exactAuthorityContext);
    // Passive ownership fence for the native Vehicle lane, NOT a permission
    // claim from caller data. The sender also MUST invoke its full fence.
    bool commandContextCurrent(QObject* actualVehicle, QObject* actualPrimary,
        const QString& startupId, const QString& stepId, int step) const;
    // Only the actual new-ground native basis may skip an ARM packet. This
    // passive lookup never accepts a caller boolean, ACK or synthetic receipt.
    bool reviewedArmedGroundBasisCurrent(QObject* actualVehicle, const QString& startupId,
        const QVariantMap& exactReadiness) const;
    // The native lane derives its additional periodic clock from this actual
    // frozen policy, never a caller flag. This pure lookup grants no command.
    bool odometryContinuityRequired(QObject* actualVehicle, const QString& startupId) const;
    // Pure native lookup of the reviewed EST gap bound. No caller default,
    // authentication prompt, source callback or extension of the old budget.
    qint64 estimatorContinuityMaximumGapMs(QObject* actualVehicle, const QString& startupId) const;
    // Actual frozen target lookup only; no QML map can install a baseline.
    // Legacy policies explicitly return required=false. Invalid context is {}.
    // A retained terminal basis is historical only, not renewed authorization.
    QVariantMap reviewedAltitudeContext(QObject* actualVehicle, QObject* actualPrimary,
        const QString& startupId) const;
    // Original immutable native model only. No prepare, source recapture,
    // callback or renewed permission at a phase lookup.
    std::shared_ptr<const FleetMissionStartupGeographicMotion> startupGeographicMotionBasis(
        QObject* actualVehicle, const QString& startupId, const QVariantMap& exactReadiness) const;
    bool phaseBasisCurrent(QObject* actualVehicle, const QString& startupId,
        const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& payload,
        const QVariantMap& exactReadiness, const QVariantList& orderedStepIds, int phase) const;
    // NEW-read eligibility/reference only after terminal drain. Never restores
    // the old authentication, command fence, or original expired budget.
    QVariantMap terminalReassessmentContext(const QString& startupId);
    bool terminalReassessmentContextCurrent(const QString& startupId, const QVariantMap& exactContext);
    bool terminalReassessmentSourcesRetained(QObject* actualReview, const QString& startupId,
        const QVariantMap& exactContext) const;
    QVariantMap actualSourcesForReassessment(const QString& startupId);
    Vehicle* terminalReassessmentVehicle(const QString& startupId, int vehicleId) const;
    MissionManager* terminalReassessmentManager(const QString& startupId, int vehicleId) const;
    LinkInterface* terminalReassessmentPrimaryLink(const QString& startupId, int vehicleId) const;
    FleetFirmwareAdapterCatalog* nativeReassessmentFirmwareCatalog(const QString& startupId) const;
    FleetMissionReviewCoordinator* nativeReassessmentReviewCoordinator(const QString& startupId) const;
    FleetRegistry* nativeReassessmentRegistry(const QString& startupId) const;
    FaultToleranceManager* nativeReassessmentFaultPolicy(const QString& startupId) const;
signals:
    void changed();
    void executionObservationChanged();
private:
    friend class FleetMissionDeploymentEpisode;
    friend class FleetMissionStartupSeries;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    friend class FleetMissionExecutionObservation;
    friend class FleetMissionObservationClose;
    QVariantMap observationCloseContext(const QString& startupId) const;
    QVariantMap observationCloseSourceObjects(const QString& startupId, const QVariantMap& context) const;
    bool observationCloseContextCurrent(const QString& startupId, const QVariantMap& context) const;
    bool commitObservationClose(const QString& startupId, const QVariantMap& context);
    QVariantMap observationCloseDisposition(const QString& startupId, const QVariantMap& context) const;
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        QVariantMap member;
        QVariantMap payload;
        QVariantMap beforeArm;
        std::shared_ptr<const FleetMissionStartupGeographicMotion> geographicMotion;
        QVariantMap readReceipt;
        bool reviewedArmed = false;
        QVariantList stepIds;
        QVariantList stepResults;
        QString state = QStringLiteral("queued");
        QString reason;
        QString operationId;
        bool queued[3] = {false, false, false};
        bool observed[3] = {false, false, false};
        bool uncertain[3] = {false, false, false};
    };
    struct PhaseMemorySeal {
        int vehicleId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<LinkInterface> primary;
        std::shared_ptr<const FleetStartupMemorySeal> seal;
    };
    bool _live(quint64 generation) const;
    QVariantMap _requestStartup(const QVariantMap& actualSources, const QString& startupId,
        bool operatorConfirmed, bool groundRestart);
    QVariantList _orderedSteps(const Target& target, int stage, bool issued) const;
    bool _same(quint64 generation) const;
    bool _targetsCurrent(quint64 generation, bool* allObserved, QString* reason, QList<PhaseMemorySeal>* memorySeals);
    bool _phaseMemoryCurrent(quint64 generation) const;
    void _retirePhaseMemorySeals();
    bool _gate(quint64 generation, bool requireAuthentication, bool* allObserved = nullptr);
    void _tick();
    void _stop(quint64 generation, const QString& state, const QString& reason);
    void _drain();
    void _detachDrain();
    void _connectSources(quint64 generation);
    void _disconnectSources();
    bool _publish(quint64 generation);
    QVariantMap _bundle() const;
    bool _terminalReassessmentEligible(const QString& startupId, bool allowOwnedRead = false,
        QObject* actualReview = nullptr) const;
    QVariantMap _captureTerminalReassessmentContext(const QString& startupId, bool retainReference);
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<FleetMissionStartupAuthority> _authority;
    QPointer<FleetMissionGroundStartupAuthority> _groundAuthority;
    QPointer<FleetMissionExecutionObservation> _executionObservation;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<SwarmController> _swarm;
    QPointer<FleetHealthCenter> _health;
    QPointer<QObject> _authorization;
    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetRegistry> _registry;
    QPointer<FaultToleranceManager> _faultPolicy;
    QList<Target> _targets;
    QList<PhaseMemorySeal> _phaseMemorySeals;
    quint64 _phaseMemoryGeneration = 0;
    int _phaseMemoryStage = -1;
    QString _phaseMemoryStartupId;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _authorityContext;
    std::shared_ptr<FleetMissionStartupClearanceWitness> _startupClearanceWitness;
    bool _startupClearanceRequired = false;
    // Fixed native digest of this generation's full actual admission basis.
    // Terminal references embed this summary, never a recursive authority /
    // earlier-review chain or any old authentication as renewed permission.
    QVariantMap _nativeAuthorityReference;
    QVariantMap _authenticationReceipt;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QVariantMap _constraints;
    QVariantMap _binding;
    QVariantMap _healthGate;
    QVariantMap _catalogSnapshot;
    QVariantMap _deploymentContext;
    QVariantMap _planningTask;
    mutable bool _remainderSourceInProgress = false;
    // Exact native capture, never caller-deserialized RAW provenance. Risk
    // comparison may ignore transport arrival only AFTER this exact binding.
    QVariantMap _reassessmentReferenceContext;
    QVariantList _vehicleIds;
    QByteArray _recordBytes;
    QString _startupId;
    QString _transactionEpoch;
    QString _sourceHash;
    QString _state = QStringLiteral("empty");
    QString _reason;
    QString _gateReason;
    quint64 _generation = 0;
    qint64 _expiresAtMs = -1;
    qint64 _stageDeadlineMs = -1;
    qint64 _lastQueueAtMs = -1;
    int _stage = 0;
    int _sendingIndex = -1;
    int _leaderId = 0;
    bool _requireLeader = false;
    bool _leaderRegistered = false;
    bool _active = false;
    bool _reservationHeld = false;
    bool _handoffPending = false;
    bool _transportDraining = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _anyFlightCommandQueued = false;
    bool _deferredSource = false;
    bool _groundRestart = false;
    bool _executionObservationRetired = false;
    // Sticky native fact: only the independently authorized Close commit
    // installs this. Retirement elsewhere never fabricates that history.
    bool _independentObservationCloseCommitted = false;
    QElapsedTimer _clock;
    QTimer _timer;
};

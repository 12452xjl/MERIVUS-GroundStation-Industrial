#pragma once

#include "VehicleMissionExecutionObserver.h"
#include "LinkInterface.h"

#include <QBitArray>
#include <QElapsedTimer>
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class FleetMissionStartupTransaction;
class MissionManager;
class MultiVehicleManager;
class QGCApplication;
class QGCToolbox;
class SwarmController;
class SwarmMissionOrchestrator;
class VehicleLinkManager;

// Actual native MODE-queue observation owner. Local run attribution is not
// remote mission-instance authentication, execution permission or completion.
class FleetMissionExecutionObservation : public VehicleMissionExecutionObserver
{
    Q_OBJECT
public:
    // Native-only value contract. No QObject, authority, credentials or raw
    // payload is exported into the task-model projection.
    struct TaskModelEvidence {
        QVariantMap identity;
        QVariantList targetVehicleIds;
        QVariantMap observation;
        bool modeCommitted = false;
        bool busy = false;
        bool closed = false;
    };
    explicit FleetMissionExecutionObservation(QObject* parent = nullptr);
    ~FleetMissionExecutionObservation() override;
    QVariantMap current() const;
    // Native, explicit end of observation only: never disarm/land/change mode.
    bool retireObservation(bool operatorConfirmed);

signals:
    void changed();

protected:
    bool beginExecutionQueue(Vehicle* vehicle, LinkInterface* link, const QueueContext& context) override;
    void executionQueueCommitted(const QueueContext& context) override;
    void observeExecutionFrame(Vehicle* vehicle, LinkInterface* link, const mavlink_message_t& originalMessage,
        const QString& sourceEpoch, quint64 sourceSequence, qint64 receivedMonotonicMs) override;
    void observeExecutionSourceRetired(Vehicle* vehicle, const QString& sourceEpoch,
        quint64 lastSourceSequence, const QString& fixedCode) override;

private:
    friend class FleetMissionDeploymentEpisode;
    friend class FleetMissionStartupTransaction;
    friend class SwarmMissionOrchestrator;
    bool taskModelEvidence(SwarmMissionOrchestrator* actualOrchestrator, TaskModelEvidence* evidence) const;
    // Success means the same protected model committed this snapshot and the
    // original ReadContext survived all native source probes unchanged.
    bool updateTaskModel();
    bool retireUnissuedPreparation(FleetMissionStartupTransaction* actualStartup);
    // Fixed post-commit reception references, not aircraft sample timestamps
    // or a remote mission identity. Missing references remain absent.
    struct ReceiptRef {
        quint64 sourceSequence = 0;
        qint64 receivedMonotonicMs = -1;
    };
    struct ModeExit {
        quint32 customMode = 0;
        quint8 baseMode = 0;
        quint8 systemStatus = 0;
        ReceiptRef priorMissionReceipt;
        ReceiptRef exitReceipt;
        bool continuityUnknownAtExit = false;
    };
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        int itemCount = 0;
        int lastCommand = 0;
        int currentSequence = -1;
        int reachedSequence = -1;
        int provisionalCurrentSequence = -1;
        int provisionalReachedSequence = -1;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> mission;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        QVariantMap readReceipt;
        QVariantList sources;
        QString payloadHash;
        QString cacheFingerprint;
        QString modeStepId;
        QueueContext queue;
        QBitArray currentSeen;
        QBitArray reachedSeen;
        QMap<QString, qint64> lastRawAt;
        QMap<QString, qint64> lastHostAt;
        QMap<QString, qint64> maximumGap;
        quint64 lastSourceSequence = 0;
        quint64 frames = 0;
        quint64 provisionalFrames = 0;
        quint64 commitBaselineSequence = 0;
        quint64 ignoredFrames = 0;
        quint64 invalidFrames = 0;
        quint64 sourceMissingFrames = 0;
        quint64 duplicateReached = 0;
        quint64 currentRegressions = 0;
        quint64 currentJumps = 0;
        quint64 modeDepartures = 0;
        quint64 homeChanges = 0;
        ReceiptRef missionModeReceipt;
        ReceiptRef lastMissionModeReceipt;
        ModeExit firstModeExit;
        ModeExit lastModeExit;
        ReceiptRef missionCurrentReceipt;
        ReceiptRef missionReachedReceipt;
        ReceiptRef terminalCurrentReceipt;
        ReceiptRef terminalAfterMissionModeReceipt;
        ReceiptRef terminalReachedReceipt;
        ReceiptRef terminalReachedAfterCurrentReceipt;
        ReceiptRef terminalAfterAirborneReceipt;
        ReceiptRef airborneReceipt;
        ReceiptRef landingAfterTerminalReceipt;
        ReceiptRef landedAfterTerminalReceipt;
        int reportedCurrentSequence = -1;
        int reportedMissionTotal = 0;
        int reportedMissionState = 0;
        int reportedMissionMode = 0;
        quint64 retiredSourceSequence = 0;
        qint64 lastRawReceivedAt = -1;
        QString homeHash;
        QString reason;
        QString sourceRetirementCode;
        QString sourceRetirementCodeOrigin;
        bool queueBegan = false;
        bool queueCommitted = false;
        bool provisionalAirborneObserved = false;
        bool provisionalOnGroundObserved = false;
        bool provisionalMissionModeObserved = false;
        bool sourceActive = false;
        bool tracking = false;
        bool cacheMatches = true;
        bool continuityUnknown = false;
        bool armedKnown = false;
        bool armed = false;
        bool missionModeKnown = false;
        bool missionMode = false;
        bool missionModeEverObserved = false;
        bool landedKnown = false;
        int landedState = 0;
        bool bootKnown = false;
        quint32 lastBootMs = 0;
        bool bootRegressionObserved = false;
    };
    struct ReadContext {
        quint64 serial = 0, startupGeneration = 0;
        bool exhausted = false, prepared = false, retired = false, startupRetired = false, ownerDestroying = false;
        bool hadStartup = false, hadSwarm = false, hadOrchestrator = false;
        int targetCount = 0;
        QPointer<FleetMissionStartupTransaction> startup;
        QPointer<SwarmController> swarm;
        QPointer<SwarmMissionOrchestrator> orchestrator;
        QString runId, startupId, startupEpoch, deploymentId, taskModelId;
        QVariantMap binding;
        QVariantList vehicleIds;
    };
    ReadContext readContext() const;
    bool readContextCurrent(const ReadContext& context) const;
    void noteMutation();
    int targetIndex(const Target& original) const;

    bool prepareFromStartup(FleetMissionStartupTransaction* transaction);
    bool readyForQueue(Vehicle* vehicle, LinkInterface* primary,
        const QString& startupId, const QString& modeStepId) const;
    bool hasCommittedMode() const;
    bool adoptReservation(SwarmController* swarm, const QString& previousOwner, const QVariantList& ids);
    void startupRetired(const QString& state, const QString& reason);
    bool nativeTargetCurrent(const Target& target) const;
    bool nativeBindingCurrent(const Target& target) const;
    bool receiptObserved(const Target& target, const ReceiptRef& receipt) const;
    QVariant receiptReference(const Target& target, const ReceiptRef& receipt) const;
    QVariantMap missionModeExitObservation(const Target& target) const;
    QString outcomeCandidate(const Target& target) const;
    QVariantMap targetOutcome(const Target& target, bool sourceCurrent, bool retired) const;
    QVariantMap outcomeSummary(const QList<Target>& targets, bool retired) const;
    bool advance(Target& target, quint64* counter, quint64 amount = 1);
    void invalidate(Target& target, const QString& fixedCode);
    void detachBindings();
    void poll();
    void releaseOwnReservation();

    QPointer<FleetMissionStartupTransaction> _startup;
    QPointer<SwarmController> _swarm;
    QPointer<SwarmMissionOrchestrator> _taskOrchestrator;
    QPointer<QGCApplication> _application;
    QPointer<QGCToolbox> _toolbox;
    QPointer<MultiVehicleManager> _vehicles;
    QString _taskModelId;
    QString _taskModelReason;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _binding;
    QVariantList _vehicleIds;
    QString _runId;
    QString _startupId;
    QString _startupEpoch;
    QString _deploymentId;
    QString _startupState;
    QString _startupReason;
    quint64 _startupGeneration = 0;
    QTimer _timer;
    QElapsedTimer _clock;
    bool _prepared = false;
    bool _startupRetired = false;
    bool _reservationAdopted = false;
    bool _reservationReleased = false;
    bool _retired = false;
    bool _explicitRetirementRequested = false;
    bool _ownerDestroying = false;
    bool _unissuedPreparationClosed = false;
    bool _polling = false;
    quint64 _mutationSerial = 1;
    bool _mutationSerialExhausted = false;
    mutable bool _liveProbeBusy = false;
};

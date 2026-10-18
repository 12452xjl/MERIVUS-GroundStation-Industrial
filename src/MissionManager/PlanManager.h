/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantMap>
#include <functional>

#include "MissionItem.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "LinkInterface.h"

class Vehicle;
class MissionCommandTree;
class FleetMissionArmedGroundReview;
class FleetMissionDeployment;
class FleetMissionReconciliation;

Q_DECLARE_LOGGING_CATEGORY(PlanManagerLog)

/// The PlanManager class is the base class for the Mission, GeoFence and Rally Point managers. All of which use the
/// new mavlink v2 mission protocol.
class PlanManager : public QObject
{
    Q_OBJECT

public:
    PlanManager(Vehicle* vehicle, MAV_MISSION_TYPE planType);
    ~PlanManager();

    bool inProgress(void) const;
    /// Monotonically increasing local identity for the active mission transaction.
    /// This is ground-station metadata only; it is not carried by MAVLink.
    quint64 activeOperationId(void) const { return _activeOperationId; }
    /// True until native entry/protocol/finish stacks unwind, including the
    /// completion-signal interval after inProgress() becomes false. Callers
    /// must queue a subsequent transaction rather than re-enter this stack.
    bool transportCallInProgress(void) const;
    /// Frozen local normal-write ACK evidence, published before any finish
    /// signal. Not readback proof, a signature, or permission to execute.
    quint64 lastAcknowledgedWriteOperationId(void) const { return _lastAcknowledgedWriteOperationId; }
    QString lastAcknowledgedWriteFingerprint(void) const { return _lastAcknowledgedWriteFingerprint; }
    /// Only an explicit public writeMissionItems upload has fresh provenance;
    /// generated resume and guided operations cannot clear upload quarantine.
    bool lastAcknowledgedWriteWasFreshUpload(void) const { return _lastAcknowledgedWriteWasFreshUpload; }
    /// Fingerprint of MissionItem::save JSON (excluding mutable current-item
    /// state). Compare only to the frozen same-operation local ACK evidence.
    static QString missionItemsContentFingerprint(const QList<MissionItem*>& items);

    // Public for consumers which must distinguish the initial read
    // transaction from a later user-initiated write/remove operation.
    typedef enum {
        TransactionNone,
        TransactionRead,
        TransactionWrite,
        TransactionRemoveAll
    } TransactionType_t;
    const QList<MissionItem*>& missionItems(void) { return _missionItems; }

    /// Current mission item as reported by MISSION_CURRENT
    int currentIndex(void) const { return _currentMissionIndex; }

    /// Last current mission item reported while in Mission flight mode
    int lastCurrentIndex(void) const { return _lastCurrentIndex; }

    /// Load the mission items from the vehicle
    ///     Signals newMissionItemsAvailable when done
    /// `allowInitialConnectRead` is reserved for the
    /// InitialConnectStateMachine passive read. UI/user-initiated reads must
    /// use the default false and therefore require initialConnectComplete.
    void loadFromVehicle(bool allowInitialConnectRead = false);

    // Native-only authorized, grounded, complete RAW INT mission read. The
    // fence is mandatory and checked at every send/retry, never a QML bool.
    QVariantMap loadFleetMissionForStart(LinkInterface* expectedPrimary, const QString& readScopeId,
                                         std::function<bool()> authorizationFence);
    // Separate actual-owner READ admission after a stopped/drained startup.
    // Ground POSCTL only; each target's actual armed state is frozen, never
    // changed by this operation. Does not relax the ordinary disarmed path.
    QVariantMap loadFleetMissionForArmedGroundReview(LinkInterface* expectedPrimary, const QString& readScopeId,
        QObject* actualReview, std::function<bool()> authorizationFence);
    QVariantMap fleetMissionArmedGroundReadReceipt(qint64 maximumAgeMs, QObject* actualReview) const;
    bool fleetMissionArmedGroundReadReceiptSourceMatches(LinkInterface* expectedPrimary,
        const QString& operationId, const QString& readScopeId, QObject* actualReview) const;
    bool cancelFleetMissionRead(const QString& readScopeId);
    bool fleetMissionReadScopeCurrent(const QString& readScopeId) const;
    bool ownsFleetMissionRead(const QString& readScopeId, LinkInterface* expectedPrimary,
                              quint64 operationId = 0) const;
    QVariantMap fleetMissionReadReceipt(qint64 maximumAgeMs = 5000) const;
    bool fleetMissionReadReceiptSourceMatches(LinkInterface* expectedPrimary,
                                              const QString& operationId, const QString& readScopeId) const;
    // Passive identity of an actual retained completed RAW read, including
    // cache/source/operation and original read-start age. Deliberately does
    // not assert a grounded flight phase; the startup owner must gate phases.
    // It cannot authorize a send, extend freshness, or restore a JSON receipt.
    bool fleetMissionReadReceiptRetainedSourceMatches(LinkInterface* expectedPrimary,
                                                      const QVariantMap& actualReceipt) const;

    /// Writes the specified set of mission items to the vehicle
    /// IMPORTANT NOTE: PlanManager will take control of the MissionItem objects with the missionItems list. It will free them when done.
    ///     @param missionItems Items to send to vehicle
    ///     Signals sendComplete when done
    void writeMissionItems(const QList<MissionItem*>& missionItems);

    /// Removes all mission items from vehicle
    ///     Signals removeAllComplete when done
    void removeAll(void);

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        AckTimeoutError,        ///< Timed out waiting for response from vehicle
        ProtocolError,          ///< Incorrect protocol sequence from vehicle
        RequestRangeError,      ///< Vehicle requested item out of range
        ItemMismatchError,      ///< Vehicle returned item with seq # different than requested
        VehicleAckError,        ///< Vehicle returned error in ack
        MissingRequestsError,   ///< Vehicle did not request all items during write sequence
        MaxRetryExceeded,       ///< Retry failed
        MissionTypeMismatch,    ///< MAV_MISSION_TYPE does not match _planType
    } ErrorCode_t;

    // These values are public so the unit test can set appropriate signal wait times
    // When passively waiting for a mission process, use a longer timeout.
    static const int _ackTimeoutMilliseconds = 1500;
    // When actively retrying to request mission items, use a shorter timeout instead.
    static const int _retryTimeoutMilliseconds = 250;
    static const int _maxRetryCount = 5;

signals:
    /// Emitted when a new read/write/remove-all/guided operation begins. The legacy
    /// completion signals below remain unchanged for source compatibility.
    void operationStarted                          (quint64 operationId, int transactionType);
    void newMissionItemsAvailable   (bool removeAllRequested);
    void newMissionItemsAvailableWithOperationId   (quint64 operationId, bool removeAllRequested);
    void inProgressChanged          (bool inProgress);
    void error                      (int errorCode, const QString& errorMsg);
    void errorWithOperationId       (quint64 operationId, int errorCode, const QString& errorMsg);
    void currentIndexChanged        (int currentIndex);
    void lastCurrentIndexChanged    (int lastCurrentIndex);
    void progressPct                (double progressPercentPct);
    void removeAllComplete          (bool error);
    void removeAllCompleteWithOperationId          (quint64 operationId, bool error);
    void sendComplete               (bool error);
    void sendCompleteWithOperationId               (quint64 operationId, bool error);
    void resumeMissionReady         (void);
    void resumeMissionReadyWithOperationId         (quint64 operationId);
    void resumeMissionUploadFail    (void);
    void resumeMissionUploadFailWithOperationId    (quint64 operationId);
    // Independent diagnostic completion; never a legacy cache/start receipt.
    void fleetMissionDiagnosticReadComplete(quint64 operationId, const QString& readScopeId, bool success);

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _ackTimeout(void);

protected:
    enum class TransportCallType { OperationEntry, WriteWorker, TransactionFinish, ProtocolDispatch };
    // Guards private flags through a QPointer, so direct-connected deletion of
    // the manager cannot leave the guard destructor writing freed memory.
    class TransportCallGuard
    {
    public:
        TransportCallGuard(PlanManager* owner, TransportCallType type);
        ~TransportCallGuard();
        bool alive() const { return !_owner.isNull(); }
        TransportCallGuard(const TransportCallGuard&) = delete;
        TransportCallGuard& operator=(const TransportCallGuard&) = delete;
    private:
        QPointer<PlanManager> _owner;
        TransportCallType _type;
    };
    bool operationEntryBlocked(void) const;
    typedef enum {
        AckNone,            ///< State machine is idle
        AckMissionCount,    ///< MISSION_COUNT message expected
        AckMissionItem,     ///< MISSION_ITEM expected
        AckMissionRequest,  ///< MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        AckMissionClearAll, ///< MISSION_CLEAR_ALL sent, MISSION_ACK is expected
        AckGuidedItem,      ///< MISSION_ACK expected in response to ArduPilot guided mode single item send
    } AckType_t;

    void _startAckTimeout(AckType_t ack);
    bool _checkForExpectedAck(AckType_t receivedAck);
    void _readTransactionComplete(void);
    void _handleMissionCount(const mavlink_message_t& message);
    void _handleMissionItem(const mavlink_message_t& message);
    void _handleMissionRequest(const mavlink_message_t& message);
    void _handleMissionAck(const mavlink_message_t& message);
    void _requestNextMissionItem(void);
    void _clearMissionItems(void);
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);
    QString _ackTypeToString(AckType_t ackType);
    QString _missionResultToString(MAV_MISSION_RESULT result);
    void _finishTransaction(bool success, bool apmGuidedItemWrite = false);
    void _requestList(bool* fleetRequestIssued = nullptr);
    void _writeMissionCount(void);
    void _writeMissionItemsWorker(bool freshUpload = false);
    void _clearAndDeleteMissionItems(void);
    void _clearAndDeleteWriteMissionItems(void);
    QString _lastMissionReqestString(MAV_MISSION_RESULT result);
    void _removeAllWorker(void);
    void _connectToMavlink(void);
    void _disconnectFromMavlink(void);
    QString _planTypeString(void);
    // Re-check the live transport at every asynchronous mission protocol
    // edge. A transaction can outlive its confirmation dialog and the vehicle
    // may be armed or disconnected before a retry/callback fires.
    bool _liveTransactionAllowed(bool writeOperation) const;

protected:
    Vehicle*            _vehicle =              nullptr;
    MissionCommandTree* _missionCommandTree =   nullptr;
    MAV_MISSION_TYPE    _planType;

    QTimer*             _ackTimeoutTimer =      nullptr;
    AckType_t           _expectedAck;
    int                 _retryCount;

    TransactionType_t   _transactionInProgress;
    bool                _resumeMission;
    QList<int>          _itemIndicesToWrite;    ///< List of mission items which still need to be written to vehicle
    QList<int>          _itemIndicesToRead;     ///< List of mission items which still need to be requested from vehicle
    int                 _lastMissionRequest;    ///< Index of item last requested by MISSION_REQUEST
    int                 _missionItemCountToRead;///< Count of all mission items to read

    QList<MissionItem*> _missionItems;          ///< Set of mission items on vehicle
    QList<MissionItem*> _writeMissionItems;     ///< Set of mission items currently being written to vehicle
    int                 _currentMissionIndex;
    int                 _lastCurrentIndex;

    quint64             _activeOperationId = 0;
    quint64             _nextOperationId = 0;
    bool                _allowInitialConnectRead = false;

    /// Starts a new local transaction identity for derived PlanManagers.
    quint64 _beginTransaction(TransactionType_t type, bool freshUpload = false);

private:
    friend class Vehicle;
    friend class FleetMissionDeployment;
    friend class FleetMissionReconciliation;
    QVariantMap loadFleetMissionForDiagnostic(FleetMissionReconciliation* actualOwner, LinkInterface* expectedPrimary,
        const QString& readScopeId, int maximumItemCount, std::function<bool()> authorizationFence);
    QVariantMap takeFleetMissionDiagnosticResult(FleetMissionReconciliation* actualOwner, LinkInterface* expectedPrimary,
        const QString& readScopeId, quint64 operationId);
    bool cancelFleetMissionDiagnosticRead(FleetMissionReconciliation* actualOwner, const QString& readScopeId);
    void _clearFleetMissionDiagnosticResult();
    // Actual native deployment-only wrappers. Legacy public entry points do
    // not receive, inherit or reconstruct this per-operation scope.
    void writeFleetDeploymentMissionItems(const QList<MissionItem*>& missionItems, FleetMissionDeployment* actualOwner,
        LinkInterface* expectedPrimary, const QString& deploymentId, quint64 generation, std::function<bool()> authorizationFence);
    void loadFleetDeploymentFromVehicle(FleetMissionDeployment* actualOwner, LinkInterface* expectedPrimary,
        const QString& deploymentId, quint64 generation, std::function<bool()> authorizationFence);
    // Only the original native owner may inspect this during the actual
    // completion callback. A failed ordinary READ also emits the legacy
    // availability signal with an empty cache; that is not a real COUNT(0).
    bool fleetDeploymentReadSucceeded(FleetMissionDeployment* actualOwner, Vehicle* expectedVehicle,
        LinkInterface* expectedPrimary, const QString& deploymentId, quint64 generation, quint64 operationId) const;
    void _writeMissionItemsEntry(const QList<MissionItem*>& missionItems);
    void _loadFromVehicleEntry(bool allowInitialConnectRead);
    bool _beginFleetDeploymentScope(FleetMissionDeployment* actualOwner, LinkInterface* expectedPrimary,
        const QString& deploymentId, quint64 generation, TransactionType_t type, std::function<bool()> authorizationFence);
    bool _fleetDeploymentContextCurrent() const;
    bool _fleetDeploymentFenceCurrent();
    bool _sendFleetDeploymentMessage(LinkInterface* expectedPrimary, const mavlink_message_t& message);
    void _clearFleetDeploymentScopeWhenQuiet();
    bool _fleetDeploymentScoped = false;
    bool _fleetDeploymentClosed = false;
    bool _fleetDeploymentFenceInProgress = false;
    bool _fleetDeploymentFenceReentered = false;
    bool _fleetDeploymentSendInProgress = false;
    bool _fleetDeploymentReadEndAckQueued = false;
    bool _fleetDeploymentReadCompletionSucceeded = false;
    int _fleetDeploymentReadCompletedCount = -1;
    QPointer<FleetMissionDeployment> _fleetDeploymentOwner;
    QPointer<Vehicle> _fleetDeploymentVehicle;
    QPointer<LinkInterface> _fleetDeploymentLink;
    WeakLinkInterfacePtr _fleetDeploymentWeakLink;
    QString _fleetDeploymentId;
    quint64 _fleetDeploymentGeneration = 0;
    quint64 _fleetDeploymentOperationId = 0;
    TransactionType_t _fleetDeploymentTransaction = TransactionNone;
    std::function<bool()> _fleetDeploymentAuthorizationFence;
    enum class FleetReadAdmission { StartDisarmed, ReviewDisarmedPOSCTL, ReviewArmedPOSCTL, DiagnosticDisarmed };
    QVariantMap _loadFleetMissionScoped(LinkInterface* expectedPrimary, const QString& readScopeId,
        FleetMissionArmedGroundReview* actualReview, FleetReadAdmission admission,
        std::function<bool()> authorizationFence, FleetMissionReconciliation* actualDiagnostic = nullptr,
        int diagnosticMaximumItems = 2048);
    bool _fleetReadContextCurrent() const;
    bool _fleetReadFenceCurrent();
    bool _sendFleetReadMessage(const mavlink_message_t& message, bool* requestIssued = nullptr);
    void _handleFleetMissionRawMessage(const mavlink_message_t& message, LinkInterface* sourceLink,
                                       qint64 receivedAtMs);
    void _clearFleetReadPendingItems();
    void _clearFleetReadScope();
    void _setTransactionInProgress(TransactionType_t type);
    void _setTransportCallFlag(TransportCallType type, bool active);
    bool _operationEntryInProgress = false;
    bool _writeWorkerInProgress = false;
    bool _transactionFinishInProgress = false;
    bool _protocolDispatchInProgress = false;
    bool _activeWriteWasFreshUpload = false;
    quint64 _lastAcknowledgedWriteOperationId = 0;
    QString _lastAcknowledgedWriteFingerprint;
    bool _lastAcknowledgedWriteWasFreshUpload = false;
    QString _fleetReadEpoch;
    QElapsedTimer _fleetReadClock;
    QPointer<Vehicle> _fleetReadVehicle;
    QPointer<LinkInterface> _fleetReadLink;
    QString _fleetReadScopeId;
    QString _fleetReadVehicleEpoch;
    QString _fleetReadFlightEpoch;
    FleetReadAdmission _fleetReadAdmission = FleetReadAdmission::StartDisarmed;
    QPointer<FleetMissionArmedGroundReview> _fleetReadReviewOwner;
    QPointer<FleetMissionReconciliation> _fleetReadDiagnosticOwner;
    WeakLinkInterfacePtr _fleetReadDiagnosticPrimary;
    int _fleetReadDiagnosticMaximumItems = 0;
    QString _fleetReadReviewEpoch;
    std::function<bool()> _fleetReadAuthorizationFence;
    quint64 _fleetReadOperationId = 0;
    quint64 _fleetReadRawReceiptSequence = 0;
    qint64 _fleetReadStartedAtMs = -1;
    qint64 _fleetReadCountAtMs = -1;
    qint64 _fleetReadFirstItemAtMs = -1;
    qint64 _fleetReadLastItemAtMs = -1;
    QString _fleetReadCountPayloadHash;
    int _fleetReadExpectedCount = -1;
    bool _fleetReadRequestIssued = false;
    bool _fleetReadEndAckQueued = false;
    bool _fleetReadActive = false;
    bool _fleetReadCancelled = false;
    bool _fleetReadExhausted = false;
    QList<MissionItem*> _fleetReadPendingItems;
    QVariantList _fleetReadRawItems;
    QVariantMap _fleetReadCompletionReceipt;
    QVariantMap _lastFleetDiagnosticResult;
    QPointer<FleetMissionReconciliation> _lastFleetDiagnosticOwner;
    QPointer<Vehicle> _lastFleetDiagnosticVehicle;
    QPointer<LinkInterface> _lastFleetDiagnosticLink;
    WeakLinkInterfacePtr _lastFleetDiagnosticPrimary;
    QVariantMap _lastFleetReadReceipt;
    QPointer<Vehicle> _lastFleetReadVehicle;
    QPointer<LinkInterface> _lastFleetReadLink;
    FleetReadAdmission _lastFleetReadAdmission = FleetReadAdmission::StartDisarmed;
    QPointer<FleetMissionArmedGroundReview> _lastFleetReadReviewOwner;
    QString _lastFleetReadReviewEpoch;
    QString _lastFleetReadFlightEpoch;
    bool _lastFleetReadReceiptAvailable = false;
};

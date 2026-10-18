#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantMap>
#include <QPointer>
#include <memory>

class FleetMissionDeploymentEpisode;
class FleetMissionStartupAuthority;
class FleetMissionStartupTransaction;
class FleetMissionGroundStartApproval;
class FleetMissionGroundStartupAuthority;
class FleetMissionObservationClose;
class FleetOccupancyRecoveryReview;
class FleetFirmwareCatalogReload;
class FleetCapabilityReview;
class FleetOptionalTelemetryConfiguration;
class FleetCameraStreamControl;

#ifdef Q_OS_WIN
class FleetWindowsSessionWatch;
#endif

// Local Windows credentials and OS-managed role membership. This is not a
// cloud account, signed process boundary, or permission to issue flight commands.
class FleetOperatorSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)

public:
    explicit FleetOperatorSession(QObject* parent = nullptr);
    ~FleetOperatorSession() override;
    QVariantMap current() const;

    // Process-wide native GUI authentication stack, including modal callbacks
    // and final publication. Wrong-thread/no-application queries fail closed.
    // Not an identity/permission check and deliberately not exposed to QML.
    static bool authenticationPromptInProgress();

    Q_INVOKABLE QVariantMap authenticateOperator();
    Q_INVOKABLE void logout();
    Q_INVOKABLE QVariantMap identitySnapshot() const;
    Q_INVOKABLE bool identitySnapshotCurrent(const QVariantMap& receipt) const;

    // Only an actual native approval service may request/check this receipt.
    // An applicant and an independent approver must be different TokenUser SIDs.
    QVariantMap authenticateApprover(const QString& action, const QString& bindingHash,
                                    const QVariantMap& applicantIdentity, const QString& operationSummary);
    bool approverSnapshotCurrent(const QVariantMap& receipt) const;

    // One-use approval identity -> bounded upload-only lease. Never refreshes
    // either login lifetime; no QML entry point or deserialization restoration.
    QVariantMap consumeUploadIdentityLease(const QVariantMap& primary, const QVariantMap& approver,
                                          const QString& leaseId, int maximumDurationMs);
    bool uploadIdentityLeaseCurrent(const QVariantMap& receipt) const;
    bool revokeUploadIdentityLease(const QString& leaseId);

    // Independently reviewed start intent ONLY. This consumes authentication,
    // not a startup permit, upload lease, ARM command, or takeoff command.
    QVariantMap consumeStartApprovalAuthentication(const QVariantMap& primary, const QVariantMap& approver,
                                                  const QString& bindingHash, const QString& approvalId);
    bool startApprovalAuthenticationCurrent(const QVariantMap& receipt) const;
    bool revokeStartApprovalAuthentication(const QString& approvalId);

    // A separate actual-owner ground-reassessment CONDITIONAL identity basis.
    // Never adopts/overwrites an old StartApproval, Authority or transaction.
    QVariantMap consumeGroundStartApprovalAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        QObject* actualApproval, const QString& approvalId, const QString& approvalEpoch, const QString& bindingHash,
        const QString& reviewId, const QString& reviewEpoch, int maximumRemainingMs);
    bool groundStartApprovalAuthenticationCurrent(const QVariantMap& receipt, QObject* actualApproval,
        const QString& approvalId, const QString& approvalEpoch, const QString& bindingHash,
        const QString& reviewId, const QString& reviewEpoch) const;
    bool revokeGroundStartApprovalAuthentication(QObject* actualApproval, const QString& approvalId,
        const QString& approvalEpoch);
    // Dedicated ground-basis move only. Original login/approval/read budgets
    // do not restart, and old conditional-owner revocation cannot clear it.
    QVariantMap transferGroundStartApprovalAuthentication(const QVariantMap& primary, const QVariantMap& basis,
        QObject* actualApproval, QObject* actualGroundAuthority, const QString& startupId,
        const QString& authorityEpoch, const QString& sourceHash, int maximumRemainingMs);
    bool groundStartupAuthenticationCurrent(const QVariantMap& receipt, QObject* actualGroundAuthority,
        const QString& startupId, const QString& authorityEpoch, const QString& sourceHash) const;
    bool revokeGroundStartupAuthentication(QObject* actualGroundAuthority, const QString& startupId,
        const QString& authorityEpoch);
    // Independent ground Authority -> actual Transaction move; this private
    // proof never replaces the existing non-ground transaction proof.
    QVariantMap transferGroundStartupAuthenticationToTransaction(const QVariantMap& authentication,
        QObject* authorityObject, QObject* transactionObject, const QString& startupId,
        const QString& authorityEpoch, const QString& transactionEpoch, const QString& sourceHash,
        int maximumRemainingMs);
    bool groundStartupTransactionAuthenticationCurrent(const QVariantMap& receipt, QObject* transactionObject,
        const QString& startupId, const QString& transactionEpoch, const QString& sourceHash) const;
    bool revokeGroundStartupTransactionAuthentication(QObject* transactionObject, const QString& startupId,
        const QString& transactionEpoch);

    // One actual consumed conditional authentication -> one actual Authority
    // owner/scope. Identity basis only; never a StartPermit or command lease.
    QVariantMap transferStartApprovalAuthentication(const QVariantMap& primary, const QVariantMap& authentication,
        QObject* authorityObject, const QString& startupId, const QString& authorityEpoch,
        const QString& sourceHash, int maximumRemainingMs);
    bool startupAuthenticationCurrent(const QVariantMap& receipt, QObject* authorityObject,
        const QString& startupId, const QString& authorityEpoch, const QString& sourceHash) const;
    bool revokeStartupAuthentication(QObject* authorityObject, const QString& startupId, const QString& authorityEpoch);
    QVariantMap transferStartupAuthenticationToTransaction(const QVariantMap& authentication, QObject* authorityObject,
        QObject* transactionObject, const QString& startupId, const QString& authorityEpoch,
        const QString& transactionEpoch, const QString& sourceHash, int maximumRemainingMs);
    bool startupTransactionAuthenticationCurrent(const QVariantMap& receipt, QObject* transactionObject,
        const QString& startupId, const QString& transactionEpoch, const QString& sourceHash) const;
    bool revokeStartupTransactionAuthentication(QObject* transactionObject, const QString& startupId,
        const QString& transactionEpoch);

signals:
    void changed();

private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    friend class FleetMissionObservationClose;
    // One immediate, actual-owner monitoring-close workflow only. Burns the
    // pending approver identity; creates no persistent proof or flight lease.
    bool consumeObservationCloseAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetMissionObservationClose* actualOwner, const QString& bindingHash);
    friend class FleetOccupancyRecoveryReview;
    // Independent, immediate local restart/history-review purpose. This
    // consumes actual account approval once, not a lease or aircraft release.
    bool consumeOccupancyRecoveryAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetOccupancyRecoveryReview* actualOwner, const QString& bindingHash);
    // Final pure-memory/original-clock tail fence after actual one-use consume.
    // Review separately proves consumption and the actual source/binding; this
    // is not an independently usable identity receipt, OS check or new lease.
    bool occupancyRecoveryConsumedIdentityCurrent(const QVariantMap& primary,
        const QVariantMap& consumedApprover) const;
    friend class FleetFirmwareCatalogReload;
    // One immediate replacement of the actual local declaration catalog.
    // Consumes this independent account decision once; creates no lease,
    // firmware authentication, signature proof or upload/start permission.
    bool consumeFirmwareCatalogReloadAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetFirmwareCatalogReload* actualOwner, const QString& bindingHash);
    // Final original-clock/current-Session fence for the actual Reload after
    // consumption. The owner separately proves its binding and consumed fact;
    // this passive check performs no OS query and grants no reusable proof.
    bool firmwareCatalogReloadConsumedIdentityCurrent(const QVariantMap& primary,
        const QVariantMap& consumedApprover) const;
    friend class FleetCapabilityReview;
    // Immediate capability, performance-declaration or configuration-baseline decision for one frozen
    // record. The ACTUAL Review operation selects its closed, exact purpose;
    // none of the three purposes can consume or reuse another's approval receipt.
    bool consumeCapabilityReviewAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetCapabilityReview* actualOwner, const QString& bindingHash);
    // Actual Review retains its own consumed fact and frozen sources. This
    // original-clock/current-Session tail performs no OS query and creates no
    // reusable receipt, lease or notification.
    bool capabilityReviewConsumedIdentityCurrent(const QVariantMap& primary,
        const QVariantMap& consumedApprover, const FleetCapabilityReview* actualOwner) const;
    friend class FleetOptionalTelemetryConfiguration;
    // Independent, actual-owner configuration scope. One pending second-account
    // decision is burned once; this private multi-step basis cannot authorize
    // upload/start, restore a capture, or renew either account/capture deadline.
    bool consumeTelemetryConfigurationAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetOptionalTelemetryConfiguration* actualOwner, const QString& bindingHash, int maximumRemainingMs);
    bool telemetryConfigurationAuthenticationCurrent(const FleetOptionalTelemetryConfiguration* actualOwner) const;
    bool telemetryConfigurationConsumedIdentityCurrent(const FleetOptionalTelemetryConfiguration* actualOwner) const;
    bool revokeTelemetryConfigurationAuthentication(FleetOptionalTelemetryConfiguration* actualOwner,
        const QString& bindingHash);
    void clearTelemetryConfigurationAuthentication();
    friend class FleetCameraStreamControl;
    // This typed one-stream control purpose cannot consume READ/interval or
    // flight approvals. Its original review/account deadlines never renew.
    bool consumeCameraStreamControlAuthentication(const QVariantMap& primary, const QVariantMap& approver,
        FleetCameraStreamControl* actualOwner, const QString& bindingHash, int maximumRemainingMs);
    bool cameraStreamControlAuthenticationCurrent(const FleetCameraStreamControl* actualOwner) const;
    bool cameraStreamControlConsumedIdentityCurrent(const FleetCameraStreamControl* actualOwner) const;
    bool revokeCameraStreamControlAuthentication(FleetCameraStreamControl* actualOwner,
        const QString& bindingHash);
    void clearCameraStreamControlAuthentication();
    bool ownerThread() const;
    bool advanceSession();
    bool identityCurrent(const QVariantMap& receipt, bool allowPrompt) const;
    bool workstationCurrent() const;
    void workstationRevoked(const QString& reason);
    void clearUploadIdentityLease();
    void clearStartApprovalAuthentication();
    void clearGroundStartApprovalAuthentication();
    void clearGroundStartupAuthentication();
    void clearGroundStartupTransactionAuthentication();
    void clearStartupAuthentication();
    void clearStartupTransactionAuthentication();
    void expireReceipts();
    static QVariantMap denied(const QString& reason);

    QElapsedTimer _clock;
    QTimer _expiryTimer;
    QString _sessionEpoch;
    quint64 _sessionRevision = 0;
    QVariantMap _identityReceipt;
    QVariantMap _approverReceipt;
    QVariantMap _approvedApplicantIdentity;
    QPointer<FleetOptionalTelemetryConfiguration> _telemetryConfigurationOwner;
    QVariantMap _telemetryConfigurationPrimary, _telemetryConfigurationApprover;
    QString _telemetryConfigurationBindingHash;
    qint64 _telemetryConfigurationDeadlineMs = -1;
    QPointer<FleetCameraStreamControl> _cameraStreamControlOwner;
    QVariantMap _cameraStreamControlPrimary, _cameraStreamControlApprover;
    QString _cameraStreamControlBindingHash;
    qint64 _cameraStreamControlDeadlineMs = -1;
    QVariantMap _uploadIdentityLease;
    QVariantMap _uploadLeasePrimaryIdentity;
    QVariantMap _uploadLeaseApproverIdentity;
    QVariantMap _startApprovalAuthentication;
    QVariantMap _startApprovalPrimaryIdentity;
    QVariantMap _startApprovalApproverIdentity;
    QVariantMap _groundStartApprovalAuthentication;
    QVariantMap _groundStartApprovalPrimaryIdentity;
    QVariantMap _groundStartApprovalApproverIdentity;
    QPointer<FleetMissionGroundStartApproval> _groundStartApprovalOwner;
    QMetaObject::Connection _groundStartApprovalOwnerDestroyedConnection;
    QVariantMap _groundStartupAuthentication;
    QVariantMap _groundStartupOriginalBasis;
    QVariantMap _groundStartupPrimaryIdentity;
    QVariantMap _groundStartupApproverIdentity;
    QPointer<FleetMissionGroundStartupAuthority> _groundStartupOwner;
    QMetaObject::Connection _groundStartupOwnerDestroyedConnection;
    QVariantMap _groundStartupTransactionAuthentication;
    QVariantMap _groundStartupTransactionOriginalAuthorityBasis;
    QVariantMap _groundStartupTransactionOriginalApprovalBasis;
    QVariantMap _groundStartupTransactionPrimaryIdentity;
    QVariantMap _groundStartupTransactionApproverIdentity;
    QPointer<FleetMissionStartupTransaction> _groundStartupTransactionOwner;
    QMetaObject::Connection _groundStartupTransactionOwnerDestroyedConnection;
    qint64 _groundStartupTransactionTransferBudgetDeadlineMs = -1;
    qint64 _groundStartupTransactionTransferredAtMs = -1;
    QVariantMap _startupAuthentication;
    QVariantMap _startupPrimaryIdentity;
    QVariantMap _startupApproverIdentity;
    QPointer<FleetMissionStartupAuthority> _startupAuthenticationOwner;
    QMetaObject::Connection _startupOwnerDestroyedConnection;
    QVariantMap _startupTransactionAuthentication;
    QVariantMap _startupTransactionPrimaryIdentity;
    QVariantMap _startupTransactionApproverIdentity;
    QVariantMap _startupTransactionOriginalBasis;
    QPointer<FleetMissionStartupTransaction> _startupTransactionAuthenticationOwner;
    QMetaObject::Connection _startupTransactionOwnerDestroyedConnection;
    QString _state = QStringLiteral("signed-out");
    QString _lastError;
    bool _promptActive = false;
    bool _revisionExhausted = false;
#ifdef Q_OS_WIN
    friend class FleetWindowsSessionWatch;
    std::unique_ptr<FleetWindowsSessionWatch> _windowsSessionWatch;
#endif
};

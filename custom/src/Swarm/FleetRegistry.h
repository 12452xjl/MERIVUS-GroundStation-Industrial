#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>
#include <functional>
#include <memory>

#include "VehicleCapability.h"

class SwarmController;
class Vehicle;
class ParameterManager;
class VehicleLinkManager;
class LinkInterface;
class MultiVehicleManager;
class QmlObjectListModel;
class Fact;
class FirmwarePlugin;
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
class ArduCopterFirmwarePlugin;
#endif

/// Registry and health summary for a heterogeneous MERIVUS fleet.
///
/// The registry intentionally accepts QVariantMap descriptors so it can be
/// fed by QML, SwarmController, MAVLink handlers, or a future MAVSDK backend.
/// It does not own QGroundControl Vehicle objects and does not send commands.
class FleetRegistry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int vehicleCount READ vehicleCount NOTIFY registryChanged)
    Q_PROPERTY(int onlineVehicleCount READ onlineVehicleCount NOTIFY registryChanged)
    // A vehicle is counted as healthy only after at least one valid health
    // signal has been observed; online-but-unverified vehicles remain unknown.
    Q_PROPERTY(int healthyVehicleCount READ healthyVehicleCount NOTIFY healthChanged)
    Q_PROPERTY(int offlineTimeoutMs READ offlineTimeoutMs WRITE setOfflineTimeoutMs NOTIFY offlineTimeoutMsChanged)
    Q_PROPERTY(QVariantList vehicles READ vehicles NOTIFY registryChanged)
    Q_PROPERTY(QVariantMap fleetHealthSummary READ fleetHealthSummary NOTIFY healthChanged)

public:
    explicit FleetRegistry(QObject* parent = nullptr);

    int vehicleCount() const { return _vehicles.size(); }
    int onlineVehicleCount() const;
    int healthyVehicleCount() const;
    int offlineTimeoutMs() const { return _offlineTimeoutMs; }
    QVariantList vehicles() const;
    QVariantMap fleetHealthSummary() const;

    void setOfflineTimeoutMs(int timeoutMs);

    /// C++ discovery/backend API; intentionally not Q_INVOKABLE so arbitrary
    /// QML surfaces cannot forge, clear, or approve the live fleet registry.
    /// Register or partially merge a descriptor from the C++ discovery/backend
    /// path. The descriptor must contain a positive systemId (vehicleId/id
    /// are accepted as aliases); QML receives snapshots but cannot write here.
    /// Capability evidence and recursive descriptor metadata are bounded before
    /// any merge or configuration fingerprint is computed.
    bool registerVehicle(const QVariantMap& descriptor);
    /// Update only fields present in updates; unknown fields are retained in
    /// metadata.  `metadata.configuration` is accepted as a legacy alias for
    /// static configuration evidence; telemetry updates cannot approve it.
    bool updateVehicle(int systemId, const QVariantMap& updates);
    bool unregisterVehicle(int systemId);
    Q_INVOKABLE bool containsVehicle(int systemId) const;
    Q_INVOKABLE QVariantMap vehicle(int systemId) const;
    // Readonly numerical review from this Registry's actual native Vehicle
    // binding. No caller observation/metadata can install the baseline. Null
    // or zero means no proposal, never SET_MESSAGE_INTERVAL's default action.
    // Even a complete non-increasing comparison grants no configuration,
    // account, telemetry-write or flight permission.
    Q_INVOKABLE QVariantMap optionalTelemetryRateReview(int systemId,
        const QVariant& requestedIntervalUs = QVariant()) const;
    // One passive local receive-window snapshot from this Registry's actual
    // Vehicle binding. A successful read may still have sourceCurrent=false;
    // neither throughput nor this value wrapper proves available bandwidth,
    // latency, packet loss, remote transmit rate or any command permission.
    Q_INVOKABLE QVariantMap nativeLocalReceptionReview(int systemId) const;
    // Passive camera VIDEO_STREAM_INFORMATION/STATUS values from this
    // Registry's actual Vehicle binding. This read never requests a catalog,
    // starts/stops video, authenticates the peer or grants control permission.
    Q_INVOKABLE QVariantMap nativeCameraStreamReview(int systemId) const;
    Q_INVOKABLE QVariantList onlineVehicles() const;
    /// Return vehicles advertising a capability.  The default observes the
    /// broad capability list for compatibility; safety-sensitive callers must
    /// pass requireVerified=true to match only explicit verified evidence.
    Q_INVOKABLE QVariantList vehiclesWithCapability(const QString& capability,
                                                    bool requireVerified = false) const;
    void markSeen(int systemId, const QVariantMap& telemetry = QVariantMap());
    void refreshHealth();
    /// Return the stable SHA-256 fingerprint used by configuration evidence.
    /// This lets a discovery backend compute the same value before publishing
    /// a descriptor, without coupling that backend to VehicleCapability.
    Q_INVOKABLE QString configurationFingerprint(const QVariantMap& configuration) const;
    /// Approve the currently observed configuration locally. This only
    /// changes the ground-station baseline record; it never writes a vehicle.
    /// Requires the current native row's still-complete typed observation;
    /// a hash-only/unknown observation cannot approve a merge-retained map.
    bool approveCurrentConfiguration(int systemId,
                                    const QString& reason = QString());
    /// Native fenced CAS, exact actual comparison16 and immutable receipt14.
    /// The fence/receipt is not account authentication or a flight permit.
    QVariantMap approveCurrentConfiguration(int systemId, const QVariantMap& expectedComparison,
                                            const QString& reason, const std::function<bool()>& finalCommitFence);
    /// Revoke a local approval and return the vehicle to unverified state.
    /// This does not restore physical hardware or flight-controller params.
    bool rollbackConfigurationBaseline(int systemId);
    /// Also permits revoking an actual stored hash-only local approval.
    QVariantMap rollbackConfigurationBaseline(int systemId, const QVariantMap& expectedComparison,
                                               const QString& reason, const std::function<bool()>& finalCommitFence);
    Q_INVOKABLE QVariantMap configurationApproval(int systemId) const;
    /// Actual stored approval only, without the legacy hash-report fallback.
    /// Native readonly API; no actor, hardware or execution authorization.
    QVariantMap configurationApprovalRecord(int systemId) const;
    /// Process-local typed observation history. Missing full observations and
    /// imported hash approvals never manufacture a complete local baseline.
    /// Native only; comparison DTOs are readonly evidence, not authentication.
    QVariantMap configurationComparison(int systemId) const;
    /// Current, privately bound Vehicle observation plus local typed baseline.
    /// Exact3 {ok, reason, evidence}; no QML or imported source can bind it.
    QVariantMap nativeConfigurationEvidence(Vehicle* actualVehicle) const;
    QVariantMap nativeConfigurationEvidenceCurrent(Vehicle* actualVehicle,
                                                    const QVariantMap& expectedEvidence) const;
    /// Callback-free final group fence; no capture or virtual plugin dispatch.
    bool nativeConfigurationEvidenceMemoryCurrent(Vehicle* actualVehicle,
                                                   const QVariantMap& expectedEvidence) const;
    /// Native, local engineering-attestation workflow. The frozen context is
    /// a CAS identity for this registry record, not actor/hardware proof.
    /// None of these methods is exposed as a QML approval/write interface.
    QVariantMap capabilityReviewContext(int systemId) const;
    bool approveCurrentCapabilities(int systemId, const QVariantMap& expectedContext,
                                    const QStringList& capabilities, const QString& reason);
    /// Native fenced CAS with an immutable local mutation receipt. The fence
    /// is not itself account, hardware or performance authentication.
    QVariantMap approveCurrentCapabilities(int systemId, const QVariantMap& expectedContext,
                                           const QStringList& capabilities, const QString& reason,
                                           const std::function<bool()>& finalCommitFence);
    bool revokeCapabilityApproval(int systemId, const QVariantMap& expectedContext,
                                 const QString& reason);
    QVariantMap revokeCapabilityApproval(int systemId, const QVariantMap& expectedContext,
                                        const QString& reason, const std::function<bool()>& finalCommitFence);
    /// Native-only, pure preview of one COMPLETE numeric declaration. Exact4
    /// {ok,reason,proposedPerformance,proposedContext}; failure clears both
    /// maps. The exact2 proposal has maxSpeedMps/enduranceSeconds, -1 unknown,
    /// finite abs <=2^53-1, speed >=0 or -1, endurance >0 or -1, and at least
    /// one known value. A current complete matching local baseline is required.
    /// No row, approval, observation, revision or authority is changed here.
    QVariantMap previewPerformanceDeclaration(int systemId, const QVariantMap& expectedContext,
                                              const QVariantMap& proposedPerformance) const;
    /// One fenced native CAS for canonical performance, one new capability
    /// revision/context, and the original approval14 with explicit nonempty
    /// canonical capabilities. Keeps the capability mutation receipt14 shape;
    /// action is approve-performance. Does not approve/change the baseline,
    /// restore a source, certify performance or release any flight command.
    QVariantMap approveDeclaredPerformance(int systemId, const QVariantMap& expectedContext,
                                           const QVariantMap& proposedPerformance,
                                           const QVariantMap& expectedProposedContext,
                                           const QStringList& capabilities, const QString& reason,
                                           const std::function<bool()>& finalCommitFence);
    /// Export/import a versioned capability passport. Runtime health is
    /// included as a snapshot, but is recomputed on import from the available
    /// fields so stale health cannot silently become current truth. The
    /// versioned root and approval records reject unknown fields.
    QString exportCapabilityPassport() const;
    bool importCapabilityPassport(const QString& json,
                                  bool replaceExisting = false);
    bool saveCapabilityPassportToFile(const QString& filePath) const;
    bool loadCapabilityPassportFromFile(const QString& filePath,
                                        bool replaceExisting = false);
    void clear();

signals:
    void registryChanged();
    void healthChanged();
    void vehicleAdded(int systemId);
    void vehicleUpdated(int systemId);
    void vehicleRemoved(int systemId);
    void vehicleHealthChanged(int systemId, const QString& state, int score);
    void offlineTimeoutMsChanged();

private slots:
    void _refreshHealthTimer();

private:
    friend class SwarmController;
    // Actual Swarm ownership is checked by the caller. Descriptors, passport
    // imports and QML cannot install a live source through this private API.
    void bindNativeBatterySource(int systemId, Vehicle* source);
    void bindNativeConfigurationSource(int systemId, Vehicle* source, const QVariantMap& configuration);
    void _retireNativeConfigurationSource(int systemId);
    struct NativeConfigurationSource {
        QPointer<Vehicle> vehicle;
        QPointer<ParameterManager> parameters;
        QPointer<FirmwarePlugin> firmwarePlugin;
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
        QPointer<ArduCopterFirmwarePlugin> copterPlugin;
        bool copterCoaxialMotors = false;
#endif
        QPointer<VehicleLinkManager> links;
        QPointer<MultiVehicleManager> manager;
        QPointer<QmlObjectListModel> model;
        std::weak_ptr<LinkInterface> primary;
        QString vehicleRecordEpoch;
        QString bindingEpoch;
        int componentId = 0;
        int vehicleType = -1;
        int firmwareType = -1;
        QString versionSourceEpoch;
        QString motorLayoutParameter;
        QPointer<Fact> motorLayoutFact;
        QVariant motorLayoutValue;
        QVariantMap configuration;
        QVariantMap registryContext;
        QString observationEpoch;
        quint64 observationRevision = 0;
        QList<QMetaObject::Connection> connections;
        bool ready = false;
    };
    bool _nativeConfigurationBindingCurrent(int systemId, const NativeConfigurationSource& expected) const;
    QVariantMap _nativeConfigurationEvidenceValue(int systemId, const NativeConfigurationSource& binding) const;
    struct NativeBatterySource {
        QPointer<Vehicle> vehicle;
        QString vehicleRecordEpoch;
        QString bindingEpoch;
    };
    struct CapabilityReviewState {
        QString vehicleRecordEpoch;
        quint64 revision = 1;
        QString identityFingerprint;
        QVariantMap approval;
        bool exhausted = false;
    };
    struct ConfigurationHistory {
        QString vehicleRecordEpoch;
        QString observationEpoch;
        quint64 revision = 1;
        bool exhausted = false;
        bool currentFullKnown = false;
        bool lastFullKnown = false;
        QVariantMap lastFull;
        QString lastFingerprint;
        QString reportedFingerprint;
        bool previousKnown = false;
        QVariantMap previousDifferent;
        QString previousFingerprint;
        bool baselineKnown = false;
        QVariantMap baseline;
        QString baselineFingerprint;
        QVariantMap baselineApproval;
        QString basis;
        QVariantMap comparison;
        QString currentTypedFingerprint;
        QString baselineTypedFingerprint;
        QString baselineApprovalHash;
        bool baselineMatches = false;
    };
    struct PerformanceDeclarationStage {
        VehicleCapability originalVehicle;
        CapabilityReviewState originalReview;
        ConfigurationHistory originalHistory;
        VehicleCapability vehicle;
        CapabilityReviewState review;
        QVariantMap performance, context, originalContext, configurationApproval;
        QString registryEpoch, originalRecordFingerprint, configurationApprovalFingerprint;
        QString baselineFingerprint;
        bool configurationApprovalPresent = false;
    };
    VehicleCapability* _find(int systemId);
    const VehicleCapability* _find(int systemId) const;
    QVariantMap _vehicleSnapshot(int systemId) const;
    bool _nativeBatteryBindingCurrent(int systemId, const QString& recordEpoch,
                                     const NativeBatterySource& binding) const;
    QVariantMap _configurationBaselineEvidence(int systemId) const;
    static void _recomputeHealthValue(VehicleCapability& vehicle, const QDateTime& nowUtc,
                                      int offlineTimeoutMs);
    void _recomputeHealth(VehicleCapability& vehicle, const QDateTime& nowUtc, bool notify = true);
    bool _registerVehicle(const QVariantMap& descriptor, bool allowTrustedBaseline);
    bool _mergeDescriptor(VehicleCapability& target, const QVariantMap& updates,
                          bool allowTrustedBaseline = false);
    QVariantMap _capabilityContext(const VehicleCapability& vehicle, const CapabilityReviewState& state) const;
    bool _synchronizeCapabilityReview(VehicleCapability& vehicle, CapabilityReviewState& state) const;
    void _projectCapabilityReview(VehicleCapability& vehicle, const CapabilityReviewState& state) const;
    void _removeCapabilityEcho(const VehicleCapability& vehicle, QVariantMap* updates) const;
    bool _observeConfiguration(ConfigurationHistory& history, const VehicleCapability& vehicle,
                               const CapabilityReviewState& review, const QVariantMap& update,
                               const VehicleCapability& parsed) const;
    bool _advanceConfigurationHistory(ConfigurationHistory& history) const;
    void _cacheConfigurationComparison(ConfigurationHistory& history) const;
    bool _configurationHistoryCurrent(int systemId, const ConfigurationHistory& expected) const;
    void _notifyCapabilityMutation(int systemId);
    bool _capabilityMutationStateCurrent(int systemId, const QString& registryEpoch,
                                         const VehicleCapability& vehicle, const CapabilityReviewState& review,
                                         bool configurationApprovalPresent, const QVariantMap& configurationApproval) const;
    bool _stagePerformanceDeclaration(int systemId, const QVariantMap& expectedContext,
                                       const QVariantMap& proposedPerformance,
                                       PerformanceDeclarationStage* stage, QString* reason) const;
    QVariantMap _mutateCapabilityApproval(int systemId, const QVariantMap& expectedContext,
                                          const QStringList& capabilities, const QString& reason, bool revoke,
                                          const std::function<bool()>& finalCommitFence, bool requireFence);
    QVariantMap _mutateConfigurationBaseline(int systemId, const QVariantMap& expectedComparison,
                                             const QString& reason, bool revoke,
                                             const std::function<bool()>& finalCommitFence, bool requireFence);

    QHash<int, VehicleCapability> _vehicles;
    QHash<int, NativeBatterySource> _nativeBatterySources;
    QHash<int, NativeConfigurationSource> _nativeConfigurationSources;
    mutable bool _nativeConfigurationObservationActive = false;
    QString _nativeBatteryBindingsEpoch;
    mutable bool _nativeBatteryProjectionActive = false;
    QHash<int, QVariantMap> _configurationApprovals;
    QString _capabilityRegistryEpoch;
    QHash<int, CapabilityReviewState> _capabilityReviews;
    QHash<int, ConfigurationHistory> _configurationHistories;
    int _offlineTimeoutMs = 3000;
    QTimer _healthTimer;
};

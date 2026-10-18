#pragma once

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <memory>

class Vehicle;
class ParameterManager;
class FleetFirmwareAdapterCatalog;
class FleetFirmwareCatalogReload;
class FleetMissionPlanningWorker;

// Composes the mission review stages into one immutable
// operator-review artifact.  This is deliberately a proposal/report path:
// it never mutates a Vehicle, sends MAVLink, uploads a mission, or releases
// a flight command. Value-only review accepts supplied snapshots; opt-in
// native review additionally observes actual Vehicle/ParameterManager data
// without requesting or writing it. Both retain named evidence snapshots.
class FleetMissionReviewCoordinator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastReview READ lastReview NOTIFY reviewChanged)
    Q_PROPERTY(QVariantMap firmwareAdapterCatalog READ firmwareAdapterCatalogSnapshot NOTIFY firmwareAdapterCatalogChanged)
    Q_PROPERTY(QVariantMap firmwareAdapterCatalogLoadStatus READ firmwareAdapterCatalogLoadStatus NOTIFY firmwareAdapterCatalogChanged)
    Q_PROPERTY(QObject* firmwareCatalogReload READ firmwareCatalogReloadObject NOTIFY firmwareCatalogReloadChanged)
    Q_PROPERTY(QVariantMap planningStatus READ planningStatus NOTIFY planningStatusChanged)

public:
    explicit FleetMissionReviewCoordinator(QObject* parent = nullptr);
    ~FleetMissionReviewCoordinator() override;

    QVariantMap lastReview() const { return _lastReview; }
    // Trusted native engineering/configuration backend only; no QML mutation.
    FleetFirmwareAdapterCatalog* firmwareAdapterCatalogBackend() const;
    QVariantMap firmwareAdapterCatalogSnapshot() const;
    QVariantMap firmwareAdapterCatalogLoadStatus() const { return _firmwareAdapterCatalogLoadStatus; }
    QObject* firmwareCatalogReloadObject() const;
    FleetFirmwareCatalogReload* firmwareCatalogReloadBackend() const;
    // Actual owned objects only; no identifier lookup or imported authority.
    Q_INVOKABLE bool ownsFirmwareCatalogReload(QObject* actualReload, QObject* actualCatalog) const;

    // task is the operator's intent/task description.  Optional snapshots
    // are intentionally not synthesized: missing health, energy, route, or
    // airspace evidence remains an explicit review item in the result.
    Q_INVOKABLE QVariantMap review(const QVariantMap& task,
                                   const QVariantList& fleet = QVariantList(),
                                   const QVariantList& healthReports = QVariantList(),
                                   const QVariant& noFlyZones = QVariant(QVariantList()),
                                   const QVariantMap& options = QVariantMap(),
                                   const QVariantList& riskSignals = QVariantList());

    // Opt-in native source capture; caller-supplied metadata cannot stand in
    // for actual Vehicle/ParameterManager observations. Still read-only.
    Q_INVOKABLE QVariantMap reviewWithNativeEvidence(const QVariantMap& task,
                                   const QVariantList& fleet = QVariantList(),
                                   const QVariantList& healthReports = QVariantList(),
                                   const QVariant& noFlyZones = QVariant(QVariantList()),
                                   const QVariantMap& options = QVariantMap(),
                                   const QVariantList& riskSignals = QVariantList());
    Q_INVOKABLE bool nativeEvidenceCurrent() const;

    // One immutable background compilation at a time. Ready is not published
    // review evidence: the owner must recheck its actual inputs then complete.
    QVariantMap planningStatus() const;
    Q_INVOKABLE QVariantMap beginReviewWithNativeEvidence(const QVariantMap& task,
                                   const QVariantList& fleet = QVariantList(),
                                   const QVariantList& healthReports = QVariantList(),
                                   const QVariant& noFlyZones = QVariant(QVariantList()),
                                   const QVariantMap& options = QVariantMap(),
                                   const QVariantList& riskSignals = QVariantList());
    Q_INVOKABLE QVariantMap completePlanningReview(const QString& requestId);
    Q_INVOKABLE bool cancelPlanningReview(const QString& requestId);

    // Pure value projection for THIS supplied snapshot. Never looks up the
    // current Registry, authenticates its provenance, or changes a review.
    // Fixed exact2 {state: missing|invalid|available, evidence: map14|{}};
    // available may be valid negative evidence, not a baseline permission.
    Q_INVOKABLE QVariantMap configurationBaselineIdentity(const QVariantMap& snapshot) const;

    // Pure validation of the optional native SOC decline sibling. Exact6:
    // state, identity, usable, remainingFlightSeconds, percentPerSecond, ageMs.
    // The stable identity omits natural receipt/age ticks. Usable ageMs is the
    // conservative budget deduction age, not a remote measurement timestamp.
    Q_INVOKABLE QVariantMap nativeBatteryDrainReviewIdentity(const QVariantMap& vehicle) const;

    // Pure proposal construction from a caller-owned native Run snapshot.
    // This does not authenticate an imported source, replace lastReview,
    // allocate vehicles, or restore the original run's execution authority.
    Q_INVOKABLE QVariantMap buildRemainderProposal(const QVariantMap& source,
                                                  const QVariantMap& request) const;

    // One bounded, display-only projection of the supplied compiled remainder
    // record. Neither consumes a Run nor changes lastReview or any approval.
    Q_INVOKABLE QVariantMap buildRemainderView(const QVariantMap& plan) const;
    // Independent complete-group proposal/display contracts. These remain
    // pure value helpers; the ordinary new review owns current evidence.
    Q_INVOKABLE QVariantMap buildGroupRemainderProposal(const QVariantMap& source,
                                                       const QVariantMap& request) const;
    Q_INVOKABLE QVariantMap buildGroupRemainderView(const QVariantMap& plan) const;

    // Pure new-task construction from one complete retained reserve source.
    // Current native evidence and all approval remain with the normal review.
    Q_INVOKABLE QVariantMap buildReserveDispatchProposal(const QVariantMap& source,
                                                         const QVariantMap& request) const;

    // Pure frozen-readiness geometry display. Does not change lastReview,
    // authenticate supplied values, emit review signals or grant permission.
    Q_INVOKABLE QVariantMap startupSpatialReview(const QVariantMap& source) const;

    // Pure whole-plan/archived-evidence display. Rebuilds the unchanged plan
    // once for encoding detail; does not change reviews or flight authority.
    Q_INVOKABLE QVariantMap missionSpatialReview(const QVariantMap& plan,
                                                const QVariantMap& reviewEvidence) const;

    // Independent, pure nominal geometry reassessment of the supplied whole
    // mission projection and startup snapshot. No live lookup or permission.
    Q_INVOKABLE QVariantMap jointSpatialReview(const QVariantMap& source) const;

    // Type-only provenance for the original no-fly-zone input. QVariantList
    // may reach QML as a native wrapper, not a JavaScript Array. No geometry
    // validity, coverage, freshness or authority follows from list presence.
    Q_INVOKABLE QVariantMap airspaceInputEvidence(const QVariantMap& input) const;

    // Same bounded value inspection used by review, exposed for a caller's
    // current-input fence. No review mutation, source lookup or permission.
    Q_INVOKABLE QVariantMap inspectNoFlyZoneInput(const QVariant& input) const;

    // Bounded, read-only actual 12-parameter observation for an operator's
    // explicit executionProfile.verticalPolicy.requiredProfileHash binding.
    // Does not require that hash to be bound already, request data, or grant
    // a review/flight permission. Call again before accepting an old value.
    Q_INVOKABLE QVariantMap executionProfileObservation(const QVariant& vehicleId) const;

    Q_INVOKABLE void clear();

signals:
    void reviewChanged();
    void firmwareAdapterCatalogChanged();
    void firmwareCatalogReloadChanged();
    void planningStatusChanged();
    void planningReady(const QString& requestId);

private:
    QVariantMap _review(const QVariantMap& task, const QVariantList& fleet,
                       const QVariantList& healthReports, const QVariant& noFlyZones,
                       const QVariantMap& options, const QVariantList& riskSignals,
                       bool captureNativeEvidence, const QVariantMap* compiledPlan = nullptr,
                       const QString& compilerFingerprint = QString());
    void _retirePlanning(const QString& reason);
    bool _planningSourcesMatch(const QVariantMap& plan) const;
    QVariantMap _lastReview;
    QMap<int, QPointer<Vehicle>> _nativeReviewVehicles;
    QMap<int, QPointer<ParameterManager>> _nativeReviewParameterManagers;
    QPointer<FleetFirmwareAdapterCatalog> _firmwareAdapters;
    QPointer<FleetFirmwareCatalogReload> _firmwareCatalogReload;
    QPointer<FleetFirmwareAdapterCatalog> _nativeReviewFirmwareAdapters;
    QVariantMap _firmwareAdapterCatalogLoadStatus;
    quint64 _reviewEpoch = 0; // Local invocation identity; never wraps.
    QPointer<FleetMissionPlanningWorker> _planningWorker;
    std::shared_ptr<std::atomic_bool> _planningCancellation;
    QPointer<FleetFirmwareAdapterCatalog> _planningCatalog;
    QMap<int, QPointer<Vehicle>> _planningVehicles;
    QMap<int, QPointer<ParameterManager>> _planningManagers;
    QSet<int> _planningOriginallyPresent;
    QVariantMap _planningInput, _planningPlan, _planningCatalogSnapshot;
    QString _planningRequestId = QStringLiteral("");
    QString _planningInputFingerprint = QStringLiteral(""), _planningCompilerFingerprint = QStringLiteral("");
    QString _planningState = QStringLiteral("idle"), _planningError = QStringLiteral("");
    quint64 _planningEpoch = 0;
    bool _planningThreadActive = false;
    bool _planningEntry = false;
};

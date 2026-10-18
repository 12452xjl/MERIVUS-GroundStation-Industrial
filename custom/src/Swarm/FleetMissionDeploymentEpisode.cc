#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionDeploymentLifecycle.h"
#include "FleetHealthCenter.h"
#include "FaultToleranceManager.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetIntentTask.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionEvidenceRefresh.h"
#include "FleetMissionExecutionObservation.h"
#include "FleetMissionGroundStartApproval.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionObservationClose.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionStartApproval.h"
#include "FleetMissionStartContentCheck.h"
#include "FleetMissionStartReview.h"
#include "FleetMissionStartupAuthority.h"
#include "FleetMissionStartupSeries.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetOperationApproval.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "FleetRegistry.h"
#include "MissionManager.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <QThread>
#include <QUuid>

namespace {
bool uuid(const QString& value)
{
    const QUuid parsed(value);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value;
}
bool terminalTransaction(const QString& state)
{
    return state == QStringLiteral("failed") || state == QStringLiteral("stale") || state == QStringLiteral("cancelled")
        || state == QStringLiteral("expired") || state == QStringLiteral("started-observed");
}
// Only inspect the captured original objects. Missing originals are unknown,
// not "idle", and are never replaced by a same-ID/current-source lookup.
// All getters below are pure native reads: no signal, QML or OS gate.
QVariantMap targetEvidence(int vehicleId, Vehicle* vehicle, MissionManager* manager, ParameterManager* parameters,
    VehicleLinkManager* links, LinkInterface* primary, bool capturedParameters, bool capturedLinks,
    bool capturedPrimary, QThread* ownerThread)
{
    const QPointer<Vehicle> actualVehicle(vehicle);
    const QPointer<MissionManager> actualManager(manager);
    const QPointer<ParameterManager> actualParameters(parameters);
    const QPointer<VehicleLinkManager> actualLinks(links);
    const QPointer<LinkInterface> actualPrimary(primary);
    if (vehicleId < 1 || vehicleId > 255 || QThread::currentThread() != ownerThread
        || (!capturedParameters && actualParameters) || (!capturedLinks && actualLinks) || (!capturedPrimary && actualPrimary)) return {};
    QVariant vehicleQuiet, missionCallBusy, missionProtocolBusy, parameterWritesPending;
    QVariant vehicleMissionMatches, vehicleParametersMatch, vehicleLinksMatch, linksPrimaryMatches;
    bool primaryBindingUnknown = false;
    if (actualVehicle) {
        if (actualVehicle->thread() != ownerThread || actualVehicle->id() != vehicleId) return {};
        const bool quiet = actualVehicle->fleetStartupTransportQuiescent();
        if (!actualVehicle || !quiet) return {};
        vehicleQuiet = quiet;
    }
    // Check a surviving MissionManager even when its original Vehicle is gone.
    if (actualManager) {
        if (actualManager->thread() != ownerThread) return {};
        const bool callBusy = actualManager->transportCallInProgress();
        const bool protocolBusy = actualManager->inProgress();
        if (!actualManager || callBusy || protocolBusy) return {};
        missionCallBusy = callBusy; missionProtocolBusy = protocolBusy;
    }
    if (actualParameters) {
        if (actualParameters->thread() != ownerThread) return {};
        const bool writesPending = actualParameters->pendingWrites();
        if (!actualParameters || writesPending) return {};
        parameterWritesPending = writesPending;
    }
    if (actualLinks && actualLinks->thread() != ownerThread) return {};
    // A Link may legitimately use a worker thread. Retention is not proof of
    // a connected link, current primary link, remote quiet, or physical stop.
    if (actualVehicle && actualManager) {
        if (actualVehicle->missionManager() != actualManager.data()) return {};
        vehicleMissionMatches = true;
    }
    if (actualVehicle && actualParameters) {
        if (actualVehicle->parameterManager() != actualParameters.data()) return {};
        vehicleParametersMatch = true;
    }
    if (actualVehicle && actualLinks) {
        if (actualVehicle->vehicleLinkManager() != actualLinks.data()) return {};
        vehicleLinksMatch = true;
    }
    if (actualLinks && actualPrimary) {
        const SharedLinkInterfacePtr currentPrimary = actualLinks->primaryLink().lock();
        const bool matches = currentPrimary && currentPrimary.get() == actualPrimary.data();
        linksPrimaryMatches = matches;
        // A changed/absent current primary remains unknown, not a replacement
        // source or reason to cancel/release the old run while allocating empty.
        primaryBindingUnknown = !matches;
    }
    if ((vehicle && !actualVehicle) || (manager && !actualManager) || (parameters && !actualParameters)
        || (links && !actualLinks) || (primary && !actualPrimary)) return {};
    const QVariantMap retained{{QStringLiteral("vehicle"), bool(actualVehicle)}, {QStringLiteral("mission"), bool(actualManager)},
        {QStringLiteral("parameters"), capturedParameters ? QVariant(bool(actualParameters)) : QVariant()},
        {QStringLiteral("links"), capturedLinks ? QVariant(bool(actualLinks)) : QVariant()},
        {QStringLiteral("primary"), capturedPrimary ? QVariant(bool(actualPrimary)) : QVariant()}};
    const QVariantMap transport{{QStringLiteral("scope"), QStringLiteral("local-original-object-stacks-only")},
        {QStringLiteral("vehicleReceiveStackQuiet"), vehicleQuiet}, {QStringLiteral("missionCallBusy"), missionCallBusy},
        {QStringLiteral("missionProtocolBusy"), missionProtocolBusy}, {QStringLiteral("parameterWritesPending"), parameterWritesPending},
        {QStringLiteral("vehicleMissionMatches"), vehicleMissionMatches}, {QStringLiteral("vehicleParametersMatch"), vehicleParametersMatch},
        {QStringLiteral("vehicleLinksMatch"), vehicleLinksMatch}, {QStringLiteral("linksPrimaryMatches"), linksPrimaryMatches}};
    return {{QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("sourceRetained"), retained}, {QStringLiteral("localTransport"), transport},
        {QStringLiteral("sourceContinuityUnknown"), !actualVehicle || !actualManager || (capturedParameters && !actualParameters)
            || (capturedLinks && !actualLinks) || (capturedPrimary && !actualPrimary) || primaryBindingUnknown}};
}
QVariantMap component(const QString& name, const QString& state, quint64 generation, const QVariant& targets = QVariant())
{
    return {{QStringLiteral("name"), name}, {QStringLiteral("state"), state.isEmpty() ? QStringLiteral("empty") : state},
        {QStringLiteral("generation"), QString::number(generation)}, {QStringLiteral("targetEvidence"), targets}};
}
}

FleetMissionDeploymentEpisode::FleetMissionDeploymentEpisode(QObject* parent)
    : QObject(parent), _episodeId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _deployment = new FleetMissionDeployment(this);
    _session = new FleetOperatorSession(this); // New signed-out Session, never copied.
    _uploadApproval = new FleetOperationApproval(this);
    _evidenceRefresh = new FleetMissionEvidenceRefresh(this);
    _contentCheck = new FleetMissionStartContentCheck(this);
    _startReview = new FleetMissionStartReview(this);
    _startApproval = new FleetMissionStartApproval(this);
    _startupAuthority = new FleetMissionStartupAuthority(this);
    _initialTransaction = new FleetMissionStartupTransaction(this);
    _groundReview = new FleetMissionArmedGroundReview(this);
    _groundApproval = new FleetMissionGroundStartApproval(this);
    _groundAuthority = new FleetMissionGroundStartupAuthority(this);
    _startupSeries = new FleetMissionStartupSeries(this);
    _observationClose = new FleetMissionObservationClose(this);
    const auto manage = [this](auto* actual) {
        if (!actual) return; // A lost constructor child stays lost; fresh rejects it.
        actual->_deploymentEpisodeManaged = true;
        actual->_deploymentEpisodeOwner = this;
    };
    manage(_deployment.data()); manage(_session.data()); manage(_uploadApproval.data()); manage(_evidenceRefresh.data());
    manage(_contentCheck.data()); manage(_startReview.data()); manage(_startApproval.data()); manage(_startupAuthority.data());
    manage(_initialTransaction.data()); manage(_groundReview.data()); manage(_groundApproval.data()); manage(_groundAuthority.data());
    manage(_startupSeries.data()); manage(_observationClose.data());
    if (_startupSeries && _initialTransaction) _startupSeries->setInitialTransaction(_initialTransaction.data());
}
FleetMissionDeploymentEpisode::~FleetMissionDeploymentEpisode() = default;
QObject* FleetMissionDeploymentEpisode::deployment() const { return _deployment.data(); }
QObject* FleetMissionDeploymentEpisode::session() const { return _session.data(); }
QObject* FleetMissionDeploymentEpisode::uploadApproval() const { return _uploadApproval.data(); }
QObject* FleetMissionDeploymentEpisode::evidenceRefresh() const { return _evidenceRefresh.data(); }
QObject* FleetMissionDeploymentEpisode::contentCheck() const { return _contentCheck.data(); }
QObject* FleetMissionDeploymentEpisode::startReview() const { return _startReview.data(); }
QObject* FleetMissionDeploymentEpisode::startApproval() const { return _startApproval.data(); }
QObject* FleetMissionDeploymentEpisode::startupAuthority() const { return _startupAuthority.data(); }
QObject* FleetMissionDeploymentEpisode::initialTransaction() const { return _initialTransaction.data(); }
QObject* FleetMissionDeploymentEpisode::groundReview() const { return _groundReview.data(); }
QObject* FleetMissionDeploymentEpisode::groundApproval() const { return _groundApproval.data(); }
QObject* FleetMissionDeploymentEpisode::groundAuthority() const { return _groundAuthority.data(); }
QObject* FleetMissionDeploymentEpisode::startupSeries() const { return _startupSeries.data(); }
QObject* FleetMissionDeploymentEpisode::observationClose() const { return _observationClose.data(); }

bool FleetMissionDeploymentEpisode::bundleCurrent() const
{
    if (QThread::currentThread() != thread() || !uuid(_episodeId)) return false;
    const QList<QObject*> sources{_deployment.data(), _session.data(), _uploadApproval.data(), _evidenceRefresh.data(), _contentCheck.data(),
        _startReview.data(), _startApproval.data(), _startupAuthority.data(), _initialTransaction.data(), _groundReview.data(),
        _groundApproval.data(), _groundAuthority.data(), _startupSeries.data(), _observationClose.data()};
    QSet<QObject*> seen;
    for (QObject* actual : sources) {
        if (!actual || actual->thread() != thread() || actual->parent() != this || seen.contains(actual)) return false;
        seen.insert(actual);
    }
    const auto managed = [this](const auto* actual) { return actual->_deploymentEpisodeManaged && actual->_deploymentEpisodeOwner.data() == this; };
    return managed(_deployment.data()) && managed(_session.data()) && managed(_uploadApproval.data()) && managed(_evidenceRefresh.data())
        && managed(_contentCheck.data()) && managed(_startReview.data()) && managed(_startApproval.data()) && managed(_startupAuthority.data())
        && managed(_initialTransaction.data()) && managed(_groundReview.data()) && managed(_groundApproval.data()) && managed(_groundAuthority.data())
        && managed(_startupSeries.data()) && managed(_observationClose.data())
        && !_startupSeries->_evidenceLost && _startupSeries->_initialAssigned && _startupSeries->_initial == _initialTransaction
        && !_startupSeries->_owners.isEmpty() && _startupSeries->_owners.first().transaction == _initialTransaction;
}
bool FleetMissionDeploymentEpisode::outboundCurrent() const
{
    const auto* factory = qobject_cast<const FleetMissionDeploymentLifecycle*>(parent());
    return bundleCurrent() && _publishedForOutbound && !_supersededForOutbound && factory && factory->thread() == thread()
        && factory->currentEpisode() == this;
}
bool FleetMissionDeploymentEpisode::ownsOutboundObject(const QObject* source) const
{
    if (!source || !bundleCurrent()) return false;
    for (QObject* actual : {static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_session.data()), static_cast<QObject*>(_uploadApproval.data()),
        static_cast<QObject*>(_evidenceRefresh.data()), static_cast<QObject*>(_contentCheck.data()), static_cast<QObject*>(_startReview.data()),
        static_cast<QObject*>(_startApproval.data()), static_cast<QObject*>(_startupAuthority.data()), static_cast<QObject*>(_initialTransaction.data()),
        static_cast<QObject*>(_groundReview.data()), static_cast<QObject*>(_groundApproval.data()), static_cast<QObject*>(_groundAuthority.data()),
        static_cast<QObject*>(_startupSeries.data()), static_cast<QObject*>(_observationClose.data())}) if (source == actual) return true;
    if (_startupSeries->_evidenceLost || _startupSeries->_destroying || source->parent() != _startupSeries.data()) return false;
    for (const auto& owner : _startupSeries->_owners)
        if (!owner.lost && owner.transaction.data() == source && owner.transaction->_deploymentEpisodeManaged
            && owner.transaction->_deploymentEpisodeOwner.data() == this) return true;
    return false;
}
FleetMissionDeploymentEpisode::SourceOwner FleetMissionDeploymentEpisode::sourceOwner(const QObject* source)
{
    if (!source || source->thread() != QThread::currentThread()) return {};
    const auto owner = [](const auto* actual) {
        SourceOwner result; result.recognized = true; result.managed = actual->_deploymentEpisodeManaged;
        result.episode = actual->_deploymentEpisodeOwner; return result;
    };
    if (const auto* actual = qobject_cast<const FleetMissionDeployment*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetOperatorSession*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetOperationApproval*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionEvidenceRefresh*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartContentCheck*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartReview*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartApproval*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartupAuthority*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartupTransaction*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionArmedGroundReview*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionGroundStartApproval*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionGroundStartupAuthority*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionStartupSeries*>(source)) return owner(actual);
    if (const auto* actual = qobject_cast<const FleetMissionObservationClose*>(source)) return owner(actual);
    return {}; // Unknown/destructing dynamic type is never a legacy grant.
}
bool FleetMissionDeploymentEpisode::outboundOwnerAllowed(const QObject* source)
{
    const SourceOwner owner = sourceOwner(source);
    if (!owner.recognized) return false;
    if (!owner.managed) {
        if (owner.episode) return false;
        // A new/unknown object underneath an Episode is not a legacy object.
        for (const QObject* parent = source->parent(); parent; parent = parent->parent())
            if (qobject_cast<const FleetMissionDeploymentEpisode*>(parent)) return false;
        return true;
    }
    const auto episode = owner.episode;
    if (!episode || episode->thread() != QThread::currentThread() || !episode->ownsOutboundObject(source)) return false;
    // Only retained monitoring-close is exempt. Normal account facts do not
    // call this outbound-intent helper; Session outbound purposes do call it.
    return source == episode->_observationClose.data() || episode->outboundCurrent();
}
bool FleetMissionDeploymentEpisode::outboundOwnersCompatible(const QObject* source, const QObject* peer)
{
    if (!outboundOwnerAllowed(source) || !outboundOwnerAllowed(peer)) return false;
    const SourceOwner sourceProvenance = sourceOwner(source), peerProvenance = sourceOwner(peer);
    if (!sourceProvenance.recognized || !peerProvenance.recognized) return false;
    if (!sourceProvenance.managed && !peerProvenance.managed) return true;
    return sourceProvenance.managed && peerProvenance.managed && sourceProvenance.episode && peerProvenance.episode
        && sourceProvenance.episode == peerProvenance.episode && sourceProvenance.episode->ownsOutboundObject(source)
        && sourceProvenance.episode->ownsOutboundObject(peer);
}

bool FleetMissionDeploymentEpisode::sharedSourcesMatch(FleetRolePolicy* roles, FleetMissionPlanStore* store, FleetHealthCenter* health) const
{
    if (!bundleCurrent() || !capturedSourcesRetained() || !roles || !store || !health || roles->thread() != thread() || store->thread() != thread() || health->thread() != thread()) return false;
    const auto matches = [](QObject* actual, QObject* expected) { return !actual || actual == expected; };
    if (!matches(_deployment->_rolePolicy.data(), roles) || !matches(_deployment->_store.data(), store) || !matches(_deployment->_health.data(), health)
        || !matches(_uploadApproval->_roles.data(), roles) || !matches(_uploadApproval->_store.data(), store)
        || !matches(_evidenceRefresh->_roles.data(), roles) || !matches(_contentCheck->_roles.data(), roles) || !matches(_contentCheck->_store.data(), store)
        || !matches(_startReview->_roles.data(), roles) || !matches(_startReview->_store.data(), store) || !matches(_startReview->_health.data(), health)
        || !matches(_startApproval->_roles.data(), roles) || !matches(_startApproval->_store.data(), store)
        || !matches(_startupAuthority->_roles.data(), roles) || !matches(_startupAuthority->_store.data(), store) || !matches(_startupAuthority->_health.data(), health)
        || !matches(_groundReview->_roles.data(), roles) || !matches(_groundReview->_store.data(), store) || !matches(_groundReview->_health.data(), health)
        || !matches(_groundApproval->_roles.data(), roles) || !matches(_groundApproval->_store.data(), store) || !matches(_groundApproval->_health.data(), health)
        || !matches(_groundAuthority->_roles.data(), roles) || !matches(_groundAuthority->_store.data(), store) || !matches(_groundAuthority->_health.data(), health)
        || !matches(_observationClose->_roles.data(), roles)) return false;
    for (const auto& owner : _startupSeries->_owners) {
        const auto transaction = owner.transaction;
        if (!transaction || owner.lost || !matches(transaction->_roles.data(), roles) || !matches(transaction->_store.data(), store)
            || !matches(transaction->_health.data(), health) || (transaction->_executionObservation
                && (transaction->_executionObservation->_startup != transaction || !transaction->_swarm
                    || transaction->_executionObservation->_swarm != transaction->_swarm
                    || transaction->_executionObservation->_taskOrchestrator.data() != transaction->_swarm->executionTaskOrchestratorBackend()))) return false;
    }
    return true;
}

bool FleetMissionDeploymentEpisode::capturedSourcesRetained() const
{
    if (!bundleCurrent() || _startupSeries->_evidenceLost || _startupSeries->_destroying) return false;
    const auto required = [this](quint64 generation, const QList<QObject*>& sources) {
        if (generation == 0) return true;
        for (QObject* source : sources) if (!source || source->thread() != thread()) return false;
        return true;
    };
    const auto own = [](QObject* source, QObject* expected) { return !source || source == expected; };
    const auto member = [this](QObject* source) {
        if (!source) return true;
        for (const auto& owner : _startupSeries->_owners) if (!owner.lost && owner.transaction.data() == source) return true;
        return false;
    };
    // A used component retains these exact sources through terminal cleanup;
    // disconnect/receipt revocation does not turn a lost QPointer into "unused".
    if (!required(_deployment->_generation, {_deployment->_store.data(), _deployment->_intent.data(), _deployment->_review.data(), _deployment->_health.data(),
        _deployment->_swarm.data(), _deployment->_rolePolicy.data(), _deployment->_operatorSession.data(), _deployment->_operationApproval.data(), _deployment->_authorization.data()})
        || !required(_uploadApproval->_generation, {_uploadApproval->_session.data(), _uploadApproval->_roles.data(), _uploadApproval->_store.data(), _uploadApproval->_authorization.data()})
        || !required(_evidenceRefresh->_generation, {_evidenceRefresh->_session.data(), _evidenceRefresh->_roles.data(), _evidenceRefresh->_authorization.data()})
        || !required(_contentCheck->_generation, {_contentCheck->_session.data(), _contentCheck->_roles.data(), _contentCheck->_store.data(), _contentCheck->_deployment.data(), _contentCheck->_swarm.data(), _contentCheck->_authorization.data()})
        || !required(_startReview->_generation, {_startReview->_deployment.data(), _startReview->_store.data(), _startReview->_contentCheck.data(), _startReview->_coordinator.data(),
            _startReview->_health.data(), _startReview->_roles.data(), _startReview->_authorization.data(), _startReview->_catalog.data(), _startReview->_registry.data(), _startReview->_faultPolicy.data()})
        || !required(_startApproval->_generation, {_startApproval->_session.data(), _startApproval->_roles.data(), _startApproval->_store.data(), _startApproval->_review.data(), _startApproval->_deployment.data(), _startApproval->_authorization.data()})
        || !required(_startupAuthority->_generation, {_startupAuthority->_session.data(), _startupAuthority->_roles.data(), _startupAuthority->_store.data(), _startupAuthority->_review.data(),
            _startupAuthority->_approval.data(), _startupAuthority->_content.data(), _startupAuthority->_deployment.data(), _startupAuthority->_swarm.data(), _startupAuthority->_health.data(),
            _startupAuthority->_authorization.data(), _startupAuthority->_coordinator.data(), _startupAuthority->_catalog.data(), _startupAuthority->_registry.data(), _startupAuthority->_faultPolicy.data()})
        || !required(_groundReview->_generation, {_groundReview->_session.data(), _groundReview->_roles.data(), _groundReview->_store.data(), _groundReview->_previous.data(),
            _groundReview->_deployment.data(), _groundReview->_swarm.data(), _groundReview->_health.data(), _groundReview->_authorization.data(), _groundReview->_catalog.data(),
            _groundReview->_coordinator.data(), _groundReview->_registry.data(), _groundReview->_faults.data()})
        || !required(_groundApproval->_generation, {_groundApproval->_session.data(), _groundApproval->_roles.data(), _groundApproval->_store.data(), _groundApproval->_previous.data(),
            _groundApproval->_deployment.data(), _groundApproval->_swarm.data(), _groundApproval->_health.data(), _groundApproval->_authorization.data(), _groundApproval->_review.data(),
            _groundApproval->_catalog.data(), _groundApproval->_coordinator.data(), _groundApproval->_registry.data(), _groundApproval->_faults.data()})
        || !required(_groundAuthority->_generation, {_groundAuthority->_session.data(), _groundAuthority->_roles.data(), _groundAuthority->_store.data(), _groundAuthority->_review.data(),
            _groundAuthority->_approval.data(), _groundAuthority->_previous.data(), _groundAuthority->_deployment.data(), _groundAuthority->_swarm.data(), _groundAuthority->_health.data(),
            _groundAuthority->_authorization.data(), _groundAuthority->_catalog.data(), _groundAuthority->_coordinator.data(), _groundAuthority->_registry.data(), _groundAuthority->_faults.data()})
        || !required(_observationClose->_generation, {_observationClose->_session.data(), _observationClose->_roles.data(), _observationClose->_series.data(), _observationClose->_authorization.data()})) return false;
    if (!own(_deployment->_operatorSession.data(), _session.data()) || !own(_deployment->_operationApproval.data(), _uploadApproval.data())
        || !own(_uploadApproval->_session.data(), _session.data()) || !own(_evidenceRefresh->_session.data(), _session.data())
        || !own(_contentCheck->_session.data(), _session.data()) || !own(_contentCheck->_deployment.data(), _deployment.data())
        || !own(_startReview->_deployment.data(), _deployment.data()) || !own(_startReview->_contentCheck.data(), _contentCheck.data())
        || !own(_startApproval->_session.data(), _session.data()) || !own(_startApproval->_review.data(), _startReview.data()) || !own(_startApproval->_deployment.data(), _deployment.data())
        || !own(_startApproval->_transferredAuthority.data(), _startupAuthority.data()) || !own(_startupAuthority->_session.data(), _session.data())
        || !own(_startupAuthority->_review.data(), _startReview.data()) || !own(_startupAuthority->_approval.data(), _startApproval.data())
        || !own(_startupAuthority->_content.data(), _contentCheck.data()) || !own(_startupAuthority->_deployment.data(), _deployment.data())
        || !own(_groundReview->_session.data(), _session.data()) || !own(_groundReview->_deployment.data(), _deployment.data()) || !member(_groundReview->_previous.data())
        || !own(_groundApproval->_session.data(), _session.data()) || !own(_groundApproval->_deployment.data(), _deployment.data())
        || !own(_groundApproval->_review.data(), _groundReview.data()) || !own(_groundApproval->_transferredAuthority.data(), _groundAuthority.data()) || !member(_groundApproval->_previous.data())
        || !own(_groundAuthority->_session.data(), _session.data()) || !own(_groundAuthority->_review.data(), _groundReview.data())
        || !own(_groundAuthority->_approval.data(), _groundApproval.data()) || !own(_groundAuthority->_deployment.data(), _deployment.data()) || !member(_groundAuthority->_previous.data())
        || !member(_startupAuthority->_transferredTransaction.data()) || !member(_groundAuthority->_pendingTransaction.data()) || !member(_groundAuthority->_transferredTransaction.data())
        || !own(_observationClose->_session.data(), _session.data()) || !own(_observationClose->_series.data(), _startupSeries.data()) || !member(_observationClose->_transaction.data())) return false;
    if ((_startApproval->_state == QStringLiteral("transferred") && !_startApproval->_transferredAuthority)
        || (_startupAuthority->_state == QStringLiteral("transferred") && !_startupAuthority->_transferredTransaction)
        || (_groundApproval->_state == QStringLiteral("transferred") && !_groundApproval->_transferredAuthority)
        || (_groundAuthority->_state == QStringLiteral("transferred") && !_groundAuthority->_transferredTransaction)
        || (_observationClose->_nativeOwnersFrozen && (!_observationClose->_transaction || !_observationClose->_runOwner
            || !_observationClose->_swarmOwner || !_observationClose->_orchestratorOwner))) return false;
    for (const auto& owner : _startupSeries->_owners) {
        const auto transaction = owner.transaction;
        if (!transaction || owner.lost || !transaction->_deploymentEpisodeManaged || transaction->_deploymentEpisodeOwner.data() != this
            || !ownsOutboundObject(transaction.data())
            || !required(transaction->_generation, {transaction->_session.data(), transaction->_roles.data(), transaction->_store.data(),
            transaction->_deployment.data(), transaction->_swarm.data(), transaction->_health.data(), transaction->_authorization.data(), transaction->_coordinator.data(),
            transaction->_catalog.data(), transaction->_registry.data(), transaction->_faultPolicy.data()}) || !own(transaction->_session.data(), _session.data())
            || !own(transaction->_deployment.data(), _deployment.data()) || !own(transaction->_authority.data(), _startupAuthority.data())
            || !own(transaction->_groundAuthority.data(), _groundAuthority.data())) return false;
        if (transaction->_generation > 0 && (transaction->_groundRestart ? !transaction->_groundAuthority : !transaction->_authority)) return false;
    }
    return true;
}

bool FleetMissionDeploymentEpisode::freshForPublication() const
{
    if (!bundleCurrent() || _publishedForOutbound || _supersededForOutbound
        || _deployment->_generation != 0 || !_deployment->_deploymentId.isEmpty() || !_deployment->_state.isEmpty()
        || !_deployment->_targets.isEmpty() || !_deployment->_binding.isEmpty() || !_deployment->_history.isEmpty()
        || _deployment->_beginInProgress || _deployment->_gateInProgress || _deployment->_gateReentered || _deployment->_dispatching || _deployment->_inspectionInProgress
        || _deployment->_reservationHeld || _deployment->_transportDraining || _deployment->_deferredStep != 0
        || _deployment->_reconciliationBusy()
        || _session->_sessionRevision != 0 || _session->_state != QStringLiteral("signed-out") || !uuid(_session->_sessionEpoch)
        || _session->_promptActive || _session->_revisionExhausted || !_session->_lastError.isEmpty()
        || _uploadApproval->_generation != 0 || !_uploadApproval->_approvalId.isEmpty() || _uploadApproval->_state != QStringLiteral("empty")
        || _uploadApproval->_requestInProgress || _uploadApproval->_checking || _uploadApproval->_reentered || !uuid(_uploadApproval->_approvalEpoch)
        || _evidenceRefresh->_generation != 0 || !_evidenceRefresh->_requestId.isEmpty() || _evidenceRefresh->_state != QStringLiteral("empty")
        || _evidenceRefresh->_active || _evidenceRefresh->_busy || _evidenceRefresh->_checking || _evidenceRefresh->_reentered || !_evidenceRefresh->_targets.isEmpty()
        || _contentCheck->_generation != 0 || !_contentCheck->_checkId.isEmpty() || _contentCheck->_state != QStringLiteral("empty")
        || _contentCheck->_active || _contentCheck->_busy || _contentCheck->_checking || _contentCheck->_reentered || _contentCheck->_reservationHeld || _contentCheck->_transportDraining
        || !_contentCheck->_targets.isEmpty() || !uuid(_contentCheck->_checkEpoch)
        || _startReview->_generation != 0 || !_startReview->_reviewId.isEmpty() || !_startReview->_state.isEmpty()
        || _startReview->_busy || _startReview->_healthChecking || _startReview->_reentered || !_startReview->_artifact.isEmpty() || !_startReview->_sourceCapture.isEmpty()
        || !_startReview->_targets.isEmpty() || !uuid(_startReview->_reviewEpoch)
        || _startApproval->_generation != 0 || !_startApproval->_approvalId.isEmpty() || _startApproval->_state != QStringLiteral("empty")
        || _startApproval->_busy || _startApproval->_checking || _startApproval->_reentered || !_startApproval->_targets.isEmpty() || !uuid(_startApproval->_approvalEpoch)
        || _startupAuthority->_generation != 0 || !_startupAuthority->_startupId.isEmpty() || _startupAuthority->_state != QStringLiteral("empty")
        || _startupAuthority->_busy || _startupAuthority->_checking || _startupAuthority->_reentered || _startupAuthority->_reservationHeld || _startupAuthority->_reservationTransferred
        || !_startupAuthority->_targets.isEmpty() || _startupAuthority->_transferredTransaction || !uuid(_startupAuthority->_authorityEpoch)
        || _initialTransaction->_generation != 0 || !_initialTransaction->_startupId.isEmpty() || _initialTransaction->_state != QStringLiteral("empty")
        || _initialTransaction->_active || _initialTransaction->_busy || _initialTransaction->_checking || _initialTransaction->_reentered || _initialTransaction->_sendingIndex != -1
        || _initialTransaction->_reservationHeld || _initialTransaction->_handoffPending || _initialTransaction->_transportDraining
        || _initialTransaction->_anyFlightCommandQueued || _initialTransaction->_executionObservation || !_initialTransaction->_targets.isEmpty()
        || !uuid(_initialTransaction->_transactionEpoch)
        || _groundReview->_generation != 0 || !_groundReview->_reviewId.isEmpty() || _groundReview->_state != QStringLiteral("empty")
        || _groundReview->_active || _groundReview->_busy || _groundReview->_checking || _groundReview->_reentered || _groundReview->_reservationHeld || _groundReview->_transportDraining
        || !_groundReview->_targets.isEmpty() || !_groundReview->_artifact.isEmpty() || !uuid(_groundReview->_reviewEpoch)
        || _groundApproval->_generation != 0 || !_groundApproval->_approvalId.isEmpty() || _groundApproval->_state != QStringLiteral("empty")
        || _groundApproval->_busy || _groundApproval->_checking || _groundApproval->_reentered || _groundApproval->_prompt || _groundApproval->_consumeInProgress
        || !_groundApproval->_targets.isEmpty() || !uuid(_groundApproval->_approvalEpoch)
        || _groundAuthority->_generation != 0 || !_groundAuthority->_startupId.isEmpty() || _groundAuthority->_state != QStringLiteral("empty")
        || _groundAuthority->_active || _groundAuthority->_busy || _groundAuthority->_checking || _groundAuthority->_reentered || _groundAuthority->_transferInProgress
        || _groundAuthority->_reservationHeld || _groundAuthority->_transportDraining || _groundAuthority->_reservationTransferred
        || _groundAuthority->_pendingTransaction || _groundAuthority->_transferredTransaction || !_groundAuthority->_targets.isEmpty()
        || !uuid(_groundAuthority->_authorityEpoch)
        || _startupSeries->_revision != 1 || _startupSeries->_owners.size() != 1 || _startupSeries->_anchorFrozen
        || !_startupSeries->_anchorBinding.isEmpty() || !_startupSeries->_anchorIds.isEmpty() || !_startupSeries->_anchorDeploymentId.isEmpty()
        || _startupSeries->_evidenceLost || _startupSeries->_busy || _startupSeries->_reentered || _startupSeries->_destroying || !_startupSeries->_lastError.isEmpty()
        || _observationClose->_generation != 0 || !_observationClose->_reviewId.isEmpty() || _observationClose->_state != QStringLiteral("empty")
        || _observationClose->_busy || _observationClose->_checking || _observationClose->_prompt || _observationClose->_consuming
        || _observationClose->_authenticationConsumed || _observationClose->_nativeOwnersFrozen || !uuid(_observationClose->_workflowEpoch)) return false;
    const auto& first = _startupSeries->_owners.first();
    if (first.initialized || first.lost || first.authority || !first.startupId.isEmpty() || !first.expectedStartupId.isEmpty()
        || !first.generation.isEmpty() || !first.sourceHash.isEmpty() || !first.basis.isEmpty() || first.epoch != _initialTransaction->_transactionEpoch) return false;
    for (const QVariantMap& proof : {_session->_identityReceipt, _session->_approverReceipt, _session->_approvedApplicantIdentity,
        _session->_uploadIdentityLease, _session->_uploadLeasePrimaryIdentity, _session->_uploadLeaseApproverIdentity,
        _session->_startApprovalAuthentication, _session->_startApprovalPrimaryIdentity, _session->_startApprovalApproverIdentity,
        _session->_groundStartApprovalAuthentication, _session->_groundStartApprovalPrimaryIdentity, _session->_groundStartApprovalApproverIdentity,
        _session->_groundStartupAuthentication, _session->_groundStartupOriginalBasis, _session->_groundStartupPrimaryIdentity, _session->_groundStartupApproverIdentity,
        _session->_groundStartupTransactionAuthentication, _session->_groundStartupTransactionOriginalAuthorityBasis, _session->_groundStartupTransactionOriginalApprovalBasis,
        _session->_groundStartupTransactionPrimaryIdentity, _session->_groundStartupTransactionApproverIdentity, _session->_startupAuthentication,
        _session->_startupPrimaryIdentity, _session->_startupApproverIdentity, _session->_startupTransactionAuthentication,
        _session->_startupTransactionPrimaryIdentity, _session->_startupTransactionApproverIdentity, _session->_startupTransactionOriginalBasis,
        _uploadApproval->_approvalReceipt, _uploadApproval->_grant, _uploadApproval->_identityLease,
        _startApproval->_authenticationReceipt, _startApproval->_approvalReceipt, _startApproval->_grant,
        _startupAuthority->_authenticationReceipt, _startupAuthority->_conditionalGrant, _initialTransaction->_authenticationReceipt,
        _groundApproval->_authentication, _groundApproval->_receipt, _groundApproval->_grant, _groundAuthority->_authentication,
        _groundAuthority->_grant, _groundAuthority->_transferContext, _observationClose->_context, _observationClose->_identity,
        _observationClose->_role, _observationClose->_approver, _observationClose->_disposition}) if (!proof.isEmpty()) return false;
    for (QObject* source : QList<QObject*>{_deployment->_store.data(), _deployment->_intent.data(), _deployment->_review.data(), _deployment->_health.data(),
        _deployment->_swarm.data(), _deployment->_rolePolicy.data(), _deployment->_operatorSession.data(), _deployment->_operationApproval.data(), _deployment->_authorization.data(),
        _uploadApproval->_session.data(), _uploadApproval->_roles.data(), _uploadApproval->_store.data(), _uploadApproval->_authorization.data(),
        _evidenceRefresh->_session.data(), _evidenceRefresh->_roles.data(), _evidenceRefresh->_authorization.data(),
        _contentCheck->_session.data(), _contentCheck->_roles.data(), _contentCheck->_store.data(), _contentCheck->_deployment.data(), _contentCheck->_swarm.data(), _contentCheck->_authorization.data(),
        _startReview->_deployment.data(), _startReview->_store.data(), _startReview->_contentCheck.data(), _startReview->_coordinator.data(), _startReview->_health.data(), _startReview->_roles.data(), _startReview->_authorization.data(),
        _startApproval->_session.data(), _startApproval->_roles.data(), _startApproval->_store.data(), _startApproval->_review.data(), _startApproval->_deployment.data(), _startApproval->_authorization.data(),
        _startReview->_catalog.data(), _startReview->_registry.data(), _startReview->_faultPolicy.data(), _startApproval->_transferredAuthority.data(),
        _startupAuthority->_session.data(), _startupAuthority->_roles.data(), _startupAuthority->_store.data(), _startupAuthority->_review.data(),
        _startupAuthority->_approval.data(), _startupAuthority->_content.data(), _startupAuthority->_deployment.data(), _startupAuthority->_swarm.data(),
        _startupAuthority->_health.data(), _startupAuthority->_authorization.data(), _startupAuthority->_coordinator.data(), _startupAuthority->_catalog.data(),
        _startupAuthority->_registry.data(), _startupAuthority->_faultPolicy.data(),
        _initialTransaction->_session.data(), _initialTransaction->_roles.data(), _initialTransaction->_store.data(), _initialTransaction->_authority.data(),
        _initialTransaction->_groundAuthority.data(), _initialTransaction->_deployment.data(), _initialTransaction->_swarm.data(), _initialTransaction->_health.data(),
        _initialTransaction->_authorization.data(), _initialTransaction->_coordinator.data(), _initialTransaction->_catalog.data(), _initialTransaction->_registry.data(), _initialTransaction->_faultPolicy.data(),
        _groundReview->_session.data(), _groundReview->_roles.data(), _groundReview->_store.data(), _groundReview->_previous.data(), _groundReview->_deployment.data(),
        _groundReview->_swarm.data(), _groundReview->_health.data(), _groundReview->_authorization.data(), _groundReview->_catalog.data(), _groundReview->_coordinator.data(), _groundReview->_registry.data(), _groundReview->_faults.data(),
        _groundApproval->_session.data(), _groundApproval->_roles.data(), _groundApproval->_store.data(), _groundApproval->_previous.data(), _groundApproval->_deployment.data(),
        _groundApproval->_swarm.data(), _groundApproval->_health.data(), _groundApproval->_authorization.data(), _groundApproval->_review.data(), _groundApproval->_catalog.data(), _groundApproval->_coordinator.data(),
        _groundApproval->_registry.data(), _groundApproval->_faults.data(), _groundApproval->_transferredAuthority.data(),
        _groundAuthority->_session.data(), _groundAuthority->_roles.data(), _groundAuthority->_store.data(), _groundAuthority->_review.data(), _groundAuthority->_approval.data(),
        _groundAuthority->_previous.data(), _groundAuthority->_deployment.data(), _groundAuthority->_swarm.data(), _groundAuthority->_health.data(), _groundAuthority->_authorization.data(),
        _groundAuthority->_catalog.data(), _groundAuthority->_coordinator.data(), _groundAuthority->_registry.data(), _groundAuthority->_faults.data(),
        _session->_groundStartApprovalOwner.data(), _session->_groundStartupOwner.data(), _session->_groundStartupTransactionOwner.data(),
        _session->_startupAuthenticationOwner.data(), _session->_startupTransactionAuthenticationOwner.data(),
        _observationClose->_session.data(), _observationClose->_roles.data(), _observationClose->_series.data(), _observationClose->_transaction.data(), _observationClose->_authorization.data()})
        if (source) return false;
    return true;
}

QVariantMap FleetMissionDeploymentEpisode::switchEvidence(SwarmController* actualSwarm) const
{
    const QPointer<FleetMissionDeploymentEpisode> self(const_cast<FleetMissionDeploymentEpisode*>(this));
    const QPointer<SwarmController> swarm(actualSwarm);
    if (!bundleCurrent() || !capturedSourcesRetained() || !swarm || swarm->thread() != thread() || !swarm->executionTaskOrchestratorBackend()
        || swarm->executionTaskOrchestratorBackend()->thread() != thread()) return {};
    const auto sameSwarm = [&](SwarmController* value) { return !value || value == swarm.data(); };
    // Read actual native stack/activity flags, not a caller/current map saying
    // "inactive". Old Run observation/busy assignment is intentionally allowed.
    if (_deployment->_beginInProgress || _deployment->_gateInProgress || _deployment->_dispatching || _deployment->_inspectionInProgress
        || _deployment->_active() || _deployment->_transportDraining || _deployment->_deferredStep != 0
        || _deployment->_reconciliationBusy()
        || _session->_promptActive || _uploadApproval->_requestInProgress || _uploadApproval->_checking
        || _evidenceRefresh->_active || _evidenceRefresh->_busy || _evidenceRefresh->_checking
        || _contentCheck->_active || _contentCheck->_busy || _contentCheck->_checking || _contentCheck->_transportDraining
        || _startReview->_busy || _startReview->_healthChecking || _startReview->_state == QStringLiteral("preparing")
        || _startApproval->_busy || _startApproval->_checking || _startupAuthority->_busy || _startupAuthority->_checking
        || _startupAuthority->_state == QStringLiteral("preparing") || _startupAuthority->_state == QStringLiteral("accepted")
        || _groundReview->_active || _groundReview->_busy || _groundReview->_checking || _groundReview->_transportDraining
        || _groundApproval->_busy || _groundApproval->_checking || _groundApproval->_prompt || _groundApproval->_consumeInProgress
        || _groundAuthority->_active || _groundAuthority->_busy || _groundAuthority->_checking || _groundAuthority->_transportDraining
        || _groundAuthority->_transferInProgress || _groundAuthority->_pendingTransaction
        || _groundAuthority->_state == QStringLiteral("preparing") || _groundAuthority->_state == QStringLiteral("accepted")
        || _observationClose->_busy || _observationClose->_checking || _observationClose->_prompt || _observationClose->_consuming
        || _observationClose->_state == QStringLiteral("committing") || _startupSeries->_busy || _startupSeries->_destroying
        || _startupSeries->_evidenceLost || _startupSeries->_owners.size() > 9
        || !sameSwarm(_deployment->_swarm.data()) || !sameSwarm(_contentCheck->_swarm.data()) || !sameSwarm(_startupAuthority->_swarm.data())
        || !sameSwarm(_groundReview->_swarm.data()) || !sameSwarm(_groundApproval->_swarm.data()) || !sameSwarm(_groundAuthority->_swarm.data())) return {};

    QMap<QString, QVariantMap> reservations;
    const auto reserveState = [&](const QString& ownerId, const QVariantList& ids) {
        if (ownerId.isEmpty()) return ids.isEmpty();
        if (!uuid(ownerId) || ids.size() > 32) return false;
        QVariantMap state;
        if (ids.isEmpty()) {
            // An unused/failed owner may have no captured target list. Scan the
            // real entire map before claiming its occupancy is empty.
            for (auto it = swarm->_missionDeploymentOwners.constBegin(); it != swarm->_missionDeploymentOwners.constEnd(); ++it)
                if (it.value() == ownerId) return false;
            state = {{QStringLiteral("owner"), ownerId}, {QStringLiteral("occupiedVehicleIds"), QVariantList()},
                {QStringLiteral("authorityRevoked"), swarm->_revokedMissionDeploymentOwners.contains(ownerId)}};
        } else state = swarm->_missionDeploymentReservationState(ownerId, ids);
        if (state.size() != 3 || (reservations.contains(ownerId) && reservations.value(ownerId) != state)) return false;
        reservations.insert(ownerId, state); return reservations.size() <= 32;
    };
    const auto nativeIds = [](const auto& targets) {
        QVariantList ids;
        QSet<int> seen;
        for (const auto& target : targets) {
            if (target.vehicleId < 1 || target.vehicleId > 255 || seen.contains(target.vehicleId)) return QVariantList();
            seen.insert(target.vehicleId); ids.append(target.vehicleId);
        }
        return ids;
    };
    const auto observeTargets = [&](const auto& targets, QVariantList* rows) {
        if (!rows || targets.size() > 32) return false;
        QSet<int> seen;
        for (const auto& target : targets) {
            if (seen.contains(target.vehicleId)) return false;
            const QVariantMap row = targetEvidence(target.vehicleId, target.vehicle.data(), target.mission.data(), target.parameters.data(),
                target.links.data(), target.primary.data(), true, true, true, thread());
            if (!self || !swarm || row.isEmpty()) return false;
            seen.insert(target.vehicleId); rows->append(row);
        }
        return true;
    };
    QVariantList deploymentTargets, evidenceTargets, contentTargets, reviewTargets, approvalTargets, authorityTargets;
    QVariantList groundReviewTargets, groundApprovalTargets, groundAuthorityTargets;
    if (_deployment->_targets.size() > 32 || _startReview->_targets.size() > 32
        || nativeIds(_deployment->_targets).size() != _deployment->_targets.size()
        || nativeIds(_startReview->_targets).size() != _startReview->_targets.size()) return {};
    for (const auto& target : _deployment->_targets) {
        const QVariantMap row = targetEvidence(target.vehicleId, target.vehicle.data(), target.manager.data(), nullptr,
            nullptr, target.transportPrimary.data(), false, false, target.transportPrimaryCaptured, thread());
        if (!self || !swarm || row.isEmpty()) return {};
        deploymentTargets.append(row);
    }
    for (const auto& target : _startReview->_targets) {
        const QVariantMap row = targetEvidence(target.vehicleId, target.vehicle.data(), target.manager.data(), target.parameters.data(),
            nullptr, target.primary.data(), true, false, true, thread());
        if (!self || !swarm || row.isEmpty()) return {};
        reviewTargets.append(row);
    }
    if (!observeTargets(_evidenceRefresh->_targets, &evidenceTargets) || !observeTargets(_contentCheck->_targets, &contentTargets)
        || !observeTargets(_startApproval->_targets, &approvalTargets) || !observeTargets(_startupAuthority->_targets, &authorityTargets)
        || !observeTargets(_groundReview->_targets, &groundReviewTargets) || !observeTargets(_groundApproval->_targets, &groundApprovalTargets)
        || !observeTargets(_groundAuthority->_targets, &groundAuthorityTargets)) return {};
    if (!reserveState(_deployment->_deploymentId, nativeIds(_deployment->_targets))
        || !reserveState(_contentCheck->_checkId, nativeIds(_contentCheck->_targets))
        || !reserveState(_startupAuthority->_startupId, nativeIds(_startupAuthority->_targets))
        || !reserveState(_groundReview->_reviewId, nativeIds(_groundReview->_targets))
        || !reserveState(_groundAuthority->_startupId, nativeIds(_groundAuthority->_targets))) return {};

    QVariantList transactions, runs;
    for (int index = 0; index < _startupSeries->_owners.size(); ++index) {
        const auto& owner = _startupSeries->_owners.at(index);
        const QPointer<FleetMissionStartupTransaction> transaction(owner.transaction);
        if (!transaction || owner.lost || transaction->thread() != thread() || transaction->_active || transaction->_busy || transaction->_checking
            || transaction->_sendingIndex >= 0 || transaction->_handoffPending || transaction->_transportDraining || !uuid(transaction->_transactionEpoch)
            || (index == 0 ? transaction != _initialTransaction : transaction->parent() != _startupSeries.data())
            || (owner.initialized ? !terminalTransaction(transaction->_state) : transaction->_state != QStringLiteral("empty"))
            || !sameSwarm(transaction->_swarm.data())) return {};
        QVariantList transactionTargets;
        if (!observeTargets(transaction->_targets, &transactionTargets)) return {};
        if (owner.initialized && (owner.startupId != transaction->_startupId || owner.epoch != transaction->_transactionEpoch
            || owner.generation != QString::number(transaction->_generation) || owner.sourceHash != transaction->_sourceHash)) return {};
        if (!owner.initialized && (!transaction->_startupId.isEmpty() || transaction->_generation != 0)) return {};
        const QVariantList ids = nativeIds(transaction->_targets);
        if (ids.size() != transaction->_targets.size() || !reserveState(transaction->_startupId, ids)) return {};
        if (owner.initialized && (transaction->_deployment != _deployment || !transaction->_session || !transaction->_roles || !transaction->_store
            || !transaction->_health || !transaction->_authorization || !transaction->_swarm || !transaction->_coordinator || !transaction->_catalog
            || !transaction->_registry || !transaction->_faultPolicy || transaction->_session != _session
            || transaction->_binding != _deployment->_binding)) return {};
        transactions.append(QVariantMap{{QStringLiteral("transactionEpoch"), transaction->_transactionEpoch}, {QStringLiteral("startupId"), transaction->_startupId},
            {QStringLiteral("generation"), QString::number(transaction->_generation)}, {QStringLiteral("state"), transaction->_state},
            {QStringLiteral("sourceHash"), transaction->_sourceHash}, {QStringLiteral("groundRestart"), transaction->_groundRestart},
            {QStringLiteral("targetVehicleIds"), ids}, {QStringLiteral("binding"), transaction->_binding},
            {QStringLiteral("targetEvidence"), transactionTargets}});
        const QPointer<FleetMissionExecutionObservation> run(transaction->_executionObservation);
        if (!run) {
            for (const auto& target : transaction->_targets) if (target.queued[2] || target.uncertain[2]) return {};
            continue;
        }
        if (run->thread() != thread() || run->parent() != transaction.data() || run->_polling || run->_ownerDestroying || !run->_prepared
            || run->_startup != transaction || !run->_startupRetired || run->_startupId != transaction->_startupId
            || run->_startupEpoch != transaction->_transactionEpoch || run->_startupGeneration != transaction->_generation
            || run->_binding != transaction->_binding || run->_swarm != swarm || !run->_taskOrchestrator
            || run->_taskOrchestrator.data() != swarm->executionTaskOrchestratorBackend() || run->_targets.size() != transaction->_targets.size()
            || run->_vehicleIds != ids || !uuid(run->_runId) || !reserveState(run->_runId, ids)) return {};
        bool unknown = false;
        for (int targetIndex = 0; targetIndex < run->_targets.size(); ++targetIndex) {
            const auto& target = run->_targets.at(targetIndex);
            const auto& original = transaction->_targets.at(targetIndex);
            if (target.vehicleId != original.vehicleId || target.vehicle != original.vehicle || target.mission != original.mission
                || target.links != original.links || target.primary != original.primary) return {};
            unknown = unknown || target.continuityUnknown || !target.sourceActive || !target.tracking || !target.vehicle || !target.mission
                || !target.links || !target.primary || transactionTargets.at(targetIndex).toMap().value(QStringLiteral("sourceContinuityUnknown")).toBool();
        }
        runs.append(QVariantMap{{QStringLiteral("runId"), run->_runId}, {QStringLiteral("startupId"), run->_startupId},
            {QStringLiteral("transactionEpoch"), run->_startupEpoch}, {QStringLiteral("transactionGeneration"), QString::number(run->_startupGeneration)},
            {QStringLiteral("deploymentId"), run->_deploymentId}, {QStringLiteral("executionObservationTaskId"), run->_taskModelId},
            {QStringLiteral("binding"), run->_binding}, {QStringLiteral("targetVehicleIds"), run->_vehicleIds},
            {QStringLiteral("retired"), run->_retired}, {QStringLiteral("modeQueueCommitted"), run->hasCommittedMode()}, {QStringLiteral("observationSourceUnknown"), unknown},
            {QStringLiteral("missionResultVerified"), false}});
    }
    QVariantList componentStates{
        component(QStringLiteral("deployment"), _deployment->_state, _deployment->_generation, deploymentTargets), component(QStringLiteral("session"), _session->_state, _session->_sessionRevision),
        component(QStringLiteral("uploadApproval"), _uploadApproval->_state, _uploadApproval->_generation), component(QStringLiteral("evidenceRefresh"), _evidenceRefresh->_state, _evidenceRefresh->_generation, evidenceTargets),
        component(QStringLiteral("contentCheck"), _contentCheck->_state, _contentCheck->_generation, contentTargets), component(QStringLiteral("startReview"), _startReview->_state, _startReview->_generation, reviewTargets),
        component(QStringLiteral("startApproval"), _startApproval->_state, _startApproval->_generation, approvalTargets), component(QStringLiteral("startupAuthority"), _startupAuthority->_state, _startupAuthority->_generation, authorityTargets),
        // Initial/child transaction target evidence appears only in transactions.
        component(QStringLiteral("initialTransaction"), _initialTransaction->_state, _initialTransaction->_generation), component(QStringLiteral("groundReview"), _groundReview->_state, _groundReview->_generation, groundReviewTargets),
        component(QStringLiteral("groundApproval"), _groundApproval->_state, _groundApproval->_generation, groundApprovalTargets), component(QStringLiteral("groundAuthority"), _groundAuthority->_state, _groundAuthority->_generation, groundAuthorityTargets),
        component(QStringLiteral("startupSeries"), QStringLiteral("retained"), _startupSeries->_revision), component(QStringLiteral("observationClose"), _observationClose->_state, _observationClose->_generation)};
    QVariantList reservationRows;
    for (auto it = reservations.constBegin(); it != reservations.constEnd(); ++it) reservationRows.append(it.value());
    const QVariantMap evidence{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("episodeId"), _episodeId},
        {QStringLiteral("scope"), QStringLiteral("native-quiescent-outbound-not-flight-or-run-retirement")},
        {QStringLiteral("deploymentId"), _deployment->_deploymentId}, {QStringLiteral("deploymentGeneration"), QString::number(_deployment->_generation)},
        {QStringLiteral("deploymentState"), _deployment->_state.isEmpty() ? QStringLiteral("empty") : _deployment->_state}, {QStringLiteral("binding"), _deployment->_binding},
        {QStringLiteral("sessionState"), _session->_state}, {QStringLiteral("sessionRevision"), QString::number(_session->_sessionRevision)},
        {QStringLiteral("componentStates"), componentStates}, {QStringLiteral("seriesRevision"), QString::number(_startupSeries->_revision)},
        {QStringLiteral("transactions"), transactions}, {QStringLiteral("reservations"), reservationRows}, {QStringLiteral("runs"), runs},
        {QStringLiteral("outboundQuiescent"), true}, {QStringLiteral("flightStateVerified"), false},
        {QStringLiteral("missionResultVerified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (!self || !swarm || !bundleCurrent() || QJsonDocument::fromVariant(evidence).toJson(QJsonDocument::Compact).size() > 512 * 1024) return {};
    return evidence;
}

QVariantMap FleetMissionDeploymentEpisode::retirementEvidence(SwarmController* actualSwarm,
    SwarmMissionOrchestrator* actualOrchestrator, bool published) const
{
    const QPointer<FleetMissionDeploymentEpisode> self(const_cast<FleetMissionDeploymentEpisode*>(this));
    const QPointer<SwarmController> swarm(actualSwarm);
    const QPointer<SwarmMissionOrchestrator> orchestrator(actualOrchestrator);
    if (!swarm || !orchestrator || QThread::currentThread() != thread()
        || swarm->thread() != thread() || orchestrator->thread() != thread()
        || swarm->executionTaskOrchestratorBackend() != orchestrator.data()
        || (published ? !_publishedForOutbound || !_supersededForOutbound : _publishedForOutbound || !freshForPublication())) return {};
    const QVariantMap quiet = switchEvidence(swarm.data());
    if (!self || !swarm || !orchestrator || quiet.size() != 18) return {};
    QSet<QString> ownerIds;
    for (const QVariant& value : quiet.value(QStringLiteral("reservations")).toList()) {
        if (value.type() != QVariant::Map || value.toMap().size() != 3
            || value.toMap().value(QStringLiteral("occupiedVehicleIds")).type() != QVariant::List
            || !value.toMap().value(QStringLiteral("occupiedVehicleIds")).toList().isEmpty()) return {};
        ownerIds.insert(value.toMap().value(QStringLiteral("owner")).toString());
    }
    // The original helper can project a target subset. Retirement must not
    // overlook an entry for one of these actual owners outside that subset.
    for (auto it = swarm->_missionDeploymentOwners.constBegin(); it != swarm->_missionDeploymentOwners.constEnd(); ++it)
        if (ownerIds.contains(it.value())) return {};
    const quint64 seriesRevision = _startupSeries->_revision;
    const auto owners = _startupSeries->_owners;
    QVariantList records;
    for (const auto& owner : owners) {
        const QPointer<FleetMissionStartupTransaction> transaction(owner.transaction);
        if (!transaction || owner.lost || transaction->_active || transaction->_busy || transaction->_checking
            || transaction->_sendingIndex >= 0 || transaction->_transportDraining || transaction->_handoffPending) return {};
        const QPointer<FleetMissionExecutionObservation> run(transaction->_executionObservation);
        if (!run) {
            for (const auto& target : transaction->_targets)
                if (target.queued[2] || target.uncertain[2]) return {};
            continue;
        }
        if (!run->_prepared || !run->_retired || run->_polling || run->_liveProbeBusy || run->_ownerDestroying
            || run->_startup != transaction || run->_swarm != swarm || run->_taskOrchestrator != orchestrator) return {};
        bool unissued = run->_unissuedPreparationClosed;
        if (unissued) {
            for (const auto& target : run->_targets) if (target.queueBegan || target.queueCommitted) unissued = false;
            for (const auto& target : transaction->_targets) if (target.queued[2] || target.uncertain[2]) unissued = false;
        }
        if (!transaction->_independentObservationCloseCommitted && !unissued) return {};
        if (run->_taskModelId.isEmpty()) {
            if (!unissued) return {};
            continue;
        }
        const QVariantMap record = orchestrator->captureClosedExecutionRecord(run.data(), run->_taskModelId);
        if (!self || !swarm || !orchestrator || !transaction || !run || record.size() != 16) return {};
        records.append(record);
    }
    if (records.size() > 9 || !self || !swarm || !orchestrator || !_startupSeries
        || _startupSeries->_revision != seriesRevision || _startupSeries->_owners.size() != owners.size()
        || switchEvidence(swarm.data()) != quiet) return {};
    if (!self || !swarm || !orchestrator || !_startupSeries
        || swarm->executionTaskOrchestratorBackend() != orchestrator.data()) return {};
    for (int i = 0; i < owners.size(); ++i)
        if (_startupSeries->_owners.at(i).transaction != owners.at(i).transaction
            || _startupSeries->_owners.at(i).lost != owners.at(i).lost) return {};
    // The last target-source probe may reenter. Check the actual whole owner
    // table again after it, not merely the helper's original target subset.
    for (auto it = swarm->_missionDeploymentOwners.constBegin(); it != swarm->_missionDeploymentOwners.constEnd(); ++it)
        if (ownerIds.contains(it.value())) return {};
    const QString kind = !published ? QStringLiteral("unpublished-pristine")
        : quiet.value(QStringLiteral("runs")).toList().isEmpty() ? QStringLiteral("unused-native") : QStringLiteral("closed-observations");
    return {{QStringLiteral("quietEvidence"), quiet}, {QStringLiteral("records"), records}, {QStringLiteral("retirementKind"), kind}};
}

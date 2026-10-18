#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"

#include "FleetHealthCenter.h"
#include "FleetIntentTask.h"
#include "FleetMissionDraft.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionReadbackDiff.h"
#include "FleetMissionReconciliation.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionRiskEvaluator.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionProgressObservation.h"
#include "FleetRolePolicy.h"
#include "FleetOperatorSession.h"
#include "FleetOperationApproval.h"
#include "FirmwarePlugin.h"
#include "LinkInterface.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <functional>
#include <limits>

namespace {
bool outboundSources(const QObject* owner, const FleetOperatorSession* session, const FleetOperationApproval* approval)
{
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(owner)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, session)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(owner, approval);
}
struct BeginGuard {
    QPointer<QObject> object;
    bool* flag;
    ~BeginGuard() { if (object) *flag = false; }
};

bool strictTrue(const QVariant& value)
{
    return value.type() == QVariant::Bool && value.toBool();
}

constexpr qint64 PlanningInputMaximumBytes = 16LL * 1024 * 1024;
bool planningInputTree(const QVariant& value, int depth, int* nodes, qint64* amount)
{
    if (depth > 20 || ++(*nodes) > 524288) return false;
    if (!value.isValid() || (value.userType() == QMetaType::Nullptr && value.isNull())) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 1024) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 1024 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *amount += static_cast<qint64>(it.key().size()) * 2;
            if (*amount > PlanningInputMaximumBytes || !planningInputTree(it.value(), depth + 1, nodes, amount)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > 32768) return false;
        for (const QVariant& entry : list) if (!planningInputTree(entry, depth + 1, nodes, amount)) return false;
        return true;
    }
    case QVariant::StringList: {
        const QStringList list = value.toStringList();
        if (list.size() > 32768) return false;
        for (const QString& entry : list) if (!planningInputTree(entry, depth + 1, nodes, amount)) return false;
        return true;
    }
    case QVariant::String:
        *amount += static_cast<qint64>(value.toString().size()) * 2;
        return value.toString().size() <= 32768 && *amount <= PlanningInputMaximumBytes
            && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong:
        return value.toLongLong() >= -9007199254740991LL && value.toLongLong() <= 9007199254740991LL;
    case QVariant::ULongLong: return value.toULongLong() <= 9007199254740991ULL;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false; // In particular, even null QObject/opaque pointers are not JSON planning input.
    }
}

bool planningInputFingerprint(const QVariantMap& input, QByteArray* fingerprint)
{
    int nodes = 0; qint64 amount = 0;
    if (input.isEmpty() || !planningInputTree(input, 0, &nodes, &amount)) return false;
    const QByteArray bytes = QJsonDocument::fromVariant(input).toJson(QJsonDocument::Compact);
    if (bytes.isEmpty() || bytes.size() > PlanningInputMaximumBytes) return false;
    *fingerprint = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
    return true; // Local input identity only, never authenticity or approval.
}

bool originalPlanningTask(const QVariantMap& parameters, QVariantMap* task, QByteArray* fingerprint)
{
    // Validate the WHOLE original before dropping runtime snapshots: no
    // opaque source or oversized prefix is silently accepted as complete.
    if (!planningInputFingerprint(parameters, fingerprint)) return false;
    QVariantMap candidate = parameters;
    for (const QString& key : {QStringLiteral("fleet"), QStringLiteral("healthReports"),
        QStringLiteral("nativeVehicleEvidence"), QStringLiteral("capabilityReviewEvidence"),
        QStringLiteral("templateValidation")}) candidate.remove(key);
    for (const QString& key : {QStringLiteral("reviewOptions"), QStringLiteral("options")}) {
        if (!candidate.contains(key)) continue;
        if (candidate.value(key).type() != QVariant::Map || candidate.value(key).isNull()) return false;
        QVariantMap options = candidate.value(key).toMap();
        options.remove(QStringLiteral("events"));
        options.remove(QStringLiteral("healthSnapshotMismatch"));
        candidate.insert(key, options);
    }
    QByteArray ignored;
    if (!planningInputFingerprint(candidate, &ignored)) return false;
    *task = candidate;
    return true;
}

bool missionParameter4(const QVariantMap& item, double* parameter)
{
    const int command = item.value(QStringLiteral("command")).toInt();
    const QVariant value = item.value(QStringLiteral("param4"));
    QVariant canonical;
    QString error;
    if (!item.contains(QStringLiteral("param4"))
        || !FleetMissionHeadingEncoding::canonicalParam4(command, value,
            item.value(QStringLiteral("yawMode")), false, canonical, error)) return false;
    // Upload the exact already-quantized request, never a readback-derived
    // value or a second quantization. Only declared default null becomes NaN.
    *parameter = canonical.isValid() ? value.toDouble() : std::numeric_limits<double>::quiet_NaN();
    return true;
}

bool boundedBinding(const QVariantMap& input, QVariantMap* output)
{
    const QSet<QString> textKeys = {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"),
        QStringLiteral("storeEpoch")};
    const QSet<QString> numberKeys = {QStringLiteral("recordRevision"),
        QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")};
    if (input.size() < 7 || input.size() > 9
        || input.contains(QStringLiteral("reviewGeneration")) != input.contains(QStringLiteral("inputGeneration"))) return false;
    QVariantMap staged;
    for (auto it = input.constBegin(); it != input.constEnd(); ++it) {
        if (textKeys.contains(it.key())) {
            if (it.value().type() != QVariant::String || it.value().toString().isEmpty()
                || it.value().toString().size() > 128) return false;
        } else if (numberKeys.contains(it.key())) {
            const QVariant::Type type = it.value().type();
            if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
                && type != QVariant::ULongLong && type != QVariant::Double) return false;
            const double number = it.value().toDouble();
            if (!std::isfinite(number) || std::floor(number) != number || number < 0
                || number > 9007199254740991.0
                || (it.key() == QStringLiteral("recordRevision") && number == 0)) return false;
        } else return false; // Never retain caller-provided nested metadata.
        staged.insert(it.key(), it.value());
    }
    for (const QString& key : textKeys) if (!staged.contains(key)) return false;
    if (!staged.contains(QStringLiteral("recordRevision"))) return false;
    *output = staged;
    return true;
}

Vehicle* liveVehicle(int id)
{
    if (!qgcApp() || !qgcApp()->toolbox() || !qgcApp()->toolbox()->multiVehicleManager()) return nullptr;
    QmlObjectListModel* model = qgcApp()->toolbox()->multiVehicleManager()->vehicles();
    Vehicle* result = nullptr;
    for (int i = 0; model && i < model->count(); ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (vehicle && vehicle->id() == id) {
            if (result) return nullptr; // Ambiguous identity is not a replacement policy.
            result = vehicle;
        }
    }
    return result;
}

QVariantMap rejection(const QString& code)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("errorCode"), code},
        {QStringLiteral("executionVerified"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

QString boundedDigest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return QString();
    const QString digest = value.toString();
    for (QChar character : digest)
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return QString();
    return digest;
}

QVariantMap decodedReadbackItem(const MissionItem* item)
{
    const int command = static_cast<int>(item->command());
    const double parameter4 = item->param4();
    QVariant decoded;
    QString yawMode = QStringLiteral("unsupported-command"), error;
    if (std::isnan(parameter4) || (std::isfinite(parameter4) && double(float(parameter4)) == parameter4)) {
        if (!FleetMissionHeadingEncoding::decodeParam4(command, float(parameter4), decoded, yawMode, error))
            yawMode = QStringLiteral("unsupported-command");
    }
    return {{QStringLiteral("sequenceNumber"), item->sequenceNumber()},
        {QStringLiteral("command"), command}, {QStringLiteral("frame"), static_cast<int>(item->frame())},
        {QStringLiteral("param1"), item->param1()}, {QStringLiteral("param2"), item->param2()},
        {QStringLiteral("param3"), item->param3()},
        {QStringLiteral("param4"), std::isnan(parameter4) ? QVariant() : QVariant(parameter4)},
        {QStringLiteral("param5"), item->param5()}, {QStringLiteral("param6"), item->param6()},
        {QStringLiteral("param7"), item->param7()}, {QStringLiteral("autoContinue"), item->autoContinue()},
        {QStringLiteral("isCurrentItem"), item->isCurrentItem()},
        {QStringLiteral("yawMode"), yawMode}};
}

bool sameDecodedReadbackItem(const QVariantMap& first, const QVariantMap& second)
{
    if (first.size() != second.size()) return false;
    for (auto it = first.constBegin(); it != first.constEnd(); ++it) {
        const QVariant value = second.value(it.key());
        if (it.value().type() != value.type()) return false;
        if (value.type() == QVariant::Double) {
            const double a = it.value().toDouble(), b = value.toDouble();
            if (a != b && !(std::isnan(a) && std::isnan(b))) return false;
        } else if (it.value() != value) return false;
    }
    return true; // Stable invalid numbers remain diagnostic, never valid wire items.
}
}

FleetMissionDeployment::FleetMissionDeployment(QObject* parent) : QObject(parent)
{
    _reconciliation = new FleetMissionReconciliation(this);
    connect(_reconciliation.data(), &FleetMissionReconciliation::changed, this, &FleetMissionDeployment::reconciliationChanged);
    _clock.start();
    _timer.setInterval(500);
    connect(&_timer, &QTimer::timeout, this, &FleetMissionDeployment::_tick);
    connect(this, &FleetMissionDeployment::changed, this, &FleetMissionDeployment::inspectionChanged);
}

FleetMissionDeployment::~FleetMissionDeployment()
{
    // Cancel/drain the private child while this owner's members still exist.
    delete _reconciliation.data();
    _disconnectAll();
    // Do not release a reservation while a transport can still be writing.
    // SwarmController can outlive this object; retained occupancy then fails
    // closed until the application/controller is destroyed, not an auto-clear.
    bool pending = _dispatching;
    for (const Target& target : _targets) pending = pending
        || (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent())
        || (target.manager && (target.manager->inProgress() || target.manager->transportCallInProgress()));
    if (_reservationHeld && !pending && _swarm) _swarm->releaseMissionDeployment(_deploymentId);
}

bool FleetMissionDeployment::_active() const
{
    return _state == QStringLiteral("preparing") || _state == QStringLiteral("uploading")
        || _state == QStringLiteral("readback");
}

QVariantMap FleetMissionDeployment::reconciliation() const
{
    return _reconciliation ? _reconciliation->current() : QVariantMap();
}
bool FleetMissionDeployment::_reconciliationBusy() const
{
    return !_reconciliation || _reconciliation->parent() != this || _reconciliation->busy();
}
QVariantMap FleetMissionDeployment::requestReconciliation(QObject* sessionObject, QObject* roleObject,
    QObject* storeObject, QObject* swarmObject, QObject* authorizationOwner,
    const QString& expectedDeploymentId, bool operatorConfirmed)
{
    if (!_reconciliation || _reconciliation->parent() != this) return rejection(QStringLiteral("native-reconciliation-owner-unavailable"));
    return _reconciliation->request(sessionObject, roleObject, storeObject, swarmObject, authorizationOwner, expectedDeploymentId, operatorConfirmed);
}
bool FleetMissionDeployment::cancelReconciliation(const QString& requestId)
{
    return _reconciliation && _reconciliation->parent() == this && _reconciliation->cancel(requestId);
}
QVariantMap FleetMissionDeployment::repairDraftSource(const QString& expectedDeploymentId,
    const QString& expectedReconciliationRequestId) const
{
    const auto deny = [](const QString& error) -> QVariantMap {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error},
            {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
    };
    // No live-vehicle lookup, account/QML callback or new observation occurs.
    // Existing local stacks must nevertheless finish before another editing
    // workflow may consume this failed deployment's frozen source values.
    if (thread() != QThread::currentThread()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || expectedDeploymentId.isEmpty() || expectedDeploymentId.size() > 64
        || expectedDeploymentId != _deploymentId || !_generation
        || (_state != QStringLiteral("failed") && _state != QStringLiteral("cancelled"))
        || _active() || _reservationHeld || _transportDraining || _dispatching
        || _beginInProgress || _gateInProgress || _inspectionInProgress || _deferredStep != 0
        || _stackBusy() || _reconciliationBusy())
        return deny(QStringLiteral("current-drained-failed-or-cancelled-deployment-required"));
    for (const Target& target : _targets) {
        if (target.manager && target.manager->inProgress())
            return deny(QStringLiteral("original-mission-transport-still-active"));
    }
    const QVariantMap observed = _reconciliation->current();
    const QString terminal = observed.value(QStringLiteral("state")).toString();
    const QVariant pending = observed.value(QStringLiteral("pendingVehicleCount"));
    const QVariant total = observed.value(QStringLiteral("totalVehicleCount"));
    if (expectedReconciliationRequestId.isEmpty() || expectedReconciliationRequestId.size() > 64
        || observed.size() != 30
        || observed.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || observed.value(QStringLiteral("schemaVersion")).toInt() != 1
        || observed.value(QStringLiteral("requestId")) != expectedReconciliationRequestId
        || observed.value(QStringLiteral("deploymentId")) != _deploymentId
        || observed.value(QStringLiteral("deploymentGeneration")) != QString::number(_generation)
        || observed.value(QStringLiteral("binding")) != _binding
        || observed.value(QStringLiteral("vehicleIds")) != _vehicleIds
        || total.type() != QVariant::Int || total.toInt() < 1 || total.toInt() > 32
        || total.toInt() != _targets.size()
        || pending.type() != QVariant::Int || pending.toInt() != 0
        || (terminal != QStringLiteral("completed") && terminal != QStringLiteral("stopped")
            && terminal != QStringLiteral("cancelled")))
        return deny(QStringLiteral("same-deployment-terminal-reconciliation-required"));
    for (const QString& key : {QStringLiteral("active"), QStringLiteral("reservationHeld"),
        QStringLiteral("transportDraining"), QStringLiteral("currentOnboardContentsProven"),
        QStringLiteral("startupPermitGranted"), QStringLiteral("executionVerified"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("executable")}) {
        const QVariant value = observed.value(key);
        if (value.type() != QVariant::Bool || value.toBool())
            return deny(QStringLiteral("reconciliation-not-drained-or-not-historical"));
    }
    if (!strictTrue(observed.value(QStringLiteral("historicalOnly")))
        || !strictTrue(observed.value(QStringLiteral("snapshotOnly"))))
        return deny(QStringLiteral("reconciliation-historical-boundary-required"));
    QVariantMap binding;
    if (!boundedBinding(_binding, &binding)) return deny(QStringLiteral("original-deployment-binding-unavailable"));
    const QVariant planValue = _recordSnapshot.value(QStringLiteral("plan"));
    if (planValue.type() != QVariant::Map || planValue.isNull())
        return deny(QStringLiteral("original-archived-plan-unavailable"));
    const QVariantMap plan = planValue.toMap();
    if (!plan.contains(QStringLiteral("planningSource")))
        return deny(QStringLiteral("original-planning-source-not-recorded-cannot-restore-draft"));
    const QVariant sourceValue = plan.value(QStringLiteral("planningSource"));
    if (sourceValue.type() != QVariant::Map || sourceValue.isNull())
        return deny(QStringLiteral("original-planning-source-invalid"));
    // This pure-data validator preserves retained editor fields, including
    // inactive routes/endpoints/profiles. It does not infer polygon geometry
    // from generated mission items or borrow the current mutable Intent.
    const QVariantMap validated = FleetMissionDraft::validatePlanningSource(sourceValue.toMap());
    if (validated.size() != 4 || !strictTrue(validated.value(QStringLiteral("ok")))
        || validated.value(QStringLiteral("source")).type() != QVariant::Map
        || validated.value(QStringLiteral("source")).isNull()
        || validated.value(QStringLiteral("source")).toMap().isEmpty()
        || validated.value(QStringLiteral("task")).type() != QVariant::Map
        || validated.value(QStringLiteral("task")).isNull()
        || validated.value(QStringLiteral("task")).toMap().isEmpty()
        || validated.value(QStringLiteral("error")).type() != QVariant::String
        || !validated.value(QStringLiteral("error")).toString().isEmpty())
        return deny(QStringLiteral("original-planning-source-invalid-or-not-editable"));
    // Unknown/different observations remain in the copied diagnostic summary;
    // producing editable intent never means that the aircraft was repaired.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("deploymentId"), _deploymentId}, {QStringLiteral("deploymentGeneration"), QString::number(_generation)},
        {QStringLiteral("binding"), binding}, {QStringLiteral("reconciliation"), observed},
        {QStringLiteral("planningSource"), validated.value(QStringLiteral("source"))},
        {QStringLiteral("manualConfirmationRequired"), true},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}
bool FleetMissionDeployment::_reconciliationSourcesMatch(FleetOperatorSession* session, FleetRolePolicy* roles,
    FleetMissionPlanStore* store, SwarmController* swarm, QObject* authorization)
{
    const QPointer<FleetMissionDeployment> self(this);
    const QPointer<QObject> owner(authorization);
    if (!owner || owner != _authorization || session != _operatorSession || roles != _rolePolicy || store != _store || swarm != _swarm) return false;
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(owner.data(), "fleetReconciliationSourcesMatch", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(this))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(store))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm))));
    return self && owner && owner == _authorization && invoked && strictTrue(result);
}
bool FleetMissionDeployment::_reconciliationContextAllowed(QObject* authorization, const QString& requestId,
    const QVariantMap& binding, const QVariantList& vehicleIds)
{
    const QPointer<FleetMissionDeployment> self(this);
    const QPointer<QObject> owner(authorization);
    if (!owner || owner != _authorization) return false;
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(owner.data(), "fleetReconciliationContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(this))),
        Q_ARG(QVariant, QVariant(requestId)), Q_ARG(QVariant, QVariant(binding)), Q_ARG(QVariant, QVariant(vehicleIds)));
    return self && owner && owner == _authorization && invoked && strictTrue(result);
}

bool FleetMissionDeployment::_same(quint64 generation) const
{
    return generation != 0 && generation == _generation;
}

bool FleetMissionDeployment::nativeTransportContextCurrent(PlanManager* actualManager, Vehicle* actualVehicle,
    LinkInterface* actualPrimary, const QString& deploymentId, quint64 generation,
    int transactionType, quint64 operationId) const
{
    // Pure native admission only: no Session/WTS, QML, full gate, virtual
    // firmware method, or temporary primary-link strong reference here.
    // PlanManager separately checks its frozen primary control block.
    if (thread() != QThread::currentThread() || !_same(generation)
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())
        || deploymentId.isEmpty() || deploymentId != _deploymentId
        || !_reservationHeld || _transportDraining || !_swarm || _swarm->thread() != thread()
        || _nextTarget < 0 || _nextTarget >= _targets.size()
        || (transactionType != PlanManager::TransactionWrite && transactionType != PlanManager::TransactionRead)
        || _state != (transactionType == PlanManager::TransactionWrite ? QStringLiteral("uploading") : QStringLiteral("readback"))) return false;
    const Target& target = _targets.at(_nextTarget);
    if (!actualManager || !actualVehicle || !actualPrimary || !target.vehicle || !target.manager
        || !target.transportPrimaryCaptured || !target.transportPrimary
        || target.vehicle.data() != actualVehicle || static_cast<PlanManager*>(target.manager.data()) != actualManager
        || target.transportPrimary.data() != actualPrimary || target.awaitingType != transactionType || target.operationError
        || actualVehicle->thread() != thread() || actualManager->thread() != thread()
        || actualVehicle->id() != target.vehicleId || actualVehicle->missionManager() != target.manager.data()
        || !_swarm->missionDeploymentOwns(deploymentId, _vehicleIds)) return false;
    const quint64 actualOperation = actualManager->activeOperationId();
    if (operationId == 0) return _dispatching && target.operationId == 0 && actualOperation == 0;
    return target.operationId == operationId && actualOperation == operationId
        && actualManager->inProgress() && target.deadlineMs > 0 && _clock.elapsed() < target.deadlineMs;
}

bool FleetMissionDeployment::_stackBusy() const
{
    if (_dispatching) return true;
    for (const Target& target : _targets) {
        // The original Vehicle can outlive a destroyed MissionManager while
        // its scoped sender is still inside plugin/fence/physical queue work.
        // This is local stack evidence only, never remote-idle certification.
        if (target.vehicle && !target.vehicle->fleetStartupTransportQuiescent()) return true;
        if (target.manager && target.manager->transportCallInProgress()) return true;
    }
    return false;
}

bool FleetMissionDeployment::_publish(quint64 generation)
{
    QPointer<FleetMissionDeployment> guard(this);
    emit changed();
    return guard && _same(generation);
}

QVariantMap FleetMissionDeployment::current() const
{
    if (_deploymentId.isEmpty()) return QVariantMap();
    const QVariantMap nativeEvidence = _reviewSnapshot.value(QStringLiteral("nativeVehicleEvidence")).toMap();
    const QVariantMap encoding = _reviewSnapshot.value(QStringLiteral("transportEncoding")).toMap();
    const QVariantMap sourceSummary{
        {QStringLiteral("nativeCaptureRequested"), strictTrue(nativeEvidence.value(QStringLiteral("captureRequested")))},
        {QStringLiteral("nativeCaptureComplete"), strictTrue(nativeEvidence.value(QStringLiteral("captureComplete")))},
        {QStringLiteral("parameterEvidenceUsable"), strictTrue(nativeEvidence.value(QStringLiteral("allParameterEvidenceUsable")))},
        {QStringLiteral("firmwareObservationUsable"), strictTrue(nativeEvidence.value(QStringLiteral("allFirmwareObservationsUsable")))},
        {QStringLiteral("adapterDeclarationMatched"), strictTrue(nativeEvidence.value(QStringLiteral("allAdapterDeclarationsMatched")))},
        {QStringLiteral("evidenceFingerprint"), boundedDigest(nativeEvidence.value(QStringLiteral("evidenceFingerprint")))},
        {QStringLiteral("encodingPreviewOk"), strictTrue(encoding.value(QStringLiteral("ok")))},
        {QStringLiteral("encodingPreviewScope"), QStringLiteral("source-encoding-preview")},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    QVariantList vehicles;
    bool possibleWrite = false;
    for (const Target& target : _targets) {
        possibleWrite = possibleWrite || target.writeMayHaveCommitted;
        vehicles.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
            {QStringLiteral("state"), target.state},
            {QStringLiteral("operationId"), QString::number(target.operationId)},
            {QStringLiteral("writeOperationId"), QString::number(target.writeOperationId)},
            {QStringLiteral("readOperationId"), QString::number(target.readOperationId)},
            {QStringLiteral("uploadAcknowledged"), target.uploadAcknowledged},
            {QStringLiteral("readbackVerified"), target.readbackVerified},
            {QStringLiteral("readbackDiagnostic"), target.readbackDiagnostic},
            {QStringLiteral("contentHash"), target.payload.value(QStringLiteral("contentHash"))},
            {QStringLiteral("wireSchema"), target.payload.value(QStringLiteral("wireSchema"))},
            // Keep per-item semantic/source manifests in the one current
            // payload only. Terminal history must remain a small summary.
            {QStringLiteral("encodedSemanticCount"), target.payload.value(QStringLiteral("encodedSemantics")).toList().size()},
            {QStringLiteral("unencodedRequirements"), target.payload.value(QStringLiteral("unencodedRequirements"))},
            {QStringLiteral("relativeDwellEncoded"), target.payload.value(QStringLiteral("relativeDwellEncoded"))},
            {QStringLiteral("endActionEncoded"), target.payload.value(QStringLiteral("endActionEncoded"))},
            {QStringLiteral("itemCount"), target.payload.value(QStringLiteral("items")).toList().size()},
            {QStringLiteral("writeMayHaveCommitted"), target.writeMayHaveCommitted},
            {QStringLiteral("error"), target.error}});
    }
    return {{QStringLiteral("deploymentId"), _deploymentId}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("state"), _state},
        {QStringLiteral("reason"), _reason}, {QStringLiteral("createdAtUtc"), _createdAt},
        {QStringLiteral("binding"), _binding}, {QStringLiteral("vehicles"), vehicles},
        {QStringLiteral("reviewSourceSummary"), sourceSummary},
        {QStringLiteral("uploadActorReceipt"), _uploadRoleReceipt},
        {QStringLiteral("payload"), _payloadBuild}, {QStringLiteral("transportDraining"), _transportDraining},
        {QStringLiteral("reservationHeld"), _reservationHeld},
        {QStringLiteral("writeMayHaveCommitted"), possibleWrite},
        {QStringLiteral("verificationScope"), QStringLiteral("fresh-onboard-mission-items-readback")},
        {QStringLiteral("executionVerified"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("rollbackPerformed"), false}, {QStringLiteral("startBlockedReasons"),
            _payloadBuild.value(QStringLiteral("blockingLimitations"))}};
}

QVariantMap FleetMissionDeployment::_inspectionSnapshot(const QVariantMap& captured) const
{
    if (captured.isEmpty()) return {};
    const qint64 age = _inspectionCapturedMs < 0 ? -1 : _clock.elapsed() - _inspectionCapturedMs;
    // This is a short-lived display capture, not a retained live approval.
    // Even inside this bound a dispatcher would have to capture again.
    if (_inspectionGeneration != _generation || _inspectionState != _state || age < 0 || age > 1000) {
        QVariantMap expired = rejection(QStringLiteral("inspection-expired-or-context-changed"));
        expired.insert(QStringLiteral("deploymentId"), captured.value(QStringLiteral("deploymentId")));
        expired.insert(QStringLiteral("snapshotOnly"), true);
        expired.insert(QStringLiteral("snapshotExpired"), true);
        expired.insert(QStringLiteral("startupPermitGranted"), false);
        return expired;
    }
    QVariantMap view = captured;
    view.insert(QStringLiteral("snapshotAgeMs"), age);
    view.insert(QStringLiteral("snapshotOnly"), true);
    view.insert(QStringLiteral("snapshotExpired"), false);
    return view;
}

QVariantMap FleetMissionDeployment::startReadiness() const
{
    return _inspectionSnapshot(_startReadinessSnapshot);
}

QVariantMap FleetMissionDeployment::progressObservation() const
{
    return _inspectionSnapshot(_progressObservationSnapshot);
}

QVariantMap FleetMissionDeployment::planningTaskForRun(const QVariantMap& binding) const
{
    // Retained historical input only. Neither current health nor the latest
    // intent/store revision can supply or replace a missing original input.
    if (QThread::currentThread() != thread() || _generation == 0 || _deploymentId.isEmpty()
        || binding != _binding || _planningTask.isEmpty() || _planningParametersFingerprint.size() != 64
        || !_planningTaskUnavailableReason.isEmpty()) return {};
    return _planningTask;
}

QVariantMap FleetMissionDeployment::startReviewContext(const QString& deploymentId) const
{
    if (thread() != QThread::currentThread() || deploymentId.isEmpty() || deploymentId.size() > 64
        || deploymentId != _deploymentId || _generation == 0 || _active() || _transportDraining
        || _beginInProgress || _gateInProgress || _inspectionInProgress || _stackBusy() || _reconciliationBusy()
        || _targets.isEmpty() || _targets.size() > 32)
        return rejection(QStringLiteral("new-start-review-context-unavailable"));
    QVariantList receipts;
    int totalItems = 0;
    for (const Target& target : _targets) {
        const QVariantMap receipt = target.historicalReadbackReceipt;
        if (!target.vehicle || !target.manager || liveVehicle(target.vehicleId) != target.vehicle.data()
            || target.vehicle->missionManager() != target.manager.data()
            || target.vehicle->thread() != thread() || target.manager->thread() != thread()
            || target.manager->inProgress() || target.manager->transportCallInProgress()
            || !strictTrue(receipt.value(QStringLiteral("matched")))
            || receipt.value(QStringLiteral("contentHash")) != target.payload.value(QStringLiteral("contentHash"))
            || receipt.value(QStringLiteral("wireSchema")) != target.payload.value(QStringLiteral("wireSchema"))
            || receipt.value(QStringLiteral("writeOperationId")) != QString::number(target.writeOperationId)
            || receipt.value(QStringLiteral("readOperationId")) != QString::number(target.readOperationId)
            || !target.writeOperationId || !target.readOperationId)
            return rejection(QStringLiteral("whole-runtime-historical-readback-required"));
        totalItems += target.payload.value(QStringLiteral("items")).toList().size();
        if (totalItems > 8192) return rejection(QStringLiteral("start-review-item-budget-exceeded"));
        receipts.append(receipt);
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("deploymentId"), deploymentId},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("binding"), _binding},
        {QStringLiteral("plan"), _recordSnapshot.value(QStringLiteral("plan"))},
        {QStringLiteral("payloadBuild"), _payloadBuild}, {QStringLiteral("historicalReadbacks"), receipts},
        {QStringLiteral("stagingState"), _state}, {QStringLiteral("historicalReadbackOnly"), true},
        {QStringLiteral("currentOnboardContentsProven"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetMissionDeployment::prepareStartContentCheck(const QString& requestedDeploymentId)
{
    const QString deploymentId = requestedDeploymentId;
    if (thread() != QThread::currentThread() || !outboundSources(this, _operatorSession.data(), _operationApproval.data())
        || deploymentId != _deploymentId || _active()
        || _reservationHeld || _transportDraining || _stackBusy() || _beginInProgress || _gateInProgress || _inspectionInProgress || _reconciliationBusy()) return false;
    const QPointer<FleetMissionDeployment> self(this);
    const quint64 generation = _generation;
    const QVariantMap historical = startReviewContext(deploymentId);
    if (!self || !outboundSources(this, _operatorSession.data(), _operationApproval.data())
        || !strictTrue(historical.value(QStringLiteral("ok")))) return false;
    // The fresh reader must freeze its context AFTER this revocation. Later
    // own READ notifications may not restore the old verified/live approval.
    if (_state == QStringLiteral("verified")) _fail(generation, QStringLiteral("separate-start-content-read-revokes-live-staging"));
    return self && outboundSources(this, _operatorSession.data(), _operationApproval.data()) && _same(generation) && _deploymentId == deploymentId && !_active()
        && !_reservationHeld && !_transportDraining && !_stackBusy()
        && strictTrue(startReviewContext(deploymentId).value(QStringLiteral("ok")));
}

Vehicle* FleetMissionDeployment::startReviewVehicle(const QString& deploymentId, int vehicleId) const
{
    if (thread() != QThread::currentThread() || deploymentId != _deploymentId || vehicleId < 1 || vehicleId > 255) return nullptr;
    Vehicle* result = nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) {
        if (result || !target.vehicle || !target.manager || liveVehicle(vehicleId) != target.vehicle.data()
            || target.vehicle->missionManager() != target.manager.data()) return nullptr;
        result = target.vehicle.data();
    }
    return result;
}

MissionManager* FleetMissionDeployment::startReviewManager(const QString& deploymentId, int vehicleId) const
{
    Vehicle* vehicle = startReviewVehicle(deploymentId, vehicleId);
    if (!vehicle) return nullptr;
    for (const Target& target : _targets) if (target.vehicleId == vehicleId) return target.manager.data();
    return nullptr;
}

QVariantMap FleetMissionDeployment::inspectExecutionState(const QString& deploymentId)
{
    if (thread() != QThread::currentThread() || _inspectionInProgress || _beginInProgress || _gateInProgress || _reconciliationBusy()
        || _active() || _transportDraining || _stackBusy() || deploymentId.isEmpty() || deploymentId.size() > 64
        || deploymentId != _deploymentId || _targets.isEmpty() || _targets.size() > 32)
        return rejection(QStringLiteral("inspection-context-busy-or-mismatched"));
    _inspectionInProgress = true;
    BeginGuard inspecting{QPointer<QObject>(this), &_inspectionInProgress};
    QPointer<FleetMissionDeployment> self(this);
    const quint64 generation = _generation;
    const QString state = _state;
    const QList<Target> targets = _targets;
    const QVariantMap binding = _binding;
    const QVariantMap plan = _recordSnapshot.value(QStringLiteral("plan")).toMap();
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.size() != targets.size() || members.size() > 32
        || plan.value(QStringLiteral("constraints")).type() != QVariant::Map)
        return rejection(QStringLiteral("inspection-frozen-plan-required"));
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    QPointer<FleetMissionReviewCoordinator> review(_review);
    QPointer<FleetFirmwareAdapterCatalog> catalog(review ? review->firmwareAdapterCatalogBackend() : nullptr);
    QString stagingReason;
    bool stagingCurrent = state == QStringLiteral("verified") && _gate(generation, false, &stagingReason);
    if (!self) return rejection(QStringLiteral("inspection-owner-destroyed"));
    auto contextCurrent = [&]() {
        if (!self || !_same(generation) || _state != state || _binding != binding || _targets.size() != targets.size()
            || _active() || _transportDraining || _stackBusy() || _review != review
            || (review && review->firmwareAdapterCatalogBackend() != catalog.data())) return false;
        for (int i = 0; i < targets.size(); ++i) {
            if (targets[i].vehicle != _targets[i].vehicle || targets[i].manager != _targets[i].manager
                || !targets[i].vehicle || !targets[i].manager || liveVehicle(targets[i].vehicleId) != targets[i].vehicle.data()
                || targets[i].vehicle->missionManager() != targets[i].manager.data()) return false;
        }
        return true;
    };
    if (!contextCurrent()) return rejection(QStringLiteral("inspection-native-context-changed"));
    QVariantList readinessRows;
    QVariantList progressRows;
    bool allEvidence = true;
    bool allPreconditions = true;
    bool allProgressStable = true;
    for (const Target& target : targets) {
        QVariantMap member;
        for (const QVariant& value : members) {
            const QVariantMap row = value.toMap();
            if (row.value(QStringLiteral("vehicleId")).toInt() == target.vehicleId) {
                if (!member.isEmpty()) return rejection(QStringLiteral("inspection-duplicate-plan-member"));
                member = row;
            }
        }
        if (member.isEmpty()) return rejection(QStringLiteral("inspection-plan-member-not-found"));
        const QVariantMap readiness = FleetMissionStartReadiness::capture(target.vehicle.data(), member, constraints,
            target.payload, catalog.data());
        const QVariantMap progress = FleetMissionProgressObservation::capture(target.vehicle.data(), target.manager.data(), target.payload);
        if (!contextCurrent()) return rejection(QStringLiteral("inspection-native-context-changed"));
        readinessRows.append(readiness);
        progressRows.append(progress);
        allEvidence = allEvidence && strictTrue(readiness.value(QStringLiteral("sourceEvidenceComplete")));
        allPreconditions = allPreconditions && strictTrue(readiness.value(QStringLiteral("preconditionsMatch")));
        allProgressStable = allProgressStable && strictTrue(progress.value(QStringLiteral("sourceCurrent")));
    }
    // A changing staging approval is not renewed by this read-only query.
    // Progress may still be observed after staging was revoked by ARM or an
    // external operation; that never attributes execution to this plan.
    if (stagingCurrent) stagingCurrent = _gate(generation, false, &stagingReason);
    if (!contextCurrent()) return rejection(QStringLiteral("inspection-superseded-at-final-capture"));
    const QString capturedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QVariantMap common{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true},
        {QStringLiteral("deploymentId"), deploymentId}, {QStringLiteral("binding"), binding},
        {QStringLiteral("capturedAtUtc"), capturedAt}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
    QVariantMap readiness = common;
    readiness.insert(QStringLiteral("stagingSourceCurrentAtCapture"), stagingCurrent);
    readiness.insert(QStringLiteral("stagingBlockReason"), stagingCurrent ? QString()
        : stagingReason.isEmpty() ? QStringLiteral("no-current-ground-staging-approval") : stagingReason);
    readiness.insert(QStringLiteral("allSourceEvidenceComplete"), allEvidence);
    readiness.insert(QStringLiteral("allPreconditionsMatch"), stagingCurrent && allPreconditions);
    readiness.insert(QStringLiteral("vehicles"), readinessRows);
    QVariantMap progress = common;
    progress.insert(QStringLiteral("captureStable"), allProgressStable);
    progress.insert(QStringLiteral("executionAttributionVerified"), false);
    progress.insert(QStringLiteral("missionCompleted"), false);
    progress.insert(QStringLiteral("vehicles"), progressRows);
    _startReadinessSnapshot = readiness;
    _progressObservationSnapshot = progress;
    _inspectionCapturedMs = _clock.elapsed();
    _inspectionGeneration = generation;
    _inspectionState = state;
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("startReadiness"), readiness},
        {QStringLiteral("progressObservation"), progress}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    emit inspectionChanged();
    if (!contextCurrent()) return rejection(QStringLiteral("inspection-context-changed-during-publication"));
    return result;
}

bool FleetMissionDeployment::_vehicleReady(const Target& target, bool requireIdle, QString* reason) const
{
    const QPointer<FleetMissionDeployment> self(const_cast<FleetMissionDeployment*>(this));
    const QPointer<Vehicle> vehicle(target.vehicle);
    const QPointer<MissionManager> manager(target.manager);
    if (!vehicle || !manager || liveVehicle(target.vehicleId) != vehicle.data()
        || vehicle->missionManager() != manager.data()) {
        *reason = QStringLiteral("vehicle-instance-replaced-or-destroyed"); return false;
    }
    const QPointer<VehicleLinkManager> link(vehicle->vehicleLinkManager());
    if (vehicle->isOfflineEditingVehicle() || !vehicle->isInitialConnectComplete()
        || !link || link->communicationLost() || link->primaryLink().expired()) {
        *reason = QStringLiteral("vehicle-transport-unavailable"); return false;
    }
    const QPointer<FirmwarePlugin> plugin(vehicle->firmwarePlugin());
    if (!vehicle->px4Firmware() || !vehicle->multiRotor() || !vehicle->capabilitiesKnown()
        || !(vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_INT) || !plugin) {
        *reason = QStringLiteral("unsupported-px4-ground-upload-adapter"); return false;
    }
    const bool sendsHome = plugin->sendHomePositionToVehicle();
    if (!self || !vehicle || !manager || !link || !plugin || sendsHome || vehicle->firmwarePlugin() != plugin.data()
        || liveVehicle(target.vehicleId) != vehicle.data() || vehicle->missionManager() != manager.data()) {
        *reason = QStringLiteral("unsupported-px4-ground-upload-adapter"); return false;
    }
    if (vehicle->armed() || vehicle->flying()) {
        *reason = QStringLiteral("grounded-disarmed-required"); return false;
    }
    const QGeoCoordinate home = vehicle->homePosition();
    if (!home.isValid() || !std::isfinite(home.altitude())) {
        *reason = QStringLiteral("finite-home-amsl-required"); return false;
    }
    if (requireIdle && (manager->inProgress() || manager->transportCallInProgress())) {
        *reason = QStringLiteral("mission-manager-busy"); return false;
    }
    return true;
}

bool FleetMissionDeployment::_gate(quint64 generation, bool requireReservation, QString* reason)
{
    QPointer<FleetMissionDeployment> guard(this);
    if (!outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        *reason = QStringLiteral("current-native-deployment-episode-required"); return false;
    }
    if (_gateInProgress) {
        _gateReentered = true;
        *reason = QStringLiteral("deployment-gate-reentered"); return false;
    }
    _gateInProgress = true;
    _gateReentered = false;
    BeginGuard checking{QPointer<QObject>(this), &_gateInProgress};
    if (!_same(generation) || !_store || !_intent || !_review || !_health || !_swarm || !_rolePolicy
        || !_operatorSession || !_operationApproval || !_authorization) {
        *reason = QStringLiteral("deployment-context-unavailable"); return false;
    }
    const QPointer<FleetMissionPlanStore> expectedStore(_store);
    const QPointer<FleetIntentTask> expectedIntent(_intent);
    const QPointer<FleetMissionReviewCoordinator> expectedReview(_review);
    const QPointer<FleetHealthCenter> expectedHealth(_health);
    const QPointer<SwarmController> expectedSwarm(_swarm);
    const QPointer<FleetRolePolicy> expectedPolicy(_rolePolicy);
    const QVariantMap expectedRoleReceipt(_uploadRoleReceipt);
    const QPointer<FleetOperatorSession> expectedSession(_operatorSession);
    const QPointer<FleetOperationApproval> expectedApproval(_operationApproval);
    const QVariantMap expectedIdentity(_uploadIdentityReceipt);
    const QVariantMap expectedGrant(_uploadApprovalGrant);
    const QPointer<QObject> expectedAuthorization(_authorization);
    const QVariantMap expectedBinding = _binding;
    const QVariantMap expectedRecord = _recordSnapshot;
    const QVariantMap expectedSnapshot = _reviewSnapshot;
    const QVariantList expectedIds = _vehicleIds;
    const QString expectedDeployment = _deploymentId;
    const auto contextUnchanged = [&]() {
        return guard && outboundSources(this, expectedSession.data(), expectedApproval.data()) && !_gateReentered && _same(generation)
            && (_active() || _state == QStringLiteral("verified"))
            && expectedStore && expectedIntent && expectedReview && expectedHealth && expectedSwarm
            && expectedPolicy && expectedSession && expectedApproval && expectedAuthorization
            && _store == expectedStore && _intent == expectedIntent && _review == expectedReview
            && _health == expectedHealth && _swarm == expectedSwarm && _authorization == expectedAuthorization
            && _rolePolicy == expectedPolicy && _uploadRoleReceipt == expectedRoleReceipt
            && _operatorSession == expectedSession && _operationApproval == expectedApproval
            && _uploadIdentityReceipt == expectedIdentity && _uploadApprovalGrant == expectedGrant
            && expectedPolicy->actionSnapshotCurrent(expectedRoleReceipt)
            && _binding == expectedBinding && _recordSnapshot == expectedRecord && _reviewSnapshot == expectedSnapshot
            && _vehicleIds == expectedIds && _deploymentId == expectedDeployment
            && expectedStore->matchesBinding(expectedBinding) && expectedIntent->state() == QStringLiteral("approved")
            && expectedIntent->taskId() == expectedBinding.value(QStringLiteral("sourceTaskId")).toString()
            && expectedReview->lastReview() == expectedSnapshot
            && expectedStore->record(expectedIntent->taskId(), expectedBinding.value(QStringLiteral("recordRevision"))) == expectedRecord;
    };
    if (_rolePolicy->thread() != thread() || !_rolePolicy->actionSnapshotCurrent(_uploadRoleReceipt)
        || _uploadRoleReceipt.value(QStringLiteral("action")).toString() != QStringLiteral("mission.upload")) {
        *reason = QStringLiteral("local-upload-actor-or-additional-approval-invalid"); return false;
    }
    // A local role flag is never an alternative to actual authentication and
    // independently issued approval. Both native sources remain frozen.
    if (expectedSession->thread() != thread() || expectedApproval->thread() != thread()
        || !expectedSession->uploadIdentityLeaseCurrent(expectedGrant.value(QStringLiteral("identityLease")).toMap())
        || !contextUnchanged() || !expectedApproval->grantCurrent(expectedGrant, expectedDeployment)
        || !contextUnchanged()) {
        *reason = QStringLiteral("authenticated-upload-session-or-approval-revoked"); return false;
    }
    QVariant ownerPolicyMatches;
    const bool ownerPolicyInvoked = QMetaObject::invokeMethod(_authorization.data(), "fleetDeploymentAuthorizationSourceMatches",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, ownerPolicyMatches),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(guard.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(_rolePolicy.data()))));
    if (!contextUnchanged() || !ownerPolicyInvoked || !strictTrue(ownerPolicyMatches)) {
        *reason = QStringLiteral("actual-owner-role-policy-source-mismatch"); return false;
    }
    if (!_store->matchesBinding(_binding) || _intent->state() != QStringLiteral("approved")
        || _intent->taskId() != _binding.value(QStringLiteral("sourceTaskId")).toString()
        || _store->record(_intent->taskId(), _binding.value(QStringLiteral("recordRevision"))) != _recordSnapshot
        || _review->lastReview() != _reviewSnapshot
        || _reviewSnapshot.value(QStringLiteral("plan")).toMap() != _recordSnapshot.value(QStringLiteral("plan")).toMap()
        || _reviewSnapshot.value(QStringLiteral("needsReview")).type() != QVariant::Bool
        || _reviewSnapshot.value(QStringLiteral("needsReview")).toBool()
        || _reviewSnapshot.value(QStringLiteral("reviewStatus")).toString() != QStringLiteral("reviewable")) {
        *reason = QStringLiteral("approved-review-version-invalidated"); return false;
    }
    const QVariantMap evidence = _recordSnapshot.value(QStringLiteral("reviewEvidence")).toMap();
    if (evidence.value(QStringLiteral("needsReview")) != _reviewSnapshot.value(QStringLiteral("needsReview"))
        || evidence.value(QStringLiteral("reviewStatus")) != _reviewSnapshot.value(QStringLiteral("reviewStatus"))) {
        *reason = QStringLiteral("archived-review-evidence-mismatch"); return false;
    }
    // A display/report's inputValid describes its syntax, not the absence
    // of business blockers. Check the actual frozen assessment and its exact
    // archived counterpart before staging or continuing a transfer. Compare
    // canonical JSON so a native QStringList and its JSON List round-trip
    // preserve the same content without accepting bad types as an empty list.
    const QVariant reviewedRisk = expectedSnapshot.value(QStringLiteral("riskAssessment"));
    const QVariant archivedRisk = evidence.value(QStringLiteral("riskAssessment"));
    if (reviewedRisk.type() != QVariant::Map || reviewedRisk.isNull()
        || archivedRisk.type() != QVariant::Map || archivedRisk.isNull()
        || !FleetMissionRiskEvaluator::hasNoBlockingRisks(reviewedRisk.toMap())
        || !FleetMissionRiskEvaluator::hasNoBlockingRisks(archivedRisk.toMap())
        || QJsonDocument::fromVariant(reviewedRisk).toJson(QJsonDocument::Compact)
            != QJsonDocument::fromVariant(archivedRisk).toJson(QJsonDocument::Compact)) {
        *reason = QStringLiteral("archived-review-risk-blocked-invalid-or-mismatched"); return false;
    }
    // The archived capability/baseline observation must be the same one as
    // the actual review. An archive is still historical evidence, not a live
    // Registry source or a substitute for the controller's final live fence.
    for (const QString& field : {QStringLiteral("nativeVehicleEvidence"), QStringLiteral("transportEncoding"),
            QStringLiteral("capabilityReviewEvidence")}) {
        if (evidence.value(field).type() != QVariant::Map || expectedSnapshot.value(field).type() != QVariant::Map
            || evidence.value(field) != expectedSnapshot.value(field)) {
            *reason = QStringLiteral("archived-review-evidence-missing-or-mismatched"); return false;
        }
    }
    const QVariantMap nativeEvidence = expectedSnapshot.value(QStringLiteral("nativeVehicleEvidence")).toMap();
    const QVariantMap encoding = expectedSnapshot.value(QStringLiteral("transportEncoding")).toMap();
    if (!strictTrue(nativeEvidence.value(QStringLiteral("captureRequested")))
        || !strictTrue(nativeEvidence.value(QStringLiteral("captureComplete")))
        || boundedDigest(nativeEvidence.value(QStringLiteral("evidenceFingerprint"))).isEmpty()
        || !strictTrue(encoding.value(QStringLiteral("ok")))) {
        *reason = QStringLiteral("native-review-and-encoding-reprepare-required"); return false;
    }
    const QVariantMap gate = _health->evaluateTaskGate(_vehicleIds, false);
    if (!strictTrue(gate.value(QStringLiteral("allowed")))) {
        *reason = QStringLiteral("live-task-health-blocked"); return false;
    }
    // This is a second, live native-to-controller check, not a caller's
    // supplied boolean. Missing method/non-boolean return is an explicit deny.
    QVariant permitted;
    const QVariant argument(_binding);
    const bool invoked = QMetaObject::invokeMethod(_authorization.data(), "fleetDeploymentBindingAllowed",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, permitted),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(guard.data()))), Q_ARG(QVariant, argument));
    if (!guard) return false;
    if (!invoked || !strictTrue(permitted) || !contextUnchanged()) {
        *reason = QStringLiteral("controller-review-permission-revoked"); return false;
    }
    if (requireReservation && (!_reservationHeld || !_swarm->missionDeploymentOwns(_deploymentId, _vehicleIds))) {
        *reason = QStringLiteral("deployment-reservation-revoked"); return false;
    }
    if (!strictTrue(_health->evaluateTaskGate(_vehicleIds, false).value(QStringLiteral("allowed")))) {
        *reason = QStringLiteral("live-task-health-blocked"); return false;
    }
    for (const Target& target : _targets) {
        if (!_vehicleReady(target, false, reason) || !contextUnchanged()) return false;
        if (target.manager->inProgress() && (target.operationId == 0
            || target.manager->activeOperationId() != target.operationId)) {
            *reason = QStringLiteral("foreign-or-unidentified-mission-transfer"); return false;
        }
    }
    // Last live source edge: not a supplied options flag, archived boolean,
    // parameter-profile hash, or claim of firmware/flight certification.
    if (!contextUnchanged()) {
        *reason = QStringLiteral("native-final-gate-context-unavailable"); return false;
    }
    const bool nativeCurrent = expectedReview->nativeEvidenceCurrent();
    if (!guard) return false;
    if (!nativeCurrent || !contextUnchanged()) {
        *reason = QStringLiteral("native-observation-binding-stale-or-superseded"); return false;
    }
    if (requireReservation && (!_reservationHeld || !expectedSwarm->missionDeploymentOwns(expectedDeployment, expectedIds))) {
        *reason = QStringLiteral("deployment-reservation-revoked"); return false;
    }
    if (!strictTrue(expectedHealth->evaluateTaskGate(expectedIds, false).value(QStringLiteral("allowed")))) {
        *reason = QStringLiteral("live-task-health-blocked"); return false;
    }
    for (const Target& target : _targets) {
        if (!_vehicleReady(target, false, reason) || !contextUnchanged()) return false;
        if (target.manager->inProgress() && (target.operationId == 0 || target.manager->activeOperationId() != target.operationId)) {
            *reason = QStringLiteral("foreign-or-unidentified-mission-transfer"); return false;
        }
    }
    if (!contextUnchanged()) {
        *reason = QStringLiteral("native-final-gate-superseded"); return false;
    }
    if (!expectedSession->uploadIdentityLeaseCurrent(expectedGrant.value(QStringLiteral("identityLease")).toMap()) || !contextUnchanged()
        || !expectedApproval->grantCurrent(expectedGrant, expectedDeployment) || !contextUnchanged()) {
        *reason = QStringLiteral("upload-approval-revoked-at-final-gate"); return false;
    }
    return true;
}

QVariantMap FleetMissionDeployment::beginUpload(QObject* storeObject, QObject* intentObject,
    QObject* reviewObject, QObject* healthObject, QObject* swarmObject, QObject* rolePolicyObject,
    QObject* sessionObject, QObject* approvalObject, QObject* authorizationOwner,
    const QVariantMap& binding, const QVariantMap& authorizationReceipt,
    const QVariantMap& identityReceipt, const QVariantMap& approvalReceipt, bool operatorConfirmed)
{
    if (thread() != QThread::currentThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _beginInProgress || _gateInProgress || _inspectionInProgress || _reconciliationBusy()
        || _active() || _reservationHeld || _transportDraining)
        return rejection(QStringLiteral("deployment-busy-or-draining"));
    if (!operatorConfirmed) return rejection(QStringLiteral("explicit-overwrite-confirmation-required"));
    QVariantMap canonicalBinding;
    if (!boundedBinding(binding, &canonicalBinding)) return rejection(QStringLiteral("invalid-or-unbounded-deployment-binding"));
    if (_generation == std::numeric_limits<quint64>::max()) return rejection(QStringLiteral("generation-exhausted"));
    _beginInProgress = true;
    BeginGuard beginning{QPointer<QObject>(this), &_beginInProgress};
    QPointer<FleetMissionDeployment> self(this);
    QPointer<FleetMissionPlanStore> store = qobject_cast<FleetMissionPlanStore*>(storeObject);
    QPointer<FleetIntentTask> intent = qobject_cast<FleetIntentTask*>(intentObject);
    QPointer<FleetMissionReviewCoordinator> review = qobject_cast<FleetMissionReviewCoordinator*>(reviewObject);
    QPointer<FleetHealthCenter> health = qobject_cast<FleetHealthCenter*>(healthObject);
    QPointer<SwarmController> swarm = qobject_cast<SwarmController*>(swarmObject);
    QPointer<FleetRolePolicy> role = qobject_cast<FleetRolePolicy*>(rolePolicyObject);
    QPointer<FleetOperatorSession> session = qobject_cast<FleetOperatorSession*>(sessionObject);
    QPointer<FleetOperationApproval> approval = qobject_cast<FleetOperationApproval*>(approvalObject);
    QPointer<QObject> owner(authorizationOwner);
    const auto sourcesAlive = [&]() { return self && store && intent && review && health && swarm && role && session && approval && owner
        && outboundSources(this, session.data(), approval.data()); };
    if (!sourcesAlive()
        || role->thread() != thread() || owner->thread() != thread()
        || session->thread() != thread() || approval->thread() != thread())
        return rejection(QStringLiteral("native-context-required"));
    if (!role->actionSnapshotCurrent(authorizationReceipt)
        || authorizationReceipt.value(QStringLiteral("action")).toString() != QStringLiteral("mission.upload"))
        return rejection(QStringLiteral("current-local-upload-actor-required"));
    if (!session->identitySnapshotCurrent(identityReceipt) || !sourcesAlive()
        || !role->actionSnapshotCurrent(authorizationReceipt))
        return rejection(QStringLiteral("authenticated-upload-session-required"));
    if (!store->matchesBinding(canonicalBinding) || intent->state() != QStringLiteral("approved")
        || intent->taskId() != canonicalBinding.value(QStringLiteral("sourceTaskId")).toString())
        return rejection(QStringLiteral("approved-live-binding-required"));
    const QVariantMap record = store->record(intent->taskId(), canonicalBinding.value(QStringLiteral("recordRevision")));
    const QVariantMap reviewSnapshot = review->lastReview();
    if (record.value(QStringLiteral("plan")).toMap().isEmpty()
        || record.value(QStringLiteral("plan")).toMap() != reviewSnapshot.value(QStringLiteral("plan")).toMap())
        return rejection(QStringLiteral("review-plan-mismatch"));
    const QVariantMap originalParameters = intent->parameters();
    QVariantMap planningTask;
    QByteArray planningParametersFingerprint;
    const bool planningInputAvailable = originalPlanningTask(originalParameters, &planningTask, &planningParametersFingerprint);
    const QVariantMap build = FleetMissionPayloadBuilder::build(record.value(QStringLiteral("plan")).toMap());
    if (!strictTrue(build.value(QStringLiteral("ok")))) {
        QVariantMap denied = build;
        denied.insert(QStringLiteral("executionVerified"), false);
        return denied;
    }
    QList<Target> targets;
    QVariantList ids;
    QSet<int> seen;
    const QVariantList payloads = build.value(QStringLiteral("payloads")).toList();
    if (payloads.isEmpty() || payloads.size() > 32) return rejection(QStringLiteral("invalid-payload-target-count"));
    for (const QVariant& value : payloads) {
        const QVariantMap payload = value.toMap();
        const int id = payload.value(QStringLiteral("vehicleId")).toInt();
        if (id < 1 || id > 255 || seen.contains(id)) return rejection(QStringLiteral("invalid-payload-target-identity"));
        seen.insert(id);
        Target target;
        target.vehicleId = id;
        target.vehicle = liveVehicle(id);
        target.manager = target.vehicle ? target.vehicle->missionManager() : nullptr;
        target.payload = payload;
        QString reason;
        if (!_vehicleReady(target, true, &reason) || !sourcesAlive()) return rejection(reason.isEmpty()
            ? QStringLiteral("native-episode-changed-during-target-capture") : reason);
        targets.append(target);
        ids.append(id);
    }
    // Consume only after full payload/actual target validation; every later
    // gate checks the same native grant without renewing its lifetime.
    const QString frozenDeploymentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!sourcesAlive()) return rejection(QStringLiteral("native-episode-changed-before-upload-consumption"));
    const QVariantMap grant = approval->consumeUploadApproval(session.data(), role.data(), store.data(), owner.data(),
        canonicalBinding, authorizationReceipt, identityReceipt, approvalReceipt, frozenDeploymentId);
    if (!sourcesAlive() || !approval->grantCurrent(grant, frozenDeploymentId) || !sourcesAlive())
        return rejection(QStringLiteral("independent-upload-approval-required-or-consumed"));
    _archive();
    _disconnectAll();
    const quint64 generation = ++_generation;
    _store = store; _intent = intent; _review = review; _health = health;
    _swarm = swarm; _authorization = authorizationOwner;
    _rolePolicy = role; _uploadRoleReceipt = authorizationReceipt;
    _operatorSession = session; _operationApproval = approval;
    _uploadIdentityReceipt = identityReceipt; _uploadApprovalGrant = grant;
    _binding = canonicalBinding; _recordSnapshot = record; _reviewSnapshot = reviewSnapshot;
    _planningTask.clear(); _planningParametersFingerprint.clear();
    _planningTaskUnavailableReason = QStringLiteral("original-planning-input-not-captured");
    _payloadBuild = build; _targets = targets; _vehicleIds = ids;
    _deploymentId = frozenDeploymentId;
    _createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    _state = QStringLiteral("preparing"); _reason.clear(); _nextTarget = 0;
    _dispatching = false; _transportDraining = false; _reservationHeld = false;
    _deferredStep = 0; _deferredTarget = -1; _deferredDeadlineMs = 0;
    QString reason;
    const bool gateAllowed = _gate(generation, false, &reason);
    if (!self) return rejection(QStringLiteral("deployment-context-destroyed"));
    if (!gateAllowed) {
        _fail(generation, reason); return rejection(reason);
    }
    // This is supplementary planning provenance, NOT another upload gate.
    // Callback-sensitive admission has completed; freeze only if the exact
    // original parameters still belong to this unchanged native intent.
    QByteArray afterPlanningFingerprint;
    if (planningInputAvailable && _same(generation) && intent && _intent == intent
        && intent->parameters() == originalParameters
        && planningInputFingerprint(intent->parameters(), &afterPlanningFingerprint)
        && afterPlanningFingerprint == planningParametersFingerprint) {
        _planningTask = planningTask;
        _planningParametersFingerprint = planningParametersFingerprint;
        _planningTaskUnavailableReason.clear();
    } else {
        _planningTaskUnavailableReason = planningInputAvailable
            ? QStringLiteral("original-planning-input-changed-during-admission")
            : QStringLiteral("original-planning-input-not-bounded-pure-values");
    }
    if (!_swarm->reserveMissionDeployment(_deploymentId, _vehicleIds)) {
        _fail(generation, QStringLiteral("whole-fleet-reservation-denied"));
        return rejection(QStringLiteral("whole-fleet-reservation-denied"));
    }
    _reservationHeld = true;
    _connectContext(generation);
    for (int i = 0; i < _targets.size(); ++i) _connectTarget(i, generation);
    _timer.start();
    const QString acceptedId = _deploymentId;
    if (!_publish(generation)) return rejection(QStringLiteral("deployment-superseded"));
    QTimer::singleShot(0, this, [this, generation] { _uploadNext(generation); });
    return {{QStringLiteral("ok"), _same(generation) && _active()},
        {QStringLiteral("deploymentId"), acceptedId}, {QStringLiteral("state"), _state},
        {QStringLiteral("executionVerified"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

void FleetMissionDeployment::_disconnectAll()
{
    for (const auto& connection : _connections) disconnect(connection);
    _connections.clear();
}

void FleetMissionDeployment::_connectContext(quint64 generation)
{
    auto invalidate = [this, generation] {
        if (!_same(generation) || (!_active() && _state != QStringLiteral("verified"))) return;
        QPointer<FleetMissionDeployment> guard(this);
        QString reason;
        const bool allowed = _gate(generation, _active(), &reason);
        if (guard && !allowed) _fail(generation, reason);
    };
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, invalidate));
    _connections.append(connect(_intent.data(), &FleetIntentTask::stateChanged, this, invalidate));
    _connections.append(connect(_intent.data(), &FleetIntentTask::parametersChanged, this, invalidate));
    _connections.append(connect(_intent.data(), &FleetIntentTask::intentChanged, this, invalidate));
    _connections.append(connect(_review.data(), &FleetMissionReviewCoordinator::reviewChanged, this, invalidate));
    _connections.append(connect(_health.data(), &FleetHealthCenter::healthChanged, this, invalidate));
    _connections.append(connect(_swarm.data(), &SwarmController::transactionsChanged, this, invalidate));
    _connections.append(connect(_rolePolicy.data(), &FleetRolePolicy::currentUserChanged, this, invalidate));
    _connections.append(connect(_rolePolicy.data(), &FleetRolePolicy::currentRoleChanged, this, invalidate));
    _connections.append(connect(_rolePolicy.data(), &FleetRolePolicy::policyChanged, this, invalidate));
    _connections.append(connect(_operatorSession.data(), &FleetOperatorSession::changed, this, invalidate));
    _connections.append(connect(_operationApproval.data(), &FleetOperationApproval::changed, this, invalidate));
    for (QObject* context : QList<QObject*>() << _store.data() << _intent.data() << _review.data()
             << _health.data() << _swarm.data() << _rolePolicy.data() << _operatorSession.data()
             << _operationApproval.data() << _authorization.data()) {
        _connections.append(connect(context, &QObject::destroyed, this, [this, generation] {
            _fail(generation, QStringLiteral("deployment-context-destroyed"));
        }));
    }
}

void FleetMissionDeployment::_connectTarget(int index, quint64 generation)
{
    MissionManager* manager = _targets[index].manager.data();
    _connections.append(connect(manager, &PlanManager::operationStarted, this,
        [this, index, generation](quint64 id, int type) { _started(index, generation, id, type); }));
    _connections.append(connect(manager, &PlanManager::sendCompleteWithOperationId, this,
        [this, index, generation](quint64 id, bool error) { _sent(index, generation, id, error); }));
    _connections.append(connect(manager, &PlanManager::newMissionItemsAvailableWithOperationId, this,
        [this, index, generation](quint64 id, bool removeAll) { _read(index, generation, id, removeAll); }));
    _connections.append(connect(manager, &PlanManager::errorWithOperationId, this,
        [this, index, generation](quint64 id, int, const QString& error) { _error(index, generation, id, error); }));
    // Finish signals run inside PlanManager::_finishTransaction. Defer drain
    // and all subsequent reads/writes until its outer stack has unwound.
    _connections.append(connect(manager, &PlanManager::inProgressChanged, this,
        [this, generation](bool) { QTimer::singleShot(0, this, [this, generation] {
            if (_same(generation)) _drain();
        }); }));
    auto lost = [this, generation] { _fail(generation, QStringLiteral("frozen-vehicle-context-changed")); };
    _connections.append(connect(manager, &QObject::destroyed, this, lost));
    _connections.append(connect(_targets[index].vehicle.data(), &QObject::destroyed, this, lost));
    _connections.append(connect(_targets[index].vehicle.data(), &Vehicle::armedChanged, this,
        [this, generation](bool armed) { if (armed) _fail(generation, QStringLiteral("vehicle-armed-during-staging")); }));
    _connections.append(connect(_targets[index].vehicle.data(), &Vehicle::flyingChanged, this,
        [this, generation](bool flying) { if (flying) _fail(generation, QStringLiteral("vehicle-flying-during-staging")); }));
}

void FleetMissionDeployment::_uploadNext(quint64 generation)
{
    if (!_same(generation) || !_active()) return;
    if (!outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        _fail(generation, QStringLiteral("current-native-deployment-episode-required")); return;
    }
    if (_stackBusy()) {
        _deferredStep = 1; _deferredTarget = -1;
        if (_deferredDeadlineMs == 0) _deferredDeadlineMs = _clock.elapsed() + 120000;
        return;
    }
    QPointer<FleetMissionDeployment> self(this);
    QString reason;
    bool allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed) { _fail(generation, reason); return; }
    if (_nextTarget >= _targets.size()) {
        _state = QStringLiteral("verified");
        _reason = QStringLiteral("onboard-mission-items-matched-not-executable");
        if (!_drain() || !_same(generation)) return;
        _archive(); _publish(generation); return;
    }
    const int index = _nextTarget;
    if (!_vehicleReady(_targets[index], true, &reason) || !self
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (self) _fail(generation, reason);
        return;
    }
    _state = QStringLiteral("uploading");
    _targets[index].state = QStringLiteral("uploading");
    _targets[index].awaitingType = PlanManager::TransactionWrite;
    _targets[index].operationId = 0;
    _targets[index].operationError = false;
    if (!_publish(generation) || !_active()) return;
    allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed || !_vehicleReady(_targets[index], true, &reason) || !self
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (!self) return;
        _fail(generation, reason); return;
    }
    const QString deploymentId = _deploymentId;
    const QPointer<Vehicle> vehicle(_targets.at(index).vehicle);
    const QPointer<MissionManager> manager(_targets.at(index).manager);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const SharedLinkInterfacePtr primary = links ? links->primaryLink().lock() : SharedLinkInterfacePtr();
    if (!self || !_same(generation) || !_active() || _nextTarget != index || !vehicle || !manager || !links || !primary
        || _deploymentId != deploymentId || _targets.at(index).vehicle != vehicle || _targets.at(index).manager != manager
        || vehicle->missionManager() != manager.data() || vehicle->vehicleLinkManager() != links.data()
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (self && _same(generation)) _fail(generation, QStringLiteral("original-upload-primary-unavailable"));
        return;
    }
    _targets[index].transportPrimary = primary.get();
    _targets[index].transportPrimaryOwner = primary;
    _targets[index].transportPrimaryCaptured = true;
    const QPointer<LinkInterface> expectedPrimary(primary.get());
    const QPointer<FleetOperatorSession> session(_operatorSession);
    const QPointer<FleetOperationApproval> approval(_operationApproval);
    const std::function<bool()> fence = [self, generation, deploymentId, index, vehicle, manager, expectedPrimary, session, approval]() {
        const auto same = [&]() {
            return self && self->thread() == QThread::currentThread() && vehicle && manager && expectedPrimary && session && approval
                && self->_nextTarget == index && self->_operatorSession == session && self->_operationApproval == approval
                && self->nativeTransportContextCurrent(manager.data(), vehicle.data(), expectedPrimary.data(), deploymentId,
                    generation, PlanManager::TransactionWrite, manager->activeOperationId());
        };
        if (!same()) return false;
        QString reason;
        const bool allowed = self->_gate(generation, true, &reason);
        return same() && allowed;
    };
    const QVariantList raw = _targets[index].payload.value(QStringLiteral("items")).toList();
    QList<double> parameters4;
    for (const QVariant& value : raw) {
        double parameter = 0.0;
        if (value.type() != QVariant::Map || !missionParameter4(value.toMap(), &parameter)) {
            _fail(generation, QStringLiteral("unsupported-native-command-parameter4"));
            return;
        }
        parameters4.append(parameter);
    }
    QList<MissionItem*> items;
    QList<QPointer<MissionItem>> guards;
    const QGeoCoordinate home = _targets[index].vehicle->homePosition();
    QPointer<MissionItem> syntheticHome = new MissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL,
        0, 0, 0, 0, home.latitude(), home.longitude(), home.altitude(), true, false);
    items.append(syntheticHome.data());
    for (int i = 0; i < raw.size(); ++i) {
        const QVariantMap item = raw[i].toMap();
        MissionItem* native = new MissionItem(i + 1, static_cast<MAV_CMD>(item.value(QStringLiteral("command")).toInt()),
            static_cast<MAV_FRAME>(item.value(QStringLiteral("frame")).toInt()),
            item.value(QStringLiteral("param1")).toDouble(), item.value(QStringLiteral("param2")).toDouble(),
            item.value(QStringLiteral("param3")).toDouble(), parameters4.at(i),
            item.value(QStringLiteral("param5")).toDouble(), item.value(QStringLiteral("param6")).toDouble(),
            item.value(QStringLiteral("param7")).toDouble(), item.value(QStringLiteral("autoContinue")).toBool(), false);
        items.append(native); guards.append(QPointer<MissionItem>(native));
    }
    // Synthetic home is NOT adopted by the PX4 PlanManager; the others are
    // adopted only if the scoped native writer actually begins an operation.
    if (!self) {
        if (syntheticHome) delete syntheticHome.data();
        for (const QPointer<MissionItem>& item : guards) if (item) delete item.data();
        return;
    }
    // The actual native scope retains this by-value fence through adoption,
    // outgoing-plugin callbacks and every queue attempt, including retries.
    if (!self || !_same(generation) || !_active() || !manager
        || !vehicle || !expectedPrimary || _deploymentId != deploymentId || _nextTarget != index
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (syntheticHome) delete syntheticHome.data();
        for (const QPointer<MissionItem>& item : guards) if (item) delete item.data();
        if (self && _same(generation)) _fail(generation, QStringLiteral("native-episode-changed-before-upload-entry"));
        return;
    }
    _dispatching = true;
    manager->writeFleetDeploymentMissionItems(items, self.data(), expectedPrimary.data(), deploymentId, generation, fence);
    if (syntheticHome) delete syntheticHome.data();
    if (!self) return;
    _dispatching = false;
    const bool adopted = _same(generation) && _targets[index].operationId != 0;
    if (!adopted) for (const QPointer<MissionItem>& item : guards) if (item) delete item.data();
    if (!_same(generation)) return;
    if (!adopted) _fail(generation, QStringLiteral("write-not-started"));
    else if (!_active()) _drain();
    else _resumeDeferred(generation);
}

void FleetMissionDeployment::_resumeDeferred(quint64 generation)
{
    if (!_same(generation) || !_active() || _stackBusy() || _deferredStep == 0) return;
    if (!outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        _fail(generation, QStringLiteral("current-native-deployment-episode-required")); return;
    }
    const int step = _deferredStep;
    const int index = _deferredTarget;
    _deferredStep = 0; _deferredTarget = -1; _deferredDeadlineMs = 0;
    QTimer::singleShot(0, this, [this, generation, step, index] {
        if (!_same(generation) || !_active()) return;
        if (step == 1) _uploadNext(generation);
        else if (step == 2) _readback(index, generation);
    });
}

void FleetMissionDeployment::_started(int index, quint64 generation, quint64 operationId, int type)
{
    if (!_same(generation) || index < 0 || index >= _targets.size()) return;
    Target& target = _targets[index];
    if (operationId != 0 && _dispatching && index == _nextTarget
        && target.awaitingType == type && target.operationId == 0) {
        QPointer<FleetMissionDeployment> guard(this);
        target.operationId = operationId;
        target.deadlineMs = _clock.elapsed()
            + qBound<qint64>(120000, target.payload.value(QStringLiteral("items")).toList().size() * qint64(4000), 3600000);
        if (type == PlanManager::TransactionWrite) {
            target.writeOperationId = operationId;
            target.writeMayHaveCommitted = true;
            // Geometry staging must not become startable through the legacy
            // mission-start UI after this job releases its local reservation.
            if (_swarm) _swarm->noteMissionDeploymentWrite(_deploymentId, target.vehicleId,
                operationId, target.manager.data());
        } else if (type == PlanManager::TransactionRead) target.readOperationId = operationId;
        if (!guard) return;
        QString reason;
        const bool allowed = !_active() || _gate(generation, true, &reason);
        if (guard && !allowed) _fail(generation, reason);
        return;
    }
    // A cache-changing operation outside this exact phase invalidates even a
    // previously verified receipt. Never silently attach to somebody's read.
    _fail(generation, QStringLiteral("foreign-mission-operation"));
}

void FleetMissionDeployment::_sent(int index, quint64 generation, quint64 operationId, bool error)
{
    if (!_same(generation) || index != _nextTarget || index >= _targets.size()) return;
    Target& target = _targets[index];
    if (!operationId || target.operationId != operationId || target.awaitingType != PlanManager::TransactionWrite) return;
    if (!_active()) { QTimer::singleShot(0, this, [this, generation] { if (_same(generation)) _drain(); }); return; }
    if (error || target.operationError) { _fail(generation, QStringLiteral("mission-upload-failed")); return; }
    QPointer<FleetMissionDeployment> self(this);
    QString reason;
    const bool allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed) { _fail(generation, reason); return; }
    target.uploadAcknowledged = true;
    target.state = QStringLiteral("upload-acknowledged");
    target.awaitingType = PlanManager::TransactionNone;
    target.deadlineMs = 0;
    if (!_publish(generation) || !_active()) return;
    QTimer::singleShot(0, this, [this, index, generation] { _readback(index, generation); });
}

void FleetMissionDeployment::_readback(int index, quint64 generation)
{
    if (!_same(generation) || !_active() || index != _nextTarget) return;
    if (!outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        _fail(generation, QStringLiteral("current-native-deployment-episode-required")); return;
    }
    if (_stackBusy()) {
        _deferredStep = 2; _deferredTarget = index;
        if (_deferredDeadlineMs == 0) _deferredDeadlineMs = _clock.elapsed() + 120000;
        return;
    }
    QPointer<FleetMissionDeployment> self(this);
    QString reason;
    bool allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed || !_vehicleReady(_targets[index], true, &reason) || !self
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (!self) return;
        _fail(generation, reason); return;
    }
    _state = QStringLiteral("readback");
    _targets[index].state = QStringLiteral("readback");
    _targets[index].awaitingType = PlanManager::TransactionRead;
    _targets[index].operationId = 0;
    _targets[index].operationError = false;
    if (!_publish(generation) || !_active()) return;
    allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed || !_vehicleReady(_targets[index], true, &reason) || !self
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (!self) return;
        _fail(generation, reason); return;
    }
    const QString deploymentId = _deploymentId;
    const QPointer<Vehicle> vehicle(_targets.at(index).vehicle);
    const QPointer<MissionManager> manager(_targets.at(index).manager);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const SharedLinkInterfacePtr primary = links ? links->primaryLink().lock() : SharedLinkInterfacePtr();
    if (!self || !_same(generation) || !_active() || _nextTarget != index || !vehicle || !manager || !links || !primary
        || _deploymentId != deploymentId || _targets.at(index).vehicle != vehicle || _targets.at(index).manager != manager
        || vehicle->missionManager() != manager.data() || vehicle->vehicleLinkManager() != links.data()
        || !outboundSources(this, _operatorSession.data(), _operationApproval.data())) {
        if (self && _same(generation)) _fail(generation, QStringLiteral("original-readback-primary-unavailable"));
        return;
    }
    _targets[index].transportPrimary = primary.get();
    _targets[index].transportPrimaryOwner = primary;
    _targets[index].transportPrimaryCaptured = true;
    const QPointer<LinkInterface> expectedPrimary(primary.get());
    const QPointer<FleetOperatorSession> session(_operatorSession);
    const QPointer<FleetOperationApproval> approval(_operationApproval);
    const std::function<bool()> fence = [self, generation, deploymentId, index, vehicle, manager, expectedPrimary, session, approval]() {
        const auto same = [&]() {
            return self && self->thread() == QThread::currentThread() && vehicle && manager && expectedPrimary && session && approval
                && self->_nextTarget == index && self->_operatorSession == session && self->_operationApproval == approval
                && self->nativeTransportContextCurrent(manager.data(), vehicle.data(), expectedPrimary.data(), deploymentId,
                    generation, PlanManager::TransactionRead, manager->activeOperationId());
        };
        if (!same()) return false;
        QString reason;
        const bool allowed = self->_gate(generation, true, &reason);
        return same() && allowed;
    };
    _dispatching = true;
    manager->loadFleetDeploymentFromVehicle(self.data(), expectedPrimary.data(), deploymentId, generation, fence);
    if (!self) return;
    _dispatching = false;
    if (!_same(generation)) return;
    if (_targets[index].operationId == 0) _fail(generation, QStringLiteral("readback-not-started"));
    else if (!_active()) _drain();
    else _resumeDeferred(generation);
}

void FleetMissionDeployment::_read(int index, quint64 generation, quint64 operationId, bool removeAll)
{
    if (!_same(generation) || index < 0 || index != _nextTarget || index >= _targets.size()) return;
    // Do not retain a Target reference across either full gate: native/QML
    // authorization callbacks can cancel the deployment or destroy its owner.
    const Target original = _targets.at(index);
    if (!operationId || original.operationId != operationId || original.awaitingType != PlanManager::TransactionRead) return;
    if (!_active()) { QTimer::singleShot(0, this, [this, generation] { if (_same(generation)) _drain(); }); return; }
    if (removeAll || original.operationError || !original.manager) {
        _fail(generation, QStringLiteral("mission-readback-failed")); return;
    }
    QPointer<FleetMissionDeployment> self(this);
    const QString deploymentId = _deploymentId;
    const QPointer<Vehicle> vehicle = original.vehicle;
    const QPointer<MissionManager> manager = original.manager;
    const QPointer<LinkInterface> primary = original.transportPrimary;
    const QPointer<VehicleLinkManager> links = vehicle ? vehicle->vehicleLinkManager() : nullptr;
    const WeakLinkInterfacePtr primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    const auto sameTarget = [self, index, generation, operationId, deploymentId, original,
                             vehicle, manager, primary, links, primaryOwner]() {
        if (!self || !self->_same(generation) || !self->_active() || self->_deploymentId != deploymentId
            || self->_state != QStringLiteral("readback") || self->_nextTarget != index
            || index < 0 || index >= self->_targets.size() || !vehicle || !manager || !primary || !links
            || vehicle->missionManager() != manager.data() || vehicle->vehicleLinkManager() != links.data()
            || vehicle->id() != original.vehicleId || manager->inProgress() || manager->activeOperationId() != 0) return false;
        const Target& now = self->_targets.at(index);
        if (now.vehicle != vehicle || now.manager != manager || !now.transportPrimaryCaptured
            || now.transportPrimary != primary || now.operationId != operationId
            || now.writeOperationId != original.writeOperationId || now.readOperationId != operationId
            || now.readOperationId != original.readOperationId || now.writeOperationId == 0
            || now.awaitingType != PlanManager::TransactionRead || now.operationError
            || now.payload != original.payload) return false;
        const WeakLinkInterfacePtr currentPrimary = links->primaryLink();
        // Never pin the link alive through a callback. The original shared
        // control block, not just a recycled raw address, must still own it.
        return !primaryOwner.expired() && !currentPrimary.expired()
            && !primaryOwner.owner_before(currentPrimary) && !currentPrimary.owner_before(primaryOwner);
    };
    const auto completionConfirmed = [self, manager, vehicle, primary, deploymentId, generation, operationId]() {
        return self && manager && vehicle && primary
            && manager->fleetDeploymentReadSucceeded(self.data(), vehicle.data(), primary.data(),
                deploymentId, generation, operationId);
    };
    const auto failOriginal = [self, generation, deploymentId](const QString& reason) {
        if (self && self->_same(generation) && self->_deploymentId == deploymentId && self->_active())
            self->_fail(generation, reason);
    };
    if (!sameTarget()) { failOriginal(QStringLiteral("readback-context-changed")); return; }
    // A failed ordinary READ can emit the same legacy completion signal after
    // clearing its cache. Only native scoped success/COUNT/terminal-ACK evidence
    // distinguishes that failure from an actual completed COUNT=0 observation.
    if (!completionConfirmed()) { failOriginal(QStringLiteral("readback-completion-unconfirmed")); return; }
    QString reason;
    const bool allowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!allowed) { failOriginal(reason); return; }
    if (!sameTarget()) { failOriginal(QStringLiteral("readback-context-changed")); return; }
    if (!completionConfirmed()) { failOriginal(QStringLiteral("readback-completion-unconfirmed")); return; }

    constexpr int MaxDiagnosticItems = 2048; // Same bound as the payload builder.
    const int observedItemCount = manager->missionItems().size();
    const bool withinItemLimit = observedItemCount <= MaxDiagnosticItems;
    QList<QPointer<MissionItem>> observedItems;
    QVariantList raw;
    bool hasNullItem = false;
    if (withinItemLimit) {
        observedItems.reserve(observedItemCount);
        raw.reserve(observedItemCount);
        for (MissionItem* item : manager->missionItems()) {
            observedItems.append(QPointer<MissionItem>(item));
            hasNullItem = hasNullItem || !item;
            raw.append(item ? QVariant(decodedReadbackItem(item)) : QVariant());
        }
    }
    const qint64 observedAt = _clock.elapsed();
    reason.clear();
    const bool stillAllowed = _gate(generation, true, &reason);
    if (!self) return;
    if (!stillAllowed) { failOriginal(reason); return; }
    if (!sameTarget()) { failOriginal(QStringLiteral("readback-context-changed")); return; }
    if (!completionConfirmed()) { failOriginal(QStringLiteral("readback-completion-unconfirmed")); return; }
    if (manager->missionItems().size() != observedItemCount) {
        failOriginal(QStringLiteral("readback-snapshot-changed")); return;
    }
    if (withinItemLimit) {
        for (int i = 0; i < observedItemCount; ++i) {
            const QPointer<MissionItem> item = observedItems.at(i);
            // QPointer also detects destruction/address reuse while the full
            // gate ran. Never dereference a replaced pointer from the cache.
            if (manager->missionItems().at(i) != item.data()
                || (raw.at(i).isValid() && (!item
                    || !sameDecodedReadbackItem(raw.at(i).toMap(), decodedReadbackItem(item.data()))))) {
                failOriginal(QStringLiteral("readback-snapshot-changed")); return;
            }
        }
    }
    // From here to publication there are no full gates or external callbacks.
    // This diagnostic is explanatory history, not the verification decision.
    Target& target = _targets[index];
    target.readbackDiagnostic = {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("deploymentId"), deploymentId}, {QStringLiteral("generation"), QString::number(generation)},
        {QStringLiteral("vehicleId"), original.vehicleId},
        {QStringLiteral("writeOperationId"), QString::number(original.writeOperationId)},
        {QStringLiteral("readOperationId"), QString::number(operationId)},
        {QStringLiteral("observedAtMonotonicMs"), QString::number(observedAt)},
        {QStringLiteral("observationScope"), QStringLiteral("ordinary-decoded-mission-items-history")},
        {QStringLiteral("captureState"), withinItemLimit ? QStringLiteral("captured") : QStringLiteral("item-limit-exceeded")},
        {QStringLiteral("observedItemCount"), observedItemCount},
        {QStringLiteral("comparison"), withinItemLimit
            ? FleetMissionReadbackDiff::compare(original.payload.value(QStringLiteral("items")).toList(), raw) : QVariantMap()},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (!withinItemLimit) { _fail(generation, QStringLiteral("readback-item-limit-exceeded")); return; }
    const QVariantList expected = original.payload.value(QStringLiteral("canonicalWireItems")).toList();
    if (raw.isEmpty() || raw.size() != expected.size()) {
        _fail(generation, QStringLiteral("readback-item-count-mismatch")); return;
    }
    if (hasNullItem) { _fail(generation, QStringLiteral("readback-null-item")); return; }
    const QVariantMap canonical = FleetMissionPayloadBuilder::canonicalWireItems(raw, true);
    if (!strictTrue(canonical.value(QStringLiteral("ok")))
        || canonical.value(QStringLiteral("items")).toList() != expected
        || canonical.value(QStringLiteral("contentHash")) != target.payload.value(QStringLiteral("contentHash"))) {
        _fail(generation, QStringLiteral("readback-content-mismatch")); return;
    }
    target.readbackVerified = true;
    // Immutable in-memory provenance. Fail/cancel/ARM revokes the live flag
    // above but never turns this past match into a current start authorization.
    if (target.historicalReadbackReceipt.isEmpty()) target.historicalReadbackReceipt = {
        {QStringLiteral("matched"), true}, {QStringLiteral("vehicleId"), target.vehicleId},
        {QStringLiteral("writeOperationId"), QString::number(target.writeOperationId)},
        {QStringLiteral("readOperationId"), QString::number(target.readOperationId)},
        {QStringLiteral("contentHash"), target.payload.value(QStringLiteral("contentHash"))},
        {QStringLiteral("wireSchema"), target.payload.value(QStringLiteral("wireSchema"))},
        {QStringLiteral("itemCount"), expected.size()}, {QStringLiteral("observedAtMonotonicMs"), _clock.elapsed()},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("startupPermitGranted"), false}};
    target.state = QStringLiteral("verified");
    target.awaitingType = PlanManager::TransactionNone;
    target.deadlineMs = 0;
    ++_nextTarget;
    if (!_publish(generation) || !_active()) return;
    QTimer::singleShot(0, this, [this, generation] { _uploadNext(generation); });
}

void FleetMissionDeployment::_error(int index, quint64 generation, quint64 operationId, const QString& message)
{
    if (!_same(generation) || index < 0 || index >= _targets.size()) return;
    Target& target = _targets[index];
    if (!operationId || target.operationId != operationId || target.awaitingType == PlanManager::TransactionNone) return;
    target.operationError = true;
    target.error = message.left(1024);
    _fail(generation, QStringLiteral("mission-protocol-error"));
}

void FleetMissionDeployment::_fail(quint64 generation, const QString& reason)
{
    if (!_same(generation) || _deploymentId.isEmpty()) return;
    if (_state == QStringLiteral("failed") || _state == QStringLiteral("cancelled")) {
        QTimer::singleShot(0, this, [this, generation] { if (_same(generation)) _drain(); }); return;
    }
    _state = QStringLiteral("failed"); _reason = reason.left(1024);
    _deferredStep = 0; _deferredTarget = -1; _deferredDeadlineMs = 0;
    for (Target& target : _targets) {
        target.readbackVerified = false;
        if (target.state != QStringLiteral("waiting")) target.state = QStringLiteral("failed");
    }
    _transportDraining = _reservationHeld;
    _archive();
    if (!_publish(generation)) return;
    QTimer::singleShot(0, this, [this, generation] { if (_same(generation)) _drain(); });
}

bool FleetMissionDeployment::cancel(const QString& deploymentId, const QString& reason)
{
    if (deploymentId.isEmpty() || deploymentId != _deploymentId || _state == QStringLiteral("cancelled")) return false;
    const quint64 generation = _generation;
    _state = QStringLiteral("cancelled");
    _deferredStep = 0; _deferredTarget = -1; _deferredDeadlineMs = 0;
    _reason = reason.isEmpty() ? QStringLiteral("operator-cancelled-no-rollback") : reason.left(1024);
    for (Target& target : _targets) target.readbackVerified = false;
    _transportDraining = _reservationHeld;
    _archive();
    if (!_publish(generation)) return false;
    QTimer::singleShot(0, this, [this, generation] { if (_same(generation)) _drain(); });
    return _same(generation) && _state == QStringLiteral("cancelled");
}

bool FleetMissionDeployment::_drain()
{
    if (_active() || !_reservationHeld) return true;
    bool pending = _stackBusy();
    for (Target& target : _targets) {
        if (target.manager && (target.manager->inProgress() || target.manager->transportCallInProgress())) {
            pending = true;
            target.state = QStringLiteral("transport-pending");
        }
    }
    const bool previous = _transportDraining;
    _transportDraining = pending;
    if (!pending) {
        if (_swarm) _swarm->releaseMissionDeployment(_deploymentId);
        _reservationHeld = false;
        for (Target& target : _targets) if (target.state == QStringLiteral("transport-pending"))
            target.state = _state == QStringLiteral("verified") ? QStringLiteral("verified") : _state;
    }
    _archive();
    if (previous != _transportDraining || !pending) return _publish(_generation);
    return true;
}

void FleetMissionDeployment::_tick()
{
    // Clear expired display captures even when no staging transaction is live.
    // This signal is the final action before checking owner lifetime again.
    QPointer<FleetMissionDeployment> inspectionOwner(this);
    if (!_startReadinessSnapshot.isEmpty() && (_inspectionGeneration != _generation
            || _inspectionState != _state || _inspectionCapturedMs < 0
            || _clock.elapsed() - _inspectionCapturedMs > 1000)) {
        _startReadinessSnapshot.clear();
        _progressObservationSnapshot.clear();
        _inspectionCapturedMs = -1;
        emit inspectionChanged();
        if (!inspectionOwner) return;
    }
    const quint64 generation = _generation;
    if (!_active() && _state != QStringLiteral("verified")) { _drain(); return; }
    QPointer<FleetMissionDeployment> self(this);
    QString reason;
    const bool allowed = _gate(generation, _active(), &reason);
    if (!self) return;
    if (!allowed) { _fail(generation, reason); return; }
    const qint64 now = _clock.elapsed();
    if (_deferredDeadlineMs != 0 && now > _deferredDeadlineMs) {
        _fail(generation, QStringLiteral("deployment-continuation-deadline-exceeded")); return;
    }
    for (const Target& target : _targets) {
        if (target.deadlineMs != 0 && now > target.deadlineMs) {
            _fail(generation, QStringLiteral("deployment-operation-deadline-exceeded")); return;
        }
    }
    _resumeDeferred(generation);
}

void FleetMissionDeployment::_archive()
{
    if (_deploymentId.isEmpty() || _active()) return;
    QVariantMap row = current();
    row.remove(QStringLiteral("payload"));
    QVariantList vehicles = row.value(QStringLiteral("vehicles")).toList();
    for (QVariant& value : vehicles) {
        QVariantMap vehicle = value.toMap();
        QVariantMap diagnostic = vehicle.value(QStringLiteral("readbackDiagnostic")).toMap();
        if (!diagnostic.isEmpty()) {
            const QVariantMap comparison = diagnostic.value(QStringLiteral("comparison")).toMap();
            if (!comparison.isEmpty()) diagnostic.insert(QStringLiteral("comparison"), FleetMissionReadbackDiff::summary(comparison));
            vehicle.insert(QStringLiteral("readbackDiagnostic"), diagnostic);
            value = vehicle;
        }
    }
    row.insert(QStringLiteral("vehicles"), vehicles);
    for (int i = 0; i < _history.size(); ++i) {
        if (_history[i].toMap().value(QStringLiteral("deploymentId")).toString() == _deploymentId) {
            _history[i] = row; return;
        }
    }
    _history.append(row);
    while (_history.size() > 32) _history.removeFirst();
}

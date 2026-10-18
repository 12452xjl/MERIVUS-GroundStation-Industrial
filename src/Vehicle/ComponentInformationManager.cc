/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComponentInformationManager.h"
#include "ComponentMetadataArtifactWorker.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "LinkInterface.h"
#include "FTPManager.h"
#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "CompInfoEvents.h"
#include "CompInfoActuators.h"

#include <QByteArray>
#include <QFile>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QUuid>
#include <QVector>
#include <limits>

QGC_LOGGING_CATEGORY(ComponentInformationManagerLog, "ComponentInformationManagerLog")

struct ComponentInformationRequestSource
{
    QPointer<Vehicle> vehicle;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    QVector<QMetaObject::Connection> connections;
    quint64 serial = 0;
    int vehicleId = 0;
    int componentId = 0;
    MAV_AUTOPILOT firmware = MAV_AUTOPILOT_GENERIC;
    QString versionEpoch;
    bool requestMessagesAllowed = false;
    bool retired = false;
    ~ComponentInformationRequestSource() { for (const auto& connection : connections) QObject::disconnect(connection); }
};

struct ComponentMetadataFileRequest
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    std::shared_ptr<ComponentInformationRequestSource> source;
    quint64 serial = 0;
    int stateIndex = -1;
    QString cacheTag;
    QString uri;
    quint32 expectedCrc = 0;
    bool hasExpectedCrc = false;
    bool compressed = false;
    bool allowDecodedCrc = false;
    bool translation = false;
    bool invalidationAttempted = false;
    bool cacheHit = false;
    quint64 workerGeneration = 0;
    bool resultSeen = false;
    bool finishedSeen = false;
    bool workerOk = false;
    bool downloadStarting = false;
    bool downloadCompletionSeen = false;
    QString downloadFile;
    QString downloadError;
    QString workerError;
    const QObject* workerIdentity = nullptr; // Comparison only, never dereferenced.
    bool workerScheduling = false;
    bool workerNotStarted = false;
    QString workerNotStartedReason;
    QGCOwnedDownloadFilePtr downloadOutput;
    QPointer<FTPManager> ftp;
    QPointer<QGCCachedFileDownload> http;
    QPointer<ComponentMetadataArtifactWorker> worker;
    QMetaObject::Connection downloadConnection;
};

struct ComponentMetadataTranslationRequest
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    std::shared_ptr<ComponentInformationRequestSource> source;
    std::shared_ptr<const ComponentMetadataArtifact> metadataInput;
    std::shared_ptr<const ComponentMetadataArtifact> summaryInput;
    QPointer<ComponentInformationTranslation> producer;
    quint64 serial = 0;
    int stateIndex = -1;
    bool starting = true;
    bool completionSeen = false;
    QString file;
    QString error;
    QString outputToRemove;
    ~ComponentMetadataTranslationRequest() { if (!outputToRemove.isEmpty()) QFile::remove(outputToRemove); }
};

// Each opaque callback data has its own immutable QObject lifetime, not a
// borrowed machine which could already represent a later request.
struct ComponentMetadataMessageContext : QObject
{
    ComponentMetadataMessageContext(QObject* parent, RequestMetaDataTypeStateMachine* machine,
                                    CompInfo* info, quint64 serial, StateMachine::StateFn state)
        : QObject(parent), machine(machine), info(info), serial(serial), state(state) {}
    const QPointer<RequestMetaDataTypeStateMachine> machine;
    const QPointer<CompInfo> info;
    const quint64 serial;
    const StateMachine::StateFn state;
};

const ComponentInformationManager::StateFn ComponentInformationManager::_rgStates[] = {
    _stateRequestCompInfoGeneral, _stateRequestCompInfoGeneralComplete, _stateRequestCompInfoParam,
    _stateRequestCompInfoEvents, _stateRequestCompInfoActuators, _stateRequestAllCompInfoComplete
};
const int ComponentInformationManager::_cStates = sizeof(_rgStates) / sizeof(_rgStates[0]);
const RequestMetaDataTypeStateMachine::StateFn RequestMetaDataTypeStateMachine::_rgStates[] = {
    _stateRequestCompInfo, _stateRequestCompInfoDeprecated, _stateRequestMetaDataJson,
    _stateRequestMetaDataJsonFallback, _stateRequestTranslationJson, _stateRequestTranslate, _stateRequestComplete
};
const int RequestMetaDataTypeStateMachine::_cStates = sizeof(_rgStates) / sizeof(_rgStates[0]);

ComponentInformationManager::ComponentInformationManager(Vehicle* vehicle)
    : _vehicle(vehicle), _requestTypeStateMachine(this)
    , _cachedFileDownload(new QGCCachedFileDownload(this, QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/QGCCompInfoFileDownloadCache")))
    , _fileCache(ComponentInformationCache::defaultInstance())
    , _translation(new ComponentInformationTranslation(this, _cachedFileDownload))
{
    setParent(vehicle);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_GENERAL] = new CompInfoGeneral(MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_PARAMETER] = new CompInfoParam(MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_EVENTS] = new CompInfoEvents(MAV_COMP_ID_AUTOPILOT1, vehicle, this);
    _compInfoMap[MAV_COMP_ID_AUTOPILOT1][COMP_METADATA_TYPE_ACTUATORS] = new CompInfoActuators(MAV_COMP_ID_AUTOPILOT1, vehicle, this);
}

ComponentInformationManager::~ComponentInformationManager() { _destroying = true; _retireSource(_source); }
Vehicle* ComponentInformationManager::vehicle() { return _vehicle.data(); }
int ComponentInformationManager::stateCount() const { return _cStates; }
const ComponentInformationManager::StateFn* ComponentInformationManager::rgStates() const { return _rgStates; }

bool ComponentInformationManager::_sourceCurrent(const std::shared_ptr<ComponentInformationRequestSource>& source) const
{
    if (QThread::currentThread() != thread()) return false;
    // Copy first: callers may pass the actual member, which a release callback
    // is allowed to replace. A weak control block alone is not a raw identity.
    const auto original = source;
    const QPointer<ComponentInformationManager> self(const_cast<ComponentInformationManager*>(this));
    const auto memoryCurrent = [this, &original]() {
        if (_destroying || !original || _source != original || original->retired || original->serial != _serial
            || !original->vehicle || _vehicle != original->vehicle || !original->links || !original->primary
            || original->vehicle->compInfoManager() != this || original->vehicle->vehicleLinkManager() != original->links
            || !original->vehicle->componentInformationCallbackAllowed()
            || original->vehicle->id() != original->vehicleId || original->vehicle->defaultComponentId() != original->componentId
            || original->vehicle->firmwareType() != original->firmware
            || original->vehicle->autopilotVersionObservationSourceEpochMemory() != original->versionEpoch) return false;
        const auto current = original->links->primaryLink();
        return !current.expired() && !current.owner_before(original->primaryOwner) && !original->primaryOwner.owner_before(current);
    };
    if (!memoryCurrent()) return false;
    auto current = original->links->primaryLink().lock();
    const bool samePointer = current && current.get() == original->primary.data();
    current.reset();
    // No Link virtual/OS probe, fresh-version or initial-connect-complete gate.
    // Releasing a strong reference is nevertheless an external lifetime edge.
    return self && samePointer && memoryCurrent();
}

bool ComponentInformationManager::_resetParameterMetadata(const std::shared_ptr<ComponentInformationRequestSource>& source)
{
    const auto original = source;
    const QPointer<ComponentInformationManager> self(this);
    const auto components = _compInfoMap.values();
    for (const auto& infos : components) {
        if (!self || _source != original) return false;
        const QPointer<CompInfoParam> parameters(qobject_cast<CompInfoParam*>(infos.value(COMP_METADATA_TYPE_PARAMETER).data()));
        if (parameters) parameters->resetMetadata();
        // The reset can release a retired native catalogue. Never continue
        // through QObject destruction into another Manager/source generation.
        if (!self || _source != original) return false;
    }
    return true;
}

void ComponentInformationManager::_retireSource(const std::shared_ptr<ComponentInformationRequestSource>& source)
{
    if (!source || _source != source || source->retired) return;
    const auto original = source;
    const QPointer<ComponentInformationManager> self(this);
    const auto fn = _requestAllCompleteFn;
    void* const data = _requestAllCompleteFnData;
    const QPointer<QObject> owner(_requestAllCompleteOwner);
    const bool ownerRequired = _requestAllCompleteOwnerRequired;
    const bool previousBusy = _entryBusy;
    _entryBusy = true;
    original->retired = true;
    _active = false;
    _requestAllCompleteFn = nullptr; _requestAllCompleteFnData = nullptr;
    _requestAllCompleteOwner.clear(); _requestAllCompleteOwnerRequired = false;
    _requestTypeStateMachine._retire();
    if (!self || _source != original) return;
    if (!_destroying && !_resetParameterMetadata(original)) return;
    if (!self || _source != original) return;
    const auto connections = original->connections;
    original->connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
    if (!self || _source != original) return;
    _entryBusy = previousBusy;
    if (!_destroying && original->vehicle && _vehicle == original->vehicle
        && original->vehicle->compInfoManager() == this && original->vehicle->componentInformationCallbackAllowed()
        && fn && (!ownerRequired || owner)) {
        // Metadata is an optional initial-connect step. Retirement completes
        // that step once; it does NOT report any artifact as verified/applied.
        _entryBusy = false;
        qCDebug(ComponentInformationManagerLog) << "Optional component metadata step retired without adopting a later source";
        if (self && _source == original && !_destroying && original->vehicle && _vehicle == original->vehicle
            && original->vehicle->compInfoManager() == this && original->vehicle->componentInformationCallbackAllowed()
            && (!ownerRequired || owner)) fn(data);
    }
}

float ComponentInformationManager::progress() const { return _active ? _stateIndex / static_cast<float>(_cStates) : 1.f; }

void ComponentInformationManager::advance()
{
    if (QThread::currentThread() != thread()) return;
    const QPointer<ComponentInformationManager> self(this);
    const auto source = _source;
    if (!_sourceCurrent(source)) return;
    const bool previousBusy = _entryBusy;
    _entryBusy = true;
    StateMachine::advance();
    if (!self) return;
    if (_source == source) _entryBusy = previousBusy;
    if (_sourceCurrent(source)) emit progressUpdate(progress());
}

void ComponentInformationManager::requestAllComponentInformation(RequestAllCompleteFn fn, void* data, QObject* callbackOwner)
{
    if (QThread::currentThread() != thread() || _destroying || _entryBusy || _active || !_requestTypeStateMachine._idle() || _serialExhausted) return;
    if (_serial == std::numeric_limits<quint64>::max()) { _serialExhausted = true; return; }
    const QPointer<ComponentInformationManager> self(this);
    const QPointer<Vehicle> callbackVehicle(_vehicle);
    const QPointer<QObject> ownerObject(callbackOwner);
    const bool ownerRequired = callbackOwner != nullptr;
    auto expectedSource = _source;
    _entryBusy = true;
    const auto releaseEntry = qScopeGuard([self, &expectedSource]() {
        if (self && self->_source == expectedSource) self->_entryBusy = false;
    });
    _retireSource(expectedSource);
    if (!self || _source != expectedSource) return;
    const auto rejectEntry = [self, fn, data, ownerObject, ownerRequired, expectedSource, callbackVehicle]() {
        if (!self || self->_source != expectedSource || !callbackVehicle || self->_vehicle != callbackVehicle
            || callbackVehicle->compInfoManager() != self || !callbackVehicle->componentInformationCallbackAllowed()) return;
        self->_entryBusy = false;
        qCDebug(ComponentInformationManagerLog) << "Optional component metadata step ended without an admitted source";
        if (self && self->_source == expectedSource && callbackVehicle && self->_vehicle == callbackVehicle
            && callbackVehicle->compInfoManager() == self && callbackVehicle->componentInformationCallbackAllowed()
            && fn && (!ownerRequired || ownerObject)) fn(data);
    };
    const QPointer<Vehicle> v(_vehicle);
    const QPointer<VehicleLinkManager> links(v ? v->vehicleLinkManager() : nullptr);
    if (!v || !links || v->compInfoManager() != this) { rejectEntry(); return; }
    const auto owner = links->primaryLink();
    auto strong = owner.lock();
    if (!strong) { rejectEntry(); return; }
    const QPointer<LinkInterface> primary(strong.get());
    auto source = std::make_shared<ComponentInformationRequestSource>();
    source->vehicle = v; source->links = links; source->primary = primary; source->primaryOwner = owner;
    source->serial = ++_serial; source->vehicleId = v->id(); source->componentId = v->defaultComponentId();
    source->firmware = v->firmwareType(); source->versionEpoch = v->autopilotVersionObservationSourceEpochMemory();
    _source = source;
    expectedSource = source;
    _requestAllCompleteFn = fn; _requestAllCompleteFnData = data;
    _requestAllCompleteOwner = ownerObject; _requestAllCompleteOwnerRequired = ownerRequired;
    auto retire = [self, source]() { if (self) self->_retireSource(source); };
    // Every callback is pinned to this source; A->B->A is not the old run.
    source->connections.append(connect(links, &VehicleLinkManager::primaryLinkChanged, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(links, &VehicleLinkManager::allLinksRemoved, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(links, &VehicleLinkManager::communicationLostChanged, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(primary, &LinkInterface::connected, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(primary, &LinkInterface::disconnected, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(primary, &QObject::destroyed, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(v, &Vehicle::firmwareTypeChanged, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(v, &Vehicle::autopilotVersionObservationSourceChanged, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    source->connections.append(connect(v, &QObject::destroyed, this, retire));
    if (!self || !_sourceCurrent(source)) return;
    if (ownerRequired) {
        if (!ownerObject) { _retireSource(source); return; }
        source->connections.append(connect(ownerObject, &QObject::destroyed, this, retire));
        if (!self || !_sourceCurrent(source)) return;
    }
    const bool connected = primary->isConnected();
    if (!self || !_sourceCurrent(source)) return;
    const bool replay = primary->isLogReplay();
    if (!self || !_sourceCurrent(source)) return;
    const auto config = primary->linkConfiguration();
    source->requestMessagesAllowed = connected && !replay && !primary->isPX4Flow() && config && !config->isHighLatency();
    strong.reset();
    if (!self) return;
    if (!_sourceCurrent(source)) { _entryBusy = false; return; }
    _stateIndex = -1;
    start();
    if (!self) return;
    if (_source == source) _entryBusy = false;
    if (_sourceCurrent(source)) emit progressUpdate(progress());
}

void ComponentInformationManager::_stateRequestCompInfoGeneral(StateMachine* machine)
{
    const QPointer<ComponentInformationManager> self(static_cast<ComponentInformationManager*>(machine));
    const auto source = self->_source;
    const QPointer<CompInfoGeneral> general(self->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1));
    if (!general || !self->_sourceCurrent(source)) return;
    if (!self->_resetParameterMetadata(source) || !self || !general || !self->_sourceCurrent(source)) return;
    general->resetMetadata();
    if (self && general && self->_sourceCurrent(source)) self->_requestTypeStateMachine.request(general);
}
void ComponentInformationManager::_stateRequestCompInfoGeneralComplete(StateMachine* machine)
{
    const QPointer<ComponentInformationManager> self(static_cast<ComponentInformationManager*>(machine));
    const auto source = self->_source;
    self->_updateAllUri();
    if (self && self->_sourceCurrent(source)) self->advance();
}
void ComponentInformationManager::_updateAllUri()
{
    const QPointer<ComponentInformationManager> self(this);
    const auto source = _source;
    const QPointer<CompInfoGeneral> general(compInfoGeneral(MAV_COMP_ID_AUTOPILOT1));
    const auto infos = _compInfoMap.value(MAV_COMP_ID_AUTOPILOT1).values();
    for (const auto& info : infos) {
        if (!self || !general || !info || !_sourceCurrent(source)) return;
        general->setUris(*info);
        if (!self || !general || !info || !_sourceCurrent(source)) return;
    }
}
void ComponentInformationManager::_stateRequestCompInfoComplete() { advance(); }
void ComponentInformationManager::_stateRequestCompInfoParam(StateMachine* machine)
{
    auto* self = static_cast<ComponentInformationManager*>(machine);
    if (self->_isCompTypeSupported(COMP_METADATA_TYPE_PARAMETER)) self->_requestTypeStateMachine.request(self->_compInfoMap.value(MAV_COMP_ID_AUTOPILOT1).value(COMP_METADATA_TYPE_PARAMETER));
    else self->advance();
}
void ComponentInformationManager::_stateRequestCompInfoEvents(StateMachine* machine)
{
    auto* self = static_cast<ComponentInformationManager*>(machine);
    if (self->_isCompTypeSupported(COMP_METADATA_TYPE_EVENTS)) self->_requestTypeStateMachine.request(self->_compInfoMap.value(MAV_COMP_ID_AUTOPILOT1).value(COMP_METADATA_TYPE_EVENTS));
    else self->advance();
}
void ComponentInformationManager::_stateRequestCompInfoActuators(StateMachine* machine)
{
    auto* self = static_cast<ComponentInformationManager*>(machine);
    if (self->_isCompTypeSupported(COMP_METADATA_TYPE_ACTUATORS)) self->_requestTypeStateMachine.request(self->_compInfoMap.value(MAV_COMP_ID_AUTOPILOT1).value(COMP_METADATA_TYPE_ACTUATORS));
    else self->advance();
}
void ComponentInformationManager::_stateRequestAllCompInfoComplete(StateMachine* machine)
{
    auto* self = static_cast<ComponentInformationManager*>(machine);
    if (!self->_sourceCurrent(self->_source)) return;
    const auto fn = self->_requestAllCompleteFn;
    void* const data = self->_requestAllCompleteFnData;
    const QPointer<QObject> owner(self->_requestAllCompleteOwner);
    const bool ownerRequired = self->_requestAllCompleteOwnerRequired;
    const QPointer<Vehicle> v(self->_vehicle);
    self->_active = false;
    // Clear BEFORE the external callback, never after it can destroy/reenter.
    self->_requestAllCompleteFn = nullptr; self->_requestAllCompleteFnData = nullptr;
    self->_requestAllCompleteOwner.clear(); self->_requestAllCompleteOwnerRequired = false;
    self->_entryBusy = false;
    if (v && v->compInfoManager() == self && v->componentInformationCallbackAllowed() && fn && (!ownerRequired || owner)) fn(data);
}
bool ComponentInformationManager::_isCompTypeSupported(COMP_METADATA_TYPE type)
{
    const QPointer<CompInfoGeneral> general(compInfoGeneral(MAV_COMP_ID_AUTOPILOT1));
    return general && general->isMetaDataTypeSupported(type);
}
CompInfoParam* ComponentInformationManager::compInfoParam(uint8_t compId)
{
    const QPointer<ComponentInformationManager> self(this);
    const QPointer<Vehicle> v(_vehicle);
    if (!_compInfoMap.contains(compId)) {
        const QPointer<CompInfoParam> info(new CompInfoParam(compId, v, this));
        if (!self || !v || !info) return nullptr;
        _compInfoMap[compId][COMP_METADATA_TYPE_PARAMETER] = info;
    }
    return qobject_cast<CompInfoParam*>(_compInfoMap.value(compId).value(COMP_METADATA_TYPE_PARAMETER).data());
}
CompInfoGeneral* ComponentInformationManager::compInfoGeneral(uint8_t compId)
{
    return qobject_cast<CompInfoGeneral*>(_compInfoMap.value(compId).value(COMP_METADATA_TYPE_GENERAL).data());
}
QString ComponentInformationManager::_getFileCacheTag(int type, uint32_t crc, bool translation)
{
    return QStringLiteral("raw-v1_") + QString::asprintf("%08x_%02i_%i", crc, type, static_cast<int>(translation));
}

RequestMetaDataTypeStateMachine::RequestMetaDataTypeStateMachine(ComponentInformationManager* manager) : _compMgr(manager) {}
RequestMetaDataTypeStateMachine::~RequestMetaDataTypeStateMachine() { _retire(); }
CompInfo* RequestMetaDataTypeStateMachine::compInfo() { return _compInfo.data(); }
int RequestMetaDataTypeStateMachine::stateCount() const { return _cStates; }
const RequestMetaDataTypeStateMachine::StateFn* RequestMetaDataTypeStateMachine::rgStates() const { return _rgStates; }
bool RequestMetaDataTypeStateMachine::_current() const
{
    return QThread::currentThread() == thread() && _active && !_serialExhausted && _compMgr && _compInfo
        && _compInfo->vehicle == _compMgr->vehicle() && _compMgr->_sourceCurrent(_source);
}
bool RequestMetaDataTypeStateMachine::_idle() const { return !_active && !_transportBusy && !_translationBusy && (!_file || !_file->worker); }
void RequestMetaDataTypeStateMachine::_retire()
{
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    const auto request = _file;
    const auto translationRequest = _translationRequest;
    _active = false;
    if (_messageOwner) { _messageOwner->deleteLater(); _messageOwner.clear(); }
    if (request && request->worker) request->worker->requestCancel();
    // New metadata-only transports accept cancellation for exactly the original
    // native request. During admission, defer until its real return is known.
    // The terminal listener still drains only this request, not a later job.
    if (request && _transportBusy && !request->downloadStarting) {
        const QPointer<FTPManager> ftp(request->ftp);
        const QPointer<QGCCachedFileDownload> http(request->http);
        if (ftp) ftp->cancelUnique(request->id);
        if (!self) return;
        if (_file == request && http) http->cancelUnique(request->id);
    }
    if (!self) return;
    if (translationRequest && _translationBusy && _translationRequest == translationRequest
        && !translationRequest->starting && translationRequest->producer) {
        translationRequest->producer->cancelArtifacts(translationRequest->id,
            translationRequest->summaryInput, translationRequest->metadataInput);
    }
}
void RequestMetaDataTypeStateMachine::request(CompInfo* info)
{
    if (QThread::currentThread() != thread() || !_idle() || !_compMgr || !_compMgr->_sourceCurrent(_compMgr->_source) || !info || _serialExhausted) return;
    if (_serial == std::numeric_limits<quint64>::max()) { _serialExhausted = true; _compMgr->_retireSource(_compMgr->_source); return; }
    ++_serial;
    _compInfo = info; _source = _compMgr->_source; _stateIndex = -1;
    _file.reset(); _translationRequest.reset(); _metadataArtifact.reset(); _translationArtifact.reset();
    _jsonMetadataFileName.clear(); _jsonTranslationFileName.clear(); _jsonMetadataTranslatedFileName.clear();
    start();
}
void RequestMetaDataTypeStateMachine::advance()
{
    if (!_current()) return;
    if (_messageOwner) { _messageOwner->deleteLater(); _messageOwner.clear(); }
    StateMachine::advance();
}
void RequestMetaDataTypeStateMachine::statesCompleted() const
{
    if (_compMgr && _compInfo && _compMgr->_sourceCurrent(_source)) _compMgr->_stateRequestCompInfoComplete();
}
QString RequestMetaDataTypeStateMachine::typeToString()
{
    if (!_compInfo) return QStringLiteral("Unknown");
    switch (_compInfo->type) {
    case COMP_METADATA_TYPE_GENERAL: return QStringLiteral("COMP_METADATA_TYPE_GENERAL");
    case COMP_METADATA_TYPE_PARAMETER: return QStringLiteral("COMP_METADATA_TYPE_PARAMETER");
    case COMP_METADATA_TYPE_COMMANDS: return QStringLiteral("COMP_METADATA_TYPE_COMMANDS");
    case COMP_METADATA_TYPE_PERIPHERALS: return QStringLiteral("COMP_METADATA_TYPE_PERIPHERALS");
    case COMP_METADATA_TYPE_EVENTS: return QStringLiteral("COMP_METADATA_TYPE_EVENTS");
    case COMP_METADATA_TYPE_ACTUATORS: return QStringLiteral("COMP_METADATA_TYPE_ACTUATORS");
    default: return QStringLiteral("Unknown");
    }
}

void RequestMetaDataTypeStateMachine::_requestMessage(bool deprecated)
{
    if (!_current()) return;
    if (_compInfo->type != COMP_METADATA_TYPE_GENERAL || (deprecated && _compInfo->crcMetaDataValid()) || !_source->requestMessagesAllowed) { advance(); return; }
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    const QPointer<Vehicle> v(_source->vehicle);
    const QPointer<ComponentMetadataMessageContext> context(new ComponentMetadataMessageContext(this, this, _compInfo, _serial, currentState()));
    if (!self || !_current() || !context) return;
    _messageOwner = context;
    const auto handler = [](void* data, MAV_RESULT result, Vehicle::RequestMessageResultHandlerFailureCode_t, const mavlink_message_t& message) {
        auto* context = static_cast<ComponentMetadataMessageContext*>(data);
        const QPointer<RequestMetaDataTypeStateMachine> self(context->machine);
        const QPointer<CompInfo> info(context->info);
        const auto serial = context->serial;
        const auto state = context->state;
        if (!self || !info || !self->_current() || self->_messageOwner != context || self->_serial != serial || self->currentState() != state) return;
        if (result == MAV_RESULT_ACCEPTED) {
            const auto setUri = [info](const char* bytes, int size, quint32 crc) {
                const QByteArray raw(bytes, size);
                const int terminator = raw.indexOf('\0');
                if (terminator < 0) info->setUriMetaData(QString(), 0);
                else info->setUriMetaData(QString::fromUtf8(raw.constData(), terminator), crc);
            };
            if (message.msgid == MAVLINK_MSG_ID_COMPONENT_METADATA) {
                mavlink_component_metadata_t value; mavlink_msg_component_metadata_decode(&message, &value);
                setUri(value.uri, sizeof(value.uri), value.file_crc);
            } else if (message.msgid == MAVLINK_MSG_ID_COMPONENT_INFORMATION) {
                mavlink_component_information_t value; mavlink_msg_component_information_decode(&message, &value);
                setUri(value.general_metadata_uri, sizeof(value.general_metadata_uri), value.general_metadata_file_crc);
            }
        }
        if (self && info && self->_current() && self->_messageOwner == context && self->_serial == serial && self->currentState() == state) self->advance();
    };
    v->requestMessage(handler, context.data(), MAV_COMP_ID_AUTOPILOT1,
        deprecated ? MAVLINK_MSG_ID_COMPONENT_INFORMATION : MAVLINK_MSG_ID_COMPONENT_METADATA, 0, 0, 0, 0, 0, context.data());
}
void RequestMetaDataTypeStateMachine::_stateRequestCompInfo(StateMachine* machine) { static_cast<RequestMetaDataTypeStateMachine*>(machine)->_requestMessage(false); }
void RequestMetaDataTypeStateMachine::_stateRequestCompInfoDeprecated(StateMachine* machine) { static_cast<RequestMetaDataTypeStateMachine*>(machine)->_requestMessage(true); }
bool RequestMetaDataTypeStateMachine::_fileCurrent(const std::shared_ptr<ComponentMetadataFileRequest>& request) const
{
    return _current() && request && _file == request && request->source == _source && request->serial == _serial && request->stateIndex == _stateIndex;
}
void RequestMetaDataTypeStateMachine::_requestFile(const QString& tag, bool crcValid, quint32 expectedCrc, const QString& uri, bool translation)
{
    const QString originalUri = uri;
    const QString originalTag = tag;
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    if (!_current()) return;
    if (originalUri.isEmpty()) { advance(); return; }
    bool uriValid = originalUri.size() <= 2048;
    for (const QChar ch : originalUri) if (ch.unicode() < 0x20 || ch.unicode() == 0x7f) uriValid = false;
    QString path;
    if (_uriIsMAVLinkFTP(originalUri)) {
        // MFTP has a legacy [;comp=N] prefix, not an RFC URL authority. Its
        // actual parser treats the remaining text as a remote file path.
        path = originalUri.mid(QStringLiteral("mftp://").size());
        uriValid = uriValid && !path.isEmpty() && !path.contains(QStringLiteral("://"))
            && !path.contains(QLatin1Char('?')) && !path.contains(QLatin1Char('#'));
    } else {
        const QUrl url(originalUri, QUrl::StrictMode);
        // QGCFileDownload only dispatches these exact lower-case prefixes to
        // HTTP. Do not let a case-insensitive URL check reach its local path.
        uriValid = uriValid && (originalUri.startsWith(QStringLiteral("http://")) || originalUri.startsWith(QStringLiteral("https://")))
            && url.isValid() && !url.isRelative() && !url.host().isEmpty() && url.userInfo().isEmpty();
        path = url.path();
    }
    if (!uriValid) {
        qCWarning(ComponentInformationManagerLog) << "Unsupported component metadata URI";
        if (self && self->_current()) self->advance();
        return;
    }
    auto request = std::make_shared<ComponentMetadataFileRequest>();
    request->source = _source; request->serial = _serial; request->stateIndex = _stateIndex;
    request->cacheTag = originalTag; request->uri = originalUri; request->hasExpectedCrc = crcValid;
    request->expectedCrc = expectedCrc; request->translation = translation;
    request->compressed = path.endsWith(QStringLiteral(".xz"), Qt::CaseInsensitive) || path.endsWith(QStringLiteral(".lzma"), Qt::CaseInsensitive);
    request->allowDecodedCrc = !translation && _compInfo->type == COMP_METADATA_TYPE_GENERAL && _source->firmware == MAV_AUTOPILOT_PX4;
    _file = request;
    const QString cached = crcValid ? _compMgr->fileCache().access(originalTag) : QString();
    if (!self || !_fileCurrent(request)) return;
    if (cached.isEmpty()) _downloadFile(request); else _startArtifact(request, cached, true);
}
void RequestMetaDataTypeStateMachine::_downloadFile(const std::shared_ptr<ComponentMetadataFileRequest>& request)
{
    if (!_fileCurrent(request) || _transportBusy) return;
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    QString createError;
    const auto output = QGCOwnedDownloadFile::create(createError);
    if (!self || !_fileCurrent(request)) return;
    if (!output) {
        qCWarning(ComponentInformationManagerLog) << "Metadata input creation failed" << createError;
        if (self && _fileCurrent(request)) advance();
        return;
    }
    // Each admission gets a fresh native file. Never adopt a cache filename or
    // reuse a remote basename, even across a one-time invalidation retry.
    request->downloadOutput = output;
    request->ftp.clear(); request->http.clear();
    _transportBusy = true;
    request->downloadStarting = true;
    request->downloadCompletionSeen = false;
    bool accepted = false;
    QPointer<FTPManager> originalFtp;
    QPointer<QGCCachedFileDownload> originalHttp;
    if (_uriIsMAVLinkFTP(request->uri)) {
        const QPointer<FTPManager> ftp(_source->vehicle->ftpManager());
        if (!ftp) { request->downloadStarting = false; _downloadComplete(request, QString(), QStringLiteral("ftp-unavailable")); return; }
        request->ftp = ftp;
        originalFtp = ftp;
        request->downloadConnection = connect(ftp, &FTPManager::uniqueDownloadComplete, this, [self, request, ftp, output](const QString& id, const QString& file, const QString& error) {
            if (self && ftp && request->ftp == ftp && id == request->id && file == output->path())
                self->_downloadComplete(request, file, error);
        });
        if (!self) return;
        if (request->downloadConnection && _fileCurrent(request)) accepted = ftp->downloadUnique(MAV_COMP_ID_AUTOPILOT1, request->uri, request->id, output);
    } else {
        const QPointer<QGCCachedFileDownload> downloader(_compMgr->_cachedFileDownload);
        if (!downloader) { request->downloadStarting = false; _downloadComplete(request, QString(), QStringLiteral("http-unavailable")); return; }
        request->http = downloader;
        originalHttp = downloader;
        request->downloadConnection = connect(downloader, &QGCCachedFileDownload::uniqueDownloadComplete, this,
            [self, request, downloader, output](const QString& id, const QString& file, const QString& error) {
                if (self && downloader && request->http == downloader && id == request->id && file == output->path())
                    self->_downloadComplete(request, file, error);
            });
        if (!self) return;
        if (request->downloadConnection && _fileCurrent(request)) accepted = downloader->downloadUnique(request->id, request->uri, output,
            request->hasExpectedCrc ? 0 : ComponentInformationManager::cachedFileMaxAgeSec);
    }
    if (!self || _file != request) {
        // Retirement/destruction may have happened while admission was still
        // starting, before that provider actually installed its native job.
        // This local-only tail never reads this or cancels a later request.
        if (accepted && originalFtp) originalFtp->cancelUnique(request->id);
        if (accepted && originalHttp) originalHttp->cancelUnique(request->id);
        return;
    }
    request->downloadStarting = false;
    // A direct completion cannot be adopted before download() has actually
    // accepted this operation. On retirement, drain only this exact listener.
    if (!accepted) _downloadComplete(request, QString(), QStringLiteral("download-not-started"));
    else if (request->downloadCompletionSeen) _downloadComplete(request, request->downloadFile, request->downloadError);
    else if (!_fileCurrent(request)) {
        const QPointer<FTPManager> ftp(request->ftp);
        const QPointer<QGCCachedFileDownload> http(request->http);
        if (ftp) ftp->cancelUnique(request->id);
        if (!self || _file != request) return;
        if (http) http->cancelUnique(request->id);
    }
}
void RequestMetaDataTypeStateMachine::_downloadComplete(const std::shared_ptr<ComponentMetadataFileRequest>& request, const QString& file, const QString& error)
{
    if (_file != request || !_transportBusy) return;
    if (request->downloadStarting) {
        if (!request->downloadCompletionSeen) {
            request->downloadCompletionSeen = true; request->downloadFile = file; request->downloadError = error;
        }
        return;
    }
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    const auto connection = request->downloadConnection; request->downloadConnection = {};
    _transportBusy = false;
    disconnect(connection);
    if (!self || !_fileCurrent(request)) return;
    if (!error.isEmpty() || file.isEmpty() || !request->downloadOutput || file != request->downloadOutput->path()) { advance(); return; }
    _startArtifact(request, file, false);
}
void RequestMetaDataTypeStateMachine::_startArtifact(const std::shared_ptr<ComponentMetadataFileRequest>& request, const QString& file, bool cacheHit)
{
    if (!_fileCurrent(request) || request->worker) return;
    // A file phase has at most its first job and one post-invalidation job.
    if (request->workerGeneration >= 2) { _compMgr->_retireSource(request->source); return; }
    const quint64 generation = ++request->workerGeneration;
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    const QPointer<ComponentMetadataArtifactWorker> worker(new ComponentMetadataArtifactWorker(request->id, file, request->compressed,
        request->hasExpectedCrc, request->expectedCrc, request->allowDecodedCrc, !cacheHit && request->hasExpectedCrc,
        cacheHit ? QGCOwnedDownloadFilePtr() : request->downloadOutput));
    const QObject* const workerIdentity = worker.data();
    bool admitted = false;
    // An unadmitted parentless worker owns no running thread. Never start a
    // cancelled worker merely to obtain QThread::finished for its cleanup.
    const auto unadmitted = qScopeGuard([worker, &admitted]() { if (!admitted && worker) worker->deleteLater(); });
    if (!self || !worker || self->_file != request || request->workerGeneration != generation) return;
    request->worker = worker; request->cacheHit = cacheHit;
    request->workerIdentity = workerIdentity;
    request->workerScheduling = true;
    request->workerNotStarted = false; request->workerNotStartedReason.clear();
    request->resultSeen = false; request->finishedSeen = false; request->workerOk = false; request->workerError.clear();
    const auto notStarted = [self, request, generation, workerIdentity](const QString& reason) {
        if (!self || self->_file != request || request->workerGeneration != generation || request->workerIdentity != workerIdentity) return;
        request->workerNotStarted = true;
        request->workerNotStartedReason = reason;
        if (request->workerScheduling) return; // schedule() acceptance has not returned yet.
        request->worker.clear(); request->workerIdentity = nullptr;
        // Queue overload/cancellation is NOT a failed CRC inspection. End
        // only this optional batch; do not invalidate a good cache or enter
        // a fallback URI/re-download because no worker ever inspected it.
        if (!self->_fileCurrent(request)) return;
        qCWarning(ComponentInformationManagerLog) << "Metadata artifact never started" << reason;
        if (self && self->_file == request && request->workerGeneration == generation && self->_fileCurrent(request) && self->_compMgr)
            self->_compMgr->_retireSource(request->source);
    };
    const auto releaseUnadmitted = qScopeGuard([self, request, generation, workerIdentity, notStarted, &admitted]() {
        if (!admitted && self && self->_file == request && request->workerGeneration == generation && request->workerIdentity == workerIdentity) {
            request->workerScheduling = false;
            notStarted(QStringLiteral("metadata-artifact-not-admitted"));
        }
    });
    // Parentless worker must outlive its run even when Manager is deleted.
    connect(worker, &ComponentMetadataArtifactWorker::resultReady, this, [self, request, worker, generation](const QString& id, bool ok, const QString& error) {
        if (!self || !worker || request->worker != worker || request->workerGeneration != generation || id != request->id || request->resultSeen) return;
        request->resultSeen = true; request->workerOk = ok; request->workerError = error;
        self->_finishArtifact(request);
    });
    if (!self || !worker || self->_file != request || request->workerIdentity != workerIdentity) return;
    connect(worker, &QThread::finished, this, [self, request, worker, generation]() {
        if (!self || !worker || request->worker != worker || request->workerGeneration != generation) return;
        request->finishedSeen = true;
        self->_finishArtifact(request);
    });
    if (!self || !worker || self->_file != request || request->workerIdentity != workerIdentity) return;
    connect(worker, &ComponentMetadataArtifactWorker::neverStarted, this,
        [request, notStarted](const QString& id, const QString& reason) { if (id == request->id) notStarted(reason); });
    if (!self || !worker || self->_file != request || request->workerIdentity != workerIdentity) return;
    connect(worker, &QObject::destroyed, this, [self, request, generation, workerIdentity](QObject* original) {
        if (original != workerIdentity || !self || self->_file != request || request->workerGeneration != generation
            || request->workerIdentity != workerIdentity || request->workerNotStarted || (request->resultSeen && request->finishedSeen)) return;
        // Broken/missing terminal delivery is not a cache-validation result.
        // End only this optional step; never infer an artifact or retry cache.
        if (self->_compMgr) self->_compMgr->_retireSource(request->source);
    });
    if (!self || !worker || self->_file != request || request->workerIdentity != workerIdentity) return;
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    if (!self || !worker || !self->_fileCurrent(request) || request->workerIdentity != workerIdentity) return;
    admitted = worker->schedule();
    if (!self || self->_file != request || request->workerGeneration != generation || request->workerIdentity != workerIdentity) {
        if (admitted && worker) worker->requestCancel();
        return;
    }
    request->workerScheduling = false;
    if (!admitted) notStarted(QStringLiteral("metadata-artifact-not-admitted"));
    else if (request->workerNotStarted) notStarted(request->workerNotStartedReason);
    else if (!self->_fileCurrent(request) && worker) worker->requestCancel();
}
void RequestMetaDataTypeStateMachine::_finishArtifact(const std::shared_ptr<ComponentMetadataFileRequest>& request)
{
    if (_file != request || request->workerNotStarted || !request->resultSeen || !request->finishedSeen || !request->worker) return;
    const QPointer<RequestMetaDataTypeStateMachine> self(this);
    const QPointer<ComponentMetadataArtifactWorker> worker(request->worker);
    // Neither delivery alone can consume the job, invalidate cache or advance.
    const auto artifact = request->workerOk ? worker->artifact() : ComponentMetadataArtifactPtr();
    request->workerScheduling = false;
    request->worker.clear();
    request->workerIdentity = nullptr;
    if (!_fileCurrent(request)) return;
    if (!artifact) {
        qCWarning(ComponentInformationManagerLog) << "Metadata artifact rejected" << request->workerError;
        if (!self || !_fileCurrent(request)) return;
        if (request->cacheHit && !request->invalidationAttempted) {
            request->invalidationAttempted = true;
            const bool removed = _compMgr->fileCache().invalidate(request->cacheTag);
            if (!self || !_fileCurrent(request)) return;
            if (removed) { _downloadFile(request); return; }
        }
        advance(); return;
    }
    if (!request->cacheHit && request->hasExpectedCrc && !artifact->cacheCopyPath.isEmpty()) {
        // Only the THIRD disposable raw candidate may move or be deleted.
        // Never use insert's returned old cache path as this run's JSON.
        _compMgr->fileCache().insert(request->cacheTag, artifact->cacheCopyPath);
        if (!self || !_fileCurrent(request)) return;
    }
    qCDebug(ComponentInformationManagerLog) << "Metadata artifact CRC domain" << artifact->crcByteDomain
        << "raw SHA256" << artifact->rawSha256 << "JSON SHA256" << artifact->jsonSha256;
    if (!self || !_fileCurrent(request)) return;
    if (request->translation) { _translationArtifact = artifact; _jsonTranslationFileName = artifact->jsonSnapshotPath; }
    else { _metadataArtifact = artifact; _jsonMetadataFileName = artifact->jsonSnapshotPath; }
    advance();
}
void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJson(StateMachine* machine)
{
    auto* self = static_cast<RequestMetaDataTypeStateMachine*>(machine);
    if (!self->_current()) return;
    const auto info = self->_compInfo;
    self->_requestFile(ComponentInformationManager::_getFileCacheTag(info->type, info->crcMetaData(), false), info->crcMetaDataValid(), info->crcMetaData(), info->uriMetaData(), false);
}
void RequestMetaDataTypeStateMachine::_stateRequestMetaDataJsonFallback(StateMachine* machine)
{
    auto* self = static_cast<RequestMetaDataTypeStateMachine*>(machine);
    if (!self->_current()) return;
    if (!self->_jsonMetadataFileName.isEmpty()) { self->advance(); return; }
    const auto info = self->_compInfo;
    self->_requestFile(ComponentInformationManager::_getFileCacheTag(info->type, info->crcMetaDataFallback(), false), info->crcMetaDataFallbackValid(), info->crcMetaDataFallback(), info->uriMetaDataFallback(), false);
}
void RequestMetaDataTypeStateMachine::_stateRequestTranslationJson(StateMachine* machine)
{
    auto* self = static_cast<RequestMetaDataTypeStateMachine*>(machine);
    if (self->_current()) self->_requestFile(QString(), false, 0, self->_compInfo->uriTranslation(), true);
}
void RequestMetaDataTypeStateMachine::_stateRequestTranslate(StateMachine* machine)
{
    const QPointer<RequestMetaDataTypeStateMachine> self(static_cast<RequestMetaDataTypeStateMachine*>(machine));
    if (!self->_current()) return;
    if (self->_jsonTranslationFileName.isEmpty() || self->_jsonMetadataFileName.isEmpty()) { self->advance(); return; }
    const QPointer<ComponentInformationTranslation> translation(self->_compMgr->translation());
    if (!translation) { self->advance(); return; }
    auto request = std::make_shared<ComponentMetadataTranslationRequest>();
    request->source = self->_source; request->serial = self->_serial; request->stateIndex = self->_stateIndex;
    request->metadataInput = self->_metadataArtifact;
    request->summaryInput = self->_translationArtifact;
    request->producer = translation;
    self->_translationRequest = request;
    self->_translationBusy = true;
    const auto complete = [self, request](const QString& file, const QString& error) {
        if (!self || !self->_translationBusy || self->_translationRequest != request) return;
        // Freeze cleanup even for a synchronous completion buffered until the
        // acceptance return, including a later false/retirement/destruction.
        if (!file.isEmpty() && request->outputToRemove.isEmpty()) request->outputToRemove = file;
        if (request->starting) {
            if (!request->completionSeen) { request->completionSeen = true; request->file = file; request->error = error; }
            return;
        }
        // The producer creates this unique temporary path. Its frozen plain
        // owner cleans it even if disconnect/current checks destroy Manager.
        self->_translationBusy = false;
        const auto connection = self->_translationConnection; self->_translationConnection = {};
        disconnect(connection);
        if (!self) return;
        if (!self->_current() || self->_translationRequest != request || self->_source != request->source
            || self->_serial != request->serial || self->_stateIndex != request->stateIndex) {
            if (self && self->_translationRequest == request) self->_translationRequest.reset();
            return;
        }
        // Legacy display transformation is NOT the original CRC artifact.
        if (error.isEmpty()) self->_jsonMetadataTranslatedFileName = file;
        else qCWarning(ComponentInformationManagerLog) << "Metadata translation failed" << error;
        if (self && self->_translationRequest == request && self->_source == request->source
            && self->_serial == request->serial && self->_stateIndex == request->stateIndex && self->_current()) self->advance();
    };
    self->_translationConnection = connect(translation, &ComponentInformationTranslation::artifactTranslationComplete, self,
        [self, request, translation, complete](const QString& id, const QString& file, const QString& error) {
            if (self && translation && request->producer == translation && id == request->id) complete(file, error);
        });
    if (!self) return;
    // Keep both immutable input artifacts alive through a synchronous
    // translation callback, even if that callback retires/deletes Manager.
    const auto summaryOwner = self->_translationArtifact;
    const auto metadataOwner = self->_metadataArtifact;
    bool accepted = false;
    if (self->_translationConnection && self->_current())
        accepted = translation->downloadAndTranslateArtifacts(request->id, summaryOwner, metadataOwner,
            ComponentInformationManager::cachedFileMaxAgeSec);
    if (!self || self->_translationRequest != request) {
        if (accepted && translation) translation->cancelArtifacts(request->id, summaryOwner, metadataOwner);
        return;
    }
    request->starting = false;
    if (!accepted) complete(QString(), QStringLiteral("translation-not-started"));
    else if (request->completionSeen) complete(request->file, request->error);
    else if (!self->_current() && translation)
        translation->cancelArtifacts(request->id, summaryOwner, metadataOwner);
}
void RequestMetaDataTypeStateMachine::_stateRequestComplete(StateMachine* machine)
{
    const QPointer<RequestMetaDataTypeStateMachine> self(static_cast<RequestMetaDataTypeStateMachine*>(machine));
    if (!self->_current()) return;
    const auto source = self->_source; const auto serial = self->_serial; const auto info = self->_compInfo;
    // Hold immutable originals across the virtual parser and every signal.
    const auto metadata = self->_metadataArtifact; const auto translations = self->_translationArtifact;
    const auto translatedOwner = self->_translationRequest;
    const QString translated = self->_jsonMetadataTranslatedFileName;
    const QString input = translated.isEmpty() ? self->_jsonMetadataFileName : translated;
    info->setJson(input);
    if (!self || !info || !self->_current() || self->_source != source || self->_serial != serial) return;
    self->_translationRequest.reset();
    self->_metadataArtifact.reset(); self->_translationArtifact.reset();
    self->_jsonMetadataFileName.clear(); self->_jsonTranslationFileName.clear(); self->_jsonMetadataTranslatedFileName.clear();
    self->advance();
}
bool RequestMetaDataTypeStateMachine::_uriIsMAVLinkFTP(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}

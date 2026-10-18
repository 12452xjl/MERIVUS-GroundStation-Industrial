/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCCameraControl.h"
#include "QGCCameraIO.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "QGCMapEngine.h"
#include "QGCCameraManager.h"
#include "FTPManager.h"
#include "QGCLZMA.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "MAVLinkProtocol.h"
#include "QGCToolbox.h"

#include <QDir>
#include <QStandardPaths>
#include <QDomDocument>
#include <QDomNodeList>
#include <QScopeGuard>
#include <QThread>
#include <cstring>
#include <limits>

namespace {
QString boundedCameraText(const char* bytes, int capacity)
{
    int length = 0;
    while (length < capacity && bytes[length] != '\0') ++length;
    return QString::fromUtf8(bytes, length);
}

bool sameStreamInformation(const mavlink_video_stream_information_t& left, const mavlink_video_stream_information_t& right)
{
    // Compare fields, not C-struct padding. Status samples have their own cache.
    return left.stream_id == right.stream_id && left.count == right.count && left.type == right.type
        && left.flags == right.flags && std::memcmp(&left.framerate, &right.framerate, sizeof(left.framerate)) == 0
        && left.resolution_h == right.resolution_h && left.resolution_v == right.resolution_v
        && left.bitrate == right.bitrate && left.rotation == right.rotation && left.hfov == right.hfov
        && std::memcmp(left.name, right.name, sizeof(left.name)) == 0
        && std::memcmp(left.uri, right.uri, sizeof(left.uri)) == 0;
}
}

QGC_LOGGING_CATEGORY(CameraControlLog, "CameraControlLog")
QGC_LOGGING_CATEGORY(CameraControlVerboseLog, "CameraControlVerboseLog")

static const char* kCondition       = "condition";
static const char* kControl         = "control";
static const char* kDefault         = "default";
static const char* kDefnition       = "definition";
static const char* kDescription     = "description";
static const char* kExclusion       = "exclude";
static const char* kExclusions      = "exclusions";
static const char* kLocale          = "locale";
static const char* kLocalization    = "localization";
static const char* kMax             = "max";
static const char* kMin             = "min";
static const char* kModel           = "model";
static const char* kName            = "name";
static const char* kOption          = "option";
static const char* kOptions         = "options";
static const char* kOriginal        = "original";
static const char* kParameter       = "parameter";
static const char* kParameterrange  = "parameterrange";
static const char* kParameterranges = "parameterranges";
static const char* kParameters      = "parameters";
static const char* kReadOnly        = "readonly";
static const char* kWriteOnly       = "writeonly";
static const char* kRoption         = "roption";
static const char* kStep            = "step";
static const char* kDecimalPlaces   = "decimalPlaces";
static const char* kStrings         = "strings";
static const char* kTranslated      = "translated";
static const char* kType            = "type";
static const char* kUnit            = "unit";
static const char* kUpdate          = "update";
static const char* kUpdates         = "updates";
static const char* kValue           = "value";
static const char* kVendor          = "vendor";
static const char* kVersion         = "version";

static const char* kPhotoMode       = "PhotoMode";
static const char* kPhotoLapse      = "PhotoLapse";
static const char* kPhotoLapseCount = "PhotoLapseCount";
static const char* kThermalOpacity  = "ThermalOpacity";
static const char* kThermalMode     = "ThermalMode";

//-----------------------------------------------------------------------------
// Known Parameters
const char* QGCCameraControl::kCAM_EV          = "CAM_EV";
const char* QGCCameraControl::kCAM_EXPMODE     = "CAM_EXPMODE";
const char* QGCCameraControl::kCAM_ISO         = "CAM_ISO";
const char* QGCCameraControl::kCAM_SHUTTERSPD  = "CAM_SHUTTERSPD";
const char* QGCCameraControl::kCAM_APERTURE    = "CAM_APERTURE";
const char* QGCCameraControl::kCAM_WBMODE      = "CAM_WBMODE";
const char* QGCCameraControl::kCAM_MODE        = "CAM_MODE";

//-----------------------------------------------------------------------------
QGCCameraOptionExclusion::QGCCameraOptionExclusion(QObject* parent, QString param_, QString value_, QStringList exclusions_)
    : QObject(parent)
    , param(param_)
    , value(value_)
    , exclusions(exclusions_)
{
}

//-----------------------------------------------------------------------------
QGCCameraOptionRange::QGCCameraOptionRange(QObject* parent, QString param_, QString value_, QString targetParam_, QString condition_, QStringList optNames_, QStringList optValues_)
    : QObject(parent)
    , param(param_)
    , value(value_)
    , targetParam(targetParam_)
    , condition(condition_)
    , optNames(optNames_)
    , optValues(optValues_)
{
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, bool& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue() != "0";
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, int& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue().toInt();
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_attribute(QDomNode& node, const char* tagName, QString& target)
{
    QDomNamedNodeMap attrs = node.attributes();
    if(!attrs.count()) {
        return false;
    }
    QDomNode subNode = attrs.namedItem(tagName);
    if(subNode.isNull()) {
        return false;
    }
    target = subNode.nodeValue();
    return true;
}

//-----------------------------------------------------------------------------
static bool
read_value(QDomNode& element, const char* tagName, QString& target)
{
    QDomElement de = element.firstChildElement(tagName);
    if(de.isNull()) {
        return false;
    }
    target = de.text();
    return true;
}

//-----------------------------------------------------------------------------
QGCCameraControl::QGCCameraControl(const mavlink_camera_information_t *info, Vehicle* vehicle, int compID, QObject* parent)
    : FactGroup(0, parent, true /* ignore camel case */)
    , _vehicle(vehicle)
    , _compID(compID)
{
    QPointer<QGCCameraControl> self(this);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    if (!self) return;
    QQmlEngine::setObjectOwnership(&_streamTombstone, QQmlEngine::CppOwnership);
    if (!self) return;
    if (info) memcpy(&_info, info, sizeof(mavlink_camera_information_t));
    _streamManager = qobject_cast<QGCCameraManager*>(parent);
    _streamManagerRequired = !_streamManager.isNull();
    if (_vehicle) {
        // Match the actual live child before pinning the legacy raw provider slot.
        const auto children = _vehicle->children();
        for (QObject* child : children) {
            if (child == _vehicle->vehicleLinkManager()) {
                _streamLinks = qobject_cast<VehicleLinkManager*>(child);
                break;
            }
        }
    }
    _streamLinksRequired = _vehicle && _vehicle->vehicleLinkManager();
    if (qgcApp()) _streamToolbox = qgcApp()->toolbox();
    if (_streamToolbox) {
        const auto tools = _streamToolbox->children();
        for (QObject* tool : tools) {
            if (tool == _streamToolbox->mavlinkProtocol()) _streamProtocol = qobject_cast<MAVLinkProtocol*>(tool);
            if (tool == _streamToolbox->videoManager()) _streamVideoManager = qobject_cast<VideoManager*>(tool);
        }
    }
    _streamProtocolRequired = _streamToolbox && _streamToolbox->mavlinkProtocol();
    _advanceStreamEpoch();
    const auto sourceChanged = [this]() { _invalidateStreamSource(); };
    if (_vehicle) {
        connect(_vehicle.data(), &QObject::destroyed, this, sourceChanged);
        if (!self) return;
        connect(_vehicle.data(), &Vehicle::cameraManagerChanged, this, sourceChanged);
        if (!self) return;
        connect(_vehicle.data(), &Vehicle::firmwareTypeChanged, this, sourceChanged);
        if (!self) return;
        connect(_vehicle.data(), &Vehicle::vehicleTypeChanged, this, sourceChanged);
        if (!self) return;
    }
    if (_streamManager) {
        connect(_streamManager.data(), &QObject::destroyed, this, sourceChanged);
        if (!self) return;
        // Manager intent is compared through its own non-wrapping epoch.
        // Do not mutate our epoch during its normal currentCameraChanged
        // notification: that would self-cancel the manager's stream tail.
    }
    if (_streamLinks) {
        connect(_streamLinks.data(), &QObject::destroyed, this, sourceChanged);
        if (!self) return;
        connect(_streamLinks.data(), &VehicleLinkManager::primaryLinkChanged, this, sourceChanged);
        if (!self) return;
        connect(_streamLinks.data(), &VehicleLinkManager::communicationLostChanged, this, sourceChanged);
        if (!self) return;
        connect(_streamLinks.data(), &VehicleLinkManager::allLinksRemoved, this, sourceChanged);
        if (!self) return;
    }
    if (_streamProtocol) {
        connect(_streamProtocol.data(), &QObject::destroyed, this, sourceChanged);
        if (!self) return;
        connect(_streamProtocol.data(), &MAVLinkProtocol::systemIdChanged, this, sourceChanged);
        if (!self) return;
    }
    connect(&_streams, &QmlObjectListModel::countChanged, this, [this]() { _advanceStreamEpoch(); });
    connect(&_streamInfoTimer, &QTimer::timeout, this, &QGCCameraControl::_streamTimeout);
    _streamInfoTimer.setSingleShot(false);
    connect(&_streamStatusTimer, &QTimer::timeout, this, &QGCCameraControl::_streamStatusTimeout);
    _streamStatusTimer.setSingleShot(true);
    connect(this, &QGCCameraControl::dataReady, this, &QGCCameraControl::_dataReady);
    if (!self) return;
    _vendor = boundedCameraText(reinterpret_cast<const char*>(_info.vendor_name), sizeof(_info.vendor_name));
    _modelName = boundedCameraText(reinterpret_cast<const char*>(_info.model_name), sizeof(_info.model_name));
    int ver = static_cast<int>(_info.cam_definition_version);
    _cacheFile = QString::asprintf("%s/%s_%s_%03d.xml",
        qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str(),
        _vendor.toStdString().c_str(),
        _modelName.toStdString().c_str(),
        ver);
    QSettings settings;
    _photoMode       = static_cast<PhotoMode>(settings.value(kPhotoMode, static_cast<int>(PHOTO_CAPTURE_SINGLE)).toInt());
    _photoLapse      = settings.value(kPhotoLapse, 1.0).toDouble();
    _photoLapseCount = settings.value(kPhotoLapseCount, 0).toInt();
    _thermalOpacity  = settings.value(kThermalOpacity, 85.0).toDouble();
    _thermalMode     = static_cast<ThermalViewMode>(settings.value(kThermalMode, static_cast<uint32_t>(THERMAL_BLEND)).toUInt());
    _recTimer.setSingleShot(false);
    _recTimer.setInterval(333);
    connect(&_recTimer, &QTimer::timeout, this, &QGCCameraControl::_recTimerHandler);
    if (!self) return;
    // Initialization can synchronously notify/delete this controller. It is last.
    const QString definition = boundedCameraText(_info.cam_definition_uri, sizeof(_info.cam_definition_uri));
    if (!definition.isEmpty()) _handleDefinitionFile(definition);
    else _initWhenReady();
}

//-----------------------------------------------------------------------------
QGCCameraControl::~QGCCameraControl()
{
    _streamDestroying = true;
    _streamInfoTimer.stop();
    _streamStatusTimer.stop();
    QNetworkAccessManager* manager = _netManager;
    _netManager = nullptr;
    delete manager;
}

bool QGCCameraControl::_advanceStreamEpoch()
{
    if (_streamDestroying || _streamExhausted) return false;
    if (_streamEpoch == std::numeric_limits<quint64>::max()) {
        _streamExhausted = true;
        _streamEpoch = 0;
        _streamInfoTimer.stop();
        _streamStatusTimer.stop();
        return false;
    }
    ++_streamEpoch;
    return true;
}

bool QGCCameraControl::_advanceStreamStatusEpoch()
{
    if (_streamDestroying || _streamExhausted) return false;
    if (_streamStatusEpoch == std::numeric_limits<quint64>::max()) {
        _streamExhausted = true;
        _streamEpoch = 0;
        _streamInfoTimer.stop();
        _streamStatusTimer.stop();
        return false;
    }
    ++_streamStatusEpoch;
    return true;
}

bool QGCCameraControl::_advanceCameraModeEpoch()
{
    if (_streamDestroying || _cameraModeEpochExhausted) return false;
    if (_cameraModeEpoch == std::numeric_limits<quint64>::max()) {
        _cameraModeEpochExhausted = true;
        _cameraModeEpoch = 0;
        return false;
    }
    ++_cameraModeEpoch;
    return true;
}

QPointer<Fact> QGCCameraControl::_ownedCameraModeFact() const
{
    // FactGroup's native map is a raw-address map. Do not getFact()/re-pin an
    // address which may have been removed by an earlier notification.
    Fact* const address = _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr);
    if (!address) return {};
    const auto actualChildren = children();
    for (QObject* actual : actualChildren)
        if (actual == address) return qobject_cast<Fact*>(actual);
    return {};
}

void QGCCameraControl::_requestCameraMode(CameraMode requestedMode)
{
    if (_resetting || !_advanceCameraModeEpoch()) return;
    const quint64 modeEpoch = _cameraModeEpoch;
    const QPointer<QGCCameraControl> self(this);
    const bool parametersComplete = _paramComplete;
    Fact* const nativeAddress = _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr);
    const auto parameter = _ownedCameraModeFact();
    const auto operation = _streamOperation();
    const auto current = [&]() {
        const auto localCurrent = [&]() {
            return self && !_resetting && !_cameraModeEpochExhausted && _cameraModeEpoch == modeEpoch
                && _paramComplete == parametersComplete
                && _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr) == nativeAddress
                && (!nativeAddress || (parameter && parameter->parent() == self.data()));
        };
        if (!localCurrent()) return false;
        if (!_streamOperationCurrent(operation)) return false;
        // Releasing the Link inside the source check is itself a boundary.
        return localCurrent();
    };
    if (!current()) return;
    const bool supported = hasModes();
    if (!current() || !supported) return;
    // A stale native map is not the basic-camera/no-parameter fallback.
    if (nativeAddress && !parameter) return;
    Fact* const returned = mode();
    if (!current() || _paramComplete != parametersComplete
        || _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr) != nativeAddress
        || (nativeAddress && !parameter)) return;
    // Custom foreign/newly-created Fact returns have no pre-call lifetime pin.
    if (returned && (!parameter || returned != parameter.data())) return;
    const bool useParameter = returned != nullptr;
    const auto previousMode = cameraMode();
    if (!current() || _paramComplete != parametersComplete
        || _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr) != nativeAddress
        || (nativeAddress && !parameter)) return;
    if (previousMode == requestedMode) return; // the explicit intent still revoked an older caller
    if (useParameter) {
        if (!parameter || parameter->parent() != this) return;
        parameter->setRawValue(requestedMode);
        if (!current() || !parameter || parameter->parent() != this || _paramComplete != parametersComplete
            || _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr) != nativeAddress
            || parameter->rawValue() != QVariant(static_cast<int>(requestedMode))) return;
    } else {
        operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_SET_CAMERA_MODE, true, 0, requestedMode);
        if (!current() || _paramComplete != parametersComplete
            || _nameToFactMap.value(QString::fromLatin1(kCAM_MODE), nullptr) != nativeAddress
            || (nativeAddress && !parameter)) return;
    }
    _setCameraMode(requestedMode); // terminal virtual call; no old member continuation
}

void QGCCameraControl::_invalidateStreamSource()
{
    _advanceStreamEpoch();
    _advanceStreamStatusEpoch();
    _streamInfoTimer.stop();
    _streamStatusTimer.stop();
    _streamStatusTarget.clear();
    _streamStatusTargetId = -1;
}

QGCCameraControl::StreamOperation QGCCameraControl::_streamOperation() const
{
    StreamOperation operation;
    operation.vehicle = _vehicle;
    operation.manager = _streamManager;
    operation.links = _streamLinks;
    operation.protocol = _streamProtocol;
    operation.parent = parent();
    operation.parentRequired = parent() != nullptr;
    operation.epoch = _streamEpoch;
    operation.managerEpoch = _streamManager ? _streamManager->cameraSelectionEpoch() : 0;
    operation.selection = _currentStream;
    operation.componentId = _compID;
    operation.vehicleId = _vehicle ? _vehicle->id() : 0;
    if (_streamLinks) {
        operation.primaryOwner = _streamLinks->primaryLink();
        const auto primary = operation.primaryOwner.lock();
        operation.primary = primary.get();
        operation.hadPrimary = bool(primary);
    }
    return operation;
}

bool QGCCameraControl::_streamOperationCurrent(const StreamOperation& operation) const
{
    QPointer<const QGCCameraControl> self(this);
    const auto memoryCurrent = [&]() {
        if (!self || _streamDestroying || _streamExhausted || QThread::currentThread() != thread()
            || !operation.epoch || operation.epoch != _streamEpoch
            || operation.selection != _currentStream || operation.componentId != _compID
            || !operation.vehicle || operation.vehicle != _vehicle || operation.vehicle->id() != operation.vehicleId
            || operation.parent != parent() || (operation.parentRequired && !operation.parent)
            || operation.manager != _streamManager
            || (_streamManagerRequired && (!operation.manager || operation.manager.data() != parent()))
            || (operation.manager && (!operation.managerEpoch || operation.manager->cameraSelectionEpoch() != operation.managerEpoch))
            || operation.links != _streamLinks || (_streamLinksRequired && !operation.links)
            || operation.protocol != _streamProtocol || (_streamProtocolRequired && !operation.protocol)
            || (operation.hadPrimary && !operation.primary)) return false;
        if (operation.links) {
            if (operation.vehicle->vehicleLinkManager() != operation.links.data()) return false;
            const auto owner = operation.links->primaryLink();
            if (owner.owner_before(operation.primaryOwner) || operation.primaryOwner.owner_before(owner)) return false;
        }
        return true;
    };
    if (!memoryCurrent()) return false;
    if (operation.links) {
        if (operation.vehicle->vehicleLinkManager() != operation.links.data()) return false;
        const auto currentOwner = operation.links->primaryLink();
        if (currentOwner.owner_before(operation.primaryOwner) || operation.primaryOwner.owner_before(currentOwner)) return false;
        auto current = currentOwner.lock();
        const bool matches = current.get() == operation.primary.data();
        current.reset();
        // Releasing the last strong Link reference may synchronously change
        // source/intent or destroy providers. This tail is memory-only: no
        // recursive lock or virtual source probe can start another cycle.
        if (!self || !matches || !memoryCurrent()) return false;
    }
    return true;
}

QPointer<QGCVideoStreamInfo> QGCCameraControl::_ownedStream(QObject* candidate) const
{
    if (!candidate) return {};
    const auto children = this->children();
    for (QObject* child : children) {
        if (child == candidate) return qobject_cast<QGCVideoStreamInfo*>(child);
    }
    return {};
}

QPointer<QGCVideoStreamInfo> QGCCameraControl::_streamAtSelection(int selection)
{
    if (selection < 0) return {};
    int ordinal = 0;
    for (int index = 0; index < _streams.count(); ++index) {
        const auto stream = _ownedStream(_streams[index]);
        if (stream && !stream->isThermal()) {
            if (ordinal == selection) return stream;
            ++ordinal;
        }
    }
    return {};
}

int QGCCameraControl::_streamSelectionIndex(const QPointer<QGCVideoStreamInfo>& selected)
{
    if (!selected) return -1;
    int ordinal = 0;
    for (int index = 0; index < _streams.count(); ++index) {
        const auto stream = _ownedStream(_streams[index]);
        if (stream && !stream->isThermal()) {
            if (stream == selected) return ordinal;
            ++ordinal;
        }
    }
    return -1;
}

void QGCCameraControl::_streamDestroyed(QObject* original)
{
    if (_streamDestroying || !original) return;
    // Do not dereference or reconstruct a QObject pin from the dying address.
    // A live, owner-local tombstone makes even TextRole safe immediately. The
    // structural removal is deferred until the enclosing model insert ends.
    auto* const rows = _streams.objectList();
    bool replaced = false;
    for (int index = 0; index < rows->size(); ++index) {
        if (rows->at(index) == original) { (*rows)[index] = &_streamTombstone; replaced = true; }
    }
    _invalidateStreamSource();
    if (replaced) {
        const int previousSelection = _currentStream;
        const int survivingSelection = _streamSelectionIndex(_selectedStreamIdentity);
        _currentStream = survivingSelection >= 0 ? survivingSelection : 0;
        _streamSelectionRetired = _streamSelectionRetired || survivingSelection < 0 || _currentStream != previousSelection;
        _selectedStreamIdentity = _streamAtSelection(_currentStream);
        _streamRowsRetired = true;
        _queueRetiredStreamCleanup();
    }
}

void QGCCameraControl::_queueRetiredStreamCleanup()
{
    if (_streamDestroying || !_streamRowsRetired || _streamStructuralBusy || _streamCleanupQueued) return;
    _streamCleanupQueued = true;
    const QPointer<QGCCameraControl> self(this);
    QTimer::singleShot(0, this, [self]() { if (self) self->_cleanupRetiredStreams(); });
}

void QGCCameraControl::_cleanupRetiredStreams()
{
    _streamCleanupQueued = false;
    if (_streamDestroying || !_streamRowsRetired || _streamStructuralBusy) return;
    const QPointer<QGCCameraControl> self(this);
    auto operation = _streamOperation();
    if (!self) return;
    _streamStructuralBusy = true;
    const auto release = qScopeGuard([self]() {
        if (self) { self->_streamStructuralBusy = false; self->_queueRetiredStreamCleanup(); }
    });
    _streamRowsRetired = false;
    const auto selected = _selectedStreamIdentity;
    const bool retiredSelection = _streamSelectionRetired;
    _streamSelectionRetired = false;
    QStringList labels;
    for (int index = 0; index < _streams.count(); ++index) {
        const auto stream = _ownedStream(_streams[index]);
        if (stream && !stream->isThermal()) labels.append(stream->name());
    }
    const bool labelsChanged = labels != _streamLabels;
    const int previousSelection = _currentStream;
    _streamLabels = labels;
    _currentStream = _streamSelectionIndex(selected);
    if (_currentStream < 0) _currentStream = 0;
    _selectedStreamIdentity = _streamAtSelection(_currentStream);
    operation.selection = _currentStream;
    quint64 expectedEpoch = _streamEpoch;
    bool originalIntent = !_streamExhausted;
    const int maximumRemovals = _streams.count();
    for (int remaining = maximumRemovals; remaining > 0; --remaining) {
        const int index = _streams.indexOf(&_streamTombstone);
        if (index < 0) break;
        _streams.removeAt(index);
        if (!self) return;
        if (expectedEpoch == std::numeric_limits<quint64>::max()) originalIntent = false;
        else ++expectedEpoch; // exactly the countChanged owned by this removal
        if (_streamExhausted || _streamEpoch != expectedEpoch) originalIntent = false;
    }
    if (!originalIntent || _streamEpoch != expectedEpoch) return;
    operation.epoch = expectedEpoch;
    if (!_streamOperationCurrent(operation)) return;
    emit streamsChanged();
    if (!self || !_streamOperationCurrent(operation)) return;
    if (labelsChanged) {
        emit streamLabelsChanged();
        if (!self || !_streamOperationCurrent(operation)) return;
    }
    if (retiredSelection || _currentStream != previousSelection) {
        emit currentStreamChanged();
        if (!self || !_streamOperationCurrent(operation)) return;
    }
    emit thermalStreamChanged();
    if (!self || !_streamOperationCurrent(operation)) return;
    emit autoStreamChanged();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto manager = operation.manager;
    if (manager) emit manager->streamChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_initWhenReady()
{
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    qCDebug(CameraControlLog) << "_initWhenReady()";
    const bool basic = isBasic();
    if (!self || !_streamOperationCurrent(operation)) return;
    // Install common observers before a request/readiness signal can reenter.
    connect(operation.vehicle.data(), &Vehicle::mavCommandResult, this, &QGCCameraControl::_mavCommandResult, Qt::UniqueConnection);
    if (!self || !_streamOperationCurrent(operation)) return;
    connect(&_captureStatusTimer, &QTimer::timeout, this, &QGCCameraControl::_requestCaptureStatus, Qt::UniqueConnection);
    if (!self || !_streamOperationCurrent(operation)) return;
    _captureStatusTimer.setSingleShot(true);
    QTimer::singleShot(2500, this, &QGCCameraControl::_requestStorageInfo);
    _captureStatusTimer.start(2750);
    if(basic) {
        qCDebug(CameraControlLog) << "Basic, MAVLink only messages.";
        _requestCameraSettings();
        if (!self || !_streamOperationCurrent(operation)) return;
        QTimer::singleShot(250, this, &QGCCameraControl::_checkForVideoStreams);
        //-- Basic cameras have no parameters
        _paramComplete = true;
        emit parametersReady();
        if (!self || !_streamOperationCurrent(operation)) return;
    } else {
        _requestAllParameters();
        if (!self || !_streamOperationCurrent(operation)) return;
        //-- Give some time to load the parameters before going after the camera settings
        QTimer::singleShot(2000, this, &QGCCameraControl::_requestCameraSettings);
    }
    emit infoChanged();
    if (!self || !_streamOperationCurrent(operation)) return;
    QNetworkAccessManager* manager = _netManager;
    _netManager = nullptr;
    delete manager;
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::firmwareVersion()
{
    int major = (_info.firmware_version >> 24) & 0xFF;
    int minor = (_info.firmware_version >> 16) & 0xFF;
    int build = _info.firmware_version & 0xFFFF;
    return QString::asprintf("%d.%d.%d", major, minor, build);
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

//-----------------------------------------------------------------------------
QGCCameraControl::VideoStatus
QGCCameraControl::videoStatus()
{
    return _video_status;
}

//-----------------------------------------------------------------------------
QGCCameraControl::PhotoStatus
QGCCameraControl::photoStatus()
{
    return _photo_status;
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::storageFreeStr()
{
    return QGCMapEngine::storageFreeSizeToString(static_cast<quint64>(_storageFree));
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::batteryRemainingStr()
{
    if(_batteryRemaining >= 0) {
        return QGCMapEngine::numberToString(static_cast<quint64>(_batteryRemaining)) + " %";
    }
    return "";
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setCameraMode(CameraMode mode)
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "setCameraMode(" << mode << ")";
        if(mode == CAM_MODE_VIDEO) {
            setVideoMode();
        } else if(mode == CAM_MODE_PHOTO) {
            setPhotoMode();
        } else {
            qCDebug(CameraControlLog) << "setCameraMode() Invalid mode:" << mode;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoMode(PhotoMode mode)
{
    if(!_resetting) {
        _photoMode = mode;
        QSettings settings;
        settings.setValue(kPhotoMode, static_cast<int>(mode));
        emit photoModeChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoLapse(qreal interval)
{
    _photoLapse = interval;
    QSettings settings;
    settings.setValue(kPhotoLapse, interval);
    emit photoLapseChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoLapseCount(int count)
{
    _photoLapseCount = count;
    QSettings settings;
    settings.setValue(kPhotoLapseCount, count);
    emit photoLapseCountChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setCameraMode(CameraMode mode)
{
    if(_cameraMode != mode) {
        if (!_advanceCameraModeEpoch()) return;
        const quint64 modeEpoch = _cameraModeEpoch;
        QPointer<QGCCameraControl> self(this);
        const auto operation = _streamOperation();
        if (!self || !_streamOperationCurrent(operation) || _cameraModeEpochExhausted || modeEpoch != _cameraModeEpoch) return;
        if (!_advanceStreamStatusEpoch()) return;
        const quint64 statusEpoch = _streamStatusEpoch;
        _cameraMode = mode;
        emit cameraModeChanged();
        if (!self || !_streamOperationCurrent(operation) || _cameraModeEpochExhausted || modeEpoch != _cameraModeEpoch
            || _cameraMode != mode || statusEpoch != _streamStatusEpoch) return;
        //-- Update stream status
        const auto raw = currentStreamInstance();
        if (!self || !_streamOperationCurrent(operation) || _cameraModeEpochExhausted || modeEpoch != _cameraModeEpoch
            || _cameraMode != mode || statusEpoch != _streamStatusEpoch) return;
        const auto stream = _ownedStream(raw);
        if (!stream) return;
        _streamStatusOperation = operation;
        _streamStatusTarget = stream;
        _streamStatusTargetId = stream->streamID();
        _streamStatusTimer.start(1000);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::toggleMode()
{
    if (_resetting || !_advanceCameraModeEpoch()) return;
    const quint64 modeEpoch = _cameraModeEpoch;
    const QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation) || _cameraModeEpoch != modeEpoch || _cameraModeEpochExhausted) return;
    const auto mode = cameraMode();
    if (!self || !_streamOperationCurrent(operation) || _cameraModeEpoch != modeEpoch || _cameraModeEpochExhausted || _resetting) return;
    if (mode == CAM_MODE_PHOTO || mode == CAM_MODE_SURVEY) setVideoMode();
    else if (mode == CAM_MODE_VIDEO) setPhotoMode();
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::toggleVideo()
{
    if(!_resetting) {
        if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            return stopVideo();
        } else {
            return startVideo();
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::takePhoto()
{
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return false;
    qCDebug(CameraControlLog) << "takePhoto()";
    //-- Check if camera can capture photos or if it can capture it while in Video Mode
    const bool photos = capturesPhotos();
    if (!self || !_streamOperationCurrent(operation)) return false;
    if(!photos) {
        qCWarning(CameraControlLog) << "Camera does not handle image capture";
        return false;
    }
    const auto mode = cameraMode();
    if (!self || !_streamOperationCurrent(operation)) return false;
    const bool videoPhotos = mode != CAM_MODE_VIDEO || photosInVideoMode();
    if (!self || !_streamOperationCurrent(operation)) return false;
    if(!videoPhotos) {
        qCWarning(CameraControlLog) << "Camera does not handle image capture while in video mode";
        return false;
    }
    const auto status = photoStatus();
    if (!self || !_streamOperationCurrent(operation)) return false;
    if(status != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }
    if(!_resetting) {
        if(photos) {
            const PhotoMode photoMode = _photoMode;
            const qreal lapse = _photoLapse;
            const int count = _photoLapseCount;
            const auto toolbox = _streamToolbox;
            const auto video = _streamVideoManager;
            operation.vehicle->sendMavCommand(
                operation.componentId,                                                     // Target component
                MAV_CMD_IMAGE_START_CAPTURE,                                                // Command id
                false,                                                                      // ShowError
                0,                                                                          // Reserved (Set to 0)
                static_cast<float>(photoMode == PHOTO_CAPTURE_SINGLE ? 0 : lapse),
                photoMode == PHOTO_CAPTURE_SINGLE ? 1 : count);
            // Preserve the existing submitted result; do not continue old state
            // writes or rediscover a provider after the command boundary.
            if (!self || !_streamOperationCurrent(operation) || _photoMode != photoMode
                || _photoLapse != lapse || _photoLapseCount != count) return true;
            _setPhotoStatus(PHOTO_CAPTURE_IN_PROGRESS);
            if (!self || !_streamOperationCurrent(operation) || _photoMode != photoMode
                || _photoLapse != lapse || _photoLapseCount != count) return true;
            _captureInfoRetries = 0;
            //-- Capture local image as well
            if (toolbox && video && toolbox == _streamToolbox && video == _streamVideoManager
                && toolbox->videoManager() == video.data()) video->grabImage();
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::stopTakePhoto()
{
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return false;
    if(!_resetting) {
        qCDebug(CameraControlLog) << "stopTakePhoto()";
        const auto status = photoStatus();
        if (!self || !_streamOperationCurrent(operation)) return false;
        if(status == PHOTO_CAPTURE_IDLE || (status != PHOTO_CAPTURE_INTERVAL_IDLE && status != PHOTO_CAPTURE_INTERVAL_IN_PROGRESS)) {
            return false;
        }
        const bool photos = capturesPhotos();
        if (!self || !_streamOperationCurrent(operation)) return false;
        if(photos) {
            operation.vehicle->sendMavCommand(
                operation.componentId,                                     // Target component
                MAV_CMD_IMAGE_STOP_CAPTURE,                                 // Command id
                false,                                                      // ShowError
                0);                                                         // Reserved (Set to 0)
            if (!self || !_streamOperationCurrent(operation)) return true;
            _setPhotoStatus(PHOTO_CAPTURE_IDLE);
            if (!self || !_streamOperationCurrent(operation)) return true;
            _captureInfoRetries = 0;
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::startVideo()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "startVideo()";
        //-- Check if camera can capture videos or if it can capture it while in Photo Mode
        if(!capturesVideo() || (cameraMode() == CAM_MODE_PHOTO && !videoInPhotoMode())) {
            return false;
        }
        if(videoStatus() != VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(
                _compID,                                    // Target component
                MAV_CMD_VIDEO_START_CAPTURE,                // Command id
                false,                                      // Don't Show Error (handle locally)
                0,                                          // Reserved (Set to 0)
                0);                                         // CAMERA_CAPTURE_STATUS Frequency
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::stopVideo()
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "stopVideo()";
        if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
            _vehicle->sendMavCommand(
                _compID,                                    // Target component
                MAV_CMD_VIDEO_STOP_CAPTURE,                 // Command id
                false,                                      // Don't Show Error (handle locally)
                0);                                         // Reserved (Set to 0)
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setVideoMode()
{
    _requestCameraMode(CAM_MODE_VIDEO);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setPhotoMode()
{
    _requestCameraMode(CAM_MODE_PHOTO);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setThermalMode(ThermalViewMode mode)
{
    QSettings settings;
    settings.setValue(kThermalMode, static_cast<uint32_t>(mode));
    _thermalMode = mode;
    emit thermalModeChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setThermalOpacity(double val)
{
    if(val < 0.0) val = 0.0;
    if(val > 100.0) val = 100.0;
    if(fabs(_thermalOpacity - val) > 0.1) {
        _thermalOpacity = val;
        QSettings settings;
        settings.setValue(kThermalOpacity, val);
        emit thermalOpacityChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setZoomLevel(qreal level)
{
    qCDebug(CameraControlLog) << "setZoomLevel()" << level;
    if(hasZoom()) {
        //-- Limit
        level = std::min(std::max(level, 0.0), 100.0);
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_SET_CAMERA_ZOOM,                // Command id
                false,                                  // ShowError
                ZOOM_TYPE_RANGE,                        // Zoom type
                static_cast<float>(level));             // Level
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setFocusLevel(qreal level)
{
    qCDebug(CameraControlLog) << "setFocusLevel()" << level;
    if(hasFocus()) {
        //-- Limit
        level = std::min(std::max(level, 0.0), 100.0);
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_SET_CAMERA_FOCUS,               // Command id
                false,                                  // ShowError
                FOCUS_TYPE_RANGE,                       // Focus type
                static_cast<float>(level));             // Level
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::resetSettings()
{
    if(!_resetting) {
        _advanceCameraModeEpoch(); // reset is a newer mode intent, even if the final mode value returns to the old value
        qCDebug(CameraControlLog) << "resetSettings()";
        _resetting = true;
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_RESET_CAMERA_SETTINGS,          // Command id
            true,                                   // ShowError
            1);                                     // Do Reset
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::formatCard(int id)
{
    if(!_resetting) {
        qCDebug(CameraControlLog) << "formatCard()";
        if(_vehicle) {
            _vehicle->sendMavCommand(
                _compID,                                // Target component
                MAV_CMD_STORAGE_FORMAT,                 // Command id
                true,                                   // ShowError
                id,                                     // Storage ID (1 for first, 2 for second, etc.)
                1);                                     // Do Format
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stepZoom(int direction)
{
    qCDebug(CameraControlLog) << "stepZoom()" << direction;
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_STEP,                         // Zoom type
            direction);                             // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::startZoom(int direction)
{
    qCDebug(CameraControlLog) << "startZoom()" << direction;
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_CONTINUOUS,                   // Zoom type
            direction);                             // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stopZoom()
{
    qCDebug(CameraControlLog) << "stopZoom()";
    if(_vehicle && hasZoom()) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_SET_CAMERA_ZOOM,                // Command id
            false,                                  // ShowError
            ZOOM_TYPE_CONTINUOUS,                   // Zoom type
            0);                                     // Direction (-1 wide, 1 tele)
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestCaptureStatus()
{
    qCDebug(CameraControlLog) << "_requestCaptureStatus()";
    _vehicle->sendMavCommand(
        _compID,                                // target component
        MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS,  // command id
        false,                                  // showError
        1);                                     // Do Request
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::factChanged(Fact* pFact)
{
    _updateActiveList();
    _updateRanges(pFact);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    //-- Is this ours?
    if(_vehicle->id() != vehicleId || compID() != component) {
        return;
    }
    if(!noReponseFromVehicle && result == MAV_RESULT_IN_PROGRESS) {
        //-- Do Nothing
        qCDebug(CameraControlLog) << "In progress response for" << command;
    }else if(!noReponseFromVehicle && result == MAV_RESULT_ACCEPTED) {
        switch(command) {
            case MAV_CMD_RESET_CAMERA_SETTINGS:
                _resetting = false;
                if(isBasic()) {
                    _requestCameraSettings();
                } else {
                    QTimer::singleShot(500, this, &QGCCameraControl::_requestAllParameters);
                    QTimer::singleShot(2500, this, &QGCCameraControl::_requestCameraSettings);
                }
                break;
            case MAV_CMD_VIDEO_START_CAPTURE:
                _setVideoStatus(VIDEO_CAPTURE_STATUS_RUNNING);
                _captureStatusTimer.start(1000);
                break;
            case MAV_CMD_VIDEO_STOP_CAPTURE:
                _setVideoStatus(VIDEO_CAPTURE_STATUS_STOPPED);
                _captureStatusTimer.start(1000);
                break;
            case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                _captureInfoRetries = 0;
                break;
            case MAV_CMD_REQUEST_STORAGE_INFORMATION:
                _storageInfoRetries = 0;
                break;
            case MAV_CMD_IMAGE_START_CAPTURE:
                _captureStatusTimer.start(1000);
                break;
        }
    } else {
        if(noReponseFromVehicle || result == MAV_RESULT_TEMPORARILY_REJECTED || result == MAV_RESULT_FAILED) {
            if(noReponseFromVehicle) {
                qCDebug(CameraControlLog) << "No response for" << command;
            } else if (result == MAV_RESULT_TEMPORARILY_REJECTED) {
                qCDebug(CameraControlLog) << "Command temporarily rejected for" << command;
            } else {
                qCDebug(CameraControlLog) << "Command failed for" << command;
            }
            switch(command) {
                case MAV_CMD_RESET_CAMERA_SETTINGS:
                    _resetting = false;
                    qCDebug(CameraControlLog) << "Failed to reset camera settings";
                break;
                case MAV_CMD_IMAGE_START_CAPTURE:
                case MAV_CMD_IMAGE_STOP_CAPTURE:
                    if(++_captureInfoRetries < 3) {
                        _captureStatusTimer.start(1000);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up start/stop image capture";
                        _setPhotoStatus(PHOTO_CAPTURE_IDLE);
                    }
                    break;
                case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
                    if(++_captureInfoRetries < 3) {
                        _captureStatusTimer.start(500);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up requesting capture status";
                    }
                    break;
                case MAV_CMD_REQUEST_STORAGE_INFORMATION:
                    if(++_storageInfoRetries < 3) {
                        QTimer::singleShot(500, this, &QGCCameraControl::_requestStorageInfo);
                    } else {
                        qCDebug(CameraControlLog) << "Giving up requesting storage status";
                    }
                    break;
            }
        } else {
            qCDebug(CameraControlLog) << "Bad response for" << command << result;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setVideoStatus(VideoStatus status)
{
    if(_video_status != status) {
        _video_status = status;
        emit videoStatusChanged();
        if(status == VIDEO_CAPTURE_STATUS_RUNNING) {
             _recordTime = 0;
             _recTime = QTime::currentTime();
             _recTimer.start();
        } else {
             _recTimer.stop();
             _recordTime = 0;
             emit recordTimeChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_recTimerHandler()
{
    _recordTime = static_cast<uint32_t>(_recTime.msecsTo(QTime::currentTime()));
    emit recordTimeChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_setPhotoStatus(PhotoStatus status)
{
    if(_photo_status != status) {
        qCDebug(CameraControlLog) << "Set Photo Status:" << status;
        _photo_status = status;
        emit photoStatusChanged();
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadCameraDefinitionFile(QByteArray& bytes)
{
    QByteArray originalData(bytes);
    //-- Handle localization
    if(!_handleLocalization(bytes)) {
        return false;
    }
    int errorLine;
    QString errorMsg;
    QDomDocument doc;
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCCritical(CameraControlLog) << "Unable to parse camera definition file on line:" << errorLine;
        qCCritical(CameraControlLog) << errorMsg;
        return false;
    }
    //-- Load camera constants
    QDomNodeList defElements = doc.elementsByTagName(kDefnition);
    if(!defElements.size() || !_loadConstants(defElements)) {
        qCWarning(CameraControlLog) <<  "Unable to load camera constants from camera definition";
        return false;
    }
    //-- Load camera parameters
    QDomNodeList paramElements = doc.elementsByTagName(kParameters);
    if(!paramElements.size()) {
        qCDebug(CameraControlLog) <<  "No parameters to load from camera";
        return false;
    }
    if(!_loadSettings(paramElements)) {
        qCWarning(CameraControlLog) <<  "Unable to load camera parameters from camera definition";
        return false;
    }
    //-- If this is new, cache it
    if(!_cached) {
        qCDebug(CameraControlLog) << "Saving camera definition file" << _cacheFile;
        QFile file(_cacheFile);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << QString("Could not save cache file %1. Error: %2").arg(_cacheFile).arg(file.errorString());
        } else {
            file.write(originalData);
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadConstants(const QDomNodeList nodeList)
{
    QDomNode node = nodeList.item(0);
    if(!read_attribute(node, kVersion, _version)) {
        return false;
    }
    if(!read_value(node, kModel, _modelName)) {
        return false;
    }
    if(!read_value(node, kVendor, _vendor)) {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadSettings(const QDomNodeList nodeList)
{
    QDomNode node = nodeList.item(0);
    QDomElement elem = node.toElement();
    QDomNodeList parameters = elem.elementsByTagName(kParameter);
    //-- Pre-process settings (maintain order and skip non-controls)
    for(int i = 0; i < parameters.size(); i++) {
        QDomNode parameterNode = parameters.item(i);
        QString name;
        if(read_attribute(parameterNode, kName, name)) {
            bool control = true;
            read_attribute(parameterNode, kControl, control);
            if(control) {
                _settings << name;
            }
        } else {
            qCritical() << "Parameter entry missing parameter name";
            return false;
        }
    }
    //-- Load parameters
    for(int i = 0; i < parameters.size(); i++) {
        QDomNode parameterNode = parameters.item(i);
        QString factName;
        read_attribute(parameterNode, kName, factName);
        QString type;
        if(!read_attribute(parameterNode, kType, type)) {
            qCritical() << QString("Parameter %1 missing parameter type").arg(factName);
            return false;
        }
        //-- Does it have a control?
        bool control = true;
        read_attribute(parameterNode, kControl, control);
        //-- Is it read only?
        bool readOnly = false;
        read_attribute(parameterNode, kReadOnly, readOnly);
        //-- Is it write only?
        bool writeOnly = false;
        read_attribute(parameterNode, kWriteOnly, writeOnly);
        //-- It can't be both
        if(readOnly && writeOnly) {
            qCritical() << QString("Parameter %1 cannot be both read only and write only").arg(factName);
        }
        //-- Param type
        bool unknownType;
        FactMetaData::ValueType_t factType = FactMetaData::stringToType(type, unknownType);
        if (unknownType) {
            qCritical() << QString("Unknown type for parameter %1").arg(factName);
            return false;
        }
        //-- By definition, custom types do not have control
        if(factType == FactMetaData::valueTypeCustom) {
            control = false;
        }
        //-- Description
        QString description;
        if(!read_value(parameterNode, kDescription, description)) {
            qCritical() << QString("Parameter %1 missing parameter description").arg(factName);
            return false;
        }
        //-- Check for updates
        QStringList updates = _loadUpdates(parameterNode);
        if(updates.size()) {
            qCDebug(CameraControlVerboseLog) << "Parameter" << factName << "requires updates for:" << updates;
            _requestUpdates[factName] = updates;
        }
        //-- Build metadata
        FactMetaData* metaData = new FactMetaData(factType, factName, this);
        QQmlEngine::setObjectOwnership(metaData, QQmlEngine::CppOwnership);
        metaData->setShortDescription(description);
        metaData->setLongDescription(description);
        metaData->setHasControl(control);
        metaData->setReadOnly(readOnly);
        metaData->setWriteOnly(writeOnly);
        //-- Options (enums)
        QDomElement optionElem = parameterNode.toElement();
        QDomNodeList optionsRoot = optionElem.elementsByTagName(kOptions);
        if(optionsRoot.size()) {
            //-- Iterate options
            QDomNode node = optionsRoot.item(0);
            QDomElement elem = node.toElement();
            QDomNodeList options = elem.elementsByTagName(kOption);
            for(int i = 0; i < options.size(); i++) {
                QDomNode option = options.item(i);
                QString optName;
                QString optValue;
                QVariant optVariant;
                if(!_loadNameValue(option, factName, metaData, optName, optValue, optVariant)) {
                    delete metaData;
                    return false;
                }
                metaData->addEnumInfo(optName, optVariant);
                _originalOptNames[factName]  << optName;
                _originalOptValues[factName] << optVariant;
                //-- Check for exclusions
                QStringList exclusions = _loadExclusions(option);
                if(exclusions.size()) {
                    qCDebug(CameraControlVerboseLog) << "New exclusions:" << factName << optValue << exclusions;
                    QGCCameraOptionExclusion* pExc = new QGCCameraOptionExclusion(this, factName, optValue, exclusions);
                    QQmlEngine::setObjectOwnership(pExc, QQmlEngine::CppOwnership);
                    _valueExclusions.append(pExc);
                }
                //-- Check for range rules
                if(!_loadRanges(option, factName, optValue)) {
                    delete metaData;
                    return false;
                }
            }
        }
        QString defaultValue;
        if(read_attribute(parameterNode, kDefault, defaultValue)) {
            QVariant defaultVariant;
            QString  errorString;
            if (metaData->convertAndValidateRaw(defaultValue, false, defaultVariant, errorString)) {
                metaData->setRawDefaultValue(defaultVariant);
            } else {
                qWarning() << "Invalid default value for" << factName
                           << " type:"  << metaData->type()
                           << " value:" << defaultValue
                           << " error:" << errorString;
            }
        }
        //-- Set metadata and Fact
        if (_nameToFactMetaDataMap.contains(factName)) {
            qWarning() << QStringLiteral("Duplicate fact name:") << factName;
            delete metaData;
        } else {
            {
                //-- Check for Min Value
                QString attr;
                if(read_attribute(parameterNode, kMin, attr)) {
                    QVariant typedValue;
                    QString  errorString;
                    if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                        metaData->setRawMin(typedValue);
                    } else {
                        qWarning() << "Invalid min value for" << factName
                                   << " type:"  << metaData->type()
                                   << " value:" << attr
                                   << " error:" << errorString;
                    }
                }
            }
            {
                //-- Check for Max Value
                QString attr;
                if(read_attribute(parameterNode, kMax, attr)) {
                    QVariant typedValue;
                    QString  errorString;
                    if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                        metaData->setRawMax(typedValue);
                    } else {
                        qWarning() << "Invalid max value for" << factName
                                   << " type:"  << metaData->type()
                                   << " value:" << attr
                                   << " error:" << errorString;
                    }
                }
            }
            {
                //-- Check for Step Value
                QString attr;
                if(read_attribute(parameterNode, kStep, attr)) {
                    QVariant typedValue;
                    QString  errorString;
                    if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                        metaData->setRawIncrement(typedValue.toDouble());
                    } else {
                        qWarning() << "Invalid step value for" << factName
                                   << " type:"  << metaData->type()
                                   << " value:" << attr
                                   << " error:" << errorString;
                    }
                }
            }
            {
                //-- Check for Decimal Places
                QString attr;
                if(read_attribute(parameterNode, kDecimalPlaces, attr)) {
                    QVariant typedValue;
                    QString  errorString;
                    if (metaData->convertAndValidateRaw(attr, true /* convertOnly */, typedValue, errorString)) {
                        metaData->setDecimalPlaces(typedValue.toInt());
                    } else {
                        qWarning() << "Invalid decimal places value for" << factName
                                   << " type:"  << metaData->type()
                                   << " value:" << attr
                                   << " error:" << errorString;
                    }
                }
            }
            {
                //-- Check for Units
                QString attr;
                if(read_attribute(parameterNode, kUnit, attr)) {
                    metaData->setRawUnits(attr);
                }
            }
            qCDebug(CameraControlLog) << "New parameter:" << factName << (readOnly ? "ReadOnly" : "Writable") << (writeOnly ? "WriteOnly" : "Readable");
            _nameToFactMetaDataMap[factName] = metaData;
            Fact* pFact = new Fact(_compID, factName, factType, this);
            QQmlEngine::setObjectOwnership(pFact, QQmlEngine::CppOwnership);
            pFact->setMetaData(metaData);
            pFact->_containerSetRawValue(metaData->rawDefaultValue());
            QGCCameraParamIO* pIO = new QGCCameraParamIO(this, pFact, _vehicle);
            QQmlEngine::setObjectOwnership(pIO, QQmlEngine::CppOwnership);
            _paramIO[factName] = pIO;
            _addFact(pFact, factName);
        }
    }
    if(_nameToFactMetaDataMap.size() > 0) {
        _addFactGroup(this, "camera");
        _processRanges();
        _activeSettings = _settings;
        emit activeSettingsChanged();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_handleLocalization(QByteArray& bytes)
{
    QString errorMsg;
    int errorLine;
    QDomDocument doc;
    if(!doc.setContent(bytes, false, &errorMsg, &errorLine)) {
        qCritical() << "Unable to parse camera definition file on line:" << errorLine;
        qCritical() << errorMsg;
        return false;
    }
    //-- Find out where we are
    QLocale locale = QLocale::system();
#if defined (Q_OS_MAC)
    locale = QLocale(locale.name());
#endif
    QString localeName = locale.name().toLower().replace("-", "_");
    qCDebug(CameraControlLog) << "Current locale:" << localeName;
    if(localeName == "en_us") {
        // Nothing to do
        return true;
    }
    QDomNodeList locRoot = doc.elementsByTagName(kLocalization);
    if(!locRoot.size()) {
        // Nothing to do
        return true;
    }
    //-- Iterate locales
    QDomNode node = locRoot.item(0);
    QDomElement elem = node.toElement();
    QDomNodeList locales = elem.elementsByTagName(kLocale);
    for(int i = 0; i < locales.size(); i++) {
        QDomNode locale = locales.item(i);
        QString name;
        if(!read_attribute(locale, kName, name)) {
            qWarning() << "Localization entry is missing its name attribute";
            continue;
        }
        // If we found a direct match, deal with it now
        if(localeName == name.toLower().replace("-", "_")) {
            return _replaceLocaleStrings(locale, bytes);
        }
    }
    //-- No direct match. Pick first matching language (if any)
    localeName = localeName.left(3);
    for(int i = 0; i < locales.size(); i++) {
        QDomNode locale = locales.item(i);
        QString name;
        read_attribute(locale, kName, name);
        if(name.toLower().startsWith(localeName)) {
            return _replaceLocaleStrings(locale, bytes);
        }
    }
    //-- Could not find a language to use
    qWarning() <<  "No match for" << QLocale::system().name() << "in camera definition file";
    //-- Just use default, en_US
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_replaceLocaleStrings(const QDomNode node, QByteArray& bytes)
{
    QDomElement stringElem = node.toElement();
    QDomNodeList strings = stringElem.elementsByTagName(kStrings);
    for(int i = 0; i < strings.size(); i++) {
        QDomNode stringNode = strings.item(i);
        QString original;
        QString translated;
        if(read_attribute(stringNode, kOriginal, original)) {
            if(read_attribute(stringNode, kTranslated, translated)) {
                QString o; o = "\"" + original + "\"";
                QString t; t = "\"" + translated + "\"";
                bytes.replace(o.toUtf8(), t.toUtf8());
                o = ">" + original + "<";
                t = ">" + translated + "<";
                bytes.replace(o.toUtf8(), t.toUtf8());
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestAllParameters()
{
    //-- Reset receive list
    for(const QString& paramName: _paramIO.keys()) {
        if(_paramIO[paramName]) {
            _paramIO[paramName]->setParamRequest();
        } else {
            qCritical() << "QGCParamIO is NULL" << paramName;
        }
    }
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();

        MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t msg;
        mavlink_msg_param_ext_request_list_pack_chan(
                    static_cast<uint8_t>(mavlink->getSystemId()),
                    static_cast<uint8_t>(mavlink->getComponentId()),
                    sharedLink->mavlinkChannel(),
                    &msg,
                    static_cast<uint8_t>(_vehicle->id()),
                    static_cast<uint8_t>(compID()));
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
    qCDebug(CameraControlVerboseLog) << "Request all parameters";
}

//-----------------------------------------------------------------------------
QString
QGCCameraControl::_getParamName(const char* param_id)
{
    QByteArray bytes(param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
    QString parameterName(bytes);
    return parameterName;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamAck(const mavlink_param_ext_ack_t& ack)
{
    QString paramName = _getParamName(ack.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_ACK for unknown param:" << paramName;
        return;
    }
    if(_paramIO[paramName]) {
        _paramIO[paramName]->handleParamAck(ack);
    } else {
        qCritical() << "QGCParamIO is NULL" << paramName;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleParamValue(const mavlink_param_ext_value_t& value)
{
    QString paramName = _getParamName(value.param_id);
    if(!_paramIO.contains(paramName)) {
        qCWarning(CameraControlLog) << "Received PARAM_EXT_VALUE for unknown param:" << paramName;
        return;
    }
    if(_paramIO[paramName]) {
        _paramIO[paramName]->handleParamValue(value);
    } else {
        qCritical() << "QGCParamIO is NULL" << paramName;
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateActiveList()
{
    //-- Clear out excluded parameters based on exclusion rules
    QStringList exclusionList;
    for(QGCCameraOptionExclusion* param: _valueExclusions) {
        Fact* pFact = getFact(param->param);
        if(pFact) {
            QString option = pFact->rawValueString();
            if(param->value == option) {
                exclusionList << param->exclusions;
            }
        }
    }
    QStringList active;
    for(QString key: _settings) {
        if(!exclusionList.contains(key)) {
            active.append(key);
        }
    }
    if(active != _activeSettings) {
        qCDebug(CameraControlVerboseLog) << "Excluding" << exclusionList;
        _activeSettings = active;
        emit activeSettingsChanged();
        //-- Force validity of "Facts" based on active set
        if(_paramComplete) {
            emit parametersReady();
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processConditionTest(const QString conditionTest)
{
    enum {
        TEST_NONE,
        TEST_EQUAL,
        TEST_NOT_EQUAL,
        TEST_GREATER,
        TEST_SMALLER
    };
    qCDebug(CameraControlVerboseLog) << "_processConditionTest(" << conditionTest << ")";
    int op = TEST_NONE;
    QStringList test;

    auto split = [&conditionTest](const QString& sep ) {
        return conditionTest.split(sep, Qt::SkipEmptyParts);
    };

    if(conditionTest.contains("!=")) {
        test = split("!=");
        op = TEST_NOT_EQUAL;
    } else if(conditionTest.contains("=")) {
        test = split("=");
        op = TEST_EQUAL;
    } else if(conditionTest.contains(">")) {
        test = split(">");
        op = TEST_GREATER;
    } else if(conditionTest.contains("<")) {
        test = split("<");
        op = TEST_SMALLER;
    }
    if(test.size() == 2) {
        Fact* pFact = getFact(test[0]);
        if(pFact) {
            switch(op) {
            case TEST_EQUAL:
                return pFact->rawValueString() == test[1];
            case TEST_NOT_EQUAL:
                return pFact->rawValueString() != test[1];
            case TEST_GREATER:
                return pFact->rawValueString() > test[1];
            case TEST_SMALLER:
                return pFact->rawValueString() < test[1];
            case TEST_NONE:
                break;
            }
        } else {
            qWarning() << "Invalid condition parameter:" << test[0] << "in" << conditionTest;
            return false;
        }
    }
    qWarning() << "Invalid condition" << conditionTest;
    return false;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_processCondition(const QString condition)
{
    qCDebug(CameraControlVerboseLog) << "_processCondition(" << condition << ")";
    bool result = true;
    bool andOp  = true;
    if(!condition.isEmpty()) {
        QStringList scond = condition.split(" ", Qt::SkipEmptyParts);
        while(scond.size()) {
            QString test = scond.first();
            scond.removeFirst();
            if(andOp) {
                result = result && _processConditionTest(test);
            } else {
                result = result || _processConditionTest(test);
            }
            if(!scond.size()) {
                return result;
            }
            andOp = scond.first().toUpper() == "AND";
            scond.removeFirst();
        }
    }
    return result;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_updateRanges(Fact* pFact)
{
    QMap<Fact*, QGCCameraOptionRange*> rangesSet;
    QMap<Fact*, QString> rangesReset;
    QStringList changedList;
    QStringList resetList;
    QStringList updates;
    //-- Iterate range sets looking for limited ranges
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        //-- If this fact or one of its conditions is part of this range set
        if(!changedList.contains(pRange->targetParam) && (pRange->param == pFact->name() || pRange->condition.contains(pFact->name()))) {
            Fact* pRFact = getFact(pRange->param);          //-- This parameter
            Fact* pTFact = getFact(pRange->targetParam);    //-- The target parameter (the one its range is to change)
            if(pRFact && pTFact) {
                //qCDebug(CameraControlVerboseLog) << "Check new set of options for" << pTFact->name();
                QString option = pRFact->rawValueString();  //-- This parameter value
                //-- If this value (and condition) triggers a change in the target range
                //qCDebug(CameraControlVerboseLog) << "Range value:" << pRange->value << "Current value:" << option << "Condition:" << pRange->condition;
                if(pRange->value == option && _processCondition(pRange->condition)) {
                    if(pTFact->enumStrings() != pRange->optNames) {
                        //-- Set limited range set
                        rangesSet[pTFact] = pRange;
                    }
                    changedList << pRange->targetParam;
                }
            }
        }
    }
    //-- Iterate range sets again looking for resets
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        if(!changedList.contains(pRange->targetParam) && (pRange->param == pFact->name() || pRange->condition.contains(pFact->name()))) {
            Fact* pTFact = getFact(pRange->targetParam);    //-- The target parameter (the one its range is to change)
            if(!resetList.contains(pRange->targetParam)) {
                if(pTFact->enumStrings() != _originalOptNames[pRange->targetParam]) {
                    //-- Restore full option set
                    rangesReset[pTFact] = pRange->targetParam;
                }
                resetList << pRange->targetParam;
            }
        }
    }
    //-- Update limited range set
    for (Fact* f: rangesSet.keys()) {
        f->setEnumInfo(rangesSet[f]->optNames, rangesSet[f]->optVariants);
        if(!updates.contains(f->name())) {
            _paramIO[f->name()]->optNames = rangesSet[f]->optNames;
            _paramIO[f->name()]->optVariants = rangesSet[f]->optVariants;
            emit f->enumsChanged();
            qCDebug(CameraControlVerboseLog) << "Limited set of options for:" << f->name() << rangesSet[f]->optNames;;
            updates << f->name();
        }
    }
    //-- Restore full range set
    for (Fact* f: rangesReset.keys()) {
        f->setEnumInfo(_originalOptNames[rangesReset[f]], _originalOptValues[rangesReset[f]]);
        if(!updates.contains(f->name())) {
            _paramIO[f->name()]->optNames = _originalOptNames[rangesReset[f]];
            _paramIO[f->name()]->optVariants = _originalOptValues[rangesReset[f]];
            emit f->enumsChanged();
            qCDebug(CameraControlVerboseLog) << "Restore full set of options for:" << f->name() << _originalOptNames[f->name()];
            updates << f->name();
        }
    }
    //-- Parameter update requests
    if(_requestUpdates.contains(pFact->name())) {
        for(const QString& param: _requestUpdates[pFact->name()]) {
            if(!_updatesToRequest.contains(param)) {
                _updatesToRequest << param;
            }
        }
    }
    if(_updatesToRequest.size()) {
        QTimer::singleShot(500, this, &QGCCameraControl::_requestParamUpdates);
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestParamUpdates()
{
    for(const QString& param: _updatesToRequest) {
        _paramIO[param]->paramRequest();
    }
    _updatesToRequest.clear();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestCameraSettings()
{
    qCDebug(CameraControlLog) << "_requestCameraSettings()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_REQUEST_CAMERA_SETTINGS,        // command id
            false,                                  // showError
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStorageInfo()
{
    qCDebug(CameraControlLog) << "_requestStorageInfo()";
    if(_vehicle) {
        _vehicle->sendMavCommand(
            _compID,                                // Target component
            MAV_CMD_REQUEST_STORAGE_INFORMATION,    // command id
            false,                                  // showError
            0,                                      // Storage ID (0 for all, 1 for first, 2 for second, etc.)
            1);                                     // Do Request
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleSettings(const mavlink_camera_settings_t& settings)
{
    const mavlink_camera_settings_t snapshot = settings;
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    qCDebug(CameraControlLog) << "handleSettings() Mode:" << snapshot.mode_id;
    _setCameraMode(static_cast<CameraMode>(snapshot.mode_id));
    if (!self || !_streamOperationCurrent(operation)) return;
    qreal z = static_cast<qreal>(snapshot.zoomLevel);
    qreal f = static_cast<qreal>(snapshot.focusLevel);
    if(std::isfinite(z) && z != _zoomLevel) {
        _zoomLevel = z;
        emit zoomLevelChanged();
        if (!self || !_streamOperationCurrent(operation)) return;
    }
    if(std::isfinite(f) && f != _focusLevel) {
        _focusLevel = f;
        emit focusLevelChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleStorageInfo(const mavlink_storage_information_t& st)
{
    qCDebug(CameraControlLog) << "handleStorageInfo:" << st.available_capacity << st.status << st.storage_count << st.storage_id << st.total_capacity << st.used_capacity;
    if(st.status == STORAGE_STATUS_READY) {
        uint32_t t = static_cast<uint32_t>(st.total_capacity);
        if(_storageTotal != t) {
            _storageTotal = t;
            emit storageTotalChanged();
        }
        uint32_t a = static_cast<uint32_t>(st.available_capacity);
        if(_storageFree != a) {
            _storageFree = a;
            emit storageFreeChanged();
        }
    }
    if(_storageStatus != static_cast<StorageStatus>(st.status)) {
        _storageStatus = static_cast<StorageStatus>(st.status);
        emit storageStatusChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleBatteryStatus(const mavlink_battery_status_t& bs)
{
    qCDebug(CameraControlLog) << "handleBatteryStatus:" << bs.battery_remaining;
    if(bs.battery_remaining >= 0 && _batteryRemaining != static_cast<int>(bs.battery_remaining)) {
        _batteryRemaining = static_cast<int>(bs.battery_remaining);
        emit batteryRemainingChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleCaptureStatus(const mavlink_camera_capture_status_t& cap)
{
    //-- This is a response to MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS
    qCDebug(CameraControlLog) << "handleCaptureStatus:" << cap.available_capacity << cap.image_interval << cap.image_status << cap.recording_time_ms << cap.video_status;
    //-- Disk Free Space
    uint32_t a = static_cast<uint32_t>(cap.available_capacity);
    if(_storageFree != a) {
        _storageFree = a;
        emit storageFreeChanged();
    }
    //-- Do we have recording time?
    if(cap.recording_time_ms) {
        // Resync our _recTime timer to the time info received from the camera component
        _recordTime = cap.recording_time_ms;
        _recTime = _recTime.addMSecs(_recTime.msecsTo(QTime::currentTime()) - static_cast<int>(cap.recording_time_ms));
        emit recordTimeChanged();
    }
    //-- Video/Image Capture Status
    uint8_t vs = cap.video_status < static_cast<uint8_t>(VIDEO_CAPTURE_STATUS_LAST) ? cap.video_status : static_cast<uint8_t>(VIDEO_CAPTURE_STATUS_UNDEFINED);
    uint8_t ps = cap.image_status < static_cast<uint8_t>(PHOTO_CAPTURE_LAST) ? cap.image_status : static_cast<uint8_t>(PHOTO_CAPTURE_STATUS_UNDEFINED);
    _setVideoStatus(static_cast<VideoStatus>(vs));
    _setPhotoStatus(static_cast<PhotoStatus>(ps));
    //-- Keep asking for it once in a while when recording
    if(videoStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        _captureStatusTimer.start(5000);
    //-- Same while (single) image capture is busy
    } else if(photoStatus() != PHOTO_CAPTURE_IDLE && photoMode() == PHOTO_CAPTURE_SINGLE) {
        _captureStatusTimer.start(1000);
    }
    //-- Time Lapse
    if(photoStatus() == PHOTO_CAPTURE_INTERVAL_IDLE || photoStatus() == PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
        //-- Capture local image as well
        if(qgcApp()->toolbox()->videoManager()) {
            QString photoPath = qgcApp()->toolbox()->settingsManager()->appSettings()->savePath()->rawValue().toString() + QStringLiteral("/Photo");
            QDir().mkpath(photoPath);
            photoPath += + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + ".jpg";
            qgcApp()->toolbox()->videoManager()->grabImage(photoPath);
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleVideoInfo(const mavlink_video_stream_information_t* vi)
{
    if (!vi || _streamStructuralBusy || _streamDestroying || _streamExhausted) return;
    const mavlink_video_stream_information_t information = *vi;
    // Stream metadata is one-based. Zero in query commands means "all", and
    // must never become a STOP/START target through a malformed catalog row.
    if (!information.stream_id || !information.count || information.stream_id > information.count) return;
    QPointer<QGCCameraControl> self(this);
    auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    _streamStructuralBusy = true;
    const auto cleanup = qScopeGuard([self]() {
        if (self) { self->_streamStructuralBusy = false; self->_queueRetiredStreamCleanup(); }
    });
    const auto existing = _findStream(information.stream_id, false);
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto ownedExisting = _ownedStream(existing);
    if (ownedExisting && _expectedCount == information.count
        && sameStreamInformation(ownedExisting->_lastInformation, information)) return;
    if (!_advanceStreamEpoch()) return;
    operation.epoch = _streamEpoch;
    _expectedCount = information.count;
    if (!ownedExisting) {
        // Parent assignment and model insertion can both synchronously notify.
        QPointer<QGCVideoStreamInfo> stream = new QGCVideoStreamInfo(nullptr, &information);
        if (!self || !_streamOperationCurrent(operation)) { if (stream) delete stream.data(); return; }
        QQmlEngine::setObjectOwnership(stream.data(), QQmlEngine::CppOwnership);
        if (!self || !stream || !_streamOperationCurrent(operation)) { if (stream) delete stream.data(); return; }
        stream->setParent(this);
        if (!self || !stream || stream->parent() != self.data() || !_streamOperationCurrent(operation)) {
            if (stream) delete stream.data();
            return;
        }
        const bool thermal = stream->isThermal();
        const QString label = stream->name();
        QObject* const original = stream.data();
        connect(stream.data(), &QObject::destroyed, this, [this, original]() { _streamDestroyed(original); });
        if (!self || !stream || !_streamOperationCurrent(operation)) return;
        // Keep native list/labels coherent before any model notification.
        if (!thermal) _streamLabels.append(label);
        const quint64 previousEpoch = operation.epoch;
        _streams.append(stream.data());
        if (!self || !stream || _streamExhausted
            || previousEpoch == std::numeric_limits<quint64>::max()
            || _streamEpoch != previousEpoch + 1) return;
        operation.epoch = _streamEpoch; // exactly the one countChanged from this append
        if (!_streamOperationCurrent(operation)) return;
        _selectedStreamIdentity = _streamAtSelection(_currentStream);
        if (!thermal) {
            emit streamsChanged();
            if (!self || !_streamOperationCurrent(operation)) return;
            emit streamLabelsChanged();
            if (!self || !_streamOperationCurrent(operation)) return;
        } else {
            emit thermalStreamChanged();
            if (!self || !_streamOperationCurrent(operation)) return;
        }
    } else {
        const bool wasThermal = ownedExisting->isThermal();
        const bool thermal = bool(information.flags & VIDEO_STREAM_STATUS_FLAGS_THERMAL);
        const auto selected = _selectedStreamIdentity ? _selectedStreamIdentity : _streamAtSelection(_currentStream);
        // Stage complete native metadata/labels before notifying its consumers.
        ownedExisting->_streamInfo = information;
        ownedExisting->_lastInformation = information;
        QStringList labels;
        for (int index = 0; index < _streams.count(); ++index) {
            const auto stream = _ownedStream(_streams[index]);
            if (stream && !stream->isThermal()) labels.append(stream->name());
        }
        const bool labelsChanged = labels != _streamLabels;
        _streamLabels = labels;
        const int previousSelection = _currentStream;
        _currentStream = _streamSelectionIndex(selected);
        if (_currentStream < 0) _currentStream = 0;
        _selectedStreamIdentity = _streamAtSelection(_currentStream);
        operation.selection = _currentStream;
        emit ownedExisting->infoChanged();
        if (!self || !ownedExisting || !_streamOperationCurrent(operation)) return;
        emit streamsChanged();
        if (!self || !ownedExisting || !_streamOperationCurrent(operation)) return;
        if (labelsChanged) {
            emit streamLabelsChanged();
            if (!self || !ownedExisting || !_streamOperationCurrent(operation)) return;
        }
        if (_currentStream != previousSelection || _selectedStreamIdentity != selected) {
            emit currentStreamChanged();
            if (!self || !ownedExisting || !_streamOperationCurrent(operation)) return;
        }
        if (wasThermal || thermal) {
            emit thermalStreamChanged();
            if (!self || !ownedExisting || !_streamOperationCurrent(operation)) return;
        }
    }
    if (_streams.count() < _expectedCount) _streamInfoTimer.start(1000);
    else {
        _streamInfoTimer.stop();
        emit autoStreamChanged();
        if (!self || !_streamOperationCurrent(operation)) return;
        const auto manager = operation.manager;
        if (manager) emit manager->streamChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::handleVideoStatus(const mavlink_video_stream_status_t* vs)
{
    if (!vs || _streamDestroying || _streamExhausted) return;
    const mavlink_video_stream_status_t status = *vs;
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    const quint64 statusEpoch = _streamStatusEpoch;
    const auto found = _findStream(status.stream_id);
    if (!self || !_streamOperationCurrent(operation) || statusEpoch != _streamStatusEpoch) return;
    const auto stream = _ownedStream(found);
    if (!stream) return;
    if (_streamStatusTarget == stream && _streamStatusTargetId == status.stream_id
        && _streamOperationCurrent(_streamStatusOperation)) {
        _streamStatusTimer.stop();
        _advanceStreamStatusEpoch(); // a synchronous response must not restart the old timer
    }
    if (!self || !_streamOperationCurrent(operation)) return;
    stream->update(&status); // update's only notification is last; no continuation through the stream
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::setCurrentStream(int stream)
{
    // The dead-row bridge has not published its rebuilt labels yet. An old
    // displayed index must not silently select a surviving successor object.
    if (_streamRowsRetired || stream < 0 || stream >= _streamLabels.count() || !_advanceStreamEpoch()) return;
    _streamStatusTimer.stop();
    if (!_advanceStreamStatusEpoch()) return;
    QPointer<QGCCameraControl> self(this);
    auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    if (stream == operation.selection) {
        _selectedStreamIdentity = _streamAtSelection(stream);
        return; // still revokes an older in-flight intent
    }
    const auto oldRaw = currentStreamInstance();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto oldStream = _ownedStream(oldRaw);
    const auto target = _streamAtSelection(stream);
    if (!target) return;
    const int targetId = target ? target->streamID() : -1;
    if (target && targetId <= 0) return;
    if (oldStream) {
        const int oldId = oldStream->streamID();
        if (oldId <= 0) return;
        operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_VIDEO_STOP_STREAMING, false, oldId);
        if (!self || !_streamOperationCurrent(operation) || !oldStream || (targetId >= 0 && !target)) return;
    }
    _currentStream = stream;
    _selectedStreamIdentity = target;
    operation.selection = stream;
    if (target) {
        operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_VIDEO_START_STREAMING, false, targetId);
        if (!self || !target || !_streamOperationCurrent(operation)) return;
        _requestStreamStatus(static_cast<uint8_t>(targetId));
        if (!self || !target || !_streamOperationCurrent(operation)) return;
    }
    emit currentStreamChanged();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto manager = operation.manager;
    if (manager) emit manager->streamChanged();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::stopStream()
{
    if (!_advanceStreamEpoch() || !_advanceStreamStatusEpoch()) return;
    _streamStatusTimer.stop();
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto raw = currentStreamInstance();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto stream = _ownedStream(raw);
    if (stream && stream->streamID() > 0) operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_VIDEO_STOP_STREAMING, false, stream->streamID());
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::resumeStream()
{
    if (!_advanceStreamEpoch() || !_advanceStreamStatusEpoch()) return;
    _streamStatusTimer.stop();
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto raw = currentStreamInstance();
    if (!self || !_streamOperationCurrent(operation)) return;
    const auto stream = _ownedStream(raw);
    if (stream && stream->streamID() > 0) operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_VIDEO_START_STREAMING, false, stream->streamID());
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::autoStream()
{
    QPointer<QGCCameraControl> self(this);
    const auto epoch = _streamEpoch;
    const bool available = hasVideoStream();
    return self && !_streamDestroying && !_streamExhausted && epoch == _streamEpoch && available && _streams.count() > 0;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::currentStreamInstance()
{
    if (_streamDestroying || _streamExhausted) return nullptr;
    const auto selected = _streamAtSelection(_currentStream);
    _selectedStreamIdentity = selected;
    return selected.data();
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::thermalStreamInstance()
{
    if (_streamDestroying || _streamExhausted) return nullptr;
    //-- For now, it will return the first thermal listed (if any)
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            const auto pStream = _ownedStream(_streams[i]);
            if(pStream) {
                if(pStream->isThermal()) {
                    return pStream;
                }
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStreamInfo(uint8_t streamID)
{
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION, false, streamID);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_requestStreamStatus(uint8_t streamID)
{
    if (!_advanceStreamStatusEpoch()) return;
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    const quint64 statusEpoch = _streamStatusEpoch;
    const auto raw = _findStream(streamID, false);
    if (!self || !_streamOperationCurrent(operation) || statusEpoch != _streamStatusEpoch) return;
    const auto stream = _ownedStream(raw);
    _streamStatusOperation = operation;
    _streamStatusTarget = stream;
    _streamStatusTargetId = streamID;
    operation.vehicle->sendMavCommand(operation.componentId, MAV_CMD_REQUEST_VIDEO_STREAM_STATUS, false, streamID);
    if (!self || !_streamOperationCurrent(operation) || statusEpoch != _streamStatusEpoch
        || _streamStatusTarget != stream || _streamStatusTargetId != streamID) return;
    _streamStatusTimer.start(1000);
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::_findStream(uint8_t id, bool report)
{
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            const auto pStream = _ownedStream(_streams[i]);
            if(pStream) {
                if(pStream->streamID() == id) {
                    return pStream;
                }
            } else {
                qCritical() << "Null QGCVideoStreamInfo instance";
            }
        }
    }
    if(report) {
        qWarning() << "Stream id not found:" << id;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo*
QGCCameraControl::_findStream(const QString name)
{
    for(int i = 0; i < _streams.count(); i++) {
        if(_streams[i]) {
            const auto pStream = _ownedStream(_streams[i]);
            if(pStream) {
                if(pStream->name() == name) {
                    return pStream;
                }
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_streamTimeout()
{
    if (_streamDestroying || _streamExhausted || _streamStructuralBusy) return;
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    if (!self || !_streamOperationCurrent(operation)) return;
    _requestCount++;
    int count = _expectedCount * 3;
    if(_requestCount > count) {
        qCWarning(CameraControlLog) << "Giving up requesting video stream info";
        _streamInfoTimer.stop();
        //-- If we have at least one stream, work with what we have.
        if(_streams.count()) {
            emit autoStreamChanged();
            if (!self || !_streamOperationCurrent(operation)) return;
            const auto manager = operation.manager;
            if (manager) emit manager->streamChanged();
        }
        return;
    }
    for(uint8_t i = 0; i < _expectedCount; i++) {
        //-- Stream ID starts at 1
        const auto found = _findStream(i+1, false);
        if (!self || !_streamOperationCurrent(operation)) return;
        if(!_ownedStream(found)) {
            _requestStreamInfo(i+1);
            return;
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_streamStatusTimeout()
{
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamStatusOperation;
    const auto target = _streamStatusTarget;
    const int targetId = _streamStatusTargetId;
    if (!_streamOperationCurrent(operation) || !target || targetId < 0) return;
    const auto raw = currentStreamInstance();
    if (!self || !_streamOperationCurrent(operation) || !target || _ownedStream(raw) != target) return;
    _requestStreamStatus(static_cast<uint8_t>(targetId));
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraControl::_loadExclusions(QDomNode option)
{
    QStringList exclusionList;
    QDomElement optionElem = option.toElement();
    QDomNodeList excRoot = optionElem.elementsByTagName(kExclusions);
    if(excRoot.size()) {
        //-- Iterate exclusions
        QDomNode node = excRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList exclusions = elem.elementsByTagName(kExclusion);
        for(int i = 0; i < exclusions.size(); i++) {
            QString exclude = exclusions.item(i).toElement().text();
            if(!exclude.isEmpty()) {
                exclusionList << exclude;
            }
        }
    }
    return exclusionList;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraControl::_loadUpdates(QDomNode option)
{
    QStringList updateList;
    QDomElement optionElem = option.toElement();
    QDomNodeList updateRoot = optionElem.elementsByTagName(kUpdates);
    if(updateRoot.size()) {
        //-- Iterate updates
        QDomNode node = updateRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList updates = elem.elementsByTagName(kUpdate);
        for(int i = 0; i < updates.size(); i++) {
            QString update = updates.item(i).toElement().text();
            if(!update.isEmpty()) {
                updateList << update;
            }
        }
    }
    return updateList;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadRanges(QDomNode option, const QString factName, QString paramValue)
{
    QDomElement optionElem = option.toElement();
    QDomNodeList rangeRoot = optionElem.elementsByTagName(kParameterranges);
    if(rangeRoot.size()) {
        QDomNode node = rangeRoot.item(0);
        QDomElement elem = node.toElement();
        QDomNodeList parameterRanges = elem.elementsByTagName(kParameterrange);
        //-- Iterate parameter ranges
        for(int i = 0; i < parameterRanges.size(); i++) {
            QString param;
            QString condition;
            QDomNode paramRange = parameterRanges.item(i);
            if(!read_attribute(paramRange, kParameter, param)) {
                qCritical() << QString("Malformed option range for parameter %1").arg(factName);
                return false;
            }
            read_attribute(paramRange, kCondition, condition);
            QDomElement pelem = paramRange.toElement();
            QDomNodeList rangeOptions = pelem.elementsByTagName(kRoption);
            QStringList  optNames;
            QStringList  optValues;
            //-- Iterate options
            for(int i = 0; i < rangeOptions.size(); i++) {
                QString optName;
                QString optValue;
                QDomNode roption = rangeOptions.item(i);
                if(!read_attribute(roption, kName, optName)) {
                    qCritical() << QString("Malformed roption for parameter %1").arg(factName);
                    return false;
                }
                if(!read_attribute(roption, kValue, optValue)) {
                    qCritical() << QString("Malformed rvalue for parameter %1").arg(factName);
                    return false;
                }
                optNames  << optName;
                optValues << optValue;
            }
            if(optNames.size()) {
                QGCCameraOptionRange* pRange = new QGCCameraOptionRange(this, factName, paramValue, param, condition, optNames, optValues);
                _optionRanges.append(pRange);
                qCDebug(CameraControlVerboseLog) << "New range limit:" << factName << paramValue << param << condition << optNames << optValues;
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_processRanges()
{
    //-- After all parameter are loaded, process parameter ranges
    for(QGCCameraOptionRange* pRange: _optionRanges) {
        Fact* pRFact = getFact(pRange->targetParam);
        if(pRFact) {
            for(int i = 0; i < pRange->optNames.size(); i++) {
                QVariant optVariant;
                QString  errorString;
                if (!pRFact->metaData()->convertAndValidateRaw(pRange->optValues[i], false, optVariant, errorString)) {
                    qWarning() << "Invalid roption value, name:" << pRange->targetParam
                               << " type:"  << pRFact->metaData()->type()
                               << " value:" << pRange->optValues[i]
                               << " error:" << errorString;
                } else {
                    pRange->optVariants << optVariant;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::_loadNameValue(QDomNode option, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant)
{
    if(!read_attribute(option, kName, optName)) {
        qCritical() << QString("Malformed option for parameter %1").arg(factName);
        return false;
    }
    if(!read_attribute(option, kValue, optValue)) {
        qCritical() << QString("Malformed value for parameter %1").arg(factName);
        return false;
    }
    QString  errorString;
    if (!metaData->convertAndValidateRaw(optValue, false, optVariant, errorString)) {
        qWarning() << "Invalid option value, name:" << factName
                   << " type:"  << metaData->type()
                   << " value:" << optValue
                   << " error:" << errorString;
    }
    return true;
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_handleDefinitionFile(const QString &url)
{
    //-- First check and see if we have it cached
    QFile xmlFile(_cacheFile);

    QString ftpPrefix(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme));
    if (!xmlFile.exists() && url.startsWith(ftpPrefix, Qt::CaseInsensitive)) {
        qCDebug(CameraControlLog) << "No camera definition file cached, attempt ftp download";
        int ver = static_cast<int>(_info.cam_definition_version);
        QString ext = "";
        if (url.endsWith(".lzma", Qt::CaseInsensitive)) { ext = ".lzma"; }
        if (url.endsWith(".xz", Qt::CaseInsensitive)) { ext = ".xz"; }
        QString fileName = QString::asprintf("%s_%s_%03d.xml%s",
            _vendor.toStdString().c_str(),
            _modelName.toStdString().c_str(),
            ver,
            ext.toStdString().c_str());
        connect(_vehicle->ftpManager(), &FTPManager::downloadComplete, this, &QGCCameraControl::_ftpDownloadComplete);
        _vehicle->ftpManager()->download(_compID, url,
            qgcApp()->toolbox()->settingsManager()->appSettings()->parameterSavePath().toStdString().c_str(),
            fileName);
        return;
    }

    if (!xmlFile.exists()) {
        qCDebug(CameraControlLog) << "No camera definition file cached, attempt http download";
        _httpRequest(url);
        return;
    }
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not read cached camera definition file:" << _cacheFile;
        _httpRequest(url);
        return;
    }
    QByteArray bytes = xmlFile.readAll();
    QDomDocument doc;
    if(!doc.setContent(bytes, false)) {
        qWarning() << "Could not parse cached camera definition file:" << _cacheFile;
        _httpRequest(url);
        return;
    }
    //-- We have it
    qCDebug(CameraControlLog) << "Using cached camera definition file:" << _cacheFile;
    _cached = true;
    emit dataReady(bytes);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_httpRequest(const QString &url)
{
    qCDebug(CameraControlLog) << "Request camera definition:" << url;
    if(!_netManager) {
        _netManager = new QNetworkAccessManager(this);
    }
    QNetworkProxy savedProxy = _netManager->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    _netManager->setProxy(tempProxy);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply* reply = _netManager->get(request);
    connect(reply, &QNetworkReply::finished,  this, &QGCCameraControl::_downloadFinished);
    _netManager->setProxy(savedProxy);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_downloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    int err = reply->error();
    int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if(err == QNetworkReply::NoError && http_code == 200) {
        data.append("\n");
    } else {
        data.clear();
        qWarning() << QString("Camera Definition (%1) download error: %2 status: %3").arg(
            reply->url().toDisplayString(),
            reply->errorString(),
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString()
        );
    }
    emit dataReady(data);
    //reply->deleteLater();
}

void QGCCameraControl::_ftpDownloadComplete(const QString& fileName, const QString& errorMsg)
{
    qCDebug(CameraControlLog) << "FTP Download completed: " << fileName << ", " << errorMsg;

    disconnect(_vehicle->ftpManager(), &FTPManager::downloadComplete, this, &QGCCameraControl::_ftpDownloadComplete);

    QString outputFileName = fileName;

    if (fileName.endsWith(".lzma", Qt::CaseInsensitive) || fileName.endsWith(".xz", Qt::CaseInsensitive)) {
        outputFileName = fileName.left(fileName.lastIndexOf("."));
        if (QGCLZMA::inflateLZMAFile(fileName, outputFileName)) {
            QFile(fileName).remove();
        } else {
            qCWarning(CameraControlLog) << "Inflate of compressed xml failed" << fileName;
            outputFileName.clear();
        }
    }

    QFile xmlFile(outputFileName);

    if (!xmlFile.exists()) {
        qCDebug(CameraControlLog) << "No camera definition file present after ftp download completed";
        return;
    }
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not read downloaded camera definition file: " << fileName;
        return;
    }

    _cached = true;
    QByteArray bytes = xmlFile.readAll();
    emit dataReady(bytes);
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_dataReady(QByteArray data)
{
    if(data.size()) {
        qCDebug(CameraControlLog) << "Parsing camera definition";
        _loadCameraDefinitionFile(data);
    } else {
        qCDebug(CameraControlLog) << "No camera definition";
    }
    _initWhenReady();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_paramDone()
{
    QPointer<QGCCameraControl> self(this);
    const quint64 streamEpoch = _streamEpoch;
    for(const QString& param: _paramIO.keys()) {
        if(!_paramIO[param]->paramDone()) {
            return;
        }
    }
    //-- All parameters loaded (or timed out)
    _paramComplete = true;
    emit parametersReady();
    if (!self || streamEpoch != _streamEpoch || _streamDestroying || _streamExhausted) return;
    //-- Check for video streaming
    _checkForVideoStreams();
}

//-----------------------------------------------------------------------------
void
QGCCameraControl::_checkForVideoStreams()
{
    if (!(_info.flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM)) return;
    QPointer<QGCCameraControl> self(this);
    const auto operation = _streamOperation();
    const auto toolbox = _streamToolbox;
    const auto video = _streamVideoManager;
    if (!self || !_streamOperationCurrent(operation) || !toolbox || !video
        || toolbox->videoManager() != video.data()) return;
    // isTaisync is virtual. Never rediscover a deleted provider from a raw slot.
    const bool taisync = video->isTaisync();
    if (!self || !_streamOperationCurrent(operation) || !toolbox || !video
        || toolbox != _streamToolbox || video != _streamVideoManager || toolbox->videoManager() != video.data() || taisync) return;
    _requestStreamInfo(0);
    if (!self || !_streamOperationCurrent(operation) || !video || !toolbox) return;
    _streamInfoTimer.start(2000);
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::incomingParameter(Fact* pFact, QVariant& newValue)
{
    Q_UNUSED(pFact);
    Q_UNUSED(newValue);
    return true;
}

//-----------------------------------------------------------------------------
bool
QGCCameraControl::validateParameter(Fact* pFact, QVariant& newValue)
{
    Q_UNUSED(pFact);
    Q_UNUSED(newValue);
    return true;
}

//-----------------------------------------------------------------------------
QStringList
QGCCameraControl::activeSettings()
{
    qCDebug(CameraControlLog) << "Active:" << _activeSettings;
    return _activeSettings;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::exposureMode()
{
    return (_paramComplete && _activeSettings.contains(kCAM_EXPMODE)) ? getFact(kCAM_EXPMODE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::ev()
{
    return (_paramComplete && _activeSettings.contains(kCAM_EV)) ? getFact(kCAM_EV) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::iso()
{
    return (_paramComplete && _activeSettings.contains(kCAM_ISO)) ? getFact(kCAM_ISO) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::shutterSpeed()
{
    return (_paramComplete && _activeSettings.contains(kCAM_SHUTTERSPD)) ? getFact(kCAM_SHUTTERSPD) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::aperture()
{
    return (_paramComplete && _activeSettings.contains(kCAM_APERTURE)) ? getFact(kCAM_APERTURE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::wb()
{
    return (_paramComplete && _activeSettings.contains(kCAM_WBMODE)) ? getFact(kCAM_WBMODE) : nullptr;
}

//-----------------------------------------------------------------------------
Fact*
QGCCameraControl::mode()
{
    return _paramComplete ? _ownedCameraModeFact().data() : nullptr;
}

//-----------------------------------------------------------------------------
QGCVideoStreamInfo::QGCVideoStreamInfo(QObject* parent, const mavlink_video_stream_information_t *si)
    : QObject(parent)
{
    if (si) {
        memcpy(&_streamInfo, si, sizeof(mavlink_video_stream_information_t));
        _lastInformation = _streamInfo;
    }
}

QString QGCVideoStreamInfo::uri() const
{
    return boundedCameraText(_streamInfo.uri, sizeof(_streamInfo.uri));
}

QString QGCVideoStreamInfo::name() const
{
    return boundedCameraText(_streamInfo.name, sizeof(_streamInfo.name));
}

//-----------------------------------------------------------------------------
qreal
QGCVideoStreamInfo::aspectRatio() const
{
    qreal ar = 1.0;
    if(_streamInfo.resolution_h && _streamInfo.resolution_v) {
        ar = static_cast<double>(_streamInfo.resolution_h) / static_cast<double>(_streamInfo.resolution_v);
    }
    return ar;
}

//-----------------------------------------------------------------------------
bool
QGCVideoStreamInfo::update(const mavlink_video_stream_status_t* vs)
{
    if (!vs) return false;
    const mavlink_video_stream_status_t status = *vs;
    vs = &status;
    bool changed = false;
    if(_streamInfo.hfov != vs->hfov) {
        changed = true;
        _streamInfo.hfov = vs->hfov;
    }
    if(_streamInfo.flags != vs->flags) {
        changed = true;
        _streamInfo.flags = vs->flags;
    }
    if(_streamInfo.bitrate != vs->bitrate) {
        changed = true;
        _streamInfo.bitrate = vs->bitrate;
    }
    if(_streamInfo.rotation != vs->rotation) {
        changed = true;
        _streamInfo.rotation = vs->rotation;
    }
    if(_streamInfo.framerate != vs->framerate) {
        changed = true;
        _streamInfo.framerate = vs->framerate;
    }
    if(_streamInfo.resolution_h != vs->resolution_h) {
        changed = true;
        _streamInfo.resolution_h = vs->resolution_h;
    }
    if(_streamInfo.resolution_v != vs->resolution_v) {
        changed = true;
        _streamInfo.resolution_v = vs->resolution_v;
    }
    if(changed) {
        emit infoChanged();
    }
    return changed;
}

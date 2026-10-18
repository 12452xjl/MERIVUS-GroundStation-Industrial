/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCApplication.h"
#include "QGCCameraManager.h"
#include "JoystickManager.h"
#include "FirmwarePlugin.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include <QScopeGuard>
#include <QQmlEngine>
#include <QThread>
#include <limits>

QGC_LOGGING_CATEGORY(CameraManagerLog, "CameraManagerLog")

QGCCameraManager::CameraStruct::CameraStruct(QObject* parent, uint8_t component)
    : QObject(parent), compID(component)
{
}

QGCCameraManager::QGCCameraManager(Vehicle* vehicle)
    : QObject(vehicle), _vehicle(vehicle)
{
    const QPointer<QGCCameraManager> self(this);
    _toolbox = qgcApp() ? qgcApp()->toolbox() : nullptr;
    if (!_toolbox) return;
    _multiVehicleManager = _toolbox->multiVehicleManager();
    _joystickManager = _toolbox->joystickManager();
    _firmwarePlugin = _vehicle ? _vehicle->firmwarePlugin() : nullptr;
    _vehicleId = _vehicle ? _vehicle->id() : 0;
    _firmwareType = _vehicle ? int(_vehicle->firmwareType()) : 0;
    _vehicleType = _vehicle ? int(_vehicle->vehicleType()) : 0;
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    if (!self || !_sourceCurrent()) return;
    connect(_multiVehicleManager.data(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged,
            this, &QGCCameraManager::_vehicleReady);
    if (!self || !_sourceCurrent()) return;
    connect(_multiVehicleManager.data(), &MultiVehicleManager::activeVehicleChanged, this, [this]() {
        _vehicleReady(_multiVehicleManager && _multiVehicleManager->parameterReadyVehicleAvailable());
    });
    if (!self || !_sourceCurrent()) return;
    // Connect once, rather than accumulating one connection per ready=true.
    connect(_joystickManager.data(), &JoystickManager::activeJoystickChanged,
            this, &QGCCameraManager::_activeJoystickChanged);
    if (!self || !_sourceCurrent()) return;
    connect(_vehicle.data(), &Vehicle::mavlinkMessageReceived, this, &QGCCameraManager::_mavlinkMessageReceived);
    if (!self || !_sourceCurrent()) return;
    connect(_vehicle.data(), &Vehicle::firmwareTypeChanged, this, &QGCCameraManager::_sourceChanged);
    if (!self || !_sourceCurrent()) return;
    connect(_vehicle.data(), &Vehicle::vehicleTypeChanged, this, &QGCCameraManager::_sourceChanged);
    if (!self || !_sourceCurrent()) return;
    connect(&_cameraTimer, &QTimer::timeout, this, &QGCCameraManager::_cameraTimeout);
    if (!self || !_sourceCurrent()) return;
    _lastZoomChange.start();
    _lastCameraChange.start();
    _cameraTimer.start(500);
}

QGCCameraManager::~QGCCameraManager()
{
    _destroying = true;
    _cameraTimer.stop();
    // QObject disconnects the sources. Do not call a foreign camera/joystick
    // or emit notifications from destruction.
}

bool QGCCameraManager::_sourceCurrent() const
{
    return !_destroying && !_epochExhausted && QThread::currentThread() == thread()
        && _vehicle && parent() == _vehicle.data() && _vehicle->id() == _vehicleId
        && int(_vehicle->firmwareType()) == _firmwareType && int(_vehicle->vehicleType()) == _vehicleType
        && _toolbox && _multiVehicleManager && _firmwarePlugin
        && _toolbox->multiVehicleManager() == _multiVehicleManager.data()
        && _vehicle->firmwarePlugin() == _firmwarePlugin.data();
}

bool QGCCameraManager::_contextCurrent(quint64 epoch) const
{
    return epoch != 0 && cameraSelectionEpoch() == epoch && _sourceCurrent();
}

bool QGCCameraManager::_advanceCameraEpoch()
{
    if (_destroying || _epochExhausted) return false;
    if (_cameraEpoch == std::numeric_limits<quint64>::max()) {
        _epochExhausted = true;
        return false;
    }
    ++_cameraEpoch;
    return true;
}

void QGCCameraManager::_sourceChanged()
{
    if (!_advanceCameraEpoch()) return;
    _vehicleReadyState = false;
    _activeJoystickChanged(nullptr);
}

QPointer<QGCCameraControl> QGCCameraManager::_knownCamera(QGCCameraControl* camera) const
{
    // Resolve borrowed raw returns only against persistent native pointers.
    for (const auto& actual : _cameraControls) if (actual && actual.data() == camera) {
        for (auto it = _cameraInfoRequest.constBegin(); it != _cameraInfoRequest.constEnd(); ++it) {
            const auto request = it.value();
            if (request && !request->retired && request->infoReceived && request->camera == actual) return actual;
        }
    }
    return {};
}

void QGCCameraManager::setCurrentCamera(int selection)
{
    if (!_sourceCurrent() || selection < 0 || selection >= _cameras.count()
        || selection >= _cameraControls.size() || !_knownCamera(_cameraControls.at(selection).data())) return;
    // A same-index explicit intent also retires an older notify continuation.
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    if (selection == _currentCamera) return;
    const QPointer<QGCCameraManager> self(this);
    const QPointer<QGCCameraControl> selected = _cameraControls.at(selection);
    const quint64 streamEpoch = selected->streamSelectionEpoch();
    _currentCamera = selection;
    emit currentCameraChanged();
    if (!self || !_contextCurrent(epoch) || !selected) return;
    QGCCameraControl* actual = currentCameraInstance();
    if (!self || !_contextCurrent(epoch) || !selected || actual != selected.data()
        || !streamEpoch || selected->streamSelectionEpoch() != streamEpoch) return;
    emit streamChanged();
}

void QGCCameraManager::_vehicleReady(bool ready)
{
    if (!_sourceCurrent()) return;
    const bool eligible = ready && _multiVehicleManager->parameterReadyVehicleAvailable()
        && _multiVehicleManager->activeVehicle() == _vehicle.data();
    if (_vehicleReadyState != eligible) {
        if (!_advanceCameraEpoch()) return;
        _vehicleReadyState = eligible;
    }
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    Joystick* joystick = eligible && _joystickManager ? _joystickManager->activeJoystick() : nullptr;
    if (!self || !_contextCurrent(epoch)) return;
    _activeJoystickChanged(joystick);
}

bool QGCCameraManager::_joystickCurrent() const
{
    return _sourceCurrent() && _vehicleReadyState && _activeJoystick && _joystickManager
        && _toolbox->joystickManager() == _joystickManager.data()
        && _multiVehicleManager->parameterReadyVehicleAvailable()
        && _multiVehicleManager->activeVehicle() == _vehicle.data()
        && _joystickManager->activeJoystick() == _activeJoystick.data();
}

void QGCCameraManager::_activeJoystickChanged(Joystick* joystick)
{
    if (_destroying || _epochExhausted || QThread::currentThread() != thread()) return;
    const bool eligible = _sourceCurrent() && _vehicleReadyState && _joystickManager
        && _multiVehicleManager->parameterReadyVehicleAvailable()
        && _multiVehicleManager->activeVehicle() == _vehicle.data()
        && _joystickManager->activeJoystick() == joystick;
    const QPointer<Joystick> requested = eligible ? joystick : nullptr;
    if (_activeJoystick == requested && (requested ? !_joystickConnections.isEmpty() : _joystickConnections.isEmpty())) return;
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    // Retire before disconnectNotify can reenter with a different source.
    _activeJoystick.clear();
    const auto oldConnections = _joystickConnections;
    _joystickConnections.clear();
    for (const auto& connection : oldConnections) {
        disconnect(connection);
        // Continue exact old-connection cleanup after a nested newer binding;
        // never abandon old same-joystick slots merely because intent changed.
        if (!self) return;
    }
    if (!_contextCurrent(epoch)) return;
    if (!requested || !_vehicleReadyState || !_joystickManager
        || _multiVehicleManager->activeVehicle() != _vehicle.data()
        || _joystickManager->activeJoystick() != requested.data()) return;
    QList<QMetaObject::Connection> staged;
    const auto cleanup = qScopeGuard([&staged]() { for (const auto& connection : staged) QObject::disconnect(connection); });
    const auto add = [&](const QMetaObject::Connection& connection) {
        staged.append(connection);
        return bool(connection) && self && _contextCurrent(epoch) && requested
            && _vehicleReadyState && _joystickManager
            && _multiVehicleManager->activeVehicle() == _vehicle.data()
            && _joystickManager->activeJoystick() == requested.data();
    };
    if (!add(connect(requested.data(), &Joystick::stepZoom, this, &QGCCameraManager::_stepZoom))
        || !add(connect(requested.data(), &Joystick::startContinuousZoom, this, &QGCCameraManager::_startZoom))
        || !add(connect(requested.data(), &Joystick::stopContinuousZoom, this, &QGCCameraManager::_stopZoom))
        || !add(connect(requested.data(), &Joystick::stepCamera, this, &QGCCameraManager::_stepCamera))
        || !add(connect(requested.data(), &Joystick::stepStream, this, &QGCCameraManager::_stepStream))
        || !add(connect(requested.data(), &Joystick::triggerCamera, this, &QGCCameraManager::_triggerCamera))
        || !add(connect(requested.data(), &Joystick::startVideoRecord, this, &QGCCameraManager::_startVideoRecording))
        || !add(connect(requested.data(), &Joystick::stopVideoRecord, this, &QGCCameraManager::_stopVideoRecording))
        || !add(connect(requested.data(), &Joystick::toggleVideoRecord, this, &QGCCameraManager::_toggleVideoRecording))) return;
    _activeJoystick = requested;
    _joystickConnections = staged;
    staged.clear();
}

void QGCCameraManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (!_sourceCurrent() || message.sysid != _vehicleId
        || message.compid < MAV_COMP_ID_CAMERA || message.compid > MAV_COMP_ID_CAMERA6) return;
    // This remains Vehicle's adjusted message cache, not RAW PRIMARY evidence.
    switch (message.msgid) {
    case MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS: _handleCaptureStatus(message); break;
    case MAVLINK_MSG_ID_STORAGE_INFORMATION: _handleStorageInfo(message); break;
    case MAVLINK_MSG_ID_HEARTBEAT: _handleHeartbeat(message); break;
    case MAVLINK_MSG_ID_CAMERA_INFORMATION: _handleCameraInfo(message); break;
    case MAVLINK_MSG_ID_CAMERA_SETTINGS: _handleCameraSettings(message); break;
    case MAVLINK_MSG_ID_PARAM_EXT_ACK: _handleParamAck(message); break;
    case MAVLINK_MSG_ID_PARAM_EXT_VALUE: _handleParamValue(message); break;
    case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION: _handleVideoStreamInfo(message); break;
    case MAVLINK_MSG_ID_VIDEO_STREAM_STATUS: _handleVideoStreamStatus(message); break;
    case MAVLINK_MSG_ID_BATTERY_STATUS: _handleBatteryStatus(message); break;
    }
}

void QGCCameraManager::_handleHeartbeat(const mavlink_message_t& message)
{
    if (!_sourceCurrent() || _structuralBusy) return;
    const QString key = QString::number(message.compid);
    QPointer<CameraStruct> request = _cameraInfoRequest.value(key);
    if (!request) {
        if (!_advanceCameraEpoch()) return;
        const quint64 epoch = cameraSelectionEpoch();
        const QPointer<QGCCameraManager> self(this);
        _structuralBusy = true;
        {
            const auto release = qScopeGuard([self]() {
                if (self) { self->_structuralBusy = false; self->_scheduleDeadCameras(); }
            });
            request = new CameraStruct(nullptr, message.compid);
            request->lastHeartbeat.start();
            request->lastRequest.start();
            request->setParent(this);
            if (!self || !_contextCurrent(epoch) || !request || request->parent() != this || _cameraInfoRequest.value(key)) {
                if (request) request->deleteLater();
                return;
            }
            _cameraInfoRequest.insert(key, request);
        }
        _requestCameraInfo(message.compid);
        return;
    }
    if (request->retired || request->compID != message.compid) return;
    request->lastHeartbeat.restart();
    if (request->infoReceived || request->gaveUp || !request->lastRequest.isValid()
        || request->lastRequest.elapsed() <= 2000) return;
    // Original initial request + eleven retries, with an independent 2s gap.
    // Repeated heartbeats cannot reset either the gap or the retry budget.
    if (request->tryCount > 10) { request->gaveUp = true; return; }
    ++request->tryCount;
    request->lastRequest.restart();
    _requestCameraInfo(message.compid);
}

QGCCameraControl* QGCCameraManager::currentCameraInstance()
{
    if (!_sourceCurrent() || _currentCamera < 0 || _currentCamera >= _cameraControls.size()
        || _currentCamera >= _cameras.count()) return nullptr;
    const auto camera = _knownCamera(_cameraControls.at(_currentCamera).data());
    return camera && _cameras.get(_currentCamera) == camera.data() ? camera.data() : nullptr;
}

QGCVideoStreamInfo* QGCCameraManager::_streamInstance(bool thermal)
{
    if (!_sourceCurrent()) return nullptr;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    const QPointer<QGCCameraControl> originalSelection = _currentCamera >= 0 && _currentCamera < _cameraControls.size()
        ? _knownCamera(_cameraControls.at(_currentCamera).data()) : QPointer<QGCCameraControl>();
    if (!originalSelection) return nullptr;
    const quint64 streamEpoch = originalSelection->streamSelectionEpoch();
    if (!streamEpoch) return nullptr;
    QGCCameraControl* returned = currentCameraInstance();
    if (!self || !_contextCurrent(epoch)) return nullptr;
    const QPointer<QGCCameraControl> camera = _knownCamera(returned);
    if (!camera || camera != originalSelection || camera->streamSelectionEpoch() != streamEpoch) return nullptr;
    QGCVideoStreamInfo* stream = thermal ? camera->thermalStreamInstance() : camera->currentStreamInstance();
    if (!self || !_contextCurrent(epoch) || !camera || camera->streamSelectionEpoch() != streamEpoch) return nullptr;
    // The default factory owns stream-info objects as camera children. Do not
    // repin an arbitrary raw virtual return after its callback boundary.
    const auto children = camera->findChildren<QGCVideoStreamInfo*>(QString(), Qt::FindDirectChildrenOnly);
    for (QGCVideoStreamInfo* child : children) if (child == stream) return child;
    return nullptr;
}

QGCVideoStreamInfo* QGCCameraManager::currentStreamInstance() { return _streamInstance(false); }
QGCVideoStreamInfo* QGCCameraManager::thermalStreamInstance() { return _streamInstance(true); }

QGCCameraControl* QGCCameraManager::_findCamera(int component)
{
    if (!_sourceCurrent()) return nullptr;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    const auto cameras = _cameraControls;
    for (const auto& camera : cameras) if (camera && _knownCamera(camera.data()) && _cameras.indexOf(camera.data()) >= 0) {
        const int actualComponent = camera->compID();
        if (!self || !_contextCurrent(epoch) || !camera) return nullptr;
        if (actualComponent == component) return camera.data();
    }
    return nullptr;
}

void QGCCameraManager::_handleCameraInfo(const mavlink_message_t& message)
{
    if (!_sourceCurrent() || _structuralBusy) return;
    const QString key = QString::number(message.compid);
    const QPointer<CameraStruct> request = _cameraInfoRequest.value(key);
    if (!request || request->retired || request->infoReceived || request->factoryPending || request->compID != message.compid) return;
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    const QPointer<Vehicle> vehicle = _vehicle;
    const QPointer<FirmwarePlugin> plugin = _firmwarePlugin;
    _structuralBusy = true;
    const auto release = qScopeGuard([self]() {
        if (self) { self->_structuralBusy = false; self->_scheduleDeadCameras(); }
    });
    request->factoryPending = true;
    const auto factoryFinished = qScopeGuard([self, request, key]() {
        if (self && request && self->_cameraInfoRequest.value(key) == request) request->factoryPending = false;
    });
    bool accepted = false;
    mavlink_camera_information_t info{};
    mavlink_msg_camera_information_decode(&message, &info);
    QGCCameraControl* returned = plugin->createCameraControl(&info, vehicle.data(), message.compid, this);
    if (!self) return;
    QPointer<QGCCameraControl> camera;
    const auto children = findChildren<QGCCameraControl*>(QString(), Qt::FindDirectChildrenOnly);
    for (QGCCameraControl* actual : children) if (actual == returned) { camera = actual; break; }
    const auto same = [&]() { return self && _contextCurrent(epoch) && vehicle && _vehicle == vehicle && plugin
        && _firmwarePlugin == plugin && request && !request->retired && request->factoryPending
        && request->infoReceived == accepted
        && request->compID == message.compid && _cameraInfoRequest.value(key) == request; };
    if (!same()) { if (camera && !_knownCamera(camera.data())) camera->deleteLater(); return; }
    if (!camera || _knownCamera(camera.data())) return;
    const int component = camera->compID();
    if (!same() || !camera || component != message.compid) { if (camera) camera->deleteLater(); return; }
    const QString label = camera->modelName();
    if (!same() || !camera) { if (camera) camera->deleteLater(); return; }
    QQmlEngine::setObjectOwnership(camera.data(), QQmlEngine::CppOwnership);
    request->camera = camera;
    request->cameraIdentity = camera.data();
    const auto identity = camera.data();
    const auto destroyedConnection = connect(camera.data(), &QObject::destroyed, this, [self, request, key, identity]() {
        if (self) self->_cameraDestroyed(key, request, identity);
    });
    if (!same() || !camera || !destroyedConnection) { if (camera) camera->deleteLater(); return; }
    accepted = true;
    request->infoReceived = true;
    _cameraControls.append(camera);
    _cameraIdentities.append(identity);
    _cameraLabels.append(label);
    _cameras.append(camera.data());
    if (!same() || !camera || _cameras.indexOf(camera.data()) < 0) return;
    emit camerasChanged();
    if (!same() || !camera) return;
    emit cameraLabelsChanged();
}

void QGCCameraManager::_cameraDestroyed(const QString& key, const QPointer<CameraStruct>& request,
                                       QGCCameraControl* identity)
{
    if (_destroying || !request || request->cameraIdentity != identity || _cameraInfoRequest.value(key) != request) return;
    request->retired = true;
    _advanceCameraEpoch();
    // Tombstone synchronously, even inside an insertion/removal notification.
    // Only compare the old address; never construct a QPointer or call through it.
    auto* const rows = _cameras.objectList();
    // Native lists may already be staged ahead of beginRemoveRows. Match the
    // actual raw rows by this exact published address, not a parallel index.
    for (int index = 0; index < rows->size(); ++index)
        if (rows->at(index) == identity) (*rows)[index] = nullptr;
    if (_structuralBusy) return; // the owning scope schedules after model return
    _retireCameras(true);
}

void QGCCameraManager::_scheduleDeadCameras()
{
    if (_destroying || _structuralBusy || _deadRetirementQueued) return;
    bool pending = false;
    for (auto iterator = _cameraInfoRequest.constBegin(); iterator != _cameraInfoRequest.constEnd(); ++iterator) {
        const auto request = iterator.value();
        if (request && request->retired && request->cameraIdentity && !request->camera) { pending = true; break; }
    }
    if (!pending) return;
    _deadRetirementQueued = true;
    const QPointer<QGCCameraManager> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self) return;
        self->_deadRetirementQueued = false;
        if (self->_structuralBusy) return; // scope exit, not a zero-delay busy loop
        self->_retireCameras(true);
    });
}

void QGCCameraManager::_cameraTimeout() { _retireCameras(false); }

void QGCCameraManager::_retireCameras(bool deadOnly)
{
    if (_destroying || _structuralBusy || QThread::currentThread() != thread()) return;
    const bool sourceAvailable = _sourceCurrent();
    const QPointer<QGCCameraManager> self(this);
    const auto requests = _cameraInfoRequest;
    for (auto iterator = requests.constBegin(); iterator != requests.constEnd(); ++iterator) {
        const QString key = iterator.key();
        const QPointer<CameraStruct> request = iterator.value();
        const bool deadPublishedCamera = request && request->cameraIdentity && !request->camera;
        if (!request || _cameraInfoRequest.value(key) != request
            || (deadOnly && !deadPublishedCamera)
            || (!sourceAvailable && !deadPublishedCamera)
            || (!request->retired && (!request->lastHeartbeat.isValid() || request->lastHeartbeat.elapsed() <= 5000))) continue;
        if (!_advanceCameraEpoch() && !deadPublishedCamera) return;
        const quint64 epoch = _cameraEpoch;
        _structuralBusy = true;
        const auto release = qScopeGuard([self]() {
            if (self) { self->_structuralBusy = false; self->_scheduleDeadCameras(); }
        });
        // Retired requests remain inert if a virtual callback changes intent;
        // a later tick may finish that same retirement, never a newer request.
        request->retired = true;
        const QPointer<QGCCameraControl> camera = request->camera;
        const QPointer<QGCCameraControl> previousSelection = _currentCamera >= 0 && _currentCamera < _cameraControls.size()
            ? _cameraControls.at(_currentCamera) : QPointer<QGCCameraControl>();
        bool autoStream = false;
        if (camera) {
            autoStream = camera->autoStream();
            if (!self || !_contextCurrent(epoch) || !request || _cameraInfoRequest.value(key) != request) return;
        }
        _cameraInfoRequest.remove(key);
        const auto disposeRetired = qScopeGuard([camera, request]() {
            if (camera) camera->deleteLater();
            if (request) request->deleteLater();
        });
        const auto identity = request->cameraIdentity;
        const int knownIndex = identity ? _cameraIdentities.indexOf(identity) : -1;
        const int index = knownIndex >= 0 && knownIndex < _cameras.count()
            && (_cameras.get(knownIndex) == identity || !_cameras.get(knownIndex)) ? knownIndex : -1;
        if (knownIndex >= 0) {
            _cameraControls.removeAt(knownIndex);
            _cameraIdentities.removeAt(knownIndex);
            if (knownIndex < _cameraLabels.size()) _cameraLabels.removeAt(knownIndex);
        }
        const int survivingSelection = previousSelection ? _cameraControls.indexOf(previousSelection) : -1;
        _currentCamera = survivingSelection >= 0 ? survivingSelection : 0;
        if (index >= 0) {
            // removeAt would inspect/re-pin a raw dead QObject. removeRow only
            // removes the exact native row, including its null tombstone.
            _cameras.removeRow(index);
            if (!self) return;
            if (_cameraEpoch != epoch || _destroying) return;
            _cameras.setDirty(true);
        }
        if (!self || !_contextCurrent(epoch)) return;
        // An expired unanswered/failed discovery has no model selection to
        // publish and must not restart another camera's existing stream.
        if (!camera && index < 0) return;
        emit cameraLabelsChanged();
        if (!self || !_contextCurrent(epoch)) return;
        emit camerasChanged();
        if (!self || !_contextCurrent(epoch)) return;
        emit currentCameraChanged();
        if (!self || !_contextCurrent(epoch)) return;
        if (_cameras.count() || autoStream) emit streamChanged();
        return;
    }
}

// A virtual lookup may delete/change the manager. Resolve its return against
// persistent native pointers, then make one terminal call with no old tail.
#define CAMERA_MESSAGE_HANDLER(method, type, decode, action) \
void QGCCameraManager::method(const mavlink_message_t& message) { \
    if (!_sourceCurrent()) return; \
    const quint64 epoch = cameraSelectionEpoch(); \
    const QPointer<QGCCameraManager> self(this); \
    QGCCameraControl* returned = _findCamera(message.compid); \
    if (!self || !_contextCurrent(epoch)) return; \
    const QPointer<QGCCameraControl> camera = _knownCamera(returned); \
    if (!camera) return; \
    type value{}; decode(&message, &value); \
    camera->action; \
}
CAMERA_MESSAGE_HANDLER(_handleCaptureStatus, mavlink_camera_capture_status_t, mavlink_msg_camera_capture_status_decode, handleCaptureStatus(value))
CAMERA_MESSAGE_HANDLER(_handleStorageInfo, mavlink_storage_information_t, mavlink_msg_storage_information_decode, handleStorageInfo(value))
CAMERA_MESSAGE_HANDLER(_handleCameraSettings, mavlink_camera_settings_t, mavlink_msg_camera_settings_decode, handleSettings(value))
CAMERA_MESSAGE_HANDLER(_handleParamAck, mavlink_param_ext_ack_t, mavlink_msg_param_ext_ack_decode, handleParamAck(value))
CAMERA_MESSAGE_HANDLER(_handleParamValue, mavlink_param_ext_value_t, mavlink_msg_param_ext_value_decode, handleParamValue(value))
CAMERA_MESSAGE_HANDLER(_handleVideoStreamInfo, mavlink_video_stream_information_t, mavlink_msg_video_stream_information_decode, handleVideoInfo(&value))
CAMERA_MESSAGE_HANDLER(_handleVideoStreamStatus, mavlink_video_stream_status_t, mavlink_msg_video_stream_status_decode, handleVideoStatus(&value))
CAMERA_MESSAGE_HANDLER(_handleBatteryStatus, mavlink_battery_status_t, mavlink_msg_battery_status_decode, handleBatteryStatus(value))
#undef CAMERA_MESSAGE_HANDLER

void QGCCameraManager::_requestCameraInfo(int component)
{
    if (!_sourceCurrent()) return;
    const QPointer<CameraStruct> request = _cameraInfoRequest.value(QString::number(component));
    if (!request || request->retired || request->infoReceived || request->gaveUp || request->compID != component) return;
    _vehicle->sendMavCommand(component, MAV_CMD_REQUEST_CAMERA_INFORMATION, false, 1);
}

QPointer<QGCCameraControl> QGCCameraManager::_inputCamera(quint64 epoch)
{
    if (!_contextCurrent(epoch) || !_joystickCurrent() || (sender() && sender() != _activeJoystick.data())) return {};
    const QPointer<QGCCameraManager> self(this);
    const QPointer<Joystick> joystick = _activeJoystick;
    QGCCameraControl* returned = currentCameraInstance();
    if (!self || !_contextCurrent(epoch) || !joystick || _activeJoystick != joystick || !_joystickCurrent()) return {};
    return _knownCamera(returned);
}

#define CAMERA_INPUT_HANDLER(method, action) \
void QGCCameraManager::method() { \
    if (!_advanceCameraEpoch()) return; \
    const quint64 epoch = cameraSelectionEpoch(); \
    const QPointer<QGCCameraManager> self(this); \
    const QPointer<QGCCameraControl> camera = _inputCamera(epoch); \
    if (!self || !_contextCurrent(epoch) || !camera) return; \
    camera->action; \
}
CAMERA_INPUT_HANDLER(_triggerCamera, takePhoto())
CAMERA_INPUT_HANDLER(_startVideoRecording, startVideo())
CAMERA_INPUT_HANDLER(_stopVideoRecording, stopVideo())
CAMERA_INPUT_HANDLER(_toggleVideoRecording, toggleVideo())
CAMERA_INPUT_HANDLER(_stopZoom, stopZoom())
#undef CAMERA_INPUT_HANDLER

void QGCCameraManager::_stepZoom(int direction)
{
    if (!_joystickCurrent() || _lastZoomChange.elapsed() <= 40) return;
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    _lastZoomChange.restart();
    const QPointer<QGCCameraControl> camera = _inputCamera(epoch);
    if (!self || !_contextCurrent(epoch) || !camera) return;
    camera->stepZoom(direction);
}

void QGCCameraManager::_startZoom(int direction)
{
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    const QPointer<QGCCameraControl> camera = _inputCamera(epoch);
    if (!self || !_contextCurrent(epoch) || !camera) return;
    camera->startZoom(direction);
}

void QGCCameraManager::_stepCamera(int direction)
{
    if (!_joystickCurrent() || (sender() && sender() != _activeJoystick.data()) || _lastCameraChange.elapsed() <= 1000) return;
    _lastCameraChange.restart();
    const qint64 next = qint64(_currentCamera) + direction;
    const int selection = next < 0 ? _cameras.count() - 1 : (next >= _cameras.count() ? 0 : int(next));
    setCurrentCamera(selection);
}

void QGCCameraManager::_stepStream(int direction)
{
    if (!_joystickCurrent() || _lastCameraChange.elapsed() <= 1000) return;
    if (!_advanceCameraEpoch()) return;
    const quint64 epoch = cameraSelectionEpoch();
    const QPointer<QGCCameraManager> self(this);
    _lastCameraChange.restart();
    const QPointer<QGCCameraControl> camera = _inputCamera(epoch);
    if (!self || !_contextCurrent(epoch) || !camera) return;
    const quint64 streamEpoch = camera->streamSelectionEpoch();
    if (!streamEpoch) return;
    const QPointer<QmlObjectListModel> streams(camera->streamModelNative());
    if (!streams) return;
    const int current = camera->currentStream();
    if (!self || !_contextCurrent(epoch) || !camera || !streams
        || camera->streamSelectionEpoch() != streamEpoch || !_joystickCurrent()) return;
    QmlObjectListModel* returned = camera->streams();
    if (!self || !_contextCurrent(epoch) || !camera || !streams || camera->streamSelectionEpoch() != streamEpoch
        || !_joystickCurrent() || returned != streams.data()) return;
    const QStringList labels = camera->streamLabels();
    if (!self || !_contextCurrent(epoch) || !camera || !streams || camera->streamSelectionEpoch() != streamEpoch
        || camera->streamModelNative() != streams.data() || !_joystickCurrent()) return;
    // currentStream indexes ordinary labels, not the raw model's thermal rows.
    const int count = labels.size();
    if (count <= 0) return;
    const qint64 next = qint64(current) + direction;
    const int selection = next < 0 ? count - 1 : (next >= count ? 0 : int(next));
    camera->setCurrentStream(selection);
}

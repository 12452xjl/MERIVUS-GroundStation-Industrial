/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QUrl>
#include <QDir>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QThread>
#include <limits>
#include "VideoSettings.h"

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

#include "ScreenToolsController.h"
#include "VideoManager.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "MultiVehicleManager.h"
#include "Settings/SettingsManager.h"
#include "Vehicle.h"
#include "QGCCameraManager.h"

#if defined(QGC_GST_STREAMING)
#include "GStreamer.h"
#include "VideoSettings.h"
#else
#include "GLVideoItemStub.h"
#endif

#ifdef QGC_GST_TAISYNC_ENABLED
#include "TaisyncHandler.h"
#endif

QGC_LOGGING_CATEGORY(VideoManagerLog, "VideoManagerLog")

#if defined(QGC_GST_STREAMING)
static const char* kFileExtension[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN] = {
    "mkv",
    "mov",
    "mp4"
};
#endif

struct VideoManager::StreamSelection {
    QPointer<Vehicle> vehicle;
    QPointer<QGCCameraManager> manager;
    QPointer<QGCCameraControl> camera;
    QPointer<QGCVideoStreamInfo> stream;
    quint64 binding = 0, cameraEpoch = 0, streamEpoch = 0;
    bool hadVehicle = false, hadManager = false, hadCamera = false, hadStream = false;
    bool valid = true;
};

bool VideoManager::_advancePlaybackEpoch()
{
    if (_shuttingDown || _bindingExhausted || _playbackEpoch == std::numeric_limits<quint64>::max()) {
        _videoRequested = false;
        return false;
    }
    ++_playbackEpoch;
    // Only an explicit local/source/settings intent renews the bounded retry
    // budget. A successful start followed by another failure does not.
    for (unsigned id = 0; id < 2; ++id) { _receiverRetry[id].stop(); _receiverRetries[id] = 0; }
    return true;
}

bool VideoManager::_selectionCurrent(const StreamSelection& s) const
{
    if (!s.valid || _shuttingDown || _bindingExhausted || s.binding != _bindingEpoch || s.vehicle != _activeVehicle
        || s.manager != _activeCameraManager || s.hadVehicle != bool(s.vehicle)
        || s.hadManager != bool(s.manager) || s.hadCamera != bool(s.camera) || s.hadStream != bool(s.stream)) return false;
    if (!s.manager) return true; // Explicit manual source, no discovered camera.
    if (!s.cameraEpoch || s.manager->cameraSelectionEpoch() != s.cameraEpoch) return false;
    if (!s.camera) return true;
    return s.camera->parent() == s.manager.data() && s.streamEpoch
        && s.camera->streamSelectionEpoch() == s.streamEpoch
        && (!s.stream || s.stream->parent() == s.camera.data());
}

VideoManager::StreamSelection VideoManager::_captureStream(bool thermal)
{
    const QPointer<VideoManager> self(this);
    StreamSelection s;
    if (_bindingExhausted || _shuttingDown) { s.valid = false; return s; }
    s.binding = _bindingEpoch; s.vehicle = _activeVehicle; s.manager = _activeCameraManager;
    s.hadVehicle = bool(s.vehicle); s.hadManager = bool(s.manager);
    if (!s.manager) return s;
    s.cameraEpoch = s.manager->cameraSelectionEpoch();
    if (!s.cameraEpoch) { s.valid = false; return s; }
    QGCCameraControl* returnedCamera = s.manager->currentCameraInstance();
    if (!self || !_selectionCurrent(s)) { s.valid = false; return s; }
    if (!returnedCamera) return s;
    // Compare the returned address with actual live children before creating
    // a weak QObject pin; never reconstruct one from a dead virtual result.
    const auto cameras = s.manager->findChildren<QGCCameraControl*>(QString(), Qt::FindDirectChildrenOnly);
    for (QGCCameraControl* actual : cameras) if (actual == returnedCamera) { s.camera = actual; break; }
    if (!s.camera) { s.valid = false; return s; }
    s.hadCamera = true;
    s.streamEpoch = s.camera->streamSelectionEpoch();
    if (!_selectionCurrent(s)) { s.valid = false; return s; }
    QGCVideoStreamInfo* returnedStream = thermal ? s.camera->thermalStreamInstance() : s.camera->currentStreamInstance();
    if (!self || !_selectionCurrent(s)) { s.valid = false; return s; }
    if (!returnedStream) return s;
    const auto streams = s.camera->findChildren<QGCVideoStreamInfo*>(QString(), Qt::FindDirectChildrenOnly);
    for (QGCVideoStreamInfo* actual : streams) if (actual == returnedStream) { s.stream = actual; break; }
    s.hadStream = bool(s.stream);
    if (!s.stream || !_selectionCurrent(s)) s.valid = false;
    return s;
}

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
#if !defined(QGC_GST_STREAMING)
    static bool once = false;
    if (!once) {
        qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.GLVideoItem", 1, 0, "GstGLVideoItem");
        once = true;
    }
#endif
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    _shuttingDown = true;
    _videoRequested = false;
    for (int i = 0; i < 2; i++) {
        _receiverRetry[i].stop();
        if (_videoReceiver[i] != nullptr) {
            const QPointer<VideoReceiver> receiver(_videoReceiver[i]);
            _videoReceiver[i].clear();
            delete receiver.data();
        }
#if defined(QGC_GST_STREAMING)
        if (_videoSink[i] != nullptr) {
            // FIXME: AV: we need some interaface for video sink with .release() call
            // Currently VideoManager is destroyed after corePlugin() and we are crashing on app exit
            // calling qgcApp()->toolbox()->corePlugin()->releaseVideoSink(_videoSink[i]);
            // As for now let's call GStreamer::releaseVideoSink() directly
            GStreamer::releaseVideoSink(_videoSink[i]);
            _videoSink[i] = nullptr;
        }
#endif
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
    if (_toolboxConfigured || _shuttingDown || !toolbox) return;
    _toolboxConfigured = true;
    const QPointer<VideoManager> self(this);
    const QPointer<QGCToolbox> actualToolbox(toolbox);
    _videoToolbox = actualToolbox;
    _videoPlugin = toolbox->corePlugin(); // Pin once, before a factory or notification.
    QGCTool::setToolbox(toolbox);
    if (!self || !actualToolbox) return;
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<VideoManager>("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
    qmlRegisterUncreatableType<VideoReceiver>("QGroundControl", 1, 0, "VideoReceiver", "Reference only");
    _videoSettings = toolbox->settingsManager()->videoSettings();
    _vehicleManager = toolbox->multiVehicleManager();
    if (!_videoSettings || !_vehicleManager) return;
    const QPointer<VideoSettings> settings(_videoSettings);
    const QPointer<MultiVehicleManager> vehicles(_vehicleManager);
    QList<QMetaObject::Connection> staged;
    bool adopted = false;
    const auto cleanup = qScopeGuard([&]() {
        if (!adopted) for (const auto& connection : staged) QObject::disconnect(connection);
    });
    const auto add = [&](const QMetaObject::Connection& connection) {
        staged.append(connection);
        return bool(connection) && self && actualToolbox && settings && vehicles && !self->_shuttingDown
            && self->_videoToolbox == actualToolbox && self->_videoSettings == settings && self->_vehicleManager == vehicles;
    };
    if (!add(connect(settings->videoSource(), &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged))) return;
    if (!add(connect(settings->udpPort(), &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged))) return;
    if (!add(connect(settings->rtspUrl(), &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged))) return;
    if (!add(connect(settings->tcpUrl(), &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged))) return;
    if (!add(connect(settings->aspectRatio(), &Fact::rawValueChanged, this, &VideoManager::_aspectRatioChanged))) return;
    if (!add(connect(settings->lowLatencyMode(), &Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged))) return;
    if (!add(connect(settings->streamEnabled(), &Fact::rawValueChanged, this, [self]() {
        if (!self) return;
        if (self->_advancePlaybackEpoch()) self->_restartAllVideos();
        else if (self) self->stopVideo();
    }))) return;
    if (!add(connect(vehicles.data(), &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle))) return;
    adopted = true;
#if defined(QGC_GST_STREAMING)
    GStreamer::blacklist(static_cast<VideoSettings::VideoDecoderOptions>(_videoSettings->forceVideoDecoder()->rawValue().toInt()));
#ifndef QGC_DISABLE_UVC
    _updateUVC();
    if (!self || !actualToolbox) return;
#endif
    emit isGStreamerChanged();
    if (!self || !actualToolbox) return;
    const QPointer<QGCCorePlugin> plugin(_videoPlugin);
    for (unsigned id = 0; id < 2; ++id) {
        if (!plugin) break;
        VideoReceiver* returned = plugin->createVideoReceiver(this);
        if (!self || !actualToolbox || !plugin) return;
        const auto children = findChildren<VideoReceiver*>(QString(), Qt::FindDirectChildrenOnly);
        for (VideoReceiver* actual : children) if (actual == returned) { _videoReceiver[id] = actual; break; }
        _configureReceiver(id);
        if (!self || !actualToolbox) return;
    }
    _updateSettings(0);
    if (!self) return;
    _updateSettings(1);
    if (!self) return;
    const quint64 initialIntent = _playbackEpoch, initialBinding = _bindingEpoch;
    const bool useGStreamer = isGStreamer();
    if (!self || initialIntent != _playbackEpoch || initialBinding != _bindingEpoch) return;
    // Initialization is not a new explicit start intent. In particular, a
    // manual stop from an earlier notification must remain stopped.
    if (useGStreamer && _videoRequested) _restartAllVideos();
#endif
}

void VideoManager::_configureReceiver(unsigned id)
{
    if (id > 1 || !_videoReceiver[id]) return;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoReceiver> receiver(_videoReceiver[id]);
    QList<QMetaObject::Connection> staged;
    bool adopted = false;
    const auto cleanup = qScopeGuard([&]() {
        if (!adopted) for (const auto& connection : staged) QObject::disconnect(connection);
    });
    const auto add = [&](const QMetaObject::Connection& connection) {
        staged.append(connection);
        return bool(connection) && self && receiver && !self->_shuttingDown && self->_videoReceiver[id] == receiver;
    };
    _receiverRetry[id].setSingleShot(true);
    if (!add(connect(&_receiverRetry[id], &QTimer::timeout, this, [self, receiver, id]() {
        if (self && receiver && !self->_shuttingDown && self->_videoReceiver[id] == receiver) self->_restartVideo(id);
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::onStartComplete, this, [self, receiver, id](VideoReceiver::STATUS status) {
        if (self && receiver && self->_videoReceiver[id] == receiver) self->_receiverStarted(id, status);
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::onStopComplete, this, [self, receiver, id](VideoReceiver::STATUS status) {
        if (self && receiver && self->_videoReceiver[id] == receiver) self->_receiverStopped(id, status);
    }))) return;
    if (!add(connect(receiver.data(), &QObject::destroyed, this, [self, receiver, id]() {
        if (!self || self->_videoReceiver[id] != receiver) return;
        self->_startPending[id] = self->_stopPending[id] = self->_videoStarted[id] = false;
        self->_receiverRetry[id].stop();
    }))) return;
    if (id != 0) { adopted = true; return; }
    if (!add(connect(receiver.data(), &VideoReceiver::streamingChanged, this, [self, receiver](bool active) {
        if (!self || !receiver || self->_videoReceiver[0] != receiver) return;
        self->_streaming = active; emit self->streamingChanged();
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::decodingChanged, this, [self, receiver](bool active) {
        if (!self || !receiver || self->_videoReceiver[0] != receiver) return;
        self->_decoding = active; emit self->decodingChanged();
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::recordingChanged, this, [self, receiver](bool active) {
        if (!self || !receiver || self->_videoReceiver[0] != receiver) return;
        self->_recording = active;
        if (!active) self->_subtitleWriter.stopCapturingTelemetry();
        if (self && receiver && self->_videoReceiver[0] == receiver) emit self->recordingChanged();
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::recordingStarted, this, [self, receiver]() {
        if (self && receiver && self->_videoReceiver[0] == receiver) self->_subtitleWriter.startCapturingTelemetry(self->_videoFile);
    }))) return;
    if (!add(connect(receiver.data(), &VideoReceiver::videoSizeChanged, this, [self, receiver](QSize size) {
        if (!self || !receiver || self->_videoReceiver[0] != receiver) return;
        self->_videoSize = (quint32(size.width()) << 16) | (quint32(size.height()) & 0xFFFF);
        emit self->videoSizeChanged();
    }))) return;
    adopted = true;
}

void VideoManager::_receiverStarted(unsigned id, VideoReceiver::STATUS status)
{
    if (id > 1 || _shuttingDown || !_videoReceiver[id] || !_startPending[id]) return;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoReceiver> receiver(_videoReceiver[id]);
    _startPending[id] = false;
    const bool obsolete = _stopAfterStart[id] || !_videoRequested || _startBinding[id] != _bindingEpoch
        || _startIntent[id] != _playbackEpoch || _startUri[id] != _videoUri[id]
        || _startLowLatency[id] != _lowLatencyStreaming[id] || !_startSelection[id]
        || !_selectionCurrent(*_startSelection[id]);
    _stopAfterStart[id] = false;
    if (status == VideoReceiver::STATUS_OK || status == VideoReceiver::STATUS_INVALID_STATE) {
        _videoStarted[id] = true;
        if (obsolete || status == VideoReceiver::STATUS_INVALID_STATE) {
            _retryAfterStop[id] = status == VideoReceiver::STATUS_INVALID_STATE;
            _stopReceiver(id); return;
        }
        if (receiver && _videoSink[id]) receiver->startDecoding(_videoSink[id]);
        return; // No access after the receiver's virtual call.
    }
    _videoStarted[id] = false;
    if (obsolete && _videoRequested) { _scheduleRestart(id); return; }
    if (status != VideoReceiver::STATUS_INVALID_URL) _retryReceiver(id);
    Q_UNUSED(self)
}

void VideoManager::_receiverStopped(unsigned id, VideoReceiver::STATUS status)
{
    if (id > 1 || _shuttingDown || !_videoReceiver[id] || (!_stopPending[id] && !_videoStarted[id])) return;
    const bool requestedStop = _stopPending[id];
    const bool retry = _retryAfterStop[id];
    _retryAfterStop[id] = false;
    _stopPending[id] = false;
    _videoStarted[id] = false;
    // GST's watchdog can stop an actually managed stream without stop(). Its
    // tokenless completion is a local observation, not our request receipt.
    if (!requestedStop || retry) { if (_videoRequested) _retryReceiver(id); return; }
    if (status == VideoReceiver::STATUS_OK && _videoRequested) _scheduleRestart(id);
    else if (_videoRequested && status != VideoReceiver::STATUS_INVALID_URL) _retryReceiver(id);
}

void VideoManager::_retryReceiver(unsigned id)
{
    if (id > 1 || _shuttingDown || !_videoRequested || _receiverRetries[id] >= 5) return;
    ++_receiverRetries[id];
    _receiverRetry[id].start(1000); // Bounded, never a synchronous failure loop.
}

void VideoManager::_scheduleRestart(unsigned id)
{
    if (id > 1 || _shuttingDown || _restartScheduled[id]) return;
    _restartScheduled[id] = true;
    const QPointer<VideoManager> self(this);
    QTimer::singleShot(0, this, [self, id]() {
        if (!self) return;
        self->_restartScheduled[id] = false;
        if (!self->_shuttingDown) self->_restartVideo(id);
    });
}

void VideoManager::_cleanupOldVideos()
{
#if defined(QGC_GST_STREAMING)
    const QPointer<VideoManager> self(this);
    const quint64 action = _mediaActionEpoch, binding = _bindingEpoch, intent = _playbackEpoch;
    if (!_videoSettings || !_videoToolbox) return;
    //-- Only perform cleanup if storage limit is enabled
    if(!_videoSettings->enableStorageLimit()->rawValue().toBool()) {
        return;
    }
    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();
    QDir videoDir = QDir(savePath);
    videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    videoDir.setSorting(QDir::Time);

    QStringList nameFilters;

    for(size_t i = 0; i < sizeof(kFileExtension) / sizeof(kFileExtension[0]); i += 1) {
        nameFilters << QString("*.") + kFileExtension[i];
    }

    videoDir.setNameFilters(nameFilters);
    //-- get the list of videos stored
    QFileInfoList vidList = videoDir.entryInfoList();
    if(!vidList.isEmpty()) {
        uint64_t total   = 0;
        //-- Settings are stored using MB
        uint64_t maxSize = _videoSettings->maxVideoSize()->rawValue().toUInt() * 1024 * 1024;
        //-- Compute total used storage
        for(int i = 0; i < vidList.size(); i++) {
            total += vidList[i].size();
        }
        //-- Remove old movies until max size is satisfied.
        while(total >= maxSize && !vidList.isEmpty()) {
            total -= vidList.last().size();
            qCDebug(VideoManagerLog) << "Removing old video file:" << vidList.last().filePath();
            QFile file (vidList.last().filePath());
            file.remove();
            if (!self || _bindingExhausted || _mediaActionEpoch != action || _bindingEpoch != binding || _playbackEpoch != intent) return;
            vidList.removeLast();
        }
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::startVideo()
{
    if (qgcApp()->runningUnitTests() || _shuttingDown || _bindingExhausted || QThread::currentThread() != thread()) return;
    _videoRequested = true;
    if (!_advancePlaybackEpoch()) return;
    for (unsigned id = 0; id < 2; ++id) { _receiverRetry[id].stop(); _receiverRetries[id] = 0; }
    _restartAllVideos();
}

//-----------------------------------------------------------------------------
void
VideoManager::stopVideo()
{
    if (qgcApp()->runningUnitTests() || _shuttingDown || QThread::currentThread() != thread()) return;
    _videoRequested = false; // Local playback only, not a remote stream stop.
    _advancePlaybackEpoch();
    const QPointer<VideoManager> self(this);
    const quint64 intent = _playbackEpoch;
    for (int id = 1; id >= 0; --id) {
        _receiverRetry[id].stop(); _stopReceiver(unsigned(id));
        if (!self || _playbackEpoch != intent) return;
    }
}

void
VideoManager::startRecording(const QString& videoFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (_shuttingDown || _bindingExhausted || !_videoSettings || !_videoToolbox || _mediaActionEpoch == std::numeric_limits<quint64>::max()) return;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoSettings> settings(_videoSettings);
    const QPointer<QGCToolbox> toolbox(_videoToolbox);
    const QPointer<VideoReceiver> receivers[2] = {_videoReceiver[0], _videoReceiver[1]};
    const bool hadReceiver[2] = {bool(receivers[0]), bool(receivers[1])};
    const quint64 action = ++_mediaActionEpoch, binding = _bindingEpoch, intent = _playbackEpoch;
    const QString requestedFile = videoFile;
    const auto current = [&]() { return self && settings && toolbox && !self->_shuttingDown && !self->_bindingExhausted
        && self->_videoSettings == settings && self->_videoToolbox == toolbox && self->_mediaActionEpoch == action
        && self->_bindingEpoch == binding && self->_playbackEpoch == intent
        && (!hadReceiver[0] || receivers[0]) && (!hadReceiver[1] || receivers[1])
        && self->_videoReceiver[0] == receivers[0] && self->_videoReceiver[1] == receivers[1]; };
    if (!_videoReceiver[0]) {
        qgcApp()->showAppMessage(tr("Video receiver is not ready."));
        return;
    }

    const VideoReceiver::FILE_FORMAT fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_videoSettings->recordingFormat()->rawValue().toInt());

    if(fileFormat < VideoReceiver::FILE_FORMAT_MIN || fileFormat >= VideoReceiver::FILE_FORMAT_MAX) {
        qgcApp()->showAppMessage(tr("Invalid video format defined."));
        return;
    }
    QString ext = kFileExtension[fileFormat - VideoReceiver::FILE_FORMAT_MIN];

    //-- Disk usage maintenance
    _cleanupOldVideos();
    if (!current()) return;

    QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();

    if (savePath.isEmpty()) {
        qgcApp()->showAppMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    _videoFile = savePath + "/"
            + (requestedFile.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") : requestedFile)
            + ".";
    QString videoFile2 = _videoFile + "2." + ext;
    _videoFile += ext;

    const QString primaryFile = _videoFile;
    if (receivers[0] && _videoStarted[0]) {
        receivers[0]->startRecording(primaryFile, fileFormat);
        if (!current()) return;
    }
    if (receivers[1] && _videoStarted[1]) {
        receivers[1]->startRecording(videoFile2, fileFormat);
    }

#else
    Q_UNUSED(videoFile)
#endif
}

void
VideoManager::stopRecording()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (_shuttingDown || _mediaActionEpoch == std::numeric_limits<quint64>::max()) return;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoReceiver> receivers[2] = {_videoReceiver[0], _videoReceiver[1]};
    const quint64 action = ++_mediaActionEpoch, binding = _bindingEpoch, intent = _playbackEpoch;
    for (int i = 0; i < 2; i++) {
        if (receivers[i] && _videoReceiver[i] == receivers[i]) {
            receivers[i]->stopRecording();
            if (!self || _mediaActionEpoch != action || _bindingEpoch != binding || _playbackEpoch != intent) return;
        }
    }
#endif
}

void
VideoManager::grabImage(const QString& imageFile)
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    if (_shuttingDown || _bindingExhausted || !_videoReceiver[0] || _mediaActionEpoch == std::numeric_limits<quint64>::max()) {
        return;
    }
    const QPointer<VideoManager> self(this);
    const QPointer<VideoReceiver> receiver(_videoReceiver[0]);
    const quint64 action = ++_mediaActionEpoch, binding = _bindingEpoch, intent = _playbackEpoch;

    if (imageFile.isEmpty()) {
        _imageFile = qgcApp()->toolbox()->settingsManager()->appSettings()->photoSavePath();
        _imageFile += + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + ".jpg";
    } else {
        _imageFile = imageFile;
    }

    const QString filename = _imageFile;
    emit imageFileChanged();
    if (!self || _bindingExhausted || !receiver || _videoReceiver[0] != receiver || _mediaActionEpoch != action
        || _bindingEpoch != binding || _playbackEpoch != intent || _imageFile != filename) return;
    receiver->takeScreenshot(filename);
#else
    Q_UNUSED(imageFile)
#endif
}

//-----------------------------------------------------------------------------
double VideoManager::aspectRatio()
{
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(false);
    if (!self) return 1.0;
    if (_selectionCurrent(s) && s.stream) return s.stream->aspectRatio();
    return _videoSettings ? _videoSettings->aspectRatio()->rawValue().toDouble() : 1.0;
}

double VideoManager::thermalAspectRatio()
{
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(true);
    return self && _selectionCurrent(s) && s.stream ? s.stream->aspectRatio() : 1.0;
}

double VideoManager::hfov()
{
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(false);
    return self && _selectionCurrent(s) && s.stream ? s.stream->hfov() : 1.0;
}

double VideoManager::thermalHfov()
{
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(true);
    return self && _selectionCurrent(s) && s.stream ? s.stream->hfov() : 1.0;
}

bool VideoManager::hasThermal()
{
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(true);
    return self && _selectionCurrent(s) && s.stream;
}

QString VideoManager::imageFile() { return _imageFile; }

bool VideoManager::autoStreamConfigured()
{
#if defined(QGC_GST_STREAMING)
    const QPointer<VideoManager> self(this);
    const StreamSelection s = _captureStream(false);
    return self && _selectionCurrent(s) && s.stream && !s.stream->uri().isEmpty();
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_updateUVC()
{
#ifndef QGC_DISABLE_UVC
    const QPointer<VideoManager> self(this);
    const QPointer<VideoSettings> settings(_videoSettings);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    const auto current = [&]() { return self && !self->_bindingExhausted && settings && settings == _videoSettings
        && intent == _playbackEpoch && binding == _bindingEpoch; };
    if (!current()) return;
    QString oldUvcVideoSrcID = _uvcVideoSourceID;
    const bool video = hasVideo();
    if (!current()) return;
    const bool gstreamer = video ? isGStreamer() : false;
    if (!current()) return;
    if (!video || gstreamer) {
        _uvcVideoSourceID = "";
    } else {
        _uvcVideoSourceID.clear();
        QString videoSource = _videoSettings->videoSource()->rawValue().toString();
        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        if (!current()) return;
        for (const QCameraInfo &cameraInfo : cameras) {
            if (cameraInfo.description() == videoSource) {
                _uvcVideoSourceID = cameraInfo.deviceName();
                qCDebug(VideoManagerLog)
                    << "Found USB source:" << _uvcVideoSourceID << " Name:" << videoSource;
                break;
            }
        }
    }

    if (oldUvcVideoSrcID != _uvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << oldUvcVideoSrcID << "] to [" << _uvcVideoSourceID << "]";
        emit uvcVideoSourceIDChanged();
        if (!current()) return;
        emit isUvcChanged();
    }

#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_videoSourceChanged()
{
    if (!_advancePlaybackEpoch()) { stopVideo(); return; }
    const QPointer<VideoManager> self(this);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    _updateUVC();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    emit hasVideoChanged();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    emit isGStreamerChanged();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    emit isUvcChanged();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    emit isAutoStreamChanged();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    _restartAllVideos(); // Settings never clear a manual local stop.
}

//-----------------------------------------------------------------------------
void
VideoManager::_udpPortChanged()
{
    if (!_advancePlaybackEpoch()) { stopVideo(); return; }
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_rtspUrlChanged()
{
    if (!_advancePlaybackEpoch()) { stopVideo(); return; }
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_tcpUrlChanged()
{
    if (!_advancePlaybackEpoch()) { stopVideo(); return; }
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_lowLatencyModeChanged()
{
    if (!_advancePlaybackEpoch()) { stopVideo(); return; }
    _restartAllVideos();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo()
{
    const QPointer<VideoManager> self(this);
    const quint64 binding = _bindingEpoch, intent = _playbackEpoch;
    const bool automatic = autoStreamConfigured();
    if (!self || _bindingExhausted || binding != _bindingEpoch || intent != _playbackEpoch || !_videoSettings) return false;
    if(automatic) {
        return true;
    }
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return !videoSource.isEmpty() && videoSource != VideoSettings::videoSourceNoVideo && videoSource != VideoSettings::videoDisabled;
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer()
{
#if defined(QGC_GST_STREAMING)
    if (!_videoSettings || _shuttingDown || _bindingExhausted) return false;
    QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return videoSource == VideoSettings::videoSourceUDPH264 ||
            videoSource == VideoSettings::videoSourceUDPH265 ||
            videoSource == VideoSettings::videoSourceRTSP ||
            videoSource == VideoSettings::videoSourceTCP ||
            videoSource == VideoSettings::videoSourceMPEGTS ||
            videoSource == VideoSettings::videoSource3DRSolo ||
            videoSource == VideoSettings::videoSourceParrotDiscovery ||
            videoSource == VideoSettings::videoSourceYuneecMantisG ||
            videoSource == VideoSettings::videoSourceHerelinkAirUnit ||
            videoSource == VideoSettings::videoSourceHerelinkHotspot ||
            autoStreamConfigured();
#else
    return false;
#endif
}

bool
VideoManager::isUvc()
{
#ifndef QGC_DISABLE_UVC
    const QPointer<VideoManager> self(this);
    const quint64 binding = _bindingEpoch, intent = _playbackEpoch;
    const bool video = hasVideo();
    if (!self || binding != _bindingEpoch || intent != _playbackEpoch) return false;
    auto isUvc = video && !_uvcVideoSourceID.isEmpty();
    qCDebug(VideoManagerLog) << "Is Video source UVC: " << (isUvc ? "yes" : "no");
    return isUvc;
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
#ifndef QGC_DISABLE_UVC
bool
VideoManager::uvcEnabled()
{
    return QCameraInfo::availableCameras().count() > 0;
}
#endif

//-----------------------------------------------------------------------------
void
VideoManager::setfullScreen(bool f)
{
    if(f) {
        //-- No can do if no vehicle or connection lost
        if(!_activeVehicle || !_activeLinkManager || _activeLinkManager->communicationLost()) {
            f = false;
        }
    }
    _fullScreen = f;
    emit fullScreenChanged();
}

//-----------------------------------------------------------------------------
void
VideoManager::_initVideo()
{
#if defined(QGC_GST_STREAMING)
    const QPointer<VideoManager> self(this);
    if (_initializingVideo || _shuttingDown) return;
    _initializingVideo = true;
    const auto release = qScopeGuard([self]() { if (self) self->_initializingVideo = false; });
    const QPointer<QQuickWindow> window(qgcApp()->mainRootWindow());
    const QPointer<QGCToolbox> toolbox(_videoToolbox);
    const QPointer<QGCCorePlugin> plugin(_videoPlugin);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    if (!window || !toolbox || !plugin || _shuttingDown) return;
    const auto current = [&]() { return self && window && toolbox && plugin && !self->_shuttingDown && !self->_bindingExhausted
        && self->_videoToolbox == toolbox && self->_videoPlugin == plugin
        && self->_playbackEpoch == intent && self->_bindingEpoch == binding; };
    for (unsigned id = 0; id < 2; ++id) {
        if (!current()) return;
        const QPointer<VideoReceiver> receiver(_videoReceiver[id]);
        const QPointer<QQuickItem> widget(window->findChild<QQuickItem*>(id == 0 ? "videoContent" : "thermalVideo"));
        if (!receiver || !widget) continue;
        if (!_videoSink[id]) {
            void* const sink = plugin->createVideoSink(this, widget.data());
            if (!current() || !receiver || !widget || _videoReceiver[id] != receiver) {
                if (sink) GStreamer::releaseVideoSink(sink);
                return;
            }
            _videoSink[id] = sink;
        }
        if (_videoSink[id] && _videoStarted[id] && _videoRequested && _startSelection[id]
            && _selectionCurrent(*_startSelection[id])) {
            receiver->startDecoding(_videoSink[id]);
            if (!current() || !receiver || _videoReceiver[id] != receiver) return;
        }
    }
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateSettings(unsigned id)
{
    if (id > 1 || !_videoSettings || _shuttingDown || _bindingExhausted || _settingsUpdating[id]) return false;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoSettings> settings(_videoSettings);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    _settingsResolved[id] = false; _settingsUpdating[id] = true;
    const auto release = qScopeGuard([self, id]() {
        if (!self) return;
        self->_settingsUpdating[id] = false;
        if (self->_restartDeferred[id]) {
            self->_restartDeferred[id] = false;
            self->_scheduleRestart(id); // One fresh turn after the original resolver unwinds.
        }
    });
    const bool lowLatency = settings->lowLatencyMode()->rawValue().toBool();
    const StreamSelection selection = _captureStream(id == 1);
    if (!self || !settings || settings != _videoSettings || intent != _playbackEpoch || binding != _bindingEpoch
        || !_selectionCurrent(selection)) return false;
    QString uri, autoSource;
    if (selection.stream) {
        const QString declared = selection.stream->uri();
        switch (selection.stream->type()) {
        case VIDEO_STREAM_TYPE_RTSP: uri = declared; autoSource = VideoSettings::videoSourceRTSP; break;
        case VIDEO_STREAM_TYPE_TCP_MPEG: uri = declared; autoSource = VideoSettings::videoSourceTCP; break;
        case VIDEO_STREAM_TYPE_RTPUDP:
            uri = declared.startsWith(QStringLiteral("udp://")) ? declared : QStringLiteral("udp://0.0.0.0:%1").arg(declared);
            autoSource = VideoSettings::videoSourceUDPH264; break;
        case VIDEO_STREAM_TYPE_MPEG_TS_H264:
            uri = declared.startsWith(QStringLiteral("mpegts://")) ? declared : QStringLiteral("mpegts://0.0.0.0:%1").arg(declared);
            autoSource = VideoSettings::videoSourceMPEGTS; break;
        default: uri = declared; break;
        }
    } else if (id == 0) {
        const QString source = settings->videoSource()->rawValue().toString();
        if (source == VideoSettings::videoSourceUDPH264) uri = QStringLiteral("udp://0.0.0.0:%1").arg(settings->udpPort()->rawValue().toInt());
        else if (source == VideoSettings::videoSourceUDPH265) uri = QStringLiteral("udp265://0.0.0.0:%1").arg(settings->udpPort()->rawValue().toInt());
        else if (source == VideoSettings::videoSourceMPEGTS) uri = QStringLiteral("mpegts://0.0.0.0:%1").arg(settings->udpPort()->rawValue().toInt());
        else if (source == VideoSettings::videoSourceRTSP) uri = settings->rtspUrl()->rawValue().toString();
        else if (source == VideoSettings::videoSourceTCP) uri = QStringLiteral("tcp://%1").arg(settings->tcpUrl()->rawValue().toString());
        else if (source == VideoSettings::videoSource3DRSolo) uri = QStringLiteral("udp://0.0.0.0:5600");
        else if (source == VideoSettings::videoSourceParrotDiscovery) uri = QStringLiteral("udp://0.0.0.0:8888");
        else if (source == VideoSettings::videoSourceYuneecMantisG) uri = QStringLiteral("rtsp://192.168.42.1:554/live");
        else if (source == VideoSettings::videoSourceHerelinkAirUnit) uri = QStringLiteral("rtsp://192.168.0.10:8554/H264Video");
        else if (source == VideoSettings::videoSourceHerelinkHotspot) uri = QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream");
    }
    // An absent thermal source must clear the previous vehicle's URI.
    if (id == 0 && !autoSource.isEmpty() && settings->videoSource()->rawValue().toString() != autoSource) {
        settings->videoSource()->setRawValue(autoSource);
        if (!self || !settings || settings != _videoSettings || intent != _playbackEpoch || binding != _bindingEpoch
            || !_selectionCurrent(selection)) return false;
    }
    if (!self || !settings || settings != _videoSettings || intent != _playbackEpoch || binding != _bindingEpoch
        || !_selectionCurrent(selection)) return false;
    const bool latencyChanged = _lowLatencyStreaming[id] != lowLatency;
    const bool uriChanged = _updateVideoUri(id, uri);
    if (!self || !settings || settings != _videoSettings || intent != _playbackEpoch
        || binding != _bindingEpoch || !_selectionCurrent(selection)) return false;
    _lowLatencyStreaming[id] = lowLatency;
    _resolvedSelection[id] = std::make_shared<StreamSelection>(selection);
    _settingsResolved[id] = true;
    return latencyChanged || uriChanged;
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateVideoUri(unsigned id, const QString& uri)
{
    if (id > 1) return false;
    QString resolved = uri;
#if defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
    const QPointer<VideoManager> self(this);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    const bool taisync = isTaisync();
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return false;
    if (taisync) resolved = id == 0 ? QStringLiteral("tsusb://0.0.0.0:%1").arg(TAISYNC_VIDEO_UDP_PORT) : QString();
#endif
    if (resolved == _videoUri[id]) return false;
    _videoUri[id] = resolved;
    return true;
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo(unsigned id)
{
#if defined(QGC_GST_STREAMING)
    if (id > 1 || _shuttingDown || qgcApp()->runningUnitTests()) return;
    if (_settingsUpdating[id]) { _restartDeferred[id] = true; return; }
    const QPointer<VideoManager> self(this);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    _receiverRetry[id].stop();
    const QString oldUri = _videoUri[id];
    const bool oldLatency = _lowLatencyStreaming[id];
    _updateSettings(id);
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    if (!_settingsResolved[id] || !_videoRequested || !_videoSettings
        || !_videoSettings->streamEnabled()->rawValue().toBool() || _videoUri[id].isEmpty()) {
        _stopReceiver(id); return;
    }
    if (_startPending[id]) {
        if (_startUri[id] != _videoUri[id] || _startLowLatency[id] != _lowLatencyStreaming[id]
            || _startBinding[id] != binding || _startIntent[id] != intent || !_startSelection[id]
            || !_selectionCurrent(*_startSelection[id])) _stopAfterStart[id] = true;
        return;
    }
    if (_stopPending[id]) return;
    if (_videoStarted[id] && (oldUri != _videoUri[id] || oldLatency != _lowLatencyStreaming[id]
        || _startBinding[id] != binding || _startIntent[id] != intent || !_startSelection[id]
        || !_selectionCurrent(*_startSelection[id]))) _stopReceiver(id);
    else if (!_videoStarted[id]) _startReceiver(id);
#else
    Q_UNUSED(id)
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartAllVideos()
{
    const QPointer<VideoManager> self(this);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    _restartVideo(0);
    if (!self || intent != _playbackEpoch || binding != _bindingEpoch) return;
    _restartVideo(1);
}

//-----------------------------------------------------------------------------
void
VideoManager::_startReceiver(unsigned id)
{
#if defined(QGC_GST_STREAMING)
    if (id > 1 || _shuttingDown || _bindingExhausted || !_videoRequested || !_videoSettings || !_videoReceiver[id]
        || _startPending[id] || _stopPending[id] || _videoStarted[id] || !_settingsResolved[id]
        || !_resolvedSelection[id] || !_selectionCurrent(*_resolvedSelection[id]) || _videoUri[id].isEmpty()) return;
    const QPointer<VideoManager> self(this);
    const QPointer<VideoReceiver> receiver(_videoReceiver[id]);
    const QPointer<VideoSettings> settings(_videoSettings);
    const quint64 intent = _playbackEpoch, binding = _bindingEpoch;
    const auto selection = _resolvedSelection[id];
    if (!settings || !settings->streamEnabled()->rawValue().toBool()) return;
    const bool configured = settings->streamConfigured();
    if (!self || !settings || settings != _videoSettings || !configured || !receiver || _videoReceiver[id] != receiver || !_videoRequested
        || intent != _playbackEpoch || binding != _bindingEpoch || !_selectionCurrent(*selection)) return;
    const QString source = settings->videoSource()->rawValue().toString();
    const unsigned timeout = source == VideoSettings::videoSourceRTSP ? settings->rtspTimeout()->rawValue().toUInt() : 2;
    _startPending[id] = true; _stopAfterStart[id] = false;
    _startUri[id] = _videoUri[id]; _startLowLatency[id] = _lowLatencyStreaming[id];
    _startBinding[id] = binding; _startIntent[id] = intent;
    _startSelection[id] = selection;
    const QString uri = _startUri[id];
    const int buffer = _lowLatencyStreaming[id] ? -1 : 0;
    receiver->start(uri, timeout, buffer); // Local values survive a deleting callback.
#else
    Q_UNUSED(id)
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_stopReceiver(unsigned id)
{
#if defined(QGC_GST_STREAMING)
    if (id > 1 || _shuttingDown || !_videoReceiver[id] || _stopPending[id]) return;
    _receiverRetry[id].stop();
    if (_startPending[id]) { _stopAfterStart[id] = true; return; }
    if (!_videoStarted[id]) return; // Empty-URI GST stop emits no completion.
    const QPointer<VideoReceiver> receiver(_videoReceiver[id]);
    _stopPending[id] = true;
    receiver->stop();
#else
    Q_UNUSED(id)
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    if (_shuttingDown || QThread::currentThread() != thread()) return;
    if (_bindingExhausted || _bindingEpoch == std::numeric_limits<quint64>::max()) {
        // Permanently retire the old binding, not just this switch call. An
        // explicit later start must not revive old A after actual MVM moved B.
        _bindingExhausted = true;
        stopVideo();
        return;
    }
    const QPointer<VideoManager> self(this);
    const QPointer<Vehicle> requested(vehicle);
    const QPointer<MultiVehicleManager> vehicles(_vehicleManager);
    const bool hadVehicles = bool(vehicles);
    const bool hadRequested = bool(requested);
    if ((requested && !_vehicleManager) || (_vehicleManager && _vehicleManager->activeVehicle() != requested.data())) return;
    const bool differentVehicle = requested != _activeVehicle;
    const quint64 beforeBinding = _bindingEpoch, beforeIntent = _playbackEpoch;
    const QPointer<Vehicle> beforeVehicle(_activeVehicle);
    const StreamSelection old = differentVehicle ? _captureStream(false) : StreamSelection();
    if (!self || _shuttingDown || beforeBinding != _bindingEpoch || beforeIntent != _playbackEpoch
        || beforeVehicle != _activeVehicle || hadRequested != bool(requested)
        || hadVehicles != bool(vehicles) || _vehicleManager != vehicles
        || (_vehicleManager && _vehicleManager->activeVehicle() != requested.data())) return;
    const quint64 binding = ++_bindingEpoch;
    _advancePlaybackEpoch();
    const quint64 intent = _playbackEpoch;
    const auto oldConnections = _vehicleConnections; _vehicleConnections.clear();
    for (const auto& connection : oldConnections) {
        disconnect(connection);
        if (!self) return;
    }
    if (_bindingEpoch != binding || _playbackEpoch != intent || hadRequested != bool(requested)) return;
    _activeVehicle = requested; _activeCameraManager.clear(); _activeLinkManager.clear();
    for (unsigned id = 0; id < 2; ++id) { _settingsResolved[id] = false; _resolvedSelection[id].reset(); }
    if (requested) {
        const auto managers = requested->findChildren<QGCCameraManager*>(QString(), Qt::FindDirectChildrenOnly);
        for (QGCCameraManager* actual : managers) if (actual == requested->cameraManager()) { _activeCameraManager = actual; break; }
        const auto links = requested->findChildren<VehicleLinkManager*>(QString(), Qt::FindDirectChildrenOnly);
        for (VehicleLinkManager* actual : links) if (actual == requested->vehicleLinkManager()) { _activeLinkManager = actual; break; }
    }
    const QPointer<QGCCameraManager> manager(_activeCameraManager);
    const QPointer<VehicleLinkManager> links(_activeLinkManager);
    const bool hadManager = bool(manager), hadLinks = bool(links);
    const auto current = [&]() { return self && !self->_shuttingDown && !self->_bindingExhausted && self->_bindingEpoch == binding
        && self->_playbackEpoch == intent && hadRequested == bool(requested) && self->_activeVehicle == requested
        && hadVehicles == bool(vehicles) && self->_vehicleManager == vehicles
        && hadManager == bool(manager) && hadLinks == bool(links)
        && self->_activeCameraManager == manager && self->_activeLinkManager == links
        && (!self->_vehicleManager || self->_vehicleManager->activeVehicle() == requested.data()); };
    const auto add = [&](const QMetaObject::Connection& connection) {
        if (!connection || !current()) { QObject::disconnect(connection); return false; }
        _vehicleConnections.append(connection);
        return true;
    };
    if (requested) {
        const auto rebind = [self, binding]() {
            if (self && !self->_shuttingDown && self->_bindingEpoch == binding)
                self->_setActiveVehicle(self->_activeVehicle.data());
        };
        if (!add(connect(requested.data(), &QObject::destroyed, this, rebind))) return;
        if (!add(connect(requested.data(), &Vehicle::cameraManagerChanged, this, rebind))) return;
        if (links) {
            if (!add(connect(links.data(), &VehicleLinkManager::communicationLostChanged, this, [self, links, binding](bool lost) {
                if (self && links && self->_bindingEpoch == binding && self->_activeLinkManager == links)
                    self->_communicationLostChanged(lost);
            }))) return;
            if (!add(connect(links.data(), &QObject::destroyed, this, rebind))) return;
        }
        if (manager) {
            if (!add(connect(manager.data(), &QObject::destroyed, this, rebind))) return;
            if (!add(connect(manager.data(), &QGCCameraManager::streamChanged, this, [self, manager, binding]() {
                if (self && self->_bindingEpoch == binding && self->_activeCameraManager == manager
                    && self->_advancePlaybackEpoch()) self->_restartAllVideos();
            }))) return;
        }
    }
    // Preserve the existing active-vehicle auto-switch, never use a callback
    // from the old camera to choose another current vehicle or camera.
    if (!current()) return;
    if (differentVehicle && old.valid && old.camera && old.manager
        && old.hadCamera && old.hadManager && (!old.hadStream || old.stream)
        && old.camera->parent() == old.manager.data() && old.manager->cameraSelectionEpoch() == old.cameraEpoch
        && old.camera->streamSelectionEpoch() == old.streamEpoch) old.camera->stopStream();
    if (!current()) return;
    if (differentVehicle && requested && _videoRequested) {
        const StreamSelection next = _captureStream(false);
        if (!current()) return;
        if (_selectionCurrent(next) && next.camera) {
            if (next.streamEpoch == std::numeric_limits<quint64>::max()) return;
            const quint64 expectedStreamEpoch = next.streamEpoch + 1;
            next.camera->resumeStream();
            // The base resume consumes exactly one native stream intent. Do
            // not reject that own step or swallow additional callback ABA.
            if (!current() || !next.camera || !next.manager || (next.hadStream && !next.stream)
                || next.camera->parent() != next.manager.data() || next.manager->cameraSelectionEpoch() != next.cameraEpoch
                || next.camera->streamSelectionEpoch() != expectedStreamEpoch) return;
        } else if (!_selectionCurrent(next)) return;
    }
    if (!requested) {
        setfullScreen(false);
        if (!current()) return;
    }
    emit autoStreamConfiguredChanged();
    if (!current()) return;
    _restartAllVideos();
}

//-----------------------------------------------------------------------------
void
VideoManager::_communicationLostChanged(bool connectionLost)
{
    if(connectionLost) {
        //-- Disable full screen video if connection is lost
        setfullScreen(false);
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_aspectRatioChanged()
{
    emit aspectRatioChanged();
}

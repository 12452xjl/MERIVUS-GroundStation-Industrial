#include "ReviewVideoController.h"

#include "Fact.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoReceiver.h"
#include "VideoSettings.h"

#include <QSettings>
#include <QUrl>

ReviewVideoController::ReviewVideoController(QObject* parent)
    : QObject(parent)
{
    _retryTimer.setSingleShot(true);
    _retryTimer.setInterval(3000);
    connect(&_retryTimer, &QTimer::timeout, this, [this]() {
        _resetStream();
        _syncStream();
    });
}

ReviewVideoController::~ReviewVideoController()
{
    _resetStream();
}

void ReviewVideoController::setVehicle(Vehicle* vehicle)
{
    if (_vehicle == vehicle) {
        return;
    }
    _resetStream();
    if (_vehicle) {
        disconnect(_vehicle, nullptr, this, nullptr);
    }
    _vehicle = vehicle;
    _loadVehicleUrl();
    if (_vehicle) {
        connect(_vehicle, &Vehicle::vehicleUIDChanged, this, [this]() {
            const QString key = _settingsKey();
            if (!key.isEmpty()) {
                QSettings settings;
                const QString savedUrl = settings.value(key).toString();
                if (savedUrl.isEmpty() && !_rtspUrl.isEmpty()) {
                    settings.setValue(key, _rtspUrl);
                } else if (!savedUrl.isEmpty()) {
                    setRtspUrl(savedUrl);
                }
            }
        });
        connect(_vehicle, &QObject::destroyed, this, [this]() { setVehicle(nullptr); });
    }
    emit vehicleChanged();
    _syncStream();
}

void ReviewVideoController::setVideoItem(QQuickItem* item)
{
    if (_videoItem == item) {
        return;
    }
    _resetStream();
    _videoItem = item;
    emit videoItemChanged();
    _syncStream();
}

void ReviewVideoController::setRtspUrl(const QString& url)
{
    const QString trimmed = url.trimmed();
    if (_rtspUrl == trimmed) {
        return;
    }
    _resetStream();
    _rtspUrl = trimmed;
    const QString key = _settingsKey();
    if (!key.isEmpty()) {
        QSettings().setValue(key, _rtspUrl);
    }
    emit rtspUrlChanged();
    _syncStream();
}

void ReviewVideoController::setActive(bool active)
{
    if (_active == active) {
        return;
    }
    _active = active;
    if (!active) {
        _resetStream();
    } else {
        _syncStream();
    }
    emit stateChanged();
}

QString ReviewVideoController::statusText() const
{
    if (!_active) {
        return QString();
    }
    if (!_vehicle) {
        return tr("未选择无人机");
    }
    if (_rtspUrl.isEmpty()) {
        return tr("请设置此无人机的自拍杆 RTSP 地址");
    }
    const QUrl url(_rtspUrl);
    if (!url.isValid() || url.scheme().toLower() != QStringLiteral("rtsp") || url.host().isEmpty()) {
        return tr("自拍杆 RTSP 地址无效");
    }
    if (!_streamError.isEmpty()) {
        return _streamError;
    }
    return _decoding ? QString() : tr("正在连接自拍杆镜头…");
}

QString ReviewVideoController::_settingsKey() const
{
    if (!_vehicle || _vehicle->vehicleUID() == 0) {
        return QString();
    }
    return QStringLiteral("Merivus/Review/SelfieRtsp/%1").arg(_vehicle->vehicleUIDStr());
}

void ReviewVideoController::_loadVehicleUrl()
{
    const QString key = _settingsKey();
    _rtspUrl = key.isEmpty() ? QString() : QSettings().value(key).toString();
    _streamError.clear();
    emit rtspUrlChanged();
    emit stateChanged();
}

void ReviewVideoController::_syncStream()
{
    if (!_active || !_vehicle || !_videoItem || _receiver) {
        emit stateChanged();
        return;
    }
    const QUrl url(_rtspUrl);
    if (!url.isValid() || url.scheme().toLower() != QStringLiteral("rtsp") || url.host().isEmpty()) {
        emit stateChanged();
        return;
    }
    auto* toolbox = qgcApp()->toolbox();
    if (!toolbox || !toolbox->corePlugin()) {
        _streamError = tr("视频播放组件不可用");
        emit stateChanged();
        return;
    }
    _receiver = toolbox->corePlugin()->createVideoReceiver(this);
    _sink = toolbox->corePlugin()->createVideoSink(this, _videoItem);
    if (!_receiver || !_sink) {
        _resetStream();
        _streamError = tr("当前构建不支持自拍杆视频播放");
        emit stateChanged();
        return;
    }
    _streamError.clear();
    VideoReceiver* receiver = _receiver;
    connect(receiver, &VideoReceiver::onStartComplete, this, [this, receiver](VideoReceiver::STATUS status) {
        if (_receiver != receiver) {
            return;
        }
        if (status == VideoReceiver::STATUS_OK && _active && _sink) {
            receiver->startDecoding(_sink);
        } else if (status != VideoReceiver::STATUS_OK) {
            _streamError = tr("自拍杆视频连接失败，正在重试");
            _retryTimer.start();
            emit stateChanged();
        }
    });
    connect(receiver, &VideoReceiver::onStopComplete, this, [this, receiver](VideoReceiver::STATUS) {
        if (_receiver == receiver && _active) {
            _streamError = tr("自拍杆视频中断，正在重连");
            _retryTimer.start();
            emit stateChanged();
        }
    });
    connect(receiver, &VideoReceiver::onStartDecodingComplete, this, [this, receiver](VideoReceiver::STATUS status) {
        if (_receiver == receiver && status != VideoReceiver::STATUS_OK) {
            _streamError = tr("自拍杆画面解码失败，正在重试");
            _retryTimer.start();
            emit stateChanged();
        }
    });
    connect(receiver, &VideoReceiver::decodingChanged, this, [this, receiver](bool decoding) {
        if (_receiver == receiver) {
            _decoding = decoding;
            emit stateChanged();
        }
    });
    const unsigned timeout = toolbox->settingsManager()->videoSettings()->rtspTimeout()->rawValue().toUInt();
    receiver->start(_rtspUrl, timeout, 0);
    emit stateChanged();
}

void ReviewVideoController::_resetStream()
{
    _retryTimer.stop();
    if (_receiver) {
        VideoReceiver* receiver = _receiver;
        _receiver = nullptr;
        disconnect(receiver, nullptr, this, nullptr);
        delete receiver;
    }
    if (_sink) {
        qgcApp()->toolbox()->corePlugin()->releaseVideoSink(_sink);
        _sink = nullptr;
    }
    _decoding = false;
    _streamError.clear();
    emit stateChanged();
}

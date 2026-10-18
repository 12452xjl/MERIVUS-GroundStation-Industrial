#pragma once

#include <QObject>
#include <QQuickItem>
#include <QTimer>

class Vehicle;
class VideoReceiver;

class ReviewVideoController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Vehicle* vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)
    Q_PROPERTY(QQuickItem* videoItem READ videoItem WRITE setVideoItem NOTIFY videoItemChanged)
    Q_PROPERTY(QString rtspUrl READ rtspUrl WRITE setRtspUrl NOTIFY rtspUrlChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY stateChanged)
    Q_PROPERTY(bool decoding READ decoding NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)

public:
    explicit ReviewVideoController(QObject* parent = nullptr);
    ~ReviewVideoController() override;

    Vehicle* vehicle() const { return _vehicle; }
    void setVehicle(Vehicle* vehicle);
    QQuickItem* videoItem() const { return _videoItem; }
    void setVideoItem(QQuickItem* item);
    QString rtspUrl() const { return _rtspUrl; }
    void setRtspUrl(const QString& url);
    bool active() const { return _active; }
    void setActive(bool active);
    bool decoding() const { return _decoding; }
    QString statusText() const;

signals:
    void vehicleChanged();
    void videoItemChanged();
    void rtspUrlChanged();
    void stateChanged();

private:
    QString _settingsKey() const;
    void _loadVehicleUrl();
    void _syncStream();
    void _resetStream();

    Vehicle* _vehicle = nullptr;
    QQuickItem* _videoItem = nullptr;
    VideoReceiver* _receiver = nullptr;
    void* _sink = nullptr;
    QTimer _retryTimer;
    QString _rtspUrl;
    QString _streamError;
    bool _active = false;
    bool _decoding = false;
};

/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

/// @file
/// @brief  MAVLink Camera API. Camera Manager.
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>
#include "QmlObjectListModel.h"
#include "QGCCameraControl.h"

#include <QObject>
#include <QElapsedTimer>
#include <QList>
#include <QPointer>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(CameraManagerLog)

class Joystick;
class JoystickManager;
class MultiVehicleManager;
class QGCToolbox;
class FirmwarePlugin;

//-----------------------------------------------------------------------------
/// Camera Manager
class QGCCameraManager : public QObject
{
    Q_OBJECT
public:
    QGCCameraManager(Vehicle* vehicle);
    virtual ~QGCCameraManager();

    Q_PROPERTY(QmlObjectListModel*  cameras                 READ cameras                                        NOTIFY camerasChanged)
    Q_PROPERTY(QStringList          cameraLabels            READ cameraLabels                                   NOTIFY cameraLabelsChanged)
    Q_PROPERTY(QGCCameraControl*    currentCameraInstance   READ currentCameraInstance                          NOTIFY currentCameraChanged)
    Q_PROPERTY(int                  currentCamera           READ currentCamera      WRITE  setCurrentCamera     NOTIFY currentCameraChanged)

    //-- Return a list of cameras provided by this vehicle
    virtual QmlObjectListModel* cameras             () { return &_cameras; }
    //-- Camera names to show the user (for selection)
    virtual QStringList         cameraLabels        () { return _cameraLabels; }
    //-- Current selected camera
    virtual int                 currentCamera       () { return _currentCamera; }
    virtual QGCCameraControl*   currentCameraInstance();
    //-- Set current camera
    virtual void                setCurrentCamera    (int sel);
    //-- Current stream
    virtual QGCVideoStreamInfo* currentStreamInstance();
    //-- Current thermal stream
    virtual QGCVideoStreamInfo* thermalStreamInstance();

    // Native, callback-free local intent fence. Zero is permanently unavailable
    // after exhaustion/destruction; this is not a source or control permission.
    quint64 cameraSelectionEpoch() const { return _sourceCurrent() ? _cameraEpoch : 0; }

signals:
    void    camerasChanged          ();
    void    cameraLabelsChanged     ();
    void    currentCameraChanged    ();
    void    streamChanged           ();

protected slots:
    virtual void    _vehicleReady           (bool ready);
    virtual void    _mavlinkMessageReceived (const mavlink_message_t& message);
    virtual void    _activeJoystickChanged  (Joystick* joystick);
    virtual void    _stepZoom               (int direction);
    virtual void    _startZoom              (int direction);
    virtual void    _stopZoom               ();
    virtual void    _stepCamera             (int direction);
    virtual void    _stepStream             (int direction);
    virtual void    _cameraTimeout          ();
    virtual void    _triggerCamera          ();
    virtual void    _startVideoRecording    ();
    virtual void    _stopVideoRecording     ();
    virtual void    _toggleVideoRecording   ();

protected:
    virtual QGCCameraControl* _findCamera   (int id);
    virtual void    _requestCameraInfo      (int compID);
    virtual void    _handleHeartbeat        (const mavlink_message_t& message);
    virtual void    _handleCameraInfo       (const mavlink_message_t& message);
    virtual void    _handleStorageInfo      (const mavlink_message_t& message);
    virtual void    _handleCameraSettings   (const mavlink_message_t& message);
    virtual void    _handleParamAck         (const mavlink_message_t& message);
    virtual void    _handleParamValue       (const mavlink_message_t& message);
    virtual void    _handleCaptureStatus    (const mavlink_message_t& message);
    virtual void    _handleVideoStreamInfo  (const mavlink_message_t& message);
    virtual void    _handleVideoStreamStatus(const mavlink_message_t& message);
    virtual void    _handleBatteryStatus    (const mavlink_message_t& message);

protected:

    class CameraStruct : public QObject {
    public:
        CameraStruct(QObject* parent, uint8_t compID_);
        QElapsedTimer lastHeartbeat;
        QElapsedTimer lastRequest;
        bool    infoReceived = false;
        bool    gaveUp       = false;
        bool    retired      = false;
        bool    factoryPending = false;
        QPointer<QGCCameraControl> camera;
        QGCCameraControl* cameraIdentity = nullptr; // comparison only; never dereferenced/re-pinned
        int     tryCount     = 0;
        uint8_t compID       = 0;
    };

    QPointer<Vehicle>    _vehicle;
    QPointer<Joystick>   _activeJoystick;
    bool                _vehicleReadyState  = false;
    int                 _currentTask        = 0;
    QmlObjectListModel  _cameras;
    QStringList         _cameraLabels;
    int                 _currentCamera      = 0;
    QElapsedTimer       _lastZoomChange;
    QElapsedTimer       _lastCameraChange;
    QTimer              _cameraTimer;
    QMap<QString, QPointer<CameraStruct>> _cameraInfoRequest;

private:
    bool _sourceCurrent() const;
    bool _contextCurrent(quint64 epoch) const;
    bool _advanceCameraEpoch();
    void _sourceChanged();
    bool _joystickCurrent() const;
    QPointer<QGCCameraControl> _knownCamera(QGCCameraControl* camera) const;
    QPointer<QGCCameraControl> _inputCamera(quint64 epoch);
    QGCVideoStreamInfo* _streamInstance(bool thermal);
    void _cameraDestroyed(const QString& key, const QPointer<CameraStruct>& request, QGCCameraControl* identity);
    void _scheduleDeadCameras();
    void _retireCameras(bool deadOnly);

    QPointer<QGCToolbox> _toolbox;
    QPointer<MultiVehicleManager> _multiVehicleManager;
    QPointer<JoystickManager> _joystickManager;
    QPointer<FirmwarePlugin> _firmwarePlugin;
    QList<QPointer<QGCCameraControl>> _cameraControls;
    QList<QGCCameraControl*> _cameraIdentities; // parallel opaque identities, including tombstones
    QList<QMetaObject::Connection> _joystickConnections;
    int _vehicleId = 0;
    int _firmwareType = 0;
    int _vehicleType = 0;
    quint64 _cameraEpoch = 1;
    bool _epochExhausted = false;
    bool _destroying = false;
    bool _structuralBusy = false;
    bool _deadRetirementQueued = false;
};

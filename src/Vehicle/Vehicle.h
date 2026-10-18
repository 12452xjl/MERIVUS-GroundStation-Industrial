/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QGeoCoordinate>
#include <QTime>
#include <QQueue>
#include <QMap>
#include <QSharedPointer>
#include <QPointer>
#include <QVector>
#include <functional>
#include <memory>

#include "FactGroup.h"
#include "QGCMAVLink.h"
#include "VehicleTelemetryObserver.h"
#include "VehicleMissionExecutionObserver.h"
#include "QmlObjectListModel.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkStreamConfig.h"
#include "UASMessageHandler.h"
#include "SettingsFact.h"
#include "QGCMapCircle.h"
#include "TerrainFactGroup.h"
#include "SysStatusSensorInfo.h"
#include "VehicleClockFactGroup.h"
#include "VehicleDistanceSensorFactGroup.h"
#include "VehicleLocalPositionFactGroup.h"
#include "VehicleLocalPositionSetpointFactGroup.h"
#include "VehicleWindFactGroup.h"
#include "VehicleGPSFactGroup.h"
#include "VehicleGPS2FactGroup.h"
#include "VehicleSetpointFactGroup.h"
#include "VehicleTemperatureFactGroup.h"
#include "VehicleVibrationFactGroup.h"
#include "VehicleEscStatusFactGroup.h"
#include "VehicleFtcStatusFactGroup.h"
#include "VehicleEstimatorStatusFactGroup.h"
#include "VehicleHygrometerFactGroup.h"
#include "VehicleLinkManager.h"
#include "MissionManager.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "FTPManager.h"
#include "ImageProtocolManager.h"
#include "HealthAndArmingCheckReport.h"
#include "TerrainQuery.h"
#include "StandardModes.h"
#include "VehicleGeneratorFactGroup.h"
#include "VehicleEFIFactGroup.h"

class Actuators;
class EventHandler;
class UAS;
class UASInterface;
class FirmwarePlugin;
class QGCCorePlugin;
class FirmwarePluginManager;
class AutoPilotPlugin;
class ParameterManager;
class JoystickManager;
class UASMessage;
class SettingsManager;
class QGCCameraManager;
class Joystick;
class VehicleObjectAvoidance;
class TrajectoryPoints;
class TerrainProtocolHandler;
class ComponentInformationManager;
class VehicleBatteryFactGroup;
class SendMavCommandWithSignallingTest;
class SendMavCommandWithHandlerTest;
class RequestMessageTest;
class LinkInterface;
class LinkManager;
class InitialConnectStateMachine;
class Autotune;
class RemoteIDManager;
class FleetMissionStartupTransaction;
class FleetStartupMemorySeal;
class MultiVehicleManager;
class FleetOptionalTelemetryConfigurationBasis;
class FleetOptionalTelemetryConfigurationRun;
class FleetLinkLatencyRun;
class FleetCameraStreamRun;
class FleetCameraStreamControlBasis;
class FleetCameraStreamControlRun;

namespace events {
namespace parser {
class ParsedEvent;
}
}

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

class Vehicle : public FactGroup
{
    Q_OBJECT
    friend class FleetOptionalTelemetryConfigurationBasis;
    friend class FleetOptionalTelemetryConfigurationRun;
    friend class FleetLinkLatencyRun;
    friend class FleetCameraStreamRun;
    friend class FleetCameraStreamControlBasis;
    friend class FleetCameraStreamControlRun;

    friend class InitialConnectStateMachine;
    friend class ParameterManager;                // Native Fleet read-only transport gate
    friend class PlanManager;                     // Native scoped mission read/deployment transport only
    friend class VehicleLinkManager;
    friend class FleetStartupMemorySeal;           // Native whole-group startup memory fence only
    friend class VehicleBatteryFactGroup;           // Allow VehicleBatteryFactGroup to call _addFactGroup
    friend class SendMavCommandWithSignallingTest;  // Unit test
    friend class SendMavCommandWithHandlerTest;     // Unit test
    friend class RequestMessageTest;                // Unit test


public:
    Vehicle(LinkInterface*          link,
            int                     vehicleId,
            int                     defaultComponentId,
            MAV_AUTOPILOT           firmwareType,
            MAV_TYPE                vehicleType,
            FirmwarePluginManager*  firmwarePluginManager,
            JoystickManager*        joystickManager);

    // Pass these into the offline constructor to create an offline vehicle which tracks the offline vehicle settings
    static const MAV_AUTOPILOT    MAV_AUTOPILOT_TRACK = static_cast<MAV_AUTOPILOT>(-1);
    static const MAV_TYPE         MAV_TYPE_TRACK = static_cast<MAV_TYPE>(-1);

    // The following is used to create a disconnected Vehicle for use while offline editing.
    Vehicle(MAV_AUTOPILOT           firmwareType,
            MAV_TYPE                vehicleType,
            FirmwarePluginManager*  firmwarePluginManager,
            QObject*                parent = nullptr);

    ~Vehicle();

    /// Sensor bits from sensors*Bits properties
    enum MavlinkSysStatus {
        SysStatusSensor3dGyro =                 MAV_SYS_STATUS_SENSOR_3D_GYRO,
        SysStatusSensor3dAccel =                MAV_SYS_STATUS_SENSOR_3D_ACCEL,
        SysStatusSensor3dMag =                  MAV_SYS_STATUS_SENSOR_3D_MAG,
        SysStatusSensorAbsolutePressure =       MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE,
        SysStatusSensorDifferentialPressure =   MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE,
        SysStatusSensorGPS =                    MAV_SYS_STATUS_SENSOR_GPS,
        SysStatusSensorOpticalFlow =            MAV_SYS_STATUS_SENSOR_OPTICAL_FLOW,
        SysStatusSensorVisionPosition =         MAV_SYS_STATUS_SENSOR_VISION_POSITION,
        SysStatusSensorLaserPosition =          MAV_SYS_STATUS_SENSOR_LASER_POSITION,
        SysStatusSensorExternalGroundTruth =    MAV_SYS_STATUS_SENSOR_EXTERNAL_GROUND_TRUTH,
        SysStatusSensorAngularRateControl =     MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL,
        SysStatusSensorAttitudeStabilization =  MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION,
        SysStatusSensorYawPosition =            MAV_SYS_STATUS_SENSOR_YAW_POSITION,
        SysStatusSensorZAltitudeControl =       MAV_SYS_STATUS_SENSOR_Z_ALTITUDE_CONTROL,
        SysStatusSensorXYPositionControl =      MAV_SYS_STATUS_SENSOR_XY_POSITION_CONTROL,
        SysStatusSensorMotorOutputs =           MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS,
        SysStatusSensorRCReceiver =             MAV_SYS_STATUS_SENSOR_RC_RECEIVER,
        SysStatusSensor3dGyro2 =                MAV_SYS_STATUS_SENSOR_3D_GYRO2,
        SysStatusSensor3dAccel2 =               MAV_SYS_STATUS_SENSOR_3D_ACCEL2,
        SysStatusSensor3dMag2 =                 MAV_SYS_STATUS_SENSOR_3D_MAG2,
        SysStatusSensorGeoFence =               MAV_SYS_STATUS_GEOFENCE,
        SysStatusSensorAHRS =                   MAV_SYS_STATUS_AHRS,
        SysStatusSensorTerrain =                MAV_SYS_STATUS_TERRAIN,
        SysStatusSensorReverseMotor =           MAV_SYS_STATUS_REVERSE_MOTOR,
        SysStatusSensorLogging =                MAV_SYS_STATUS_LOGGING,
        SysStatusSensorBattery =                MAV_SYS_STATUS_SENSOR_BATTERY,
    };
    Q_ENUM(MavlinkSysStatus)

    enum CheckList {
        CheckListNotSetup = 0,
        CheckListPassed,
        CheckListFailed,
    };
    Q_ENUM(CheckList)

    Q_PROPERTY(int                  id                          READ id                                                             CONSTANT)
    Q_PROPERTY(AutoPilotPlugin*     autopilot                   MEMBER _autopilotPlugin                                             CONSTANT)
    Q_PROPERTY(QGeoCoordinate       coordinate                  READ coordinate                                                     NOTIFY coordinateChanged)
    Q_PROPERTY(QGeoCoordinate       homePosition                READ homePosition                                                   NOTIFY homePositionChanged)
    Q_PROPERTY(QGeoCoordinate       armedPosition               READ armedPosition                                                  NOTIFY armedPositionChanged)
    Q_PROPERTY(bool                 armed                       READ armed                      WRITE setArmedShowError             NOTIFY armedChanged)
    Q_PROPERTY(bool                 autoDisarm                  READ autoDisarm                                                     NOTIFY autoDisarmChanged)
    Q_PROPERTY(bool                 flightModeSetAvailable      READ flightModeSetAvailable                                         CONSTANT)
    Q_PROPERTY(QStringList          flightModes                 READ flightModes                                                    NOTIFY flightModesChanged)
    Q_PROPERTY(QString              flightMode                  READ flightMode                 WRITE setFlightMode                 NOTIFY flightModeChanged)
    Q_PROPERTY(TrajectoryPoints*    trajectoryPoints            MEMBER _trajectoryPoints                                            CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  cameraTriggerPoints         READ cameraTriggerPoints                                            CONSTANT)
    Q_PROPERTY(float                latitude                    READ latitude                                                       NOTIFY coordinateChanged)
    Q_PROPERTY(float                longitude                   READ longitude                                                      NOTIFY coordinateChanged)
    Q_PROPERTY(bool                 messageTypeNone             READ messageTypeNone                                                NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeNormal           READ messageTypeNormal                                              NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeWarning          READ messageTypeWarning                                             NOTIFY messageTypeChanged)
    Q_PROPERTY(bool                 messageTypeError            READ messageTypeError                                               NOTIFY messageTypeChanged)
    Q_PROPERTY(int                  newMessageCount             READ newMessageCount                                                NOTIFY newMessageCountChanged)
    Q_PROPERTY(int                  messageCount                READ messageCount                                                   NOTIFY messageCountChanged)
    Q_PROPERTY(QString              formattedMessages           READ formattedMessages                                              NOTIFY formattedMessagesChanged)
    Q_PROPERTY(bool                 joystickEnabled             READ joystickEnabled            WRITE setJoystickEnabled            NOTIFY joystickEnabledChanged)
    Q_PROPERTY(int                  flowImageIndex              READ flowImageIndex                                                 NOTIFY flowImageIndexChanged)
    Q_PROPERTY(int                  rcRSSI                      READ rcRSSI                                                         NOTIFY rcRSSIChanged)
    Q_PROPERTY(bool                 px4Firmware                 READ px4Firmware                                                    NOTIFY firmwareTypeChanged)
    Q_PROPERTY(bool                 apmFirmware                 READ apmFirmware                                                    NOTIFY firmwareTypeChanged)
    Q_PROPERTY(bool                 soloFirmware                READ soloFirmware               WRITE setSoloFirmware               NOTIFY soloFirmwareChanged)
    Q_PROPERTY(bool                 genericFirmware             READ genericFirmware                                                CONSTANT)
    Q_PROPERTY(uint                 messagesReceived            READ messagesReceived                                               NOTIFY messagesReceivedChanged)
    Q_PROPERTY(uint                 messagesSent                READ messagesSent                                                   NOTIFY messagesSentChanged)
    Q_PROPERTY(uint                 messagesLost                READ messagesLost                                                   NOTIFY messagesLostChanged)
    Q_PROPERTY(bool                 airship                     READ airship                                                        NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 fixedWing                   READ fixedWing                                                      NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 multiRotor                  READ multiRotor                                                     NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 vtol                        READ vtol                                                           NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 rover                       READ rover                                                          NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool                 sub                         READ sub                                                            NOTIFY vehicleTypeChanged)
    Q_PROPERTY(bool        supportsThrottleModeCenterZero       READ supportsThrottleModeCenterZero                                 CONSTANT)
    Q_PROPERTY(bool                supportsNegativeThrust       READ supportsNegativeThrust                                         CONSTANT)
    Q_PROPERTY(bool                 supportsJSButton            READ supportsJSButton                                               CONSTANT)
    Q_PROPERTY(bool                 supportsRadio               READ supportsRadio                                                  CONSTANT)
    Q_PROPERTY(bool               supportsMotorInterference     READ supportsMotorInterference                                      CONSTANT)
    Q_PROPERTY(QString              prearmError                 READ prearmError                WRITE setPrearmError                NOTIFY prearmErrorChanged)
    Q_PROPERTY(int                  motorCount                  READ motorCount                                                     CONSTANT)
    Q_PROPERTY(bool                 coaxialMotors               READ coaxialMotors                                                  CONSTANT)
    Q_PROPERTY(bool                 xConfigMotors               READ xConfigMotors                                                  CONSTANT)
    Q_PROPERTY(bool                 isOfflineEditingVehicle     READ isOfflineEditingVehicle                                        CONSTANT)
    Q_PROPERTY(QString              brandImageIndoor            READ brandImageIndoor                                               NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QString              brandImageOutdoor           READ brandImageOutdoor                                              NOTIFY firmwareTypeChanged)
    Q_PROPERTY(int                  sensorsPresentBits          READ sensorsPresentBits                                             NOTIFY sensorsPresentBitsChanged)
    Q_PROPERTY(int                  sensorsEnabledBits          READ sensorsEnabledBits                                             NOTIFY sensorsEnabledBitsChanged)
    Q_PROPERTY(int                  sensorsHealthBits           READ sensorsHealthBits                                              NOTIFY sensorsHealthBitsChanged)
    Q_PROPERTY(int                  sensorsUnhealthyBits        READ sensorsUnhealthyBits                                           NOTIFY sensorsUnhealthyBitsChanged) ///< Combination of enabled and health
    Q_PROPERTY(QString              missionFlightMode           READ missionFlightMode                                              CONSTANT)
    Q_PROPERTY(QString              pauseFlightMode             READ pauseFlightMode                                                CONSTANT)
    Q_PROPERTY(QString              rtlFlightMode               READ rtlFlightMode                                                  CONSTANT)
    Q_PROPERTY(QString              smartRTLFlightMode          READ smartRTLFlightMode                                             CONSTANT)
    Q_PROPERTY(bool                 supportsSmartRTL            READ supportsSmartRTL                                               CONSTANT)
    Q_PROPERTY(QString              landFlightMode              READ landFlightMode                                                 CONSTANT)
    Q_PROPERTY(QString              takeControlFlightMode       READ takeControlFlightMode                                          CONSTANT)
    Q_PROPERTY(QString              followFlightMode            READ followFlightMode                                               CONSTANT)
    Q_PROPERTY(QString              firmwareTypeString          READ firmwareTypeString                                             NOTIFY firmwareTypeChanged)
    Q_PROPERTY(QString              vehicleTypeString           READ vehicleTypeString                                              NOTIFY vehicleTypeChanged)
    Q_PROPERTY(QString              vehicleImageOpaque          READ vehicleImageOpaque                                             CONSTANT)
    Q_PROPERTY(QString              vehicleImageOutline         READ vehicleImageOutline                                            CONSTANT)
    Q_PROPERTY(QString              vehicleImageCompass         READ vehicleImageCompass                                            CONSTANT)
    Q_PROPERTY(int                  telemetryRRSSI              READ telemetryRRSSI                                                 NOTIFY telemetryRRSSIChanged)
    Q_PROPERTY(int                  telemetryLRSSI              READ telemetryLRSSI                                                 NOTIFY telemetryLRSSIChanged)
    Q_PROPERTY(unsigned int         telemetryRXErrors           READ telemetryRXErrors                                              NOTIFY telemetryRXErrorsChanged)
    Q_PROPERTY(unsigned int         telemetryFixed              READ telemetryFixed                                                 NOTIFY telemetryFixedChanged)
    Q_PROPERTY(unsigned int         telemetryTXBuffer           READ telemetryTXBuffer                                              NOTIFY telemetryTXBufferChanged)
    Q_PROPERTY(int                  telemetryLNoise             READ telemetryLNoise                                                NOTIFY telemetryLNoiseChanged)
    Q_PROPERTY(int                  telemetryRNoise             READ telemetryRNoise                                                NOTIFY telemetryRNoiseChanged)
    Q_PROPERTY(QVariantList         toolIndicators              READ toolIndicators                                                 NOTIFY toolIndicatorsChanged)
    Q_PROPERTY(QVariantList         modeIndicators              READ modeIndicators                                                 NOTIFY modeIndicatorsChanged)
    Q_PROPERTY(bool              initialPlanRequestComplete     READ initialPlanRequestComplete                                     NOTIFY initialPlanRequestCompleteChanged)
    Q_PROPERTY(QVariantList         staticCameraList            READ staticCameraList                                               CONSTANT)
    Q_PROPERTY(QGCCameraManager*    cameraManager               READ cameraManager                                                  NOTIFY cameraManagerChanged)
    Q_PROPERTY(QString              hobbsMeter                  READ hobbsMeter                                                     NOTIFY hobbsMeterChanged)
    Q_PROPERTY(bool                 vtolInFwdFlight             READ vtolInFwdFlight            WRITE setVtolInFwdFlight            NOTIFY vtolInFwdFlightChanged)
    Q_PROPERTY(bool                 supportsTerrainFrame        READ supportsTerrainFrame                                           NOTIFY firmwareTypeChanged)
    Q_PROPERTY(quint64              mavlinkSentCount            READ mavlinkSentCount                                               NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(quint64              mavlinkReceivedCount        READ mavlinkReceivedCount                                           NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(quint64              mavlinkLossCount            READ mavlinkLossCount                                               NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(float                mavlinkLossPercent          READ mavlinkLossPercent                                             NOTIFY mavlinkStatusChanged)
    Q_PROPERTY(qreal                gimbalRoll                  READ gimbalRoll                                                     NOTIFY gimbalRollChanged)
    Q_PROPERTY(qreal                gimbalPitch                 READ gimbalPitch                                                    NOTIFY gimbalPitchChanged)
    Q_PROPERTY(qreal                gimbalYaw                   READ gimbalYaw                                                      NOTIFY gimbalYawChanged)
    Q_PROPERTY(bool                 gimbalData                  READ gimbalData                                                     NOTIFY gimbalDataChanged)
    Q_PROPERTY(bool                 hasGripper                  READ hasGripper                                                     CONSTANT)
    Q_PROPERTY(bool                 isROIEnabled                READ isROIEnabled                                                   NOTIFY isROIEnabledChanged)
    Q_PROPERTY(CheckList            checkListState              READ checkListState             WRITE setCheckListState             NOTIFY checkListStateChanged)
    Q_PROPERTY(bool                 readyToFlyAvailable         READ readyToFlyAvailable                                            NOTIFY readyToFlyAvailableChanged)  ///< true: readyToFly signalling is available on this vehicle
    Q_PROPERTY(bool                 readyToFly                  READ readyToFly                                                     NOTIFY readyToFlyChanged)
    Q_PROPERTY(QObject*             sysStatusSensorInfo         READ sysStatusSensorInfo                                            CONSTANT)
    Q_PROPERTY(bool                 allSensorsHealthy           READ allSensorsHealthy                                              NOTIFY allSensorsHealthyChanged)    //< true: all sensors in SYS_STATUS reported as healthy
    Q_PROPERTY(bool                 requiresGpsFix              READ requiresGpsFix                                                 NOTIFY requiresGpsFixChanged)
    Q_PROPERTY(double               loadProgress                READ loadProgress                                                   NOTIFY loadProgressChanged)
    Q_PROPERTY(bool                 initialConnectComplete      READ isInitialConnectComplete                                       NOTIFY initialConnectComplete)

    // The following properties relate to Orbit status
    Q_PROPERTY(bool             orbitActive     READ orbitActive        NOTIFY orbitActiveChanged)
    Q_PROPERTY(QGCMapCircle*    orbitMapCircle  READ orbitMapCircle     CONSTANT)

    // Vehicle state used for guided control
    Q_PROPERTY(bool     flying                  READ flying                                         NOTIFY flyingChanged)       ///< Vehicle is flying
    Q_PROPERTY(bool     landing                 READ landing                                        NOTIFY landingChanged)      ///< Vehicle is in landing pattern (DO_LAND_START)
    Q_PROPERTY(bool     guidedMode              READ guidedMode                 WRITE setGuidedMode NOTIFY guidedModeChanged)   ///< Vehicle is in Guided mode and can respond to guided commands
    Q_PROPERTY(bool     guidedModeSupported     READ guidedModeSupported                            CONSTANT)                   ///< Guided mode commands are supported by this vehicle
    Q_PROPERTY(bool     pauseVehicleSupported   READ pauseVehicleSupported                          CONSTANT)                   ///< Pause vehicle command is supported
    Q_PROPERTY(bool     orbitModeSupported      READ orbitModeSupported                             CONSTANT)                   ///< Orbit mode is supported by this vehicle
    Q_PROPERTY(bool     roiModeSupported        READ roiModeSupported                               CONSTANT)                   ///< Orbit mode is supported by this vehicle
    Q_PROPERTY(bool     takeoffVehicleSupported READ takeoffVehicleSupported                        CONSTANT)                   ///< Guided takeoff supported
    Q_PROPERTY(QString  gotoFlightMode          READ gotoFlightMode                                 CONSTANT)                   ///< Flight mode vehicle is in while performing goto
    Q_PROPERTY(bool     haveMRSpeedLimits       READ haveMRSpeedLimits                              NOTIFY haveMRSpeedLimChanged)
    Q_PROPERTY(bool     haveFWSpeedLimits       READ haveFWSpeedLimits                              NOTIFY haveFWSpeedLimChanged)

    Q_PROPERTY(ParameterManager*        parameterManager    READ parameterManager   CONSTANT)
    Q_PROPERTY(VehicleLinkManager*      vehicleLinkManager  READ vehicleLinkManager CONSTANT)
    Q_PROPERTY(VehicleObjectAvoidance*  objectAvoidance     READ objectAvoidance    CONSTANT)
    Q_PROPERTY(Autotune*                autotune            READ autotune           CONSTANT)
    Q_PROPERTY(RemoteIDManager*         remoteIDManager     READ remoteIDManager    CONSTANT)

    // FactGroup object model properties

    Q_PROPERTY(Fact* roll               READ roll               CONSTANT)
    Q_PROPERTY(Fact* pitch              READ pitch              CONSTANT)
    Q_PROPERTY(Fact* heading            READ heading            CONSTANT)
    Q_PROPERTY(Fact* rollRate           READ rollRate           CONSTANT)
    Q_PROPERTY(Fact* pitchRate          READ pitchRate          CONSTANT)
    Q_PROPERTY(Fact* yawRate            READ yawRate            CONSTANT)
    Q_PROPERTY(Fact* groundSpeed        READ groundSpeed        CONSTANT)
    Q_PROPERTY(Fact* airSpeed           READ airSpeed           CONSTANT)
    Q_PROPERTY(Fact* airSpeedSetpoint   READ airSpeedSetpoint   CONSTANT)
    Q_PROPERTY(Fact* climbRate          READ climbRate          CONSTANT)
    Q_PROPERTY(Fact* altitudeRelative   READ altitudeRelative   CONSTANT)
    Q_PROPERTY(Fact* altitudeAMSL       READ altitudeAMSL       CONSTANT)
    Q_PROPERTY(Fact* altitudeAboveTerr  READ altitudeAboveTerr  CONSTANT)
    Q_PROPERTY(Fact* altitudeTuning     READ altitudeTuning     CONSTANT)
    Q_PROPERTY(Fact* altitudeTuningSetpoint READ altitudeTuningSetpoint CONSTANT)
    Q_PROPERTY(Fact* xTrackError        READ xTrackError        CONSTANT)
    Q_PROPERTY(Fact* rangeFinderDist    READ rangeFinderDist    CONSTANT)
    Q_PROPERTY(Fact* flightDistance     READ flightDistance     CONSTANT)
    Q_PROPERTY(Fact* distanceToHome     READ distanceToHome     CONSTANT)
    Q_PROPERTY(Fact* timeToHome         READ timeToHome         CONSTANT)
    Q_PROPERTY(Fact* missionItemIndex   READ missionItemIndex   CONSTANT)
    Q_PROPERTY(Fact* headingToNextWP    READ headingToNextWP    CONSTANT)
    Q_PROPERTY(Fact* headingToHome      READ headingToHome      CONSTANT)
    Q_PROPERTY(Fact* distanceToGCS      READ distanceToGCS      CONSTANT)
    Q_PROPERTY(Fact* hobbs              READ hobbs              CONSTANT)
    Q_PROPERTY(Fact* throttlePct        READ throttlePct        CONSTANT)
    Q_PROPERTY(Fact* imuTemp            READ imuTemp            CONSTANT)

    Q_PROPERTY(FactGroup*           gps             READ gpsFactGroup               CONSTANT)
    Q_PROPERTY(FactGroup*           gps2            READ gps2FactGroup              CONSTANT)
    Q_PROPERTY(FactGroup*           wind            READ windFactGroup              CONSTANT)
    Q_PROPERTY(FactGroup*           vibration       READ vibrationFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup*           temperature     READ temperatureFactGroup       CONSTANT)
    Q_PROPERTY(FactGroup*           clock           READ clockFactGroup             CONSTANT)
    Q_PROPERTY(FactGroup*           setpoint        READ setpointFactGroup          CONSTANT)
    Q_PROPERTY(FactGroup*           escStatus       READ escStatusFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup*           ftcStatus       READ ftcStatusFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup*           estimatorStatus READ estimatorStatusFactGroup   CONSTANT)
    Q_PROPERTY(FactGroup*           terrain         READ terrainFactGroup           CONSTANT)
    Q_PROPERTY(FactGroup*           distanceSensors READ distanceSensorFactGroup    CONSTANT)
    Q_PROPERTY(FactGroup*           localPosition   READ localPositionFactGroup     CONSTANT)
    Q_PROPERTY(FactGroup*           localPositionSetpoint READ localPositionSetpointFactGroup CONSTANT)
    Q_PROPERTY(FactGroup*           hygrometer      READ hygrometerFactGroup        CONSTANT)
    Q_PROPERTY(FactGroup*           generator       READ generatorFactGroup         CONSTANT)
    Q_PROPERTY(FactGroup*           efi             READ efiFactGroup               CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  batteries       READ batteries                  CONSTANT)
    Q_PROPERTY(Actuators*           actuators       READ actuators                  CONSTANT)
    Q_PROPERTY(HealthAndArmingCheckReport* healthAndArmingCheckReport READ healthAndArmingCheckReport CONSTANT)

    Q_PROPERTY(int      firmwareMajorVersion        READ firmwareMajorVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareMinorVersion        READ firmwareMinorVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwarePatchVersion        READ firmwarePatchVersion       NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareVersionType         READ firmwareVersionType        NOTIFY firmwareVersionChanged)
    Q_PROPERTY(QString  firmwareVersionTypeString   READ firmwareVersionTypeString  NOTIFY firmwareVersionChanged)
    Q_PROPERTY(int      firmwareCustomMajorVersion  READ firmwareCustomMajorVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(int      firmwareCustomMinorVersion  READ firmwareCustomMinorVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(int      firmwareCustomPatchVersion  READ firmwareCustomPatchVersion NOTIFY firmwareCustomVersionChanged)
    Q_PROPERTY(QString  gitHash                     READ gitHash                    NOTIFY gitHashChanged)
    Q_PROPERTY(quint64  vehicleUID                  READ vehicleUID                 NOTIFY vehicleUIDChanged)
    Q_PROPERTY(QString  vehicleUIDStr               READ vehicleUIDStr              NOTIFY vehicleUIDChanged)

    /// Resets link status counters
    Q_INVOKABLE void resetCounters  ();

    // Called when the message drop-down is invoked to clear current count
    Q_INVOKABLE void resetAllMessages();
    Q_INVOKABLE void resetErrorLevelMessages();

    Q_INVOKABLE void virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust);

    /// Command vehicle to return to launch
    Q_INVOKABLE void guidedModeRTL(bool smartRTL);

    /// Command vehicle to land at current location
    Q_INVOKABLE void guidedModeLand();

    /// Command vehicle to takeoff from current location
    Q_INVOKABLE void guidedModeTakeoff(double altitudeRelative);

    /// @return The minimum takeoff altitude (relative) for guided takeoff.
    Q_INVOKABLE double minimumTakeoffAltitude();

    /// @return The configured guided takeoff climb speed, or NAN when unsupported.
    Q_INVOKABLE double guidedTakeoffSpeed();

    /// Queue a firmware-specific guided takeoff climb speed parameter write.
    Q_INVOKABLE bool setGuidedTakeoffSpeed(double metersSecond);

    /// @return Maximum horizontal speed multirotor.
    Q_INVOKABLE double maximumHorizontalSpeedMultirotor();

    /// @return Maximum equivalent airspeed.
    Q_INVOKABLE double maximumEquivalentAirspeed();

    /// @return Minumum equivalent airspeed.
    Q_INVOKABLE double minimumEquivalentAirspeed();

    /// Command vehicle to move to specified location (altitude is included and relative)
    Q_INVOKABLE void guidedModeGotoLocation(const QGeoCoordinate& gotoCoord);

    /// Command vehicle to change altitude
    ///     @param altitudeChange If > 0, go up by amount specified, if < 0, go down by amount specified
    ///     @param pauseVehicle true: pause vehicle prior to altitude change
    Q_INVOKABLE void guidedModeChangeAltitude(double altitudeChange, bool pauseVehicle);

    /// Command vehicle to change groundspeed
    ///     @param groundspeed Target horizontal groundspeed
    Q_INVOKABLE void guidedModeChangeGroundSpeedMetersSecond(double groundspeed);
    /// Command vehicle to change equivalent airspeed
    ///     @param airspeed Target equivalent airspeed
    Q_INVOKABLE void guidedModeChangeEquivalentAirspeedMetersSecond(double airspeed);

    /// Command vehicle to orbit given center point
    ///     @param centerCoord Orit around this point
    ///     @param radius Distance from vehicle to centerCoord
    ///     @param amslAltitude Desired vehicle altitude
    Q_INVOKABLE void guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude);

    /// Command vehicle to keep given point as ROI
    ///     @param centerCoord ROI coordinates
    Q_INVOKABLE void guidedModeROI(const QGeoCoordinate& centerCoord);
    Q_INVOKABLE void stopGuidedModeROI();

    /// Command vehicle to pause at current location. If vehicle supports guide mode, vehicle will be left
    /// in guided mode after pause.
    Q_INVOKABLE void pauseVehicle();

    /// Command vehicle to kill all motors no matter what state
    Q_INVOKABLE void emergencyStop();

    /// Command vehicle to abort landing
    Q_INVOKABLE void abortLanding(double climbOutAltitude);

    Q_INVOKABLE void startMission();

    /// Alter the current mission item on the vehicle
    Q_INVOKABLE void setCurrentMissionSequence(int seq);

    /// Reboot vehicle
    Q_INVOKABLE void rebootVehicle();

    /// Clear Messages
    Q_INVOKABLE void clearMessages();

    // C++/backend-only plan upload entry. Keep this out of the QML method
    // surface so arbitrary QML/plugins cannot bypass PlanMasterController's
    // target and operation-identity checks.
    void sendPlan(QString planFile);

    /// Used to check if running current version is equal or higher than the one being compared.
    //  returns 1 if current > compare, 0 if current == compare, -1 if current < compare
    Q_INVOKABLE int versionCompare(QString& compare);
    Q_INVOKABLE int versionCompare(int major, int minor, int patch);

    /// Test motor
    ///     @param motor Motor number, 1-based
    ///     @param percent 0-no power, 100-full power
    ///     @param timeoutSec Disabled motor after this amount of time
    Q_INVOKABLE void motorTest(int motor, int percent, int timeoutSecs, bool showError);

    enum PIDTuningTelemetryMode {
        ModeDisabled,
        ModeRateAndAttitude,
        ModeVelocityAndPosition,
        ModeAltitudeAndAirspeed,
    };
    Q_ENUM(PIDTuningTelemetryMode)

    Q_INVOKABLE void setPIDTuningTelemetryMode(PIDTuningTelemetryMode mode);

    Q_INVOKABLE void gimbalControlValue (double pitch, double yaw);
    Q_INVOKABLE void gimbalPitchStep    (int direction);
    Q_INVOKABLE void gimbalYawStep      (int direction);
    Q_INVOKABLE void centerGimbal       ();
    Q_INVOKABLE void forceArm           ();

    /// Sends PARAM_MAP_RC message to vehicle
    Q_INVOKABLE void sendParamMapRC(const QString& paramName, double scale, double centerValue, int tuningID, double minValue, double maxValue);

    /// Clears all PARAM_MAP_RC settings from vehicle
    Q_INVOKABLE void clearAllParamMapRC(void);

    /// Removes the vehicle from the system
    Q_INVOKABLE void closeVehicle(void);

    /// Trigger camera using MAV_CMD_DO_DIGICAM_CONTROL command
    Q_INVOKABLE void triggerSimpleCamera(void);

#if !defined(NO_ARDUPILOT_DIALECT)
    Q_INVOKABLE void flashBootloader();
#endif
    /// Set home from flight map coordinate
    Q_INVOKABLE void doSetHome(const QGeoCoordinate& coord);

    /// Save the joystick enable setting to the settings group
    Q_INVOKABLE void saveJoystickSettings(void);

    bool    isInitialConnectComplete() const;
    bool    guidedModeSupported     () const;
    bool    pauseVehicleSupported   () const;
    bool    orbitModeSupported      () const;
    bool    roiModeSupported        () const;
    bool    takeoffVehicleSupported () const;
    QString gotoFlightMode          () const;
    bool    hasGripper              () const;
    bool haveMRSpeedLimits() const { return _multirotor_speed_limits_available; }
    bool haveFWSpeedLimits() const { return _fixed_wing_airspeed_limits_available; }

    // Property accessors

    QGeoCoordinate coordinate() { return _coordinate; }
    QGeoCoordinate armedPosition    () { return _armedPosition; }

    void updateFlightDistance(double distance);

    bool joystickEnabled            () const;
    void setJoystickEnabled         (bool enabled);
    void sendJoystickDataThreadSafe (float roll, float pitch, float yaw, float thrust, quint16 buttons);

    // Property accesors
    int id() const{ return _id; }
    int compId() const{ return _compID; }
    MAV_AUTOPILOT firmwareType() const { return _firmwareType; }
    MAV_TYPE vehicleType() const { return _vehicleType; }
    QGCMAVLink::VehicleClass_t vehicleClass(void) const { return QGCMAVLink::vehicleClass(_vehicleType); }
    Q_INVOKABLE QString vehicleTypeName() const;

    /// Sends a message to the specified link
    /// @return true: message sent, false: Link no longer connected
    bool sendMessageOnLinkThreadSafe(LinkInterface* link, mavlink_message_t message);

    /// Sends the specified messages multiple times to the vehicle in order to attempt to
    /// guarantee that it makes it to the vehicle.
    void sendMessageMultiple(mavlink_message_t message);

    /// Provides access to uas from vehicle. Temporary workaround until UAS is fully phased out.
    UAS* uas() { return _uas; }

    /// Provides access to uas from vehicle. Temporary workaround until AutoPilotPlugin is fully phased out.
    AutoPilotPlugin* autopilotPlugin() { return _autopilotPlugin; }

    /// Provides access to the Firmware Plugin for this Vehicle
    FirmwarePlugin* firmwarePlugin() { return _firmwarePlugin; }


    QGeoCoordinate homePosition();

    bool armed              () const{ return _armed; }
    void setArmed           (bool armed, bool showError);
    void setArmedShowError  (bool armed) { setArmed(armed, true); }

    bool flightModeSetAvailable             ();
    QStringList flightModes                 ();
    QString flightMode                      () const;
    void setFlightMode                      (const QString& flightMode);

    bool airship() const;

    /**
     * @brief Send MAV_CMD_DO_GRIPPER command to trigger specified action in the vehicle
     * 
     * @param gripperAction Gripper action to trigger
    */

    enum    GRIPPER_OPTIONS
    {
    Gripper_release = GRIPPER_ACTION_RELEASE, 
    Gripper_grab    = GRIPPER_ACTION_GRAB,
    Invalid_option  = GRIPPER_ACTIONS_ENUM_END,
    }; 
    Q_ENUM(GRIPPER_OPTIONS)

    void setGripperAction(GRIPPER_ACTIONS gripperAction);
    Q_INVOKABLE void sendGripperAction(GRIPPER_OPTIONS gripperOption); 

    bool fixedWing() const;
    bool multiRotor() const;
    bool vtol() const;
    bool rover() const;
    bool sub() const;

    bool supportsThrottleModeCenterZero () const;
    bool supportsNegativeThrust         ();
    bool supportsRadio                  () const;
    bool supportsJSButton               () const;
    bool supportsMotorInterference      () const;
    bool supportsTerrainFrame           () const;

    void setGuidedMode(bool guidedMode);

    QString prearmError() const { return _prearmError; }
    void setPrearmError(const QString& prearmError);

    QmlObjectListModel* cameraTriggerPoints () { return &_cameraTriggerPoints; }

    int  flowImageIndex() const{ return _flowImageIndex; }

    //-- Mavlink Logging
    void startMavlinkLog();
    void stopMavlinkLog();

    /// Requests the specified data stream from the vehicle
    ///     @param stream Stream which is being requested
    ///     @param rate Rate at which to send stream in Hz
    ///     @param sendMultiple Send multiple time to guarantee Vehicle reception
    void requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple = true);

    // The follow method are used to turn on/off the tracking of settings updates for firmware/vehicle type on offline vehicles.
    void trackFirmwareVehicleTypeChanges(void);
    void stopTrackingFirmwareVehicleTypeChanges(void);

    typedef enum {
        MessageNone,
        MessageNormal,
        MessageWarning,
        MessageError
    } MessageType_t;

    bool            messageTypeNone             () { return _currentMessageType == MessageNone; }
    bool            messageTypeNormal           () { return _currentMessageType == MessageNormal; }
    bool            messageTypeWarning          () { return _currentMessageType == MessageWarning; }
    bool            messageTypeError            () { return _currentMessageType == MessageError; }
    int             newMessageCount             () const{ return _currentMessageCount; }
    int             messageCount                () const{ return _messageCount; }
    QString         formattedMessages           ();
    float           latitude                    () { return static_cast<float>(_coordinate.latitude()); }
    float           longitude                   () { return static_cast<float>(_coordinate.longitude()); }
    bool            mavPresent                  () { return _mav != nullptr; }
    int             rcRSSI                      () const{ return _rcRSSI; }
    bool            px4Firmware                 () const { return _firmwareType == MAV_AUTOPILOT_PX4; }
    bool            apmFirmware                 () const { return _firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    bool            genericFirmware             () const { return !px4Firmware() && !apmFirmware(); }
    uint            messagesReceived            () const{ return _messagesReceived; }
    uint            messagesSent                () const{ return _messagesSent; }
    uint            messagesLost                () const{ return _messagesLost; }
    bool            flying                      () const { return _flying; }
    bool            landing                     () const { return _landing; }
    bool            guidedMode                  () const;
    bool            vtolInFwdFlight             () const { return _vtolInFwdFlight; }
    uint8_t         baseMode                    () const { return _base_mode; }
    uint32_t        customMode                  () const { return _custom_mode; }
    bool            isOfflineEditingVehicle     () const { return _offlineEditingVehicle; }
    QString         brandImageIndoor            () const;
    QString         brandImageOutdoor           () const;
    int             sensorsPresentBits          () const { return static_cast<int>(_onboardControlSensorsPresent); }
    int             sensorsEnabledBits          () const { return static_cast<int>(_onboardControlSensorsEnabled); }
    int             sensorsHealthBits           () const { return static_cast<int>(_onboardControlSensorsHealth); }
    int             sensorsUnhealthyBits        () const { return static_cast<int>(_onboardControlSensorsUnhealthy); }
    QString         missionFlightMode           () const;
    QString         pauseFlightMode             () const;
    QString         rtlFlightMode               () const;
    QString         smartRTLFlightMode          () const;
    bool            supportsSmartRTL            () const;
    QString         landFlightMode              () const;
    QString         takeControlFlightMode       () const;
    QString         followFlightMode            () const;
    double          defaultCruiseSpeed          () const { return _defaultCruiseSpeed; }
    double          defaultHoverSpeed           () const { return _defaultHoverSpeed; }
    QString         firmwareTypeString          () const;
    QString         vehicleTypeString           () const;
    int             telemetryRRSSI              () const{ return _telemetryRRSSI; }
    int             telemetryLRSSI              () const{ return _telemetryLRSSI; }
    unsigned int    telemetryRXErrors           () const{ return _telemetryRXErrors; }
    unsigned int    telemetryFixed              () const{ return _telemetryFixed; }
    unsigned int    telemetryTXBuffer           () const{ return _telemetryTXBuffer; }
    int             telemetryLNoise             () const{ return _telemetryLNoise; }
    int             telemetryRNoise             () const{ return _telemetryRNoise; }
    bool            autoDisarm                  ();
    bool            orbitActive                 () const { return _orbitActive; }
    QGCMapCircle*   orbitMapCircle              () { return &_orbitMapCircle; }
    bool            readyToFlyAvailable         () const{ return _readyToFlyAvailable; }
    bool            readyToFly                  () const{ return _readyToFly; }
    bool            allSensorsHealthy           () const{ return _allSensorsHealthy; }
    QObject*        sysStatusSensorInfo         () { return &_sysStatusSensorInfo; }
    bool            requiresGpsFix              () const { return static_cast<bool>(_onboardControlSensorsPresent & SysStatusSensorGPS); }
    bool            hilMode                     () const { return _base_mode & MAV_MODE_FLAG_HIL_ENABLED; }
    Actuators*      actuators                   () const { return _actuators; }

    /// Get the maximum MAVLink protocol version supported
    /// @return the maximum version
    unsigned        maxProtoVersion         () const { return _maxProtoVersion; }

    enum CalibrationType {
        CalibrationRadio,
        CalibrationGyro,
        CalibrationMag,
        CalibrationAccel,
        CalibrationLevel,
        CalibrationEsc,
        CalibrationCopyTrims,
        CalibrationAPMCompassMot,
        CalibrationAPMPressureAirspeed,
        CalibrationAPMPreFlight,
        CalibrationPX4Airspeed,
        CalibrationPX4Pressure,
        CalibrationAPMAccelSimple,
    };

    void startCalibration   (CalibrationType calType);
    void stopCalibration    (bool showError);

    void startUAVCANBusConfig(void);
    void stopUAVCANBusConfig(void);

    Fact* roll                              () { return &_rollFact; }
    Fact* pitch                             () { return &_pitchFact; }
    Fact* heading                           () { return &_headingFact; }
    Fact* rollRate                          () { return &_rollRateFact; }
    Fact* pitchRate                         () { return &_pitchRateFact; }
    Fact* yawRate                           () { return &_yawRateFact; }
    Fact* airSpeed                          () { return &_airSpeedFact; }
    Fact* airSpeedSetpoint                  () { return &_airSpeedSetpointFact; }
    Fact* groundSpeed                       () { return &_groundSpeedFact; }
    Fact* climbRate                         () { return &_climbRateFact; }
    Fact* altitudeRelative                  () { return &_altitudeRelativeFact; }
    Fact* altitudeAMSL                      () { return &_altitudeAMSLFact; }
    Fact* altitudeAboveTerr                 () { return &_altitudeAboveTerrFact; }
    Fact* altitudeTuning                    () { return &_altitudeTuningFact; }
    Fact* altitudeTuningSetpoint            () { return &_altitudeTuningSetpointFact; }
    Fact* xTrackError                       () { return &_xTrackErrorFact; }
    Fact* rangeFinderDist                   () { return &_rangeFinderDistFact; }
    Fact* flightDistance                    () { return &_flightDistanceFact; }
    Fact* distanceToHome                    () { return &_distanceToHomeFact; }
    Fact* timeToHome                        () { return &_timeToHomeFact; }
    Fact* missionItemIndex                  () { return &_missionItemIndexFact; }
    Fact* headingToNextWP                   () { return &_headingToNextWPFact; }
    Fact* headingToHome                     () { return &_headingToHomeFact; }
    Fact* distanceToGCS                     () { return &_distanceToGCSFact; }
    Fact* hobbs                             () { return &_hobbsFact; }
    Fact* throttlePct                       () { return &_throttlePctFact; }
    Fact* imuTemp                           () { return &_imuTempFact; }

    FactGroup* gpsFactGroup                 () { return &_gpsFactGroup; }
    FactGroup* gps2FactGroup                () { return &_gps2FactGroup; }
    FactGroup* windFactGroup                () { return &_windFactGroup; }
    FactGroup* vibrationFactGroup           () { return &_vibrationFactGroup; }
    FactGroup* temperatureFactGroup         () { return &_temperatureFactGroup; }
    FactGroup* clockFactGroup               () { return &_clockFactGroup; }
    FactGroup* setpointFactGroup            () { return &_setpointFactGroup; }
    FactGroup* distanceSensorFactGroup      () { return &_distanceSensorFactGroup; }
    FactGroup* localPositionFactGroup       () { return &_localPositionFactGroup; }
    FactGroup* localPositionSetpointFactGroup() { return &_localPositionSetpointFactGroup; }
    FactGroup* escStatusFactGroup           () { return &_escStatusFactGroup; }
    FactGroup* ftcStatusFactGroup           () { return &_ftcStatusFactGroup; }
    FactGroup* estimatorStatusFactGroup     () { return &_estimatorStatusFactGroup; }
    FactGroup* terrainFactGroup             () { return &_terrainFactGroup; }
    FactGroup* hygrometerFactGroup          () { return &_hygrometerFactGroup; }
    FactGroup* generatorFactGroup           () { return &_generatorFactGroup; }
    FactGroup* efiFactGroup                 () { return &_efiFactGroup; }
    QmlObjectListModel* batteries           () { return &_batteryFactGroupListModel; }

    MissionManager*                 missionManager      () { return _missionManager; }
    GeoFenceManager*                geoFenceManager     () { return _geoFenceManager; }
    RallyPointManager*              rallyPointManager   () { return _rallyPointManager; }
    ParameterManager*               parameterManager    ();
    ParameterManager*               parameterManager    () const;
    VehicleLinkManager*             vehicleLinkManager  () { return _vehicleLinkManager; }
    FTPManager*                     ftpManager          () { return _ftpManager; }
    ComponentInformationManager*    compInfoManager     () { return _componentInformationManager; }
    // Native callback-lifetime fence only. Valid during normal initial
    // connection, but false while this Vehicle is closing/being removed.
    // Not metadata availability, source authentication or command permission.
    bool componentInformationCallbackAllowed() const;
    VehicleObjectAvoidance*         objectAvoidance     () { return _objectAvoidance; }
    Autotune*                       autotune            () const { return _autotune; }
    RemoteIDManager*                remoteIDManager     () { return _remoteIDManager; }

    static const int cMaxRcChannels = 18;

    /// Sends the specified MAV_CMD to the vehicle. If no Ack is received command will be retried. If a sendMavCommand is already in progress
    /// the command will be queued and sent when the previous command completes.
    ///     @param compId Component to send to.
    ///     @param command MAV_CMD to send
    ///     @param showError true: Display error to user if command failed, false:  no error shown
    /// Signals: mavCommandResult on success or failure
    void sendMavCommand(int compId, MAV_CMD command, bool showError, float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);
    void sendMavCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7);

    ///
    /// \brief isMavCommandPending
    ///     Query whether the specified MAV_CMD is in queue to be sent or has
    /// already been sent but whose reply has not yet been received and whose
    /// timeout has not yet expired.
    ///
    ///     Or, said another way: if you call `sendMavCommand(compId, command, true, ...)`
    /// will an error be shown because you (or another part of QGC) has already
    /// sent that command?
    ///
    /// \param targetCompId
    /// \param command
    /// \return
    ///
    bool isMavCommandPending(int targetCompId, MAV_CMD command);

    /// Same as sendMavCommand, intentionally backend-only. QML command
    /// surfaces must use an explicit safety-class/confirmation controller.
    void sendCommand(int compId, int command, bool showError, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);

    typedef enum {
        MavCmdResultCommandResultOnly,          ///< commandResult specifies full success/fail info
        MavCmdResultFailureNoResponseToCommand, ///< No response from vehicle to command
        MavCmdResultFailureDuplicateCommand,    ///< Unable to send command since duplicate is already being waited on for response
        MavCmdResultFailureLateAckQuarantine,   ///< Command temporarily blocked after a no-response timeout to quarantine delayed ACKs
    } MavCmdResultFailureCode_t;

    /// Callback for sendMavCommandWithHandler which handles MAV_RESULT_IN_PROGRESS acks
    ///     @param progressHandlerData  Opaque data passed in to sendMavCommand call
    ///     @param ack                  Received COMMAND_ACK
    typedef void (*MavCmdProgressHandler)(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);

    /// Callback for sendMavCommandWithHandler which handles all acks which are not MAV_RESULT_IN_PROGRESS
    ///     @param resultHandlerData    Opaque data passed in to sendMavCommand call
    ///     @param ack                  Received COMMAND_ACK
    ///     @param failureCode          Failure reason. If not MavCmdResultCommandResultOnly only ack.result == MAV_RESULT_FAILED is valid.
    typedef void (*MavCmdResultHandler)(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);

    // Callback info for sendMavCommandWithHandler
    typedef struct MavCmdAckHandlerInfo_s {
        MavCmdResultHandler     resultHandler;          ///> nullptr for no handler
        void*                   resultHandlerData; 
        MavCmdProgressHandler   progressHandler;
        void*                   progressHandlerData;    ///> nullptr for no handler
    } MavCmdAckHandlerInfo_t;

    /// Sends the command and calls the callback with the result
    void sendMavCommandWithHandler(
        const MavCmdAckHandlerInfo_t* ackHandlerInfo,   ///> nullptr to signale no handlers
        int compId, MAV_CMD command, 
        float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f, float param6 = 0.0f, float param7 = 0.0f);

    /// Sends the command and calls the callback with the result
    ///     @param resultHandler    Callback for result, nullptr for no callback
    ///     @param resultHandleData Opaque data passed through callback
    void sendMavCommandIntWithHandler(
        const MavCmdAckHandlerInfo_t* ackHandlerInfo,   ///> nullptr to signale no handlers
        int compId, MAV_CMD command, MAV_FRAME frame, 
        float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, double param5 = 0.0f, double param6 = 0.0f, float param7 = 0.0f);

    typedef enum {
        RequestMessageNoFailure,
        RequestMessageFailureCommandError,
        RequestMessageFailureCommandNotAcked,
        RequestMessageFailureMessageNotReceived,
        RequestMessageFailureDuplicateCommand,    ///< Unabled to send command since duplicate is already being waited on for response
    } RequestMessageResultHandlerFailureCode_t;

    /// Callback for requestMessage
    ///     @param resultHandlerData    Opaque data which was passed in to requestMessage call
    ///     @param commandResult        Result from ack to REQUEST_MESSAGE command
    ///     @param failureCode          Failure code
    ///     @param message              Received message which was requested
    typedef void (*RequestMessageResultHandler)(void* resultHandlerData, MAV_RESULT commandResult, RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

    /// Requests the vehicle to send the specified message. Will retry a number of times.
    ///     @param resultHandler Callback for result
    ///     @param resultHandlerData Opaque data passed back to resultHandler
    ///     @param callbackOwner Optional native lifetime only; when destroyed,
    ///            terminate locally without invoking borrowed callback data.
    void requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId,
        float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f,
        QObject* callbackOwner = nullptr);

    int firmwareMajorVersion() const { return _firmwareMajorVersion; }
    int firmwareMinorVersion() const { return _firmwareMinorVersion; }
    int firmwarePatchVersion() const { return _firmwarePatchVersion; }
    int firmwareVersionType() const { return _firmwareVersionType; }
    int firmwareCustomMajorVersion() const { return _firmwareCustomMajorVersion; }
    int firmwareCustomMinorVersion() const { return _firmwareCustomMinorVersion; }
    int firmwareCustomPatchVersion() const { return _firmwareCustomPatchVersion; }
    int firmwareBoardVendorId() const { return _firmwareBoardVendorId; }
    int firmwareBoardProductId() const { return _firmwareBoardProductId; }
    QString firmwareVersionTypeString() const;
    void setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType = FIRMWARE_VERSION_TYPE_OFFICIAL);
    void setFirmwareCustomVersion(int majorVersion, int minorVersion, int patchVersion);
    static const int versionNotSetValue = -1;

    QString gitHash() const { return _gitHash; }
    quint64 vehicleUID() const { return _uid; }
    QString vehicleUIDStr();

    /// Native C++ only: immutable fields from an actual AUTOPILOT_VERSION
    /// receive, never InitialConnect fallback capabilities or cache values.
    /// Owner/GUI-thread read; monotonic age domain is 1..120000 ms. It is not
    /// authentication, hardware qualification, or an execution permission.
    QVariantMap autopilotVersionObservation(qint64 maximumAgeMs = 60000) const;
    bool autopilotVersionObservationSourceMatches(LinkInterface* expectedLink) const;
    // Callback-free lifecycle identity only. No sample availability, freshness,
    // origin/HOME validity, authentication or permission is implied.
    QString autopilotVersionObservationSourceEpochMemory() const;
    // Native callback-free final source/content/age check. The caller must
    // also retain the actual PRIMARY weak owner; this is not reachability,
    // hardware/firmware authentication, a datum proof or an execution permit.
    bool autopilotVersionObservationMemoryCurrent(LinkInterface* expectedPrimary,
        const QString& sourceEpoch, const QString& contentRevision,
        const QString& contentHash, qint64 maximumAgeMs = 30000) const;
    // Non-owning native pointer to this actual VERSION source, only while the
    // same callback-free source/content/30s-age check succeeds. No weak.lock()
    // or temporary strong ownership; caller retains QPointer + PRIMARY owner.
    LinkInterface* autopilotVersionObservationPrimaryLinkMemory() const;
    // Explicit native passive observer; never requests streams or controls a
    // vehicle. One actual current primary link is frozen for this binding.
    QVariantMap bindFleetTelemetryObserver(VehicleTelemetryObserver* observer);
    bool unbindFleetTelemetryObserver(VehicleTelemetryObserver* observer, const QString& sourceEpoch);
    bool fleetTelemetryBindingMatches(VehicleTelemetryObserver* observer, const QString& sourceEpoch) const;
    // Execution provenance is installed only inside the actual native MODE
    // queue path, never by a caller bind/import or a public snapshot.
    bool fleetExecutionBindingMatches(VehicleMissionExecutionObserver* observer, const QString& sourceEpoch) const;
    bool unbindFleetExecutionObserver(VehicleMissionExecutionObserver* observer, const QString& sourceEpoch);
    QVariantMap requestFleetAutopilotVersion(LinkInterface* expectedLink, std::function<bool()> authorizationFence);
    enum class FleetStartupStep { Seq0 = 0, NormalArm = 1, MissionModeOnly = 2 };
    QVariantMap sendFleetStartupStep(QObject* transactionObject, LinkInterface* expectedPrimary,
        const QString& startupId, const QString& stepId, FleetStartupStep step,
        std::function<bool()> authorizationFence);
    QVariantMap fleetStartupStepReceipt(QObject* transactionObject, const QString& startupId, const QString& stepId) const;
    bool fleetStartupStepReceiptSourceMatches(QObject* transactionObject, LinkInterface* expectedPrimary,
        const QString& startupId, const QString& stepId) const;
    bool cancelFleetStartupStep(QObject* transactionObject, const QString& startupId, const QString& stepId);
    bool fleetStartupTransportQuiescent() const;
    // Passive recovery-review admission only. Includes normal command/wait,
    // mission/fence/rally, parameter and FTP queues, not just native RX stacks.
    // No request is sent; this does not prove remote task absence or a lease.
    bool fleetOccupancyRecoveryTransportQuiescent(LinkInterface* expectedPrimary) const;
    // Passive source identity only, valid even while a same-source RAW receive
    // stack is active. No link virtual, freshness/ground check, or permission.
    bool fleetOccupancyRecoverySourceMatches(LinkInterface* expectedPrimary,
        const QString& expectedFlightEpoch, const QString& expectedVersionEpoch) const;
    // Whole-set review's FINAL passive fence. Exact original two-channel
    // semantic identity, <=2s age, ground/disarmed and all local queues quiet;
    // no virtual link getter, OS call, notification or outgoing request.
    bool fleetOccupancyRecoveryGroundMatches(LinkInterface* expectedPrimary,
        const QVariantMap& frozenStableIdentity) const;
    // Supplementary RAW coordinates, not estimator validity or a rebase permit.
    // Original six-channel flight-state schema remains unchanged.
    QVariantMap startupCoordinateObservation(qint64 maximumAgeMs = 3000) const;
    bool startupCoordinateObservationSourceMatches(LinkInterface* expectedPrimary) const;
    // Independent passive MAVLink2 ODOMETRY receipts. Metadata is not PX4
    // local/home validity, a clock/frame relation, or permission to rebase.
    QVariantMap startupOdometryObservation(qint64 maximumAgeMs = 3000) const;
    QVariantMap startupOdometryReferenceObservation(qint64 maximumAgeMs = 3000) const;
    bool startupOdometryObservationSourceMatches(LinkInterface* expectedPrimary) const;
    // Independent, passive event-based GPS_GLOBAL_ORIGIN reference. Age is
    // diagnostic, not a periodic freshness requirement or origin certification.
    QVariantMap startupGlobalOriginObservation() const;
    bool startupGlobalOriginObservationSourceMatches(LinkInterface* expectedPrimary) const;
    // Independent passive ALTITUDE same-message observations. Diagnostic only:
    // no v1-v3 policy/admission or command permission consumes this ledger.
    QVariantMap startupAltitudeObservation(qint64 maximumAgeMs = 3000) const;
    bool startupAltitudeObservationSourceMatches(LinkInterface* expectedPrimary) const;
    bool startupAltitudeHomeReferenceMatches(LinkInterface* expectedPrimary,
        const QVariantMap& homeCoordinates, qint64 maximumAgeMs) const;
    // Independent v4 scope monitor. It reports observed constraints only,
    // never a startup permission, and does not alter the step33 receipt.
    QVariantMap fleetStartupAltitudeMonitor(QObject* actualOwner, LinkInterface* actualPrimary,
        const QString& startupId, const QVariantMap& originalReference) const;
    bool retireFleetStartupAltitudeMonitor(QObject* actualOwner, const QString& startupId);
    // Drain-only identity, including after owner destruction. Never a source
    // or permission check; unrelated/unbound scopes have no owned ALT stack.
    bool fleetStartupAltitudeTransportQuiescent(const QString& startupId, const QString& transactionEpoch) const;
    // Native, passive PRIMARY SYS_STATUS / ESTIMATOR_STATUS receipts only.
    // Independent of the six flight channels and startup/ODOMETRY contracts;
    // age is local monotonic time, not authentication or a flight permission.
    QVariantMap fleetHealthTelemetryObservation(qint64 maximumAgeMs = 3000) const;
    // Passive original PRIMARY BATTERY_STATUS reports. Remaining time is a
    // locally aged autopilot estimate, never measured endurance or a permit.
    // C++ only: portable maps cannot install/restore this native ledger.
    QVariantMap fleetBatteryTelemetryObservation(qint64 maximumAgeMs = 3000) const;
    // Independent bounded decline of locally received SOC reports. This can
    // only lower another energy estimate; it certifies neither pack nor range.
    QVariantMap fleetBatteryDrainObservation(qint64 maximumAgeMs = 3000) const;
    // Passive original PRIMARY MESSAGE_INTERVAL reports for 62/74/83 and
    // separate bounded local receive windows. Callback-free native getter;
    // never requests streams, occupies an observer slot or permits a change.
    QVariantMap optionalTelemetryObservations(qint64 maximumAgeMs = 30000) const;
    // Original PRIMARY/default-component encoded MAVLink reception only.
    // Bounded local delivery observations, not bandwidth, loss, RTT or a permit.
    // C++ only; the getter has no Link virtual call, callback or request.
    QVariantMap fleetLocalReceptionObservation() const;
    // Passive original PRIMARY camera-component 269/270 observations only.
    // C++ memory getter: no URI/name disclosure, requests or control authority.
    QVariantMap fleetCameraStreamObservation(qint64 maximumAgeMs = 3000) const;
    std::shared_ptr<FleetCameraStreamRun> prepareFleetCameraStreamCapture(QObject* actualOwner,
        LinkInterface* expectedPrimary, int componentId, int streamId, qint64 remainingMs,
        const std::function<bool()>& authorizationFence);
    bool advanceFleetCameraStreamCapture(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run);
    QVariantMap fleetCameraStreamCaptureSnapshot(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run) const;
    static QVariantMap fleetCameraStreamCaptureRetiredSnapshot(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run);
    bool cancelFleetCameraStreamCapture(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run);
    bool fleetCameraStreamCaptureTransportQuiescent() const;
    // One fresh native capture may be sealed once for an explicitly reviewed
    // opposite reported RUNNING target. Maps/IDs cannot restore this basis.
    std::shared_ptr<FleetCameraStreamControlBasis> sealFleetCameraStreamControlBasis(QObject* actualCapture,
        const std::shared_ptr<FleetCameraStreamRun>& capture, bool targetRunning, qint64 remainingMs,
        const std::function<bool()>& pureSourceFence);
    bool fleetCameraStreamControlBasisCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const;
    QVariantMap fleetCameraStreamControlBasisSummary(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const;
    std::shared_ptr<FleetCameraStreamControlRun> startFleetCameraStreamControl(QObject* actualControl,
        const std::shared_ptr<FleetCameraStreamControlBasis>& basis, const std::function<bool()>& actualTwoAccountFinalFence);
    bool advanceFleetCameraStreamControl(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run);
    QVariantMap fleetCameraStreamControlSnapshot(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run) const;
    static QVariantMap fleetCameraStreamControlRetiredSnapshot(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run);
    bool cancelFleetCameraStreamControl(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run);
    // One explicit targeted TIMESYNC-v2 diagnostic. The opaque native run
    // cannot be recreated from IDs/maps; no clock, link or flight authority.
    std::shared_ptr<FleetLinkLatencyRun> prepareFleetLinkLatencyCapture(QObject* actualOwner,
        LinkInterface* expectedPrimary, qint64 remainingMs, const std::function<bool()>& authorizationFence);
    bool advanceFleetLinkLatencyCapture(QObject* actualOwner, const std::shared_ptr<FleetLinkLatencyRun>& run);
    QVariantMap fleetLinkLatencyCaptureSnapshot(QObject* actualOwner, const std::shared_ptr<FleetLinkLatencyRun>& run) const;
    static QVariantMap fleetLinkLatencyCaptureRetiredSnapshot(QObject* actualOwner,
        const std::shared_ptr<FleetLinkLatencyRun>& run);
    bool cancelFleetLinkLatencyCapture(QObject* actualOwner, const std::shared_ptr<FleetLinkLatencyRun>& run);
    bool fleetLinkLatencyCaptureTransportQuiescent() const;
    // Explicit native read service only. COMMAND 510, one of 62/74/83;
    // accepted ACK plus an original matching 244 are observations, not an
    // authenticated response, stream change, measured rate or permission.
    QVariantMap beginFleetMessageIntervalQuery(QObject* actualOwner, LinkInterface* expectedPrimary,
        int messageId, std::function<bool()> authorizationFence);
    QVariantMap fleetMessageIntervalQuerySnapshot(QObject* actualOwner, const QString& requestId) const;
    bool cancelFleetMessageIntervalQuery(QObject* actualOwner, const QString& requestId);
    // A temporary RAW-stack fence, not a source/freshness/authorization test.
    // The service checks its original deadline/source separately before waiting.
    bool fleetMessageIntervalQueryTransportQuiescent() const;
    std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> sealFleetOptionalTelemetryCapture(QObject* actualCapture,
        const QStringList& queryIds, qint64 remainingMs, std::function<bool()> pureSourceFence);
    bool fleetOptionalTelemetryConfigurationBasisCurrent(
        const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const;
    QVariantMap fleetOptionalTelemetryConfigurationBasisSummary(
        const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const;
    std::shared_ptr<FleetOptionalTelemetryConfigurationRun> startFleetOptionalTelemetryConfiguration(QObject* actualConfig,
        const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis, int positiveExactIntervalUs,
        std::function<bool()> actualTwoAccountFinalFence);
    bool advanceFleetOptionalTelemetryConfiguration(QObject* actualConfig,
        const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run);
    QVariantMap fleetOptionalTelemetryConfigurationSnapshot(QObject* actualConfig,
        const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run) const;
    static QVariantMap fleetOptionalTelemetryConfigurationRetiredSnapshot(QObject* actualConfig,
        const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run);
    bool cancelFleetOptionalTelemetryConfiguration(QObject* actualConfig,
        const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run);
    // Native observed ground POSCTL source only, not an execution permit.
    // Allows either armed state only when current cache and original RAW agree.
    bool fleetArmedGroundReviewSourceMatches(LinkInterface* expectedPrimary) const;

    /// Native C++ only: six fixed raw flight-state receive channels. Never
    /// uses display/cache state, requests telemetry or issues a command.
    /// All age limits are monotonic milliseconds in the 1..120000 domain.
    QVariantMap flightStateObservation(qint64 maximumPositionAgeMs = 3000,
        qint64 maximumStatusAgeMs = 5000, qint64 maximumHomeAgeMs = 60000,
        qint64 maximumProgressAgeMs = 10000) const;
    /// Native risk identity keeps full RAW evidence, with a separate monotonic
    /// revision/hash excluding ONLY GLOBAL_POSITION_INT's transport boot clock.
    /// Does not authenticate telemetry or attribute execution/completion.
    QVariantMap flightStateRiskObservation(qint64 maximumPositionAgeMs = 3000,
        qint64 maximumStatusAgeMs = 5000, qint64 maximumHomeAgeMs = 60000,
        qint64 maximumProgressAgeMs = 10000) const;

    bool soloFirmware() const { return _soloFirmware; }
    void setSoloFirmware(bool soloFirmware);

    int defaultComponentId() const{ return _defaultComponentId; }

    /// Sets the default component id for an offline editing vehicle
    void setOfflineEditingDefaultComponentId(int defaultComponentId);

    /// @return -1 = Unknown, Number of motors on vehicle
    int motorCount();

    /// @return true: Motors are coaxial like an X8 config, false: Quadcopter for example
    bool coaxialMotors();

    /// @return true: X confiuration, false: Plus configuration
    bool xConfigMotors();

    /// @return Firmware plugin instance data associated with this Vehicle
    QObject* firmwarePluginInstanceData() { return _firmwarePluginInstanceData; }

    /// Sets the firmware plugin instance data associated with this Vehicle. This object will be parented to the Vehicle
    /// and destroyed when the vehicle goes away.
    void setFirmwarePluginInstanceData(QObject* firmwarePluginInstanceData);

    QString vehicleImageOpaque  () const;
    QString vehicleImageOutline () const;
    QString vehicleImageCompass () const;

    const QVariantList&         toolIndicators      ();
    const QVariantList&         modeIndicators      ();
    const QVariantList&         staticCameraList    () const;

    bool capabilitiesKnown      () const { return _capabilityBitsKnown; }
    uint64_t capabilityBits     () const { return _capabilityBits; }    // Change signalled by capabilityBitsChanged

    QGCCameraManager*           cameraManager       () { return _cameraManager; }
    QString                     hobbsMeter          ();

    /// The vehicle is responsible for making the initial request for the Plan.
    /// @return: true: initial request is complete, false: initial request is still in progress;
    bool initialPlanRequestComplete() const { return _initialPlanRequestComplete; }

    void forceInitialPlanRequestComplete();

    void _setFlying(bool flying);
    void _setLanding(bool landing);
    void _setHomePosition(QGeoCoordinate& homeCoord);
    void _setMaxProtoVersion(unsigned version);
    void _setMaxProtoVersionFromBothSources();

    /// Vehicle is about to be deleted
    void prepareDelete();

    quint64     mavlinkSentCount        () const{ return _mavlinkSentCount; }        /// Calculated total number of messages sent to us
    quint64     mavlinkReceivedCount    () const{ return _mavlinkReceivedCount; }    /// Total number of sucessful messages received
    quint64     mavlinkLossCount        () const{ return _mavlinkLossCount; }        /// Total number of lost messages
    float       mavlinkLossPercent      () const{ return _mavlinkLossPercent; }      /// Running loss rate

    qreal       gimbalRoll              () const{ return static_cast<qreal>(_curGimbalRoll);}
    qreal       gimbalPitch             () const{ return static_cast<qreal>(_curGimbalPitch); }
    qreal       gimbalYaw               () const{ return static_cast<qreal>(_curGimbalYaw); }
    bool        gimbalData              () const{ return _haveGimbalData; }
    bool        isROIEnabled            () const{ return _isROIEnabled; }

    CheckList   checkListState          () { return _checkListState; }
    void        setCheckListState       (CheckList cl)  { _checkListState = cl; emit checkListStateChanged(); }

    double loadProgress                 () const { return _loadProgress; }

    void setEventsMetadata(uint8_t compid, const QString& metadataJsonFileName);
    void setActuatorsMetadata(uint8_t compid, const QString& metadataJsonFileName);

    HealthAndArmingCheckReport* healthAndArmingCheckReport() { return &_healthAndArmingCheckReport; }

public slots:
    void setVtolInFwdFlight                 (bool vtolInFwdFlight);
    void _offlineFirmwareTypeSettingChanged (QVariant varFirmwareType); // Should only be used by MissionControler to set firmware from Plan file
    void _offlineVehicleTypeSettingChanged  (QVariant varVehicleType);  // Should only be used by MissionController to set vehicle type from Plan file

signals:
    void fleetStartupStepObservationChanged();
    void coordinateChanged              (QGeoCoordinate coordinate);
    void joystickEnabledChanged         (bool enabled);
    void mavlinkMessageReceived         (const mavlink_message_t& message);
    void homePositionChanged            (const QGeoCoordinate& homePosition);
    void armedPositionChanged();
    void armedChanged                   (bool armed);
    void flightModeChanged              (const QString& flightMode);
    void flyingChanged                  (bool flying);
    void landingChanged                 (bool landing);
    void guidedModeChanged              (bool guidedMode);
    void vtolInFwdFlightChanged         (bool vtolInFwdFlight);
    void prearmErrorChanged             (const QString& prearmError);
    void soloFirmwareChanged            (bool soloFirmware);
    void defaultCruiseSpeedChanged      (double cruiseSpeed);
    void defaultHoverSpeedChanged       (double hoverSpeed);
    void firmwareTypeChanged            ();
    void vehicleTypeChanged             ();
    // Source revocation only: never a request, write, authentication or flight
    // permit. Emitted after a new raw version source identity is committed.
    void autopilotVersionObservationSourceChanged();
    void cameraManagerChanged           ();
    void hobbsMeterChanged              ();
    void capabilitiesKnownChanged       (bool capabilitiesKnown);
    void initialPlanRequestCompleteChanged(bool initialPlanRequestComplete);
    void capabilityBitsChanged          (uint64_t capabilityBits);
    void toolIndicatorsChanged          ();
    void modeIndicatorsChanged          ();
    void textMessageReceived            (int uasid, int componentid, int severity, QString text, QString description);
    void calibrationEventReceived       (int uasid, int componentid, int severity, QSharedPointer<events::parser::ParsedEvent> event);
    void checkListStateChanged          ();
    void messagesReceivedChanged        ();
    void messagesSentChanged            ();
    void messagesLostChanged            ();
    void messageTypeChanged             ();
    void newMessageCountChanged         ();
    void messageCountChanged            ();
    void formattedMessagesChanged       ();
    void newFormattedMessage            (QString formattedMessage);
    void longitudeChanged               ();
    void currentConfigChanged           ();
    void flowImageIndexChanged          ();
    void rcRSSIChanged                  (int rcRSSI);
    void telemetryRRSSIChanged          (int value);
    void telemetryLRSSIChanged          (int value);
    void telemetryRXErrorsChanged       (unsigned int value);
    void telemetryFixedChanged          (unsigned int value);
    void telemetryTXBufferChanged       (unsigned int value);
    void telemetryLNoiseChanged         (int value);
    void telemetryRNoiseChanged         (int value);
    void autoDisarmChanged              ();
    void flightModesChanged             ();
    void sensorsPresentBitsChanged      (int sensorsPresentBits);
    void sensorsEnabledBitsChanged      (int sensorsEnabledBits);
    void sensorsHealthBitsChanged       (int sensorsHealthBits);
    void sensorsUnhealthyBitsChanged    (int sensorsUnhealthyBits);
    void orbitActiveChanged             (bool orbitActive);
    void readyToFlyAvailableChanged     (bool readyToFlyAvailable);
    void readyToFlyChanged              (bool readyToFy);
    void allSensorsHealthyChanged       (bool allSensorsHealthy);
    void requiresGpsFixChanged          ();
    void haveMRSpeedLimChanged          ();
    void haveFWSpeedLimChanged          ();

    void firmwareVersionChanged         ();
    void firmwareCustomVersionChanged   ();
    void gitHashChanged                 (QString hash);
    void vehicleUIDChanged              ();
    void loadProgressChanged            (float value);

    /// New RC channel values coming from RC_CHANNELS message
    ///     @param channelCount Number of available channels, cMaxRcChannels max
    ///     @param pwmValues -1 signals channel not available
    void rcChannelsChanged              (int channelCount, int pwmValues[cMaxRcChannels]);

    /// Remote control RSSI changed  (0% - 100%)
    void remoteControlRSSIChanged       (uint8_t rssi);

    void mavlinkRawImu                  (mavlink_message_t message);
    void mavlinkScaledImu1              (mavlink_message_t message);
    void mavlinkScaledImu2              (mavlink_message_t message);
    void mavlinkScaledImu3              (mavlink_message_t message);

    // Mavlink Log Download
    void mavlinkLogData                 (Vehicle* vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t first_message, QByteArray data, bool acked);

    /// Signalled in response to usage of sendMavCommand
    ///     @param vehicleId        Vehicle which command was sent to
    ///     @param targetComponent  Component which command was sent to
    ///     @param command          Command which was sent
    ///     @param ackResult        MAV_RESULT returned in ack
    ///     @param failureCode      More detailed failure code Vehicle::MavCmdResultFailureCode_t
    void mavCommandResult               (int vehicleId, int targetComponent, int command, int ackResult, int failureCode);

    // MAVlink Serial Data
    void mavlinkSerialControl           (uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

    // MAVLink protocol version
    void requestProtocolVersion         (unsigned version);
    void mavlinkStatusChanged           ();

    void gimbalRollChanged              ();
    void gimbalPitchChanged             ();
    void gimbalYawChanged               ();
    void gimbalDataChanged              ();
    void isROIEnabledChanged            ();
    void initialConnectComplete         ();

    void sensorsParametersResetAck      (bool success);

private slots:
    void _mavlinkMessageReceived            (LinkInterface* link, mavlink_message_t message);
    void _sendMessageMultipleNext           ();
    void _parametersReady                   (bool parametersReady);
    void _remoteControlRSSIChanged          (uint8_t rssi);
    void _handleFlightModeChanged           (const QString& flightMode);
    void _announceArmedChanged              (bool armed);
    void _offlineCruiseSpeedSettingChanged  (QVariant value);
    void _offlineHoverSpeedSettingChanged   (QVariant value);
    void _handleTextMessage                 (int newCount);
    void _handletextMessageReceived         (UASMessage* message);
    void _imageProtocolImageReady           (void);
    void _prearmErrorTimeout                ();
    void _firstMissionLoadComplete          ();
    void _firstGeoFenceLoadComplete         ();
    void _firstRallyPointLoadComplete       ();
    void _firstMissionLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested);
    void _firstGeoFenceLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested);
    void _firstRallyPointLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested);
    void _sendMavCommandResponseTimeoutCheck();
    void _clearCameraTriggerPoints          ();
    void _clearCameraTriggerPointsWithOperationId(quint64 operationId, bool status);
    void _clearTrajectoryPointsWithOperationId(quint64 operationId, bool status);
    void _updateDistanceHeadingToHome       ();
    void _updateMissionItemIndex            ();
    void _updateHeadingToNextWP             ();
    void _updateDistanceToGCS               ();
    void _updateHomepoint                   ();
    void _updateHobbsMeter                  ();
    void _vehicleParamLoaded                (bool ready);
    void _sendQGCTimeToVehicle              ();
    void _mavlinkMessageStatus              (int uasId, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent);
    void _orbitTelemetryTimeout             ();
    void _updateFlightTime                  ();
    void _gotProgressUpdate                 (float progressValue);
    void _doSetHomeTerrainReceived          (bool success, QList<double> heights);
    void _updateAltAboveTerrain             ();
    void _altitudeAboveTerrainReceived      (bool sucess, QList<double> heights);

private:
    // ParameterManager only: retain the original cache HASH acknowledgement
    // source through the actual link-write entry, not a general sender API.
    bool _sendParameterCacheHashAck(LinkInterface* expectedLink, mavlink_message_t message,
                                   std::function<bool()> sourceFence);
    // ParameterManager import only. One native row owns this plain ledger
    // across retries; writer entry remains a fact after cancellation/death.
    struct ParameterImportWriteLedger { quint64 entered = 0; };
    bool _parameterImportEncoding(int componentId, bool& numericFloat) const;
    bool _sendParameterImport(LinkInterface* original, mavlink_message_t logical,
                              const mavlink_param_set_t& expectedWire, std::function<bool()> sourceFence,
                              const std::shared_ptr<ParameterImportWriteLedger>& ledger);
    bool _fleetTelemetrySourceAllowed(LinkInterface* link) const;
    void _observeFleetTelemetry(const mavlink_message_t& originalMessage, LinkInterface* link);
    void _retireFleetTelemetryObserver(const QString& fixedCode);
    bool _fleetExecutionSourceAllowed(LinkInterface* link) const;
    bool _advanceFleetExecutionSlotRevision();
    bool _bindFleetExecutionObserver(VehicleMissionExecutionObserver* observer, LinkInterface* link,
        VehicleMissionExecutionObserver::QueueContext* context);
    void _observeFleetExecution(const mavlink_message_t& originalMessage, LinkInterface* link);
    void _retireFleetExecutionObserver(const QString& fixedCode, bool deferNotification = false);
    // Transport-side safety gates for C++/QML command entry points. UI checks
    // remain useful for UX, but stale dialogs and plugins must not bypass these
    // backend checks.
    bool _commandTransportAvailable(const char* operation) const;
    bool _maintenanceCommandAllowed(const char* operation) const;

    void _initializeAutopilotVersionObservation();
    bool _autopilotVersionSourceAllowed(LinkInterface* sourceLink) const;
    void _invalidateAutopilotVersionObservation(const QString& source);
    quint64 _prepareAutopilotVersionObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink);
    bool _autopilotVersionObservationUpdateCurrent(quint64 token, const QString& epoch) const;
    void _completeAutopilotVersionObservation(quint64 token, const QString& epoch,
        const mavlink_message_t& originalMessage, LinkInterface* sourceLink, qint64 receivedAtMs);
    void _invalidateFlightStateObservations(const QString& source);
    void _invalidateFlightStateObservation(const QString& channel, const QString& source);
    quint64 _prepareFlightStateObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink,
        QString* channel, QVariantMap* value);
    bool _flightStateObservationUpdateCurrent(const QString& channel, quint64 token, const QString& epoch) const;
    void _completeFlightStateObservation(const QString& channel, quint64 token, const QString& epoch,
        const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs);
    void _initializeFleetHealthTelemetryObservation();
    void _invalidateFleetHealthTelemetryObservation(const QString& source);
    bool _fleetHealthTelemetrySourceAllowed(LinkInterface* sourceLink) const;
    bool _fleetHealthTelemetrySourceMatches() const;
    quint64 _prepareFleetHealthTelemetryObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink,
        QString* channel, QVariantMap* value);
    void _completeFleetHealthTelemetryObservation(const QString& channel, quint64 token, const QString& epoch,
        const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs, qint64 startupReceivedAtMs);
    void _initializeFleetBatteryTelemetryObservation();
    void _invalidateFleetBatteryTelemetryObservation();
    bool _fleetBatteryTelemetrySourceAllowed(LinkInterface* sourceLink) const;
    bool _fleetBatteryTelemetrySourceMatches() const;
    bool _fleetBatteryTelemetryMemoryCurrent() const;
    quint64 _prepareFleetBatteryTelemetryObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink,
        int* batteryId, QVariantMap* value);
    struct FleetBatteryDrainIngress {
        QPointer<LinkInterface> link;
        WeakLinkInterfacePtr primary;
        QString versionEpoch;
        qint64 receivedAtMs = -1;
        quint64 baselineReceipt = 0;
        int vehicleId = 0, componentId = 0, firmwareType = -1, vehicleType = -1;
        bool eligible = false;
    };
    struct FleetBatteryTelemetryRow;
    FleetBatteryDrainIngress _prepareFleetBatteryDrainIngress(const mavlink_message_t& originalMessage, LinkInterface* link) const;
    void _observeFleetBatteryDrain(FleetBatteryTelemetryRow& row, quint64 receipt, const FleetBatteryDrainIngress& ingress);
    void _resetFleetBatteryDrain(FleetBatteryTelemetryRow& row);
    void _advanceFleetBatteryDrainRevision();
    void _completeFleetBatteryTelemetryObservation(int batteryId, quint64 token, const QString& epoch,
        const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs, const FleetBatteryDrainIngress& drainIngress);
    QVariantMap _fleetStartupEstimatorProjection(bool requireHealthy) const;
    bool _fleetStartupEstimatorMemoryCurrent(bool requireHealthy) const;
    void _initializeStartupGlobalOriginObservation();
    void _invalidateStartupGlobalOriginObservation(const QString& source);
    bool _startupGlobalOriginSourceAllowed(LinkInterface* sourceLink) const;
    bool _startupGlobalOriginMemoryCurrent(LinkInterface* expectedPrimary, bool requireQuiescent) const;
    quint64 _prepareStartupGlobalOriginObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink);
    void _completeStartupGlobalOriginObservation(quint64 token, const QString& epoch,
        const mavlink_message_t& originalMessage, LinkInterface* sourceLink, qint64 receivedAtMs);
    QVariantMap _startupGlobalOriginStableReference() const;
    bool _fleetStartupGlobalOriginBasisCurrent() const;
    void _initializeStartupAltitudeObservation();
    void _invalidateStartupAltitudeObservation(const QString& source, bool sourceLost);
    bool _startupAltitudeSourceAllowed(LinkInterface* sourceLink) const;
    bool _startupAltitudeMemoryCurrent(LinkInterface* expectedPrimary, bool requireQuiescent) const;
    quint64 _prepareStartupAltitudeObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink, qint64 receivedAtMs);
    void _completeStartupAltitudeObservation(quint64 token, const QString& epoch,
        const mavlink_message_t& originalMessage, LinkInterface* sourceLink, qint64 receivedAtMs);
    bool _bindFleetStartupAltitude(const QVariantMap& actualContext);
    void _failFleetStartupAltitude(const QString& reason);
    void _consumeFleetStartupAltitude(const QVariantMap& decoded, qint64 receivedAtMs);
    bool _fleetStartupAltitudeCurrent() const;
    qint64 _fleetStartupAltitudeMaximumGap() const;

    void _loadJoystickSettings          ();
    void _activeVehicleChanged          (Vehicle* newActiveVehicle);
    void _captureJoystick               ();
    void _handlePing                    (LinkInterface* link, mavlink_message_t& message);
    void _handleHomePosition            (mavlink_message_t& message);
    void _handleHeartbeat               (mavlink_message_t& message);
    void _handleCurrentMode             (mavlink_message_t& message);
    void _handleRadioStatus             (mavlink_message_t& message);
    void _handleRCChannels              (mavlink_message_t& message);
    void _handleBatteryStatus           (mavlink_message_t& message);
    void _handleSysStatus               (mavlink_message_t& message);
    void _handleExtendedSysState        (mavlink_message_t& message);
    void _handleCommandAck              (mavlink_message_t& message, LinkInterface* sourceLink = nullptr,
        const mavlink_message_t* originalMessage = nullptr);
    void _handleGpsRawInt               (mavlink_message_t& message);
    void _handleGlobalPositionInt       (mavlink_message_t& message);
    void _handleAltitude                (mavlink_message_t& message);
    void _handleVfrHud                  (mavlink_message_t& message);
    void _handleNavControllerOutput     (mavlink_message_t& message);
    void _handleHighLatency             (mavlink_message_t& message);
    void _handleHighLatency2            (mavlink_message_t& message);
    void _handleAttitudeWorker          (double rollRadians, double pitchRadians, double yawRadians);
    void _handleAttitude                (mavlink_message_t& message);
    void _handleAttitudeQuaternion      (mavlink_message_t& message);
    void _handleStatusText              (mavlink_message_t& message);
    void _handleOrbitExecutionStatus    (const mavlink_message_t& message);
    void _handleGimbalOrientation       (const mavlink_message_t& message);
    void _handleObstacleDistance        (const mavlink_message_t& message);
    void _handleFenceStatus             (const mavlink_message_t& message);
    void _handleEvent(uint8_t comp_id, std::unique_ptr<events::parser::ParsedEvent> event);
    // ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    void _handleCameraFeedback          (const mavlink_message_t& message);
    void _handleRangefinder             (mavlink_message_t& message);
#endif
    void _handleCameraImageCaptured     (const mavlink_message_t& message);
    void _handleADSBVehicle             (const mavlink_message_t& message);
    void _handleRawImuTemp              (mavlink_message_t& message);
    void _missionManagerError           (int errorCode, const QString& errorMsg);
    void _geoFenceManagerError          (int errorCode, const QString& errorMsg);
    void _rallyPointManagerError        (int errorCode, const QString& errorMsg);
    void _say                           (const QString& text);
    QString _vehicleIdSpeech            ();
    void _handleMavlinkLoggingData      (mavlink_message_t& message);
    void _handleMavlinkLoggingDataAcked (mavlink_message_t& message);
    void _ackMavlinkLogData             (uint16_t sequence);
    void _commonInit                    ();
    void _setupAutoDisarmSignalling     ();
    void _setCapabilities               (uint64_t capabilityBits);
    void _updateArmed                   (bool armed);
    bool _apmArmingNotRequired          ();
    void _initializeCsv                 ();
    void _writeCsvLine                  ();
    void _flightTimerStart              ();
    void _flightTimerStop               ();
    void _chunkedStatusTextTimeout      (void);
    void _chunkedStatusTextCompleted    (uint8_t compId);
    void _setMessageInterval            (MAVLinkStreamConfig::RequestId requestId, int messageId, int intervalUs);
    static void _streamIntervalCommandResult(void*, int, const mavlink_command_ack_t&, MavCmdResultFailureCode_t);
    EventHandler& _eventHandler         (uint8_t compid);
    bool setFlightModeCustom            (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode);

    static void _rebootCommandResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);

    int     _id;                    ///< Mavlink system id
    int     _defaultComponentId;
    bool    _offlineEditingVehicle = false; ///< true: This Vehicle is a "disconnected" vehicle for ui use while offline editing

    MAV_AUTOPILOT       _firmwareType;
    MAV_TYPE            _vehicleType;
    FirmwarePlugin*     _firmwarePlugin = nullptr;
    QObject*            _firmwarePluginInstanceData = nullptr;
    AutoPilotPlugin*    _autopilotPlugin = nullptr;
    MAVLinkProtocol*    _mavlink = nullptr;
    bool                _soloFirmware = false;
    QGCToolbox*         _toolbox = nullptr;
    SettingsManager*    _settingsManager = nullptr;

    QTimer              _csvLogTimer;
    QFile               _csvLogFile;

    bool            _joystickEnabled = false;

    UAS* _uas = nullptr;

    QGeoCoordinate  _coordinate;
    QGeoCoordinate  _homePosition;
    QGeoCoordinate  _armedPosition;

    UASInterface*   _mav = nullptr;
    int             _currentMessageCount = 0;
    int             _messageCount = 0;
    int             _currentErrorCount = 0;
    int             _currentWarningCount = 0;
    int             _currentNormalCount = 0;
    MessageType_t   _currentMessageType = MessageNone;
    int             _updateCount = 0;
    int             _rcRSSI = 255;
    double          _rcRSSIstore = 255;
    bool            _flying = false;
    bool            _landing = false;
    bool            _vtolInFwdFlight = false;
    uint32_t        _onboardControlSensorsPresent = 0;
    uint32_t        _onboardControlSensorsEnabled = 0;
    uint32_t        _onboardControlSensorsHealth = 0;
    uint32_t        _onboardControlSensorsUnhealthy = 0;
    bool            _gpsRawIntMessageAvailable              = false;
    bool            _gps2RawMessageAvailable                = false;
    bool            _globalPositionIntMessageAvailable      = false;
    bool            _altitudeMessageAvailable               = false;
    double          _defaultCruiseSpeed = qQNaN();
    double          _defaultHoverSpeed = qQNaN();
    int             _telemetryRRSSI = 0;
    int             _telemetryLRSSI = 0;
    uint32_t        _telemetryRXErrors = 0;
    uint32_t        _telemetryFixed = 0;
    uint32_t        _telemetryTXBuffer = 0;
    int             _telemetryLNoise = 0;
    int             _telemetryRNoise = 0;
    bool            _mavlinkProtocolRequestComplete         = false;
    unsigned        _mavlinkProtocolRequestMaxProtoVersion  = 0;
    unsigned        _maxProtoVersion                        = 0;
    bool            _capabilityBitsKnown                    = false;
    uint64_t        _capabilityBits                         = 0;
    // One bounded observation; no caller-supplied rows or components. Counter
    // exhaustion is terminal for this Vehicle instance rather than wrapping.
    QVariantMap     _autopilotVersionObservationFields;
    QPointer<LinkInterface> _autopilotVersionObservationLink;
    QMetaObject::Connection _autopilotVersionObservationLinkDestroyedConnection;
    QElapsedTimer   _autopilotVersionObservationClock;
    QPointer<VehicleTelemetryObserver> _fleetTelemetryObserver;
    QPointer<LinkInterface> _fleetTelemetryLink;
    QList<QMetaObject::Connection> _fleetTelemetryConnections;
    QElapsedTimer _fleetTelemetryClock;
    QString _fleetTelemetryEpoch;
    quint64 _fleetTelemetrySequence = 0;
    bool _fleetTelemetryReceiveBusy = false;
    bool _fleetTelemetryRemoving = false;
    QPointer<VehicleMissionExecutionObserver> _fleetExecutionObserver;
    QPointer<LinkInterface> _fleetExecutionLink;
    WeakLinkInterfacePtr _fleetExecutionPrimaryOwner;
    QPointer<VehicleLinkManager> _fleetExecutionSourceLinks;
    QPointer<MAVLinkProtocol> _fleetExecutionSourceProtocol;
    QPointer<FirmwarePlugin> _messageIntervalSourcePlugin;
    QPointer<QGCCorePlugin> _messageCommandSourceCore;
    QPointer<TerrainProtocolHandler> _messageCommandSourceTerrain;
    QPointer<FTPManager> _messageCommandSourceFtp;
    QPointer<ImageProtocolManager> _messageCommandSourceImage;
    QPointer<RemoteIDManager> _messageCommandSourceRemoteId;
    QPointer<MultiVehicleManager> _fleetExecutionSourceVehicles;
    QPointer<QGCToolbox> _fleetExecutionSourceToolbox;
    struct OptionalTelemetryRow {
        qint32 intervalUs = 0;
        qint64 reportedAtMs = -1, newestFrameAtMs = -1, lastDroppedFrameAtMs = -1;
        quint64 reportedReceiptSequence = 0, contentRevision = 0, frameReceiptSequence = 0;
        bool reportedObserved = false;
        QVector<qint64> receivedAtMs;
    };
    QMap<int, OptionalTelemetryRow> _optionalTelemetryRows;
    QElapsedTimer _optionalTelemetryClock;
    QPointer<LinkInterface> _optionalTelemetryPrimary;
    WeakLinkInterfacePtr _optionalTelemetryPrimaryOwner;
    QString _optionalTelemetryLedgerEpoch, _optionalTelemetryVersionEpoch;
    qint64 _optionalTelemetryBoundAtMs = -1;
    quint64 _optionalTelemetryNextReceipt = 0, _optionalTelemetryRevision = 0, _optionalTelemetryReceiveDepth = 0;
    quint64 _optionalTelemetryMutationGeneration = 0, _optionalTelemetryWriteDepth = 0;
    bool _optionalTelemetryMutationExhausted = false;
    int _optionalTelemetryComponentId = 0, _optionalTelemetryFirmwareType = -1, _optionalTelemetryVehicleType = -1;
    bool _optionalTelemetryExhausted = false, _optionalTelemetryProbeBusy = false;
    void _initializeOptionalTelemetryObservations();
    void _invalidateOptionalTelemetryObservations();
    bool _optionalTelemetryRawSourceMatches(LinkInterface* sourceLink) const;
    bool _optionalTelemetrySourceAllowed(LinkInterface* sourceLink);
    bool _optionalTelemetrySourceMemoryCurrent() const;
    void _observeOptionalTelemetry(const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs);
    struct FleetLocalReceptionFrame {
        qint64 receivedAtMs = -1;
        int frameBytes = 0;
    };
    struct FleetLocalReceptionIngress {
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        QPointer<VehicleLinkManager> links;
        QPointer<MAVLinkProtocol> protocol;
        QPointer<MultiVehicleManager> vehicles;
        QPointer<QGCToolbox> toolbox;
        QString sourceEpoch, transportEpoch, versionEpoch;
        quint64 receiptSequence = 0;
        qint64 receivedAtMs = -1;
        int vehicleId = 0, componentId = 0, firmwareType = -1, vehicleType = -1;
        bool eligible = false;
    };
    QElapsedTimer _fleetLocalReceptionClock;
    QVector<FleetLocalReceptionFrame> _fleetLocalReceptionFrames;
    QPointer<LinkInterface> _fleetLocalReceptionPrimary;
    WeakLinkInterfacePtr _fleetLocalReceptionPrimaryOwner;
    QString _fleetLocalReceptionEpoch, _fleetLocalReceptionTransportEpoch, _fleetLocalReceptionVersionEpoch;
    QString _fleetLocalReceptionReason = QStringLiteral("local-reception-unobserved");
    qint64 _fleetLocalReceptionBoundAtMs = -1, _fleetLocalReceptionNewestAtMs = -1, _fleetLocalReceptionLastDroppedAtMs = -1;
    quint64 _fleetLocalReceptionNextReceipt = 0;
    int _fleetLocalReceptionVehicleId = 0, _fleetLocalReceptionComponentId = 0;
    int _fleetLocalReceptionFirmwareType = -1, _fleetLocalReceptionVehicleType = -1;
    bool _fleetLocalReceptionExhausted = false, _fleetLocalReceptionProbeBusy = false, _fleetLocalReceptionRemoving = false;
    void _initializeFleetLocalReceptionObservation();
    void _invalidateFleetLocalReceptionObservation(const QString& reason);
    bool _fleetLocalReceptionRawSourceMatches(LinkInterface* sourceLink) const;
    FleetLocalReceptionIngress _prepareFleetLocalReceptionIngress(const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs);
    void _observeFleetLocalReception(const mavlink_message_t& original, const FleetLocalReceptionIngress& originalIngress);
    struct FleetCameraStreamRow {
        mavlink_video_stream_information_t information{};
        mavlink_video_stream_status_t status{};
        qint64 informationAtMs = -1, statusAtMs = -1;
        quint64 informationReceipt = 0, statusReceipt = 0;
        quint64 informationRevision = 0, statusRevision = 0, statusInformationRevision = 0;
        // Per-row configuration token, allocated from the same never-reused
        // native revision sequence; only Info RUNNING changes retain it.
        quint64 configurationRevision = 0;
        bool informationObserved = false, statusObserved = false;
        bool informationValid = false, statusValid = false;
    };
    struct FleetCameraStreamIngress {
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        QPointer<VehicleLinkManager> links;
        QPointer<MAVLinkProtocol> protocol;
        QPointer<MultiVehicleManager> vehicles;
        QPointer<QGCToolbox> toolbox;
        QString sourceEpoch, transportEpoch, versionEpoch;
        quint64 receiptSequence = 0;
        qint64 receivedAtMs = -1;
        int vehicleId = 0, componentId = 0, defaultComponentId = 0, firmwareType = -1, vehicleType = -1;
        bool eligible = false;
    };
    QElapsedTimer _fleetCameraStreamClock;
    QMap<int, FleetCameraStreamRow> _fleetCameraStreamRows;
    QMap<int, int> _fleetCameraStreamDeclaredCounts;
    QPointer<LinkInterface> _fleetCameraStreamPrimary;
    WeakLinkInterfacePtr _fleetCameraStreamPrimaryOwner;
    QString _fleetCameraStreamEpoch, _fleetCameraStreamTransportEpoch, _fleetCameraStreamVersionEpoch;
    QString _fleetCameraStreamReason = QStringLiteral("camera-stream-unobserved");
    qint64 _fleetCameraStreamBoundAtMs = -1, _fleetCameraStreamNewestAtMs = -1;
    quint64 _fleetCameraStreamNextReceipt = 0, _fleetCameraStreamRevision = 0;
    quint64 _fleetCameraStreamMutation = 0;
    bool _fleetCameraStreamMutationExhausted = false;
    int _fleetCameraStreamVehicleId = 0, _fleetCameraStreamDefaultComponentId = 0;
    int _fleetCameraStreamFirmwareType = -1, _fleetCameraStreamVehicleType = -1;
    bool _fleetCameraStreamExhausted = false, _fleetCameraStreamProbeBusy = false;
    bool _fleetCameraStreamRemoving = false, _fleetCameraStreamSaturated = false;
    void _initializeFleetCameraStreamObservation();
    void _invalidateFleetCameraStreamObservation(const QString& reason);
    bool _fleetCameraStreamRawSourceMatches(LinkInterface* sourceLink) const;
    FleetCameraStreamIngress _prepareFleetCameraStreamIngress(const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs);
    void _observeFleetCameraStream(const mavlink_message_t& original, const FleetCameraStreamIngress& originalIngress);
    static QVariantMap _fleetCameraStreamRowValue(int componentId, int streamId, const FleetCameraStreamRow& row,
        qint64 now, qint64 maximumAgeMs, bool current, bool saturated);
    QList<QMetaObject::Connection> _fleetExecutionConnections;
    QElapsedTimer _fleetExecutionClock;
    QString _fleetExecutionEpoch;
    QString _fleetExecutionVersionEpoch;
    quint64 _fleetExecutionSequence = 0;
    quint64 _fleetExecutionSlotRevision = 0;
    bool _fleetExecutionSlotExhausted = false;
    mutable bool _fleetExecutionSourceProbeBusy = false;
    bool _fleetExecutionBindBusy = false;
    int _fleetExecutionComponentId = 0;
    bool _fleetExecutionReceiveBusy = false;
    bool _fleetExecutionRemoving = false;
    struct FleetHealthTelemetryChannel {
        QVariantMap value;
        QString source = QStringLiteral("unknown");
        quint64 receiptSequence = 0, contentRevision = 0, updateToken = 0;
        quint64 lastEstimatorTimeUsec = 0;
        qint64 receivedAtMs = -1, startupReceivedAtMs = -1;
        bool observed = false, complete = false, updating = false;
        bool estimatorTimeObserved = false, clockDiscontinuityObserved = false;
    };
    QMap<QString, FleetHealthTelemetryChannel> _fleetHealthTelemetryRows;
    QPointer<LinkInterface> _fleetHealthTelemetryLink;
    WeakLinkInterfacePtr _fleetHealthTelemetryWeakLink;
    QPointer<VehicleLinkManager> _fleetHealthTelemetryLinks;
    QPointer<MAVLinkProtocol> _fleetHealthTelemetryProtocol;
    QPointer<MultiVehicleManager> _fleetHealthTelemetryVehicles;
    QList<QMetaObject::Connection> _fleetHealthTelemetryLinkConnections;
    QElapsedTimer _fleetHealthTelemetryClock;
    QString _fleetHealthTelemetryEpoch, _fleetHealthTelemetryVersionEpoch;
    quint64 _fleetHealthTelemetryNextReceipt = 0, _fleetHealthTelemetryRevision = 0;
    quint64 _fleetHealthTelemetryReceiveDepth = 0;
    int _fleetHealthTelemetryComponentId = 0, _fleetHealthTelemetryFirmwareType = -1, _fleetHealthTelemetryVehicleType = -1;
    bool _fleetHealthTelemetryExhausted = false, _fleetHealthTelemetryRemoving = false;
    struct FleetBatteryDrainSample {
        qint64 receivedAtMs = -1, maximumGapMs = 0;
        quint64 receipt = 0;
        int percent = -1;
    };
    struct FleetBatteryDrainWindow {
        quint64 startReceipt = 0, endReceipt = 0;
        qint64 startAtMs = -1, endAtMs = -1, maximumGapMs = -1;
        int startPercent = -1, endPercent = -1, sampleCount = 0;
        bool published = false;
    };
    struct FleetBatteryTelemetryRow {
        QVariantMap value;
        quint64 receiptSequence = 0, contentRevision = 0, updateToken = 0;
        qint64 receivedAtMs = -1;
        bool observed = false, complete = false, updating = false;
        QVector<FleetBatteryDrainSample> drainSamples;
        FleetBatteryDrainWindow drainWindow;
        qint64 drainReceivedAtMs = -1, drainPendingMaximumGapMs = 0;
        int drainLastPercent = -1, drainFunction = -1, drainType = -1;
        bool drainSampleOverflow = false;
    };
    QMap<int, FleetBatteryTelemetryRow> _fleetBatteryTelemetryRows;
    QPointer<LinkInterface> _fleetBatteryTelemetryLink;
    WeakLinkInterfacePtr _fleetBatteryTelemetryWeakLink;
    QPointer<VehicleLinkManager> _fleetBatteryTelemetryLinks;
    QPointer<MAVLinkProtocol> _fleetBatteryTelemetryProtocol;
    QPointer<MultiVehicleManager> _fleetBatteryTelemetryVehicles;
    QList<QMetaObject::Connection> _fleetBatteryTelemetryLinkConnections;
    QElapsedTimer _fleetBatteryTelemetryClock;
    QString _fleetBatteryTelemetryEpoch, _fleetBatteryTelemetryVersionEpoch;
    quint64 _fleetBatteryTelemetryNextReceipt = 0, _fleetBatteryTelemetryRevision = 0;
    quint64 _fleetBatteryTelemetryReceiveDepth = 0;
    int _fleetBatteryTelemetryComponentId = 0, _fleetBatteryTelemetryFirmwareType = -1, _fleetBatteryTelemetryVehicleType = -1;
    bool _fleetBatteryTelemetryExhausted = false, _fleetBatteryTelemetryRemoving = false;
    bool _fleetBatteryTelemetrySlotOverflow = false;
    quint64 _fleetBatteryDrainRevision = 0;
    bool _fleetBatteryDrainExhausted = false;
    QString         _autopilotVersionObservationEpoch;
    QString         _autopilotVersionObservationSource = QStringLiteral("unknown");
    QString         _autopilotVersionObservationContentHash;
    qint64          _autopilotVersionObservationReceivedAtMs = -1;
    quint64         _autopilotVersionObservationRevision = 0;
    quint64         _autopilotVersionObservationUpdateToken = 0;
    quint64         _autopilotVersionObservationLastReceipt = 0;
    qint64          _fleetVersionReadLastAtMs = -1;
    QString         _fleetVersionReadLastIssuedId;
    quint64         _autopilotVersionObservationReceiveDepth = 0;
    bool            _autopilotVersionObservationObserved = false;
    bool            _autopilotVersionObservationComplete = false;
    bool            _autopilotVersionObservationUpdating = false;
    bool            _autopilotVersionObservationExhausted = false;
    bool            _autopilotVersionObservationRemoving = false;
    struct FlightStateObservationChannel {
        QVariantMap value;
        QString source = QStringLiteral("unknown");
        QString contentHash;
        qint64 receivedAtMs = -1;
        quint64 contentRevision = 0;
        QString riskContentHash;
        quint64 riskContentRevision = 0;
        quint64 receiptSequence = 0;
        quint64 updateToken = 0;
        bool observed = false;
        bool complete = false;
        bool updating = false;
    };
    QMap<QString, FlightStateObservationChannel> _flightStateObservations;
    QPointer<LinkInterface> _flightStateObservationLink;
    QMetaObject::Connection _flightStateObservationLinkDestroyedConnection;
    QString _flightStateObservationEpoch;
    quint64 _flightStateObservationRevision = 0;
    quint64 _flightStateRiskObservationRevision = 0;
    quint64 _flightStateObservationReceiptSequence = 0;
    quint64 _fleetMissionObservationReceiveDepth = 0;
    bool _fleetMissionObservationReceiveExhausted = false;
    struct StartupCoordinateRow {
        QVariantMap value;
        QString contentHash, riskContentHash, rawPayloadHash;
        quint64 updateToken = 0, receiptSequence = 0, contentRevision = 0, riskContentRevision = 0;
        qint64 receivedAtMs = -1;
        bool observed = false, complete = false, updating = false;
    };
    QMap<QString, StartupCoordinateRow> _startupCoordinateRows;
    QPointer<LinkInterface> _startupCoordinateLink;
    QString _startupCoordinateEpoch, _startupCoordinateVersionEpoch, _startupCoordinateFlightEpoch;
    quint64 _startupCoordinateNextReceipt = 0, _startupCoordinateContentRevision = 0, _startupCoordinateRiskRevision = 0;
    quint64 _startupCoordinateReceiveDepth = 0;
    bool _startupCoordinateExhausted = false;
    struct StartupOdometryRow {
        QVariantMap value, referenceProjection;
        QString contentHash, referenceHash, rawPayloadHash;
        quint64 updateToken = 0, receiptSequence = 0;
        qint64 receivedAtMs = -1;
        int rawPayloadLength = 0;
        bool observed = false, complete = false, updating = false;
    };
    StartupOdometryRow _startupOdometryRow;
    QPointer<LinkInterface> _startupOdometryLink;
    QMetaObject::Connection _startupOdometryLinkDestroyedConnection;
    QString _startupOdometryEpoch, _startupOdometryVersionEpoch, _startupOdometryFlightEpoch;
    QString _startupOdometrySource = QStringLiteral("unknown");
    QString _startupOdometryLastPreparedReferenceHash;
    quint64 _startupOdometryLastRawSampleTime = 0;
    quint64 _startupOdometryNextReceipt = 0, _startupOdometryContentRevision = 0, _startupOdometryReferenceRevision = 0;
    quint64 _startupOdometryReceiveDepth = 0, _startupOdometryClockDiscontinuityRevision = 0;
    bool _startupOdometryExhausted = false, _startupOdometryClockDiscontinuityObserved = false, _startupOdometryRawSampleObserved = false;
    bool _startupOdometryEverObservedRawFrame = false;
    struct StartupGlobalOriginRow {
        QVariantMap value, referenceProjection;
        QString referenceHash, rawPayloadHash;
        quint64 updateToken = 0, receiptSequence = 0;
        qint64 receivedAtMs = -1;
        int rawPayloadLength = 0;
        bool observed = false, complete = false, updating = false;
    };
    StartupGlobalOriginRow _startupGlobalOriginRow;
    QPointer<LinkInterface> _startupGlobalOriginLink;
    WeakLinkInterfacePtr _startupGlobalOriginWeakLink;
    QPointer<VehicleLinkManager> _startupGlobalOriginLinks;
    QPointer<MAVLinkProtocol> _startupGlobalOriginProtocol;
    QPointer<MultiVehicleManager> _startupGlobalOriginVehicles;
    QList<QMetaObject::Connection> _startupGlobalOriginLinkConnections;
    QElapsedTimer _startupGlobalOriginClock;
    QString _startupGlobalOriginEpoch, _startupGlobalOriginVersionEpoch, _startupGlobalOriginFlightEpoch;
    QString _startupGlobalOriginSource = QStringLiteral("unknown"), _startupGlobalOriginLastPreparedReferenceHash;
    quint64 _startupGlobalOriginNextReceipt = 0, _startupGlobalOriginReferenceRevision = 0, _startupGlobalOriginReceiveDepth = 0;
    int _startupGlobalOriginComponentId = 0, _startupGlobalOriginFirmwareType = -1, _startupGlobalOriginVehicleType = -1;
    bool _startupGlobalOriginEverObservedRawFrame = false, _startupGlobalOriginExhausted = false, _startupGlobalOriginRemoving = false;
    QVariantMap _fleetStartupGlobalOriginBasis;
    struct StartupAltitudeRow {
        QVariantMap value;
        QString rawPayloadHash;
        quint64 updateToken = 0, receiptSequence = 0;
        qint64 receivedAtMs = -1;
        int rawPayloadLength = 0;
        bool observed = false, complete = false, updating = false;
    };
    StartupAltitudeRow _startupAltitudeRow;
    QPointer<LinkInterface> _startupAltitudeLink;
    WeakLinkInterfacePtr _startupAltitudeWeakLink;
    QPointer<VehicleLinkManager> _startupAltitudeLinks;
    QPointer<MAVLinkProtocol> _startupAltitudeProtocol;
    QPointer<MultiVehicleManager> _startupAltitudeVehicles;
    QList<QMetaObject::Connection> _startupAltitudeLinkConnections;
    QElapsedTimer _startupAltitudeClock;
    QString _startupAltitudeEpoch, _startupAltitudeVersionEpoch, _startupAltitudeFlightEpoch;
    QString _startupAltitudeSource = QStringLiteral("unknown"), _startupAltitudeLastPreparedStateHash;
    quint64 _startupAltitudeNextReceipt = 0, _startupAltitudeStateRevision = 0, _startupAltitudeReceiveDepth = 0;
    quint64 _startupAltitudeLastRawSampleTime = 0, _startupAltitudeClockDiscontinuityRevision = 0;
    int _startupAltitudeComponentId = 0, _startupAltitudeFirmwareType = -1, _startupAltitudeVehicleType = -1;
    bool _startupAltitudeEverObservedRawFrame = false, _startupAltitudeExhausted = false, _startupAltitudeRemoving = false;
    bool _startupAltitudeRawSampleObserved = false, _startupAltitudeClockDiscontinuityObserved = false;
    bool _startupAltitudeLinkHooksReady = false;
    struct FleetStartupAltitudeMonitor {
        QVariantMap context;
        QString reason;
        QString sourceEpoch, flightEpoch, altitudeEpoch, coordinateEpoch;
        WeakLinkInterfacePtr primary;
        quint64 clockRevision = 0, homeRiskRevision = 0;
        quint64 baselineReceipt = 0, frameCount = 0;
        qint64 maximumGapMs = -1, lastReceivedAtMs = -1, endedAtMs = -1;
        bool required = false, bound = false, violation = false, retired = false;
    };
    FleetStartupAltitudeMonitor _fleetStartupAltitude;
    QPointer<FleetMissionStartupTransaction> _fleetStartupOwner;
    QPointer<LinkInterface> _fleetStartupLink;
    QMetaObject::Connection _fleetStartupOwnerDestroyedConnection;
    QString _fleetStartupId, _fleetStartupStepId, _fleetStartupOwnerEpoch, _fleetStartupVersionEpoch, _fleetStartupFlightEpoch;
    QString _fleetStartupWireHash, _fleetStartupCloseReason;
    QString _fleetStartupJournalStepId;
    QString _fleetStartupEstimatorSourceEpoch;
    qint64 _fleetStartupEstimatorMaximumGapMs = -1;
    struct FleetStartupEstimatorIngress {
        QPointer<FleetMissionStartupTransaction> owner;
        QString startupId, ownerEpoch, journalStepId, operationId;
        QString versionEpoch, flightEpoch, healthEpoch;
    };
    struct FleetStartupRawIngress {
        QPointer<FleetMissionStartupTransaction> owner;
        QPointer<LinkInterface> primary;
        QPointer<MAVLinkProtocol> protocol;
        QString startupId, ownerEpoch, journalStepId, operationId;
        QString versionEpoch, flightEpoch, healthEpoch;
        int vehicleId = 0, componentId = 0;
        qint64 receivedAtMs = -1;
        QVariant issuedAtMs;
        quint64 rawSequence = 0;
        bool eligible = false;
    };
    FleetStartupStep _fleetStartupStep = FleetStartupStep::Seq0;
    quint64 _fleetStartupNextOperation = 0, _fleetStartupOperation = 0, _fleetStartupRawReceiptSequence = 0;
    quint64 _fleetStartupBaselineReceipt = 0, _fleetStartupReceiveDepth = 0;
    qint64 _fleetStartupIssuedAtMs = -1, _fleetStartupExpiresAtMs = -1;
    bool _fleetStartupExhausted = false, _fleetStartupEntryBusy = false, _fleetStartupActive = false;
    bool _fleetStartupRequestIssued = false, _fleetStartupClosed = true;
    bool _fleetStartupScopeCancelled = false;
    bool _fleetStartupQueueInProgress = false;
    // Covers only the native deployment packet fence/plugin/queue stack.
    // It is not a remote-idle observation or a flight-start permission.
    bool _fleetDeploymentSendBusy = false;
    // Frozen from the actual transaction's reviewed policy, never a caller
    // flag. ODOMETRY remains conditional on the existing motion policy;
    // ESTIMATOR_STATUS is independent of the original six flight channels.
    bool _fleetStartupOdometryContinuityRequired = false;
    QVariantMap _fleetStartupQueueFlightObservation, _fleetStartupQueueCoordinateObservation, _fleetStartupQueueEstimatorObservation;
    QMap<QString, QVariantMap> _fleetStartupRetainedReceipts;
    QMap<QString, QMap<QString, qint64>> _fleetStartupRawLastAt;
    QMap<QString, qint64> _fleetStartupMaximumRawGap;
    QMap<QString, qint64> _fleetStartupHistoryEndedAt;
    QMap<QString, QString> _fleetStartupLastRiskHash;
    quint64 _fleetStartupJournalSequence = 0;
    quint64 _flightStateObservationReceiveDepth = 0;
    bool _flightStateObservationExhausted = false;
    bool            _receivingAttitudeQuaternion = false;
    CheckList       _checkListState                         = CheckListNotSetup;
    bool            _readyToFlyAvailable                    = false;
    bool            _readyToFly                             = false;
    bool            _allSensorsHealthy                      = true;

    SysStatusSensorInfo _sysStatusSensorInfo;

    QGCCameraManager* _cameraManager = nullptr;

    QString             _prearmError;
    QTimer              _prearmErrorTimer;
    static const int    _prearmErrorTimeoutMSecs = 35 * 1000;   ///< Take away prearm error after 35 seconds

    bool                _initialPlanRequestComplete = false;

    LinkManager*                    _linkManager                    = nullptr;
    QPointer<ParameterManager>      _parameterManager;
    FirmwarePluginManager*          _firmwarePluginManager          = nullptr;
    JoystickManager*                _joystickManager                = nullptr;
    ComponentInformationManager*    _componentInformationManager    = nullptr;
    bool                            _componentInformationCallbacksRetired = false;
    VehicleObjectAvoidance*         _objectAvoidance                = nullptr;
    Autotune*                       _autotune                       = nullptr;

    bool    _armed = false;         ///< true: vehicle is armed
    uint8_t _base_mode = 0;     ///< base_mode from HEARTBEAT
    uint32_t _custom_mode = 0;  ///< custom_mode from HEARTBEAT
    uint32_t _custom_mode_user_intention = 0;  ///< custom_mode_user_intention from CURRENT_MODE
    bool _has_custom_mode_user_intention = false;

    /// Used to store a message being sent by sendMessageMultiple
    typedef struct {
        mavlink_message_t   message;    ///< Message to send multiple times
        int                 retryCount; ///< Number of retries left
    } SendMessageMultipleInfo_t;

    QList<SendMessageMultipleInfo_t> _sendMessageMultipleList;    ///< List of messages being sent multiple times

    static const int _sendMessageMultipleRetries = 5;
    static const int _sendMessageMultipleIntraMessageDelay = 500;

    QTimer  _sendMultipleTimer;
    int     _nextSendMessageMultipleIndex = 0;

    QElapsedTimer                   _flightTimer;
    QTimer                          _flightTimeUpdater;
    TrajectoryPoints*               _trajectoryPoints = nullptr;
    QmlObjectListModel              _cameraTriggerPoints;
    //QMap<QString, ADSBVehicle*>     _trafficVehicleMap;

    // Toolbox references

    int                         _flowImageIndex = 0;

    bool _allLinksRemovedSent = false; ///< true: allLinkRemoved signal already sent one time

    // Expected identities for the three initial plan reads.  The old
    // no-argument slots remain as explicit skip paths used by the initial
    // connection state machine; manager completions must match these IDs.
    quint64 _initialMissionLoadOperationId = 0;
    quint64 _initialGeoFenceLoadOperationId = 0;
    quint64 _initialRallyPointLoadOperationId = 0;

    uint                _messagesReceived = 0;
    uint                _messagesSent = 0;
    uint                _messagesLost = 0;
    uint8_t             _messageSeq = 0;
    uint8_t             _compID = 0;
    bool                _heardFrom = false;

    float               _curGimbalRoll  = 0.0f;
    float               _curGimbalPitch = 0.0f;
    float               _curGimbalYaw  = 0.0f;
    bool                _haveGimbalData = false;
    bool                _isROIEnabled   = false;
    Joystick*           _activeJoystick = nullptr;

    bool _checkLatestStableFWDone = false;
    int _firmwareMajorVersion = versionNotSetValue;
    int _firmwareMinorVersion = versionNotSetValue;
    int _firmwarePatchVersion = versionNotSetValue;
    int _firmwareCustomMajorVersion = versionNotSetValue;
    int _firmwareCustomMinorVersion = versionNotSetValue;
    int _firmwareCustomPatchVersion = versionNotSetValue;
    FIRMWARE_VERSION_TYPE _firmwareVersionType = FIRMWARE_VERSION_TYPE_OFFICIAL;

    // Vendor and Product as reported from the first autopilot version message
    // during the initial connect. They may be zero eg ArduPilot SITL reports 0
    // by default.
    uint16_t       _firmwareBoardVendorId = 0;
    uint16_t       _firmwareBoardProductId = 0;


    QString _gitHash;
    quint64 _uid = 0;

    uint64_t    _mavlinkSentCount       = 0;
    uint64_t    _mavlinkReceivedCount   = 0;
    uint64_t    _mavlinkLossCount       = 0;
    float       _mavlinkLossPercent     = 0.0f;

    float       _loadProgress           = 0.0f;

    QMap<QString, QTime> _noisySpokenPrearmMap; ///< Used to prevent PreArm messages from being spoken too often

    // Orbit status values
    bool            _orbitActive = false;
    QGCMapCircle    _orbitMapCircle;
    QTimer          _orbitTelemetryTimer;
    static const int _orbitTelemetryTimeoutMsecs = 3000; // No telemetry for this amount and orbit will go inactive

    QMap<uint8_t, QSharedPointer<EventHandler>> _events; ///< One protocol handler for each component ID
    HealthAndArmingCheckReport _healthAndArmingCheckReport;

    MAVLinkStreamConfig _mavlinkStreamConfig;

    // Chunked status text support
    typedef struct {
        uint16_t    chunkId;
        uint8_t     severity;
        QStringList rgMessageChunks;
    } ChunkedStatusTextInfo_t;
    QMap<uint8_t /* compId */, ChunkedStatusTextInfo_t> _chunkedStatusTextInfoMap;
    QTimer _chunkedStatusTextTimer;

    /// Callback for waitForMavlinkMessage
    ///     @param resultHandleData     Opaque data passed in to waitForMavlinkMessage call
    ///     @param commandResult        Ack result for command send
    ///     @param noReponseFromVehicle true: The vehicle did not responsed to the COMMAND_LONG message
    typedef void (*WaitForMavlinkMessageResultHandler)(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message);

    /// Waits for the specified msecs for the message to be received. Calls timeoutHandler if not received.
    void _waitForMavlinkMessage     (WaitForMavlinkMessageResultHandler resultHandler, void* resultHandlerData, int messageId, int timeoutMsecs);
    void _waitForMavlinkMessageClear(void);

    int                                 _waitForMavlinkMessageId                = 0;
    bool                                _waitForMavlinkMessageTimeoutActive     = false;
    int                                 _waitForMavlinkMessageTimeoutMsecs      = 0;
    QElapsedTimer                       _waitForMavlinkMessageElapsed;
    WaitForMavlinkMessageResultHandler  _waitForMavlinkMessageResultHandler     = nullptr;
    void*                               _waitForMavlinkMessageResultHandlerData = nullptr;

    void _waitForMavlinkMessageMessageReceived(const mavlink_message_t& message, LinkInterface* sourceLink);

    // requestMessage handling
    struct MavCommandListEntry;
    typedef struct RequestMessageInfo {
        QPointer<Vehicle>           vehicle;
        int                         msgId;
        int                         compId;
        RequestMessageResultHandler resultHandler;
        void*                       resultHandlerData;
        bool                        commandAckReceived  = false; // We keep track of the ack/message being received since the order in which this will come in is random
        bool                        messageReceived     = false;    // Shared context lives through both terminal paths and any active callback.
        bool                        resultPublished     = false;
        bool                        ownerRequired       = false;
        QPointer<QObject>           callbackOwner;
        QString                     requestId;
        float                       requestedMessageParameter = 0;
        std::shared_ptr<MavCommandListEntry> source;
        mavlink_message_t           message{};
    } RequestMessageInfo_t;
    std::shared_ptr<RequestMessageInfo_t> _requestMessageContext;

    static void _requestMessageCmdResultHandler             (void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);
    static void _requestMessageWaitForMessageResultHandler  (void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message);

    typedef struct MavCommandListEntry {
        int                     targetCompId        = MAV_COMP_ID_AUTOPILOT1;
        bool                    useCommandInt       = false;
        MAV_CMD                 command;
        MAV_FRAME               frame;
        float                   rgParam1            = 0;
        float                   rgParam2            = 0;
        float                   rgParam3            = 0;
        float                   rgParam4            = 0;
        double                  rgParam5            = 0;
        double                  rgParam6            = 0;
        float                   rgParam7            = 0;
        bool                    showError           = true;
        MavCmdAckHandlerInfo_t  ackHandlerInfo;
        int                     maxTries            = _mavCommandMaxRetryCount;
        int                     tryCount            = 0;
        QElapsedTimer           elapsedTimer;
        int                     ackTimeoutMSecs     = _mavCommandAckTimeoutMSecs;
        bool                    fleetEvidenceReadOnly = false;
        QPointer<LinkInterface> fleetEvidenceLink;
        QString                 fleetEvidenceEpoch;
        QString                 fleetEvidenceRequestId;
        std::function<bool()>   fleetEvidenceAuthorizationFence;
        // One Vehicle-level lane for 510/511/512 and tagged native camera
        // actions across components. Legacy camera STOP->START stays legacy.
        // The old names remain native-only, never a wire operation token.
        QString                 messageIntervalRequestId;
        QString                 messageIntervalSourceEpoch;
        QString                 messageCommandTransportEpoch;
        int                     messageCommandVehicleId = 0, messageCommandDefaultComponentId = 0;
        int                     messageCommandFirmwareType = -1, messageCommandVehicleType = -1;
        QPointer<VehicleLinkManager> messageIntervalLinks;
        QPointer<MAVLinkProtocol> messageIntervalProtocol;
        QPointer<FirmwarePlugin> messageIntervalPlugin;
        QPointer<LinkInterface> messageIntervalPrimary;
        WeakLinkInterfacePtr    messageIntervalPrimaryOwner;
        quint64                 messageCommandSerial = 0;
        qint64                  messageCommandReservedAtMs = -1;
        qint64                  messageCommandDeadlineMs = -1;
        bool                    messageIntervalMutationReserved = false;
        bool                    messageCommandWriteStarted = false;
        bool                    messageCommandPrepared = false;
        bool                    messageCommandProgressSeen = false;
        bool                    fleetMessageIntervalQuery = false;
        QString                 fleetOptionalTelemetryConfigurationRunId;
        QString                 fleetCameraStreamCaptureRunId;
        QString                 fleetCameraStreamControlRunId;
    } MavCommandListEntry_t;

    struct FleetMessageIntervalQuery {
        QPointer<QObject> owner;
        MavCommandListEntry_t source;
        QPointer<ParameterManager> parameters;
        QString parameterEpoch;
        std::function<bool()> authorizationFence;
        int messageId = 0;
        qint64 originalDeadlineMs = -1;
        quint64 versionRevision = 0, mutationGeneration = 0;
        quint64 baselineReceiptSequence = 0, receiptSequence = 0, contentRevision = 0;
        QString ledgerEpoch;
        bool ledgerBound = false, active = true, requestIssued = false;
        bool ackObserved = false, reportObserved = false, authorizationBusy = false;
        bool configurationSealed = false;
        int ackResult = -1, intervalUs = 0;
        QString state = QStringLiteral("waiting-ack-and-report"), reason = QStringLiteral("");
    };
    QList<std::shared_ptr<FleetMessageIntervalQuery>> _fleetMessageIntervalQueries;
    std::shared_ptr<FleetLinkLatencyRun> _fleetLinkLatencyRun;
    std::shared_ptr<FleetCameraStreamRun> _fleetCameraStreamRun;
    std::shared_ptr<FleetCameraStreamControlRun> _fleetCameraStreamControlRun;
    bool _fleetCameraStreamControlOriginCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const;
    bool _fleetCameraStreamControlBasisMemoryCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis,
        quint64 expectedMutation, bool requireOriginalStatus) const;
    bool _fleetCameraStreamControlSourceCurrent(const std::shared_ptr<FleetCameraStreamControlRun>& run, bool requireQuiescent) const;
    bool _fleetCameraStreamControlAllowed(const std::shared_ptr<FleetCameraStreamControlRun>& run);
    bool _advanceFleetCameraStreamMutation(const std::shared_ptr<FleetCameraStreamControlRun>& ownRun);
    void _finishFleetCameraStreamControl(const std::shared_ptr<FleetCameraStreamControlRun>& run,
        const QString& state, const QString& reason, bool quarantine);
    void _completeFleetCameraStreamControlQuery(const std::shared_ptr<FleetCameraStreamControlRun>& run);
    void _observeFleetCameraStreamControl(const std::shared_ptr<FleetCameraStreamControlRun>& run,
        const QString& originalRequestId, bool originalIssued, quint64 originalBaseline,
        const mavlink_message_t& original, LinkInterface* sourceLink);
    static QVariantMap _fleetCameraStreamControlValue(const std::shared_ptr<FleetCameraStreamControlRun>& run,
        bool current, qint64 now, qint64 sampleNow, bool retired);
    bool _fleetCameraStreamRunSourceCurrent(const std::shared_ptr<FleetCameraStreamRun>& run, bool requireQuiescent) const;
    bool _fleetCameraStreamRunAllowed(const std::shared_ptr<FleetCameraStreamRun>& run);
    void _finishFleetCameraStreamCapture(const std::shared_ptr<FleetCameraStreamRun>& run,
        const QString& state, const QString& reason, bool quarantine);
    void _completeFleetCameraStreamQuery(const std::shared_ptr<FleetCameraStreamRun>& run);
    void _observeFleetCameraStreamCapture(const std::shared_ptr<FleetCameraStreamRun>& run, int originalStage,
        const QString& originalRequestId, bool originalIssued, quint64 originalBaseline,
        const QString& originalEpoch, const mavlink_message_t& original, LinkInterface* sourceLink);
    static QVariantMap _fleetCameraStreamCaptureValue(const std::shared_ptr<FleetCameraStreamRun>& run,
        bool current, qint64 now, qint64 sampleNow, bool retired);
    bool _fleetLinkLatencySourceCurrent(const std::shared_ptr<FleetLinkLatencyRun>& run, bool requireQuiescent) const;
    bool _fleetLinkLatencyAllowed(const std::shared_ptr<FleetLinkLatencyRun>& run);
    static QVariantMap _fleetLinkLatencyValue(const std::shared_ptr<FleetLinkLatencyRun>& run, bool current, bool retired);
    void _retireFleetLinkLatencyCapture(const QString& reason);
    void _observeFleetLinkLatencyCapture(const std::shared_ptr<FleetLinkLatencyRun>& run,
        const mavlink_message_t& original, LinkInterface* originalLink, qint64 receivedAtNs, bool originalWriteStarted);
    std::shared_ptr<FleetMessageIntervalQuery> _fleetMessageIntervalActiveQuery;
    quint64 _fleetMessageIntervalRawReceiveDepth = 0;
    bool _fleetMessageIntervalRawReceiveExhausted = false;
    std::shared_ptr<FleetOptionalTelemetryConfigurationRun> _fleetOptionalTelemetryConfigurationRun;
    bool _fleetOptionalTelemetryBasisMemoryCurrent(
        const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis, bool requireRows) const;
    bool _fleetOptionalTelemetryOriginCurrent(const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const;
    bool _fleetOptionalTelemetryRunSourceCurrent(
        const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run, bool requireQuiescent) const;
    bool _fleetOptionalTelemetryRunAllowed(const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run);
    void _finishFleetOptionalTelemetryConfiguration(const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run,
        const QString& state, const QString& reason, bool quarantine);
    QVariantMap _beginFleetMessageIntervalQuery(QObject* actualOwner, LinkInterface* expectedPrimary,
        int messageId, std::function<bool()> authorizationFence, const QString& configurationRunId);
    QPointer<InitialConnectStateMachine> _fleetIntervalQueryInitialConnect;
    QPointer<MissionManager> _fleetIntervalQueryMission;
    QPointer<GeoFenceManager> _fleetIntervalQueryFence;
    QPointer<RallyPointManager> _fleetIntervalQueryRally;
    bool _fleetMessageIntervalQuerySourceCurrent(const std::shared_ptr<FleetMessageIntervalQuery>& query,
        bool requireQuiescent) const;
    bool _fleetMessageIntervalQueryAllowed(const std::shared_ptr<FleetMessageIntervalQuery>& query);
    QVariantMap _fleetMessageIntervalQueryValue(const std::shared_ptr<FleetMessageIntervalQuery>& query,
        const QString& unavailableReason = QStringLiteral("fleet-interval-query-unavailable")) const;
    static QVariantMap _fleetMessageIntervalQueryRetiredValue(const std::shared_ptr<FleetMessageIntervalQuery>& query);
    void _finishFleetMessageIntervalQuery(const std::shared_ptr<FleetMessageIntervalQuery>& query,
        const QString& state, const QString& reason, bool quarantine);
    void _observeFleetMessageIntervalQuery(const std::shared_ptr<FleetMessageIntervalQuery>& query,
        const mavlink_message_t& original, LinkInterface* sourceLink);

    QList<MavCommandListEntry_t>    _mavCommandList;
    // COMMAND_ACK does not echo a ground-station operation ID. Keep a short
    // per-command quarantine after a no-response timeout so a delayed
    // ACK cannot be consumed by an immediately re-issued command. This is a
    // conservative compatibility guard, not a protocol-level token.
    QMap<quint64, qint64>           _mavCommandQuarantine;
    QElapsedTimer                   _mavCommandQuarantineClock;
    QString                         _messageCommandLaneId;
    QString                         _messageCommandTransportEpoch;
    QString                         _messageCommandDispatchingId;
    quint64                         _messageCommandLaneSerial = 0;
    qint64                          _messageCommandLaneDeadlineMs = -1, _messageCommandQuarantineUntilMs = -1;
    bool                            _messageCommandLaneRetired = false, _messageCommandLaneExhausted = false;
    QTimer                          _mavCommandResponseCheckTimer;
    static const int                _mavCommandMaxRetryCount                = 3;
    static const int                _mavCommandResponseCheckTimeoutMSecs    = 500;
    static const int                _mavCommandAckTimeoutMSecs              = 3000;
    static const int                _mavCommandAckTimeoutMSecsHighLatency   = 120000;

    void _sendMavCommandWorker  (
            bool commandInt, bool showError, 
            const MavCmdAckHandlerInfo_t* ackHandlerInfo,   ///> nullptr to signale no handlers
            int compId, MAV_CMD command, MAV_FRAME frame, 
            float param1, float param2, float param3, float param4, double param5, double param6, float param7);
    void _sendMavCommandFromList(int index, bool* fleetReadIssued = nullptr);
    int  _findMavCommandListEntryIndex(int targetCompId, MAV_CMD command);
    quint64 _mavCommandQuarantineKey(int targetCompId, MAV_CMD command) const;
    void _purgeMavCommandQuarantine(qint64 now = -1);
    bool _isMavCommandQuarantined(int targetCompId, MAV_CMD command);
    void _quarantineMavCommand(int targetCompId, MAV_CMD command, int ackTimeoutMSecs);
    bool _messageIntervalSourceCurrent(const MavCommandListEntry_t& entry) const;
    bool _messageCommandSourceMemoryCurrent(const MavCommandListEntry_t& entry) const;
    bool _sendMessageIntervalCommand(const MavCommandListEntry_t& entry, mavlink_message_t message);
    bool _reserveMessageCommand(MavCommandListEntry_t* entry);
    void _releaseMessageCommand(const QString& requestId);
    int _messageCommandIndex(const QString& requestId) const;
    void _finishMessageCommand(const QString& requestId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failure);
    void _showMessageCommandFailure(const MavCommandListEntry_t& entry, const mavlink_command_ack_t& ack,
        MavCmdResultFailureCode_t failure);
    void _sendMessageCommandWorker(bool commandInt, bool showError, const MavCmdAckHandlerInfo_t* handler,
        int componentId, MAV_CMD command, MAV_FRAME frame, float p1, float p2, float p3, float p4,
        double p5, double p6, float p7);
    void _sendMessageCommandFromList(const QString& requestId, bool* fleetReadIssued);
    void _finishRequestMessage(std::shared_ptr<RequestMessageInfo_t> context, MAV_RESULT result,
        RequestMessageResultHandlerFailureCode_t failure, const mavlink_message_t& message);
    void _checkMessageCommandDeadline();
    void _handleMessageCommandAck(const mavlink_message_t& original, LinkInterface* sourceLink);
    bool _sendMavCommandShouldRetry(MAV_CMD command);
    bool _commandCanBeDuplicated(MAV_CMD command);
    bool _fleetEvidenceReadAllowed(LinkInterface* expectedLink, bool ownVersionCommand = false,
                                  QString* reason = nullptr) const;
    bool _sendFleetEvidenceReadMessage(LinkInterface* expectedLink, mavlink_message_t message,
                                      bool ownVersionCommand, std::function<bool()> authorizationFence,
                                      bool* requestIssued = nullptr);
    bool _fleetMissionReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
                                  const QString& readScopeId, QString* reason = nullptr) const;
    bool _fleetMissionDiagnosticReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
                                          const QString& readScopeId, QString* reason = nullptr) const;
    bool _fleetMissionArmedGroundReviewReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
        const QString& readScopeId, bool expectedArmed, QString* reason = nullptr) const;
    bool _sendFleetMissionReadMessage(PlanManager* manager, LinkInterface* expectedLink,
                                      const QString& readScopeId, mavlink_message_t message,
                                      std::function<bool()> authorizationFence, bool* requestIssued);
    bool _sendFleetDeploymentMessage(PlanManager* actualManager, LinkInterface* expectedPrimary,
        mavlink_message_t message, std::function<bool()> authorizationFence, bool* requestIssued);
    void _invalidateStartupCoordinates();
    quint64 _prepareStartupCoordinateObservation(const mavlink_message_t& original, LinkInterface* link,
        QString* channel, QVariantMap* value);
    void _completeStartupCoordinateObservation(const QString& channel, quint64 token, const QString& epoch,
        const QVariantMap& value, const mavlink_message_t& original, LinkInterface* link, qint64 receivedAtMs);
    void _invalidateStartupOdometry(const QString& source);
    quint64 _prepareStartupOdometryObservation(const mavlink_message_t& original, LinkInterface* link, QVariantMap* value);
    void _completeStartupOdometryObservation(quint64 token, const QString& epoch, const QVariantMap& value,
        const mavlink_message_t& original, LinkInterface* link, qint64 receivedAtMs);
    bool _fleetStartupTransportCurrent(bool requireQuiescent) const;
    bool _fleetStartupRawIngressSame(const FleetStartupRawIngress& ingress) const;
    void _discardFleetStartupRawIngress(const FleetStartupRawIngress& ingress);
    bool _handleFleetStartupRawMessage(const mavlink_message_t& original, LinkInterface* link,
        const FleetStartupRawIngress& ingress);
    void _handleFleetStartupEstimatorRawMessage(const mavlink_message_t& original, LinkInterface* link,
        qint64 receivedAtMs, const FleetStartupEstimatorIngress& ingress, quint64 healthToken, const QString& healthEpoch);
    void _closeFleetStartupStep(const QString& reason);
    void _snapshotFleetStartupStep();
    static void _fleetEvidenceCommandResult(void*, int, const mavlink_command_ack_t&, MavCmdResultFailureCode_t);
    void _quarantineFleetEvidenceCommand(const MavCommandListEntry_t& entry);

    QMap<uint8_t /* batteryId */, uint8_t /* MAV_BATTERY_CHARGE_STATE_OK */> _lowestBatteryChargeStateAnnouncedMap;

    float _altitudeTuningOffset = qQNaN(); // altitude offset, so the plotted value is around 0

    // these flags are used to determine if the speed change action from fly view should be shown
    bool _multirotor_speed_limits_available = false;
    bool _fixed_wing_airspeed_limits_available = false;

    // FactGroup facts

    Fact _rollFact;
    Fact _pitchFact;
    Fact _headingFact;
    Fact _rollRateFact;
    Fact _pitchRateFact;
    Fact _yawRateFact;
    Fact _groundSpeedFact;
    Fact _airSpeedFact;
    Fact _airSpeedSetpointFact;
    Fact _climbRateFact;
    Fact _altitudeRelativeFact;
    Fact _altitudeAMSLFact;
    Fact _altitudeAboveTerrFact;
    Fact _altitudeTuningFact;
    Fact _altitudeTuningSetpointFact;
    Fact _xTrackErrorFact;
    Fact _rangeFinderDistFact;
    Fact _flightDistanceFact;
    Fact _flightTimeFact;
    Fact _distanceToHomeFact;
    Fact _timeToHomeFact;
    Fact _missionItemIndexFact;
    Fact _headingToNextWPFact;
    Fact _headingToHomeFact;
    Fact _distanceToGCSFact;
    Fact _hobbsFact;
    Fact _throttlePctFact;
    Fact _imuTempFact;

    VehicleGPSFactGroup             _gpsFactGroup;
    VehicleGPS2FactGroup            _gps2FactGroup;
    VehicleWindFactGroup            _windFactGroup;
    VehicleVibrationFactGroup       _vibrationFactGroup;
    VehicleTemperatureFactGroup     _temperatureFactGroup;
    VehicleClockFactGroup           _clockFactGroup;
    VehicleSetpointFactGroup        _setpointFactGroup;
    VehicleDistanceSensorFactGroup  _distanceSensorFactGroup;
    VehicleLocalPositionFactGroup   _localPositionFactGroup;
    VehicleLocalPositionSetpointFactGroup _localPositionSetpointFactGroup;
    VehicleEscStatusFactGroup       _escStatusFactGroup;
    VehicleFtcStatusFactGroup       _ftcStatusFactGroup;
    VehicleEstimatorStatusFactGroup _estimatorStatusFactGroup;
    VehicleHygrometerFactGroup      _hygrometerFactGroup;
    VehicleGeneratorFactGroup       _generatorFactGroup;
    VehicleEFIFactGroup             _efiFactGroup;
    TerrainFactGroup                _terrainFactGroup;
    QmlObjectListModel              _batteryFactGroupListModel;

    TerrainProtocolHandler* _terrainProtocolHandler = nullptr;

    MissionManager*                 _missionManager             = nullptr;
    GeoFenceManager*                _geoFenceManager            = nullptr;
    RallyPointManager*              _rallyPointManager          = nullptr;
    VehicleLinkManager*             _vehicleLinkManager         = nullptr;
    FTPManager*                     _ftpManager                 = nullptr;
    ImageProtocolManager*           _imageProtocolManager       = nullptr;
    InitialConnectStateMachine*     _initialConnectStateMachine = nullptr;
    Actuators*                      _actuators                  = nullptr;
    RemoteIDManager*                _remoteIDManager            = nullptr;
    StandardModes*                  _standardModes              = nullptr;

    static const char* _rollFactName;
    static const char* _pitchFactName;
    static const char* _headingFactName;
    static const char* _rollRateFactName;
    static const char* _pitchRateFactName;
    static const char* _yawRateFactName;
    static const char* _groundSpeedFactName;
    static const char* _airSpeedFactName;
    static const char* _airSpeedSetpointFactName;
    static const char* _climbRateFactName;
    static const char* _altitudeRelativeFactName;
    static const char* _altitudeAMSLFactName;
    static const char* _altitudeAboveTerrFactName;
    static const char* _altitudeTuningFactName;
    static const char* _altitudeTuningSetpointFactName;
    static const char* _xTrackErrorFactName;
    static const char* _rangeFinderDistFactName;
    static const char* _flightDistanceFactName;
    static const char* _flightTimeFactName;
    static const char* _distanceToHomeFactName;
    static const char* _timeToHomeFactName;
    static const char* _missionItemIndexFactName;
    static const char* _headingToNextWPFactName;
    static const char* _headingToHomeFactName;
    static const char* _distanceToGCSFactName;
    static const char* _hobbsFactName;
    static const char* _throttlePctFactName;
    static const char* _imuTempFactName;

    static const char* _gpsFactGroupName;
    static const char* _gps2FactGroupName;
    static const char* _windFactGroupName;
    static const char* _vibrationFactGroupName;
    static const char* _temperatureFactGroupName;
    static const char* _clockFactGroupName;
    static const char* _setpointFactGroupName;
    static const char* _distanceSensorFactGroupName;
    static const char* _localPositionFactGroupName;
    static const char* _localPositionSetpointFactGroupName;
    static const char* _escStatusFactGroupName;
    static const char* _ftcStatusFactGroupName;
    static const char* _estimatorStatusFactGroupName;
    static const char* _hygrometerFactGroupName;
    static const char* _generatorFactGroupName;
    static const char* _efiFactGroupName;
    static const char* _terrainFactGroupName;

    static const int _vehicleUIUpdateRateMSecs      = 100;

    // Settings keys
    static const char* _settingsGroup;
    static const char* _joystickEnabledSettingsKey;

    // Terrain query members, used to get terrain altitude for doSetHome()
    TerrainAtCoordinateQuery*   _currentDoSetHomeTerrainAtCoordinateQuery = nullptr;
    QGeoCoordinate              _doSetHomeCoordinate;

    // Terrain query members, used to get altitude above terrain Fact
    QElapsedTimer               _altitudeAboveTerrQueryTimer;
    TerrainAtCoordinateQuery*   _altitudeAboveTerrTerrainAtCoordinateQuery = nullptr;
    // We use this to limit above terrain altitude queries based on distance and altitude change
    QGeoCoordinate              _altitudeAboveTerrLastCoord;
    float                       _altitudeAboveTerrLastRelAlt = qQNaN();
};

Q_DECLARE_METATYPE(Vehicle::MavCmdResultFailureCode_t)

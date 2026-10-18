/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "PX4AutoPilotPlugin.h"
#include "PX4SimpleFlightModesController.h"
#include "AirframeComponentController.h"
#include "SensorsComponentController.h"
#include "PowerComponentController.h"
#include "RadioComponentController.h"
#include "QGCCameraManager.h"
#include "QGCFileDownload.h"
#include "SettingsManager.h"
#include "PlanViewSettings.h"
#include "HealthAndArmingCheckReport.h"
#include "VehicleGPSFactGroup.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QDebug>
#include <QThread>

#include "px4_custom_mode.h"

namespace {

constexpr int kTakeoffHealthCheckTimeoutMs = 3000;

bool isTakeoffHealthRefreshStage(PX4TakeoffHealthCheckStage stage)
{
    return stage == PX4TakeoffHealthCheckStage::RefreshBeforeTakeoff
        || stage == PX4TakeoffHealthCheckStage::RefreshBeforeArm
        || stage == PX4TakeoffHealthCheckStage::RefreshAfterTakeoffRejection
        || stage == PX4TakeoffHealthCheckStage::RefreshAfterArmRejection;
}

bool isRejectedCommandRefreshStage(PX4TakeoffHealthCheckStage stage)
{
    return stage == PX4TakeoffHealthCheckStage::RefreshAfterTakeoffRejection
        || stage == PX4TakeoffHealthCheckStage::RefreshAfterArmRejection;
}

// Each handler belongs to one actual queued request, not to the global 511
// signal. Vehicle owns the context until that request terminates or closes.
class EscIntervalRequest final : public QObject
{
public:
    EscIntervalRequest(const QPointer<Vehicle>& actualVehicle,
                       const QPointer<PX4FirmwarePluginInstanceData>& actualInstance,
                       const QPointer<VehicleLinkManager>& actualLinks,
                       const QPointer<LinkInterface>& actualPrimary,
                       const WeakLinkInterfacePtr& actualOwner, const QString& actualEpoch, int actualMessageId)
        : QObject()
        , vehicle(actualVehicle), instance(actualInstance), links(actualLinks)
        , primary(actualPrimary), primaryOwner(actualOwner), sourceEpoch(actualEpoch), messageId(actualMessageId)
    {
    }

    const QPointer<Vehicle> vehicle;
    const QPointer<PX4FirmwarePluginInstanceData> instance;
    const QPointer<VehicleLinkManager> links;
    const QPointer<LinkInterface> primary;
    const WeakLinkInterfacePtr primaryOwner;
    const QString sourceEpoch;
    const int messageId;
    bool finished = false;
};

bool escIntervalContextCurrent(const QPointer<Vehicle>& vehicle,
                               const QPointer<PX4FirmwarePluginInstanceData>& instance,
                               const QPointer<VehicleLinkManager>& links,
                               const QPointer<LinkInterface>& primary,
                               const WeakLinkInterfacePtr& owner, const QString& sourceEpoch)
{
    if (!vehicle || !instance || !links || !primary || owner.expired()
        || sourceEpoch.isEmpty() || vehicle->autopilotVersionObservationSourceEpochMemory() != sourceEpoch
        || QThread::currentThread() != vehicle->thread() || instance->thread() != vehicle->thread()
        || links->thread() != vehicle->thread() || vehicle->firmwarePluginInstanceData() != instance.data()
        || vehicle->vehicleLinkManager() != links.data() || instance->parent() != vehicle.data()) return false;
    const WeakLinkInterfacePtr current = links->primaryLink();
    const SharedLinkInterfacePtr retained = current.lock();
    return retained && retained.get() == primary.data()
        && !current.owner_before(owner) && !owner.owner_before(current);
}

void requestEscInterval(QPointer<Vehicle> vehicle, QPointer<PX4FirmwarePluginInstanceData> instance,
                        QPointer<VehicleLinkManager> links, QPointer<LinkInterface> primary,
                        WeakLinkInterfacePtr owner, QString sourceEpoch, int messageId);

void escIntervalResult(void* resultHandlerData, int component, const mavlink_command_ack_t& ack,
                       Vehicle::MavCmdResultFailureCode_t failureCode)
{
    const QPointer<EscIntervalRequest> request(static_cast<EscIntervalRequest*>(resultHandlerData));
    if (!request || request->finished) return;
    request->finished = true;
    // Freeze owned values before deleting the one-shot context. Destruction
    // notifications may remove the Vehicle, replace its instance or source.
    const QPointer<Vehicle> vehicle = request->vehicle;
    const QPointer<PX4FirmwarePluginInstanceData> instance = request->instance;
    const QPointer<VehicleLinkManager> links = request->links;
    const QPointer<LinkInterface> primary = request->primary;
    const WeakLinkInterfacePtr owner = request->primaryOwner;
    const QString sourceEpoch = request->sourceEpoch;
    const bool own = vehicle && instance && request->parent() == vehicle.data()
        && instance->escTelemetryIntervalRequest.data() == request.data();
    const bool advance = own && request->messageId == MAVLINK_MSG_ID_ESC_STATUS
        && failureCode == Vehicle::MavCmdResultCommandResultOnly
        && component == MAV_COMP_ID_AUTOPILOT1 && ack.command == MAV_CMD_SET_MESSAGE_INTERVAL
        && ack.result == MAV_RESULT_ACCEPTED
        && escIntervalContextCurrent(vehicle, instance, links, primary, owner, sourceEpoch);
    if (own) instance->escTelemetryIntervalRequest.clear();
    delete request.data();
    if (!advance || !escIntervalContextCurrent(vehicle, instance, links, primary, owner, sourceEpoch)
        || !instance->escTelemetryRequestStarted || instance->escTelemetryIntervalRequest) return;
    // An accepted ACK only advances our initialization sequence; it is not
    // wire-causal attribution or proof that either stream rate was applied.
    requestEscInterval(vehicle, instance, links, primary, owner, sourceEpoch, MAVLINK_MSG_ID_ESC_INFO);
}

void requestEscInterval(QPointer<Vehicle> vehicle, QPointer<PX4FirmwarePluginInstanceData> instance,
                        QPointer<VehicleLinkManager> links, QPointer<LinkInterface> primary,
                        WeakLinkInterfacePtr owner, QString sourceEpoch, int messageId)
{
    if ((messageId != MAVLINK_MSG_ID_ESC_STATUS && messageId != MAVLINK_MSG_ID_ESC_INFO)
        || !escIntervalContextCurrent(vehicle, instance, links, primary, owner, sourceEpoch)
        || !instance->escTelemetryRequestStarted || instance->escTelemetryIntervalRequest) return;
    const QPointer<EscIntervalRequest> request(new EscIntervalRequest(vehicle, instance,
        links, primary, owner, sourceEpoch, messageId));
    // Parent adoption can invoke ChildAdded/destruction callbacks. Construct
    // all fields without a parent first, then check the frozen context again.
    request->setParent(vehicle.data());
    if (!request || !escIntervalContextCurrent(vehicle, instance, links, primary, owner, sourceEpoch)
        || request->parent() != vehicle.data() || instance->escTelemetryIntervalRequest) {
        if (request) delete request.data();
        return;
    }
    instance->escTelemetryIntervalRequest = request.data();
    Vehicle::MavCmdAckHandlerInfo_t handler = {};
    handler.resultHandler = &escIntervalResult;
    handler.resultHandlerData = request.data();
    vehicle->sendMavCommandWithHandler(&handler, MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL,
        float(messageId), messageId == MAVLINK_MSG_ID_ESC_STATUS ? 200000.0f : 1000000.0f);
    // No access after this call: a local failure may synchronously finish and
    // delete the request, its owner, or both. IN_PROGRESS never advances it.
}

}

PX4FirmwarePluginInstanceData::PX4FirmwarePluginInstanceData(QObject* parent)
    : QObject(parent)
    , versionNotified(false)
    , takeoffHealthCheckTimer(this)
{
    takeoffHealthCheckTimer.setSingleShot(true);
}

PX4FirmwarePlugin::PX4FirmwarePlugin()
    : _manualFlightMode     (tr("Manual"))
    , _acroFlightMode       (tr("Acro"))
    , _stabilizedFlightMode (tr("Stabilized"))
    , _rattitudeFlightMode  (tr("Rattitude"))
    , _altCtlFlightMode     (tr("Altitude"))
    , _posCtlFlightMode     (tr("Position"))
    , _offboardFlightMode   (tr("Offboard"))
    , _readyFlightMode      (tr("Ready"))
    , _takeoffFlightMode    (tr("Takeoff"))
    , _holdFlightMode       (tr("Hold"))
    , _missionFlightMode    (tr("Mission"))
    , _rtlFlightMode        (tr("Return"))
    , _landingFlightMode    (tr("Land"))
    , _preclandFlightMode   (tr("Precision Land"))
    , _rtgsFlightMode       (tr("Return to Groundstation"))
    , _followMeFlightMode   (tr("Follow Me"))
    , _simpleFlightMode     (tr("Simple"))
    , _orbitFlightMode      (tr("Orbit"))
{
    qmlRegisterType<PX4SimpleFlightModesController>     ("QGroundControl.Controllers", 1, 0, "PX4SimpleFlightModesController");
    qmlRegisterType<AirframeComponentController>        ("QGroundControl.Controllers", 1, 0, "AirframeComponentController");
    qmlRegisterType<SensorsComponentController>         ("QGroundControl.Controllers", 1, 0, "SensorsComponentController");
    qmlRegisterType<PowerComponentController>           ("QGroundControl.Controllers", 1, 0, "PowerComponentController");

    struct Modes2Name {
        uint8_t     main_mode;
        uint8_t     sub_mode;
        bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
        bool        fixedWing;  /// fixed wing compatible
        bool        multiRotor;  /// multi rotor compatible
    };

    static const struct Modes2Name rgModes2Name[] = {
        //main_mode                         sub_mode                                canBeSet  FW      MC
        { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      true,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_SIMPLE,      0,                                      false,  false,  true },
        { PX4_CUSTOM_MAIN_MODE_POSCTL,      PX4_CUSTOM_SUB_MODE_POSCTL_POSCTL,      true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_POSCTL,      PX4_CUSTOM_SUB_MODE_POSCTL_ORBIT,       false,  false,   false },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,        true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,       true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,           true,   true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, true,   false,  true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,          false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_PRECLAND,      true,  false,  true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,         false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTGS,          false,  true,   true },
        { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,       false,  true,   true },
    };

    // Must be in same order as above structure
    const QString* rgModeNames[] = {
        &_manualFlightMode,
        &_stabilizedFlightMode,
        &_acroFlightMode,
        &_rattitudeFlightMode,
        &_altCtlFlightMode,
        &_offboardFlightMode,
        &_simpleFlightMode,
        &_posCtlFlightMode,
        &_orbitFlightMode,
        &_holdFlightMode,
        &_missionFlightMode,
        &_rtlFlightMode,
        &_followMeFlightMode,
        &_landingFlightMode,
        &_preclandFlightMode,
        &_readyFlightMode,
        &_rtgsFlightMode,
        &_takeoffFlightMode,
    };

    // Convert static information to dynamic list. This allows for plugin override class to manipulate list.
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        FlightModeInfo_t info;

        info.main_mode =    pModes2Name->main_mode;
        info.sub_mode =     pModes2Name->sub_mode;
        info.name =         rgModeNames[i];
        info.canBeSet =     pModes2Name->canBeSet;
        info.fixedWing =    pModes2Name->fixedWing;
        info.multiRotor =   pModes2Name->multiRotor;

        _flightModeInfoList.append(info);
    }
}

PX4FirmwarePlugin::~PX4FirmwarePlugin()
{
}

AutoPilotPlugin* PX4FirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new PX4AutoPilotPlugin(vehicle, vehicle);
}

QList<VehicleComponent*> PX4FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList PX4FirmwarePlugin::flightModes(Vehicle* vehicle)
{
    QStringList flightModes;

    foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
        if (info.canBeSet) {
            bool fw = (vehicle->fixedWing() && info.fixedWing);
            bool mc = (vehicle->multiRotor() && info.multiRotor);

            // show all modes for generic, vtol, etc
            bool other = !vehicle->fixedWing() && !vehicle->multiRotor();

            if (fw || mc || other) {
                flightModes += *info.name;
            }
        }
    }

    return flightModes;
}

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        union px4_custom_mode px4_mode;
        px4_mode.data = custom_mode;

        bool found = false;
        foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
            if (info.main_mode == px4_mode.main_mode && info.sub_mode == px4_mode.sub_mode) {
                flightMode = *info.name;
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning() << "Unknown flight mode" << custom_mode;
            return tr("Unknown %1:%2").arg(base_mode).arg(custom_mode);
        }
    }

    return flightMode;
}

bool PX4FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    *base_mode = 0;
    *custom_mode = 0;

    bool found = false;
    foreach (const FlightModeInfo_t& info, _flightModeInfoList) {
        if (flightMode.compare(info.name, Qt::CaseInsensitive) == 0) {
            union px4_custom_mode px4_mode;

            px4_mode.data = 0;
            px4_mode.main_mode = info.main_mode;
            px4_mode.sub_mode = info.sub_mode;

            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = px4_mode.data;

            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "Unknown flight Mode" << flightMode;
    }

    return found;
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    int available = SetFlightModeCapability | PauseVehicleCapability | GuidedModeCapability;
    //-- This is arbitrary until I find how to really tell if ROI is avaiable
    if (vehicle->multiRotor()) {
        available |= ROIModeCapability;
    }
    if (vehicle->multiRotor() || vehicle->vtol()) {
        available |= TakeoffVehicleCapability | OrbitModeCapability;
    }
    return (capabilities & available) == capabilities;
}

void PX4FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    const QPointer<Vehicle> escVehicle(vehicle);
    const QPointer<VehicleLinkManager> escLinks(vehicle->vehicleLinkManager());
    const QPointer<PX4FirmwarePlugin> plugin(this);
    const QPointer<PX4FirmwarePluginInstanceData> escInstance(new PX4FirmwarePluginInstanceData);
    const auto initializationCurrent = [escVehicle, escInstance, plugin]() {
        return escVehicle && escInstance && plugin
            && escVehicle->firmwarePluginInstanceData() == escInstance.data();
    };
    vehicle->setFirmwarePluginInstanceData(escInstance.data());
    if (!initializationCurrent()) return;

    connect(vehicle, &Vehicle::mavCommandResult,
            this, &PX4FirmwarePlugin::_mavCommandResult,
            Qt::UniqueConnection);
    if (!initializationCurrent()) return;
    connect(vehicle->healthAndArmingCheckReport(), &HealthAndArmingCheckReport::updated,
            vehicle, [this, vehicle]() { _takeoffHealthReportUpdated(vehicle); });
    if (!initializationCurrent()) return;
    connect(&escInstance->takeoffHealthCheckTimer, &QTimer::timeout,
            vehicle, [this, vehicle]() { _takeoffHealthCheckTimedOut(vehicle); });
    if (!initializationCurrent()) return;

    // ESC messages are not part of every default PX4 stream. Request both the
    // fast measurements and the slow health data after the initial handshake.
    const auto requestEscTelemetry = [escVehicle, escInstance, escLinks]() {
        if (!escVehicle || !escInstance || !escLinks
            || escVehicle->firmwarePluginInstanceData() != escInstance.data()
            || escInstance->escTelemetryRequestStarted) return;
        escInstance->escTelemetryRequestStarted = true;
        const QString sourceEpoch = escVehicle->autopilotVersionObservationSourceEpochMemory();
        const WeakLinkInterfacePtr owner = escLinks->primaryLink();
        const SharedLinkInterfacePtr primary = owner.lock();
        if (!primary) return;
        requestEscInterval(escVehicle, escInstance, escLinks, QPointer<LinkInterface>(primary.get()),
                           owner, sourceEpoch, MAVLINK_MSG_ID_ESC_STATUS);
    };

    if (vehicle->isInitialConnectComplete()) {
        requestEscTelemetry();
    } else {
        QObject::connect(vehicle, &Vehicle::initialConnectComplete,
                         vehicle, requestEscTelemetry);
    }
}

bool PX4FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // PX4 stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

FactMetaData* PX4FirmwarePlugin::_getMetaDataForFact(QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType)
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        return px4MetaData->getMetaDataForFact(name, vehicleType, type);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::getMetaDataForFact not PX4ParameterMetaData";
    }

    return nullptr;
}

void PX4FirmwarePlugin::_getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion)
{
    return PX4ParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion);
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass)
{
    QList<MAV_CMD> supportedCommands = {
        MAV_CMD_NAV_WAYPOINT,
        MAV_CMD_NAV_LOITER_UNLIM, MAV_CMD_NAV_LOITER_TIME,
        MAV_CMD_NAV_RETURN_TO_LAUNCH,
        MAV_CMD_DO_JUMP,
        MAV_CMD_DO_DIGICAM_CONTROL,
        MAV_CMD_DO_SET_CAM_TRIGG_DIST,
        MAV_CMD_DO_SET_SERVO,
        MAV_CMD_DO_SET_ACTUATOR,
        MAV_CMD_DO_CHANGE_SPEED,
        MAV_CMD_DO_SET_HOME,
        MAV_CMD_DO_LAND_START,
        MAV_CMD_DO_SET_ROI_LOCATION, MAV_CMD_DO_SET_ROI_WPNEXT_OFFSET, MAV_CMD_DO_SET_ROI_NONE,
        MAV_CMD_DO_MOUNT_CONFIGURE,
        MAV_CMD_DO_MOUNT_CONTROL,
        MAV_CMD_SET_CAMERA_MODE,
        MAV_CMD_IMAGE_START_CAPTURE, MAV_CMD_IMAGE_STOP_CAPTURE, MAV_CMD_VIDEO_START_CAPTURE, MAV_CMD_VIDEO_STOP_CAPTURE,
        MAV_CMD_NAV_DELAY,
        MAV_CMD_CONDITION_YAW,
        MAV_CMD_NAV_LOITER_TO_ALT,
        MAV_CMD_DO_GRIPPER
    };

    QList<MAV_CMD> vtolCommands = {
        MAV_CMD_DO_VTOL_TRANSITION, MAV_CMD_NAV_VTOL_TAKEOFF, MAV_CMD_NAV_VTOL_LAND,
    };

    QList<MAV_CMD> flightCommands = {
        MAV_CMD_NAV_LAND, MAV_CMD_NAV_TAKEOFF,
    };

    if (vehicleClass == QGCMAVLink::VehicleClassGeneric) {
        supportedCommands   += vtolCommands;
        supportedCommands   += flightCommands;
    }
    if (vehicleClass == QGCMAVLink::VehicleClassVTOL) {
        supportedCommands += vtolCommands;
        supportedCommands += flightCommands;
    } else if (vehicleClass == QGCMAVLink::VehicleClassFixedWing || vehicleClass == QGCMAVLink::VehicleClassMultiRotor) {
        supportedCommands += flightCommands;
    }

    if (qgcApp()->toolbox()->settingsManager()->planViewSettings()->useConditionGate()->rawValue().toBool()) {
        supportedCommands.append(MAV_CMD_CONDITION_GATE);
    }

    return supportedCommands;
}

QString PX4FirmwarePlugin::missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const
{
    switch (vehicleClass) {
    case QGCMAVLink::VehicleClassGeneric:
        return QStringLiteral(":/json/PX4-MavCmdInfoCommon.json");
    case QGCMAVLink::VehicleClassFixedWing:
        return QStringLiteral(":/json/PX4-MavCmdInfoFixedWing.json");
    case QGCMAVLink::VehicleClassMultiRotor:
        return QStringLiteral(":/json/PX4-MavCmdInfoMultiRotor.json");
    case QGCMAVLink::VehicleClassVTOL:
        return QStringLiteral(":/json/PX4-MavCmdInfoVTOL.json");
    case QGCMAVLink::VehicleClassSub:
        return QStringLiteral(":/json/PX4-MavCmdInfoSub.json");
    case QGCMAVLink::VehicleClassRoverBoat:
        return QStringLiteral(":/json/PX4-MavCmdInfoRover.json");
    default:
        qWarning() << "PX4FirmwarePlugin::missionCommandOverrides called with bad VehicleClass_t:" << vehicleClass;
        return QString();
    }
}

QObject* PX4FirmwarePlugin::_loadParameterMetaData(const QString& metaDataFile)
{
    PX4ParameterMetaData* metaData = new PX4ParameterMetaData;
    if (!metaDataFile.isEmpty()) {
        metaData->loadParameterFactMetaDataFile(metaDataFile);
    }
    return metaData;
}

void PX4FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_DO_REPOSITION,
                            true,   // show error if failed
                            -1.0f,
                            MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                            0.0f,
                            NAN,
                            NAN,
                            NAN,
                            NAN);
}

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle, bool smartRTL)
{
    Q_UNUSED(smartRTL);
    _setFlightModeAndValidate(vehicle, _rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, _landingFlightMode);
}

void PX4FirmwarePlugin::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(component);

    auto* vehicle = qobject_cast<Vehicle*>(sender());
    if (!vehicle) {
        qWarning() << "Dynamic cast failed!";
        return;
    }

    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData) {
        return;
    }

    if (command == MAV_CMD_RUN_PREARM_CHECKS
        && isTakeoffHealthRefreshStage(instanceData->takeoffHealthCheckStage)) {
        if (noReponseFromVehicle || result != MAV_RESULT_ACCEPTED) {
            instanceData->takeoffHealthCheckTimer.stop();
            instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;
            _showTakeoffHealthFailure(vehicle,
                                      tr("No current preflight check result was received. Check the vehicle link and try again."));
            return;
        }

        instanceData->prearmCheckAcknowledged = true;
        if (vehicle->healthAndArmingCheckReport()->updateSequence()
            <= instanceData->takeoffHealthReportSequence) {
            instanceData->takeoffHealthCheckTimer.start(kTakeoffHealthCheckTimeoutMs);
        }
        _tryCompleteTakeoffHealthCheck(vehicle);
        return;
    }

    if (command == MAV_CMD_NAV_TAKEOFF
        && instanceData->takeoffHealthCheckStage == PX4TakeoffHealthCheckStage::WaitingForTakeoffAcceptance) {
        if (result == MAV_RESULT_ACCEPTED && !noReponseFromVehicle && !vehicle->armed()) {
            _requestTakeoffHealthCheck(vehicle, PX4TakeoffHealthCheckStage::RefreshBeforeArm);
        } else if (result == MAV_RESULT_ACCEPTED && !noReponseFromVehicle) {
            instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;
        } else {
            _requestTakeoffHealthCheck(vehicle, PX4TakeoffHealthCheckStage::RefreshAfterTakeoffRejection);
        }
        return;
    }

    if (command == MAV_CMD_COMPONENT_ARM_DISARM
        && instanceData->takeoffHealthCheckStage == PX4TakeoffHealthCheckStage::WaitingForArmAcceptance) {
        if (result == MAV_RESULT_ACCEPTED && !noReponseFromVehicle) {
            instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;
        } else {
            _requestTakeoffHealthCheck(vehicle, PX4TakeoffHealthCheckStage::RefreshAfterArmRejection);
        }
        return;
    }
}

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double takeoffAltRel)
{
    // 每架飞机独立运行“刷新预检 -> TAKEOFF ACK -> 再刷新 -> ARM ACK”状态机；
    // 不能复用旧健康报告，也不能把 TAKEOFF 已接受等同于自动解锁成功。
    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData || instanceData->takeoffHealthCheckStage != PX4TakeoffHealthCheckStage::Idle) {
        return;
    }

    const double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        _showTakeoffHealthFailure(vehicle, tr("No valid local position/GPS accuracy check failed."));
        return;
    }

    _requestTakeoffHealthCheck(vehicle, PX4TakeoffHealthCheckStage::RefreshBeforeTakeoff, takeoffAltRel);
}

void PX4FirmwarePlugin::_requestTakeoffHealthCheck(Vehicle* vehicle,
                                                   PX4TakeoffHealthCheckStage stage,
                                                   double takeoffAltRel)
{
    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData) {
        return;
    }

    HealthAndArmingCheckReport* report = vehicle->healthAndArmingCheckReport();
    if (!report->supported()) {
        instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;
        _showTakeoffHealthFailure(vehicle, _basicTakeoffHealthFailure(vehicle));
        return;
    }

    instanceData->takeoffHealthCheckStage = stage;
    instanceData->takeoffHealthReportSequence = report->updateSequence();
    instanceData->pendingTakeoffAltitudeRelative = takeoffAltRel;
    instanceData->prearmCheckAcknowledged = false;

    vehicle->sendMavCommand(vehicle->defaultComponentId(),
                            MAV_CMD_RUN_PREARM_CHECKS,
                            false);
}

void PX4FirmwarePlugin::_takeoffHealthReportUpdated(Vehicle* vehicle)
{
    _tryCompleteTakeoffHealthCheck(vehicle);
}

void PX4FirmwarePlugin::_tryCompleteTakeoffHealthCheck(Vehicle* vehicle)
{
    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData
        || !isTakeoffHealthRefreshStage(instanceData->takeoffHealthCheckStage)
        || !instanceData->prearmCheckAcknowledged
        || vehicle->healthAndArmingCheckReport()->updateSequence() <= instanceData->takeoffHealthReportSequence) {
        return;
    }

    // 只有预检命令 ACK 和更新序号更大的健康报告同时到达，才允许推进状态机。
    const PX4TakeoffHealthCheckStage completedStage = instanceData->takeoffHealthCheckStage;
    const double takeoffAltRel = instanceData->pendingTakeoffAltitudeRelative;
    instanceData->takeoffHealthCheckTimer.stop();
    instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;

    HealthAndArmingCheckReport* report = vehicle->healthAndArmingCheckReport();
    if (isRejectedCommandRefreshStage(completedStage)) {
        if (!report->canTakeoff() || !report->canArm()) {
            _showTakeoffHealthFailure(vehicle, report->takeoffFailureMessage());
        } else {
            const QString commandName = completedStage == PX4TakeoffHealthCheckStage::RefreshAfterArmRejection
                ? tr("automatic arming")
                : tr("the takeoff command");
            _showTakeoffHealthFailure(
                vehicle,
                tr("The vehicle rejected %1, but the refreshed preflight report did not identify a blocking item.")
                    .arg(commandName)
                    + QStringLiteral("\n") + _basicTakeoffStatusDetails(vehicle));
        }
        return;
    }

    if (!report->canTakeoff()
        || (completedStage == PX4TakeoffHealthCheckStage::RefreshBeforeArm && !report->canArm())) {
        _showTakeoffHealthFailure(vehicle, report->takeoffFailureMessage());
        return;
    }

    _continueTakeoff(vehicle, completedStage, takeoffAltRel);
}

void PX4FirmwarePlugin::_takeoffHealthCheckTimedOut(Vehicle* vehicle)
{
    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData
        || !isTakeoffHealthRefreshStage(instanceData->takeoffHealthCheckStage)) {
        return;
    }

    instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::Idle;
    _showTakeoffHealthFailure(vehicle, _basicTakeoffHealthFailure(vehicle));
}

void PX4FirmwarePlugin::_continueTakeoff(Vehicle* vehicle,
                                         PX4TakeoffHealthCheckStage completedStage,
                                         double takeoffAltRel)
{
    if (completedStage == PX4TakeoffHealthCheckStage::RefreshBeforeTakeoff) {
        _sendTakeoffCommand(vehicle, takeoffAltRel);
    } else if (completedStage == PX4TakeoffHealthCheckStage::RefreshBeforeArm && !vehicle->armed()) {
        auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
        if (!instanceData) {
            return;
        }
        instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::WaitingForArmAcceptance;
        vehicle->setArmed(true, false);
    }
}

void PX4FirmwarePlugin::_sendTakeoffCommand(Vehicle* vehicle, double takeoffAltRel)
{
    const double vehicleAltitudeAMSL = vehicle->altitudeAMSL()->rawValue().toDouble();
    if (qIsNaN(vehicleAltitudeAMSL)) {
        _showTakeoffHealthFailure(vehicle, tr("No valid local position/GPS accuracy check failed."));
        return;
    }

    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData) {
        return;
    }

    const double takeoffAltAMSL = takeoffAltRel + vehicleAltitudeAMSL;
    instanceData->takeoffHealthCheckStage = PX4TakeoffHealthCheckStage::WaitingForTakeoffAcceptance;

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_NAV_TAKEOFF,
        false,                                  // handled by the takeoff health state machine
        -1,                                     // No pitch requested
        0, 0,                                   // param 2-4 unused
        NAN, NAN, NAN,                          // No yaw, lat, lon
        static_cast<float>(takeoffAltAMSL));    // AMSL altitude
}

void PX4FirmwarePlugin::_showTakeoffHealthFailure(Vehicle* vehicle, const QString& reason)
{
    qgcApp()->showAppMessage(tr("Takeoff blocked for Vehicle %1:\n%2").arg(vehicle->id()).arg(reason));
}

QString PX4FirmwarePlugin::_basicTakeoffHealthFailure(Vehicle* vehicle) const
{
    QString message = tr("No valid local position/GPS accuracy check failed.");
    message += QStringLiteral("\n\u2022 ")
        + tr("The flight controller did not provide a current detailed preflight result; automatic arming was not attempted.");

    const QString statusDetails = _basicTakeoffStatusDetails(vehicle);
    if (!statusDetails.isEmpty()) {
        message += QStringLiteral("\n") + statusDetails;
    }
    return message;
}

QString PX4FirmwarePlugin::_basicTakeoffStatusDetails(Vehicle* vehicle) const
{
    QStringList details;
    auto* gps = qobject_cast<VehicleGPSFactGroup*>(vehicle->gpsFactGroup());

    if (gps) {
        const int fixType = gps->lock()->rawValue().toInt();
        const int satelliteCount = gps->count()->rawValue().toInt();

        if (fixType < GPS_FIX_TYPE_3D_FIX) {
            details.append(tr("No live 3D GPS fix (fix type %1, %2 satellites).")
                               .arg(fixType)
                               .arg(satelliteCount));
        } else {
            details.append(tr("GPS reports fix type %1 with %2 satellites, but a valid EKF local position was not confirmed.")
                               .arg(fixType)
                               .arg(satelliteCount));
        }

        const double hdop = gps->hdop()->rawValue().toDouble();
        if (!qIsNaN(hdop)) {
            details.append(tr("GPS HDOP: %1.").arg(hdop, 0, 'f', 1));
        }
    }

    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        details.append(tr("No valid vehicle position/altitude is available."));
    }

    QString message;
    for (const QString& detail : details) {
        if (!message.isEmpty()) {
            message += QStringLiteral("\n");
        }
        message += QStringLiteral("\u2022 ") + detail;
    }
    return message;
}

double PX4FirmwarePlugin::maximumHorizontalSpeedMultirotor(Vehicle* vehicle)
{
    QString speedParam("MPC_XY_VEL_MAX");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, speedParam)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, speedParam)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumHorizontalSpeedMultirotor(vehicle);
}

double PX4FirmwarePlugin::guidedTakeoffSpeed(Vehicle* vehicle)
{
    static const QString takeoffSpeedParam(QStringLiteral("MPC_TKO_SPEED"));

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffSpeedParam)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffSpeedParam)->rawValue().toDouble();
    }

    return FirmwarePlugin::guidedTakeoffSpeed(vehicle);
}

bool PX4FirmwarePlugin::setGuidedTakeoffSpeed(Vehicle* vehicle, double metersSecond)
{
    static const QString takeoffSpeedParam(QStringLiteral("MPC_TKO_SPEED"));

    if (!qIsFinite(metersSecond)
        || !vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, takeoffSpeedParam)) {
        return false;
    }

    Fact* speedFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, takeoffSpeedParam);
    if (!speedFact->metaData()) {
        return false;
    }
    const double minimum = speedFact->metaData()->rawMin().toDouble();
    const double maximum = speedFact->metaData()->rawMax().toDouble();
    if (metersSecond < minimum || metersSecond > maximum) {
        return false;
    }

    speedFact->setRawValue(metersSecond);
    return true;
}

double PX4FirmwarePlugin::maximumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMax("FW_AIRSPD_MAX");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, airspeedMax)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, airspeedMax)->rawValue().toDouble();
    }

    return FirmwarePlugin::maximumEquivalentAirspeed(vehicle);
}

double PX4FirmwarePlugin::minimumEquivalentAirspeed(Vehicle* vehicle)
{
    QString airspeedMin("FW_AIRSPD_MIN");

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, airspeedMin)) {
        return vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, airspeedMin)->rawValue().toDouble();
    }

    return FirmwarePlugin::minimumEquivalentAirspeed(vehicle);
}

bool PX4FirmwarePlugin::mulirotorSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "MPC_XY_VEL_MAX");
}

bool PX4FirmwarePlugin::fixedWingAirSpeedLimitsAvailable(Vehicle* vehicle)
{
    return vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "FW_AIRSPD_MIN") &&
            vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "FW_AIRSPD_MAX");
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showAppMessage(tr("Unable to go to location, vehicle position not known."));
        return;
    }

    if (vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        vehicle->sendMavCommandInt(vehicle->defaultComponentId(),
                                   MAV_CMD_DO_REPOSITION,
                                   MAV_FRAME_GLOBAL,
                                   true,   // show error is fails
                                   -1.0f,
                                   MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                                   0.0f,
                                   NAN,
                                   gotoCoord.latitude(),
                                   gotoCoord.longitude(),
                                   vehicle->altitudeAMSL()->rawValue().toFloat());
    } else {
        vehicle->sendMavCommand(vehicle->defaultComponentId(),
                                MAV_CMD_DO_REPOSITION,
                                true,   // show error is fails
                                -1.0f,
                                MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                                0.0f,
                                NAN,
                                static_cast<float>(gotoCoord.latitude()),
                                static_cast<float>(gotoCoord.longitude()),
                                vehicle->altitudeAMSL()->rawValue().toFloat());
    }
}

typedef struct {
    PX4FirmwarePlugin*  plugin;
    Vehicle*            vehicle;
    double              newAMSLAlt;
} PauseVehicleThenChangeAltData_t;

static void _pauseVehicleThenChangeAltResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            qDebug() << QStringLiteral("MAV_CMD_DO_REPOSITION error(%1)").arg(ack.result);
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            qDebug() << "MAV_CMD_DO_REPOSITION no response from vehicle";
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            qDebug() << "Internal Error: MAV_CMD_DO_REPOSITION could not be sent due to duplicate command";
            break;
        case Vehicle::MavCmdResultFailureLateAckQuarantine:
            qDebug() << "MAV_CMD_DO_REPOSITION blocked by delayed ACK quarantine";
            break;
        default:
            break;
        }
    }

    PauseVehicleThenChangeAltData_t* pData = static_cast<PauseVehicleThenChangeAltData_t*>(resultHandlerData);
    pData->plugin->_changeAltAfterPause(resultHandlerData, ack.result == MAV_RESULT_ACCEPTED /* pauseSucceeded */);
}

void PX4FirmwarePlugin::_changeAltAfterPause(void* resultHandlerData, bool pauseSucceeded)
{
    PauseVehicleThenChangeAltData_t* pData = static_cast<PauseVehicleThenChangeAltData_t*>(resultHandlerData);

    if (pauseSucceeded) {
        pData->vehicle->sendMavCommand(
                    pData->vehicle->defaultComponentId(),
                    MAV_CMD_DO_REPOSITION,
                    true,                                   // show error is fails
                    -1.0f,                                  // Don't change groundspeed
                    MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                    0.0f,                                   // Reserved
                    qQNaN(), qQNaN(), qQNaN(),              // No change to yaw, lat, lon
                    static_cast<float>(pData->newAMSLAlt));
    } else {
        qgcApp()->showAppMessage(tr("Unable to pause vehicle."));
    }

    delete pData;
}

void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange, bool pauseVehicle)
{
    if (!vehicle->homePosition().isValid()) {
        qgcApp()->showAppMessage(tr("Unable to change altitude, home position unknown."));
        return;
    }
    if (qIsNaN(vehicle->homePosition().altitude())) {
        qgcApp()->showAppMessage(tr("Unable to change altitude, home position altitude unknown."));
        return;
    }

    double currentAltRel = vehicle->altitudeRelative()->rawValue().toDouble();
    double newAltRel = currentAltRel + altitudeChange;

    PauseVehicleThenChangeAltData_t* resultData = new PauseVehicleThenChangeAltData_t;
    resultData->plugin      = this;
    resultData->vehicle     = vehicle;
    resultData->newAMSLAlt  = vehicle->homePosition().altitude() + newAltRel;

    if (pauseVehicle) {
        Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
        handlerInfo.resultHandler       = _pauseVehicleThenChangeAltResultHandler;
        handlerInfo.resultHandlerData   = resultData;

        vehicle->sendMavCommandWithHandler(
                    &handlerInfo,
                    vehicle->defaultComponentId(),
                    MAV_CMD_DO_REPOSITION,
                    -1.0f,                                  // Don't change groundspeed
                    MAV_DO_REPOSITION_FLAGS_CHANGE_MODE,
                    0.0f,                                   // Reserved
                    qQNaN(), qQNaN(), qQNaN(), qQNaN());    // No change to yaw, lat, lon, alt
    } else {
        _changeAltAfterPause(resultData, true /* pauseSucceeded */);
    }
}

void PX4FirmwarePlugin::guidedModeChangeGroundSpeedMetersSecond(Vehicle* vehicle, double groundspeed)
{

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                   // show error is fails
        1,                                     // 0: airspeed, 1: groundspeed
        static_cast<float>(groundspeed),       // groundspeed setpoint
        -1,                                   // throttle
        0,                                    // 0: absolute speed, 1: relative to current
        NAN, NAN,NAN);                        // param 5-7 unused
}

void PX4FirmwarePlugin::guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle* vehicle, double airspeed_equiv)
{

    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_DO_CHANGE_SPEED,
        true,                                   // show error is fails
        0,                                     // 0: airspeed, 1: groundspeed
        static_cast<float>(airspeed_equiv),       // groundspeed setpoint
        -1,                                   // throttle
        0,                                    // 0: absolute speed, 1: relative to current
        NAN, NAN,NAN);                        // param 5-7 unused
}

void PX4FirmwarePlugin::startMission(Vehicle* vehicle)
{
    if (_setFlightModeAndValidate(vehicle, missionFlightMode())) {
        if (!_armVehicleAndValidate(vehicle)) {
            qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle rejected arming."));
            return;
        }
    } else {
        qgcApp()->showAppMessage(tr("Unable to start mission: Vehicle not changing to %1 flight mode.").arg(missionFlightMode()));
    }
}

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        _setFlightModeAndValidate(vehicle, _holdFlightMode);
    } else {
        pauseVehicle(vehicle);
    }
}

bool PX4FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    return (vehicle->flightMode() == _holdFlightMode || vehicle->flightMode() == _takeoffFlightMode
            || vehicle->flightMode() == _landingFlightMode);
}

bool PX4FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    //-- Don't process messages to/from UDP Bridge. It doesn't suffer from these issues
    if (message->compid == MAV_COMP_ID_UDP_BRIDGE) {
        return true;
    }

    switch (message->msgid) {
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(vehicle, message);
        break;
    }

    return true;
}

void PX4FirmwarePlugin::_handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

    auto* instanceData = qobject_cast<PX4FirmwarePluginInstanceData*>(vehicle->firmwarePluginInstanceData());
    if (!instanceData->versionNotified) {
        bool notifyUser = false;
        int supportedMajorVersion = 1;
        int supportedMinorVersion = 4;
        int supportedPatchVersion = 1;

        mavlink_autopilot_version_t version;
        mavlink_msg_autopilot_version_decode(message, &version);

        if (version.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;

            majorVersion = (version.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (version.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (version.flight_sw_version >> (8*1)) & 0xFF;

            if (majorVersion < supportedMajorVersion) {
                notifyUser = true;
            } else if (majorVersion == supportedMajorVersion) {
                if (minorVersion < supportedMinorVersion) {
                    notifyUser = true;
                } else if (minorVersion == supportedMinorVersion) {
                    notifyUser = patchVersion < supportedPatchVersion;
                }
            }
        } else {
            notifyUser = true;
        }

        if (notifyUser) {
            instanceData->versionNotified = true;
            qgcApp()->showAppMessage(tr("QGroundControl supports PX4 Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.").arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}

uint32_t PX4FirmwarePlugin::highLatencyCustomModeTo32Bits(uint16_t hlCustomMode)
{
    union px4_custom_mode px4_cm;
    px4_cm.data = 0;
    px4_cm.custom_mode_hl = hlCustomMode;

    return px4_cm.data;
}

QString PX4FirmwarePlugin::_getLatestVersionFileUrl(Vehicle* vehicle){
    Q_UNUSED(vehicle);
    return QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases");
}

QString PX4FirmwarePlugin::_versionRegex() {
    return QStringLiteral("v([0-9,\\.]*) Stable");
}

bool PX4FirmwarePlugin::supportsNegativeThrust(Vehicle* vehicle)
{
    return ((vehicle->vehicleType() == MAV_TYPE_GROUND_ROVER) || (vehicle->vehicleType() == MAV_TYPE_SUBMARINE));
}

QString PX4FirmwarePlugin::getHobbsMeter(Vehicle* vehicle)
{
    static const char* HOOBS_HI = "LND_FLIGHT_T_HI";
    static const char* HOOBS_LO = "LND_FLIGHT_T_LO";
    uint64_t hobbsTimeSeconds = 0;

    if (vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, HOOBS_HI) &&
            vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, HOOBS_LO)) {
        Fact* factHi = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, HOOBS_HI);
        Fact* factLo = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, HOOBS_LO);
        hobbsTimeSeconds = ((uint64_t)factHi->rawValue().toUInt() << 32 | (uint64_t)factLo->rawValue().toUInt()) / 1000000;
        qCDebug(VehicleLog) << "Hobbs Meter raw PX4:" << "(" << factHi->rawValue().toUInt() << factLo->rawValue().toUInt() << ")";
    }

    int hours   = hobbsTimeSeconds / 3600;
    int minutes = (hobbsTimeSeconds % 3600) / 60;
    int seconds = hobbsTimeSeconds % 60;
    QString timeStr = QString::asprintf("%04d:%02d:%02d", hours, minutes, seconds);
    qCDebug(VehicleLog) << "Hobbs Meter string:" << timeStr;
    return timeStr;
}

bool PX4FirmwarePlugin::hasGripper(const Vehicle* vehicle) const
{
    if(vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, QStringLiteral("PD_GRIPPER_EN"))) {
        bool _hasGripper = (vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, QStringLiteral("PD_GRIPPER_EN"))->rawValue().toInt()) != 0 ? true : false;
        return _hasGripper;
    }
    return false;
}

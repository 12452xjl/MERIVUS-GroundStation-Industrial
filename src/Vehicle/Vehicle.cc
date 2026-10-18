/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTime>
#include <QDateTime>
#include <QLocale>
#include <QQuaternion>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QThread>
#include <QUuid>
#include <QMutex>
#include <QMutexLocker>

#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>

#include <Eigen/Eigen>

#include "Vehicle.h"
#include "FleetStartupOdometry.h"
#include "FleetStartupGlobalOrigin.h"
#include "FleetStartupAltitude.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionReconciliation.h"
#include "FleetLinkLatencyCapture.h"
#include "FleetCameraStreamCapture.h"
#include "FleetCameraStreamControl.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
#include "APMFirmwarePlugin.h"
#endif
#include "UAS.h"
#include "JoystickManager.h"
#include "MissionManager.h"
#include "MissionController.h"
#include "PlanMasterController.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "FlightPathSegment.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QGCImageProvider.h"
#include "MissionCommandTree.h"
#include "SettingsManager.h"
#include "QGCQGeoCoordinate.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "ADSBVehicleManager.h"
#include "QGCCameraManager.h"
#include "VideoReceiver.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "PositionManager.h"
#include "VehicleObjectAvoidance.h"
#include "TrajectoryPoints.h"
#include "QGCGeo.h"
#include "TerrainProtocolHandler.h"
#include "ParameterManager.h"
#include "FTPManager.h"
#include "ComponentInformationManager.h"
#include "InitialConnectStateMachine.h"
#include "VehicleBatteryFactGroup.h"
#include "EventHandler.h"
#include "Actuators/Actuators.h"
#ifdef QT_DEBUG
#include "MockLink.h"
#endif
#include "Autotune.h"
#include "RemoteIDManager.h"

// These native objects have no metatype, invokable constructor, JSON codec or
// public definition. A portable report/ID cannot recreate either authority.
class FleetOptionalTelemetryConfigurationBasis
{
    friend class Vehicle;
    QPointer<Vehicle> vehicle;
    QPointer<QObject> captureOwner;
    QString id;
    QVector<std::shared_ptr<Vehicle::FleetMessageIntervalQuery>> queries;
    std::function<bool()> sourceFence;
    qint64 deadlineMs = -1;
    bool consumed = false;
    mutable bool checkingSource = false;
};

class FleetOptionalTelemetryConfigurationRun
{
    friend class Vehicle;
    struct Row {
        QString setRequestId = QStringLiteral("");
        bool setRequestIssued = false, setAckObserved = false;
        int setAckResult = -1;
        std::shared_ptr<Vehicle::FleetMessageIntervalQuery> readback, finalReadback;
    };
    QPointer<Vehicle> vehicle;
    QPointer<QObject> owner;
    std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> basis;
    std::function<bool()> finalFence;
    QString id, ledgerEpoch;
    quint64 expectedMutation = 0;
    int intervalUs = 0, stage = 0;
    qint64 wireDeadlineMs = -1;
    bool active = true, ledgerBound = true, authorizationBusy = false;
    QString state = QStringLiteral("ready"), reason = QStringLiteral("");
    QVector<Row> rows;
    std::shared_ptr<Vehicle::FleetMessageIntervalQuery> currentQuery;
};

// Only Vehicle can create this run. In particular a portable diagnostic map
// cannot recreate its owner, PRIMARY control block, send fact or deadline.
class FleetLinkLatencyRun
{
    friend class Vehicle;
    QPointer<Vehicle> vehicle;
    QPointer<FleetLinkLatencyCapture> owner;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    QPointer<VehicleLinkManager> links;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<FirmwarePlugin> plugin;
    QPointer<QGCToolbox> toolbox;
    QPointer<MultiVehicleManager> vehicles;
    QPointer<ParameterManager> parameters;
    QString id, versionEpoch, transportEpoch, parameterEpoch;
    quint64 versionRevision = 0;
    int vehicleId = 0, componentId = 0, firmwareType = 0, vehicleType = 0;
    uint8_t channel = 0, systemId = 0, sourceComponentId = 0;
    QElapsedTimer clock;
    qint64 deadlineMs = -1, nonce = 0, sentAtNs = -1, responseAtNs = -1;
    double roundTripMs = -1.0;
    std::function<bool()> authorizationFence;
    bool active = true, requestIssued = false, responseObserved = false;
    bool authorizationBusy = false, dispatching = false, sourceRetired = false;
    QString state = QStringLiteral("ready"), reason = QStringLiteral("");
};

class FleetCameraStreamRun
{
    friend class Vehicle;
    struct Query {
        Vehicle::MavCommandListEntry_t source{};
        QString id = QStringLiteral("");
        int messageId = 0, ackResult = -1;
        bool requestIssued = false, ackObserved = false, reportObserved = false;
        quint64 baseline = 0, receipt = 0, revision = 0;
        qint64 deadlineMs = -1;
        QString state = QStringLiteral("ready"), reason = QStringLiteral("");
    };
    QPointer<Vehicle> vehicle;
    QPointer<FleetCameraStreamCapture> owner;
    Vehicle::MavCommandListEntry_t origin{};
    Vehicle::FleetCameraStreamRow reports;
    Query queries[2];
    QString id, sourceEpoch;
    quint64 mutationGeneration = 0;
    bool controlSealed = false;
    int componentId = 0, streamId = 0, stage = 0;
    qint64 deadlineMs = -1;
    bool active = true, sourceRetired = false, authorizationBusy = false, dispatching = false;
    QString state = QStringLiteral("ready"), reason = QStringLiteral("");
    std::function<bool()> authorizationFence;
};

class FleetCameraStreamControlBasis
{
    friend class Vehicle;
    QPointer<Vehicle> vehicle;
    QPointer<FleetCameraStreamCapture> captureOwner;
    Vehicle::MavCommandListEntry_t origin{};
    Vehicle::FleetCameraStreamRow reports;
    QString id, captureId, sourceEpoch;
    quint64 mutation = 0;
    qint64 deadlineMs = -1;
    bool originalRunning = false, targetRunning = false, consumed = false;
    mutable bool checkingSource = false;
    std::function<bool()> sourceFence;
};

class FleetCameraStreamControlRun
{
    friend class Vehicle;
    QPointer<Vehicle> vehicle;
    QPointer<FleetCameraStreamControl> owner;
    std::shared_ptr<FleetCameraStreamControlBasis> basis;
    Vehicle::MavCommandListEntry_t actionSource{}, querySource{};
    Vehicle::FleetCameraStreamRow reports;
    QString id, actionId = QStringLiteral(""), queryId = QStringLiteral("");
    quint64 expectedMutation = 0, baseline = 0, receipt = 0, revision = 0;
    qint64 wireDeadlineMs = -1;
    int stage = 0, actionAckResult = -1, queryAckResult = -1;
    bool actionIssued = false, actionAckObserved = false, queryIssued = false, queryAckObserved = false;
    bool reportObserved = false, active = true, sourceRetired = false, authorizationBusy = false, dispatching = false;
    QString state = QStringLiteral("ready"), reason = QStringLiteral("");
    QString actionState = QStringLiteral("ready"), actionReason = QStringLiteral("");
    QString queryState = QStringLiteral("ready"), queryReason = QStringLiteral("");
    std::function<bool()> finalFence;
};

QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f
#define SET_HOME_TERRAIN_ALT_MAX 10000
#define SET_HOME_TERRAIN_ALT_MIN -500

const QString guided_mode_not_supported_by_vehicle = QObject::tr("Guided mode not supported by Vehicle.");

const char* Vehicle::_settingsGroup =               "Vehicle%1";        // %1 replaced with mavlink system id
const char* Vehicle::_joystickEnabledSettingsKey =  "JoystickEnabled";

const char* Vehicle::_rollFactName =                "roll";
const char* Vehicle::_pitchFactName =               "pitch";
const char* Vehicle::_headingFactName =             "heading";
const char* Vehicle::_rollRateFactName =             "rollRate";
const char* Vehicle::_pitchRateFactName =           "pitchRate";
const char* Vehicle::_yawRateFactName =             "yawRate";
const char* Vehicle::_airSpeedFactName =            "airSpeed";
const char* Vehicle::_airSpeedSetpointFactName =    "airSpeedSetpoint";
const char* Vehicle::_xTrackErrorFactName =         "xTrackError";
const char* Vehicle::_rangeFinderDistFactName =     "rangeFinderDist";
const char* Vehicle::_groundSpeedFactName =         "groundSpeed";
const char* Vehicle::_climbRateFactName =           "climbRate";
const char* Vehicle::_altitudeRelativeFactName =    "altitudeRelative";
const char* Vehicle::_altitudeAMSLFactName =        "altitudeAMSL";
const char* Vehicle::_altitudeAboveTerrFactName =   "altitudeAboveTerr";
const char* Vehicle::_altitudeTuningFactName =      "altitudeTuning";
const char* Vehicle::_altitudeTuningSetpointFactName = "altitudeTuningSetpoint";
const char* Vehicle::_flightDistanceFactName =      "flightDistance";
const char* Vehicle::_flightTimeFactName =          "flightTime";
const char* Vehicle::_distanceToHomeFactName =      "distanceToHome";
const char* Vehicle::_timeToHomeFactName =          "timeToHome";
const char* Vehicle::_missionItemIndexFactName =    "missionItemIndex";
const char* Vehicle::_headingToNextWPFactName =     "headingToNextWP";
const char* Vehicle::_headingToHomeFactName =       "headingToHome";
const char* Vehicle::_distanceToGCSFactName =       "distanceToGCS";
const char* Vehicle::_hobbsFactName =               "hobbs";
const char* Vehicle::_throttlePctFactName =         "throttlePct";
const char* Vehicle::_imuTempFactName =             "imuTemp";

const char* Vehicle::_gpsFactGroupName =                "gps";
const char* Vehicle::_gps2FactGroupName =               "gps2";
const char* Vehicle::_windFactGroupName =               "wind";
const char* Vehicle::_vibrationFactGroupName =          "vibration";
const char* Vehicle::_temperatureFactGroupName =        "temperature";
const char* Vehicle::_clockFactGroupName =              "clock";
const char* Vehicle::_setpointFactGroupName =           "setpoint";
const char* Vehicle::_distanceSensorFactGroupName =     "distanceSensor";
const char* Vehicle::_localPositionFactGroupName =      "localPosition";
const char* Vehicle::_localPositionSetpointFactGroupName ="localPositionSetpoint";
const char* Vehicle::_escStatusFactGroupName =          "escStatus";
const char* Vehicle::_ftcStatusFactGroupName =          "ftcStatus";
const char* Vehicle::_estimatorStatusFactGroupName =    "estimatorStatus";
const char* Vehicle::_terrainFactGroupName =            "terrain";
const char* Vehicle::_hygrometerFactGroupName =         "hygrometer";
const char* Vehicle::_generatorFactGroupName =          "generator";
const char* Vehicle::_efiFactGroupName =                "efi";

namespace {

constexpr qint64 OptionalTelemetryWindowMs = 5000;
constexpr int OptionalTelemetryMaximumSamples = 512;
constexpr int FleetLocalReceptionWindowMs = 5000;
constexpr int FleetLocalReceptionMaximumFrames = 4096;
constexpr int FleetLocalReceptionMaximumAgeMs = 30000;
constexpr int FleetCameraStreamMaximumRows = 32;
bool fleetCameraStreamFrame(const mavlink_message_t& message)
{
    return (message.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION || message.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_STATUS)
        && message.compid >= MAV_COMP_ID_CAMERA && message.compid <= MAV_COMP_ID_CAMERA6;
}
bool sameFleetCameraInformation(const mavlink_video_stream_information_t& left, const mavlink_video_stream_information_t& right)
{
    // Private bounded bytes participate in change detection but are never
    // exposed: stream URIs may contain credentials. No struct padding compare.
    return memcmp(&left.framerate, &right.framerate, sizeof(left.framerate)) == 0
        && left.bitrate == right.bitrate && left.flags == right.flags
        && left.resolution_h == right.resolution_h && left.resolution_v == right.resolution_v
        && left.rotation == right.rotation && left.hfov == right.hfov && left.stream_id == right.stream_id
        && left.count == right.count && left.type == right.type
        && memcmp(left.name, right.name, sizeof(left.name)) == 0 && memcmp(left.uri, right.uri, sizeof(left.uri)) == 0;
}
bool sameFleetCameraInformationConfiguration(const mavlink_video_stream_information_t& left,
    const mavlink_video_stream_information_t& right)
{
    // RUNNING is observed operating state, not stream configuration. Keep all
    // other raw fields (including the private bounded name/URI) significant.
    auto first = left;
    auto second = right;
    first.flags &= uint16_t(~uint16_t(VIDEO_STREAM_STATUS_FLAGS_RUNNING));
    second.flags &= uint16_t(~uint16_t(VIDEO_STREAM_STATUS_FLAGS_RUNNING));
    return sameFleetCameraInformation(first, second);
}
bool sameFleetCameraStatus(const mavlink_video_stream_status_t& left, const mavlink_video_stream_status_t& right)
{
    return memcmp(&left.framerate, &right.framerate, sizeof(left.framerate)) == 0
        && left.bitrate == right.bitrate && left.flags == right.flags
        && left.resolution_h == right.resolution_h && left.resolution_v == right.resolution_v
        && left.rotation == right.rotation && left.hfov == right.hfov && left.stream_id == right.stream_id;
}
qint64 nextFleetLinkLatencyTimestamp()
{
    // Timestamp-like nanoseconds anchored once to wall time, then advanced
    // only by a process-wide monotonic clock. Serialise all Vehicle instances;
    // last+1 prevents equal timestamps and never wraps/reuses a nonce.
    static QMutex mutex;
    static QElapsedTimer clock;
    static qint64 originNs = 0, last = 0;
    const QMutexLocker lock(&mutex);
    if (!clock.isValid()) {
        const qint64 milliseconds = QDateTime::currentMSecsSinceEpoch();
        if (milliseconds <= 0 || milliseconds > std::numeric_limits<qint64>::max() / 1000000) return 0;
        originNs = milliseconds * 1000000;
        clock.start();
    }
    const qint64 elapsed = clock.nsecsElapsed();
    if (elapsed < 0 || originNs > std::numeric_limits<qint64>::max() - elapsed
        || last == std::numeric_limits<qint64>::max()) return 0;
    const qint64 timestamp = qMax(originNs + elapsed, last + 1);
    if (timestamp <= 0) return 0;
    last = timestamp;
    return timestamp;
}
bool optionalTelemetryMessageId(int messageId)
{
    return messageId == MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT || messageId == MAVLINK_MSG_ID_VFR_HUD
        || messageId == MAVLINK_MSG_ID_ATTITUDE_TARGET;
}
bool optionalTelemetryFrameId(int messageId)
{
    return messageId == MAVLINK_MSG_ID_MESSAGE_INTERVAL || optionalTelemetryMessageId(messageId);
}
bool messageCommand(MAV_CMD command)
{
    return command == MAV_CMD_GET_MESSAGE_INTERVAL || command == MAV_CMD_SET_MESSAGE_INTERVAL
        || command == MAV_CMD_REQUEST_MESSAGE;
}
bool cameraStreamMutationCommand(MAV_CMD command)
{
    return command == MAV_CMD_VIDEO_START_STREAMING || command == MAV_CMD_VIDEO_STOP_STREAMING;
}
bool rawCameraStreamMutationCommand(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_LONG && message.msgid != MAVLINK_MSG_ID_COMMAND_INT) return false;
    if (int(message.len) > MAVLINK_MAX_PAYLOAD_LEN) return true;
    mavlink_message_t normalized = message;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&message), message.len);
    if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t command{}; mavlink_msg_command_long_decode(&normalized, &command);
        return cameraStreamMutationCommand(static_cast<MAV_CMD>(command.command));
    }
    mavlink_command_int_t command{}; mavlink_msg_command_int_decode(&normalized, &command);
    return cameraStreamMutationCommand(static_cast<MAV_CMD>(command.command));
}
QString messageCommandName(MAV_CMD command)
{
    if (command == MAV_CMD_VIDEO_START_STREAMING) return QStringLiteral("MAV_CMD_VIDEO_START_STREAMING");
    if (command == MAV_CMD_VIDEO_STOP_STREAMING) return QStringLiteral("MAV_CMD_VIDEO_STOP_STREAMING");
    if (command == MAV_CMD_GET_MESSAGE_INTERVAL) return QStringLiteral("MAV_CMD_GET_MESSAGE_INTERVAL");
    if (command == MAV_CMD_SET_MESSAGE_INTERVAL) return QStringLiteral("MAV_CMD_SET_MESSAGE_INTERVAL");
    return QStringLiteral("MAV_CMD_REQUEST_MESSAGE");
}
bool rawMessageCommand(const mavlink_message_t& message)
{
    if (int(message.len) > MAVLINK_MAX_PAYLOAD_LEN) return true;
    if (message.msgid != MAVLINK_MSG_ID_COMMAND_LONG && message.msgid != MAVLINK_MSG_ID_COMMAND_INT) return false;
    mavlink_message_t normalized = message;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&message), message.len);
    if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t command{};
        mavlink_msg_command_long_decode(&normalized, &command);
        return messageCommand(static_cast<MAV_CMD>(command.command));
    }
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&normalized, &command);
    return messageCommand(static_cast<MAV_CMD>(command.command));
}

// A callback context belongs to exactly one PID-stream request. The Vehicle
// owns it if no terminal callback arrives before Vehicle destruction.
class StreamIntervalCommandContext final : public QObject
{
public:
    QPointer<Vehicle> vehicle;
    QPointer<VehicleLinkManager> links;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<FirmwarePlugin> plugin;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    QString sourceEpoch;
    MAVLinkStreamConfig::RequestId requestId = 0;
    int componentId = 0;
};

const QStringList& flightObservationChannels()
{
    static const QStringList channels = {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"),
        QStringLiteral("homePosition"), QStringLiteral("landedState"), QStringLiteral("missionCurrent"), QStringLiteral("missionReached")};
    return channels;
}

QString flightObservationChannel(uint32_t messageId, int* fullLength, int* minimumLength)
{
    switch (messageId) {
    case MAVLINK_MSG_ID_HEARTBEAT:
        *fullLength = MAVLINK_MSG_ID_HEARTBEAT_LEN; *minimumLength = MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN;
        return QStringLiteral("heartbeat");
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        *fullLength = MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN; *minimumLength = MAVLINK_MSG_ID_GLOBAL_POSITION_INT_MIN_LEN;
        return QStringLiteral("globalPosition");
    case MAVLINK_MSG_ID_HOME_POSITION:
        *fullLength = MAVLINK_MSG_ID_HOME_POSITION_LEN; *minimumLength = MAVLINK_MSG_ID_HOME_POSITION_MIN_LEN;
        return QStringLiteral("homePosition");
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
        *fullLength = MAVLINK_MSG_ID_EXTENDED_SYS_STATE_LEN; *minimumLength = MAVLINK_MSG_ID_EXTENDED_SYS_STATE_MIN_LEN;
        return QStringLiteral("landedState");
    case MAVLINK_MSG_ID_MISSION_CURRENT:
        *fullLength = MAVLINK_MSG_ID_MISSION_CURRENT_LEN; *minimumLength = MAVLINK_MSG_ID_MISSION_CURRENT_MIN_LEN;
        return QStringLiteral("missionCurrent");
    case MAVLINK_MSG_ID_MISSION_ITEM_REACHED:
        *fullLength = MAVLINK_MSG_ID_MISSION_ITEM_REACHED_LEN; *minimumLength = MAVLINK_MSG_ID_MISSION_ITEM_REACHED_MIN_LEN;
        return QStringLiteral("missionReached");
    default:
        return QString();
    }
}

QString flightObservationSource(const QString& channel)
{
    if (channel == QStringLiteral("heartbeat")) return QStringLiteral("live-heartbeat");
    if (channel == QStringLiteral("globalPosition")) return QStringLiteral("live-global-position-int");
    if (channel == QStringLiteral("homePosition")) return QStringLiteral("live-home-position");
    if (channel == QStringLiteral("landedState")) return QStringLiteral("live-extended-sys-state");
    if (channel == QStringLiteral("missionCurrent")) return QStringLiteral("live-mission-current");
    if (channel == QStringLiteral("missionReached")) return QStringLiteral("live-mission-item-reached");
    return QStringLiteral("unknown");
}

bool flightObservationCoordinatesValid(int32_t latitude, int32_t longitude)
{
    // (0,0) is a real WGS84 location. GPS fix/accuracy is a separate claim.
    return latitude >= -900000000 && latitude <= 900000000
        && longitude >= -1800000000 && longitude <= 1800000000;
}

bool decodeFlightObservation(const mavlink_message_t& original, MAV_AUTOPILOT firmwareType,
    MAV_TYPE vehicleType, QVariantMap* value)
{
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    // Every stored scalar is an actual bounded integer or bool; no cached
    // flying/armed/coordinate/mission fields are used to fill omissions.
    switch (original.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t heartbeat{};
        mavlink_msg_heartbeat_decode(&normalized, &heartbeat);
        if (heartbeat.mavlink_version != 3 || heartbeat.system_status >= MAV_STATE_ENUM_END
            || heartbeat.autopilot >= MAV_AUTOPILOT_ENUM_END || heartbeat.type >= MAV_TYPE_ENUM_END
            || heartbeat.autopilot != static_cast<int>(firmwareType) || heartbeat.type != static_cast<int>(vehicleType)) return false;
        *value = {{QStringLiteral("armed"), (heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0},
            {QStringLiteral("baseMode"), static_cast<uint>(heartbeat.base_mode)}, {QStringLiteral("customMode"), static_cast<uint>(heartbeat.custom_mode)},
            {QStringLiteral("systemStatus"), static_cast<uint>(heartbeat.system_status)},
            {QStringLiteral("autopilot"), static_cast<uint>(heartbeat.autopilot)}, {QStringLiteral("vehicleType"), static_cast<uint>(heartbeat.type)}};
        return true;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        mavlink_global_position_int_t position{};
        mavlink_msg_global_position_int_decode(&normalized, &position);
        if (!flightObservationCoordinatesValid(position.lat, position.lon)
            || (position.hdg > 35999 && position.hdg != std::numeric_limits<uint16_t>::max())) return false;
        *value = {{QStringLiteral("latitudeE7"), static_cast<int>(position.lat)}, {QStringLiteral("longitudeE7"), static_cast<int>(position.lon)},
            {QStringLiteral("altitudeAmslMm"), static_cast<int>(position.alt)}, {QStringLiteral("relativeAltitudeMm"), static_cast<int>(position.relative_alt)},
            {QStringLiteral("velocityNorthCmPerSecond"), static_cast<int>(position.vx)}, {QStringLiteral("velocityEastCmPerSecond"), static_cast<int>(position.vy)},
            {QStringLiteral("velocityDownCmPerSecond"), static_cast<int>(position.vz)}, {QStringLiteral("headingCdeg"), static_cast<uint>(position.hdg)},
            {QStringLiteral("headingKnown"), position.hdg != std::numeric_limits<uint16_t>::max()}, {QStringLiteral("timeBootMs"), static_cast<uint>(position.time_boot_ms)}};
        return true;
    }
    case MAVLINK_MSG_ID_HOME_POSITION: {
        mavlink_home_position_t home{};
        mavlink_msg_home_position_decode(&normalized, &home);
        if (!flightObservationCoordinatesValid(home.latitude, home.longitude)) return false;
        // Local/quaternion/approach/time extensions are not elevation or
        // landing-clearance proof, and are not in this geodetic channel.
        *value = {{QStringLiteral("latitudeE7"), static_cast<int>(home.latitude)},
            {QStringLiteral("longitudeE7"), static_cast<int>(home.longitude)}, {QStringLiteral("altitudeAmslMm"), static_cast<int>(home.altitude)}};
        return true;
    }
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE: {
        mavlink_extended_sys_state_t state{};
        mavlink_msg_extended_sys_state_decode(&normalized, &state);
        if (state.landed_state >= MAV_LANDED_STATE_ENUM_END || state.vtol_state >= MAV_VTOL_STATE_ENUM_END) return false;
        *value = {{QStringLiteral("landedState"), static_cast<uint>(state.landed_state)}, {QStringLiteral("vtolState"), static_cast<uint>(state.vtol_state)},
            {QStringLiteral("landedStateKnown"), state.landed_state != MAV_LANDED_STATE_UNDEFINED},
            {QStringLiteral("vtolStateKnown"), state.vtol_state != MAV_VTOL_STATE_UNDEFINED}};
        return true;
    }
    case MAVLINK_MSG_ID_MISSION_CURRENT: {
        mavlink_mission_current_t current{};
        mavlink_msg_mission_current_decode(&normalized, &current);
        const bool noMission = current.total == std::numeric_limits<uint16_t>::max();
        const bool totalKnown = current.total != 0 && !noMission;
        if (current.mission_state >= MISSION_STATE_ENUM_END || current.mission_mode > 2
            || (totalKnown && current.seq > current.total)) return false;
        // Extension zeros mean unsupported/unknown, not evidence of a total,
        // a mode or completion. UINT16_MAX reports no mission, not 65535 items.
        *value = {{QStringLiteral("sequence"), static_cast<uint>(current.seq)}, {QStringLiteral("total"), static_cast<uint>(current.total)},
            {QStringLiteral("missionState"), static_cast<uint>(current.mission_state)}, {QStringLiteral("missionMode"), static_cast<uint>(current.mission_mode)},
            {QStringLiteral("totalKnown"), totalKnown}, {QStringLiteral("missionStateKnown"), current.mission_state != MISSION_STATE_UNKNOWN},
            {QStringLiteral("missionModeKnown"), current.mission_mode != 0}, {QStringLiteral("noMissionReported"), noMission}};
        return true;
    }
    case MAVLINK_MSG_ID_MISSION_ITEM_REACHED: {
        mavlink_mission_item_reached_t reached{};
        mavlink_msg_mission_item_reached_decode(&normalized, &reached);
        // PX4 can periodically repeat its last reached sequence. This is a
        // receipt observation, not a unique completion event or disarm proof.
        *value = {{QStringLiteral("sequence"), static_cast<uint>(reached.seq)}};
        return true;
    }
    default:
        return false;
    }
}

QString startupValueHash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}

QString startupRawHash(const mavlink_message_t& message)
{
    return QString::fromLatin1(QCryptographicHash::hash(QByteArray(_MAV_PAYLOAD(&message), message.len), QCryptographicHash::Sha256).toHex());
}

bool startupCoordinateDecode(const mavlink_message_t& original, QString* channel, QVariantMap* value)
{
    value->clear(); channel->clear();
    const bool local = original.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED;
    const bool home = original.msgid == MAVLINK_MSG_ID_HOME_POSITION;
    if (!local && !home) return false;
    *channel = local ? QStringLiteral("localPosition") : QStringLiteral("homePosition");
    const int maximum = local ? MAVLINK_MSG_ID_LOCAL_POSITION_NED_LEN : MAVLINK_MSG_ID_HOME_POSITION_LEN;
    const int minimum = local ? MAVLINK_MSG_ID_LOCAL_POSITION_NED_MIN_LEN : MAVLINK_MSG_ID_HOME_POSITION_MIN_LEN;
    if ((original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)
        || original.len < 1 || original.len > maximum || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != minimum)) return false;
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    if (local) {
        mavlink_local_position_ned_t item{};
        mavlink_msg_local_position_ned_decode(&normalized, &item);
        for (float number : {item.x, item.y, item.z, item.vx, item.vy, item.vz}) if (!std::isfinite(number)) return false;
        *value = {{QStringLiteral("x"), double(item.x)}, {QStringLiteral("y"), double(item.y)}, {QStringLiteral("z"), double(item.z)},
            {QStringLiteral("vx"), double(item.vx)}, {QStringLiteral("vy"), double(item.vy)}, {QStringLiteral("vz"), double(item.vz)},
            {QStringLiteral("timeBootMs"), uint(item.time_boot_ms)}};
    } else {
        mavlink_home_position_t item{};
        mavlink_msg_home_position_decode(&normalized, &item);
        if (!flightObservationCoordinatesValid(item.latitude, item.longitude)) return false;
        for (float number : {item.x, item.y, item.z}) if (std::isinf(number)) return false;
        *value = {{QStringLiteral("latitudeE7"), int(item.latitude)}, {QStringLiteral("longitudeE7"), int(item.longitude)},
            {QStringLiteral("altitudeAmslMm"), int(item.altitude)},
            {QStringLiteral("localX"), std::isnan(item.x) ? QVariant() : QVariant(double(item.x))},
            {QStringLiteral("localY"), std::isnan(item.y) ? QVariant() : QVariant(double(item.y))},
            {QStringLiteral("localZ"), std::isnan(item.z) ? QVariant() : QVariant(double(item.z))},
            {QStringLiteral("localCoordinatesKnown"), std::isfinite(item.x) && std::isfinite(item.y) && std::isfinite(item.z)},
            {QStringLiteral("timeUsec"), QString::number(item.time_usec)}, {QStringLiteral("timeUsecKnown"), item.time_usec != 0}};
    }
    return true;
}

bool startupOdometryNedObservationMatches(const QVariantMap& value)
{
    // This is only a native observation sanity gate for the explicitly
    // reviewed version-2 periodic clock. It does not certify an origin,
    // LOCAL/HOME sample relation, producer implementation or flight permit.
    // Sparse covariance NaNs and omitted/zero quality can remain unknown.
    return value.size() == 50
        && value.value(QStringLiteral("decodeMapShape")).toString() == QStringLiteral("fleet-startup-odometry-v1")
        && value.value(QStringLiteral("schemaVersion")).toInt() == 1
        && value.value(QStringLiteral("messageId")).toInt() == MAVLINK_MSG_ID_ODOMETRY
        && value.value(QStringLiteral("protocol")).toString() == QStringLiteral("mavlink2")
        && value.value(QStringLiteral("observationValid")).toBool()
        && !value.value(QStringLiteral("observationInvalid")).toBool()
        && value.value(QStringLiteral("frameKnown")).toBool()
        && value.value(QStringLiteral("frameId")).toInt() == MAV_FRAME_LOCAL_NED
        && value.value(QStringLiteral("childFrameKnown")).toBool()
        && value.value(QStringLiteral("childFrameId")).toInt() == MAV_FRAME_LOCAL_NED
        && value.value(QStringLiteral("resetCounterPresent")).toBool()
        && !value.value(QStringLiteral("resetCounterUnknown")).toBool()
        && value.value(QStringLiteral("estimatorTypePresent")).toBool()
        && value.value(QStringLiteral("estimatorTypeKnown")).toBool()
        && !value.value(QStringLiteral("estimatorTypeUnknown")).toBool()
        && value.value(QStringLiteral("estimatorType")).toInt() == MAV_ESTIMATOR_TYPE_AUTOPILOT
        && value.value(QStringLiteral("qualityValidRange")).toBool();
}

} // namespace

// Standard connected vehicle
Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 int                        defaultComponentId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 JoystickManager*           joystickManager)
    : FactGroup                     (_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json")
    , _id                           (vehicleId)
    , _defaultComponentId           (defaultComponentId)
    , _firmwareType                 (firmwareType)
    , _vehicleType                  (vehicleType)
    , _toolbox                      (qgcApp()->toolbox())
    , _settingsManager              (_toolbox->settingsManager())
    , _defaultCruiseSpeed           (_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed            (_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _firmwarePluginManager        (firmwarePluginManager)
    , _joystickManager              (joystickManager)
    , _trajectoryPoints             (new TrajectoryPoints(this, this))
    , _mavlinkStreamConfig          (std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _rollFact                     (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact                    (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact                  (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact                 (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact                (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact                  (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact              (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact                 (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _airSpeedSetpointFact         (0, _airSpeedSetpointFactName,  FactMetaData::valueTypeDouble)
    , _climbRateFact                (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact         (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact             (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _altitudeAboveTerrFact        (0, _altitudeAboveTerrFactName, FactMetaData::valueTypeDouble)
    , _altitudeTuningFact           (0, _altitudeTuningFactName,    FactMetaData::valueTypeDouble)
    , _altitudeTuningSetpointFact   (0, _altitudeTuningSetpointFactName, FactMetaData::valueTypeDouble)
    , _xTrackErrorFact              (0, _xTrackErrorFactName,       FactMetaData::valueTypeDouble)
    , _rangeFinderDistFact          (0, _rangeFinderDistFactName,   FactMetaData::valueTypeFloat)
    , _flightDistanceFact           (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact               (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact           (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _timeToHomeFact               (0, _timeToHomeFactName,        FactMetaData::valueTypeDouble)
    , _missionItemIndexFact         (0, _missionItemIndexFactName,  FactMetaData::valueTypeUint16)
    , _headingToNextWPFact          (0, _headingToNextWPFactName,   FactMetaData::valueTypeDouble)
    , _headingToHomeFact            (0, _headingToHomeFactName,     FactMetaData::valueTypeDouble)
    , _distanceToGCSFact            (0, _distanceToGCSFactName,     FactMetaData::valueTypeDouble)
    , _hobbsFact                    (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _throttlePctFact              (0, _throttlePctFactName,       FactMetaData::valueTypeUint16)
    , _imuTempFact                  (0, _imuTempFactName,           FactMetaData::valueTypeInt16)
    , _gpsFactGroup                 (this)
    , _gps2FactGroup                (this)
    , _windFactGroup                (this)
    , _vibrationFactGroup           (this)
    , _temperatureFactGroup         (this)
    , _clockFactGroup               (this)
    , _setpointFactGroup            (this)
    , _distanceSensorFactGroup      (this)
    , _localPositionFactGroup       (this)
    , _localPositionSetpointFactGroup(this)
    , _escStatusFactGroup           (this)
    , _ftcStatusFactGroup           (this)
    , _estimatorStatusFactGroup     (this)
    , _hygrometerFactGroup          (this)
    , _generatorFactGroup           (this)
    , _efiFactGroup                 (this)
    , _terrainFactGroup             (this)
    , _terrainProtocolHandler       (new TerrainProtocolHandler(this, &_terrainFactGroup, this))
{
    _linkManager = _toolbox->linkManager();

    connect(_joystickManager, &JoystickManager::activeJoystickChanged, this, &Vehicle::_loadJoystickSettings);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &Vehicle::_activeVehicleChanged);

    _mavlink = _toolbox->mavlinkProtocol();
    qCDebug(VehicleLog) << "Link started with Mavlink " << (_mavlink->getCurrentVersion() >= 200 ? "V2" : "V1");

    connect(_mavlink, &MAVLinkProtocol::messageReceived,        this, &Vehicle::_mavlinkMessageReceived);
    connect(_mavlink, &MAVLinkProtocol::mavlinkMessageStatus,   this, &Vehicle::_mavlinkMessageStatus);

    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);

    connect(_toolbox->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &Vehicle::_vehicleParamLoaded);

    _uas = new UAS(_mavlink, this, _firmwarePluginManager);
    _uas->setParent(this);

    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _commonInit();

    _vehicleLinkManager->_addLink(link);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && _settingsManager->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        _settingsManager->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        _settingsManager->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    _autopilotPlugin = _firmwarePlugin->autopilotPlugin(this);
    _autopilotPlugin->setParent(this);

    // PreArm Error self-destruct timer
    connect(&_prearmErrorTimer, &QTimer::timeout, this, &Vehicle::_prearmErrorTimeout);
    _prearmErrorTimer.setInterval(_prearmErrorTimeoutMSecs);
    _prearmErrorTimer.setSingleShot(true);

    // Send MAV_CMD ack timer
    _mavCommandResponseCheckTimer.setSingleShot(false);
    _mavCommandResponseCheckTimer.setInterval(_mavCommandResponseCheckTimeoutMSecs);
    _mavCommandResponseCheckTimer.start();
    connect(&_mavCommandResponseCheckTimer, &QTimer::timeout, this, &Vehicle::_sendMavCommandResponseTimeoutCheck);

    // Chunked status text timeout timer
    _chunkedStatusTextTimer.setSingleShot(true);
    _chunkedStatusTextTimer.setInterval(1000);
    connect(&_chunkedStatusTextTimer, &QTimer::timeout, this, &Vehicle::_chunkedStatusTextTimeout);

    _mav = uas();

    // Listen for system messages
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageCountChanged,  this, &Vehicle::_handleTextMessage);
    connect(_toolbox->uasMessageHandler(), &UASMessageHandler::textMessageReceived,      this, &Vehicle::_handletextMessageReceived);

    // MAV_TYPE_GENERIC is used by unit test for creating a vehicle which doesn't do the connect sequence. This
    // way we can test the methods that are used within the connect sequence.
    if (!qgcApp()->runningUnitTests() || _vehicleType != MAV_TYPE_GENERIC) {
        _initialConnectStateMachine->start();
    }

    _firmwarePlugin->initializeVehicle(this);
    for(auto& factName: factNames()) {
        _firmwarePlugin->adjustMetaData(vehicleType, getFact(factName)->metaData());
    }

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    connect(&_orbitTelemetryTimer, &QTimer::timeout, this, &Vehicle::_orbitTelemetryTimeout);

    // Create camera manager instance
    _cameraManager = _firmwarePlugin->createCameraManager(this);
    emit cameraManagerChanged();

    // Start csv logger
    connect(&_csvLogTimer, &QTimer::timeout, this, &Vehicle::_writeCsvLine);
    _csvLogTimer.start(1000);

    // Start timer to limit altitude above terrain queries
    _altitudeAboveTerrQueryTimer.restart();
}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : FactGroup                         (_vehicleUIUpdateRateMSecs, ":/json/Vehicle/VehicleFact.json", parent)
    , _id                               (0)
    , _defaultComponentId               (MAV_COMP_ID_ALL)
    , _offlineEditingVehicle            (true)
    , _firmwareType                     (firmwareType)
    , _vehicleType                      (vehicleType)
    , _toolbox                          (qgcApp()->toolbox())
    , _settingsManager                  (_toolbox->settingsManager())
    , _defaultCruiseSpeed               (_settingsManager->appSettings()->offlineEditingCruiseSpeed()->rawValue().toDouble())
    , _defaultHoverSpeed                (_settingsManager->appSettings()->offlineEditingHoverSpeed()->rawValue().toDouble())
    , _mavlinkProtocolRequestComplete   (true)
    , _maxProtoVersion                  (200)
    , _capabilityBitsKnown              (true)
    , _capabilityBits                   (MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY)
    , _firmwarePluginManager            (firmwarePluginManager)
    , _trajectoryPoints                 (new TrajectoryPoints(this, this))
    , _mavlinkStreamConfig              (std::bind(&Vehicle::_setMessageInterval, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    , _rollFact                         (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact                        (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact                      (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _rollRateFact                     (0, _rollRateFactName,          FactMetaData::valueTypeDouble)
    , _pitchRateFact                    (0, _pitchRateFactName,         FactMetaData::valueTypeDouble)
    , _yawRateFact                      (0, _yawRateFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact                  (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact                     (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _airSpeedSetpointFact             (0, _airSpeedSetpointFactName,  FactMetaData::valueTypeDouble)
    , _climbRateFact                    (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact             (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact                 (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _altitudeAboveTerrFact            (0, _altitudeAboveTerrFactName, FactMetaData::valueTypeDouble)
    , _altitudeTuningFact               (0, _altitudeTuningFactName,    FactMetaData::valueTypeDouble)
    , _altitudeTuningSetpointFact       (0, _altitudeTuningSetpointFactName, FactMetaData::valueTypeDouble)
    , _xTrackErrorFact                  (0, _xTrackErrorFactName,       FactMetaData::valueTypeDouble)
    , _rangeFinderDistFact              (0, _rangeFinderDistFactName,   FactMetaData::valueTypeFloat)
    , _flightDistanceFact               (0, _flightDistanceFactName,    FactMetaData::valueTypeDouble)
    , _flightTimeFact                   (0, _flightTimeFactName,        FactMetaData::valueTypeElapsedTimeInSeconds)
    , _distanceToHomeFact               (0, _distanceToHomeFactName,    FactMetaData::valueTypeDouble)
    , _missionItemIndexFact             (0, _missionItemIndexFactName,  FactMetaData::valueTypeUint16)
    , _headingToNextWPFact              (0, _headingToNextWPFactName,   FactMetaData::valueTypeDouble)
    , _headingToHomeFact                (0, _headingToHomeFactName,     FactMetaData::valueTypeDouble)
    , _distanceToGCSFact                (0, _distanceToGCSFactName,     FactMetaData::valueTypeDouble)
    , _hobbsFact                        (0, _hobbsFactName,             FactMetaData::valueTypeString)
    , _throttlePctFact                  (0, _throttlePctFactName,       FactMetaData::valueTypeUint16)
    , _imuTempFact                      (0, _imuTempFactName,           FactMetaData::valueTypeInt16)
    , _gpsFactGroup                     (this)
    , _gps2FactGroup                    (this)
    , _windFactGroup                    (this)
    , _vibrationFactGroup               (this)
    , _clockFactGroup                   (this)
    , _distanceSensorFactGroup          (this)
    , _localPositionFactGroup           (this)
    , _localPositionSetpointFactGroup   (this)
{
    _linkManager = _toolbox->linkManager();

    // This will also set the settings based firmware/vehicle types. So it needs to happen first.
    if (_firmwareType == MAV_AUTOPILOT_TRACK) {
        trackFirmwareVehicleTypeChanges();
    }

    _commonInit();

    connect(_settingsManager->appSettings()->offlineEditingCruiseSpeed(),   &Fact::rawValueChanged, this, &Vehicle::_offlineCruiseSpeedSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingHoverSpeed(),    &Fact::rawValueChanged, this, &Vehicle::_offlineHoverSpeedSettingChanged);

    _offlineFirmwareTypeSettingChanged(_firmwareType);  // This adds correct terrain capability bit
    _firmwarePlugin->initializeVehicle(this);
}

void Vehicle::trackFirmwareVehicleTypeChanges(void)
{
    connect(_settingsManager->appSettings()->offlineEditingFirmwareClass(), &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    connect(_settingsManager->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);

    _offlineFirmwareTypeSettingChanged(_settingsManager->appSettings()->offlineEditingFirmwareClass()->rawValue());
    _offlineVehicleTypeSettingChanged(_settingsManager->appSettings()->offlineEditingVehicleClass()->rawValue());
}

void Vehicle::stopTrackingFirmwareVehicleTypeChanges(void)
{
    disconnect(_settingsManager->appSettings()->offlineEditingFirmwareClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineFirmwareTypeSettingChanged);
    disconnect(_settingsManager->appSettings()->offlineEditingVehicleClass(),  &Fact::rawValueChanged, this, &Vehicle::_offlineVehicleTypeSettingChanged);
}

void Vehicle::_commonInit()
{
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    _messageIntervalSourcePlugin = _firmwarePlugin;
    _messageCommandSourceCore = _toolbox->corePlugin();
    _messageCommandSourceTerrain = _terrainProtocolHandler;

    connect(_firmwarePlugin, &FirmwarePlugin::toolIndicatorsChanged, this, &Vehicle::toolIndicatorsChanged);
    connect(_firmwarePlugin, &FirmwarePlugin::modeIndicatorsChanged, this, &Vehicle::modeIndicatorsChanged);

    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceHeadingToHome);
    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateDistanceToGCS);
    connect(this, &Vehicle::homePositionChanged,    this, &Vehicle::_updateDistanceHeadingToHome);
    connect(this, &Vehicle::hobbsMeterChanged,      this, &Vehicle::_updateHobbsMeter);
    connect(this, &Vehicle::coordinateChanged,      this, &Vehicle::_updateAltAboveTerrain);
    // Initialize alt above terrain to Nan so frontend can display it correctly in case the terrain query had no response
    _altitudeAboveTerrFact.setRawValue(qQNaN());

    connect(_toolbox->qgcPositionManager(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateDistanceToGCS);
    connect(_toolbox->qgcPositionManager(), &QGCPositionManager::gcsPositionChanged, this, &Vehicle::_updateHomepoint);

    _missionManager = new MissionManager(this);
    _fleetIntervalQueryMission = _missionManager;
    connect(_missionManager, &MissionManager::error,                    this, &Vehicle::_missionManagerError);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateHeadingToNextWP);
    connect(_missionManager, &MissionManager::currentIndexChanged,      this, &Vehicle::_updateMissionItemIndex);


    _standardModes                  = new StandardModes                 (this, this);
    _componentInformationManager    = new ComponentInformationManager   (this);
    _initialConnectStateMachine     = new InitialConnectStateMachine    (this);
    _fleetIntervalQueryInitialConnect = _initialConnectStateMachine;
    _ftpManager                     = new FTPManager                    (this);
    _imageProtocolManager           = new ImageProtocolManager          ();
    _messageCommandSourceFtp = _ftpManager;
    _messageCommandSourceImage = _imageProtocolManager;
    _vehicleLinkManager             = new VehicleLinkManager            (this);
    // Pin the original QObject lifetimes once, before any execution source
    // probe. A later entry must not reconstruct QPointers from stale raw
    // toolbox/manager/protocol fields after a nested destruction callback.
    _fleetExecutionSourceLinks = _vehicleLinkManager;
    _fleetExecutionSourceProtocol = _mavlink;
    _fleetExecutionSourceVehicles = _toolbox->multiVehicleManager();
    _fleetExecutionSourceToolbox = _toolbox;
    _initializeAutopilotVersionObservation();

    connect(_standardModes, &StandardModes::modesUpdated, this, &Vehicle::flightModesChanged);
    connect(_standardModes, &StandardModes::modesUpdated, this, [this](){ Vehicle::flightModeChanged(flightMode()); });

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager.data(), &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);

    connect(_initialConnectStateMachine, &InitialConnectStateMachine::progressUpdate,
            this, &Vehicle::_gotProgressUpdate);
    connect(_parameterManager.data(), &ParameterManager::loadProgressChanged, this, &Vehicle::_gotProgressUpdate);

    _objectAvoidance = new VehicleObjectAvoidance(this, this);

    _autotune = _firmwarePlugin->createAutotune(this);

    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = new GeoFenceManager(this);
    _fleetIntervalQueryFence = _geoFenceManager;
    connect(_geoFenceManager, &GeoFenceManager::error,          this, &Vehicle::_geoFenceManagerError);

    _rallyPointManager = new RallyPointManager(this);
    _fleetIntervalQueryRally = _rallyPointManager;
    connect(_rallyPointManager, &RallyPointManager::error,          this, &Vehicle::_rallyPointManagerError);

    connect(_missionManager, &PlanManager::operationStarted, this, [this](quint64 operationId, int transactionType) {
        if (_initialConnectStateMachine && _initialConnectStateMachine->active()
                && transactionType == PlanManager::TransactionRead) {
            _initialMissionLoadOperationId = operationId;
        }
    });
    connect(_geoFenceManager, &PlanManager::operationStarted, this, [this](quint64 operationId, int transactionType) {
        if (_initialConnectStateMachine && _initialConnectStateMachine->active()
                && transactionType == PlanManager::TransactionRead) {
            _initialGeoFenceLoadOperationId = operationId;
        }
    });
    connect(_rallyPointManager, &PlanManager::operationStarted, this, [this](quint64 operationId, int transactionType) {
        if (_initialConnectStateMachine && _initialConnectStateMachine->active()
                && transactionType == PlanManager::TransactionRead) {
            _initialRallyPointLoadOperationId = operationId;
        }
    });
    connect(_missionManager, &MissionManager::newMissionItemsAvailableWithOperationId,
            this, &Vehicle::_firstMissionLoadCompleteWithOperationId);
    connect(_missionManager, &MissionManager::newMissionItemsAvailableWithOperationId,
            this, &Vehicle::_clearCameraTriggerPointsWithOperationId);
    connect(_missionManager, &MissionManager::sendCompleteWithOperationId,
            this, &Vehicle::_clearCameraTriggerPointsWithOperationId);
    connect(_missionManager, &MissionManager::newMissionItemsAvailableWithOperationId,
            this, &Vehicle::_clearTrajectoryPointsWithOperationId);
    connect(_missionManager, &MissionManager::sendCompleteWithOperationId,
            this, &Vehicle::_clearTrajectoryPointsWithOperationId);
    connect(_geoFenceManager, &PlanManager::newMissionItemsAvailableWithOperationId,
            this, &Vehicle::_firstGeoFenceLoadCompleteWithOperationId);
    connect(_rallyPointManager, &PlanManager::newMissionItemsAvailableWithOperationId,
            this, &Vehicle::_firstRallyPointLoadCompleteWithOperationId);

    // Remote ID manager might want to acces parameters so make sure to create it after
    _remoteIDManager = new RemoteIDManager(this);
    _messageCommandSourceRemoteId = _remoteIDManager;

    // Flight modes can differ based on advanced mode
    connect(_toolbox->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &Vehicle::flightModesChanged);

    connect(_imageProtocolManager, &ImageProtocolManager::imageReady, this, &Vehicle::_imageProtocolImageReady);

    // Build FactGroup object model

    _addFact(&_rollFact,                _rollFactName);
    _addFact(&_pitchFact,               _pitchFactName);
    _addFact(&_headingFact,             _headingFactName);
    _addFact(&_rollRateFact,            _rollRateFactName);
    _addFact(&_pitchRateFact,           _pitchRateFactName);
    _addFact(&_yawRateFact,             _yawRateFactName);
    _addFact(&_groundSpeedFact,         _groundSpeedFactName);
    _addFact(&_airSpeedFact,            _airSpeedFactName);
    _addFact(&_airSpeedSetpointFact,    _airSpeedSetpointFactName);
    _addFact(&_climbRateFact,           _climbRateFactName);
    _addFact(&_altitudeRelativeFact,    _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,        _altitudeAMSLFactName);
    _addFact(&_altitudeAboveTerrFact,   _altitudeAboveTerrFactName);
    _addFact(&_altitudeTuningFact,       _altitudeTuningFactName);
    _addFact(&_altitudeTuningSetpointFact, _altitudeTuningSetpointFactName);
    _addFact(&_xTrackErrorFact,         _xTrackErrorFactName);
    _addFact(&_rangeFinderDistFact,     _rangeFinderDistFactName);
    _addFact(&_flightDistanceFact,      _flightDistanceFactName);
    _addFact(&_flightTimeFact,          _flightTimeFactName);
    _addFact(&_distanceToHomeFact,      _distanceToHomeFactName);
    _addFact(&_timeToHomeFact,          _timeToHomeFactName);
    _addFact(&_missionItemIndexFact,    _missionItemIndexFactName);
    _addFact(&_headingToNextWPFact,     _headingToNextWPFactName);
    _addFact(&_headingToHomeFact,       _headingToHomeFactName);
    _addFact(&_distanceToGCSFact,       _distanceToGCSFactName);
    _addFact(&_throttlePctFact,         _throttlePctFactName);
    _addFact(&_imuTempFact,             _imuTempFactName);

    _hobbsFact.setRawValue(QVariant(QString("0000:00:00")));
    _addFact(&_hobbsFact,               _hobbsFactName);

    _addFactGroup(&_gpsFactGroup,               _gpsFactGroupName);
    _addFactGroup(&_gps2FactGroup,              _gps2FactGroupName);
    _addFactGroup(&_windFactGroup,              _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup,         _vibrationFactGroupName);
    _addFactGroup(&_temperatureFactGroup,       _temperatureFactGroupName);
    _addFactGroup(&_clockFactGroup,             _clockFactGroupName);
    _addFactGroup(&_setpointFactGroup,          _setpointFactGroupName);
    _addFactGroup(&_distanceSensorFactGroup,    _distanceSensorFactGroupName);
    _addFactGroup(&_localPositionFactGroup,     _localPositionFactGroupName);
    _addFactGroup(&_localPositionSetpointFactGroup,_localPositionSetpointFactGroupName);
    _addFactGroup(&_escStatusFactGroup,         _escStatusFactGroupName);
    _addFactGroup(&_ftcStatusFactGroup,         _ftcStatusFactGroupName);
    _addFactGroup(&_estimatorStatusFactGroup,   _estimatorStatusFactGroupName);
    _addFactGroup(&_hygrometerFactGroup,        _hygrometerFactGroupName);
    _addFactGroup(&_generatorFactGroup,         _generatorFactGroupName);
    _addFactGroup(&_efiFactGroup,               _efiFactGroupName);
    _addFactGroup(&_terrainFactGroup,           _terrainFactGroupName);

    // Add firmware-specific fact groups, if provided
    QMap<QString, FactGroup*>* fwFactGroups = _firmwarePlugin->factGroups();
    if (fwFactGroups) {
        QMapIterator<QString, FactGroup*> i(*fwFactGroups);
        while(i.hasNext()) {
            i.next();
            _addFactGroup(i.value(), i.key());
        }
    }

    _flightDistanceFact.setRawValue(0);
    _flightTimeFact.setRawValue(0);
    _flightTimeUpdater.setInterval(1000);
    _flightTimeUpdater.setSingleShot(false);
    connect(&_flightTimeUpdater, &QTimer::timeout, this, &Vehicle::_updateFlightTime);

    // Set video stream to udp if running ArduSub and Video is disabled
    if (sub() && _settingsManager->videoSettings()->videoSource()->rawValue() == VideoSettings::videoDisabled) {
        _settingsManager->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
        _settingsManager->videoSettings()->lowLatencyMode()->setRawValue(true);
    }

    // enable Joystick if appropriate
    _loadJoystickSettings();
}

Vehicle::~Vehicle()
{
    _componentInformationCallbacksRetired = true;
    if (_fleetCameraStreamControlRun) {
        _fleetCameraStreamControlRun->sourceRetired = true;
        _finishFleetCameraStreamControl(_fleetCameraStreamControlRun, QStringLiteral("failed"), QStringLiteral("camera-control-vehicle-destroyed"), false);
    }
    if (_fleetCameraStreamRun) {
        _fleetCameraStreamRun->sourceRetired = true;
        _finishFleetCameraStreamCapture(_fleetCameraStreamRun, QStringLiteral("failed"), QStringLiteral("camera-capture-vehicle-destroyed"), false);
    }
    _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-vehicle-destroying"));
    _fleetCameraStreamRemoving = true;
    _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-vehicle-destroying"));
    _fleetLocalReceptionRemoving = true;
    _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-vehicle-destroying"));
    _autopilotVersionObservationRemoving = true;
    _messageCommandLaneRetired = true;
    _mavCommandResponseCheckTimer.stop();
    if (_fleetOptionalTelemetryConfigurationRun) _fleetOptionalTelemetryConfigurationRun->active = false;
    _fleetOptionalTelemetryConfigurationRun.reset();
    if (_requestMessageContext) _requestMessageContext->resultPublished = true;
    // No user callbacks from a partially destroyed Vehicle. Remove the raw
    // handler slots before dropping the context that owns their data.
    for (int index = _mavCommandList.size() - 1; index >= 0; --index)
        if (messageCommand(_mavCommandList.at(index).command)) _mavCommandList.removeAt(index);
    _waitForMavlinkMessageResultHandler = nullptr; _waitForMavlinkMessageResultHandlerData = nullptr;
    _waitForMavlinkMessageId = 0; _waitForMavlinkMessageTimeoutActive = false;
    _requestMessageContext.reset(); _messageCommandLaneId.clear();
    _fleetMessageIntervalActiveQuery.reset();
    _fleetMessageIntervalQueries.clear();
    _fleetBatteryTelemetryRemoving = true;
    _invalidateFleetBatteryTelemetryObservation();
    _fleetHealthTelemetryRemoving = true;
    _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-vehicle-destroying"));
    _fleetExecutionRemoving = true;
    // No virtual callbacks from partial destruction. Execution observers also
    // watch QObject::destroyed and retain an unknown source tail independently.
    _fleetExecutionObserver.clear(); _fleetExecutionLink.clear(); _fleetExecutionPrimaryOwner.reset();
    for (const QMetaObject::Connection& connection : _fleetExecutionConnections) disconnect(connection);
    _fleetExecutionConnections.clear(); _fleetExecutionClock.invalidate();
    _fleetTelemetryRemoving = true;
    // No virtual callback from partial Vehicle destruction. The collector's
    // QObject destruction observer independently records an unknown tail.
    _fleetTelemetryObserver.clear(); _fleetTelemetryLink.clear();
    for (const QMetaObject::Connection& connection : _fleetTelemetryConnections) disconnect(connection);
    _fleetTelemetryConnections.clear();
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = nullptr;

    delete _autopilotPlugin;
    _autopilotPlugin = nullptr;

    delete _mav;
    _mav = nullptr;
}

void Vehicle::closeVehicle(void)
{
    _componentInformationCallbacksRetired = true;
    if (_fleetCameraStreamControlRun) {
        _fleetCameraStreamControlRun->sourceRetired = true;
        _finishFleetCameraStreamControl(_fleetCameraStreamControlRun, QStringLiteral("failed"), QStringLiteral("camera-control-vehicle-closing"), true);
    }
    const QPointer<Vehicle> telemetryOwner(this);
    if (_fleetCameraStreamRun) {
        _fleetCameraStreamRun->sourceRetired = true;
        _finishFleetCameraStreamCapture(_fleetCameraStreamRun, QStringLiteral("failed"), QStringLiteral("camera-capture-vehicle-closing"), true);
    }
    _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-vehicle-closing"));
    _fleetCameraStreamRemoving = true;
    _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-vehicle-closing"));
    _fleetLocalReceptionRemoving = true;
    _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-vehicle-closing"));
    const auto configurationRun = _fleetOptionalTelemetryConfigurationRun;
    if (configurationRun) _finishFleetOptionalTelemetryConfiguration(configurationRun, QStringLiteral("failed"),
        QStringLiteral("configuration-vehicle-closing-no-rollback"), true);
    if (!telemetryOwner) return;
    if (_fleetMessageIntervalActiveQuery) _finishFleetMessageIntervalQuery(_fleetMessageIntervalActiveQuery,
        QStringLiteral("failed"), QStringLiteral("fleet-interval-query-vehicle-closing"), true);
    _fleetBatteryTelemetryRemoving = true;
    _invalidateFleetBatteryTelemetryObservation();
    _fleetHealthTelemetryRemoving = true;
    _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-vehicle-closing"));
    _fleetExecutionRemoving = true;
    _fleetTelemetryRemoving = true;
    _retireFleetExecutionObserver(QStringLiteral("execution-vehicle-closing"));
    if (!telemetryOwner) return;
    _retireFleetTelemetryObserver(QStringLiteral("telemetry-vehicle-closing"));
    if (!telemetryOwner) return;
    _autopilotVersionObservationRemoving = true;
    _invalidateAutopilotVersionObservation(QStringLiteral("vehicle-closing"));
    if (!telemetryOwner) return;
    // Stop local dispatchers before closing the link so a timer callback cannot
    // enqueue or retransmit a command against a vehicle that is being removed.
    _mavCommandResponseCheckTimer.stop();
    _sendMultipleTimer.stop();

    if (_vehicleLinkManager) {
        _vehicleLinkManager->closeVehicle();
        if (!telemetryOwner) return;
    }

    _sendMessageMultipleList.clear();
    _nextSendMessageMultipleIndex = 0;

    // Complete all command handlers locally. A vehicle removal must not leave
    // callbacks waiting for an ACK that can never arrive, and must not let a
    // later vehicle reuse the old handler state.
    const QList<MavCommandListEntry_t> pendingCommands = _mavCommandList;
    const auto pendingRequest = _requestMessageContext;
    _mavCommandList.clear();
    for (const MavCommandListEntry_t& commandEntry: pendingCommands) {
        mavlink_command_ack_t ack = {};
        ack.result = MAV_RESULT_FAILED;
        if (messageCommand(commandEntry.command) && (!pendingRequest
            || commandEntry.ackHandlerInfo.resultHandlerData != pendingRequest.get()))
            _releaseMessageCommand(commandEntry.messageIntervalRequestId);
        if (commandEntry.ackHandlerInfo.resultHandler) {
            (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData,
                                                         commandEntry.targetCompId,
                                                         ack,
                                                         MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id,
                                  commandEntry.targetCompId,
                                  commandEntry.command,
                                  MAV_RESULT_FAILED,
                                  MavCmdResultFailureNoResponseToCommand);
        }
        if (!telemetryOwner) return;
    }

    // requestMessage() uses the private waiter below. Clear it before the
    // callback so re-entrant callers cannot inherit stale request identity.
    if (_waitForMavlinkMessageResultHandler) {
        WaitForMavlinkMessageResultHandler resultHandler = _waitForMavlinkMessageResultHandler;
        void* resultHandlerData = _waitForMavlinkMessageResultHandlerData;
        _waitForMavlinkMessageClear();
        mavlink_message_t emptyMessage = {};
        (*resultHandler)(resultHandlerData, true /* noResponseFromVehicle */, emptyMessage);
    }
}

void Vehicle::prepareDelete()
{
    _componentInformationCallbacksRetired = true;
    if (_fleetCameraStreamControlRun) {
        _fleetCameraStreamControlRun->sourceRetired = true;
        _finishFleetCameraStreamControl(_fleetCameraStreamControlRun, QStringLiteral("failed"), QStringLiteral("camera-control-vehicle-removing"), true);
    }
    const QPointer<Vehicle> telemetryOwner(this);
    if (_fleetCameraStreamRun) {
        _fleetCameraStreamRun->sourceRetired = true;
        _finishFleetCameraStreamCapture(_fleetCameraStreamRun, QStringLiteral("failed"), QStringLiteral("camera-capture-vehicle-removing"), true);
    }
    _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-vehicle-removing"));
    _fleetCameraStreamRemoving = true;
    _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-vehicle-removing"));
    _fleetLocalReceptionRemoving = true;
    _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-vehicle-removing"));
    const auto configurationRun = _fleetOptionalTelemetryConfigurationRun;
    if (configurationRun) _finishFleetOptionalTelemetryConfiguration(configurationRun, QStringLiteral("failed"),
        QStringLiteral("configuration-vehicle-removing-no-rollback"), true);
    if (!telemetryOwner) return;
    if (_fleetMessageIntervalActiveQuery) _finishFleetMessageIntervalQuery(_fleetMessageIntervalActiveQuery,
        QStringLiteral("failed"), QStringLiteral("fleet-interval-query-vehicle-removing"), true);
    _messageCommandLaneRetired = true;
    _fleetBatteryTelemetryRemoving = true;
    _invalidateFleetBatteryTelemetryObservation();
    _fleetHealthTelemetryRemoving = true;
    _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-vehicle-removing"));
    _fleetExecutionRemoving = true;
    _fleetTelemetryRemoving = true;
    _retireFleetExecutionObserver(QStringLiteral("execution-vehicle-removing"));
    if (!telemetryOwner) return;
    _retireFleetTelemetryObserver(QStringLiteral("telemetry-vehicle-removing"));
    if (!telemetryOwner) return;
    _autopilotVersionObservationRemoving = true;
    _invalidateAutopilotVersionObservation(QStringLiteral("vehicle-removing"));
    if (!telemetryOwner) return;
    if(_cameraManager) {
        // because of _cameraManager QML bindings check for nullptr won't work in the binding pipeline
        // the dangling pointer access will cause a runtime fault
        auto tmpCameras = _cameraManager;
        _cameraManager = nullptr;
        delete tmpCameras;
        if (!telemetryOwner) return;
        emit cameraManagerChanged();
        if (!telemetryOwner) return;
        qApp->processEvents();
    }
}

void Vehicle::_offlineFirmwareTypeSettingChanged(QVariant varFirmwareType)
{
    _firmwareType = static_cast<MAV_AUTOPILOT>(varFirmwareType.toInt());
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    if (_firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        _capabilityBits |= MAV_PROTOCOL_CAPABILITY_TERRAIN;
    } else {
        _capabilityBits &= ~MAV_PROTOCOL_CAPABILITY_TERRAIN;
    }
    emit firmwareTypeChanged();
    emit capabilityBitsChanged(_capabilityBits);
}

void Vehicle::_offlineVehicleTypeSettingChanged(QVariant varVehicleType)
{
    _vehicleType = static_cast<MAV_TYPE>(varVehicleType.toInt());
    emit vehicleTypeChanged();
}

void Vehicle::_offlineCruiseSpeedSettingChanged(QVariant value)
{
    _defaultCruiseSpeed = value.toDouble();
    emit defaultCruiseSpeedChanged(_defaultCruiseSpeed);
}

void Vehicle::_offlineHoverSpeedSettingChanged(QVariant value)
{
    _defaultHoverSpeed = value.toDouble();
    emit defaultHoverSpeedChanged(_defaultHoverSpeed);
}

QString Vehicle::firmwareTypeString() const
{
    if (px4Firmware()) {
        return QStringLiteral("PX4 Pro");
    } else if (apmFirmware()) {
        return QStringLiteral("ArduPilot");
    } else {
        return tr("MAVLink Generic");
    }
}

QString Vehicle::vehicleTypeString() const
{
    if (airship()) {
        return tr("Airship");
    } else if (fixedWing()) {
        return tr("Fixed Wing");
    } else if (multiRotor()) {
        return tr("Multi-Rotor");
    } else if (vtol()) {
        return tr("VTOL");
    } else if (rover()) {
        return tr("Rover");
    } else if (sub()) {
        return tr("Sub");
    } else {
        return tr("Unknown");
    }
}

void Vehicle::resetCounters()
{
    _messagesReceived   = 0;
    _messagesSent       = 0;
    _messagesLost       = 0;
    _messageSeq         = 0;
    _heardFrom          = false;
}

void Vehicle::_initializeAutopilotVersionObservation()
{
    _autopilotVersionObservationClock.start();
    _initializeFleetHealthTelemetryObservation();
    _initializeFleetBatteryTelemetryObservation();
    _autopilotVersionObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _messageCommandTransportEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _initializeOptionalTelemetryObservations();
    _initializeFleetLocalReceptionObservation();
    _initializeFleetCameraStreamObservation();
    _flightStateObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupOdometryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _initializeStartupGlobalOriginObservation();
    _initializeStartupAltitudeObservation();
    for (const QString& channel : flightObservationChannels()) {
        _flightStateObservations.insert(channel, FlightStateObservationChannel());
    }
    connect(this, &Vehicle::autopilotVersionObservationSourceChanged, this, [this]() {
        _invalidateFleetBatteryTelemetryObservation();
        _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-version-source-change"));
        _invalidateFlightStateObservations(QStringLiteral("autopilot-version-source-change"));
    });
    connect(this, &Vehicle::firmwareTypeChanged, this, [this]() {
        _invalidateAutopilotVersionObservation(QStringLiteral("vehicle-model-change"));
    });
    connect(this, &Vehicle::vehicleTypeChanged, this, [this]() {
        _invalidateAutopilotVersionObservation(QStringLiteral("vehicle-model-change"));
    });
    connect(_vehicleLinkManager, &VehicleLinkManager::communicationLostChanged, this, [this](bool) {
        _invalidateAutopilotVersionObservation(QStringLiteral("link-state-change"));
    });
    connect(_vehicleLinkManager, &VehicleLinkManager::primaryLinkChanged, this, [this]() {
        _invalidateAutopilotVersionObservation(QStringLiteral("primary-link-change"));
    });
    connect(_vehicleLinkManager, &VehicleLinkManager::linkNamesChanged, this, [this]() {
        _invalidateAutopilotVersionObservation(QStringLiteral("link-membership-change"));
    });
    connect(_vehicleLinkManager, &VehicleLinkManager::linkStatusesChanged, this, [this]() {
        _invalidateAutopilotVersionObservation(QStringLiteral("link-status-change"));
    });
    connect(_vehicleLinkManager, &VehicleLinkManager::allLinksRemoved, this, [this](Vehicle*) {
        _autopilotVersionObservationRemoving = true;
        _invalidateAutopilotVersionObservation(QStringLiteral("links-removed"));
    });
    // Deliberately do not invalidate on normal initialConnectComplete: a real
    // version received during the same initial connection may remain fresh.
}

void Vehicle::_initializeStartupGlobalOriginObservation()
{
    _startupGlobalOriginClock.start();
    _startupGlobalOriginEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupGlobalOriginLinks = _vehicleLinkManager;
    _startupGlobalOriginProtocol = _mavlink;
    _startupGlobalOriginVehicles = qgcApp() && qgcApp()->toolbox() ? qgcApp()->toolbox()->multiVehicleManager() : nullptr;
    const auto removed = [this]() {
        if (QThread::currentThread() != thread()) return;
        _startupGlobalOriginRemoving = true;
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-native-source-removed"));
    };
    if (_startupGlobalOriginProtocol) connect(_startupGlobalOriginProtocol.data(), &QObject::destroyed, this, removed);
    if (_startupGlobalOriginLinks) connect(_startupGlobalOriginLinks.data(), &QObject::destroyed, this, removed);
    if (_startupGlobalOriginVehicles) {
        connect(_startupGlobalOriginVehicles.data(), &MultiVehicleManager::vehicleRemoved, this, [this, removed](Vehicle* vehicle) {
            if (QThread::currentThread() == thread() && vehicle == this) removed();
        });
        connect(_startupGlobalOriginVehicles.data(), &QObject::destroyed, this, removed);
    }
}

void Vehicle::_invalidateStartupGlobalOriginObservation(const QString& source)
{
    if (QThread::currentThread() != thread()) return;
    _startupGlobalOriginEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupGlobalOriginLink.clear(); _startupGlobalOriginWeakLink.reset();
    _startupGlobalOriginVersionEpoch.clear(); _startupGlobalOriginFlightEpoch.clear();
    _startupGlobalOriginComponentId = 0; _startupGlobalOriginFirmwareType = -1; _startupGlobalOriginVehicleType = -1;
    _startupGlobalOriginRow = StartupGlobalOriginRow();
    _startupGlobalOriginLastPreparedReferenceHash.clear();
    _startupGlobalOriginSource = _startupGlobalOriginEverObservedRawFrame ? source : QStringLiteral("unknown");
    if (_startupGlobalOriginEverObservedRawFrame) {
        if (_startupGlobalOriginReferenceRevision == std::numeric_limits<quint64>::max()) _startupGlobalOriginExhausted = true;
        else ++_startupGlobalOriginReferenceRevision;
    }
    // Sticky ever-seen and non-wrapping counters survive loss. In particular,
    // a nested invalidation must not release another receive stack's fence.
    // Invalidation stays callback-free. Old, epoch-scoped link hooks cannot
    // mutate this new epoch; release their bounded pair only on guarded bind.
}

bool Vehicle::_startupGlobalOriginMemoryCurrent(LinkInterface* expectedPrimary, bool requireQuiescent) const
{
    // Callback-free local continuity only; ingress separately checks real link
    // virtuals. This does not establish current remote reachability or trust.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !_startupGlobalOriginClock.isValid() || !QElapsedTimer::isMonotonic()
        || (requireQuiescent && _startupGlobalOriginReceiveDepth != 0)
        || _startupGlobalOriginRemoving || _startupGlobalOriginExhausted || _offlineEditingVehicle
        || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted || _flightStateObservationExhausted
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255 || !expectedPrimary
        || !_startupGlobalOriginLinks || !_startupGlobalOriginProtocol || !_startupGlobalOriginVehicles
        || _startupGlobalOriginLinks->thread() != thread() || _startupGlobalOriginProtocol->thread() != thread()
        || _startupGlobalOriginVehicles->thread() != thread()
        || _startupGlobalOriginLinks.data() != _vehicleLinkManager || _startupGlobalOriginProtocol.data() != _mavlink
        || qgcApp()->toolbox()->mavlinkProtocol() != _startupGlobalOriginProtocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != _startupGlobalOriginVehicles.data()
        || _startupGlobalOriginVehicles->getVehicleById(_id) != this || _startupGlobalOriginLinks->communicationLost()
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || _autopilotVersionObservationLink.data() != expectedPrimary || _flightStateObservationLink.data() != expectedPrimary) return false;
    const WeakLinkInterfacePtr primary = _startupGlobalOriginLinks->primaryLink();
    if (primary.expired()) return false;
    if (_startupGlobalOriginEverObservedRawFrame
        && (_startupGlobalOriginLink.data() != expectedPrimary || _startupGlobalOriginWeakLink.expired()
            || primary.owner_before(_startupGlobalOriginWeakLink) || _startupGlobalOriginWeakLink.owner_before(primary)
            || _startupGlobalOriginVersionEpoch != _autopilotVersionObservationEpoch
            || _startupGlobalOriginFlightEpoch != _flightStateObservationEpoch
            || _startupGlobalOriginComponentId != _defaultComponentId
            || _startupGlobalOriginFirmwareType != int(_firmwareType) || _startupGlobalOriginVehicleType != int(_vehicleType))) return false;
    for (const auto& entry : _startupGlobalOriginLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == expectedPrimary
            && !primary.owner_before(candidate) && !candidate.owner_before(primary)) return true;
    }
    return false;
}

bool Vehicle::_startupGlobalOriginSourceAllowed(LinkInterface* sourceLink) const
{
    if (!_startupGlobalOriginMemoryCurrent(sourceLink, false)) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(sourceLink);
    const QString epoch = _startupGlobalOriginEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch;
    const quint64 revision = _startupGlobalOriginReferenceRevision, receipt = _startupGlobalOriginNextReceipt;
    const auto same = [&]() {
        return self && link && self->_startupGlobalOriginEpoch == epoch
            && self->_autopilotVersionObservationEpoch == versionEpoch && self->_flightStateObservationEpoch == flightEpoch
            && self->_startupGlobalOriginReferenceRevision == revision && self->_startupGlobalOriginNextReceipt == receipt
            && self->_startupGlobalOriginMemoryCurrent(link.data(), false);
    };
    const bool connected = link->isConnected();
    if (!same() || !connected) return false;
    const bool mock = link->isMockLink();
    if (!same() || mock) return false;
    const bool replay = link->isLogReplay();
    return same() && !replay;
}

quint64 Vehicle::_prepareStartupGlobalOriginObservation(const mavlink_message_t& original, LinkInterface* sourceLink)
{
    if (original.msgid != MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN || QThread::currentThread() != thread()
        || !_mavlink || sender() != _mavlink || original.sysid != _id || original.compid != _defaultComponentId
        || !_startupGlobalOriginLinks || _startupGlobalOriginLinks->thread() != thread()) return 0;
    const QPointer<LinkInterface> source(sourceLink);
    const WeakLinkInterfacePtr primary = _startupGlobalOriginLinks->primaryLink();
    bool primaryFrame = false;
    for (const auto& entry : _startupGlobalOriginLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !primary.expired()
            && !primary.owner_before(candidate) && !candidate.owner_before(primary)) { primaryFrame = true; break; }
    }
    if (!primaryFrame) return 0; // A secondary link cannot replace PRIMARY evidence.
    _startupGlobalOriginEverObservedRawFrame = true;
    if (_startupGlobalOriginReceiveDepth != 1 || _startupGlobalOriginExhausted) {
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-nested-or-exhausted")); return 0;
    }
    if (_startupGlobalOriginLink && (_startupGlobalOriginLink.data() != sourceLink
        || _startupGlobalOriginVersionEpoch != _autopilotVersionObservationEpoch || _startupGlobalOriginFlightEpoch != _flightStateObservationEpoch
        || primary.owner_before(_startupGlobalOriginWeakLink) || _startupGlobalOriginWeakLink.owner_before(primary)
        || _startupGlobalOriginComponentId != _defaultComponentId || _startupGlobalOriginFirmwareType != int(_firmwareType)
        || _startupGlobalOriginVehicleType != int(_vehicleType)))
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-source-change"));
    bool decoded = false;
    const QVariantMap value = FleetStartupGlobalOrigin::decode(original, &decoded);
    const QVariantMap projection = decoded ? FleetStartupGlobalOrigin::referenceProjection(value) : QVariantMap();
    if (!decoded || projection.size() != 8) {
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-invalid-envelope")); return 0;
    }
    const QString hash = startupValueHash(projection);
    // Retire the old semantic reference BEFORE any link virtual/callback. The
    // full reference includes reported time_usec; A -> B -> A is not revival.
    if (hash != _startupGlobalOriginLastPreparedReferenceHash) {
        if (_startupGlobalOriginReferenceRevision == std::numeric_limits<quint64>::max()) {
            _startupGlobalOriginExhausted = true;
            _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-revision-exhausted")); return 0;
        }
        ++_startupGlobalOriginReferenceRevision;
        _startupGlobalOriginLastPreparedReferenceHash = hash;
    }
    if (_startupGlobalOriginNextReceipt == std::numeric_limits<quint64>::max()) {
        _startupGlobalOriginExhausted = true;
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-receipt-exhausted")); return 0;
    }
    const QPointer<Vehicle> self(this);
    if (!_startupGlobalOriginLink) {
        const QString bindingEpoch = _startupGlobalOriginEpoch;
        while (!_startupGlobalOriginLinkConnections.isEmpty()) {
            const auto connection = _startupGlobalOriginLinkConnections.takeFirst();
            QObject::disconnect(connection);
            if (!self) return 0;
            if (!source) { _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-link-removed")); return 0; }
            if (_startupGlobalOriginEpoch != bindingEpoch) return 0;
        }
        _startupGlobalOriginLink = sourceLink; _startupGlobalOriginWeakLink = primary;
        _startupGlobalOriginVersionEpoch = _autopilotVersionObservationEpoch;
        _startupGlobalOriginFlightEpoch = _flightStateObservationEpoch;
        _startupGlobalOriginComponentId = _defaultComponentId;
        _startupGlobalOriginFirmwareType = int(_firmwareType); _startupGlobalOriginVehicleType = int(_vehicleType);
        const QString epoch = _startupGlobalOriginEpoch;
        const auto lost = [this, epoch]() {
            if (QThread::currentThread() == thread() && _startupGlobalOriginEpoch == epoch)
                _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-link-removed"));
        };
        const auto disconnected = connect(source.data(), &LinkInterface::disconnected, this, lost);
        if (!self) return 0;
        if (!source) { _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-link-removed")); return 0; }
        if (_startupGlobalOriginEpoch != epoch) { QObject::disconnect(disconnected); return 0; }
        _startupGlobalOriginLinkConnections.append(disconnected);
        const auto destroyed = connect(source.data(), &QObject::destroyed, this, lost);
        if (!self) return 0;
        if (!source) { _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-link-removed")); return 0; }
        if (_startupGlobalOriginEpoch != epoch) { QObject::disconnect(destroyed); return 0; }
        _startupGlobalOriginLinkConnections.append(destroyed);
    }
    const quint64 token = ++_startupGlobalOriginNextReceipt;
    _startupGlobalOriginRow.updateToken = token; _startupGlobalOriginRow.updating = true;
    _startupGlobalOriginRow.complete = false;
    const bool allowed = _startupGlobalOriginSourceAllowed(source.data());
    if (!self) return 0;
    if (!allowed || _startupGlobalOriginRow.updateToken != token || !_startupGlobalOriginRow.updating) {
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-untrusted-source")); return 0;
    }
    return token;
}

void Vehicle::_completeStartupGlobalOriginObservation(quint64 token, const QString& epoch,
    const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (!token || _startupGlobalOriginEpoch != epoch || _startupGlobalOriginRow.updateToken != token
        || !_startupGlobalOriginRow.updating) return;
    const QPointer<Vehicle> self(this);
    const bool allowed = _startupGlobalOriginSourceAllowed(sourceLink);
    if (!self) return;
    if (_startupGlobalOriginEpoch != epoch || _startupGlobalOriginRow.updateToken != token || !_startupGlobalOriginRow.updating) return;
    bool decoded = false;
    const QVariantMap value = FleetStartupGlobalOrigin::decode(original, &decoded);
    const QVariantMap projection = decoded ? FleetStartupGlobalOrigin::referenceProjection(value) : QVariantMap();
    if (!allowed || _startupGlobalOriginReceiveDepth != 1 || receivedAtMs < 0
        || projection.size() != 8 || startupValueHash(projection) != _startupGlobalOriginLastPreparedReferenceHash) {
        _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-commit-unavailable")); return;
    }
    _startupGlobalOriginRow.value = value; _startupGlobalOriginRow.referenceProjection = projection;
    _startupGlobalOriginRow.referenceHash = _startupGlobalOriginLastPreparedReferenceHash;
    _startupGlobalOriginRow.rawPayloadHash = startupRawHash(original); _startupGlobalOriginRow.rawPayloadLength = int(original.len);
    _startupGlobalOriginRow.receiptSequence = token; _startupGlobalOriginRow.receivedAtMs = receivedAtMs;
    _startupGlobalOriginRow.observed = true; _startupGlobalOriginRow.complete = true; _startupGlobalOriginRow.updating = false;
    _startupGlobalOriginSource = QStringLiteral("live-gps-global-origin");
}

bool Vehicle::startupGlobalOriginObservationSourceMatches(LinkInterface* expectedPrimary) const
{
    return _startupGlobalOriginMemoryCurrent(expectedPrimary, true);
}

QVariantMap Vehicle::startupGlobalOriginObservation() const
{
    if (QThread::currentThread() != thread()) return {};
    const StartupGlobalOriginRow row = _startupGlobalOriginRow;
    const qint64 now = _startupGlobalOriginClock.isValid() ? _startupGlobalOriginClock.elapsed() : -1;
    const qint64 age = row.observed && row.receivedAtMs >= 0 && now >= row.receivedAtMs ? now - row.receivedAtMs : -1;
    const bool available = row.observed && row.complete && !row.updating && row.receiptSequence != 0 && age >= 0
        && row.referenceProjection.value(QStringLiteral("coordinatesInRange")).toBool()
        && _startupGlobalOriginMemoryCurrent(_startupGlobalOriginLink.data(), true);
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), _id}, {QStringLiteral("componentId"), _defaultComponentId},
        {QStringLiteral("sourceEpoch"), _autopilotVersionObservationEpoch}, {QStringLiteral("flightSourceEpoch"), _flightStateObservationEpoch},
        {QStringLiteral("originSourceEpoch"), _startupGlobalOriginEpoch}, {QStringLiteral("observed"), row.observed},
        {QStringLiteral("everObservedRawFrame"), _startupGlobalOriginEverObservedRawFrame}, {QStringLiteral("available"), available},
        {QStringLiteral("receiveStackQuiescent"), _startupGlobalOriginReceiveDepth == 0},
        {QStringLiteral("referenceRevision"), QString::number(_startupGlobalOriginReferenceRevision)},
        {QStringLiteral("referenceHash"), row.referenceHash.isEmpty() ? QStringLiteral("") : row.referenceHash},
        {QStringLiteral("referenceProjection"), row.referenceProjection}, {QStringLiteral("receiptSequence"), QString::number(row.receiptSequence)},
        {QStringLiteral("receivedAtMonotonicMs"), double(row.receivedAtMs)}, {QStringLiteral("ageMs"), double(age)},
        {QStringLiteral("source"), _startupGlobalOriginSource}, {QStringLiteral("value"), row.value},
        {QStringLiteral("rawPayloadHash"), row.rawPayloadHash.isEmpty() ? QStringLiteral("") : row.rawPayloadHash},
        {QStringLiteral("rawPayloadLength"), row.rawPayloadLength}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap Vehicle::_startupGlobalOriginStableReference() const
{
    QVariantMap reference = startupGlobalOriginObservation();
    for (const QString& key : {QStringLiteral("value"), QStringLiteral("receiptSequence"), QStringLiteral("receivedAtMonotonicMs"),
             QStringLiteral("ageMs"), QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength")}) reference.remove(key);
    return reference;
}

bool Vehicle::_fleetStartupGlobalOriginBasisCurrent() const
{
    if (_fleetStartupGlobalOriginBasis.size() != 18 || !_startupGlobalOriginMemoryCurrent(_fleetStartupLink.data(), true)) return false;
    const QVariantMap current = _startupGlobalOriginStableReference();
    if (current != _fleetStartupGlobalOriginBasis) return false;
    // Never seen is backward-compatible absence, not a valid origin proof.
    // Seen-but-lost, malformed, out-of-range or exhausted is never absence.
    return !_startupGlobalOriginEverObservedRawFrame
        ? (!_startupGlobalOriginRow.observed && _startupGlobalOriginReferenceRevision == 0)
        : current.value(QStringLiteral("available")).toBool();
}

void Vehicle::_initializeStartupAltitudeObservation()
{
    _startupAltitudeClock.start();
    _startupAltitudeEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupAltitudeLinks = _vehicleLinkManager;
    _startupAltitudeProtocol = _mavlink;
    _startupAltitudeVehicles = qgcApp() && qgcApp()->toolbox() ? qgcApp()->toolbox()->multiVehicleManager() : nullptr;
    const auto removed = [this]() {
        if (QThread::currentThread() != thread()) return;
        _startupAltitudeRemoving = true;
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-native-source-removed"), true);
    };
    if (_startupAltitudeProtocol) connect(_startupAltitudeProtocol.data(), &QObject::destroyed, this, removed);
    if (_startupAltitudeLinks) connect(_startupAltitudeLinks.data(), &QObject::destroyed, this, removed);
    if (_startupAltitudeVehicles) {
        connect(_startupAltitudeVehicles.data(), &MultiVehicleManager::vehicleRemoved, this, [this, removed](Vehicle* vehicle) {
            if (QThread::currentThread() == thread() && vehicle == this) removed();
        });
        connect(_startupAltitudeVehicles.data(), &QObject::destroyed, this, removed);
    }
}

void Vehicle::_invalidateStartupAltitudeObservation(const QString& source, bool sourceLost)
{
    if (QThread::currentThread() != thread()) return;
    _failFleetStartupAltitude(sourceLost ? QStringLiteral("altitude-monitor-source-retired")
        : QStringLiteral("altitude-monitor-invalid-or-discarded-frame"));
    _startupAltitudeRow = StartupAltitudeRow();
    _startupAltitudeLastPreparedStateHash.clear();
    _startupAltitudeSource = _startupAltitudeEverObservedRawFrame ? source : QStringLiteral("unknown");
    if (_startupAltitudeEverObservedRawFrame) {
        if (_startupAltitudeStateRevision == std::numeric_limits<quint64>::max()) _startupAltitudeExhausted = true;
        else ++_startupAltitudeStateRevision;
    }
    if (sourceLost) {
        _startupAltitudeEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
        _startupAltitudeLink.clear(); _startupAltitudeWeakLink.reset();
        _startupAltitudeLinkHooksReady = false;
        _startupAltitudeVersionEpoch.clear(); _startupAltitudeFlightEpoch.clear();
        _startupAltitudeComponentId = 0; _startupAltitudeFirmwareType = -1; _startupAltitudeVehicleType = -1;
        _startupAltitudeLastRawSampleTime = 0; _startupAltitudeRawSampleObserved = false;
        _startupAltitudeClockDiscontinuityObserved = false;
    }
    // A bad/discarded same-source frame must NOT erase the previous remote
    // timestamp or sticky regression history. Only actual source retirement
    // starts a new sequence; all revision/receipt counters remain monotonic.
    // Callback-free invalidation retains at most two inert old link hooks,
    // released individually on guarded rebind. Never reset held RX depth.
}

bool Vehicle::_startupAltitudeMemoryCurrent(LinkInterface* expectedPrimary, bool requireQuiescent) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !_startupAltitudeClock.isValid() || !QElapsedTimer::isMonotonic()
        || (requireQuiescent && _startupAltitudeReceiveDepth != 0)
        || _startupAltitudeRemoving || _startupAltitudeExhausted || _offlineEditingVehicle
        || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted || _flightStateObservationExhausted
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255 || !expectedPrimary
        || !_startupAltitudeLinks || !_startupAltitudeProtocol || !_startupAltitudeVehicles
        || _startupAltitudeLinks->thread() != thread() || _startupAltitudeProtocol->thread() != thread()
        || _startupAltitudeVehicles->thread() != thread()
        || _startupAltitudeLinks.data() != _vehicleLinkManager || _startupAltitudeProtocol.data() != _mavlink
        || qgcApp()->toolbox()->mavlinkProtocol() != _startupAltitudeProtocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != _startupAltitudeVehicles.data()
        || _startupAltitudeVehicles->getVehicleById(_id) != this || _startupAltitudeLinks->communicationLost()
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || _autopilotVersionObservationLink.data() != expectedPrimary || _flightStateObservationLink.data() != expectedPrimary) return false;
    const WeakLinkInterfacePtr primary = _startupAltitudeLinks->primaryLink();
    if (primary.expired()) return false;
    if (_startupAltitudeEverObservedRawFrame
        && (!_startupAltitudeLinkHooksReady || _startupAltitudeLink.data() != expectedPrimary || _startupAltitudeWeakLink.expired()
            || primary.owner_before(_startupAltitudeWeakLink) || _startupAltitudeWeakLink.owner_before(primary)
            || _startupAltitudeVersionEpoch != _autopilotVersionObservationEpoch || _startupAltitudeFlightEpoch != _flightStateObservationEpoch
            || _startupAltitudeComponentId != _defaultComponentId || _startupAltitudeFirmwareType != int(_firmwareType)
            || _startupAltitudeVehicleType != int(_vehicleType))) return false;
    for (const auto& entry : _startupAltitudeLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == expectedPrimary
            && !primary.owner_before(candidate) && !candidate.owner_before(primary)) return true;
    }
    return false;
}

bool Vehicle::_startupAltitudeSourceAllowed(LinkInterface* sourceLink) const
{
    if (!_startupAltitudeMemoryCurrent(sourceLink, false)) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(sourceLink);
    const QString epoch = _startupAltitudeEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch;
    const quint64 revision = _startupAltitudeStateRevision, receipt = _startupAltitudeNextReceipt;
    const auto same = [&]() {
        return self && link && self->_startupAltitudeEpoch == epoch
            && self->_autopilotVersionObservationEpoch == versionEpoch && self->_flightStateObservationEpoch == flightEpoch
            && self->_startupAltitudeStateRevision == revision && self->_startupAltitudeNextReceipt == receipt
            && self->_startupAltitudeMemoryCurrent(link.data(), false);
    };
    const bool connected = link->isConnected();
    if (!same() || !connected) return false;
    const bool mock = link->isMockLink();
    if (!same() || mock) return false;
    const bool replay = link->isLogReplay();
    return same() && !replay;
}

quint64 Vehicle::_prepareStartupAltitudeObservation(const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (original.msgid != MAVLINK_MSG_ID_ALTITUDE || QThread::currentThread() != thread()
        || !_mavlink || sender() != _mavlink || original.sysid != _id || original.compid != _defaultComponentId
        || !_startupAltitudeLinks || _startupAltitudeLinks->thread() != thread()) return 0;
    const QPointer<LinkInterface> source(sourceLink);
    const WeakLinkInterfacePtr primary = _startupAltitudeLinks->primaryLink();
    bool primaryFrame = false;
    for (const auto& entry : _startupAltitudeLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !primary.expired()
            && !primary.owner_before(candidate) && !candidate.owner_before(primary)) { primaryFrame = true; break; }
    }
    if (!primaryFrame) return 0;
    if (_fleetStartupAltitude.required && _fleetStartupAltitude.bound && !_fleetStartupAltitude.retired) {
        if (_fleetStartupAltitude.frameCount == std::numeric_limits<quint64>::max())
            _failFleetStartupAltitude(QStringLiteral("altitude-monitor-counter-exhausted"));
        else ++_fleetStartupAltitude.frameCount;
    }
    _startupAltitudeEverObservedRawFrame = true;
    if (_startupAltitudeNextReceipt == std::numeric_limits<quint64>::max()) {
        _startupAltitudeExhausted = true;
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-receipt-exhausted"), false); return 0;
    }
    // Count each actual PRIMARY frame, even when its envelope is rejected;
    // only a completed observation publishes a non-zero receiptSequence.
    const quint64 token = ++_startupAltitudeNextReceipt;
    if (_startupAltitudeReceiveDepth != 1 || _startupAltitudeExhausted) {
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-nested-or-exhausted"), false); return 0;
    }
    if (_startupAltitudeLink && (_startupAltitudeLink.data() != sourceLink
        || _startupAltitudeVersionEpoch != _autopilotVersionObservationEpoch || _startupAltitudeFlightEpoch != _flightStateObservationEpoch
        || primary.owner_before(_startupAltitudeWeakLink) || _startupAltitudeWeakLink.owner_before(primary)
        || _startupAltitudeComponentId != _defaultComponentId || _startupAltitudeFirmwareType != int(_firmwareType)
        || _startupAltitudeVehicleType != int(_vehicleType)))
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-source-change"), true);
    bool decoded = false;
    const QVariantMap value = FleetStartupAltitude::decode(original, &decoded);
    const QVariantMap state = decoded ? FleetStartupAltitude::stateProjection(value) : QVariantMap();
    bool sampleTimeOk = false;
    const quint64 sampleTime = value.value(QStringLiteral("timeUsec")).toString().toULongLong(&sampleTimeOk);
    if (!decoded || state.size() != 8 || !sampleTimeOk) {
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-invalid-envelope"), false); return 0;
    }
    const QString stateHash = startupValueHash(state);
    const bool clockRegressed = _startupAltitudeRawSampleObserved && sampleTime < _startupAltitudeLastRawSampleTime;
    const bool stateChanged = stateHash != _startupAltitudeLastPreparedStateHash || clockRegressed;
    if ((stateChanged && _startupAltitudeStateRevision == std::numeric_limits<quint64>::max())
        || (clockRegressed && _startupAltitudeClockDiscontinuityRevision == std::numeric_limits<quint64>::max())) {
        _startupAltitudeExhausted = true;
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-state-counter-exhausted"), false); return 0;
    }
    // Classify before the first Link callback. Numeric height changes and
    // ordinary time progression do not churn this state-only identity.
    if (stateChanged) ++_startupAltitudeStateRevision;
    if (clockRegressed) {
        ++_startupAltitudeClockDiscontinuityRevision;
        _startupAltitudeClockDiscontinuityObserved = true;
    }
    _startupAltitudeLastPreparedStateHash = stateHash;
    _startupAltitudeLastRawSampleTime = sampleTime; _startupAltitudeRawSampleObserved = true;
    // Evaluate the actual original frame before any Link virtual/connect
    // callback. A later in-envelope packet cannot erase an observed excursion.
    _consumeFleetStartupAltitude(value, receivedAtMs);
    const QPointer<Vehicle> self(this);
    const QString preparedEpoch = _startupAltitudeEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch;
    const quint64 preparedRevision = _startupAltitudeStateRevision;
    const MAV_AUTOPILOT firmware = _firmwareType;
    const MAV_TYPE model = _vehicleType;
    const auto preparedCurrent = [&]() {
        return self && source && _startupAltitudeEpoch == preparedEpoch
            && _autopilotVersionObservationEpoch == versionEpoch && _flightStateObservationEpoch == flightEpoch
            && _startupAltitudeNextReceipt == token && _startupAltitudeStateRevision == preparedRevision
            && _startupAltitudeLastPreparedStateHash == stateHash && _startupAltitudeLastRawSampleTime == sampleTime
            && original.sysid == _id && original.compid == _defaultComponentId
            && _firmwareType == firmware && _vehicleType == model;
    };
    if (!_startupAltitudeLink || !_startupAltitudeLinkHooksReady) {
        _startupAltitudeLinkHooksReady = false;
        const QString bindingEpoch = _startupAltitudeEpoch;
        while (!_startupAltitudeLinkConnections.isEmpty()) {
            const auto connection = _startupAltitudeLinkConnections.takeFirst();
            QObject::disconnect(connection);
            if (!self) return 0;
            if (!source) { _invalidateStartupAltitudeObservation(QStringLiteral("altitude-link-removed"), true); return 0; }
            if (_startupAltitudeEpoch != bindingEpoch || !preparedCurrent()) return 0;
        }
        _startupAltitudeLink = source.data(); _startupAltitudeWeakLink = primary;
        _startupAltitudeVersionEpoch = _autopilotVersionObservationEpoch; _startupAltitudeFlightEpoch = _flightStateObservationEpoch;
        _startupAltitudeComponentId = _defaultComponentId;
        _startupAltitudeFirmwareType = int(_firmwareType); _startupAltitudeVehicleType = int(_vehicleType);
        const QString epoch = _startupAltitudeEpoch;
        const auto lost = [this, epoch]() {
            if (QThread::currentThread() == thread() && _startupAltitudeEpoch == epoch)
                _invalidateStartupAltitudeObservation(QStringLiteral("altitude-link-removed"), true);
        };
        const auto disconnected = connect(source.data(), &LinkInterface::disconnected, this, lost);
        if (!self) return 0;
        if (!source) { _invalidateStartupAltitudeObservation(QStringLiteral("altitude-link-removed"), true); return 0; }
        if (_startupAltitudeEpoch != epoch || !preparedCurrent()) { QObject::disconnect(disconnected); return 0; }
        _startupAltitudeLinkConnections.append(disconnected);
        const auto destroyed = connect(source.data(), &QObject::destroyed, this, lost);
        if (!self) return 0;
        if (!source) { _invalidateStartupAltitudeObservation(QStringLiteral("altitude-link-removed"), true); return 0; }
        if (_startupAltitudeEpoch != epoch || !preparedCurrent()) { QObject::disconnect(destroyed); return 0; }
        _startupAltitudeLinkConnections.append(destroyed);
        _startupAltitudeLinkHooksReady = true;
    }
    if (!preparedCurrent()) return 0;
    _startupAltitudeRow.updateToken = token; _startupAltitudeRow.updating = true; _startupAltitudeRow.complete = false;
    const bool allowed = _startupAltitudeSourceAllowed(source.data());
    if (!self) return 0;
    if (!allowed || _startupAltitudeRow.updateToken != token || !_startupAltitudeRow.updating) {
        // A nested same-source bad frame has already retired this update and
        // must not be reclassified as a source change that erases clock history.
        if (_startupAltitudeRow.updateToken == token && _startupAltitudeRow.updating)
            _invalidateStartupAltitudeObservation(QStringLiteral("altitude-untrusted-source"), true);
        return 0;
    }
    return token;
}

void Vehicle::_completeStartupAltitudeObservation(quint64 token, const QString& epoch,
    const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (!token || _startupAltitudeEpoch != epoch || _startupAltitudeRow.updateToken != token || !_startupAltitudeRow.updating) return;
    const QPointer<Vehicle> self(this);
    const bool allowed = _startupAltitudeSourceAllowed(sourceLink);
    if (!self) return;
    if (_startupAltitudeEpoch != epoch || _startupAltitudeRow.updateToken != token || !_startupAltitudeRow.updating) return;
    if (!allowed) { _invalidateStartupAltitudeObservation(QStringLiteral("altitude-commit-source-unavailable"), true); return; }
    bool decoded = false;
    const QVariantMap value = FleetStartupAltitude::decode(original, &decoded);
    const QVariantMap state = decoded ? FleetStartupAltitude::stateProjection(value) : QVariantMap();
    if (_startupAltitudeReceiveDepth != 1 || receivedAtMs < 0 || state.size() != 8
        || startupValueHash(state) != _startupAltitudeLastPreparedStateHash
        || value.value(QStringLiteral("timeUsec")).toString() != QString::number(_startupAltitudeLastRawSampleTime)) {
        _invalidateStartupAltitudeObservation(QStringLiteral("altitude-commit-unavailable"), false); return;
    }
    _startupAltitudeRow.value = value;
    _startupAltitudeRow.rawPayloadHash = startupRawHash(original); _startupAltitudeRow.rawPayloadLength = int(original.len);
    _startupAltitudeRow.receiptSequence = token; _startupAltitudeRow.receivedAtMs = receivedAtMs;
    _startupAltitudeRow.observed = true; _startupAltitudeRow.complete = true; _startupAltitudeRow.updating = false;
    _startupAltitudeSource = QStringLiteral("live-altitude");
}

bool Vehicle::startupAltitudeObservationSourceMatches(LinkInterface* expectedPrimary) const
{
    return _startupAltitudeMemoryCurrent(expectedPrimary, true);
}

bool Vehicle::startupAltitudeHomeReferenceMatches(LinkInterface* expectedPrimary,
    const QVariantMap& homeCoordinates, qint64 maximumAgeMs) const
{
    // Additional v4 tail fence only. Reuse actual native owner/PRIMARY weak
    // identity without entering any Link virtual or manufacturing a sample.
    if (maximumAgeMs < 1 || maximumAgeMs > 60000 || homeCoordinates.size() != 9
        || !_startupAltitudeMemoryCurrent(expectedPrimary, true)
        || _startupCoordinateExhausted || _startupCoordinateReceiveDepth != 0
        || _flightStateObservationReceiveDepth != 0 || _autopilotVersionObservationReceiveDepth != 0
        || _fleetStartupReceiveDepth != 0 || _startupOdometryReceiveDepth != 0 || _startupGlobalOriginReceiveDepth != 0
        || _startupCoordinateEpoch.isEmpty() || _startupCoordinateLink.data() != expectedPrimary
        || _startupCoordinateVersionEpoch != _autopilotVersionObservationEpoch
        || _startupCoordinateFlightEpoch != _flightStateObservationEpoch || !_autopilotVersionObservationClock.isValid()) return false;
    const StartupCoordinateRow home = _startupCoordinateRows.value(QStringLiteral("homePosition"));
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    return home.observed && home.complete && !home.updating && home.receiptSequence != 0
        && home.receivedAtMs >= 0 && now >= home.receivedAtMs && now - home.receivedAtMs <= maximumAgeMs
        && home.value == homeCoordinates;
}

QVariantMap Vehicle::startupAltitudeObservation(qint64 maximumAgeMs) const
{
    if (QThread::currentThread() != thread() || maximumAgeMs < 1 || maximumAgeMs > 60000) return {};
    const StartupAltitudeRow row = _startupAltitudeRow;
    const qint64 now = _startupAltitudeClock.isValid() ? _startupAltitudeClock.elapsed() : -1;
    const qint64 age = row.observed && row.receivedAtMs >= 0 && now >= row.receivedAtMs ? now - row.receivedAtMs : -1;
    // Availability is complete current-source RAW receipt availability, NOT
    // all-six-known, estimator validity, or a permission to use an altitude.
    const bool available = row.observed && row.complete && !row.updating && row.receiptSequence != 0 && age >= 0
        && _startupAltitudeMemoryCurrent(_startupAltitudeLink.data(), true);
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), _id}, {QStringLiteral("componentId"), _defaultComponentId},
        {QStringLiteral("sourceEpoch"), _autopilotVersionObservationEpoch}, {QStringLiteral("flightSourceEpoch"), _flightStateObservationEpoch},
        {QStringLiteral("altitudeSourceEpoch"), _startupAltitudeEpoch}, {QStringLiteral("observed"), row.observed},
        {QStringLiteral("everObservedRawFrame"), _startupAltitudeEverObservedRawFrame}, {QStringLiteral("available"), available},
        {QStringLiteral("fresh"), available && age <= maximumAgeMs}, {QStringLiteral("receiveStackQuiescent"), _startupAltitudeReceiveDepth == 0},
        {QStringLiteral("receiptSequence"), QString::number(row.receiptSequence)}, {QStringLiteral("stateRevision"), QString::number(_startupAltitudeStateRevision)},
        {QStringLiteral("clockDiscontinuityRevision"), QString::number(_startupAltitudeClockDiscontinuityRevision)},
        {QStringLiteral("clockDiscontinuityObserved"), _startupAltitudeClockDiscontinuityObserved},
        {QStringLiteral("receivedAtMonotonicMs"), double(row.receivedAtMs)}, {QStringLiteral("ageMs"), double(age)},
        {QStringLiteral("source"), _startupAltitudeSource}, {QStringLiteral("value"), row.value},
        {QStringLiteral("rawPayloadHash"), row.rawPayloadHash.isEmpty() ? QStringLiteral("") : row.rawPayloadHash},
        {QStringLiteral("rawPayloadLength"), row.rawPayloadLength}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("px4BranchValidityProven"), false},
        {QStringLiteral("sameEstimatorSampleProven"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

void Vehicle::_failFleetStartupAltitude(const QString& reason)
{
    if (!_fleetStartupAltitude.required || !_fleetStartupAltitude.bound || _fleetStartupAltitude.retired) return;
    _fleetStartupAltitude.violation = true;
    if (_fleetStartupAltitude.reason.isEmpty()) _fleetStartupAltitude.reason = reason;
}

qint64 Vehicle::_fleetStartupAltitudeMaximumGap() const
{
    if (!_fleetStartupAltitude.required || !_fleetStartupAltitude.bound || !_startupAltitudeClock.isValid()
        || _fleetStartupAltitude.lastReceivedAtMs < 0 || _fleetStartupAltitude.maximumGapMs < 0) return -1;
    const qint64 now = _fleetStartupAltitude.retired ? _fleetStartupAltitude.endedAtMs : _startupAltitudeClock.elapsed();
    if (now < _fleetStartupAltitude.lastReceivedAtMs) return -1;
    return qMax(_fleetStartupAltitude.maximumGapMs, now - _fleetStartupAltitude.lastReceivedAtMs);
}

bool Vehicle::_bindFleetStartupAltitude(const QVariantMap& actualContext)
{
    // Called only when installing a different actual owner/startup scope.
    // No step, retry, getter or recovered packet may reset this monitor.
    _fleetStartupAltitude = FleetStartupAltitudeMonitor();
    _fleetStartupAltitude.context = actualContext;
    if (actualContext.size() != 4 || actualContext.value(QStringLiteral("required")).type() != QVariant::Bool) {
        _fleetStartupAltitude.required = true; _fleetStartupAltitude.bound = true;
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-actual-policy-context-invalid")); return false;
    }
    _fleetStartupAltitude.required = actualContext.value(QStringLiteral("required")).toBool();
    if (!_fleetStartupAltitude.required) return true; // v1-v3 do not depend on ALTITUDE.
    _fleetStartupAltitude.bound = true;
    const QVariantMap policy = actualContext.value(QStringLiteral("policy")).toMap();
    const QVariantMap reference = actualContext.value(QStringLiteral("reference")).toMap();
    const QVariantMap baseline = actualContext.value(QStringLiteral("sample")).toMap();
    const qint64 gap = policy.value(QStringLiteral("maximumRawGapMs")).toLongLong();
    const QVariantMap observation = startupAltitudeObservation(gap);
    QVariantMap sample;
    _fleetStartupAltitude.sourceEpoch = _autopilotVersionObservationEpoch;
    _fleetStartupAltitude.flightEpoch = _flightStateObservationEpoch;
    _fleetStartupAltitude.altitudeEpoch = _startupAltitudeEpoch;
    _fleetStartupAltitude.coordinateEpoch = _startupCoordinateEpoch;
    _fleetStartupAltitude.primary = _startupAltitudeWeakLink;
    _fleetStartupAltitude.clockRevision = _startupAltitudeClockDiscontinuityRevision;
    _fleetStartupAltitude.homeRiskRevision = _startupCoordinateRows.value(QStringLiteral("homePosition")).riskContentRevision;
    _fleetStartupAltitude.baselineReceipt = _startupAltitudeRow.receiptSequence;
    _fleetStartupAltitude.lastReceivedAtMs = _startupAltitudeRow.receivedAtMs;
    const qint64 now = _startupAltitudeClock.isValid() ? _startupAltitudeClock.elapsed() : -1;
    _fleetStartupAltitude.maximumGapMs = now >= _fleetStartupAltitude.lastReceivedAtMs
        && _fleetStartupAltitude.lastReceivedAtMs >= 0 ? now - _fleetStartupAltitude.lastReceivedAtMs : -1;
    if (gap < 100 || gap > 3000 || !_startupAltitudeMemoryCurrent(_fleetStartupLink.data(), true)
        || !FleetMissionStartupAltitude::capture(observation, reference,
            policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap(), gap, &sample)
        || !FleetMissionStartupAltitude::sampleWithinBaseline(sample, baseline,
            policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap())) {
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-original-reviewed-baseline-required")); return false;
    }
    if (!_fleetStartupAltitudeCurrent()) {
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-original-reference-not-current")); return false;
    }
    return true;
}

void Vehicle::_consumeFleetStartupAltitude(const QVariantMap& decoded, qint64 receivedAtMs)
{
    if (!_fleetStartupAltitude.required || !_fleetStartupAltitude.bound || _fleetStartupAltitude.retired) return;
    const QVariantMap policy = _fleetStartupAltitude.context.value(QStringLiteral("policy")).toMap();
    const qint64 maximumGap = policy.value(QStringLiteral("maximumRawGapMs")).toLongLong();
    const WeakLinkInterfacePtr primary = _startupAltitudeLinks ? _startupAltitudeLinks->primaryLink() : WeakLinkInterfacePtr();
    if (_fleetStartupAltitude.sourceEpoch != _autopilotVersionObservationEpoch
        || _fleetStartupAltitude.flightEpoch != _flightStateObservationEpoch || _fleetStartupAltitude.altitudeEpoch != _startupAltitudeEpoch
        || primary.expired() || _fleetStartupAltitude.primary.expired()
        || primary.owner_before(_fleetStartupAltitude.primary) || _fleetStartupAltitude.primary.owner_before(primary)
        || !_startupAltitudeMemoryCurrent(_fleetStartupLink.data(), false)
        || _startupAltitudeClockDiscontinuityRevision != _fleetStartupAltitude.clockRevision
        || _startupAltitudeClockDiscontinuityObserved != _fleetStartupAltitude.context.value(QStringLiteral("reference")).toMap()
            .value(QStringLiteral("clockDiscontinuityObserved")).toBool())
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-source-or-clock-changed"));
    if (receivedAtMs < 0 || _fleetStartupAltitude.lastReceivedAtMs < 0
        || receivedAtMs < _fleetStartupAltitude.lastReceivedAtMs || maximumGap < 100 || maximumGap > 3000) {
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-reception-clock-unavailable"));
    } else {
        const qint64 gap = receivedAtMs - _fleetStartupAltitude.lastReceivedAtMs;
        _fleetStartupAltitude.maximumGapMs = qMax(_fleetStartupAltitude.maximumGapMs, gap);
        if (gap > maximumGap) _failFleetStartupAltitude(QStringLiteral("altitude-monitor-observed-reception-gap-exceeded"));
        _fleetStartupAltitude.lastReceivedAtMs = receivedAtMs;
    }
    if (!FleetMissionStartupAltitude::decodedMatches(decoded,
        _fleetStartupAltitude.context.value(QStringLiteral("reference")).toMap(),
        _fleetStartupAltitude.context.value(QStringLiteral("sample")).toMap(),
        policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap()))
        _failFleetStartupAltitude(QStringLiteral("altitude-monitor-observed-envelope-violation"));
}

QVariantMap Vehicle::fleetStartupAltitudeMonitor(QObject* actualOwner, LinkInterface* actualPrimary,
    const QString& startupId, const QVariantMap& originalReference) const
{
    const auto owner = qobject_cast<FleetMissionStartupTransaction*>(actualOwner);
    if (QThread::currentThread() != thread() || !_fleetStartupAltitude.required || !_fleetStartupAltitude.bound
        || !owner || owner != _fleetStartupOwner || owner->thread() != thread()
        || owner->transactionEpoch() != _fleetStartupOwnerEpoch || startupId != _fleetStartupId
        || !actualPrimary || actualPrimary != _fleetStartupLink.data()
        || originalReference != _fleetStartupAltitude.context.value(QStringLiteral("reference")).toMap()) return {};
    const QVariantMap actualContext = owner->reviewedAltitudeContext(const_cast<Vehicle*>(this), actualPrimary, startupId);
    if (actualContext != _fleetStartupAltitude.context) return {};
    const QVariantMap policy = actualContext.value(QStringLiteral("policy")).toMap();
    const WeakLinkInterfacePtr primary = _startupAltitudeLinks ? _startupAltitudeLinks->primaryLink() : WeakLinkInterfacePtr();
    const bool sourceCurrent = !_fleetStartupScopeCancelled && !_fleetStartupAltitude.retired
        && _startupAltitudeMemoryCurrent(actualPrimary, true)
        && _fleetStartupAltitude.sourceEpoch == _autopilotVersionObservationEpoch
        && _fleetStartupAltitude.flightEpoch == _flightStateObservationEpoch && _fleetStartupAltitude.altitudeEpoch == _startupAltitudeEpoch
        && !primary.expired() && !_fleetStartupAltitude.primary.expired()
        && !primary.owner_before(_fleetStartupAltitude.primary) && !_fleetStartupAltitude.primary.owner_before(primary);
    const StartupCoordinateRow home = _startupCoordinateRows.value(QStringLiteral("homePosition"));
    const bool referenceMatches = sourceCurrent
        && _startupAltitudeClockDiscontinuityRevision == _fleetStartupAltitude.clockRevision
        && _startupAltitudeClockDiscontinuityObserved == originalReference.value(QStringLiteral("clockDiscontinuityObserved")).toBool()
        && _startupCoordinateEpoch == _fleetStartupAltitude.coordinateEpoch && _startupCoordinateReceiveDepth == 0
        && !_startupCoordinateExhausted && _startupCoordinateLink.data() == actualPrimary
        && _startupCoordinateVersionEpoch == _autopilotVersionObservationEpoch && _startupCoordinateFlightEpoch == _flightStateObservationEpoch
        && home.observed && home.complete && !home.updating && home.riskContentRevision == _fleetStartupAltitude.homeRiskRevision
        && home.value == originalReference.value(QStringLiteral("homeCoordinates")).toMap()
        && startupAltitudeHomeReferenceMatches(actualPrimary, originalReference.value(QStringLiteral("homeCoordinates")).toMap(),
            policy.value(QStringLiteral("maximumRawGapMs")).toLongLong())
        && _startupGlobalOriginMemoryCurrent(actualPrimary, true)
        && _startupGlobalOriginStableReference() == originalReference.value(QStringLiteral("originReference")).toMap();
    const bool within = referenceMatches && _startupAltitudeRow.observed && _startupAltitudeRow.complete && !_startupAltitudeRow.updating
        && FleetMissionStartupAltitude::decodedMatches(_startupAltitudeRow.value, originalReference,
            actualContext.value(QStringLiteral("sample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
    const qint64 maximumGap = _fleetStartupAltitudeMaximumGap();
    const qint64 allowedGap = policy.value(QStringLiteral("maximumRawGapMs")).toLongLong();
    const bool gapValid = maximumGap >= 0 && allowedGap >= 100 && allowedGap <= 3000 && maximumGap <= allowedGap;
    const bool ok = sourceCurrent && referenceMatches && within && gapValid && !_fleetStartupAltitude.violation;
    QString reason = _fleetStartupAltitude.reason;
    if (reason.isEmpty()) reason = _fleetStartupAltitude.retired ? QStringLiteral("altitude-monitor-scope-retired")
        : !sourceCurrent ? QStringLiteral("altitude-monitor-current-source-unavailable")
        : !referenceMatches ? QStringLiteral("altitude-monitor-reference-changed")
        : !within ? QStringLiteral("altitude-monitor-current-sample-outside-envelope")
        : !gapValid ? QStringLiteral("altitude-monitor-current-reception-gap-exceeded") : QStringLiteral("");
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("required"), true}, {QStringLiteral("bound"), true},
        {QStringLiteral("sourceCurrent"), sourceCurrent}, {QStringLiteral("referenceMatches"), referenceMatches},
        {QStringLiteral("sampleWithinEnvelope"), within}, {QStringLiteral("continuousGapValid"), gapValid},
        {QStringLiteral("violationObserved"), _fleetStartupAltitude.violation}, {QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("startupId"), _fleetStartupId}, {QStringLiteral("transactionEpoch"), _fleetStartupOwnerEpoch},
        {QStringLiteral("referenceHash"), startupValueHash(originalReference)}, {QStringLiteral("maximumObservedGapMs"), double(maximumGap)},
        {QStringLiteral("observedFrameCount"), QString::number(_fleetStartupAltitude.frameCount)},
        {QStringLiteral("baselineReceiptSequence"), QString::number(_fleetStartupAltitude.baselineReceipt)}};
}

bool Vehicle::_fleetStartupAltitudeCurrent() const
{
    if (!_fleetStartupAltitude.required) return true;
    return fleetStartupAltitudeMonitor(_fleetStartupOwner.data(), _fleetStartupLink.data(), _fleetStartupId,
        _fleetStartupAltitude.context.value(QStringLiteral("reference")).toMap()).value(QStringLiteral("ok")).toBool();
}

bool Vehicle::retireFleetStartupAltitudeMonitor(QObject* actualOwner, const QString& startupId)
{
    const auto owner = qobject_cast<FleetMissionStartupTransaction*>(actualOwner);
    if (QThread::currentThread() != thread() || !owner || owner != _fleetStartupOwner
        || owner->thread() != thread() || owner->transactionEpoch() != _fleetStartupOwnerEpoch
        || startupId != _fleetStartupId || !_fleetStartupAltitude.required || !_fleetStartupAltitude.bound) return false;
    if (!_fleetStartupAltitude.retired) {
        _fleetStartupAltitude.maximumGapMs = _fleetStartupAltitudeMaximumGap();
        _fleetStartupAltitude.endedAtMs = _startupAltitudeClock.isValid() ? _startupAltitudeClock.elapsed() : -1;
        _fleetStartupAltitude.retired = true;
    }
    return true; // No state clearing, renewed permit, rollback or command.
}

bool Vehicle::fleetStartupAltitudeTransportQuiescent(const QString& startupId, const QString& transactionEpoch) const
{
    if (QThread::currentThread() != thread()) return false;
    if (!_fleetStartupAltitude.required || !_fleetStartupAltitude.bound
        || startupId != _fleetStartupId || transactionEpoch != _fleetStartupOwnerEpoch) return true;
    return _startupAltitudeReceiveDepth == 0;
}

void Vehicle::_initializeFleetCameraStreamObservation()
{
    _fleetCameraStreamClock.start();
    _fleetCameraStreamEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Vehicle::_invalidateFleetCameraStreamObservation(const QString& reason)
{
    // A source clear never renews the global receipt or semantic revision.
    // Do not reset an older ingress/probe guard's outstanding stack here.
    _fleetCameraStreamEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetCameraStreamRows.clear();
    _fleetCameraStreamDeclaredCounts.clear();
    _fleetCameraStreamPrimary.clear();
    _fleetCameraStreamPrimaryOwner.reset();
    _fleetCameraStreamTransportEpoch.clear();
    _fleetCameraStreamVersionEpoch.clear();
    _fleetCameraStreamBoundAtMs = _fleetCameraStreamNewestAtMs = -1;
    _fleetCameraStreamVehicleId = _fleetCameraStreamDefaultComponentId = 0;
    _fleetCameraStreamFirmwareType = _fleetCameraStreamVehicleType = -1;
    _fleetCameraStreamSaturated = false;
    _fleetCameraStreamReason = reason;
}

bool Vehicle::_fleetCameraStreamRawSourceMatches(LinkInterface* sourceLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || qgcApp()->thread() != thread()
        || _offlineEditingVehicle || _fleetCameraStreamRemoving || _fleetCameraStreamExhausted
        || _autopilotVersionObservationRemoving || _fleetMessageIntervalRawReceiveExhausted
        || !_fleetCameraStreamClock.isValid() || !QElapsedTimer::isMonotonic()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255 || !sourceLink
        || _messageCommandTransportEpoch.isEmpty() || _autopilotVersionObservationEpoch.isEmpty()
        || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
        || !_fleetExecutionSourceVehicles || !_fleetExecutionSourceToolbox
        || _fleetExecutionSourceLinks->thread() != thread() || _fleetExecutionSourceProtocol->thread() != thread()
        || _fleetExecutionSourceVehicles->thread() != thread() || _fleetExecutionSourceToolbox->thread() != thread()
        || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || _fleetExecutionSourceToolbox->mavlinkProtocol() != _fleetExecutionSourceProtocol.data()
        || _fleetExecutionSourceToolbox->multiVehicleManager() != _fleetExecutionSourceVehicles.data()
        || _fleetExecutionSourceVehicles->getVehicleById(_id) != this || _fleetExecutionSourceLinks->communicationLost()) return false;
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (primary.expired()) return false;
    for (const auto& member : _fleetExecutionSourceLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(member.link);
        if (member.link && member.link.get() == sourceLink && !primary.owner_before(candidate)
            && !candidate.owner_before(primary)) return true;
    }
    return false;
}

Vehicle::FleetCameraStreamIngress Vehicle::_prepareFleetCameraStreamIngress(
    const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    FleetCameraStreamIngress ingress;
    if (!fleetCameraStreamFrame(original) || original.sysid != _id
        || sender() != _fleetExecutionSourceProtocol.data() || !_fleetCameraStreamRawSourceMatches(sourceLink)) return ingress;
    if (_fleetMessageIntervalRawReceiveDepth != 1 || _fleetCameraStreamProbeBusy) {
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-nested-raw"));
        return ingress;
    }
    if (receivedAtMs < 0) {
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-clock-unavailable"));
        return ingress;
    }
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (_fleetCameraStreamBoundAtMs >= 0 && (_fleetCameraStreamPrimary.data() != sourceLink
        || _fleetCameraStreamTransportEpoch != _messageCommandTransportEpoch
        || _fleetCameraStreamVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetCameraStreamVehicleId != _id || _fleetCameraStreamDefaultComponentId != _defaultComponentId
        || _fleetCameraStreamFirmwareType != int(_firmwareType) || _fleetCameraStreamVehicleType != int(_vehicleType)
        || _fleetCameraStreamPrimaryOwner.expired() || primary.owner_before(_fleetCameraStreamPrimaryOwner)
        || _fleetCameraStreamPrimaryOwner.owner_before(primary)))
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-source-binding-changed"));
    ingress.primary = sourceLink; ingress.primaryOwner = primary;
    ingress.links = _fleetExecutionSourceLinks; ingress.protocol = _fleetExecutionSourceProtocol;
    ingress.vehicles = _fleetExecutionSourceVehicles; ingress.toolbox = _fleetExecutionSourceToolbox;
    ingress.sourceEpoch = _fleetCameraStreamEpoch; ingress.transportEpoch = _messageCommandTransportEpoch;
    ingress.versionEpoch = _autopilotVersionObservationEpoch; ingress.receiptSequence = _fleetCameraStreamNextReceipt;
    ingress.receivedAtMs = receivedAtMs; ingress.vehicleId = _id; ingress.componentId = original.compid;
    ingress.defaultComponentId = _defaultComponentId;
    ingress.firmwareType = int(_firmwareType); ingress.vehicleType = int(_vehicleType); ingress.eligible = true;
    return ingress;
}

void Vehicle::_observeFleetCameraStream(const mavlink_message_t& original, const FleetCameraStreamIngress& originalIngress)
{
    const FleetCameraStreamIngress ingress = originalIngress;
    if (!ingress.eligible) return;
    const QPointer<Vehicle> self(this);
    const auto sameMemory = [&]() {
        if (!self || !ingress.primary || !ingress.links || !ingress.protocol || !ingress.vehicles || !ingress.toolbox
            || self->_fleetExecutionSourceLinks != ingress.links || self->_fleetExecutionSourceProtocol != ingress.protocol
            || self->_fleetExecutionSourceVehicles != ingress.vehicles || self->_fleetExecutionSourceToolbox != ingress.toolbox
            || self->_fleetCameraStreamEpoch != ingress.sourceEpoch || self->_messageCommandTransportEpoch != ingress.transportEpoch
            || self->_autopilotVersionObservationEpoch != ingress.versionEpoch
            || self->_fleetCameraStreamNextReceipt != ingress.receiptSequence
            || self->_fleetMessageIntervalRawReceiveDepth != 1 || self->_id != ingress.vehicleId
            || self->_defaultComponentId != ingress.defaultComponentId || int(self->_firmwareType) != ingress.firmwareType
            || int(self->_vehicleType) != ingress.vehicleType
            || !self->_fleetCameraStreamRawSourceMatches(ingress.primary.data())) return false;
        const WeakLinkInterfacePtr current = ingress.links->primaryLink();
        return !current.expired() && !current.owner_before(ingress.primaryOwner) && !ingress.primaryOwner.owner_before(current);
    };
    const auto invalidateOriginal = [&](const QString& reason) {
        if (self && self->_fleetCameraStreamEpoch == ingress.sourceEpoch
            && self->_fleetCameraStreamNextReceipt == ingress.receiptSequence)
            self->_invalidateFleetCameraStreamObservation(reason);
    };
    if (!sameMemory() || _fleetCameraStreamProbeBusy) return;
    // IDs 269/270 cannot be represented by MAVLink1. Normalize v2 trailing
    // zero truncation before either generated decoder implementation reads.
    const bool informationFrame = original.msgid == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION;
    const int maximum = informationFrame ? MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION_LEN : MAVLINK_MSG_ID_VIDEO_STREAM_STATUS_LEN;
    if (original.magic != MAVLINK_STX || original.len < 1 || original.len > maximum
        || !fleetCameraStreamFrame(original) || original.sysid != ingress.vehicleId || original.compid != ingress.componentId) {
        invalidateOriginal(QStringLiteral("camera-stream-invalid-wire")); return;
    }
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    mavlink_video_stream_information_t information{};
    mavlink_video_stream_status_t status{};
    if (informationFrame) mavlink_msg_video_stream_information_decode(&normalized, &information);
    else mavlink_msg_video_stream_status_decode(&normalized, &status);
    const int streamId = informationFrame ? information.stream_id : status.stream_id;
    if (!streamId || (informationFrame && (!information.count || information.stream_id > information.count))) {
        invalidateOriginal(QStringLiteral("camera-stream-invalid-stream-id-or-count")); return;
    }
    _fleetCameraStreamProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_fleetCameraStreamProbeBusy = false; });
    const bool connected = ingress.primary->isConnected();
    if (!sameMemory() || !connected) { invalidateOriginal(QStringLiteral("camera-stream-source-probe-unavailable")); return; }
    const bool mock = ingress.primary->isMockLink();
    if (!sameMemory() || mock) { invalidateOriginal(QStringLiteral("camera-stream-source-probe-unavailable")); return; }
    const bool replay = ingress.primary->isLogReplay();
    if (!sameMemory() || replay) { invalidateOriginal(QStringLiteral("camera-stream-source-probe-unavailable")); return; }
    if ((_fleetCameraStreamBoundAtMs >= 0 && ingress.receivedAtMs < _fleetCameraStreamBoundAtMs)
        || (_fleetCameraStreamNewestAtMs >= 0 && ingress.receivedAtMs < _fleetCameraStreamNewestAtMs)) {
        invalidateOriginal(QStringLiteral("camera-stream-clock-regressed")); return;
    }
    if (_fleetCameraStreamNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetCameraStreamExhausted = true;
        invalidateOriginal(QStringLiteral("camera-stream-receipt-exhausted")); return;
    }
    if (!_fleetCameraStreamPrimary) {
        _fleetCameraStreamPrimary = ingress.primary; _fleetCameraStreamPrimaryOwner = ingress.primaryOwner;
        _fleetCameraStreamTransportEpoch = ingress.transportEpoch; _fleetCameraStreamVersionEpoch = ingress.versionEpoch;
        _fleetCameraStreamVehicleId = ingress.vehicleId; _fleetCameraStreamDefaultComponentId = ingress.defaultComponentId;
        _fleetCameraStreamFirmwareType = ingress.firmwareType; _fleetCameraStreamVehicleType = ingress.vehicleType;
        _fleetCameraStreamBoundAtMs = ingress.receivedAtMs;
    }
    const quint64 receipt = ++_fleetCameraStreamNextReceipt;
    if (_fleetCameraStreamNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetCameraStreamExhausted = true;
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-receipt-exhausted")); return;
    }
    _fleetCameraStreamNewestAtMs = ingress.receivedAtMs;
    // Once truncated, keep only the bounded historical prefix until an actual
    // source clear. A later count or friendly packet cannot certify recovery.
    if (_fleetCameraStreamSaturated) return;
    if (informationFrame && _fleetCameraStreamDeclaredCounts.value(ingress.componentId, 0) != information.count) {
        for (auto row = _fleetCameraStreamRows.begin(); row != _fleetCameraStreamRows.end();) {
            if (row.key() / 256 == ingress.componentId) row = _fleetCameraStreamRows.erase(row);
            else ++row;
        }
        _fleetCameraStreamDeclaredCounts.insert(ingress.componentId, information.count);
    }
    const int key = ingress.componentId * 256 + streamId;
    auto found = _fleetCameraStreamRows.find(key);
    if (found == _fleetCameraStreamRows.end()) {
        if (_fleetCameraStreamRows.size() >= FleetCameraStreamMaximumRows) {
            _fleetCameraStreamSaturated = true;
            _fleetCameraStreamReason = QStringLiteral("camera-stream-row-capacity-truncated"); return;
        }
        found = _fleetCameraStreamRows.insert(key, FleetCameraStreamRow());
    }
    const bool informationValid = std::isfinite(information.framerate) && information.framerate >= 0;
    const bool configurationChanged = informationFrame && (!found->informationObserved
        || found->informationValid != informationValid
        || !sameFleetCameraInformationConfiguration(found->information, information));
    const bool changed = informationFrame ? !found->informationObserved || found->informationValid != informationValid
        || !sameFleetCameraInformation(found->information, information)
        : !found->statusObserved || !sameFleetCameraStatus(found->status, status)
            || found->statusInformationRevision != found->informationRevision;
    if (changed && _fleetCameraStreamRevision == std::numeric_limits<quint64>::max()) {
        _fleetCameraStreamExhausted = true;
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-revision-exhausted")); return;
    }
    const quint64 revision = changed ? ++_fleetCameraStreamRevision
        : (informationFrame ? found->informationRevision : found->statusRevision);
    if (informationFrame) {
        // The shared native allocator never wraps or resets on a source/count
        // clear. A removed/recreated row and configuration A->B->A therefore
        // cannot regain an earlier per-row configuration token.
        if (configurationChanged) found->configurationRevision = revision;
        if (changed) {
            found->status = mavlink_video_stream_status_t{};
            found->statusAtMs = -1; found->statusReceipt = found->statusRevision = found->statusInformationRevision = 0;
            found->statusObserved = found->statusValid = false;
        }
        found->information = information; found->informationObserved = true;
        found->informationValid = informationValid;
        found->informationAtMs = ingress.receivedAtMs; found->informationReceipt = receipt; found->informationRevision = revision;
    } else {
        found->status = status; found->statusObserved = true;
        found->statusValid = std::isfinite(status.framerate) && status.framerate >= 0;
        found->statusAtMs = ingress.receivedAtMs; found->statusReceipt = receipt; found->statusRevision = revision;
        found->statusInformationRevision = found->informationObserved ? found->informationRevision : 0;
    }
    _fleetCameraStreamReason = QStringLiteral("camera-stream-observed");
}

QVariantMap Vehicle::_fleetCameraStreamRowValue(int componentId, int streamId, const FleetCameraStreamRow& value,
    qint64 now, qint64 maximumAgeMs, bool current, bool saturated)
{
    const bool info = value.informationObserved, status = value.statusObserved;
    const qint64 infoAge = info && value.informationAtMs >= 0 && now >= value.informationAtMs ? now - value.informationAtMs : -1;
    const qint64 statusAge = status && value.statusAtMs >= 0 && now >= value.statusAtMs ? now - value.statusAtMs : -1;
    const QVariantMap information{{QStringLiteral("observed"), info},
        {QStringLiteral("fresh"), current && !saturated && info && value.informationValid && infoAge >= 0 && infoAge <= maximumAgeMs},
        {QStringLiteral("ageMs"), QVariant::fromValue(infoAge)}, {QStringLiteral("receiptSequence"), QString::number(value.informationReceipt)},
        {QStringLiteral("contentRevision"), QString::number(value.informationRevision)},
        {QStringLiteral("declaredCount"), info ? int(value.information.count) : -1},
        {QStringLiteral("type"), info ? int(value.information.type) : -1}, {QStringLiteral("flags"), info ? int(value.information.flags) : -1},
        {QStringLiteral("framerate"), info && value.informationValid ? double(value.information.framerate) : -1.0},
        {QStringLiteral("resolutionH"), info ? int(value.information.resolution_h) : -1},
        {QStringLiteral("resolutionV"), info ? int(value.information.resolution_v) : -1},
        {QStringLiteral("bitrate"), QVariant::fromValue(info ? qint64(value.information.bitrate) : qint64(-1))},
        {QStringLiteral("rotation"), info ? int(value.information.rotation) : -1}, {QStringLiteral("hfov"), info ? int(value.information.hfov) : -1},
        {QStringLiteral("valid"), value.informationValid}};
    const bool matches = current && !saturated && info && status && value.informationValid && value.statusValid
        && value.statusInformationRevision != 0 && value.statusInformationRevision == value.informationRevision;
    const QVariantMap statusValue{{QStringLiteral("observed"), status},
        {QStringLiteral("fresh"), current && !saturated && status && value.statusValid && statusAge >= 0 && statusAge <= maximumAgeMs},
        {QStringLiteral("ageMs"), QVariant::fromValue(statusAge)}, {QStringLiteral("receiptSequence"), QString::number(value.statusReceipt)},
        {QStringLiteral("contentRevision"), QString::number(value.statusRevision)},
        {QStringLiteral("informationRevision"), QString::number(value.statusInformationRevision)},
        {QStringLiteral("matchesCurrentInformation"), matches},
        {QStringLiteral("flags"), status ? int(value.status.flags) : -1},
        {QStringLiteral("framerate"), status && value.statusValid ? double(value.status.framerate) : -1.0},
        {QStringLiteral("resolutionH"), status ? int(value.status.resolution_h) : -1},
        {QStringLiteral("resolutionV"), status ? int(value.status.resolution_v) : -1},
        {QStringLiteral("bitrate"), QVariant::fromValue(status ? qint64(value.status.bitrate) : qint64(-1))},
        {QStringLiteral("rotation"), status ? int(value.status.rotation) : -1}, {QStringLiteral("hfov"), status ? int(value.status.hfov) : -1},
        {QStringLiteral("valid"), value.statusValid}};
    return {{QStringLiteral("componentId"), componentId}, {QStringLiteral("streamId"), streamId},
        {QStringLiteral("information"), information}, {QStringLiteral("status"), statusValue}};
}

QVariantMap Vehicle::fleetCameraStreamObservation(qint64 maximumAgeMs) const
{
    const bool ownerThread = QThread::currentThread() == thread();
    const bool validAge = maximumAgeMs >= 1 && maximumAgeMs <= 30000;
    const qint64 now = ownerThread && _fleetCameraStreamClock.isValid() ? _fleetCameraStreamClock.elapsed() : -1;
    const bool observed = ownerThread && !_fleetCameraStreamRows.isEmpty();
    const bool busy = ownerThread && (_fleetMessageIntervalRawReceiveDepth != 0 || _fleetCameraStreamProbeBusy);
    const bool current = ownerThread && validAge && observed && !busy && _fleetCameraStreamPrimary
        && !_fleetCameraStreamPrimaryOwner.expired() && _fleetCameraStreamBoundAtMs >= 0
        && now >= _fleetCameraStreamBoundAtMs && now >= _fleetCameraStreamNewestAtMs
        && _fleetCameraStreamTransportEpoch == _messageCommandTransportEpoch
        && _fleetCameraStreamVersionEpoch == _autopilotVersionObservationEpoch
        && _fleetCameraStreamVehicleId == _id && _fleetCameraStreamDefaultComponentId == _defaultComponentId
        && _fleetCameraStreamFirmwareType == int(_firmwareType) && _fleetCameraStreamVehicleType == int(_vehicleType)
        && _fleetCameraStreamRawSourceMatches(_fleetCameraStreamPrimary.data())
        && !_fleetExecutionSourceLinks->primaryLink().owner_before(_fleetCameraStreamPrimaryOwner)
        && !_fleetCameraStreamPrimaryOwner.owner_before(_fleetExecutionSourceLinks->primaryLink());
    const bool saturated = ownerThread && _fleetCameraStreamSaturated;
    QVariantList rows;
    if (ownerThread) for (auto row = _fleetCameraStreamRows.constBegin(); row != _fleetCameraStreamRows.constEnd(); ++row) {
        const FleetCameraStreamRow value = row.value();
        const bool info = value.informationObserved, status = value.statusObserved;
        const qint64 infoAge = info && value.informationAtMs >= 0 && now >= value.informationAtMs ? now - value.informationAtMs : -1;
        const qint64 statusAge = status && value.statusAtMs >= 0 && now >= value.statusAtMs ? now - value.statusAtMs : -1;
        const QVariantMap information{{QStringLiteral("observed"), info},
            {QStringLiteral("fresh"), current && !saturated && info && value.informationValid && infoAge >= 0 && infoAge <= maximumAgeMs},
            {QStringLiteral("ageMs"), QVariant::fromValue(infoAge)}, {QStringLiteral("receiptSequence"), QString::number(value.informationReceipt)},
            {QStringLiteral("contentRevision"), QString::number(value.informationRevision)},
            {QStringLiteral("declaredCount"), info ? int(value.information.count) : -1},
            {QStringLiteral("type"), info ? int(value.information.type) : -1}, {QStringLiteral("flags"), info ? int(value.information.flags) : -1},
            {QStringLiteral("framerate"), info && value.informationValid ? double(value.information.framerate) : -1.0},
            {QStringLiteral("resolutionH"), info ? int(value.information.resolution_h) : -1},
            {QStringLiteral("resolutionV"), info ? int(value.information.resolution_v) : -1},
            {QStringLiteral("bitrate"), QVariant::fromValue(info ? qint64(value.information.bitrate) : qint64(-1))},
            {QStringLiteral("rotation"), info ? int(value.information.rotation) : -1}, {QStringLiteral("hfov"), info ? int(value.information.hfov) : -1},
            {QStringLiteral("valid"), value.informationValid}};
        const bool matches = current && !saturated && info && status && value.informationValid && value.statusValid
            && value.statusInformationRevision != 0 && value.statusInformationRevision == value.informationRevision;
        const QVariantMap statusValue{{QStringLiteral("observed"), status},
            {QStringLiteral("fresh"), current && !saturated && status && value.statusValid && statusAge >= 0 && statusAge <= maximumAgeMs},
            {QStringLiteral("ageMs"), QVariant::fromValue(statusAge)}, {QStringLiteral("receiptSequence"), QString::number(value.statusReceipt)},
            {QStringLiteral("contentRevision"), QString::number(value.statusRevision)},
            {QStringLiteral("informationRevision"), QString::number(value.statusInformationRevision)},
            {QStringLiteral("matchesCurrentInformation"), matches},
            {QStringLiteral("flags"), status ? int(value.status.flags) : -1},
            {QStringLiteral("framerate"), status && value.statusValid ? double(value.status.framerate) : -1.0},
            {QStringLiteral("resolutionH"), status ? int(value.status.resolution_h) : -1},
            {QStringLiteral("resolutionV"), status ? int(value.status.resolution_v) : -1},
            {QStringLiteral("bitrate"), QVariant::fromValue(status ? qint64(value.status.bitrate) : qint64(-1))},
            {QStringLiteral("rotation"), status ? int(value.status.rotation) : -1}, {QStringLiteral("hfov"), status ? int(value.status.hfov) : -1},
            {QStringLiteral("valid"), value.statusValid}};
        rows.append(QVariantMap{{QStringLiteral("componentId"), row.key() / 256}, {QStringLiteral("streamId"), row.key() % 256},
            {QStringLiteral("information"), information}, {QStringLiteral("status"), statusValue}});
    }
    QString reason = ownerThread ? _fleetCameraStreamReason : QStringLiteral("camera-stream-owner-thread-required");
    if (ownerThread) {
        if (!validAge) reason = QStringLiteral("camera-stream-invalid-maximum-age");
        else if (_fleetCameraStreamRemoving) reason = QStringLiteral("camera-stream-vehicle-removing");
        else if (_fleetCameraStreamExhausted) reason = QStringLiteral("camera-stream-counter-exhausted");
        else if (_fleetMessageIntervalRawReceiveExhausted) reason = QStringLiteral("camera-stream-raw-depth-exhausted");
        else if (busy) reason = QStringLiteral("camera-stream-transport-not-quiescent");
        else if (observed && !current) reason = QStringLiteral("camera-stream-source-not-current");
        else if (saturated) reason = QStringLiteral("camera-stream-row-capacity-truncated");
    }
    const QString empty = QStringLiteral("");
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-camera-stream-observation")},
        {QStringLiteral("vehicleId"), ownerThread ? _id : 0},
        {QStringLiteral("sourceEpoch"), ownerThread && !_fleetCameraStreamEpoch.isEmpty() ? _fleetCameraStreamEpoch : empty},
        {QStringLiteral("transportEpoch"), ownerThread && !_fleetCameraStreamTransportEpoch.isEmpty() ? _fleetCameraStreamTransportEpoch : empty},
        {QStringLiteral("versionEpoch"), ownerThread && !_fleetCameraStreamVersionEpoch.isEmpty() ? _fleetCameraStreamVersionEpoch : empty},
        {QStringLiteral("receiptSequence"), QString::number(ownerThread ? _fleetCameraStreamNextReceipt : 0)},
        {QStringLiteral("sourceCurrent"), current}, {QStringLiteral("observed"), observed}, {QStringLiteral("saturated"), saturated},
        {QStringLiteral("maximumRows"), FleetCameraStreamMaximumRows}, {QStringLiteral("maximumAgeMs"), QVariant::fromValue(maximumAgeMs)},
        {QStringLiteral("rows"), rows}, {QStringLiteral("reason"), reason}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("catalogComplete"), false},
        {QStringLiteral("remoteRunningVerified"), false}, {QStringLiteral("remoteStoppedVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool Vehicle::fleetCameraStreamCaptureTransportQuiescent() const
{
    return fleetMessageIntervalQueryTransportQuiescent() && !_fleetCameraStreamProbeBusy;
}

bool Vehicle::_fleetCameraStreamRunSourceCurrent(const std::shared_ptr<FleetCameraStreamRun>& run, bool requireQuiescent) const
{
    if (!run || !run->owner || run->vehicle.data() != this || _fleetCameraStreamRun != run || run->sourceRetired
        || QThread::currentThread() != thread() || run->owner->thread() != thread()
        || run->sourceEpoch.isEmpty() || run->sourceEpoch != _fleetCameraStreamEpoch
        || _fleetCameraStreamMutationExhausted || run->mutationGeneration != _fleetCameraStreamMutation
        || run->origin.messageIntervalSourceEpoch != _autopilotVersionObservationEpoch
        || !_messageCommandSourceMemoryCurrent(run->origin)
        || !_fleetCameraStreamRawSourceMatches(run->origin.messageIntervalPrimary.data()) || _fleetCameraStreamSaturated
        || (requireQuiescent && !fleetCameraStreamCaptureTransportQuiescent())) return false;
    const qint64 now = _mavCommandQuarantineClock.isValid() ? _mavCommandQuarantineClock.elapsed() : -1;
    if (now < 0 || (run->active && now >= run->deadlineMs)) return false;
    if (run->active && run->stage < 2 && !run->queries[run->stage].id.isEmpty()
        && !_messageIntervalSourceCurrent(run->queries[run->stage].source)) return false;
    const auto row = _fleetCameraStreamRows.constFind(run->componentId * 256 + run->streamId);
    if (run->reports.informationObserved && (row == _fleetCameraStreamRows.constEnd() || !row->informationObserved
        || row->informationRevision != run->reports.informationRevision
        || row->informationReceipt < run->reports.informationReceipt)) return false;
    if (run->reports.statusObserved && (row == _fleetCameraStreamRows.constEnd() || !row->statusObserved
        || row->statusRevision != run->reports.statusRevision || row->statusReceipt < run->reports.statusReceipt
        || row->statusInformationRevision != run->reports.informationRevision)) return false;
    return true;
}

bool Vehicle::_fleetCameraStreamRunAllowed(const std::shared_ptr<FleetCameraStreamRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || !run->active || run->authorizationBusy || !run->authorizationFence
        || !_fleetCameraStreamRunSourceCurrent(run, true)) return false;
    run->authorizationBusy = true;
    const auto release = qScopeGuard([run]() { run->authorizationBusy = false; });
    const auto memoryCurrent = [&]() {
        if (!self || !self->_fleetCameraStreamRunSourceCurrent(run, true) || !run->active || run->stage >= 2
            || run->queries[run->stage].requestIssued || !run->owner) return false;
        const auto& query = run->queries[run->stage];
        const int index = self->_messageCommandIndex(query.id);
        return index >= 0 && self->_mavCommandList.at(index).fleetCameraStreamCaptureRunId == run->id
            && self->_mavCommandList.at(index).command == MAV_CMD_REQUEST_MESSAGE
            && self->_mavCommandList.at(index).targetCompId == run->componentId;
    };
    const auto settled = [&]() {
        if (!memoryCurrent()) return false;
        const QPointer<FleetCameraStreamCapture> owner(run->owner);
        const bool matches = owner && owner->nativeVehicleMatches(self.data(), run->componentId, run->streamId);
        return self && owner && matches && memoryCurrent();
    };
    if (!settled()) return false;
    const QPointer<LinkInterface> primary = run->origin.messageIntervalPrimary;
    const bool connected = primary->isConnected();
    if (!settled() || !primary || !connected) return false;
    const bool mock = primary->isMockLink();
    if (!settled() || !primary || mock) return false;
    const bool replay = primary->isLogReplay();
    if (!settled() || !primary || replay) return false;
    bool authorized = false;
    {
        const auto fence = run->authorizationFence;
        if (!self || !settled()) return false; // A callable copy may reenter.
        authorized = fence && fence();
    }
    return self && authorized && settled(); // Include the callable destructor.
}

std::shared_ptr<FleetCameraStreamRun> Vehicle::prepareFleetCameraStreamCapture(QObject* actualOwner,
    LinkInterface* expectedPrimary, int componentId, int streamId, qint64 remainingMs,
    const std::function<bool()>& authorizationFence)
{
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamCapture> owner(qobject_cast<FleetCameraStreamCapture*>(actualOwner));
    const qint64 entered = _mavCommandQuarantineClock.isValid() ? _mavCommandQuarantineClock.elapsed() : -1;
    const auto previous = _fleetCameraStreamRun;
    if (!owner || owner->thread() != thread() || componentId < MAV_COMP_ID_CAMERA || componentId > MAV_COMP_ID_CAMERA6
        || streamId < 1 || streamId > 255 || remainingMs < 1 || remainingMs > 8000 || !authorizationFence
        || !fleetCameraStreamCaptureTransportQuiescent() || !_fleetCameraStreamRawSourceMatches(expectedPrimary)
        || _fleetCameraStreamSaturated || !QElapsedTimer::isMonotonic() || (previous && previous->active)
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || !_messageCommandLaneId.isEmpty() || _requestMessageContext || _waitForMavlinkMessageResultHandler
        || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)) return {};
    const bool matches = owner->nativeVehicleMatches(this, componentId, streamId);
    if (!self || !owner || !matches) return {};
    if (entered < 0 || entered > std::numeric_limits<qint64>::max() - remainingMs) return {};
    const auto run = std::make_shared<FleetCameraStreamRun>();
    run->vehicle = this; run->owner = owner; run->componentId = componentId; run->streamId = streamId;
    run->id = QUuid::createUuid().toString(QUuid::WithoutBraces); run->sourceEpoch = _fleetCameraStreamEpoch;
    run->mutationGeneration = _fleetCameraStreamMutation;
    run->deadlineMs = entered + remainingMs;
    run->queries[0].messageId = MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION;
    run->queries[1].messageId = MAVLINK_MSG_ID_VIDEO_STREAM_STATUS;
    auto& origin = run->origin;
    origin.command = MAV_CMD_REQUEST_MESSAGE; origin.targetCompId = componentId;
    origin.messageIntervalSourceEpoch = _autopilotVersionObservationEpoch;
    origin.messageCommandTransportEpoch = _messageCommandTransportEpoch;
    origin.messageCommandVehicleId = _id; origin.messageCommandDefaultComponentId = _defaultComponentId;
    origin.messageCommandFirmwareType = int(_firmwareType); origin.messageCommandVehicleType = int(_vehicleType);
    origin.messageIntervalLinks = _fleetExecutionSourceLinks; origin.messageIntervalProtocol = _fleetExecutionSourceProtocol;
    origin.messageIntervalPlugin = _messageIntervalSourcePlugin; origin.messageIntervalPrimary = expectedPrimary;
    origin.messageIntervalPrimaryOwner = _fleetExecutionSourceLinks->primaryLink();
    run->authorizationFence = authorizationFence;
    if (!self || !owner || _fleetCameraStreamRun != previous || (previous && previous->active)
        || !_messageCommandLaneId.isEmpty() || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)) return {};
    const bool stillMatches = owner->nativeVehicleMatches(this, componentId, streamId);
    if (!self || !owner || !stillMatches || _fleetCameraStreamRun != previous || (previous && previous->active)
        || !_messageCommandLaneId.isEmpty() || run->sourceEpoch != _fleetCameraStreamEpoch
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || !_messageCommandSourceMemoryCurrent(origin)) return {};
    _fleetCameraStreamRun = run;
    return run; // No write before the service has adopted this actual Run.
}

void Vehicle::_finishFleetCameraStreamCapture(const std::shared_ptr<FleetCameraStreamRun>& run,
    const QString& state, const QString& reason, bool quarantine)
{
    if (!run || _fleetCameraStreamRun != run || !run->active) return;
    run->active = false; run->state = state; run->reason = reason;
    if (run->stage < 2) {
        auto& query = run->queries[run->stage];
        if (query.state != QStringLiteral("complete")) { query.state = state; query.reason = reason; }
        const int index = _messageCommandIndex(query.id);
        if (index >= 0) _mavCommandList.removeAt(index);
        _releaseMessageCommand(query.id);
    }
    // A completed information request can still have an uncorrelated late
    // COMMAND_ACK while the status stage has not yet issued anything.
    if (quarantine && (run->queries[0].requestIssued || run->queries[1].requestIssued)) {
        _quarantineMavCommand(run->componentId, MAV_CMD_REQUEST_MESSAGE, 4000);
        _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs,
            qMax(run->queries[0].deadlineMs, run->queries[1].deadlineMs));
    }
}

void Vehicle::_completeFleetCameraStreamQuery(const std::shared_ptr<FleetCameraStreamRun>& run)
{
    if (!run || _fleetCameraStreamRun != run || !run->active || run->stage >= 2) return;
    auto& query = run->queries[run->stage];
    if (!query.requestIssued || !query.ackObserved || query.ackResult != MAV_RESULT_ACCEPTED || !query.reportObserved) return;
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamCapture> owner(run->owner);
    const int originalStage = run->stage;
    const QString requestId = query.id;
    const bool responseCurrent = owner && owner->nativeResponseCurrent(run);
    if (!self || !owner || _fleetCameraStreamRun != run || !run->active || run->stage != originalStage
        || run->queries[originalStage].id != requestId) return;
    if (!responseCurrent || !_fleetCameraStreamRunSourceCurrent(run, false)) {
        _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-original-response-scope-expired"), true); return;
    }
    query.state = QStringLiteral("complete"); query.reason = QStringLiteral("");
    const int index = _messageCommandIndex(query.id);
    if (index >= 0) _mavCommandList.removeAt(index);
    _releaseMessageCommand(query.id);
    ++run->stage;
    if (run->stage == 2) {
        run->active = false; run->state = QStringLiteral("complete");
        run->reason = QStringLiteral("matching-reports-observed-not-wire-causality");
    } else run->state = QStringLiteral("ready"); // Next write only from a later service poll.
}

bool Vehicle::advanceFleetCameraStreamCapture(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || run->owner.data() != actualOwner || run->vehicle.data() != this || _fleetCameraStreamRun != run
        || QThread::currentThread() != thread()) return false;
    if (!run->active) return true; // A RAW completion remains readable by its owner.
    if (run->stage >= 2 || run->dispatching) return false;
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    auto& query = run->queries[run->stage];
    if (now < 0 || now >= run->deadlineMs || (query.deadlineMs >= 0 && now >= query.deadlineMs)) {
        _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-deadline-expired"), true); return false;
    }
    if (!fleetCameraStreamCaptureTransportQuiescent()) return true; // Wait only within the original deadlines above.
    if (!_fleetCameraStreamRunSourceCurrent(run, true)) {
        _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-source-changed"), true); return false;
    }
    if (!query.id.isEmpty()) return true;
    run->dispatching = true;
    const auto release = qScopeGuard([run]() { run->dispatching = false; });
    MavCommandListEntry_t entry{};
    entry.command = MAV_CMD_REQUEST_MESSAGE; entry.targetCompId = run->componentId; entry.showError = false;
    entry.maxTries = 1; entry.rgParam1 = query.messageId; entry.rgParam2 = run->streamId;
    entry.fleetCameraStreamCaptureRunId = run->id;
    if (!_reserveMessageCommand(&entry)) {
        _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-message-lane-unavailable"), false); return false;
    }
    const qint64 stageLimit = now > std::numeric_limits<qint64>::max() - 4000
        ? std::numeric_limits<qint64>::max() : now + 4000;
    entry.messageCommandDeadlineMs = qMin(run->deadlineMs, stageLimit);
    _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
    query.id = entry.messageIntervalRequestId; query.deadlineMs = entry.messageCommandDeadlineMs; query.source = entry;
    query.state = QStringLiteral("waiting-ack-and-report");
    run->state = run->stage == 0 ? QStringLiteral("waiting-info") : QStringLiteral("waiting-status");
    entry.messageCommandPrepared = true; query.source.messageCommandPrepared = true;
    _mavCommandList.append(entry);
    _sendMessageCommandFromList(entry.messageIntervalRequestId, nullptr);
    return self && _fleetCameraStreamRun == run; // Include synchronous terminal facts from the actual write.
}

void Vehicle::_observeFleetCameraStreamCapture(const std::shared_ptr<FleetCameraStreamRun>& run, int originalStage,
    const QString& originalRequestId, bool originalIssued, quint64 originalBaseline, const QString& originalEpoch,
    const mavlink_message_t& original, LinkInterface* sourceLink)
{
    if (!run || _fleetCameraStreamRun != run || !run->active || originalStage < 0 || originalStage > 1
        || run->stage != originalStage || !originalIssued || run->queries[originalStage].id != originalRequestId
        || !run->queries[originalStage].requestIssued || run->queries[originalStage].reportObserved
        || run->queries[originalStage].baseline != originalBaseline || originalEpoch != run->sourceEpoch
        || original.msgid != unsigned(run->queries[originalStage].messageId) || original.sysid != _id
        || original.magic != MAVLINK_STX || original.len < 19
        || uint8_t(_MAV_PAYLOAD(&original)[18]) != run->streamId
        || original.compid != run->componentId || sourceLink != run->origin.messageIntervalPrimary.data()
        || sender() != run->origin.messageIntervalProtocol.data() || !_fleetCameraStreamRunSourceCurrent(run, false)) return;
    const auto row = _fleetCameraStreamRows.constFind(run->componentId * 256 + run->streamId);
    if (row == _fleetCameraStreamRows.constEnd()) return;
    const FleetCameraStreamRow received = row.value();
    const quint64 receipt = originalStage == 0 ? received.informationReceipt : received.statusReceipt;
    if (receipt <= originalBaseline || receipt != _fleetCameraStreamNextReceipt
        || (originalStage == 0 ? !received.informationObserved : !received.statusObserved)) return;
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamCapture> owner(run->owner);
    if (!owner || !owner->nativeResponseCurrent(run)) return;
    if (!self || !owner || !_fleetCameraStreamRunSourceCurrent(run, false) || run->stage != originalStage
        || !run->active || run->queries[originalStage].id != originalRequestId) return;
    auto& query = run->queries[originalStage];
    query.reportObserved = true; query.receipt = receipt;
    query.revision = originalStage == 0 ? received.informationRevision : received.statusRevision;
    if (originalStage == 0) {
        run->reports.information = received.information; run->reports.informationObserved = true;
        run->reports.informationValid = received.informationValid; run->reports.informationAtMs = received.informationAtMs;
        run->reports.informationReceipt = receipt; run->reports.informationRevision = received.informationRevision;
        run->reports.configurationRevision = received.configurationRevision;
    } else {
        run->reports.status = received.status; run->reports.statusObserved = true;
        run->reports.statusValid = received.statusValid; run->reports.statusAtMs = received.statusAtMs;
        run->reports.statusReceipt = receipt; run->reports.statusRevision = received.statusRevision;
        run->reports.statusInformationRevision = received.statusInformationRevision;
    }
    if (!(originalStage == 0 ? received.informationValid : received.statusValid)) {
        _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-invalid-reported-values"), true); return;
    }
    _completeFleetCameraStreamQuery(run);
}

QVariantMap Vehicle::_fleetCameraStreamCaptureValue(const std::shared_ptr<FleetCameraStreamRun>& run,
    bool current, qint64 now, qint64 sampleNow, bool retired)
{
    QVariantMap queries[2];
    for (int i = 0; i < 2; ++i) {
        const auto& query = run->queries[i];
        const bool terminal = query.state == QStringLiteral("complete") || query.state == QStringLiteral("failed")
            || query.state == QStringLiteral("cancelled");
        queries[i] = {{QStringLiteral("requestId"), query.id}, {QStringLiteral("messageId"), query.messageId},
            {QStringLiteral("requestIssued"), query.requestIssued}, {QStringLiteral("ackObserved"), query.ackObserved},
            {QStringLiteral("ackResult"), query.ackResult}, {QStringLiteral("reportObserved"), query.reportObserved},
            {QStringLiteral("baselineReceiptSequence"), QString::number(query.baseline)},
            {QStringLiteral("receiptSequence"), QString::number(query.receipt)}, {QStringLiteral("contentRevision"), QString::number(query.revision)},
            {QStringLiteral("remainingMs"), QVariant::fromValue(!retired && run->active && !terminal && query.deadlineMs >= 0
                && now >= 0 ? qMax(qint64(0), query.deadlineMs - now) : qint64(0))},
            {QStringLiteral("state"), retired && !terminal ? QStringLiteral("failed") : query.state},
            {QStringLiteral("reason"), retired && !terminal ? QStringLiteral("camera-capture-vehicle-destroyed") : query.reason}};
    }
    const QVariantMap samples = _fleetCameraStreamRowValue(run->componentId, run->streamId, run->reports, sampleNow, 3000, current, false);
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-camera-stream-capture")},
        {QStringLiteral("requestId"), run->id}, {QStringLiteral("vehicleId"), run->origin.messageCommandVehicleId},
        {QStringLiteral("componentId"), run->componentId}, {QStringLiteral("streamId"), run->streamId},
        {QStringLiteral("active"), !retired && run->active},
        {QStringLiteral("state"), retired && run->active ? QStringLiteral("failed") : run->state},
        {QStringLiteral("reason"), retired ? QStringLiteral("camera-capture-vehicle-destroyed") : run->reason},
        {QStringLiteral("sourceCurrent"), current},
        {QStringLiteral("remainingMs"), QVariant::fromValue(!retired && run->active && now >= 0 ? qMax(qint64(0), run->deadlineMs - now) : qint64(0))},
        {QStringLiteral("stage"), run->stage}, {QStringLiteral("informationQuery"), queries[0]}, {QStringLiteral("statusQuery"), queries[1]},
        {QStringLiteral("information"), samples.value(QStringLiteral("information"))}, {QStringLiteral("status"), samples.value(QStringLiteral("status"))},
        {QStringLiteral("sourceEpoch"), run->sourceEpoch}, {QStringLiteral("transportEpoch"), run->origin.messageCommandTransportEpoch},
        {QStringLiteral("versionEpoch"), run->origin.messageIntervalSourceEpoch},
        {QStringLiteral("receiptSequence"), QString::number(qMax(run->queries[0].receipt, run->queries[1].receipt))},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("wireCausalityProven"), false},
        {QStringLiteral("catalogComplete"), false}, {QStringLiteral("remoteRunningVerified"), false}, {QStringLiteral("remoteStoppedVerified"), false},
        {QStringLiteral("availableBandwidthVerified"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap Vehicle::fleetCameraStreamCaptureSnapshot(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run) const
{
    if (!run || run->owner.data() != actualOwner || run->vehicle.data() != this || QThread::currentThread() != thread()) return {};
    return _fleetCameraStreamCaptureValue(run, _fleetCameraStreamRunSourceCurrent(run, true),
        _mavCommandQuarantineClock.elapsed(), _fleetCameraStreamClock.elapsed(), false);
}

QVariantMap Vehicle::fleetCameraStreamCaptureRetiredSnapshot(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run)
{
    if (!run || run->vehicle || !run->owner || run->owner.data() != actualOwner || QThread::currentThread() != run->owner->thread()) return {};
    return _fleetCameraStreamCaptureValue(run, false, -1, -1, true);
}

bool Vehicle::cancelFleetCameraStreamCapture(QObject* actualOwner, const std::shared_ptr<FleetCameraStreamRun>& run)
{
    if (!run || run->owner.data() != actualOwner || run->vehicle.data() != this || QThread::currentThread() != thread()) return false;
    if (_fleetCameraStreamRun == run && run->active)
        _finishFleetCameraStreamCapture(run, QStringLiteral("cancelled"), QStringLiteral("camera-capture-cancelled-no-retract"), true);
    return true;
}

bool Vehicle::_fleetCameraStreamControlBasisMemoryCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis,
    quint64 expectedMutation, bool requireOriginalStatus) const
{
    if (!basis || basis->vehicle.data() != this || !basis->captureOwner || QThread::currentThread() != thread()
        || basis->captureOwner->thread() != thread() || _fleetCameraStreamMutationExhausted
        || expectedMutation != _fleetCameraStreamMutation || basis->sourceEpoch != _fleetCameraStreamEpoch
        || basis->origin.messageIntervalSourceEpoch != _autopilotVersionObservationEpoch
        || !_messageCommandSourceMemoryCurrent(basis->origin)
        || !_fleetCameraStreamRawSourceMatches(basis->origin.messageIntervalPrimary.data()) || _fleetCameraStreamSaturated
        || !_mavCommandQuarantineClock.isValid() || _mavCommandQuarantineClock.elapsed() < 0
        || _mavCommandQuarantineClock.elapsed() >= basis->deadlineMs) return false;
    const auto row = _fleetCameraStreamRows.constFind(basis->origin.targetCompId * 256 + int(basis->reports.information.stream_id));
    return row != _fleetCameraStreamRows.constEnd() && row->informationObserved && row->informationValid
        && basis->reports.configurationRevision != 0 && row->configurationRevision == basis->reports.configurationRevision
        && row->informationReceipt >= basis->reports.informationReceipt
        && (!requireOriginalStatus || (row->informationRevision == basis->reports.informationRevision
            && row->statusObserved && row->statusValid
            && row->statusRevision == basis->reports.statusRevision && row->statusReceipt >= basis->reports.statusReceipt
            && row->statusInformationRevision == basis->reports.informationRevision));
}

bool Vehicle::_fleetCameraStreamControlOriginCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    if (!basis || !basis->sourceFence || basis->checkingSource || !basis->captureOwner || basis->vehicle.data() != this) return false;
    basis->checkingSource = true;
    const auto release = qScopeGuard([basis]() { basis->checkingSource = false; });
    const QPointer<FleetCameraStreamCapture> capture(basis->captureOwner);
    bool current = false;
    {
        const auto fence = basis->sourceFence;
        if (!self || !capture) return false;
        current = fence && fence();
    }
    return self && capture && basis->captureOwner == capture && current;
}

std::shared_ptr<FleetCameraStreamControlBasis> Vehicle::sealFleetCameraStreamControlBasis(QObject* actualCapture,
    const std::shared_ptr<FleetCameraStreamRun>& capture, bool targetRunning, qint64 remainingMs,
    const std::function<bool()>& pureSourceFence)
{
    const QPointer<Vehicle> self(this);
    const qint64 now = _mavCommandQuarantineClock.isValid() ? _mavCommandQuarantineClock.elapsed() : -1;
    const qint64 sampleNow = _fleetCameraStreamClock.isValid() ? _fleetCameraStreamClock.elapsed() : -1;
    if (!capture || !capture->owner || capture->owner.data() != actualCapture || capture->controlSealed
        || capture->active || capture->state != QStringLiteral("complete") || capture->stage != 2
        || remainingMs < 1 || remainingMs > 30000 || !pureSourceFence || now < 0
        || now > std::numeric_limits<qint64>::max() - remainingMs
        || !fleetCameraStreamCaptureTransportQuiescent() || !_fleetCameraStreamRunSourceCurrent(capture, true)
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)) return {};
    for (const auto& query : capture->queries)
        if (!query.requestIssued || !query.ackObserved || query.ackResult != MAV_RESULT_ACCEPTED || !query.reportObserved
            || query.state != QStringLiteral("complete")) return {};
    const auto& reports = capture->reports;
    if (!reports.informationObserved || !reports.statusObserved || !reports.informationValid || !reports.statusValid
        || reports.configurationRevision == 0
        || reports.statusInformationRevision != reports.informationRevision || sampleNow < reports.informationAtMs
        || sampleNow < reports.statusAtMs || reports.informationAtMs < 0 || reports.statusAtMs < 0
        || sampleNow - reports.informationAtMs > 3000 || sampleNow - reports.statusAtMs > 3000) return {};
    const bool originalRunning = (reports.status.flags & VIDEO_STREAM_STATUS_FLAGS_RUNNING) != 0;
    if (originalRunning == targetRunning) return {}; // An explicit opposite reported target only.
    capture->controlSealed = true; // Never unburn a native capture after an external callable.
    const auto basis = std::make_shared<FleetCameraStreamControlBasis>();
    basis->vehicle = this; basis->captureOwner = capture->owner; basis->origin = capture->origin; basis->reports = reports;
    basis->id = QUuid::createUuid().toString(QUuid::WithoutBraces); basis->captureId = capture->id;
    basis->sourceEpoch = capture->sourceEpoch; basis->mutation = capture->mutationGeneration;
    basis->deadlineMs = now + remainingMs; basis->originalRunning = originalRunning; basis->targetRunning = targetRunning;
    basis->sourceFence = pureSourceFence;
    if (!self || !_fleetCameraStreamRunSourceCurrent(capture, true)
        || !_fleetCameraStreamControlOriginCurrent(basis) || !self || !capture->owner
        || !_fleetCameraStreamControlBasisMemoryCurrent(basis, basis->mutation, true)) return {};
    return basis;
}

bool Vehicle::fleetCameraStreamControlBasisCurrent(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    return basis && !basis->consumed && _fleetCameraStreamControlBasisMemoryCurrent(basis, basis->mutation, true)
        && _fleetCameraStreamControlOriginCurrent(basis) && self
        && _fleetCameraStreamControlBasisMemoryCurrent(basis, basis->mutation, true);
}

QVariantMap Vehicle::fleetCameraStreamControlBasisSummary(const std::shared_ptr<FleetCameraStreamControlBasis>& basis) const
{
    if (!basis || basis->vehicle.data() != this || QThread::currentThread() != thread()) return {};
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-camera-stream-control-basis")},
        {QStringLiteral("basisId"), basis->id}, {QStringLiteral("captureRequestId"), basis->captureId},
        {QStringLiteral("vehicleId"), basis->origin.messageCommandVehicleId}, {QStringLiteral("componentId"), basis->origin.targetCompId},
        {QStringLiteral("streamId"), int(basis->reports.information.stream_id)}, {QStringLiteral("sourceEpoch"), basis->sourceEpoch},
        {QStringLiteral("transportEpoch"), basis->origin.messageCommandTransportEpoch}, {QStringLiteral("versionEpoch"), basis->origin.messageIntervalSourceEpoch},
        {QStringLiteral("mutationGeneration"), QString::number(basis->mutation)},
        {QStringLiteral("informationRevision"), QString::number(basis->reports.informationRevision)},
        {QStringLiteral("statusRevision"), QString::number(basis->reports.statusRevision)},
        {QStringLiteral("originalRunning"), basis->originalRunning}, {QStringLiteral("targetRunning"), basis->targetRunning},
        {QStringLiteral("remainingMs"), QVariant::fromValue(now >= 0 ? qMax(qint64(0), basis->deadlineMs - now) : qint64(0))},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

std::shared_ptr<FleetCameraStreamControlRun> Vehicle::startFleetCameraStreamControl(QObject* actualControl,
    const std::shared_ptr<FleetCameraStreamControlBasis>& basis, const std::function<bool()>& actualTwoAccountFinalFence)
{
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamControl> owner(qobject_cast<FleetCameraStreamControl*>(actualControl));
    const auto previous = _fleetCameraStreamControlRun;
    if (!owner || owner->thread() != thread() || !actualTwoAccountFinalFence || !fleetCameraStreamCaptureTransportQuiescent()
        || (previous && previous->active) || !_messageCommandLaneId.isEmpty() || _requestMessageContext
        || _waitForMavlinkMessageResultHandler || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
        || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)
        || !fleetCameraStreamControlBasisCurrent(basis) || !self || !owner) return {};
    for (const auto& entry : _mavCommandList)
        if (messageCommand(entry.command) || cameraStreamMutationCommand(entry.command)) return {};
    const bool matches = owner->nativeVehicleMatches(this, basis->origin.targetCompId, int(basis->reports.information.stream_id));
    if (!self || !owner || !matches || !fleetCameraStreamControlBasisCurrent(basis) || !self || !owner) return {};
    if (_isMavCommandQuarantined(basis->origin.targetCompId, MAV_CMD_REQUEST_MESSAGE)
        || _isMavCommandQuarantined(basis->origin.targetCompId,
            basis->targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING)) return {};
    basis->consumed = true;
    const auto run = std::make_shared<FleetCameraStreamControlRun>();
    run->vehicle = this; run->owner = owner; run->basis = basis; run->expectedMutation = basis->mutation;
    run->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    run->reports.information = basis->reports.information; run->reports.informationObserved = true;
    run->reports.informationValid = true; run->reports.informationAtMs = basis->reports.informationAtMs;
    run->reports.informationReceipt = basis->reports.informationReceipt; run->reports.informationRevision = basis->reports.informationRevision;
    run->reports.configurationRevision = basis->reports.configurationRevision;
    run->finalFence = actualTwoAccountFinalFence;
    if (!self || !owner || _fleetCameraStreamControlRun != previous || (previous && previous->active)
        || !_messageCommandLaneId.isEmpty() || !_fleetCameraStreamControlBasisMemoryCurrent(basis, basis->mutation, true)
        || !_fleetCameraStreamControlOriginCurrent(basis) || !self || !owner
        || _fleetCameraStreamControlRun != previous || (previous && previous->active) || !_messageCommandLaneId.isEmpty()
        || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
        || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)
        || _requestMessageContext || _waitForMavlinkMessageResultHandler
        || !_fleetCameraStreamControlBasisMemoryCurrent(basis, basis->mutation, true)) return {};
    for (const auto& entry : _mavCommandList)
        if (messageCommand(entry.command) || cameraStreamMutationCommand(entry.command)) return {};
    _fleetCameraStreamControlRun = run;
    return run; // Explicit owner adoption before a later advance may write.
}

bool Vehicle::_fleetCameraStreamControlSourceCurrent(const std::shared_ptr<FleetCameraStreamControlRun>& run, bool requireQuiescent) const
{
    if (!run || !run->owner || run->vehicle.data() != this || _fleetCameraStreamControlRun != run || run->sourceRetired
        || run->owner->thread() != thread() || (requireQuiescent && !fleetCameraStreamCaptureTransportQuiescent())
        || !_fleetCameraStreamControlBasisMemoryCurrent(run->basis, run->expectedMutation, !run->actionIssued)) return false;
    if (run->active && !run->actionId.isEmpty() && run->stage == 0 && !_messageIntervalSourceCurrent(run->actionSource)) return false;
    if (run->active && !run->queryId.isEmpty() && run->stage == 1 && !_messageIntervalSourceCurrent(run->querySource)) return false;
    // Current source/configuration is separate from the fixed reported pair.
    // An active query may wait for a new actual Status after a RUNNING-only
    // Info update; a committed pair remains historical evidence, not a claim
    // that later operating-state reports cannot change.
    if (run->reportObserved && (run->reports.configurationRevision != run->basis->reports.configurationRevision
        || run->reports.statusInformationRevision != run->reports.informationRevision)) return false;
    return true;
}

bool Vehicle::_fleetCameraStreamControlAllowed(const std::shared_ptr<FleetCameraStreamControlRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || !run->active || run->authorizationBusy || !run->finalFence || !_fleetCameraStreamControlSourceCurrent(run, true)) return false;
    run->authorizationBusy = true;
    const auto release = qScopeGuard([run]() { run->authorizationBusy = false; });
    const auto memory = [&]() {
        if (!self || !run->owner || !run->active || !self->_fleetCameraStreamControlSourceCurrent(run, true) || run->stage > 1) return false;
        const QString id = run->stage == 0 ? run->actionId : run->queryId;
        const int index = self->_messageCommandIndex(id);
        return !(run->stage == 0 ? run->actionIssued : run->queryIssued) && index >= 0
            && self->_mavCommandList.at(index).fleetCameraStreamControlRunId == run->id;
    };
    const auto settled = [&]() {
        if (!memory()) return false;
        const QPointer<FleetCameraStreamControl> owner(run->owner);
        const bool matches = owner->nativeVehicleMatches(self.data(), run->basis->origin.targetCompId, int(run->reports.information.stream_id));
        return self && owner && matches && memory();
    };
    if (!settled() || !_fleetCameraStreamControlOriginCurrent(run->basis) || !settled()) return false;
    const QPointer<LinkInterface> primary(run->basis->origin.messageIntervalPrimary);
    const bool connected = primary->isConnected();
    if (!settled() || !primary || !connected) return false;
    const bool mock = primary->isMockLink();
    if (!settled() || !primary || mock) return false;
    const bool replay = primary->isLogReplay();
    if (!settled() || !primary || replay) return false;
    bool authorized = false;
    {
        const auto fence = run->finalFence;
        if (!self || !settled()) return false;
        authorized = fence && fence();
    }
    return self && authorized && settled() && _fleetCameraStreamControlOriginCurrent(run->basis) && settled();
}

bool Vehicle::_advanceFleetCameraStreamMutation(const std::shared_ptr<FleetCameraStreamControlRun>& ownRun)
{
    if (QThread::currentThread() != thread() || _fleetCameraStreamMutationExhausted
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active && _fleetCameraStreamControlRun != ownRun)) return false;
    if (_fleetCameraStreamMutation == std::numeric_limits<quint64>::max()) {
        _fleetCameraStreamMutationExhausted = true;
        _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-mutation-exhausted")); return false;
    }
    ++_fleetCameraStreamMutation;
    if (ownRun) ownRun->expectedMutation = _fleetCameraStreamMutation;
    // Preserve the original information identity/age. Old reported status is
    // not a post-write observation, nor is an old Info cache a new receipt.
    for (auto row = _fleetCameraStreamRows.begin(); row != _fleetCameraStreamRows.end();) {
        if (!row->informationObserved) { row = _fleetCameraStreamRows.erase(row); continue; }
        row->status = mavlink_video_stream_status_t{}; row->statusObserved = row->statusValid = false;
        row->statusAtMs = -1; row->statusReceipt = row->statusRevision = row->statusInformationRevision = 0;
        ++row;
    }
    return true;
}

void Vehicle::_finishFleetCameraStreamControl(const std::shared_ptr<FleetCameraStreamControlRun>& run,
    const QString& state, const QString& reason, bool quarantine)
{
    if (!run || _fleetCameraStreamControlRun != run || !run->active) return;
    run->active = false; run->state = state; run->reason = reason;
    if (run->stage == 0 && run->actionState != QStringLiteral("complete")) { run->actionState = state; run->actionReason = reason; }
    if (run->stage == 1 && run->queryState != QStringLiteral("complete")) { run->queryState = state; run->queryReason = reason; }
    const QString id = run->stage == 0 ? run->actionId : run->queryId;
    const int index = _messageCommandIndex(id);
    if (index >= 0) _mavCommandList.removeAt(index);
    _releaseMessageCommand(id);
    if (quarantine && (run->actionIssued || run->queryIssued)) {
        if (run->actionIssued) _quarantineMavCommand(run->basis->origin.targetCompId,
            run->basis->targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING, 4000);
        if (run->queryIssued) _quarantineMavCommand(run->basis->origin.targetCompId, MAV_CMD_REQUEST_MESSAGE, 4000);
        const qint64 now = _mavCommandQuarantineClock.elapsed();
        const qint64 horizon = now > std::numeric_limits<qint64>::max() - 4000 ? std::numeric_limits<qint64>::max() : now + 4000;
        _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, qMax(horizon, run->wireDeadlineMs));
    }
}

void Vehicle::_completeFleetCameraStreamControlQuery(const std::shared_ptr<FleetCameraStreamControlRun>& run)
{
    if (!run || _fleetCameraStreamControlRun != run || !run->active || run->stage != 1 || !run->actionAckObserved
        || run->actionAckResult != MAV_RESULT_ACCEPTED || !run->queryIssued || !run->queryAckObserved
        || run->queryAckResult != MAV_RESULT_ACCEPTED || !run->reportObserved) return;
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamControl> owner(run->owner);
    const bool responseCurrent = owner && owner->nativeResponseCurrent(run);
    if (!self || !owner || _fleetCameraStreamControlRun != run || !run->active || run->stage != 1) return;
    if (!responseCurrent || !_fleetCameraStreamControlSourceCurrent(run, false)) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-original-response-scope-expired"), true); return;
    }
    const auto row = _fleetCameraStreamRows.constFind(run->basis->origin.targetCompId * 256 + int(run->reports.information.stream_id));
    if (row == _fleetCameraStreamRows.constEnd() || !row->informationObserved || !row->informationValid
        || !row->statusObserved || !row->statusValid || row->configurationRevision != run->basis->reports.configurationRevision
        || row->informationRevision != run->reports.informationRevision || row->statusInformationRevision != row->informationRevision
        || row->statusRevision != run->revision || row->statusReceipt < run->receipt) return;
    const bool matches = ((run->reports.status.flags & VIDEO_STREAM_STATUS_FLAGS_RUNNING) != 0) == run->basis->targetRunning;
    if (!matches) { _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-reported-target-not-matched"), true); return; }
    const int index = _messageCommandIndex(run->queryId);
    if (index >= 0) _mavCommandList.removeAt(index);
    _releaseMessageCommand(run->queryId);
    run->queryState = QStringLiteral("complete"); run->queryReason = QStringLiteral("");
    run->stage = 2; run->active = false; run->state = QStringLiteral("reported-target-match");
    run->reason = QStringLiteral("reported-target-match-not-physical-or-wire-causality");
}

bool Vehicle::advanceFleetCameraStreamControl(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || run->owner.data() != actualControl || run->vehicle.data() != this || _fleetCameraStreamControlRun != run
        || QThread::currentThread() != thread()) return false;
    if (!run->active) return true;
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    if (now < 0 || now >= run->basis->deadlineMs || (run->wireDeadlineMs >= 0 && now >= run->wireDeadlineMs)) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-original-deadline-expired"), true); return false;
    }
    if (!fleetCameraStreamCaptureTransportQuiescent()) return true;
    if (!_fleetCameraStreamControlSourceCurrent(run, true)) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-source-changed"), true); return false;
    }
    if (run->dispatching || run->stage > 1) return false;
    if (!(run->stage == 0 ? run->actionId : run->queryId).isEmpty()) return true;
    if (run->stage == 0 && _isMavCommandQuarantined(run->basis->origin.targetCompId, MAV_CMD_REQUEST_MESSAGE)) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-readback-lane-quarantined"), false); return false;
    }
    run->dispatching = true;
    const auto release = qScopeGuard([run]() { run->dispatching = false; });
    MavCommandListEntry_t entry{};
    entry.targetCompId = run->basis->origin.targetCompId; entry.showError = false; entry.maxTries = 1;
    entry.fleetCameraStreamControlRunId = run->id;
    if (run->stage == 0) {
        entry.command = run->basis->targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING;
        entry.rgParam1 = run->reports.information.stream_id;
    } else {
        entry.command = MAV_CMD_REQUEST_MESSAGE; entry.rgParam1 = MAVLINK_MSG_ID_VIDEO_STREAM_STATUS;
        entry.rgParam2 = run->reports.information.stream_id;
    }
    if (!_reserveMessageCommand(&entry)) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-message-lane-unavailable"), true); return false;
    }
    const qint64 limit = now > std::numeric_limits<qint64>::max() - 4000 ? std::numeric_limits<qint64>::max() : now + 4000;
    entry.messageCommandDeadlineMs = qMin(run->basis->deadlineMs, limit);
    _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs; run->wireDeadlineMs = entry.messageCommandDeadlineMs;
    entry.messageCommandPrepared = true;
    if (run->stage == 0) {
        run->actionId = entry.messageIntervalRequestId; run->actionSource = entry;
        run->actionState = QStringLiteral("waiting-ack"); run->state = QStringLiteral("waiting-action");
    } else {
        run->queryId = entry.messageIntervalRequestId; run->querySource = entry;
        run->queryState = QStringLiteral("waiting-ack-and-report"); run->state = QStringLiteral("waiting-status");
    }
    _mavCommandList.append(entry);
    _sendMessageCommandFromList(entry.messageIntervalRequestId, nullptr);
    return self && _fleetCameraStreamControlRun == run;
}

void Vehicle::_observeFleetCameraStreamControl(const std::shared_ptr<FleetCameraStreamControlRun>& run,
    const QString& originalRequestId, bool originalIssued, quint64 originalBaseline,
    const mavlink_message_t& original, LinkInterface* sourceLink)
{
    if (!run || _fleetCameraStreamControlRun != run || !run->active || run->stage != 1 || !originalIssued
        || !run->queryIssued || run->queryId != originalRequestId || run->baseline != originalBaseline
        || original.msgid != MAVLINK_MSG_ID_VIDEO_STREAM_STATUS || original.magic != MAVLINK_STX || original.len < 19
        || original.sysid != run->basis->origin.messageCommandVehicleId || original.compid != run->basis->origin.targetCompId
        || uint8_t(_MAV_PAYLOAD(&original)[18]) != run->reports.information.stream_id
        || sourceLink != run->basis->origin.messageIntervalPrimary.data() || sender() != run->basis->origin.messageIntervalProtocol.data()
        || !_fleetCameraStreamControlSourceCurrent(run, false)) return;
    const auto row = _fleetCameraStreamRows.constFind(original.compid * 256 + int(run->reports.information.stream_id));
    if (row == _fleetCameraStreamRows.constEnd() || !row->statusObserved
        || !row->informationObserved || !row->informationValid
        || row->configurationRevision != run->basis->reports.configurationRevision
        || row->statusReceipt <= originalBaseline || row->statusReceipt != _fleetCameraStreamNextReceipt
        || row->statusInformationRevision != row->informationRevision) return;
    const FleetCameraStreamRow received = row.value();
    const QPointer<Vehicle> self(this);
    const QPointer<FleetCameraStreamControl> owner(run->owner);
    const bool responseCurrent = owner && owner->nativeResponseCurrent(run);
    if (!self || !owner || !responseCurrent || !_fleetCameraStreamControlSourceCurrent(run, false)
        || !run->active || run->stage != 1 || run->queryId != originalRequestId) return;
    run->reportObserved = true; run->receipt = received.statusReceipt; run->revision = received.statusRevision;
    // Preserve both real receipts/times. Never rebind an old Status to a new
    // Info revision or refresh Info's age just because this Status arrived.
    run->reports = received;
    if (!received.statusValid) {
        _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-invalid-reported-values"), true); return;
    }
    _completeFleetCameraStreamControlQuery(run);
}

QVariantMap Vehicle::_fleetCameraStreamControlValue(const std::shared_ptr<FleetCameraStreamControlRun>& run,
    bool current, qint64 now, qint64 sampleNow, bool retired)
{
    const auto basis = run->basis;
    const bool actionTerminal = run->actionState == QStringLiteral("complete") || run->actionState == QStringLiteral("failed") || run->actionState == QStringLiteral("cancelled");
    const bool queryTerminal = run->queryState == QStringLiteral("complete") || run->queryState == QStringLiteral("failed") || run->queryState == QStringLiteral("cancelled");
    const qint64 remaining = !retired && run->active && now >= 0 && run->wireDeadlineMs >= 0 ? qMax(qint64(0), run->wireDeadlineMs - now) : 0;
    const QVariantMap action{{QStringLiteral("requestId"), run->actionId},
        {QStringLiteral("command"), int(basis->targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING)},
        {QStringLiteral("requestIssued"), run->actionIssued}, {QStringLiteral("ackObserved"), run->actionAckObserved},
        {QStringLiteral("ackResult"), run->actionAckResult}, {QStringLiteral("state"), retired && !actionTerminal ? QStringLiteral("failed") : run->actionState},
        {QStringLiteral("reason"), retired && !actionTerminal ? QStringLiteral("camera-control-vehicle-destroyed") : run->actionReason},
        {QStringLiteral("remainingMs"), QVariant::fromValue(run->stage == 0 && !actionTerminal ? remaining : qint64(0))}};
    const QVariantMap query{{QStringLiteral("requestId"), run->queryId}, {QStringLiteral("messageId"), int(MAVLINK_MSG_ID_VIDEO_STREAM_STATUS)},
        {QStringLiteral("requestIssued"), run->queryIssued}, {QStringLiteral("ackObserved"), run->queryAckObserved}, {QStringLiteral("ackResult"), run->queryAckResult},
        {QStringLiteral("reportObserved"), run->reportObserved}, {QStringLiteral("baselineReceiptSequence"), QString::number(run->baseline)},
        {QStringLiteral("receiptSequence"), QString::number(run->receipt)}, {QStringLiteral("contentRevision"), QString::number(run->revision)},
        {QStringLiteral("remainingMs"), QVariant::fromValue(run->stage == 1 && !queryTerminal ? remaining : qint64(0))},
        {QStringLiteral("state"), retired && !queryTerminal ? QStringLiteral("failed") : run->queryState},
        {QStringLiteral("reason"), retired && !queryTerminal ? QStringLiteral("camera-control-vehicle-destroyed") : run->queryReason}};
    const QVariantMap samples = _fleetCameraStreamRowValue(basis->origin.targetCompId, int(run->reports.information.stream_id), run->reports,
        sampleNow, 3000, current, false);
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-camera-stream-control")},
        {QStringLiteral("requestId"), run->id}, {QStringLiteral("basisId"), basis->id}, {QStringLiteral("captureRequestId"), basis->captureId},
        {QStringLiteral("vehicleId"), basis->origin.messageCommandVehicleId}, {QStringLiteral("componentId"), basis->origin.targetCompId},
        {QStringLiteral("streamId"), int(run->reports.information.stream_id)}, {QStringLiteral("targetRunning"), basis->targetRunning},
        {QStringLiteral("originalRunning"), basis->originalRunning}, {QStringLiteral("active"), !retired && run->active},
        {QStringLiteral("state"), retired && run->active ? QStringLiteral("failed") : run->state},
        {QStringLiteral("reason"), retired ? QStringLiteral("camera-control-vehicle-destroyed") : run->reason}, {QStringLiteral("sourceCurrent"), current},
        {QStringLiteral("remainingMs"), QVariant::fromValue(!retired && run->active && now >= 0 ? qMax(qint64(0), basis->deadlineMs - now) : qint64(0))},
        {QStringLiteral("stage"), run->stage}, {QStringLiteral("mutationGeneration"), QString::number(run->expectedMutation)},
        {QStringLiteral("action"), action}, {QStringLiteral("statusQuery"), query},
        {QStringLiteral("information"), samples.value(QStringLiteral("information"))}, {QStringLiteral("status"), samples.value(QStringLiteral("status"))},
        {QStringLiteral("sourceEpoch"), basis->sourceEpoch}, {QStringLiteral("transportEpoch"), basis->origin.messageCommandTransportEpoch},
        {QStringLiteral("versionEpoch"), basis->origin.messageIntervalSourceEpoch}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("wireCausalityProven"), false}, {QStringLiteral("remoteRunningVerified"), false},
        {QStringLiteral("remoteStoppedVerified"), false}, {QStringLiteral("availableBandwidthVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("reportedTargetMatch"), !retired && current && run->state == QStringLiteral("reported-target-match") && run->stage == 2}};
}

QVariantMap Vehicle::fleetCameraStreamControlSnapshot(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run) const
{
    if (!run || run->owner.data() != actualControl || run->vehicle.data() != this || QThread::currentThread() != thread()) return {};
    return _fleetCameraStreamControlValue(run, _fleetCameraStreamControlSourceCurrent(run, true),
        _mavCommandQuarantineClock.elapsed(), _fleetCameraStreamClock.elapsed(), false);
}

QVariantMap Vehicle::fleetCameraStreamControlRetiredSnapshot(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run)
{
    if (!run || run->vehicle || !run->owner || run->owner.data() != actualControl || QThread::currentThread() != run->owner->thread()) return {};
    return _fleetCameraStreamControlValue(run, false, -1, -1, true);
}

bool Vehicle::cancelFleetCameraStreamControl(QObject* actualControl, const std::shared_ptr<FleetCameraStreamControlRun>& run)
{
    if (!run || run->owner.data() != actualControl || run->vehicle.data() != this || QThread::currentThread() != thread()) return false;
    if (_fleetCameraStreamControlRun == run && run->active)
        _finishFleetCameraStreamControl(run, QStringLiteral("cancelled"), QStringLiteral("camera-control-cancelled-no-rollback"), true);
    return true;
}

void Vehicle::_initializeFleetLocalReceptionObservation()
{
    _fleetLocalReceptionClock.start();
    _fleetLocalReceptionEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Vehicle::_invalidateFleetLocalReceptionObservation(const QString& reason)
{
    // Pure memory only. Neither a source break nor a later bind restarts the
    // receipt counter; an old receive/probe guard must unwind its own stack.
    _fleetLocalReceptionEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetLocalReceptionFrames.clear();
    _fleetLocalReceptionPrimary.clear();
    _fleetLocalReceptionPrimaryOwner.reset();
    _fleetLocalReceptionTransportEpoch.clear();
    _fleetLocalReceptionVersionEpoch.clear();
    _fleetLocalReceptionBoundAtMs = _fleetLocalReceptionNewestAtMs = _fleetLocalReceptionLastDroppedAtMs = -1;
    _fleetLocalReceptionVehicleId = _fleetLocalReceptionComponentId = 0;
    _fleetLocalReceptionFirmwareType = _fleetLocalReceptionVehicleType = -1;
    _fleetLocalReceptionReason = reason;
}

bool Vehicle::_fleetLocalReceptionRawSourceMatches(LinkInterface* sourceLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || qgcApp()->thread() != thread()
        || _offlineEditingVehicle || _fleetLocalReceptionRemoving || _fleetLocalReceptionExhausted
        || _autopilotVersionObservationRemoving || _fleetMessageIntervalRawReceiveExhausted
        || !_fleetLocalReceptionClock.isValid() || !QElapsedTimer::isMonotonic()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255 || !sourceLink
        || _messageCommandTransportEpoch.isEmpty() || _autopilotVersionObservationEpoch.isEmpty()
        || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
        || !_fleetExecutionSourceVehicles || !_fleetExecutionSourceToolbox
        || _fleetExecutionSourceLinks->thread() != thread() || _fleetExecutionSourceProtocol->thread() != thread()
        || _fleetExecutionSourceVehicles->thread() != thread() || _fleetExecutionSourceToolbox->thread() != thread()
        || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || _fleetExecutionSourceToolbox->mavlinkProtocol() != _fleetExecutionSourceProtocol.data()
        || _fleetExecutionSourceToolbox->multiVehicleManager() != _fleetExecutionSourceVehicles.data()
        || _fleetExecutionSourceVehicles->getVehicleById(_id) != this || _fleetExecutionSourceLinks->communicationLost()) return false;
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (primary.expired()) return false;
    // Inspect the actual retained member. No temporary shared owner, virtual
    // Link access or secondary-link fallback belongs in the pure getter.
    for (const auto& member : _fleetExecutionSourceLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(member.link);
        if (member.link && member.link.get() == sourceLink && !primary.owner_before(candidate)
            && !candidate.owner_before(primary)) return true;
    }
    return false;
}

Vehicle::FleetLocalReceptionIngress Vehicle::_prepareFleetLocalReceptionIngress(
    const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    FleetLocalReceptionIngress ingress;
    if (sender() != _fleetExecutionSourceProtocol.data() || original.sysid != _id || original.compid != _defaultComponentId
        || !_fleetLocalReceptionRawSourceMatches(sourceLink)) return ingress;
    if (_fleetMessageIntervalRawReceiveDepth != 1 || _fleetLocalReceptionProbeBusy) {
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-nested-raw"));
        return ingress;
    }
    if (receivedAtMs < 0 || (original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)) {
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-invalid-frame-or-clock"));
        return ingress;
    }
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (_fleetLocalReceptionBoundAtMs >= 0 && (_fleetLocalReceptionPrimary.data() != sourceLink
        || _fleetLocalReceptionTransportEpoch != _messageCommandTransportEpoch
        || _fleetLocalReceptionVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetLocalReceptionVehicleId != _id || _fleetLocalReceptionComponentId != _defaultComponentId
        || _fleetLocalReceptionFirmwareType != int(_firmwareType) || _fleetLocalReceptionVehicleType != int(_vehicleType)
        || _fleetLocalReceptionPrimaryOwner.expired() || primary.owner_before(_fleetLocalReceptionPrimaryOwner)
        || _fleetLocalReceptionPrimaryOwner.owner_before(primary)))
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-source-binding-changed"));
    ingress.primary = sourceLink; ingress.primaryOwner = primary;
    ingress.links = _fleetExecutionSourceLinks; ingress.protocol = _fleetExecutionSourceProtocol;
    ingress.vehicles = _fleetExecutionSourceVehicles; ingress.toolbox = _fleetExecutionSourceToolbox;
    ingress.sourceEpoch = _fleetLocalReceptionEpoch; ingress.transportEpoch = _messageCommandTransportEpoch;
    ingress.versionEpoch = _autopilotVersionObservationEpoch; ingress.receiptSequence = _fleetLocalReceptionNextReceipt;
    ingress.receivedAtMs = receivedAtMs; ingress.vehicleId = _id; ingress.componentId = _defaultComponentId;
    ingress.firmwareType = int(_firmwareType); ingress.vehicleType = int(_vehicleType); ingress.eligible = true;
    return ingress;
}

void Vehicle::_observeFleetLocalReception(const mavlink_message_t& original, const FleetLocalReceptionIngress& originalIngress)
{
    // The timestamp, frame and source identity were frozen before the first
    // RAW callback. The live probes run only AFTER existing P0 prepares.
    const FleetLocalReceptionIngress ingress = originalIngress;
    if (!ingress.eligible) return;
    const QPointer<Vehicle> self(this);
    const auto sameMemory = [&]() {
        if (!self || !ingress.primary || !ingress.links || !ingress.protocol || !ingress.vehicles || !ingress.toolbox
            || self->_fleetExecutionSourceLinks != ingress.links || self->_fleetExecutionSourceProtocol != ingress.protocol
            || self->_fleetExecutionSourceVehicles != ingress.vehicles || self->_fleetExecutionSourceToolbox != ingress.toolbox
            || self->_fleetLocalReceptionEpoch != ingress.sourceEpoch || self->_messageCommandTransportEpoch != ingress.transportEpoch
            || self->_autopilotVersionObservationEpoch != ingress.versionEpoch
            || self->_fleetLocalReceptionNextReceipt != ingress.receiptSequence
            || self->_fleetMessageIntervalRawReceiveDepth != 1 || self->_id != ingress.vehicleId
            || self->_defaultComponentId != ingress.componentId || int(self->_firmwareType) != ingress.firmwareType
            || int(self->_vehicleType) != ingress.vehicleType
            || !self->_fleetLocalReceptionRawSourceMatches(ingress.primary.data())) return false;
        const WeakLinkInterfacePtr current = ingress.links->primaryLink();
        return !current.expired() && !current.owner_before(ingress.primaryOwner) && !ingress.primaryOwner.owner_before(current);
    };
    const auto invalidateOriginal = [&]() {
        // Never erase a later source/binding installed by a nested callback.
        if (self && self->_fleetLocalReceptionEpoch == ingress.sourceEpoch
            && self->_fleetLocalReceptionNextReceipt == ingress.receiptSequence)
            self->_invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-source-probe-unavailable"));
    };
    if (!sameMemory() || _fleetLocalReceptionProbeBusy) return;
    _fleetLocalReceptionProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_fleetLocalReceptionProbeBusy = false; });
    const bool connected = ingress.primary->isConnected();
    if (!sameMemory() || !connected) { invalidateOriginal(); return; }
    const bool mock = ingress.primary->isMockLink();
    if (!sameMemory() || mock) { invalidateOriginal(); return; }
    const bool replay = ingress.primary->isLogReplay();
    if (!sameMemory() || replay) { invalidateOriginal(); return; }
    if ((_fleetLocalReceptionBoundAtMs >= 0 && ingress.receivedAtMs < _fleetLocalReceptionBoundAtMs)
        || (_fleetLocalReceptionNewestAtMs >= 0 && ingress.receivedAtMs < _fleetLocalReceptionNewestAtMs)) {
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-clock-regressed"));
        return;
    }
    if (_fleetLocalReceptionNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetLocalReceptionExhausted = true;
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-receipt-exhausted"));
        return;
    }
    if (!_fleetLocalReceptionPrimary) {
        _fleetLocalReceptionPrimary = ingress.primary; _fleetLocalReceptionPrimaryOwner = ingress.primaryOwner;
        _fleetLocalReceptionTransportEpoch = ingress.transportEpoch; _fleetLocalReceptionVersionEpoch = ingress.versionEpoch;
        _fleetLocalReceptionVehicleId = ingress.vehicleId; _fleetLocalReceptionComponentId = ingress.componentId;
        _fleetLocalReceptionFirmwareType = ingress.firmwareType; _fleetLocalReceptionVehicleType = ingress.vehicleType;
        _fleetLocalReceptionBoundAtMs = ingress.receivedAtMs;
    }
    while (!_fleetLocalReceptionFrames.isEmpty()
        && ingress.receivedAtMs - _fleetLocalReceptionFrames.first().receivedAtMs >= FleetLocalReceptionWindowMs)
        _fleetLocalReceptionFrames.remove(0);
    if (_fleetLocalReceptionFrames.size() >= FleetLocalReceptionMaximumFrames) {
        _fleetLocalReceptionLastDroppedAtMs = _fleetLocalReceptionFrames.first().receivedAtMs;
        _fleetLocalReceptionFrames.remove(0);
    }
    // Same framing formula as local MAVLink protocol.h. This counts encoded
    // MAVLink bytes INCLUDING magic/checksum/signature, not UDP/TCP/serial
    // overhead, physical traffic, capacity or a remote transmission rate.
    const int frameBytes = original.magic == MAVLINK_STX_MAVLINK1
        ? int(original.len) + MAVLINK_CORE_HEADER_MAVLINK1_LEN + 1 + 2
        : int(original.len) + MAVLINK_NUM_NON_PAYLOAD_BYTES
            + ((original.incompat_flags & MAVLINK_IFLAG_SIGNED) ? MAVLINK_SIGNATURE_BLOCK_LEN : 0);
    FleetLocalReceptionFrame frame; frame.receivedAtMs = ingress.receivedAtMs; frame.frameBytes = frameBytes;
    _fleetLocalReceptionFrames.append(frame);
    ++_fleetLocalReceptionNextReceipt;
    if (_fleetLocalReceptionNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetLocalReceptionExhausted = true;
        _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-receipt-exhausted"));
        return;
    }
    _fleetLocalReceptionNewestAtMs = ingress.receivedAtMs;
    _fleetLocalReceptionReason = QStringLiteral("local-reception-observed");
}

QVariantMap Vehicle::fleetLocalReceptionObservation() const
{
    const bool ownerThread = QThread::currentThread() == thread();
    const qint64 now = ownerThread && _fleetLocalReceptionClock.isValid() ? _fleetLocalReceptionClock.elapsed() : -1;
    const bool observed = ownerThread && _fleetLocalReceptionNewestAtMs >= 0;
    const qint64 newestAge = observed && now >= _fleetLocalReceptionNewestAtMs ? now - _fleetLocalReceptionNewestAtMs : -1;
    const qint64 boundAge = ownerThread && _fleetLocalReceptionBoundAtMs >= 0 && now >= _fleetLocalReceptionBoundAtMs
        ? now - _fleetLocalReceptionBoundAtMs : -1;
    const bool busy = ownerThread && (_fleetMessageIntervalRawReceiveDepth != 0 || _fleetLocalReceptionProbeBusy);
    const bool current = ownerThread && !busy && observed && newestAge >= 0 && newestAge <= FleetLocalReceptionMaximumAgeMs
        && _fleetLocalReceptionPrimary && !_fleetLocalReceptionPrimaryOwner.expired()
        && _fleetLocalReceptionBoundAtMs >= 0 && _fleetLocalReceptionTransportEpoch == _messageCommandTransportEpoch
        && _fleetLocalReceptionVersionEpoch == _autopilotVersionObservationEpoch
        && _fleetLocalReceptionVehicleId == _id && _fleetLocalReceptionComponentId == _defaultComponentId
        && _fleetLocalReceptionFirmwareType == int(_firmwareType) && _fleetLocalReceptionVehicleType == int(_vehicleType)
        && _fleetLocalReceptionRawSourceMatches(_fleetLocalReceptionPrimary.data())
        && !_fleetExecutionSourceLinks->primaryLink().owner_before(_fleetLocalReceptionPrimaryOwner)
        && !_fleetLocalReceptionPrimaryOwner.owner_before(_fleetExecutionSourceLinks->primaryLink());
    const bool saturated = ownerThread && _fleetLocalReceptionLastDroppedAtMs >= 0
        && now >= _fleetLocalReceptionLastDroppedAtMs && now - _fleetLocalReceptionLastDroppedAtMs < FleetLocalReceptionWindowMs;
    int count = 0; qint64 bytes = 0, previous = -1, maximumGap = -1;
    bool ordered = true;
    if (ownerThread) for (const FleetLocalReceptionFrame& frame : _fleetLocalReceptionFrames) {
        if (frame.receivedAtMs < 0 || now < frame.receivedAtMs) { ordered = false; continue; }
        if (now - frame.receivedAtMs >= FleetLocalReceptionWindowMs) continue;
        if (previous >= 0) {
            if (frame.receivedAtMs < previous) ordered = false;
            else maximumGap = qMax(maximumGap, frame.receivedAtMs - previous);
        }
        previous = frame.receivedAtMs; ++count; bytes += frame.frameBytes;
    }
    const bool complete = current && ordered && !saturated && boundAge >= FleetLocalReceptionWindowMs;
    const QVariantMap window{{QStringLiteral("durationMs"), FleetLocalReceptionWindowMs},
        {QStringLiteral("maximumRetainedFrames"), FleetLocalReceptionMaximumFrames}, {QStringLiteral("retainedFrameCount"), count},
        {QStringLiteral("complete"), complete}, {QStringLiteral("saturated"), saturated},
        {QStringLiteral("receivedFrameCount"), complete ? count : -1},
        {QStringLiteral("receivedFrameBytes"), QVariant::fromValue(complete ? bytes : qint64(-1))},
        {QStringLiteral("observedReceiveKbps"), complete ? double(bytes) * 8.0 / double(FleetLocalReceptionWindowMs) : -1.0},
        {QStringLiteral("maximumInternalGapMs"), QVariant::fromValue(complete && count >= 2 ? maximumGap : qint64(-1))}};
    QString reason = ownerThread ? _fleetLocalReceptionReason : QStringLiteral("local-reception-owner-thread-required");
    if (ownerThread) {
        if (_fleetLocalReceptionRemoving) reason = QStringLiteral("local-reception-vehicle-removing");
        else if (_fleetLocalReceptionExhausted) reason = QStringLiteral("local-reception-receipt-exhausted");
        else if (_fleetMessageIntervalRawReceiveExhausted) reason = QStringLiteral("local-reception-raw-depth-exhausted");
        else if (busy) reason = QStringLiteral("local-reception-transport-not-quiescent");
        else if (!observed) { /* Keep the actual unobserved/source-clear reason. */ }
        else if (newestAge < 0 || boundAge < 0) reason = QStringLiteral("local-reception-clock-unavailable");
        else if (newestAge > FleetLocalReceptionMaximumAgeMs) reason = QStringLiteral("local-reception-source-stale");
        else if (!current) reason = QStringLiteral("local-reception-source-not-current");
        else if (!ordered) reason = QStringLiteral("local-reception-invalid-window");
        else if (saturated) reason = QStringLiteral("local-reception-window-capacity-truncated");
        else if (!complete) reason = QStringLiteral("local-reception-window-not-covered");
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-local-reception")},
        {QStringLiteral("vehicleId"), ownerThread ? _id : 0}, {QStringLiteral("componentId"), ownerThread ? _defaultComponentId : 0},
        {QStringLiteral("sourceEpoch"), ownerThread && !_fleetLocalReceptionEpoch.isEmpty() ? _fleetLocalReceptionEpoch : QStringLiteral("")},
        {QStringLiteral("receiptSequence"), QString::number(ownerThread ? _fleetLocalReceptionNextReceipt : 0)},
        {QStringLiteral("sourceCurrent"), current}, {QStringLiteral("observed"), observed},
        {QStringLiteral("newestAgeMs"), QVariant::fromValue(newestAge)}, {QStringLiteral("window"), window},
        {QStringLiteral("boundAgeMs"), QVariant::fromValue(boundAge)}, {QStringLiteral("maximumAgeMs"), FleetLocalReceptionMaximumAgeMs},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("availableBandwidthVerified"), false}, {QStringLiteral("latencyVerified"), false},
        {QStringLiteral("packetLossVerified"), false}, {QStringLiteral("remoteTransmitRateVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("reason"), reason}};
}

void Vehicle::_initializeOptionalTelemetryObservations()
{
    _optionalTelemetryClock.start();
    _optionalTelemetryLedgerEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces)
        + QLatin1Char(':') + QString::number(_optionalTelemetryMutationGeneration);
    for (int messageId : {MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, MAVLINK_MSG_ID_VFR_HUD, MAVLINK_MSG_ID_ATTITUDE_TARGET})
        _optionalTelemetryRows.insert(messageId, OptionalTelemetryRow());
}

void Vehicle::_invalidateOptionalTelemetryObservations()
{
    // No callbacks, requests, observer ownership or receipt-counter restart.
    // Clear old values rather than relabelling them with a new source epoch.
    _optionalTelemetryLedgerEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces)
        + QLatin1Char(':') + QString::number(_optionalTelemetryMutationGeneration);
    _optionalTelemetryVersionEpoch.clear();
    _optionalTelemetryPrimary.clear();
    _optionalTelemetryPrimaryOwner.reset();
    _optionalTelemetryBoundAtMs = -1;
    _optionalTelemetryComponentId = 0;
    _optionalTelemetryFirmwareType = _optionalTelemetryVehicleType = -1;
    for (auto row = _optionalTelemetryRows.begin(); row != _optionalTelemetryRows.end(); ++row) {
        quint64 revision = row->contentRevision;
        if (row->reportedObserved) {
            if (_optionalTelemetryRevision == std::numeric_limits<quint64>::max()) _optionalTelemetryExhausted = true;
            else revision = ++_optionalTelemetryRevision;
        }
        row.value() = OptionalTelemetryRow();
        row->contentRevision = revision;
    }
}

bool Vehicle::_optionalTelemetryRawSourceMatches(LinkInterface* sourceLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || qgcApp()->thread() != thread()
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving || _optionalTelemetryExhausted
        || _optionalTelemetryMutationExhausted || _optionalTelemetryWriteDepth != 0
        || !_optionalTelemetryClock.isValid() || !QElapsedTimer::isMonotonic()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255 || !sourceLink
        || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
        || !_fleetExecutionSourceVehicles || !_fleetExecutionSourceToolbox
        || _fleetExecutionSourceLinks->thread() != thread() || _fleetExecutionSourceProtocol->thread() != thread()
        || _fleetExecutionSourceVehicles->thread() != thread() || _fleetExecutionSourceToolbox->thread() != thread()
        || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || _fleetExecutionSourceToolbox->mavlinkProtocol() != _fleetExecutionSourceProtocol.data()
        || _fleetExecutionSourceToolbox->multiVehicleManager() != _fleetExecutionSourceVehicles.data()
        || _fleetExecutionSourceVehicles->getVehicleById(_id) != this
        || _fleetExecutionSourceLinks->communicationLost() || _autopilotVersionObservationEpoch.isEmpty()) return false;
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (primary.expired()) return false;
    // Inspect the actual retained member without weak.lock() or temporary
    // strong ownership. Both the pointer and the control block must match.
    for (const auto& entry : _fleetExecutionSourceLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !primary.owner_before(candidate)
            && !candidate.owner_before(primary)) return true;
    }
    return false;
}

bool Vehicle::_optionalTelemetrySourceAllowed(LinkInterface* suppliedLink)
{
    if (_optionalTelemetryProbeBusy || _optionalTelemetryReceiveDepth != 1
        || sender() != _fleetExecutionSourceProtocol.data() || !_optionalTelemetryRawSourceMatches(suppliedLink)) return false;
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> source(suppliedLink);
    const QPointer<VehicleLinkManager> links(_fleetExecutionSourceLinks);
    const QPointer<MAVLinkProtocol> protocol(_fleetExecutionSourceProtocol);
    const QPointer<MultiVehicleManager> vehicles(_fleetExecutionSourceVehicles);
    const QPointer<QGCToolbox> toolbox(_fleetExecutionSourceToolbox);
    const WeakLinkInterfacePtr owner = links->primaryLink();
    const QString ledgerEpoch = _optionalTelemetryLedgerEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const quint64 receipt = _optionalTelemetryNextReceipt, revision = _optionalTelemetryRevision;
    const int vehicleId = _id, component = _defaultComponentId, firmware = int(_firmwareType), model = int(_vehicleType);
    const auto same = [&]() {
        if (!self || !source || !links || !protocol || !vehicles || !toolbox
            || self->_fleetExecutionSourceLinks != links || self->_fleetExecutionSourceProtocol != protocol
            || self->_fleetExecutionSourceVehicles != vehicles || self->_fleetExecutionSourceToolbox != toolbox
            || self->_optionalTelemetryLedgerEpoch != ledgerEpoch || self->_autopilotVersionObservationEpoch != versionEpoch
            || self->_optionalTelemetryNextReceipt != receipt || self->_optionalTelemetryRevision != revision
            || self->_id != vehicleId || self->_defaultComponentId != component
            || int(self->_firmwareType) != firmware || int(self->_vehicleType) != model
            || self->_optionalTelemetryReceiveDepth != 1 || !self->_optionalTelemetryProbeBusy
            || !self->_optionalTelemetryRawSourceMatches(source.data())) return false;
        const WeakLinkInterfacePtr current = links->primaryLink();
        return !current.expired() && !current.owner_before(owner) && !owner.owner_before(current);
    };
    _optionalTelemetryProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_optionalTelemetryProbeBusy = false; });
    const bool connected = source->isConnected();
    if (!same() || !connected) return false;
    const bool mock = source->isMockLink();
    if (!same() || mock) return false;
    const bool replay = source->isLogReplay();
    return same() && !replay;
}

bool Vehicle::_optionalTelemetrySourceMemoryCurrent() const
{
    if (_optionalTelemetryReceiveDepth != 0 || _optionalTelemetryProbeBusy
        || !_optionalTelemetryPrimary || _optionalTelemetryPrimaryOwner.expired()
        || _optionalTelemetryLedgerEpoch.isEmpty() || _optionalTelemetryVersionEpoch.isEmpty()
        || _optionalTelemetryVersionEpoch != _autopilotVersionObservationEpoch || _optionalTelemetryBoundAtMs < 0
        || _optionalTelemetryComponentId != _defaultComponentId || _optionalTelemetryFirmwareType != int(_firmwareType)
        || _optionalTelemetryVehicleType != int(_vehicleType)
        || !_optionalTelemetryRawSourceMatches(_optionalTelemetryPrimary.data())) return false;
    const WeakLinkInterfacePtr current = _fleetExecutionSourceLinks->primaryLink();
    return !current.expired() && !current.owner_before(_optionalTelemetryPrimaryOwner)
        && !_optionalTelemetryPrimaryOwner.owner_before(current);
}

void Vehicle::_observeOptionalTelemetry(const mavlink_message_t& original, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (!optionalTelemetryFrameId(original.msgid) || sender() != _fleetExecutionSourceProtocol.data()
        || original.sysid != _id || original.compid != _defaultComponentId) return;
    const int maximum = original.msgid == MAVLINK_MSG_ID_MESSAGE_INTERVAL ? MAVLINK_MSG_ID_MESSAGE_INTERVAL_LEN
        : original.msgid == MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT ? MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT_LEN
        : original.msgid == MAVLINK_MSG_ID_VFR_HUD ? MAVLINK_MSG_ID_VFR_HUD_LEN : MAVLINK_MSG_ID_ATTITUDE_TARGET_LEN;
    const int minimum = original.msgid == MAVLINK_MSG_ID_MESSAGE_INTERVAL ? MAVLINK_MSG_ID_MESSAGE_INTERVAL_MIN_LEN
        : original.msgid == MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT ? MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT_MIN_LEN
        : original.msgid == MAVLINK_MSG_ID_VFR_HUD ? MAVLINK_MSG_ID_VFR_HUD_MIN_LEN : MAVLINK_MSG_ID_ATTITUDE_TARGET_MIN_LEN;
    if ((original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)
        || original.len < 1 || original.len > maximum || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != minimum)) {
        _invalidateOptionalTelemetryObservations(); return;
    }
    int messageId = original.msgid;
    qint32 intervalUs = 0;
    const bool report = original.msgid == MAVLINK_MSG_ID_MESSAGE_INTERVAL;
    if (report) {
        // The generated byte-swap decoder reads full fields. Normalize the
        // original MAVLink2 trimmed payload before using either decoder path.
        mavlink_message_t normalized = original;
        memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
        memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
        mavlink_message_interval_t value{};
        mavlink_msg_message_interval_decode(&normalized, &value);
        messageId = value.message_id; intervalUs = value.interval_us;
        if (!optionalTelemetryMessageId(messageId)) return;
    }
    if (_optionalTelemetryReceiveDepth != 1 || receivedAtMs < 0 || _optionalTelemetryExhausted) {
        _invalidateOptionalTelemetryObservations(); return;
    }
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> source(sourceLink);
    const bool allowed = _optionalTelemetrySourceAllowed(source.data());
    if (!self) return;
    if (!source || !allowed) { _invalidateOptionalTelemetryObservations(); return; }
    const WeakLinkInterfacePtr primary = _fleetExecutionSourceLinks->primaryLink();
    if (_optionalTelemetryPrimary != source || _optionalTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _optionalTelemetryComponentId != _defaultComponentId || _optionalTelemetryFirmwareType != int(_firmwareType)
        || _optionalTelemetryVehicleType != int(_vehicleType) || primary.expired()
        || _optionalTelemetryPrimaryOwner.owner_before(primary) || primary.owner_before(_optionalTelemetryPrimaryOwner)) {
        // This is the sole permitted initial-ledger transition for an active
        // query. Any earlier clear already changed its frozen unbound token
        // and therefore cannot be adopted here as a fresh baseline.
        const auto initialQuery = _fleetMessageIntervalActiveQuery;
        const bool bindInitialQuery = initialQuery && initialQuery->active && !initialQuery->ledgerBound
            && initialQuery->source.messageIntervalPrimary == source
            && _fleetMessageIntervalQuerySourceCurrent(initialQuery, false);
        const auto configurationRun = _fleetOptionalTelemetryConfigurationRun;
        const bool bindConfigurationRun = configurationRun && configurationRun->active && !configurationRun->ledgerBound
            && configurationRun->basis->queries.first()->source.messageIntervalPrimary == source
            && _fleetOptionalTelemetryRunSourceCurrent(configurationRun, false);
        _invalidateOptionalTelemetryObservations();
        if (_optionalTelemetryExhausted) return;
        _optionalTelemetryPrimary = source; _optionalTelemetryPrimaryOwner = primary;
        _optionalTelemetryVersionEpoch = _autopilotVersionObservationEpoch;
        _optionalTelemetryComponentId = _defaultComponentId;
        _optionalTelemetryFirmwareType = int(_firmwareType); _optionalTelemetryVehicleType = int(_vehicleType);
        // This is passive observation coverage beginning with an eligible
        // original packet, not a subscription or an active stream request.
        _optionalTelemetryBoundAtMs = receivedAtMs;
        if (bindInitialQuery && _fleetMessageIntervalActiveQuery == initialQuery
            && initialQuery->active && initialQuery->owner) {
            initialQuery->ledgerEpoch = _optionalTelemetryLedgerEpoch;
            initialQuery->ledgerBound = true;
        }
        if (bindConfigurationRun && _fleetOptionalTelemetryConfigurationRun == configurationRun
            && configurationRun->active && configurationRun->owner) {
            configurationRun->ledgerEpoch = _optionalTelemetryLedgerEpoch;
            configurationRun->ledgerBound = true;
        }
    }
    auto found = _optionalTelemetryRows.find(messageId);
    if (found == _optionalTelemetryRows.end() || receivedAtMs < _optionalTelemetryBoundAtMs
        || (report && found->reportedAtMs > receivedAtMs) || (!report && found->newestFrameAtMs > receivedAtMs)) {
        _invalidateOptionalTelemetryObservations(); return;
    }
    const bool contentChanged = report && (!found->reportedObserved || found->intervalUs != intervalUs);
    if (_optionalTelemetryNextReceipt == std::numeric_limits<quint64>::max()
        || (contentChanged && _optionalTelemetryRevision == std::numeric_limits<quint64>::max())) {
        _optionalTelemetryExhausted = true; _invalidateOptionalTelemetryObservations(); return;
    }
    const quint64 receipt = ++_optionalTelemetryNextReceipt;
    if (report) {
        if (contentChanged) found->contentRevision = ++_optionalTelemetryRevision;
        found->intervalUs = intervalUs; found->reportedAtMs = receivedAtMs;
        found->reportedReceiptSequence = receipt; found->reportedObserved = true;
    } else {
        // Count native packet envelopes, not the numerical quality of their
        // flight fields. This is neither a transmit-rate nor a loss measure.
        while (!found->receivedAtMs.isEmpty() && receivedAtMs - found->receivedAtMs.first() >= OptionalTelemetryWindowMs)
            found->receivedAtMs.remove(0);
        if (found->receivedAtMs.size() >= OptionalTelemetryMaximumSamples) {
            found->lastDroppedFrameAtMs = found->receivedAtMs.first();
            found->receivedAtMs.remove(0);
        }
        found->receivedAtMs.append(receivedAtMs);
        found->newestFrameAtMs = receivedAtMs; found->frameReceiptSequence = receipt;
    }
}

QVariantMap Vehicle::optionalTelemetryObservations(qint64 maximumAgeMs) const
{
    const bool ownerThread = QThread::currentThread() == thread();
    const bool validAge = maximumAgeMs >= 1 && maximumAgeMs <= 120000;
    const bool current = ownerThread && validAge && _optionalTelemetrySourceMemoryCurrent();
    const qint64 now = ownerThread && _optionalTelemetryClock.isValid() ? _optionalTelemetryClock.elapsed() : -1;
    QVariantList rows;
    for (int messageId : {MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, MAVLINK_MSG_ID_VFR_HUD, MAVLINK_MSG_ID_ATTITUDE_TARGET}) {
        const OptionalTelemetryRow row = ownerThread ? _optionalTelemetryRows.value(messageId) : OptionalTelemetryRow();
        const qint64 age = row.reportedObserved && row.reportedAtMs >= 0 && now >= row.reportedAtMs ? now - row.reportedAtMs : -1;
        const bool reportedFresh = current && row.reportedObserved && row.intervalUs >= -1 && age >= 0 && age <= maximumAgeMs;
        const QString state = !row.reportedObserved ? QStringLiteral("unobserved")
            : row.intervalUs == -1 ? QStringLiteral("disabled") : row.intervalUs == 0 ? QStringLiteral("not-available")
            : row.intervalUs > 0 ? QStringLiteral("positive") : QStringLiteral("invalid");
        const QVariantMap reported{{QStringLiteral("observed"), row.reportedObserved},
            {QStringLiteral("available"), reportedFresh && row.intervalUs > 0}, {QStringLiteral("fresh"), reportedFresh},
            {QStringLiteral("ageMs"), QVariant::fromValue(age)},
            {QStringLiteral("receiptSequence"), QString::number(row.reportedReceiptSequence)},
            {QStringLiteral("contentRevision"), QString::number(row.contentRevision)},
            {QStringLiteral("intervalUs"), row.reportedObserved ? QVariant(int(row.intervalUs)) : QVariant::fromValue(nullptr)},
            {QStringLiteral("state"), state}};
        const qint64 newestAge = row.newestFrameAtMs >= 0 && now >= row.newestFrameAtMs ? now - row.newestFrameAtMs : -1;
        const bool saturated = row.lastDroppedFrameAtMs >= 0 && now >= row.lastDroppedFrameAtMs
            && now - row.lastDroppedFrameAtMs < OptionalTelemetryWindowMs;
        int retainedCount = 0;
        qint64 previous = -1, maximumGap = -1;
        for (qint64 receivedAt : row.receivedAtMs) {
            if (receivedAt < 0 || now < receivedAt || now - receivedAt >= OptionalTelemetryWindowMs) continue;
            if (previous >= 0) maximumGap = qMax(maximumGap, receivedAt - previous);
            previous = receivedAt; ++retainedCount;
        }
        // A stream never seen in this source epoch stays unknown, even if a
        // 244 report bound the passive ledger earlier. Once seen, a full
        // covered tail may contain zero packets while its newest receipt is
        // still within maximumAgeMs: zero then means local zero-reception,
        // not that the remote sender is disabled or that no packets were lost.
        const bool complete = current && !saturated && _optionalTelemetryBoundAtMs >= 0 && now >= _optionalTelemetryBoundAtMs
            && now - _optionalTelemetryBoundAtMs >= OptionalTelemetryWindowMs && newestAge >= 0 && newestAge <= maximumAgeMs;
        const QVariantMap window{{QStringLiteral("durationMs"), QVariant::fromValue(OptionalTelemetryWindowMs)},
            {QStringLiteral("retainedFrameCount"), retainedCount}, {QStringLiteral("complete"), complete},
            {QStringLiteral("saturated"), saturated}, {QStringLiteral("receivedFrameCount"), complete ? retainedCount : -1},
            {QStringLiteral("receivedMeanHz"), complete ? double(retainedCount) * 1000.0 / double(OptionalTelemetryWindowMs) : -1.0},
            {QStringLiteral("maximumInternalGapMs"), QVariant::fromValue(complete ? maximumGap : qint64(-1))},
            {QStringLiteral("newestAgeMs"), QVariant::fromValue(newestAge)},
            {QStringLiteral("receiptSequence"), QString::number(row.frameReceiptSequence)}};
        rows.append(QVariantMap{{QStringLiteral("messageId"), messageId}, {QStringLiteral("reported"), reported},
            {QStringLiteral("receivedWindow"), window}});
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-optional-telemetry-observations")},
        {QStringLiteral("vehicleId"), ownerThread ? _id : 0}, {QStringLiteral("componentId"), ownerThread ? _defaultComponentId : 0},
        {QStringLiteral("ledgerEpoch"), ownerThread && !_optionalTelemetryLedgerEpoch.isEmpty()
            ? _optionalTelemetryLedgerEpoch : QStringLiteral("")},
        {QStringLiteral("versionEpoch"), ownerThread && !_optionalTelemetryVersionEpoch.isEmpty()
            ? _optionalTelemetryVersionEpoch : QStringLiteral("")},
        {QStringLiteral("sourceCurrent"), current}, {QStringLiteral("maximumAgeMs"), QVariant::fromValue(maximumAgeMs)},
        {QStringLiteral("rows"), rows}, {QStringLiteral("readonly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("actualTransmitRateVerified"), false}, {QStringLiteral("availableBandwidthVerified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool Vehicle::fleetLinkLatencyCaptureTransportQuiescent() const
{
    // Own authorization/dispatch is deliberately not part of this temporary
    // RAW fence: its final fence may itself read the same pure snapshot.
    return fleetMessageIntervalQueryTransportQuiescent() && !_fleetLocalReceptionProbeBusy
        && !_fleetExecutionSourceProbeBusy;
}

bool Vehicle::_fleetLinkLatencySourceCurrent(const std::shared_ptr<FleetLinkLatencyRun>& run,
    bool requireQuiescent) const
{
    if (!run || run->vehicle.data() != this || !run->owner || run->owner->thread() != thread()
        || QThread::currentThread() != thread() || run->sourceRetired || _offlineEditingVehicle
        || _autopilotVersionObservationRemoving || !run->clock.isValid()
        || !run->links || run->links != _fleetExecutionSourceLinks || run->links.data() != _vehicleLinkManager
        || !run->protocol || run->protocol != _fleetExecutionSourceProtocol || run->protocol.data() != _mavlink
        || !run->plugin || run->plugin != _messageIntervalSourcePlugin || run->plugin.data() != _firmwarePlugin
        || !run->toolbox || run->toolbox != _fleetExecutionSourceToolbox || !qgcApp()
        || qgcApp()->toolbox() != run->toolbox.data() || run->toolbox->mavlinkProtocol() != run->protocol.data()
        || !run->vehicles || run->vehicles != _fleetExecutionSourceVehicles
        || run->toolbox->multiVehicleManager() != run->vehicles.data() || run->vehicles->getVehicleById(_id) != this
        || !run->primary || run->primaryOwner.expired() || run->links->communicationLost()
        || run->vehicleId != _id || run->componentId != _defaultComponentId
        || run->firmwareType != int(_firmwareType) || run->vehicleType != int(_vehicleType)
        || run->transportEpoch.isEmpty() || run->transportEpoch != _messageCommandTransportEpoch
        || run->versionEpoch.isEmpty() || run->versionEpoch != _autopilotVersionObservationEpoch
        || run->versionRevision != _autopilotVersionObservationRevision
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete
        || _autopilotVersionObservationUpdating || _autopilotVersionObservationExhausted
        || _autopilotVersionObservationLink != run->primary || !_autopilotVersionObservationClock.isValid()
        || !run->primary->mavlinkChannelIsSet() || run->primary->mavlinkChannel() != run->channel
        || run->channel >= MAVLINK_COMM_NUM_BUFFERS || run->systemId == 0 || run->sourceComponentId == 0
        || run->protocol->getSystemId() != run->systemId || run->protocol->getComponentId() != run->sourceComponentId
        || armed() || flying() || !run->owner->nativeVehicleMatches(this)
        || (requireQuiescent && !fleetLinkLatencyCaptureTransportQuiescent())) return false;
    const WeakLinkInterfacePtr current = run->links->primaryLink();
    if (current.expired() || current.owner_before(run->primaryOwner) || run->primaryOwner.owner_before(current)) return false;
    bool actualPrimary = false;
    for (const auto& member : run->links->_rgLinkInfo) {
        const WeakLinkInterfacePtr owner(member.link);
        if (member.link && member.link.get() == run->primary.data()
            && !current.owner_before(owner) && !owner.owner_before(current)) { actualPrimary = true; break; }
    }
    const qint64 versionNow = _autopilotVersionObservationClock.elapsed();
    if (!actualPrimary || _autopilotVersionObservationReceivedAtMs < 0
        || versionNow < _autopilotVersionObservationReceivedAtMs
        || versionNow - _autopilotVersionObservationReceivedAtMs > 30000) return false;
    const qint64 elapsed = run->clock.elapsed();
    if (elapsed < 0 || (run->active && (_fleetLinkLatencyRun != run || elapsed >= run->deadlineMs))) return false;
    if (run->responseObserved && (run->responseAtNs < 0 || run->clock.nsecsElapsed() < run->responseAtNs
        || (run->clock.nsecsElapsed() - run->responseAtNs) / 1000000 > 30000)) return false;
    return true;
}

bool Vehicle::_fleetLinkLatencyAllowed(const std::shared_ptr<FleetLinkLatencyRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || !run->active || _fleetLinkLatencyRun != run || run->authorizationBusy
        || !run->authorizationFence || !_fleetLinkLatencySourceCurrent(run, true)) return false;
    run->authorizationBusy = true;
    const auto release = qScopeGuard([run]() { run->authorizationBusy = false; });
    const auto settled = [&]() {
        if (!self || !run->active || self->_fleetLinkLatencyRun != run
            || !self->_fleetLinkLatencySourceCurrent(run, true)
            || !self->_fleetIntervalQueryInitialConnect
            || self->_fleetIntervalQueryInitialConnect.data() != self->_initialConnectStateMachine
            || !self->isInitialConnectComplete() || !self->px4Firmware() || !self->multiRotor()
            || self->_fleetStartupEntryBusy || self->_fleetStartupActive || self->_maxProtoVersion < 200
            || !run->parameters || self->_parameterManager != run->parameters
            || run->parameters->_parameterObservationEpoch != run->parameterEpoch
            || !run->parameters->_parameterObservationMemoryContextAllowed()
            || run->parameters->_parameterObservationPendingWrites()
            || !self->_fleetIntervalQueryMission || self->_fleetIntervalQueryMission.data() != self->_missionManager
            || !self->_fleetIntervalQueryFence || self->_fleetIntervalQueryFence.data() != self->_geoFenceManager
            || !self->_fleetIntervalQueryRally || self->_fleetIntervalQueryRally.data() != self->_rallyPointManager
            || self->_fleetIntervalQueryMission->inProgress() || self->_fleetIntervalQueryMission->transportCallInProgress()
            || self->_fleetIntervalQueryFence->inProgress() || self->_fleetIntervalQueryFence->transportCallInProgress()
            || self->_fleetIntervalQueryRally->inProgress() || self->_fleetIntervalQueryRally->transportCallInProgress()
            || !self->_messageCommandSourceFtp || self->_messageCommandSourceFtp.data() != self->_ftpManager
            || !self->_messageCommandSourceFtp->_rgStateMachine.isEmpty()
            || self->_messageCommandSourceFtp->_ackOrNakTimeoutTimer.isActive()
            || !self->_messageCommandLaneId.isEmpty() || !self->_mavCommandList.isEmpty()
            || self->_waitForMavlinkMessageResultHandler || self->_requestMessageContext
            || self->_fleetMessageIntervalActiveQuery
            || (self->_fleetOptionalTelemetryConfigurationRun && self->_fleetOptionalTelemetryConfigurationRun->active)
            || self->_flightStateObservationExhausted || self->_flightStateObservationLink != run->primary) return false;
        for (auto row = run->parameters->_waitingReadParamIndexMap.constBegin();
             row != run->parameters->_waitingReadParamIndexMap.constEnd(); ++row) if (!row.value().isEmpty()) return false;
        for (auto row = run->parameters->_waitingReadParamNameMap.constBegin();
             row != run->parameters->_waitingReadParamNameMap.constEnd(); ++row) if (!row.value().isEmpty()) return false;
        const qint64 now = self->_autopilotVersionObservationClock.elapsed();
        for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
            const auto row = self->_flightStateObservations.constFind(channel);
            if (row == self->_flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
                || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > 5000) return false;
        }
        const QVariantMap heartbeat = self->_flightStateObservations.value(QStringLiteral("heartbeat")).value;
        const QVariantMap landed = self->_flightStateObservations.value(QStringLiteral("landedState")).value;
        return heartbeat.value(QStringLiteral("armed")).type() == QVariant::Bool
            && !heartbeat.value(QStringLiteral("armed")).toBool()
            && landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
            && landed.value(QStringLiteral("landedStateKnown")).toBool()
            && landed.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND;
    };
    if (!settled()) return false;
    const QPointer<LinkInterface> primary = run->primary;
    const bool connected = primary->isConnected();
    if (!settled() || !primary || !connected) return false;
    const bool mock = primary->isMockLink();
    if (!settled() || !primary || mock) return false;
    const bool replay = primary->isLogReplay();
    if (!settled() || !primary || replay) return false;
    bool authorized = false;
    {
        const auto fence = run->authorizationFence;
        // A captured function object's copy/destructor is also an external
        // lifetime boundary, even though the actual service uses QQ captures.
        if (self && settled()) authorized = fence && fence();
    }
    return self && authorized && settled();
}

QVariantMap Vehicle::_fleetLinkLatencyValue(const std::shared_ptr<FleetLinkLatencyRun>& run, bool current, bool retired)
{
    if (!run) return {};
    const qint64 nowNs = run->clock.isValid() ? run->clock.nsecsElapsed() : -1;
    const qint64 age = run->responseObserved && run->responseAtNs >= 0 && nowNs >= run->responseAtNs
        ? (nowNs - run->responseAtNs) / 1000000 : -1;
    const bool active = !retired && run->active;
    const qint64 remaining = active && run->clock.isValid()
        ? qBound(qint64(0), run->deadlineMs - run->clock.elapsed(), qint64(4000)) : 0;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-link-rtt")},
        {QStringLiteral("requestId"), run->id}, {QStringLiteral("vehicleId"), run->vehicleId},
        {QStringLiteral("componentId"), run->componentId}, {QStringLiteral("active"), active},
        {QStringLiteral("state"), retired && run->active ? QStringLiteral("failed") : run->state},
        {QStringLiteral("reason"), retired ? QStringLiteral("link-rtt-vehicle-destroyed-history") : run->reason},
        {QStringLiteral("requestIssued"), run->requestIssued}, {QStringLiteral("responseObserved"), run->responseObserved},
        {QStringLiteral("sourceCurrent"), !retired && current}, {QStringLiteral("remainingMs"), remaining},
        {QStringLiteral("roundTripMs"), run->roundTripMs},
        {QStringLiteral("nonce"), run->nonce > 0 ? QString::number(run->nonce) : QStringLiteral("")},
        {QStringLiteral("versionEpoch"), run->versionEpoch}, {QStringLiteral("transportEpoch"), run->transportEpoch},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("wireCausalityProven"), false}, {QStringLiteral("availableBandwidthVerified"), false},
        {QStringLiteral("packetLossVerified"), false}, {QStringLiteral("remoteClockOffsetVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("ageMs"), age}};
}

QVariantMap Vehicle::fleetLinkLatencyCaptureSnapshot(QObject* actualOwner,
    const std::shared_ptr<FleetLinkLatencyRun>& run) const
{
    if (QThread::currentThread() != thread() || !run || !actualOwner || run->owner.data() != actualOwner
        || run->vehicle.data() != this || actualOwner->thread() != thread()) return {};
    return _fleetLinkLatencyValue(run, _fleetLinkLatencySourceCurrent(run, true), false);
}

QVariantMap Vehicle::fleetLinkLatencyCaptureRetiredSnapshot(QObject* actualOwner,
    const std::shared_ptr<FleetLinkLatencyRun>& run)
{
    if (!run || run->vehicle || !actualOwner || run->owner.data() != actualOwner
        || actualOwner->thread() != QThread::currentThread()) return {};
    return _fleetLinkLatencyValue(run, false, true);
}

void Vehicle::_retireFleetLinkLatencyCapture(const QString& reason)
{
    // Pure memory only: never invoke/destroy an arbitrary fence during central
    // source invalidation or a partially destroyed Vehicle.
    const auto run = _fleetLinkLatencyRun;
    if (!run) return;
    run->sourceRetired = true;
    if (run->active) { run->active = false; run->state = QStringLiteral("failed"); run->reason = reason; }
}

std::shared_ptr<FleetLinkLatencyRun> Vehicle::prepareFleetLinkLatencyCapture(QObject* actualOwner,
    LinkInterface* expectedPrimary, qint64 remainingMs, const std::function<bool()>& authorizationFence)
{
    QElapsedTimer entered;
    entered.start(); // No Link/authorization preparation can renew this cap.
    if (QThread::currentThread() != thread() || !QElapsedTimer::isMonotonic() || !fleetLinkLatencyCaptureTransportQuiescent()
        || remainingMs < 1 || remainingMs > 4000 || !authorizationFence) return {};
    const QPointer<Vehicle> self(this);
    const QPointer<FleetLinkLatencyCapture> owner(qobject_cast<FleetLinkLatencyCapture*>(actualOwner));
    const QPointer<LinkInterface> primary(expectedPrimary);
    const auto oldRun = _fleetLinkLatencyRun;
    if (oldRun && oldRun->active && (!oldRun->owner || !oldRun->clock.isValid()
        || oldRun->clock.elapsed() >= oldRun->deadlineMs)) _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-expired-or-owner-destroyed"));
    if (!owner || owner->thread() != thread() || !primary || !owner->nativeVehicleMatches(this)
        || (oldRun && oldRun->active) || !_fleetExecutionSourceLinks || !_fleetExecutionSourceProtocol
        || !_fleetExecutionSourceToolbox || !_fleetExecutionSourceVehicles || !_messageIntervalSourcePlugin
        || !primary->mavlinkChannelIsSet() || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255) return {};
    const auto run = std::make_shared<FleetLinkLatencyRun>();
    run->clock = entered; run->deadlineMs = remainingMs;
    run->vehicle = self; run->owner = owner; run->primary = primary;
    run->primaryOwner = _fleetExecutionSourceLinks->primaryLink();
    run->links = _fleetExecutionSourceLinks; run->protocol = _fleetExecutionSourceProtocol;
    run->plugin = _messageIntervalSourcePlugin; run->toolbox = _fleetExecutionSourceToolbox;
    run->vehicles = _fleetExecutionSourceVehicles; run->parameters = _parameterManager;
    run->parameterEpoch = run->parameters ? run->parameters->_parameterObservationEpoch : QStringLiteral("");
    run->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    run->versionEpoch = _autopilotVersionObservationEpoch; run->versionRevision = _autopilotVersionObservationRevision;
    run->transportEpoch = _messageCommandTransportEpoch;
    run->vehicleId = _id; run->componentId = _defaultComponentId;
    run->firmwareType = int(_firmwareType); run->vehicleType = int(_vehicleType);
    run->channel = primary->mavlinkChannel(); run->systemId = run->protocol->getSystemId();
    run->sourceComponentId = run->protocol->getComponentId(); run->authorizationFence = authorizationFence;
    if (!self || !owner || !primary || _fleetLinkLatencyRun != oldRun
        || (oldRun && oldRun->active) || !owner->nativeVehicleMatches(this)) return {};
    _fleetLinkLatencyRun = run;
    if (!_fleetLinkLatencyAllowed(run)) {
        if (self && _fleetLinkLatencyRun == run && run->active) {
            run->active = false; run->state = QStringLiteral("failed"); run->reason = QStringLiteral("link-rtt-not-authorized");
        }
        return {};
    }
    return self && owner && primary && _fleetLinkLatencyRun == run && run->active ? run : nullptr;
}

bool Vehicle::advanceFleetLinkLatencyCapture(QObject* actualOwner, const std::shared_ptr<FleetLinkLatencyRun>& run)
{
    if (QThread::currentThread() != thread() || !run || !actualOwner || run->owner.data() != actualOwner
        || run->vehicle.data() != this || actualOwner->thread() != thread()) return false;
    if (!run->active) return true;
    if (_fleetLinkLatencyRun != run || !run->clock.isValid() || run->clock.elapsed() >= run->deadlineMs) {
        run->active = false; run->state = QStringLiteral("failed"); run->reason = QStringLiteral("link-rtt-deadline-no-confirmed-response"); return true;
    }
    if (!fleetLinkLatencyCaptureTransportQuiescent()) return true;
    const QPointer<Vehicle> self(this);
    if (!_fleetLinkLatencyAllowed(run)) {
        if (self && run->active) { run->active = false; run->state = QStringLiteral("failed"); run->reason = QStringLiteral("link-rtt-source-or-authorization-changed"); }
        return bool(self);
    }
    if (!self) return false;
    if (run->requestIssued || run->dispatching) return true;
    run->dispatching = true;
    const auto release = qScopeGuard([run]() { run->dispatching = false; });
    const QPointer<LinkInterface> primary = run->primary;
    const QPointer<FirmwarePlugin> plugin = run->plugin;
    const auto retained = run->primaryOwner.lock();
    const auto current = [&]() { return self && primary && plugin && retained && retained.get() == primary.data()
        && _fleetLinkLatencyRun == run && run->active && !run->requestIssued && _fleetLinkLatencySourceCurrent(run, true); };
    const auto fail = [&]() { if (self && _fleetLinkLatencyRun == run && run->active) {
        run->active = false; run->state = QStringLiteral("failed"); run->reason = QStringLiteral("link-rtt-v2-send-not-accepted"); } };
    if (!current() || (mavlink_get_channel_status(run->channel)->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1)) { fail(); return bool(self); }
    run->nonce = nextFleetLinkLatencyTimestamp();
    if (run->nonce <= 0) { fail(); return bool(self); }
    mavlink_message_t message{};
    mavlink_msg_timesync_pack_chan(run->systemId, run->sourceComponentId, run->channel, &message,
        0, run->nonce, uint8_t(run->vehicleId), uint8_t(run->componentId));
    const mavlink_message_t immutable = message;
    const auto encodedCurrent = [&]() { return current() && message.magic == MAVLINK_STX
        && message.msgid == MAVLINK_MSG_ID_TIMESYNC && message.len == MAVLINK_MSG_ID_TIMESYNC_LEN
        && !(mavlink_get_channel_status(run->channel)->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1)
        && memcmp(&message, &immutable, sizeof(message)) == 0; };
    if (!encodedCurrent() || !_fleetLinkLatencyAllowed(run)) { fail(); return bool(self); }
    if (!encodedCurrent()) { fail(); return bool(self); }
    plugin->adjustOutgoingMavlinkMessageThreadSafe(this, primary.data(), &message);
    if (!encodedCurrent()) { fail(); return bool(self); }
    if (!_fleetLinkLatencyAllowed(run)) { fail(); return bool(self); }
    if (!encodedCurrent()) { fail(); return bool(self); }
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length <= 0 || length > MAVLINK_MAX_PACKET_LEN || !encodedCurrent()) { fail(); return bool(self); }
    if (!_fleetLinkLatencyAllowed(run)) { fail(); return bool(self); }
    if (!encodedCurrent()) { fail(); return bool(self); }
    run->sentAtNs = run->clock.nsecsElapsed();
    if (run->sentAtNs < 0 || run->clock.elapsed() >= run->deadlineMs) { fail(); return bool(self); }
    run->requestIssued = true; run->state = QStringLiteral("waiting-response");
    primary->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    // Possibly queued is a retained fact, even if this write/notification
    // synchronously destroys Vehicle. It is not verified wire transmission.
    if (!self) return true;
    ++_messagesSent;
    emit messagesSentChanged();
    return true;
}

bool Vehicle::cancelFleetLinkLatencyCapture(QObject* actualOwner, const std::shared_ptr<FleetLinkLatencyRun>& run)
{
    if (QThread::currentThread() != thread() || !run || !actualOwner || run->owner.data() != actualOwner
        || run->vehicle.data() != this || actualOwner->thread() != thread()) return false;
    if (run->active) { run->active = false; run->state = QStringLiteral("cancelled"); run->reason = QStringLiteral("link-rtt-cancelled-no-retraction"); }
    run->sourceRetired = true;
    if (_fleetLinkLatencyRun == run) _fleetLinkLatencyRun.reset();
    return true;
}

void Vehicle::_observeFleetLinkLatencyCapture(const std::shared_ptr<FleetLinkLatencyRun>& run,
    const mavlink_message_t& original, LinkInterface* originalLink, qint64 receivedAtNs, bool originalWriteStarted)
{
    if (!run || _fleetLinkLatencyRun != run || !originalWriteStarted || !run->requestIssued
        || sender() != run->protocol.data() || originalLink != run->primary.data()
        || original.sysid != run->vehicleId || original.compid != run->componentId) return;
    // Retire a real departure from the ground branch, not just a transient
    // getter-in-RAW failure. Do not turn a later disarm into an ABA renewal.
    if ((original.msgid == MAVLINK_MSG_ID_HEARTBEAT && original.len >= MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN
            && (mavlink_msg_heartbeat_get_base_mode(&original) & MAV_MODE_FLAG_SAFETY_ARMED))
        || (original.msgid == MAVLINK_MSG_ID_EXTENDED_SYS_STATE && original.len >= MAVLINK_MSG_ID_EXTENDED_SYS_STATE_MIN_LEN
            && mavlink_msg_extended_sys_state_get_landed_state(&original) != MAV_LANDED_STATE_ON_GROUND)) {
        _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-ground-branch-ended")); return;
    }
    if (!run->active || original.msgid != MAVLINK_MSG_ID_TIMESYNC || original.magic != MAVLINK_STX
        || original.len != MAVLINK_MSG_ID_TIMESYNC_LEN || !_fleetLinkLatencySourceCurrent(run, false)
        || receivedAtNs < run->sentAtNs || run->sentAtNs < 0 || receivedAtNs / 1000000 >= run->deadlineMs) return;
    mavlink_timesync_t response{};
    mavlink_msg_timesync_decode(&original, &response);
    if (response.tc1 <= 0 || response.ts1 != run->nonce
        || response.target_system != run->systemId || response.target_component != run->sourceComponentId) return;
    // The native cap is additionally bounded by the service's ORIGINAL
    // entry clock. Millisecond conversion/prepare time cannot extend that
    // window. This pure RAW admission fence is not a terminal source gate.
    const QPointer<Vehicle> self(this);
    const QPointer<FleetLinkLatencyCapture> owner = run->owner;
    const qint64 nonce = run->nonce;
    if (!owner || !owner->nativeResponseCurrent(run) || !self || !owner || run->owner != owner
        || _fleetLinkLatencyRun != run || !run->active || run->nonce != nonce
        || originalLink != run->primary.data() || !_fleetLinkLatencySourceCurrent(run, false)) return;
    run->responseObserved = true; run->responseAtNs = receivedAtNs;
    run->roundTripMs = double(receivedAtNs - run->sentAtNs) / 1000000.0;
    run->active = false; run->state = QStringLiteral("complete"); run->reason = QStringLiteral("");
}

bool Vehicle::fleetMessageIntervalQueryTransportQuiescent() const
{
    // Pure memory: an unbound ledger is perfectly compatible with a quiet
    // transport. No virtual Link call, authorization or command dispatch.
    return QThread::currentThread() == thread() && !_fleetMessageIntervalRawReceiveExhausted
        && _fleetMessageIntervalRawReceiveDepth == 0 && _optionalTelemetryReceiveDepth == 0
        && !_optionalTelemetryProbeBusy && _optionalTelemetryWriteDepth == 0
        && _autopilotVersionObservationReceiveDepth == 0 && _flightStateObservationReceiveDepth == 0
        && _startupOdometryReceiveDepth == 0 && _fleetMissionObservationReceiveDepth == 0;
}

bool Vehicle::_fleetMessageIntervalQuerySourceCurrent(const std::shared_ptr<FleetMessageIntervalQuery>& query,
    bool requireQuiescent) const
{
    if (!query || !query->owner || query->owner->thread() != thread()
        || !_messageCommandSourceMemoryCurrent(query->source)
        || !_optionalTelemetryRawSourceMatches(query->source.messageIntervalPrimary.data())
        || !_optionalTelemetryClock.isValid() || _optionalTelemetryExhausted || _optionalTelemetryMutationExhausted
        || _optionalTelemetryWriteDepth != 0 || _optionalTelemetryMutationGeneration != query->mutationGeneration
        || query->ledgerEpoch.isEmpty() || query->ledgerEpoch != _optionalTelemetryLedgerEpoch
        || _autopilotVersionObservationEpoch != query->source.messageIntervalSourceEpoch
        || _autopilotVersionObservationRevision != query->versionRevision
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete
        || _autopilotVersionObservationUpdating || _autopilotVersionObservationExhausted
        || _autopilotVersionObservationLink != query->source.messageIntervalPrimary
        || !_autopilotVersionObservationClock.isValid()
        || (requireQuiescent && !fleetMessageIntervalQueryTransportQuiescent())) return false;
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    if (_autopilotVersionObservationReceivedAtMs < 0 || now < _autopilotVersionObservationReceivedAtMs
        || now - _autopilotVersionObservationReceivedAtMs > 30000) return false;
    if (query->active && (!_mavCommandQuarantineClock.isValid()
        || _mavCommandQuarantineClock.elapsed() >= query->source.messageCommandDeadlineMs
        || _messageCommandLaneId != query->source.messageIntervalRequestId
        || _messageCommandLaneDeadlineMs != query->source.messageCommandDeadlineMs
        || _messageCommandLaneSerial != query->source.messageCommandSerial || _messageCommandLaneRetired)) return false;
    if (query->ledgerBound) {
        const WeakLinkInterfacePtr current = _optionalTelemetryPrimaryOwner;
        if (_optionalTelemetryPrimary != query->source.messageIntervalPrimary
            || _optionalTelemetryVersionEpoch != query->source.messageIntervalSourceEpoch
            || current.expired() || current.owner_before(query->source.messageIntervalPrimaryOwner)
            || query->source.messageIntervalPrimaryOwner.owner_before(current)) return false;
    }
    if (query->reportObserved) {
        const auto row = _optionalTelemetryRows.constFind(query->messageId);
        const qint64 receiptNow = _optionalTelemetryClock.elapsed();
        if (!query->ledgerBound || row == _optionalTelemetryRows.constEnd() || !row->reportedObserved
            || row->contentRevision != query->contentRevision || row->intervalUs != query->intervalUs
            || row->reportedReceiptSequence < query->receiptSequence || row->reportedAtMs < 0
            || receiptNow < row->reportedAtMs || receiptNow - row->reportedAtMs > 30000) return false;
    }
    return true;
}

bool Vehicle::_fleetMessageIntervalQueryAllowed(const std::shared_ptr<FleetMessageIntervalQuery>& query)
{
    const QPointer<Vehicle> self(this);
    if (!query || !query->active || _fleetMessageIntervalActiveQuery != query || query->authorizationBusy
        || !query->authorizationFence || !_fleetMessageIntervalQuerySourceCurrent(query, true)) return false;
    query->authorizationBusy = true;
    const auto release = qScopeGuard([self, query]() { if (self) query->authorizationBusy = false; });
    const auto settled = [&]() {
        if (!self || !query->active || self->_fleetMessageIntervalActiveQuery != query
            || !self->_fleetMessageIntervalQuerySourceCurrent(query, true)
            || !self->_messageIntervalSourceCurrent(query->source)
            || !self->_fleetIntervalQueryInitialConnect
            || self->_fleetIntervalQueryInitialConnect.data() != self->_initialConnectStateMachine
            || !self->isInitialConnectComplete() || !self->px4Firmware() || !self->multiRotor()
            || self->armed() || self->flying() || self->_fleetStartupEntryBusy || self->_fleetStartupActive
            || !query->parameters || self->_parameterManager != query->parameters
            || query->parameters->_parameterObservationEpoch != query->parameterEpoch
            || !query->parameters->_parameterObservationMemoryContextAllowed()
            || query->parameters->_parameterObservationPendingWrites()
            || !self->_fleetIntervalQueryMission || self->_fleetIntervalQueryMission.data() != self->_missionManager
            || !self->_fleetIntervalQueryFence || self->_fleetIntervalQueryFence.data() != self->_geoFenceManager
            || !self->_fleetIntervalQueryRally || self->_fleetIntervalQueryRally.data() != self->_rallyPointManager
            || self->_fleetIntervalQueryMission->inProgress() || self->_fleetIntervalQueryMission->transportCallInProgress()
            || self->_fleetIntervalQueryFence->inProgress() || self->_fleetIntervalQueryFence->transportCallInProgress()
            || self->_fleetIntervalQueryRally->inProgress() || self->_fleetIntervalQueryRally->transportCallInProgress()
            || !self->_messageCommandSourceFtp || self->_messageCommandSourceFtp.data() != self->_ftpManager
            || !self->_messageCommandSourceFtp->_rgStateMachine.isEmpty()
            || self->_messageCommandSourceFtp->_ackOrNakTimeoutTimer.isActive()
            || self->_fleetMissionObservationReceiveDepth != 0 || self->_flightStateObservationExhausted
            || self->_startupOdometryExhausted || self->_waitForMavlinkMessageResultHandler
            || self->_requestMessageContext || self->_flightStateObservationLink != query->source.messageIntervalPrimary)
            return false;
        // The only permitted queue member is this independently owned 510;
        // it is not a version-refresh entry and cannot borrow that exemption.
        if (self->_mavCommandList.size() != 1
            || self->_mavCommandList.first().messageIntervalRequestId != query->source.messageIntervalRequestId
            || !self->_mavCommandList.first().fleetMessageIntervalQuery) return false;
        for (auto row = query->parameters->_waitingReadParamIndexMap.constBegin();
             row != query->parameters->_waitingReadParamIndexMap.constEnd(); ++row)
            if (!row.value().isEmpty()) return false;
        for (auto row = query->parameters->_waitingReadParamNameMap.constBegin();
             row != query->parameters->_waitingReadParamNameMap.constEnd(); ++row)
            if (!row.value().isEmpty()) return false;
        const qint64 now = self->_autopilotVersionObservationClock.elapsed();
        for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
            const auto row = self->_flightStateObservations.constFind(channel);
            if (row == self->_flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
                || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > 5000) return false;
        }
        const QVariantMap heartbeat = self->_flightStateObservations.value(QStringLiteral("heartbeat")).value;
        const QVariantMap landed = self->_flightStateObservations.value(QStringLiteral("landedState")).value;
        return heartbeat.value(QStringLiteral("armed")).type() == QVariant::Bool
            && !heartbeat.value(QStringLiteral("armed")).toBool()
            && landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
            && landed.value(QStringLiteral("landedStateKnown")).toBool()
            && landed.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND;
    };
    if (!settled()) return false;
    const QPointer<LinkInterface> primary = query->source.messageIntervalPrimary;
    const bool connected = primary->isConnected();
    if (!settled() || !primary || !connected) return false;
    const bool mock = primary->isMockLink();
    if (!settled() || !primary || mock) return false;
    const bool replay = primary->isLogReplay();
    if (!settled() || !primary || replay) return false;
    bool authorized = false;
    {
        // Copy and destroy the closure locally; its invocation/destructor may
        // cancel, destroy the owner or delete Vehicle. Never retain a list row.
        const auto fence = query->authorizationFence;
        authorized = fence && fence();
    }
    return self && authorized && settled();
}

QVariantMap Vehicle::_fleetMessageIntervalQueryValue(const std::shared_ptr<FleetMessageIntervalQuery>& query,
    const QString& unavailableReason) const
{
    const bool found = QThread::currentThread() == thread() && query && query->owner;
    const bool current = found && _fleetMessageIntervalQuerySourceCurrent(query, true);
    const qint64 now = found && _mavCommandQuarantineClock.isValid() ? _mavCommandQuarantineClock.elapsed() : -1;
    const qint64 remaining = found && now >= 0 ? qMax(qint64(0), query->source.messageCommandDeadlineMs - now) : -1;
    const bool expired = found && query->active && remaining == 0;
    const QString empty = QStringLiteral("");
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("reportType"), QStringLiteral("fleet-message-interval-query")},
        {QStringLiteral("requestAccepted"), found},
        {QStringLiteral("requestId"), found ? query->source.messageIntervalRequestId : empty},
        {QStringLiteral("active"), found && query->active && !expired},
        {QStringLiteral("state"), !found ? QStringLiteral("unavailable") : expired ? QStringLiteral("failed") : query->state},
        {QStringLiteral("reason"), !found ? unavailableReason : expired ? QStringLiteral("fleet-interval-query-deadline") : query->reason},
        {QStringLiteral("vehicleId"), found ? query->source.messageCommandVehicleId : 0},
        {QStringLiteral("componentId"), found ? query->source.targetCompId : 0},
        {QStringLiteral("messageId"), found ? query->messageId : 0},
        {QStringLiteral("requestIssued"), found && query->requestIssued},
        {QStringLiteral("ackObserved"), found && query->ackObserved},
        {QStringLiteral("ackResult"), found && query->ackObserved ? QVariant(query->ackResult) : QVariant::fromValue(nullptr)},
        {QStringLiteral("reportObserved"), found && query->reportObserved},
        {QStringLiteral("intervalUs"), found && query->reportObserved ? QVariant(query->intervalUs) : QVariant::fromValue(nullptr)},
        {QStringLiteral("receiptSequence"), QString::number(found ? query->receiptSequence : 0)},
        {QStringLiteral("baselineReceiptSequence"), QString::number(found ? query->baselineReceiptSequence : 0)},
        {QStringLiteral("contentRevision"), QString::number(found ? query->contentRevision : 0)},
        {QStringLiteral("versionEpoch"), found ? query->source.messageIntervalSourceEpoch : empty},
        {QStringLiteral("transportEpoch"), found ? query->source.messageCommandTransportEpoch : empty},
        {QStringLiteral("mutationGeneration"), QString::number(found ? query->mutationGeneration : 0)},
        {QStringLiteral("ledgerEpoch"), found && query->ledgerBound ? query->ledgerEpoch : empty},
        {QStringLiteral("sourceCurrent"), current}, {QStringLiteral("remainingMs"), QVariant::fromValue(remaining)},
        {QStringLiteral("readOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("responseAttributionVerified"), false}, {QStringLiteral("actualTransmitRateVerified"), false},
        {QStringLiteral("availableBandwidthVerified"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap Vehicle::_fleetMessageIntervalQueryRetiredValue(const std::shared_ptr<FleetMessageIntervalQuery>& query)
{
    if (!query) return {};
    const QString empty = QStringLiteral("");
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-message-interval-query")},
        {QStringLiteral("requestAccepted"), true}, {QStringLiteral("requestId"), query->source.messageIntervalRequestId},
        {QStringLiteral("active"), false}, {QStringLiteral("state"), query->active ? QStringLiteral("failed") : query->state},
        {QStringLiteral("reason"), query->active ? QStringLiteral("fleet-interval-query-vehicle-destroyed-historical") : query->reason},
        {QStringLiteral("vehicleId"), query->source.messageCommandVehicleId}, {QStringLiteral("componentId"), query->source.targetCompId},
        {QStringLiteral("messageId"), query->messageId}, {QStringLiteral("requestIssued"), query->requestIssued},
        {QStringLiteral("ackObserved"), query->ackObserved},
        {QStringLiteral("ackResult"), query->ackObserved ? QVariant(query->ackResult) : QVariant::fromValue(nullptr)},
        {QStringLiteral("reportObserved"), query->reportObserved},
        {QStringLiteral("intervalUs"), query->reportObserved ? QVariant(query->intervalUs) : QVariant::fromValue(nullptr)},
        {QStringLiteral("receiptSequence"), QString::number(query->receiptSequence)},
        {QStringLiteral("baselineReceiptSequence"), QString::number(query->baselineReceiptSequence)},
        {QStringLiteral("contentRevision"), QString::number(query->contentRevision)},
        {QStringLiteral("versionEpoch"), query->source.messageIntervalSourceEpoch},
        {QStringLiteral("transportEpoch"), query->source.messageCommandTransportEpoch},
        {QStringLiteral("mutationGeneration"), QString::number(query->mutationGeneration)},
        {QStringLiteral("ledgerEpoch"), query->ledgerBound ? query->ledgerEpoch : empty},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("remainingMs"), QVariant::fromValue(qint64(0))},
        {QStringLiteral("readOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("responseAttributionVerified"), false}, {QStringLiteral("actualTransmitRateVerified"), false},
        {QStringLiteral("availableBandwidthVerified"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap Vehicle::fleetMessageIntervalQuerySnapshot(QObject* actualOwner, const QString& requestId) const
{
    if (QThread::currentThread() != thread() || !actualOwner || requestId.isEmpty())
        return _fleetMessageIntervalQueryValue(nullptr);
    for (const auto& query : _fleetMessageIntervalQueries)
        if (query->owner == actualOwner && query->source.messageIntervalRequestId == requestId)
            return _fleetMessageIntervalQueryValue(query);
    return _fleetMessageIntervalQueryValue(nullptr);
}

void Vehicle::_finishFleetMessageIntervalQuery(const std::shared_ptr<FleetMessageIntervalQuery>& query,
    const QString& state, const QString& reason, bool quarantine)
{
    if (!query || !query->active || _fleetMessageIntervalActiveQuery != query) return;
    query->active = false; query->state = state; query->reason = reason;
    const QString requestId = query->source.messageIntervalRequestId;
    const int index = _messageCommandIndex(requestId);
    if (index >= 0) _mavCommandList.removeAt(index);
    if (quarantine) {
        // Cancellation and failure retain at least the ORIGINAL reservation
        // horizon, not just the shortened post-ACK wait. No rollback claim.
        _quarantineMavCommand(query->source.targetCompId, MAV_CMD_GET_MESSAGE_INTERVAL, 4000);
        _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, query->originalDeadlineMs);
    }
    _fleetMessageIntervalActiveQuery.reset();
    _releaseMessageCommand(requestId);
}

bool Vehicle::cancelFleetMessageIntervalQuery(QObject* actualOwner, const QString& requestId)
{
    if (QThread::currentThread() != thread() || !actualOwner || requestId.isEmpty()) return false;
    for (int index = 0; index < _fleetMessageIntervalQueries.size(); ++index) {
        const auto query = _fleetMessageIntervalQueries.at(index);
        if (query->owner != actualOwner || query->source.messageIntervalRequestId != requestId) continue;
        if (query->active) _finishFleetMessageIntervalQuery(query, QStringLiteral("cancelled"),
            QStringLiteral("fleet-interval-query-cancelled"), true);
        _fleetMessageIntervalQueries.removeAt(index); // Release only this retained receipt, including a cancelled active one.
        return true;
    }
    return false;
}

QVariantMap Vehicle::beginFleetMessageIntervalQuery(QObject* actualOwner, LinkInterface* expectedPrimary,
    int messageId, std::function<bool()> authorizationFence)
{
    return _beginFleetMessageIntervalQuery(actualOwner, expectedPrimary, messageId, std::move(authorizationFence), QStringLiteral(""));
}

QVariantMap Vehicle::_beginFleetMessageIntervalQuery(QObject* actualOwner, LinkInterface* expectedPrimary,
    int messageId, std::function<bool()> authorizationFence, const QString& configurationRunId)
{
    if (QThread::currentThread() != thread() || !actualOwner || actualOwner->thread() != thread()
        || !expectedPrimary || !optionalTelemetryMessageId(messageId) || !authorizationFence)
        return _fleetMessageIntervalQueryValue(nullptr, QStringLiteral("fleet-interval-query-invalid-input"));
    if (!fleetMessageIntervalQueryTransportQuiescent())
        return _fleetMessageIntervalQueryValue(nullptr, QStringLiteral("fleet-interval-query-transport-not-quiescent"));
    const QPointer<Vehicle> self(this);
    const QPointer<QObject> owner(actualOwner);
    const QPointer<LinkInterface> primary(expectedPrimary);
    // Do not drain the generic command dispatcher here: it may publish an
    // unrelated legacy user callback. This service only disposes its own IDs.
    const auto oldActive = _fleetMessageIntervalActiveQuery;
    if (oldActive && !oldActive->owner) _finishFleetMessageIntervalQuery(oldActive,
        QStringLiteral("failed"), QStringLiteral("fleet-interval-query-owner-destroyed"), true);
    QList<std::shared_ptr<FleetMessageIntervalQuery>> retired;
    for (int index = _fleetMessageIntervalQueries.size() - 1; index >= 0; --index)
        if (!_fleetMessageIntervalQueries.at(index)->owner && !_fleetMessageIntervalQueries.at(index)->active) {
            retired.append(_fleetMessageIntervalQueries.at(index));
            _fleetMessageIntervalQueries.removeAt(index);
        }
    retired.clear(); // A captured closure's destructor can reenter or delete Vehicle.
    if (!self) return {};
    if (!owner || !primary || _fleetMessageIntervalActiveQuery || _fleetMessageIntervalQueries.size() >= 3)
        return _fleetMessageIntervalQueryValue(nullptr, QStringLiteral("fleet-interval-query-busy"));
    MavCommandListEntry_t entry{};
    entry.command = MAV_CMD_GET_MESSAGE_INTERVAL; entry.targetCompId = _defaultComponentId;
    entry.frame = MAV_FRAME_GLOBAL; entry.rgParam1 = float(messageId);
    entry.maxTries = 1; entry.ackTimeoutMSecs = 4000; entry.showError = false;
    entry.fleetMessageIntervalQuery = true;
    entry.fleetOptionalTelemetryConfigurationRunId = configurationRunId;
    if (!_reserveMessageCommand(&entry))
        return _fleetMessageIntervalQueryValue(nullptr, QStringLiteral("fleet-interval-query-lane-unavailable"));
    entry.messageCommandDeadlineMs = entry.messageCommandReservedAtMs + 4000;
    _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
    const auto query = std::make_shared<FleetMessageIntervalQuery>();
    query->owner = owner; query->source = entry; query->messageId = messageId;
    query->originalDeadlineMs = entry.messageCommandDeadlineMs;
    if (!configurationRunId.isEmpty() && _fleetOptionalTelemetryConfigurationRun
        && _fleetOptionalTelemetryConfigurationRun->id == configurationRunId) {
        entry.messageCommandDeadlineMs = qMin(entry.messageCommandDeadlineMs, _fleetOptionalTelemetryConfigurationRun->basis->deadlineMs);
        _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
        query->source = entry; query->originalDeadlineMs = entry.messageCommandDeadlineMs;
    }
    query->parameters = _parameterManager;
    query->parameterEpoch = query->parameters ? query->parameters->_parameterObservationEpoch : QStringLiteral("");
    query->versionRevision = _autopilotVersionObservationRevision;
    query->mutationGeneration = _optionalTelemetryMutationGeneration;
    query->baselineReceiptSequence = _optionalTelemetryNextReceipt;
    query->authorizationFence = std::move(authorizationFence);
    // An unbound passive ledger is not a rejection. Its first actual eligible
    // 244 may bind once under this frozen transport/version/mutation identity.
    query->ledgerBound = bool(_optionalTelemetryPrimary);
    query->ledgerEpoch = _optionalTelemetryLedgerEpoch;
    _fleetMessageIntervalQueries.append(query);
    _fleetMessageIntervalActiveQuery = query;
    _mavCommandList.append(entry);
    if (!configurationRunId.isEmpty() && _fleetOptionalTelemetryConfigurationRun
        && _fleetOptionalTelemetryConfigurationRun->active && _fleetOptionalTelemetryConfigurationRun->id == configurationRunId
        && _fleetOptionalTelemetryConfigurationRun->owner == owner) {
        // Pin the actual owned child BEFORE any live/auth/plugin/write call.
        // Vehicle destruction during begin cannot discard its issued facts.
        _fleetOptionalTelemetryConfigurationRun->currentQuery = query;
        _fleetOptionalTelemetryConfigurationRun->wireDeadlineMs = query->source.messageCommandDeadlineMs;
    }
    if (primary != entry.messageIntervalPrimary || !_fleetMessageIntervalQueryAllowed(query)) {
        if (!self) return {};
        _finishFleetMessageIntervalQuery(query, QStringLiteral("failed"), QStringLiteral("fleet-interval-query-not-authorized"), true);
        return _fleetMessageIntervalQueryValue(query);
    }
    if (!self) return {};
    const int index = _messageCommandIndex(entry.messageIntervalRequestId);
    if (index < 0 || _fleetMessageIntervalActiveQuery != query || !query->active)
        return _fleetMessageIntervalQueryValue(query);
    _mavCommandList[index].messageCommandPrepared = true;
    query->source.messageCommandPrepared = true;
    _sendMessageCommandFromList(entry.messageIntervalRequestId, nullptr);
    if (!self) return {};
    return _fleetMessageIntervalQueryValue(query);
}

void Vehicle::_observeFleetMessageIntervalQuery(const std::shared_ptr<FleetMessageIntervalQuery>& query,
    const mavlink_message_t& original, LinkInterface* sourceLink)
{
    if (!query || !query->active || query->reportObserved || _fleetMessageIntervalActiveQuery != query
        || !query->source.messageCommandWriteStarted || !_fleetMessageIntervalQuerySourceCurrent(query, false)
        || !_messageIntervalSourceCurrent(query->source) || sourceLink != query->source.messageIntervalPrimary.data()
        || sender() != query->source.messageIntervalProtocol.data() || original.sysid != query->source.messageCommandVehicleId
        || original.compid != query->source.targetCompId || original.msgid != MAVLINK_MSG_ID_MESSAGE_INTERVAL
        || original.len < 1 || original.len > MAVLINK_MSG_ID_MESSAGE_INTERVAL_LEN
        || (original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)
        || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != MAVLINK_MSG_ID_MESSAGE_INTERVAL_MIN_LEN)) return;
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    mavlink_message_interval_t value{};
    mavlink_msg_message_interval_decode(&normalized, &value);
    if (value.message_id != query->messageId) return;
    const auto row = _optionalTelemetryRows.constFind(query->messageId);
    const WeakLinkInterfacePtr observedOwner = _optionalTelemetryPrimaryOwner;
    if (_optionalTelemetryReceiveDepth != 1 || _optionalTelemetryProbeBusy
        || _optionalTelemetryPrimary != query->source.messageIntervalPrimary
        || _optionalTelemetryVersionEpoch != query->source.messageIntervalSourceEpoch
        || observedOwner.expired() || observedOwner.owner_before(query->source.messageIntervalPrimaryOwner)
        || query->source.messageIntervalPrimaryOwner.owner_before(observedOwner)
        || row == _optionalTelemetryRows.constEnd() || !row->reportedObserved
        || row->reportedReceiptSequence <= query->baselineReceiptSequence || row->intervalUs != value.interval_us) {
        _finishFleetMessageIntervalQuery(query, QStringLiteral("failed"), QStringLiteral("fleet-interval-query-report-source-unavailable"), true);
        return;
    }
    if (!query->ledgerBound) {
        _finishFleetMessageIntervalQuery(query, QStringLiteral("failed"), QStringLiteral("fleet-interval-query-ledger-unbound"), true);
        return;
    }
    query->reportObserved = true; query->intervalUs = value.interval_us;
    query->receiptSequence = row->reportedReceiptSequence; query->contentRevision = row->contentRevision;
    if (value.interval_us < -1) {
        _finishFleetMessageIntervalQuery(query, QStringLiteral("failed"), QStringLiteral("fleet-interval-query-invalid-interval"), true);
    } else if (query->ackObserved && query->ackResult == MAV_RESULT_ACCEPTED) {
        _finishFleetMessageIntervalQuery(query, QStringLiteral("complete"), QStringLiteral(""), false);
    } else query->state = QStringLiteral("waiting-ack");
}

bool Vehicle::_fleetOptionalTelemetryBasisMemoryCurrent(
    const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis, bool requireRows) const
{
    if (!basis || basis->vehicle != this || !basis->captureOwner || basis->captureOwner->thread() != thread()
        || QThread::currentThread() != thread() || basis->queries.size() != 3
        || !_mavCommandQuarantineClock.isValid() || _mavCommandQuarantineClock.elapsed() >= basis->deadlineMs
        || !_fleetExecutionSourceVehicles || _fleetExecutionSourceVehicles->getVehicleById(_id) != this
        || _autopilotVersionObservationExhausted || !_autopilotVersionObservationObserved
        || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || !_autopilotVersionObservationClock.isValid()
        || _optionalTelemetryMutationExhausted || _optionalTelemetryExhausted) return false;
    const auto first = basis->queries.first();
    if (!first || !_messageCommandSourceMemoryCurrent(first->source)
        || _autopilotVersionObservationEpoch != first->source.messageIntervalSourceEpoch
        || _autopilotVersionObservationRevision != first->versionRevision
        || _autopilotVersionObservationLink != first->source.messageIntervalPrimary) return false;
    const qint64 versionNow = _autopilotVersionObservationClock.elapsed();
    if (_autopilotVersionObservationReceivedAtMs < 0 || versionNow < _autopilotVersionObservationReceivedAtMs
        || versionNow - _autopilotVersionObservationReceivedAtMs > 30000) return false;
    for (int index = 0; index < 3; ++index) {
        const auto query = basis->queries.at(index);
        if (!query || query->owner != basis->captureOwner || query->active || query->state != QStringLiteral("complete")
            || !query->requestIssued || !query->ackObserved || query->ackResult != MAV_RESULT_ACCEPTED
            || !query->reportObserved || query->intervalUs < -1
            || query->source.messageIntervalSourceEpoch != first->source.messageIntervalSourceEpoch
            || query->source.messageCommandTransportEpoch != first->source.messageCommandTransportEpoch
            || query->source.messageIntervalPrimary != first->source.messageIntervalPrimary
            || query->source.messageIntervalPrimaryOwner.owner_before(first->source.messageIntervalPrimaryOwner)
            || first->source.messageIntervalPrimaryOwner.owner_before(query->source.messageIntervalPrimaryOwner)
            || query->versionRevision != first->versionRevision || query->mutationGeneration != first->mutationGeneration
            || query->ledgerEpoch != first->ledgerEpoch
            || (requireRows && !_fleetMessageIntervalQuerySourceCurrent(query, true))) return false;
    }
    return true;
}

bool Vehicle::_fleetOptionalTelemetryOriginCurrent(const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const
{
    if (!_fleetOptionalTelemetryBasisMemoryCurrent(basis, false) || !basis->sourceFence || basis->checkingSource) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    basis->checkingSource = true;
    const auto release = qScopeGuard([self, basis]() { if (self) basis->checkingSource = false; });
    bool valid = false;
    { const auto fence = basis->sourceFence; valid = fence && fence(); }
    return self && valid && _fleetOptionalTelemetryBasisMemoryCurrent(basis, false);
}

std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> Vehicle::sealFleetOptionalTelemetryCapture(QObject* actualCapture,
    const QStringList& queryIds, qint64 remainingMs, std::function<bool()> pureSourceFence)
{
    if (QThread::currentThread() != thread() || !actualCapture || actualCapture->thread() != thread()
        || !fleetMessageIntervalQueryTransportQuiescent() || queryIds.size() != 3 || remainingMs < 1 || remainingMs > 30000
        || !pureSourceFence || !_mavCommandQuarantineClock.isValid()) return {};
    const QPointer<Vehicle> self(this);
    const qint64 enteredAt = _mavCommandQuarantineClock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - remainingMs) return {};
    const auto basis = std::make_shared<FleetOptionalTelemetryConfigurationBasis>();
    basis->vehicle = this; basis->captureOwner = actualCapture;
    basis->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    basis->deadlineMs = enteredAt + remainingMs; basis->sourceFence = std::move(pureSourceFence);
    const int ids[] = {62, 74, 83};
    for (int index = 0; index < 3; ++index) {
        std::shared_ptr<FleetMessageIntervalQuery> found;
        for (const auto& query : _fleetMessageIntervalQueries)
            if (query->owner == actualCapture && query->source.messageIntervalRequestId == queryIds.at(index)
                && query->messageId == ids[index] && !query->configurationSealed) found = query;
        if (!found) return {};
        basis->queries.append(found);
    }
    if (!_fleetOptionalTelemetryBasisMemoryCurrent(basis, true) || !_fleetOptionalTelemetryOriginCurrent(basis)) return {};
    if (!self || !_fleetOptionalTelemetryBasisMemoryCurrent(basis, true)) return {};
    // Rows may only be sealed once while they are still the actual private
    // terminal contexts. Their maps/UUIDs alone cannot re-create this object.
    for (const auto& query : basis->queries) if (query->configurationSealed) return {};
    for (const auto& query : basis->queries) query->configurationSealed = true;
    return basis;
}

bool Vehicle::fleetOptionalTelemetryConfigurationBasisCurrent(
    const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    if (!basis || basis->consumed || !_fleetOptionalTelemetryBasisMemoryCurrent(basis, true)) return false;
    const bool origin = _fleetOptionalTelemetryOriginCurrent(basis);
    return self && origin && !basis->consumed && _fleetOptionalTelemetryBasisMemoryCurrent(basis, true);
}

QVariantMap Vehicle::fleetOptionalTelemetryConfigurationBasisSummary(
    const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    if (!basis || basis->vehicle != this || QThread::currentThread() != thread() || basis->queries.size() != 3) return {};
    const bool current = fleetOptionalTelemetryConfigurationBasisCurrent(basis);
    if (!self) return {};
    const auto first = basis->queries.first();
    QVariantList rows;
    for (const auto& query : basis->queries) rows.append(QVariantMap{{QStringLiteral("messageId"), query->messageId},
        {QStringLiteral("intervalUs"), query->intervalUs}, {QStringLiteral("contentRevision"), QString::number(query->contentRevision)}});
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("reportType"), QStringLiteral("fleet-optional-telemetry-configuration-basis")},
        {QStringLiteral("basisId"), basis->id}, {QStringLiteral("vehicleId"), first->source.messageCommandVehicleId},
        {QStringLiteral("componentId"), first->source.targetCompId}, {QStringLiteral("sourceCurrent"), current},
        {QStringLiteral("consumed"), basis->consumed},
        {QStringLiteral("remainingMs"), QVariant::fromValue(qMax(qint64(0), basis->deadlineMs - _mavCommandQuarantineClock.elapsed()))},
        {QStringLiteral("originalMutationGeneration"), QString::number(first->mutationGeneration)},
        {QStringLiteral("ledgerEpoch"), first->ledgerEpoch}, {QStringLiteral("rows"), rows},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool Vehicle::_fleetOptionalTelemetryRunSourceCurrent(
    const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run, bool requireQuiescent) const
{
    if (!run || run->vehicle != this || !run->owner || run->owner->thread() != thread()
        || !_fleetOptionalTelemetryBasisMemoryCurrent(run->basis, false)
        || !run->basis->consumed || _optionalTelemetryMutationGeneration != run->expectedMutation
        || run->ledgerEpoch.isEmpty() || run->ledgerEpoch != _optionalTelemetryLedgerEpoch
        || (requireQuiescent && !fleetMessageIntervalQueryTransportQuiescent())) return false;
    const auto first = run->basis->queries.first();
    if (run->ledgerBound) {
        if (_optionalTelemetryPrimary != first->source.messageIntervalPrimary
            || _optionalTelemetryVersionEpoch != first->source.messageIntervalSourceEpoch
            || _optionalTelemetryPrimaryOwner.expired()
            || _optionalTelemetryPrimaryOwner.owner_before(first->source.messageIntervalPrimaryOwner)
            || first->source.messageIntervalPrimaryOwner.owner_before(_optionalTelemetryPrimaryOwner)) return false;
    }
    if (run->stage >= 6) {
        // The final sweep is all under the final own mutation. A later
        // A->B->A report cannot be hidden by reusing an earlier per-step row.
        for (const auto& row : run->rows)
            if (row.finalReadback && !_fleetMessageIntervalQuerySourceCurrent(row.finalReadback, requireQuiescent)) return false;
    }
    return true;
}

bool Vehicle::_fleetOptionalTelemetryRunAllowed(const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run)
{
    const QPointer<Vehicle> self(this);
    if (!run || !run->active || _fleetOptionalTelemetryConfigurationRun != run || run->authorizationBusy
        || !run->finalFence || !_fleetOptionalTelemetryRunSourceCurrent(run, true)) return false;
    run->authorizationBusy = true;
    const auto release = qScopeGuard([self, run]() { if (self) run->authorizationBusy = false; });
    const auto first = run->basis->queries.first();
    const auto settled = [&]() {
        if (!self || !run->active || self->_fleetOptionalTelemetryConfigurationRun != run
            || !self->_fleetOptionalTelemetryRunSourceCurrent(run, true)
            || !self->_fleetIntervalQueryInitialConnect || self->_fleetIntervalQueryInitialConnect.data() != self->_initialConnectStateMachine
            || !self->isInitialConnectComplete() || !self->px4Firmware() || !self->multiRotor() || self->armed() || self->flying()
            || self->_fleetStartupActive || self->_fleetStartupEntryBusy || !first->parameters
            || self->_parameterManager != first->parameters || first->parameters->_parameterObservationEpoch != first->parameterEpoch
            || !first->parameters->_parameterObservationMemoryContextAllowed() || first->parameters->_parameterObservationPendingWrites()
            || !self->_fleetIntervalQueryMission || self->_fleetIntervalQueryMission.data() != self->_missionManager
            || !self->_fleetIntervalQueryFence || self->_fleetIntervalQueryFence.data() != self->_geoFenceManager
            || !self->_fleetIntervalQueryRally || self->_fleetIntervalQueryRally.data() != self->_rallyPointManager
            || self->_fleetIntervalQueryMission->inProgress() || self->_fleetIntervalQueryMission->transportCallInProgress()
            || self->_fleetIntervalQueryFence->inProgress() || self->_fleetIntervalQueryFence->transportCallInProgress()
            || self->_fleetIntervalQueryRally->inProgress() || self->_fleetIntervalQueryRally->transportCallInProgress()
            || !self->_messageCommandSourceFtp || self->_messageCommandSourceFtp.data() != self->_ftpManager
            || !self->_messageCommandSourceFtp->_rgStateMachine.isEmpty() || self->_messageCommandSourceFtp->_ackOrNakTimeoutTimer.isActive()
            || self->_requestMessageContext || self->_waitForMavlinkMessageResultHandler
            || self->_flightStateObservationLink != first->source.messageIntervalPrimary
            || self->_flightStateObservationExhausted || self->_startupOdometryExhausted) return false;
        if (self->_mavCommandList.size() > 1 || (!self->_mavCommandList.isEmpty()
            && self->_mavCommandList.first().fleetOptionalTelemetryConfigurationRunId != run->id)) return false;
        for (auto row = first->parameters->_waitingReadParamIndexMap.constBegin(); row != first->parameters->_waitingReadParamIndexMap.constEnd(); ++row)
            if (!row.value().isEmpty()) return false;
        for (auto row = first->parameters->_waitingReadParamNameMap.constBegin(); row != first->parameters->_waitingReadParamNameMap.constEnd(); ++row)
            if (!row.value().isEmpty()) return false;
        const qint64 now = self->_autopilotVersionObservationClock.elapsed();
        for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
            const auto row = self->_flightStateObservations.constFind(channel);
            if (row == self->_flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
                || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > 5000) return false;
        }
        const QVariantMap hb = self->_flightStateObservations.value(QStringLiteral("heartbeat")).value;
        const QVariantMap landed = self->_flightStateObservations.value(QStringLiteral("landedState")).value;
        return hb.value(QStringLiteral("armed")).type() == QVariant::Bool && !hb.value(QStringLiteral("armed")).toBool()
            && landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
            && landed.value(QStringLiteral("landedStateKnown")).toBool()
            && landed.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND;
    };
    if (!settled()) return false;
    const QPointer<LinkInterface> primary = first->source.messageIntervalPrimary;
    const bool connected = primary->isConnected();
    if (!settled() || !primary || !connected) return false;
    const bool mock = primary->isMockLink();
    if (!settled() || !primary || mock) return false;
    const bool replay = primary->isLogReplay();
    if (!settled() || !primary || replay) return false;
    const bool origin = _fleetOptionalTelemetryOriginCurrent(run->basis);
    if (!self || !origin || !settled()) return false;
    bool allowed = false;
    { const auto fence = run->finalFence; allowed = fence && fence(); }
    if (!self || !allowed || !settled()) return false;
    const bool finalOrigin = _fleetOptionalTelemetryOriginCurrent(run->basis);
    return self && finalOrigin && settled();
}

std::shared_ptr<FleetOptionalTelemetryConfigurationRun> Vehicle::startFleetOptionalTelemetryConfiguration(QObject* actualConfig,
    const std::shared_ptr<FleetOptionalTelemetryConfigurationBasis>& basis, int intervalUs,
    std::function<bool()> actualTwoAccountFinalFence)
{
    if (QThread::currentThread() != thread() || !actualConfig || actualConfig->thread() != thread()
        || !fleetMessageIntervalQueryTransportQuiescent() || !actualTwoAccountFinalFence || intervalUs <= 0
        || double(float(intervalUs)) != double(intervalUs) || _fleetOptionalTelemetryConfigurationRun
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || !_messageCommandLaneId.isEmpty() || !_mavCommandList.isEmpty() || _fleetMessageIntervalActiveQuery
        || _requestMessageContext || _waitForMavlinkMessageResultHandler) return {};
    const QPointer<Vehicle> self(this);
    const QPointer<QObject> owner(actualConfig);
    const bool current = fleetOptionalTelemetryConfigurationBasisCurrent(basis);
    if (!self || !owner || !current || _fleetOptionalTelemetryConfigurationRun || basis->consumed
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)) return {};
    for (const auto& query : basis->queries) if (query->intervalUs <= 0 || intervalUs < query->intervalUs) return {};
    const auto run = std::make_shared<FleetOptionalTelemetryConfigurationRun>();
    run->vehicle = this; run->owner = owner; run->basis = basis; run->intervalUs = intervalUs;
    run->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    run->expectedMutation = basis->queries.first()->mutationGeneration;
    run->ledgerEpoch = basis->queries.first()->ledgerEpoch; run->rows.resize(3);
    run->finalFence = std::move(actualTwoAccountFinalFence);
    basis->consumed = true; // One native run, including failure before any write.
    _fleetOptionalTelemetryConfigurationRun = run;
    const bool allowed = _fleetOptionalTelemetryRunAllowed(run);
    if (!self) return run;
    if (!allowed) _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-final-authority-denied"), false);
    return run;
}

void Vehicle::_finishFleetOptionalTelemetryConfiguration(const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run,
    const QString& state, const QString& reason, bool quarantine)
{
    if (!run || !run->active || _fleetOptionalTelemetryConfigurationRun != run) return;
    run->active = false; run->state = state; run->reason = reason;
    if (quarantine && run->currentQuery) {
        _quarantineMavCommand(run->currentQuery->source.targetCompId, MAV_CMD_GET_MESSAGE_INTERVAL, 4000);
        _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, run->currentQuery->originalDeadlineMs);
    }
    const auto activeQuery = _fleetMessageIntervalActiveQuery;
    if (activeQuery && activeQuery->source.fleetOptionalTelemetryConfigurationRunId == run->id) {
        if (!run->currentQuery) run->currentQuery = activeQuery; // Preserve a late begin's actual write facts.
        if (activeQuery->active) _finishFleetMessageIntervalQuery(activeQuery, QStringLiteral("cancelled"),
            QStringLiteral("configuration-run-retired"), quarantine);
    }
    if (run->currentQuery && run->currentQuery->active)
        _finishFleetMessageIntervalQuery(run->currentQuery, QStringLiteral("cancelled"), QStringLiteral("configuration-run-retired"), quarantine);
    for (int index = _mavCommandList.size() - 1; index >= 0; --index) {
        const auto entry = _mavCommandList.at(index);
        if (entry.fleetOptionalTelemetryConfigurationRunId != run->id) continue;
        if (quarantine) {
            _quarantineMavCommand(entry.targetCompId, entry.command, 4000);
            _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, entry.messageCommandDeadlineMs);
        }
        _mavCommandList.removeAt(index); _releaseMessageCommand(entry.messageIntervalRequestId);
    }
    // The Run keeps native child receipts for its report; free only the live
    // Vehicle slots, never a newer owner's lane or a serialized replacement.
    for (int index = _fleetMessageIntervalQueries.size() - 1; index >= 0; --index)
        if (_fleetMessageIntervalQueries.at(index)->source.fleetOptionalTelemetryConfigurationRunId == run->id)
            _fleetMessageIntervalQueries.removeAt(index);
    for (const auto& row : run->rows) _releaseMessageCommand(row.setRequestId);
    _fleetOptionalTelemetryConfigurationRun.reset();
}

bool Vehicle::cancelFleetOptionalTelemetryConfiguration(QObject* actualConfig,
    const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run)
{
    if (QThread::currentThread() != thread() || !run || run->vehicle != this || run->owner != actualConfig || !actualConfig) return false;
    if (run->active) _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("cancelled"), QStringLiteral("configuration-operator-cancelled-no-rollback"), true);
    return true;
}

bool Vehicle::advanceFleetOptionalTelemetryConfiguration(QObject* actualConfig,
    const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run)
{
    if (QThread::currentThread() != thread() || !run || run->vehicle != this || run->owner != actualConfig || !actualConfig) return false;
    if (!run->active) return true;
    if (_fleetOptionalTelemetryConfigurationRun != run) return false;
    const qint64 now = _mavCommandQuarantineClock.isValid() ? _mavCommandQuarantineClock.elapsed() : -1;
    if (!run->owner || !run->basis->captureOwner || now < 0 || now >= run->basis->deadlineMs) {
        _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-source-or-original-deadline-expired"), true);
        return true;
    }
    if ((run->wireDeadlineMs >= 0 && now >= run->wireDeadlineMs)
        || (run->currentQuery && run->currentQuery->active
            && now >= run->currentQuery->source.messageCommandDeadlineMs)) {
        _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-original-wire-deadline-expired"), true);
        return true;
    }
    if (!fleetMessageIntervalQueryTransportQuiescent()) return true;
    if (!_fleetOptionalTelemetryRunSourceCurrent(run, false)) {
        _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-original-source-changed"), true);
        return true;
    }
    const QPointer<Vehicle> self(this);
    if (!_fleetOptionalTelemetryRunAllowed(run)) {
        if (self) _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-final-authority-changed"), true);
        return true;
    }
    if (!self) return true;
    if (run->state == QStringLiteral("waiting-set-ack")) return true;
    if (run->currentQuery) {
        const auto query = run->currentQuery;
        if (query->active) return true;
        if (query->state != QStringLiteral("complete") || !_fleetMessageIntervalQuerySourceCurrent(query, true)
            || query->intervalUs != run->intervalUs) {
            _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-readback-not-matching"), true);
            return true;
        }
        const int rowIndex = run->stage < 6 ? run->stage / 2 : run->stage - 6;
        if (run->stage < 6) {
            run->rows[rowIndex].readback = query;
            cancelFleetMessageIntervalQuery(actualConfig, query->source.messageIntervalRequestId);
        } else run->rows[rowIndex].finalReadback = query;
        run->currentQuery.reset(); ++run->stage; run->wireDeadlineMs = -1; run->state = QStringLiteral("ready");
        if (run->stage == 9) {
            const bool matching = _fleetOptionalTelemetryRunSourceCurrent(run, true);
            _finishFleetOptionalTelemetryConfiguration(run, matching ? QStringLiteral("reported-values-match") : QStringLiteral("failed"),
                matching ? QStringLiteral("") : QStringLiteral("configuration-final-sweep-source-changed"), !matching);
        }
        return true; // A later native service turn, never RAW, starts the next wire request.
    }
    const int ids[] = {62, 74, 83};
    const int rowIndex = run->stage < 6 ? run->stage / 2 : run->stage - 6;
    if (rowIndex < 0 || rowIndex > 2) {
        _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-invalid-native-stage"), true); return true;
    }
    if (run->stage < 6 && run->stage % 2 == 0) {
        MavCommandListEntry_t entry{};
        entry.command = MAV_CMD_SET_MESSAGE_INTERVAL; entry.frame = MAV_FRAME_GLOBAL;
        entry.targetCompId = _defaultComponentId; entry.rgParam1 = float(ids[rowIndex]); entry.rgParam2 = float(run->intervalUs);
        entry.maxTries = 1; entry.ackTimeoutMSecs = 4000; entry.showError = false;
        entry.fleetOptionalTelemetryConfigurationRunId = run->id;
        if (!_reserveMessageCommand(&entry)) {
            _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-shared-lane-unavailable"), false); return true;
        }
        entry.messageCommandDeadlineMs = qMin(entry.messageCommandReservedAtMs + 4000, run->basis->deadlineMs);
        _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
        entry.messageCommandPrepared = true;
        run->rows[rowIndex].setRequestId = entry.messageIntervalRequestId;
        run->wireDeadlineMs = entry.messageCommandDeadlineMs;
        run->state = QStringLiteral("waiting-set-ack");
        _mavCommandList.append(entry);
        _sendMessageCommandFromList(entry.messageIntervalRequestId, nullptr);
        return true;
    }
    const std::weak_ptr<FleetOptionalTelemetryConfigurationRun> weakRun(run);
    const auto finalFence = [self, weakRun]() {
        const auto locked = weakRun.lock();
        return self && locked && self->_fleetOptionalTelemetryRunAllowed(locked);
    };
    const QVariantMap started = _beginFleetMessageIntervalQuery(actualConfig,
        run->basis->queries.first()->source.messageIntervalPrimary.data(), ids[rowIndex], finalFence, run->id);
    if (!self) return true;
    const QString id = started.value(QStringLiteral("requestId")).toString();
    std::shared_ptr<FleetMessageIntervalQuery> query;
    for (const auto& candidate : _fleetMessageIntervalQueries)
        if (candidate->source.messageIntervalRequestId == id && candidate->owner == actualConfig
            && candidate->source.fleetOptionalTelemetryConfigurationRunId == run->id) query = candidate;
    if (!run->active || _fleetOptionalTelemetryConfigurationRun != run) {
        if (query) cancelFleetMessageIntervalQuery(actualConfig, id);
        return true;
    }
    if (!query) {
        _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-read-query-not-accepted"), true); return true;
    }
    run->currentQuery = query;
    run->wireDeadlineMs = query->source.messageCommandDeadlineMs;
    run->state = run->stage < 6 ? QStringLiteral("waiting-readback") : QStringLiteral("waiting-final-readback");
    return true;
}

QVariantMap Vehicle::fleetOptionalTelemetryConfigurationSnapshot(QObject* actualConfig,
    const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run) const
{
    if (QThread::currentThread() != thread() || !run || run->vehicle != this || !actualConfig || run->owner != actualConfig) return {};
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const bool origin = fleetMessageIntervalQueryTransportQuiescent() && _fleetOptionalTelemetryOriginCurrent(run->basis);
    if (!self) return Vehicle::fleetOptionalTelemetryConfigurationRetiredSnapshot(actualConfig, run);
    const bool current = origin && _fleetOptionalTelemetryRunSourceCurrent(run, true);
    QVariantList rows;
    bool issued = false, finalMatching = run->stage == 9;
    for (int index = 0; index < 3; ++index) {
        const auto& row = run->rows.at(index);
        const bool match = row.finalReadback && row.finalReadback->state == QStringLiteral("complete")
            && row.finalReadback->intervalUs == run->intervalUs && _fleetMessageIntervalQuerySourceCurrent(row.finalReadback, true);
        finalMatching = finalMatching && match; issued = issued || row.setRequestIssued;
        auto intermediate = row.readback;
        auto final = row.finalReadback;
        if (run->currentQuery && (run->stage < 6 ? run->stage / 2 : run->stage - 6) == index) {
            if (run->stage < 6) intermediate = run->currentQuery; else final = run->currentQuery;
        }
        rows.append(QVariantMap{{QStringLiteral("messageId"), run->basis->queries.at(index)->messageId},
            {QStringLiteral("originalIntervalUs"), run->basis->queries.at(index)->intervalUs},
            {QStringLiteral("setRequestId"), row.setRequestId}, {QStringLiteral("setRequestIssued"), row.setRequestIssued},
            {QStringLiteral("setAckObserved"), row.setAckObserved},
            {QStringLiteral("setAckResult"), row.setAckObserved ? QVariant(row.setAckResult) : QVariant::fromValue(nullptr)},
            {QStringLiteral("readbackQuery"), intermediate ? _fleetMessageIntervalQueryValue(intermediate) : QVariantMap()},
            {QStringLiteral("finalQuery"), final ? _fleetMessageIntervalQueryValue(final) : QVariantMap()},
            {QStringLiteral("reportedValueMatches"), current && match}});
        if (intermediate) issued = issued || intermediate->requestIssued;
        if (final) issued = issued || final->requestIssued;
    }
    const int currentId = run->stage < 9 ? run->basis->queries.at(run->stage < 6 ? run->stage / 2 : run->stage - 6)->messageId : 0;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-optional-telemetry-configuration-run")},
        {QStringLiteral("runId"), run->id}, {QStringLiteral("basisId"), run->basis->id}, {QStringLiteral("active"), run->active},
        {QStringLiteral("state"), run->state}, {QStringLiteral("reason"), run->reason},
        {QStringLiteral("vehicleId"), run->basis->queries.first()->source.messageCommandVehicleId},
        {QStringLiteral("componentId"), run->basis->queries.first()->source.targetCompId}, {QStringLiteral("requestedIntervalUs"), run->intervalUs},
        {QStringLiteral("currentStageIndex"), run->stage}, {QStringLiteral("currentMessageId"), currentId},
        {QStringLiteral("remainingMs"), QVariant::fromValue(qMax(qint64(0), run->basis->deadlineMs - _mavCommandQuarantineClock.elapsed()))},
        {QStringLiteral("rows"), rows}, {QStringLiteral("requestIssued"), issued}, {QStringLiteral("sourceCurrent"), current},
        {QStringLiteral("originalMutationGeneration"), QString::number(run->basis->queries.first()->mutationGeneration)},
        {QStringLiteral("expectedMutationGeneration"), QString::number(run->expectedMutation)}, {QStringLiteral("ledgerEpoch"), run->ledgerEpoch},
        {QStringLiteral("reportedValuesMatch"), current && finalMatching}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("responseAttributionVerified"), false},
        {QStringLiteral("actualTransmitRateVerified"), false}, {QStringLiteral("availableBandwidthVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
}

QVariantMap Vehicle::fleetOptionalTelemetryConfigurationRetiredSnapshot(QObject* actualConfig,
    const std::shared_ptr<FleetOptionalTelemetryConfigurationRun>& run)
{
    // Only the actual living owner may recover the already-held native facts
    // after Vehicle destruction. No live fallback, OS/fence callback, clock,
    // Link, map import, permission restoration or new command exists here.
    if (!run || run->vehicle || !run->owner || run->owner.data() != actualConfig
        || run->owner->thread() != QThread::currentThread() || !run->basis
        || run->basis->queries.size() != 3 || run->rows.size() != 3) return {};
    QVariantList rows;
    bool issued = false;
    for (int index = 0; index < 3; ++index) {
        const auto& row = run->rows.at(index);
        auto intermediate = row.readback;
        auto final = row.finalReadback;
        if (run->currentQuery && (run->stage < 6 ? run->stage / 2 : run->stage - 6) == index) {
            if (run->stage < 6) intermediate = run->currentQuery; else final = run->currentQuery;
        }
        issued = issued || row.setRequestIssued || (intermediate && intermediate->requestIssued) || (final && final->requestIssued);
        rows.append(QVariantMap{{QStringLiteral("messageId"), run->basis->queries.at(index)->messageId},
            {QStringLiteral("originalIntervalUs"), run->basis->queries.at(index)->intervalUs},
            {QStringLiteral("setRequestId"), row.setRequestId}, {QStringLiteral("setRequestIssued"), row.setRequestIssued},
            {QStringLiteral("setAckObserved"), row.setAckObserved},
            {QStringLiteral("setAckResult"), row.setAckObserved ? QVariant(row.setAckResult) : QVariant::fromValue(nullptr)},
            {QStringLiteral("readbackQuery"), intermediate ? _fleetMessageIntervalQueryRetiredValue(intermediate) : QVariantMap()},
            {QStringLiteral("finalQuery"), final ? _fleetMessageIntervalQueryRetiredValue(final) : QVariantMap()},
            {QStringLiteral("reportedValueMatches"), false}});
    }
    const bool wasTerminal = run->state == QStringLiteral("reported-values-match") || run->state == QStringLiteral("failed")
        || run->state == QStringLiteral("cancelled");
    const int currentId = run->stage >= 0 && run->stage < 9
        ? run->basis->queries.at(run->stage < 6 ? run->stage / 2 : run->stage - 6)->messageId : 0;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-optional-telemetry-configuration-run")},
        {QStringLiteral("runId"), run->id}, {QStringLiteral("basisId"), run->basis->id}, {QStringLiteral("active"), false},
        {QStringLiteral("state"), wasTerminal ? run->state : QStringLiteral("failed")},
        {QStringLiteral("reason"), QStringLiteral("configuration-vehicle-destroyed-historical-snapshot")},
        {QStringLiteral("vehicleId"), run->basis->queries.first()->source.messageCommandVehicleId},
        {QStringLiteral("componentId"), run->basis->queries.first()->source.targetCompId}, {QStringLiteral("requestedIntervalUs"), run->intervalUs},
        {QStringLiteral("currentStageIndex"), run->stage}, {QStringLiteral("currentMessageId"), currentId},
        {QStringLiteral("remainingMs"), QVariant::fromValue(qint64(0))}, {QStringLiteral("rows"), rows},
        {QStringLiteral("requestIssued"), issued}, {QStringLiteral("sourceCurrent"), false},
        {QStringLiteral("originalMutationGeneration"), QString::number(run->basis->queries.first()->mutationGeneration)},
        {QStringLiteral("expectedMutationGeneration"), QString::number(run->expectedMutation)}, {QStringLiteral("ledgerEpoch"), run->ledgerEpoch},
        {QStringLiteral("reportedValuesMatch"), false}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("responseAttributionVerified"), false},
        {QStringLiteral("actualTransmitRateVerified"), false}, {QStringLiteral("availableBandwidthVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
}

void Vehicle::_initializeFleetHealthTelemetryObservation()
{
    _fleetHealthTelemetryClock.start();
    _fleetHealthTelemetryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetHealthTelemetryLinks = _vehicleLinkManager;
    _fleetHealthTelemetryProtocol = _mavlink;
    _fleetHealthTelemetryVehicles = qgcApp() && qgcApp()->toolbox() ? qgcApp()->toolbox()->multiVehicleManager() : nullptr;
    for (const QString& channel : {QStringLiteral("systemStatus"), QStringLiteral("estimatorStatus")})
        _fleetHealthTelemetryRows.insert(channel, FleetHealthTelemetryChannel());
    const auto removed = [this]() {
        if (QThread::currentThread() != thread()) return;
        _fleetHealthTelemetryRemoving = true;
        _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-native-source-removed"));
    };
    if (_mavlink) connect(_mavlink, &QObject::destroyed, this, removed);
    if (_fleetHealthTelemetryLinks) connect(_fleetHealthTelemetryLinks.data(), &QObject::destroyed, this, removed);
    if (_fleetHealthTelemetryVehicles) {
        auto* vehicles = _fleetHealthTelemetryVehicles.data();
        connect(vehicles, &MultiVehicleManager::vehicleRemoved, this, [this, removed](Vehicle* vehicle) {
            if (QThread::currentThread() != thread()) return;
            if (vehicle == this) removed();
        });
        connect(vehicles, &QObject::destroyed, this, removed);
    }
}

void Vehicle::_invalidateFleetHealthTelemetryObservation(const QString& source)
{
    if (QThread::currentThread() != thread()) return;
    // A later healthy packet cannot erase a source break during an actual
    // retained startup interval. Preserve queue facts, only revoke evidence.
    if (!_fleetStartupEstimatorSourceEpoch.isEmpty()) {
        for (auto record = _fleetStartupRetainedReceipts.begin(); record != _fleetStartupRetainedReceipts.end(); ++record) {
            record->insert(QStringLiteral("journalExhausted"), true);
            _fleetStartupMaximumRawGap[record.key()] = -1;
        }
    }
    _fleetHealthTelemetryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    for (const auto& connection : _fleetHealthTelemetryLinkConnections) QObject::disconnect(connection);
    _fleetHealthTelemetryLinkConnections.clear();
    _fleetHealthTelemetryLink.clear(); _fleetHealthTelemetryWeakLink.reset();
    // The original protocol/manager/MVM QPointers stay pinned to this Vehicle
    // instance. A dead native source must not be reconstructed from raw fields
    // or a replacement object returned by the toolbox.
    _fleetHealthTelemetryVersionEpoch.clear(); _fleetHealthTelemetryComponentId = 0;
    _fleetHealthTelemetryFirmwareType = -1; _fleetHealthTelemetryVehicleType = -1;
    for (auto row = _fleetHealthTelemetryRows.begin(); row != _fleetHealthTelemetryRows.end(); ++row) {
        if (_fleetHealthTelemetryRevision == std::numeric_limits<quint64>::max()) _fleetHealthTelemetryExhausted = true;
        else row->contentRevision = ++_fleetHealthTelemetryRevision;
        row->value.clear(); row->source = source; row->receivedAtMs = -1; row->startupReceivedAtMs = -1; row->updateToken = 0;
        row->observed = false; row->complete = false; row->updating = false;
        row->lastEstimatorTimeUsec = 0; row->estimatorTimeObserved = false; row->clockDiscontinuityObserved = false;
    }
    // Counters and receive-stack depth are never reset by a nested source
    // invalidation. Old guards must unwind their own stack, not a new one.
}

bool Vehicle::_fleetHealthTelemetrySourceAllowed(LinkInterface* suppliedLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_fleetHealthTelemetryClock.isValid() || !QElapsedTimer::isMonotonic()
        || _fleetHealthTelemetryRemoving || _fleetHealthTelemetryExhausted || _offlineEditingVehicle
        || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255
        || !qgcApp()->toolbox() || !_fleetHealthTelemetryProtocol || !_fleetHealthTelemetryLinks || !_fleetHealthTelemetryVehicles
        || _fleetHealthTelemetryProtocol.data() != _mavlink || _fleetHealthTelemetryLinks.data() != _vehicleLinkManager) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<MAVLinkProtocol> protocol(_fleetHealthTelemetryProtocol);
    const QPointer<VehicleLinkManager> links(_fleetHealthTelemetryLinks);
    const QPointer<MultiVehicleManager> vehicles(_fleetHealthTelemetryVehicles);
    if (!protocol || !links || !vehicles || protocol->thread() != thread() || links->thread() != thread()
        || vehicles->thread() != thread() || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
        || vehicles->getVehicleById(_id) != this || links->communicationLost()) return false;
    const WeakLinkInterfacePtr primary = links->primaryLink();
    if (!suppliedLink || primary.expired()) return false;
    // Inspect the actual manager's owning entries by reference. Do not lock a
    // temporary strong pointer whose destructor could run callbacks on return.
    LinkInterface* actualLink = nullptr;
    for (const auto& entry : links->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == suppliedLink && !primary.owner_before(candidate) && !candidate.owner_before(primary)) {
            actualLink = entry.link.get(); break;
        }
    }
    if (!actualLink) return false;
    const QPointer<LinkInterface> link(actualLink);
    const QString epoch = _fleetHealthTelemetryEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const quint64 receipts = _fleetHealthTelemetryNextReceipt, revision = _fleetHealthTelemetryRevision;
    const int vehicleId = _id, componentId = _defaultComponentId;
    const MAV_AUTOPILOT firmware = _firmwareType;
    const MAV_TYPE model = _vehicleType;
    const auto same = [&]() {
        if (!self || !link || !protocol || !links || !vehicles || !qgcApp() || !qgcApp()->toolbox()
            || QThread::currentThread() != self->thread() || self->thread() != qgcApp()->thread()
            || protocol->thread() != self->thread() || links->thread() != self->thread() || vehicles->thread() != self->thread()
            || self->_mavlink != protocol.data() || self->_vehicleLinkManager != links.data()
            || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data() || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
            || vehicles->getVehicleById(vehicleId) != self.data() || links->communicationLost()
            || self->_fleetHealthTelemetryRemoving || self->_fleetHealthTelemetryExhausted || self->_offlineEditingVehicle
            || self->_autopilotVersionObservationRemoving || self->_autopilotVersionObservationExhausted
            || self->_id != vehicleId || self->_defaultComponentId != componentId
            || self->_firmwareType != firmware || self->_vehicleType != model
            || self->_fleetHealthTelemetryEpoch != epoch || self->_autopilotVersionObservationEpoch != versionEpoch
            || self->_fleetHealthTelemetryNextReceipt != receipts || self->_fleetHealthTelemetryRevision != revision) return false;
        const WeakLinkInterfacePtr now = links->primaryLink();
        if (now.expired() || primary.owner_before(now) || now.owner_before(primary)) return false;
        for (const auto& entry : links->_rgLinkInfo) {
            const WeakLinkInterfacePtr candidate(entry.link);
            if (entry.link && entry.link.get() == link.data() && !now.owner_before(candidate) && !candidate.owner_before(now)) return true;
        }
        return false;
    };
    if (!same()) return false;
    const bool connected = link->isConnected();
    if (!same() || !connected) return false;
    const bool mock = link->isMockLink();
    if (!same() || mock) return false;
    const bool replay = link->isLogReplay();
    return same() && !replay;
}

bool Vehicle::_fleetHealthTelemetrySourceMatches() const
{
    if (QThread::currentThread() != thread() || !_fleetHealthTelemetryLink || !_fleetHealthTelemetryLinks
        || !_fleetHealthTelemetryProtocol || !_fleetHealthTelemetryVehicles || _fleetHealthTelemetryVersionEpoch.isEmpty()
        || _fleetHealthTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetHealthTelemetryLinks.data() != _vehicleLinkManager || _fleetHealthTelemetryProtocol.data() != _mavlink
        || _fleetHealthTelemetryComponentId != _defaultComponentId || _fleetHealthTelemetryFirmwareType != int(_firmwareType)
        || _fleetHealthTelemetryVehicleType != int(_vehicleType) || _fleetHealthTelemetryWeakLink.expired()) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(_fleetHealthTelemetryLink);
    const QPointer<VehicleLinkManager> links(_fleetHealthTelemetryLinks);
    const QPointer<MAVLinkProtocol> protocol(_fleetHealthTelemetryProtocol);
    const QPointer<MultiVehicleManager> vehicles(_fleetHealthTelemetryVehicles);
    const WeakLinkInterfacePtr primary(_fleetHealthTelemetryWeakLink);
    const QString epoch = _fleetHealthTelemetryEpoch, versionEpoch = _fleetHealthTelemetryVersionEpoch;
    const bool allowed = _fleetHealthTelemetrySourceAllowed(link.data());
    if (!self || !link || !links || !protocol || !vehicles || !allowed || !qgcApp() || !qgcApp()->toolbox()) return false;
    const WeakLinkInterfacePtr current = links->primaryLink();
    return self->_fleetHealthTelemetryEpoch == epoch && self->_fleetHealthTelemetryVersionEpoch == versionEpoch
        && self->_autopilotVersionObservationEpoch == versionEpoch && self->_fleetHealthTelemetryLink == link
        && self->_fleetHealthTelemetryLinks == links && self->_fleetHealthTelemetryProtocol == protocol
        && self->_fleetHealthTelemetryVehicles == vehicles && qgcApp()->toolbox()->multiVehicleManager() == vehicles.data()
        && !primary.expired() && !current.expired() && !primary.owner_before(current) && !current.owner_before(primary);
}

quint64 Vehicle::_prepareFleetHealthTelemetryObservation(const mavlink_message_t& original, LinkInterface* sourceLink,
    QString* channel, QVariantMap* value)
{
    channel->clear(); value->clear();
    if (original.msgid == MAVLINK_MSG_ID_SYS_STATUS) *channel = QStringLiteral("systemStatus");
    else if (original.msgid == MAVLINK_MSG_ID_ESTIMATOR_STATUS) *channel = QStringLiteral("estimatorStatus");
    else return 0;
    if (QThread::currentThread() != thread() || !_mavlink || sender() != _mavlink
        || original.sysid != _id || original.compid != _defaultComponentId) return 0;
    // Other configured links can carry the same system/component. Ignore
    // those packets instead of letting a secondary stream replace or erase
    // this ledger's PRIMARY observation. A changed actual primary is still
    // rejected by the source matcher and the native source-change hooks.
    const QPointer<VehicleLinkManager> actualLinks(_fleetHealthTelemetryLinks);
    if (!actualLinks || actualLinks->thread() != thread()) return 0;
    const WeakLinkInterfacePtr actualPrimary = actualLinks->primaryLink();
    bool primaryFrame = false;
    for (const auto& entry : actualLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !actualPrimary.expired()
            && !actualPrimary.owner_before(candidate) && !candidate.owner_before(actualPrimary)) { primaryFrame = true; break; }
    }
    if (!primaryFrame) return 0;
    const QPointer<Vehicle> self(this);
    if (_fleetHealthTelemetryReceiveDepth != 1) {
        _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-reentrant-raw-receive")); return 0;
    }
    const bool allowed = _fleetHealthTelemetrySourceAllowed(sourceLink);
    if (!self) return 0;
    if (!allowed) { _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-unavailable-native-source")); return 0; }
    const int maximum = original.msgid == MAVLINK_MSG_ID_SYS_STATUS ? MAVLINK_MSG_ID_SYS_STATUS_LEN : MAVLINK_MSG_ID_ESTIMATOR_STATUS_LEN;
    const int minimum = original.msgid == MAVLINK_MSG_ID_SYS_STATUS ? MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN : MAVLINK_MSG_ID_ESTIMATOR_STATUS_MIN_LEN;
    if ((original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1) || original.len < 1 || original.len > maximum
        || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != minimum)) {
        _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-invalid-raw-frame")); return 0;
    }
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    quint64 estimatorTime = 0;
    if (original.msgid == MAVLINK_MSG_ID_SYS_STATUS) {
        mavlink_sys_status_t status{}; mavlink_msg_sys_status_decode(&normalized, &status);
        const uint present = status.onboard_control_sensors_present, enabled = status.onboard_control_sensors_enabled;
        const uint health = status.onboard_control_sensors_health;
        *value = {{QStringLiteral("sensorsPresentBits"), present}, {QStringLiteral("sensorsEnabledBits"), enabled},
            {QStringLiteral("sensorsHealthBits"), health}, {QStringLiteral("sensorsUnhealthyBits"), uint(enabled & ~health)}};
    } else {
        mavlink_estimator_status_t status{}; mavlink_msg_estimator_status_decode(&normalized, &status);
        const uint flags = status.flags;
        const uint known = ESTIMATOR_ATTITUDE | ESTIMATOR_VELOCITY_HORIZ | ESTIMATOR_VELOCITY_VERT | ESTIMATOR_POS_HORIZ_REL
            | ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS | ESTIMATOR_POS_VERT_AGL | ESTIMATOR_CONST_POS_MODE
            | ESTIMATOR_PRED_POS_HORIZ_REL | ESTIMATOR_PRED_POS_HORIZ_ABS | ESTIMATOR_GPS_GLITCH | ESTIMATOR_ACCEL_ERROR;
        if ((flags & ~known) != 0) { _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-unknown-estimator-flags")); return 0; }
        estimatorTime = status.time_usec;
        *value = {{QStringLiteral("flags"), flags}, {QStringLiteral("goodAttitudeEstimate"), bool(flags & ESTIMATOR_ATTITUDE)},
            {QStringLiteral("goodHorizVelEstimate"), bool(flags & ESTIMATOR_VELOCITY_HORIZ)},
            {QStringLiteral("goodVertVelEstimate"), bool(flags & ESTIMATOR_VELOCITY_VERT)},
            {QStringLiteral("goodHorizPosRelEstimate"), bool(flags & ESTIMATOR_POS_HORIZ_REL)},
            {QStringLiteral("gpsGlitch"), bool(flags & ESTIMATOR_GPS_GLITCH)}, {QStringLiteral("accelError"), bool(flags & ESTIMATOR_ACCEL_ERROR)},
            {QStringLiteral("timeUsec"), QString::number(estimatorTime)}};
    }
    const WeakLinkInterfacePtr primary = _vehicleLinkManager->primaryLink();
    if (_fleetHealthTelemetryLink.data() != sourceLink || _fleetHealthTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetHealthTelemetryLinks.data() != _vehicleLinkManager || _fleetHealthTelemetryProtocol.data() != _mavlink
        || _fleetHealthTelemetryWeakLink.owner_before(primary) || primary.owner_before(_fleetHealthTelemetryWeakLink)) {
        _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-source-context-change"));
        if (_fleetHealthTelemetryExhausted) { value->clear(); return 0; }
        _fleetHealthTelemetryLink = sourceLink; _fleetHealthTelemetryWeakLink = primary;
        _fleetHealthTelemetryVersionEpoch = _autopilotVersionObservationEpoch;
        _fleetHealthTelemetryComponentId = _defaultComponentId;
        _fleetHealthTelemetryFirmwareType = int(_firmwareType); _fleetHealthTelemetryVehicleType = int(_vehicleType);
        const QString epoch = _fleetHealthTelemetryEpoch;
        const auto lost = [this, epoch]() {
            if (QThread::currentThread() != thread()) return;
            if (_fleetHealthTelemetryEpoch == epoch) _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-primary-link-retired"));
        };
        const auto disconnected = connect(sourceLink, &LinkInterface::disconnected, this, lost);
        if (!self) return 0;
        if (_fleetHealthTelemetryEpoch != epoch) { QObject::disconnect(disconnected); value->clear(); return 0; }
        _fleetHealthTelemetryLinkConnections.append(disconnected);
        const auto destroyed = connect(sourceLink, &QObject::destroyed, this, lost);
        if (!self) return 0;
        if (_fleetHealthTelemetryEpoch != epoch) { QObject::disconnect(destroyed); value->clear(); return 0; }
        _fleetHealthTelemetryLinkConnections.append(destroyed);
    }
    const bool stillCurrent = _fleetHealthTelemetrySourceMatches();
    if (!self) return 0;
    if (!stillCurrent) { _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-source-changed-before-receipt")); value->clear(); return 0; }
    auto row = _fleetHealthTelemetryRows.find(*channel);
    if (row == _fleetHealthTelemetryRows.end() || _fleetHealthTelemetryNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetHealthTelemetryExhausted = true; _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-receipt-exhausted")); value->clear(); return 0;
    }
    QVariantMap semantic = *value, previous = row->value;
    semantic.remove(QStringLiteral("timeUsec")); previous.remove(QStringLiteral("timeUsec"));
    const bool clockRegressed = *channel == QStringLiteral("estimatorStatus") && row->estimatorTimeObserved && estimatorTime < row->lastEstimatorTimeUsec;
    if (!row->observed || semantic != previous || clockRegressed) {
        if (_fleetHealthTelemetryRevision == std::numeric_limits<quint64>::max()) {
            _fleetHealthTelemetryExhausted = true; _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-content-exhausted")); value->clear(); return 0;
        }
        row->contentRevision = ++_fleetHealthTelemetryRevision;
    }
    if (*channel == QStringLiteral("estimatorStatus")) {
        row->lastEstimatorTimeUsec = estimatorTime; row->estimatorTimeObserved = true;
        row->clockDiscontinuityObserved = row->clockDiscontinuityObserved || clockRegressed;
    }
    row->updateToken = ++_fleetHealthTelemetryNextReceipt;
    row->updating = true; row->complete = false;
    return row->updateToken;
}

void Vehicle::_completeFleetHealthTelemetryObservation(const QString& channel, quint64 token, const QString& epoch,
    const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs, qint64 startupReceivedAtMs)
{
    if (!token || QThread::currentThread() != thread() || epoch != _fleetHealthTelemetryEpoch
        || _fleetHealthTelemetryReceiveDepth != 1 || receivedAtMs < 0 || sourceLink != _fleetHealthTelemetryLink.data()) return;
    auto row = _fleetHealthTelemetryRows.constFind(channel);
    if (row == _fleetHealthTelemetryRows.constEnd() || row->updateToken != token || !row->updating) return;
    const QPointer<Vehicle> self(this);
    const bool sourceCurrent = _fleetHealthTelemetrySourceMatches();
    if (!self || !sourceCurrent || epoch != _fleetHealthTelemetryEpoch || _fleetHealthTelemetryReceiveDepth != 1) return;
    auto current = _fleetHealthTelemetryRows.find(channel);
    if (current == _fleetHealthTelemetryRows.end() || current->updateToken != token || !current->updating
        || value.size() != (channel == QStringLiteral("systemStatus") ? 4 : 8)) return;
    current->value = value; current->receiptSequence = token; current->receivedAtMs = receivedAtMs;
    // Independently sampled at the same native entry. Never derive one
    // elapsed-clock coordinate by subtracting another clock's reported age.
    current->startupReceivedAtMs = startupReceivedAtMs;
    current->source = channel == QStringLiteral("systemStatus") ? QStringLiteral("live-sys-status") : QStringLiteral("live-estimator-status");
    current->observed = true; current->complete = true; current->updating = false;
}

QVariantMap Vehicle::_fleetStartupEstimatorProjection(bool requireHealthy) const
{
    if (QThread::currentThread() != thread() || _fleetHealthTelemetryExhausted || _fleetHealthTelemetryRemoving
        || _fleetHealthTelemetryEpoch.isEmpty()) return {};
    const FleetHealthTelemetryChannel row = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus"));
    if (!row.observed || !row.complete || row.updating || !row.receiptSequence || !row.contentRevision
        || row.source != QStringLiteral("live-estimator-status") || row.value.size() != 8
        || row.value.value(QStringLiteral("flags")).userType() != QMetaType::UInt) return {};
    const uint flags = row.value.value(QStringLiteral("flags")).toUInt();
    const uint known = ESTIMATOR_ATTITUDE | ESTIMATOR_VELOCITY_HORIZ | ESTIMATOR_VELOCITY_VERT | ESTIMATOR_POS_HORIZ_REL
        | ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS | ESTIMATOR_POS_VERT_AGL | ESTIMATOR_CONST_POS_MODE
        | ESTIMATOR_PRED_POS_HORIZ_REL | ESTIMATOR_PRED_POS_HORIZ_ABS | ESTIMATOR_GPS_GLITCH | ESTIMATOR_ACCEL_ERROR;
    if (flags & ~known) return {};
    const uint requiredAbsolutePosition = ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS;
    // Match the global WGS84/MSL mission-start healthy predicate at the final
    // native queue gate. Diagnostic/journal projections must retain legal
    // relative-only and constant-position observations when health is not
    // required; a later good packet must not erase those negative RAW leaves.
    if (requireHealthy && ((flags & requiredAbsolutePosition) != requiredAbsolutePosition
            || (flags & ESTIMATOR_CONST_POS_MODE) != 0)) return {};
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("sourceEpoch"), _fleetHealthTelemetryEpoch},
        {QStringLiteral("contentRevision"), QString::number(row.contentRevision)}, {QStringLiteral("flags"), flags},
        {QStringLiteral("clockDiscontinuityObserved"), row.clockDiscontinuityObserved}};
    const QStringList names{QStringLiteral("goodAttitudeEstimate"), QStringLiteral("goodHorizVelEstimate"),
        QStringLiteral("goodVertVelEstimate"), QStringLiteral("goodHorizPosRelEstimate"), QStringLiteral("gpsGlitch"), QStringLiteral("accelError")};
    const uint bits[]{ESTIMATOR_ATTITUDE, ESTIMATOR_VELOCITY_HORIZ, ESTIMATOR_VELOCITY_VERT,
        ESTIMATOR_POS_HORIZ_REL, ESTIMATOR_GPS_GLITCH, ESTIMATOR_ACCEL_ERROR};
    for (int index = 0; index < names.size(); ++index) {
        const QVariant value = row.value.value(names.at(index));
        if (value.userType() != QMetaType::Bool || value.isNull() || value.toBool() != bool(flags & bits[index])
            || (requireHealthy && value.toBool() != (index < 4))) return {};
        result.insert(names.at(index), value);
    }
    if (requireHealthy && row.clockDiscontinuityObserved) return {};
    return result; // Exact 11; no clock sample, age or per-packet receipt.
}

bool Vehicle::_fleetStartupEstimatorMemoryCurrent(bool requireHealthy) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread() || !qgcApp()->toolbox()
        || !_fleetHealthTelemetryClock.isValid() || !_autopilotVersionObservationClock.isValid() || !QElapsedTimer::isMonotonic()
        || _fleetHealthTelemetryExhausted || _fleetHealthTelemetryRemoving || _fleetStartupEstimatorSourceEpoch.isEmpty()
        || _fleetStartupEstimatorSourceEpoch != _fleetHealthTelemetryEpoch || _fleetHealthTelemetryLink != _fleetStartupLink
        || !_fleetHealthTelemetryLink || !_fleetHealthTelemetryProtocol || !_fleetHealthTelemetryLinks || !_fleetHealthTelemetryVehicles
        || _fleetHealthTelemetryProtocol.data() != _mavlink || _fleetHealthTelemetryLinks.data() != _vehicleLinkManager
        || qgcApp()->toolbox()->mavlinkProtocol() != _fleetHealthTelemetryProtocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != _fleetHealthTelemetryVehicles.data()
        || _fleetHealthTelemetryProtocol->thread() != thread() || _fleetHealthTelemetryLinks->thread() != thread()
        || _fleetHealthTelemetryVehicles->thread() != thread() || _fleetHealthTelemetryVehicles->getVehicleById(_id) != this
        || _fleetHealthTelemetryLinks->communicationLost() || _fleetHealthTelemetryVersionEpoch != _fleetStartupVersionEpoch
        || _fleetHealthTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetHealthTelemetryComponentId != _defaultComponentId || _fleetHealthTelemetryFirmwareType != int(_firmwareType)
        || _fleetHealthTelemetryVehicleType != int(_vehicleType) || _fleetStartupEstimatorProjection(requireHealthy).size() != 11) return false;
    const WeakLinkInterfacePtr current = _fleetHealthTelemetryLinks->primaryLink();
    if (current.expired() || _fleetHealthTelemetryWeakLink.expired() || current.owner_before(_fleetHealthTelemetryWeakLink)
        || _fleetHealthTelemetryWeakLink.owner_before(current)) return false;
    bool member = false;
    for (const auto& entry : _fleetHealthTelemetryLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == _fleetStartupLink.data() && !current.owner_before(candidate) && !candidate.owner_before(current)) {
            member = true; break;
        }
    }
    const FleetHealthTelemetryChannel row = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus"));
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    if (requireHealthy && !_fleetStartupJournalStepId.isEmpty()) {
        const auto record = _fleetStartupRetainedReceipts.constFind(_fleetStartupJournalStepId);
        if (record != _fleetStartupRetainedReceipts.constEnd() && record->value(QStringLiteral("requestIssued")).toBool()) {
            const auto maximumGap = _fleetStartupMaximumRawGap.constFind(_fleetStartupJournalStepId);
            if (record->value(QStringLiteral("journalExhausted")).toBool() || maximumGap == _fleetStartupMaximumRawGap.constEnd()
                || *maximumGap < 0 || *maximumGap > _fleetStartupEstimatorMaximumGapMs) return false;
        }
    }
    return member && row.startupReceivedAtMs >= 0 && now >= row.startupReceivedAtMs
        && (!requireHealthy || (_fleetStartupEstimatorMaximumGapMs >= 100 && _fleetStartupEstimatorMaximumGapMs <= 3000
            && now - row.startupReceivedAtMs <= _fleetStartupEstimatorMaximumGapMs));
}

QVariantMap Vehicle::fleetHealthTelemetryObservation(qint64 maximumAgeMs) const
{
    QVariantMap channels;
    for (const QString& channel : {QStringLiteral("systemStatus"), QStringLiteral("estimatorStatus")})
        channels.insert(channel, QVariantMap{{QStringLiteral("observed"), false}, {QStringLiteral("available"), false},
            {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), QVariant::fromValue(qint64(-1))},
            {QStringLiteral("receiptSequence"), QStringLiteral("0")}, {QStringLiteral("contentRevision"), QStringLiteral("0")},
            {QStringLiteral("value"), QVariantMap()}, {QStringLiteral("source"), QStringLiteral("unknown")},
            {QStringLiteral("clockDiscontinuityObserved"), false}});
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("sourceEpoch"), QString()},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("componentId"), 0}, {QStringLiteral("sourceCurrent"), false},
        {QStringLiteral("channels"), channels}, {QStringLiteral("readonly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || maximumAgeMs < 1 || maximumAgeMs > 120000) return result;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QString epoch = _fleetHealthTelemetryEpoch;
    const quint64 receipts = _fleetHealthTelemetryNextReceipt, revision = _fleetHealthTelemetryRevision;
    result[QStringLiteral("vehicleId")] = _id; result[QStringLiteral("componentId")] = _defaultComponentId;
    result[QStringLiteral("sourceEpoch")] = epoch;
    // Unknown/current-source rejection does not turn monotonic semantic
    // history back into zero. No old value or availability is restored here.
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        const FleetHealthTelemetryChannel row = _fleetHealthTelemetryRows.value(it.key());
        QVariantMap unknown = it.value().toMap();
        unknown[QStringLiteral("receiptSequence")] = QString::number(row.receiptSequence);
        unknown[QStringLiteral("contentRevision")] = QString::number(row.contentRevision);
        unknown[QStringLiteral("source")] = row.source;
        unknown[QStringLiteral("clockDiscontinuityObserved")] = row.clockDiscontinuityObserved;
        it.value() = unknown;
    }
    result[QStringLiteral("channels")] = channels;
    if (_fleetHealthTelemetryReceiveDepth != 0 || !_fleetHealthTelemetryClock.isValid()) return result;
    const bool sourceCurrent = _fleetHealthTelemetrySourceMatches();
    if (!self || !sourceCurrent || self->_fleetHealthTelemetryEpoch != epoch
        || self->_fleetHealthTelemetryNextReceipt != receipts || self->_fleetHealthTelemetryRevision != revision
        || self->_fleetHealthTelemetryReceiveDepth != 0) return result;
    const qint64 now = self->_fleetHealthTelemetryClock.elapsed();
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        const FleetHealthTelemetryChannel row = self->_fleetHealthTelemetryRows.value(it.key());
        const qint64 age = row.observed && row.receivedAtMs >= 0 && now >= row.receivedAtMs ? now - row.receivedAtMs : -1;
        const bool available = row.observed && row.complete && !row.updating && row.receiptSequence != 0
            && !row.clockDiscontinuityObserved && !row.value.isEmpty() && age >= 0;
        it.value() = QVariantMap{{QStringLiteral("observed"), row.observed}, {QStringLiteral("available"), available},
            {QStringLiteral("fresh"), available && age <= maximumAgeMs}, {QStringLiteral("ageMs"), QVariant::fromValue(age)},
            {QStringLiteral("receiptSequence"), QString::number(row.receiptSequence)}, {QStringLiteral("contentRevision"), QString::number(row.contentRevision)},
            {QStringLiteral("value"), row.value}, {QStringLiteral("source"), row.source},
            {QStringLiteral("clockDiscontinuityObserved"), row.clockDiscontinuityObserved}};
    }
    result[QStringLiteral("sourceCurrent")] = true; result[QStringLiteral("channels")] = channels;
    return result;
}

Vehicle::FleetBatteryDrainIngress Vehicle::_prepareFleetBatteryDrainIngress(const mavlink_message_t& original, LinkInterface* sourceLink) const
{
    FleetBatteryDrainIngress ingress;
    if (QThread::currentThread() != thread() || original.msgid != MAVLINK_MSG_ID_BATTERY_STATUS
        || !_fleetBatteryTelemetryClock.isValid() || !QElapsedTimer::isMonotonic()
        || !_fleetBatteryTelemetryProtocol || sender() != _fleetBatteryTelemetryProtocol.data()
        || !_fleetBatteryTelemetryLinks || _fleetBatteryTelemetryLinks->thread() != thread()
        || original.sysid != _id || original.compid != _defaultComponentId
        || _fleetMessageIntervalRawReceiveDepth != 1 || _fleetMessageIntervalRawReceiveExhausted) return ingress;
    ingress.receivedAtMs = _fleetBatteryTelemetryClock.elapsed();
    const WeakLinkInterfacePtr primary = _fleetBatteryTelemetryLinks->primaryLink();
    if (primary.expired()) return ingress;
    for (const auto& entry : _fleetBatteryTelemetryLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !primary.owner_before(candidate) && !candidate.owner_before(primary)) {
            ingress.link = entry.link.get(); ingress.primary = primary;
            ingress.versionEpoch = _autopilotVersionObservationEpoch;
            ingress.baselineReceipt = _fleetBatteryTelemetryNextReceipt;
            ingress.vehicleId = _id; ingress.componentId = _defaultComponentId;
            ingress.firmwareType = int(_firmwareType); ingress.vehicleType = int(_vehicleType);
            ingress.eligible = ingress.receivedAtMs >= 0;
            break;
        }
    }
    return ingress;
}

void Vehicle::_advanceFleetBatteryDrainRevision()
{
    if (_fleetBatteryDrainRevision == std::numeric_limits<quint64>::max()) _fleetBatteryDrainExhausted = true;
    else ++_fleetBatteryDrainRevision;
}

void Vehicle::_resetFleetBatteryDrain(FleetBatteryTelemetryRow& row)
{
    if (row.drainWindow.published) _advanceFleetBatteryDrainRevision();
    row.drainWindow = FleetBatteryDrainWindow(); row.drainSamples.clear();
    row.drainReceivedAtMs = -1; row.drainPendingMaximumGapMs = 0;
    row.drainLastPercent = -1; row.drainFunction = -1; row.drainType = -1;
}

void Vehicle::_observeFleetBatteryDrain(FleetBatteryTelemetryRow& row, quint64 receipt, const FleetBatteryDrainIngress& ingress)
{
    // Called only after the old complete transaction has reacquired this row.
    // Nothing below emits signals, calls provider virtuals, or performs
    // device/file/network operations; local monotonic clock reads are allowed.
    const bool same = ingress.eligible && ingress.link && ingress.link == _fleetBatteryTelemetryLink
        && ingress.vehicleId == _id && ingress.componentId == _defaultComponentId
        && ingress.firmwareType == int(_firmwareType) && ingress.vehicleType == int(_vehicleType)
        && ingress.versionEpoch == _autopilotVersionObservationEpoch
        && ingress.versionEpoch == _fleetBatteryTelemetryVersionEpoch && !ingress.primary.expired()
        && !ingress.primary.owner_before(_fleetBatteryTelemetryWeakLink) && !_fleetBatteryTelemetryWeakLink.owner_before(ingress.primary)
        && ingress.baselineReceipt != std::numeric_limits<quint64>::max() && receipt == ingress.baselineReceipt + 1;
    const qint64 now = _fleetBatteryTelemetryClock.isValid() ? _fleetBatteryTelemetryClock.elapsed() : -1;
    if (!same || ingress.receivedAtMs < 0 || now < ingress.receivedAtMs) { _resetFleetBatteryDrain(row); return; }
    const int function = row.value.value(QStringLiteral("batteryFunction")).toInt();
    const int type = row.value.value(QStringLiteral("batteryType")).toInt();
    const bool known = row.value.value(QStringLiteral("batteryPercentKnown")).toBool();
    const int percent = known ? int(row.value.value(QStringLiteral("batteryPercent")).toDouble()) : -1;
    const qint64 rawAt = ingress.receivedAtMs;
    const qint64 gap = row.drainReceivedAtMs >= 0 && rawAt >= row.drainReceivedAtMs ? rawAt - row.drainReceivedAtMs : -1;
    const bool broken = !known || percent < 0 || percent > 100 || row.drainReceivedAtMs < 0 || gap < 0 || gap > 3000
        || row.drainFunction != function || row.drainType != type || row.drainLastPercent < 0 || percent > row.drainLastPercent;
    const bool declined = !broken && percent < row.drainLastPercent;
    if (broken) _resetFleetBatteryDrain(row);
    row.drainReceivedAtMs = rawAt; row.drainFunction = function; row.drainType = type; row.drainLastPercent = percent;
    if (!known || percent < 0 || percent > 100 || _fleetBatteryDrainExhausted || row.drainSampleOverflow) return;
    row.drainPendingMaximumGapMs = qMax(row.drainPendingMaximumGapMs, broken ? qint64(0) : gap);
    const qint64 earliest = rawAt > 120000 ? rawAt - 120000 : 0;
    while (!row.drainSamples.isEmpty() && row.drainSamples.first().receivedAtMs < earliest) row.drainSamples.removeFirst();
    const bool retain = row.drainSamples.isEmpty() || declined
        || rawAt - row.drainSamples.last().receivedAtMs >= 1000;
    if (!retain) return; // Flat reports never move a published endpoint.
    if (row.drainSamples.size() >= 256) {
        row.drainSampleOverflow = true; _resetFleetBatteryDrain(row);
        row.drainReceivedAtMs = rawAt; row.drainFunction = function; row.drainType = type; row.drainLastPercent = percent;
        return;
    }
    FleetBatteryDrainSample sample;
    sample.receivedAtMs = rawAt; sample.receipt = receipt; sample.percent = percent;
    sample.maximumGapMs = row.drainPendingMaximumGapMs; row.drainPendingMaximumGapMs = 0;
    row.drainSamples.append(sample);
    if (!declined || row.drainSamples.size() < 2) return;
    const FleetBatteryDrainSample& first = row.drainSamples.first();
    const qint64 duration = rawAt - first.receivedAtMs;
    if (duration < 30000 || duration > 120000 || first.percent - percent < 2) return;
    qint64 maximumGap = 0;
    for (int index = 1; index < row.drainSamples.size(); ++index) maximumGap = qMax(maximumGap, row.drainSamples.at(index).maximumGapMs);
    if (maximumGap > 3000) return;
    _advanceFleetBatteryDrainRevision();
    if (_fleetBatteryDrainExhausted) { row.drainWindow = FleetBatteryDrainWindow(); return; }
    FleetBatteryDrainWindow window;
    window.startReceipt = first.receipt; window.endReceipt = receipt;
    window.startAtMs = first.receivedAtMs; window.endAtMs = rawAt;
    window.startPercent = first.percent; window.endPercent = percent;
    window.maximumGapMs = maximumGap; window.sampleCount = row.drainSamples.size(); window.published = true;
    row.drainWindow = window;
}

void Vehicle::_initializeFleetBatteryTelemetryObservation()
{
    _fleetBatteryTelemetryClock.start();
    _fleetBatteryTelemetryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetBatteryTelemetryLinks = _vehicleLinkManager;
    _fleetBatteryTelemetryProtocol = _mavlink;
    _fleetBatteryTelemetryVehicles = qgcApp() && qgcApp()->toolbox() ? qgcApp()->toolbox()->multiVehicleManager() : nullptr;
    const auto removed = [this]() {
        if (QThread::currentThread() != thread()) return;
        _fleetBatteryTelemetryRemoving = true;
        _invalidateFleetBatteryTelemetryObservation();
    };
    if (_mavlink) connect(_mavlink, &QObject::destroyed, this, removed);
    if (_fleetBatteryTelemetryLinks) connect(_fleetBatteryTelemetryLinks.data(), &QObject::destroyed, this, removed);
    if (_fleetBatteryTelemetryVehicles) {
        auto* vehicles = _fleetBatteryTelemetryVehicles.data();
        connect(vehicles, &MultiVehicleManager::vehicleRemoved, this, [this, removed](Vehicle* vehicle) {
            if (QThread::currentThread() == thread() && vehicle == this) removed();
        });
        connect(vehicles, &QObject::destroyed, this, removed);
    }
}

void Vehicle::_invalidateFleetBatteryTelemetryObservation()
{
    if (QThread::currentThread() != thread()) return;
    // Retire the independent model before disconnectNotify can re-enter.
    bool publishedDrain = false;
    for (auto it = _fleetBatteryTelemetryRows.begin(); it != _fleetBatteryTelemetryRows.end(); ++it) {
        publishedDrain = publishedDrain || it->drainWindow.published;
        it->drainWindow = FleetBatteryDrainWindow(); it->drainSamples.clear(); it->drainReceivedAtMs = -1;
    }
    if (publishedDrain) _advanceFleetBatteryDrainRevision();
    _fleetBatteryTelemetryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    for (const auto& connection : _fleetBatteryTelemetryLinkConnections) QObject::disconnect(connection);
    _fleetBatteryTelemetryLinkConnections.clear();
    _fleetBatteryTelemetryLink.clear(); _fleetBatteryTelemetryWeakLink.reset();
    _fleetBatteryTelemetryVersionEpoch.clear(); _fleetBatteryTelemetryComponentId = 0;
    _fleetBatteryTelemetryFirmwareType = -1; _fleetBatteryTelemetryVehicleType = -1;
    _fleetBatteryTelemetryRows.clear(); _fleetBatteryTelemetrySlotOverflow = false;
    if (_fleetBatteryTelemetryRevision == std::numeric_limits<quint64>::max()) _fleetBatteryTelemetryExhausted = true;
    else ++_fleetBatteryTelemetryRevision;
    // Keep original protocol/manager/MVM owners, counters, and receive depth.
    // A nested invalidation must not release another receive stack's fence.
}

bool Vehicle::_fleetBatteryTelemetrySourceAllowed(LinkInterface* suppliedLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_fleetBatteryTelemetryClock.isValid() || !QElapsedTimer::isMonotonic()
        || _fleetBatteryTelemetryRemoving || _fleetBatteryTelemetryExhausted || _offlineEditingVehicle
        || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255
        || !qgcApp()->toolbox() || !_fleetBatteryTelemetryProtocol || !_fleetBatteryTelemetryLinks || !_fleetBatteryTelemetryVehicles
        || _fleetBatteryTelemetryProtocol.data() != _mavlink || _fleetBatteryTelemetryLinks.data() != _vehicleLinkManager) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<MAVLinkProtocol> protocol(_fleetBatteryTelemetryProtocol);
    const QPointer<VehicleLinkManager> links(_fleetBatteryTelemetryLinks);
    const QPointer<MultiVehicleManager> vehicles(_fleetBatteryTelemetryVehicles);
    if (!protocol || !links || !vehicles || protocol->thread() != thread() || links->thread() != thread()
        || vehicles->thread() != thread() || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
        || vehicles->getVehicleById(_id) != this || links->communicationLost()) return false;
    const WeakLinkInterfacePtr primary = links->primaryLink();
    if (!suppliedLink || primary.expired()) return false;
    LinkInterface* actualLink = nullptr;
    for (const auto& entry : links->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == suppliedLink && !primary.owner_before(candidate) && !candidate.owner_before(primary)) {
            actualLink = entry.link.get(); break;
        }
    }
    if (!actualLink) return false;
    const QPointer<LinkInterface> link(actualLink);
    const QString epoch = _fleetBatteryTelemetryEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const quint64 receipts = _fleetBatteryTelemetryNextReceipt, revision = _fleetBatteryTelemetryRevision;
    const int vehicleId = _id, componentId = _defaultComponentId;
    const MAV_AUTOPILOT firmware = _firmwareType;
    const MAV_TYPE model = _vehicleType;
    const auto same = [&]() {
        if (!self || !link || !protocol || !links || !vehicles || !qgcApp() || !qgcApp()->toolbox()
            || QThread::currentThread() != self->thread() || self->thread() != qgcApp()->thread()
            || protocol->thread() != self->thread() || links->thread() != self->thread() || vehicles->thread() != self->thread()
            || self->_mavlink != protocol.data() || self->_vehicleLinkManager != links.data()
            || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data() || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
            || vehicles->getVehicleById(vehicleId) != self.data() || links->communicationLost()
            || self->_fleetBatteryTelemetryRemoving || self->_fleetBatteryTelemetryExhausted || self->_offlineEditingVehicle
            || self->_autopilotVersionObservationRemoving || self->_autopilotVersionObservationExhausted
            || self->_id != vehicleId || self->_defaultComponentId != componentId
            || self->_firmwareType != firmware || self->_vehicleType != model
            || self->_fleetBatteryTelemetryEpoch != epoch || self->_autopilotVersionObservationEpoch != versionEpoch
            || self->_fleetBatteryTelemetryNextReceipt != receipts || self->_fleetBatteryTelemetryRevision != revision) return false;
        const WeakLinkInterfacePtr now = links->primaryLink();
        if (now.expired() || primary.owner_before(now) || now.owner_before(primary)) return false;
        for (const auto& entry : links->_rgLinkInfo) {
            const WeakLinkInterfacePtr candidate(entry.link);
            if (entry.link && entry.link.get() == link.data() && !now.owner_before(candidate) && !candidate.owner_before(now)) return true;
        }
        return false;
    };
    if (!same()) return false;
    const bool connected = link->isConnected();
    if (!same() || !connected) return false;
    const bool mock = link->isMockLink();
    if (!same() || mock) return false;
    const bool replay = link->isLogReplay();
    return same() && !replay;
}

bool Vehicle::_fleetBatteryTelemetrySourceMatches() const
{
    if (QThread::currentThread() != thread() || !_fleetBatteryTelemetryLink || !_fleetBatteryTelemetryLinks
        || !_fleetBatteryTelemetryProtocol || !_fleetBatteryTelemetryVehicles || _fleetBatteryTelemetryVersionEpoch.isEmpty()
        || _fleetBatteryTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetBatteryTelemetryLinks.data() != _vehicleLinkManager || _fleetBatteryTelemetryProtocol.data() != _mavlink
        || _fleetBatteryTelemetryComponentId != _defaultComponentId || _fleetBatteryTelemetryFirmwareType != int(_firmwareType)
        || _fleetBatteryTelemetryVehicleType != int(_vehicleType) || _fleetBatteryTelemetryWeakLink.expired()) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(_fleetBatteryTelemetryLink);
    const QPointer<VehicleLinkManager> links(_fleetBatteryTelemetryLinks);
    const QPointer<MAVLinkProtocol> protocol(_fleetBatteryTelemetryProtocol);
    const QPointer<MultiVehicleManager> vehicles(_fleetBatteryTelemetryVehicles);
    const WeakLinkInterfacePtr primary(_fleetBatteryTelemetryWeakLink);
    const QString epoch = _fleetBatteryTelemetryEpoch, versionEpoch = _fleetBatteryTelemetryVersionEpoch;
    const bool allowed = _fleetBatteryTelemetrySourceAllowed(link.data());
    if (!self || !link || !links || !protocol || !vehicles || !allowed || !qgcApp() || !qgcApp()->toolbox()) return false;
    const WeakLinkInterfacePtr current = links->primaryLink();
    return self->_fleetBatteryTelemetryEpoch == epoch && self->_fleetBatteryTelemetryVersionEpoch == versionEpoch
        && self->_autopilotVersionObservationEpoch == versionEpoch && self->_fleetBatteryTelemetryLink == link
        && self->_fleetBatteryTelemetryLinks == links && self->_fleetBatteryTelemetryProtocol == protocol
        && self->_fleetBatteryTelemetryVehicles == vehicles && qgcApp()->toolbox()->multiVehicleManager() == vehicles.data()
        && !primary.expired() && !current.expired() && !primary.owner_before(current) && !current.owner_before(primary);
}

bool Vehicle::_fleetBatteryTelemetryMemoryCurrent() const
{
    // Public projections stay callback-free: link virtuals are checked at
    // ingress/commit, not from the Registry's ordinary snapshot getters.
    // This proves local source continuity, not present remote reachability.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !_fleetBatteryTelemetryClock.isValid() || !QElapsedTimer::isMonotonic()
        || _fleetBatteryTelemetryReceiveDepth || _fleetBatteryTelemetryRemoving || _fleetBatteryTelemetryExhausted
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255
        || !_fleetBatteryTelemetryLink || !_fleetBatteryTelemetryLinks || !_fleetBatteryTelemetryProtocol || !_fleetBatteryTelemetryVehicles
        || _fleetBatteryTelemetryLinks->thread() != thread()
        || _fleetBatteryTelemetryProtocol->thread() != thread() || _fleetBatteryTelemetryVehicles->thread() != thread()
        || _fleetBatteryTelemetryLinks.data() != _vehicleLinkManager || _fleetBatteryTelemetryProtocol.data() != _mavlink
        || qgcApp()->toolbox()->mavlinkProtocol() != _fleetBatteryTelemetryProtocol.data()
        || qgcApp()->toolbox()->multiVehicleManager() != _fleetBatteryTelemetryVehicles.data()
        || _fleetBatteryTelemetryVehicles->getVehicleById(_id) != this || _fleetBatteryTelemetryLinks->communicationLost()
        || _fleetBatteryTelemetryVersionEpoch.isEmpty() || _fleetBatteryTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetBatteryTelemetryComponentId != _defaultComponentId || _fleetBatteryTelemetryFirmwareType != int(_firmwareType)
        || _fleetBatteryTelemetryVehicleType != int(_vehicleType) || _fleetBatteryTelemetryWeakLink.expired()) return false;
    const WeakLinkInterfacePtr primary = _fleetBatteryTelemetryLinks->primaryLink();
    if (primary.expired() || primary.owner_before(_fleetBatteryTelemetryWeakLink) || _fleetBatteryTelemetryWeakLink.owner_before(primary)) return false;
    for (const auto& entry : _fleetBatteryTelemetryLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == _fleetBatteryTelemetryLink.data()
            && !primary.owner_before(candidate) && !candidate.owner_before(primary)) return true;
    }
    return false;
}

quint64 Vehicle::_prepareFleetBatteryTelemetryObservation(const mavlink_message_t& original, LinkInterface* sourceLink,
    int* batteryId, QVariantMap* value)
{
    *batteryId = -1; value->clear();
    if (original.msgid != MAVLINK_MSG_ID_BATTERY_STATUS || QThread::currentThread() != thread()
        || !_mavlink || sender() != _mavlink || original.sysid != _id || original.compid != _defaultComponentId) return 0;
    const QPointer<VehicleLinkManager> actualLinks(_fleetBatteryTelemetryLinks);
    if (!actualLinks || actualLinks->thread() != thread()) return 0;
    const WeakLinkInterfacePtr actualPrimary = actualLinks->primaryLink();
    bool primaryFrame = false;
    for (const auto& entry : actualLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == sourceLink && !actualPrimary.expired()
            && !actualPrimary.owner_before(candidate) && !candidate.owner_before(actualPrimary)) { primaryFrame = true; break; }
    }
    // Secondary-link reports cannot replace or refresh the PRIMARY ledger.
    if (!primaryFrame) return 0;
    const QPointer<Vehicle> self(this);
    if (_fleetBatteryTelemetryReceiveDepth != 1) { _invalidateFleetBatteryTelemetryObservation(); return 0; }
    const bool allowed = _fleetBatteryTelemetrySourceAllowed(sourceLink);
    if (!self) return 0;
    if (!allowed) { _invalidateFleetBatteryTelemetryObservation(); return 0; }
    if ((original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)
        || original.len < 1 || original.len > MAVLINK_MSG_ID_BATTERY_STATUS_LEN
        || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != MAVLINK_MSG_ID_BATTERY_STATUS_MIN_LEN)) {
        _invalidateFleetBatteryTelemetryObservation(); return 0;
    }
    // MAVLink2 may trim zero tails, including within scalar fields. Decode
    // an explicitly zero-filled payload; missing time_remaining stays unknown
    // (zero), never a measured zero-second flight capability.
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    mavlink_battery_status_t status{}; mavlink_msg_battery_status_decode(&normalized, &status);
    *batteryId = int(status.id);
    const bool percentKnown = status.battery_remaining >= 0 && status.battery_remaining <= 100;
    const bool remainingKnown = original.magic == MAVLINK_STX && original.len > MAVLINK_MSG_ID_BATTERY_STATUS_MIN_LEN
        && status.time_remaining > 0;
    // Extension presence is determined from the ORIGINAL frame, not the
    // zero-filled decoded struct. An absent/undefined state is not healthy;
    // a present zero fault mask is only a report, not a health certificate.
    const bool chargeKnown = original.magic == MAVLINK_STX && original.len > 40
        && status.charge_state != MAV_BATTERY_CHARGE_STATE_UNDEFINED;
    const bool faultKnown = original.magic == MAVLINK_STX && original.len > 50;
    *value = {{QStringLiteral("batteryFunction"), int(status.battery_function)}, {QStringLiteral("batteryType"), int(status.type)},
        {QStringLiteral("batteryPercentKnown"), percentKnown}, {QStringLiteral("batteryPercent"), percentKnown ? double(status.battery_remaining) : -1.0},
        {QStringLiteral("remainingSecondsKnown"), remainingKnown}, {QStringLiteral("reportedRemainingSeconds"), remainingKnown ? double(status.time_remaining) : -1.0},
        {QStringLiteral("chargeStateKnown"), chargeKnown}, {QStringLiteral("chargeState"), chargeKnown ? int(status.charge_state) : -1},
        {QStringLiteral("faultBitmaskKnown"), faultKnown},
        {QStringLiteral("faultBitmask"), QVariant::fromValue(faultKnown ? qint64(status.fault_bitmask) : qint64(-1))}};
    const WeakLinkInterfacePtr primary = _vehicleLinkManager->primaryLink();
    if (_fleetBatteryTelemetryLink.data() != sourceLink || _fleetBatteryTelemetryVersionEpoch != _autopilotVersionObservationEpoch
        || _fleetBatteryTelemetryLinks.data() != _vehicleLinkManager || _fleetBatteryTelemetryProtocol.data() != _mavlink
        || _fleetBatteryTelemetryWeakLink.owner_before(primary) || primary.owner_before(_fleetBatteryTelemetryWeakLink)) {
        _invalidateFleetBatteryTelemetryObservation();
        if (_fleetBatteryTelemetryExhausted) { value->clear(); return 0; }
        _fleetBatteryTelemetryLink = sourceLink; _fleetBatteryTelemetryWeakLink = primary;
        _fleetBatteryTelemetryVersionEpoch = _autopilotVersionObservationEpoch;
        _fleetBatteryTelemetryComponentId = _defaultComponentId;
        _fleetBatteryTelemetryFirmwareType = int(_firmwareType); _fleetBatteryTelemetryVehicleType = int(_vehicleType);
        const QString epoch = _fleetBatteryTelemetryEpoch;
        const auto lost = [this, epoch]() {
            if (QThread::currentThread() == thread() && _fleetBatteryTelemetryEpoch == epoch) _invalidateFleetBatteryTelemetryObservation();
        };
        const auto disconnected = connect(sourceLink, &LinkInterface::disconnected, this, lost);
        if (!self) return 0;
        if (_fleetBatteryTelemetryEpoch != epoch) { QObject::disconnect(disconnected); value->clear(); return 0; }
        _fleetBatteryTelemetryLinkConnections.append(disconnected);
        const auto destroyed = connect(sourceLink, &QObject::destroyed, this, lost);
        if (!self) return 0;
        if (_fleetBatteryTelemetryEpoch != epoch) { QObject::disconnect(destroyed); value->clear(); return 0; }
        _fleetBatteryTelemetryLinkConnections.append(destroyed);
    }
    const bool stillCurrent = _fleetBatteryTelemetrySourceMatches();
    if (!self) return 0;
    if (!stillCurrent) { _invalidateFleetBatteryTelemetryObservation(); value->clear(); return 0; }
    if (!_fleetBatteryTelemetryRows.contains(*batteryId)) {
        if (_fleetBatteryTelemetryRows.size() >= 16) {
            if (!_fleetBatteryTelemetrySlotOverflow) {
                _fleetBatteryTelemetrySlotOverflow = true;
                if (_fleetBatteryTelemetryRevision == std::numeric_limits<quint64>::max()) _fleetBatteryTelemetryExhausted = true;
                else ++_fleetBatteryTelemetryRevision;
            }
            value->clear(); return 0;
        }
        _fleetBatteryTelemetryRows.insert(*batteryId, FleetBatteryTelemetryRow());
    }
    auto row = _fleetBatteryTelemetryRows.find(*batteryId);
    if (_fleetBatteryTelemetryNextReceipt == std::numeric_limits<quint64>::max()) {
        _fleetBatteryTelemetryExhausted = true; _invalidateFleetBatteryTelemetryObservation(); value->clear(); return 0;
    }
    if (!row->observed || row->value != *value) {
        if (_fleetBatteryTelemetryRevision == std::numeric_limits<quint64>::max()) {
            _fleetBatteryTelemetryExhausted = true; _invalidateFleetBatteryTelemetryObservation(); value->clear(); return 0;
        }
        row->contentRevision = ++_fleetBatteryTelemetryRevision;
    }
    row->updateToken = ++_fleetBatteryTelemetryNextReceipt;
    row->updating = true; row->complete = false;
    return row->updateToken;
}

void Vehicle::_completeFleetBatteryTelemetryObservation(int batteryId, quint64 token, const QString& epoch,
    const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs, const FleetBatteryDrainIngress& drainIngress)
{
    if (!token || QThread::currentThread() != thread() || epoch != _fleetBatteryTelemetryEpoch
        || _fleetBatteryTelemetryReceiveDepth != 1 || receivedAtMs < 0 || sourceLink != _fleetBatteryTelemetryLink.data()) return;
    const auto row = _fleetBatteryTelemetryRows.constFind(batteryId);
    if (row == _fleetBatteryTelemetryRows.constEnd() || row->updateToken != token || !row->updating) return;
    const QPointer<Vehicle> self(this);
    const bool sourceCurrent = _fleetBatteryTelemetrySourceMatches();
    if (!self || !sourceCurrent || epoch != _fleetBatteryTelemetryEpoch || _fleetBatteryTelemetryReceiveDepth != 1) return;
    auto current = _fleetBatteryTelemetryRows.find(batteryId);
    if (current == _fleetBatteryTelemetryRows.end() || current->updateToken != token || !current->updating || value.size() != 10) return;
    current->value = value; current->receiptSequence = token; current->receivedAtMs = receivedAtMs;
    current->observed = true; current->complete = true; current->updating = false;
    _observeFleetBatteryDrain(current.value(), token, drainIngress);
}

QVariantMap Vehicle::fleetBatteryDrainObservation(qint64 maximumAgeMs) const
{
    const QVariantMap unknownWindow{{QStringLiteral("startReceiptSequence"), QStringLiteral("0")},
        {QStringLiteral("endReceiptSequence"), QStringLiteral("0")}, {QStringLiteral("startPercent"), -1.0},
        {QStringLiteral("endPercent"), -1.0}, {QStringLiteral("durationMs"), QVariant::fromValue(qint64(-1))},
        {QStringLiteral("sampleCount"), 0}, {QStringLiteral("maximumGapMs"), QVariant::fromValue(qint64(-1))},
        {QStringLiteral("endAgeMs"), QVariant::fromValue(qint64(-1))}};
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("native-soc-decline-window")},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("componentId"), 0}, {QStringLiteral("sourceEpoch"), QStringLiteral("")},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("batteryContentRevision"), QStringLiteral("0")},
        {QStringLiteral("batteryId"), -1}, {QStringLiteral("batteryReceiptSequence"), QStringLiteral("0")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("observed"), false}, {QStringLiteral("state"), QStringLiteral("source-unavailable")},
        {QStringLiteral("reason"), QStringLiteral("source-unavailable")}, {QStringLiteral("ageMs"), QVariant::fromValue(qint64(-1))},
        {QStringLiteral("window"), unknownWindow}, {QStringLiteral("drainKnown"), false}, {QStringLiteral("percentPerSecond"), -1.0},
        {QStringLiteral("remainingFlightSeconds"), -1.0}, {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("remoteMeasurementTimeVerified"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || maximumAgeMs < 1 || maximumAgeMs > 3000) return result;
    const QVariantMap battery = fleetBatteryTelemetryObservation(maximumAgeMs);
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("sourceEpoch"), QStringLiteral("sourceCurrent")})
        result[key] = battery.value(key);
    result[QStringLiteral("batteryContentRevision")] = battery.value(QStringLiteral("contentRevision"));
    result[QStringLiteral("contentRevision")] = QString::number(_fleetBatteryDrainRevision);
    const auto state = [&result](const QString& value) { result[QStringLiteral("state")] = value; result[QStringLiteral("reason")] = value; };
    if (_fleetMessageIntervalRawReceiveDepth || _fleetMessageIntervalRawReceiveExhausted || _fleetBatteryTelemetryReceiveDepth) {
        result[QStringLiteral("sourceCurrent")] = false; state(QStringLiteral("receive-in-progress")); return result;
    }
    const QString aggregation = battery.value(QStringLiteral("aggregationState")).toString();
    if (aggregation != QStringLiteral("reported-estimate")) { state(aggregation); return result; }
    const int id = battery.value(QStringLiteral("batteryId")).toInt();
    const auto found = _fleetBatteryTelemetryRows.constFind(id);
    if (found == _fleetBatteryTelemetryRows.constEnd() || !found->observed || !found->complete || found->updating
        || QString::number(found->receiptSequence) != battery.value(QStringLiteral("receiptSequence")).toString()) return result;
    const FleetBatteryTelemetryRow& row = found.value();
    result[QStringLiteral("observed")] = true; result[QStringLiteral("batteryId")] = id;
    result[QStringLiteral("batteryReceiptSequence")] = battery.value(QStringLiteral("receiptSequence"));
    const qint64 now = _fleetBatteryTelemetryClock.isValid() ? _fleetBatteryTelemetryClock.elapsed() : -1;
    const qint64 age = row.drainReceivedAtMs >= 0 && now >= row.drainReceivedAtMs ? now - row.drainReceivedAtMs : -1;
    result[QStringLiteral("ageMs")] = QVariant::fromValue(age);
    if (_fleetBatteryDrainExhausted) { state(QStringLiteral("counter-exhausted")); return result; }
    if (!battery.value(QStringLiteral("batteryPercentKnown")).toBool()) { state(QStringLiteral("unknown-soc")); return result; }
    if (age < 0 || age > maximumAgeMs) { state(QStringLiteral("stale-flight-battery")); return result; }
    if (row.drainSampleOverflow) { state(QStringLiteral("sample-limit-exceeded")); return result; }
    const FleetBatteryDrainWindow& window = row.drainWindow;
    if (!window.published) { state(QStringLiteral("warming-window")); return result; }
    const qint64 endAge = window.endAtMs >= 0 && now >= window.endAtMs ? now - window.endAtMs : -1;
    const double percent = battery.value(QStringLiteral("batteryPercent")).toDouble();
    const qint64 duration = window.endAtMs - window.startAtMs;
    if (duration < 30000 || duration > 120000 || window.startPercent - window.endPercent < 2
        || window.endPercent != percent || window.maximumGapMs < 0 || window.maximumGapMs > 3000
        || window.sampleCount < 2 || window.sampleCount > 256 || !window.startReceipt || !window.endReceipt
        || window.startReceipt >= window.endReceipt || window.endReceipt > row.receiptSequence
        || endAge < 0 || endAge > 9007199254740991LL) {
        state(QStringLiteral("warming-window")); return result;
    }
    result[QStringLiteral("window")] = QVariantMap{{QStringLiteral("startReceiptSequence"), QString::number(window.startReceipt)},
        {QStringLiteral("endReceiptSequence"), QString::number(window.endReceipt)}, {QStringLiteral("startPercent"), double(window.startPercent)},
        {QStringLiteral("endPercent"), double(window.endPercent)}, {QStringLiteral("durationMs"), QVariant::fromValue(duration)},
        {QStringLiteral("sampleCount"), window.sampleCount}, {QStringLiteral("maximumGapMs"), QVariant::fromValue(window.maximumGapMs)},
        {QStringLiteral("endAgeMs"), QVariant::fromValue(endAge)}};
    if (endAge > 30000) { state(QStringLiteral("stale-decline-window")); return result; }
    const double rate = double(window.startPercent - window.endPercent) * 1000.0 / double(duration);
    const double remaining = qMax(0.0, percent / rate - double(qMax(age, endAge)) / 1000.0);
    if (!qIsFinite(rate) || rate <= 0.0 || !qIsFinite(remaining)) {
        result[QStringLiteral("window")] = unknownWindow;
        state(QStringLiteral("warming-window")); return result;
    }
    result[QStringLiteral("drainKnown")] = true; result[QStringLiteral("percentPerSecond")] = rate;
    result[QStringLiteral("remainingFlightSeconds")] = remaining;
    result[QStringLiteral("state")] = QStringLiteral("reported-decline"); result[QStringLiteral("reason")] = QStringLiteral("");
    return result;
}

QVariantMap Vehicle::fleetBatteryTelemetryObservation(qint64 maximumAgeMs) const
{
    QVariantMap result{{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("vehicleId"), 0}, {QStringLiteral("componentId"), 0},
        {QStringLiteral("sourceEpoch"), QStringLiteral("")}, {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("rows"), QVariantList()},
        {QStringLiteral("aggregationState"), QStringLiteral("source-unavailable")}, {QStringLiteral("batteryPercentKnown"), false},
        {QStringLiteral("batteryPercent"), -1.0}, {QStringLiteral("remainingSecondsKnown"), false}, {QStringLiteral("remainingFlightSeconds"), -1.0},
        {QStringLiteral("reportedRemainingSeconds"), -1.0}, {QStringLiteral("ageMs"), QVariant::fromValue(qint64(-1))}, {QStringLiteral("batteryId"), -1},
        {QStringLiteral("receiptSequence"), QStringLiteral("0")}, {QStringLiteral("contentRevision"), QStringLiteral("0")},
        {QStringLiteral("readonly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || maximumAgeMs < 1 || maximumAgeMs > 120000) return result;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QString epoch = _fleetBatteryTelemetryEpoch;
    const quint64 receipts = _fleetBatteryTelemetryNextReceipt, revision = _fleetBatteryTelemetryRevision;
    result[QStringLiteral("vehicleId")] = _id; result[QStringLiteral("componentId")] = _defaultComponentId;
    result[QStringLiteral("sourceEpoch")] = epoch; result[QStringLiteral("contentRevision")] = QString::number(revision);
    if (_fleetBatteryTelemetryReceiveDepth != 0) {
        result[QStringLiteral("aggregationState")] = QStringLiteral("receive-in-progress"); return result;
    }
    if (!_fleetBatteryTelemetryClock.isValid()) return result;
    if (_fleetBatteryTelemetryRows.isEmpty() && !_fleetBatteryTelemetryRemoving && !_fleetBatteryTelemetryExhausted) {
        result[QStringLiteral("aggregationState")] = QStringLiteral("no-observation"); return result;
    }
    const bool sourceCurrent = _fleetBatteryTelemetryMemoryCurrent();
    if (!self || !sourceCurrent || self->_fleetBatteryTelemetryEpoch != epoch
        || self->_fleetBatteryTelemetryNextReceipt != receipts || self->_fleetBatteryTelemetryRevision != revision
        || self->_fleetBatteryTelemetryReceiveDepth != 0) return result;
    // No callbacks in this getter. These ages are time since
    // LOCAL receipt, not the remote sensor's measurement age or authentication.
    const qint64 now = self->_fleetBatteryTelemetryClock.elapsed();
    QVariantList rows;
    QVariantMap selected;
    int flightBatteries = 0;
    bool unknownRole = false;
    for (auto it = self->_fleetBatteryTelemetryRows.constBegin(); it != self->_fleetBatteryTelemetryRows.constEnd(); ++it) {
        const FleetBatteryTelemetryRow& row = it.value();
        const qint64 age = row.observed && row.receivedAtMs >= 0 && now >= row.receivedAtMs ? now - row.receivedAtMs : -1;
        const bool available = row.observed && row.complete && !row.updating && row.receiptSequence && row.value.size() == 10 && age >= 0;
        const bool fresh = available && age <= maximumAgeMs;
        const bool percentKnown = available && row.value.value(QStringLiteral("batteryPercentKnown")).toBool();
        const bool remainingKnown = available && row.value.value(QStringLiteral("remainingSecondsKnown")).toBool();
        const bool chargeKnown = available && row.value.value(QStringLiteral("chargeStateKnown")).toBool();
        const bool faultKnown = available && row.value.value(QStringLiteral("faultBitmaskKnown")).toBool();
        const int function = row.value.value(QStringLiteral("batteryFunction"), 0).toInt();
        const double reportedRemaining = remainingKnown ? row.value.value(QStringLiteral("reportedRemainingSeconds")).toDouble() : -1.0;
        const double agedRemaining = remainingKnown ? qMax(0.0, reportedRemaining - double(age) / 1000.0) : -1.0;
        QVariantMap projected{{QStringLiteral("batteryId"), it.key()}, {QStringLiteral("batteryFunction"), function},
            {QStringLiteral("batteryType"), row.value.value(QStringLiteral("batteryType"), 0).toInt()},
            {QStringLiteral("observed"), row.observed}, {QStringLiteral("fresh"), fresh}, {QStringLiteral("ageMs"), QVariant::fromValue(age)},
            {QStringLiteral("batteryPercentKnown"), percentKnown},
            {QStringLiteral("batteryPercent"), percentKnown ? row.value.value(QStringLiteral("batteryPercent")).toDouble() : -1.0},
            {QStringLiteral("remainingSecondsKnown"), remainingKnown}, {QStringLiteral("reportedRemainingSeconds"), reportedRemaining},
            {QStringLiteral("chargeStateKnown"), chargeKnown},
            {QStringLiteral("chargeState"), chargeKnown ? row.value.value(QStringLiteral("chargeState")).toInt() : -1},
            {QStringLiteral("faultBitmaskKnown"), faultKnown},
            {QStringLiteral("faultBitmask"), QVariant::fromValue(faultKnown ? row.value.value(QStringLiteral("faultBitmask")).toLongLong() : qint64(-1))},
            {QStringLiteral("remainingFlightSeconds"), agedRemaining}, {QStringLiteral("receiptSequence"), QString::number(row.receiptSequence)},
            {QStringLiteral("contentRevision"), QString::number(row.contentRevision)}, {QStringLiteral("sourceEpoch"), epoch},
            {QStringLiteral("sourceCurrent"), true}};
        rows.append(projected);
        if (function == MAV_BATTERY_FUNCTION_ALL || function == MAV_BATTERY_FUNCTION_PROPULSION) {
            ++flightBatteries; selected = projected;
        } else if (function != MAV_BATTERY_FUNCTION_AVIONICS && function != MAV_BATTERY_FUNCTION_PAYLOAD) {
            unknownRole = true;
        }
    }
    result[QStringLiteral("sourceCurrent")] = true; result[QStringLiteral("rows")] = rows;
    QString state;
    if (self->_fleetBatteryTelemetrySlotOverflow) state = QStringLiteral("slot-limit-exceeded");
    else if (unknownRole) state = QStringLiteral("unknown-battery-role");
    else if (flightBatteries > 1) state = QStringLiteral("multiple-flight-batteries");
    else if (flightBatteries == 0) state = QStringLiteral("no-flight-battery");
    else if (!selected.value(QStringLiteral("fresh")).toBool()) state = QStringLiteral("stale-flight-battery");
    else {
        state = QStringLiteral("reported-estimate");
        for (const QString& key : {QStringLiteral("batteryId"), QStringLiteral("ageMs"), QStringLiteral("receiptSequence"),
             QStringLiteral("batteryPercentKnown"), QStringLiteral("batteryPercent"), QStringLiteral("remainingSecondsKnown"),
             QStringLiteral("reportedRemainingSeconds"), QStringLiteral("remainingFlightSeconds")}) result[key] = selected.value(key);
    }
    result[QStringLiteral("aggregationState")] = state;
    return result;
}

bool Vehicle::_fleetTelemetrySourceAllowed(LinkInterface* link) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_mavlink || _mavlink->thread() != thread() || _offlineEditingVehicle || _fleetTelemetryRemoving
        || !qgcApp()->toolbox() || qgcApp()->toolbox()->mavlinkProtocol() != _mavlink
        || !qgcApp()->toolbox()->multiVehicleManager()
        || qgcApp()->toolbox()->multiVehicleManager()->thread() != thread()
        || qgcApp()->toolbox()->multiVehicleManager()->getVehicleById(_id) != this
        || _id < 1 || _id > 255 || !_vehicleLinkManager || _vehicleLinkManager->thread() != thread()
        || _vehicleLinkManager->communicationLost()
        || !link || !link->isConnected()
        || link->isLogReplay() || link->isMockLink() || !_vehicleLinkManager->containsLink(link)) return false;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    return primary && primary.get() == link;
}

QVariantMap Vehicle::bindFleetTelemetryObserver(VehicleTelemetryObserver* observer)
{
    const auto denied = [](const QString& code) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("errorCode"), code}};
    };
    if (QThread::currentThread() != thread() || !observer || observer->thread() != thread()
        || _fleetTelemetryObserver || _fleetTelemetryReceiveBusy || !QElapsedTimer::isMonotonic()
        || !_vehicleLinkManager) return denied(QStringLiteral("telemetry-binding-unavailable"));
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || !_fleetTelemetrySourceAllowed(primary.get()))
        return denied(QStringLiteral("telemetry-live-primary-unavailable"));
    _fleetTelemetryObserver = observer;
    _fleetTelemetryLink = primary.get();
    _fleetTelemetryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetTelemetrySequence = 0;
    _fleetTelemetryClock.start();
    const QString epoch = _fleetTelemetryEpoch;
    _fleetTelemetryConnections.append(connect(_vehicleLinkManager, &VehicleLinkManager::primaryLinkChanged,
        this, [this, epoch]() {
            if (_fleetTelemetryEpoch == epoch) _retireFleetTelemetryObserver(QStringLiteral("telemetry-primary-changed"));
        }, Qt::DirectConnection));
    _fleetTelemetryConnections.append(connect(_vehicleLinkManager, &VehicleLinkManager::communicationLostChanged,
        this, [this, epoch](bool lost) {
            if (lost && _fleetTelemetryEpoch == epoch) _retireFleetTelemetryObserver(QStringLiteral("telemetry-communication-lost"));
        }, Qt::DirectConnection));
    const auto recheck = [this, epoch]() {
        if (_fleetTelemetryEpoch == epoch && !_fleetTelemetrySourceAllowed(_fleetTelemetryLink.data()))
            _retireFleetTelemetryObserver(QStringLiteral("telemetry-source-unavailable"));
    };
    _fleetTelemetryConnections.append(connect(_vehicleLinkManager, &VehicleLinkManager::linkNamesChanged,
        this, recheck, Qt::DirectConnection));
    _fleetTelemetryConnections.append(connect(_vehicleLinkManager, &VehicleLinkManager::linkStatusesChanged,
        this, recheck, Qt::DirectConnection));
    _fleetTelemetryConnections.append(connect(_vehicleLinkManager, &VehicleLinkManager::allLinksRemoved,
        this, [this, epoch](Vehicle*) {
            if (_fleetTelemetryEpoch == epoch) _retireFleetTelemetryObserver(QStringLiteral("telemetry-links-removed"));
        }, Qt::DirectConnection));
    _fleetTelemetryConnections.append(connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved,
        this, [this, epoch](Vehicle* removed) {
            if (removed == this && _fleetTelemetryEpoch == epoch)
                _retireFleetTelemetryObserver(QStringLiteral("telemetry-vehicle-removed"));
        }, Qt::DirectConnection));
    _fleetTelemetryConnections.append(connect(primary.get(), &QObject::destroyed, this, [this, epoch]() {
        if (_fleetTelemetryEpoch == epoch) _retireFleetTelemetryObserver(QStringLiteral("telemetry-link-destroyed"));
    }));
    _fleetTelemetryConnections.append(connect(observer, &QObject::destroyed, this, [this, epoch]() {
        if (_fleetTelemetryEpoch == epoch) _retireFleetTelemetryObserver(QStringLiteral("telemetry-observer-destroyed"));
    }));
    return {{QStringLiteral("ok"), true}, {QStringLiteral("sourceEpoch"), epoch},
        {QStringLiteral("sourceSequence"), QStringLiteral("0")}};
}

bool Vehicle::fleetTelemetryBindingMatches(VehicleTelemetryObserver* observer, const QString& sourceEpoch) const
{
    return QThread::currentThread() == thread() && observer && observer->thread() == thread()
        && _fleetTelemetryObserver.data() == observer && !sourceEpoch.isEmpty() && sourceEpoch == _fleetTelemetryEpoch
        && _fleetTelemetryClock.isValid() && _fleetTelemetrySourceAllowed(_fleetTelemetryLink.data());
}

bool Vehicle::unbindFleetTelemetryObserver(VehicleTelemetryObserver* observer, const QString& sourceEpoch)
{
    if (QThread::currentThread() != thread() || !observer || _fleetTelemetryObserver.data() != observer
        || sourceEpoch != _fleetTelemetryEpoch) return false;
    _fleetTelemetryObserver.clear(); _fleetTelemetryLink.clear();
    for (const QMetaObject::Connection& connection : _fleetTelemetryConnections) disconnect(connection);
    _fleetTelemetryConnections.clear();
    _fleetTelemetryClock.invalidate();
    return true;
}

void Vehicle::_retireFleetTelemetryObserver(const QString& fixedCode)
{
    if (QThread::currentThread() != thread()) return;
    const QPointer<VehicleTelemetryObserver> observer(_fleetTelemetryObserver);
    const QString epoch = _fleetTelemetryEpoch;
    const quint64 sequence = _fleetTelemetrySequence;
    _fleetTelemetryObserver.clear(); _fleetTelemetryLink.clear();
    for (const QMetaObject::Connection& connection : _fleetTelemetryConnections) disconnect(connection);
    _fleetTelemetryConnections.clear(); _fleetTelemetryClock.invalidate();
    // Clear ownership before the direct value-only callback; it can retire
    // other members, destroy either object, or re-enter native code.
    if (observer && observer->thread() == thread())
        observer->observeTelemetrySourceRetired(this, epoch, sequence, fixedCode);
}

void Vehicle::_observeFleetTelemetry(const mavlink_message_t& originalMessage, LinkInterface* link)
{
    if (!_fleetTelemetryObserver || sender() != _mavlink || originalMessage.sysid != _id
        || originalMessage.compid == 0 || link != _fleetTelemetryLink.data()) return;
    if (_fleetTelemetryObserver->thread() != thread() || !_fleetTelemetrySourceAllowed(link) || !_fleetTelemetryClock.isValid()) {
        _retireFleetTelemetryObserver(QStringLiteral("telemetry-native-source-invalid")); return;
    }
    if (_fleetTelemetryReceiveBusy || _fleetTelemetrySequence == std::numeric_limits<quint64>::max()) {
        _retireFleetTelemetryObserver(QStringLiteral("telemetry-native-sequence-exhausted-or-reentered")); return;
    }
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> source(link);
    const QPointer<VehicleTelemetryObserver> observer(_fleetTelemetryObserver);
    const QString epoch = _fleetTelemetryEpoch;
    const quint64 sequence = ++_fleetTelemetrySequence;
    const qint64 receivedAtMs = _fleetTelemetryClock.elapsed();
    _fleetTelemetryReceiveBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_fleetTelemetryReceiveBusy = false; });
    if (observer && source) observer->observeTelemetryFrame(this, source.data(), originalMessage, epoch, sequence, receivedAtMs);
    // No post-callback dereference of this/source; the outer receive owns its
    // independent QPointer checks. This hook has no send or stream request.
}

bool Vehicle::_fleetExecutionSourceAllowed(LinkInterface* link) const
{
    if (QThread::currentThread() != thread() || !_fleetExecutionSourceLinks || !_fleetExecutionSourceProtocol
        || !_fleetExecutionSourceVehicles || !_fleetExecutionSourceToolbox
        || !qgcApp() || thread() != qgcApp()->thread()
        || _offlineEditingVehicle || _fleetExecutionRemoving || _autopilotVersionObservationRemoving
        || !px4Firmware() || _defaultComponentId < 1 || _defaultComponentId > 255
        || _fleetExecutionSourceProtocol.data() != _mavlink || _fleetExecutionSourceProtocol->thread() != thread()
        || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || _fleetExecutionSourceToolbox->mavlinkProtocol() != _fleetExecutionSourceProtocol.data()
        || _fleetExecutionSourceToolbox->multiVehicleManager() != _fleetExecutionSourceVehicles.data()
        || _fleetExecutionSourceVehicles->thread() != thread()
        || _fleetExecutionSourceVehicles->getVehicleById(_id) != this
        || _id < 1 || _id > 255 || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || _fleetExecutionSourceLinks->thread() != thread() || _fleetExecutionSourceLinks->communicationLost() || !link
        || _fleetExecutionSlotExhausted || _fleetExecutionSourceProbeBusy
        || !_fleetExecutionSourceLinks->containsLink(link)) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> source(link);
    const QPointer<VehicleLinkManager> links(_fleetExecutionSourceLinks);
    const QPointer<MAVLinkProtocol> protocol(_fleetExecutionSourceProtocol);
    const QPointer<QGCApplication> application(qgcApp());
    const QPointer<QGCToolbox> toolbox(_fleetExecutionSourceToolbox);
    const QPointer<MultiVehicleManager> vehicles(_fleetExecutionSourceVehicles);
    const QPointer<VehicleMissionExecutionObserver> observer(_fleetExecutionObserver);
    const QPointer<LinkInterface> slotLink(_fleetExecutionLink);
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const WeakLinkInterfacePtr slotOwner = _fleetExecutionPrimaryOwner;
    const QString versionEpoch(_autopilotVersionObservationEpoch.constData(), _autopilotVersionObservationEpoch.size());
    const QString slotEpoch(_fleetExecutionEpoch.constData(), _fleetExecutionEpoch.size());
    const QString slotVersion(_fleetExecutionVersionEpoch.constData(), _fleetExecutionVersionEpoch.size());
    const quint64 revision = _fleetExecutionSlotRevision;
    const int vehicleId = _id, componentId = _defaultComponentId, slotComponentId = _fleetExecutionComponentId;
    const bool bound = !slotEpoch.isEmpty(), clockValid = _fleetExecutionClock.isValid();
    const auto sameOwner = [](const WeakLinkInterfacePtr& left, const WeakLinkInterfacePtr& right) {
        return !left.owner_before(right) && !right.owner_before(left);
    };
    // This fence invokes no Link virtual method. In particular, an equal raw
    // address cannot stand in for the captured shared-owner control block.
    const auto current = [&]() {
        if (!self || !source || !links || !protocol || !application || !toolbox || !vehicles
            || (bound && !observer) || qgcApp() != application.data()
            || QThread::currentThread() != self->thread() || self->thread() != application->thread()
            || application->toolbox() != toolbox.data() || toolbox->mavlinkProtocol() != protocol.data()
            || toolbox->multiVehicleManager() != vehicles.data() || self->_mavlink != protocol.data()
            || self->_vehicleLinkManager != links.data() || protocol->thread() != self->thread()
            || self->_fleetExecutionSourceLinks != links || self->_fleetExecutionSourceProtocol != protocol
            || self->_fleetExecutionSourceToolbox != toolbox || self->_fleetExecutionSourceVehicles != vehicles
            || links->thread() != self->thread() || vehicles->thread() != self->thread()
            || self->_offlineEditingVehicle || self->_fleetExecutionRemoving || self->_autopilotVersionObservationRemoving
            || !self->px4Firmware() || self->_id != vehicleId || self->_defaultComponentId != componentId
            || self->_autopilotVersionObservationEpoch != versionEpoch
            || self->_fleetExecutionSlotExhausted || self->_fleetExecutionSlotRevision != revision
            || self->_fleetExecutionObserver != observer || self->_fleetExecutionLink != slotLink
            || self->_fleetExecutionEpoch != slotEpoch || self->_fleetExecutionVersionEpoch != slotVersion
            || self->_fleetExecutionComponentId != slotComponentId || self->_fleetExecutionClock.isValid() != clockValid
            || !sameOwner(self->_fleetExecutionPrimaryOwner, slotOwner)
            || vehicles->getVehicleById(vehicleId) != self.data() || links->communicationLost()
            || !links->containsLink(source.data())) return false;
        const WeakLinkInterfacePtr now = links->primaryLink();
        if (primaryOwner.expired() || now.expired() || !sameOwner(now, primaryOwner)
            || now.lock().get() != source.data()) return false;
        return !bound || (slotLink == source && !slotOwner.expired() && sameOwner(slotOwner, primaryOwner));
    };
    if (!current()) return false;
    _fleetExecutionSourceProbeBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_fleetExecutionSourceProbeBusy = false; });
    // Keep the original live transport checks. Each may synchronously retire
    // the slot, replace its source, or destroy any captured QObject.
    if (!current()) return false;
    const bool connected = source->isConnected();
    if (!current() || !connected) return false;
    const bool mock = source->isMockLink();
    if (!current() || mock) return false;
    const bool replay = source->isLogReplay();
    return current() && !replay;
}

bool Vehicle::_advanceFleetExecutionSlotRevision()
{
    if (_fleetExecutionSlotExhausted) return false;
    if (_fleetExecutionSlotRevision == std::numeric_limits<quint64>::max()) {
        _fleetExecutionSlotExhausted = true;
        return false;
    }
    ++_fleetExecutionSlotRevision;
    return true;
}

bool Vehicle::_bindFleetExecutionObserver(VehicleMissionExecutionObserver* observer, LinkInterface* link,
    VehicleMissionExecutionObserver::QueueContext* context)
{
    if (QThread::currentThread() != thread() || !observer || !context || !link || observer->thread() != thread() || _fleetExecutionObserver
        || _fleetExecutionLink || !_fleetExecutionEpoch.isEmpty() || !_fleetExecutionConnections.isEmpty() || _fleetExecutionClock.isValid()
        || _fleetExecutionReceiveBusy || _fleetExecutionBindBusy || _fleetExecutionSlotExhausted
        || _fleetExecutionSourceProbeBusy || !QElapsedTimer::isMonotonic()
        || context->vehicleId != _id || context->componentId != _defaultComponentId
        || context->startupId.isEmpty() || context->stepId.isEmpty() || context->operationId.isEmpty()
        || context->wirePayloadHash.isEmpty() || !context->sourceEpoch.isEmpty()) return false;
    const QPointer<Vehicle> self(this);
    _fleetExecutionBindBusy = true;
    const auto releaseBind = qScopeGuard([self]() { if (self) self->_fleetExecutionBindBusy = false; });
    const QPointer<VehicleMissionExecutionObserver> expectedObserver(observer);
    const QPointer<LinkInterface> source(link);
    const QPointer<VehicleLinkManager> links(_fleetExecutionSourceLinks);
    const QPointer<MAVLinkProtocol> protocol(_fleetExecutionSourceProtocol);
    const QPointer<QGCToolbox> toolbox(_fleetExecutionSourceToolbox);
    const QPointer<MultiVehicleManager> vehicles(_fleetExecutionSourceVehicles);
    if (!links || !protocol || !toolbox || !vehicles || !qgcApp() || qgcApp()->toolbox() != toolbox.data()) return false;
    const QPointer<QGCApplication> application(qgcApp());
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const quint64 beforeRevision = _fleetExecutionSlotRevision;
    const QString beforeEpoch(_fleetExecutionEpoch.constData(), _fleetExecutionEpoch.size());
    const QString versionEpoch(_autopilotVersionObservationEpoch.constData(), _autopilotVersionObservationEpoch.size());
    VehicleMissionExecutionObserver::QueueContext proposed;
    const auto own = [](const QString& value) { return QString(value.constData(), value.size()); };
    proposed.sourceEpoch = own(context->sourceEpoch); proposed.startupId = own(context->startupId);
    proposed.stepId = own(context->stepId); proposed.operationId = own(context->operationId);
    proposed.wirePayloadHash = own(context->wirePayloadHash);
    proposed.vehicleId = context->vehicleId; proposed.componentId = context->componentId;
    const auto beforeCurrent = [&]() {
        if (!self || !expectedObserver || !source || !links || !protocol || !application || !toolbox || !vehicles
            || qgcApp() != application.data() || application->toolbox() != toolbox.data()
            || self->_vehicleLinkManager != links.data() || self->_mavlink != protocol.data()
            || toolbox->mavlinkProtocol() != protocol.data() || toolbox->multiVehicleManager() != vehicles.data()
            || self->_fleetExecutionSourceLinks != links || self->_fleetExecutionSourceProtocol != protocol
            || self->_fleetExecutionSourceToolbox != toolbox || self->_fleetExecutionSourceVehicles != vehicles
            || QThread::currentThread() != self->thread() || self->thread() != application->thread()
            || links->thread() != self->thread() || protocol->thread() != self->thread() || vehicles->thread() != self->thread()
            || self->_offlineEditingVehicle || self->_fleetExecutionRemoving || self->_autopilotVersionObservationRemoving
            || !self->px4Firmware() || links->communicationLost() || !links->containsLink(source.data())
            || vehicles->getVehicleById(proposed.vehicleId) != self.data()
            || expectedObserver->thread() != self->thread() || self->_id != proposed.vehicleId
            || self->_defaultComponentId != proposed.componentId || self->_fleetExecutionObserver || self->_fleetExecutionLink
            || !self->_fleetExecutionEpoch.isEmpty() || !self->_fleetExecutionConnections.isEmpty() || self->_fleetExecutionClock.isValid()
            || self->_fleetExecutionReceiveBusy || self->_fleetExecutionSlotExhausted
            || self->_fleetExecutionSlotRevision != beforeRevision || self->_fleetExecutionEpoch != beforeEpoch
            || self->_autopilotVersionObservationEpoch != versionEpoch) return false;
        const WeakLinkInterfacePtr now = links->primaryLink();
        return !primaryOwner.expired() && !now.expired() && !now.owner_before(primaryOwner)
            && !primaryOwner.owner_before(now) && now.lock().get() == source.data();
    };
    const bool allowed = _fleetExecutionSourceAllowed(source.data());
    if (!beforeCurrent() || !allowed) return false;
    const QString epoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (beforeRevision == std::numeric_limits<quint64>::max()) {
        _fleetExecutionSlotExhausted = true; return false;
    }
    const quint64 boundRevision = beforeRevision + 1;
    const auto retire = [self, epoch, boundRevision](const QString& reason) {
        if (!self || QThread::currentThread() != self->thread()) return;
        if (self->_fleetExecutionEpoch == epoch && self->_fleetExecutionSlotRevision == boundRevision)
            self->_retireFleetExecutionObserver(reason);
    };
    const auto recheck = [self, epoch, boundRevision]() {
        if (!self || QThread::currentThread() != self->thread() || self->_fleetExecutionEpoch != epoch
            || self->_fleetExecutionSlotRevision != boundRevision) return;
        const QPointer<LinkInterface> originalLink(self->_fleetExecutionLink);
        const QPointer<VehicleMissionExecutionObserver> originalObserver(self->_fleetExecutionObserver);
        const bool available = self->_fleetExecutionSourceAllowed(originalLink.data());
        if (!self || self->_fleetExecutionEpoch != epoch || self->_fleetExecutionSlotRevision != boundRevision
            || self->_fleetExecutionLink != originalLink || self->_fleetExecutionObserver != originalObserver) return;
        if (!available) self->_retireFleetExecutionObserver(QStringLiteral("execution-source-unavailable"));
    };
    QList<QMetaObject::Connection> connections;
    bool installed = false;
    const auto discard = qScopeGuard([&connections, &installed]() {
        if (!installed) for (const QMetaObject::Connection& connection : connections) QObject::disconnect(connection);
    });
    const auto add = [&](const QMetaObject::Connection& connection) {
        connections.append(connection);
        return bool(connection) && beforeCurrent();
    };
    // Stage connections locally: connectNotify is also an overridable Qt
    // boundary. No partially installed slot may overwrite a reentrant bind.
    if (!add(connect(links.data(), &VehicleLinkManager::primaryLinkChanged,
            this, [retire]() { retire(QStringLiteral("execution-primary-changed")); }, Qt::DirectConnection))
        || !add(connect(links.data(), &VehicleLinkManager::communicationLostChanged,
            this, [retire](bool lost) { if (lost) retire(QStringLiteral("execution-communication-lost")); }, Qt::DirectConnection))
        || !add(connect(links.data(), &VehicleLinkManager::linkNamesChanged, this, recheck, Qt::DirectConnection))
        || !add(connect(links.data(), &VehicleLinkManager::linkStatusesChanged, this, recheck, Qt::DirectConnection))
        || !add(connect(links.data(), &VehicleLinkManager::allLinksRemoved,
            this, [retire](Vehicle*) { retire(QStringLiteral("execution-links-removed")); }, Qt::DirectConnection))
        || !add(connect(vehicles.data(), &MultiVehicleManager::vehicleRemoved,
            this, [self, retire](Vehicle* removed) { if (self && removed == self.data()) retire(QStringLiteral("execution-vehicle-removed")); }, Qt::DirectConnection))
        || !add(connect(this, &Vehicle::firmwareTypeChanged,
            this, [retire]() { retire(QStringLiteral("execution-vehicle-model-changed")); }, Qt::DirectConnection))
        || !add(connect(this, &Vehicle::vehicleTypeChanged,
            this, [retire]() { retire(QStringLiteral("execution-vehicle-model-changed")); }, Qt::DirectConnection))
        || !add(connect(this, &Vehicle::autopilotVersionObservationSourceChanged,
            this, [retire]() { retire(QStringLiteral("execution-version-source-changed")); }, Qt::DirectConnection))
        || !add(connect(source.data(), &LinkInterface::disconnected,
            this, [retire]() { retire(QStringLiteral("execution-link-disconnected")); }))
        || !add(connect(source.data(), &QObject::destroyed,
            this, [retire]() { retire(QStringLiteral("execution-link-destroyed")); }))
        || !add(connect(protocol.data(), &QObject::destroyed,
            this, [retire]() { retire(QStringLiteral("execution-protocol-destroyed")); }, Qt::DirectConnection))
        || !add(connect(expectedObserver.data(), &QObject::destroyed,
            this, [retire]() { retire(QStringLiteral("execution-observer-destroyed")); }, Qt::DirectConnection))) return false;
    if (!beforeCurrent()) return false;
    const bool finalAllowed = self->_fleetExecutionSourceAllowed(source.data());
    if (!beforeCurrent() || !finalAllowed || context->sourceEpoch != proposed.sourceEpoch || context->startupId != proposed.startupId
        || context->stepId != proposed.stepId || context->operationId != proposed.operationId
        || context->wirePayloadHash != proposed.wirePayloadHash || context->vehicleId != proposed.vehicleId
        || context->componentId != proposed.componentId || !_advanceFleetExecutionSlotRevision()) return false;
    _fleetExecutionObserver = expectedObserver;
    _fleetExecutionLink = source;
    _fleetExecutionPrimaryOwner = primaryOwner;
    _fleetExecutionEpoch = epoch;
    _fleetExecutionVersionEpoch = versionEpoch;
    _fleetExecutionComponentId = proposed.componentId;
    _fleetExecutionSequence = 0;
    _fleetExecutionClock.start();
    _fleetExecutionConnections = connections;
    installed = true;
    context->sourceEpoch = QString(epoch.constData(), epoch.size());
    return true;
}

bool Vehicle::fleetExecutionBindingMatches(VehicleMissionExecutionObserver* observer, const QString& sourceEpoch) const
{
    if (QThread::currentThread() != thread() || !observer || observer->thread() != thread()
        || _fleetExecutionObserver.data() != observer || sourceEpoch.isEmpty() || sourceEpoch != _fleetExecutionEpoch
        || _fleetExecutionComponentId != _defaultComponentId || !_fleetExecutionClock.isValid()
        || _fleetExecutionVersionEpoch != _autopilotVersionObservationEpoch || _fleetExecutionSlotExhausted) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<VehicleMissionExecutionObserver> expectedObserver(observer);
    const QPointer<LinkInterface> link(_fleetExecutionLink);
    const QString epoch(sourceEpoch.constData(), sourceEpoch.size());
    const QString version(_fleetExecutionVersionEpoch.constData(), _fleetExecutionVersionEpoch.size());
    const quint64 revision = _fleetExecutionSlotRevision;
    const bool allowed = _fleetExecutionSourceAllowed(link.data());
    return self && expectedObserver && link && allowed && !self->_fleetExecutionSlotExhausted
        && self->_fleetExecutionSlotRevision == revision && self->_fleetExecutionObserver == expectedObserver
        && self->_fleetExecutionLink == link && self->_fleetExecutionEpoch == epoch
        && self->_fleetExecutionVersionEpoch == version && self->_autopilotVersionObservationEpoch == version
        && self->_fleetExecutionComponentId == self->_defaultComponentId && self->_fleetExecutionClock.isValid();
}

bool Vehicle::unbindFleetExecutionObserver(VehicleMissionExecutionObserver* observer, const QString& sourceEpoch)
{
    if (QThread::currentThread() != thread() || !observer || _fleetExecutionObserver.data() != observer
        || sourceEpoch.isEmpty() || sourceEpoch != _fleetExecutionEpoch) return false;
    _advanceFleetExecutionSlotRevision();
    _fleetExecutionObserver.clear(); _fleetExecutionLink.clear(); _fleetExecutionPrimaryOwner.reset();
    const QList<QMetaObject::Connection> connections = _fleetExecutionConnections;
    _fleetExecutionConnections.clear(); _fleetExecutionClock.invalidate();
    _fleetExecutionEpoch.clear(); _fleetExecutionVersionEpoch.clear();
    // Clear the old slot before disconnectNotify can reenter or destroy it.
    for (const QMetaObject::Connection& connection : connections) QObject::disconnect(connection);
    return true;
}

void Vehicle::_retireFleetExecutionObserver(const QString& fixedCode, bool deferNotification)
{
    if (QThread::currentThread() != thread()) return;
    const QPointer<VehicleMissionExecutionObserver> observer(_fleetExecutionObserver);
    const QPointer<Vehicle> self(this);
    QThread* const ownerThread = thread();
    const QString epoch = _fleetExecutionEpoch;
    const QString code(fixedCode.constData(), fixedCode.size());
    const quint64 sequence = _fleetExecutionSequence;
    _advanceFleetExecutionSlotRevision();
    _fleetExecutionObserver.clear(); _fleetExecutionLink.clear(); _fleetExecutionPrimaryOwner.reset();
    const QList<QMetaObject::Connection> connections = _fleetExecutionConnections;
    _fleetExecutionConnections.clear(); _fleetExecutionClock.invalidate();
    _fleetExecutionEpoch.clear(); _fleetExecutionVersionEpoch.clear();
    for (const QMetaObject::Connection& connection : connections) QObject::disconnect(connection);
    // Invalidate first. Retirement cannot erase the separately captured queue
    // context; an already-entered actual write must still report its fact.
    if (!observer || epoch.isEmpty() || observer->thread() != ownerThread) return;
    if (deferNotification) {
        // Version invalidation historically introduced no synchronous callback
        // into its many callers. Clear the slot now; publish the old epoch only
        // after that stack unwinds, without extending Vehicle's lifetime.
        QTimer::singleShot(0, observer.data(), [self, observer, epoch, sequence, code]() {
            if (observer) observer->observeExecutionSourceRetired(self.data(), epoch, sequence, code);
        });
    } else {
        observer->observeExecutionSourceRetired(self.data(), epoch, sequence, code);
    }
}

void Vehicle::_observeFleetExecution(const mavlink_message_t& originalMessage, LinkInterface* link)
{
    if (!_fleetExecutionObserver || sender() != _mavlink || originalMessage.sysid != _id
        || originalMessage.compid != _fleetExecutionComponentId || link != _fleetExecutionLink.data()) return;
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> source(link);
    const QPointer<VehicleMissionExecutionObserver> observer(_fleetExecutionObserver);
    const QString epoch(_fleetExecutionEpoch.constData(), _fleetExecutionEpoch.size());
    const quint64 revision = _fleetExecutionSlotRevision;
    const bool matched = fleetExecutionBindingMatches(observer.data(), epoch);
    if (!self || self->_fleetExecutionSlotRevision != revision || self->_fleetExecutionEpoch != epoch
        || self->_fleetExecutionObserver != observer || self->_fleetExecutionLink != source) return;
    if (!matched || !source || !observer) {
        self->_retireFleetExecutionObserver(QStringLiteral("execution-native-source-invalid")); return;
    }
    if (_fleetExecutionReceiveBusy || _fleetExecutionSequence == std::numeric_limits<quint64>::max()) {
        _retireFleetExecutionObserver(QStringLiteral("execution-native-sequence-exhausted-or-reentered")); return;
    }
    const qint64 receivedAtMs = _fleetExecutionClock.elapsed();
    if (receivedAtMs < 0) { _retireFleetExecutionObserver(QStringLiteral("execution-native-clock-invalid")); return; }
    const quint64 sequence = ++_fleetExecutionSequence;
    _fleetExecutionReceiveBusy = true;
    const auto release = qScopeGuard([self, revision, epoch]() {
        if (self && (!self->_fleetExecutionObserver
            || (self->_fleetExecutionSlotRevision == revision && self->_fleetExecutionEpoch == epoch)))
            self->_fleetExecutionReceiveBusy = false;
    });
    if (observer && source) observer->observeExecutionFrame(this, source.data(), originalMessage, epoch, sequence, receivedAtMs);
    // Direct passive call only. No signal, decoder import, stream request or
    // flight command is added to this raw pre-plugin ingress.
}

void Vehicle::_invalidateStartupCoordinates()
{
    _startupCoordinateEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _startupCoordinateRows.clear();
    _startupCoordinateRows.insert(QStringLiteral("localPosition"), StartupCoordinateRow());
    _startupCoordinateRows.insert(QStringLiteral("homePosition"), StartupCoordinateRow());
    _startupCoordinateLink.clear(); _startupCoordinateVersionEpoch.clear(); _startupCoordinateFlightEpoch.clear();
}

quint64 Vehicle::_prepareStartupCoordinateObservation(const mavlink_message_t& original, LinkInterface* link,
    QString* channel, QVariantMap* value)
{
    channel->clear(); value->clear();
    if (original.msgid != MAVLINK_MSG_ID_HOME_POSITION && original.msgid != MAVLINK_MSG_ID_LOCAL_POSITION_NED) return 0;
    if (sender() != _mavlink || original.sysid != _id || original.compid != _defaultComponentId
        || !_autopilotVersionSourceAllowed(link) || !link || link->isMockLink() || _startupCoordinateExhausted) return 0;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != link) return 0;
    if (_startupCoordinateLink.data() != link || _startupCoordinateVersionEpoch != _autopilotVersionObservationEpoch
        || _startupCoordinateFlightEpoch != _flightStateObservationEpoch) {
        _invalidateStartupCoordinates();
        _startupCoordinateLink = link; _startupCoordinateVersionEpoch = _autopilotVersionObservationEpoch;
        _startupCoordinateFlightEpoch = _flightStateObservationEpoch;
    }
    if (!startupCoordinateDecode(original, channel, value)) {
        const QString key = original.msgid == MAVLINK_MSG_ID_HOME_POSITION ? QStringLiteral("homePosition") : QStringLiteral("localPosition");
        _startupCoordinateRows[key] = StartupCoordinateRow();
        _startupCoordinateEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
        return 0;
    }
    const StartupCoordinateRow previous = _startupCoordinateRows.value(*channel);
    if (*channel == QStringLiteral("localPosition") && previous.observed) {
        const quint32 before = previous.value.value(QStringLiteral("timeBootMs")).toUInt(), now = value->value(QStringLiteral("timeBootMs")).toUInt();
        if (now < before && quint64(before) - now <= 0x80000000ULL) {
            _invalidateAutopilotVersionObservation(QStringLiteral("startup-local-timeboot-regressed"));
            value->clear();
            emit autopilotVersionObservationSourceChanged();
            return 0; // No access after observers can destroy this Vehicle.
        }
    }
    if (_startupCoordinateNextReceipt == std::numeric_limits<quint64>::max()) {
        _startupCoordinateExhausted = true; _invalidateStartupCoordinates(); return 0;
    }
    auto row = _startupCoordinateRows.find(*channel);
    row->updateToken = ++_startupCoordinateNextReceipt; row->complete = false; row->updating = true;
    return row->updateToken;
}

void Vehicle::_completeStartupCoordinateObservation(const QString& channel, quint64 token, const QString& epoch,
    const QVariantMap& value, const mavlink_message_t& original, LinkInterface* link, qint64 receivedAtMs)
{
    auto row = _startupCoordinateRows.find(channel);
    if (!token || epoch != _startupCoordinateEpoch || row == _startupCoordinateRows.end() || row->updateToken != token
        || _startupCoordinateExhausted || value.size() != (channel == QStringLiteral("homePosition") ? 9 : 7)
        || receivedAtMs < 0 || !_autopilotVersionSourceAllowed(link)
        || _startupCoordinateLink.data() != link || _startupCoordinateVersionEpoch != _autopilotVersionObservationEpoch
        || _startupCoordinateFlightEpoch != _flightStateObservationEpoch) return;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != link) return;
    QVariantMap risk = value;
    if (channel == QStringLiteral("localPosition")) risk.remove(QStringLiteral("timeBootMs"));
    const QString content = startupValueHash(value), riskHash = startupValueHash(risk);
    if ((!row->observed || row->contentHash != content) && _startupCoordinateContentRevision == std::numeric_limits<quint64>::max()) {
        _startupCoordinateExhausted = true; _invalidateStartupCoordinates(); return;
    }
    if ((!row->observed || row->riskContentHash != riskHash) && _startupCoordinateRiskRevision == std::numeric_limits<quint64>::max()) {
        _startupCoordinateExhausted = true; _invalidateStartupCoordinates(); return;
    }
    if (!row->observed || row->contentHash != content) row->contentRevision = ++_startupCoordinateContentRevision;
    if (!row->observed || row->riskContentHash != riskHash) row->riskContentRevision = ++_startupCoordinateRiskRevision;
    row->value = value; row->contentHash = content; row->riskContentHash = riskHash; row->rawPayloadHash = startupRawHash(original);
    row->receiptSequence = token; row->receivedAtMs = receivedAtMs; row->observed = row->complete = true; row->updating = false;
}

bool Vehicle::startupCoordinateObservationSourceMatches(LinkInterface* expectedPrimary) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(expectedPrimary);
    const QPointer<VehicleLinkManager> links(_vehicleLinkManager);
    if (!links || links->thread() != thread()) return false;
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const QString coordinateEpoch = _startupCoordinateEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch;
    const quint64 receipt = _startupCoordinateNextReceipt, riskRevision = _startupCoordinateRiskRevision;
    const bool sourceAllowed = _autopilotVersionSourceAllowed(link.data());
    if (!self || !link || !links || !sourceAllowed) return false;
    const bool mock = link->isMockLink();
    if (!self || !link || !links || QThread::currentThread() != self->thread() || !qgcApp() || self->thread() != qgcApp()->thread()
        || links->thread() != self->thread() || self->_vehicleLinkManager != links.data() || mock
        || _startupCoordinateEpoch != coordinateEpoch || _autopilotVersionObservationEpoch != versionEpoch || _flightStateObservationEpoch != flightEpoch
        || _startupCoordinateNextReceipt != receipt || _startupCoordinateRiskRevision != riskRevision
        || _startupCoordinateExhausted || _startupCoordinateReceiveDepth != 0 || _startupOdometryReceiveDepth != 0 || _flightStateObservationReceiveDepth != 0
        || _autopilotVersionObservationReceiveDepth != 0 || _fleetStartupReceiveDepth != 0
        || _startupCoordinateEpoch.isEmpty() || _startupCoordinateLink.data() != expectedPrimary
        || _startupCoordinateVersionEpoch != _autopilotVersionObservationEpoch
        || _startupCoordinateFlightEpoch != _flightStateObservationEpoch || !isInitialConnectComplete()) return false;
    const WeakLinkInterfacePtr primary = links->primaryLink();
    if (primary.expired() || primaryOwner.expired() || primary.owner_before(primaryOwner) || primaryOwner.owner_before(primary)) return false;
    for (const auto& entry : links->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == link.data() && !primary.owner_before(candidate) && !candidate.owner_before(primary)) return true;
    }
    return false;
}

QVariantMap Vehicle::startupCoordinateObservation(qint64 maximumAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const bool source = maximumAgeMs >= 1 && maximumAgeMs <= 120000
        && startupCoordinateObservationSourceMatches(_startupCoordinateLink.data());
    if (!self || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const qint64 now = _autopilotVersionObservationClock.isValid() ? _autopilotVersionObservationClock.elapsed() : -1;
    QVariantMap channels;
    bool observed = false;
    for (const QString& key : {QStringLiteral("localPosition"), QStringLiteral("homePosition")}) {
        const StartupCoordinateRow row = _startupCoordinateRows.value(key);
        const qint64 age = row.receivedAtMs >= 0 && now >= row.receivedAtMs ? now - row.receivedAtMs : -1;
        const bool available = source && row.observed && row.complete && !row.updating && row.receiptSequence != 0 && age >= 0;
        observed = observed || row.observed;
        channels.insert(key, QVariantMap{{QStringLiteral("observed"), row.observed}, {QStringLiteral("available"), available},
            {QStringLiteral("fresh"), available && age <= maximumAgeMs}, {QStringLiteral("ageMs"), double(age)},
            {QStringLiteral("contentRevision"), QString::number(row.contentRevision)}, {QStringLiteral("receiptSequence"), QString::number(row.receiptSequence)},
            {QStringLiteral("riskContentRevision"), QString::number(row.riskContentRevision)}, {QStringLiteral("contentHash"), row.contentHash},
            {QStringLiteral("rawPayloadHash"), row.rawPayloadHash}, {QStringLiteral("riskContentHash"), row.riskContentHash},
            {QStringLiteral("source"), row.observed ? (key == QStringLiteral("localPosition") ? QStringLiteral("live-local-position-ned") : QStringLiteral("live-home-position-geo-local")) : QStringLiteral("unknown")},
            {QStringLiteral("value"), row.value}, {QStringLiteral("receivedAtMonotonicMs"), double(row.receivedAtMs)}});
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), _id}, {QStringLiteral("componentId"), _defaultComponentId},
        {QStringLiteral("sourceEpoch"), _autopilotVersionObservationEpoch}, {QStringLiteral("flightSourceEpoch"), _flightStateObservationEpoch},
        {QStringLiteral("coordinateSourceEpoch"), _startupCoordinateEpoch}, {QStringLiteral("observed"), observed},
        {QStringLiteral("available"), source}, {QStringLiteral("channels"), channels}, {QStringLiteral("authenticated"), false}};
}

void Vehicle::_invalidateStartupOdometry(const QString& source)
{
    // Counter history survives source loss and invalid frames. An equal later
    // projection cannot resurrect an earlier reference after an ABA change.
    if (_startupOdometryReferenceRevision == std::numeric_limits<quint64>::max()) {
        _startupOdometryExhausted = true;
    } else {
        ++_startupOdometryReferenceRevision;
    }
    _startupOdometryEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QObject::disconnect(_startupOdometryLinkDestroyedConnection);
    _startupOdometryRow = StartupOdometryRow();
    _startupOdometryLink.clear(); _startupOdometryVersionEpoch.clear(); _startupOdometryFlightEpoch.clear();
    _startupOdometrySource = source;
    _startupOdometryClockDiscontinuityObserved = false;
    _startupOdometryLastPreparedReferenceHash.clear();
    _startupOdometryLastRawSampleTime = 0; _startupOdometryRawSampleObserved = false;
    // Active receive depths belong to their scope guards and are never reset.
}

quint64 Vehicle::_prepareStartupOdometryObservation(const mavlink_message_t& original, LinkInterface* link,
    QVariantMap* value)
{
    value->clear();
    if (original.msgid != MAVLINK_MSG_ID_ODOMETRY || !_mavlink || sender() != _mavlink
        || original.sysid != _id || original.compid != _defaultComponentId) return 0;
    if (!_autopilotVersionSourceAllowed(link) || !link || link->isMockLink() || !px4Firmware()
        || !multiRotor() || !isInitialConnectComplete() || _startupOdometryExhausted
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete
        || _autopilotVersionObservationUpdating || _autopilotVersionObservationLink.data() != link) return 0;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != link || primary->isMockLink()) return 0;
#ifdef QT_DEBUG
    const SharedLinkConfigurationPtr configuration = link->linkConfiguration();
    if (!configuration || configuration->type() == LinkConfiguration::TypeMock) return 0;
#endif
    // A malformed/filtered same-source stream must not later masquerade as an
    // optional stream that has never existed. This survives every epoch loss.
    _startupOdometryEverObservedRawFrame = true;
    if (_startupOdometryLink.data() != link || _startupOdometryVersionEpoch != _autopilotVersionObservationEpoch
        || _startupOdometryFlightEpoch != _flightStateObservationEpoch) {
        _invalidateStartupOdometry(QStringLiteral("odometry-source-context-change"));
        if (_startupOdometryExhausted) return 0;
        _startupOdometryLink = link;
        _startupOdometryVersionEpoch = _autopilotVersionObservationEpoch;
        _startupOdometryFlightEpoch = _flightStateObservationEpoch;
        const QString epoch = _startupOdometryEpoch;
        _startupOdometryLinkDestroyedConnection = connect(link, &QObject::destroyed, this, [this, epoch]() {
            if (_startupOdometryEpoch == epoch) _invalidateStartupOdometry(QStringLiteral("odometry-source-link-removed"));
        });
    }
    bool accepted = false;
    *value = FleetStartupOdometry::decode(original, &accepted);
    if (!accepted || value->size() != 50
        || value->value(QStringLiteral("decodeMapShape")).toString() != QStringLiteral("fleet-startup-odometry-v1")) {
        value->clear();
        _invalidateStartupOdometry(QStringLiteral("invalid-odometry-wire"));
        return 0;
    }
    if (_startupOdometryNextReceipt == std::numeric_limits<quint64>::max()) {
        _startupOdometryExhausted = true;
        _invalidateStartupOdometry(QStringLiteral("odometry-receipt-exhausted"));
        value->clear(); return 0;
    }
    const QVariantMap projection = FleetStartupOdometry::referenceProjection(*value);
    bool sampleTimeOk = false;
    const quint64 sampleTime = value->value(QStringLiteral("timeUsec")).toString().toULongLong(&sampleTimeOk);
    if (projection.size() != 42 || !sampleTimeOk
        || projection.value(QStringLiteral("referenceMapShape")).toString() != QStringLiteral("fleet-startup-odometry-reference-v1")) {
        _invalidateStartupOdometry(QStringLiteral("invalid-odometry-reference")); value->clear(); return 0;
    }
    const QString referenceHash = startupValueHash(projection);
    // time_usec has an unknown clock domain. A decrease is a discontinuity
    // observation, never authenticated reboot or LOCAL/HOME sample linkage.
    const bool clockDiscontinuity = _startupOdometryRawSampleObserved && sampleTime < _startupOdometryLastRawSampleTime;
    const bool referenceChanged = _startupOdometryLastPreparedReferenceHash != referenceHash || clockDiscontinuity;
    if ((referenceChanged && _startupOdometryReferenceRevision == std::numeric_limits<quint64>::max())
        || (clockDiscontinuity && _startupOdometryClockDiscontinuityRevision == std::numeric_limits<quint64>::max())) {
        _startupOdometryExhausted = true;
        _invalidateStartupOdometry(QStringLiteral("odometry-reference-exhausted")); value->clear(); return 0;
    }
    // Count RAW reference changes before the first plugin/notification call.
    // Nested B -> A receipts therefore cannot erase B by publishing only A.
    if (referenceChanged) ++_startupOdometryReferenceRevision;
    if (clockDiscontinuity) {
        ++_startupOdometryClockDiscontinuityRevision;
        _startupOdometryClockDiscontinuityObserved = true;
    }
    _startupOdometryLastPreparedReferenceHash = referenceHash;
    _startupOdometryLastRawSampleTime = sampleTime; _startupOdometryRawSampleObserved = true;
    _startupOdometryRow.updateToken = ++_startupOdometryNextReceipt;
    _startupOdometryRow.complete = false; _startupOdometryRow.updating = true;
    return _startupOdometryRow.updateToken;
}

void Vehicle::_completeStartupOdometryObservation(quint64 token, const QString& epoch, const QVariantMap& value,
    const mavlink_message_t& original, LinkInterface* link, qint64 receivedAtMs)
{
    if (!token || token != _startupOdometryRow.updateToken || epoch != _startupOdometryEpoch
        || !_startupOdometryRow.updating || _startupOdometryExhausted || value.size() != 50
        || value.value(QStringLiteral("decodeMapShape")).toString() != QStringLiteral("fleet-startup-odometry-v1")
        || original.msgid != MAVLINK_MSG_ID_ODOMETRY || original.sysid != _id || original.compid != _defaultComponentId
        || receivedAtMs < 0 || !_autopilotVersionSourceAllowed(link) || !link || link->isMockLink()
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete()
        || _startupOdometryLink.data() != link || _startupOdometryVersionEpoch != _autopilotVersionObservationEpoch
        || _startupOdometryFlightEpoch != _flightStateObservationEpoch
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete
        || _autopilotVersionObservationUpdating || _autopilotVersionObservationLink.data() != link) return;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != link || primary->isMockLink()) return;
    const QVariantMap projection = FleetStartupOdometry::referenceProjection(value);
    if (projection.size() != 42
        || projection.value(QStringLiteral("referenceMapShape")).toString() != QStringLiteral("fleet-startup-odometry-reference-v1")) return;
    const QString contentHash = startupValueHash(value), referenceHash = startupValueHash(projection);
    if (referenceHash != _startupOdometryLastPreparedReferenceHash) return;
    const bool contentChanged = !_startupOdometryRow.observed || contentHash != _startupOdometryRow.contentHash;
    if (contentChanged && _startupOdometryContentRevision == std::numeric_limits<quint64>::max()) {
        _startupOdometryExhausted = true;
        _invalidateStartupOdometry(QStringLiteral("odometry-content-exhausted")); return;
    }
    if (contentChanged) ++_startupOdometryContentRevision;
    _startupOdometryRow.value = value; _startupOdometryRow.referenceProjection = projection;
    _startupOdometryRow.contentHash = contentHash; _startupOdometryRow.referenceHash = referenceHash;
    _startupOdometryRow.rawPayloadHash = startupRawHash(original); _startupOdometryRow.rawPayloadLength = original.len;
    _startupOdometryRow.receiptSequence = token; _startupOdometryRow.receivedAtMs = receivedAtMs;
    _startupOdometryRow.observed = _startupOdometryRow.complete = true; _startupOdometryRow.updating = false;
    _startupOdometrySource = QStringLiteral("live-odometry");
}

bool Vehicle::startupOdometryObservationSourceMatches(LinkInterface* expectedPrimary) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<LinkInterface> link(expectedPrimary);
    const QPointer<VehicleLinkManager> links(_vehicleLinkManager);
    if (!links || links->thread() != thread()) return false;
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const QString odometryEpoch = _startupOdometryEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch;
    const quint64 receipt = _startupOdometryNextReceipt, referenceRevision = _startupOdometryReferenceRevision;
    const bool sourceAllowed = _autopilotVersionSourceAllowed(link.data());
    if (!self || !link || !links || !sourceAllowed) return false;
    const bool mock = link->isMockLink();
    if (!self || !link || !links || QThread::currentThread() != self->thread() || !qgcApp() || self->thread() != qgcApp()->thread()
        || links->thread() != self->thread() || self->_vehicleLinkManager != links.data() || mock
        || _startupOdometryEpoch != odometryEpoch || _autopilotVersionObservationEpoch != versionEpoch || _flightStateObservationEpoch != flightEpoch
        || _startupOdometryNextReceipt != receipt || _startupOdometryReferenceRevision != referenceRevision
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete() || _startupOdometryExhausted
        || _startupOdometryReceiveDepth != 0 || _startupCoordinateReceiveDepth != 0 || _fleetStartupReceiveDepth != 0
        || _flightStateObservationReceiveDepth != 0 || _autopilotVersionObservationReceiveDepth != 0
        || !_startupOdometryRow.observed || !_startupOdometryRow.complete || _startupOdometryRow.updating
        || _startupOdometryRow.receiptSequence == 0 || _startupOdometryEpoch.isEmpty()
        || _startupOdometryLink.data() != expectedPrimary || _startupOdometryVersionEpoch != _autopilotVersionObservationEpoch
        || _startupOdometryFlightEpoch != _flightStateObservationEpoch
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || _autopilotVersionObservationLink.data() != expectedPrimary) return false;
    const WeakLinkInterfacePtr primary = links->primaryLink();
    if (primary.expired() || primaryOwner.expired() || primary.owner_before(primaryOwner) || primaryOwner.owner_before(primary)) return false;
    for (const auto& entry : links->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && entry.link.get() == link.data() && !primary.owner_before(candidate) && !candidate.owner_before(primary)) return true;
    }
    return false; // The same actual PRIMARY was already checked for mock above.
}

QVariantMap Vehicle::startupOdometryObservation(qint64 maximumAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    QVariantMap result = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), 0}, {QStringLiteral("componentId"), 0},
        {QStringLiteral("sourceEpoch"), QString()}, {QStringLiteral("flightSourceEpoch"), QString()}, {QStringLiteral("odometrySourceEpoch"), QString()},
        {QStringLiteral("observed"), false}, {QStringLiteral("everObservedRawFrame"), false},
        {QStringLiteral("available"), false}, {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), -1.0},
        {QStringLiteral("source"), QStringLiteral("unknown")}, {QStringLiteral("receiptSequence"), QStringLiteral("0")},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("contentHash"), QString()}, {QStringLiteral("rawPayloadHash"), QString()},
        {QStringLiteral("rawPayloadLength"), 0}, {QStringLiteral("referenceRevision"), QStringLiteral("0")}, {QStringLiteral("referenceHash"), QString()},
        {QStringLiteral("receivedAtMonotonicMs"), -1.0}, {QStringLiteral("clockDiscontinuityObserved"), false},
        {QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("0")}, {QStringLiteral("value"), QVariantMap()},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return result;
    const qint64 now = _autopilotVersionObservationClock.isValid() ? _autopilotVersionObservationClock.elapsed() : -1;
    const qint64 age = _startupOdometryRow.receivedAtMs >= 0 && now >= _startupOdometryRow.receivedAtMs ? now - _startupOdometryRow.receivedAtMs : -1;
    const bool available = maximumAgeMs >= 1 && maximumAgeMs <= 120000 && age >= 0
        && startupOdometryObservationSourceMatches(_startupOdometryLink.data());
    if (!self || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    result[QStringLiteral("vehicleId")] = _id; result[QStringLiteral("componentId")] = _defaultComponentId;
    result[QStringLiteral("sourceEpoch")] = _autopilotVersionObservationEpoch;
    result[QStringLiteral("flightSourceEpoch")] = _flightStateObservationEpoch;
    result[QStringLiteral("odometrySourceEpoch")] = _startupOdometryEpoch;
    result[QStringLiteral("observed")] = _startupOdometryRow.observed;
    result[QStringLiteral("everObservedRawFrame")] = _startupOdometryEverObservedRawFrame;
    result[QStringLiteral("available")] = available; result[QStringLiteral("fresh")] = available && age <= maximumAgeMs;
    result[QStringLiteral("ageMs")] = double(age); result[QStringLiteral("source")] = _startupOdometrySource;
    result[QStringLiteral("receiptSequence")] = QString::number(_startupOdometryRow.receiptSequence);
    result[QStringLiteral("contentRevision")] = QString::number(_startupOdometryContentRevision);
    result[QStringLiteral("contentHash")] = _startupOdometryRow.contentHash; result[QStringLiteral("rawPayloadHash")] = _startupOdometryRow.rawPayloadHash;
    result[QStringLiteral("rawPayloadLength")] = _startupOdometryRow.rawPayloadLength;
    result[QStringLiteral("referenceRevision")] = QString::number(_startupOdometryReferenceRevision);
    result[QStringLiteral("referenceHash")] = _startupOdometryRow.referenceHash;
    result[QStringLiteral("receivedAtMonotonicMs")] = double(_startupOdometryRow.receivedAtMs);
    result[QStringLiteral("clockDiscontinuityObserved")] = _startupOdometryClockDiscontinuityObserved;
    result[QStringLiteral("clockDiscontinuityRevision")] = QString::number(_startupOdometryClockDiscontinuityRevision);
    result[QStringLiteral("value")] = _startupOdometryRow.value;
    return result;
}

QVariantMap Vehicle::startupOdometryReferenceObservation(qint64 maximumAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    QVariantMap result = startupOdometryObservation(maximumAgeMs);
    if (!self) return {};
    for (const QString& key : {QStringLiteral("receiptSequence"), QStringLiteral("contentRevision"), QStringLiteral("contentHash"),
        QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength"), QStringLiteral("receivedAtMonotonicMs"), QStringLiteral("value")}) result.remove(key);
    // Stable metadata only: no per-packet pose/quaternion/velocity, sample
    // timestamp or RAW receipt identity leaks into this risk projection.
    result[QStringLiteral("referenceProjection")] = QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        ? _startupOdometryRow.referenceProjection : QVariantMap();
    return result;
}

bool Vehicle::_autopilotVersionSourceAllowed(LinkInterface* sourceLink) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving
        || _autopilotVersionObservationExhausted || !_autopilotVersionObservationClock.isValid()
        || !QElapsedTimer::isMonotonic() || _id < 1 || _id > 255
        || _defaultComponentId < 1 || _defaultComponentId > 255 || !_vehicleLinkManager || !sourceLink) return false;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<VehicleLinkManager> links(_vehicleLinkManager);
    const QPointer<MAVLinkProtocol> protocol(_mavlink);
    if (!links || links->thread() != thread() || links->communicationLost()) return false;
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    if (primaryOwner.expired()) return false;
    QPointer<LinkInterface> source, primary;
    for (const auto& entry : links->_rgLinkInfo) {
        if (entry.link && entry.link.get() == sourceLink) source = entry.link.get();
        const WeakLinkInterfacePtr candidate(entry.link);
        if (entry.link && !primaryOwner.owner_before(candidate) && !candidate.owner_before(primaryOwner)) primary = entry.link.get();
    }
    // Preserve the original secondary-link allowance. Both that actual member
    // and the separately observed PRIMARY must remain connected/non-replay.
    if (!source || !primary) return false;
    const QString epoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch, coordinateEpoch = _startupCoordinateEpoch;
    const quint64 updateToken = _autopilotVersionObservationUpdateToken, receipt = _autopilotVersionObservationLastReceipt;
    const quint64 revision = _autopilotVersionObservationRevision, flightReceipt = _flightStateObservationReceiptSequence;
    const quint64 coordinateReceipt = _startupCoordinateNextReceipt, odometryReceipt = _startupOdometryNextReceipt;
    const int vehicleId = _id, componentId = _defaultComponentId;
    const MAV_AUTOPILOT firmware = _firmwareType;
    const MAV_TYPE model = _vehicleType;
    const auto same = [&]() {
        if (!self || !links || !source || !primary || !qgcApp() || self->thread() != qgcApp()->thread()
            || QThread::currentThread() != self->thread() || links->thread() != self->thread()
            || self->_vehicleLinkManager != links.data() || self->_mavlink != protocol.data()
            || self->_offlineEditingVehicle || self->_autopilotVersionObservationRemoving || self->_autopilotVersionObservationExhausted
            || !self->_autopilotVersionObservationClock.isValid() || links->communicationLost()
            || self->_id != vehicleId || self->_defaultComponentId != componentId || self->_firmwareType != firmware || self->_vehicleType != model
            || self->_autopilotVersionObservationEpoch != epoch || self->_flightStateObservationEpoch != flightEpoch
            || self->_startupCoordinateEpoch != coordinateEpoch || self->_autopilotVersionObservationUpdateToken != updateToken
            || self->_autopilotVersionObservationLastReceipt != receipt || self->_autopilotVersionObservationRevision != revision
            || self->_flightStateObservationReceiptSequence != flightReceipt || self->_startupCoordinateNextReceipt != coordinateReceipt
            || self->_startupOdometryNextReceipt != odometryReceipt) return false;
        const WeakLinkInterfacePtr now = links->primaryLink();
        if (now.expired() || primaryOwner.expired() || now.owner_before(primaryOwner) || primaryOwner.owner_before(now)) return false;
        bool sourcePresent = false, primaryPresent = false;
        for (const auto& entry : links->_rgLinkInfo) {
            if (entry.link && entry.link.get() == source.data()) sourcePresent = true;
            const WeakLinkInterfacePtr candidate(entry.link);
            if (entry.link && entry.link.get() == primary.data() && !now.owner_before(candidate) && !candidate.owner_before(now)) primaryPresent = true;
        }
        return sourcePresent && primaryPresent;
    };
    const bool sourceConnected = source->isConnected();
    if (!same() || !sourceConnected) return false;
    const bool sourceReplay = source->isLogReplay();
    if (!same() || sourceReplay) return false;
    const bool primaryConnected = primary->isConnected();
    if (!same() || !primaryConnected) return false;
    const bool primaryReplay = primary->isLogReplay();
    return same() && !primaryReplay;
}

void Vehicle::_invalidateAutopilotVersionObservation(const QString& source)
{
    if (_fleetCameraStreamControlRun) {
        _fleetCameraStreamControlRun->sourceRetired = true;
        _finishFleetCameraStreamControl(_fleetCameraStreamControlRun, QStringLiteral("failed"), QStringLiteral("camera-control-source-invalidated"), true);
    }
    if (_fleetCameraStreamRun) {
        _fleetCameraStreamRun->sourceRetired = true;
        _finishFleetCameraStreamCapture(_fleetCameraStreamRun, QStringLiteral("failed"), QStringLiteral("camera-capture-source-invalidated"), true);
    }
    _retireFleetLinkLatencyCapture(QStringLiteral("link-rtt-source-invalidated"));
    _invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-source-invalidated"));
    _invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-source-invalidated"));
    // Retire memory only. Dispatching a user result here could delete this
    // Vehicle while the existing central invalidation is still in progress.
    // A VERSION datum's first anchor is not a new physical transport. The
    // constructor's legitimate generic 512 may receive VERSION before ACK.
    // Native approved refresh still keeps its original version epoch below.
    if (source != QStringLiteral("version-source-link-change")) {
        _messageCommandTransportEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!_messageCommandLaneId.isEmpty()) _messageCommandLaneRetired = true;
    } else {
        const int index = _messageCommandIndex(_messageCommandLaneId);
        if (index >= 0 && _mavCommandList.at(index).fleetEvidenceReadOnly) _messageCommandLaneRetired = true;
    }
    _invalidateOptionalTelemetryObservations();
    _invalidateFleetBatteryTelemetryObservation();
    _invalidateFleetHealthTelemetryObservation(source);
    _autopilotVersionObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool changed = _autopilotVersionObservationSource != source
        || _autopilotVersionObservationObserved || !_autopilotVersionObservationFields.isEmpty();
    if (changed) {
        if (_autopilotVersionObservationRevision == std::numeric_limits<quint64>::max()) {
            _autopilotVersionObservationExhausted = true;
        } else {
            ++_autopilotVersionObservationRevision;
        }
    }
    _autopilotVersionObservationSource = source;
    _autopilotVersionObservationFields.clear();
    _autopilotVersionObservationContentHash.clear();
    _autopilotVersionObservationLink.clear();
    _autopilotVersionObservationReceivedAtMs = -1;
    _autopilotVersionObservationLastReceipt = 0;
    _autopilotVersionObservationObserved = false;
    _autopilotVersionObservationComplete = false;
    _autopilotVersionObservationUpdating = false;
    _invalidateStartupCoordinates();
    _invalidateFlightStateObservations(source);
    _retireFleetExecutionObserver(QStringLiteral("execution-version-source-invalidated"), true);
}

quint64 Vehicle::_prepareAutopilotVersionObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink)
{
    // Only this Vehicle's actual MAVLinkProtocol signal entry can prepare a
    // receipt. A plugin-adjusted copy, fallback, or ordinary helper call is
    // not a wire observation; this still is not sender authentication.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_mavlink || sender() != _mavlink || originalMessage.msgid != MAVLINK_MSG_ID_AUTOPILOT_VERSION
        || _id < 1 || _id > 255 || originalMessage.sysid != _id
        || _defaultComponentId < 1 || _defaultComponentId > 255 || originalMessage.compid != _defaultComponentId) return 0;
    if (!_autopilotVersionSourceAllowed(sourceLink)) {
        _invalidateAutopilotVersionObservation(QStringLiteral("untrusted-autopilot-version-source"));
        return 0;
    }
    // MAVLink2 trims any trailing zero payload bytes, including base fields
    // and partially present UID2. Do not impose MIN_LEN60 on a valid V2 frame.
    if ((originalMessage.magic != MAVLINK_STX && originalMessage.magic != MAVLINK_STX_MAVLINK1)
        || originalMessage.len < 1 || originalMessage.len > MAVLINK_MSG_ID_AUTOPILOT_VERSION_LEN
        || (originalMessage.magic == MAVLINK_STX_MAVLINK1 && originalMessage.len != MAVLINK_MSG_ID_AUTOPILOT_VERSION_MIN_LEN)) {
        _invalidateAutopilotVersionObservation(QStringLiteral("invalid-autopilot-version-wire"));
        return 0;
    }
    if (_autopilotVersionObservationLink.data() != sourceLink
        && (_autopilotVersionObservationLink || _autopilotVersionObservationObserved || _autopilotVersionObservationUpdating)) {
        _invalidateAutopilotVersionObservation(QStringLiteral("version-source-link-change"));
    }
    if (_autopilotVersionObservationUpdateToken == std::numeric_limits<quint64>::max()) {
        _autopilotVersionObservationExhausted = true;
        _invalidateAutopilotVersionObservation(QStringLiteral("observation-exhausted"));
        return 0;
    }
    if (_autopilotVersionObservationLink.data() != sourceLink) {
        QObject::disconnect(_autopilotVersionObservationLinkDestroyedConnection);
        _autopilotVersionObservationLink = sourceLink;
        const QString sourceEpoch = _autopilotVersionObservationEpoch;
        _autopilotVersionObservationLinkDestroyedConnection = connect(sourceLink, &QObject::destroyed, this, [this, sourceEpoch]() {
            if (_autopilotVersionObservationEpoch == sourceEpoch) {
                _invalidateAutopilotVersionObservation(QStringLiteral("version-source-link-removed"));
            }
        });
    }
    _autopilotVersionObservationUpdating = true;
    _autopilotVersionObservationComplete = false;
    return ++_autopilotVersionObservationUpdateToken;
}

bool Vehicle::_autopilotVersionObservationUpdateCurrent(quint64 token, const QString& epoch) const
{
    return token != 0 && token == _autopilotVersionObservationUpdateToken
        && epoch == _autopilotVersionObservationEpoch && _autopilotVersionObservationUpdating;
}

void Vehicle::_completeAutopilotVersionObservation(quint64 token, const QString& epoch,
    const mavlink_message_t& originalMessage, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (!_autopilotVersionObservationUpdateCurrent(token, epoch) || !_autopilotVersionSourceAllowed(sourceLink)
        || _autopilotVersionObservationLink.data() != sourceLink || originalMessage.sysid != _id
        || originalMessage.compid != _defaultComponentId || receivedAtMs < 0) return;
    // Zero-fill the omitted wire tail explicitly for both decoder branches.
    QByteArray payload(MAVLINK_MSG_ID_AUTOPILOT_VERSION_LEN, '\0');
    memcpy(payload.data(), _MAV_PAYLOAD(&originalMessage), originalMessage.len);
    mavlink_message_t normalizedMessage = originalMessage;
    memset(_MAV_PAYLOAD_NON_CONST(&normalizedMessage), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalizedMessage), payload.constData(), payload.size());
    mavlink_autopilot_version_t version{};
    mavlink_msg_autopilot_version_decode(&normalizedMessage, &version);
    const QString wireHex = QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(version.flight_custom_version),
        MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_FLIGHT_CUSTOM_VERSION_LEN).toHex());
    QString displayHex;
    if (_firmwareType == MAV_AUTOPILOT_PX4) {
        for (int index = MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_FLIGHT_CUSTOM_VERSION_LEN - 1; index >= 0; --index) {
            displayHex.append(QStringLiteral("%1").arg(version.flight_custom_version[index], 2, 16, QChar('0')));
        }
    }
    // This is the existing reverse-byte PX4 display, NOT a pure Git16 hash:
    // vendor encodings may occupy these bytes. Catalog interpretation must
    // declare its encoding, and a declared encoding is not authentication.
    QString uid2Hex;
    bool uid2Known = false;
#ifdef MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_UID2_LEN
    uid2Hex = QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(version.uid2),
        MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_UID2_LEN).toHex());
    for (const uint8_t byte : version.uid2) uid2Known = uid2Known || byte != 0;
#endif
    const QVariantMap fields = {{QStringLiteral("flightSoftwareVersion"), static_cast<uint>(version.flight_sw_version)},
        {QStringLiteral("middlewareSoftwareVersion"), static_cast<uint>(version.middleware_sw_version)},
        {QStringLiteral("osSoftwareVersion"), static_cast<uint>(version.os_sw_version)},
        {QStringLiteral("flightCustomVersionWireHex"), wireHex}, {QStringLiteral("px4CustomVersionDisplayHex"), displayHex},
        {QStringLiteral("middlewareCustomVersionWireHex"), QString::fromLatin1(QByteArray(
            reinterpret_cast<const char*>(version.middleware_custom_version),
            MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_MIDDLEWARE_CUSTOM_VERSION_LEN).toHex())},
        {QStringLiteral("osCustomVersionWireHex"), QString::fromLatin1(QByteArray(
            reinterpret_cast<const char*>(version.os_custom_version),
            MAVLINK_MSG_AUTOPILOT_VERSION_FIELD_OS_CUSTOM_VERSION_LEN).toHex())},
        {QStringLiteral("boardVersion"), static_cast<uint>(version.board_version)},
        {QStringLiteral("vendorId"), static_cast<uint>(version.vendor_id)}, {QStringLiteral("productId"), static_cast<uint>(version.product_id)},
        {QStringLiteral("capabilitiesDecimal"), QString::number(static_cast<qulonglong>(version.capabilities))},
        {QStringLiteral("uidDecimal"), QString::number(static_cast<qulonglong>(version.uid))},
        {QStringLiteral("uidKnown"), version.uid != 0}, {QStringLiteral("uid2Hex"), uid2Hex}, {QStringLiteral("uid2Known"), uid2Known},
        {QStringLiteral("vehicleId"), _id}, {QStringLiteral("componentId"), _defaultComponentId},
        {QStringLiteral("firmwareType"), static_cast<int>(_firmwareType)}, {QStringLiteral("vehicleType"), static_cast<int>(_vehicleType)}};
    // Hash identity is the 12 normalized original payload fields below as
    // QVariantMap -> QJsonObject -> Compact JSON. uint64 fields are decimal
    // strings, byte arrays are lowercase wire-order hex, uint32/16 are exact
    // JSON-safe numbers. No header, model/cache, derived display, UID-known
    // flags, signature, age, epoch, revision or callback state is in this hash.
    const QVariantMap sourceFields = {
        {QStringLiteral("capabilitiesDecimal"), fields.value(QStringLiteral("capabilitiesDecimal"))},
        {QStringLiteral("uidDecimal"), fields.value(QStringLiteral("uidDecimal"))},
        {QStringLiteral("flightSoftwareVersion"), fields.value(QStringLiteral("flightSoftwareVersion"))},
        {QStringLiteral("middlewareSoftwareVersion"), fields.value(QStringLiteral("middlewareSoftwareVersion"))},
        {QStringLiteral("osSoftwareVersion"), fields.value(QStringLiteral("osSoftwareVersion"))},
        {QStringLiteral("boardVersion"), fields.value(QStringLiteral("boardVersion"))},
        {QStringLiteral("vendorId"), fields.value(QStringLiteral("vendorId"))},
        {QStringLiteral("productId"), fields.value(QStringLiteral("productId"))},
        {QStringLiteral("flightCustomVersionWireHex"), wireHex},
        {QStringLiteral("middlewareCustomVersionWireHex"), fields.value(QStringLiteral("middlewareCustomVersionWireHex"))},
        {QStringLiteral("osCustomVersionWireHex"), fields.value(QStringLiteral("osCustomVersionWireHex"))},
        {QStringLiteral("uid2Hex"), uid2Hex}};
    const QByteArray contentJson = QJsonDocument(QJsonObject::fromVariantMap(sourceFields)).toJson(QJsonDocument::Compact);
    const QString contentHash = QString::fromLatin1(QCryptographicHash::hash(contentJson, QCryptographicHash::Sha256).toHex());
    const bool sourceIdentityChanged = !_autopilotVersionObservationObserved
        || _autopilotVersionObservationSource != QStringLiteral("live-autopilot-version")
        || _autopilotVersionObservationContentHash != contentHash;
    if (!_autopilotVersionObservationObserved || _autopilotVersionObservationSource != QStringLiteral("live-autopilot-version")
        || _autopilotVersionObservationContentHash != contentHash || _autopilotVersionObservationFields != fields) {
        if (_autopilotVersionObservationRevision == std::numeric_limits<quint64>::max()) {
            _autopilotVersionObservationExhausted = true;
            _invalidateAutopilotVersionObservation(QStringLiteral("observation-exhausted"));
            return;
        }
        ++_autopilotVersionObservationRevision;
    }
    _autopilotVersionObservationFields = fields;
    _autopilotVersionObservationContentHash = contentHash;
    _autopilotVersionObservationSource = QStringLiteral("live-autopilot-version");
    _autopilotVersionObservationReceivedAtMs = receivedAtMs;
    _autopilotVersionObservationLastReceipt = token;
    _autopilotVersionObservationObserved = true;
    _autopilotVersionObservationComplete = true;
    _autopilotVersionObservationUpdating = false;
    // First anchoring, changed raw content (including UID/UID2) or a new link
    // context revokes old parameter receipts. Identical same-source refreshes
    // must not churn their evidence. Keep this emission last: a direct slot
    // may delete this Vehicle or commit a newer nested version observation.
    if (sourceIdentityChanged) emit autopilotVersionObservationSourceChanged();
}

void Vehicle::_invalidateFlightStateObservation(const QString& channel, const QString& source)
{
    auto row = _flightStateObservations.find(channel);
    if (row == _flightStateObservations.end()) return;
    const bool changed = row->source != source || row->observed || !row->value.isEmpty();
    if (changed) {
        if (_flightStateObservationRevision == std::numeric_limits<quint64>::max()) {
            _flightStateObservationExhausted = true;
        } else {
            row->contentRevision = ++_flightStateObservationRevision;
        }
        if (_flightStateRiskObservationRevision == std::numeric_limits<quint64>::max()) {
            _flightStateObservationExhausted = true;
        } else {
            row->riskContentRevision = ++_flightStateRiskObservationRevision;
        }
    }
    // Keep the last decimal counters monotonic even when the value is revoked;
    // these are history of receipts, not an active-source execution identity.
    row->value.clear();
    row->contentHash.clear();
    row->riskContentHash.clear();
    row->source = source;
    row->receivedAtMs = -1;
    row->updateToken = 0;
    row->observed = false;
    row->complete = false;
    row->updating = false;
}

void Vehicle::_invalidateFlightStateObservations(const QString& source)
{
    _flightStateObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _invalidateStartupOdometry(source);
    _invalidateStartupGlobalOriginObservation(source);
    _invalidateStartupAltitudeObservation(source, true);
    _flightStateObservationLink.clear();
    for (const QString& channel : flightObservationChannels()) {
        _invalidateFlightStateObservation(channel, source);
    }
    // Active callback depths are owned by their QPointer scope guards, not
    // reset by an invalidation that can occur within one of those callbacks.
}

quint64 Vehicle::_prepareFlightStateObservation(const mavlink_message_t& originalMessage, LinkInterface* sourceLink,
    QString* channel, QVariantMap* value)
{
    int fullLength = 0;
    int minimumLength = 0;
    *channel = flightObservationChannel(originalMessage.msgid, &fullLength, &minimumLength);
    value->clear();
    if (channel->isEmpty() || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_mavlink || sender() != _mavlink || _id < 1 || _id > 255 || originalMessage.sysid != _id
        || _defaultComponentId < 1 || _defaultComponentId > 255 || originalMessage.compid != _defaultComponentId) return 0;
    if (!_autopilotVersionSourceAllowed(sourceLink) || _flightStateObservationExhausted) {
        _invalidateFlightStateObservations(QStringLiteral("untrusted-flight-state-source"));
        return 0;
    }
    if ((originalMessage.magic != MAVLINK_STX && originalMessage.magic != MAVLINK_STX_MAVLINK1)
        || originalMessage.len < 1 || originalMessage.len > fullLength
        || (originalMessage.magic == MAVLINK_STX_MAVLINK1 && originalMessage.len != minimumLength)) {
        _invalidateFlightStateObservation(*channel, QStringLiteral("invalid-flight-state-wire"));
        return 0;
    }
    if (_flightStateObservationLink.data() != sourceLink) {
        // Includes a removed old QPointer and the first anchoring. No six-row
        // snapshot may be assembled from different physical receive links.
        _invalidateFlightStateObservations(QStringLiteral("flight-source-link-change"));
        QObject::disconnect(_flightStateObservationLinkDestroyedConnection);
        _flightStateObservationLink = sourceLink;
        const QString sourceEpoch = _flightStateObservationEpoch;
        _flightStateObservationLinkDestroyedConnection = connect(sourceLink, &QObject::destroyed, this, [this, sourceEpoch]() {
            if (_flightStateObservationEpoch == sourceEpoch) {
                _invalidateFlightStateObservations(QStringLiteral("flight-source-link-removed"));
            }
        });
    }
    if (!decodeFlightObservation(originalMessage, _firmwareType, _vehicleType, value) || value->isEmpty() || value->size() > 16) {
        value->clear();
        _invalidateFlightStateObservation(*channel, QStringLiteral("invalid-flight-state-value"));
        return 0;
    }
    if (*channel == QStringLiteral("globalPosition") && _flightStateObservationLink.data() == sourceLink) {
        const auto previous = _flightStateObservations.constFind(*channel);
        if (previous != _flightStateObservations.constEnd() && previous->observed) {
            const quint32 oldTime = previous->value.value(QStringLiteral("timeBootMs")).toUInt();
            const quint32 newTime = value->value(QStringLiteral("timeBootMs")).toUInt();
            if (newTime < oldTime && static_cast<quint64>(oldTime) - newTime <= 0x80000000ULL) {
                // A same-source boot-time regression may be a reboot OR a
                // delayed/reordered packet. Conservatively revoke availability
                // across version/parameters/flight; this is not remote boot
                // authentication. Constant zero gives no such proof, and a
                // large uint32 natural-wrap difference is not this regression.
                _invalidateAutopilotVersionObservation(QStringLiteral("flight-timeboot-regressed"));
                value->clear();
                // Central invalidation has no signal side effects. Emit this
                // source-only revocation LAST, so PM drops old receipts without
                // a request/write; a listener may delete this or re-enter.
                emit autopilotVersionObservationSourceChanged();
                return 0;
            }
        }
    }
    if (_flightStateObservationReceiptSequence == std::numeric_limits<quint64>::max()) {
        _flightStateObservationExhausted = true;
        _invalidateFlightStateObservations(QStringLiteral("observation-exhausted"));
        return 0;
    }
    auto row = _flightStateObservations.find(*channel);
    if (row == _flightStateObservations.end()) return 0;
    row->updateToken = ++_flightStateObservationReceiptSequence;
    row->updating = true;
    row->complete = false;
    return row->updateToken;
}

bool Vehicle::_flightStateObservationUpdateCurrent(const QString& channel, quint64 token, const QString& epoch) const
{
    const auto row = _flightStateObservations.constFind(channel);
    return token != 0 && epoch == _flightStateObservationEpoch && row != _flightStateObservations.constEnd()
        && row->updateToken == token && row->updating;
}

void Vehicle::_completeFlightStateObservation(const QString& channel, quint64 token, const QString& epoch,
    const QVariantMap& value, LinkInterface* sourceLink, qint64 receivedAtMs)
{
    if (!_flightStateObservationUpdateCurrent(channel, token, epoch) || _flightStateObservationExhausted
        || !_autopilotVersionSourceAllowed(sourceLink) || _flightStateObservationLink.data() != sourceLink
        || receivedAtMs < 0 || value.isEmpty() || value.size() > 16) return;
    auto row = _flightStateObservations.find(channel);
    const QString source = flightObservationSource(channel);
    const QByteArray valueJson = QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact);
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(valueJson, QCryptographicHash::Sha256).toHex());
    QVariantMap riskValue = value;
    if (channel == QStringLiteral("globalPosition")) riskValue.remove(QStringLiteral("timeBootMs"));
    const QString riskHash = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(riskValue)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    if (!row->observed || row->source != source || row->riskContentHash != riskHash) {
        if (_flightStateRiskObservationRevision == std::numeric_limits<quint64>::max()) {
            _flightStateObservationExhausted = true;
            _invalidateFlightStateObservations(QStringLiteral("risk-observation-exhausted"));
            return;
        }
        row->riskContentRevision = ++_flightStateRiskObservationRevision;
    }
    if (!row->observed || row->source != source || row->contentHash != hash || row->value != value) {
        if (_flightStateObservationRevision == std::numeric_limits<quint64>::max()) {
            _flightStateObservationExhausted = true;
            _invalidateFlightStateObservations(QStringLiteral("observation-exhausted"));
            return;
        }
        row->contentRevision = ++_flightStateObservationRevision;
    }
    row->value = value;
    row->source = source;
    row->contentHash = hash;
    row->riskContentHash = riskHash;
    row->receivedAtMs = receivedAtMs;
    row->receiptSequence = token;
    row->observed = true;
    row->complete = true;
    row->updating = false;
}

QVariantMap Vehicle::flightStateObservation(qint64 maximumPositionAgeMs, qint64 maximumStatusAgeMs,
    qint64 maximumHomeAgeMs, qint64 maximumProgressAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    QVariantMap channels;
    for (const QString& channel : flightObservationChannels()) {
        channels.insert(channel, QVariantMap{{QStringLiteral("observed"), false}, {QStringLiteral("available"), false},
            {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), -1}, {QStringLiteral("source"), QStringLiteral("unknown")},
            {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("receiptSequence"), QStringLiteral("0")},
            {QStringLiteral("contentHash"), QString()}, {QStringLiteral("value"), QVariantMap()}});
    }
    QVariantMap result = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), 0}, {QStringLiteral("componentId"), 0},
        {QStringLiteral("sourceEpoch"), QString()}, {QStringLiteral("channels"), channels}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return result;
    result.insert(QStringLiteral("vehicleId"), _id);
    result.insert(QStringLiteral("componentId"), _defaultComponentId);
    result.insert(QStringLiteral("sourceEpoch"), _flightStateObservationEpoch);
    const bool agePolicyValid = maximumPositionAgeMs >= 1 && maximumPositionAgeMs <= 120000
        && maximumStatusAgeMs >= 1 && maximumStatusAgeMs <= 120000
        && maximumHomeAgeMs >= 1 && maximumHomeAgeMs <= 120000
        && maximumProgressAgeMs >= 1 && maximumProgressAgeMs <= 120000;
    const bool contextAvailable = agePolicyValid && !_flightStateObservationExhausted
        && _flightStateObservationReceiveDepth == 0 && _autopilotVersionObservationReceiveDepth == 0 && _startupOdometryReceiveDepth == 0
        && _initialConnectStateMachine && isInitialConnectComplete()
        && _autopilotVersionSourceAllowed(_flightStateObservationLink.data());
    if (!self || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const qint64 now = _autopilotVersionObservationClock.isValid() ? _autopilotVersionObservationClock.elapsed() : -1;
    for (const QString& channel : flightObservationChannels()) {
        const auto row = _flightStateObservations.constFind(channel);
        if (row == _flightStateObservations.constEnd()) continue;
        const qint64 maximumAge = channel == QStringLiteral("globalPosition") ? maximumPositionAgeMs
            : channel == QStringLiteral("homePosition") ? maximumHomeAgeMs
            : (channel == QStringLiteral("missionCurrent") || channel == QStringLiteral("missionReached")) ? maximumProgressAgeMs
            : maximumStatusAgeMs;
        const qint64 age = row->receivedAtMs >= 0 && now >= row->receivedAtMs ? now - row->receivedAtMs : -1;
        const bool available = contextAvailable && row->observed && row->complete && !row->updating;
        channels.insert(channel, QVariantMap{{QStringLiteral("observed"), row->observed}, {QStringLiteral("available"), available},
            {QStringLiteral("fresh"), available && age >= 0 && age <= maximumAge}, {QStringLiteral("ageMs"), age},
            {QStringLiteral("source"), row->source}, {QStringLiteral("contentRevision"), QString::number(row->contentRevision)},
            {QStringLiteral("receiptSequence"), QString::number(row->receiptSequence)}, {QStringLiteral("contentHash"), row->contentHash},
            {QStringLiteral("value"), row->value}});
    }
    result.insert(QStringLiteral("channels"), channels);
    return result;
}

QVariantMap Vehicle::flightStateRiskObservation(qint64 maximumPositionAgeMs, qint64 maximumStatusAgeMs,
    qint64 maximumHomeAgeMs, qint64 maximumProgressAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    QVariantMap result = flightStateObservation(maximumPositionAgeMs, maximumStatusAgeMs,
        maximumHomeAgeMs, maximumProgressAgeMs);
    if (!self || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const QString epoch = _flightStateObservationEpoch;
    if (result.value(QStringLiteral("sourceEpoch")).toString() != epoch) return {};
    QVariantMap channels = result.value(QStringLiteral("channels")).toMap();
    for (const QString& channel : flightObservationChannels()) {
        QVariantMap row = channels.value(channel).toMap();
        const auto native = _flightStateObservations.constFind(channel);
        const bool observed = row.value(QStringLiteral("observed")).type() == QVariant::Bool
            && row.value(QStringLiteral("observed")).toBool();
        if (native == _flightStateObservations.constEnd()) {
            if (observed) return {};
            row.insert(QStringLiteral("riskContentHash"), QString());
            row.insert(QStringLiteral("riskContentRevision"), QStringLiteral("0"));
        } else {
            // Match the full RAW capture before attaching native risk identity.
            // No caller-supplied semantic hash or reconstructed revision.
            if (native->observed != observed || row.value(QStringLiteral("value")).toMap() != native->value
                || row.value(QStringLiteral("contentHash")).toString() != native->contentHash
                || row.value(QStringLiteral("contentRevision")).toString() != QString::number(native->contentRevision)
                || row.value(QStringLiteral("receiptSequence")).toString() != QString::number(native->receiptSequence)) return {};
            row.insert(QStringLiteral("riskContentHash"), native->riskContentHash);
            row.insert(QStringLiteral("riskContentRevision"), QString::number(native->riskContentRevision));
        }
        channels.insert(channel, row);
    }
    result.insert(QStringLiteral("channels"), channels);
    result.insert(QStringLiteral("schemaVersion"), 2);
    result.insert(QStringLiteral("riskIdentityPurpose"), QStringLiteral("risk-content-not-reception-sequence"));
    return result;
}

QString Vehicle::autopilotVersionObservationSourceEpochMemory() const
{
    return QThread::currentThread() == thread() ? _autopilotVersionObservationEpoch : QString();
}

bool Vehicle::componentInformationCallbackAllowed() const
{
    return QThread::currentThread() == thread()
        && qgcApp() && thread() == qgcApp()->thread()
        && !_componentInformationCallbacksRetired
        && !_autopilotVersionObservationRemoving && !_allLinksRemovedSent;
}

bool Vehicle::autopilotVersionObservationMemoryCurrent(LinkInterface* expectedPrimary,
    const QString& sourceEpoch, const QString& contentRevision,
    const QString& contentHash, qint64 maximumAgeMs) const
{
    // This final fence deliberately avoids _autopilotVersionSourceAllowed:
    // that initial observation check calls Link virtuals. Finish every such
    // callback before using this memory-only check on the entire group.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !QElapsedTimer::isMonotonic()
        || maximumAgeMs < 1 || maximumAgeMs > 120000 || !expectedPrimary
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted
        || !_autopilotVersionObservationClock.isValid() || !_autopilotVersionObservationObserved
        || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || _autopilotVersionObservationReceiveDepth != 0 || _startupOdometryReceiveDepth != 0
        || !_initialConnectStateMachine || !isInitialConnectComplete()
        || _id < 1 || _id > 255 || _defaultComponentId < 1 || _defaultComponentId > 255
        || sourceEpoch.isEmpty() || _autopilotVersionObservationEpoch != sourceEpoch
        || _autopilotVersionObservationRevision == 0 || contentRevision != QString::number(_autopilotVersionObservationRevision)
        || contentHash.size() != 64 || contentHash != _autopilotVersionObservationContentHash
        || _autopilotVersionObservationLastReceipt == 0
        || _autopilotVersionObservationSource != QStringLiteral("live-autopilot-version")
        || _autopilotVersionObservationLink.data() != expectedPrimary
        || _autopilotVersionObservationFields.value(QStringLiteral("vehicleId")).toInt() != _id
        || _autopilotVersionObservationFields.value(QStringLiteral("componentId")).toInt() != _defaultComponentId
        || _autopilotVersionObservationFields.value(QStringLiteral("firmwareType")).toInt() != int(_firmwareType)
        || _autopilotVersionObservationFields.value(QStringLiteral("vehicleType")).toInt() != int(_vehicleType)) return false;
    const QPointer<VehicleLinkManager> links(_vehicleLinkManager);
    const QPointer<MAVLinkProtocol> protocol(_mavlink);
    const QPointer<MultiVehicleManager> vehicles(qgcApp()->toolbox()->multiVehicleManager());
    if (!links || !protocol || !vehicles || links->thread() != thread() || protocol->thread() != thread()
        || vehicles->thread() != thread() || links->communicationLost()
        || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data() || vehicles->getVehicleById(_id) != this) return false;
    const WeakLinkInterfacePtr primary = links->primaryLink();
    if (primary.expired()) return false;
    bool primaryPresent = false;
    for (const auto& entry : links->_rgLinkInfo) {
        const WeakLinkInterfacePtr member(entry.link);
        if (entry.link && entry.link.get() == expectedPrimary
            && !member.owner_before(primary) && !primary.owner_before(member)) { primaryPresent = true; break; }
    }
    if (!primaryPresent) return false;
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    return _autopilotVersionObservationReceivedAtMs >= 0 && now >= _autopilotVersionObservationReceivedAtMs
        && now - _autopilotVersionObservationReceivedAtMs <= maximumAgeMs;
}

LinkInterface* Vehicle::autopilotVersionObservationPrimaryLinkMemory() const
{
    if (QThread::currentThread() != thread()) return nullptr;
    LinkInterface* const primary = _autopilotVersionObservationLink.data();
    return autopilotVersionObservationMemoryCurrent(primary, _autopilotVersionObservationEpoch,
        QString::number(_autopilotVersionObservationRevision), _autopilotVersionObservationContentHash, 30000)
        ? primary : nullptr;
}

QVariantMap Vehicle::autopilotVersionObservation(qint64 maximumAgeMs) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    // Null/zero data below means unknown, never a fabricated version receipt.
    // uint64 values are decimal strings to avoid JSON's integer precision loss.
    QVariantMap result = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), 0},
        {QStringLiteral("componentId"), 0}, {QStringLiteral("observed"), false},
        {QStringLiteral("available"), false}, {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), -1},
        {QStringLiteral("source"), QStringLiteral("unknown")}, {QStringLiteral("sourceEpoch"), QString()},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("receiptSequence"), QStringLiteral("0")},
        {QStringLiteral("contentHash"), QString()},
        {QStringLiteral("flightSoftwareVersion"), 0U}, {QStringLiteral("middlewareSoftwareVersion"), 0U},
        {QStringLiteral("osSoftwareVersion"), 0U}, {QStringLiteral("flightCustomVersionWireHex"), QString()},
        {QStringLiteral("middlewareCustomVersionWireHex"), QString()}, {QStringLiteral("osCustomVersionWireHex"), QString()},
        {QStringLiteral("px4CustomVersionDisplayHex"), QString()}, {QStringLiteral("boardVersion"), 0U},
        {QStringLiteral("vendorId"), 0U}, {QStringLiteral("productId"), 0U},
        {QStringLiteral("capabilitiesDecimal"), QStringLiteral("0")}, {QStringLiteral("uidDecimal"), QStringLiteral("0")},
        {QStringLiteral("uidKnown"), false}, {QStringLiteral("uid2Hex"), QString()}, {QStringLiteral("uid2Known"), false},
        {QStringLiteral("firmwareType"), 0}, {QStringLiteral("vehicleType"), 0},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) {
        result.insert(QStringLiteral("source"), QStringLiteral("unsupported-thread"));
        return result;
    }
    result.insert(QStringLiteral("vehicleId"), _id);
    result.insert(QStringLiteral("componentId"), _defaultComponentId);
    result.insert(QStringLiteral("firmwareType"), static_cast<int>(_firmwareType));
    result.insert(QStringLiteral("vehicleType"), static_cast<int>(_vehicleType));
    for (auto field = _autopilotVersionObservationFields.constBegin(); field != _autopilotVersionObservationFields.constEnd(); ++field) {
        result.insert(field.key(), field.value());
    }
    const qint64 now = _autopilotVersionObservationClock.isValid() ? _autopilotVersionObservationClock.elapsed() : -1;
    const qint64 age = _autopilotVersionObservationReceivedAtMs >= 0 && now >= _autopilotVersionObservationReceivedAtMs
        ? now - _autopilotVersionObservationReceivedAtMs : -1;
    const bool available = maximumAgeMs >= 1 && maximumAgeMs <= 120000 && _autopilotVersionObservationObserved
        && _autopilotVersionObservationComplete && !_autopilotVersionObservationUpdating
        && _autopilotVersionObservationReceiveDepth == 0 && _startupOdometryReceiveDepth == 0
        && _initialConnectStateMachine && isInitialConnectComplete()
        && _autopilotVersionObservationFields.value(QStringLiteral("vehicleId")).toInt() == _id
        && _autopilotVersionObservationFields.value(QStringLiteral("componentId")).toInt() == _defaultComponentId
        && _autopilotVersionObservationFields.value(QStringLiteral("firmwareType")).toInt() == static_cast<int>(_firmwareType)
        && _autopilotVersionObservationFields.value(QStringLiteral("vehicleType")).toInt() == static_cast<int>(_vehicleType)
        && _autopilotVersionSourceAllowed(_autopilotVersionObservationLink.data());
    if (!self || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    result.insert(QStringLiteral("observed"), _autopilotVersionObservationObserved);
    result.insert(QStringLiteral("available"), available);
    result.insert(QStringLiteral("fresh"), available && age >= 0 && age <= maximumAgeMs);
    result.insert(QStringLiteral("ageMs"), age);
    result.insert(QStringLiteral("source"), _autopilotVersionObservationSource);
    result.insert(QStringLiteral("sourceEpoch"), _autopilotVersionObservationEpoch);
    result.insert(QStringLiteral("contentRevision"), QString::number(_autopilotVersionObservationRevision));
    result.insert(QStringLiteral("receiptSequence"), QString::number(_autopilotVersionObservationLastReceipt));
    result.insert(QStringLiteral("contentHash"), _autopilotVersionObservationContentHash);
    return result;
}

bool Vehicle::autopilotVersionObservationSourceMatches(LinkInterface* expectedLink) const
{
    if (!_autopilotVersionSourceAllowed(expectedLink) || _autopilotVersionObservationLink.data() != expectedLink
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete
        || _autopilotVersionObservationUpdating || _autopilotVersionObservationReceiveDepth != 0 || _startupOdometryReceiveDepth != 0
        || !_initialConnectStateMachine || !isInitialConnectComplete()
        || _autopilotVersionObservationLastReceipt == 0) return false;
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    return primary && primary.get() == expectedLink;
}

bool Vehicle::_fleetEvidenceReadAllowed(LinkInterface* expectedLink, bool ownVersionCommand, QString* reason) const
{
    const auto reject = [reason](const QString& value) { if (reason) *reason = value; return false; };
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    if (!_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
        || !_messageIntervalSourcePlugin || _messageIntervalSourcePlugin.data() != _firmwarePlugin)
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    const bool sourceAllowed = _autopilotVersionSourceAllowed(expectedLink);
    if (!self || !sourceAllowed) return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    const QPointer<ParameterManager> parameters(_parameterManager);
    const bool parametersAllowed = parameters && parameters->_parameterObservationContextAllowed();
    if (!self || !parameters || _parameterManager != parameters || !parametersAllowed)
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    if (!_mavlink || !_firmwarePlugin
        || !isInitialConnectComplete() || !px4Firmware() || !multiRotor() || armed() || flying()
        || _autopilotVersionObservationReceiveDepth != 0 || _flightStateObservationReceiveDepth != 0 || _startupOdometryReceiveDepth != 0
        || _flightStateObservationExhausted || _startupOdometryExhausted
        || !_parameterManager || _parameterManager->thread() != thread()
        || _parameterManager->_parameterObservationPendingWrites())
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != expectedLink
        || (_flightStateObservationLink && _flightStateObservationLink.data() != expectedLink))
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    if (!_missionManager || !_geoFenceManager || !_rallyPointManager
        || _missionManager->inProgress() || _missionManager->transportCallInProgress()
        || _geoFenceManager->inProgress() || _geoFenceManager->transportCallInProgress()
        || _rallyPointManager->inProgress() || _rallyPointManager->transportCallInProgress()
        || !_ftpManager || !_ftpManager->_rgStateMachine.isEmpty() || _ftpManager->_ackOrNakTimeoutTimer.isActive())
        return reject(QStringLiteral("fleet-evidence-transfer-busy"));
    const bool ownOnly = ownVersionCommand && _mavCommandList.size() == 1
        && _mavCommandList.first().fleetEvidenceReadOnly
        && _mavCommandList.first().fleetEvidenceLink.data() == expectedLink
        && _mavCommandList.first().fleetEvidenceEpoch == _autopilotVersionObservationEpoch;
    if ((!_mavCommandList.isEmpty() && !ownOnly) || _waitForMavlinkMessageResultHandler || _waitForMavlinkMessageId != 0)
        return reject(QStringLiteral("fleet-evidence-request-busy"));
    for (auto row = _parameterManager->_waitingReadParamIndexMap.constBegin();
         row != _parameterManager->_waitingReadParamIndexMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-evidence-request-busy"));
    for (auto row = _parameterManager->_waitingReadParamNameMap.constBegin();
         row != _parameterManager->_waitingReadParamNameMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-evidence-request-busy"));
    if (!_flightStateObservationLink) return reject(QStringLiteral("fleet-evidence-status-wait"));
    const auto heartbeatRow = _flightStateObservations.constFind(QStringLiteral("heartbeat"));
    const auto landedRow = _flightStateObservations.constFind(QStringLiteral("landedState"));
    if ((heartbeatRow != _flightStateObservations.constEnd() && heartbeatRow->observed && heartbeatRow->complete
            && heartbeatRow->value.value(QStringLiteral("armed")).toBool())
        || (landedRow != _flightStateObservations.constEnd() && landedRow->observed && landedRow->complete
            && landedRow->value.value(QStringLiteral("landedStateKnown")).toBool()
            && landedRow->value.value(QStringLiteral("landedState")).toUInt() != MAV_LANDED_STATE_ON_GROUND))
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
        const auto row = _flightStateObservations.constFind(channel);
        if (row == _flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
            || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > 5000)
            return reject(QStringLiteral("fleet-evidence-status-wait"));
    }
    const QVariantMap heartbeat = _flightStateObservations.value(QStringLiteral("heartbeat")).value;
    const QVariantMap landed = _flightStateObservations.value(QStringLiteral("landedState")).value;
    if (landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
        && !landed.value(QStringLiteral("landedStateKnown")).toBool())
        return reject(QStringLiteral("fleet-evidence-status-wait"));
    if (heartbeat.value(QStringLiteral("armed")).type() != QVariant::Bool
        || heartbeat.value(QStringLiteral("armed")).toBool()
        || landed.value(QStringLiteral("landedState")).toUInt() != MAV_LANDED_STATE_ON_GROUND)
        return reject(QStringLiteral("fleet-evidence-context-not-ready"));
    return true;
}

bool Vehicle::_sendFleetEvidenceReadMessage(LinkInterface* expectedLink, mavlink_message_t message,
                                          bool ownVersionCommand, std::function<bool()> authorizationFence,
                                          bool* requestIssued)
{
    if (requestIssued) *requestIssued = false;
    QPointer<Vehicle> self(this);
    QPointer<LinkInterface> link(expectedLink);
    const bool initiallyAllowed = _fleetEvidenceReadAllowed(expectedLink, ownVersionCommand);
    if (!self || !link || !initiallyAllowed) return false;
    QPointer<ParameterManager> manager(_parameterManager);
    const QPointer<FirmwarePlugin> plugin(_messageIntervalSourcePlugin);
    const QString managerEpoch = manager ? manager->_parameterObservationEpoch : QString();
    const SharedLinkInterfacePtr retained = _vehicleLinkManager->primaryLink().lock();
    const QString epoch = _autopilotVersionObservationEpoch;
    const QString versionRequestId = ownVersionCommand && !_mavCommandList.isEmpty()
        ? _mavCommandList.first().fleetEvidenceRequestId : QString();
    const mavlink_message_t original = message;
    const auto current = [&]() {
        if (!self || !link || !manager || !plugin) return false;
        const bool allowed = self->_fleetEvidenceReadAllowed(link.data(), ownVersionCommand);
        if (!self || !link || !manager || !plugin || !allowed) return false;
        return _parameterManager == manager.data() && plugin.data() == _firmwarePlugin
            && manager->_parameterObservationEpoch == managerEpoch && retained && retained.get() == link.data()
            && epoch == _autopilotVersionObservationEpoch
            && (!ownVersionCommand || (!_mavCommandList.isEmpty()
                && _mavCommandList.first().fleetEvidenceRequestId == versionRequestId))
            && (!ownVersionCommand || _messageIntervalSourceCurrent(_mavCommandList.first()))
            && message.msgid == original.msgid && message.len == original.len
            && message.sysid == original.sysid && message.compid == original.compid
            && memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) == 0;
    };
    // This actual native closure owns service generation, actor/role and
    // original deadlines. A JSON/UI bool or an empty callback cannot authorize.
    if (!authorizationFence || !authorizationFence() || !current()) return false;
    plugin->adjustOutgoingMavlinkMessageThreadSafe(this, expectedLink, &message);
    // Both plugin and fence callbacks may cancel, log out, destroy owners or
    // replace the source. Revalidate physical context AFTER every callback.
    if (!self || !authorizationFence() || !current()) return false;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN) return false;
    if (!authorizationFence() || !current()) return false;
    if (ownVersionCommand) {
        const QString laneId = _mavCommandList.first().messageIntervalRequestId;
        const int index = _messageCommandIndex(laneId);
        if (index < 0) return false;
        _mavCommandList[index].messageCommandWriteStarted = true;
    }
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    if (requestIssued) *requestIssued = true;
    if (!self) return true; // Queuing the packet does not promise a response.
    if (ownVersionCommand && !_messageCommandLaneId.isEmpty()
        && !_mavCommandList.isEmpty() && _mavCommandList.first().fleetEvidenceRequestId == versionRequestId)
        _fleetVersionReadLastIssuedId = versionRequestId;
    ++_messagesSent;
    emit messagesSentChanged();
    return true;
}

bool Vehicle::_fleetMissionReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
                                      const QString& readScopeId, QString* reason) const
{
    const auto reject = [reason](const QString& value) { if (reason) *reason = value; return false; };
    if (!manager || manager != _missionManager || manager->thread() != thread()
        || !_autopilotVersionSourceAllowed(expectedLink) || expectedLink->isMockLink()
        || !_mavlink || !_firmwarePlugin || !px4Firmware() || !multiRotor()
        || _firmwarePlugin->sendHomePositionToVehicle() || !isInitialConnectComplete() || armed() || flying()
        || _autopilotVersionObservationReceiveDepth != 0 || _flightStateObservationReceiveDepth != 0 || _startupOdometryReceiveDepth != 0
        || _flightStateObservationExhausted || _startupOdometryExhausted || _fleetMissionObservationReceiveExhausted
        || !_parameterManager || _parameterManager->thread() != thread()
        || !_parameterManager->_parameterObservationContextAllowed()
        || _parameterManager->_parameterObservationPendingWrites())
        return reject(QStringLiteral("fleet-mission-read-context-not-ready"));
    // A mission frame already on the receive stack must never be adopted by
    // a NEW read started from link/plugin/notification reentry. Existing owned
    // read replies and their scoped next-item/end-ACK sends remain permitted.
    if (readScopeId.isEmpty() && _fleetMissionObservationReceiveDepth != 0)
        return reject(QStringLiteral("fleet-mission-read-receive-stack-busy"));
#ifdef QT_DEBUG
    const SharedLinkConfigurationPtr configuration = expectedLink->linkConfiguration();
    if (!configuration || configuration->type() == LinkConfiguration::TypeMock)
        return reject(QStringLiteral("fleet-mission-read-mock-link-not-allowed"));
#endif
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != expectedLink || primary->isMockLink()
        || _flightStateObservationLink.data() != expectedLink)
        return reject(QStringLiteral("fleet-mission-read-primary-source-not-current"));
    const bool own = !readScopeId.isEmpty() && manager->_fleetReadActive && !manager->_fleetReadCancelled
        && manager->_fleetReadScopeId == readScopeId && manager->_fleetReadVehicle.data() == this
        && manager->_fleetReadLink.data() == expectedLink && manager->_planType == MAV_MISSION_TYPE_MISSION
        && ((manager->_fleetReadOperationId == 0 && manager->_operationEntryInProgress
            && manager->_transactionInProgress == PlanManager::TransactionNone)
            || (manager->_fleetReadOperationId != 0 && manager->_activeOperationId == manager->_fleetReadOperationId
                && manager->_transactionInProgress == PlanManager::TransactionRead));
    if ((!readScopeId.isEmpty() && !own) || (readScopeId.isEmpty()
        && (manager->inProgress() || manager->transportCallInProgress())))
        return reject(QStringLiteral("fleet-mission-read-owner-or-operation-busy"));
    if (!_geoFenceManager || !_rallyPointManager || _geoFenceManager->inProgress()
        || _geoFenceManager->transportCallInProgress() || _rallyPointManager->inProgress()
        || _rallyPointManager->transportCallInProgress() || !_ftpManager
        || !_ftpManager->_rgStateMachine.isEmpty() || _ftpManager->_ackOrNakTimeoutTimer.isActive()
        || !_mavCommandList.isEmpty() || _waitForMavlinkMessageResultHandler || _waitForMavlinkMessageId != 0)
        return reject(QStringLiteral("fleet-mission-read-foreign-transfer-busy"));
    for (auto row = _parameterManager->_waitingReadParamIndexMap.constBegin();
         row != _parameterManager->_waitingReadParamIndexMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-mission-read-foreign-transfer-busy"));
    for (auto row = _parameterManager->_waitingReadParamNameMap.constBegin();
         row != _parameterManager->_waitingReadParamNameMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-mission-read-foreign-transfer-busy"));
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
        const auto row = _flightStateObservations.constFind(channel);
        if (row == _flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
            || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > 5000)
            return reject(QStringLiteral("fleet-mission-read-ground-state-not-fresh"));
    }
    const QVariantMap heartbeat = _flightStateObservations.value(QStringLiteral("heartbeat")).value;
    const QVariantMap landed = _flightStateObservations.value(QStringLiteral("landedState")).value;
    if (heartbeat.value(QStringLiteral("armed")).type() != QVariant::Bool || heartbeat.value(QStringLiteral("armed")).toBool()
        || landed.value(QStringLiteral("landedStateKnown")).type() != QVariant::Bool
        || !landed.value(QStringLiteral("landedStateKnown")).toBool()
        || landed.value(QStringLiteral("landedState")).toUInt() != MAV_LANDED_STATE_ON_GROUND)
        return reject(QStringLiteral("fleet-mission-read-ground-state-not-fresh"));
    return true;
}

bool Vehicle::_fleetMissionDiagnosticReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
    const QString& readScopeId, QString* reason) const
{
    // Reuse the original disarmed/on-ground/real-source/foreign-transfer
    // policy; diagnostic scope never adopts armed-review admission.
    if (!_fleetMissionReadAllowed(manager, expectedLink, readScopeId, reason)) return false;
    const auto reject = [reason](const QString& value) { if (reason) *reason = value; return false; };
    if (_fleetStartupEntryBusy || _fleetStartupActive)
        return reject(QStringLiteral("fleet-diagnostic-read-startup-busy"));
    if (readScopeId.isEmpty()) return true; // Entry or completed-result source check.
    if (manager->_fleetReadAdmission != PlanManager::FleetReadAdmission::DiagnosticDisarmed
        || manager->_fleetReadReviewOwner || !manager->_fleetReadDiagnosticOwner
        || manager->_fleetReadDiagnosticOwner->thread() != thread())
        return reject(QStringLiteral("fleet-diagnostic-read-owner-not-current"));
    const WeakLinkInterfacePtr primary = _vehicleLinkManager->primaryLink();
    if (primary.expired() || manager->_fleetReadDiagnosticPrimary.expired()
        || primary.owner_before(manager->_fleetReadDiagnosticPrimary)
        || manager->_fleetReadDiagnosticPrimary.owner_before(primary))
        return reject(QStringLiteral("fleet-diagnostic-read-primary-owner-changed"));
    return true;
}

bool Vehicle::fleetArmedGroundReviewSourceMatches(LinkInterface* expectedPrimary) const
{
    // Pure native source/phase evidence. It does not invoke a role/authentication
    // callback, accept caller flight-state flags, or permit a command.
    if (!_autopilotVersionSourceAllowed(expectedPrimary) || expectedPrimary->thread() != thread()
        || expectedPrimary->isMockLink() || !_firmwarePlugin || !_mavlink
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete() || flying()
        || _firmwarePlugin->sendHomePositionToVehicle()
        || _autopilotVersionObservationReceiveDepth != 0 || !_autopilotVersionObservationObserved
        || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || _autopilotVersionObservationLink.data() != expectedPrimary
        || _flightStateObservationReceiveDepth != 0 || _flightStateObservationExhausted || _startupOdometryExhausted
        || _startupCoordinateReceiveDepth != 0 || _startupOdometryReceiveDepth != 0 || _fleetStartupReceiveDepth != 0
        || _fleetMissionObservationReceiveExhausted || _flightStateObservationLink.data() != expectedPrimary) return false;
#ifdef QT_DEBUG
    const SharedLinkConfigurationPtr configuration = expectedPrimary->linkConfiguration();
    if (!configuration || configuration->type() == LinkConfiguration::TypeMock) return false;
#endif
    const SharedLinkInterfacePtr primary = _vehicleLinkManager->primaryLink().lock();
    if (!primary || primary.get() != expectedPrimary || primary->isMockLink()) return false;
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState"), QStringLiteral("globalPosition")}) {
        const auto row = _flightStateObservations.constFind(channel);
        const qint64 limit = channel == QStringLiteral("globalPosition") ? 3000 : 5000;
        if (row == _flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
            || row->receiptSequence == 0 || row->source != flightObservationSource(channel)
            || row->receivedAtMs < 0 || now < row->receivedAtMs || now - row->receivedAtMs > limit) return false;
    }
    const QVariantMap heartbeat = _flightStateObservations.value(QStringLiteral("heartbeat")).value;
    const QVariantMap landed = _flightStateObservations.value(QStringLiteral("landedState")).value;
    const uint base = heartbeat.value(QStringLiteral("baseMode")).toUInt();
    const uint custom = heartbeat.value(QStringLiteral("customMode")).toUInt();
    // PX4 custom-mode union: reserved bits 0..15, main 16..23, sub 24..31.
    // POSCTL/POSCTL is main=3/sub=0; ORBIT and AUTO are not this admission.
    return heartbeat.value(QStringLiteral("armed")).type() == QVariant::Bool
        && heartbeat.value(QStringLiteral("armed")).toBool() == armed()
        && base == baseMode() && custom == customMode()
        && (base & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) != 0
        && (base & (MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_HIL_ENABLED)) == 0
        && (custom & 0xffffU) == 0 && ((custom >> 16) & 0xffU) == 3 && ((custom >> 24) & 0xffU) == 0
        && landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
        && landed.value(QStringLiteral("landedStateKnown")).toBool()
        && landed.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND;
}

bool Vehicle::_fleetMissionArmedGroundReviewReadAllowed(PlanManager* manager, LinkInterface* expectedLink,
    const QString& readScopeId, bool expectedArmed, QString* reason) const
{
    const auto reject = [reason](const QString& value) { if (reason) *reason = value; return false; };
    if (!manager || manager != _missionManager || manager->thread() != thread()
        || !fleetArmedGroundReviewSourceMatches(expectedLink) || armed() != expectedArmed
        || !_parameterManager || _parameterManager->thread() != thread()
        || !_parameterManager->_parameterObservationContextAllowed() || _parameterManager->_parameterObservationPendingWrites())
        return reject(QStringLiteral("fleet-armed-ground-read-context-not-ready"));
    // Ongoing owned mission replies may send their next INT request. A NEW
    // admission cannot adopt a packet already on the original receive stack.
    if (readScopeId.isEmpty() && _fleetMissionObservationReceiveDepth != 0)
        return reject(QStringLiteral("fleet-armed-ground-read-receive-stack-busy"));
    const bool own = !readScopeId.isEmpty() && manager->_fleetReadActive && !manager->_fleetReadCancelled
        && manager->_fleetReadScopeId == readScopeId && manager->_fleetReadVehicle.data() == this
        && manager->_fleetReadLink.data() == expectedLink && manager->_planType == MAV_MISSION_TYPE_MISSION
        && (manager->_fleetReadAdmission == PlanManager::FleetReadAdmission::ReviewDisarmedPOSCTL
            || manager->_fleetReadAdmission == PlanManager::FleetReadAdmission::ReviewArmedPOSCTL)
        && manager->_fleetReadReviewOwner && manager->_fleetReadReviewOwner->thread() == thread()
        && manager->_fleetReadReviewOwner->reviewEpoch() == manager->_fleetReadReviewEpoch
        && manager->_fleetReadFlightEpoch == _flightStateObservationEpoch
        && (manager->_fleetReadAdmission == PlanManager::FleetReadAdmission::ReviewArmedPOSCTL) == expectedArmed
        && ((manager->_fleetReadOperationId == 0 && manager->_operationEntryInProgress
                && manager->_transactionInProgress == PlanManager::TransactionNone)
            || (manager->_fleetReadOperationId != 0 && manager->_activeOperationId == manager->_fleetReadOperationId
                && manager->_transactionInProgress == PlanManager::TransactionRead));
    if ((!readScopeId.isEmpty() && !own) || (readScopeId.isEmpty()
        && (manager->inProgress() || manager->transportCallInProgress())))
        return reject(QStringLiteral("fleet-armed-ground-read-owner-or-operation-busy"));
    if (!_geoFenceManager || !_rallyPointManager || _geoFenceManager->inProgress()
        || _geoFenceManager->transportCallInProgress() || _rallyPointManager->inProgress()
        || _rallyPointManager->transportCallInProgress() || !_ftpManager
        || !_ftpManager->_rgStateMachine.isEmpty() || _ftpManager->_ackOrNakTimeoutTimer.isActive()
        || !_mavCommandList.isEmpty() || _waitForMavlinkMessageResultHandler || _waitForMavlinkMessageId != 0
        || _fleetStartupEntryBusy || _fleetStartupActive)
        return reject(QStringLiteral("fleet-armed-ground-read-foreign-transfer-busy"));
    for (auto row = _parameterManager->_waitingReadParamIndexMap.constBegin();
         row != _parameterManager->_waitingReadParamIndexMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-armed-ground-read-foreign-transfer-busy"));
    for (auto row = _parameterManager->_waitingReadParamNameMap.constBegin();
         row != _parameterManager->_waitingReadParamNameMap.constEnd(); ++row)
        if (!row.value().isEmpty()) return reject(QStringLiteral("fleet-armed-ground-read-foreign-transfer-busy"));
    return true;
}

bool Vehicle::_sendFleetMissionReadMessage(PlanManager* requestedManager, LinkInterface* expectedLink,
                                          const QString& requestedScopeId, mavlink_message_t message,
                                          std::function<bool()> authorizationFence, bool* requestIssued)
{
    if (requestIssued) *requestIssued = false;
    const QString scopeId = requestedScopeId;
    const QPointer<Vehicle> self(this);
    const QPointer<PlanManager> manager(requestedManager);
    const QPointer<LinkInterface> link(expectedLink);
    if (!manager || !authorizationFence) return false;
    const PlanManager::FleetReadAdmission admission = manager->_fleetReadAdmission;
    const QPointer<FleetMissionArmedGroundReview> review(manager->_fleetReadReviewOwner);
    const QPointer<FleetMissionReconciliation> diagnostic(manager->_fleetReadDiagnosticOwner);
    const QString reviewEpoch = manager->_fleetReadReviewEpoch;
    const QString flightEpoch = manager->_fleetReadFlightEpoch;
    const auto readAllowed = [&]() {
        return self && manager && link && manager->_fleetReadAdmission == admission
            && manager->_fleetReadReviewOwner == review && manager->_fleetReadReviewEpoch == reviewEpoch
            && manager->_fleetReadDiagnosticOwner == diagnostic
            && manager->_fleetReadFlightEpoch == flightEpoch
            && (admission == PlanManager::FleetReadAdmission::DiagnosticDisarmed
                ? diagnostic && !review && _fleetMissionDiagnosticReadAllowed(manager.data(), link.data(), scopeId)
                : admission == PlanManager::FleetReadAdmission::StartDisarmed
                ? !review && !diagnostic && _fleetMissionReadAllowed(manager.data(), link.data(), scopeId)
                : (admission == PlanManager::FleetReadAdmission::ReviewDisarmedPOSCTL
                    || admission == PlanManager::FleetReadAdmission::ReviewArmedPOSCTL)
                    && !diagnostic && review && review->reviewEpoch() == reviewEpoch
                    && _fleetMissionArmedGroundReviewReadAllowed(manager.data(), link.data(), scopeId,
                        admission == PlanManager::FleetReadAdmission::ReviewArmedPOSCTL));
    };
    if (!readAllowed()) return false;
    const SharedLinkInterfacePtr retained = _vehicleLinkManager->primaryLink().lock();
    const QString epoch = _autopilotVersionObservationEpoch;
    const quint64 operationId = manager->_fleetReadOperationId;
    const QPointer<ParameterManager> parameters(_parameterManager);
    const QString parameterEpoch = parameters ? parameters->_parameterObservationEpoch : QString();
    const mavlink_message_t original = message;
    // Only the three bounded read protocol messages are permitted, never
    // MISSION_COUNT/ITEM/CLEAR/SET_CURRENT, a command, or a caller-defined packet.
    if (message.msgid != MAVLINK_MSG_ID_MISSION_REQUEST_LIST
        && message.msgid != MAVLINK_MSG_ID_MISSION_REQUEST_INT && message.msgid != MAVLINK_MSG_ID_MISSION_ACK) return false;
    if (operationId == 0 || message.sysid != _mavlink->getSystemId() || message.compid != _mavlink->getComponentId()) return false;
    if (message.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_LIST) {
        mavlink_mission_request_list_t request;
        mavlink_msg_mission_request_list_decode(&message, &request);
        if (request.target_system != _id || request.target_component != _defaultComponentId
            || request.mission_type != MAV_MISSION_TYPE_MISSION || manager->_expectedAck != PlanManager::AckMissionCount) return false;
    } else if (message.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT) {
        mavlink_mission_request_int_t request;
        mavlink_msg_mission_request_int_decode(&message, &request);
        if (request.target_system != _id || request.target_component != _defaultComponentId
            || request.mission_type != MAV_MISSION_TYPE_MISSION || manager->_expectedAck != PlanManager::AckMissionItem
            || manager->_itemIndicesToRead.isEmpty() || request.seq != manager->_itemIndicesToRead.first()) return false;
    } else {
        mavlink_mission_ack_t ack;
        mavlink_msg_mission_ack_decode(&message, &ack);
        if (ack.target_system != _id || ack.target_component != _defaultComponentId
            || ack.mission_type != MAV_MISSION_TYPE_MISSION || ack.type != MAV_MISSION_ACCEPTED
            || !manager->_itemIndicesToRead.isEmpty()
            || manager->_fleetReadExpectedCount < (admission == PlanManager::FleetReadAdmission::DiagnosticDisarmed ? 0 : 1)
            || manager->_fleetReadPendingItems.size() != manager->_fleetReadExpectedCount) return false;
    }
    const PlanManager::AckType_t expectedAck = manager->_expectedAck;
    const auto current = [&]() {
        return self && manager && link && parameters && _parameterManager == parameters.data()
            && parameters->_parameterObservationEpoch == parameterEpoch && retained && retained.get() == link.data()
            && epoch == _autopilotVersionObservationEpoch && manager->_fleetReadOperationId == operationId
            && manager->_expectedAck == expectedAck
            && manager->_fleetReadScopeId == scopeId && readAllowed()
            && message.msgid == original.msgid && message.len == original.len
            && message.sysid == original.sysid && message.compid == original.compid
            && message.magic == original.magic && message.incompat_flags == original.incompat_flags
            && message.compat_flags == original.compat_flags
            && memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) == 0;
    };
    if (!current() || !authorizationFence() || !current()) return false;
    _firmwarePlugin->adjustOutgoingMavlinkMessageThreadSafe(this, link.data(), &message);
    if (!self || !manager || !link || !authorizationFence() || !current()) return false;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN) return false;
    if (!authorizationFence() || !current()) return false;
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    if (requestIssued) *requestIssued = true;
    if (!self) return true;
    ++_messagesSent;
    emit messagesSentChanged();
    return true;
}

bool Vehicle::_sendFleetDeploymentMessage(PlanManager* actualManager, LinkInterface* expectedPrimary,
    mavlink_message_t message, std::function<bool()> authorizationFence, bool* requestIssued)
{
    if (requestIssued) *requestIssued = false;
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _fleetDeploymentSendBusy || !authorizationFence || !actualManager || !expectedPrimary) return false;
    const QPointer<Vehicle> self(this);
    const QPointer<PlanManager> manager(actualManager);
    const QPointer<LinkInterface> link(expectedPrimary);
    const QPointer<FleetMissionDeployment> owner(manager->_fleetDeploymentOwner);
    const QPointer<VehicleLinkManager> links(_vehicleLinkManager);
    const QPointer<ParameterManager> parameters(_parameterManager);
    const QPointer<FirmwarePlugin> plugin(_firmwarePlugin);
    const QPointer<MAVLinkProtocol> protocol(_mavlink);
    const QPointer<InitialConnectStateMachine> initialConnect(_initialConnectStateMachine);
    const QPointer<MultiVehicleManager> vehicles(qgcApp()->toolbox() ? qgcApp()->toolbox()->multiVehicleManager() : nullptr);
    if (!manager || manager->thread() != thread() || manager.data() != _missionManager
        || !owner || !links || !parameters || !plugin || !protocol || !initialConnect || !vehicles
        || !manager->_fleetDeploymentScoped || manager->_fleetDeploymentClosed || !manager->_fleetDeploymentSendInProgress
        || !manager->_fleetDeploymentContextCurrent()) return false;

    const WeakLinkInterfacePtr weak = manager->_fleetDeploymentWeakLink;
    const SharedLinkInterfacePtr retained = weak.lock();
    if (!retained || retained.get() != link.data() || !link->mavlinkChannelIsSet()) return false;
    const uint8_t channel = link->mavlinkChannel();
    const QString deploymentId = manager->_fleetDeploymentId;
    const quint64 generation = manager->_fleetDeploymentGeneration;
    const quint64 operationId = manager->_fleetDeploymentOperationId;
    const PlanManager::TransactionType_t transaction = manager->_fleetDeploymentTransaction;
    const PlanManager::AckType_t expectedAck = manager->_expectedAck;
    const int vehicleId = _id, componentId = _defaultComponentId;
    const MAV_AUTOPILOT firmware = _firmwareType;
    const MAV_TYPE vehicleType = _vehicleType;
    const bool capabilityBitsKnown = _capabilityBitsKnown;
    const uint64_t capabilityBits = _capabilityBits;
    const QString versionEpoch = _autopilotVersionObservationEpoch;
    const QString versionHash = _autopilotVersionObservationContentHash;
    const quint64 versionRevision = _autopilotVersionObservationRevision;
    const QString flightEpoch = _flightStateObservationEpoch;
    const QString parameterEpoch = parameters->_parameterObservationEpoch;
    const int systemId = protocol->getSystemId(), sourceComponentId = protocol->getComponentId();
    const QList<int> readIndices = manager->_itemIndicesToRead, writeIndices = manager->_itemIndicesToWrite;
    const QList<MissionItem*> readItems = manager->_missionItems, writeItems = manager->_writeMissionItems;
    const int countToRead = manager->_missionItemCountToRead, lastRequest = manager->_lastMissionRequest;
    const mavlink_message_t original = message;
    QPointer<MissionItem> selectedItem;
    if (operationId == 0 || generation == 0 || deploymentId.isEmpty()
        || message.sysid != systemId || message.compid != sourceComponentId
        || systemId < 1 || systemId > 255 || sourceComponentId < 1 || sourceComponentId > 255
        || (message.magic != MAVLINK_STX && message.magic != MAVLINK_STX_MAVLINK1)) return false;
    // These are ordinary MISSION protocol packets for this actual operation,
    // not RAW read receipts, a caller-defined message, or a StartPermit.
    const auto lengthAllowed = [&](int minimum, int maximum) {
        return original.len > 0 && original.len <= maximum
            && (original.magic != MAVLINK_STX_MAVLINK1 || original.len == minimum);
    };
    if (transaction == PlanManager::TransactionWrite) {
        if (!manager->_activeWriteWasFreshUpload || manager->_resumeMission || writeItems.isEmpty()
            || writeItems.size() > std::numeric_limits<quint16>::max() || expectedAck != PlanManager::AckMissionRequest) return false;
        if (original.msgid == MAVLINK_MSG_ID_MISSION_COUNT) {
            mavlink_mission_count_t count;
            mavlink_msg_mission_count_decode(&original, &count);
            if (!lengthAllowed(MAVLINK_MSG_ID_MISSION_COUNT_MIN_LEN, MAVLINK_MSG_ID_MISSION_COUNT_LEN)
                || count.target_system != vehicleId || count.target_component != componentId
                || count.mission_type != MAV_MISSION_TYPE_MISSION || count.count != writeItems.size()
                || count.count == 0 || lastRequest != -1) return false;
        } else if (original.msgid == MAVLINK_MSG_ID_MISSION_ITEM_INT) {
            mavlink_mission_item_int_t item;
            mavlink_msg_mission_item_int_decode(&original, &item);
            if (!lengthAllowed(MAVLINK_MSG_ID_MISSION_ITEM_INT_MIN_LEN, MAVLINK_MSG_ID_MISSION_ITEM_INT_LEN)
                || item.target_system != vehicleId || item.target_component != componentId
                || item.mission_type != MAV_MISSION_TYPE_MISSION || item.seq >= writeItems.size()
                || item.seq != lastRequest || item.current != (item.seq == 0 ? 1 : 0) || item.autocontinue > 1) return false;
            selectedItem = writeItems.at(item.seq);
            if (!selectedItem || selectedItem->thread() != thread()) return false;
        } else return false; // No CLEAR_ALL, SET_CURRENT, guided ITEM, or command.
    } else if (transaction == PlanManager::TransactionRead) {
        if (manager->_resumeMission || manager->_activeWriteWasFreshUpload) return false;
        if (original.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_LIST) {
            mavlink_mission_request_list_t request;
            mavlink_msg_mission_request_list_decode(&original, &request);
            if (!lengthAllowed(MAVLINK_MSG_ID_MISSION_REQUEST_LIST_MIN_LEN, MAVLINK_MSG_ID_MISSION_REQUEST_LIST_LEN)
                || request.target_system != vehicleId || request.target_component != componentId
                || request.mission_type != MAV_MISSION_TYPE_MISSION || expectedAck != PlanManager::AckMissionCount
                || countToRead != -1 || !readIndices.isEmpty() || !readItems.isEmpty()) return false;
        } else if (original.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT || original.msgid == MAVLINK_MSG_ID_MISSION_REQUEST) {
            // REQUEST and REQUEST_INT have the same native payload layout.
            mavlink_mission_request_int_t request;
            mavlink_msg_mission_request_int_decode(&original, &request);
            if (!lengthAllowed(MAVLINK_MSG_ID_MISSION_REQUEST_INT_MIN_LEN, MAVLINK_MSG_ID_MISSION_REQUEST_INT_LEN)
                || request.target_system != vehicleId || request.target_component != componentId
                || request.mission_type != MAV_MISSION_TYPE_MISSION || expectedAck != PlanManager::AckMissionItem
                || countToRead < 1 || countToRead > std::numeric_limits<quint16>::max()
                || readIndices.isEmpty() || request.seq != readIndices.first() || request.seq >= countToRead
                || readItems.size() >= countToRead) return false;
        } else if (original.msgid == MAVLINK_MSG_ID_MISSION_ACK) {
            mavlink_mission_ack_t ack;
            mavlink_msg_mission_ack_decode(&original, &ack);
            // A real COUNT(0) is a valid empty read. The unknown sentinel -1
            // must never be silently promoted to a successful empty catalog.
            if (!lengthAllowed(MAVLINK_MSG_ID_MISSION_ACK_MIN_LEN, MAVLINK_MSG_ID_MISSION_ACK_LEN)
                || ack.target_system != vehicleId || ack.target_component != componentId
                || ack.mission_type != MAV_MISSION_TYPE_MISSION || ack.type != MAV_MISSION_ACCEPTED
                || expectedAck != PlanManager::AckNone || countToRead < 0
                || countToRead > std::numeric_limits<quint16>::max() || !readIndices.isEmpty()
                || readItems.size() != countToRead) return false;
        } else return false;
    } else return false;

    _fleetDeploymentSendBusy = true;
    const auto releasing = qScopeGuard([self]() { if (self) self->_fleetDeploymentSendBusy = false; });
    const auto current = [&]() {
        // Pure native post-callback fence. Link affinity is deliberately not
        // constrained: serial/UDP Link implementations may live on workers.
        if (!self || QThread::currentThread() != self->thread() || !qgcApp() || self->thread() != qgcApp()->thread()
            || !manager || !owner || !links || !link || !parameters || !plugin || !protocol || !initialConnect || !vehicles
            || manager->thread() != self->thread() || owner->thread() != self->thread() || links->thread() != self->thread()
            || parameters->thread() != self->thread() || plugin->thread() != self->thread() || protocol->thread() != self->thread()
            || initialConnect->thread() != self->thread() || vehicles->thread() != self->thread()
            || !_fleetDeploymentSendBusy || _missionManager != manager.data() || manager->_vehicle != self.data()
            || _vehicleLinkManager != links.data() || _parameterManager != parameters.data() || _firmwarePlugin != plugin.data()
            || _mavlink != protocol.data() || _initialConnectStateMachine != initialConnect.data() || !qgcApp()->toolbox()
            || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data() || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
            || vehicles->getVehicleById(vehicleId) != self.data() || _id != vehicleId || _defaultComponentId != componentId
            || vehicleId < 1 || vehicleId > 255 || componentId < 1 || componentId > 255
            || _firmwareType != firmware || firmware != MAV_AUTOPILOT_PX4 || _vehicleType != vehicleType || !multiRotor()
            || _capabilityBitsKnown != capabilityBitsKnown || !capabilityBitsKnown || _capabilityBits != capabilityBits
            || !(capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_INT)
            || _offlineEditingVehicle || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted
            || _armed || _flying || initialConnect->active() || links->communicationLost() || !links->containsLink(link.data())
            || _autopilotVersionObservationEpoch != versionEpoch || _autopilotVersionObservationContentHash != versionHash
            || _autopilotVersionObservationRevision != versionRevision || _flightStateObservationEpoch != flightEpoch
            || parameters->_vehicle != self.data() || parameters->_parameterObservationEpoch != parameterEpoch
            || parameters->_parameterObservationExhausted || !parameters->_parameterObservationClock.isValid()
            || !QElapsedTimer::isMonotonic() || parameters->_logReplay || !parameters->_initialLoadComplete
            || !parameters->_parametersReady || parameters->_missingParameters || parameters->_initialRequestTimeoutTimer.isActive()
            || parameters->_parameterObservationPendingWrites() || protocol->getSystemId() != systemId
            || protocol->getComponentId() != sourceComponentId || !link->mavlinkChannelIsSet() || link->mavlinkChannel() != channel
            || !manager->_fleetDeploymentScoped || manager->_fleetDeploymentClosed || !manager->_fleetDeploymentSendInProgress
            || manager->_fleetDeploymentOwner != owner || manager->_fleetDeploymentVehicle != self
            || manager->_fleetDeploymentLink != link || manager->_fleetDeploymentId != deploymentId
            || manager->_fleetDeploymentGeneration != generation || manager->_fleetDeploymentOperationId != operationId
            || manager->_fleetDeploymentTransaction != transaction || manager->_transactionInProgress != transaction
            || manager->_activeOperationId != operationId || manager->_nextOperationId != operationId
            || manager->_planType != MAV_MISSION_TYPE_MISSION || manager->_fleetReadActive || manager->_resumeMission
            || manager->_activeWriteWasFreshUpload != (transaction == PlanManager::TransactionWrite)
            || manager->_expectedAck != expectedAck || manager->_missionItemCountToRead != countToRead
            || manager->_lastMissionRequest != lastRequest || manager->_itemIndicesToRead != readIndices
            || manager->_itemIndicesToWrite != writeIndices || manager->_missionItems != readItems || manager->_writeMissionItems != writeItems
            || (original.msgid == MAVLINK_MSG_ID_MISSION_ITEM_INT && !selectedItem)
            || message.msgid != original.msgid || message.len != original.len || message.sysid != original.sysid
            || message.compid != original.compid || message.magic != original.magic
            || message.incompat_flags != original.incompat_flags || message.compat_flags != original.compat_flags
            // Serialization neither recomputes nor re-signs a signed frame.
            // Preserve its signed sequence/checksum/signature without reading
            // the uninitialized signature storage of an unsigned packet.
            || (original.magic == MAVLINK_STX && (original.incompat_flags & MAVLINK_IFLAG_SIGNED)
                && (message.seq != original.seq || message.checksum != original.checksum
                    || memcmp(message.signature, original.signature, sizeof(original.signature)) != 0))
            || memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) != 0) return false;
        const WeakLinkInterfacePtr primary = links->primaryLink();
        if (primary.owner_before(weak) || weak.owner_before(primary)
            || manager->_fleetDeploymentWeakLink.owner_before(weak) || weak.owner_before(manager->_fleetDeploymentWeakLink)) return false;
        return retained && retained.get() == link.data() && !primary.expired() && manager->_fleetDeploymentContextCurrent();
    };
    const auto transportCurrent = [&]() {
        // These virtual/native adapter calls can reenter. Check the complete
        // frozen actual context after each, before touching its next object.
        return current() && link->isConnected() && current() && !link->isLogReplay() && current()
            && !link->isMockLink() && current() && !plugin->sendHomePositionToVehicle() && current();
    };
    const auto authorized = [&]() {
        // PlanManager supplies the single combined full closure for its
        // frozen scope; do not recursively call its full gate a second time.
        return transportCurrent() && authorizationFence() && transportCurrent();
    };
    if (!authorized()) return false;
    plugin->adjustOutgoingMavlinkMessageThreadSafe(self.data(), link.data(), &message);
    if (!authorized()) return false;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN || !authorized() || !current()) return false;
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    // This queue fact survives nested source/owner retirement or deletion.
    // Save it before the first public notification, and never turn it false
    // because a post-write source is no longer current.
    if (requestIssued) *requestIssued = true;
    if (!self) return true;
    ++_messagesSent;
    emit messagesSentChanged();
    return true;
}

void Vehicle::_fleetEvidenceCommandResult(void*, int, const mavlink_command_ack_t&, MavCmdResultFailureCode_t)
{
    // ACK is only queue disposal, never a version receipt or refresh success.
}

void Vehicle::_quarantineFleetEvidenceCommand(const MavCommandListEntry_t& entry)
{
    if (!entry.fleetEvidenceReadOnly) return;
    _purgeMavCommandQuarantine();
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    const qint64 horizon = qMax(1000, entry.ackTimeoutMSecs);
    const qint64 expires = now > std::numeric_limits<qint64>::max() - horizon
        ? std::numeric_limits<qint64>::max() : now + horizon;
    _mavCommandQuarantine.insert(_mavCommandQuarantineKey(entry.targetCompId, entry.command), expires);
}

QVariantMap Vehicle::requestFleetAutopilotVersion(LinkInterface* expectedLink, std::function<bool()> authorizationFence)
{
    const auto denied = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), false}, {QStringLiteral("reason"), reason}};
    };
    if (!authorizationFence) return denied(QStringLiteral("fleet-evidence-authorization-fence-missing"));
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> expected(expectedLink);
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    if ((_fleetVersionReadLastAtMs >= 0 && now - _fleetVersionReadLastAtMs < 1000)
        || _isMavCommandQuarantined(_defaultComponentId, MAV_CMD_REQUEST_MESSAGE))
        return denied(QStringLiteral("fleet-evidence-rate-limited"));
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString epoch = _autopilotVersionObservationEpoch;
    const QVariantMap request {{QStringLiteral("ok"), true}, {QStringLiteral("requestIssued"), true},
        {QStringLiteral("requestId"), requestId}, {QStringLiteral("vehicleId"), _id},
        {QStringLiteral("componentId"), _defaultComponentId}, {QStringLiteral("sourceEpoch"), epoch},
        {QStringLiteral("baselineReceiptSequence"), QString::number(_autopilotVersionObservationLastReceipt)}};
    MavCommandListEntry_t entry;
    entry.command = MAV_CMD_REQUEST_MESSAGE;
    entry.targetCompId = _defaultComponentId;
    entry.frame = MAV_FRAME_GLOBAL;
    entry.showError = false;
    entry.rgParam1 = MAVLINK_MSG_ID_AUTOPILOT_VERSION;
    entry.maxTries = 1;
    entry.ackTimeoutMSecs = _mavCommandAckTimeoutMSecs;
    entry.ackHandlerInfo = {};
    entry.ackHandlerInfo.resultHandler = &Vehicle::_fleetEvidenceCommandResult;
    entry.fleetEvidenceReadOnly = true;
    entry.fleetEvidenceLink = expectedLink;
    entry.fleetEvidenceEpoch = epoch;
    entry.fleetEvidenceRequestId = requestId;
    entry.fleetEvidenceAuthorizationFence = authorizationFence;
    if ((_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
        || !_messageCommandLaneId.isEmpty() || !_mavCommandList.isEmpty()
        || _waitForMavlinkMessageResultHandler || _requestMessageContext)
        return denied(QStringLiteral("fleet-evidence-request-busy"));
    if (!_reserveMessageCommand(&entry)) return denied(_messageCommandLaneExhausted
        ? QStringLiteral("fleet-evidence-message-lane-exhausted") : QStringLiteral("fleet-evidence-message-source-unavailable"));
    if (entry.messageIntervalPrimary != expected) {
        _releaseMessageCommand(entry.messageIntervalRequestId);
        return denied(QStringLiteral("fleet-evidence-source-changed"));
    }
    entry.messageCommandDeadlineMs = entry.messageCommandReservedAtMs + _mavCommandAckTimeoutMSecs;
    _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
    _mavCommandList.append(entry);
    // Reserve first, then preserve the original refresh consumer's busy /
    // status-wait reasons. No timer may send this unprepared entry while the
    // full strict gate is inside a callback.
    QString reason;
    const bool allowed = _fleetEvidenceReadAllowed(expected.data(), true, &reason);
    if (!self) return denied(QStringLiteral("fleet-evidence-sender-changed"));
    const bool sourceCurrent = _messageIntervalSourceCurrent(entry);
    const int pendingIndex = _messageCommandIndex(entry.messageIntervalRequestId);
    if (!allowed || !sourceCurrent || pendingIndex < 0) {
        if (pendingIndex >= 0) _mavCommandList.removeAt(pendingIndex);
        _releaseMessageCommand(entry.messageIntervalRequestId);
        return denied(!sourceCurrent || pendingIndex < 0 ? QStringLiteral("fleet-evidence-source-changed") : reason);
    }
    _mavCommandList[pendingIndex].messageCommandPrepared = true;
    _mavCommandList[pendingIndex].elapsedTimer.start();
    _fleetVersionReadLastAtMs = now;
    bool issued = false;
    _sendMessageCommandFromList(entry.messageIntervalRequestId, &issued);
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), issued},
        {QStringLiteral("reason"), QStringLiteral("fleet-evidence-sender-changed")}};
    // This local write fact cannot be lost to synchronous ACK disposal, a
    // replacement request's last-issued ID, or retirement inside write().
    if (!issued) return denied(QStringLiteral("fleet-evidence-sender-changed"));
    if (!expected || _autopilotVersionObservationEpoch != epoch)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), true},
            {QStringLiteral("reason"), QStringLiteral("fleet-evidence-source-changed-after-write")}};
    return request;
}

bool Vehicle::_fleetStartupTransportCurrent(bool requireQuiescent) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    // The health source validator rechecks QPointers/PRIMARY after each Link
    // virtual getter. Recheck this before reading any further native fields.
    const bool healthSource = _fleetHealthTelemetrySourceMatches();
    if (!self || !healthSource) return false;
    if (!_fleetStartupOwner || !_fleetStartupLink || _fleetStartupScopeCancelled || _fleetStartupExhausted
        || _fleetStartupOwner->thread() != thread() || _fleetStartupOwner->transactionEpoch() != _fleetStartupOwnerEpoch
        || _autopilotVersionObservationRemoving || _autopilotVersionObservationExhausted || _offlineEditingVehicle
        || _fleetStartupVersionEpoch != _autopilotVersionObservationEpoch || _fleetStartupFlightEpoch != _flightStateObservationEpoch
        || _flightStateObservationLink != _fleetStartupLink || _autopilotVersionObservationLink != _fleetStartupLink
        || !_autopilotVersionObservationObserved || !_autopilotVersionObservationComplete || _autopilotVersionObservationUpdating
        || !px4Firmware() || !multiRotor() || !isInitialConnectComplete() || !_parameterManager
        || !_parameterManager->_parameterObservationContextAllowed() || _parameterManager->_parameterObservationPendingWrites()
        || !_missionManager || !_geoFenceManager || !_rallyPointManager || _missionManager->inProgress()
        || _missionManager->transportCallInProgress() || _geoFenceManager->inProgress() || _geoFenceManager->transportCallInProgress()
        || _rallyPointManager->inProgress() || _rallyPointManager->transportCallInProgress() || !_ftpManager
        || !_ftpManager->_rgStateMachine.isEmpty() || _ftpManager->_ackOrNakTimeoutTimer.isActive()
        || !_mavCommandList.isEmpty() || _waitForMavlinkMessageId != 0 || _waitForMavlinkMessageResultHandler
        || _flightStateObservationExhausted || _autopilotVersionObservationExhausted || _startupCoordinateExhausted || _startupOdometryExhausted
        || !_fleetStartupEstimatorMemoryCurrent(requireQuiescent) || !_fleetStartupGlobalOriginBasisCurrent()
        // Passive RX journaling must still retain actual facts while inside
        // its own flight/HOME stack. ALT's quiet monitor gates queue/read
        // verdicts, never erases those original raw journal observations.
        || (requireQuiescent && !_fleetStartupAltitudeCurrent())) return false;
    if (requireQuiescent && (_fleetStartupReceiveDepth != 0 || _startupCoordinateReceiveDepth != 0
        || _flightStateObservationReceiveDepth != 0 || _autopilotVersionObservationReceiveDepth != 0
        || _fleetMissionObservationReceiveDepth != 0 || _startupOdometryReceiveDepth != 0 || _fleetHealthTelemetryReceiveDepth != 0
        || _startupGlobalOriginReceiveDepth != 0)) return false;
    return true; // Actual PRIMARY/control block was also checked above.
}

void Vehicle::_snapshotFleetStartupStep()
{
    if (_fleetStartupStepId.isEmpty() || _fleetStartupOperation == 0) return;
    const QVariantMap previous = _fleetStartupRetainedReceipts.value(_fleetStartupStepId);
    const QString step = _fleetStartupStep == FleetStartupStep::Seq0 ? QStringLiteral("Seq0")
        : (_fleetStartupStep == FleetStartupStep::NormalArm ? QStringLiteral("NormalArm") : QStringLiteral("MissionModeOnly"));
    _fleetStartupRetainedReceipts[_fleetStartupStepId] = {
        {QStringLiteral("schemaVersion"), 3}, {QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), _fleetStartupRequestIssued},
        {QStringLiteral("startupId"), _fleetStartupId}, {QStringLiteral("stepId"), _fleetStartupStepId},
        {QStringLiteral("operationId"), QString::number(_fleetStartupOperation)}, {QStringLiteral("step"), step},
        {QStringLiteral("vehicleId"), _id}, {QStringLiteral("componentId"), _defaultComponentId},
        {QStringLiteral("sourceEpoch"), _fleetStartupVersionEpoch}, {QStringLiteral("flightSourceEpoch"), _fleetStartupFlightEpoch},
        {QStringLiteral("baselineRawReceiptSequence"), QString::number(_fleetStartupBaselineReceipt)},
        {QStringLiteral("issuedAtMonotonicMs"), double(_fleetStartupIssuedAtMs)}, {QStringLiteral("expiresAtMonotonicMs"), double(_fleetStartupExpiresAtMs)},
        {QStringLiteral("wirePayloadHash"), _fleetStartupWireHash}, {QStringLiteral("closed"), _fleetStartupClosed},
        {QStringLiteral("closeReason"), _fleetStartupCloseReason},
        {QStringLiteral("ack"), previous.value(QStringLiteral("ack"), QVariantMap())},
        {QStringLiteral("heartbeat"), previous.value(QStringLiteral("heartbeat"), QVariantMap())},
        {QStringLiteral("missionCurrent"), previous.value(QStringLiteral("missionCurrent"), QVariantMap())},
        {QStringLiteral("receiveStackQuiescent"), false}, {QStringLiteral("requestCausalityVerified"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("flightCommandReleased"), _fleetStartupRequestIssued && _fleetStartupStep != FleetStartupStep::Seq0},
        {QStringLiteral("queueFlightObservation"), _fleetStartupQueueFlightObservation},
        {QStringLiteral("queueCoordinateObservation"), _fleetStartupQueueCoordinateObservation},
        {QStringLiteral("queueEstimatorObservation"), _fleetStartupQueueEstimatorObservation},
        {QStringLiteral("riskObservationsJournal"), previous.value(QStringLiteral("riskObservationsJournal"), QVariantList())},
        {QStringLiteral("journalExhausted"), previous.value(QStringLiteral("journalExhausted"), false)},
        {QStringLiteral("maximumObservedRawGapMs"), previous.value(QStringLiteral("maximumObservedRawGapMs"), -1.0)},
        {QStringLiteral("historyDurationMs"), previous.value(QStringLiteral("historyDurationMs"), 0.0)}};
}

void Vehicle::_closeFleetStartupStep(const QString& reason)
{
    _fleetStartupActive = false; _fleetStartupClosed = true; _fleetStartupCloseReason = reason;
    if (_fleetStartupRequestIssued && _fleetStartupStep != FleetStartupStep::Seq0)
        _quarantineMavCommand(_defaultComponentId, _fleetStartupStep == FleetStartupStep::NormalArm
            ? MAV_CMD_COMPONENT_ARM_DISARM : MAV_CMD_DO_SET_MODE, 4000);
    _snapshotFleetStartupStep();
    // Closed sender lane does NOT stop the journal. It runs through the
    // group barrier until the next actual queue or explicit scope cancellation.
}

bool Vehicle::fleetStartupStepReceiptSourceMatches(QObject* transactionObject, LinkInterface* expectedPrimary,
    const QString& suppliedStartupId, const QString& suppliedStepId) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return false;
    const QString startupId = suppliedStartupId, stepId = suppliedStepId;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<FleetMissionStartupTransaction> owner(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    const QPointer<LinkInterface> link(expectedPrimary);
    const QVariantMap receipt = _fleetStartupRetainedReceipts.value(stepId);
    const QString ownerEpoch = _fleetStartupOwnerEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch, healthEpoch = _fleetHealthTelemetryEpoch;
    const QString originEpoch = _startupGlobalOriginEpoch;
    const quint64 originRevision = _startupGlobalOriginReferenceRevision;
    const QString journalStepId = _fleetStartupJournalStepId;
    const quint64 rawSequence = _fleetStartupRawReceiptSequence, journalSequence = _fleetStartupJournalSequence;
    const quint64 operation = _fleetStartupOperation;
    const quint64 estimatorRevision = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus")).contentRevision;
    const auto unchanged = [&]() {
        return self && owner && link && self->_fleetStartupOwner == owner && self->_fleetStartupLink == link
            && self->_fleetStartupId == startupId && self->_fleetStartupOwnerEpoch == ownerEpoch
            && self->_autopilotVersionObservationEpoch == versionEpoch && self->_flightStateObservationEpoch == flightEpoch
            && self->_fleetHealthTelemetryEpoch == healthEpoch && self->_fleetStartupJournalStepId == journalStepId
            && self->_fleetStartupRawReceiptSequence == rawSequence && self->_fleetStartupJournalSequence == journalSequence
            && self->_fleetStartupOperation == operation
            && self->_startupGlobalOriginEpoch == originEpoch && self->_startupGlobalOriginReferenceRevision == originRevision
            && self->_fleetStartupGlobalOriginBasisCurrent()
            && self->_fleetStartupAltitudeCurrent()
            && self->_fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus")).contentRevision == estimatorRevision
            && self->_fleetStartupRetainedReceipts.value(stepId) == receipt;
    };
    if (!owner || owner != _fleetStartupOwner || startupId != _fleetStartupId || receipt.size() != 33
        || receipt.value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int || receipt.value(QStringLiteral("schemaVersion")).toInt() != 3
        || !link || expectedPrimary != _fleetStartupLink.data()
        || receipt.value(QStringLiteral("startupId")).toString() != startupId || receipt.value(QStringLiteral("stepId")).toString() != stepId
        || receipt.value(QStringLiteral("vehicleId")).toInt() != _id || receipt.value(QStringLiteral("componentId")).toInt() != _defaultComponentId
        || receipt.value(QStringLiteral("sourceEpoch")).toString() != _autopilotVersionObservationEpoch
        || receipt.value(QStringLiteral("flightSourceEpoch")).toString() != _flightStateObservationEpoch
        || receipt.value(QStringLiteral("journalExhausted")).toBool()
        || (!receipt.value(QStringLiteral("closed")).toBool() && _autopilotVersionObservationClock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())) return false;
    const bool transportCurrent = _fleetStartupTransportCurrent(true);
    if (!unchanged() || !transportCurrent) return false;
    const QVariantMap coordinates = receipt.value(QStringLiteral("queueCoordinateObservation")).toMap();
    const QVariantMap estimator = receipt.value(QStringLiteral("queueEstimatorObservation")).toMap();
    // An unissued prepared entry is useful solely for identifying the exact
    // first native call. It cannot form actual queued/flight evidence.
    if (!receipt.value(QStringLiteral("requestIssued")).toBool()) return unchanged();
    if (coordinates.value(QStringLiteral("coordinateSourceEpoch")).toString() != _startupCoordinateEpoch
        || estimator.size() != 11 || estimator.value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int
        || estimator.value(QStringLiteral("schemaVersion")).toInt() != 1
        || estimator.value(QStringLiteral("sourceEpoch")).toString() != _fleetStartupEstimatorSourceEpoch) return false;
    const bool coordinatesCurrent = startupCoordinateObservationSourceMatches(link.data());
    return unchanged() && coordinatesCurrent;
}

QVariantMap Vehicle::fleetStartupStepReceipt(QObject* transactionObject, const QString& suppliedStartupId,
    const QString& suppliedStepId) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const QString startupId = suppliedStartupId, stepId = suppliedStepId;
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    const QPointer<FleetMissionStartupTransaction> owner(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    const QPointer<LinkInterface> link(_fleetStartupLink);
    const QString ownerEpoch = _fleetStartupOwnerEpoch, versionEpoch = _autopilotVersionObservationEpoch;
    const QString flightEpoch = _flightStateObservationEpoch, healthEpoch = _fleetHealthTelemetryEpoch;
    const QString originEpoch = _startupGlobalOriginEpoch;
    const quint64 originRevision = _startupGlobalOriginReferenceRevision;
    const QString journalStepId = _fleetStartupJournalStepId;
    const quint64 rawSequence = _fleetStartupRawReceiptSequence, journalSequence = _fleetStartupJournalSequence;
    const quint64 operation = _fleetStartupOperation;
    const quint64 estimatorRevision = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus")).contentRevision;
    const QVariantMap original = _fleetStartupRetainedReceipts.value(stepId);
    if (!owner || owner != _fleetStartupOwner || startupId != _fleetStartupId || original.size() != 33
        || original.value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int || original.value(QStringLiteral("schemaVersion")).toInt() != 3) return {};
    // Source validation can enter Link virtual getters. Reject this capture
    // on any nested RAW/semantic/source change, then copy the ACTUAL retained
    // record only after those callbacks. Nothing below dispatches callbacks.
    const bool sourceCurrent = fleetStartupStepReceiptSourceMatches(owner.data(), link.data(), startupId, stepId);
    if (!self || !owner || _fleetStartupOwner != owner || _fleetStartupLink != link || _fleetStartupId != startupId
        || _fleetStartupOwnerEpoch != ownerEpoch || _autopilotVersionObservationEpoch != versionEpoch
        || _flightStateObservationEpoch != flightEpoch || _fleetHealthTelemetryEpoch != healthEpoch
        || _fleetStartupJournalStepId != journalStepId || _fleetStartupRawReceiptSequence != rawSequence
        || _fleetStartupJournalSequence != journalSequence || _fleetStartupOperation != operation
        || _startupGlobalOriginEpoch != originEpoch || _startupGlobalOriginReferenceRevision != originRevision
        || _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus")).contentRevision != estimatorRevision
        || _fleetStartupRetainedReceipts.value(stepId) != original) return {};
    QVariantMap receipt = _fleetStartupRetainedReceipts.value(stepId);
    if (!owner || owner != _fleetStartupOwner || startupId != _fleetStartupId || receipt.size() != 33
        || receipt.value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int || receipt.value(QStringLiteral("schemaVersion")).toInt() != 3) return {};
    const qint64 started = qint64(receipt.value(QStringLiteral("issuedAtMonotonicMs")).toDouble());
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    const qint64 ended = _fleetStartupHistoryEndedAt.value(stepId, -1);
    const qint64 terminal = ended >= 0 ? ended : now;
    qint64 maximumGap = _fleetStartupMaximumRawGap.value(stepId, -1);
    const QMap<QString, qint64> latest = _fleetStartupRawLastAt.value(stepId);
    for (auto channel = latest.constBegin(); channel != latest.constEnd(); ++channel) {
        if (channel.value() < 0 || terminal < channel.value()) { maximumGap = -1; break; }
        if (maximumGap >= 0) maximumGap = qMax(maximumGap, terminal - channel.value());
    }
    receipt[QStringLiteral("maximumObservedRawGapMs")] = double(maximumGap);
    receipt[QStringLiteral("historyDurationMs")] = double(started >= 0 && terminal >= started ? terminal - started : 0);
    const bool quiescent = _fleetStartupReceiveDepth == 0 && _startupCoordinateReceiveDepth == 0 && _startupOdometryReceiveDepth == 0
        && _flightStateObservationReceiveDepth == 0 && _autopilotVersionObservationReceiveDepth == 0 && _fleetMissionObservationReceiveDepth == 0
        && _fleetHealthTelemetryReceiveDepth == 0 && _startupGlobalOriginReceiveDepth == 0
        && (!_fleetStartupAltitude.required || _startupAltitudeReceiveDepth == 0);
    receipt[QStringLiteral("receiveStackQuiescent")] = quiescent;
    receipt[QStringLiteral("ok")] = quiescent && sourceCurrent;
    return receipt; // Native provenance only; never a permit/phase verdict.
}

bool Vehicle::cancelFleetStartupStep(QObject* transactionObject, const QString& suppliedStartupId, const QString& suppliedStepId)
{
    const QString startupId = suppliedStartupId, stepId = suppliedStepId;
    const QPointer<FleetMissionStartupTransaction> owner(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread() || !owner || owner != _fleetStartupOwner
        || owner->transactionEpoch() != _fleetStartupOwnerEpoch || startupId != _fleetStartupId || !_fleetStartupRetainedReceipts.contains(stepId)) return false;
    _fleetStartupScopeCancelled = true; _fleetStartupJournalStepId.clear();
    retireFleetStartupAltitudeMonitor(owner.data(), startupId);
    for (auto record = _fleetStartupRetainedReceipts.constBegin(); record != _fleetStartupRetainedReceipts.constEnd(); ++record)
        if (_fleetStartupHistoryEndedAt.value(record.key(), -1) < 0) _fleetStartupHistoryEndedAt[record.key()] = _autopilotVersionObservationClock.elapsed();
    _closeFleetStartupStep(QStringLiteral("scope-cancelled"));
    return true; // Already queued packets may still execute. No disarm/rollback.
}

bool Vehicle::fleetStartupTransportQuiescent() const
{
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread() && !_fleetStartupEntryBusy && !_fleetDeploymentSendBusy
        && _fleetStartupReceiveDepth == 0 && _startupCoordinateReceiveDepth == 0 && _startupOdometryReceiveDepth == 0 && _flightStateObservationReceiveDepth == 0
        && _autopilotVersionObservationReceiveDepth == 0 && _fleetMissionObservationReceiveDepth == 0 && _fleetHealthTelemetryReceiveDepth == 0
        && _startupGlobalOriginReceiveDepth == 0;
}

bool Vehicle::fleetOccupancyRecoveryTransportQuiescent(LinkInterface* expectedPrimary) const
{
    const QPointer<Vehicle> self(const_cast<Vehicle*>(this));
    if (!fleetStartupTransportQuiescent() || !expectedPrimary) return false;
    // Reuse the actual private normal-read admission, without requesting a
    // version/parameter/mission read or allowing its own outstanding command.
    const bool allowed = _fleetEvidenceReadAllowed(expectedPrimary, false);
    return self && allowed && fleetStartupTransportQuiescent();
}

bool Vehicle::fleetOccupancyRecoverySourceMatches(LinkInterface* expectedPrimary,
    const QString& expectedFlightEpoch, const QString& expectedVersionEpoch) const
{
    // Intentionally no *_SourceAllowed()/isConnected virtual or RX-depth/
    // updating check. A transient same-source receipt is not a source change.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving
        || _autopilotVersionObservationExhausted || _flightStateObservationExhausted
        || !expectedPrimary || expectedFlightEpoch.isEmpty() || expectedVersionEpoch.isEmpty()
        || _flightStateObservationEpoch != expectedFlightEpoch
        || _autopilotVersionObservationEpoch != expectedVersionEpoch
        || _flightStateObservationLink.data() != expectedPrimary
        || _autopilotVersionObservationLink.data() != expectedPrimary
        || !_vehicleLinkManager || _vehicleLinkManager->thread() != thread()
        || _vehicleLinkManager->communicationLost()) return false;
    const auto primary = _vehicleLinkManager->primaryLink().lock();
    return primary && primary.get() == expectedPrimary;
}

bool Vehicle::fleetOccupancyRecoveryGroundMatches(LinkInterface* expectedPrimary,
    const QVariantMap& frozenStableIdentity) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return false;
    const QVariantMap expected = frozenStableIdentity;
    if (expected.size() != 7 || expected.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || expected.value(QStringLiteral("schemaVersion")).toInt() != 1
        || expected.value(QStringLiteral("vehicleId")).type() != QVariant::Int
        || expected.value(QStringLiteral("vehicleId")).toInt() != _id
        || expected.value(QStringLiteral("componentId")).type() != QVariant::Int
        || expected.value(QStringLiteral("componentId")).toInt() != _defaultComponentId
        || expected.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || expected.value(QStringLiteral("versionSourceEpoch")).type() != QVariant::String
        || !fleetOccupancyRecoverySourceMatches(expectedPrimary,
            expected.value(QStringLiteral("sourceEpoch")).toString(), expected.value(QStringLiteral("versionSourceEpoch")).toString())
        || !fleetStartupTransportQuiescent() || armed() || flying() || !isInitialConnectComplete()
        || !_parameterManager || _parameterManager->thread() != thread()
        || _parameterManager->_parameterObservationExhausted || !_parameterManager->_parameterObservationClock.isValid()
        || _parameterManager->_logReplay || !_parameterManager->_initialLoadComplete || !_parameterManager->_parametersReady
        || _parameterManager->_missingParameters || _parameterManager->_initialRequestTimeoutTimer.isActive()
        || _parameterManager->_parameterObservationPendingWrites()
        || !_missionManager || !_geoFenceManager || !_rallyPointManager
        || _missionManager->inProgress() || _missionManager->transportCallInProgress()
        || _geoFenceManager->inProgress() || _geoFenceManager->transportCallInProgress()
        || _rallyPointManager->inProgress() || _rallyPointManager->transportCallInProgress()
        || !_ftpManager || !_ftpManager->_rgStateMachine.isEmpty() || _ftpManager->_ackOrNakTimeoutTimer.isActive()
        || !_mavCommandList.isEmpty() || _waitForMavlinkMessageResultHandler || _waitForMavlinkMessageId != 0
        || !_autopilotVersionObservationClock.isValid() || !QElapsedTimer::isMonotonic()) return false;
    for (auto it = _parameterManager->_waitingReadParamIndexMap.cbegin(); it != _parameterManager->_waitingReadParamIndexMap.cend(); ++it)
        if (!it.value().isEmpty()) return false;
    for (auto it = _parameterManager->_waitingReadParamNameMap.cbegin(); it != _parameterManager->_waitingReadParamNameMap.cend(); ++it)
        if (!it.value().isEmpty()) return false;
    const qint64 now = _autopilotVersionObservationClock.elapsed();
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("landedState")}) {
        const auto row = _flightStateObservations.constFind(channel);
        const QVariant expectedRow = expected.value(channel);
        if (row == _flightStateObservations.cend() || expectedRow.type() != QVariant::Map
            || !row->observed || !row->complete || row->updating || row->receivedAtMs < 0
            || now < row->receivedAtMs || now - row->receivedAtMs > 2000) return false;
        const QVariantMap semantic = expectedRow.toMap();
        if (semantic.size() != 3 || semantic.value(QStringLiteral("contentRevision")).type() != QVariant::String
            || semantic.value(QStringLiteral("contentRevision")).toString() != QString::number(row->contentRevision)
            || semantic.value(QStringLiteral("contentHash")).type() != QVariant::String
            || semantic.value(QStringLiteral("contentHash")).toString() != row->contentHash
            || semantic.value(QStringLiteral("value")).type() != QVariant::Map
            || semantic.value(QStringLiteral("value")).toMap() != row->value) return false;
    }
    const QVariantMap heartbeat = _flightStateObservations.value(QStringLiteral("heartbeat")).value;
    const QVariantMap landed = _flightStateObservations.value(QStringLiteral("landedState")).value;
    return heartbeat.value(QStringLiteral("armed")).type() == QVariant::Bool && !heartbeat.value(QStringLiteral("armed")).toBool()
        && landed.value(QStringLiteral("landedStateKnown")).type() == QVariant::Bool
        && landed.value(QStringLiteral("landedStateKnown")).toBool()
        && landed.value(QStringLiteral("landedState")).toUInt() == MAV_LANDED_STATE_ON_GROUND;
}

void Vehicle::_handleFleetStartupEstimatorRawMessage(const mavlink_message_t& original, LinkInterface* link,
    qint64 receivedAtMs, const FleetStartupEstimatorIngress& ingress, quint64 healthToken, const QString& healthEpoch)
{
    // This context was captured at native ingress, BEFORE source virtuals or
    // any observer/plugin callback. An older RX stack cannot be attached to a
    // scope/step which happened to be prepared while that stack was running.
    if (QThread::currentThread() != thread() || original.msgid != MAVLINK_MSG_ID_ESTIMATOR_STATUS
        || sender() != _mavlink || original.sysid != _id || original.compid != _defaultComponentId
        || !ingress.owner || ingress.owner != _fleetStartupOwner || ingress.startupId != _fleetStartupId
        || ingress.ownerEpoch != _fleetStartupOwnerEpoch || ingress.journalStepId.isEmpty()
        || ingress.journalStepId != _fleetStartupJournalStepId || _fleetStartupScopeCancelled
        || link != _fleetStartupLink.data()) return;
    auto record = _fleetStartupRetainedReceipts.find(ingress.journalStepId);
    if (record == _fleetStartupRetainedReceipts.end() || record->size() != 33
        || record->value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int
        || record->value(QStringLiteral("schemaVersion")).toInt() != 3
        || record->value(QStringLiteral("operationId")).toString() != ingress.operationId
        || record->value(QStringLiteral("startupId")).toString() != ingress.startupId
        || record->value(QStringLiteral("stepId")).toString() != ingress.journalStepId
        || (!record->value(QStringLiteral("requestIssued")).toBool()
            && !(_fleetStartupQueueInProgress && ingress.journalStepId == _fleetStartupStepId))) return;
    const auto invalidate = [&]() {
        (*record)[QStringLiteral("journalExhausted")] = true;
        _fleetStartupMaximumRawGap[ingress.journalStepId] = -1;
    };
    const FleetHealthTelemetryChannel row = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus"));
    if (receivedAtMs < 0 || receivedAtMs < record->value(QStringLiteral("issuedAtMonotonicMs")).toDouble()
        || ingress.versionEpoch != _fleetStartupVersionEpoch || ingress.flightEpoch != _fleetStartupFlightEpoch
        || ingress.versionEpoch != _autopilotVersionObservationEpoch || ingress.flightEpoch != _flightStateObservationEpoch
        || ingress.healthEpoch != _fleetStartupEstimatorSourceEpoch || healthEpoch != ingress.healthEpoch
        || healthEpoch != _fleetHealthTelemetryEpoch || link != _fleetHealthTelemetryLink.data()
        || !healthToken || row.receiptSequence != healthToken || row.updateToken != healthToken
        || !row.observed || !row.complete || row.updating || row.startupReceivedAtMs != receivedAtMs
        || !_fleetStartupEstimatorMemoryCurrent(false) || !_fleetStartupGlobalOriginBasisCurrent()) {
        invalidate(); return;
    }
    if (_fleetStartupRawReceiptSequence == std::numeric_limits<quint64>::max()) {
        _fleetStartupExhausted = true; invalidate(); return;
    }
    const QVariantMap value = _fleetStartupEstimatorProjection(false);
    if (value.size() != 11) { invalidate(); return; }
    // Every actual EST packet advances its own periodic receipt/time, even
    // when flags and semantic revision are unchanged. A recovered stream does
    // not erase an earlier gap, invalid source or bad semantic journal leaf.
    const quint64 receiptSequence = ++_fleetStartupRawReceiptSequence;
    const QString key = QStringLiteral("estimatorStatus");
    QMap<QString, qint64>& latest = _fleetStartupRawLastAt[ingress.journalStepId];
    qint64& maximumGap = _fleetStartupMaximumRawGap[ingress.journalStepId];
    const qint64 previous = latest.value(key, -1);
    if (previous < 0 || receivedAtMs < previous) invalidate();
    else if (maximumGap >= 0) maximumGap = qMax(maximumGap, receivedAtMs - previous);
    latest[key] = receivedAtMs;
    const QString riskHash = startupValueHash(value);
    if (_fleetStartupLastRiskHash.value(key) == riskHash) return;
    QVariantList journal = record->value(QStringLiteral("riskObservationsJournal")).toList();
    if (journal.size() >= 256 || _fleetStartupJournalSequence == std::numeric_limits<quint64>::max()) {
        invalidate(); return;
    }
    journal.append(QVariantMap{{QStringLiteral("journalSequence"), QString::number(++_fleetStartupJournalSequence)},
        {QStringLiteral("receiptSequence"), QString::number(receiptSequence)}, {QStringLiteral("channel"), key},
        {QStringLiteral("receivedAtMonotonicMs"), double(receivedAtMs)}, {QStringLiteral("rawPayloadHash"), startupRawHash(original)},
        {QStringLiteral("riskContentHash"), riskHash}, {QStringLiteral("value"), value},
        {QStringLiteral("source"), QStringLiteral("live-estimator-status")}});
    (*record)[QStringLiteral("riskObservationsJournal")] = journal;
    _fleetStartupLastRiskHash[key] = riskHash;
}

bool Vehicle::_fleetStartupRawIngressSame(const FleetStartupRawIngress& ingress) const
{
    if (!ingress.eligible || !ingress.owner || ingress.owner != _fleetStartupOwner
        || ingress.primary != _fleetStartupLink || _fleetStartupScopeCancelled
        || ingress.startupId != _fleetStartupId || ingress.ownerEpoch != _fleetStartupOwnerEpoch
        || ingress.journalStepId.isEmpty() || ingress.journalStepId != _fleetStartupJournalStepId) return false;
    const auto record = _fleetStartupRetainedReceipts.constFind(ingress.journalStepId);
    return record != _fleetStartupRetainedReceipts.cend() && record->size() == 33
        && record->value(QStringLiteral("schemaVersion")).userType() == QMetaType::Int
        && record->value(QStringLiteral("schemaVersion")).toInt() == 3
        && record->value(QStringLiteral("startupId")).toString() == ingress.startupId
        && record->value(QStringLiteral("stepId")).toString() == ingress.journalStepId
        && record->value(QStringLiteral("operationId")).toString() == ingress.operationId
        && record->value(QStringLiteral("issuedAtMonotonicMs")) == ingress.issuedAtMs;
}

void Vehicle::_discardFleetStartupRawIngress(const FleetStartupRawIngress& ingress)
{
    // Never invalidate a replacement scope/step, or manufacture history for a
    // packet which entered before an actual queue. A closed sender still owns
    // its journal through the group barrier, so active/closed are not gates.
    if (!_fleetStartupRawIngressSame(ingress)) return;
    auto record = _fleetStartupRetainedReceipts.find(ingress.journalStepId);
    (*record)[QStringLiteral("journalExhausted")] = true;
    _fleetStartupMaximumRawGap[ingress.journalStepId] = -1;
}

bool Vehicle::_handleFleetStartupRawMessage(const mavlink_message_t& original, LinkInterface* link,
    const FleetStartupRawIngress& ingress)
{
    if (!ingress.eligible) return true; // Explicitly pre-queue/foreign: never adopt a later step.
    const QPointer<Vehicle> self(this);
    const qint64 receivedAtMs = ingress.receivedAtMs;
    const auto current = [&]() {
        return self && _fleetStartupRawIngressSame(ingress) && ingress.primary && ingress.protocol
            && ingress.protocol == _mavlink && sender() == ingress.protocol.data() && link == ingress.primary.data()
            && ingress.vehicleId == _id && ingress.componentId == _defaultComponentId
            && original.sysid == ingress.vehicleId && original.compid == ingress.componentId
            && ingress.versionEpoch == _fleetStartupVersionEpoch && ingress.versionEpoch == _autopilotVersionObservationEpoch
            && ingress.flightEpoch == _fleetStartupFlightEpoch && ingress.flightEpoch == _flightStateObservationEpoch
            && ingress.healthEpoch == _fleetStartupEstimatorSourceEpoch && ingress.healthEpoch == _fleetHealthTelemetryEpoch
            && _fleetStartupReceiveDepth == 1 && _fleetStartupRawReceiptSequence == ingress.rawSequence
            && receivedAtMs >= 0 && receivedAtMs >= ingress.issuedAtMs.toDouble();
    };
    if (!current()) return false;
    // This source check can enter Link virtuals. Retain no map iterator and
    // recheck the original ingress identity/order after it returns. Nested
    // RAW receives must not be appended in reverse order, even in one ms.
    const bool sourceCurrent = _fleetStartupTransportCurrent(false);
    if (!current() || !sourceCurrent) return false;
    auto record = _fleetStartupRetainedReceipts.find(ingress.journalStepId);
    if (record->value(QStringLiteral("journalExhausted")).toBool()) return false;
    if (original.msgid == MAVLINK_MSG_ID_ODOMETRY) {
        if (!_fleetStartupOdometryContinuityRequired) return true; // No new v1 periodic prerequisite.
        bool decoded = false;
        const QVariantMap odometry = FleetStartupOdometry::decode(original, &decoded);
        if (!decoded || !startupOdometryNedObservationMatches(odometry)
            || _startupOdometryLink.data() != link || _startupOdometryVersionEpoch != _fleetStartupVersionEpoch
            || _startupOdometryFlightEpoch != _fleetStartupFlightEpoch
            || _fleetStartupRawReceiptSequence == std::numeric_limits<quint64>::max()) {
            if (_fleetStartupRawReceiptSequence == std::numeric_limits<quint64>::max()) _fleetStartupExhausted = true;
            return false;
        }
        // Count actual protocol receipts, including equal-content ODOMETRY.
        // Receipt time is the local monotonic clock; sample time is not used
        // to repair a missing interval or mapped to another stream's clock.
        ++_fleetStartupRawReceiptSequence;
        QMap<QString, qint64>& latest = _fleetStartupRawLastAt[ingress.journalStepId];
        qint64& maximumGap = _fleetStartupMaximumRawGap[ingress.journalStepId];
        const QString key = QStringLiteral("odometry");
        const qint64 previous = latest.value(key, -1);
        if (previous < 0 || receivedAtMs < previous) {
            return false;
        } else if (maximumGap >= 0) {
            maximumGap = qMax(maximumGap, receivedAtMs - previous);
        }
        latest[key] = receivedAtMs;
        // Pose is not a seventh flight risk channel. Reference metadata and
        // RAW-reset ABA remain in the independent native reference ledger.
        return true;
    }
    const bool ackMessage = original.msgid == MAVLINK_MSG_ID_COMMAND_ACK;
    int maximum = 0, minimum = 0;
    QString channel = flightObservationChannel(original.msgid, &maximum, &minimum);
    if (original.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED) { channel = QStringLiteral("localPosition"); maximum = MAVLINK_MSG_ID_LOCAL_POSITION_NED_LEN; minimum = MAVLINK_MSG_ID_LOCAL_POSITION_NED_MIN_LEN; }
    if (ackMessage) { maximum = MAVLINK_MSG_ID_COMMAND_ACK_LEN; minimum = MAVLINK_MSG_ID_COMMAND_ACK_MIN_LEN; }
    if (maximum == 0) return false;
    if (_fleetStartupRawReceiptSequence == std::numeric_limits<quint64>::max()) {
        _fleetStartupExhausted = true; return false;
    }
    if ((original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1) || original.len < 1 || original.len > maximum
        || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != minimum)) {
        return false;
    }
    QVariantMap value;
    if (ackMessage) {
        if (receivedAtMs >= record->value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) {
            if (_fleetStartupStepId == ingress.journalStepId && _fleetStartupActive) _closeFleetStartupStep(QStringLiteral("step-deadline-expired"));
            return true; // Late terminal ACK cannot turn the native finite wait into success.
        }
        mavlink_message_t normalized = original;
        memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
        memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
        mavlink_command_ack_t ack{};
        mavlink_msg_command_ack_decode(&normalized, &ack);
        const QString step = record->value(QStringLiteral("step")).toString();
        const int command = step == QStringLiteral("NormalArm") ? MAV_CMD_COMPONENT_ARM_DISARM : MAV_CMD_DO_SET_MODE;
        if (step == QStringLiteral("Seq0") || ack.command != command
            || (ack.target_system != 0 && ack.target_system != _mavlink->getSystemId())
            || (ack.target_component != 0 && ack.target_component != _mavlink->getComponentId())) return true;
        if (ack.result >= MAV_RESULT_ENUM_END) return false;
        const QVariantMap oldAck = record->value(QStringLiteral("ack")).toMap().value(QStringLiteral("value")).toMap();
        if (!oldAck.isEmpty() && oldAck.value(QStringLiteral("result")).toInt() != MAV_RESULT_IN_PROGRESS) return true;
        value = {{QStringLiteral("command"), int(ack.command)}, {QStringLiteral("result"), int(ack.result)}, {QStringLiteral("progress"), int(ack.progress)},
            {QStringLiteral("resultParam2"), int(ack.result_param2)}, {QStringLiteral("targetSystem"), int(ack.target_system)},
            {QStringLiteral("targetComponent"), int(ack.target_component)}, {QStringLiteral("sourceSystem"), int(original.sysid)},
            {QStringLiteral("sourceComponent"), int(original.compid)}};
    } else if (channel == QStringLiteral("localPosition")) {
        QString decoded;
        if (!startupCoordinateDecode(original, &decoded, &value)) return false;
    } else if (!decodeFlightObservation(original, _firmwareType, _vehicleType, &value)) {
        return false;
    }
    const quint64 receiptSequence = ++_fleetStartupRawReceiptSequence;
    // Event-based MISSION_REACHED is retained as risk, never treated as a
    // periodic heartbeat. Every continuous raw channel has its own gap clock.
    if (!ackMessage && channel != QStringLiteral("missionReached")) {
        QMap<QString, qint64>& latest = _fleetStartupRawLastAt[ingress.journalStepId];
        qint64& maximumGap = _fleetStartupMaximumRawGap[ingress.journalStepId];
        const qint64 previous = latest.value(channel, -1);
        if (previous < 0 || receivedAtMs < previous) return false;
        else if (maximumGap >= 0) maximumGap = qMax(maximumGap, receivedAtMs - previous);
        latest[channel] = receivedAtMs;
    }
    const QString payloadHash = startupRawHash(original);
    const QVariantMap leaf {{QStringLiteral("receiptSequence"), QString::number(receiptSequence)},
        {QStringLiteral("receivedAtMonotonicMs"), double(receivedAtMs)}, {QStringLiteral("rawPayloadHash"), payloadHash}, {QStringLiteral("value"), value}};
    if (ackMessage) {
        (*record)[QStringLiteral("ack")] = leaf;
        if (value.value(QStringLiteral("result")).toInt() != MAV_RESULT_IN_PROGRESS && _fleetStartupStepId == ingress.journalStepId)
            _closeFleetStartupStep(QStringLiteral("raw-terminal-ack-observed"));
        return true; // ACK never proves a state/phase transition or causality.
    }
    if (channel == QStringLiteral("heartbeat")) (*record)[QStringLiteral("heartbeat")] = leaf;
    if (channel == QStringLiteral("missionCurrent")) (*record)[QStringLiteral("missionCurrent")] = leaf;
    const auto appendRisk = [&](const QString& key, const QVariantMap& rawValue, const QString& source) {
        QVariantMap risk = rawValue;
        if (key == QStringLiteral("globalPosition") || key == QStringLiteral("localPosition")) risk.remove(QStringLiteral("timeBootMs"));
        const QString riskHash = startupValueHash(risk);
        if (_fleetStartupLastRiskHash.value(key) == riskHash) return;
        QVariantList journal = record->value(QStringLiteral("riskObservationsJournal")).toList();
        if (journal.size() >= 256 || _fleetStartupJournalSequence == std::numeric_limits<quint64>::max()) {
            (*record)[QStringLiteral("journalExhausted")] = true; return;
        }
        journal.append(QVariantMap{{QStringLiteral("journalSequence"), QString::number(++_fleetStartupJournalSequence)},
            {QStringLiteral("receiptSequence"), QString::number(receiptSequence)}, {QStringLiteral("channel"), key},
            {QStringLiteral("receivedAtMonotonicMs"), double(receivedAtMs)}, {QStringLiteral("rawPayloadHash"), payloadHash},
            {QStringLiteral("riskContentHash"), riskHash}, {QStringLiteral("value"), rawValue}, {QStringLiteral("source"), source}});
        (*record)[QStringLiteral("riskObservationsJournal")] = journal;
        _fleetStartupLastRiskHash[key] = riskHash;
    };
    appendRisk(channel, value, channel == QStringLiteral("localPosition") ? QStringLiteral("live-local-position-ned") : flightObservationSource(channel));
    if (channel == QStringLiteral("homePosition")) {
        QString coordinatesChannel;
        QVariantMap coordinates;
        if (!startupCoordinateDecode(original, &coordinatesChannel, &coordinates)) (*record)[QStringLiteral("journalExhausted")] = true;
        else appendRisk(QStringLiteral("homeCoordinates"), coordinates, QStringLiteral("live-home-position-geo-local"));
    }
    if (record->value(QStringLiteral("journalExhausted")).toBool()) return false;
    if (_fleetStartupActive && _fleetStartupStepId == ingress.journalStepId && _fleetStartupStep == FleetStartupStep::Seq0
        && channel == QStringLiteral("missionCurrent") && value.value(QStringLiteral("sequence")).toUInt() == 0
        && receiptSequence > record->value(QStringLiteral("baselineRawReceiptSequence")).toString().toULongLong()
        && receivedAtMs < record->value(QStringLiteral("expiresAtMonotonicMs")).toDouble())
        _closeFleetStartupStep(QStringLiteral("raw-wire-seq0-observed"));
    return true;
}

QVariantMap Vehicle::sendFleetStartupStep(QObject* transactionObject, LinkInterface* expectedPrimary,
    const QString& suppliedStartupId, const QString& suppliedStepId, FleetStartupStep step, std::function<bool()> authorizationFence)
{
    const QString startupId = suppliedStartupId, stepId = suppliedStepId;
    const QPointer<Vehicle> self(this);
    const QPointer<FleetMissionStartupTransaction> owner(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    const QPointer<LinkInterface> link(expectedPrimary);
    QPointer<VehicleMissionExecutionObserver> executionObserver;
    VehicleMissionExecutionObserver::QueueContext executionContext;
    const auto denied = [step](const QString& reason, bool issued = false) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), issued},
            {QStringLiteral("reason"), reason}, {QStringLiteral("flightCommandReleased"), issued && step != FleetStartupStep::Seq0}};
    };
    const auto validUuid = [](const QString& id) { return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id; };
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread() || _fleetStartupEntryBusy
        || !owner || !link || owner->thread() != thread() || !authorizationFence || !validUuid(startupId) || !validUuid(stepId)
        || (step != FleetStartupStep::Seq0 && step != FleetStartupStep::NormalArm && step != FleetStartupStep::MissionModeOnly)
        || _fleetStartupExhausted || _fleetStartupReceiveDepth || _startupCoordinateReceiveDepth || _flightStateObservationReceiveDepth
        || _autopilotVersionObservationReceiveDepth || _fleetMissionObservationReceiveDepth || _startupOdometryReceiveDepth || _startupOdometryExhausted
        || _fleetHealthTelemetryReceiveDepth || _fleetHealthTelemetryExhausted || _fleetHealthTelemetryRemoving
        || _startupGlobalOriginReceiveDepth || _startupGlobalOriginExhausted || _startupGlobalOriginRemoving)
        return denied(QStringLiteral("actual-quiescent-native-startup-step-required"));
    if (!owner->commandContextCurrent(self.data(), link.data(), startupId, stepId, int(step)))
        return denied(QStringLiteral("actual-startup-command-owner-context-required"));
    const QVariantMap altitudeContext = owner->reviewedAltitudeContext(self.data(), link.data(), startupId);
    if (altitudeContext.size() != 4 || altitudeContext.value(QStringLiteral("required")).type() != QVariant::Bool)
        return denied(QStringLiteral("actual-reviewed-altitude-policy-context-required"));
    if (_fleetStartupActive) return denied(QStringLiteral("native-startup-step-busy"));
    if (_fleetStartupOwner != owner || _fleetStartupId != startupId) {
        if (_fleetStartupOwner && !_fleetStartupScopeCancelled) {
            const QVariantMap previousOwner = _fleetStartupOwner->current();
            if (previousOwner.value(QStringLiteral("active")).toBool() || previousOwner.value(QStringLiteral("reservationHeld")).toBool()
                || previousOwner.value(QStringLiteral("transportDraining")).toBool()) return denied(QStringLiteral("previous-native-startup-scope-still-owned"));
        }
        QObject::disconnect(_fleetStartupOwnerDestroyedConnection);
        _fleetStartupRetainedReceipts.clear(); _fleetStartupJournalStepId.clear(); _fleetStartupLastRiskHash.clear();
        _fleetStartupRawLastAt.clear(); _fleetStartupMaximumRawGap.clear(); _fleetStartupHistoryEndedAt.clear();
        _fleetStartupOdometryContinuityRequired = false;
        _fleetStartupEstimatorSourceEpoch = _fleetHealthTelemetryEpoch;
        _fleetStartupEstimatorMaximumGapMs = -1;
        _fleetStartupScopeCancelled = false; _fleetStartupOwner = owner; _fleetStartupId = startupId;
        _fleetStartupOwnerEpoch = owner->transactionEpoch();
        _fleetStartupLink = link;
        // One basis for this actual owner/startup scope, not a new origin
        // sample for each Seq0/Arm/Mode step. Absent remains explicitly absent.
        _fleetStartupGlobalOriginBasis = _startupGlobalOriginStableReference();
        if (!_bindFleetStartupAltitude(altitudeContext))
            return denied(QStringLiteral("actual-reviewed-altitude-scope-binding-rejected"));
    }
    if (_fleetStartupAltitude.context != altitudeContext || !_fleetStartupAltitudeCurrent())
        return denied(QStringLiteral("original-altitude-scope-no-longer-current"));
    if (_fleetStartupScopeCancelled || _fleetStartupRetainedReceipts.contains(stepId) || _fleetStartupRetainedReceipts.size() >= 3)
        return denied(QStringLiteral("startup-step-already-used-or-scope-closed"));
    const QString stepName = step == FleetStartupStep::Seq0 ? QStringLiteral("Seq0") : (step == FleetStartupStep::NormalArm ? QStringLiteral("NormalArm") : QStringLiteral("MissionModeOnly"));
    for (const QVariantMap& old : _fleetStartupRetainedReceipts)
        if (old.value(QStringLiteral("step")).toString() == stepName) return denied(QStringLiteral("startup-step-opcode-already-used"));
    if (_fleetStartupNextOperation == std::numeric_limits<quint64>::max()) {
        _fleetStartupExhausted = true; return denied(QStringLiteral("startup-operation-counter-exhausted"));
    }
    const MAV_CMD command = step == FleetStartupStep::NormalArm ? MAV_CMD_COMPONENT_ARM_DISARM : MAV_CMD_DO_SET_MODE;
    if (step != FleetStartupStep::Seq0 && _isMavCommandQuarantined(_defaultComponentId, command))
        return denied(QStringLiteral("native-startup-command-quarantined"));
    _fleetStartupEntryBusy = true;
    const auto releasing = qScopeGuard([self]() { if (self) self->_fleetStartupEntryBusy = false; });
    const qint64 estimatorMaximumGap = owner->estimatorContinuityMaximumGapMs(self.data(), startupId);
    if (!self || !owner || !link || estimatorMaximumGap < 100 || estimatorMaximumGap > 3000)
        return denied(QStringLiteral("actual-reviewed-estimator-gap-policy-required"));
    _fleetStartupEstimatorMaximumGapMs = estimatorMaximumGap;
    _fleetStartupLink = link; _fleetStartupVersionEpoch = _autopilotVersionObservationEpoch; _fleetStartupFlightEpoch = _flightStateObservationEpoch;
    _fleetStartupStepId = stepId; _fleetStartupStep = step; _fleetStartupOperation = ++_fleetStartupNextOperation;
    const quint64 operation = _fleetStartupOperation;
    _fleetStartupActive = true; _fleetStartupClosed = false; _fleetStartupRequestIssued = false; _fleetStartupCloseReason.clear();
    _fleetStartupBaselineReceipt = _fleetStartupRawReceiptSequence;
    _fleetStartupIssuedAtMs = -1; _fleetStartupExpiresAtMs = _autopilotVersionObservationClock.elapsed() + 4000;
    _fleetStartupQueueFlightObservation.clear(); _fleetStartupQueueCoordinateObservation.clear(); _fleetStartupQueueEstimatorObservation.clear(); _fleetStartupWireHash.clear();
    _snapshotFleetStartupStep();
    const QVariantMap fencedEstimator = _fleetStartupEstimatorProjection(true);
    const auto current = [&]() {
        return self && owner && link && _fleetStartupOwner == owner && _fleetStartupId == startupId && _fleetStartupStepId == stepId
            && _fleetStartupOperation == operation && _fleetStartupActive && !_fleetStartupClosed && _fleetStartupStep == step
            && _fleetStartupTransportCurrent(true) && self && owner && link
            && _autopilotVersionObservationClock.elapsed() < _fleetStartupExpiresAtMs
            && owner->commandContextCurrent(self.data(), link.data(), startupId, stepId, int(step))
            && (step != FleetStartupStep::MissionModeOnly || executionContext.sourceEpoch.isEmpty()
                || (executionObserver && owner->executionObserverForQueue(self.data(), link.data(), startupId, stepId)
                    == executionObserver.data()))
            && self && owner && link && _fleetStartupEstimatorMemoryCurrent(true)
            && fencedEstimator.size() == 11 && _fleetStartupEstimatorProjection(true) == fencedEstimator
            && _fleetStartupGlobalOriginBasisCurrent() && _fleetStartupAltitudeCurrent();
    };
    const auto fail = [&](const QString& reason) {
        if (self && _fleetStartupOperation == operation && _fleetStartupStepId == stepId) _closeFleetStartupStep(reason);
        if (self && executionObserver && !executionContext.sourceEpoch.isEmpty()
            && self->_fleetExecutionObserver == executionObserver && self->_fleetExecutionEpoch == executionContext.sourceEpoch)
            self->_retireFleetExecutionObserver(QStringLiteral("execution-before-queue-rejected"));
        return denied(reason);
    };
    // The newly prepared, still-unissued slot changes the native group seal.
    // Revalidate it through the full authorization gate before asking the
    // memory-only command context to match that new prepared identity.
    if (!authorizationFence() || !current()) return fail(QStringLiteral("startup-before-plugin-fence-invalid"));
    mavlink_message_t message{};
    if (step == FleetStartupStep::Seq0) {
        mavlink_msg_mission_set_current_pack_chan(_mavlink->getSystemId(), _mavlink->getComponentId(), link->mavlinkChannel(), &message,
            _id, _defaultComponentId, 0); // Actual wire seq0. No legacy home-index remap.
    } else {
        mavlink_command_long_t item{};
        item.target_system = _id; item.target_component = _defaultComponentId; item.command = command; item.param1 = 1;
        if (step == FleetStartupStep::MissionModeOnly) { item.param2 = 4; item.param3 = 4; }
        mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(), _mavlink->getComponentId(), link->mavlinkChannel(), &message, &item);
    }
    const mavlink_message_t original = message;
    const auto immutable = [&]() {
        return current() && message.msgid == original.msgid && message.len == original.len && message.sysid == original.sysid
            && message.compid == original.compid && message.magic == original.magic && message.seq == original.seq
            && message.checksum == original.checksum && message.incompat_flags == original.incompat_flags
            && message.compat_flags == original.compat_flags && memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) == 0;
    };
    _firmwarePlugin->adjustOutgoingMavlinkMessageThreadSafe(this, link.data(), &message);
    if (!self || !owner || !link || !authorizationFence() || !immutable()) return fail(QStringLiteral("startup-after-plugin-fence-invalid"));
    if (!immutable() || !authorizationFence() || !immutable()) return fail(QStringLiteral("startup-before-queue-fence-invalid"));
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN) return fail(QStringLiteral("startup-packet-invalid"));
    if (!authorizationFence() || !immutable()) return fail(QStringLiteral("startup-final-queue-fence-invalid"));
    if (step == FleetStartupStep::MissionModeOnly) {
        executionObserver = owner->executionObserverForQueue(this, link.data(), startupId, stepId);
        if (!self || !owner || !link || !executionObserver)
            return fail(QStringLiteral("actual-native-execution-observer-required"));
        executionContext.startupId = startupId;
        executionContext.stepId = stepId;
        executionContext.operationId = QString::number(operation);
        executionContext.wirePayloadHash = startupRawHash(original);
        executionContext.vehicleId = _id;
        executionContext.componentId = _defaultComponentId;
        const bool bound = _bindFleetExecutionObserver(executionObserver.data(), link.data(), &executionContext);
        if (!self || !owner || !link || !executionObserver || !bound)
            return fail(QStringLiteral("native-execution-source-binding-rejected"));
        const quint64 executionRevision = _fleetExecutionSlotRevision;
        const QString executionEpoch(executionContext.sourceEpoch.constData(), executionContext.sourceEpoch.size());
        const auto executionSlotCurrent = [&]() {
            return self && owner && link && executionObserver && !self->_fleetExecutionSlotExhausted
                && self->_fleetExecutionSlotRevision == executionRevision
                && self->_fleetExecutionEpoch == executionEpoch && self->_fleetExecutionObserver == executionObserver
                && self->_fleetExecutionLink == link && executionContext.sourceEpoch == executionEpoch;
        };
        // Install the private source before the passive begin callback. A
        // nested RX is provisional until the captured actual-write commit.
        const bool accepted = executionObserver->beginExecutionQueue(this, link.data(), executionContext);
        if (!executionSlotCurrent() || !accepted)
            return fail(QStringLiteral("native-execution-after-begin-fence-invalid"));
        const bool beforeAuthorization = fleetExecutionBindingMatches(executionObserver.data(), executionEpoch);
        if (!executionSlotCurrent() || !beforeAuthorization)
            return fail(QStringLiteral("native-execution-after-begin-fence-invalid"));
        const bool authorized = authorizationFence();
        if (!executionSlotCurrent() || !authorized)
            return fail(QStringLiteral("native-execution-after-begin-fence-invalid"));
        const bool unchanged = immutable();
        if (!executionSlotCurrent() || !unchanged)
            return fail(QStringLiteral("native-execution-after-begin-fence-invalid"));
        const bool afterAuthorization = fleetExecutionBindingMatches(executionObserver.data(), executionEpoch);
        if (!executionSlotCurrent() || !afterAuthorization)
            return fail(QStringLiteral("native-execution-after-begin-fence-invalid"));
    }
    QObject::disconnect(_fleetStartupOwnerDestroyedConnection);
    const QString ownerEpoch = _fleetStartupOwnerEpoch;
    if (owner) _fleetStartupOwnerDestroyedConnection = connect(owner.data(), &QObject::destroyed, this, [this, startupId, ownerEpoch]() {
        if (_fleetStartupId != startupId || _fleetStartupOwnerEpoch != ownerEpoch) return;
        _fleetStartupScopeCancelled = true; _fleetStartupJournalStepId.clear();
        if (_fleetStartupAltitude.required && _fleetStartupAltitude.bound && !_fleetStartupAltitude.retired) {
            _fleetStartupAltitude.maximumGapMs = _fleetStartupAltitudeMaximumGap();
            _fleetStartupAltitude.endedAtMs = _startupAltitudeClock.isValid() ? _startupAltitudeClock.elapsed() : -1;
            _fleetStartupAltitude.retired = true;
        }
        for (auto record = _fleetStartupRetainedReceipts.constBegin(); record != _fleetStartupRetainedReceipts.constEnd(); ++record)
            if (_fleetStartupHistoryEndedAt.value(record.key(), -1) < 0) _fleetStartupHistoryEndedAt[record.key()] = _autopilotVersionObservationClock.elapsed();
        _closeFleetStartupStep(QStringLiteral("startup-owner-destroyed"));
    });
    if (!self || !owner || !link) return fail(QStringLiteral("startup-before-baseline-source-deleted"));
    // Freeze actual complete native baselines AFTER the final reentrant
    // authorization callback. Their native source checks may call Link
    // virtual getters: retain QQ guards and the immutable/final pure checks.
    // Ordinary age advancement is not a content replacement or renewal.
    const QVariantMap queueFlight = flightStateRiskObservation();
    if (!self || !owner || !link) return fail(QStringLiteral("startup-flight-baseline-source-deleted"));
    const QVariantMap queueCoordinates = startupCoordinateObservation();
    if (!self || !owner || !link) return fail(QStringLiteral("startup-coordinate-baseline-source-deleted"));
    const QVariantMap queueEstimator = _fleetStartupEstimatorProjection(true);
    const bool odometryContinuityRequired = owner->odometryContinuityRequired(self.data(), startupId);
    if (odometryContinuityRequired && (!startupOdometryObservationSourceMatches(link.data())
            || !startupOdometryNedObservationMatches(_startupOdometryRow.value)
            || _startupOdometryRow.receivedAtMs < 0
            || _startupOdometryRow.receivedAtMs > _autopilotVersionObservationClock.elapsed()))
        return fail(QStringLiteral("actual-reviewed-native-odometry-periodic-baseline-required"));
    if (!immutable() || queueEstimator.size() != 11 || queueEstimator != fencedEstimator
        || !_fleetStartupEstimatorMemoryCurrent(true)) return fail(QStringLiteral("startup-queue-baseline-changed"));
    const qint64 issuedAtMs = _autopilotVersionObservationClock.elapsed();
    QMap<QString, QString> riskHashes;
    QMap<QString, qint64> periodic;
    bool gapKnown = true;
    for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
                                  QStringLiteral("landedState"), QStringLiteral("missionCurrent")}) {
        const auto row = _flightStateObservations.constFind(channel);
        const qint64 received = row != _flightStateObservations.constEnd() && row->observed && row->complete && !row->updating ? row->receivedAtMs : -1;
        periodic[channel] = received; gapKnown = gapKnown && received >= 0 && received <= issuedAtMs;
    }
    const StartupCoordinateRow localRow = _startupCoordinateRows.value(QStringLiteral("localPosition"));
    periodic[QStringLiteral("localPosition")] = localRow.observed && localRow.complete && !localRow.updating ? localRow.receivedAtMs : -1;
    gapKnown = gapKnown && periodic.value(QStringLiteral("localPosition")) >= 0 && periodic.value(QStringLiteral("localPosition")) <= issuedAtMs;
    const FleetHealthTelemetryChannel estimatorRow = _fleetHealthTelemetryRows.value(QStringLiteral("estimatorStatus"));
    periodic[QStringLiteral("estimatorStatus")] = estimatorRow.observed && estimatorRow.complete && !estimatorRow.updating
        && estimatorRow.receiptSequence != 0 ? estimatorRow.startupReceivedAtMs : -1;
    gapKnown = gapKnown && periodic.value(QStringLiteral("estimatorStatus")) >= 0
        && periodic.value(QStringLiteral("estimatorStatus")) <= issuedAtMs;
    if (odometryContinuityRequired) {
        const qint64 received = _startupOdometryRow.observed && _startupOdometryRow.complete && !_startupOdometryRow.updating
            && _startupOdometryRow.receiptSequence != 0 ? _startupOdometryRow.receivedAtMs : -1;
        periodic[QStringLiteral("odometry")] = received;
        gapKnown = gapKnown && received >= 0 && received <= issuedAtMs;
    }
    qint64 maximumGap = gapKnown ? 0 : -1;
    if (gapKnown) for (qint64 received : periodic) maximumGap = qMax(maximumGap, issuedAtMs - received);
    const QVariantMap flightRows = queueFlight.value(QStringLiteral("channels")).toMap();
    for (auto row = flightRows.constBegin(); row != flightRows.constEnd(); ++row)
        riskHashes[row.key()] = row.value().toMap().value(QStringLiteral("riskContentHash")).toString();
    const QVariantMap coordinateRows = queueCoordinates.value(QStringLiteral("channels")).toMap();
    riskHashes[QStringLiteral("localPosition")] = coordinateRows.value(QStringLiteral("localPosition")).toMap().value(QStringLiteral("riskContentHash")).toString();
    riskHashes[QStringLiteral("homeCoordinates")] = coordinateRows.value(QStringLiteral("homePosition")).toMap().value(QStringLiteral("riskContentHash")).toString();
    riskHashes[QStringLiteral("estimatorStatus")] = startupValueHash(queueEstimator);
    // No virtual/query/authorization callback follows this EST check. The
    // frozen budget is the actual transaction policy; neither another healthy
    // packet nor an expired age can silently rebase this command's evidence.
    if (!self || !owner || !link || _fleetStartupOwner != owner || _fleetStartupScopeCancelled
        || _fleetStartupOperation != operation || _fleetStartupStepId != stepId || !_fleetStartupActive || _fleetStartupClosed
        || _autopilotVersionObservationClock.elapsed() >= _fleetStartupExpiresAtMs
        || !owner->commandContextCurrent(self.data(), link.data(), startupId, stepId, int(step))
        || !_fleetStartupEstimatorMemoryCurrent(true)
        || _fleetStartupEstimatorProjection(true) != fencedEstimator || !_fleetStartupGlobalOriginBasisCurrent()
        || !_fleetStartupAltitudeCurrent())
        return fail(QStringLiteral("startup-final-estimator-evidence-changed"));
    // Only now seal the previous interval and install this packet's journal.
    // A rejected pre-queue callback/pure check must not end old history or
    // manufacture a new interval without an actual write invocation.
    _fleetStartupBaselineReceipt = _fleetStartupRawReceiptSequence;
    _fleetStartupIssuedAtMs = issuedAtMs;
    _fleetStartupQueueFlightObservation = queueFlight; _fleetStartupQueueCoordinateObservation = queueCoordinates;
    _fleetStartupQueueEstimatorObservation = queueEstimator;
    _fleetStartupWireHash = startupRawHash(original);
    const QString previousJournalStep = _fleetStartupJournalStepId;
    if (!previousJournalStep.isEmpty()) _fleetStartupHistoryEndedAt[previousJournalStep] = issuedAtMs;
    _fleetStartupJournalStepId = stepId;
    _fleetStartupOdometryContinuityRequired = odometryContinuityRequired;
    _fleetStartupJournalSequence = 0; _fleetStartupLastRiskHash = riskHashes;
    _fleetStartupRawLastAt[stepId] = periodic;
    _fleetStartupMaximumRawGap[stepId] = maximumGap; _fleetStartupHistoryEndedAt[stepId] = -1;
    _snapshotFleetStartupStep();
    // Prepare the native journal before entering Link's write signal: a
    // direct transport observer can run a nested receive. Such observations
    // are retained, but no requestIssued flag is published until this actual
    // write invocation has returned. There is still no request-causality claim.
    _fleetStartupQueueInProgress = true;
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    if (!self) {
        // The actual write invocation happened even if a nested transport
        // callback destroyed Vehicle. Do not dereference this/owner/link here.
        if (executionObserver) executionObserver->executionQueueCommitted(executionContext);
        return denied(QStringLiteral("startup-owner-deleted-after-queue"), true);
    }
    _fleetStartupQueueInProgress = false;
    _fleetStartupRequestIssued = true;
    _snapshotFleetStartupStep(); // BEFORE messagesSent or any public observer.
    // Report the captured write fact even after nested source retirement.
    // It neither requires a live binding nor manufactures request causality.
    if (executionObserver) executionObserver->executionQueueCommitted(executionContext);
    if (!self) return denied(QStringLiteral("startup-owner-deleted-after-execution-commit"), true);
    if (_fleetStartupClosed && step != FleetStartupStep::Seq0) _quarantineMavCommand(_defaultComponentId, command, 4000);
    QTimer::singleShot(4000, this, [self, operation, stepId]() {
        if (self && self->_fleetStartupOperation == operation && self->_fleetStartupStepId == stepId && self->_fleetStartupActive) {
            self->_closeFleetStartupStep(QStringLiteral("step-deadline-expired"));
            emit self->fleetStartupStepObservationChanged();
        }
    });
    ++_messagesSent;
    emit messagesSentChanged();
    if (!self || !owner || !link) return denied(QStringLiteral("startup-source-deleted-after-queue"), true);
    QVariantMap receipt = fleetStartupStepReceipt(owner.data(), startupId, stepId);
    if (receipt.isEmpty()) return denied(QStringLiteral("startup-scope-changed-after-queue"), true);
    return receipt;
}

void Vehicle::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    QPointer<Vehicle> receiveOwner(this);
    // Independent 500 fence covers the ENTIRE original RAW stack, including
    // packets that do not participate in any existing P0/passive ledger.
    // Exhaustion only disables this capture service, never drops an old RAW.
    bool intervalRawDepthHeld = false;
    if (QThread::currentThread() == thread()) {
        if (_fleetMessageIntervalRawReceiveDepth == std::numeric_limits<quint64>::max())
            _fleetMessageIntervalRawReceiveExhausted = true;
        else { ++_fleetMessageIntervalRawReceiveDepth; intervalRawDepthHeld = true; }
    }
    const auto releaseIntervalRaw = qScopeGuard([receiveOwner, intervalRawDepthHeld]() {
        if (receiveOwner && intervalRawDepthHeld && receiveOwner->_fleetMessageIntervalRawReceiveDepth > 0)
            --receiveOwner->_fleetMessageIntervalRawReceiveDepth;
    });
    QPointer<LinkInterface> receivedLink(link);
    // Freeze the raw entry before any firmware/core callback can adjust it.
    const mavlink_message_t originalMessage = message;
    // Capture only an import that already existed at this original RAW entry.
    // Pin the actual child, not a raw manager address after a later callback.
    QPointer<ParameterManager> originalImportParameters;
    for (QObject* child : children()) {
        if (child == _parameterManager) { originalImportParameters = qobject_cast<ParameterManager*>(child); break; }
    }
    const auto originalImportReceipt = originalImportParameters
        ? originalImportParameters->_captureParameterImportReceipt(originalMessage, receivedLink.data())
        : std::shared_ptr<ParameterManager::ParameterImportReceipt>();
    const FleetBatteryDrainIngress batteryDrainIngress = _prepareFleetBatteryDrainIngress(originalMessage, receivedLink.data());
    // A packet can only answer the run that was already possibly written at
    // this earliest RAW entry. Later probes/notifications may start a new run.
    const auto originalLatencyRun = _fleetLinkLatencyRun;
    const auto originalCameraRun = _fleetCameraStreamRun;
    const auto originalCameraControlRun = _fleetCameraStreamControlRun;
    const QString originalCameraControlRequest = originalCameraControlRun ? originalCameraControlRun->queryId : QStringLiteral("");
    const bool originalCameraControlIssued = originalCameraControlRun && originalCameraControlRun->active
        && originalCameraControlRun->stage == 1 && originalCameraControlRun->queryIssued;
    const quint64 originalCameraControlBaseline = originalCameraControlRun ? originalCameraControlRun->baseline : 0;
    const int originalCameraStage = originalCameraRun && originalCameraRun->active ? originalCameraRun->stage : -1;
    const bool originalCameraStageValid = originalCameraStage >= 0 && originalCameraStage < 2;
    const QString originalCameraRequest = originalCameraStageValid ? originalCameraRun->queries[originalCameraStage].id : QStringLiteral("");
    const bool originalCameraIssued = originalCameraStageValid && originalCameraRun->queries[originalCameraStage].requestIssued;
    const quint64 originalCameraBaseline = originalCameraStageValid ? originalCameraRun->queries[originalCameraStage].baseline : 0;
    const QString originalCameraEpoch = originalCameraRun ? originalCameraRun->sourceEpoch : QStringLiteral("");
    const bool originalLatencyWriteStarted = originalLatencyRun && originalLatencyRun->requestIssued;
    const qint64 originalLatencyNonce = originalLatencyRun ? originalLatencyRun->nonce : 0;
    const qint64 latencyReceivedAtNs = originalLatencyRun && originalLatencyRun->clock.isValid()
        ? originalLatencyRun->clock.nsecsElapsed() : -1;
    const qint64 rawReceivedAtMs = _autopilotVersionObservationClock.isValid() ? _autopilotVersionObservationClock.elapsed() : -1;
    const qint64 localReceptionReceivedAtMs = _fleetLocalReceptionClock.isValid() ? _fleetLocalReceptionClock.elapsed() : -1;
    const FleetLocalReceptionIngress localReceptionIngress =
        _prepareFleetLocalReceptionIngress(originalMessage, receivedLink.data(), localReceptionReceivedAtMs);
    const qint64 cameraStreamReceivedAtMs = _fleetCameraStreamClock.isValid() ? _fleetCameraStreamClock.elapsed() : -1;
    const FleetCameraStreamIngress cameraStreamIngress =
        _prepareFleetCameraStreamIngress(originalMessage, receivedLink.data(), cameraStreamReceivedAtMs);
    const auto discardCameraStreamIngress = qScopeGuard([receiveOwner, cameraStreamIngress]() {
        if (receiveOwner && cameraStreamIngress.eligible
            && receiveOwner->_fleetCameraStreamEpoch == cameraStreamIngress.sourceEpoch
            && receiveOwner->_fleetCameraStreamNextReceipt == cameraStreamIngress.receiptSequence)
            receiveOwner->_invalidateFleetCameraStreamObservation(QStringLiteral("camera-stream-discarded-raw"));
    });
    const auto discardLocalReceptionIngress = qScopeGuard([receiveOwner, localReceptionIngress]() {
        // Existing RAW branches may return before the late observer. Missing
        // an eligible original must not silently leave a complete prefix.
        if (receiveOwner && localReceptionIngress.eligible
            && receiveOwner->_fleetLocalReceptionEpoch == localReceptionIngress.sourceEpoch
            && receiveOwner->_fleetLocalReceptionNextReceipt == localReceptionIngress.receiptSequence)
            receiveOwner->_invalidateFleetLocalReceptionObservation(QStringLiteral("local-reception-discarded-raw"));
    });
    // These pins were established when the actual providers were created,
    // not reconstructed from a possibly dangling raw field after a result.
    const QPointer<FirmwarePlugin> receivePlugin(_messageIntervalSourcePlugin);
    const QPointer<QGCToolbox> receiveToolbox(_fleetExecutionSourceToolbox);
    const QPointer<MAVLinkProtocol> receiveProtocol(_fleetExecutionSourceProtocol);
    const QPointer<VehicleLinkManager> receiveLinks(_fleetExecutionSourceLinks);
    const QPointer<QGCCorePlugin> receiveCore(_messageCommandSourceCore);
    const QPointer<TerrainProtocolHandler> receiveTerrain(_messageCommandSourceTerrain);
    const QPointer<FTPManager> receiveFtp(_messageCommandSourceFtp);
    const QPointer<ImageProtocolManager> receiveImage(_messageCommandSourceImage);
    const QPointer<RemoteIDManager> receiveRemoteId(_messageCommandSourceRemoteId);
    const QPointer<ParameterManager> receiveParameters(_parameterManager);
    const bool hadReceiveParameters = bool(receiveParameters);
    const auto receiveProvidersCurrent = [&]() {
        return receiveOwner && receivedLink && receivePlugin && receiveToolbox && receiveProtocol && receiveLinks
            && receiveCore && receiveTerrain && receiveFtp && receiveImage && receiveRemoteId
            && receiveOwner->_messageIntervalSourcePlugin == receivePlugin && receiveOwner->_firmwarePlugin == receivePlugin.data()
            && receiveOwner->_fleetExecutionSourceToolbox == receiveToolbox && receiveOwner->_toolbox == receiveToolbox.data()
            && receiveOwner->_fleetExecutionSourceProtocol == receiveProtocol && receiveOwner->_mavlink == receiveProtocol.data()
            && receiveOwner->_fleetExecutionSourceLinks == receiveLinks && receiveOwner->_vehicleLinkManager == receiveLinks.data()
            && receiveOwner->_messageCommandSourceCore == receiveCore && receiveToolbox->corePlugin() == receiveCore.data()
            && receiveToolbox->mavlinkProtocol() == receiveProtocol.data()
            && receiveOwner->_messageCommandSourceTerrain == receiveTerrain && receiveOwner->_terrainProtocolHandler == receiveTerrain.data()
            && receiveOwner->_messageCommandSourceFtp == receiveFtp && receiveOwner->_ftpManager == receiveFtp.data()
            && receiveOwner->_messageCommandSourceImage == receiveImage && receiveOwner->_imageProtocolManager == receiveImage.data()
            && receiveOwner->_messageCommandSourceRemoteId == receiveRemoteId && receiveOwner->_remoteIDManager == receiveRemoteId.data()
            && (!hadReceiveParameters || receiveParameters) && receiveOwner->_parameterManager == receiveParameters;
    };
    // Freeze the already-written request at raw entry, before ANY probe can
    // synchronously create a newer same-message request. Dispatch waits until
    // all existing P0 receive fences/prepares below have been established.
    const QString originalMessageLaneId = _messageCommandLaneId;
    const quint64 originalMessageLaneSerial = _messageCommandLaneSerial;
    const auto originalRequestContext = _requestMessageContext;
    const auto originalIntervalQuery = _fleetMessageIntervalActiveQuery;
    const int originalMessageIndex = _messageCommandIndex(originalMessageLaneId);
    const bool originalCommandEligible = originalMessageIndex >= 0
        && _mavCommandList.at(originalMessageIndex).messageCommandWriteStarted
        && sender() == _fleetExecutionSourceProtocol.data()
        && originalMessage.sysid == _id
        && originalMessage.compid == _mavCommandList.at(originalMessageIndex).targetCompId
        && receivedLink == _mavCommandList.at(originalMessageIndex).messageIntervalPrimary
        && _messageIntervalSourceCurrent(_mavCommandList.at(originalMessageIndex));
    const bool originalRequestEligible = originalRequestContext && originalRequestContext->source
        && originalRequestContext->source->messageCommandWriteStarted
        && sender() == _fleetExecutionSourceProtocol.data() && originalMessage.sysid == _id
        && originalMessage.compid == originalRequestContext->compId
        && receivedLink == originalRequestContext->source->messageIntervalPrimary
        && _messageIntervalSourceCurrent(*originalRequestContext->source);
    const bool originalIntervalQueryEligible = originalIntervalQuery && originalIntervalQuery->active
        && originalIntervalQuery->source.messageCommandWriteStarted
        && originalIntervalQuery->source.messageIntervalRequestId == originalMessageLaneId
        && sender() == originalIntervalQuery->source.messageIntervalProtocol.data()
        && originalMessage.sysid == originalIntervalQuery->source.messageCommandVehicleId
        && originalMessage.compid == originalIntervalQuery->source.targetCompId
        && receivedLink == originalIntervalQuery->source.messageIntervalPrimary
        && _fleetMessageIntervalQuerySourceCurrent(originalIntervalQuery, false);
    // Hold this independent PRIMARY-only ledger before ANY other prepare,
    // observer or plugin can enter a callback. Its public projection stays
    // unavailable until the entire original receive stack has unwound.
    const bool optionalTelemetryFrame = optionalTelemetryFrameId(originalMessage.msgid)
        && sender() == _fleetExecutionSourceProtocol.data() && originalMessage.sysid == _id && originalMessage.compid == _defaultComponentId
        && _optionalTelemetryRawSourceMatches(receivedLink.data());
    bool optionalTelemetryDepthHeld = false;
    if (optionalTelemetryFrame) {
        if (_optionalTelemetryReceiveDepth == std::numeric_limits<quint64>::max()) {
            _optionalTelemetryExhausted = true;
            _invalidateOptionalTelemetryObservations();
        } else {
            ++_optionalTelemetryReceiveDepth;
            optionalTelemetryDepthHeld = true;
        }
    }
    const auto releaseOptionalTelemetryReceive = qScopeGuard([receiveOwner, optionalTelemetryDepthHeld]() {
        if (receiveOwner && optionalTelemetryDepthHeld && receiveOwner->_optionalTelemetryReceiveDepth > 0)
            --receiveOwner->_optionalTelemetryReceiveDepth;
    });
    if (optionalTelemetryDepthHeld) {
        const qint64 receivedAtMs = _optionalTelemetryClock.isValid() ? _optionalTelemetryClock.elapsed() : -1;
        _observeOptionalTelemetry(originalMessage, receivedLink.data(), receivedAtMs);
        if (!receiveOwner || !receivedLink) return;
    }
    const bool startupFrame = QThread::currentThread() == thread()
        && (originalMessage.msgid == MAVLINK_MSG_ID_COMMAND_ACK || originalMessage.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED
            || originalMessage.msgid == MAVLINK_MSG_ID_HOME_POSITION || originalMessage.msgid == MAVLINK_MSG_ID_ODOMETRY
            || originalMessage.msgid == MAVLINK_MSG_ID_HEARTBEAT || originalMessage.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT
            || originalMessage.msgid == MAVLINK_MSG_ID_EXTENDED_SYS_STATE || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_CURRENT
            || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ITEM_REACHED);
    FleetStartupRawIngress startupIngress;
    if (startupFrame) {
        startupIngress.owner = _fleetStartupOwner; startupIngress.primary = _fleetStartupLink; startupIngress.protocol = _mavlink;
        startupIngress.startupId = _fleetStartupId; startupIngress.ownerEpoch = _fleetStartupOwnerEpoch;
        startupIngress.journalStepId = _fleetStartupJournalStepId;
        startupIngress.versionEpoch = _fleetStartupVersionEpoch; startupIngress.flightEpoch = _fleetStartupFlightEpoch;
        startupIngress.healthEpoch = _fleetStartupEstimatorSourceEpoch;
        startupIngress.vehicleId = _id; startupIngress.componentId = _defaultComponentId;
        startupIngress.receivedAtMs = rawReceivedAtMs; startupIngress.rawSequence = _fleetStartupRawReceiptSequence;
        // The journal can still be the PREVIOUS closed step while a new slot
        // is prepared. Capture that receipt's operation, never the new slot's.
        const QVariantMap record = _fleetStartupRetainedReceipts.value(startupIngress.journalStepId);
        startupIngress.operationId = record.value(QStringLiteral("operationId")).toString();
        startupIngress.issuedAtMs = record.value(QStringLiteral("issuedAtMonotonicMs"));
        startupIngress.eligible = sender() == _mavlink && startupIngress.owner && receivedLink
            && receivedLink == startupIngress.primary && !_fleetStartupScopeCancelled
            && originalMessage.sysid == _id && originalMessage.compid == _defaultComponentId
            && (originalMessage.msgid != MAVLINK_MSG_ID_ODOMETRY || _fleetStartupOdometryContinuityRequired)
            && !startupIngress.journalStepId.isEmpty() && !startupIngress.operationId.isEmpty() && record.size() == 33
            && (record.value(QStringLiteral("requestIssued")).toBool()
                || (_fleetStartupQueueInProgress && startupIngress.journalStepId == _fleetStartupStepId));
        if (_fleetStartupReceiveDepth == std::numeric_limits<quint64>::max()) {
            _fleetStartupExhausted = true; _discardFleetStartupRawIngress(startupIngress); return;
        }
        ++_fleetStartupReceiveDepth;
        if (_fleetStartupReceiveDepth != 1) _discardFleetStartupRawIngress(startupIngress);
    }
    bool startupIngressHandled = false;
    const auto releaseStartupReceive = qScopeGuard([receiveOwner, startupFrame, &startupIngress, &startupIngressHandled]() {
        if (!receiveOwner || !startupFrame) return;
        // An early return/throw before the native handler cannot silently
        // drop an eligible original packet and later publish a healthy A.
        if (!startupIngressHandled) receiveOwner->_discardFleetStartupRawIngress(startupIngress);
        if (receiveOwner->_fleetStartupReceiveDepth > 0) --receiveOwner->_fleetStartupReceiveDepth;
    });
    // This independent event-source fence starts before telemetry/execution,
    // health or plugin callbacks, and lasts through this entire receive stack.
    const bool originFrame = QThread::currentThread() == thread() && originalMessage.msgid == MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN;
    const qint64 originReceivedAtMs = originFrame && _startupGlobalOriginClock.isValid() ? _startupGlobalOriginClock.elapsed() : -1;
    quint64 originToken = 0;
    QString originEpoch;
    if (originFrame) {
        if (_startupGlobalOriginReceiveDepth == std::numeric_limits<quint64>::max()) {
            _startupGlobalOriginExhausted = true;
            _invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-receive-stack-exhausted")); return;
        }
        ++_startupGlobalOriginReceiveDepth;
    }
    const auto releaseOriginReceive = qScopeGuard([receiveOwner, originFrame, &originToken, &originEpoch]() {
        if (!receiveOwner) return;
        if (originToken && receiveOwner->_startupGlobalOriginEpoch == originEpoch
            && receiveOwner->_startupGlobalOriginRow.updateToken == originToken && receiveOwner->_startupGlobalOriginRow.updating)
            receiveOwner->_invalidateStartupGlobalOriginObservation(QStringLiteral("global-origin-discarded-raw-receipt"));
        if (originFrame && receiveOwner->_startupGlobalOriginReceiveDepth > 0) --receiveOwner->_startupGlobalOriginReceiveDepth;
    });
    // Hold BOTH independent ledgers before either prepare can enter a Link
    // callback. Legacy v1-v3 remain diagnostic-only; explicit v4 consumes
    // the independent monitor without changing either raw ledger contract.
    const bool altitudeFrame = QThread::currentThread() == thread() && originalMessage.msgid == MAVLINK_MSG_ID_ALTITUDE;
    const qint64 altitudeReceivedAtMs = altitudeFrame && _startupAltitudeClock.isValid() ? _startupAltitudeClock.elapsed() : -1;
    quint64 altitudeToken = 0;
    QString altitudeEpoch;
    if (altitudeFrame) {
        if (_startupAltitudeReceiveDepth == std::numeric_limits<quint64>::max()) {
            _startupAltitudeExhausted = true;
            _invalidateStartupAltitudeObservation(QStringLiteral("altitude-receive-stack-exhausted"), false); return;
        }
        ++_startupAltitudeReceiveDepth;
    }
    const auto releaseAltitudeReceive = qScopeGuard([receiveOwner, altitudeFrame, &altitudeToken, &altitudeEpoch]() {
        if (!receiveOwner) return;
        if (altitudeToken && receiveOwner->_startupAltitudeEpoch == altitudeEpoch
            && receiveOwner->_startupAltitudeRow.updateToken == altitudeToken && receiveOwner->_startupAltitudeRow.updating)
            receiveOwner->_invalidateStartupAltitudeObservation(QStringLiteral("altitude-discarded-raw-receipt"), false);
        if (altitudeFrame && receiveOwner->_startupAltitudeReceiveDepth > 0) --receiveOwner->_startupAltitudeReceiveDepth;
    });
    originToken = _prepareStartupGlobalOriginObservation(originalMessage, receivedLink.data());
    if (!receiveOwner || !receivedLink) return;
    originEpoch = _startupGlobalOriginEpoch;
    _completeStartupGlobalOriginObservation(originToken, originEpoch, originalMessage, receivedLink.data(), originReceivedAtMs);
    if (!receiveOwner || !receivedLink) return;
    altitudeToken = _prepareStartupAltitudeObservation(originalMessage, receivedLink.data(), altitudeReceivedAtMs);
    if (!receiveOwner || !receivedLink) return;
    altitudeEpoch = _startupAltitudeEpoch;
    _completeStartupAltitudeObservation(altitudeToken, altitudeEpoch, originalMessage, receivedLink.data(), altitudeReceivedAtMs);
    if (!receiveOwner || !receivedLink) return;
    const bool batteryFrame = QThread::currentThread() == thread() && originalMessage.msgid == MAVLINK_MSG_ID_BATTERY_STATUS;
    const qint64 batteryReceivedAtMs = batteryFrame && _fleetBatteryTelemetryClock.isValid() ? _fleetBatteryTelemetryClock.elapsed() : -1;
    int batteryId = -1;
    QString batteryEpoch;
    QVariantMap batteryValue;
    quint64 batteryToken = 0;
    if (batteryFrame) {
        if (_fleetBatteryTelemetryReceiveDepth == std::numeric_limits<quint64>::max()) {
            _fleetBatteryTelemetryExhausted = true; _invalidateFleetBatteryTelemetryObservation(); return;
        }
        ++_fleetBatteryTelemetryReceiveDepth;
    }
    const auto releaseBatteryReceive = qScopeGuard([receiveOwner, batteryFrame, &batteryId, &batteryEpoch, &batteryToken]() {
        if (!receiveOwner) return;
        if (batteryToken && receiveOwner->_fleetBatteryTelemetryEpoch == batteryEpoch) {
            const auto row = receiveOwner->_fleetBatteryTelemetryRows.constFind(batteryId);
            if (row != receiveOwner->_fleetBatteryTelemetryRows.constEnd() && row->updateToken == batteryToken && row->updating)
                receiveOwner->_invalidateFleetBatteryTelemetryObservation();
        }
        if (batteryFrame && receiveOwner->_fleetBatteryTelemetryReceiveDepth > 0) --receiveOwner->_fleetBatteryTelemetryReceiveDepth;
    });
    batteryToken = _prepareFleetBatteryTelemetryObservation(originalMessage, receivedLink.data(), &batteryId, &batteryValue);
    if (!receiveOwner || !receivedLink) return;
    batteryEpoch = _fleetBatteryTelemetryEpoch;
    _completeFleetBatteryTelemetryObservation(batteryId, batteryToken, batteryEpoch, batteryValue, receivedLink.data(), batteryReceivedAtMs, batteryDrainIngress);
    if (!receiveOwner || !receivedLink) return;
    // The committed report stays unavailable to public projections while
    // any callback is still inside this original BATTERY_STATUS receive.
    const bool healthFrame = QThread::currentThread() == thread()
        && (originalMessage.msgid == MAVLINK_MSG_ID_SYS_STATUS || originalMessage.msgid == MAVLINK_MSG_ID_ESTIMATOR_STATUS);
    const qint64 healthReceivedAtMs = healthFrame && _fleetHealthTelemetryClock.isValid() ? _fleetHealthTelemetryClock.elapsed() : -1;
    const qint64 startupHealthReceivedAtMs = healthFrame && _autopilotVersionObservationClock.isValid()
        ? _autopilotVersionObservationClock.elapsed() : -1;
    FleetStartupEstimatorIngress estimatorIngress;
    if (healthFrame && originalMessage.msgid == MAVLINK_MSG_ID_ESTIMATOR_STATUS) {
        estimatorIngress.owner = _fleetStartupOwner;
        estimatorIngress.startupId = _fleetStartupId;
        estimatorIngress.ownerEpoch = _fleetStartupOwnerEpoch;
        estimatorIngress.journalStepId = _fleetStartupJournalStepId;
        estimatorIngress.operationId = _fleetStartupRetainedReceipts.value(_fleetStartupJournalStepId)
            .value(QStringLiteral("operationId")).toString();
        estimatorIngress.versionEpoch = _fleetStartupVersionEpoch;
        estimatorIngress.flightEpoch = _fleetStartupFlightEpoch;
        estimatorIngress.healthEpoch = _fleetHealthTelemetryEpoch;
    }
    QString healthChannel, healthEpoch;
    QVariantMap healthValue;
    quint64 healthToken = 0;
    if (healthFrame) {
        if (_fleetHealthTelemetryReceiveDepth == std::numeric_limits<quint64>::max()) {
            _fleetHealthTelemetryExhausted = true;
            _invalidateFleetHealthTelemetryObservation(QStringLiteral("health-receive-stack-exhausted")); return;
        }
        ++_fleetHealthTelemetryReceiveDepth;
    }
    const auto releaseHealthReceive = qScopeGuard([receiveOwner, healthFrame, &healthChannel, &healthEpoch, &healthToken]() {
        if (!receiveOwner) return;
        if (healthToken && receiveOwner->_fleetHealthTelemetryEpoch == healthEpoch) {
            const auto row = receiveOwner->_fleetHealthTelemetryRows.constFind(healthChannel);
            if (row != receiveOwner->_fleetHealthTelemetryRows.constEnd() && row->updateToken == healthToken && row->updating)
                receiveOwner->_invalidateFleetHealthTelemetryObservation(QStringLiteral("health-discarded-raw-receipt"));
        }
        if (healthFrame && receiveOwner->_fleetHealthTelemetryReceiveDepth > 0) --receiveOwner->_fleetHealthTelemetryReceiveDepth;
    });
    healthToken = _prepareFleetHealthTelemetryObservation(originalMessage, receivedLink.data(), &healthChannel, &healthValue);
    if (!receiveOwner || !receivedLink) return;
    healthEpoch = _fleetHealthTelemetryEpoch;
    _completeFleetHealthTelemetryObservation(healthChannel, healthToken, healthEpoch, healthValue, receivedLink.data(),
        healthReceivedAtMs, startupHealthReceivedAtMs);
    if (!receiveOwner || !receivedLink) return;
    if (healthFrame && originalMessage.msgid == MAVLINK_MSG_ID_ESTIMATOR_STATUS) {
        _handleFleetStartupEstimatorRawMessage(originalMessage, receivedLink.data(), startupHealthReceivedAtMs,
            estimatorIngress, healthToken, healthEpoch);
        if (!receiveOwner || !receivedLink) return;
    }
    // Commit the original health packet before plugin/observer callbacks, but
    // keep its stack held until this receive returns. Queries during a nested
    // callback cannot publish that still-in-flight source as current/healthy.
    _observeFleetTelemetry(originalMessage, receivedLink.data());
    if (!receiveOwner || !receivedLink) return;
    _observeFleetExecution(originalMessage, receivedLink.data());
    if (!receiveOwner || !receivedLink) return;
    const bool odometryFrame = originalMessage.msgid == MAVLINK_MSG_ID_ODOMETRY;
    if (odometryFrame) {
        if (_startupOdometryReceiveDepth == std::numeric_limits<quint64>::max()) {
            _startupOdometryExhausted = true;
            _invalidateStartupOdometry(QStringLiteral("odometry-receive-stack-exhausted")); return;
        }
        ++_startupOdometryReceiveDepth;
    }
    QVariantMap odometryValue;
    const quint64 odometryToken = _prepareStartupOdometryObservation(originalMessage, receivedLink.data(), &odometryValue);
    const QString odometryEpoch = _startupOdometryEpoch;
    const auto discardOdometryUpdate = qScopeGuard([receiveOwner, odometryFrame, odometryToken, odometryEpoch]() {
        if (!receiveOwner) return;
        if (odometryToken && odometryEpoch == receiveOwner->_startupOdometryEpoch
            && receiveOwner->_startupOdometryRow.updateToken == odometryToken && receiveOwner->_startupOdometryRow.updating) {
            receiveOwner->_invalidateStartupOdometry(QStringLiteral("odometry-receive-discarded"));
        }
        if (odometryFrame && receiveOwner->_startupOdometryReceiveDepth > 0) --receiveOwner->_startupOdometryReceiveDepth;
    });
    const bool missionFrame = originalMessage.msgid == MAVLINK_MSG_ID_MISSION_COUNT
        || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ITEM_INT || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ACK
        || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ITEM;
    if (missionFrame) {
        if (_fleetMissionObservationReceiveDepth == std::numeric_limits<quint64>::max()) {
            _fleetMissionObservationReceiveExhausted = true;
            return;
        }
        ++_fleetMissionObservationReceiveDepth;
    }
    const auto releaseMissionReceive = qScopeGuard([receiveOwner, missionFrame]() {
        if (receiveOwner && missionFrame && receiveOwner->_fleetMissionObservationReceiveDepth > 0)
            --receiveOwner->_fleetMissionObservationReceiveDepth;
    });
    quint64 versionToken = originalMessage.msgid == MAVLINK_MSG_ID_AUTOPILOT_VERSION
        ? _prepareAutopilotVersionObservation(originalMessage, receivedLink.data()) : 0;
    if (versionToken != 0) {
        if (_autopilotVersionObservationReceiveDepth == std::numeric_limits<quint64>::max()) {
            _autopilotVersionObservationExhausted = true;
            _invalidateAutopilotVersionObservation(QStringLiteral("observation-exhausted"));
            versionToken = 0;
        } else {
            ++_autopilotVersionObservationReceiveDepth;
        }
    }
    const bool versionDepthHeld = versionToken != 0;
    const QString versionEpoch = _autopilotVersionObservationEpoch;
    const auto discardVersionUpdate = qScopeGuard([receiveOwner, versionToken, versionEpoch, versionDepthHeld]() {
        if (!receiveOwner) return;
        if (receiveOwner && receiveOwner->_autopilotVersionObservationUpdateCurrent(versionToken, versionEpoch)) {
            receiveOwner->_autopilotVersionObservationUpdating = false;
            receiveOwner->_autopilotVersionObservationComplete = false;
        }
        // Keep latest nested raw fields, but do not report a quiescent live
        // source until the older outer version callback stack has unwound.
        if (versionDepthHeld && receiveOwner->_autopilotVersionObservationReceiveDepth > 0) {
            --receiveOwner->_autopilotVersionObservationReceiveDepth;
        }
    });
    QString flightChannel;
    QVariantMap flightValue;
    quint64 flightToken = _prepareFlightStateObservation(originalMessage, receivedLink.data(), &flightChannel, &flightValue);
    if (!receiveOwner || !receivedLink) return;
    if (flightToken != 0) {
        if (_flightStateObservationReceiveDepth == std::numeric_limits<quint64>::max()) {
            _flightStateObservationExhausted = true;
            _invalidateFlightStateObservations(QStringLiteral("observation-exhausted"));
            flightToken = 0;
        } else {
            ++_flightStateObservationReceiveDepth;
        }
    }
    const bool flightDepthHeld = flightToken != 0;
    const QString flightEpoch = _flightStateObservationEpoch;
    const auto discardFlightUpdate = qScopeGuard([receiveOwner, flightChannel, flightToken, flightEpoch, flightDepthHeld]() {
        if (!receiveOwner) return;
        if (receiveOwner->_flightStateObservationUpdateCurrent(flightChannel, flightToken, flightEpoch)) {
            auto row = receiveOwner->_flightStateObservations.find(flightChannel);
            row->updating = false;
            row->complete = false;
        }
        if (flightDepthHeld && receiveOwner->_flightStateObservationReceiveDepth > 0) {
            --receiveOwner->_flightStateObservationReceiveDepth;
        }
    });
    const bool coordinateFrame = originalMessage.msgid == MAVLINK_MSG_ID_HOME_POSITION || originalMessage.msgid == MAVLINK_MSG_ID_LOCAL_POSITION_NED;
    if (coordinateFrame) {
        if (_startupCoordinateReceiveDepth == std::numeric_limits<quint64>::max()) { _startupCoordinateExhausted = true; return; }
        ++_startupCoordinateReceiveDepth;
    }
    QString coordinateChannel;
    QVariantMap coordinateValue;
    const quint64 coordinateToken = _prepareStartupCoordinateObservation(originalMessage, receivedLink.data(), &coordinateChannel, &coordinateValue);
    if (!receiveOwner || !receivedLink) return;
    const QString coordinateEpoch = _startupCoordinateEpoch;
    const auto discardCoordinateUpdate = qScopeGuard([receiveOwner, coordinateFrame, coordinateChannel, coordinateToken, coordinateEpoch]() {
        if (!receiveOwner) return;
        auto row = receiveOwner->_startupCoordinateRows.find(coordinateChannel);
        if (coordinateToken && coordinateEpoch == receiveOwner->_startupCoordinateEpoch && row != receiveOwner->_startupCoordinateRows.end()
            && row->updateToken == coordinateToken && row->updating) { row->updating = false; row->complete = false; }
        if (coordinateFrame && receiveOwner->_startupCoordinateReceiveDepth > 0) --receiveOwner->_startupCoordinateReceiveDepth;
    });
    // If the link is already running at Mavlink V2 set our max proto version to it.
    unsigned mavlinkVersion = _mavlink->getCurrentVersion();
    if (_maxProtoVersion != mavlinkVersion && mavlinkVersion >= 200) {
        _maxProtoVersion = mavlinkVersion;
        qCDebug(VehicleLog) << "_mavlinkMessageReceived Link already running Mavlink v2. Setting _maxProtoVersion" << _maxProtoVersion;
    }

    if (message.sysid != _id && message.sysid != 0) {
        // We allow RADIO_STATUS messages which come from a link the vehicle is using to pass through and be handled
        if (!(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS && _vehicleLinkManager->containsLink(link))) {
            return;
        }
    }

    // We give the link manager first whack since it it reponsible for adding new links
    _vehicleLinkManager->mavlinkMessageReceived(link, message);
    if (!receiveOwner || !receivedLink) return;
    if (startupFrame && sender() == _mavlink) {
        startupIngressHandled = _handleFleetStartupRawMessage(originalMessage, receivedLink.data(), startupIngress);
        if (receiveOwner && !startupIngressHandled) _discardFleetStartupRawIngress(startupIngress);
        if (!receiveOwner || !receivedLink) return;
    }

    // Only this actual protocol receive, frozen BEFORE firmware/core plugins,
    // can advance an owned scoped mission read. Source-less adjusted signals
    // remain exclusively the legacy path. The hook can send/finish/cancel and
    // therefore delete either object or change its scope before returning.
    if (sender() == _mavlink && _missionManager && (originalMessage.msgid == MAVLINK_MSG_ID_MISSION_COUNT
        || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ITEM_INT || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ACK
        || originalMessage.msgid == MAVLINK_MSG_ID_MISSION_ITEM)) {
        const QPointer<MissionManager> rawManager(_missionManager);
        rawManager->_handleFleetMissionRawMessage(originalMessage, receivedLink.data(), rawReceivedAtMs);
        if (!receiveOwner || !receivedLink) return;
    }

    // Existing P0 receive guards/prepares are established. A new observer
    // must not introduce Link virtual callbacks before those old fences.
    // Failure only withholds this independent observation, never the RAW.
    if (localReceptionIngress.eligible) {
        _observeFleetLocalReception(originalMessage, localReceptionIngress);
        if (!receiveOwner || !receivedLink) return;
    }
    if (cameraStreamIngress.eligible) {
        _observeFleetCameraStream(originalMessage, cameraStreamIngress);
        if (!receiveOwner || !receivedLink) return;
    }
    if (originalCameraRun) {
        _observeFleetCameraStreamCapture(originalCameraRun, originalCameraStage, originalCameraRequest,
            originalCameraIssued, originalCameraBaseline, originalCameraEpoch, originalMessage, receivedLink.data());
        if (!receiveOwner || !receivedLink) return;
    }
    if (originalCameraControlRun) {
        _observeFleetCameraStreamControl(originalCameraControlRun, originalCameraControlRequest,
            originalCameraControlIssued, originalCameraControlBaseline, originalMessage, receivedLink.data());
        if (!receiveOwner || !receivedLink) return;
    }
    if (originalLatencyRun && _fleetLinkLatencyRun == originalLatencyRun
        && originalLatencyRun->nonce == originalLatencyNonce) {
        _observeFleetLinkLatencyCapture(originalLatencyRun, originalMessage, receivedLink.data(),
            latencyReceivedAtNs, originalLatencyWriteStarted);
        if (!receiveOwner || !receivedLink) return;
    }

    //-- Check link status
    _messagesReceived++;
    emit messagesReceivedChanged();
    if (!receiveOwner || !receivedLink) return;
    if(!_heardFrom) {
        if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _heardFrom  = true;
            _compID     = message.compid;
            _messageSeq = message.seq + 1;
        }
    } else {
        if(_compID == message.compid) {
            uint16_t seq_received = static_cast<uint16_t>(message.seq);
            uint16_t packet_lost_count = 0;
            //-- Account for overflow during packet loss
            if(seq_received < _messageSeq) {
                packet_lost_count = (seq_received + 255) - _messageSeq;
            } else {
                packet_lost_count = seq_received - _messageSeq;
            }
            _messageSeq = message.seq + 1;
            _messagesLost += packet_lost_count;
            if(packet_lost_count) {
                emit messagesLostChanged();
                if (!receiveOwner || !receivedLink) return;
            }
        }
    }

    // P0 raw guards are now held. A response cannot claim a request created
    // by an earlier callback from this same packet, nor expose old ground
    // evidence before this packet's flight/health prepare has run.
    if (!receiveProvidersCurrent()) return;
    if (originalCommandEligible && _messageCommandLaneId == originalMessageLaneId
        && _messageCommandLaneSerial == originalMessageLaneSerial) {
        _handleMessageCommandAck(originalMessage, receivedLink.data());
        if (!receiveProvidersCurrent()) return;
    }
    if (originalRequestEligible && _requestMessageContext == originalRequestContext
        && _messageCommandLaneId == originalMessageLaneId && _messageCommandLaneSerial == originalMessageLaneSerial) {
        _waitForMavlinkMessageMessageReceived(originalMessage, receivedLink.data());
        if (!receiveProvidersCurrent()) return;
    }
    if (originalIntervalQueryEligible && _fleetMessageIntervalActiveQuery == originalIntervalQuery
        && _messageCommandLaneId == originalMessageLaneId && _messageCommandLaneSerial == originalMessageLaneSerial) {
        _observeFleetMessageIntervalQuery(originalIntervalQuery, originalMessage, receivedLink.data());
        if (!receiveProvidersCurrent()) return;
    }
    // Give the plugin a change to adjust the message contents
    if (!receiveProvidersCurrent()) return;
    const bool firmwareAccepted = receivePlugin->adjustIncomingMavlinkMessage(this, &message);
    if (!receiveProvidersCurrent() || !firmwareAccepted) {
        return;
    }

    // Give the Core Plugin access to all mavlink traffic
    const bool coreAccepted = receiveCore->mavlinkMessage(this, receivedLink.data(), message);
    if (!receiveProvidersCurrent() || !coreAccepted) {
        return;
    }

    const bool terrainAccepted = receiveTerrain->mavlinkMessageReceived(message);
    if (!receiveProvidersCurrent() || !terrainAccepted) {
        return;
    }
    receiveFtp->_mavlinkMessageReceived(message);
    if (!receiveProvidersCurrent()) return;
    // Keep the real receive link for parameter observation provenance; a
    // primary-link flag alone cannot exclude replay on a secondary link.
    if (receiveParameters) receiveParameters->mavlinkMessageReceived(message, receivedLink.data(),
        receiveParameters == originalImportParameters ? originalImportReceipt : std::shared_ptr<ParameterManager::ParameterImportReceipt>());
    if (!receiveProvidersCurrent()) return;
    receiveImage->mavlinkMessageReceived(message);
    if (!receiveProvidersCurrent()) return;
    receiveRemoteId->mavlinkMessageReceived(message);
    if (!receiveProvidersCurrent()) return;

    // Battery fact groups are created dynamically as new batteries are discovered
    VehicleBatteryFactGroup::handleMessageForFactGroupCreation(this, message);
    if (!receiveOwner || !receivedLink) return;

    // Let the fact groups take a whack at the mavlink traffic
    QList<QPointer<FactGroup>> receivedFactGroups;
    for (FactGroup* factGroup : factGroups()) {
        receivedFactGroups.append(QPointer<FactGroup>(factGroup));
    }
    for (const QPointer<FactGroup>& factGroup : receivedFactGroups) {
        if (factGroup) factGroup->handleMessage(this, message);
        if (!receiveOwner || !receivedLink) return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HOME_POSITION:
        _handleHomePosition(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MAVLINK_MSG_ID_RADIO_STATUS:
        _handleRadioStatus(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
        _handleRCChannels(message);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_SYS_STATUS:
        _handleSysStatus(message);
        break;
    case MAVLINK_MSG_ID_RAW_IMU:
        _handleRawImuTemp(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU:
        emit mavlinkScaledImu1(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU2:
        emit mavlinkScaledImu2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU3:
        emit mavlinkScaledImu3(message);
        break;
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
        _handleExtendedSysState(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message, receivedLink.data(), &originalMessage);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA:
        _handleMavlinkLoggingData(message);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA_ACKED:
        _handleMavlinkLoggingDataAcked(message);
        break;
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _handleGlobalPositionInt(message);
        break;
    case MAVLINK_MSG_ID_ALTITUDE:
        _handleAltitude(message);
        break;
    case MAVLINK_MSG_ID_VFR_HUD:
        _handleVfrHud(message);
        break;
    case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
        _handleNavControllerOutput(message);
        break;
    case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
        _handleCameraImageCaptured(message);
        break;
    case MAVLINK_MSG_ID_ADSB_VEHICLE:
        _handleADSBVehicle(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE:
        _handleAttitude(message);
        break;
    case MAVLINK_MSG_ID_ATTITUDE_QUATERNION:
        _handleAttitudeQuaternion(message);
        break;
    case MAVLINK_MSG_ID_STATUSTEXT:
        _handleStatusText(message);
        break;
    case MAVLINK_MSG_ID_ORBIT_EXECUTION_STATUS:
        _handleOrbitExecutionStatus(message);
        break;
    case MAVLINK_MSG_ID_PING:
        _handlePing(link, message);
        break;
    case MAVLINK_MSG_ID_MOUNT_ORIENTATION:
        _handleGimbalOrientation(message);
        break;
    case MAVLINK_MSG_ID_OBSTACLE_DISTANCE:
        _handleObstacleDistance(message);
        break;
    case MAVLINK_MSG_ID_FENCE_STATUS:
        _handleFenceStatus(message);
        break;

    case MAVLINK_MSG_ID_EVENT:
    case MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE:
    case MAVLINK_MSG_ID_RESPONSE_EVENT_ERROR:
        _eventHandler(message.compid).handleEvents(message);
        break;

    case MAVLINK_MSG_ID_SERIAL_CONTROL:
    {
        mavlink_serial_control_t ser;
        mavlink_msg_serial_control_decode(&message, &ser);
        if (static_cast<size_t>(ser.count) > sizeof(ser.data)) {
            qWarning() << "Invalid count for SERIAL_CONTROL, discarding." << ser.count;
        } else {
            emit mavlinkSerialControl(ser.device, ser.flags, ser.timeout, ser.baudrate,
                    QByteArray(reinterpret_cast<const char*>(ser.data), ser.count));
        }
    }
        break;
#ifdef DAILY_BUILD // Disable use of development/WIP MAVLink messages for release builds
        case MAVLINK_MSG_ID_AVAILABLE_MODES_MONITOR:
    {
        // Avoid duplicate requests during initial connection setup
        if (!_initialConnectStateMachine || !_initialConnectStateMachine->active()) {
            mavlink_available_modes_monitor_t availableModesMonitor;
            mavlink_msg_available_modes_monitor_decode(&message, &availableModesMonitor);
            _standardModes->availableModesMonitorReceived(availableModesMonitor.seq);
        }
        break;
    }
    case MAVLINK_MSG_ID_CURRENT_MODE:
        _handleCurrentMode(message);
        break;
#endif // DAILY_BUILD

        // Following are ArduPilot dialect messages
#if !defined(NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        _handleCameraFeedback(message);
        break;
    case MAVLINK_MSG_ID_RANGEFINDER:
        _handleRangefinder(message);
        break;
#endif
    }

    // This must be emitted after the vehicle processes the message. This way the vehicle state is up to date when anyone else
    // does processing.
    if (!receiveOwner || !receivedLink) return;
    emit mavlinkMessageReceived(message);
    if (!receiveOwner || !receivedLink) return;

    _uas->receiveMessage(message);
    if (!receiveOwner || !receivedLink) return;
    _completeFlightStateObservation(flightChannel, flightToken, flightEpoch, flightValue, receivedLink.data(), rawReceivedAtMs);
    _completeStartupCoordinateObservation(coordinateChannel, coordinateToken, coordinateEpoch, coordinateValue, originalMessage, receivedLink.data(), rawReceivedAtMs);
    _completeStartupOdometryObservation(odometryToken, odometryEpoch, odometryValue, originalMessage, receivedLink.data(), rawReceivedAtMs);
    _completeAutopilotVersionObservation(versionToken, versionEpoch, originalMessage, receivedLink.data(), rawReceivedAtMs);
}

#if !defined(NO_ARDUPILOT_DIALECT)
void Vehicle::_handleCameraFeedback(const mavlink_message_t& message)
{
    mavlink_camera_feedback_t feedback;

    mavlink_msg_camera_feedback_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lng / qPow(10.0, 7.0), feedback.alt_msl);
    qCDebug(VehicleLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.img_idx;
    _cameraTriggerPoints.append(new QGCQGeoCoordinate(imageCoordinate, this));
}

void Vehicle::_handleRangefinder(mavlink_message_t& message)
{

    mavlink_rangefinder_t rangefinder;
    mavlink_msg_rangefinder_decode(&message, &rangefinder);
    _rangeFinderDistFact.setRawValue(qIsNaN(rangefinder.distance) ? 0 : rangefinder.distance);
}
#endif

void Vehicle::_handleOrbitExecutionStatus(const mavlink_message_t& message)
{
    mavlink_orbit_execution_status_t orbitStatus;

    mavlink_msg_orbit_execution_status_decode(&message, &orbitStatus);

    double newRadius =  qAbs(static_cast<double>(orbitStatus.radius));
    if (!QGC::fuzzyCompare(_orbitMapCircle.radius()->rawValue().toDouble(), newRadius)) {
        _orbitMapCircle.radius()->setRawValue(newRadius);
    }

    bool newOrbitClockwise = orbitStatus.radius > 0 ? true : false;
    if (_orbitMapCircle.clockwiseRotation() != newOrbitClockwise) {
        _orbitMapCircle.setClockwiseRotation(newOrbitClockwise);
    }

    QGeoCoordinate newCenter(static_cast<double>(orbitStatus.x) / qPow(10.0, 7.0), static_cast<double>(orbitStatus.y) / qPow(10.0, 7.0));
    if (_orbitMapCircle.center() != newCenter) {
        _orbitMapCircle.setCenter(newCenter);
    }

    if (!_orbitActive) {
        _orbitActive = true;
        _orbitMapCircle.setShowRotation(true);
        emit orbitActiveChanged(true);
    }

    _orbitTelemetryTimer.start(_orbitTelemetryTimeoutMsecs);
}

void Vehicle::_orbitTelemetryTimeout()
{
    _orbitActive = false;
    emit orbitActiveChanged(false);
}

void Vehicle::_handleCameraImageCaptured(const mavlink_message_t& message)
{
    mavlink_camera_image_captured_t feedback;

    mavlink_msg_camera_image_captured_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lon / qPow(10.0, 7.0), feedback.alt);
    qCDebug(VehicleLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.image_index << feedback.capture_result;
    if (feedback.capture_result == 1) {
        _cameraTriggerPoints.append(new QGCQGeoCoordinate(imageCoordinate, this));
    }
}

void Vehicle::_chunkedStatusTextTimeout(void)
{
    // Spit out all incomplete chunks
    QList<uint8_t> rgCompId = _chunkedStatusTextInfoMap.keys();
    for (uint8_t compId : rgCompId) {
        _chunkedStatusTextInfoMap[compId].rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }
}

void Vehicle::_chunkedStatusTextCompleted(uint8_t compId)
{
    ChunkedStatusTextInfo_t&    chunkedInfo =   _chunkedStatusTextInfoMap[compId];
    uint8_t                     severity =      chunkedInfo.severity;
    QStringList&                rgChunks =      chunkedInfo.rgMessageChunks;

    // Build up message from chunks
    QString messageText;
    for (const QString& chunk : rgChunks) {
        if (chunk.isEmpty()) {
            // Indicates missing chunk
            messageText += tr(" ... ", "Indicates missing chunk from chunked STATUS_TEXT");
        } else {
            messageText += chunk;
        }
    }

    _chunkedStatusTextInfoMap.remove(compId);

    // PX4 backwards compatibility: messages sent out ending with a tab are also sent as event
    if (messageText.endsWith('\t') && px4Firmware()) {
        qCDebug(VehicleLog) << "Dropping message (expected as event):" << messageText;
        return;
    }

    bool skipSpoken = false;
    bool ardupilotPrearm = messageText.startsWith(QStringLiteral("PreArm"));
    bool px4Prearm = messageText.startsWith(QStringLiteral("preflight"), Qt::CaseInsensitive) && severity >= MAV_SEVERITY_CRITICAL;
    if (ardupilotPrearm || px4Prearm) {
        // check if expected as event
        auto eventData = _events.find(compId);
        if (eventData != _events.end()) {
            if (eventData->data()->healthAndArmingChecksSupported()) {
                qCDebug(VehicleLog) << "Dropping preflight message (expected as event):" << messageText;
                return;
            }
        }

        // Limit repeated PreArm message to once every 10 seconds
        if (_noisySpokenPrearmMap.contains(messageText) && _noisySpokenPrearmMap[messageText].msecsTo(QTime::currentTime()) < (10 * 1000)) {
            skipSpoken = true;
        } else {
            _noisySpokenPrearmMap[messageText] = QTime::currentTime();
            setPrearmError(messageText);
        }
    }

    // If the message is NOTIFY or higher severity, or starts with a '#',
    // then read it aloud.
    bool readAloud = false;

    if (messageText.startsWith("#")) {
        messageText.remove(0, 1);
        readAloud = true;
    }
    else if (severity <= MAV_SEVERITY_NOTICE) {
        readAloud = true;
    }

    if (readAloud) {
        if (!skipSpoken) {
            qgcApp()->toolbox()->audioOutput()->say(messageText);
        }
    }
    emit textMessageReceived(id(), compId, severity, messageText.toHtmlEscaped(), "");
}

void Vehicle::_handleStatusText(mavlink_message_t& message)
{
    QString     messageText;

    mavlink_statustext_t statustext;
    mavlink_msg_statustext_decode(&message, &statustext);

    uint8_t compId = message.compid;

    char b[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1];
    strncpy(b, statustext.text, MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
    b[sizeof(b)-1] = '\0';
    messageText = QString(b);
    bool includesNullTerminator = messageText.length() < MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN;

    if (_chunkedStatusTextInfoMap.contains(compId) && _chunkedStatusTextInfoMap[compId].chunkId != statustext.id) {
        // We have an incomplete chunked status still pending
        _chunkedStatusTextInfoMap[compId].rgMessageChunks.append(QString());
        _chunkedStatusTextCompleted(compId);
    }

    if (statustext.id == 0) {
        // Non-chunked status text. We still use common chunked text output mechanism.
        ChunkedStatusTextInfo_t chunkedInfo;
        chunkedInfo.chunkId = 0;
        chunkedInfo.severity = statustext.severity;
        chunkedInfo.rgMessageChunks.append(messageText);
        _chunkedStatusTextInfoMap[compId] = chunkedInfo;
    } else {
        if (_chunkedStatusTextInfoMap.contains(compId)) {
            // A chunk sequence is in progress
            QStringList& chunks = _chunkedStatusTextInfoMap[compId].rgMessageChunks;
            if (statustext.chunk_seq > chunks.size()) {
                // We are missing some chunks in between, fill them in as missing
                for (int i=chunks.size(); i<statustext.chunk_seq; i++) {
                    chunks.append(QString());
                }
            }
            chunks.append(messageText);
        } else {
            // Starting a new chunk sequence
            ChunkedStatusTextInfo_t chunkedInfo;
            chunkedInfo.chunkId = statustext.id;
            chunkedInfo.severity = statustext.severity;
            chunkedInfo.rgMessageChunks.append(messageText);
            _chunkedStatusTextInfoMap[compId] = chunkedInfo;
        }
        _chunkedStatusTextTimer.start();
    }

    if (statustext.id == 0 || includesNullTerminator) {
        _chunkedStatusTextTimer.stop();
        _chunkedStatusTextCompleted(message.compid);
    }
}

void Vehicle::_handleVfrHud(mavlink_message_t& message)
{
    mavlink_vfr_hud_t vfrHud;
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);

    _airSpeedFact.setRawValue(qIsNaN(vfrHud.airspeed) ? 0 : vfrHud.airspeed);
    _groundSpeedFact.setRawValue(qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed);
    _climbRateFact.setRawValue(qIsNaN(vfrHud.climb) ? 0 : vfrHud.climb);
    _throttlePctFact.setRawValue(static_cast<int16_t>(vfrHud.throttle));
    if (qIsNaN(_altitudeTuningOffset)) {
        _altitudeTuningOffset = vfrHud.alt;
    }
    _altitudeTuningFact.setRawValue(vfrHud.alt - _altitudeTuningOffset);
    if (!qIsNaN(vfrHud.groundspeed) && !qIsNaN(_distanceToHomeFact.cookedValue().toDouble())) {
      _timeToHomeFact.setRawValue(_distanceToHomeFact.cookedValue().toDouble() / vfrHud.groundspeed);
    }
}

void Vehicle::_handleNavControllerOutput(mavlink_message_t& message)
{
    mavlink_nav_controller_output_t navControllerOutput;
    mavlink_msg_nav_controller_output_decode(&message, &navControllerOutput);

    _altitudeTuningSetpointFact.setRawValue(_altitudeTuningFact.rawValue().toDouble() - navControllerOutput.alt_error);
    _xTrackErrorFact.setRawValue(navControllerOutput.xtrack_error);
    _airSpeedSetpointFact.setRawValue(_airSpeedFact.rawValue().toDouble() - navControllerOutput.aspd_error);
}

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__

#if __GNUC__ > 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#else
#pragma warning(push, 0)
#endif

void Vehicle::_handleAttitudeWorker(double rollRadians, double pitchRadians, double yawRadians)
{
    double roll, pitch, yaw;

    roll = QGC::limitAngleToPMPIf(rollRadians);
    pitch = QGC::limitAngleToPMPIf(pitchRadians);
    yaw = QGC::limitAngleToPMPIf(yawRadians);

    roll = qRadiansToDegrees(roll);
    pitch = qRadiansToDegrees(pitch);
    yaw = qRadiansToDegrees(yaw);

    if (yaw < 0.0) {
        yaw += 360.0;
    }
    // truncate to integer so widget never displays 360
    yaw = trunc(yaw);

    _rollFact.setRawValue(roll);
    _pitchFact.setRawValue(pitch);
    _headingFact.setRawValue(yaw);
}

void Vehicle::_handleAttitude(mavlink_message_t& message)
{
    // only accept the attitude message from the vehicle's flight controller
    if (message.sysid != _id || message.compid != _compID) {
        return;
    }

    if (_receivingAttitudeQuaternion) {
        return;
    }

    mavlink_attitude_t attitude;
    mavlink_msg_attitude_decode(&message, &attitude);

    _handleAttitudeWorker(attitude.roll, attitude.pitch, attitude.yaw);
}

void Vehicle::_handleAttitudeQuaternion(mavlink_message_t& message)
{
    // only accept the attitude message from the vehicle's flight controller
    if (message.sysid != _id || message.compid != _compID) {
        return;
    }

    _receivingAttitudeQuaternion = true;

    mavlink_attitude_quaternion_t attitudeQuaternion;
    mavlink_msg_attitude_quaternion_decode(&message, &attitudeQuaternion);

    Eigen::Quaternionf quat(attitudeQuaternion.q1, attitudeQuaternion.q2, attitudeQuaternion.q3, attitudeQuaternion.q4);
    Eigen::Vector3f rates(attitudeQuaternion.rollspeed, attitudeQuaternion.pitchspeed, attitudeQuaternion.yawspeed);
    Eigen::Quaternionf repr_offset(attitudeQuaternion.repr_offset_q[0], attitudeQuaternion.repr_offset_q[1], attitudeQuaternion.repr_offset_q[2], attitudeQuaternion.repr_offset_q[3]);

    // if repr_offset is valid, rotate attitude and rates
    if (repr_offset.norm() >= 0.5f) {
        quat = quat * repr_offset;
        rates = repr_offset * rates;
    }

    float roll, pitch, yaw;
    float q[] = { quat.w(), quat.x(), quat.y(), quat.z() };
    mavlink_quaternion_to_euler(q, &roll, &pitch, &yaw);

    _handleAttitudeWorker(roll, pitch, yaw);

    rollRate()->setRawValue(qRadiansToDegrees(rates[0]));
    pitchRate()->setRawValue(qRadiansToDegrees(rates[1]));
    yawRate()->setRawValue(qRadiansToDegrees(rates[2]));
}

void Vehicle::_handleGpsRawInt(mavlink_message_t& message)
{
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);

    _gpsRawIntMessageAvailable = true;

    if (gpsRawInt.fix_type >= GPS_FIX_TYPE_3D_FIX) {
        if (!_globalPositionIntMessageAvailable) {
            QGeoCoordinate newPosition(gpsRawInt.lat  / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt  / 1000.0);
            if (newPosition != _coordinate) {
                _coordinate = newPosition;
                emit coordinateChanged(_coordinate);
            }
            if (!_altitudeMessageAvailable) {
                _altitudeAMSLFact.setRawValue(gpsRawInt.alt / 1000.0);
            }
        }
    }
}

void Vehicle::_handleGlobalPositionInt(mavlink_message_t& message)
{
    mavlink_global_position_int_t globalPositionInt;
    mavlink_msg_global_position_int_decode(&message, &globalPositionInt);

    if (!_altitudeMessageAvailable) {
        _altitudeRelativeFact.setRawValue(globalPositionInt.relative_alt / 1000.0);
        _altitudeAMSLFact.setRawValue(globalPositionInt.alt / 1000.0);
    }

    // ArduPilot sends bogus GLOBAL_POSITION_INT messages with lat/lat 0/0 even when it has no gps signal
    // Apparently, this is in order to transport relative altitude information.
    if (globalPositionInt.lat == 0 && globalPositionInt.lon == 0) {
        return;
    }

    _globalPositionIntMessageAvailable = true;
    QGeoCoordinate newPosition(globalPositionInt.lat  / (double)1E7, globalPositionInt.lon / (double)1E7, globalPositionInt.alt  / 1000.0);
    if (newPosition != _coordinate) {
        _coordinate = newPosition;
        emit coordinateChanged(_coordinate);
    }
}

void Vehicle::_handleHighLatency(mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

    QString previousFlightMode;
    if (_base_mode != 0 || _custom_mode != 0){
        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
        // bad modes while unit testing.
        previousFlightMode = flightMode();
    }
    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency.custom_mode);
    if (previousFlightMode != flightMode()) {
        emit flightModeChanged(flightMode());
    }

    // Assume armed since we don't know
    if (_armed != true) {
        _armed = true;
        emit armedChanged(_armed);
    }

    struct {
        const double latitude;
        const double longitude;
        const double altitude;
    } coordinate {
        highLatency.latitude  / (double)1E7,
                highLatency.longitude  / (double)1E7,
                static_cast<double>(highLatency.altitude_amsl)
    };

    _coordinate.setLatitude(coordinate.latitude);
    _coordinate.setLongitude(coordinate.longitude);
    _coordinate.setAltitude(coordinate.altitude);
    emit coordinateChanged(_coordinate);

    _airSpeedFact.setRawValue((double)highLatency.airspeed / 5.0);
    _groundSpeedFact.setRawValue((double)highLatency.groundspeed / 5.0);
    _climbRateFact.setRawValue((double)highLatency.climb_rate / 10.0);
    _headingFact.setRawValue((double)highLatency.heading * 2.0);
    _altitudeRelativeFact.setRawValue(qQNaN());
    _altitudeAMSLFact.setRawValue(coordinate.altitude);
}

void Vehicle::_handleHighLatency2(mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    QString previousFlightMode;
    if (_base_mode != 0 || _custom_mode != 0){
        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
        // bad modes while unit testing.
        previousFlightMode = flightMode();
    }
    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency2.custom_mode);
    if (previousFlightMode != flightMode()) {
        emit flightModeChanged(flightMode());
    }

    // Assume armed since we don't know
    if (_armed != true) {
        _armed = true;
        emit armedChanged(_armed);
    }

    _coordinate.setLatitude(highLatency2.latitude  / (double)1E7);
    _coordinate.setLongitude(highLatency2.longitude / (double)1E7);
    _coordinate.setAltitude(highLatency2.altitude);
    emit coordinateChanged(_coordinate);

    _airSpeedFact.setRawValue((double)highLatency2.airspeed / 5.0);
    _groundSpeedFact.setRawValue((double)highLatency2.groundspeed / 5.0);
    _climbRateFact.setRawValue((double)highLatency2.climb_rate / 10.0);
    _headingFact.setRawValue((double)highLatency2.heading * 2.0);
    _altitudeRelativeFact.setRawValue(qQNaN());
    _altitudeAMSLFact.setRawValue(highLatency2.altitude);

    struct failure2Sensor_s {
        HL_FAILURE_FLAG         failureBit;
        MAV_SYS_STATUS_SENSOR   sensorBit;
    };

    static const failure2Sensor_s rgFailure2Sensor[] = {
        { HL_FAILURE_FLAG_GPS,                      MAV_SYS_STATUS_SENSOR_GPS },
        { HL_FAILURE_FLAG_DIFFERENTIAL_PRESSURE,    MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE },
        { HL_FAILURE_FLAG_ABSOLUTE_PRESSURE,        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE },
        { HL_FAILURE_FLAG_3D_ACCEL,                 MAV_SYS_STATUS_SENSOR_3D_ACCEL },
        { HL_FAILURE_FLAG_3D_GYRO,                  MAV_SYS_STATUS_SENSOR_3D_GYRO },
        { HL_FAILURE_FLAG_3D_MAG,                   MAV_SYS_STATUS_SENSOR_3D_MAG },
    };

    // Map from MAV_FAILURE bits to standard SYS_STATUS message handling
    uint32_t newOnboardControlSensorsEnabled = 0;
    for (size_t i=0; i<sizeof(rgFailure2Sensor)/sizeof(failure2Sensor_s); i++) {
        const failure2Sensor_s* pFailure2Sensor = &rgFailure2Sensor[i];
        if (highLatency2.failure_flags & pFailure2Sensor->failureBit) {
            // Assume if reporting as unhealthy that is it present and enabled
            newOnboardControlSensorsEnabled |= pFailure2Sensor->sensorBit;
        }
    }
    if (newOnboardControlSensorsEnabled != _onboardControlSensorsEnabled) {
        _onboardControlSensorsEnabled = newOnboardControlSensorsEnabled;
        _onboardControlSensorsPresent = newOnboardControlSensorsEnabled;
        _onboardControlSensorsUnhealthy = 0;
    }
}

void Vehicle::_handleAltitude(mavlink_message_t& message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    // Data from ALTITUDE message takes precedence over gps messages
    _altitudeMessageAvailable = true;
    _altitudeRelativeFact.setRawValue(altitude.altitude_relative);
    _altitudeAMSLFact.setRawValue(altitude.altitude_amsl);
}

void Vehicle::_setCapabilities(uint64_t capabilityBits)
{
    const QPointer<Vehicle> self(this);
    _capabilityBits = capabilityBits;
    _capabilityBitsKnown = true;
    emit capabilitiesKnownChanged(true);
    if (!self) return;
    emit capabilityBitsChanged(_capabilityBits);
    if (!self) return;

    QString supports("supports");
    QString doesNotSupport("does not support");

    qCDebug(VehicleLog) << QString("Vehicle %1 Mavlink 2.0").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MAVLINK2 ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_ITEM_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 MISSION_COMMAND_INT").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_COMMAND_INT ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 GeoFence").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_FENCE ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 RallyPoints").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_MISSION_RALLY ? supports : doesNotSupport);
    qCDebug(VehicleLog) << QString("Vehicle %1 Terrain").arg(_capabilityBits & MAV_PROTOCOL_CAPABILITY_TERRAIN ? supports : doesNotSupport);

    _setMaxProtoVersionFromBothSources();
}

void Vehicle::_setMaxProtoVersion(unsigned version) {

    // Set only once or if we need to reduce the max version
    if (_maxProtoVersion == 0 || version < _maxProtoVersion) {
        qCDebug(VehicleLog) << "_setMaxProtoVersion before:after" << _maxProtoVersion << version;
        _maxProtoVersion = version;
        emit requestProtocolVersion(_maxProtoVersion);
    }
}

void Vehicle::_setMaxProtoVersionFromBothSources()
{
    if (_mavlinkProtocolRequestComplete && _capabilityBitsKnown) {
        if (_mavlinkProtocolRequestMaxProtoVersion != 0) {
            qCDebug(VehicleLog) << "_setMaxProtoVersionFromBothSources using protocol version message";
            _setMaxProtoVersion(_mavlinkProtocolRequestMaxProtoVersion);
        } else {
            qCDebug(VehicleLog) << "_setMaxProtoVersionFromBothSources using capability bits";
            _setMaxProtoVersion(capabilityBits() & MAV_PROTOCOL_CAPABILITY_MAVLINK2 ? 200 : 100);
        }
    }
}

QString Vehicle::vehicleUIDStr()
{
    QString uid;
    uint8_t* pUid = (uint8_t*)(void*)&_uid;
    uid = uid.asprintf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                 pUid[0] & 0xff,
            pUid[1] & 0xff,
            pUid[2] & 0xff,
            pUid[3] & 0xff,
            pUid[4] & 0xff,
            pUid[5] & 0xff,
            pUid[6] & 0xff,
            pUid[7] & 0xff);
    return uid;
}

void Vehicle::_handleExtendedSysState(mavlink_message_t& message)
{
    mavlink_extended_sys_state_t extendedState;
    mavlink_msg_extended_sys_state_decode(&message, &extendedState);

    switch (extendedState.landed_state) {
    case MAV_LANDED_STATE_ON_GROUND:
        _setFlying(false);
        _setLanding(false);
        break;
    case MAV_LANDED_STATE_TAKEOFF:
    case MAV_LANDED_STATE_IN_AIR:
        _setFlying(true);
        _setLanding(false);
        break;
    case MAV_LANDED_STATE_LANDING:
        _setFlying(true);
        _setLanding(true);
        break;
    default:
        break;
    }

    if (vtol()) {
        bool vtolInFwdFlight = extendedState.vtol_state == MAV_VTOL_STATE_FW;
        if (vtolInFwdFlight != _vtolInFwdFlight) {
            _vtolInFwdFlight = vtolInFwdFlight;
            emit vtolInFwdFlightChanged(vtolInFwdFlight);
        }
    }
}

ParameterManager* Vehicle::parameterManager()
{
    return _parameterManager.data();
}

ParameterManager* Vehicle::parameterManager() const
{
    return _parameterManager.data();
}

bool Vehicle::_apmArmingNotRequired()
{
    QString armingRequireParam("ARMING_REQUIRE");
    return _parameterManager && _parameterManager->parameterExists(FactSystem::defaultComponentId, armingRequireParam) &&
            _parameterManager->getParameter(FactSystem::defaultComponentId, armingRequireParam)->rawValue().toInt() == 0;
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);

    _sysStatusSensorInfo.update(sysStatus);

    if (sysStatus.onboard_control_sensors_enabled & MAV_SYS_STATUS_PREARM_CHECK) {
        if (!_readyToFlyAvailable) {
            _readyToFlyAvailable = true;
            emit readyToFlyAvailableChanged(true);
        }

        bool newReadyToFly = sysStatus.onboard_control_sensors_health & MAV_SYS_STATUS_PREARM_CHECK;
        if (newReadyToFly != _readyToFly) {
            _readyToFly = newReadyToFly;
            emit readyToFlyChanged(_readyToFly);
        }
    }

    bool newAllSensorsHealthy = (sysStatus.onboard_control_sensors_enabled & sysStatus.onboard_control_sensors_health) == sysStatus.onboard_control_sensors_enabled;
    if (newAllSensorsHealthy != _allSensorsHealthy) {
        _allSensorsHealthy = newAllSensorsHealthy;
        emit allSensorsHealthyChanged(_allSensorsHealthy);
    }

    if (_onboardControlSensorsPresent != sysStatus.onboard_control_sensors_present) {
        _onboardControlSensorsPresent = sysStatus.onboard_control_sensors_present;
        emit sensorsPresentBitsChanged(_onboardControlSensorsPresent);
        emit requiresGpsFixChanged();
    }
    if (_onboardControlSensorsEnabled != sysStatus.onboard_control_sensors_enabled) {
        _onboardControlSensorsEnabled = sysStatus.onboard_control_sensors_enabled;
        emit sensorsEnabledBitsChanged(_onboardControlSensorsEnabled);
    }
    if (_onboardControlSensorsHealth != sysStatus.onboard_control_sensors_health) {
        _onboardControlSensorsHealth = sysStatus.onboard_control_sensors_health;
        emit sensorsHealthBitsChanged(_onboardControlSensorsHealth);
    }

    // ArduPilot firmare has a strange case when ARMING_REQUIRE=0. This means the vehicle is always armed but the motors are not
    // really powered up until the safety button is pressed. Because of this we can't depend on the heartbeat to tell us the true
    // armed (and dangerous) state. We must instead rely on SYS_STATUS telling us that the motors are enabled.
    if (apmFirmware() && _apmArmingNotRequired()) {
        _updateArmed(_onboardControlSensorsEnabled & MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS);
    }

    uint32_t newSensorsUnhealthy = _onboardControlSensorsEnabled & ~_onboardControlSensorsHealth;
    if (newSensorsUnhealthy != _onboardControlSensorsUnhealthy) {
        _onboardControlSensorsUnhealthy = newSensorsUnhealthy;
        emit sensorsUnhealthyBitsChanged(_onboardControlSensorsUnhealthy);
    }
}

void Vehicle::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    if (!_lowestBatteryChargeStateAnnouncedMap.contains(batteryStatus.id)) {
        _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
    }

    QString batteryMessage;

    switch (batteryStatus.charge_state) {
    case MAV_BATTERY_CHARGE_STATE_OK:
        _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
        break;
    case MAV_BATTERY_CHARGE_STATE_LOW:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level low");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_CRITICAL:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level is critical");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_EMERGENCY:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 level emergency");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_FAILED:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 failed");
        }
        break;
    case MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
        if (batteryStatus.charge_state > _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id]) {
            _lowestBatteryChargeStateAnnouncedMap[batteryStatus.id] = batteryStatus.charge_state;
            batteryMessage = tr("battery %1 unhealthy");
        }
        break;
    }

    if (!batteryMessage.isEmpty()) {
        QString batteryIdStr("%1");
        if (_batteryFactGroupListModel.count() > 1) {
            batteryIdStr = batteryIdStr.arg(batteryStatus.id);
        } else {
            batteryIdStr = batteryIdStr.arg("");
        }
        _say(tr("warning"));
        _say(QStringLiteral("%1 %2 ").arg(_vehicleIdSpeech()).arg(batteryMessage.arg(batteryIdStr)));
    }
}

void Vehicle::_setHomePosition(QGeoCoordinate& homeCoord)
{
    if (homeCoord != _homePosition) {
        _homePosition = homeCoord;
        qCDebug(VehicleLog) << "new home location set at coordinate: " << homeCoord;
        emit homePositionChanged(_homePosition);
    }
}

void Vehicle::_handleHomePosition(mavlink_message_t& message)
{
    mavlink_home_position_t homePos;

    mavlink_msg_home_position_decode(&message, &homePos);

    QGeoCoordinate newHomePosition (homePos.latitude / 10000000.0,
                                    homePos.longitude / 10000000.0,
                                    homePos.altitude / 1000.0);
    _setHomePosition(newHomePosition);
}

void Vehicle::_updateArmed(bool armed)
{
    if (_armed != armed) {
        _armed = armed;
        emit armedChanged(_armed);
        // We are transitioning to the armed state, begin tracking trajectory points for the map
        if (_armed) {
            _trajectoryPoints->start();
            _flightTimerStart();
            _clearCameraTriggerPoints();
            // Reset battery warning
            _lowestBatteryChargeStateAnnouncedMap.clear();
        } else {
            _trajectoryPoints->stop();
            _flightTimerStop();
            // Also handle Video Streaming
            if(qgcApp()->toolbox()->videoManager()->videoReceiver()) {
                if(_settingsManager->videoSettings()->disableWhenDisarmed()->rawValue().toBool()) {
                    _settingsManager->videoSettings()->streamEnabled()->setRawValue(false);
                    qgcApp()->toolbox()->videoManager()->videoReceiver()->stop();
                }
            }
        }
    }
}

void Vehicle::_handlePing(LinkInterface* link, mavlink_message_t& message)
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_handlePing: primary link gone!";
        return;
    }

    mavlink_ping_t      ping;
    mavlink_message_t   msg;

    mavlink_msg_ping_decode(&message, &ping);

    if ((ping.target_system == 0) && (ping.target_component == 0)) {
        // Mavlink defines a ping request as a MSG_ID_PING which contains target_system = 0 and target_component = 0
        // So only send a ping response when you receive a valid ping request
        mavlink_msg_ping_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                   static_cast<uint8_t>(_mavlink->getComponentId()),
                                   sharedLink->mavlinkChannel(),
                                   &msg,
                                   ping.time_usec,
                                   ping.seq,
                                   message.sysid,
                                   message.compid);
        sendMessageOnLinkThreadSafe(link, msg);
    }
}

void Vehicle::_handleEvent(uint8_t comp_id, std::unique_ptr<events::parser::ParsedEvent> event)
{
    int severity = -1;
    switch (events::externalLogLevel(event->eventData().log_levels)) {
        case events::Log::Emergency: severity = MAV_SEVERITY_EMERGENCY; break;
        case events::Log::Alert: severity = MAV_SEVERITY_ALERT; break;
        case events::Log::Critical: severity = MAV_SEVERITY_CRITICAL; break;
        case events::Log::Error: severity = MAV_SEVERITY_ERROR; break;
        case events::Log::Warning: severity = MAV_SEVERITY_WARNING; break;
        case events::Log::Notice: severity = MAV_SEVERITY_NOTICE; break;
        case events::Log::Info: severity = MAV_SEVERITY_INFO; break;
        default: break;
    }

    // handle special groups & protocols
    if (event->group() == "health" || event->group() == "arming_check") {
        // these are displayed separately
        return;
    }
    if (event->group() == "calibration") {
        emit calibrationEventReceived(id(), comp_id, severity,
                QSharedPointer<events::parser::ParsedEvent>{new events::parser::ParsedEvent{*event}});
        // these are displayed separately
        return;
    }

    // show message according to the log level, don't show unknown event groups (might be part of a new protocol)
    if (event->group() == "default" && severity != -1) {
        std::string message = event->message();
        std::string description = event->description();

        if (event->type() == "append_health_and_arming_messages" && event->numArguments() > 0) {
            uint32_t customMode = event->argumentValue(0).value.val_uint32_t;
            const QSharedPointer<EventHandler>& eventHandler = _events[comp_id];
            int modeGroup = eventHandler->getModeGroup(customMode);
            std::vector<events::HealthAndArmingChecks::Check> checks = eventHandler->healthAndArmingCheckResults().checks(modeGroup);
            QList<std::string> messageChecks;
            for (const auto& check : checks) {
                if (events::externalLogLevel(check.log_levels) <= events::Log::Warning) {
                    messageChecks.append(check.message);
                }
            }
            if (messageChecks.empty()) {
                // Add all
                for (const auto& check : checks) {
                    messageChecks.append(check.message);
                }
            }
            if (!message.empty() && !messageChecks.empty()) {
                message += "\n";
            }
            if (messageChecks.size() == 1) {
                message += messageChecks[0];
            } else {
                for (const auto& messageCheck : messageChecks) {
                    message += "- " + messageCheck + "\n";
                }
            }
        }

        if (!message.empty()) {
            emit textMessageReceived(id(), comp_id, severity, QString::fromStdString(message), QString::fromStdString(description));
        }
    }
}

EventHandler& Vehicle::_eventHandler(uint8_t compid)
{
    auto eventData = _events.find(compid);
    if (eventData == _events.end()) {
        // add new component

        auto sendRequestEventMessageCB = [this](const mavlink_request_event_t& msg) {
            SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
            if (sharedLink) {
                mavlink_message_t message;
                mavlink_msg_request_event_encode_chan(_mavlink->getSystemId(),
                        _mavlink->getComponentId(),
                        sharedLink->mavlinkChannel(),
                        &message,
                        &msg);
                sendMessageOnLinkThreadSafe(sharedLink.get(), message);
            }
        };

        QString profile = "dev"; // TODO: should be configurable

        QSharedPointer<EventHandler> eventHandler{new EventHandler(this, profile,
                std::bind(&Vehicle::_handleEvent, this, compid, std::placeholders::_1),
                sendRequestEventMessageCB,
                _mavlink->getSystemId(), _mavlink->getComponentId(), _id, compid)};
        eventData = _events.insert(compid, eventHandler);

        // connect health and arming check updates
        connect(eventHandler.data(), &EventHandler::healthAndArmingChecksUpdated, this, [compid, this]() {
            const QSharedPointer<EventHandler>& eventHandler = _events[compid];
            _healthAndArmingCheckReport.update(compid, eventHandler->healthAndArmingCheckResults(),
                    eventHandler->getModeGroup(_has_custom_mode_user_intention ? _custom_mode_user_intention : _custom_mode),
                    true);
        });
        connect(this, &Vehicle::flightModeChanged, this, [compid, this]() {
            const QSharedPointer<EventHandler>& eventHandler = _events[compid];
            if (eventHandler->healthAndArmingCheckResultsValid()) {
                _healthAndArmingCheckReport.update(compid, eventHandler->healthAndArmingCheckResults(),
                                                   eventHandler->getModeGroup(_has_custom_mode_user_intention ? _custom_mode_user_intention : _custom_mode),
                                                   false);
            }
        });
    }
    return *eventData->data();
}

void Vehicle::setEventsMetadata(uint8_t compid, const QString& metadataJsonFileName)
{
    _eventHandler(compid).setMetadata(metadataJsonFileName);

    // get the mode group for some well-known flight modes
    int modeGroups[2]{-1, -1};
    const QString modes[2]{"Takeoff", "Mission"};
    for (size_t i = 0; i < sizeof(modeGroups)/sizeof(modeGroups[0]); ++i) {
        uint8_t     base_mode;
        uint32_t    custom_mode;
        if (setFlightModeCustom(modes[i], &base_mode, &custom_mode)) {
            modeGroups[i] = _eventHandler(compid).getModeGroup(custom_mode);
            if (modeGroups[i] == -1) {
                qCDebug(VehicleLog) << "Failed to get mode group for mode" << modes[i] << "(Might not be in metadata)";
            }
        }
    }
    _healthAndArmingCheckReport.setModeGroups(modeGroups[0], modeGroups[1]);

    // Request arming checks to be reported
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_RUN_PREARM_CHECKS,
                   false);
}

void Vehicle::setActuatorsMetadata(uint8_t compid, const QString& metadataJsonFileName)
{
    if (!_actuators) {
        _actuators = new Actuators(this, this);
    }
    _actuators->load(metadataJsonFileName);
}

void Vehicle::_handleHeartbeat(mavlink_message_t& message)
{
    if (message.compid != _defaultComponentId) {
        return;
    }

    mavlink_heartbeat_t heartbeat;

    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    bool newArmed = heartbeat.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY;

    // ArduPilot firmare has a strange case when ARMING_REQUIRE=0. This means the vehicle is always armed but the motors are not
    // really powered up until the safety button is pressed. Because of this we can't depend on the heartbeat to tell us the true
    // armed (and dangerous) state. We must instead rely on SYS_STATUS telling us that the motors are enabled.
    if (apmFirmware()) {
        if (!_apmArmingNotRequired() || !(_onboardControlSensorsPresent & MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS)) {
            // If ARMING_REQUIRE!=0 or we haven't seen motor output status yet we use the hearbeat info for armed
            _updateArmed(newArmed);
        }
    } else {
        // Non-ArduPilot always updates from armed state in heartbeat
        _updateArmed(newArmed);
    }

    if (heartbeat.base_mode != _base_mode || heartbeat.custom_mode != _custom_mode) {
        QString previousFlightMode;
        if (_base_mode != 0 || _custom_mode != 0){
            // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
            // bad modes while unit testing.
            previousFlightMode = flightMode();
        }
        _base_mode   = heartbeat.base_mode;
        _custom_mode = heartbeat.custom_mode;
        if (previousFlightMode != flightMode()) {
            emit flightModeChanged(flightMode());
        }
    }
}

void Vehicle::_handleCurrentMode(mavlink_message_t& message)
{
    mavlink_current_mode_t currentMode;
    mavlink_msg_current_mode_decode(&message, &currentMode);
    if (currentMode.intended_custom_mode != 0) { // 0 == unknown/not supplied
        _has_custom_mode_user_intention = true;
        bool changed = _custom_mode_user_intention != currentMode.intended_custom_mode;
        _custom_mode_user_intention = currentMode.intended_custom_mode;
        if (changed) {
            emit flightModeChanged(flightMode());
        }
    }
}

void Vehicle::_handleRadioStatus(mavlink_message_t& message)
{

    //-- Process telemetry status message
    mavlink_radio_status_t rstatus;
    mavlink_msg_radio_status_decode(&message, &rstatus);

    int rssi    = rstatus.rssi;
    int remrssi = rstatus.remrssi;
    int lnoise = (int)(int8_t)rstatus.noise;
    int rnoise = (int)(int8_t)rstatus.remnoise;
    //-- 3DR Si1k radio needs rssi fields to be converted to dBm
    if (message.sysid == '3' && message.compid == 'D') {
        /* Per the Si1K datasheet figure 23.25 and SI AN474 code
         * samples the relationship between the RSSI register
         * and received power is as follows:
         *
         *                       10
         * inputPower = rssi * ------ 127
         *                       19
         *
         * Additionally limit to the only realistic range [-120,0] dBm
         */
        rssi    = qMin(qMax(qRound(static_cast<qreal>(rssi)    / 1.9 - 127.0), - 120), 0);
        remrssi = qMin(qMax(qRound(static_cast<qreal>(remrssi) / 1.9 - 127.0), - 120), 0);
    } else {
        rssi    = (int)(int8_t)rstatus.rssi;
        remrssi = (int)(int8_t)rstatus.remrssi;
    }
    //-- Check for changes
    if(_telemetryLRSSI != rssi) {
        _telemetryLRSSI = rssi;
        emit telemetryLRSSIChanged(_telemetryLRSSI);
    }
    if(_telemetryRRSSI != remrssi) {
        _telemetryRRSSI = remrssi;
        emit telemetryRRSSIChanged(_telemetryRRSSI);
    }
    if(_telemetryRXErrors != rstatus.rxerrors) {
        _telemetryRXErrors = rstatus.rxerrors;
        emit telemetryRXErrorsChanged(_telemetryRXErrors);
    }
    if(_telemetryFixed != rstatus.fixed) {
        _telemetryFixed = rstatus.fixed;
        emit telemetryFixedChanged(_telemetryFixed);
    }
    if(_telemetryTXBuffer != rstatus.txbuf) {
        _telemetryTXBuffer = rstatus.txbuf;
        emit telemetryTXBufferChanged(_telemetryTXBuffer);
    }
    if(_telemetryLNoise != lnoise) {
        _telemetryLNoise = lnoise;
        emit telemetryLNoiseChanged(_telemetryLNoise);
    }
    if(_telemetryRNoise != rnoise) {
        _telemetryRNoise = rnoise;
        emit telemetryRNoiseChanged(_telemetryRNoise);
    }
}

void Vehicle::_handleRCChannels(mavlink_message_t& message)
{
    mavlink_rc_channels_t channels;

    mavlink_msg_rc_channels_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
        &channels.chan9_raw,
        &channels.chan10_raw,
        &channels.chan11_raw,
        &channels.chan12_raw,
        &channels.chan13_raw,
        &channels.chan14_raw,
        &channels.chan15_raw,
        &channels.chan16_raw,
        &channels.chan17_raw,
        &channels.chan18_raw,
    };
    int pwmValues[cMaxRcChannels];

    for (int i=0; i<cMaxRcChannels; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (i < channels.chancount) {
            pwmValues[i] = channelValue == UINT16_MAX ? -1 : channelValue;
        } else {
            pwmValues[i] = -1;
        }
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channels.chancount, pwmValues);
}

// Pop warnings ignoring for mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#else
#pragma warning(pop, 0)
#endif

bool Vehicle::_sendParameterCacheHashAck(LinkInterface* expectedLink, mavlink_message_t message,
                                        std::function<bool()> sourceFence)
{
    if (QThread::currentThread() != thread() || !sourceFence || !componentInformationCallbackAllowed()
        || id() < 1 || id() > 255 || message.msgid != MAVLINK_MSG_ID_PARAM_SET
        || message.len != MAVLINK_MSG_ID_PARAM_SET_LEN) return false;
    mavlink_param_set_t parameter{};
    mavlink_msg_param_set_decode(&message, &parameter);
    char expectedId[sizeof(parameter.param_id)]{};
    const char hashId[] = "_HASH_CHECK";
    memcpy(expectedId, hashId, sizeof(hashId));
    if (memcmp(parameter.param_id, expectedId, sizeof(expectedId)) != 0
        || parameter.param_type != MAV_PARAM_TYPE_UINT32 || parameter.target_system != id()
        || parameter.target_component < 1) return false;
    const mavlink_message_t original = message;
    const QPointer<Vehicle> self(this);
    // The private caller retains its original strong Link for this entire
    // call. Never promote a later PRIMARY in this final-write bridge.
    const QPointer<LinkInterface> link(expectedLink);
    const QPointer<FirmwarePlugin> plugin(_messageIntervalSourcePlugin);
    const auto current = [self, link, plugin, &sourceFence]() {
        const auto memoryCurrent = [&]() {
            return self && link && plugin && self->componentInformationCallbackAllowed()
                && self->_firmwarePlugin == plugin.data() && self->_messageIntervalSourcePlugin == plugin;
        };
        if (!memoryCurrent() || !sourceFence()) return false;
        return memoryCurrent();
    };
    if (!current()) return false;
    const bool connected = link->isConnected();
    if (!current() || !connected) return false;
    plugin->adjustOutgoingMavlinkMessageThreadSafe(self.data(), link.data(), &message);
    if (!current() || message.msgid != original.msgid || message.len != original.len
        || message.sysid != original.sysid || message.compid != original.compid
        || memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) != 0) return false;
    // The fixed HASH id, target, type and all original value bits must survive
    // plugin adjustment. This bridge cannot send another parameter or command.
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN || !current()) return false;
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    // Entry to the original Link writer is a committed local fact. A later
    // retirement cannot make it not-issued; delivery/remote ACK is not proven.
    if (self) {
        self->_messagesSent++;
        emit self->messagesSentChanged();
    }
    return true;
}

namespace {
quint32 parameterImportValueBits(const mavlink_param_set_t& parameter)
{
    static_assert(sizeof(float) == sizeof(quint32) && std::numeric_limits<float>::is_iec559,
                  "Parameter import requires IEEE binary32");
    quint32 bits = 0;
    // Do not evaluate a bytewise integer's float alias: those bits may be an
    // IEEE NaN representation even though the underlying integer is valid.
    std::memcpy(&bits, reinterpret_cast<const char*>(&parameter) + offsetof(mavlink_param_set_t, param_value), sizeof(bits));
    return bits;
}

bool parameterImportNameValid(const char* name, int size)
{
    int length = 0;
    bool ended = false;
    for (int index = 0; index < size; ++index) {
        const auto value = static_cast<unsigned char>(name[index]);
        if (value == 0) { ended = true; continue; }
        if (ended || value < 0x21 || value > 0x7e) return false;
        ++length;
    }
    constexpr char hashName[] = "_HASH_CHECK";
    return length > 0 && !(length == int(sizeof(hashName) - 1)
        && std::memcmp(name, hashName, sizeof(hashName) - 1) == 0);
}

bool parameterImportSame(const mavlink_param_set_t& left, const mavlink_param_set_t& right)
{
    return left.target_system == right.target_system && left.target_component == right.target_component
        && left.param_type == right.param_type
        && std::memcmp(left.param_id, right.param_id, sizeof(left.param_id)) == 0
        && parameterImportValueBits(left) == parameterImportValueBits(right);
}

// Validate the seven actual PARAM_SET primitive types, including zero upper
// bytes for narrow unions. Only the actual APM numeric mode may change bits.
bool parameterImportExpected(const mavlink_param_set_t& logical, bool numericFloat, mavlink_param_set_t& expected)
{
    if (!logical.target_system || !logical.target_component
        || !parameterImportNameValid(logical.param_id, sizeof(logical.param_id))) return false;
    const quint32 bits = parameterImportValueBits(logical);
    double integer = 0;
    switch (logical.param_type) {
    case MAV_PARAM_TYPE_UINT8:
        if (bits > 0xffU) return false;
        integer = bits; break;
    case MAV_PARAM_TYPE_INT8:
        if (bits > 0xffU) return false;
        integer = (bits & 0x80U) ? int(bits) - 0x100 : int(bits); break;
    case MAV_PARAM_TYPE_UINT16:
        if (bits > 0xffffU) return false;
        integer = bits; break;
    case MAV_PARAM_TYPE_INT16:
        if (bits > 0xffffU) return false;
        integer = (bits & 0x8000U) ? int(bits) - 0x10000 : int(bits); break;
    case MAV_PARAM_TYPE_UINT32:
        integer = bits; break;
    case MAV_PARAM_TYPE_INT32: {
        qint32 value = 0;
        std::memcpy(&value, &bits, sizeof(value));
        integer = value; break;
    }
    case MAV_PARAM_TYPE_REAL32:
        if (!std::isfinite(logical.param_value)) return false;
        std::memcpy(&expected, &logical, sizeof(expected)); return true; // Also preserves the sign of zero.
    default:
        return false; // Never use the legacy INT32 fallback for 64-bit/REAL64.
    }
    std::memcpy(&expected, &logical, sizeof(expected));
    if (numericFloat) {
        const float converted = static_cast<float>(integer);
        // Compare in double before ANY inverse integer cast. E.g. rounded
        // INT32_MAX/UINT32_MAX may be outside their source integer domains.
        if (!std::isfinite(converted) || static_cast<double>(converted) != integer) return false;
        expected.param_value = converted;
    }
    return true;
}
} // namespace

bool Vehicle::_parameterImportEncoding(int componentId, bool& numericFloat) const
{
    numericFloat = false;
    if (QThread::currentThread() != thread() || !componentInformationCallbackAllowed()
        || id() < 1 || id() > 255 || componentId < 1 || componentId > 255) return false;
    const QPointer<FirmwarePlugin> plugin(_messageIntervalSourcePlugin);
    if (!plugin || plugin.data() != _firmwarePlugin || plugin->thread() != thread()) return false;
#ifdef MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION
    if (const auto* apm = qobject_cast<APMFirmwarePlugin*>(plugin.data()))
        return apm->parameterSetEncodingMode(id(), componentId, numericFloat);
#endif
    // A stripped/custom plugin is not guessed to use APM's per-component
    // conversion. Its outgoing adjustment must preserve the bytewise value.
    return true;
}

bool Vehicle::_sendParameterImport(LinkInterface* original, mavlink_message_t logical,
                                  const mavlink_param_set_t& expectedWire, std::function<bool()> sourceFence,
                                  const std::shared_ptr<ParameterImportWriteLedger>& ledger)
{
    // These may originate in a PM row that a callback retires. Own the value
    // and native ledger before the first potentially reentrant boundary.
    mavlink_param_set_t expected{};
    std::memcpy(&expected, &expectedWire, sizeof(expected));
    const auto writes = ledger;
    const quint64 enteredBefore = writes ? writes->entered : 0;
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> link(original);
    const QPointer<FirmwarePlugin> plugin(_messageIntervalSourcePlugin);
    if (QThread::currentThread() != thread() || !sourceFence || !writes || !link || !plugin
        || !componentInformationCallbackAllowed() || id() < 1 || id() > 255
        || logical.msgid != MAVLINK_MSG_ID_PARAM_SET || logical.len != MAVLINK_MSG_ID_PARAM_SET_LEN) return false;
    mavlink_param_set_t parameter{};
    mavlink_msg_param_set_decode(&logical, &parameter);
    bool numericFloat = false;
    mavlink_param_set_t derived{};
    if (parameter.target_system != id() || !_parameterImportEncoding(parameter.target_component, numericFloat)
        || !parameterImportExpected(parameter, numericFloat, derived) || !parameterImportSame(derived, expected)) return false;
    const auto memoryCurrent = [self, link, plugin, writes, parameter, numericFloat, enteredBefore]() {
        if (!self || !link || !plugin || !self->componentInformationCallbackAllowed()
            || self->_firmwarePlugin != plugin.data() || self->_messageIntervalSourcePlugin != plugin
            || self->id() != parameter.target_system || writes->entered != enteredBefore
            || writes->entered == std::numeric_limits<quint64>::max()
            || self->armed() || self->flying() || self->isOfflineEditingVehicle() || !self->isInitialConnectComplete()) return false;
        bool currentEncoding = false;
        return self->_parameterImportEncoding(parameter.target_component, currentEncoding) && currentEncoding == numericFloat;
    };
    const auto current = [&]() {
        return memoryCurrent() && sourceFence() && memoryCurrent();
    };
    if (!current()) return false;
    const bool connected = link->isConnected();
    if (!current() || !connected) return false;
    const mavlink_message_t originalLogical = logical;
    // Each retry starts with the frozen ORIGINAL logical union, never the
    // previous adjusted result. This retains APM's one legitimate conversion.
    plugin->adjustOutgoingMavlinkMessageThreadSafe(self.data(), link.data(), &logical);
    if (!current() || logical.msgid != originalLogical.msgid || logical.len != originalLogical.len
        || logical.sysid != originalLogical.sysid || logical.compid != originalLogical.compid) return false;
    mavlink_param_set_t adjusted{};
    mavlink_msg_param_set_decode(&logical, &adjusted);
    if (!parameterImportSame(adjusted, expected)) return false;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &logical);
    LinkInterface* const writer = link.data();
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN || !current()) return false;
    // No notification, virtual or helper call between this local fact and the
    // original writer entry. Synchronous receive/cancel can already see it.
    ++writes->entered;
    writer->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    // Do not turn possibly queued into false after a late cancel/destruction.
    if (self) { ++self->_messagesSent; emit self->messagesSentChanged(); }
    return true;
}

bool Vehicle::sendMessageOnLinkThreadSafe(LinkInterface* link, mavlink_message_t message)
{
    // 510/511/512 may only use the reserved native command sender. Check
    // again after the plugin: another raw command must not become a bypass.
    if (rawMessageCommand(message)) return false;
    const auto cameraWriteBlocked = [this]() {
        return !_messageCommandLaneId.isEmpty() || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
            || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
            || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active);
    };
    if (rawCameraStreamMutationCommand(message) && cameraWriteBlocked()) return false;
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> actualLink(link);
    const QPointer<FirmwarePlugin> plugin(_messageIntervalSourcePlugin);
    if (!actualLink || !plugin || plugin.data() != _firmwarePlugin) return false;
    const bool connected = actualLink->isConnected();
    if (!self || !actualLink || !plugin || plugin.data() != _firmwarePlugin) return false;
    if (!connected) {
        qCDebug(VehicleLog) << "sendMessageOnLinkThreadSafe" << link << "not connected!";
        return false;
    }

    // Give the plugin a chance to adjust
    plugin->adjustOutgoingMavlinkMessageThreadSafe(this, actualLink.data(), &message);
    if (!self || !actualLink || !plugin || plugin.data() != _firmwarePlugin || rawMessageCommand(message)) return false;
    if (rawCameraStreamMutationCommand(message) && cameraWriteBlocked()) return false;

    // Write message into buffer, prepending start sign
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    if (len < 1 || len > MAVLINK_MAX_PACKET_LEN) return false;
    if (rawCameraStreamMutationCommand(message) && !_advanceFleetCameraStreamMutation({})) return false;
    actualLink->writeBytesThreadSafe((const char*)buffer, len);
    if (!self) return true;
    _messagesSent++;
    emit messagesSentChanged();

    return true;
}

int Vehicle::motorCount()
{
    switch (_vehicleType) {
    case MAV_TYPE_HELICOPTER:
        return 1;
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        return 2;
    case MAV_TYPE_TRICOPTER:
        return 3;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        return 4;
    case MAV_TYPE_HEXAROTOR:
        return 6;
    case MAV_TYPE_OCTOROTOR:
        return 8;
    case MAV_TYPE_SUBMARINE:
    {
        // Supported frame types
        enum {
            SUB_FRAME_BLUEROV1,
            SUB_FRAME_VECTORED,
            SUB_FRAME_VECTORED_6DOF,
            SUB_FRAME_VECTORED_6DOF_90DEG,
            SUB_FRAME_SIMPLEROV_3,
            SUB_FRAME_SIMPLEROV_4,
            SUB_FRAME_SIMPLEROV_5,
            SUB_FRAME_CUSTOM
        };

        if (!parameterManager()) return 0; // Unknown layout, not a stale manager.
        uint8_t frameType = parameterManager()->getParameter(_compID, "FRAME_CONFIG")->rawValue().toInt();

        switch (frameType) {  // ardupilot/libraries/AP_Motors/AP_Motors6DOF.h sub_frame_t

        case SUB_FRAME_BLUEROV1:
        case SUB_FRAME_VECTORED:
            return 6;

        case SUB_FRAME_SIMPLEROV_3:
            return 3;

        case SUB_FRAME_SIMPLEROV_4:
            return 4;

        case SUB_FRAME_SIMPLEROV_5:
            return 5;

        case SUB_FRAME_VECTORED_6DOF:
        case SUB_FRAME_VECTORED_6DOF_90DEG:
        case SUB_FRAME_CUSTOM:
            return 8;

        default:
            return -1;
        }
    }

    default:
        return -1;
    }
}

bool Vehicle::coaxialMotors()
{
    return _firmwarePlugin->multiRotorCoaxialMotors(this);
}

bool Vehicle::xConfigMotors()
{
    return _firmwarePlugin->multiRotorXConfig(this);
}

QString Vehicle::formattedMessages()
{
    QString messages;
    for(UASMessage* message: _toolbox->uasMessageHandler()->messages()) {
        messages += message->getFormatedText();
    }
    return messages;
}

void Vehicle::clearMessages()
{
    _toolbox->uasMessageHandler()->clearMessages();
}

void Vehicle::_handletextMessageReceived(UASMessage* message)
{
    if (message) {
        emit newFormattedMessage(message->getFormatedText());
    }
}

void Vehicle::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged();
        emit messageTypeChanged();
        emit messageCountChanged();
        return;
    }

    UASMessageHandler* pMh = _toolbox->uasMessageHandler();
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged();
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged();
    }
}

void Vehicle::resetAllMessages()
{
    // Reset Counts
    int count = _currentMessageCount;
    MessageType_t type = _currentMessageType;
    _currentErrorCount   = 0;
    _currentWarningCount = 0;
    _currentNormalCount  = 0;
    _currentMessageCount = 0;
    _currentMessageType = MessageNone;
    if(count != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        emit messageTypeChanged();
    }
}

// Reset warning counts only
void Vehicle::resetErrorLevelMessages()
{
    int prevMessageCount = _currentMessageCount;
    MessageType_t prevMessagetype = _currentMessageType;

    _currentMessageCount -= _currentErrorCount;
    _currentErrorCount   = 0;

    if (_currentWarningCount > 0) {
        _currentMessageType = MessageWarning;
    } else if (_currentNormalCount > 0) {
        _currentMessageType = MessageNormal;
    } else {
        _currentMessageType = MessageNone;
    }

    if (prevMessageCount != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if (prevMessagetype != _currentMessageType) {
        emit messageTypeChanged();
    }
}

// this function called in three cases:
// 1. On constructor of vehicle, to see if we should enable a joystick
// 2. When there is a new active joystick
// 3. When the active joystick is disconnected (even if there isnt a new one)
void Vehicle::_loadJoystickSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));

    if (_toolbox->joystickManager()->activeJoystick()) {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Notified of an active joystick. Loading setting joystickenabled: " << settings.value(_joystickEnabledSettingsKey, false).toBool();
        setJoystickEnabled(settings.value(_joystickEnabledSettingsKey, false).toBool());
    } else {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Notified that there is no active joystick";
        setJoystickEnabled(false);
    }
}

// This is called from the UI when a deliberate action is taken to enable or disable the joystick
// This save allows the joystick enable state to persist restarts, disconnections of the joystick etc
void Vehicle::saveJoystickSettings()
{
    QSettings settings;
    settings.beginGroup(QString(_settingsGroup).arg(_id));

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (_toolbox->joystickManager()->joysticks().count()) {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Saving setting joystickenabled: " << _joystickEnabled;
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

bool Vehicle::joystickEnabled() const
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    if (enabled){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Joystick Enabled";
    }
    else {
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Joystick Disabled";
    }

    // _joystickEnabled is the runtime state - it determines whether a vehicle is using joystick data when it is active
    _joystickEnabled = enabled;

    // if we are the active vehicle, call start polling on the active joystick
    // This routes the joystick signals to this vehicle
    if (enabled && _toolbox->multiVehicleManager()->activeVehicle() == this){
        _captureJoystick();
    }

    emit joystickEnabledChanged(_joystickEnabled);
}

void Vehicle::_activeVehicleChanged(Vehicle *newActiveVehicle)
{
    // the new active vehicle should always capture the joystick
    // even if the new active vehicle has joystick disabled
    // capturing the joystick will stop the joystick data going to the inactive vehicle
    if (newActiveVehicle == this){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " is the new active vehicle";
        _captureJoystick();
    }
}

// tells the active joystick where to send data
void Vehicle::_captureJoystick()
{
    Joystick* joystick = _joystickManager->activeJoystick();

    if(joystick){
        qCDebug(JoystickLog) << "Vehicle " << this->id() << " Capture Joystick" << joystick->name();
        joystick->startPolling(this);
    }
}


QGeoCoordinate Vehicle::homePosition()
{
    return _homePosition;
}

void Vehicle::setArmed(bool armed, bool showError)
{
    if (!_commandTransportAvailable(armed ? "arm" : "disarm")) {
        return;
    }

    // We specifically use COMMAND_LONG:MAV_CMD_COMPONENT_ARM_DISARM since it is supported by more flight stacks.
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   showError,
                   armed ? 1.0f : 0.0f);
}

void Vehicle::forceArm(void)
{
    if (!_commandTransportAvailable("forceArm")) {
        return;
    }

    sendMavCommand(_defaultComponentId,
                   MAV_CMD_COMPONENT_ARM_DISARM,
                   true,    // show error if fails
                   1.0f,    // arm
                   2989);   // force arm
}

bool Vehicle::flightModeSetAvailable()
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::SetFlightModeCapability);
}

QStringList Vehicle::flightModes()
{
	if (_standardModes->supported()) {
		return _standardModes->flightModes();
	}
    return _firmwarePlugin->flightModes(this);
}

QString Vehicle::flightMode() const
{
    if (_standardModes->supported()) {
        return _standardModes->flightMode(_custom_mode);
    }
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

bool Vehicle::setFlightModeCustom(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    if (_standardModes->supported()) {
        *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        return _standardModes->setFlightMode(flightMode, custom_mode);
    }
    return _firmwarePlugin->setFlightMode(flightMode, base_mode, custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    if (!_commandTransportAvailable("setFlightMode")) {
        return;
    }

    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (setFlightModeCustom(flightMode, &base_mode, &custom_mode)) {
        SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
        if (!sharedLink) {
            qCDebug(VehicleLog) << "setFlightMode: primary link gone!";
            return;
        }

        uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;

        // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
        // states.
        newBaseMode |= base_mode;

        if (_firmwarePlugin->MAV_CMD_DO_SET_MODE_is_supported()) {
            sendMavCommand(defaultComponentId(),
                           MAV_CMD_DO_SET_MODE,
                           true,    // show error if fails
                           MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                           custom_mode);
        } else {
            mavlink_message_t msg;
            mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                           _mavlink->getComponentId(),
                                           sharedLink->mavlinkChannel(),
                                           &msg,
                                           id(),
                                           newBaseMode,
                                           custom_mode);
            sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        }
    } else {
        qCWarning(VehicleLog) << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
    }
}

#if 0
QVariantList Vehicle::links() const {
    QVariantList ret;

    for( const auto &item: _links )
        ret << QVariant::fromValue(item);

    return ret;
}
#endif

void Vehicle::requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple)
{
    // Stream requests are also used during the initial handshake, so this
    // intentionally does not require initialConnectComplete. It still must
    // fail closed when a stale/offline vehicle no longer has a live manager or
    // primary link.
    if (_offlineEditingVehicle || !_vehicleLinkManager
            || _vehicleLinkManager->communicationLost()
            || _vehicleLinkManager->primaryLink().expired()) {
        qCDebug(VehicleLog) << "requestDataStream blocked: vehicle transport unavailable";
        return;
    }
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "requestDataStream: primary link gone!";
        return;
    }

    mavlink_message_t               msg;
    mavlink_request_data_stream_t   dataStream;

    memset(&dataStream, 0, sizeof(dataStream));

    dataStream.req_stream_id = stream;
    dataStream.req_message_rate = rate;
    dataStream.start_stop = 1;  // start
    dataStream.target_system = id();
    dataStream.target_component = _defaultComponentId;

    mavlink_msg_request_data_stream_encode_chan(_mavlink->getSystemId(),
                                                _mavlink->getComponentId(),
                                                sharedLink->mavlinkChannel(),
                                                &msg,
                                                &dataStream);

    if (sendMultiple) {
        // We use sendMessageMultiple since we really want these to make it to the vehicle
        sendMessageMultiple(msg);
    } else {
        sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void Vehicle::_sendMessageMultipleNext()
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

        if (!_vehicleLinkManager || _vehicleLinkManager->communicationLost()
                || _vehicleLinkManager->primaryLink().expired()) {
            // Do not retain a retry queue that can outlive its link and later
            // send stale handshake data to a replacement vehicle.
            _sendMessageMultipleList.clear();
            _nextSendMessageMultipleIndex = 0;
            return;
        }
        SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
        if (sharedLink) {
            sendMessageOnLinkThreadSafe(sharedLink.get(), _sendMessageMultipleList[_nextSendMessageMultipleIndex].message);
        }

        if (--_sendMessageMultipleList[_nextSendMessageMultipleIndex].retryCount <= 0) {
            _sendMessageMultipleList.removeAt(_nextSendMessageMultipleIndex);
        } else {
            _nextSendMessageMultipleIndex++;
        }
    }

    if (_nextSendMessageMultipleIndex >= _sendMessageMultipleList.count()) {
        _nextSendMessageMultipleIndex = 0;
    }
}

void Vehicle::sendMessageMultiple(mavlink_message_t message)
{
    SendMessageMultipleInfo_t   info;

    info.message =      message;
    info.retryCount =   _sendMessageMultipleRetries;

    _sendMessageMultipleList.append(info);
}

void Vehicle::_missionManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showAppMessage(tr("Mission transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_geoFenceManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showAppMessage(tr("GeoFence transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_rallyPointManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showAppMessage(tr("Rally Point transfer failed. Error: %1").arg(errorMsg));
}

void Vehicle::_clearCameraTriggerPoints()
{
    _cameraTriggerPoints.clearAndDeleteContents();
}

void Vehicle::_clearCameraTriggerPointsWithOperationId(quint64 operationId, bool status)
{
    Q_UNUSED(status)
    if (operationId != 0) {
        _clearCameraTriggerPoints();
    }
}

void Vehicle::_clearTrajectoryPointsWithOperationId(quint64 operationId, bool status)
{
    Q_UNUSED(status)
    if (operationId != 0 && _trajectoryPoints) {
        _trajectoryPoints->clear();
    }
}

void Vehicle::_flightTimerStart()
{
    _flightTimer.start();
    _flightTimeUpdater.start();
    _flightDistanceFact.setRawValue(0);
    _flightTimeFact.setRawValue(0);
}

void Vehicle::_flightTimerStop()
{
    _flightTimeUpdater.stop();
}

void Vehicle::_updateFlightTime()
{
    _flightTimeFact.setRawValue((double)_flightTimer.elapsed() / 1000.0);
}

void Vehicle::_gotProgressUpdate(float progressValue)
{
    if (sender() != _initialConnectStateMachine && _initialConnectStateMachine->active()) {
        return;
    }
    if (sender() == _initialConnectStateMachine && !_initialConnectStateMachine->active()) {
        progressValue = 0.f;
    }
    _loadProgress = progressValue;
    emit loadProgressChanged(progressValue);
}

void Vehicle::_firstMissionLoadComplete()
{
    _initialMissionLoadOperationId = 0;
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstMissionLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested)
{
    Q_UNUSED(removeAllRequested)
    if (!_initialConnectStateMachine->active() || operationId == 0 || operationId != _initialMissionLoadOperationId) {
        qCDebug(VehicleLog) << "Ignoring stale initial mission load completion" << operationId << _initialMissionLoadOperationId;
        return;
    }
    _initialMissionLoadOperationId = 0;
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstGeoFenceLoadComplete()
{
    _initialGeoFenceLoadOperationId = 0;
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstGeoFenceLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested)
{
    Q_UNUSED(removeAllRequested)
    if (!_initialConnectStateMachine->active() || operationId == 0 || operationId != _initialGeoFenceLoadOperationId) {
        qCDebug(VehicleLog) << "Ignoring stale initial geofence load completion" << operationId << _initialGeoFenceLoadOperationId;
        return;
    }
    _initialGeoFenceLoadOperationId = 0;
    _initialConnectStateMachine->advance();
}

void Vehicle::_firstRallyPointLoadComplete()
{
    const QPointer<Vehicle> self(this);
    const QPointer<InitialConnectStateMachine> machine(_initialConnectStateMachine);
    _initialRallyPointLoadOperationId = 0;
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
    if (self && machine && _initialConnectStateMachine == machine) machine->advance();
}

void Vehicle::_firstRallyPointLoadCompleteWithOperationId(quint64 operationId, bool removeAllRequested)
{
    Q_UNUSED(removeAllRequested)
    const QPointer<Vehicle> self(this);
    const QPointer<InitialConnectStateMachine> machine(_initialConnectStateMachine);
    if (!machine) return;
    if (!_initialConnectStateMachine->active() || operationId == 0 || operationId != _initialRallyPointLoadOperationId) {
        qCDebug(VehicleLog) << "Ignoring stale initial rally load completion" << operationId << _initialRallyPointLoadOperationId;
        return;
    }
    _initialRallyPointLoadOperationId = 0;
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
    if (self && machine && _initialConnectStateMachine == machine) machine->advance();
}

void Vehicle::_parametersReady(bool parametersReady)
{
    QPointer<Vehicle> owner(this);
    QPointer<ParameterManager> manager(_parameterManager);
    if (!manager || (sender() && sender() != manager.data())) return;
    qCDebug(VehicleLog) << "_parametersReady" << parametersReady;

    // Try to set current unix time to the vehicle
    _sendQGCTimeToVehicle();
    if (!owner || !manager || _parameterManager != manager) return;
    // Send time twice, more likely to get to the vehicle on a noisy link
    _sendQGCTimeToVehicle();
    if (!owner || !manager || _parameterManager != manager) return;
    if (parametersReady) {
        disconnect(manager.data(), &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);
        _setupAutoDisarmSignalling();
        if (!owner || !manager || _parameterManager != manager) return;
        _initialConnectStateMachine->advance();
        if (!owner || !manager || _parameterManager != manager) return;
    }

    const bool multirotorLimits = _firmwarePlugin->mulirotorSpeedLimitsAvailable(this);
    if (!owner || !manager || _parameterManager != manager) return;
    const bool fixedWingLimits = _firmwarePlugin->fixedWingAirSpeedLimitsAvailable(this);
    if (!owner || !manager || _parameterManager != manager) return;
    _multirotor_speed_limits_available = multirotorLimits;
    _fixed_wing_airspeed_limits_available = fixedWingLimits;

    emit haveMRSpeedLimChanged();
    if (!owner) return;
    emit haveFWSpeedLimChanged();
}

void Vehicle::_sendQGCTimeToVehicle()
{
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_sendQGCTimeToVehicle: primary link gone!";
        return;
    }

    mavlink_message_t       msg;
    mavlink_system_time_t   cmd;

    // Timestamp of the master clock in microseconds since UNIX epoch.
    cmd.time_unix_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch()*1000;
    // Timestamp of the component clock since boot time in milliseconds (Not necessary).
    cmd.time_boot_ms = 0;
    mavlink_msg_system_time_encode_chan(_mavlink->getSystemId(),
                                        _mavlink->getComponentId(),
                                        sharedLink->mavlinkChannel(),
                                        &msg,
                                        &cmd);

    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::_imageProtocolImageReady(void)
{
    QImage img = _imageProtocolManager->getImage();
    _toolbox->imageProvider()->setImage(&img, _id);
    _flowImageIndex++;
    emit flowImageIndexChanged();
}

void Vehicle::_remoteControlRSSIChanged(uint8_t rssi)
{
    //-- 0 <= rssi <= 100 - 255 means "invalid/unknown"
    if(rssi > 100) { // Anything over 100 doesn't make sense
        if(_rcRSSI != 255) {
            _rcRSSI = 255;
            emit rcRSSIChanged(_rcRSSI);
        }
        return;
    }
    //-- Initialize it
    if(_rcRSSIstore == 255.) {
        _rcRSSIstore = (double)rssi;
    }
    // Low pass to git rid of jitter
    _rcRSSIstore = (_rcRSSIstore * 0.9f) + ((float)rssi * 0.1);
    uint8_t filteredRSSI = (uint8_t)ceil(_rcRSSIstore);
    if(_rcRSSIstore < 0.1) {
        filteredRSSI = 0;
    }
    if(_rcRSSI != filteredRSSI) {
        _rcRSSI = filteredRSSI;
        emit rcRSSIChanged(_rcRSSI);
    }
}

void Vehicle::virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust)
{
    // The following if statement prevents the virtualTabletJoystick from sending values if the standard joystick is enabled
    if (!_joystickEnabled) {
        sendJoystickDataThreadSafe(
                    static_cast<float>(roll),
                    static_cast<float>(pitch),
                    static_cast<float>(yaw),
                    static_cast<float>(thrust),
                    0);
    }
}

void Vehicle::_say(const QString& text)
{
    _toolbox->audioOutput()->say(text.toLower());
}

bool Vehicle::airship() const
{
    return QGCMAVLink::isAirship(vehicleType());
}

bool Vehicle::fixedWing() const
{
    return QGCMAVLink::isFixedWing(vehicleType());
}

bool Vehicle::rover() const
{
    return QGCMAVLink::isRoverBoat(vehicleType());
}

bool Vehicle::sub() const
{
    return QGCMAVLink::isSub(vehicleType());
}

bool Vehicle::multiRotor() const
{
    return QGCMAVLink::isMultiRotor(vehicleType());
}

bool Vehicle::vtol() const
{
    return QGCMAVLink::isVTOL(vehicleType());
}

bool Vehicle::supportsThrottleModeCenterZero() const
{
    return _firmwarePlugin->supportsThrottleModeCenterZero();
}

bool Vehicle::supportsNegativeThrust()
{
    return _firmwarePlugin->supportsNegativeThrust(this);
}

bool Vehicle::supportsRadio() const
{
    return _firmwarePlugin->supportsRadio();
}

bool Vehicle::supportsJSButton() const
{
    return _firmwarePlugin->supportsJSButton();
}

bool Vehicle::supportsMotorInterference() const
{
    return _firmwarePlugin->supportsMotorInterference();
}

bool Vehicle::supportsTerrainFrame() const
{
    return !px4Firmware();
}

QString Vehicle::vehicleTypeName() const {
    static QMap<int, QString> typeNames = {
        { MAV_TYPE_GENERIC,         tr("Generic micro air vehicle" )},
        { MAV_TYPE_FIXED_WING,      tr("Fixed wing aircraft")},
        { MAV_TYPE_QUADROTOR,       tr("Quadrotor")},
        { MAV_TYPE_COAXIAL,         tr("Coaxial helicopter")},
        { MAV_TYPE_HELICOPTER,      tr("Normal helicopter with tail rotor.")},
        { MAV_TYPE_ANTENNA_TRACKER, tr("Ground installation")},
        { MAV_TYPE_GCS,             tr("Operator control unit / ground control station")},
        { MAV_TYPE_AIRSHIP,         tr("Airship, controlled")},
        { MAV_TYPE_FREE_BALLOON,    tr("Free balloon, uncontrolled")},
        { MAV_TYPE_ROCKET,          tr("Rocket")},
        { MAV_TYPE_GROUND_ROVER,    tr("Ground rover")},
        { MAV_TYPE_SURFACE_BOAT,    tr("Surface vessel, boat, ship")},
        { MAV_TYPE_SUBMARINE,       tr("Submarine")},
        { MAV_TYPE_HEXAROTOR,       tr("Hexarotor")},
        { MAV_TYPE_OCTOROTOR,       tr("Octorotor")},
        { MAV_TYPE_TRICOPTER,       tr("Octorotor")},
        { MAV_TYPE_FLAPPING_WING,   tr("Flapping wing")},
        { MAV_TYPE_KITE,            tr("Kite")},
        { MAV_TYPE_ONBOARD_CONTROLLER, tr("Onboard companion controller")},
        { MAV_TYPE_VTOL_TAILSITTER_DUOROTOR,   tr("Two-rotor VTOL using control surfaces in vertical operation in addition. Tailsitter")},
        { MAV_TYPE_VTOL_TAILSITTER_QUADROTOR,  tr("Quad-rotor VTOL using a V-shaped quad config in vertical operation. Tailsitter")},
        { MAV_TYPE_VTOL_TILTROTOR,  tr("Tiltrotor VTOL")},
        { MAV_TYPE_VTOL_FIXEDROTOR,  tr("VTOL Fixedrotor")},
        { MAV_TYPE_VTOL_TAILSITTER,  tr("VTOL Tailsitter")},
        { MAV_TYPE_VTOL_TILTWING,   tr("VTOL Tiltwing")},
        { MAV_TYPE_VTOL_RESERVED5,  tr("VTOL reserved 5")},
        { MAV_TYPE_GIMBAL,          tr("Onboard gimbal")},
        { MAV_TYPE_ADSB,            tr("Onboard ADSB peripheral")},
    };
    return typeNames[_vehicleType];
}

/// Returns the string to speak to identify the vehicle
QString Vehicle::_vehicleIdSpeech()
{
    if (_toolbox->multiVehicleManager()->vehicles()->count() > 1) {
        return tr("Vehicle %1 ").arg(id());
    } else {
        return QString();
    }
}

void Vehicle::_handleFlightModeChanged(const QString& flightMode)
{
    _say(tr("%1 %2 flight mode").arg(_vehicleIdSpeech()).arg(flightMode));
    emit guidedModeChanged(_firmwarePlugin->isGuidedMode(this));
}

void Vehicle::_announceArmedChanged(bool armed)
{
    _say(QString("%1 %2").arg(_vehicleIdSpeech()).arg(armed ? tr("armed") : tr("disarmed")));
    if(armed) {
        //-- Keep track of armed coordinates
        _armedPosition = _coordinate;
        emit armedPositionChanged();
    }
}

void Vehicle::_setFlying(bool flying)
{
    if (_flying != flying) {
        _flying = flying;
        emit flyingChanged(flying);
    }
}

void Vehicle::_setLanding(bool landing)
{
    if (armed() && _landing != landing) {
        _landing = landing;
        emit landingChanged(landing);
    }
}

bool Vehicle::guidedModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::GuidedModeCapability);
}

bool Vehicle::pauseVehicleSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::PauseVehicleCapability);
}

bool Vehicle::orbitModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::OrbitModeCapability);
}

bool Vehicle::roiModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::ROIModeCapability);
}

bool Vehicle::takeoffVehicleSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::TakeoffVehicleCapability);
}

QString Vehicle::gotoFlightMode() const
{
    return _firmwarePlugin->gotoFlightMode();
}

void Vehicle::guidedModeRTL(bool smartRTL)
{
    if (!_commandTransportAvailable("guidedModeRTL")) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeRTL(this, smartRTL);
}

void Vehicle::guidedModeLand()
{
    if (!_commandTransportAvailable("guidedModeLand")) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeLand(this);
}

void Vehicle::guidedModeTakeoff(double altitudeRelative)
{
    if (!_commandTransportAvailable("guidedModeTakeoff")
            || !std::isfinite(altitudeRelative)
            || altitudeRelative <= 0) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeTakeoff(this, altitudeRelative);
}

double Vehicle::minimumTakeoffAltitude()
{
    return _firmwarePlugin->minimumTakeoffAltitude(this);
}

double Vehicle::guidedTakeoffSpeed()
{
    return _firmwarePlugin->guidedTakeoffSpeed(this);
}

bool Vehicle::setGuidedTakeoffSpeed(double metersSecond)
{
    if (!_maintenanceCommandAllowed("setGuidedTakeoffSpeed") || !std::isfinite(metersSecond)) {
        return false;
    }
    return _firmwarePlugin->setGuidedTakeoffSpeed(this, metersSecond);
}

double Vehicle::maximumHorizontalSpeedMultirotor()
{
    return _firmwarePlugin->maximumHorizontalSpeedMultirotor(this);
}


double Vehicle::maximumEquivalentAirspeed()
{
    return _firmwarePlugin->maximumEquivalentAirspeed(this);
}


double Vehicle::minimumEquivalentAirspeed()
{
    return _firmwarePlugin->minimumEquivalentAirspeed(this);
}

bool Vehicle::hasGripper()  const 
{ 
    return _firmwarePlugin->hasGripper(this);
}

void Vehicle::startMission()
{
    if (!_commandTransportAvailable("startMission")) {
        return;
    }
    _firmwarePlugin->startMission(this);
}

void Vehicle::guidedModeGotoLocation(const QGeoCoordinate& gotoCoord)
{
    if (!_commandTransportAvailable("guidedModeGotoLocation")) {
        return;
    }
    if (!gotoCoord.isValid()
            || !std::isfinite(gotoCoord.latitude())
            || !std::isfinite(gotoCoord.longitude())) {
        qCWarning(VehicleLog) << "guidedModeGotoLocation blocked: invalid target coordinate";
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    if (!coordinate().isValid()) {
        return;
    }
    double maxDistance = _settingsManager->flyViewSettings()->maxGoToLocationDistance()->rawValue().toDouble();
    if (coordinate().distanceTo(gotoCoord) > maxDistance) {
        qgcApp()->showAppMessage(QString("New location is too far. Must be less than %1 %2.").arg(qRound(FactMetaData::metersToAppSettingsHorizontalDistanceUnits(maxDistance).toDouble())).arg(FactMetaData::appSettingsHorizontalDistanceUnitsString()));
        return;
    }
    _firmwarePlugin->guidedModeGotoLocation(this, gotoCoord);
}

void Vehicle::guidedModeChangeAltitude(double altitudeChange, bool pauseVehicle)
{
    if (!_commandTransportAvailable("guidedModeChangeAltitude") || !std::isfinite(altitudeChange)) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeChange, pauseVehicle);
}

void
Vehicle::guidedModeChangeGroundSpeedMetersSecond(double groundspeed)
{
    if (!_commandTransportAvailable("guidedModeChangeGroundSpeed") || !std::isfinite(groundspeed) || groundspeed < 0) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeGroundSpeedMetersSecond(this, groundspeed);
}

void
Vehicle::guidedModeChangeEquivalentAirspeedMetersSecond(double airspeed)
{
    if (!_commandTransportAvailable("guidedModeChangeEquivalentAirspeed") || !std::isfinite(airspeed) || airspeed < 0) {
        return;
    }
    if (!guidedModeSupported()) {
        qgcApp()->showAppMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeChangeEquivalentAirspeedMetersSecond(this, airspeed);
}

void Vehicle::guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double amslAltitude)
{
    if (!_commandTransportAvailable("guidedModeOrbit")
            || !centerCoord.isValid()
            || !std::isfinite(centerCoord.latitude())
            || !std::isfinite(centerCoord.longitude())
            || !std::isfinite(radius) || radius <= 0 || radius > 100000
            || !std::isfinite(amslAltitude)) {
        qCWarning(VehicleLog) << "guidedModeOrbit blocked: invalid transport or orbit arguments";
        return;
    }
    if (!orbitModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("Orbit mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_ORBIT,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(radius),
                    static_cast<float>(qQNaN()),    // Use default velocity
                    0,                              // Vehicle points to center
                    static_cast<float>(qQNaN()),    // reserved
                    centerCoord.latitude(), centerCoord.longitude(), static_cast<float>(amslAltitude));
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_ORBIT,
                    true,                           // show error if fails
                    static_cast<float>(radius),
                    static_cast<float>(qQNaN()),    // Use default velocity
                    0,                              // Vehicle points to center
                    static_cast<float>(qQNaN()),    // reserved
                    static_cast<float>(centerCoord.latitude()),
                    static_cast<float>(centerCoord.longitude()),
                    static_cast<float>(amslAltitude));
    }
}

void Vehicle::guidedModeROI(const QGeoCoordinate& centerCoord)
{
    if (!_commandTransportAvailable("guidedModeROI")
            || !centerCoord.isValid()
            || !std::isfinite(centerCoord.latitude())
            || !std::isfinite(centerCoord.longitude())
            || !std::isfinite(centerCoord.altitude())) {
        qCWarning(VehicleLog) << "guidedModeROI blocked: invalid transport or target coordinate";
        return;
    }
    if (!roiModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_LOCATION,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    centerCoord.latitude(),
                    centerCoord.longitude(),
                    static_cast<float>(centerCoord.altitude()));
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_LOCATION,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(centerCoord.latitude()),
                    static_cast<float>(centerCoord.longitude()),
                    static_cast<float>(centerCoord.altitude()));
    }
}

void Vehicle::stopGuidedModeROI()
{
    if (!_commandTransportAvailable("stopGuidedModeROI")) {
        return;
    }
    if (!roiModeSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("ROI mode not supported by Vehicle."));
        return;
    }
    if (capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT) {
        sendMavCommandInt(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_NONE,
                    MAV_FRAME_GLOBAL,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<double>(qQNaN()),   // Empty
                    static_cast<double>(qQNaN()),   // Empty
                    static_cast<float>(qQNaN()));   // Empty
    } else {
        sendMavCommand(
                    defaultComponentId(),
                    MAV_CMD_DO_SET_ROI_NONE,
                    true,                           // show error if fails
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()),    // Empty
                    static_cast<float>(qQNaN()));   // Empty
    }
}

void Vehicle::pauseVehicle()
{
    if (!_commandTransportAvailable("pauseVehicle")) {
        return;
    }
    if (!pauseVehicleSupported()) {
        qgcApp()->showAppMessage(QStringLiteral("Pause not supported by vehicle."));
        return;
    }
    _firmwarePlugin->pauseVehicle(this);
}

void Vehicle::abortLanding(double climbOutAltitude)
{
    if (!_commandTransportAvailable("abortLanding") || !std::isfinite(climbOutAltitude)) {
        return;
    }
    sendMavCommand(
                defaultComponentId(),
                MAV_CMD_DO_GO_AROUND,
                true,        // show error if fails
                static_cast<float>(climbOutAltitude));
}

bool Vehicle::guidedMode() const
{
    return _firmwarePlugin->isGuidedMode(this);
}

void Vehicle::setGuidedMode(bool guidedMode)
{
    if (!_commandTransportAvailable("setGuidedMode")) {
        return;
    }
    return _firmwarePlugin->setGuidedMode(this, guidedMode);
}

void Vehicle::emergencyStop()
{
    if (!_commandTransportAvailable("emergencyStop")) {
        return;
    }
    sendMavCommand(
                _defaultComponentId,
                MAV_CMD_COMPONENT_ARM_DISARM,
                true,        // show error if fails
                0.0f,
                21196.0f);  // Magic number for emergency stop
}

void Vehicle::setCurrentMissionSequence(int seq)
{
    if (!_commandTransportAvailable("setCurrentMissionSequence") || seq < 0 || seq > 65535) {
        return;
    }
    if (!_firmwarePlugin->sendHomePositionToVehicle() && seq == 0) {
        qCWarning(VehicleLog) << "setCurrentMissionSequence blocked: sequence 0 cannot be remapped without home item";
        return;
    }
    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "setCurrentMissionSequence: primary link gone!";
        return;
    }

    mavlink_message_t       msg;

    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        seq--;
    }
    mavlink_msg_mission_set_current_pack_chan(
                static_cast<uint8_t>(_mavlink->getSystemId()),
                static_cast<uint8_t>(_mavlink->getComponentId()),
                sharedLink->mavlinkChannel(),
                &msg,
                static_cast<uint8_t>(id()),
                _compID,
                static_cast<uint16_t>(seq));
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::sendMavCommand(int compId, MAV_CMD command, bool showError, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _sendMavCommandWorker(false,            // commandInt
                          showError,
                          nullptr,          // no handlers
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendCommand(int compId, int command, bool showError, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    sendMavCommand(
                compId, static_cast<MAV_CMD>(command),
                showError,
                static_cast<float>(param1),
                static_cast<float>(param2),
                static_cast<float>(param3),
                static_cast<float>(param4),
                static_cast<float>(param5),
                static_cast<float>(param6),
                static_cast<float>(param7));
}

void Vehicle::sendMavCommandWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    _sendMavCommandWorker(false,                // commandInt
                          false,                // showError
                          ackHandlerInfo,
                          compId,
                          command,
                          MAV_FRAME_GLOBAL,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandInt(int compId, MAV_CMD command, MAV_FRAME frame, bool showError, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _sendMavCommandWorker(true,         // commandInt
                          showError,
                          nullptr,      // no handlers
                          compId,
                          command,
                          frame,
                          param1, param2, param3, param4, param5, param6, param7);
}

void Vehicle::sendMavCommandIntWithHandler(const MavCmdAckHandlerInfo_t* ackHandlerInfo, int compId, MAV_CMD command, MAV_FRAME frame, float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    _sendMavCommandWorker(true,                   // commandInt
                          false,                  // showError
                          ackHandlerInfo,
                          compId,
                          command,
                          frame,
                          param1, param2, param3, param4, param5, param6, param7);
}

bool Vehicle::isMavCommandPending(int targetCompId, MAV_CMD command)
{
    if (_fleetStartupActive && targetCompId == _defaultComponentId
        && ((_fleetStartupStep == FleetStartupStep::NormalArm && command == MAV_CMD_COMPONENT_ARM_DISARM)
            || (_fleetStartupStep == FleetStartupStep::MissionModeOnly && command == MAV_CMD_DO_SET_MODE))) return true;
    return ((-1) < _findMavCommandListEntryIndex(targetCompId, command));
}

int Vehicle::_findMavCommandListEntryIndex(int targetCompId, MAV_CMD command)
{
    for (int i=0; i<_mavCommandList.count(); i++) {
        const MavCommandListEntry_t& entry = _mavCommandList[i];
        if (entry.targetCompId == targetCompId && entry.command == command) {
            return i;
        }
    }

    return -1;
}

quint64 Vehicle::_mavCommandQuarantineKey(int targetCompId, MAV_CMD command) const
{
    return (static_cast<quint64>(static_cast<uint32_t>(targetCompId)) << 32)
        | static_cast<uint32_t>(command);
}

void Vehicle::_purgeMavCommandQuarantine(qint64 now)
{
    if (!_mavCommandQuarantineClock.isValid()) {
        _mavCommandQuarantineClock.start();
    }
    if (now < 0) {
        now = _mavCommandQuarantineClock.elapsed();
    }

    for (auto it = _mavCommandQuarantine.begin(); it != _mavCommandQuarantine.end();) {
        if (it.value() <= now) {
            it = _mavCommandQuarantine.erase(it);
        } else {
            ++it;
        }
    }
}

bool Vehicle::_isMavCommandQuarantined(int targetCompId, MAV_CMD command)
{
    _purgeMavCommandQuarantine();
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    if (messageCommand(command) && _messageCommandQuarantineUntilMs > now) return true;
    const auto it = _mavCommandQuarantine.constFind(_mavCommandQuarantineKey(targetCompId, command));
    return it != _mavCommandQuarantine.constEnd() && it.value() > now;
}

void Vehicle::_quarantineMavCommand(int targetCompId, MAV_CMD command, int ackTimeoutMSecs)
{
    // Requests without a wire operation token must be quarantined after a
    // timeout. In particular, mode changes and arm/disarm commands cannot be
    // exempted: a delayed ACK from the previous request must not be consumed
    // by an immediate retry or by a later high-risk state transition.
    _purgeMavCommandQuarantine();
    // Use the same configured timeout horizon (including high latency), not
    // an arbitrary shorter cap. A monotonic clock tolerates wall-clock jumps.
    const int quarantineMSecs = qMax(1000, ackTimeoutMSecs);
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    const qint64 expiresAt = now > std::numeric_limits<qint64>::max() - quarantineMSecs
        ? std::numeric_limits<qint64>::max() : now + quarantineMSecs;
    if (messageCommand(command)) _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, expiresAt);
    _mavCommandQuarantine.insert(_mavCommandQuarantineKey(targetCompId, command), expiresAt);
}

bool Vehicle::_sendMavCommandShouldRetry(MAV_CMD command)
{
    switch (command) {
#ifdef QT_DEBUG
    // These MockLink command should be retried so we can create unit tests to test retry code
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED:
    case MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED:
    case MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE:
        return true;
#endif

        // In general we should not retry any commands. This is for safety reasons. For example you don't want an ARM command
        // to timeout with no response over a noisy link twice and then suddenly have the third try work 6 seconds later. At that
        // point the user could have walked up to the vehicle to see what is going wrong.
        //
        // We do retry commands which are part of the initial vehicle connect sequence. This makes this process work better over noisy
        // links where commands could be lost. Also these commands tend to just be requesting status so if they end up being delayed
        // there are no safety concerns that could occur.
    case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
    case MAV_CMD_REQUEST_PROTOCOL_VERSION:
    case MAV_CMD_REQUEST_MESSAGE:
    case MAV_CMD_PREFLIGHT_STORAGE:
    case MAV_CMD_RUN_PREARM_CHECKS:
        return true;

    default:
        return false;
    }
}

bool Vehicle::_commandCanBeDuplicated(MAV_CMD command)
{
    // For some commands we don't care about response as much as we care about sending them regularly.
    // This test avoids commands not being sent due to an ACK not being received yet.
    // MOTOR_TEST in ardusub is a case where we need a constant stream of commands so it doesn't time out.
    switch (command) {
    case MAV_CMD_DO_MOTOR_TEST:
        return true;
    default:
        return false;
    }
}

bool Vehicle::_messageIntervalSourceCurrent(const MavCommandListEntry_t& entry) const
{
    if ((!messageCommand(entry.command) && (entry.fleetCameraStreamControlRunId.isEmpty() || !cameraStreamMutationCommand(entry.command)))
        || entry.messageIntervalRequestId.isEmpty()
        || QThread::currentThread() != thread() || _offlineEditingVehicle || _autopilotVersionObservationRemoving
        || _messageCommandLaneRetired || _messageCommandLaneExhausted
        || _messageCommandLaneId != entry.messageIntervalRequestId || _messageCommandLaneSerial != entry.messageCommandSerial
        || !_mavCommandQuarantineClock.isValid() || entry.messageCommandDeadlineMs < 0
        || _messageCommandLaneDeadlineMs != entry.messageCommandDeadlineMs
        || _mavCommandQuarantineClock.elapsed() >= entry.messageCommandDeadlineMs) return false;
    return _messageCommandSourceMemoryCurrent(entry);
}

bool Vehicle::_messageCommandSourceMemoryCurrent(const MavCommandListEntry_t& entry) const
{
    // The identity part is also used by retained completed read receipts.
    // It deliberately neither requires a live queue lease nor calls Link.
    if (QThread::currentThread() != thread() || _offlineEditingVehicle || _autopilotVersionObservationRemoving
        || !entry.messageIntervalLinks || entry.messageIntervalLinks.data() != _vehicleLinkManager
        || entry.messageIntervalLinks != _fleetExecutionSourceLinks
        || !entry.messageIntervalProtocol || entry.messageIntervalProtocol != _fleetExecutionSourceProtocol
        || entry.messageIntervalProtocol.data() != _mavlink
        || !entry.messageIntervalPlugin || entry.messageIntervalPlugin != _messageIntervalSourcePlugin
        || entry.messageIntervalPlugin.data() != _firmwarePlugin
        || !entry.messageIntervalPrimary || entry.messageIntervalPrimaryOwner.expired()
        || !_fleetExecutionSourceToolbox || !qgcApp() || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || _fleetExecutionSourceToolbox->mavlinkProtocol() != entry.messageIntervalProtocol.data()
        || !_fleetExecutionSourceVehicles
        || _fleetExecutionSourceToolbox->multiVehicleManager() != _fleetExecutionSourceVehicles.data()
        || entry.messageIntervalSourceEpoch.isEmpty()
        || entry.messageCommandTransportEpoch.isEmpty() || entry.messageCommandTransportEpoch != _messageCommandTransportEpoch
        || entry.messageCommandVehicleId != _id || entry.messageCommandDefaultComponentId != _defaultComponentId
        || entry.messageCommandFirmwareType != int(_firmwareType) || entry.messageCommandVehicleType != int(_vehicleType)
        || (entry.fleetEvidenceReadOnly && entry.messageIntervalSourceEpoch != _autopilotVersionObservationEpoch)
        || entry.messageIntervalLinks->communicationLost()) return false;
    const WeakLinkInterfacePtr current = entry.messageIntervalLinks->primaryLink();
    if (current.expired() || current.owner_before(entry.messageIntervalPrimaryOwner)
        || entry.messageIntervalPrimaryOwner.owner_before(current)) return false;
    for (const auto& actual : entry.messageIntervalLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr owner(actual.link);
        if (actual.link && actual.link.get() == entry.messageIntervalPrimary.data()
            && !current.owner_before(owner) && !owner.owner_before(current)) return true;
    }
    return false;
}

int Vehicle::_messageCommandIndex(const QString& requestId) const
{
    if (requestId.isEmpty()) return -1;
    for (int index = 0; index < _mavCommandList.size(); ++index)
        if (_mavCommandList.at(index).messageIntervalRequestId == requestId) return index;
    return -1;
}

bool Vehicle::_reserveMessageCommand(MavCommandListEntry_t* entry)
{
    if (!entry || (!messageCommand(entry->command)
            && (entry->fleetCameraStreamControlRunId.isEmpty() || !cameraStreamMutationCommand(entry->command)))
        || QThread::currentThread() != thread()
        || _offlineEditingVehicle || _autopilotVersionObservationRemoving || _messageCommandLaneExhausted
        || !_messageCommandLaneId.isEmpty() || entry->targetCompId < 1 || entry->targetCompId > 255
        || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
        || !_messageIntervalSourcePlugin || _messageIntervalSourcePlugin.data() != _firmwarePlugin
        || !_fleetExecutionSourceToolbox || !qgcApp() || qgcApp()->toolbox() != _fleetExecutionSourceToolbox.data()
        || !_fleetExecutionSourceVehicles || _autopilotVersionObservationEpoch.isEmpty()
        || _fleetExecutionSourceLinks->communicationLost()
        || _waitForMavlinkMessageResultHandler || (_requestMessageContext && !_requestMessageContext->requestId.isEmpty())) return false;
    const auto configurationRun = _fleetOptionalTelemetryConfigurationRun;
    const auto controlRun = _fleetCameraStreamControlRun;
    if (controlRun && controlRun->active && entry->fleetCameraStreamControlRunId != controlRun->id) return false;
    if (!entry->fleetCameraStreamControlRunId.isEmpty()
        && (!controlRun || !controlRun->active || !controlRun->owner || entry->fleetCameraStreamControlRunId != controlRun->id)) return false;
    const auto cameraRun = _fleetCameraStreamRun;
    if (cameraRun && cameraRun->active && entry->fleetCameraStreamCaptureRunId != cameraRun->id) return false;
    if (!entry->fleetCameraStreamCaptureRunId.isEmpty()
        && (!cameraRun || !cameraRun->active || !cameraRun->owner || entry->fleetCameraStreamCaptureRunId != cameraRun->id)) return false;
    if (configurationRun && configurationRun->active
        && entry->fleetOptionalTelemetryConfigurationRunId != configurationRun->id) return false;
    if (!entry->fleetOptionalTelemetryConfigurationRunId.isEmpty()
        && (!configurationRun || !configurationRun->active || !configurationRun->owner
            || entry->fleetOptionalTelemetryConfigurationRunId != configurationRun->id)) return false;
    for (const auto& pending : _mavCommandList)
        if (messageCommand(pending.command) || cameraStreamMutationCommand(pending.command)) return false;
    if (_isMavCommandQuarantined(entry->targetCompId, entry->command)) return false;
    const WeakLinkInterfacePtr owner = _fleetExecutionSourceLinks->primaryLink();
    QPointer<LinkInterface> primary;
    for (const auto& actual : _fleetExecutionSourceLinks->_rgLinkInfo) {
        const WeakLinkInterfacePtr candidate(actual.link);
        if (actual.link && !owner.owner_before(candidate) && !candidate.owner_before(owner)) primary = actual.link.get();
    }
    if (owner.expired() || !primary) return false;
    const qint64 now = _mavCommandQuarantineClock.elapsed();
    // Reserve before any Link/plugin/authorization callback. The final
    // latency-specific deadline may only shorten this original horizon.
    const qint64 horizon = qint64(qMax(1, entry->maxTries)) * _mavCommandAckTimeoutMSecsHighLatency + 1000;
    if (_messageCommandLaneSerial == std::numeric_limits<quint64>::max()
        || now < 0 || now > std::numeric_limits<qint64>::max() - horizon) {
        _messageCommandLaneExhausted = true; return false;
    }
    entry->messageIntervalRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry->messageIntervalSourceEpoch = _autopilotVersionObservationEpoch;
    entry->messageCommandTransportEpoch = _messageCommandTransportEpoch;
    entry->messageCommandVehicleId = _id; entry->messageCommandDefaultComponentId = _defaultComponentId;
    entry->messageCommandFirmwareType = int(_firmwareType); entry->messageCommandVehicleType = int(_vehicleType);
    entry->messageIntervalLinks = _fleetExecutionSourceLinks;
    entry->messageIntervalProtocol = _fleetExecutionSourceProtocol;
    entry->messageIntervalPlugin = _messageIntervalSourcePlugin;
    entry->messageIntervalPrimary = primary; entry->messageIntervalPrimaryOwner = owner;
    entry->messageCommandSerial = ++_messageCommandLaneSerial;
    entry->messageCommandReservedAtMs = now; entry->messageCommandDeadlineMs = now + horizon;
    _messageCommandLaneId = entry->messageIntervalRequestId;
    _messageCommandLaneDeadlineMs = entry->messageCommandDeadlineMs;
    _messageCommandLaneRetired = false;
    if (_messageIntervalSourceCurrent(*entry)) return true;
    _releaseMessageCommand(entry->messageIntervalRequestId);
    return false;
}

void Vehicle::_releaseMessageCommand(const QString& requestId)
{
    if (requestId.isEmpty() || _messageCommandLaneId != requestId) return;
    _messageCommandLaneId.clear(); _messageCommandLaneDeadlineMs = -1;
    _messageCommandLaneRetired = false;
}

void Vehicle::_finishMessageCommand(const QString& requestId, const mavlink_command_ack_t& ack,
    MavCmdResultFailureCode_t failure)
{
    const QPointer<Vehicle> self(this);
    const int index = _messageCommandIndex(requestId);
    if (index < 0) {
        const auto context = _requestMessageContext;
        if (context && context->requestId == requestId)
            _finishRequestMessage(context, MAV_RESULT_FAILED, RequestMessageFailureCommandNotAcked, mavlink_message_t{});
        else _releaseMessageCommand(requestId);
        return;
    }
    const MavCommandListEntry_t entry = _mavCommandList.takeAt(index);
    if (!entry.fleetCameraStreamControlRunId.isEmpty()) {
        const auto run = _fleetCameraStreamControlRun;
        if (!run || !run->active || run->id != entry.fleetCameraStreamControlRunId || run->stage > 1
            || (run->stage == 0 ? run->actionId : run->queryId) != requestId) { _releaseMessageCommand(requestId); return; }
        if (failure == MavCmdResultCommandResultOnly) {
            if (run->stage == 0) { run->actionAckObserved = true; run->actionAckResult = ack.result; }
            else { run->queryAckObserved = true; run->queryAckResult = ack.result; }
        }
        if (failure != MavCmdResultCommandResultOnly || ack.result != MAV_RESULT_ACCEPTED
            || !_fleetCameraStreamControlSourceCurrent(run, false)) {
            _finishFleetCameraStreamControl(run, QStringLiteral("failed"), QStringLiteral("camera-control-command-rejected-or-unconfirmed"), true);
        } else if (run->stage == 0) {
            run->actionState = QStringLiteral("complete"); run->actionReason = QStringLiteral("");
            _releaseMessageCommand(requestId); run->stage = 1; run->wireDeadlineMs = -1; run->state = QStringLiteral("ready");
        } else {
            const qint64 now = _mavCommandQuarantineClock.elapsed();
            const qint64 shortened = now > std::numeric_limits<qint64>::max() - 1000 ? std::numeric_limits<qint64>::max() : now + 1000;
            run->wireDeadlineMs = qMin(run->wireDeadlineMs, shortened);
            run->querySource.messageCommandDeadlineMs = run->wireDeadlineMs;
            _messageCommandLaneDeadlineMs = run->wireDeadlineMs; run->queryState = QStringLiteral("waiting-report");
            // A pre-ACK Status can have been superseded by a RUNNING-only
            // Info revision. Preserve that partial receipt, but wait for an
            // actual new coherent pair within this shortened original period.
            if (run->reportObserved) _completeFleetCameraStreamControlQuery(run);
        }
        return;
    }
    if (!entry.fleetCameraStreamCaptureRunId.isEmpty()) {
        const auto run = _fleetCameraStreamRun;
        if (!run || !run->active || run->id != entry.fleetCameraStreamCaptureRunId || run->stage >= 2
            || run->queries[run->stage].id != requestId) { _releaseMessageCommand(requestId); return; }
        auto& query = run->queries[run->stage];
        if (failure == MavCmdResultCommandResultOnly) { query.ackObserved = true; query.ackResult = ack.result; }
        if (failure != MavCmdResultCommandResultOnly || ack.result != MAV_RESULT_ACCEPTED
            || !_fleetCameraStreamRunSourceCurrent(run, false)) {
            _finishFleetCameraStreamCapture(run, QStringLiteral("failed"), QStringLiteral("camera-capture-command-rejected-or-unconfirmed"), true);
        } else if (query.reportObserved) _completeFleetCameraStreamQuery(run);
        else {
            const qint64 now = _mavCommandQuarantineClock.elapsed();
            const qint64 shortened = now > std::numeric_limits<qint64>::max() - 1000
                ? std::numeric_limits<qint64>::max() : now + 1000;
            query.deadlineMs = qMin(query.deadlineMs, shortened);
            query.source.messageCommandDeadlineMs = query.deadlineMs;
            _messageCommandLaneDeadlineMs = query.deadlineMs;
            query.state = QStringLiteral("waiting-report");
        }
        return;
    }
    if (!entry.fleetOptionalTelemetryConfigurationRunId.isEmpty() && entry.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        const auto run = _fleetOptionalTelemetryConfigurationRun;
        if (!run || run->id != entry.fleetOptionalTelemetryConfigurationRunId) { _releaseMessageCommand(requestId); return; }
        const int rowIndex = run->stage / 2;
        if (rowIndex < 0 || rowIndex > 2 || run->rows[rowIndex].setRequestId != requestId) {
            _releaseMessageCommand(requestId);
            _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-set-receipt-identity-mismatch"), true);
            return;
        }
        if (failure == MavCmdResultCommandResultOnly) {
            run->rows[rowIndex].setAckObserved = true; run->rows[rowIndex].setAckResult = ack.result;
        }
        const bool accepted = failure == MavCmdResultCommandResultOnly && ack.result == MAV_RESULT_ACCEPTED
            && _fleetOptionalTelemetryRunSourceCurrent(run, false);
        if (!accepted) {
            _quarantineMavCommand(entry.targetCompId, entry.command, 4000);
            _messageCommandQuarantineUntilMs = qMax(_messageCommandQuarantineUntilMs, entry.messageCommandDeadlineMs);
        }
        _releaseMessageCommand(requestId);
        if (!accepted) {
            _finishFleetOptionalTelemetryConfiguration(run, QStringLiteral("failed"), QStringLiteral("configuration-set-unconfirmed-or-rejected"), true);
        } else {
            ++run->stage; run->wireDeadlineMs = -1; run->state = QStringLiteral("ready");
        }
        return;
    }
    if (entry.fleetMessageIntervalQuery) {
        const auto query = _fleetMessageIntervalActiveQuery;
        if (!query || query->source.messageIntervalRequestId != requestId) {
            _releaseMessageCommand(requestId);
            return;
        }
        if (failure == MavCmdResultCommandResultOnly) {
            query->ackObserved = true; query->ackResult = ack.result;
        }
        if (failure != MavCmdResultCommandResultOnly || ack.result != MAV_RESULT_ACCEPTED
            || !_fleetMessageIntervalQuerySourceCurrent(query, false)) {
            _finishFleetMessageIntervalQuery(query, QStringLiteral("failed"),
                failure != MavCmdResultCommandResultOnly ? QStringLiteral("fleet-interval-query-unconfirmed")
                    : ack.result != MAV_RESULT_ACCEPTED ? QStringLiteral("fleet-interval-query-command-rejected")
                    : QStringLiteral("fleet-interval-query-source-changed"), true);
        } else if (query->reportObserved) {
            _finishFleetMessageIntervalQuery(query, QStringLiteral("complete"), QStringLiteral(""), false);
        } else {
            const qint64 now = _mavCommandQuarantineClock.elapsed();
            const qint64 shortened = now > std::numeric_limits<qint64>::max() - 1000
                ? std::numeric_limits<qint64>::max() : now + 1000;
            query->source.messageCommandDeadlineMs = qMin(query->source.messageCommandDeadlineMs, shortened);
            _messageCommandLaneDeadlineMs = query->source.messageCommandDeadlineMs;
            query->state = QStringLiteral("waiting-report");
        }
        return; // No global result signal or arbitrary user result callback.
    }
    if (failure != MavCmdResultCommandResultOnly)
        _quarantineMavCommand(entry.targetCompId, entry.command, entry.ackTimeoutMSecs);
    _quarantineFleetEvidenceCommand(entry); // Preserve the native refresh's existing disposal cooldown.
    const auto context = _requestMessageContext;
    const bool ownWaiter = context && context->requestId == requestId
        && entry.ackHandlerInfo.resultHandler == &Vehicle::_requestMessageCmdResultHandler
        && entry.ackHandlerInfo.resultHandlerData == context.get();
    if (!ownWaiter) _releaseMessageCommand(requestId);
    // Nothing in the live list or a container reference crosses this callback.
    if (entry.ackHandlerInfo.resultHandler)
        (*entry.ackHandlerInfo.resultHandler)(entry.ackHandlerInfo.resultHandlerData, entry.targetCompId, ack, failure);
    else emit mavCommandResult(_id, entry.targetCompId, entry.command, ack.result, failure);
    if (self) _showMessageCommandFailure(entry, ack, failure);
}

void Vehicle::_showMessageCommandFailure(const MavCommandListEntry_t& entry, const mavlink_command_ack_t& ack,
    MavCmdResultFailureCode_t failure)
{
    if (!entry.showError || (failure == MavCmdResultCommandResultOnly
        && (ack.result == MAV_RESULT_ACCEPTED || entry.ackHandlerInfo.resultHandler))
        || QThread::currentThread() != thread() || !qgcApp() || qgcApp()->thread() != thread()) return;
    const QPointer<QGCApplication> app(qgcApp());
    QString detail;
    if (failure == MavCmdResultFailureDuplicateCommand)
        detail = tr("A previous telemetry command or its response is still pending.");
    else if (failure == MavCmdResultFailureLateAckQuarantine)
        detail = tr("Waiting for delayed response quarantine to expire.");
    else if (failure != MavCmdResultCommandResultOnly)
        detail = tr("Result unconfirmed or no response; the request may already have been sent.");
    else detail = tr("Vehicle reported %1.").arg(QGCMAVLink::mavResultToString(static_cast<MAV_RESULT>(ack.result)));
    if (app) app->showAppMessage(tr("%1: %2").arg(messageCommandName(entry.command), detail));
    // The existing notification may delete Vehicle/application. No tail read.
}

void Vehicle::_sendMessageCommandWorker(bool commandInt, bool showError, const MavCmdAckHandlerInfo_t* handler,
    int componentId, MAV_CMD command, MAV_FRAME frame, float p1, float p2, float p3, float p4,
    double p5, double p6, float p7)
{
    const QPointer<Vehicle> self(this);
    MavCommandListEntry_t entry;
    entry.command = command; entry.targetCompId = componentId; entry.useCommandInt = commandInt;
    entry.frame = frame; entry.showError = showError; entry.ackHandlerInfo = handler ? *handler : MavCmdAckHandlerInfo_t{};
    entry.rgParam1 = p1; entry.rgParam2 = p2; entry.rgParam3 = p3; entry.rgParam4 = p4;
    entry.rgParam5 = p5; entry.rgParam6 = p6; entry.rgParam7 = p7;
    entry.maxTries = _sendMavCommandShouldRetry(command) ? _mavCommandMaxRetryCount : 1;
    if (!_reserveMessageCommand(&entry)) {
        mavlink_command_ack_t ack{}; ack.command = command; ack.result = MAV_RESULT_FAILED;
        const bool quarantined = QThread::currentThread() == thread() && _isMavCommandQuarantined(componentId, command);
        const auto failure = (!_messageCommandLaneId.isEmpty()
            || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
            || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
            || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)) ? MavCmdResultFailureDuplicateCommand
            : quarantined ? MavCmdResultFailureLateAckQuarantine : MavCmdResultFailureNoResponseToCommand;
        if (entry.ackHandlerInfo.resultHandler)
            (*entry.ackHandlerInfo.resultHandler)(entry.ackHandlerInfo.resultHandlerData, componentId, ack, failure);
        else emit mavCommandResult(_id, componentId, command, ack.result, failure);
        if (self) _showMessageCommandFailure(entry, ack, failure);
        return;
    }
    // Install the legacy context identity before any callback. It does not
    // publish a waiter until the reserved source has survived preparation.
    const auto context = _requestMessageContext;
    const bool ownWaiter = context && handler && handler->resultHandler == &Vehicle::_requestMessageCmdResultHandler
        && handler->resultHandlerData == context.get();
    if (ownWaiter) { context->requestId = entry.messageIntervalRequestId; context->source = std::make_shared<MavCommandListEntry_t>(entry); }
    _mavCommandList.append(entry);
    const QPointer<LinkInterface> primary = entry.messageIntervalPrimary;
    const bool connected = primary->isConnected();
    if (!self) return;
    auto fail = [&]() {
        mavlink_command_ack_t ack{}; ack.command = command; ack.result = MAV_RESULT_FAILED;
        _finishMessageCommand(entry.messageIntervalRequestId, ack, MavCmdResultFailureNoResponseToCommand);
    };
    if (!primary || !connected || !_messageIntervalSourceCurrent(entry)) { fail(); return; }
    const auto configuration = primary->linkConfiguration();
    if (!self) return;
    if (!configuration || !primary || !_messageIntervalSourceCurrent(entry)) { fail(); return; }
    entry.ackTimeoutMSecs = configuration->isHighLatency() ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs;
    if (!self) return;
    if (!_messageIntervalSourceCurrent(entry)) { fail(); return; }
    const qint64 horizon = qint64(entry.maxTries) * entry.ackTimeoutMSecs + (ownWaiter ? 1000 : 0);
    entry.messageCommandDeadlineMs = entry.messageCommandReservedAtMs + horizon;
    _messageCommandLaneDeadlineMs = entry.messageCommandDeadlineMs;
    entry.messageCommandPrepared = true;
    const int index = _messageCommandIndex(entry.messageIntervalRequestId);
    if (index < 0) return;
    _mavCommandList[index] = entry;
    if (ownWaiter) {
        context->source = std::make_shared<MavCommandListEntry_t>(entry);
        _waitForMavlinkMessage(_requestMessageWaitForMessageResultHandler, context.get(), context->msgId, 1000);
    }
    if (!_messageIntervalSourceCurrent(entry)) { fail(); return; }
    _sendMessageCommandFromList(entry.messageIntervalRequestId, nullptr);
}

bool Vehicle::_sendMessageIntervalCommand(const MavCommandListEntry_t& entry, mavlink_message_t message)
{
    const QPointer<Vehicle> self(this);
    const QPointer<LinkInterface> link = entry.messageIntervalPrimary;
    const QPointer<FirmwarePlugin> plugin = entry.messageIntervalPlugin;
    const auto controlRun = !entry.fleetCameraStreamControlRunId.isEmpty() ? _fleetCameraStreamControlRun : std::shared_ptr<FleetCameraStreamControlRun>();
    const auto cameraRun = !entry.fleetCameraStreamCaptureRunId.isEmpty() ? _fleetCameraStreamRun : std::shared_ptr<FleetCameraStreamRun>();
    const auto intervalQuery = entry.fleetMessageIntervalQuery ? _fleetMessageIntervalActiveQuery
        : std::shared_ptr<FleetMessageIntervalQuery>();
    const auto configurationRun = !entry.fleetOptionalTelemetryConfigurationRunId.isEmpty()
        ? _fleetOptionalTelemetryConfigurationRun : std::shared_ptr<FleetOptionalTelemetryConfigurationRun>();
    const mavlink_message_t original = message;
    const auto current = [&]() {
        if (!self || !link || !plugin || self->_firmwarePlugin != plugin.data()
            || !self->_messageIntervalSourceCurrent(entry)) return false;
        bool sameRequest = false;
        for (const auto& pending : self->_mavCommandList) {
            if (pending.messageIntervalRequestId == entry.messageIntervalRequestId) {
                sameRequest = pending.command == entry.command && pending.targetCompId == entry.targetCompId
                    && pending.messageIntervalSourceEpoch == entry.messageIntervalSourceEpoch
                    && pending.messageIntervalPrimary == entry.messageIntervalPrimary;
                break;
            }
        }
        return sameRequest && (entry.fleetCameraStreamControlRunId.isEmpty()
            || (controlRun && controlRun == self->_fleetCameraStreamControlRun && controlRun->active
                && controlRun->id == entry.fleetCameraStreamControlRunId && controlRun->stage < 2
                && (controlRun->stage == 0 ? controlRun->actionId : controlRun->queryId) == entry.messageIntervalRequestId
                && !(controlRun->stage == 0 ? controlRun->actionIssued : controlRun->queryIssued)
                && self->_fleetCameraStreamControlSourceCurrent(controlRun, true) && message.magic == MAVLINK_STX))
            && (entry.fleetCameraStreamCaptureRunId.isEmpty()
            || (cameraRun && cameraRun == self->_fleetCameraStreamRun && cameraRun->active
                && cameraRun->id == entry.fleetCameraStreamCaptureRunId && cameraRun->stage < 2
                && cameraRun->queries[cameraRun->stage].id == entry.messageIntervalRequestId
                && !cameraRun->queries[cameraRun->stage].requestIssued
                && self->_fleetCameraStreamRunSourceCurrent(cameraRun, true) && message.magic == MAVLINK_STX))
            && (entry.fleetOptionalTelemetryConfigurationRunId.isEmpty()
            || (configurationRun && configurationRun == self->_fleetOptionalTelemetryConfigurationRun
                && configurationRun->id == entry.fleetOptionalTelemetryConfigurationRunId && configurationRun->active
                && self->_fleetOptionalTelemetryRunSourceCurrent(configurationRun, true)))
            && (!entry.fleetMessageIntervalQuery
            || (intervalQuery && intervalQuery == self->_fleetMessageIntervalActiveQuery && intervalQuery->active
                && !intervalQuery->requestIssued
                && intervalQuery->source.messageIntervalRequestId == entry.messageIntervalRequestId
                && self->_fleetMessageIntervalQuerySourceCurrent(intervalQuery, true)))
            && message.msgid == original.msgid && message.len == original.len
            && message.sysid == original.sysid && message.compid == original.compid
            && memcmp(_MAV_PAYLOAD(&message), _MAV_PAYLOAD(&original), original.len) == 0;
    };
    if (!current()) return false;
    if (controlRun && !_fleetCameraStreamControlAllowed(controlRun)) return false;
    if (!current()) return false;
    if (cameraRun && !_fleetCameraStreamRunAllowed(cameraRun)) return false;
    if (!current()) return false;
    if (configurationRun && !_fleetOptionalTelemetryRunAllowed(configurationRun)) return false;
    if (!current()) return false;
    if (entry.fleetMessageIntervalQuery && !_fleetMessageIntervalQueryAllowed(intervalQuery)) return false;
    if (!current()) return false;
    const bool connected = link->isConnected();
    if (!current() || !connected) return false;
    if (controlRun && !_fleetCameraStreamControlAllowed(controlRun)) return false;
    if (!current()) return false;
    if (cameraRun && !_fleetCameraStreamRunAllowed(cameraRun)) return false;
    if (!current()) return false;
    if (configurationRun && !_fleetOptionalTelemetryRunAllowed(configurationRun)) return false;
    if (!current()) return false;
    if (entry.fleetMessageIntervalQuery && !_fleetMessageIntervalQueryAllowed(intervalQuery)) return false;
    if (!current()) return false;
    // The plugin may synchronously retire the original command or source.
    // Do not send an adjusted/foreign payload or reacquire a newer PRIMARY.
    plugin->adjustOutgoingMavlinkMessageThreadSafe(self.data(), link.data(), &message);
    if (!current()) return false;
    if (controlRun && !_fleetCameraStreamControlAllowed(controlRun)) return false;
    if (!current()) return false;
    if (cameraRun && !_fleetCameraStreamRunAllowed(cameraRun)) return false;
    if (!current()) return false;
    if (configurationRun && !_fleetOptionalTelemetryRunAllowed(configurationRun)) return false;
    if (!current()) return false;
    if (entry.fleetMessageIntervalQuery && !_fleetMessageIntervalQueryAllowed(intervalQuery)) return false;
    if (!current()) return false;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const int length = mavlink_msg_to_send_buffer(buffer, &message);
    if (length < 1 || length > MAVLINK_MAX_PACKET_LEN || !current()) return false;
    if (controlRun && !_fleetCameraStreamControlAllowed(controlRun)) return false;
    if (!current()) return false;
    if (cameraRun && !_fleetCameraStreamRunAllowed(cameraRun)) return false;
    if (!current()) return false;
    if (configurationRun && !_fleetOptionalTelemetryRunAllowed(configurationRun)) return false;
    if (!current()) return false;
    if (entry.fleetMessageIntervalQuery && !_fleetMessageIntervalQueryAllowed(intervalQuery)) return false;
    if (!current()) return false;
    const int index = _messageCommandIndex(entry.messageIntervalRequestId);
    if (index < 0) return false;
    if (entry.command == MAV_CMD_SET_MESSAGE_INTERVAL && _mavCommandList.at(index).messageIntervalMutationReserved) return false;
    const bool mutatesObservedComponent = entry.command == MAV_CMD_SET_MESSAGE_INTERVAL && entry.targetCompId == _defaultComponentId;
    if (mutatesObservedComponent) {
        if (_optionalTelemetryMutationGeneration == std::numeric_limits<quint64>::max()
            || _optionalTelemetryWriteDepth == std::numeric_limits<quint64>::max()) {
            _optionalTelemetryMutationExhausted = true;
            _invalidateOptionalTelemetryObservations();
            return false;
        }
        _mavCommandList[index].messageIntervalMutationReserved = true;
        if (configurationRun && (configurationRun->stage < 0 || configurationRun->stage > 4
            || configurationRun->stage % 2 != 0
            || configurationRun->rows[configurationRun->stage / 2].setRequestId != entry.messageIntervalRequestId)) return false;
        ++_optionalTelemetryMutationGeneration;
        _invalidateOptionalTelemetryObservations();
        if (configurationRun) {
            const int rowIndex = configurationRun->stage / 2;
            // Only this exact native UUID's first actual write may bridge its
            // own generation/clear. No generic owner, map or later report can.
            configurationRun->expectedMutation = _optionalTelemetryMutationGeneration;
            configurationRun->ledgerEpoch = _optionalTelemetryLedgerEpoch;
            configurationRun->ledgerBound = false;
            configurationRun->rows[rowIndex].setRequestIssued = true;
        }
        ++_optionalTelemetryWriteDepth;
    } else if (entry.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        _mavCommandList[index].messageIntervalMutationReserved = true;
    }
    if (intervalQuery) {
        // Include every report observed during Link/plugin/auth preparation,
        // not just the receipts that predated begin(). One actual write only.
        intervalQuery->baselineReceiptSequence = _optionalTelemetryNextReceipt;
    }
    _mavCommandList[index].messageCommandWriteStarted = true;
    if (intervalQuery) {
        intervalQuery->source.messageCommandWriteStarted = true;
        intervalQuery->requestIssued = true; // Preserve the possibly queued write even across nested cancellation.
    }
    const auto context = _requestMessageContext;
    if (context && context->requestId == entry.messageIntervalRequestId && context->source)
        context->source->messageCommandWriteStarted = true;
    if (cameraRun) {
        auto& query = cameraRun->queries[cameraRun->stage];
        query.baseline = _fleetCameraStreamNextReceipt;
        query.requestIssued = true;
        query.source.messageCommandWriteStarted = true;
    }
    if (controlRun) {
        if (controlRun->stage == 0) {
            // Only this once-issued native UUID bridges its own mutation.
            if (!_advanceFleetCameraStreamMutation(controlRun)) return false;
            controlRun->actionIssued = true; controlRun->actionSource.messageCommandWriteStarted = true;
        } else {
            controlRun->baseline = _fleetCameraStreamNextReceipt;
            controlRun->queryIssued = true; controlRun->querySource.messageCommandWriteStarted = true;
        }
    }
    const auto releaseWrite = qScopeGuard([self, mutatesObservedComponent]() {
        if (self && mutatesObservedComponent && self->_optionalTelemetryWriteDepth > 0) --self->_optionalTelemetryWriteDepth;
    });
    link->writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), length);
    // This is only the actual local write call. Source retirement during it
    // cannot turn a possibly queued command into a claim that nothing was sent.
    if (!self) return true;
    ++_messagesSent;
    emit messagesSentChanged();
    return true;
}

void Vehicle::_sendMessageCommandFromList(const QString& requestId, bool* fleetReadIssued)
{
    if (fleetReadIssued) *fleetReadIssued = false;
    const int initial = _messageCommandIndex(requestId);
    if (initial < 0) return;
    MavCommandListEntry_t entry = _mavCommandList.at(initial);
    if (!entry.messageCommandPrepared) return;
    const QPointer<Vehicle> self(this);
    if (_messageCommandDispatchingId == requestId) return;
    _messageCommandDispatchingId = requestId;
    const auto releaseDispatch = qScopeGuard([self, requestId]() {
        if (self && self->_messageCommandDispatchingId == requestId) self->_messageCommandDispatchingId.clear();
    });
    const auto fail = [&]() {
        if (!self) return;
        mavlink_command_ack_t ack{}; ack.command = entry.command; ack.result = MAV_RESULT_FAILED;
        self->_finishMessageCommand(requestId, ack, MavCmdResultFailureNoResponseToCommand);
    };
    if (!_messageIntervalSourceCurrent(entry)) { fail(); return; }
    if (!entry.fleetCameraStreamControlRunId.isEmpty()) {
        const auto run = _fleetCameraStreamControlRun;
        const bool allowed = _fleetCameraStreamControlAllowed(run);
        if (!self) return;
        if (!allowed || !_messageIntervalSourceCurrent(entry) || _messageCommandIndex(requestId) < 0) { fail(); return; }
    }
    if (!entry.fleetCameraStreamCaptureRunId.isEmpty()) {
        const auto run = _fleetCameraStreamRun;
        const bool allowed = _fleetCameraStreamRunAllowed(run);
        if (!self) return;
        if (!allowed || !_messageIntervalSourceCurrent(entry) || _messageCommandIndex(requestId) < 0) { fail(); return; }
    }
    if (!entry.fleetOptionalTelemetryConfigurationRunId.isEmpty()) {
        const auto run = _fleetOptionalTelemetryConfigurationRun;
        const bool allowed = _fleetOptionalTelemetryRunAllowed(run);
        if (!self) return;
        if (!allowed || !_messageIntervalSourceCurrent(entry) || _messageCommandIndex(requestId) < 0) { fail(); return; }
    }
    if (entry.fleetMessageIntervalQuery) {
        const auto query = _fleetMessageIntervalActiveQuery;
        const bool allowed = _fleetMessageIntervalQueryAllowed(query);
        if (!self) return;
        if (!allowed || !_messageIntervalSourceCurrent(entry) || _messageCommandIndex(requestId) < 0) { fail(); return; }
    }
    if (entry.fleetEvidenceReadOnly) {
        const bool allowed = _fleetEvidenceReadAllowed(entry.fleetEvidenceLink.data(), true);
        if (!self) return;
        if (!allowed || !_messageIntervalSourceCurrent(entry) || _messageCommandIndex(requestId) < 0) { fail(); return; }
    }
    int index = _messageCommandIndex(requestId);
    if (index < 0) return;
    if (_mavCommandList.at(index).tryCount >= entry.maxTries) { fail(); return; }
    const QPointer<LinkInterface> primary = entry.messageIntervalPrimary;
    const bool connected = primary->isConnected();
    if (!self) return;
    if (!primary || !connected || !_messageIntervalSourceCurrent(entry)) { fail(); return; }
    index = _messageCommandIndex(requestId);
    if (index < 0) return;
    const quint8 senderSystem = entry.messageIntervalProtocol->getSystemId();
    const quint8 senderComponent = entry.messageIntervalProtocol->getComponentId();
    const quint8 channel = primary->mavlinkChannel();
    if (!self) return;
    if (!_messageIntervalSourceCurrent(entry)) { fail(); return; }
    index = _messageCommandIndex(requestId);
    if (index < 0) return;
    ++_mavCommandList[index].tryCount;
    _mavCommandList[index].elapsedTimer.restart();
    mavlink_message_t message{};
    if (entry.useCommandInt) {
        const double encodedX = entry.frame == MAV_FRAME_MISSION ? entry.rgParam5 : entry.rgParam5 * 1e7;
        const double encodedY = entry.frame == MAV_FRAME_MISSION ? entry.rgParam6 : entry.rgParam6 * 1e7;
        if (!std::isfinite(encodedX) || !std::isfinite(encodedY)
            || encodedX < std::numeric_limits<qint32>::min() || encodedX > std::numeric_limits<qint32>::max()
            || encodedY < std::numeric_limits<qint32>::min() || encodedY > std::numeric_limits<qint32>::max()) { fail(); return; }
        mavlink_command_int_t command{};
        command.target_system = _id; command.target_component = entry.targetCompId;
        command.command = entry.command; command.frame = entry.frame;
        command.param1 = entry.rgParam1; command.param2 = entry.rgParam2;
        command.param3 = entry.rgParam3; command.param4 = entry.rgParam4;
        command.x = qint32(encodedX); command.y = qint32(encodedY);
        command.z = entry.rgParam7;
        mavlink_msg_command_int_encode_chan(senderSystem, senderComponent, channel, &message, &command);
    } else {
        mavlink_command_long_t command{};
        command.target_system = _id; command.target_component = entry.targetCompId; command.command = entry.command;
        command.param1 = entry.rgParam1; command.param2 = entry.rgParam2;
        command.param3 = entry.rgParam3; command.param4 = entry.rgParam4;
        command.param5 = float(entry.rgParam5); command.param6 = float(entry.rgParam6); command.param7 = entry.rgParam7;
        mavlink_msg_command_long_encode_chan(senderSystem, senderComponent, channel, &message, &command);
    }
    bool issued = false;
    if (entry.fleetEvidenceReadOnly) {
        // The refresh sender retains its stricter existing authorization gate.
        issued = _sendFleetEvidenceReadMessage(primary.data(), message, true, entry.fleetEvidenceAuthorizationFence, &issued);
    } else issued = _sendMessageIntervalCommand(entry, message);
    if (fleetReadIssued) *fleetReadIssued = issued;
    if (!self) return;
    if (!issued) fail();
}

void Vehicle::_sendMavCommandWorker(
    bool        commandInt, 
    bool        showError, 
    const MavCmdAckHandlerInfo_t* ackHandlerInfo,
    int         targetCompId, 
    MAV_CMD     command, 
    MAV_FRAME   frame, 
    float param1, float param2, float param3, float param4, double param5, double param6, float param7)
{
    // Preserve the normal legacy STOP(old)->START(new) public sequence. It
    // cannot interleave with a new scoped camera-control or shared read lease.
    if (cameraStreamMutationCommand(command) && (!_messageCommandLaneId.isEmpty()
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
        || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active))) {
        mavlink_command_ack_t ack{}; ack.command = command; ack.result = MAV_RESULT_FAILED;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler)
            ackHandlerInfo->resultHandler(ackHandlerInfo->resultHandlerData, targetCompId, ack, MavCmdResultFailureDuplicateCommand);
        else emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, MavCmdResultFailureDuplicateCommand);
        return;
    }
    if (messageCommand(command)) {
        _sendMessageCommandWorker(commandInt, showError, ackHandlerInfo, targetCompId, command, frame,
            param1, param2, param3, param4, param5, param6, param7);
        return;
    }
    // Compatibility code below is for other commands. Its retained legacy
    // 511 branches are unreachable; the dispatcher above is the only native
    // 510/511/512 queue path (including retries and direct version refresh).
    const QPointer<Vehicle> commandOwner(this);
    if (command == MAV_CMD_SET_MESSAGE_INTERVAL
        && (QThread::currentThread() != thread() || _offlineEditingVehicle || _autopilotVersionObservationRemoving
            || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
            || !_fleetExecutionSourceProtocol || _fleetExecutionSourceProtocol.data() != _mavlink
            || !_messageIntervalSourcePlugin || _messageIntervalSourcePlugin.data() != _firmwarePlugin)) {
        // The persistent pins were captured before callbacks at construction.
        // Never rebuild a QPointer from a stale raw provider on a later entry.
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId,
                ack, MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        return;
    }
    // Keep the generic command queue from accepting a new command after the
    // link manager has declared communication lost. Do not require
    // initialConnectComplete here: a small set of benign commands is used by
    // the initial handshake itself. User-facing/high-risk entry points have
    // the stricter Vehicle gates above.
    if (!_vehicleLinkManager) {
        qCWarning(VehicleLog) << "_sendMavCommandWorker blocked: no vehicle link manager" << command;
        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (commandOwner && showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: vehicle link manager is unavailable."));
        }
        return;
    }
    if (_vehicleLinkManager->communicationLost()) {
        qCWarning(VehicleLog) << "_sendMavCommandWorker blocked: communication lost" << command;
        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (commandOwner && showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: communication with vehicle is lost."));
        }
        return;
    }

    if (_isMavCommandQuarantined(targetCompId, command)) {
        const QString rawCommandName = command == MAV_CMD_SET_MESSAGE_INTERVAL
            ? QStringLiteral("MAV_CMD_SET_MESSAGE_INTERVAL") : _toolbox->missionCommandTree()->rawName(command);
        qCDebug(VehicleLog) << QStringLiteral("_sendMavCommandWorker blocking command during delayed ACK quarantine") << rawCommandName;

        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureLateAckQuarantine;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (commandOwner && showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: %1.").arg(tr("Waiting for delayed response quarantine to expire.")));
        }
        return;
    }

    // We can't send commands to compIdAll using this method. The reason being that we would get responses back possibly from multiple components
    // which this code can't handle.
    // We also can't send the majority of commands again if we are already waiting for a response from that same command. If we did that we would not be able to discern
    // which ack was associated with which command.
    if ((targetCompId == MAV_COMP_ID_ALL) || (isMavCommandPending(targetCompId, command) && !_commandCanBeDuplicated(command))) {
        bool    compIdAll       = targetCompId == MAV_COMP_ID_ALL;
        QString rawCommandName  = command == MAV_CMD_SET_MESSAGE_INTERVAL
            ? QStringLiteral("MAV_CMD_SET_MESSAGE_INTERVAL") : _toolbox->missionCommandTree()->rawName(command);

        qCDebug(VehicleLog) << QStringLiteral("_sendMavCommandWorker failing %1").arg(compIdAll ? "MAV_COMP_ID_ALL not supportded" : "duplicate command") << rawCommandName;

        MavCmdResultFailureCode_t failureCode = compIdAll ? MavCmdResultCommandResultOnly : MavCmdResultFailureDuplicateCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (commandOwner && showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: %1.").arg(compIdAll ? tr("Internal error - MAV_COMP_ID_ALL not supported") : tr("Waiting on previous response to same command.")));
        }

        return;
    }

    const QPointer<Vehicle> intervalSelf(this);
    const QPointer<VehicleLinkManager> intervalLinks = command == MAV_CMD_SET_MESSAGE_INTERVAL
        ? _fleetExecutionSourceLinks : QPointer<VehicleLinkManager>(_vehicleLinkManager);
    const QPointer<MAVLinkProtocol> intervalProtocol = _fleetExecutionSourceProtocol;
    const QPointer<FirmwarePlugin> intervalPlugin = _messageIntervalSourcePlugin;
    const WeakLinkInterfacePtr intervalOwner = intervalLinks->primaryLink();
    SharedLinkInterfacePtr sharedLink = intervalOwner.lock();
    const QPointer<LinkInterface> intervalPrimary(sharedLink.get());
    const QString intervalEpoch = _autopilotVersionObservationEpoch;
    const bool connected = sharedLink && sharedLink->isConnected();
    if (!intervalSelf) return;
    const WeakLinkInterfacePtr currentOwner = intervalLinks ? intervalLinks->primaryLink() : WeakLinkInterfacePtr();
    const bool intervalSourceSame = command != MAV_CMD_SET_MESSAGE_INTERVAL
        || (intervalLinks && intervalLinks.data() == _vehicleLinkManager && intervalPrimary
            && !currentOwner.expired() && !currentOwner.owner_before(intervalOwner)
            && !intervalOwner.owner_before(currentOwner) && intervalEpoch == _autopilotVersionObservationEpoch
            && intervalProtocol && intervalProtocol == _fleetExecutionSourceProtocol && intervalProtocol.data() == _mavlink
            && intervalPlugin && intervalPlugin == _messageIntervalSourcePlugin && intervalPlugin.data() == _firmwarePlugin
            && !intervalLinks->communicationLost());
    if (!sharedLink || !connected || !intervalSourceSame) {
        qCDebug(VehicleLog) << "_sendMavCommandWorker: primary link gone!";
        const MavCmdResultFailureCode_t failureCode = MavCmdResultFailureNoResponseToCommand;
        if (ackHandlerInfo && ackHandlerInfo->resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*ackHandlerInfo->resultHandler)(ackHandlerInfo->resultHandlerData, targetCompId, ack, failureCode);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failureCode);
        }
        if (commandOwner && showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: the vehicle's primary link is unavailable."));
        }
        return;
    }

    MavCommandListEntry_t   entry;

    entry.useCommandInt     = commandInt;
    entry.targetCompId      = targetCompId;
    entry.command           = command;
    entry.frame             = frame;
    entry.showError         = showError;
    entry.ackHandlerInfo    = {};
    if (ackHandlerInfo) {
        entry.ackHandlerInfo = *ackHandlerInfo;
    }
    entry.rgParam1          = param1;
    entry.rgParam2          = param2;
    entry.rgParam3          = param3;
    entry.rgParam4          = param4;
    entry.rgParam5          = param5;
    entry.rgParam6          = param6;
    entry.rgParam7          = param7;
    if (command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        entry.messageIntervalRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.messageIntervalSourceEpoch = intervalEpoch;
        entry.messageIntervalLinks = intervalLinks;
        entry.messageIntervalProtocol = intervalProtocol;
        entry.messageIntervalPlugin = intervalPlugin;
        entry.messageIntervalPrimary = intervalPrimary;
        entry.messageIntervalPrimaryOwner = intervalOwner;
    }
    if (cameraStreamMutationCommand(command))
        entry.messageIntervalRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.maxTries          = _sendMavCommandShouldRetry(command) ? _mavCommandMaxRetryCount : 1;
    entry.ackTimeoutMSecs   = sharedLink->linkConfiguration()->isHighLatency() ? _mavCommandAckTimeoutMSecsHighLatency : _mavCommandAckTimeoutMSecs;
    if (!intervalSelf) return;
    const bool intervalPending = command == MAV_CMD_SET_MESSAGE_INTERVAL && isMavCommandPending(targetCompId, command);
    const bool intervalQuarantined = command == MAV_CMD_SET_MESSAGE_INTERVAL && _isMavCommandQuarantined(targetCompId, command);
    if (command == MAV_CMD_SET_MESSAGE_INTERVAL
        && (!_messageIntervalSourceCurrent(entry) || intervalPending || intervalQuarantined)) {
        const MavCmdResultFailureCode_t failure = intervalPending ? MavCmdResultFailureDuplicateCommand
            : intervalQuarantined ? MavCmdResultFailureLateAckQuarantine : MavCmdResultFailureNoResponseToCommand;
        if (entry.ackHandlerInfo.resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*entry.ackHandlerInfo.resultHandler)(entry.ackHandlerInfo.resultHandlerData, targetCompId,
                ack, failure);
        } else {
            emit mavCommandResult(_id, targetCompId, command, MAV_RESULT_FAILED, failure);
        }
        return;
    }
    entry.elapsedTimer.start();

    _mavCommandList.append(entry);
    _sendMavCommandFromList(_mavCommandList.count() - 1);
}

void Vehicle::_sendMavCommandFromList(int index, bool* fleetReadIssued)
{
    if (fleetReadIssued) *fleetReadIssued = false;
    if (index < 0 || index >= _mavCommandList.count()) {
        return;
    }

    MavCommandListEntry_t commandEntry = _mavCommandList[index];
    if (messageCommand(commandEntry.command) || !commandEntry.fleetCameraStreamControlRunId.isEmpty()) {
        _sendMessageCommandFromList(commandEntry.messageIntervalRequestId, fleetReadIssued);
        return;
    }
    const QPointer<Vehicle> self(this);

    QString rawCommandName  = commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL
        ? QStringLiteral("MAV_CMD_SET_MESSAGE_INTERVAL") : _toolbox->missionCommandTree()->rawName(commandEntry.command);

    auto failPendingCommand = [&](const QString& reason) {
        if (!self) return;
        int failedIndex = index;
        if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL || cameraStreamMutationCommand(commandEntry.command)) {
            failedIndex = -1;
            for (int current = 0; current < _mavCommandList.size(); ++current) {
                if (_mavCommandList.at(current).messageIntervalRequestId == commandEntry.messageIntervalRequestId) {
                    failedIndex = current; break;
                }
            }
        }
        if (failedIndex < 0 || failedIndex >= _mavCommandList.count()) {
            return;
        }

        MavCommandListEntry_t failedEntry = _mavCommandList.takeAt(failedIndex);
        if (failedEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL)
            _quarantineMavCommand(failedEntry.targetCompId, failedEntry.command, failedEntry.ackTimeoutMSecs);
        _quarantineFleetEvidenceCommand(failedEntry);
        qCDebug(VehicleLog) << "_sendMavCommandFromList blocked:" << reason << rawCommandName;

        mavlink_command_ack_t ack = {};
        ack.result = MAV_RESULT_FAILED;
        if (failedEntry.ackHandlerInfo.resultHandler) {
            (*failedEntry.ackHandlerInfo.resultHandler)(failedEntry.ackHandlerInfo.resultHandlerData,
                                                        failedEntry.targetCompId,
                                                        ack,
                                                        MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id,
                                  failedEntry.targetCompId,
                                  failedEntry.command,
                                  MAV_RESULT_FAILED,
                                  MavCmdResultFailureNoResponseToCommand);
        }
        if (self && failedEntry.showError) {
            qgcApp()->showAppMessage(tr("Unable to send command: %1").arg(reason));
        }
    };

    if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL && !_messageIntervalSourceCurrent(commandEntry)) {
        failPendingCommand(tr("message interval source changed"));
        return;
    }
    if (!_vehicleLinkManager || _vehicleLinkManager->communicationLost()
            || _vehicleLinkManager->primaryLink().expired()) {
        failPendingCommand(tr("vehicle link is unavailable"));
        return;
    }

    if (commandEntry.fleetEvidenceReadOnly && (commandEntry.fleetEvidenceEpoch != _autopilotVersionObservationEpoch
        || !_fleetEvidenceReadAllowed(commandEntry.fleetEvidenceLink.data(), true))) {
        failPendingCommand(tr("Fleet read-only evidence source changed"));
        return;
    }
    if (++_mavCommandList[index].tryCount > commandEntry.maxTries) {
        qCDebug(VehicleLog) << "_sendMavCommandFromList giving up after max retries" << rawCommandName;
        _quarantineMavCommand(commandEntry.targetCompId,
                              commandEntry.command,
                              commandEntry.ackTimeoutMSecs);
        _quarantineFleetEvidenceCommand(commandEntry);
        _mavCommandList.removeAt(index);
        if (commandEntry.ackHandlerInfo.resultHandler) {
            mavlink_command_ack_t ack = {};
            ack.result = MAV_RESULT_FAILED;
            (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, commandEntry.targetCompId, ack, MavCmdResultFailureNoResponseToCommand);
        } else {
            emit mavCommandResult(_id, commandEntry.targetCompId, commandEntry.command, MAV_RESULT_FAILED, MavCmdResultFailureNoResponseToCommand);
        }
        if (self && commandEntry.showError) {
            qgcApp()->showAppMessage(tr("Vehicle did not respond to command: %1").arg(rawCommandName));
        }
        return;
    }

    if (commandEntry.tryCount > 1 && !px4Firmware() && commandEntry.command == MAV_CMD_START_RX_PAIR) {
        // The implementation of this command comes from the IO layer and is shared across stacks. So for other firmwares
        // we aren't really sure whether they are correct or not.
        return;
    }

    qCDebug(VehicleLog) << "_sendMavCommandFromList command:tryCount" << rawCommandName << commandEntry.tryCount;

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    const bool connected = sharedLink && sharedLink->isConnected();
    if (!self) return;
    if (!sharedLink || !connected || (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL
        && (sharedLink.get() != commandEntry.messageIntervalPrimary.data() || !_messageIntervalSourceCurrent(commandEntry)))) {
        failPendingCommand(tr("vehicle primary link is unavailable"));
        return;
    }
    if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        index = -1;
        for (int current = 0; current < _mavCommandList.size(); ++current) {
            if (_mavCommandList.at(current).messageIntervalRequestId == commandEntry.messageIntervalRequestId) {
                index = current; break;
            }
        }
        if (index < 0) return;
    }

    // The elapsed timer belongs to the live list entry. Restart it only when
    // a real retry is sent; restarting a local copy would leave the timeout
    // clock stale and cause rapid retry storms after the first expiry.
    _mavCommandList[index].elapsedTimer.restart();

    const uint8_t senderSystem = _mavlink->getSystemId();
    const uint8_t senderComponent = _mavlink->getComponentId();
    const uint8_t senderChannel = sharedLink->mavlinkChannel();
    if (!self) return;
    if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL && !_messageIntervalSourceCurrent(commandEntry)) {
        failPendingCommand(tr("message interval source changed during encoding"));
        return;
    }
    mavlink_message_t  msg;

    if (commandEntry.useCommandInt) {
        mavlink_command_int_t  cmd;

        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _id;
        cmd.target_component =  commandEntry.targetCompId;
        cmd.command =           commandEntry.command;
        cmd.frame =             commandEntry.frame;
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.x =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam5 : commandEntry.rgParam5 * 1e7;
        cmd.y =                 commandEntry.frame == MAV_FRAME_MISSION ? commandEntry.rgParam6 : commandEntry.rgParam6 * 1e7;
        cmd.z =                 commandEntry.rgParam7;
        mavlink_msg_command_int_encode_chan(senderSystem,
                                            senderComponent,
                                            senderChannel,
                                            &msg,
                                            &cmd);
    } else {
        mavlink_command_long_t  cmd;

        memset(&cmd, 0, sizeof(cmd));
        cmd.target_system =     _id;
        cmd.target_component =  commandEntry.targetCompId;
        cmd.command =           commandEntry.command;
        cmd.confirmation =      0;
        cmd.param1 =            commandEntry.rgParam1;
        cmd.param2 =            commandEntry.rgParam2;
        cmd.param3 =            commandEntry.rgParam3;
        cmd.param4 =            commandEntry.rgParam4;
        cmd.param5 =            static_cast<float>(commandEntry.rgParam5);
        cmd.param6 =            static_cast<float>(commandEntry.rgParam6);
        cmd.param7 =            commandEntry.rgParam7;
        mavlink_msg_command_long_encode_chan(senderSystem,
                                             senderComponent,
                                             senderChannel,
                                             &msg,
                                             &cmd);
    }

    if (commandEntry.fleetEvidenceReadOnly) {
        QPointer<Vehicle> self(this);
        const bool issued = _sendFleetEvidenceReadMessage(commandEntry.fleetEvidenceLink.data(), msg, true,
            commandEntry.fleetEvidenceAuthorizationFence, fleetReadIssued);
        if (!self) return;
        if (!issued) {
            // Plugin callbacks may remove/reorder the list. Match this owned
            // request ID before disposing anything, never a foreign entry.
            for (int current = 0; current < _mavCommandList.size(); ++current) {
                if (_mavCommandList[current].fleetEvidenceRequestId == commandEntry.fleetEvidenceRequestId) {
                    const auto failed = _mavCommandList.takeAt(current);
                    _quarantineFleetEvidenceCommand(failed);
                    break;
                }
            }
        }
    } else if (commandEntry.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        const bool issued = _sendMessageIntervalCommand(commandEntry, msg);
        if (!self) return;
        if (!issued) failPendingCommand(tr("message interval request/source changed before write"));
    } else {
        const bool issued = sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        if (!self) return;
        if (!issued && cameraStreamMutationCommand(commandEntry.command))
            failPendingCommand(tr("camera stream command source or shared transport changed before write"));
    }
}

void Vehicle::_sendMavCommandResponseTimeoutCheck(void)
{
    const QPointer<Vehicle> self(this);
    _checkMessageCommandDeadline();
    if (!self) return;
    if (_mavCommandList.isEmpty()) {
        return;
    }

    // Walk the list backwards since _sendMavCommandFromList can remove entries
    for (int i=_mavCommandList.count()-1; i>=0; i--) {
        if (i >= _mavCommandList.size()) continue;
        const MavCommandListEntry_t commandEntry = _mavCommandList.at(i);
        const bool scoped = messageCommand(commandEntry.command) || !commandEntry.fleetCameraStreamControlRunId.isEmpty();
        if (!commandEntry.fleetCameraStreamControlRunId.isEmpty()) continue; // Dedicated original <=4s stage timer; no legacy retry.
        if (scoped && !commandEntry.messageCommandPrepared) continue;
        if (scoped && !commandEntry.elapsedTimer.isValid()) continue;
        if (scoped && commandEntry.messageCommandProgressSeen) continue;
        if (commandEntry.elapsedTimer.elapsed() > commandEntry.ackTimeoutMSecs) {
            // Try sending command again
            _sendMavCommandFromList(i);
            if (!self) return;
        }
    }
}

void Vehicle::_handleMessageCommandAck(const mavlink_message_t& original, LinkInterface* sourceLink)
{
    if (original.msgid != MAVLINK_MSG_ID_COMMAND_ACK || original.sysid != _id
        || sender() != _fleetExecutionSourceProtocol.data() || original.len < 1
        || original.len > MAVLINK_MSG_ID_COMMAND_ACK_LEN
        || (original.magic != MAVLINK_STX && original.magic != MAVLINK_STX_MAVLINK1)
        || (original.magic == MAVLINK_STX_MAVLINK1 && original.len != MAVLINK_MSG_ID_COMMAND_ACK_MIN_LEN)) return;
    mavlink_message_t normalized = original;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&original), original.len);
    mavlink_command_ack_t ack{};
    mavlink_msg_command_ack_decode(&normalized, &ack);
    if (!messageCommand(static_cast<MAV_CMD>(ack.command)) && !cameraStreamMutationCommand(static_cast<MAV_CMD>(ack.command))) return;
    const QString requestId = _messageCommandLaneId;
    int index = _messageCommandIndex(requestId);
    if (index < 0) return;
    const MavCommandListEntry_t entry = _mavCommandList.at(index);
    if (!messageCommand(entry.command) && entry.fleetCameraStreamControlRunId.isEmpty()) return;
    if (entry.command != ack.command || entry.targetCompId != original.compid || !entry.messageCommandWriteStarted
        || sourceLink != entry.messageIntervalPrimary.data() || !_messageIntervalSourceCurrent(entry)
        || _isMavCommandQuarantined(entry.targetCompId, entry.command)) return;
    if (!entry.fleetCameraStreamControlRunId.isEmpty()) {
        const QPointer<Vehicle> self(this);
        const auto run = _fleetCameraStreamControlRun;
        if (!run || !run->active || run->id != entry.fleetCameraStreamControlRunId || run->stage > 1
            || (run->stage == 0 ? run->actionId : run->queryId) != requestId
            || !_fleetCameraStreamControlSourceCurrent(run, false)
            || (ack.target_system != 0 && ack.target_system != entry.messageIntervalProtocol->getSystemId())
            || (ack.target_component != 0 && ack.target_component != entry.messageIntervalProtocol->getComponentId())) return;
        const QPointer<FleetCameraStreamControl> owner(run->owner);
        const int stage = run->stage;
        const bool responseCurrent = owner && owner->nativeResponseCurrent(run);
        if (!self || !owner || !responseCurrent || _fleetCameraStreamControlRun != run || !run->active || run->stage != stage
            || !_fleetCameraStreamControlSourceCurrent(run, false)) return;
        index = _messageCommandIndex(requestId);
        if (index < 0 || _mavCommandList.at(index).messageCommandSerial != entry.messageCommandSerial) return;
    }
    if (entry.fleetMessageIntervalQuery && (!_fleetMessageIntervalActiveQuery
        || _fleetMessageIntervalActiveQuery->source.messageIntervalRequestId != requestId
        || !_fleetMessageIntervalQuerySourceCurrent(_fleetMessageIntervalActiveQuery, false)
        || (ack.target_system != 0 && ack.target_system != entry.messageIntervalProtocol->getSystemId())
        || (ack.target_component != 0 && ack.target_component != entry.messageIntervalProtocol->getComponentId()))) return;
    if (!entry.fleetCameraStreamCaptureRunId.isEmpty()
        && (!_fleetCameraStreamRun || !_fleetCameraStreamRun->active || _fleetCameraStreamRun->stage >= 2
            || _fleetCameraStreamRun->id != entry.fleetCameraStreamCaptureRunId
            || _fleetCameraStreamRun->queries[_fleetCameraStreamRun->stage].id != requestId
            || !_fleetCameraStreamRunSourceCurrent(_fleetCameraStreamRun, false)
            || (ack.target_system != 0 && ack.target_system != entry.messageIntervalProtocol->getSystemId())
            || (ack.target_component != 0 && ack.target_component != entry.messageIntervalProtocol->getComponentId()))) return;
    if (!entry.fleetCameraStreamCaptureRunId.isEmpty()) {
        const QPointer<Vehicle> self(this);
        const auto run = _fleetCameraStreamRun;
        const QPointer<FleetCameraStreamCapture> owner(run->owner);
        const int stage = run->stage;
        const bool responseCurrent = owner && owner->nativeResponseCurrent(run);
        if (!self || !owner || !responseCurrent || _fleetCameraStreamRun != run || !run->active || run->stage != stage
            || run->queries[stage].id != requestId || !_fleetCameraStreamRunSourceCurrent(run, false)) return;
        index = _messageCommandIndex(requestId);
        if (index < 0 || _mavCommandList.at(index).messageCommandSerial != entry.messageCommandSerial) return;
    }
    if (!entry.fleetOptionalTelemetryConfigurationRunId.isEmpty()
        && (!_fleetOptionalTelemetryConfigurationRun
            || _fleetOptionalTelemetryConfigurationRun->id != entry.fleetOptionalTelemetryConfigurationRunId
            || !_fleetOptionalTelemetryRunSourceCurrent(_fleetOptionalTelemetryConfigurationRun, false)
            || (ack.target_system != 0 && ack.target_system != entry.messageIntervalProtocol->getSystemId())
            || (ack.target_component != 0 && ack.target_component != entry.messageIntervalProtocol->getComponentId()))) return;
    // A progress packet cannot renew the reservation or start another retry.
    // The ACK still carries no request nonce or requested message ID.
    if (ack.result == MAV_RESULT_IN_PROGRESS) {
        _mavCommandList[index].messageCommandProgressSeen = true;
        return;
    }
    _finishMessageCommand(requestId, ack, MavCmdResultCommandResultOnly);
}

void Vehicle::_handleCommandAck(mavlink_message_t& message, LinkInterface* sourceLink, const mavlink_message_t* originalMessage)
{
    const QPointer<Vehicle> ackOwner(this);
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);
    if (messageCommand(static_cast<MAV_CMD>(ack.command))) return; // Dedicated original RAW path only.
    if (originalMessage && originalMessage->msgid == MAVLINK_MSG_ID_COMMAND_ACK
        && int(originalMessage->len) <= MAVLINK_MAX_PAYLOAD_LEN) {
        mavlink_message_t normalized = *originalMessage;
        memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
        memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(originalMessage), originalMessage->len);
        mavlink_command_ack_t rawAck{};
        mavlink_msg_command_ack_decode(&normalized, &rawAck);
        if (messageCommand(static_cast<MAV_CMD>(rawAck.command))) return;
    }
    if (_fleetStartupActive && message.compid == _defaultComponentId
        && ((_fleetStartupStep == FleetStartupStep::NormalArm && ack.command == MAV_CMD_COMPONENT_ARM_DISARM)
            || (_fleetStartupStep == FleetStartupStep::MissionModeOnly && ack.command == MAV_CMD_DO_SET_MODE))) return;

    _purgeMavCommandQuarantine();
    if (_isMavCommandQuarantined(message.compid, static_cast<MAV_CMD>(ack.command))) {
        qCDebug(VehicleLog) << QStringLiteral("_handleCommandAck ignored delayed ACK during quarantine")
                            << ack.command << message.compid;
        return;
    }

    QString rawCommandName = ack.command == MAV_CMD_SET_MESSAGE_INTERVAL
        ? QStringLiteral("MAV_CMD_SET_MESSAGE_INTERVAL") : _toolbox->missionCommandTree()->rawName(static_cast<MAV_CMD>(ack.command));
    qCDebug(VehicleLog) << QStringLiteral("_handleCommandAck command(%1) result(%2)").arg(rawCommandName).arg(QGCMAVLink::mavResultToString(static_cast<MAV_RESULT>(ack.result)));

    int entryIndex = _findMavCommandListEntryIndex(message.compid, static_cast<MAV_CMD>(ack.command));
    if (entryIndex != -1) {
        const MavCommandListEntry_t scoped = _mavCommandList[entryIndex];
        if (!scoped.fleetCameraStreamControlRunId.isEmpty()) return; // Native RAW-only action ACK; never plugin-adjusted fallback.
        if (scoped.command == MAV_CMD_SET_MESSAGE_INTERVAL) {
            // A 511 ACK carries no requested message ID. Match only this
            // still-live local entry and its original physical PRIMARY;
            // progress cannot renew its finite one-shot timeout.
            if (!sourceLink || message.sysid != _id || sourceLink != scoped.messageIntervalPrimary.data()
                || !_messageIntervalSourceCurrent(scoped)) return;
            if (ack.result == MAV_RESULT_IN_PROGRESS) return;
        }
        if (scoped.fleetEvidenceReadOnly) {
            // A raw/adjusted ACK is not refresh evidence. Dispose this local
            // queue entry only on its still-frozen physical link/context.
            if (!sourceLink || sourceLink != scoped.fleetEvidenceLink.data()
                || scoped.fleetEvidenceEpoch != _autopilotVersionObservationEpoch
                || !_fleetEvidenceReadAllowed(sourceLink, true)) return;
            if (ack.result == MAV_RESULT_IN_PROGRESS) return; // Never extend the finite one-shot timeout.
            _mavCommandList.removeAt(entryIndex);
            _quarantineFleetEvidenceCommand(scoped);
            return;
        }
        if (ack.result == MAV_RESULT_IN_PROGRESS) {
            MavCommandListEntry_t commandEntry;
            if (px4Firmware() && ack.command == MAV_CMD_DO_AUTOTUNE_ENABLE) {
                // HacK to support PX4 autotune which does not send final result ack and just sends in progress
                commandEntry = _mavCommandList.takeAt(entryIndex);
                // This legacy path treats IN_PROGRESS as terminal. Quarantine
                // a possible later final ACK before another autotune request
                // can reuse the same component/command tuple.
                _quarantineMavCommand(commandEntry.targetCompId,
                                      commandEntry.command,
                                      commandEntry.ackTimeoutMSecs);
            } else {
                MavCommandListEntry_t& activeEntry = _mavCommandList[entryIndex];
                activeEntry.maxTries = activeEntry.tryCount;    // Vehicle responded; do not retry again
                activeEntry.elapsedTimer.restart();
                commandEntry = activeEntry;                    // Command has not completed yet, don't remove
            }

            if (commandEntry.ackHandlerInfo.progressHandler) {
                (*commandEntry.ackHandlerInfo.progressHandler)(commandEntry.ackHandlerInfo.progressHandlerData, message.compid, ack);
            }
        } else {
            MavCommandListEntry_t commandEntry = _mavCommandList.takeAt(entryIndex);

            // State/UI side effects belong only to a terminal ACK that matched
            // a live command entry. An orphan or delayed ACK must not make the
            // ground station claim that an operation succeeded.
            if (ack.result == MAV_RESULT_ACCEPTED) {
                if (ack.command == MAV_CMD_DO_SET_ROI_LOCATION) {
                    _isROIEnabled = true;
                    emit isROIEnabledChanged();
                } else if (ack.command == MAV_CMD_DO_SET_ROI_NONE) {
                    _isROIEnabled = false;
                    emit isROIEnabledChanged();
                }

                if (ack.command == MAV_CMD_PREFLIGHT_STORAGE) {
                    emit sensorsParametersResetAck(true);
                }

#if !defined(NO_ARDUPILOT_DIALECT)
                if (ack.command == MAV_CMD_FLASH_BOOTLOADER) {
                    qgcApp()->showAppMessage(tr("Bootloader flash succeeded"));
                }
#endif
            } else if (ack.command == MAV_CMD_PREFLIGHT_STORAGE) {
                emit sensorsParametersResetAck(false);
            }

            if (commandEntry.ackHandlerInfo.resultHandler) {
                (*commandEntry.ackHandlerInfo.resultHandler)(commandEntry.ackHandlerInfo.resultHandlerData, message.compid, ack, MavCmdResultCommandResultOnly);
            } else {
                if (commandEntry.showError) {
                    switch (ack.result) {
                    case MAV_RESULT_TEMPORARILY_REJECTED:
                        qgcApp()->showAppMessage(tr("%1 command temporarily rejected").arg(rawCommandName));
                        break;
                    case MAV_RESULT_DENIED:
                        qgcApp()->showAppMessage(tr("%1 command denied").arg(rawCommandName));
                        break;
                    case MAV_RESULT_UNSUPPORTED:
                        qgcApp()->showAppMessage(tr("%1 command not supported").arg(rawCommandName));
                        break;
                    case MAV_RESULT_FAILED:
                        qgcApp()->showAppMessage(tr("%1 command failed").arg(rawCommandName));
                        break;
                    default:
                        // Do nothing
                        break;
                    }
                }
                if (!ackOwner) return;
                emit mavCommandResult(_id, message.compid, ack.command, ack.result, MavCmdResultCommandResultOnly);
            }

            // Request-specific handlers alone advance PID/ESC interval
            // chains. A foreign live 511 result has no such ownership.
        }
    } else {
        qCDebug(VehicleLog) << "_handleCommandAck Ack not in list" << rawCommandName;
    }
}

void Vehicle::_waitForMavlinkMessage(WaitForMavlinkMessageResultHandler resultHandler, void* resultHandlerData, int messageId, int timeoutMsecs)
{
    qCDebug(VehicleLog) << "_waitForMavlinkMessage msg:timeout" << messageId << timeoutMsecs;
    if (_waitForMavlinkMessageResultHandler) {
        qCCritical(VehicleLog) << "_waitForMavlinkMessage: collision";
    }
    _waitForMavlinkMessageResultHandler     = resultHandler;
    _waitForMavlinkMessageResultHandlerData = resultHandlerData;
    _waitForMavlinkMessageId                = messageId;
    _waitForMavlinkMessageTimeoutActive     = false;                // Timer doesn't start until ack is received from command request
    _waitForMavlinkMessageTimeoutMsecs      = timeoutMsecs;
}

void Vehicle::_waitForMavlinkMessageClear(void)
{
    qCDebug(VehicleLog) << "_waitForMavlinkMessageClear";
    _waitForMavlinkMessageResultHandler     = nullptr;
    _waitForMavlinkMessageResultHandlerData = nullptr;
    _waitForMavlinkMessageId                = 0;
    _waitForMavlinkMessageTimeoutActive     = false;
}

void Vehicle::_waitForMavlinkMessageMessageReceived(const mavlink_message_t& message, LinkInterface* sourceLink)
{
    const auto context = _requestMessageContext;
    if (!context || context->resultPublished || !context->source || context->messageReceived
        || !_waitForMavlinkMessageResultHandler || _waitForMavlinkMessageResultHandlerData != context.get()
        || sender() != context->source->messageIntervalProtocol.data()
        || !context->source->messageCommandWriteStarted || !_messageIntervalSourceCurrent(*context->source)
        || sourceLink != context->source->messageIntervalPrimary.data()
        || message.sysid != _id || message.compid != context->compId || int(message.msgid) != context->msgId
        || (message.magic != MAVLINK_STX && message.magic != MAVLINK_STX_MAVLINK1)) return;
    const mavlink_msg_entry_t* descriptor = mavlink_get_msg_entry(message.msgid);
    if (!descriptor || message.len > descriptor->max_msg_len
        || (message.magic == MAVLINK_STX ? message.len < 1 : message.len != descriptor->min_msg_len)) return;
    mavlink_message_t received = message;
    memset(_MAV_PAYLOAD_NON_CONST(&received), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&received), _MAV_PAYLOAD(&message), message.len);
    if (message.msgid == MAVLINK_MSG_ID_MESSAGE_INTERVAL) {
        if (message.len < 1 || message.len > MAVLINK_MSG_ID_MESSAGE_INTERVAL_LEN
            || (message.magic == MAVLINK_STX_MAVLINK1 && message.len != MAVLINK_MSG_ID_MESSAGE_INTERVAL_MIN_LEN)) return;
        mavlink_message_t normalized = message;
        memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
        memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&message), message.len);
        mavlink_message_interval_t interval{};
        mavlink_msg_message_interval_decode(&normalized, &interval);
        // This only filters the caller's requested target. It is not proof
        // that this generic 512 parameter is supported, causal or authentic.
        const double requested = context->requestedMessageParameter;
        if (!std::isfinite(requested) || std::floor(requested) != requested || requested < 0 || requested > 65535
            || interval.message_id != int(requested)) return;
    }
    context->messageReceived = true; context->message = received;
    if (context->commandAckReceived)
        _finishRequestMessage(context, MAV_RESULT_ACCEPTED, RequestMessageNoFailure, received);
}

void Vehicle::requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId,
    float param1, float param2, float param3, float param4, float param5, QObject* callbackOwner)
{
    if (!resultHandler) {
        qCWarning(VehicleLog) << "requestMessage blocked: result handler is null" << messageId;
        return;
    }

    // Only one outstanding message waiter is supported by this legacy API.
    // Reject a second request instead of overwriting the first request's
    // callback/data and allowing a later message to be delivered to the
    // wrong caller.
    const QPointer<QObject> owner(callbackOwner);
    const bool ownerRequired = callbackOwner != nullptr;
    if (_requestMessageContext || _waitForMavlinkMessageResultHandler || !_messageCommandLaneId.isEmpty()
        || (_fleetCameraStreamControlRun && _fleetCameraStreamControlRun->active)
        || (_fleetCameraStreamRun && _fleetCameraStreamRun->active)
        || (_fleetOptionalTelemetryConfigurationRun && _fleetOptionalTelemetryConfigurationRun->active)) {
        qCWarning(VehicleLog) << "requestMessage blocked: another message request is already pending" << messageId;
        mavlink_message_t emptyMessage = {};
        if (!ownerRequired || owner) resultHandler(resultHandlerData,
                      MAV_RESULT_FAILED,
                      RequestMessageFailureDuplicateCommand,
                      emptyMessage);
        return;
    }

    if (QThread::currentThread() != thread() || messageId < 0 || messageId > 0xFFFFFF || compId < 1 || compId > 255
        || !_fleetExecutionSourceLinks || _fleetExecutionSourceLinks.data() != _vehicleLinkManager
        || _fleetExecutionSourceLinks->communicationLost() || _fleetExecutionSourceLinks->primaryLink().expired()) {
        qCWarning(VehicleLog) << "requestMessage blocked: vehicle transport unavailable" << messageId;
        mavlink_message_t emptyMessage = {};
        if (!ownerRequired || owner) resultHandler(resultHandlerData,
                      MAV_RESULT_FAILED,
                      RequestMessageFailureCommandNotAcked,
                      emptyMessage);
        return;
    }

    const auto context = std::make_shared<RequestMessageInfo_t>();
    RequestMessageInfo_t* pInfo = context.get();
    pInfo->vehicle              = this;
    pInfo->msgId                = messageId;
    pInfo->compId               = compId;
    pInfo->resultHandler        = resultHandler;
    pInfo->resultHandlerData    = resultHandlerData;
    pInfo->callbackOwner        = owner;
    pInfo->ownerRequired        = ownerRequired;
    pInfo->requestedMessageParameter = param1;
    _requestMessageContext = context;

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _requestMessageCmdResultHandler;
    handlerInfo.resultHandlerData   = pInfo;

    _sendMavCommandWorker(false,                                    // commandInt
                          false,                                    // showError
                          &handlerInfo,
                          compId,
                          MAV_CMD_REQUEST_MESSAGE,
                          MAV_FRAME_GLOBAL,
                          messageId,
                          param1, param2, param3, param4, param5, 0);
}

void Vehicle::_requestMessageCmdResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(resultHandlerData);
    const QPointer<Vehicle> vehicle = pInfo ? pInfo->vehicle : QPointer<Vehicle>();
    if (!vehicle) return;
    const auto context = vehicle->_requestMessageContext;
    if (!context || context.get() != pInfo || context->resultPublished) return;
    pInfo->commandAckReceived = true;
    if (failureCode != MavCmdResultCommandResultOnly || ack.result != MAV_RESULT_ACCEPTED) {
        mavlink_message_t                           message{};
        RequestMessageResultHandlerFailureCode_t    requestMessageFailureCode = RequestMessageFailureCommandNotAcked;

        switch (failureCode) {
        case Vehicle::MavCmdResultCommandResultOnly:
            requestMessageFailureCode = RequestMessageFailureCommandError;
            break;
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            requestMessageFailureCode = RequestMessageFailureCommandNotAcked;
            break;
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            requestMessageFailureCode = RequestMessageFailureDuplicateCommand;
            break;
        case Vehicle::MavCmdResultFailureLateAckQuarantine:
            // A quarantine is intentionally reported as a local send refusal;
            // it must not be mistaken for a valid vehicle-side command error.
            requestMessageFailureCode = RequestMessageFailureDuplicateCommand;
            break;
        default:
            break;
        }

        vehicle->_finishRequestMessage(context, static_cast<MAV_RESULT>(ack.result), requestMessageFailureCode, message);
        return;
    }

    if (pInfo->messageReceived) {
        vehicle->_finishRequestMessage(context, static_cast<MAV_RESULT>(ack.result), RequestMessageNoFailure, pInfo->message);
    } else {
        if (!context->source || !vehicle->_messageIntervalSourceCurrent(*context->source)) {
            vehicle->_finishRequestMessage(context, MAV_RESULT_FAILED, RequestMessageFailureCommandNotAcked, mavlink_message_t{});
            return;
        }
        vehicle->_waitForMavlinkMessageTimeoutActive = true;
        vehicle->_waitForMavlinkMessageElapsed.restart();
        // The message phase may shorten, never renew, the reservation's
        // original finite horizon. Timer checks do not require another packet.
        const qint64 now = vehicle->_mavCommandQuarantineClock.elapsed();
        const qint64 messageDeadline = now > std::numeric_limits<qint64>::max() - 1000
            ? std::numeric_limits<qint64>::max() : now + 1000;
        const qint64 deadline = qMin(context->source->messageCommandDeadlineMs, messageDeadline);
        context->source->messageCommandDeadlineMs = deadline;
        vehicle->_messageCommandLaneDeadlineMs = deadline;
    }
}

void Vehicle::_requestMessageWaitForMessageResultHandler(void* resultHandlerData, bool noResponsefromVehicle, const mavlink_message_t& message)
{
    RequestMessageInfo_t* pInfo = static_cast<RequestMessageInfo_t*>(resultHandlerData);
    const QPointer<Vehicle> vehicle = pInfo ? pInfo->vehicle : QPointer<Vehicle>();
    if (!vehicle) return;
    const auto context = vehicle->_requestMessageContext;
    if (!context || context.get() != pInfo || context->resultPublished) return;
    if (noResponsefromVehicle)
        vehicle->_finishRequestMessage(context, MAV_RESULT_FAILED, RequestMessageFailureMessageNotReceived, message);
    else if (context->commandAckReceived)
        vehicle->_finishRequestMessage(context, MAV_RESULT_ACCEPTED, RequestMessageNoFailure, message);
    else { context->messageReceived = true; context->message = message; }
}

void Vehicle::_finishRequestMessage(std::shared_ptr<RequestMessageInfo_t> context, MAV_RESULT result,
    RequestMessageResultHandlerFailureCode_t failure, const mavlink_message_t& message)
{
    if (!context || context->resultPublished || _requestMessageContext != context) return;
    context->resultPublished = true;
    const QString requestId = context->requestId;
    const mavlink_message_t ownedMessage = message;
    const RequestMessageResultHandler callback = context->resultHandler;
    void* const callbackData = context->resultHandlerData;
    const QPointer<QObject> owner = context->callbackOwner;
    const bool ownerRequired = context->ownerRequired;
    const int index = _messageCommandIndex(requestId);
    if (index >= 0) _mavCommandList.removeAt(index);
    if (failure != RequestMessageNoFailure && failure != RequestMessageFailureCommandError && context->source)
        _quarantineMavCommand(context->compId, MAV_CMD_REQUEST_MESSAGE, context->source->ackTimeoutMSecs);
    if (_waitForMavlinkMessageResultHandlerData == context.get()) _waitForMavlinkMessageClear();
    _requestMessageContext.reset();
    _releaseMessageCommand(requestId);
    // Keep context locally owned through the callback. No tail cleanup may
    // clear a synchronously started new request or touch a deleted Vehicle.
    if (callback && (!ownerRequired || owner)) callback(callbackData, result, failure, ownedMessage);
}

void Vehicle::_checkMessageCommandDeadline()
{
    const QPointer<Vehicle> self(this);
    const auto controlRun = _fleetCameraStreamControlRun;
    if (controlRun && controlRun->active) {
        const qint64 now = _mavCommandQuarantineClock.elapsed();
        if (!controlRun->owner || now < 0 || now >= controlRun->basis->deadlineMs
            || (controlRun->wireDeadlineMs >= 0 && now >= controlRun->wireDeadlineMs)
            || (fleetCameraStreamCaptureTransportQuiescent() && !_fleetCameraStreamControlSourceCurrent(controlRun, false)))
            _finishFleetCameraStreamControl(controlRun, QStringLiteral("failed"), QStringLiteral("camera-control-source-or-original-deadline-expired"), true);
    }
    const auto cameraRun = _fleetCameraStreamRun;
    if (cameraRun && cameraRun->active) {
        const qint64 now = _mavCommandQuarantineClock.elapsed();
        const qint64 stageDeadline = cameraRun->stage < 2 ? cameraRun->queries[cameraRun->stage].deadlineMs : -1;
        if (!cameraRun->owner || now < 0 || now >= cameraRun->deadlineMs || (stageDeadline >= 0 && now >= stageDeadline)
            || (fleetCameraStreamCaptureTransportQuiescent() && !_fleetCameraStreamRunSourceCurrent(cameraRun, false)))
            _finishFleetCameraStreamCapture(cameraRun, QStringLiteral("failed"), QStringLiteral("camera-capture-source-or-deadline-expired"), true);
    }
    const auto configurationRun = _fleetOptionalTelemetryConfigurationRun;
    if (configurationRun && configurationRun->active
        && (!configurationRun->owner || !configurationRun->basis->captureOwner
            || _mavCommandQuarantineClock.elapsed() >= configurationRun->basis->deadlineMs
            || (fleetMessageIntervalQueryTransportQuiescent() && !_fleetOptionalTelemetryRunSourceCurrent(configurationRun, false))
            || (configurationRun->wireDeadlineMs >= 0 && _mavCommandQuarantineClock.elapsed() >= configurationRun->wireDeadlineMs)))
        _finishFleetOptionalTelemetryConfiguration(configurationRun, QStringLiteral("failed"),
            QStringLiteral("configuration-native-source-or-wire-deadline-expired"), true);
    const auto intervalQuery = _fleetMessageIntervalActiveQuery;
    if (intervalQuery && intervalQuery->active && (!intervalQuery->owner
        || _mavCommandQuarantineClock.elapsed() >= intervalQuery->source.messageCommandDeadlineMs
        || (fleetMessageIntervalQueryTransportQuiescent() && !_fleetMessageIntervalQuerySourceCurrent(intervalQuery, false)))) {
        _finishFleetMessageIntervalQuery(intervalQuery, QStringLiteral("failed"),
            !intervalQuery->owner ? QStringLiteral("fleet-interval-query-owner-destroyed")
                : QStringLiteral("fleet-interval-query-source-or-deadline-expired"), true);
    }
    // Remove only dead owners' retained receipts. Keep removed closures owned
    // until the container loop is over, then guard their destructor callbacks.
    QList<std::shared_ptr<FleetMessageIntervalQuery>> retired;
    for (int index = _fleetMessageIntervalQueries.size() - 1; index >= 0; --index)
        if (!_fleetMessageIntervalQueries.at(index)->owner && !_fleetMessageIntervalQueries.at(index)->active) {
            retired.append(_fleetMessageIntervalQueries.at(index));
            _fleetMessageIntervalQueries.removeAt(index);
        }
    retired.clear();
    if (!self) return;
    const auto context = _requestMessageContext;
    if (context && context->source && !context->resultPublished
        && ((context->ownerRequired && !context->callbackOwner) || !_messageIntervalSourceCurrent(*context->source))) {
        _finishRequestMessage(context, MAV_RESULT_FAILED,
            context->commandAckReceived ? RequestMessageFailureMessageNotReceived : RequestMessageFailureCommandNotAcked,
            mavlink_message_t{});
        return;
    }
    const QString requestId = _messageCommandLaneId;
    const int index = _messageCommandIndex(requestId);
    if (index >= 0 && !_messageIntervalSourceCurrent(_mavCommandList.at(index))) {
        mavlink_command_ack_t ack{}; ack.command = _mavCommandList.at(index).command; ack.result = MAV_RESULT_FAILED;
        _finishMessageCommand(requestId, ack, MavCmdResultFailureNoResponseToCommand);
    }
}

void Vehicle::setPrearmError(const QString& prearmError)
{
    _prearmError = prearmError;
    emit prearmErrorChanged(_prearmError);
    if (!_prearmError.isEmpty()) {
        _prearmErrorTimer.start();
    }
}

void Vehicle::_prearmErrorTimeout()
{
    setPrearmError(QString());
}

void Vehicle::setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType)
{
    _firmwareMajorVersion = majorVersion;
    _firmwareMinorVersion = minorVersion;
    _firmwarePatchVersion = patchVersion;
    _firmwareVersionType = versionType;
    emit firmwareVersionChanged();
}

void Vehicle::setFirmwareCustomVersion(int majorVersion, int minorVersion, int patchVersion)
{
    _firmwareCustomMajorVersion = majorVersion;
    _firmwareCustomMinorVersion = minorVersion;
    _firmwareCustomPatchVersion = patchVersion;
    emit firmwareCustomVersionChanged();
}

QString Vehicle::firmwareVersionTypeString() const
{
    switch (_firmwareVersionType) {
    case FIRMWARE_VERSION_TYPE_DEV:
        return QStringLiteral("dev");
    case FIRMWARE_VERSION_TYPE_ALPHA:
        return QStringLiteral("alpha");
    case FIRMWARE_VERSION_TYPE_BETA:
        return QStringLiteral("beta");
    case FIRMWARE_VERSION_TYPE_RC:
        return QStringLiteral("rc");
    case FIRMWARE_VERSION_TYPE_OFFICIAL:
    default:
        return QStringLiteral("");
    }
}

void Vehicle::_rebootCommandResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    Vehicle* vehicle = static_cast<Vehicle*>(resultHandlerData);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        switch (failureCode) {
        case MavCmdResultCommandResultOnly:
            qCDebug(VehicleLog) << QStringLiteral("MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN error(%1)").arg(ack.result);
            break;
        case MavCmdResultFailureNoResponseToCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: no response from vehicle";
            break;
        case MavCmdResultFailureDuplicateCommand:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN failed: duplicate command";
            break;
        case MavCmdResultFailureLateAckQuarantine:
            qCDebug(VehicleLog) << "MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN blocked by delayed ACK quarantine";
            break;
        default:
            break;
        }
        qgcApp()->showAppMessage(tr("Vehicle reboot failed."));
    } else {
        vehicle->closeVehicle();
    }
}

void Vehicle::rebootVehicle()
{
    if (!_maintenanceCommandAllowed("reboot")) {
        qgcApp()->showAppMessage(tr("Vehicle reboot blocked: vehicle must be connected, grounded, and disarmed."));
        return;
    }

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = _rebootCommandResultHandler;
    handlerInfo.resultHandlerData   = this;

    sendMavCommandWithHandler(&handlerInfo, _defaultComponentId, MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1);
}

bool Vehicle::_commandTransportAvailable(const char* operation) const
{
    if (_offlineEditingVehicle || !isInitialConnectComplete() || !_vehicleLinkManager
            || _vehicleLinkManager->communicationLost()
            || _vehicleLinkManager->primaryLink().expired()) {
        qCWarning(VehicleLog) << operation << "blocked: vehicle transport is not ready";
        return false;
    }
    return true;
}

bool Vehicle::_maintenanceCommandAllowed(const char* operation) const
{
    if (!_commandTransportAvailable(operation)) {
        return false;
    }
    if (armed() || flying()) {
        qCWarning(VehicleLog) << operation << "blocked: vehicle is armed or flying";
        return false;
    }
    return true;
}

void Vehicle::startCalibration(Vehicle::CalibrationType calType)
{
    if (!_maintenanceCommandAllowed("startCalibration")) {
        qgcApp()->showAppMessage(tr("Calibration blocked: vehicle must be connected, grounded, and disarmed."));
        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "startCalibration: primary link gone!";
        return;
    }

    float param1 = 0;
    float param2 = 0;
    float param3 = 0;
    float param4 = 0;
    float param5 = 0;
    float param6 = 0;
    float param7 = 0;

    switch (calType) {
    case CalibrationGyro:
        param1 = 1;
        break;
    case CalibrationMag:
        param2 = 1;
        break;
    case CalibrationRadio:
        param4 = 1;
        break;
    case CalibrationCopyTrims:
        param4 = 2;
        break;
    case CalibrationAccel:
        param5 = 1;
        break;
    case CalibrationLevel:
        param5 = 2;
        break;
    case CalibrationEsc:
        param7 = 1;
        break;
    case CalibrationPX4Airspeed:
        param6 = 1;
        break;
    case CalibrationPX4Pressure:
        param3 = 1;
        break;
    case CalibrationAPMCompassMot:
        param6 = 1;
        break;
    case CalibrationAPMPressureAirspeed:
        param3 = 1;
        break;
    case CalibrationAPMPreFlight:
        param3 = 1; // GroundPressure/Airspeed
        if (multiRotor() || rover()) {
            // Gyro cal for ArduCopter only
            param1 = 1;
        }
    case CalibrationAPMAccelSimple:
        param5 = 4;
        break;
    }

    // We can't use sendMavCommand here since we have no idea how long it will be before the command returns a result. This in turn
    // causes the retry logic to break down.
    mavlink_message_t msg;
    mavlink_msg_command_long_pack_chan(_mavlink->getSystemId(),
                                       _mavlink->getComponentId(),
                                       sharedLink->mavlinkChannel(),
                                       &msg,
                                       id(),
                                       defaultComponentId(),            // target component
                                       MAV_CMD_PREFLIGHT_CALIBRATION,    // command id
                                       0,                                // 0=first transmission of command
                                       param1, param2, param3, param4, param5, param6, param7);
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::stopCalibration(bool showError)
{
    if (!_maintenanceCommandAllowed("stopCalibration")) {
        return;
    }

    sendMavCommand(defaultComponentId(),    // target component
                   MAV_CMD_PREFLIGHT_CALIBRATION,     // command id
                   showError,
                   0,                                 // gyro cal
                   0,                                 // mag cal
                   0,                                 // ground pressure
                   0,                                 // radio cal
                   0,                                 // accel cal
                   0,                                 // airspeed cal
                   0);                                // unused
}

void Vehicle::startUAVCANBusConfig(void)
{
    if (!_maintenanceCommandAllowed("startUAVCANBusConfig")) {
        qgcApp()->showAppMessage(tr("UAVCAN configuration blocked: vehicle must be connected, grounded, and disarmed."));
        return;
    }

    sendMavCommand(defaultComponentId(),        // target component
                   MAV_CMD_PREFLIGHT_UAVCAN,    // command id
                   true,                        // showError
                   1);                          // start config
}

void Vehicle::stopUAVCANBusConfig(void)
{
    if (!_maintenanceCommandAllowed("stopUAVCANBusConfig")) {
        return;
    }

    sendMavCommand(defaultComponentId(),        // target component
                   MAV_CMD_PREFLIGHT_UAVCAN,    // command id
                   true,                        // showError
                   0);                          // stop config
}

void Vehicle::setSoloFirmware(bool soloFirmware)
{
    if (soloFirmware != _soloFirmware) {
        _soloFirmware = soloFirmware;
        emit soloFirmwareChanged(soloFirmware);
    }
}

void Vehicle::motorTest(int motor, int percent, int timeoutSecs, bool showError)
{
    if (!_maintenanceCommandAllowed("motorTest")) {
        if (showError) {
            qgcApp()->showAppMessage(tr("Motor test blocked: vehicle must be connected, grounded, and disarmed."));
        }
        return;
    }

    // A zero timeout is the established stop/disable value used by the
    // motor-test sliders. Reject negative durations, but keep timeout=0
    // compatible with the existing MAV_CMD_DO_MOTOR_TEST stop path.
    if (motor < 1 || percent < 0 || percent > 100 || timeoutSecs < 0) {
        qCWarning(VehicleLog) << "motorTest blocked: invalid motor test arguments" << motor << percent << timeoutSecs;
        return;
    }

    sendMavCommand(_defaultComponentId, MAV_CMD_DO_MOTOR_TEST, showError, motor, MOTOR_TEST_THROTTLE_PERCENT, percent, timeoutSecs, 0, MOTOR_TEST_ORDER_BOARD);
}

QString Vehicle::brandImageIndoor() const
{
    return _firmwarePlugin->brandImageIndoor(this);
}

QString Vehicle::brandImageOutdoor() const
{
    return _firmwarePlugin->brandImageOutdoor(this);
}

void Vehicle::setOfflineEditingDefaultComponentId(int defaultComponentId)
{
    if (_offlineEditingVehicle) {
        _defaultComponentId = defaultComponentId;
    } else {
        qCWarning(VehicleLog) << "Call to Vehicle::setOfflineEditingDefaultComponentId on vehicle which is not offline";
    }
}

void Vehicle::setVtolInFwdFlight(bool vtolInFwdFlight)
{
    if (!_commandTransportAvailable("setVtolInFwdFlight")) {
        return;
    }
    if (_vtolInFwdFlight != vtolInFwdFlight) {
        sendMavCommand(_defaultComponentId,
                       MAV_CMD_DO_VTOL_TRANSITION,
                       true,                                                    // show errors
                       vtolInFwdFlight ? MAV_VTOL_STATE_FW : MAV_VTOL_STATE_MC, // transition state
                       0, 0, 0, 0, 0, 0);                                       // param 2-7 unused
    }
}

void Vehicle::startMavlinkLog()
{
    if (!_commandTransportAvailable("startMavlinkLog")) {
        return;
    }
    sendMavCommand(_defaultComponentId, MAV_CMD_LOGGING_START, false /* showError */);
}

void Vehicle::stopMavlinkLog()
{
    if (!_commandTransportAvailable("stopMavlinkLog")) {
        return;
    }
    sendMavCommand(_defaultComponentId, MAV_CMD_LOGGING_STOP, false /* showError */);
}

void Vehicle::_ackMavlinkLogData(uint16_t sequence)
{
    SharedLinkInterfacePtr  sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "_ackMavlinkLogData: primary link gone!";
        return;
    }

    mavlink_message_t       msg;
    mavlink_logging_ack_t   ack;

    memset(&ack, 0, sizeof(ack));
    ack.sequence = sequence;
    ack.target_component = _defaultComponentId;
    ack.target_system = id();
    mavlink_msg_logging_ack_encode_chan(
                _mavlink->getSystemId(),
                _mavlink->getComponentId(),
                sharedLink->mavlinkChannel(),
                &msg,
                &ack);
    sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
}

void Vehicle::_handleMavlinkLoggingData(mavlink_message_t& message)
{
    mavlink_logging_data_t log;
    mavlink_msg_logging_data_decode(&message, &log);
    if (static_cast<size_t>(log.length) > sizeof(log.data)) {
        qWarning() << "Invalid length for LOGGING_DATA, discarding." << log.length;
    } else {
        emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                            log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
    }
}

void Vehicle::_handleMavlinkLoggingDataAcked(mavlink_message_t& message)
{
    mavlink_logging_data_acked_t log;
    mavlink_msg_logging_data_acked_decode(&message, &log);
    _ackMavlinkLogData(log.sequence);
    if (static_cast<size_t>(log.length) > sizeof(log.data)) {
        qWarning() << "Invalid length for LOGGING_DATA_ACKED, discarding." << log.length;
    } else {
        emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
                            log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
    }
}

void Vehicle::setFirmwarePluginInstanceData(QObject* firmwarePluginInstanceData)
{
    const QPointer<Vehicle> self(this);
    const QPointer<QObject> instance(firmwarePluginInstanceData);
    if (!instance) return;
    firmwarePluginInstanceData->setParent(this);
    if (!self || !instance || instance->parent() != self.data()) return;
    _firmwarePluginInstanceData = instance.data();
}

QString Vehicle::missionFlightMode() const
{
    return _firmwarePlugin->missionFlightMode();
}

QString Vehicle::pauseFlightMode() const
{
    return _firmwarePlugin->pauseFlightMode();
}

QString Vehicle::rtlFlightMode() const
{
    return _firmwarePlugin->rtlFlightMode();
}

QString Vehicle::smartRTLFlightMode() const
{
    return _firmwarePlugin->smartRTLFlightMode();
}

bool Vehicle::supportsSmartRTL() const
{
    return _firmwarePlugin->supportsSmartRTL();
}

QString Vehicle::landFlightMode() const
{
    return _firmwarePlugin->landFlightMode();
}

QString Vehicle::takeControlFlightMode() const
{
    return _firmwarePlugin->takeControlFlightMode();
}

QString Vehicle::followFlightMode() const
{
    return _firmwarePlugin->followFlightMode();
}

QString Vehicle::vehicleImageOpaque() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageOpaque(this);
    else
        return QString();
}

QString Vehicle::vehicleImageOutline() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageOutline(this);
    else
        return QString();
}

QString Vehicle::vehicleImageCompass() const
{
    if(_firmwarePlugin)
        return _firmwarePlugin->vehicleImageCompass(this);
    else
        return QString();
}

const QVariantList& Vehicle::toolIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->toolIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

const QVariantList& Vehicle::modeIndicators()
{
    if(_firmwarePlugin) {
        return _firmwarePlugin->modeIndicators(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

const QVariantList& Vehicle::staticCameraList() const
{
    if (_firmwarePlugin) {
        return _firmwarePlugin->cameraList(this);
    }
    static QVariantList emptyList;
    return emptyList;
}

void Vehicle::_setupAutoDisarmSignalling()
{
    QPointer<Vehicle> owner(this);
    QPointer<ParameterManager> manager(_parameterManager);
    QString param = _firmwarePlugin->autoDisarmParameter(this);
    if (!owner || !manager || _parameterManager != manager) return;

    if (!param.isEmpty() && _parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId,param);
        connect(fact, &Fact::rawValueChanged, this, &Vehicle::autoDisarmChanged);
        emit autoDisarmChanged();
    }
}

bool Vehicle::autoDisarm()
{
    QPointer<Vehicle> owner(this);
    QPointer<ParameterManager> manager(_parameterManager);
    QString param = _firmwarePlugin->autoDisarmParameter(this);
    if (!owner || !manager || _parameterManager != manager) return false;

    if (!param.isEmpty() && _parameterManager->parameterExists(FactSystem::defaultComponentId, param)) {
        Fact* fact = _parameterManager->getParameter(FactSystem::defaultComponentId,param);
        return fact->rawValue().toDouble() > 0;
    }

    return false;
}

void Vehicle::_handleADSBVehicle(const mavlink_message_t& message)
{
    mavlink_adsb_vehicle_t adsbVehicleMsg;
    static const int maxTimeSinceLastSeen = 15;

    mavlink_msg_adsb_vehicle_decode(&message, &adsbVehicleMsg);
    if ((adsbVehicleMsg.flags & ADSB_FLAGS_VALID_COORDS) && adsbVehicleMsg.tslc <= maxTimeSinceLastSeen) {
        ADSBVehicle::ADSBVehicleInfo_t vehicleInfo;

        vehicleInfo.availableFlags = 0;
        vehicleInfo.icaoAddress = adsbVehicleMsg.ICAO_address;

        vehicleInfo.location.setLatitude(adsbVehicleMsg.lat / 1e7);
        vehicleInfo.location.setLongitude(adsbVehicleMsg.lon / 1e7);
        vehicleInfo.availableFlags |= ADSBVehicle::LocationAvailable;

        vehicleInfo.callsign = adsbVehicleMsg.callsign;
        vehicleInfo.availableFlags |= ADSBVehicle::CallsignAvailable;

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_ALTITUDE) {
            vehicleInfo.altitude = (double)adsbVehicleMsg.altitude / 1e3;
            vehicleInfo.availableFlags |= ADSBVehicle::AltitudeAvailable;
        }

        if (adsbVehicleMsg.flags & ADSB_FLAGS_VALID_HEADING) {
            vehicleInfo.heading = (double)adsbVehicleMsg.heading / 100.0;
            vehicleInfo.availableFlags |= ADSBVehicle::HeadingAvailable;
        }

        _toolbox->adsbVehicleManager()->adsbVehicleUpdate(vehicleInfo);
    }
}

void Vehicle::_handleRawImuTemp(mavlink_message_t& message)
{
    // This is used by compass calibration
    emit mavlinkRawImu(message);
    
    mavlink_raw_imu_t imuRaw;
    mavlink_msg_raw_imu_decode(&message, &imuRaw);

    _imuTempFact.setRawValue(imuRaw.temperature == 0 ? 0 : imuRaw.temperature * 0.01);
}

void Vehicle::_updateDistanceHeadingToHome()
{
    if (coordinate().isValid() && homePosition().isValid()) {
        _distanceToHomeFact.setRawValue(coordinate().distanceTo(homePosition()));
        if (_distanceToHomeFact.rawValue().toDouble() > 1.0) {
            _headingToHomeFact.setRawValue(coordinate().azimuthTo(homePosition()));
        } else {
            _headingToHomeFact.setRawValue(qQNaN());
        }
    } else {
        _distanceToHomeFact.setRawValue(qQNaN());
        _headingToHomeFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHeadingToNextWP()
{
    const int currentIndex = _missionManager->currentIndex();
    QList<MissionItem*> llist = _missionManager->missionItems();

    if(llist.size()>currentIndex && currentIndex!=-1
            && llist[currentIndex]->coordinate().longitude()!=0.0
            && coordinate().distanceTo(llist[currentIndex]->coordinate())>5.0 ){

        _headingToNextWPFact.setRawValue(coordinate().azimuthTo(llist[currentIndex]->coordinate()));
    }
    else{
        _headingToNextWPFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateMissionItemIndex()
{
    const int currentIndex = _missionManager->currentIndex();

    unsigned offset = 0;
    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        offset = 1;
    }

    _missionItemIndexFact.setRawValue(currentIndex + offset);
}

void Vehicle::_updateDistanceToGCS()
{
    QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();
    if (coordinate().isValid() && gcsPosition.isValid()) {
        _distanceToGCSFact.setRawValue(coordinate().distanceTo(gcsPosition));
    } else {
        _distanceToGCSFact.setRawValue(qQNaN());
    }
}

void Vehicle::_updateHomepoint()
{
    const bool setHomeCmdSupported = firmwarePlugin()->supportedMissionCommands(vehicleClass()).contains(MAV_CMD_DO_SET_HOME);
    const bool updateHomeActivated = _toolbox->settingsManager()->flyViewSettings()->updateHomePosition()->rawValue().toBool();
    if(setHomeCmdSupported && updateHomeActivated && _commandTransportAvailable("autoUpdateHome")){
        QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();
        if (coordinate().isValid() && gcsPosition.isValid()
                && std::isfinite(gcsPosition.latitude())
                && std::isfinite(gcsPosition.longitude())
                && std::isfinite(gcsPosition.altitude())) {
            sendMavCommand(defaultComponentId(),
                           MAV_CMD_DO_SET_HOME, false,
                           0,
                           0, 0, 0,
                           static_cast<float>(gcsPosition.latitude()) ,
                           static_cast<float>(gcsPosition.longitude()),
                           static_cast<float>(gcsPosition.altitude()));
        }
    }
}

void Vehicle::_updateHobbsMeter()
{
    _hobbsFact.setRawValue(hobbsMeter());
}

void Vehicle::forceInitialPlanRequestComplete()
{
    _initialPlanRequestComplete = true;
    emit initialPlanRequestCompleteChanged(true);
}

void Vehicle::sendPlan(QString planFile)
{
    PlanMasterController::sendPlanToVehicle(this, planFile);
}

QString Vehicle::hobbsMeter()
{
    return _firmwarePlugin->getHobbsMeter(this);
}

void Vehicle::_vehicleParamLoaded(bool ready)
{
    //-- TODO: This seems silly but can you think of a better
    //   way to update this?
    if(ready) {
        emit hobbsMeterChanged();
    }
}

void Vehicle::_mavlinkMessageStatus(int uasId, uint64_t totalSent, uint64_t totalReceived, uint64_t totalLoss, float lossPercent)
{
    if(uasId == _id) {
        _mavlinkSentCount       = totalSent;
        _mavlinkReceivedCount   = totalReceived;
        _mavlinkLossCount       = totalLoss;
        _mavlinkLossPercent     = lossPercent;
        emit mavlinkStatusChanged();
    }
}

int  Vehicle::versionCompare(QString& compare)
{
    return _firmwarePlugin->versionCompare(this, compare);
}

int  Vehicle::versionCompare(int major, int minor, int patch)
{
    return _firmwarePlugin->versionCompare(this, major, minor, patch);
}

void Vehicle::setPIDTuningTelemetryMode(PIDTuningTelemetryMode mode)
{
    const QPointer<Vehicle> self(this);
    const QPointer<VehicleLinkManager> links = _fleetExecutionSourceLinks;
    if (!links || links.data() != _vehicleLinkManager || links->communicationLost()
            || links->primaryLink().expired()) {
        qCDebug(VehicleLog) << "setPIDTuningTelemetryMode blocked: vehicle transport unavailable";
        return;
    }
    bool liveUpdate = mode != ModeDisabled;
    setLiveUpdates(liveUpdate);
    if (!self) return;
    _setpointFactGroup.setLiveUpdates(liveUpdate);
    if (!self) return;
    _localPositionFactGroup.setLiveUpdates(liveUpdate);
    if (!self) return;
    _localPositionSetpointFactGroup.setLiveUpdates(liveUpdate);
    if (!self) return;

    switch (mode) {
    case ModeDisabled:
        _mavlinkStreamConfig.restoreDefaults();
        break;
    case ModeRateAndAttitude:
        _mavlinkStreamConfig.setHighRateRateAndAttitude();
        break;
    case ModeVelocityAndPosition:
        _mavlinkStreamConfig.setHighRateVelAndPos();
        break;
    case ModeAltitudeAndAirspeed:
        _mavlinkStreamConfig.setHighRateAltAirspeed();
        if (!self) return;
        // reset the altitude offset to the current value, so the plotted value is around 0
        if (!qIsNaN(_altitudeTuningOffset)) {
            _altitudeTuningOffset += _altitudeTuningFact.rawValue().toDouble();
            _altitudeTuningSetpointFact.setRawValue(0.f);
            if (!self) return;
            _altitudeTuningFact.setRawValue(0.f);
        }
        break;
    }
}

void Vehicle::_streamIntervalCommandResult(void* opaque, int componentId,
    const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failure)
{
    auto* context = static_cast<StreamIntervalCommandContext*>(opaque);
    if (!context) return;
    const QPointer<Vehicle> vehicle = context->vehicle;
    const QPointer<VehicleLinkManager> links = context->links;
    const QPointer<MAVLinkProtocol> protocol = context->protocol;
    const QPointer<FirmwarePlugin> plugin = context->plugin;
    const QPointer<LinkInterface> primary = context->primary;
    const WeakLinkInterfacePtr owner = context->primaryOwner;
    const QString epoch = context->sourceEpoch;
    const MAVLinkStreamConfig::RequestId requestId = context->requestId;
    const auto current = [&]() {
        if (!vehicle || !links || !protocol || !plugin || !primary || owner.expired()
            || links != vehicle->_fleetExecutionSourceLinks || links.data() != vehicle->_vehicleLinkManager
            || protocol != vehicle->_fleetExecutionSourceProtocol || protocol.data() != vehicle->_mavlink
            || plugin != vehicle->_messageIntervalSourcePlugin || plugin.data() != vehicle->_firmwarePlugin
            || links->communicationLost() || epoch.isEmpty() || epoch != vehicle->_autopilotVersionObservationEpoch) return false;
        const WeakLinkInterfacePtr actual = links->primaryLink();
        const SharedLinkInterfacePtr retained = actual.lock();
        return retained && retained.get() == primary.data() && !actual.owner_before(owner) && !owner.owner_before(actual);
    };
    const bool accepted = failure == MavCmdResultCommandResultOnly
        && componentId == context->componentId && ack.command == MAV_CMD_SET_MESSAGE_INTERVAL
        && ack.result == MAV_RESULT_ACCEPTED && context->parent() == vehicle.data() && current();
    delete context;
    // Deleting the context can synchronously retire its original source.
    // Stop rather than adopting a new PRIMARY for the remaining PID profile.
    if (vehicle) vehicle->_mavlinkStreamConfig.gotSetMessageIntervalResult(requestId, accepted && current());
}

void Vehicle::_setMessageInterval(MAVLinkStreamConfig::RequestId requestId, int messageId, int intervalUs)
{
    const QPointer<Vehicle> self(this);
    auto* context = new StreamIntervalCommandContext;
    context->vehicle = this; context->requestId = requestId; context->componentId = defaultComponentId();
    context->links = _fleetExecutionSourceLinks;
    context->protocol = _fleetExecutionSourceProtocol;
    context->plugin = _messageIntervalSourcePlugin;
    context->primaryOwner = context->links ? context->links->primaryLink() : WeakLinkInterfacePtr();
    const SharedLinkInterfacePtr primary = context->primaryOwner.lock();
    context->primary = primary.get(); context->sourceEpoch = _autopilotVersionObservationEpoch;
    const QPointer<StreamIntervalCommandContext> retained(context);
    context->setParent(this);
    if (!self) return;
    if (!retained || retained->parent() != self.data()) {
        if (retained) delete retained.data();
        if (self) _mavlinkStreamConfig.gotSetMessageIntervalResult(requestId, false);
        return;
    }
    MavCmdAckHandlerInfo_t handler = {};
    handler.resultHandler = &Vehicle::_streamIntervalCommandResult;
    handler.resultHandlerData = context;
    const WeakLinkInterfacePtr actual = context->links ? context->links->primaryLink() : WeakLinkInterfacePtr();
    const SharedLinkInterfacePtr actualPrimary = actual.lock();
    if (!context->links || context->links.data() != _vehicleLinkManager || !context->primary
        || !context->protocol || context->protocol.data() != _mavlink
        || !context->plugin || context->plugin.data() != _firmwarePlugin
        || !actualPrimary || actualPrimary.get() != context->primary.data()
        || actual.owner_before(context->primaryOwner) || context->primaryOwner.owner_before(actual)
        || context->sourceEpoch != _autopilotVersionObservationEpoch || context->links->communicationLost()) {
        mavlink_command_ack_t ack = {};
        ack.result = MAV_RESULT_FAILED;
        _streamIntervalCommandResult(context, context->componentId, ack, MavCmdResultFailureNoResponseToCommand);
        return;
    }
    sendMavCommandWithHandler(&handler, context->componentId, MAV_CMD_SET_MESSAGE_INTERVAL,
        float(messageId), float(intervalUs));
}

bool Vehicle::isInitialConnectComplete() const
{
    return !_initialConnectStateMachine->active();
}

void Vehicle::_initializeCsv()
{
    if(!_toolbox->settingsManager()->appSettings()->saveCsvTelemetry()->rawValue().toBool()){
        return;
    }
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString fileName = QString("%1 vehicle%2.csv").arg(now).arg(_id);
    QDir saveDir(_toolbox->settingsManager()->appSettings()->telemetrySavePath());
    _csvLogFile.setFileName(saveDir.absoluteFilePath(fileName));

    if (!_csvLogFile.open(QIODevice::Append)) {
        qCWarning(VehicleLog) << "unable to open file for csv logging, Stopping csv logging!";
        return;
    }

    QTextStream stream(&_csvLogFile);
    QStringList allFactNames;
    allFactNames << factNames();
    for (const QString& groupName: factGroupNames()) {
        for(const QString& factName: getFactGroup(groupName)->factNames()){
            allFactNames << QString("%1.%2").arg(groupName, factName);
        }
    }
    qCDebug(VehicleLog) << "Facts logged to csv:" << allFactNames;
    stream << "Timestamp," << allFactNames.join(",") << "\n";
}

void Vehicle::_writeCsvLine()
{
    // Only save the logs after the the vehicle gets armed, unless "Save logs even if vehicle was not armed" is checked
    if(!_csvLogFile.isOpen() &&
            (_armed || _toolbox->settingsManager()->appSettings()->telemetrySaveNotArmed()->rawValue().toBool())){
        _initializeCsv();
    }

    if(!_csvLogFile.isOpen()){
        return;
    }

    QStringList allFactValues;
    QTextStream stream(&_csvLogFile);

    // Write timestamp to csv file
    allFactValues << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    // Write Vehicle's own facts
    for (const QString& factName : factNames()) {
        allFactValues << getFact(factName)->cookedValueString();
    }
    // write facts from Vehicle's FactGroups
    for (const QString& groupName: factGroupNames()) {
        for (const QString& factName : getFactGroup(groupName)->factNames()) {
            allFactValues << getFactGroup(groupName)->getFact(factName)->cookedValueString();
        }
    }

    stream << allFactValues.join(",") << "\n";
}

#if !defined(NO_ARDUPILOT_DIALECT)
void Vehicle::flashBootloader()
{
    if (!_maintenanceCommandAllowed("flashBootloader")) {
        qCWarning(VehicleLog) << "flashBootloader blocked: vehicle is not connected, grounded, or disarmed";
        qgcApp()->showAppMessage(tr("Bootloader flash blocked: vehicle must be connected, grounded, and disarmed."));
        return;
    }
    sendMavCommand(defaultComponentId(),
                   MAV_CMD_FLASH_BOOTLOADER,
                   true,        // show error
                   0, 0, 0, 0,  // param 1-4 not used
                   290876);     // magic number

}
#endif

void Vehicle::doSetHome(const QGeoCoordinate& coord)
{
    if (!_commandTransportAvailable("doSetHome")
            || !coord.isValid()
            || !std::isfinite(coord.latitude())
            || !std::isfinite(coord.longitude())) {
        qCWarning(VehicleLog) << "doSetHome blocked: invalid transport or home coordinate";
        return;
    }

    if (coord.isValid()) {
        // If for some reason we already did a query and it hasn't arrived yet, disconnect signals and unset current query. TerrainQuery system will
        // automatically delete that forgotten query. This could happen if we send 2 do_set_home commands one after another, so usually the latest one
        // is the one we would want to arrive to the vehicle, so it is fine to forget about the previous ones in case it could happen.
        if (_currentDoSetHomeTerrainAtCoordinateQuery) {
            disconnect(_currentDoSetHomeTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_doSetHomeTerrainReceived);
            _currentDoSetHomeTerrainAtCoordinateQuery = nullptr;
        }
        // Save the coord for using when our terrain data arrives. If there was a pending terrain query paired with an older coordinate it is safe to 
        // Override now, as we just disconnected the signal that would trigger the command sending 
        _doSetHomeCoordinate = coord;
        // Now setup and trigger the new terrain query
        _currentDoSetHomeTerrainAtCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelet */);
        connect(_currentDoSetHomeTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_doSetHomeTerrainReceived);
        QList<QGeoCoordinate> rgCoord;
        rgCoord.append(coord);
        _currentDoSetHomeTerrainAtCoordinateQuery->requestData(rgCoord);
    }
}

// This will be called after our query started in doSetHome arrives
void Vehicle::_doSetHomeTerrainReceived(bool success, QList<double> heights)
{
    if (success && _commandTransportAvailable("doSetHome/terrainResult")
            && !heights.isEmpty()
            && _doSetHomeCoordinate.isValid()
            && std::isfinite(_doSetHomeCoordinate.latitude())
            && std::isfinite(_doSetHomeCoordinate.longitude())) {
        double terrainAltitude = heights[0];
        // Coord and terrain alt sanity check
        if (_doSetHomeCoordinate.isValid() && terrainAltitude <= SET_HOME_TERRAIN_ALT_MAX && terrainAltitude >= SET_HOME_TERRAIN_ALT_MIN) {
            sendMavCommand(
                        defaultComponentId(),
                        MAV_CMD_DO_SET_HOME,
                        true, // show error if fails
                        0,
                        0,
                        0,
                        static_cast<float>(qQNaN()),
                        _doSetHomeCoordinate.latitude(),
                        _doSetHomeCoordinate.longitude(),
                        terrainAltitude);

        } else if (_doSetHomeCoordinate.isValid()) {
            qCDebug(VehicleLog) << "_doSetHomeTerrainReceived: internal error, elevation data out of limits ";
        } else {
            qCDebug(VehicleLog) << "_doSetHomeTerrainReceived: internal error, cached home coordinate is not valid";
        }
    } else {
        qgcApp()->showAppMessage(tr("Set Home failed, terrain data not available for selected coordinate"));
    }
    // Clean up
    _currentDoSetHomeTerrainAtCoordinateQuery = nullptr;
    _doSetHomeCoordinate = QGeoCoordinate(); // So isValid() will no longer return true, for extra safety
}

void Vehicle::_updateAltAboveTerrain()
{
    // We won't do another query if the previous query was done closer than 2 meters from current position
    // or if altitude change has been less than 0.5 meters since then.
    const qreal minimumDistanceTraveled = 2;
    const float minimumAltitudeChanged  = 0.5f;

    // This is not super elegant but it works to limit the amount of queries we do. It seems more than 500ms is not possible to get
    // serviced on time. It is not a big deal if it is not serviced on time as terrain queries can manage that just fine, but QGC would
    // use resources to service those queries, and it is pointless, so this is a quick workaround to not waste that little computing time
    int altitudeAboveTerrQueryMinInterval = 500;
    if (_altitudeAboveTerrQueryTimer.elapsed() < altitudeAboveTerrQueryMinInterval) {
        // qCDebug(VehicleLog) << "_updateAltAboveTerrain: minimum 500ms interval between queries not reached, returning";
        return;
    }
    // Sanity check, although it is very unlikely that vehicle coordinate is not valid
    if (_coordinate.isValid()) {
        // Check for minimum distance and altitude traveled before doing query, to not do a lot of queries of the same data
        if (_altitudeAboveTerrLastCoord.isValid() && !qIsNaN(_altitudeAboveTerrLastRelAlt)) {
            if (_altitudeAboveTerrLastCoord.distanceTo(_coordinate) < minimumDistanceTraveled && fabs(_altitudeRelativeFact.rawValue().toFloat() - _altitudeAboveTerrLastRelAlt) < minimumAltitudeChanged ) {
                return;
            }
        }
        _altitudeAboveTerrLastCoord = _coordinate;
        _altitudeAboveTerrLastRelAlt = _altitudeRelativeFact.rawValue().toFloat();

        // If for some reason we already did a query and it hasn't arrived yet, disconnect signals and unset current query. TerrainQuery system will
        // automatically delete that forgotten query.
        if (_altitudeAboveTerrTerrainAtCoordinateQuery) {
            // qCDebug(VehicleLog) << "_updateAltAboveTerrain: cleaning previous query, no longer needed";
            disconnect(_altitudeAboveTerrTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_altitudeAboveTerrainReceived);
            _altitudeAboveTerrTerrainAtCoordinateQuery = nullptr;
        }
        // Now setup and trigger the new terrain query
        _altitudeAboveTerrTerrainAtCoordinateQuery = new TerrainAtCoordinateQuery(true /* autoDelet */);
        connect(_altitudeAboveTerrTerrainAtCoordinateQuery, &TerrainAtCoordinateQuery::terrainDataReceived, this, &Vehicle::_altitudeAboveTerrainReceived);
        QList<QGeoCoordinate> rgCoord;
        rgCoord.append(_coordinate);
        _altitudeAboveTerrTerrainAtCoordinateQuery->requestData(rgCoord);
        _altitudeAboveTerrQueryTimer.restart();
    }
}

void Vehicle::_altitudeAboveTerrainReceived(bool success, QList<double> heights)
{
    if (!success || heights.isEmpty() || !std::isfinite(heights[0])) {
        qCDebug(VehicleLog) << "_altitudeAboveTerrainReceived: terrain data not available for vehicle coordinate";
    } else {
        // Query was succesful, save the data.
        double terrainAltitude = heights[0];
        double altitudeAboveTerrain = altitudeAMSL()->rawValue().toDouble() - terrainAltitude;
        _altitudeAboveTerrFact.setRawValue(altitudeAboveTerrain);
    }
    // Clean up
    _altitudeAboveTerrTerrainAtCoordinateQuery = nullptr;
}

void Vehicle::gimbalControlValue(double pitch, double yaw)
{
    if (!_commandTransportAvailable("gimbalControlValue")
            || !std::isfinite(pitch) || !std::isfinite(yaw)
            || pitch < -90.0 || pitch > 90.0
            || yaw < -180.0 || yaw > 180.0) {
        qCWarning(VehicleLog) << "gimbalControlValue blocked: invalid transport or angle";
        return;
    }

    if (apmFirmware()) {
        // ArduPilot firmware treats this values as centi-degrees
        pitch *= 100;
        yaw *= 100;
    }

    //qDebug() << "Gimbal:" << pitch << yaw;
    sendMavCommand(
                _defaultComponentId,
                MAV_CMD_DO_MOUNT_CONTROL,
                false,                               // show errors
                static_cast<float>(pitch),           // Pitch 0 - 90
                0,                                   // Roll (not used)
                static_cast<float>(yaw),             // Yaw -180 - 180
                0,                                   // Altitude (not used)
                0,                                   // Latitude (not used)
                0,                                   // Longitude (not used)
                MAV_MOUNT_MODE_MAVLINK_TARGETING);   // MAVLink Roll,Pitch,Yaw
}

void Vehicle::gimbalPitchStep(int direction)
{
    if(_haveGimbalData) {
        //qDebug() << "Pitch:" << _curGimbalPitch << direction << (_curGimbalPitch + direction);
        double p = static_cast<double>(_curGimbalPitch + direction);
        gimbalControlValue(p, static_cast<double>(_curGimbalYaw));
    }
}

void Vehicle::gimbalYawStep(int direction)
{
    if(_haveGimbalData) {
        //qDebug() << "Yaw:" << _curGimbalYaw << direction << (_curGimbalYaw + direction);
        double y = static_cast<double>(_curGimbalYaw + direction);
        gimbalControlValue(static_cast<double>(_curGimbalPitch), y);
    }
}

void Vehicle::centerGimbal()
{
    if(_haveGimbalData) {
        gimbalControlValue(0.0, 0.0);
    }
}

void Vehicle::_handleGimbalOrientation(const mavlink_message_t& message)
{
    mavlink_mount_orientation_t o;
    mavlink_msg_mount_orientation_decode(&message, &o);
    if(fabsf(_curGimbalRoll - o.roll) > 0.5f) {
        _curGimbalRoll = o.roll;
        emit gimbalRollChanged();
    }
    if(fabsf(_curGimbalPitch - o.pitch) > 0.5f) {
        _curGimbalPitch = o.pitch;
        emit gimbalPitchChanged();
    }
    if(fabsf(_curGimbalYaw - o.yaw) > 0.5f) {
        _curGimbalYaw = o.yaw;
        emit gimbalYawChanged();
    }
    if(!_haveGimbalData) {
        _haveGimbalData = true;
        emit gimbalDataChanged();
    }
}

void Vehicle::_handleObstacleDistance(const mavlink_message_t& message)
{
    mavlink_obstacle_distance_t o;
    mavlink_msg_obstacle_distance_decode(&message, &o);
    _objectAvoidance->update(&o);
}

void Vehicle::_handleFenceStatus(const mavlink_message_t& message)
{
    mavlink_fence_status_t fenceStatus;

    mavlink_msg_fence_status_decode(&message, &fenceStatus);

    qCDebug(VehicleLog) << "_handleFenceStatus breach_status" << fenceStatus.breach_status;

    static qint64 lastUpdate = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (fenceStatus.breach_status == 1) {
        if (now - lastUpdate > 3000) {
            lastUpdate = now;
            QString breachTypeStr;
            switch (fenceStatus.breach_type) {
                case FENCE_BREACH_NONE:
                    return;
                case FENCE_BREACH_MINALT:
                    breachTypeStr = tr("minimum altitude");
                    break;
                case FENCE_BREACH_MAXALT:
                    breachTypeStr = tr("maximum altitude");
                    break;
                case FENCE_BREACH_BOUNDARY:
                    breachTypeStr = tr("boundary");
                    break;
                default:
                    break;
            }

            qgcApp()->toolbox()->audioOutput()->say(breachTypeStr + " " + tr("fence breached"));
        }
    } else {
        lastUpdate = now;
    }
}
void Vehicle::updateFlightDistance(double distance)
{
    _flightDistanceFact.setRawValue(_flightDistanceFact.rawValue().toDouble() + distance);
}

void Vehicle::sendParamMapRC(const QString& paramName, double scale, double centerValue, int tuningID, double minValue, double maxValue)
{
    if (!_maintenanceCommandAllowed("sendParamMapRC")
            || paramName.isEmpty()
            || paramName.size() >= MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN
            || tuningID < 0 || tuningID > 255
            || !std::isfinite(scale) || !std::isfinite(centerValue)
            || !std::isfinite(minValue) || !std::isfinite(maxValue)) {
        qCWarning(VehicleLog) << "sendParamMapRC blocked: invalid maintenance state or mapping values";
        return;
    }

    SharedLinkInterfacePtr  sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog) << "sendParamMapRC: primary link gone!";
        return;
    }

    mavlink_message_t       message;

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};
    // Copy string into buffer, ensuring not to exceed the buffer size
    for (unsigned int i = 0; i < sizeof(param_id_cstr); i++) {
        if ((int)i < paramName.length()) {
            param_id_cstr[i] = paramName.toLatin1()[i];
        }
    }

    mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                       static_cast<uint8_t>(_mavlink->getComponentId()),
                                       sharedLink->mavlinkChannel(),
                                       &message,
                                       _id,
                                       MAV_COMP_ID_AUTOPILOT1,
                                       param_id_cstr,
                                       -1,                                                  // parameter name specified as string in previous argument
                                       static_cast<uint8_t>(tuningID),
                                       static_cast<float>(centerValue),
                                       static_cast<float>(scale),
                                       static_cast<float>(minValue),
                                       static_cast<float>(maxValue));
    sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void Vehicle::clearAllParamMapRC(void)
{
    if (!_maintenanceCommandAllowed("clearAllParamMapRC")) {
        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog)<< "clearAllParamMapRC: primary link gone!";
        return;
    }

    char param_id_cstr[MAVLINK_MSG_PARAM_MAP_RC_FIELD_PARAM_ID_LEN] = {};

    for (int i = 0; i < 3; i++) {
        mavlink_message_t message;
        mavlink_msg_param_map_rc_pack_chan(static_cast<uint8_t>(_mavlink->getSystemId()),
                                           static_cast<uint8_t>(_mavlink->getComponentId()),
                                           sharedLink->mavlinkChannel(),
                                           &message,
                                           _id,
                                           MAV_COMP_ID_AUTOPILOT1,
                                           param_id_cstr,
                                           -2,                                                  // Disable map for specified tuning id
                                           i,                                                   // tuning id
                                           0, 0, 0, 0);                                         // unused
        sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

void Vehicle::sendJoystickDataThreadSafe(float roll, float pitch, float yaw, float thrust, quint16 buttons)
{
    // Manual-control values are converted to int16 MAVLink axes below. Reject
    // non-finite input before the conversion (which would otherwise be
    // undefined) and clamp small upstream overshoot to the documented range.
    if (!std::isfinite(roll) || !std::isfinite(pitch)
            || !std::isfinite(yaw) || !std::isfinite(thrust)) {
        return;
    }
    roll = qBound(-1.0f, roll, 1.0f);
    pitch = qBound(-1.0f, pitch, 1.0f);
    yaw = qBound(-1.0f, yaw, 1.0f);
    thrust = qBound(-1.0f, thrust, 1.0f);

    // This is a high-rate path; keep the same transport policy without
    // emitting a warning on every joystick tick while a vehicle is offline.
    if (_offlineEditingVehicle || !isInitialConnectComplete()
            || !_vehicleLinkManager || _vehicleLinkManager->communicationLost()
            || _vehicleLinkManager->primaryLink().expired()) {
        return;
    }

    SharedLinkInterfacePtr sharedLink = vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCDebug(VehicleLog)<< "sendJoystickDataThreadSafe: primary link gone!";
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency()) {
        return;
    }

    mavlink_message_t message;

    // Incoming values are in the range -1:1
    float axesScaling =         1.0 * 1000.0;
    float newRollCommand =      roll * axesScaling;
    float newPitchCommand  =    pitch * axesScaling;    // Joystick data is reverse of mavlink values
    float newYawCommand    =    yaw * axesScaling;
    float newThrustCommand =    thrust * axesScaling;

    mavlink_msg_manual_control_pack_chan(
                static_cast<uint8_t>(_mavlink->getSystemId()),
                static_cast<uint8_t>(_mavlink->getComponentId()),
                sharedLink->mavlinkChannel(),
                &message,
                static_cast<uint8_t>(_id),
                static_cast<int16_t>(newPitchCommand),
                static_cast<int16_t>(newRollCommand),
                static_cast<int16_t>(newThrustCommand),
                static_cast<int16_t>(newYawCommand),
                buttons,
                0, 0, 0, 0);
    sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void Vehicle::triggerSimpleCamera()
{
    if (!_commandTransportAvailable("triggerSimpleCamera")) {
        return;
    }
    sendMavCommand(_defaultComponentId,
                   MAV_CMD_DO_DIGICAM_CONTROL,
                   true,                        // show errors
                   0.0, 0.0, 0.0, 0.0,          // param 1-4 unused
                   1.0);                        // trigger camera
}

void Vehicle::setGripperAction(GRIPPER_ACTIONS gripperAction)
{
    if (!_commandTransportAvailable("setGripperAction")) {
        return;
    }
    sendMavCommand(
            _defaultComponentId,
            MAV_CMD_DO_GRIPPER,
            false,                               // Don't show errors
            0,                                   // Param1: Gripper ID (Always set to 0)
            gripperAction,                       // Param2: Gripper Action
            0, 0, 0, 0, 0);                      // Param 3 ~ 7 : unused
}

void Vehicle::sendGripperAction(GRIPPER_OPTIONS gripperOption)
{
    switch(gripperOption) {
        case Gripper_release: 
            setGripperAction(GRIPPER_ACTION_RELEASE);
            break;
        case Gripper_grab: 
            setGripperAction(GRIPPER_ACTION_GRAB);
            break;
        case Invalid_option:
            qDebug("unknown function");
            break;
        default: 
        break;
    }
}

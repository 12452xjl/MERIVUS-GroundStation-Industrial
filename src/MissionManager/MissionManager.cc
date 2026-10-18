/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

#include <QtAlgorithms>

#include <cmath>

QGC_LOGGING_CATEGORY(MissionManagerLog, "MissionManagerLog")

MissionManager::MissionManager(Vehicle* vehicle)
    : PlanManager               (vehicle, MAV_MISSION_TYPE_MISSION)
    , _cachedLastCurrentIndex   (-1)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MissionManager::_mavlinkMessageReceived);
}

MissionManager::~MissionManager()
{

}
void MissionManager::writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard call(this, TransportCallType::OperationEntry);
    if (!_liveTransactionAllowed(false)
            || !gotoCoord.isValid()
            || !std::isfinite(gotoCoord.latitude())
            || !std::isfinite(gotoCoord.longitude())
            || !std::isfinite(gotoCoord.altitude())) {
        qCWarning(MissionManagerLog)
            << "writeArduPilotGuidedMissionItem blocked: invalid transport or coordinate";
        return;
    }

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeArduPilotGuidedMissionItem called while transaction in progress";
        return;
    }

    const quint64 operationId = _beginTransaction(TransactionWrite);
    if (!call.alive() || operationId == 0) return;
    if (!_liveTransactionAllowed(false)) {
        _finishTransaction(false, true);
        return;
    }

    _connectToMavlink();

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();


        mavlink_message_t       messageOut;
        mavlink_mission_item_t  missionItem;

        memset(&missionItem, 0, sizeof(missionItem));
        missionItem.target_system =     _vehicle->id();
        missionItem.target_component =  _vehicle->defaultComponentId();
        missionItem.seq =               0;
        missionItem.command =           MAV_CMD_NAV_WAYPOINT;
        missionItem.param1 =            0;
        missionItem.param2 =            0;
        missionItem.param3 =            0;
        missionItem.param4 =            0;
        missionItem.x =                 gotoCoord.latitude();
        missionItem.y =                 gotoCoord.longitude();
        missionItem.z =                 gotoCoord.altitude();
        missionItem.frame =             MAV_FRAME_GLOBAL_RELATIVE_ALT;
        missionItem.current =           altChangeOnly ? 3 : 2;
        missionItem.autocontinue =      true;

        mavlink_msg_mission_item_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                             qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &messageOut,
                                             &missionItem);

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), messageOut);
        if (!call.alive()) return;
    }
    _startAckTimeout(AckGuidedItem);
}

void MissionManager::generateResumeMission(int resumeIndex)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard call(this, TransportCallType::OperationEntry);
    if (!_vehicle || _vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (!_liveTransactionAllowed(true)) {
        qCWarning(MissionManagerLog)
            << "generateResumeMission blocked: vehicle transport/state is not ready";
        return;
    }

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "generateResumeMission called while transaction in progress";
        return;
    }

    // This is also reachable through MissionController's public Q_INVOKABLE,
    // so the QML-side resume-visibility checks are not a sufficient safety
    // boundary.  An empty/stale mission can otherwise clamp resumeIndex to
    // -1 and immediately dereference _missionItems[-1] below.  Fail closed
    // before touching the mission list; the caller will remain in the
    // non-ready state and can ask the operator to reload the mission.
    if (_missionItems.isEmpty()) {
        qCWarning(MissionManagerLog)
            << "generateResumeMission ignored because the mission is empty";
        return;
    }

    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* item = _missionItems[i];
        if (item->command() == MAV_CMD_DO_JUMP) {
            qgcApp()->showAppMessage(tr("Unable to generate resume mission due to MAV_CMD_DO_JUMP command."));
            return;
        }
    }

    // Be strict about direct QML/plugin input. Clamping an invalid index to
    // zero can silently resume a different mission item than the operator
    // confirmed.
    if (resumeIndex < 0 || resumeIndex >= _missionItems.count()) {
        qCWarning(MissionManagerLog)
            << "generateResumeMission ignored because resumeIndex is out of range" << resumeIndex;
        return;
    }

    // Adjust resume index to be a location based command
    const MissionCommandUIInfo* uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_vehicle, _vehicle->vehicleClass(), _missionItems[resumeIndex]->command());
    if (!uiInfo || uiInfo->isStandaloneCoordinate() || !uiInfo->specifiesCoordinate()) {
        // We have to back up to the last command which the vehicle flies through
        while (--resumeIndex > 0) {
            uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_vehicle, _vehicle->vehicleClass(), _missionItems[resumeIndex]->command());
            if (uiInfo && (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate())) {
                // Found it
                break;
            }

        }
    }
    resumeIndex = qMax(0, resumeIndex);

    QList<MissionItem*> resumeMission;
    auto clearResumeMission = [&resumeMission, &call]() {
        if (!call.alive()) return;
        QList<QPointer<MissionItem>> deleting;
        for (MissionItem* item : resumeMission) deleting.append(QPointer<MissionItem>(item));
        resumeMission.clear();
        for (const QPointer<MissionItem>& item : deleting) {
            if (item) delete item.data();
            if (!call.alive()) return;
        }
    };

    QList<MAV_CMD> includedResumeCommands;

    // If any command in this list occurs before the resumeIndex it will be added to the front of the mission
    includedResumeCommands << MAV_CMD_DO_CONTROL_VIDEO
                           << MAV_CMD_DO_SET_ROI
                           << MAV_CMD_DO_DIGICAM_CONFIGURE
                           << MAV_CMD_DO_DIGICAM_CONTROL
                           << MAV_CMD_DO_MOUNT_CONFIGURE
                           << MAV_CMD_DO_MOUNT_CONTROL
                           << MAV_CMD_DO_SET_CAM_TRIGG_DIST
                           << MAV_CMD_DO_FENCE_ENABLE
                           << MAV_CMD_IMAGE_START_CAPTURE
                           << MAV_CMD_IMAGE_STOP_CAPTURE
                           << MAV_CMD_VIDEO_START_CAPTURE
                           << MAV_CMD_VIDEO_STOP_CAPTURE
                           << MAV_CMD_DO_CHANGE_SPEED
                           << MAV_CMD_SET_CAMERA_MODE;

    bool addHomePosition = _vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    int prefixCommandCount = 0;
    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* oldItem = _missionItems[i];
        const MissionCommandUIInfo* uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_vehicle, _vehicle->vehicleClass(), oldItem->command());
        if ((i == 0 && addHomePosition) || i >= resumeIndex || includedResumeCommands.contains(oldItem->command()) || (uiInfo && uiInfo->isTakeoffCommand())) {
            if (i < resumeIndex) {
                prefixCommandCount++;
            }
            MissionItem* newItem = new MissionItem(*oldItem, this);
            newItem->setIsCurrentItem(false);
            if (!call.alive()) return;
            resumeMission.append(newItem);
        }
    }
    prefixCommandCount = qMax(0, qMin(prefixCommandCount, resumeMission.count()));  // Anal prevention against crashes

    // De-dup and remove no-ops from the commands which were added to the front of the mission
    bool foundROI = false;
    bool foundCameraSetMode = false;
    bool foundCameraStartStop = false;
    prefixCommandCount--;   // Change from count to array index
    while (prefixCommandCount >= 0) {
        MissionItem* resumeItem = resumeMission[prefixCommandCount];
        switch (resumeItem->command()) {
        case MAV_CMD_SET_CAMERA_MODE:
            // Only keep the last one
            if (foundCameraSetMode) {
                delete resumeMission.takeAt(prefixCommandCount);
            }
            foundCameraSetMode = true;
            break;
        case MAV_CMD_DO_SET_ROI:
            // Only keep the last one
            if (foundROI) {
                delete resumeMission.takeAt(prefixCommandCount);
            }
            foundROI = true;
            break;
        case MAV_CMD_DO_SET_CAM_TRIGG_DIST:
        case MAV_CMD_IMAGE_STOP_CAPTURE:
        case MAV_CMD_VIDEO_START_CAPTURE:
        case MAV_CMD_VIDEO_STOP_CAPTURE:
            // Only keep the first of these commands that are found
            if (foundCameraStartStop) {
                delete resumeMission.takeAt(prefixCommandCount);
            }
            foundCameraStartStop = true;
            break;
        case MAV_CMD_IMAGE_START_CAPTURE:
            if (resumeItem->param3() != 0) {
                // Remove commands which do not trigger by time
                delete resumeMission.takeAt(prefixCommandCount);
                break;
            }
            if (foundCameraStartStop) {
                // Only keep the first of these commands that are found
                delete resumeMission.takeAt(prefixCommandCount);
            }
            foundCameraStartStop = true;
            break;
        default:
            break;
        }
        if (!call.alive()) return;

        prefixCommandCount--;
    }

    // Adjust sequence numbers and current item
    if (resumeMission.isEmpty()) {
        qCWarning(MissionManagerLog)
            << "generateResumeMission ignored because the generated mission is empty";
        clearResumeMission();
        return;
    }
    int seqNum = 0;
    for (int i=0; i<resumeMission.count(); i++) {
        resumeMission[i]->setSequenceNumber(seqNum++);
        if (!call.alive()) return;
    }
    // A very short/filtered mission can contain only the home item even when
    // the firmware normally sends home position separately.  Do not assume a
    // second entry exists when selecting the current item.
    const int setCurrentIndex = addHomePosition && resumeMission.count() > 1 ? 1 : 0;
    if (setCurrentIndex < 0 || setCurrentIndex >= resumeMission.count()) {
        qCWarning(MissionManagerLog)
            << "generateResumeMission ignored because no valid current item exists";
        clearResumeMission();
        return;
    }
    resumeMission[setCurrentIndex]->setIsCurrentItem(true);
    if (!call.alive()) return;

    // Send to vehicle
    _clearAndDeleteWriteMissionItems();
    if (!call.alive()) return;
    for (int i=0; i<resumeMission.count(); i++) {
        _writeMissionItems.append(new MissionItem(*resumeMission[i], this));
    }
    // The temporary resume list owns the filtered clones used to build the
    // write list. PlanManager now owns the second copy above; release the
    // temporary list on every successful generation as well.
    clearResumeMission();
    if (!call.alive()) return;
    _resumeMission = true;
    _writeMissionItemsWorker();
}

/// Called when a new mavlink message for out vehicle is received
void MissionManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (transportCallInProgress()) return;
    TransportCallGuard call(this, TransportCallType::ProtocolDispatch);
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;

    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;

    case MAVLINK_MSG_ID_MISSION_CURRENT:
        _handleMissionCurrent(message);
        break;

    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    }
}

void MissionManager::_updateMissionIndex(int index)
{
    QPointer<MissionManager> owner(this);
    if (index != _currentMissionIndex) {
        qCDebug(MissionManagerLog) << "_updateMissionIndex currentIndex:" << index;
        _currentMissionIndex = index;
        emit currentIndexChanged(_currentMissionIndex);
        if (!owner) return;
    }

    if (_currentMissionIndex != _lastCurrentIndex && _cachedLastCurrentIndex != _currentMissionIndex) {
        // We have to be careful of an RTL sequence causing a change of index to the DO_LAND_START sequence. This also triggers
        // a flight mode change away from mission flight mode. So we only update _lastCurrentIndex when the flight mode is mission.
        // But we can run into problems where we may get the MISSION_CURRENT message for the RTL/DO_LAND_START sequenc change prior
        // to the HEARTBEAT message which contains the flight mode change which will cause things to work incorrectly. To fix this
        // We force the sequencing of HEARTBEAT following by MISSION_CURRENT by caching the possible _lastCurrentIndex update until
        // the next HEARTBEAT comes through.
        qCDebug(MissionManagerLog) << "_updateMissionIndex caching _lastCurrentIndex for possible update:" << _currentMissionIndex;
        _cachedLastCurrentIndex = _currentMissionIndex;
    }
}

void MissionManager::_handleHighLatency(const mavlink_message_t& message) 
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);
    _updateMissionIndex(highLatency.wp_num);
}

void MissionManager::_handleHighLatency2(const mavlink_message_t& message) 
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);
    _updateMissionIndex(highLatency2.wp_num);
}

void MissionManager::_handleMissionCurrent(const mavlink_message_t& message)
{
    mavlink_mission_current_t missionCurrent;
    mavlink_msg_mission_current_decode(&message, &missionCurrent);
    _updateMissionIndex(missionCurrent.seq);
}

void MissionManager::_handleHeartbeat(const mavlink_message_t& message)
{
    Q_UNUSED(message);

    if (_cachedLastCurrentIndex != -1 &&  _vehicle->flightMode() == _vehicle->missionFlightMode()) {
        qCDebug(MissionManagerLog) << "_handleHeartbeat updating lastCurrentIndex from cached value:" << _cachedLastCurrentIndex;
        _lastCurrentIndex = _cachedLastCurrentIndex;
        _cachedLastCurrentIndex = -1;
        emit lastCurrentIndexChanged(_lastCurrentIndex);
    }
}

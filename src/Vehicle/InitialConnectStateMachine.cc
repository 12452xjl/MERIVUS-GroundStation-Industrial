/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InitialConnectStateMachine.h"
#include "Vehicle.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "ComponentInformationManager.h"
#include "MissionManager.h"
#include "QGCToolbox.h"

#include <QPointer>

QGC_LOGGING_CATEGORY(InitialConnectStateMachineLog, "InitialConnectStateMachineLog")

const StateMachine::StateFn InitialConnectStateMachine::_rgStates[] = {
    InitialConnectStateMachine::_stateRequestAutopilotVersion,
    InitialConnectStateMachine::_stateRequestProtocolVersion,
    InitialConnectStateMachine::_stateRequestStandardModes,
    InitialConnectStateMachine::_stateRequestCompInfo,
    InitialConnectStateMachine::_stateRequestParameters,
    InitialConnectStateMachine::_stateRequestMission,
    InitialConnectStateMachine::_stateRequestGeoFence,
    InitialConnectStateMachine::_stateRequestRallyPoints,
    InitialConnectStateMachine::_stateSignalInitialConnectComplete
};

const int InitialConnectStateMachine::_rgProgressWeights[] = {
    1, //_stateRequestCapabilities
    1, //_stateRequestProtocolVersion
    1, //_stateRequestStandardModes
    5, //_stateRequestCompInfo
    5, //_stateRequestParameters
    2, //_stateRequestMission
    1, //_stateRequestGeoFence
    1, //_stateRequestRallyPoints
    1, //_stateSignalInitialConnectComplete
};


const int InitialConnectStateMachine::_cStates = sizeof(InitialConnectStateMachine::_rgStates) / sizeof(InitialConnectStateMachine::_rgStates[0]);

InitialConnectStateMachine::InitialConnectStateMachine(Vehicle* vehicle)
    : _vehicle(vehicle)
{
    static_assert(sizeof(_rgStates)/sizeof(_rgStates[0]) == sizeof(_rgProgressWeights)/sizeof(_rgProgressWeights[0]),
            "array size mismatch");

    _progressWeightTotal = 0;
    for (int i = 0; i < _cStates; ++i) {
        _progressWeightTotal += _rgProgressWeights[i];
    }
}

int InitialConnectStateMachine::stateCount(void) const
{
    return _cStates;
}

const InitialConnectStateMachine::StateFn* InitialConnectStateMachine::rgStates(void) const
{
    return &_rgStates[0];
}

void InitialConnectStateMachine::statesCompleted(void) const
{

}

void InitialConnectStateMachine::advance()
{
    const QPointer<InitialConnectStateMachine> self(this);
    const QPointer<Vehicle> vehicle(_vehicle);
    if (!vehicle) return;
    StateMachine::advance();
    // A state can complete synchronously, notify observers, or delete either
    // original object before the base advance returns.
    if (!self || !vehicle || _vehicle != vehicle.data()) return;
    emit progressUpdate(_progress());
}

void InitialConnectStateMachine::gotProgressUpdate(float progressValue)
{
    emit progressUpdate(_progress(progressValue));
}

float InitialConnectStateMachine::_progress(float subProgress)
{
    int progressWeight = 0;
    for (int i = 0; i < _stateIndex && i < _cStates; ++i) {
        progressWeight += _rgProgressWeights[i];
    }
    int currentWeight = _stateIndex < _cStates ? _rgProgressWeights[_stateIndex] : 1;
    return (progressWeight + currentWeight * subProgress) / (float)_progressWeightTotal;
}

void InitialConnectStateMachine::_stateRequestAutopilotVersion(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->_fleetExecutionSourceLinks : QPointer<VehicleLinkManager>());
    const auto current = [&]() { return connectMachine && vehicle && links
        && connectMachine->_vehicle == vehicle.data() && vehicle->vehicleLinkManager() == links.data()
        && connectMachine->currentState() == _stateRequestAutopilotVersion; };
    if (!current()) return;
    SharedLinkInterfacePtr sharedLink = links->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:AUTOPILOT_VERSION request due to no primary link";
        connectMachine->advance();
    } else {
        const auto configuration = sharedLink->linkConfiguration();
        if (!current() || !configuration) return;
        bool skip = configuration->isHighLatency();
        if (!current()) return;
        if (!skip) {
            skip = sharedLink->isPX4Flow();
            if (!current()) return;
        }
        if (!skip) {
            skip = sharedLink->isLogReplay();
            if (!current()) return;
        }
        if (skip) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:AUTOPILOT_VERSION request due to link type";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "Sending REQUEST_MESSAGE:AUTOPILOT_VERSION";
            vehicle->requestMessage(_autopilotVersionRequestMessageHandler,
                                    connectMachine.data(),
                                    MAV_COMP_ID_AUTOPILOT1,
                                    MAVLINK_MSG_ID_AUTOPILOT_VERSION,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, connectMachine.data());
        }
    }
}

void InitialConnectStateMachine::_autopilotVersionRequestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    // requestMessage retains the original callback owner before invoking this
    // borrowed-data handler. These local pins cover callbacks within it.
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(resultHandlerData));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const auto current = [&]() { return connectMachine && vehicle
        && connectMachine->_vehicle == vehicle.data()
        && connectMachine->currentState() == _stateRequestAutopilotVersion; };
    if (!current()) return;
    switch (failureCode) {
    case Vehicle::RequestMessageNoFailure:
    {
        // These persistent native pins may already be empty on a source-loss
        // failure. Never reconstruct them from stale raw providers there.
        const QPointer<QGCToolbox> toolbox(vehicle->_fleetExecutionSourceToolbox);
        const QPointer<FirmwarePlugin> plugin(vehicle->_messageIntervalSourcePlugin);
        const QPointer<QGCCorePlugin> core(toolbox ? toolbox->corePlugin() : nullptr);
        if (!toolbox || !core || !plugin || vehicle->_toolbox != toolbox.data()
            || vehicle->_firmwarePlugin != plugin.data()) return;
        qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION received";

        mavlink_autopilot_version_t autopilotVersion;
        mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

        vehicle->_uid = (quint64)autopilotVersion.uid;
        vehicle->_firmwareBoardVendorId = autopilotVersion.vendor_id;
        vehicle->_firmwareBoardProductId = autopilotVersion.product_id;
        emit vehicle->vehicleUIDChanged();
        if (!current()) return;

        if (autopilotVersion.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;
            FIRMWARE_VERSION_TYPE versionType;

            majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
            versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
            vehicle->setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
            if (!current()) return;
        }

        if (vehicle->px4Firmware()) {
            // Lower 3 bytes is custom version
            int majorVersion, minorVersion, patchVersion;
            majorVersion = autopilotVersion.flight_custom_version[2];
            minorVersion = autopilotVersion.flight_custom_version[1];
            patchVersion = autopilotVersion.flight_custom_version[0];
            vehicle->setFirmwareCustomVersion(majorVersion, minorVersion, patchVersion);
            if (!current()) return;

            // PX4 Firmware stores the first 16 characters of the git hash as binary, with the individual bytes in reverse order
            vehicle->_gitHash = "";
            for (int i = 7; i >= 0; i--) {
                vehicle->_gitHash.append(QString("%1").arg(autopilotVersion.flight_custom_version[i], 2, 16, QChar('0')));
            }
        } else {
            // APM Firmware stores the first 8 characters of the git hash as an ASCII character string
            char nullStr[9];
            strncpy(nullStr, (char*)autopilotVersion.flight_custom_version, 8);
            nullStr[8] = 0;
            vehicle->_gitHash = nullStr;
        }
        if (!toolbox || !core || !plugin || vehicle->_toolbox != toolbox.data()
            || vehicle->_firmwarePlugin != plugin.data()) return;
        QGCOptions* const actualOptions = core->options();
        if (!current() || !toolbox || !core || !plugin
            || vehicle->_toolbox != toolbox.data() || vehicle->_firmwarePlugin != plugin.data()) return;
        const QPointer<QGCOptions> options(actualOptions);
        if (!options
            || vehicle->_toolbox != toolbox.data() || vehicle->_firmwarePlugin != plugin.data()) return;
        const bool checkFirmwareVersion = options->checkFirmwareVersion();
        if (!current() || !toolbox || !core || !plugin || !options
            || vehicle->_toolbox != toolbox.data() || vehicle->_firmwarePlugin != plugin.data()) return;
        if (checkFirmwareVersion && !vehicle->_checkLatestStableFWDone) {
            vehicle->_checkLatestStableFWDone = true;
            plugin->checkIfIsLatestStable(vehicle.data());
            if (!current()) return;
        }
        emit vehicle->gitHashChanged(vehicle->_gitHash);
        if (!current()) return;

        vehicle->_setCapabilities(autopilotVersion.capabilities);
        if (!current()) return;
    }
        break;
    case Vehicle::RequestMessageFailureCommandError:
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("REQUEST_MESSAGE:AUTOPILOT_VERSION command error(%1)").arg(commandResult);
        break;
    case Vehicle::RequestMessageFailureCommandNotAcked:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION command never acked";
        break;
    case Vehicle::RequestMessageFailureMessageNotReceived:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION command acked but message never received";
        break;
    case Vehicle::RequestMessageFailureDuplicateCommand:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION Internal Error: Duplicate command";
        break;
    }

    if (failureCode != Vehicle::RequestMessageNoFailure) {
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION failed. Setting no capabilities";
        uint64_t assumedCapabilities = 0;
        if (vehicle->_mavlinkProtocolRequestMaxProtoVersion >= 200) {
            // Link already running mavlink 2
            assumedCapabilities |= MAV_PROTOCOL_CAPABILITY_MAVLINK2;
        }
        if (vehicle->px4Firmware() || vehicle->apmFirmware()) {
            // We make some assumptions for known firmware
            assumedCapabilities |= MAV_PROTOCOL_CAPABILITY_MISSION_INT | MAV_PROTOCOL_CAPABILITY_COMMAND_INT | MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
        }
        vehicle->_setCapabilities(assumedCapabilities);
        if (!current()) return;
    }

    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestProtocolVersion(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->_fleetExecutionSourceLinks : QPointer<VehicleLinkManager>());
    const auto current = [&]() { return connectMachine && vehicle && links
        && connectMachine->_vehicle == vehicle.data() && vehicle->vehicleLinkManager() == links.data()
        && connectMachine->currentState() == _stateRequestProtocolVersion; };
    if (!current()) return;
    SharedLinkInterfacePtr sharedLink = links->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to no primary link";
        connectMachine->advance();
    } else {
        const auto configuration = sharedLink->linkConfiguration();
        if (!current() || !configuration) return;
        bool skip = configuration->isHighLatency();
        if (!current()) return;
        if (!skip) {
            skip = sharedLink->isPX4Flow();
            if (!current()) return;
        }
        if (!skip) {
            skip = sharedLink->isLogReplay();
            if (!current()) return;
        }
        if (skip) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to link type";
            connectMachine->advance();
        } else if (vehicle->apmFirmware()) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to Ardupilot firmware";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "Sending REQUEST_MESSAGE:PROTOCOL_VERSION";
            vehicle->requestMessage(_protocolVersionRequestMessageHandler,
                                    connectMachine.data(),
                                    MAV_COMP_ID_AUTOPILOT1,
                                    MAVLINK_MSG_ID_PROTOCOL_VERSION,
                                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, connectMachine.data());
        }
    }
}

void InitialConnectStateMachine::_protocolVersionRequestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(resultHandlerData));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const auto current = [&]() { return connectMachine && vehicle
        && connectMachine->_vehicle == vehicle.data()
        && connectMachine->currentState() == _stateRequestProtocolVersion; };
    if (!current()) return;

    switch (failureCode) {
    case Vehicle::RequestMessageNoFailure:
    {
        mavlink_protocol_version_t protoVersion;
        mavlink_msg_protocol_version_decode(&message, &protoVersion);

        qCDebug(InitialConnectStateMachineLog) << "PROTOCOL_VERSION received mav_version:" << protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
        if (!current()) return;
    }
        break;
    case Vehicle::RequestMessageFailureCommandError:
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("REQUEST_MESSAGE PROTOCOL_VERSION command error(%1)").arg(commandResult);
        break;
    case Vehicle::RequestMessageFailureCommandNotAcked:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION command never acked";
        break;
    case Vehicle::RequestMessageFailureMessageNotReceived:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION command acked but message never received";
        break;
    case Vehicle::RequestMessageFailureDuplicateCommand:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION Internal Error: Duplicate command";
        break;
    }

    if (failureCode != Vehicle::RequestMessageNoFailure) {
        // Either the PROTOCOL_VERSION message didn't make it through the pipe from Vehicle->QGC because the pipe is mavlink 1.
        // Or the PROTOCOL_VERSION message was lost on a noisy connection. Either way the best we can do is fall back to mavlink 1.
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("Setting _maxProtoVersion to 100 due to timeout on receiving PROTOCOL_VERSION message.");
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = 100;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
        if (!current()) return;
    }

    connectMachine->advance();
}
void InitialConnectStateMachine::_stateRequestCompInfo(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<ComponentInformationManager> manager(vehicle ? vehicle->_componentInformationManager : nullptr);
    const auto current = [&]() { return connectMachine && vehicle && manager
        && connectMachine->_vehicle == vehicle.data() && vehicle->_componentInformationManager == manager.data()
        && connectMachine->currentState() == _stateRequestCompInfo; };
    if (!current()) return;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestCompInfo";
    const auto connection = connect(manager.data(), &ComponentInformationManager::progressUpdate, connectMachine.data(),
            &InitialConnectStateMachine::gotProgressUpdate);
    if (!connection || !current()) { QObject::disconnect(connection); return; }
    manager->requestAllComponentInformation(_stateRequestCompInfoComplete, connectMachine.data(), connectMachine.data());
}

void InitialConnectStateMachine::_stateRequestStandardModes(StateMachine *stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<StandardModes> modes(vehicle ? vehicle->_standardModes : nullptr);
    const auto current = [&]() { return connectMachine && vehicle && modes
        && connectMachine->_vehicle == vehicle.data() && vehicle->_standardModes == modes.data()
        && connectMachine->currentState() == _stateRequestStandardModes; };
    if (!current()) return;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestStandardModes";
    const auto connection = connect(modes.data(), &StandardModes::requestCompleted, connectMachine.data(),
            &InitialConnectStateMachine::standardModesRequestCompleted);
    if (!connection || !current()) { QObject::disconnect(connection); return; }
    modes->request();
}

void InitialConnectStateMachine::standardModesRequestCompleted()
{
    const QPointer<InitialConnectStateMachine> self(this);
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<StandardModes> modes(vehicle ? vehicle->_standardModes : nullptr);
    if (!vehicle || !modes || currentState() != _stateRequestStandardModes) return;
    disconnect(modes.data(), &StandardModes::requestCompleted, this,
               &InitialConnectStateMachine::standardModesRequestCompleted);
    if (!self || !vehicle || !modes || _vehicle != vehicle.data()
        || vehicle->_standardModes != modes.data() || currentState() != _stateRequestStandardModes) return;
    advance();
}

void InitialConnectStateMachine::_stateRequestCompInfoComplete(void* requestAllCompleteFnData)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(requestAllCompleteFnData));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<ComponentInformationManager> manager(vehicle ? vehicle->_componentInformationManager : nullptr);
    if (!vehicle || !manager || connectMachine->currentState() != _stateRequestCompInfo) return;
    disconnect(manager.data(), &ComponentInformationManager::progressUpdate,
            connectMachine.data(), &InitialConnectStateMachine::gotProgressUpdate);
    if (!connectMachine || !vehicle || !manager || connectMachine->_vehicle != vehicle.data()
        || vehicle->_componentInformationManager != manager.data() || connectMachine->currentState() != _stateRequestCompInfo) return;

    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestParameters(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestParameters";
    QPointer<ParameterManager> manager(vehicle ? vehicle->parameterManager() : nullptr);
    if (!manager) {
        qCWarning(InitialConnectStateMachineLog) << "Parameter source disappeared; initial connect is not complete";
        return;
    }
    const auto connection = connect(manager.data(), &ParameterManager::loadProgressChanged, connectMachine.data(),
            &InitialConnectStateMachine::gotProgressUpdate);
    if (!connection || !connectMachine || !vehicle || !manager || connectMachine->_vehicle != vehicle.data()
        || vehicle->parameterManager() != manager.data() || connectMachine->currentState() != _stateRequestParameters) {
        QObject::disconnect(connection); return;
    }
    manager->refreshAllParameters();
}

void InitialConnectStateMachine::_stateRequestMission(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    QPointer<ParameterManager> manager(vehicle ? vehicle->parameterManager() : nullptr);
    if (!manager) {
        qCWarning(InitialConnectStateMachineLog) << "Parameter source disappeared; initial connect is not complete";
        return;
    }
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->_fleetExecutionSourceLinks : QPointer<VehicleLinkManager>());
    const QPointer<MissionManager> mission(vehicle ? vehicle->_missionManager : nullptr);
    const auto current = [&]() { return connectMachine && vehicle && manager && links && mission
        && connectMachine->_vehicle == vehicle.data() && vehicle->parameterManager() == manager.data()
        && vehicle->vehicleLinkManager() == links.data() && vehicle->_missionManager == mission.data()
        && connectMachine->currentState() == _stateRequestMission; };
    if (!current()) return;
    SharedLinkInterfacePtr sharedLink = links->primaryLink().lock();

    disconnect(manager.data(), &ParameterManager::loadProgressChanged, connectMachine.data(),
            &InitialConnectStateMachine::gotProgressUpdate);
    if (!current()) return;

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to no primary link";
        connectMachine->advance();
    } else {
        const auto configuration = sharedLink->linkConfiguration();
        if (!current() || !configuration) return;
        bool skip = configuration->isHighLatency();
        if (!current()) return;
        if (!skip) {
            skip = sharedLink->isPX4Flow();
            if (!current()) return;
        }
        if (!skip) {
            skip = sharedLink->isLogReplay();
            if (!current()) return;
        }
        if (skip) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to link type";
            vehicle->_firstMissionLoadComplete();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission";
            const auto connection = connect(mission.data(), &MissionManager::progressPct, connectMachine.data(),
                    &InitialConnectStateMachine::gotProgressUpdate);
            if (!connection || !current()) { QObject::disconnect(connection); return; }
            mission->loadFromVehicle(true /* allowInitialConnectRead */);
        }
    }
}

void InitialConnectStateMachine::_stateRequestGeoFence(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->_fleetExecutionSourceLinks : QPointer<VehicleLinkManager>());
    const QPointer<MissionManager> mission(vehicle ? vehicle->_missionManager : nullptr);
    const QPointer<GeoFenceManager> fence(vehicle ? vehicle->_geoFenceManager : nullptr);
    const auto current = [&]() { return connectMachine && vehicle && links && mission && fence
        && connectMachine->_vehicle == vehicle.data() && vehicle->vehicleLinkManager() == links.data()
        && vehicle->_missionManager == mission.data() && vehicle->_geoFenceManager == fence.data()
        && connectMachine->currentState() == _stateRequestGeoFence; };
    if (!current()) return;
    SharedLinkInterfacePtr sharedLink = links->primaryLink().lock();

    disconnect(mission.data(), &MissionManager::progressPct, connectMachine.data(),
               &InitialConnectStateMachine::gotProgressUpdate);
    if (!current()) return;

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to no primary link";
        connectMachine->advance();
    } else {
        const auto configuration = sharedLink->linkConfiguration();
        if (!current() || !configuration) return;
        bool skip = configuration->isHighLatency();
        if (!current()) return;
        if (!skip) {
            skip = sharedLink->isPX4Flow();
            if (!current()) return;
        }
        if (!skip) {
            skip = sharedLink->isLogReplay();
            if (!current()) return;
        }
        if (skip) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to link type";
            vehicle->_firstGeoFenceLoadComplete();
        } else {
            const bool supported = fence->supported();
            if (!current()) return;
            if (supported) {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence";
                const auto connection = connect(fence.data(), &GeoFenceManager::progressPct, connectMachine.data(),
                        &InitialConnectStateMachine::gotProgressUpdate);
                if (!connection || !current()) { QObject::disconnect(connection); return; }
                fence->loadFromVehicle(true /* allowInitialConnectRead */);
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: skipped due to no support";
                vehicle->_firstGeoFenceLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateRequestRallyPoints(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->_fleetExecutionSourceLinks : QPointer<VehicleLinkManager>());
    const QPointer<GeoFenceManager> fence(vehicle ? vehicle->_geoFenceManager : nullptr);
    const QPointer<RallyPointManager> rally(vehicle ? vehicle->_rallyPointManager : nullptr);
    const auto current = [&]() { return connectMachine && vehicle && links && fence && rally
        && connectMachine->_vehicle == vehicle.data() && vehicle->vehicleLinkManager() == links.data()
        && vehicle->_geoFenceManager == fence.data() && vehicle->_rallyPointManager == rally.data()
        && connectMachine->currentState() == _stateRequestRallyPoints; };
    if (!current()) return;
    SharedLinkInterfacePtr sharedLink = links->primaryLink().lock();

    disconnect(fence.data(), &GeoFenceManager::progressPct, connectMachine.data(),
               &InitialConnectStateMachine::gotProgressUpdate);
    if (!current()) return;

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to no primary link";
        connectMachine->advance();
    } else {
        const auto configuration = sharedLink->linkConfiguration();
        if (!current() || !configuration) return;
        bool skip = configuration->isHighLatency();
        if (!current()) return;
        if (!skip) {
            skip = sharedLink->isPX4Flow();
            if (!current()) return;
        }
        if (!skip) {
            skip = sharedLink->isLogReplay();
            if (!current()) return;
        }
        if (skip) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to link type";
            vehicle->_firstRallyPointLoadComplete();
        } else {
            const bool supported = rally->supported();
            if (!current()) return;
            if (supported) {
                const auto connection = connect(rally.data(), &RallyPointManager::progressPct, connectMachine.data(),
                        &InitialConnectStateMachine::gotProgressUpdate);
                if (!connection || !current()) { QObject::disconnect(connection); return; }
                rally->loadFromVehicle(true /* allowInitialConnectRead */);
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: skipping due to no support";
                vehicle->_firstRallyPointLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateSignalInitialConnectComplete(StateMachine* stateMachine)
{
    const QPointer<InitialConnectStateMachine> connectMachine(static_cast<InitialConnectStateMachine*>(stateMachine));
    if (!connectMachine) return;
    const QPointer<Vehicle> vehicle(connectMachine->_vehicle);
    const QPointer<RallyPointManager> rally(vehicle ? vehicle->_rallyPointManager : nullptr);
    if (!vehicle || !rally || connectMachine->currentState() != _stateSignalInitialConnectComplete) return;

    disconnect(rally.data(), &RallyPointManager::progressPct, connectMachine.data(),
               &InitialConnectStateMachine::gotProgressUpdate);
    if (!connectMachine || !vehicle || !rally || connectMachine->_vehicle != vehicle.data()
        || vehicle->_rallyPointManager != rally.data() || connectMachine->currentState() != _stateSignalInitialConnectComplete) return;

    connectMachine->advance();
    if (!connectMachine || !vehicle || connectMachine->_vehicle != vehicle.data() || connectMachine->active()) return;
    qCDebug(InitialConnectStateMachineLog) << "Signalling initialConnectComplete";
    emit vehicle->initialConnectComplete();
}

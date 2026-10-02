/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Layouts          1.2

import QGroundControl                           1.0
import QGroundControl.ScreenTools               1.0
import QGroundControl.Controls                  1.0
import QGroundControl.Palette                   1.0
import QGroundControl.Vehicle                   1.0
import QGroundControl.FlightMap                 1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Controllers           1.0
import Merivus 1.0
/// This provides the smarts behind the guided mode commands, minus the user interface. This way you can change UI
/// without affecting the underlying functionality.
Item {
    id: _root
    SwarmController {
       id: _swarm
    }
    SwarmMissionOrchestrator {
       id: _missionOrchestrator
    }
    FormationPlanner {
       id: _formationPlanner
    }
    FaultToleranceManager {
       id: _faultToleranceManager
    }
    Timer {
       id: _fleetPolicyTimer
       interval: 1000
       repeat: true
       running: true
       onTriggered: {
           var reports = _swarm.fleetRegistry ? _swarm.fleetRegistry.vehicles : []
           for (var i = 0; i < reports.length; i++) {
               var report = reports[i]
               if (report && report.systemId) {
                   _faultToleranceManager.reportVehicle(report)
               }
           }
       }
    }
    property var missionController
    property var confirmDialog
    property var actionList
    property var guidedValueSlider
    property var orbitMapCircle
    property var selectedVehicleIds: []
    property real defaultTakeoffAltitudeMeters: 10
    property string activeSwarmTaskId: ""
    onSelectedVehicleIdsChanged: _formationPlanner.memberIds = selectedVehicleIds
    readonly property bool swarmModeEnabled: _swarm.swarmModeEnabled
    // Expose the single controller instance used by map and command-center
    // views. Creating a second controller in the map would split formation
    // state, watchdogs and temporary-mission bookkeeping.
    readonly property var swarmController: _swarm
    readonly property var swarmTransactions: _swarm.transactions
    readonly property var fleetRegistry: _swarm.fleetRegistry
    readonly property var fleetHealthSummary: _swarm.fleetRegistry.fleetHealthSummary
    readonly property var missionOrchestrator: _missionOrchestrator
    readonly property var formationPlanner: _formationPlanner
    readonly property var faultToleranceManager: _faultToleranceManager
    readonly property bool formationActive: _swarm.formationActive
    readonly property bool formationBusy: _swarm.formationBusy
    readonly property string formationStatus: _swarm.formationStatus

    Connections {
        target: _swarm
        function onFormationActiveChanged() {
            if (!activeSwarmTaskId) return
            if (_swarm.formationActive) {
                _missionOrchestrator.setTaskProgress(activeSwarmTaskId, 0.5)
            }
        }
        function onFormationFault(message) {
            if (!activeSwarmTaskId) return
            _missionOrchestrator.failTask(activeSwarmTaskId, message)
            activeSwarmTaskId = ""
        }
    }

    // Fault policy only changes the task assignment/state here. It never
    // sends a flight command automatically; an operator or a dedicated
    // command adapter must execute any resulting replan.
    Connections {
        target: _faultToleranceManager
        function onVehicleOffline(systemId, reason, suggestedAction) {
            if (!activeSwarmTaskId || !_missionOrchestrator.hasTask(activeSwarmTaskId)) return
            var summary = _missionOrchestrator.taskSummary(activeSwarmTaskId)
            if (!summary || summary.phase === "completed" || summary.phase === "cancelled" || summary.phase === "failed") return

            var targets = summary.targetVehicleIds ? summary.targetVehicleIds.slice(0) : []
            var filtered = []
            for (var i = 0; i < targets.length; i++) {
                if (Number(targets[i]) !== Number(systemId)) filtered.push(targets[i])
            }

            if (suggestedAction === "reassign" && filtered.length > 0) {
                _missionOrchestrator.reassignTask(activeSwarmTaskId, filtered)
            }
            _missionOrchestrator.pauseTask(activeSwarmTaskId,
                                           "vehicle_%1_%2".arg(systemId).arg(reason))
        }
        function onVehicleLowBattery(systemId, batteryPercent, suggestedAction) {
            if (!activeSwarmTaskId || !_missionOrchestrator.hasTask(activeSwarmTaskId)) return
            var summary = _missionOrchestrator.taskSummary(activeSwarmTaskId)
            if (!summary || summary.phase === "completed" || summary.phase === "cancelled" || summary.phase === "failed") return
            _missionOrchestrator.pauseTask(activeSwarmTaskId,
                                           "vehicle_%1_low_battery_%2".arg(systemId).arg(Math.round(batteryPercent)))
        }
    }

    readonly property string emergencyStopTitle:            qsTr("EMERGENCY STOP")
    readonly property string armTitle:                      qsTr("Arm")
    readonly property string forceArmTitle:                 qsTr("Force Arm")
    readonly property string disarmTitle:                   qsTr("Disarm")
    readonly property string rtlTitle:                      qsTr("Return")
    readonly property string takeoffTitle:                  qsTr("Takeoff")
    readonly property string gripperTitle:                  qsTr("Gripper Function")
    readonly property string landTitle:                     qsTr("Land")
    readonly property string startMissionTitle:             qsTr("Start Mission")
    readonly property string mvStartMissionTitle:           qsTr("Start Mission (MV)")
    readonly property string continueMissionTitle:          qsTr("Continue Mission")
    readonly property string resumeMissionUploadFailTitle:  qsTr("Resume FAILED")
    readonly property string pauseTitle:                    qsTr("Pause")
    readonly property string mvPauseTitle:                  qsTr("Pause (MV)")
    readonly property string changeAltTitle:                qsTr("Change Altitude")
    readonly property string changeCruiseSpeedTitle:        qsTr("Change Max Ground Speed")
    readonly property string changeAirspeedTitle:           qsTr("Change Airspeed")
    readonly property string orbitTitle:                    qsTr("Orbit")
    readonly property string landAbortTitle:                qsTr("Land Abort")
    readonly property string setWaypointTitle:              qsTr("Set Waypoint")
    readonly property string gotoTitle:                     qsTr("Go To Location")
    readonly property string vtolTransitionTitle:           qsTr("VTOL Transition")
    readonly property string roiTitle:                      qsTr("ROI")
    readonly property string setHomeTitle:                  qsTr("Set Home")
    readonly property string actionListTitle:               qsTr("Action")
    readonly property string swarmTitle:                   qsTr("编队验证任务")
    readonly property string endSwarmTitle:                qsTr("结束编队")
    readonly property string queuedMissionTitle:           qsTr("实时航点队列")

    readonly property string armMessage:                        qsTr("Arm the vehicle.")
    readonly property string forceArmMessage:                   qsTr("WARNING: This will force arming of the vehicle bypassing any safety checks.")
    readonly property string disarmMessage:                     qsTr("Disarm the vehicle")
    readonly property string emergencyStopMessage:              qsTr("WARNING: THIS WILL STOP ALL MOTORS. IF VEHICLE IS CURRENTLY IN THE AIR IT WILL CRASH.")
    readonly property string takeoffMessage:                    qsTr("Takeoff from ground and hold position.")
    readonly property string gripperMessage:                       qsTr("Grab or Release the cargo")
    readonly property string startMissionMessage:               qsTr("Takeoff from ground and start the current mission.")
    readonly property string continueMissionMessage:            qsTr("Continue the mission from the current waypoint.")
    readonly property string resumeMissionUploadFailMessage:    qsTr("Upload of resume mission failed. Confirm to retry upload")
    readonly property string landMessage:                       qsTr("Land the vehicle at the current position.")
    readonly property string rtlMessage:                        qsTr("Return to the launch position of the vehicle.")
    readonly property string changeAltMessage:                  qsTr("Change the altitude of the vehicle up or down.")
    readonly property string changeCruiseSpeedMessage:          qsTr("Change the maximum horizontal cruise speed.")
    readonly property string changeAirspeedMessage:             qsTr("Change the equivalent airspeed setpoint")
    readonly property string gotoMessage:                       qsTr("Move the vehicle to the specified location.")
             property string setWaypointMessage:                qsTr("Adjust current waypoint to %1.").arg(_actionData)
    readonly property string orbitMessage:                      qsTr("Orbit the vehicle around the specified location.")
    readonly property string landAbortMessage:                  qsTr("Abort the landing sequence.")
    readonly property string pauseMessage:                      qsTr("Pause the vehicle at it's current position, adjusting altitude up or down as needed.")
    readonly property string mvPauseMessage:                    qsTr("Pause all vehicles at their current position.")
    readonly property string vtolTransitionFwdMessage:          qsTr("Transition VTOL to fixed wing flight.")
    readonly property string vtolTransitionMRMessage:           qsTr("Transition VTOL to multi-rotor flight.")
    readonly property string roiMessage:                        qsTr("Make the specified location a Region Of Interest.")
    readonly property string setHomeMessage:                    qsTr("Set vehicle home as the specified location. This will affect Return to Home position")
    readonly property string swarmMessage:                     qsTr("按 PREPARE、COMMIT、RELEASE 三阶段启动所选 1-6 机；任一成员失败会自动向整组下发 ABORT。")
    readonly property string endSwarmMessage:                  qsTr("向当前会话成员下发 ABORT、停止位置租约，并等待成员进入悬停。")

    readonly property int actionRTL:                        1
    readonly property int actionLand:                       2
    readonly property int actionTakeoff:                    3
    readonly property int actionArm:                        4
    readonly property int actionDisarm:                     5
    readonly property int actionEmergencyStop:              6
    readonly property int actionChangeAlt:                  7
    readonly property int actionGoto:                       8
    readonly property int actionSetWaypoint:                9
    readonly property int actionOrbit:                      10
    readonly property int actionLandAbort:                  11
    readonly property int actionStartMission:               12
    readonly property int actionContinueMission:            13
    readonly property int actionResumeMission:              14
    readonly property int _actionUnused:                    15
    readonly property int actionResumeMissionUploadFail:    16
    readonly property int actionPause:                      17
    readonly property int actionMVPause:                    18
    readonly property int actionMVStartMission:             19
    readonly property int actionVtolTransitionToFwdFlight:  20
    readonly property int actionVtolTransitionToMRFlight:   21
    readonly property int actionROI:                        22
    readonly property int actionActionList:                 23
    readonly property int actionForceArm:                   24
    readonly property int actionChangeSpeed:                25
    readonly property int actionGripper:                    26
    readonly property int actionSetHome:                    27
    readonly property int actionSwarm:                      28
    readonly property int actionEndSwarm:                   29
    readonly property int actionQueuedMission:              30

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _useChecklist:              QGroundControl.settingsManager.appSettings.useChecklist.rawValue && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
    property bool   _enforceChecklist:          _useChecklist && QGroundControl.settingsManager.appSettings.enforceChecklist.rawValue
    property bool   _checklistPassed:           _activeVehicle ? (_useChecklist ? (_enforceChecklist ? _activeVehicle.checkListState === Vehicle.CheckListPassed : true) : true) : true
    property bool   _canArm:                    _activeVehicle ? (_checklistPassed && (!_activeVehicle.healthAndArmingCheckReport.supported || _activeVehicle.healthAndArmingCheckReport.canArm)) : false
    property bool   _canTakeoff:                _activeVehicle ? (_checklistPassed && (!_activeVehicle.healthAndArmingCheckReport.supported || _activeVehicle.healthAndArmingCheckReport.canTakeoff)) : false
    property bool   _canStartMission:           _activeVehicle ? (_checklistPassed && (!_activeVehicle.healthAndArmingCheckReport.supported || _activeVehicle.healthAndArmingCheckReport.canStartMission)) : false
    property bool   _initialConnectComplete:    _activeVehicle ? _activeVehicle.initialConnectComplete : false

    property bool showEmergenyStop:     _guidedActionsEnabled && !_hideEmergenyStop && _vehicleArmed && _vehicleFlying
    property bool showArm:              _guidedActionsEnabled && !_vehicleArmed && _canArm
    property bool showForceArm:         _guidedActionsEnabled && !_vehicleArmed
    property bool showDisarm:           _guidedActionsEnabled && _vehicleArmed && !_vehicleFlying
    property bool showRTL:              _guidedActionsEnabled && _vehicleArmed && _activeVehicle.guidedModeSupported && _vehicleFlying && !_vehicleInRTLMode
    property bool showTakeoff:          _guidedActionsEnabled && _activeVehicle.takeoffVehicleSupported && !_vehicleFlying && _canTakeoff
    property bool showLand:             _guidedActionsEnabled && _activeVehicle.guidedModeSupported && _vehicleArmed && !_activeVehicle.fixedWing && !_vehicleInLandMode
    property bool showStartMission:     _guidedActionsEnabled && _missionAvailable && !_missionActive && !_vehicleFlying && _canStartMission
    property bool showContinueMission:  _guidedActionsEnabled && _missionAvailable && !_missionActive && _vehicleArmed && _vehicleFlying && (_currentMissionIndex < _missionItemCount - 1)
    property bool showPause:            _guidedActionsEnabled && _vehicleArmed && _activeVehicle.pauseVehicleSupported && _vehicleFlying && !_vehiclePaused && !_fixedWingOnApproach
    property bool showChangeAlt:        _guidedActionsEnabled && _vehicleFlying && _activeVehicle.guidedModeSupported && _vehicleArmed && !_missionActive
    property bool showChangeSpeed:      _guidedActionsEnabled && _vehicleFlying && _activeVehicle.guidedModeSupported && _vehicleArmed && !_missionActive && _speedLimitsAvailable
    property bool showOrbit:            _guidedActionsEnabled && _vehicleFlying && __orbitSupported && !_missionActive
    property bool showROI:              _guidedActionsEnabled && _vehicleFlying && __roiSupported && !_missionActive
    property bool showLandAbort:        _guidedActionsEnabled && _vehicleFlying && _fixedWingOnApproach
    property bool showGotoLocation:     _guidedActionsEnabled && _vehicleFlying
    property bool showSetHome:          _guidedActionsEnabled
    property bool showActionList:       _guidedActionsEnabled && (showStartMission || showContinueMission || showResumeMission || showChangeAlt || showLandAbort || actionList.hasCustomActions)
    property bool showGripper:          _initialConnectComplete ? _activeVehicle.hasGripper : false
    property string changeSpeedTitle:   _fixedWing ? changeAirspeedTitle : changeCruiseSpeedTitle
    property string changeSpeedMessage: _fixedWing ? changeAirspeedMessage : changeCruiseSpeedMessage

    // Note: The '_missionItemCount - 2' is a hack to not trigger resume mission when a mission ends with an RTL item
    property bool showResumeMission:    _activeVehicle && !_vehicleArmed && _vehicleWasFlying && _missionAvailable && _resumeMissionIndex > 0 && (_resumeMissionIndex < _missionItemCount - 2)

    property bool guidedUIVisible:      confirmDialog.visible || actionList.visible

    property var    _corePlugin:            QGroundControl.corePlugin
    property var    _corePluginOptions:     QGroundControl.corePlugin.options
    property bool   _guidedActionsEnabled:  (!ScreenTools.isDebug && _corePluginOptions.guidedActionsRequireRCRSSI && _activeVehicle) ? _rcRSSIAvailable : _activeVehicle
    property string _flightMode:            _activeVehicle ? _activeVehicle.flightMode : ""
    property bool   _missionAvailable:      missionController.containsItems
    property bool   _missionActive:         _activeVehicle ? _vehicleArmed && (_vehicleInLandMode || _vehicleInRTLMode || _vehicleInMissionMode) : false
    property bool   _vehicleArmed:          _activeVehicle ? _activeVehicle.armed  : false
    property bool   _vehicleFlying:         _activeVehicle ? _activeVehicle.flying  : false
    property bool   _vehicleLanding:        _activeVehicle ? _activeVehicle.landing  : false
    property bool   _vehiclePaused:         false
    property bool   _vehicleInMissionMode:  false
    property bool   _vehicleInRTLMode:      false
    property bool   _vehicleInLandMode:     false
    property int    _missionItemCount:      missionController.missionItemCount
    property int    _currentMissionIndex:   missionController.currentMissionIndex
    property int    _resumeMissionIndex:    missionController.resumeMissionIndex
    property bool   _hideEmergenyStop:      !_corePluginOptions.flyView.guidedBarShowEmergencyStop
    property bool   _hideOrbit:             !_corePluginOptions.flyView.guidedBarShowOrbit
    property bool   _hideROI:               !_corePluginOptions.flyView.guidedBarShowROI
    property bool   _vehicleWasFlying:      false
    property bool   _rcRSSIAvailable:       _activeVehicle ? _activeVehicle.rcRSSI > 0 && _activeVehicle.rcRSSI <= 100 : false
    property bool   _fixedWingOnApproach:   _activeVehicle ? _activeVehicle.fixedWing && _vehicleLanding : false
    property bool   _fixedWing:             _activeVehicle ? _activeVehicle.fixedWing || _activeVehicle.vtolInFwdFlight : false
    property bool  _speedLimitsAvailable:   _activeVehicle && ((_fixedWing && _activeVehicle.haveFWSpeedLimits) || (!_fixedWing && _activeVehicle.haveMRSpeedLimits))
    property var   _gripperFunction:        undefined

    // You can turn on log output for GuidedActionsController by turning on GuidedActionsControllerLog category
    property bool __guidedModeSupported:    _activeVehicle ? _activeVehicle.guidedModeSupported : false
    property bool __pauseVehicleSupported:  _activeVehicle ? _activeVehicle.pauseVehicleSupported : false
    property bool __roiSupported:           _activeVehicle ? !_hideROI && _activeVehicle.roiModeSupported : false
    property bool __orbitSupported:         _activeVehicle ? !_hideOrbit && _activeVehicle.orbitModeSupported : false
    property bool __flightMode:             _flightMode

    function _outputState() {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log(qsTr("_activeVehicle(%1) _vehicleArmed(%2) guidedModeSupported(%3) _vehicleFlying(%4) _vehicleWasFlying(%5) _vehicleInRTLMode(%6) pauseVehicleSupported(%7) _vehiclePaused(%8) _flightMode(%9) _missionItemCount(%10) roiSupported(%11) orbitSupported(%12) _missionActive(%13) _hideROI(%14) _hideOrbit(%15)").arg(_activeVehicle ? 1 : 0).arg(_vehicleArmed ? 1 : 0).arg(__guidedModeSupported ? 1 : 0).arg(_vehicleFlying ? 1 : 0).arg(_vehicleWasFlying ? 1 : 0).arg(_vehicleInRTLMode ? 1 : 0).arg(__pauseVehicleSupported ? 1 : 0).arg(_vehiclePaused ? 1 : 0).arg(_flightMode).arg(_missionItemCount).arg(__roiSupported).arg(__orbitSupported).arg(_missionActive).arg(_hideROI).arg(_hideOrbit))
        }
    }

    function _snapshotVehicleIds(ids) {
        // 冻结确认时的目标，避免对话框停留期间的 UI 选择变化扩大或替换执行范围。
        var snapshot = []
        if (!ids || ids.length === undefined) return snapshot
        for (var i = 0; i < ids.length; i++) snapshot.push(Number(ids[i]))
        return snapshot
    }

    function _vehicleIdsText(ids) {
        return ids && ids.length > 0 ? ids.join(", UAV-") : "--"
    }

    function _takeoffMinimumForTargets(ids) {
        var minimum = _activeVehicle ? _activeVehicle.minimumTakeoffAltitude() : 0
        if (!ids || ids.length === 0) return minimum

        for (var i = 0; i < ids.length; i++) {
            var vehicle = QGroundControl.multiVehicleManager.getVehicleById(ids[i])
            if (vehicle) minimum = Math.max(minimum, vehicle.minimumTakeoffAltitude())
        }
        return minimum
    }

    function _showBatchResult(title, result, showSuccess) {
        if (!result) return

        var dispatched = result.dispatchedIds ? result.dispatchedIds : []
        var skipped = result.skippedIds ? result.skippedIds : []
        if (!showSuccess && result.ok && skipped.length === 0) return

        var lines = []
        if (result.message) lines.push(result.message)
        if (result.transactionId) {
            lines.push(qsTr("事务：%1（请继续观察 ACK/遥测状态）").arg(result.transactionId))
        }
        if (dispatched.length > 0) {
            lines.push(qsTr("已下发：UAV-%1").arg(dispatched.join(", UAV-")))
        }
        if (skipped.length > 0) {
            lines.push(qsTr("已跳过：UAV-%1（请检查链路、健康状态、任务和飞行状态）").arg(skipped.join(", UAV-")))
        }
        mainWindow.showMessageDialog(title, lines.join("\n"))
    }

    function setupSlider(actionCode) {
        // generic defaults
        guidedValueSlider.configureAsLinearSlider()
        guidedValueSlider.setIsSpeedSlider(false)

        if (actionCode === actionTakeoff) {
                var takeoffMinimum = _takeoffMinimumForTargets(_actionData)
                guidedValueSlider.setMinVal(takeoffMinimum)
                guidedValueSlider.setValue(Math.max(takeoffMinimum, defaultTakeoffAltitudeMeters))
                guidedValueSlider.setDisplayText("Height")
        } else if (actionCode === actionChangeSpeed) {
            guidedValueSlider.setIsSpeedSlider(true)
            if (_fixedWing) {
                guidedValueSlider.setDisplayText("Set Airspeed")
                guidedValueSlider.setMinVal(QGroundControl.unitsConversion.metersSecondToAppSettingsSpeedUnits(_activeVehicle.minimumEquivalentAirspeed()).toFixed(1))
                guidedValueSlider.setMaxVal(QGroundControl.unitsConversion.metersSecondToAppSettingsSpeedUnits(_activeVehicle.maximumEquivalentAirspeed()).toFixed(1))
                guidedValueSlider.setValue(_activeVehicle.airSpeed.value)
            } else if (!_fixedWing && _activeVehicle.haveMRSpeedLimits) {
                guidedValueSlider.setDisplayText("Set Speed")
                guidedValueSlider.setMinVal(QGroundControl.unitsConversion.metersSecondToAppSettingsSpeedUnits(0.1).toFixed(1))
                guidedValueSlider.setMaxVal(QGroundControl.unitsConversion.metersSecondToAppSettingsSpeedUnits(_activeVehicle.maximumHorizontalSpeedMultirotor()).toFixed(1))
                guidedValueSlider.setValue(QGroundControl.unitsConversion.metersSecondToAppSettingsSpeedUnits(_activeVehicle.maximumHorizontalSpeedMultirotor()/2).toFixed(1))
            }
        } else if (actionCode === actionChangeAlt || actionCode === actionOrbit || actionCode === actionGoto || actionCode === actionPause) {
            guidedValueSlider.setDisplayText("New Alt(rel)")
            guidedValueSlider.configureAsRelativeAltSliderExp()
        }
    }

    on_ActiveVehicleChanged: _outputState()

    Component.onCompleted:              _outputState()
    on_VehicleArmedChanged:             _outputState()
    on_VehicleInRTLModeChanged:         _outputState()
    on_VehiclePausedChanged:            _outputState()
    on__FlightModeChanged:              _outputState()
    on__GuidedModeSupportedChanged:     _outputState()
    on__PauseVehicleSupportedChanged:   _outputState()
    on__RoiSupportedChanged:            _outputState()
    on__OrbitSupportedChanged:          _outputState()
    on_MissionItemCountChanged:         _outputState()
    on_MissionActiveChanged:            _outputState()

    on_CurrentMissionIndexChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("_currentMissionIndex", _currentMissionIndex)
        }
    }
    on_ResumeMissionIndexChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("_resumeMissionIndex", _resumeMissionIndex)
        }
    }
    onShowResumeMissionChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showResumeMission", showResumeMission)
        }
        _outputState()
    }
    onShowStartMissionChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showStartMission", showStartMission)
        }
        _outputState()
        if (showStartMission) {
            confirmAction(actionStartMission)
        }
    }
    onShowContinueMissionChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showContinueMission", showContinueMission)
        }
        _outputState()
    }
    onShowRTLChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showRTL", showRTL)
        }
        _outputState()
    }
    onShowChangeAltChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showChangeAlt", showChangeAlt)
        }
        _outputState()
    }
    onShowROIChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showROI", showROI)
        }
        _outputState()
    }
    onShowOrbitChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showOrbit", showOrbit)
        }
        _outputState()
    }
    onShowGotoLocationChanged: {
        if (_corePlugin.guidedActionsControllerLogging()) {
            console.log("showGotoLocation", showGotoLocation)
        }
        _outputState()
    }
    onShowLandAbortChanged: {
        if (showLandAbort) {
            confirmAction(actionLandAbort)
        }
    }

    on_VehicleFlyingChanged: {
        _outputState()
        if (!_vehicleFlying) {
            // We use _vehicleWasFLying to help trigger Resume Mission only if the vehicle actually flew and came back down.
            // Otherwise it may trigger during the Start Mission sequence due to signal ordering or armed and resume mission index.
            _vehicleWasFlying = true
        }
    }

    property var _actionData

    on_FlightModeChanged: {
        _vehiclePaused =        _activeVehicle ? _flightMode === _activeVehicle.pauseFlightMode : false
        _vehicleInRTLMode =     _activeVehicle ? _flightMode === _activeVehicle.rtlFlightMode || _flightMode === _activeVehicle.smartRTLFlightMode : false
        _vehicleInLandMode =    _activeVehicle ? _flightMode === _activeVehicle.landFlightMode : false
        _vehicleInMissionMode = _activeVehicle ? _flightMode === _activeVehicle.missionFlightMode : false // Must be last to get correct signalling for showStartMission popups
    }

    Connections {
        target:                     missionController
        function onResumeMissionUploadFail() { confirmAction(actionResumeMissionUploadFail) }
    }

    Connections {
        target: _swarm
        function onFormationFault(message) {
            mainWindow.showMessageDialog(qsTr("编队异常"), message)
        }
    }

    Connections {
        target:                             mainWindow
        function onArmVehicleRequest() { armVehicleRequest() }
        function onForceArmVehicleRequest() { forceArmVehicleRequest() }
        function onDisarmVehicleRequest() { disarmVehicleRequest() }
        function onVtolTransitionToFwdFlightRequest() { vtolTransitionToFwdFlightRequest() }
        function onVtolTransitionToMRFlightRequest() { vtolTransitionToMRFlightRequest() }
    }

    function armVehicleRequest() {
        confirmAction(actionArm)
    }

    function forceArmVehicleRequest() {
        confirmAction(actionForceArm)
    }

    function disarmVehicleRequest() {
        if (showEmergenyStop) {
            confirmAction(actionEmergencyStop)
        } else {
            confirmAction(actionDisarm)
        }

    }

    function vtolTransitionToFwdFlightRequest() {
        confirmAction(actionVtolTransitionToFwdFlight)
    }

    function vtolTransitionToMRFlightRequest() {
        confirmAction(actionVtolTransitionToMRFlight)
    }

    function closeAll() {
        if (confirmDialog && (confirmDialog.visible || confirmDialog.mapIndicator)) {
            confirmDialog.confirmCancelled()
        } else if (confirmDialog) {
            confirmDialog.visible = false
        }
        actionList.visible =        false
        guidedValueSlider.visible =    false
    }

    // 所有 MERIVUS 批量动作先在这里冻结目标并进入人工确认；确认内容与执行数据同源。
    function confirmAction(actionCode, actionData, mapIndicator) {
        var showImmediate = true
        closeAll()
        confirmDialog.action = actionCode
        confirmDialog.hideTrigger = true
        confirmDialog.mapIndicator = mapIndicator
        confirmDialog.optionText = ""

        var frozenActionData = actionData
        if (actionCode === actionTakeoff || actionCode === actionLand
                || actionCode === actionRTL || actionCode === actionSwarm) {
            frozenActionData = actionData && actionData.length !== undefined
                    ? _snapshotVehicleIds(actionData)
                    : _snapshotVehicleIds(selectedVehicleIds)
        }
        _actionData = frozenActionData
        confirmDialog.actionData = frozenActionData

        setupSlider(actionCode)

        switch (actionCode) {
        case actionArm:
            if (_vehicleFlying || !_guidedActionsEnabled) {
                return
            }
            confirmDialog.title = armTitle
            confirmDialog.message = armMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showArm })
            break;
        case actionForceArm:
            confirmDialog.title = forceArmTitle
            confirmDialog.message = forceArmMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showForceArm })
            break;
        case actionDisarm:
            if (_vehicleFlying) {
                return
            }
            confirmDialog.title = disarmTitle
            confirmDialog.message = disarmMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showDisarm })
            break;
        case actionEmergencyStop:
            confirmDialog.title = emergencyStopTitle
            confirmDialog.message = emergencyStopMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showEmergenyStop })
            break;
        case actionTakeoff:
            confirmDialog.title = takeoffTitle
            if (_actionData && _actionData.length > 0) {
                confirmDialog.message = qsTr("让框选的 %1 架无人机起飞并定高悬停。\n目标：UAV-%2")
                                                .arg(_actionData.length).arg(_vehicleIdsText(_actionData))
                confirmDialog.hideTrigger = true
            } else {
                confirmDialog.message = takeoffMessage
                confirmDialog.hideTrigger = Qt.binding(function() { return !showTakeoff })
            }
            guidedValueSlider.visible = true
            break;
        case actionStartMission:
            showImmediate = false
            confirmDialog.title = startMissionTitle
            confirmDialog.message = startMissionMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showStartMission })
            break;
        case actionMVStartMission:
            confirmDialog.title = mvStartMissionTitle
            confirmDialog.message = startMissionMessage
            confirmDialog.hideTrigger = true
            break;
        case actionContinueMission:
            confirmDialog.title = continueMissionTitle
            confirmDialog.message = continueMissionMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showContinueMission })
            break;
        case actionResumeMission:
            // Resume Mission is handled in mission end dialog
            return
        case actionResumeMissionUploadFail:
            confirmDialog.title = resumeMissionUploadFailTitle
            confirmDialog.message = resumeMissionUploadFailMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showResumeMission })
            break;
        case actionLand:
            confirmDialog.title = landTitle
            if (_actionData && _actionData.length > 1) {
                confirmDialog.message = qsTr("让框选的 %1 架无人机在当前位置降落。\n目标：UAV-%2")
                                                .arg(_actionData.length).arg(_vehicleIdsText(_actionData))
                confirmDialog.hideTrigger = true
            } else {
                confirmDialog.message = landMessage
                confirmDialog.hideTrigger = Qt.binding(function() { return !showLand })
            }
            break;
        case actionRTL:
            confirmDialog.title = rtlTitle
            if (_actionData && _actionData.length > 1) {
                confirmDialog.message = qsTr("让框选的 %1 架无人机分别返回各自起飞点。\n目标：UAV-%2\n批量返航统一使用标准 RTL。")
                                                .arg(_actionData.length).arg(_vehicleIdsText(_actionData))
                confirmDialog.hideTrigger = true
            } else {
                confirmDialog.message = rtlMessage
                if (_activeVehicle.supportsSmartRTL) {
                    confirmDialog.optionText = qsTr("Smart RTL")
                    confirmDialog.optionChecked = false
                }
                confirmDialog.hideTrigger = Qt.binding(function() { return !showRTL })
            }
            break;
        case actionChangeAlt:
            confirmDialog.title = changeAltTitle
            confirmDialog.message = changeAltMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showChangeAlt })
            guidedValueSlider.visible = true
            break;
        case actionGoto:
            confirmDialog.title = gotoTitle
            confirmDialog.message = gotoMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showGotoLocation })
            break;
        case actionSetWaypoint:
            confirmDialog.title = setWaypointTitle
            confirmDialog.message = setWaypointMessage
            break;
        case actionOrbit:
            confirmDialog.title = orbitTitle
            confirmDialog.message = orbitMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showOrbit })
            guidedValueSlider.visible = true
            break;
        case actionLandAbort:
            confirmDialog.title = landAbortTitle
            confirmDialog.message = landAbortMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showLandAbort })
            break;
        case actionPause:
            confirmDialog.title = pauseTitle
            confirmDialog.message = pauseMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showPause })
            guidedValueSlider.visible = true
            break;
        case actionMVPause:
            confirmDialog.title = mvPauseTitle
            confirmDialog.message = mvPauseMessage
            confirmDialog.hideTrigger = true
            break;
        case actionVtolTransitionToFwdFlight:
            confirmDialog.title = vtolTransitionTitle
            confirmDialog.message = vtolTransitionFwdMessage
            confirmDialog.hideTrigger = true
            break
        case actionVtolTransitionToMRFlight:
            confirmDialog.title = vtolTransitionTitle
            confirmDialog.message = vtolTransitionMRMessage
            confirmDialog.hideTrigger = true
            break
        case actionROI:
            confirmDialog.title = roiTitle
            confirmDialog.message = roiMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showROI })
            break;
        case actionActionList:
            actionList.show()
            return
        case actionChangeSpeed:
            confirmDialog.hideTrigger = true
            confirmDialog.title = changeSpeedTitle
            confirmDialog.message = changeSpeedMessage
            guidedValueSlider.visible = true
            break
        case actionGripper:
            confirmDialog.hideTrigger = true
            confirmDialog.title = gripperTitle
            confirmDialog.message = gripperMessage
            _widgetLayer._gripperMenu.createObject(mainWindow).open()
            break
        case actionSetHome:
            confirmDialog.title = setHomeTitle
            confirmDialog.message = setHomeMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showSetHome })
            break
        case actionSwarm:
            var formationIds = _actionData ? _actionData.slice(0).sort(function(a, b) { return a - b }) : []
            // The protocol supports any selected group from one to six
            // vehicles.  A previous UI guard only admitted 1/2/6, which
            // silently blocked valid 3-, 4-, and 5-vehicle sessions even
            // though SwarmController accepts them.
            var validCount = formationIds.length >= 1 && formationIds.length <= 6
            var validLeader = formationIds.length > 0 && Number(formationIds[0]) === 1
            var validSix = formationIds.length !== 6 || formationIds.join(",") === "1,2,3,4,5,6"
            if (!validCount || !validLeader || !validSix) {
                mainWindow.showMessageDialog(qsTr("无法启动编队"), qsTr("请选择包含 UAV-1 的 1～6 架无人机；若选择 6 架，必须为 UAV-1～UAV-6。"))
                return
            }
            confirmDialog.title = swarmTitle
            confirmDialog.message = swarmMessage
                    + "\n" + qsTr("主机：UAV-1")
                    + "\n" + qsTr("成员：UAV-%1").arg(formationIds.join("、UAV-"))
                    + "\n" + (swarmModeEnabled ? qsTr("编队协议：已启用") : qsTr("编队协议：未启用，确认后不会发送"))
            confirmDialog.hideTrigger =true
            break
        case actionEndSwarm:
            if (!formationActive && !formationBusy) {
                mainWindow.showMessageDialog(qsTr("无法结束编队"), qsTr("当前没有活动的编队事务。"))
                return
            }
            confirmDialog.title = endSwarmTitle
            confirmDialog.message = endSwarmMessage
            confirmDialog.hideTrigger = true
            break
        case actionQueuedMission:
            if (!_actionData || !_actionData.vehicleIds || _actionData.vehicleIds.length === 0
                    || !_actionData.waypointCount) {
                return
            }
            confirmDialog.title = queuedMissionTitle
            confirmDialog.message = qsTr("目标：UAV-%1\n航点数：%2\n%3")
                    .arg(_vehicleIdsText(_actionData.vehicleIds))
                    .arg(_actionData.waypointCount)
                    .arg(_actionData.replacementRequired
                         ? qsTr("目标中存在正在执行或未清理的任务；确认后将悬停并替换该任务。")
                         : qsTr("确认后上传并执行本次临时任务，各机保持冻结时的相对间距。"))
            break
        default:
            console.warn("Unknown actionCode", actionCode)
            return
        }
        confirmDialog.show(showImmediate)
    }

    // 这里只执行 Guided 确认框产生的人工动作；AI Assistant 没有调用此函数的路径。
    function executeAction(actionCode, actionData, sliderOutputValue, optionChecked) {
        var i;
        var rgVehicle;
        var result;
        switch (actionCode) {
        case actionRTL:
            if (actionData && actionData.length > 1) {
                result = _swarm.executeRTL(actionData)
                _showBatchResult(qsTr("批量返航"), result, true)
            } else {
                _activeVehicle.guidedModeRTL(optionChecked)
            }
            break
        case actionLand:
            if (actionData && actionData.length > 1) {
                result = _swarm.executeLand(actionData)
                _showBatchResult(qsTr("批量降落"), result, true)
            } else {
                _activeVehicle.guidedModeLand()
            }
            break
        case actionTakeoff:
            if (actionData && actionData.length > 0) {
                result = _swarm.executeTakeoff(actionData, sliderOutputValue)
                _showBatchResult(qsTr("批量起飞"), result, false)
            } else {
                _activeVehicle.guidedModeTakeoff(sliderOutputValue)
            }
            break
        case actionResumeMission:
        case actionResumeMissionUploadFail:
            missionController.resumeMission(missionController.resumeMissionIndex)
            break
        case actionStartMission:
        case actionContinueMission:
            _activeVehicle.startMission()
            break
        case actionMVStartMission:
            rgVehicle = QGroundControl.multiVehicleManager.vehicles
            for (i = 0; i < rgVehicle.count; i++) {
                rgVehicle.get(i).startMission()
            }
            break
        case actionArm:
            _activeVehicle.armed = true
            break
        case actionForceArm:
            _activeVehicle.forceArm()
            break
        case actionDisarm:
            _activeVehicle.armed = false
            break
        case actionEmergencyStop:
            _activeVehicle.emergencyStop()
            break
        case actionChangeAlt:
            _activeVehicle.guidedModeChangeAltitude(sliderOutputValue, false /* pauseVehicle */)
            break
        case actionGoto:
            _activeVehicle.guidedModeGotoLocation(actionData)
            break
        case actionSetWaypoint:
            _activeVehicle.setCurrentMissionSequence(actionData)
            break
        case actionOrbit:
            _activeVehicle.guidedModeOrbit(orbitMapCircle.center, orbitMapCircle.radius() * (orbitMapCircle.clockwiseRotation ? 1 : -1), _activeVehicle.altitudeAMSL.rawValue + sliderOutputValue)
            break
        case actionLandAbort:
            _activeVehicle.abortLanding(50)     // hardcoded value for climbOutAltitude that is currently ignored
            break
        case actionPause:
            _activeVehicle.guidedModeChangeAltitude(sliderOutputValue, true /* pauseVehicle */)
            break
        case actionMVPause:
            rgVehicle = QGroundControl.multiVehicleManager.vehicles
            for (i = 0; i < rgVehicle.count; i++) {
                rgVehicle.get(i).pauseVehicle()
            }
            break
        case actionVtolTransitionToFwdFlight:
            _activeVehicle.vtolInFwdFlight = true
            break
        case actionVtolTransitionToMRFlight:
            _activeVehicle.vtolInFwdFlight = false
            break
        case actionROI:
            _activeVehicle.guidedModeROI(actionData)
            break
        case actionChangeSpeed:
            if (_activeVehicle) {
                // We need to convert back to m/s as that is what mavlink standard uses for MAV_CMD_DO_CHANGE_SPEED
                var metersSecondSpeed = QGroundControl.unitsConversion.appSettingsSpeedUnitsToMetersSecond(sliderOutputValue).toFixed(1)
                if (_activeVehicle.vtolInFwdFlight || _activeVehicle.fixedWing) {
                   _activeVehicle.guidedModeChangeEquivalentAirspeedMetersSecond(metersSecondSpeed)
                } else {
                    _activeVehicle.guidedModeChangeGroundSpeedMetersSecond(metersSecondSpeed)
                }
            }
            break
        case actionGripper:           
            _gripperFunction === undefined ? _activeVehicle.sendGripperAction(Vehicle.Invalid_option) : _activeVehicle.sendGripperAction(_gripperFunction)
            break
        case actionSetHome:
            _activeVehicle.doSetHome(actionData)
            break
        case actionSwarm:
            var formationIds = actionData ? actionData : []
            var formationTaskId = _missionOrchestrator.createTask(
                        "formation", formationIds,
                        { protocol: "prepare_commit_release", operatorConfirmed: true })
            result = _swarm.sendStartCommand(actionData ? actionData : [])
            if (formationTaskId) {
                if (result && result.ok) {
                    _missionOrchestrator.startTask(formationTaskId)
                    _missionOrchestrator.setTaskProgress(formationTaskId, 0.1)
                    activeSwarmTaskId = formationTaskId
                } else {
                    _missionOrchestrator.failTask(formationTaskId,
                                                   result && result.message ? result.message : "formation_dispatch_failed")
                }
            }
            _showBatchResult(qsTr("编队任务"), result, true)
            break
        case actionEndSwarm:
            result = _swarm.endFormationSession()
            if (activeSwarmTaskId && result && result.ok) {
                _missionOrchestrator.cancelTask(activeSwarmTaskId, "formation_stop_requested")
                activeSwarmTaskId = ""
            }
            _showBatchResult(qsTr("结束编队"), result, true)
            break
        case actionQueuedMission:
            // The map indicator owns the frozen route and executes it after confirmation.
            break
        default:
            console.warn(qsTr("Internal error: unknown actionCode"), actionCode)
            break
        }
    }
}

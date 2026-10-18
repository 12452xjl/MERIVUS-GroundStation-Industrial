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

/// This provides the smarts behind the guided mode commands, minus the user interface. This way you can change UI
/// without affecting the underlying functionality.
Item {
    id: _root

    property var missionController
    property var confirmDialog
    property var actionList
    property var guidedValueSlider
    property var orbitMapCircle

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
    readonly property string resumeMissionTitle:            qsTr("Resume Mission")
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

    readonly property string armMessage:                        qsTr("Arm the vehicle.")
    readonly property string forceArmMessage:                   qsTr("WARNING: This will force arming of the vehicle bypassing any safety checks.")
    readonly property string disarmMessage:                     qsTr("Disarm the vehicle")
    readonly property string emergencyStopMessage:              qsTr("WARNING: THIS WILL STOP ALL MOTORS. IF VEHICLE IS CURRENTLY IN THE AIR IT WILL CRASH.")
    readonly property string takeoffMessage:                    qsTr("Takeoff from ground and hold position.")
    readonly property string gripperMessage:                       qsTr("Grab or Release the cargo")
    readonly property string startMissionMessage:               qsTr("Takeoff from ground and start the current mission.")
    readonly property string continueMissionMessage:            qsTr("Continue the mission from the current waypoint.")
    readonly property string resumeMissionMessage:              qsTr("Resume the mission from the last flown waypoint.")
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
    property bool showActionList:       _guidedActionsEnabled && (showStartMission || showResumeMission || showChangeAlt || showLandAbort || actionList.hasCustomActions)
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

    function setupSlider(actionCode) {
        // generic defaults
        guidedValueSlider.configureAsLinearSlider()
        guidedValueSlider.setIsSpeedSlider(false)

        if (actionCode === actionTakeoff) {
                guidedValueSlider.setMinVal(_activeVehicle.minimumTakeoffAltitude())
                guidedValueSlider.setValue(_activeVehicle ? _activeVehicle.minimumTakeoffAltitude() : 0)
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

    // The stock controller is also used as the compatibility fallback when a
    // custom plugin does not provide the MERIVUS controller.  Keep that path
    // fail-closed as well: a stale confirmation must never turn into a direct
    // Vehicle command after the link disappeared or before the initial
    // handshake completed.
    function _commandTransportFailure(vehicle) {
        if (!vehicle) {
            return qsTr("No vehicle is selected.")
        }
        if (!vehicle.initialConnectComplete) {
            return qsTr("Vehicle link is not ready.")
        }
        if (!vehicle.vehicleLinkManager) {
            return qsTr("Vehicle link manager is unavailable.")
        }
        if (vehicle.vehicleLinkManager.communicationLost) {
            return qsTr("Vehicle communication is lost.")
        }
        return ""
    }

    function _guardCommandVehicle(vehicle, title) {
        var failure = _commandTransportFailure(vehicle)
        if (failure.length === 0) {
            return true
        }
        console.warn("Guided command blocked:", title, failure)
        if (mainWindow && typeof mainWindow.showMessageDialog === "function") {
            mainWindow.showMessageDialog(qsTr("Command blocked"), failure)
        }
        return false
    }

    function _guardCommandFleet(vehicles, title) {
        if (!vehicles || typeof vehicles.count !== "number"
                || typeof vehicles.get !== "function") {
            return _guardCommandVehicle(null, title)
        }
        // Preflight every target before the first command is sent.  This
        // avoids a partially-started multi-vehicle action when one link has
        // already gone stale.
        for (var index = 0; index < vehicles.count; index++) {
            if (!_guardCommandVehicle(vehicles.get(index), title)) {
                return false
            }
        }
        return vehicles.count > 0
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
        if (showContinueMission) {
            confirmAction(actionContinueMission)
        }
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
    property int _confirmedSetWaypointItemCount: -1
    property var _actionTargetVehicleIds: []
    // Freeze the confirmed target so delayed callbacks cannot retarget a
    // direct Vehicle command after the operator switches active vehicles.
    property var _confirmedVehicle: null

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
        confirmDialog.visible =     false
        actionList.visible =        false
        guidedValueSlider.visible =    false
        _confirmedVehicle = null
        _confirmedSetWaypointItemCount = -1
        _actionTargetVehicleIds = []
    }

    // Called when an action is about to be executed in order to confirm
    function confirmAction(actionCode, actionData, mapIndicator) {
        var showImmediate = true
        closeAll()
        var frozenActionData = actionData
        if (actionCode === actionOrbit && mapIndicator
                && typeof mapIndicator.radius === "function") {
            var orbitRadius = Number(mapIndicator.radius())
            var orbitCenter = actionData
            if (!orbitCenter || typeof orbitCenter.isValid !== "function"
                    || !orbitCenter.isValid() || !isFinite(orbitRadius)
                    || orbitRadius <= 0 || orbitRadius > 100000) {
                console.warn("Orbit confirmation blocked: invalid center or radius")
                return
            }
            // Freeze all geometry used by the command. The map indicator is
            // intentionally retained only for display; it remains mutable
            // while the confirmation dialog is open.
            var orbitLatitude = Number(orbitCenter.latitude)
            var orbitLongitude = Number(orbitCenter.longitude)
            var orbitAltitude = Number(orbitCenter.altitude)
            if (!isFinite(orbitLatitude) || orbitLatitude < -90 || orbitLatitude > 90
                    || !isFinite(orbitLongitude) || orbitLongitude < -180 || orbitLongitude > 180
                    || !isFinite(orbitAltitude)) {
                console.warn("Orbit confirmation blocked: coordinate is not finite")
                return
            }
            frozenActionData = {
                latitude: orbitLatitude,
                longitude: orbitLongitude,
                altitude: orbitAltitude,
                radius: orbitRadius,
                clockwise: !!mapIndicator.clockwiseRotation,
                vehicleId: _activeVehicle ? _activeVehicle.id : 0
            }
        }
        if (actionCode === actionSetWaypoint) {
            var confirmedWaypointIndex = Number(actionData)
            var confirmedWaypointCount = Number(_missionItemCount)
            if (!isFinite(confirmedWaypointIndex) || Math.floor(confirmedWaypointIndex) !== confirmedWaypointIndex
                    || confirmedWaypointIndex < 0
                    || (isFinite(confirmedWaypointCount) && confirmedWaypointCount > 0
                        && confirmedWaypointIndex >= confirmedWaypointCount)) {
                console.warn("Set waypoint confirmation blocked: invalid mission index")
                return
            }
            _confirmedSetWaypointItemCount = confirmedWaypointCount
        } else {
            _confirmedSetWaypointItemCount = -1
        }
        if (actionCode === actionROI || actionCode === actionSetHome) {
            var coordinateSnapshot = actionData
            var coordinateLatitude = coordinateSnapshot ? Number(coordinateSnapshot.latitude) : NaN
            var coordinateLongitude = coordinateSnapshot ? Number(coordinateSnapshot.longitude) : NaN
            var coordinateAltitude = coordinateSnapshot ? Number(coordinateSnapshot.altitude) : NaN
            if (!coordinateSnapshot || typeof coordinateSnapshot.isValid !== "function"
                    || !coordinateSnapshot.isValid()
                    || !isFinite(coordinateLatitude) || coordinateLatitude < -90 || coordinateLatitude > 90
                    || !isFinite(coordinateLongitude) || coordinateLongitude < -180 || coordinateLongitude > 180
                    || !isFinite(coordinateAltitude)) {
                console.warn("Coordinate action confirmation blocked: invalid coordinate")
                return
            }
            frozenActionData = { latitude: coordinateLatitude,
                                 longitude: coordinateLongitude,
                                 altitude: coordinateAltitude,
                                 vehicleId: _activeVehicle ? _activeVehicle.id : 0 }
        }
        if (actionCode === actionMVPause) {
            var frozenPauseIds = []
            var pauseVehicles = QGroundControl.multiVehicleManager.vehicles
            for (var pauseIndex = 0; pauseIndex < pauseVehicles.count; pauseIndex++) {
                var pauseId = Number(pauseVehicles.get(pauseIndex).id)
                if (isFinite(pauseId) && Math.floor(pauseId) === pauseId && pauseId > 0) {
                    frozenPauseIds.push(pauseId)
                }
            }
            _actionTargetVehicleIds = frozenPauseIds
            frozenActionData = frozenPauseIds.slice(0)
        }
        confirmDialog.action = actionCode
        confirmDialog.actionData = frozenActionData
        confirmDialog.hideTrigger = true
        confirmDialog.mapIndicator = mapIndicator
        confirmDialog.optionText = ""
        _actionData = frozenActionData
        _confirmedVehicle = _activeVehicle

        setupSlider(actionCode)

        switch (actionCode) {
        case actionArm:
            if (_vehicleFlying || !_guidedActionsEnabled) {
                _confirmedVehicle = null
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
                _confirmedVehicle = null
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
            confirmDialog.message = takeoffMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showTakeoff })
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
            showImmediate = false
            confirmDialog.title = continueMissionTitle
            confirmDialog.message = continueMissionMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showContinueMission })
            break;
        case actionResumeMission:
            // Resume Mission is requested by the mission-complete dialog, but
            // still goes through the same confirmation and frozen-target path
            // as every other direct vehicle action.
            showImmediate = false
            confirmDialog.title = resumeMissionTitle
            confirmDialog.message = resumeMissionMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showResumeMission })
            break
        case actionResumeMissionUploadFail:
            confirmDialog.title = resumeMissionUploadFailTitle
            confirmDialog.message = resumeMissionUploadFailMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showResumeMission })
            break;
        case actionLand:
            confirmDialog.title = landTitle
            confirmDialog.message = landMessage
            confirmDialog.hideTrigger = Qt.binding(function() { return !showLand })
            break;
        case actionRTL:
            confirmDialog.title = rtlTitle
            confirmDialog.message = rtlMessage
            if (_activeVehicle.supportsSmartRTL) {
                confirmDialog.optionText = qsTr("Smart RTL")
                confirmDialog.optionChecked = false
            }
            confirmDialog.hideTrigger = Qt.binding(function() { return !showRTL })
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
            _confirmedVehicle = null
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
        default:
            console.warn("Unknown actionCode", actionCode)
            _confirmedVehicle = null
            return
        }
        confirmDialog.show(showImmediate)
    }

    // Executes the specified action
    function executeAction(actionCode, actionData, sliderOutputValue, optionChecked) {
        var i;
        var rgVehicle;
        var targetVehicle = _activeVehicle;
        var multiVehicleAction = actionCode === actionMVStartMission
                || actionCode === actionMVPause;

        // Re-check transport state at execution time.  The confirmation UI
        // can remain open while a vehicle disconnects, so checks made when
        // confirmAction() was opened are not sufficient.
        if (actionCode === actionMVPause) {
            if (!actionData || actionData !== _actionData || actionData.length === undefined || actionData.length === 0) {
                _confirmedVehicle = null
                return false
            }
            for (var pausePreflightIndex = 0; pausePreflightIndex < actionData.length; pausePreflightIndex++) {
                var pausePreflight = QGroundControl.multiVehicleManager.getVehicleById(actionData[pausePreflightIndex])
                if (!pausePreflight || !_guardCommandVehicle(pausePreflight, "multi-vehicle pause")
                        || typeof pausePreflight.pauseVehicle !== "function") {
                    _confirmedVehicle = null
                    return false
                }
            }
        } else if (multiVehicleAction) {
            if (!_guardCommandFleet(QGroundControl.multiVehicleManager.vehicles,
                                    "multi-vehicle guided action")) {
                _confirmedVehicle = null
                return false
            }
        } else {
            targetVehicle = _confirmedVehicle;
            if (!targetVehicle || targetVehicle !== _activeVehicle
                    || !_guardCommandVehicle(targetVehicle, "guided action")) {
                _confirmedVehicle = null
                return false
            }
        }

        switch (actionCode) {
        case actionRTL:
            targetVehicle.guidedModeRTL(optionChecked)
            break
        case actionLand:
            targetVehicle.guidedModeLand()
            break
        case actionTakeoff:
            targetVehicle.guidedModeTakeoff(sliderOutputValue)
            break
        case actionResumeMission:
        case actionResumeMissionUploadFail:
            missionController.resumeMission(missionController.resumeMissionIndex)
            break
        case actionStartMission:
        case actionContinueMission:
            targetVehicle.startMission()
            break
        case actionMVStartMission:
            rgVehicle = QGroundControl.multiVehicleManager.vehicles
            if (!_guardCommandFleet(rgVehicle, "multi-vehicle start mission")) {
                _confirmedVehicle = null
                return false
            }
            for (i = 0; i < rgVehicle.count; i++) {
                rgVehicle.get(i).startMission()
            }
            break
        case actionArm:
            targetVehicle.armed = true
            break
        case actionForceArm:
            targetVehicle.forceArm()
            break
        case actionDisarm:
            targetVehicle.armed = false
            break
        case actionEmergencyStop:
            targetVehicle.emergencyStop()
            break
        case actionChangeAlt:
            targetVehicle.guidedModeChangeAltitude(sliderOutputValue, false /* pauseVehicle */)
            break
        case actionGoto:
            targetVehicle.guidedModeGotoLocation(actionData)
            break
        case actionSetWaypoint:
            var waypointIndex = Number(actionData)
            var waypointCount = Number(_missionItemCount)
            if (!isFinite(waypointIndex) || Math.floor(waypointIndex) !== waypointIndex
                    || waypointIndex < 0
                    || (isFinite(waypointCount) && waypointCount > 0 && waypointIndex >= waypointCount)
                    || (_confirmedSetWaypointItemCount > 0 && waypointCount !== _confirmedSetWaypointItemCount)) {
                console.warn("Set waypoint execution blocked: invalid mission index")
                _confirmedVehicle = null
                return false
            }
            targetVehicle.setCurrentMissionSequence(actionData)
            break
        case actionOrbit:
            if (!actionData || actionData !== _actionData
                    || !isFinite(Number(actionData.radius))
                    || Number(actionData.radius) <= 0
                    || Number(actionData.radius) > 100000
                    || !isFinite(Number(actionData.latitude)) || Number(actionData.latitude) < -90 || Number(actionData.latitude) > 90
                    || !isFinite(Number(actionData.longitude)) || Number(actionData.longitude) < -180 || Number(actionData.longitude) > 180
                    || !isFinite(Number(actionData.altitude))
                    || Number(actionData.vehicleId) !== Number(targetVehicle.id)) {
                console.warn("Orbit execution blocked: confirmation geometry or target identity changed")
                _confirmedVehicle = null
                return false
            }
            var orbitExecutionCoordinate = QtPositioning.coordinate(Number(actionData.latitude),
                                                                    Number(actionData.longitude),
                                                                    Number(actionData.altitude))
            targetVehicle.guidedModeOrbit(orbitExecutionCoordinate,
                                          Number(actionData.radius) * (actionData.clockwise ? 1 : -1),
                                          targetVehicle.altitudeAMSL.rawValue + sliderOutputValue)
            break
        case actionLandAbort:
            targetVehicle.abortLanding(50)     // hardcoded value for climbOutAltitude that is currently ignored
            break
        case actionPause:
            targetVehicle.guidedModeChangeAltitude(sliderOutputValue, true /* pauseVehicle */)
            break
        case actionMVPause:
            for (i = 0; i < actionData.length; i++) {
                var frozenPauseVehicle = QGroundControl.multiVehicleManager.getVehicleById(actionData[i])
                if (!frozenPauseVehicle || !_guardCommandVehicle(frozenPauseVehicle, "multi-vehicle pause")
                        || typeof frozenPauseVehicle.pauseVehicle !== "function") {
                    _confirmedVehicle = null
                    return false
                }
                frozenPauseVehicle.pauseVehicle()
            }
            _actionTargetVehicleIds = []
            break
        case actionVtolTransitionToFwdFlight:
            targetVehicle.vtolInFwdFlight = true
            break
        case actionVtolTransitionToMRFlight:
            targetVehicle.vtolInFwdFlight = false
            break
        case actionROI:
            if (!actionData || actionData !== _actionData
                    || !isFinite(Number(actionData.latitude)) || Number(actionData.latitude) < -90 || Number(actionData.latitude) > 90
                    || !isFinite(Number(actionData.longitude)) || Number(actionData.longitude) < -180 || Number(actionData.longitude) > 180
                    || !isFinite(Number(actionData.altitude))) {
                console.warn("ROI execution blocked: invalid coordinate")
                _confirmedVehicle = null
                return false
            }
            targetVehicle.guidedModeROI(QtPositioning.coordinate(Number(actionData.latitude),
                                                                 Number(actionData.longitude),
                                                                 Number(actionData.altitude)))
            break
        case actionChangeSpeed:
            if (targetVehicle) {
                // We need to convert back to m/s as that is what mavlink standard uses for MAV_CMD_DO_CHANGE_SPEED
                var metersSecondSpeed = QGroundControl.unitsConversion.appSettingsSpeedUnitsToMetersSecond(sliderOutputValue).toFixed(1)
                if (targetVehicle.vtolInFwdFlight || targetVehicle.fixedWing) {
                   targetVehicle.guidedModeChangeEquivalentAirspeedMetersSecond(metersSecondSpeed)
                } else {
                    targetVehicle.guidedModeChangeGroundSpeedMetersSecond(metersSecondSpeed)
                }
            }
            break
        case actionGripper:           
            _gripperFunction === undefined ? targetVehicle.sendGripperAction(Vehicle.Invalid_option) : targetVehicle.sendGripperAction(_gripperFunction)
            break
        case actionSetHome:
            if (!actionData || actionData !== _actionData
                    || !isFinite(Number(actionData.latitude)) || Number(actionData.latitude) < -90 || Number(actionData.latitude) > 90
                    || !isFinite(Number(actionData.longitude)) || Number(actionData.longitude) < -180 || Number(actionData.longitude) > 180
                    || !isFinite(Number(actionData.altitude))) {
                console.warn("Set home execution blocked: invalid coordinate")
                _confirmedVehicle = null
                return false
            }
            targetVehicle.doSetHome(QtPositioning.coordinate(Number(actionData.latitude),
                                                             Number(actionData.longitude),
                                                             Number(actionData.altitude)))
            break
        default:
            console.warn(qsTr("Internal error: unknown actionCode"), actionCode)
            _confirmedVehicle = null
            return false
        }
        _confirmedVehicle = null
        return true
    }
}

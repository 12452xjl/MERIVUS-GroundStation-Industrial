/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                    2.11
import QtQuick.Controls             2.4
import QtLocation                   5.3
import QtPositioning                5.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import Merivus                     1.0

FlightMap {
    id:                         _root

    // Share the controller owned by GuidedActionsController. A second
    // SwarmController here would create a separate watchdog/session state and
    // make the map disagree with the command center.
    readonly property var swarmController: globals.guidedControllerFlyView
                                             ? globals.guidedControllerFlyView.swarmController
                                             : null

    // Shared fleet state is exposed by the controller so overlays can render
    // health and capability summaries without reaching into QGC internals.
    readonly property var fleetRegistry: swarmController ? swarmController.fleetRegistry : null
    readonly property var fleetHealthSummary: fleetRegistry ? fleetRegistry.fleetHealthSummary : ({})
    readonly property var missionOrchestrator: globals.guidedControllerFlyView
                                                       ? globals.guidedControllerFlyView.missionOrchestrator
                                                       : null
    readonly property var formationPlanner: globals.guidedControllerFlyView
                                                    ? globals.guidedControllerFlyView.formationPlanner
                                                    : null
    readonly property var faultToleranceManager: globals.guidedControllerFlyView
                                                        ? globals.guidedControllerFlyView.faultToleranceManager
                                                        : null

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    // ============================================================================
    // 区块 1：基础设置与状态管理 (画中画与基础属性)
    // ============================================================================
    allowGCSLocationCenter:     true
    allowVehicleLocationCenter: !_keepVehicleCentered
    planView:                   false
    zoomLevel:                  QGroundControl.flightMapZoom
    center:                     QGroundControl.flightMapPosition

    // 画中画 (PiP) 状态管理
    property Item pipState: _pipState
    QGCPipState {
        id:         _pipState
        pipOverlay: _pipOverlay
        isDark:     _isFullWindowItemDark
    }

    property var    rightPanelWidth
    property var    planMasterController
    property bool   pipMode:                false
    property var    toolInsets
    property alias  videoDockTarget:        commandCenterOverlay.videoDockTarget

    // 【原有】用于保存当前框选的多架无人机 ID 集合
    property var    selectedSwarmIds:       []
    property bool   suppressExclusiveSelectionSync: false

    function setVehicleSelection(vehicleId, selected) {
        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        var nextSelection = selectedSwarmIds ? selectedSwarmIds.slice(0) : []
        var selectionIndex = nextSelection.indexOf(vehicleId)

        if (selected && selectionIndex === -1) {
            nextSelection.push(vehicleId)
        } else if (!selected && selectionIndex !== -1) {
            nextSelection.splice(selectionIndex, 1)
        }

        selectedSwarmIds = nextSelection

        if (selected) {
            var selectedVehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
            if (selectedVehicle) {
                suppressExclusiveSelectionSync = true
                QGroundControl.multiVehicleManager.activeVehicle = selectedVehicle
                suppressExclusiveSelectionSync = false
                _showCurrentTaskRoute(vehicleId)
            }
        }
    }

    function focusVehicle(vehicleId) {
        var vehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
        if (!vehicle) {
            return
        }

        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        selectedSwarmIds = [vehicleId]
        suppressExclusiveSelectionSync = true
        QGroundControl.multiVehicleManager.activeVehicle = vehicle
        suppressExclusiveSelectionSync = false
        _showCurrentTaskRoute(vehicleId)
    }

    function clearVehicleSelection() {
        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        selectedSwarmIds = []
        shiftCommittedCoords = []
    }

    // Shift 草稿坐标只用于预览和确认；确认前不进入 MissionManager。
    property var    shiftQueuedCoords:      []
    property var    shiftCommittedCoords:   []
    property var    shiftFrozenVehicleIds:  []
    property var    shiftFrozenReferences:  []
    property bool   shiftReplacementRequired: false
    property var    activeTaskRoutes: ({})
    property bool   shiftMissionConfirmPending: false

    QtObject {
        id: shiftMissionConfirmationAdapter

        function actionConfirmed() {
            _root.shiftMissionConfirmPending = false
            _root.executeBatchQueuedFly()
        }

        function actionCancelled() {
            _root._cancelShiftDraft(true)
        }
    }

    function _cancelShiftDraft(fromConfirmation) {
        if (!fromConfirmation && shiftMissionConfirmPending
                && globals.guidedControllerFlyView
                && globals.guidedControllerFlyView.confirmDialog
                && globals.guidedControllerFlyView.confirmDialog.mapIndicator === shiftMissionConfirmationAdapter) {
            shiftMissionConfirmPending = false
            globals.guidedControllerFlyView.confirmDialog.confirmCancelled()
        }
        shiftQueuedCoords = []
        shiftFrozenVehicleIds = []
        shiftFrozenReferences = []
        shiftReplacementRequired = false
        shiftMissionConfirmPending = false
        _showCurrentTaskRoute(_activeVehicle ? _activeVehicle.id : -1)
    }

    function _showCurrentTaskRoute(vehicleId) {
        var route = activeTaskRoutes[String(vehicleId)]
        shiftCommittedCoords = route ? route.slice(0) : []
    }

    function _cloneRouteMap() {
        var clone = {}
        for (var key in activeTaskRoutes) {
            if (activeTaskRoutes.hasOwnProperty(key)) clone[key] = activeTaskRoutes[key]
        }
        return clone
    }

    function _freezeShiftTargets() {
        var ids = selectedSwarmIds && selectedSwarmIds.length > 0
                ? selectedSwarmIds.slice(0)
                : (_activeVehicle ? [_activeVehicle.id] : [])
        if (ids.length === 0) {
            mainWindow.showMessageDialog(qsTr("无法创建航点队列"), qsTr("请先选择至少一架无人机。"))
            return false
        }

        var references = []
        for (var i = 0; i < ids.length; i++) {
            var vehicle = QGroundControl.multiVehicleManager.getVehicleById(ids[i])
            if (!vehicle || !vehicle.coordinate || !vehicle.coordinate.isValid) {
                mainWindow.showMessageDialog(qsTr("无法创建航点队列"), qsTr("UAV-%1 没有有效位置。").arg(ids[i]))
                return false
            }
            references.push(vehicle.coordinate)
        }
        shiftFrozenVehicleIds = ids
        shiftFrozenReferences = references
        return true
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        function onActiveVehicleChanged(activeVehicle) {
            if (_root.suppressExclusiveSelectionSync) return
            if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
            _root.selectedSwarmIds = activeVehicle ? [activeVehicle.id] : []
            _root._showCurrentTaskRoute(activeVehicle ? activeVehicle.id : -1)
        }
    }

    Connections {
        target: swarmController
        function onTemporaryMissionCompleted(vehicleId, clearError) {
            var routes = _root._cloneRouteMap()
            delete routes[String(vehicleId)]
            _root.activeTaskRoutes = routes
            if (_root._activeVehicle && _root._activeVehicle.id === vehicleId) {
                _root.shiftCommittedCoords = []
            }
            if (clearError) {
                mainWindow.showMessageDialog(
                    qsTr("临时任务清理失败"),
                    qsTr("UAV-%1 的旧任务未收到清除 ACK，已标记为残留任务；再次下达队列任务时必须确认替换。").arg(vehicleId))
            }
        }
    }

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  planMasterController
    property var    _geoFenceController:    planMasterController.geoFenceController
    property var    _rallyPointController:  planMasterController.rallyPointController
    property var    _activeVehicleCoordinate:_activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property real   _toolButtonTopMargin:   parent.height - mainWindow.height + (ScreenTools.defaultFontPixelHeight / 2)
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75

    property var    _flyViewSettings:           QGroundControl.settingsManager.flyViewSettings
    property bool   _keepMapCenteredOnVehicle:  _flyViewSettings.keepMapCenteredOnVehicle.rawValue
    property bool   _disableVehicleTracking:    false
    property bool   _keepVehicleCentered:       pipMode ? true : false
    property bool   _saveZoomLevelSetting:      true

    function _adjustMapZoomForPipMode() {
        _saveZoomLevelSetting = false
        if (pipMode) {
            if (QGroundControl.flightMapZoom > 3) {
                zoomLevel = QGroundControl.flightMapZoom - 3
            }
        } else {
            zoomLevel = QGroundControl.flightMapZoom
        }
        _saveZoomLevelSetting = true
    }

    onPipModeChanged: _adjustMapZoomForPipMode()

    onVisibleChanged: {
        if (visible) {
            center = QGroundControl.flightMapPosition
        }
    }

    onZoomLevelChanged: {
        if (_saveZoomLevelSetting) {
            QGroundControl.flightMapZoom = zoomLevel
        }
    }
    onCenterChanged: {
        QGroundControl.flightMapPosition = center
    }

    // ============================================================================
    // 区块 2：地图平移与无人机自动追踪逻辑
    // ============================================================================
    Connections {
        target: gesture
        function onPanStarted() {       _disableVehicleTracking = true }
        function onFlickStarted() {     _disableVehicleTracking = true }
        function onPanFinished() {      panRecenterTimer.restart() }
        function onFlickFinished() {    panRecenterTimer.restart() }
    }

    function pointInRect(point, rect) {
        return point.x > rect.x && point.x < rect.x + rect.width && point.y > rect.y && point.y < rect.y + rect.height;
    }

    property real _animatedLatitudeStart
    property real _animatedLatitudeStop
    property real _animatedLongitudeStart
    property real _animatedLongitudeStop
    property real animatedLatitude
    property real animatedLongitude

    onAnimatedLatitudeChanged: _root.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)
    onAnimatedLongitudeChanged: _root.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)

    NumberAnimation on animatedLatitude { id: animateLat; from: _animatedLatitudeStart; to: _animatedLatitudeStop; duration: 1000 }
    NumberAnimation on animatedLongitude { id: animateLong; from: _animatedLongitudeStart; to: _animatedLongitudeStop; duration: 1000 }

    function animatedMapRecenter(fromCoord, toCoord) {
        _animatedLatitudeStart = fromCoord.latitude
        _animatedLongitudeStart = fromCoord.longitude
        _animatedLatitudeStop = toCoord.latitude
        _animatedLongitudeStop = toCoord.longitude
        animateLat.start()
        animateLong.start()
    }

    function _insetCenterRect() {
        return Qt.rect(toolInsets.leftEdgeCenterInset, toolInsets.topEdgeCenterInset,
                       _root.width - toolInsets.leftEdgeCenterInset - toolInsets.rightEdgeCenterInset,
                       _root.height - toolInsets.topEdgeCenterInset - toolInsets.bottomEdgeCenterInset)
    }

    function _insetCornerRects() {
        return {
            "topleft":      Qt.rect(0,0, toolInsets.leftEdgeTopInset, toolInsets.topEdgeLeftInset),
            "topright":     Qt.rect(_root.width-toolInsets.rightEdgeTopInset,0, toolInsets.rightEdgeTopInset, toolInsets.topEdgeRightInset),
            "bottomleft":   Qt.rect(0,_root.height-toolInsets.bottomEdgeLeftInset, toolInsets.leftEdgeBottomInset, toolInsets.bottomEdgeLeftInset),
            "bottomright":  Qt.rect(_root.width-toolInsets.rightEdgeBottomInset,_root.height-toolInsets.bottomEdgeRightInset, toolInsets.rightEdgeBottomInset, toolInsets.bottomEdgeRightInset)
        }
    }

    function recenterNeeded() {
        var vehiclePoint = _root.fromCoordinate(_activeVehicleCoordinate, false)
        var centerRect = _insetCenterRect()

        if(!pointInRect(vehiclePoint, centerRect)) return true

        var cornerRects = _insetCornerRects()
        if(pointInRect(vehiclePoint, cornerRects["topleft"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["topright"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["bottomleft"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["bottomright"])) return true

        return false
    }

    function updateMapToVehiclePosition() {
        if (animateLat.running || animateLong.running) return

        if (!_keepMapCenteredOnVehicle && firstVehiclePositionReceived && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            if (_keepVehicleCentered) {
                _root.center = _activeVehicleCoordinate
            } else {
                if (firstVehiclePositionReceived && recenterNeeded()) {
                    var vehiclePoint = _root.fromCoordinate(_activeVehicleCoordinate, false)
                    var centerInsetRect = _insetCenterRect()
                    var centerInsetPoint = Qt.point(centerInsetRect.x + centerInsetRect.width / 2, centerInsetRect.y + centerInsetRect.height / 2)
                    var centerOffset = Qt.point((_root.width / 2) - centerInsetPoint.x, (_root.height / 2) - centerInsetPoint.y)
                    var vehicleOffsetPoint = Qt.point(vehiclePoint.x + centerOffset.x, vehiclePoint.y + centerOffset.y)
                    var vehicleOffsetCoord = _root.toCoordinate(vehicleOffsetPoint, false)
                    animatedMapRecenter(_root.center, vehicleOffsetCoord)
                }
            }
        }
    }

    on_ActiveVehicleCoordinateChanged: {
        if (_keepMapCenteredOnVehicle && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            _root.center = _activeVehicleCoordinate
        }
    }

    Timer {
        id:         panRecenterTimer
        interval:   10000
        running:    false
        onTriggered: {
            _disableVehicleTracking = false
            updateMapToVehiclePosition()
        }
    }

    Timer {
        interval:       500
        running:        true
        repeat:         true
        onTriggered:    updateMapToVehiclePosition()
    }

    QGCMapPalette { id: mapPal; lightColors: isSatelliteMap }

    Connections {
        target:                 _missionController
        ignoreUnknownSignals:   true
        function onNewItemsFromVehicle() {
            var visualItems = _missionController.visualItems
            if (visualItems && visualItems.count !== 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
                firstVehiclePositionReceived = true
            }
        }
    }

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        _root
        usePlannedHomePosition:     false
        planMasterController:       _planMasterController
    }

    // ============================================================================
    // 区块 3：地图图层与多机数据可视化
    // ============================================================================

    ObstacleDistanceOverlayMap { id: obstacleDistance; showText: !pipMode }

    MapItemView {
        id: trajectoryItemView
        property var trajectoryColors: ["red", "green", "blue", "magenta", "darkorange", "cyan", "purple", "gold"]
        model: QGroundControl.multiVehicleManager.vehicles

        delegate: MapPolyline {
            id:          trajectoryPolyline
            line.width:  3
            z:           QGroundControl.zOrderTrajectoryLines
            visible:     !pipMode

            line.color: {
                if (object && object.id) {
                    var colorIndex = object.id % trajectoryItemView.trajectoryColors.length;
                    return trajectoryItemView.trajectoryColors[colorIndex];
                }
                return "red";
            }

            Connections {
                target: object ? object.trajectoryPoints : null
                onPointAdded:      trajectoryPolyline.addCoordinate(coordinate)
                onUpdateLastPoint: trajectoryPolyline.replaceCoordinate(trajectoryPolyline.pathLength() - 1, coordinate)
                onPointsCleared:   trajectoryPolyline.path = []
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: VehicleMapItem {
            vehicle:        object
            coordinate:     object.coordinate
            map:            _root
            size:           pipMode ? ScreenTools.defaultFontPixelHeight : ScreenTools.defaultFontPixelHeight * 3
            z:              QGroundControl.zOrderVehicles

            opacity: (_root.selectedSwarmIds.length === 0 || _root.selectedSwarmIds.indexOf(object.id) !== -1) ? 1.0 : 0.4

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: MapQuickItem {
            coordinate: object.coordinate
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            z: QGroundControl.zOrderVehicles + 1
            visible: !pipMode
            sourceItem: Item {
                property bool selected: _root.selectedSwarmIds.indexOf(object.id) !== -1
                property bool dimmed: _root.selectedSwarmIds.length > 0 && !selected
                width: selected ? 54 : 38
                height: width
                opacity: dimmed ? 0.42 : 1.0
                Rectangle {
                    anchors.centerIn: parent
                    visible: parent.selected
                    width: parent.width + 10
                    height: width
                    radius: width / 2
                    color: Qt.rgba(1.0, 0.78, 0.20, 0.10)
                    border.color: Qt.rgba(1.0, 0.78, 0.20, 0.32)
                    border.width: 1
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: parent.height
                    radius: width / 2
                    color: "transparent"
                    border.color: parent.selected ? "#FFD56A" : Qt.rgba(1.0, 1.0, 1.0, 0.45)
                    border.width: parent.selected ? 3 : 1
                }
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: ProximityRadarMapView {
            vehicle:        object
            coordinate:     object.coordinate
            map:            _root
            z:              QGroundControl.zOrderVehicles
        }
    }

    MapItemView {
        model: QGroundControl.adsbVehicleManager.adsbVehicles
        delegate: VehicleMapItem {
            coordinate:     object.coordinate
            altitude:       object.altitude
            callsign:       object.callsign
            heading:        object.heading
            alert:          object.alert
            map:            _root
            size:           pipMode ? ScreenTools.defaultFontPixelHeight : ScreenTools.defaultFontPixelHeight * 2.5
            z:              QGroundControl.zOrderVehicles
        }
    }

    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles
        PlanMapItems {
            map:                    _root
            largeMapView:           !pipMode
            planMasterController:   masterController
            vehicle:                _vehicle
            visible:                _vehicle && _activeVehicle && _vehicle.id === _activeVehicle.id
            property var _vehicle: object

            PlanMasterController {
                id: masterController
                Component.onCompleted: startStaticActiveVehicle(object)
            }
        }
    }

    MapItemView {
        model: pipMode ? undefined : _missionController.directionArrows
        delegate: MapLineArrow {
            fromCoord:      object ? object.coordinate1 : undefined
            toCoord:        object ? object.coordinate2 : undefined
            arrowPosition:  2
            z:              QGroundControl.zOrderWaypointLines
        }
    }

    CustomMapItems { map: _root; largeMapView: !pipMode }

    GeoFenceMapVisuals {
        map:                    _root
        myGeoFenceController:   _geoFenceController
        interactive:            false
        planView:               false
        homePosition:           _activeVehicle && _activeVehicle.homePosition.isValid ? _activeVehicle.homePosition :  QtPositioning.coordinate()
    }

    MapItemView {
        model: _rallyPointController.points
        delegate: MapQuickItem {
            id:             itemIndicator
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderMapItems

            sourceItem: MissionItemIndexLabel {
                id:         itemIndexLabel
                label:      qsTr("R", "rally point map item label")
            }
        }
    }

    MapItemView {
        model: _activeVehicle ? _activeVehicle.cameraTriggerPoints : 0
        delegate: CameraTriggerIndicator { coordinate: object.coordinate; z: QGroundControl.zOrderTopMost }
    }

    // ============================================================================
    // 区块 4：交互式视觉指示器 (UI 动效)
    // ============================================================================

    MapQuickItem {
        id:             gotoLocationItem
        visible:        false
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem: Item {
            width: 60
            height: 60
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: targetDot.top
                anchors.bottomMargin: 6
                width: labelText.width + 16
                height: labelText.height + 6
                color: "#B3000000"
                radius: 4
                border.color: qgcPal.buttonHighlight
                border.width: 1

                Text {
                    id: labelText
                    anchors.centerIn: parent
                    text: "目标航点"
                    color: qgcPal.buttonHighlight
                    font.bold: true
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.9
                }
            }

            Rectangle {
                id: targetDot
                anchors.centerIn: parent
                width: 14
                height: 14
                radius: 7
                color: qgcPal.buttonHighlight
                border.color: "white"
                border.width: 2
            }

            Rectangle {
                anchors.centerIn: targetDot
                width: 30
                height: 30
                radius: 15
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2

                SequentialAnimation on scale {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.8; to: 1.5; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.5; to: 0.8; duration: 1200; easing.type: Easing.InOutQuad }
                }
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.8; to: 0.0; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.0; to: 0.8; duration: 1200; easing.type: Easing.InOutQuad }
                }
            }
        }

        property bool inGotoFlightMode: _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.gotoFlightMode : false

        onInGotoFlightModeChanged: {
            if (!inGotoFlightMode && gotoLocationItem.visible) {
                gotoLocationItem.visible = false
            }
        }

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                if (!activeVehicle) gotoLocationItem.visible = false
            }
        }

        function show(coord) {
            gotoLocationItem.coordinate = coord
            gotoLocationItem.visible = true
        }

        function hide() { gotoLocationItem.visible = false }
        function actionConfirmed() {}
        function actionCancelled() { hide() }
    }

    QGCMapCircleVisuals {
        id:             orbitMapCircle
        mapControl:     parent
        mapCircle:      _mapCircle
        visible:        false

        property alias center:              _mapCircle.center
        property alias clockwiseRotation:   _mapCircle.clockwiseRotation
        readonly property real defaultRadius: 30

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                if (!activeVehicle) orbitMapCircle.visible = false
            }
        }

        function show(coord) {
            _mapCircle.radius.rawValue = defaultRadius
            orbitMapCircle.center = coord
            orbitMapCircle.visible = true
        }

        function hide() { orbitMapCircle.visible = false }
        function actionConfirmed() { hide() }
        function actionCancelled() { hide() }
        function radius() { return _mapCircle.radius.rawValue }

        Component.onCompleted: globals.guidedControllerFlyView.orbitMapCircle = orbitMapCircle

        QGCMapCircle {
            id:                 _mapCircle
            interactive:        true
            radius.rawValue:    30
            showRotation:       true
            clockwiseRotation:  true
        }
    }

    MapQuickItem {
        id:             roiLocationItem
        visible:        _activeVehicle && _activeVehicle.isROIEnabled
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("ROI here", "Make this a Region Of Interest")
        }

        function show(coord) { roiLocationItem.coordinate = coord }
        function hide() {}
        function actionConfirmed() {}
        function actionCancelled() {}
    }

    QGCMapCircleVisuals {
        id:              orbitTelemetryCircle
        mapControl:     parent
        mapCircle:      _activeVehicle ? _activeVehicle.orbitMapCircle : null
        visible:        _activeVehicle ? _activeVehicle.orbitActive : false
    }

    MapQuickItem {
        id:             orbitCenterIndicator
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        coordinate:     _activeVehicle ? _activeVehicle.orbitMapCircle.center : QtPositioning.coordinate()
        visible:        orbitTelemetryCircle.visible

        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("Orbit", "Orbit waypoint")
        }
    }

    // ============================================================================
    // 区块 4.1：Shift 临时任务的草稿轨迹与编号节点
    // ============================================================================

    // 绘制排队未下发的指点航线折线轨迹
    MapPolyline {
        id:          shiftQueuePolyline
        line.width:  3
        line.color:  _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
        z:           QGroundControl.zOrderTrajectoryLines + 1
        visible:     !pipMode && (_root.shiftQueuedCoords.length > 0 || _root.shiftCommittedCoords.length > 0)
        path:        _root.shiftQueuedCoords.length > 0 ? _root.shiftQueuedCoords : _root.shiftCommittedCoords
    }

    // 动态生成每个排队位置的节点序号标签
    MapItemView {
        model: _root.shiftQueuedCoords.length > 0 ? _root.shiftQueuedCoords : _root.shiftCommittedCoords
        delegate: MapQuickItem {
            coordinate: modelData
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            z: QGroundControl.zOrderTopMost
            sourceItem: Rectangle {
                width: 18
                height: 18
                radius: 9
                color: _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
                border.color: "white"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: index + 1 // 显示实时点击序号 (从 1 开始)
                    color: "white"
                    font.pixelSize: 10
                    font.bold: true
                }
            }
        }
    }

    // ============================================================================
    // 区块 5：全局鼠标交互与菜单 (支持框选和右键平移)
    // ============================================================================

    // 根节点持有键盘焦点，确保 Shift 释放能够结束同一批草稿。
    focus: true
    Keys.onReleased: {
        if (event.key === Qt.Key_Shift) {
            // 松开 Shift 只进入确认流程，确认前不上传临时任务。
            if (_root.shiftQueuedCoords.length > 0) {
                _root.requestBatchQueuedFly()
            }
        }
    }

    // 框选视觉矩形（半透明深蓝底色 + 暗金边框）

    Rectangle {
        id: marqueeBox
        visible: false
        radius: 8
        color: "transparent"
        border.color: _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
        border.width: 2
        opacity: 0.0
        z: QGroundControl.zOrderTopMost + 100
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.24) }
            GradientStop { position: 1.0; color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.06) }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            radius: Math.max(2, parent.radius - 3)
            color: "transparent"
            border.color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.34)
            border.width: 1
        }

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    // 全局拖拽与点击事件分流层
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: false
        preventStealing: true
        onWheel: wheel.accepted = false

        property var startPoint: Qt.point(0, 0)
        property var lastPanPoint: Qt.point(0, 0)
        property bool isDrawingBox: false
        property bool isDragging: false

        onCanceled: {
            isDrawingBox = false
            marqueeBox.visible = false
            isDragging = false
        }

        // 左键点击弹出的快捷操作菜单

        Popup {
            id: clickMenu
            modal: true
            property var coord

            function setCoordinates(mouseX, mouseY) {
                var newX = mouseX
                var newY = mouseY

                if (newX + clickMenu.width > _root.width) {
                    newX = _root.width - clickMenu.width
                }
                if (newY + clickMenu.height > _root.height) {
                    newY = _root.height - clickMenu.height
                }
                x = newX
                y = newY
            }

            background: Rectangle {
                radius: ScreenTools.defaultFontPixelHeight * 0.5
                color: qgcPal.window
                border.color: qgcPal.text
            }

            ColumnLayout {
                id: mainLayout
                spacing: ScreenTools.defaultFontPixelWidth / 2

                QGCButton {
                    Layout.fillWidth: true
                    text: "Go to location"
                    visible: globals.guidedControllerFlyView.showGotoLocation
                    onClicked: {
                        if (clickMenu.opened) clickMenu.close()
                        gotoLocationItem.show(clickMenu.coord)
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionGoto, clickMenu.coord, gotoLocationItem)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "Orbit at location"
                    visible: globals.guidedControllerFlyView.showOrbit
                    onClicked: {
                        if (clickMenu.opened) clickMenu.close()
                        orbitMapCircle.show(clickMenu.coord)
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionOrbit, clickMenu.coord, orbitMapCircle)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "ROI at location"
                    visible: globals.guidedControllerFlyView.showROI
                    onClicked: {
                        if (clickMenu.opened) clickMenu.close()
                        roiLocationItem.show(clickMenu.coord)
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionROI, clickMenu.coord, roiLocationItem)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "Set home here"
                    visible: globals.guidedControllerFlyView.showSetHome
                    onClicked: {
                        if (clickMenu.opened) clickMenu.close()
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionSetHome, clickMenu.coord)
                    }
                }
            }
        }

        onPressed: {
            isDragging = false
            _root.forceActiveFocus() // 地图重新取得焦点，避免 Shift 释放事件遗失。

            if (mouse.button === Qt.LeftButton) {
                startPoint = Qt.point(mouse.x, mouse.y)
                marqueeBox.x = mouse.x
                marqueeBox.y = mouse.y
                marqueeBox.width = 0
                marqueeBox.height = 0
                marqueeBox.opacity = 0.92
                isDrawingBox = true
                _disableVehicleTracking = true
            } else if (mouse.button === Qt.RightButton) {
                lastPanPoint = Qt.point(mouse.x, mouse.y)
                _disableVehicleTracking = true
            }
        }

        onPositionChanged: {
            if (Math.abs(mouse.x - startPoint.x) > 3 || Math.abs(mouse.y - startPoint.y) > 3) {
                isDragging = true
            }

            // 窗口切换可能丢失 Shift 释放事件；发现残留草稿时补进确认流程。
            if (_root.shiftQueuedCoords.length > 0 && !(mouse.modifiers & Qt.ShiftModifier)) {
                _root.requestBatchQueuedFly()
            }

            if (isDrawingBox && (mouse.buttons & Qt.LeftButton)) {
                marqueeBox.visible = true
                marqueeBox.x = Math.min(startPoint.x, mouse.x)
                marqueeBox.y = Math.min(startPoint.y, mouse.y)
                marqueeBox.width = Math.abs(mouse.x - startPoint.x)
                marqueeBox.height = Math.abs(mouse.y - startPoint.y)
            } else if (mouse.buttons & Qt.RightButton) {
                isDragging = true
                var deltaX = mouse.x - lastPanPoint.x
                var deltaY = mouse.y - lastPanPoint.y

                var centerPix = _root.fromCoordinate(_root.center, false)
                var newCenterPix = Qt.point(centerPix.x - deltaX, centerPix.y - deltaY)

                _root.center = _root.toCoordinate(newCenterPix, false)
                lastPanPoint = Qt.point(mouse.x, mouse.y)
            }
        }

        onReleased: {
            panRecenterTimer.restart()

            if (mouse.button === Qt.LeftButton && isDrawingBox) {
                isDrawingBox = false
                if (isDragging) {
                    _selectVehiclesInMarquee()
                }
                marqueeBox.opacity = 0.0
                var delayTimer = Qt.createQmlObject('import QtQuick 2.0; Timer { interval: 150; running: true; repeat: false; onTriggered: { marqueeBox.visible = false } }', parent)
            }
        }

        onClicked: {
            if (isDragging) return;
            var clickCoord = _root.toCoordinate(Qt.point(mouse.x, mouse.y), false)

            if (mouse.button === Qt.RightButton) {
                // Shift 建立可预览的临时任务草稿；普通右键保持即时指点语义。
                if (mouse.modifiers & Qt.ShiftModifier) {
                    // 草稿只记录坐标，不立即下发飞控。
                    if (_root.shiftQueuedCoords.length === 0 && !_root._freezeShiftTargets()) return
                    var tempCoords = _root.shiftQueuedCoords.slice(0)
                    tempCoords.push(clickCoord)
                    _root.shiftCommittedCoords = []
                    _root.shiftQueuedCoords = tempCoords // 重新赋值以触发 QML 绑定刷新。

                    // 用排队动效区分“草稿”与已经下发的任务。
                    showMobaClickAnimation(clickCoord, "#D8B56A")
                } else {
                    // 普通右键执行单点指令，并清理不属于本次操作的残留草稿。
                    if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
                    executeRightClickFly(clickCoord)
                }
            }
            else if (mouse.button === Qt.LeftButton) {
                if (!globals.guidedControllerFlyView.guidedUIVisible && (globals.guidedControllerFlyView.showGotoLocation || globals.guidedControllerFlyView.showOrbit || globals.guidedControllerFlyView.showROI || globals.guidedControllerFlyView.showSetHome)) {
                    orbitMapCircle.hide()
                    gotoLocationItem.hide()
                    clickMenu.coord = clickCoord
                    clickMenu.setCoordinates(mouse.x, mouse.y)
                    clickMenu.open()
                }
            }
        }
    }

    // 地理坐标转换与无人机筛选逻辑
    function _selectVehiclesInMarquee() {
        if (marqueeBox.width < 5 || marqueeBox.height < 5) return;
        var topLeftGeo = _root.toCoordinate(Qt.point(marqueeBox.x, marqueeBox.y), false)
        var bottomRightGeo = _root.toCoordinate(Qt.point(marqueeBox.x + marqueeBox.width, marqueeBox.y + marqueeBox.height), false)

        var maxLat = Math.max(topLeftGeo.latitude, bottomRightGeo.latitude)
        var minLat = Math.min(topLeftGeo.latitude, bottomRightGeo.latitude)
        var maxLon = Math.max(topLeftGeo.longitude, bottomRightGeo.longitude)
        var minLon = Math.min(topLeftGeo.longitude, bottomRightGeo.longitude)

        var vehicleList = QGroundControl.multiVehicleManager.vehicles
        var selectedVehicleIds = []

        for (var i = 0; i < vehicleList.count; i++) {
            var vehicle = vehicleList.get(i)
            var coord = vehicle.coordinate

            if (coord.latitude >= minLat && coord.latitude <= maxLat &&
                coord.longitude >= minLon && coord.longitude <= maxLon) {
                selectedVehicleIds.push(vehicle.id)
            }
        }

        if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
        console.log("框选成功！选中的无人机 ID 集合: ", JSON.stringify(selectedVehicleIds))
        selectedSwarmIds = selectedVehicleIds

        if (selectedVehicleIds.length > 0) {
            var firstSelectedVehicle = QGroundControl.multiVehicleManager.getVehicleById(selectedVehicleIds[0])
            if (firstSelectedVehicle) {
                suppressExclusiveSelectionSync = true
                QGroundControl.multiVehicleManager.activeVehicle = firstSelectedVehicle
                suppressExclusiveSelectionSync = false
                _showCurrentTaskRoute(firstSelectedVehicle.id)
            }
        }
    }

    MapScale {
        id:                 mapScale
        anchors.margins:    _toolsMargin
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _toolsMargin
        anchors.bottomMargin: _toolsMargin
        mapControl:         _root
        buttonsOnLeft:      false
        visible:            !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && mapControl.pipState.state === mapControl.pipState.windowState
        property real centerInset: visible ? parent.height - y : 0
    }

    // ============================================================================
    // 区块 6：自定义 MOBA 风格右键指点飞行与编队逻辑
    // ============================================================================

    // Shift 草稿在确认时冻结目标 ID、参考位置和坐标，三者必须成组传入执行层。
    function _showSwarmCommandResult(result, actionTitle) {
        if (!result) return

        if (!result.ok && result.message && result.message.length > 0) {
            mainWindow.showMessageDialog(actionTitle, result.message)
            return
        }

        if (result.skippedIds && result.skippedIds.length > 0) {
            mainWindow.showMessageDialog(
                qsTr("部分指令已跳过"),
                qsTr("以下无人机未满足任务下发条件（解锁、高度、定位或任务上传空闲状态）：\n无人机 ") + JSON.stringify(result.skippedIds)
            )
        }
    }

    function requestBatchQueuedFly() {
        if (_root.shiftQueuedCoords.length === 0 || _root.shiftMissionConfirmPending) return

        _root.shiftReplacementRequired = swarmController.hasActiveTemporaryMission(_root.shiftFrozenVehicleIds)
        _root.shiftMissionConfirmPending = true
        var guidedController = globals.guidedControllerFlyView
        if (!guidedController) {
            _root._cancelShiftDraft()
            return
        }
        guidedController.confirmAction(
            guidedController.actionQueuedMission,
            {
                vehicleIds: _root.shiftFrozenVehicleIds.slice(0),
                waypointCount: _root.shiftQueuedCoords.length,
                replacementRequired: _root.shiftReplacementRequired
            },
            shiftMissionConfirmationAdapter)
    }

    function _storeTaskRoutes(ids, references, coordinates, dispatchedIds) {
        if (!ids || ids.length === 0 || ids.length !== references.length) return

        var centerLat = 0
        var centerLon = 0
        for (var i = 0; i < references.length; i++) {
            centerLat += references[i].latitude
            centerLon += references[i].longitude
        }
        var centroid = QtPositioning.coordinate(centerLat / references.length, centerLon / references.length)
        var routes = _root._cloneRouteMap()

        for (var vehicleIndex = 0; vehicleIndex < ids.length; vehicleIndex++) {
            if (dispatchedIds && dispatchedIds.indexOf(ids[vehicleIndex]) === -1) continue
            var route = []
            for (var pointIndex = 0; pointIndex < coordinates.length; pointIndex++) {
                var distance = centroid.distanceTo(coordinates[pointIndex])
                var azimuth = centroid.azimuthTo(coordinates[pointIndex])
                var vehiclePoint = references[vehicleIndex].atDistanceAndAzimuth(distance, azimuth)
                vehiclePoint.altitude = coordinates[pointIndex].altitude
                route.push(vehiclePoint)
            }
            routes[String(ids[vehicleIndex])] = route
        }
        _root.activeTaskRoutes = routes
        _root._showCurrentTaskRoute(_root._activeVehicle ? _root._activeVehicle.id : -1)
    }

    function _removeTaskRoutes(ids) {
        var routes = _root._cloneRouteMap()
        for (var i = 0; ids && i < ids.length; i++) delete routes[String(ids[i])]
        _root.activeTaskRoutes = routes
        _root._showCurrentTaskRoute(_root._activeVehicle ? _root._activeVehicle.id : -1)
    }

    function executeBatchQueuedFly() {
        _root.shiftMissionConfirmPending = false
        var coordsArray = _root.shiftQueuedCoords.slice(0)
        var frozenIds = _root.shiftFrozenVehicleIds.slice(0)
        var frozenReferences = _root.shiftFrozenReferences.slice(0)
        var replaceExisting = _root.shiftReplacementRequired
        _root.shiftQueuedCoords = []

        if (coordsArray.length === 0) return

        console.log("Uploading temporary mission route. queued points: ", coordsArray.length)

        if (typeof orbitMapCircle !== "undefined") orbitMapCircle.hide()
        if (typeof roiLocationItem !== "undefined") roiLocationItem.hide()

        gotoLocationItem.show(coordsArray[coordsArray.length - 1])

        var result = swarmController.executeQueuedGoto(frozenIds, coordsArray, frozenReferences, replaceExisting)
        if (result && result.ok) {
            _storeTaskRoutes(frozenIds, frozenReferences, coordsArray, result.dispatchedIds)
        }
        _root.shiftFrozenVehicleIds = []
        _root.shiftFrozenReferences = []
        _root.shiftReplacementRequired = false
        _showSwarmCommandResult(result, qsTr("实时航点队列"))
    }
    function executeRightClickFly(targetCoordinate) {
        if (typeof orbitMapCircle !== "undefined") orbitMapCircle.hide()
        if (typeof roiLocationItem !== "undefined") roiLocationItem.hide()

        showMobaClickAnimation(targetCoordinate)
        gotoLocationItem.show(targetCoordinate)

        var result = swarmController.executeGoto(selectedSwarmIds, targetCoordinate)
        if (result && result.ok) _removeTaskRoutes(result.dispatchedIds)
        _showSwarmCommandResult(result, "指点飞行")
    }
    function showMobaClickAnimation(coord, customColorStr) {
        var finalColor = customColorStr ? customColorStr : qgcPal.buttonHighlight
        mobaClickIndicator.applyDynamicColor(finalColor)

        mobaClickIndicator.coordinate = coord
        mobaClickIndicator.visible = true
        mobaClickIndicator.resetAndStart()
    }

    MapQuickItem {
        id: mobaClickIndicator
        coordinate: QtPositioning.coordinate(0, 0)
        visible: false
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        z: QGroundControl.zOrderTopMost

        function resetAndStart() {
            outerRing.scale = 0.25
            outerRing.opacity = 0.72
            middleRing.scale = 0.18
            middleRing.opacity = 0.42
            waypointHalo.opacity = 0.45
            clickAnimation.restart()
        }

        function applyDynamicColor(colorValue) {
            waypointDot.color = colorValue
            waypointStem.color = colorValue
            waypointHalo.border.color = colorValue
            middleRing.border.color = colorValue
            outerRing.border.color = colorValue
        }

        sourceItem: Item {
            width: 88
            height: 88

            Rectangle {
                id: waypointHalo
                anchors.centerIn: parent
                width: 24
                height: 24
                radius: 12
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2
                opacity: 0.45
            }

            Rectangle {
                id: waypointStem
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: waypointDot.bottom
                width: 2
                height: 12
                radius: 1
                color: qgcPal.buttonHighlight
                opacity: 0.9
            }

            Rectangle {
                id: waypointDot
                anchors.centerIn: parent
                width: 9
                height: 9
                radius: 5
                color: qgcPal.buttonHighlight
                border.color: Qt.rgba(1, 1, 1, 0.72)
                border.width: 1
            }

            Rectangle {
                id: middleRing
                anchors.centerIn: parent
                width: 52
                height: 52
                radius: 26
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2
                opacity: 0
            }

            Rectangle {
                id: outerRing
                anchors.centerIn: parent
                width: 68
                height: 68
                radius: 34
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 3
                opacity: 0
            }

            SequentialAnimation {
                id: clickAnimation
                ParallelAnimation {
                    NumberAnimation { target: middleRing; property: "scale"; from: 0.18; to: 1.25; duration: 520; easing.type: Easing.OutCubic }
                    NumberAnimation { target: middleRing; property: "opacity"; from: 0.42; to: 0.0; duration: 520; easing.type: Easing.OutQuad }
                    NumberAnimation { target: outerRing; property: "scale"; from: 0.25; to: 1.65; duration: 680; easing.type: Easing.OutCubic }
                    NumberAnimation { target: outerRing; property: "opacity"; from: 0.72; to: 0.0; duration: 680; easing.type: Easing.OutQuad }
                    NumberAnimation { target: waypointHalo; property: "opacity"; from: 0.45; to: 0.12; duration: 680; easing.type: Easing.OutQuad }
                }
                onStopped: mobaClickIndicator.visible = false
            }
        }
    }
    CommandCenterOverlay {
        id: commandCenterOverlay
        anchors.fill: parent
        z: QGroundControl.zOrderTopMost + 200
        visible: !pipMode
        selectedIds: _root.selectedSwarmIds
        vehicles: QGroundControl.multiVehicleManager.vehicles
        activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
        toolInsets: _root.toolInsets

        onVehicleSelectionRequested: _root.setVehicleSelection(vehicleId, selected)
        onVehicleFocusRequested: _root.focusVehicle(vehicleId)
        onClearSelectionRequested: _root.clearVehicleSelection()
    }
}

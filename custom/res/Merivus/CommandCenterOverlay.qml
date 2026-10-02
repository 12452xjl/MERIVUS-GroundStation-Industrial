import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11
import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: root

    // 本覆盖层是人工控制入口：只把操作者当前选择交给 Guided/Swarm 确认流程，
    // 不消费 AI proposal，也不把界面显示状态当作飞控 ACK。
    GpsStatus { id: gpsStatus }

    property var selectedIds: []
    property var vehicles: QGroundControl.multiVehicleManager.vehicles
    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var toolInsets
    property alias videoDockTarget: videoViewport

    signal vehicleSelectionRequested(int vehicleId, bool selected)
    signal vehicleFocusRequested(int vehicleId)
    signal clearSelectionRequested()

    readonly property real margin: Math.max(8, ScreenTools.defaultFontPixelWidth * 0.75)
    readonly property real topInset: toolInsets ? Math.max(margin, toolInsets.topEdgeLeftInset + margin) : margin
    readonly property real panelRadius: 8
    readonly property real collapsedPanelWidth: 52
    readonly property real resizeHandleThickness: 10
    readonly property real sidePanelMinHeight: 420
    readonly property real panelBottomLiftMin: 78
    readonly property real leftPanelMinWidth: 330
    readonly property real rightPanelMinWidth: 300
    readonly property real leftPanelMaxWidth: Math.max(leftPanelMinWidth, Math.min(480, width * 0.40))
    readonly property real rightPanelMaxWidth: Math.max(rightPanelMinWidth, Math.min(480, width * 0.40))
    readonly property color panelColor: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.94)
    readonly property color raisedColor: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.96)
    readonly property color panelLine: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.20)
    readonly property color mutedLine: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.11)
    readonly property color accent: qgcPal.buttonHighlight
    readonly property color nominal: qgcPal.colorGreen
    readonly property color muted: qgcPal.colorGrey
    readonly property var focusVehicle: resolveFocusVehicle()
    readonly property var rightVehicle: resolveRightVehicle()
    readonly property var guidedController: globals.guidedControllerFlyView
    readonly property var fleetHealth: guidedController && guidedController.fleetHealthSummary
                                      ? guidedController.fleetHealthSummary
                                      : ({ state: "unknown", score: 0, onlineVehicleCount: 0, vehicleCount: 0 })
    readonly property var missionSummary: guidedController && guidedController.missionOrchestrator
                                          ? guidedController.missionOrchestrator.fleetSummary
                                          : ({ taskCount: 0, runningCount: 0 })
    readonly property var faultRecommendations: guidedController && guidedController.faultToleranceManager
                                                ? guidedController.faultToleranceManager.recommendations
                                                : []
    readonly property var fleetIntentTask: guidedController && guidedController.intentTask
                                           ? guidedController.intentTask : null
    readonly property var fleetRiskRadar: guidedController && guidedController.riskRadar
                                          ? guidedController.riskRadar : null
    readonly property var fleetEventBlackBox: guidedController && guidedController.eventBlackBox
                                              ? guidedController.eventBlackBox : null
    readonly property var fleetHandoffManager: guidedController && guidedController.missionHandoffManager
                                               ? guidedController.missionHandoffManager : null

    property real leftPanelTopExtra: 3
    property real rightPanelTopExtra: 3
    property real leftPanelBottomLift: 78
    property real rightPanelBottomLift: 78
    property real leftPanelWidth: Math.min(420, width * 0.28)
    property real rightPanelWidth: Math.min(390, width * 0.27)
    property bool leftExpanded: true
    property bool rightExpanded: width >= 1280
    property bool leftPanelResizing: false
    property bool rightPanelResizing: false
    property int rightPanelVehicleId: -1
    property real altitudeCommandMeters: 10
    property real speedCommandMetersSecond: 5
    property real takeoffSpeedCommandMetersSecond: 1.5
    property date now: new Date()

    visible: width > 900 && height > 560

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    FtcStatusPalette { id: ftcStatusPalette }
    Timer { interval: 1000; running: root.visible; repeat: true; onTriggered: root.now = new Date() }

    function tr(text) { return qsTr(text) }
    function clamp(value, minValue, maxValue) { return Math.max(minValue, Math.min(maxValue, value)) }
    // Preserve the previous 96-DPI sizing while following QGC's point-size convention.
    function fontPointSize(pixelSize) { return pixelSize * 0.75 }

    function vehicleById(vehicleId) {
        if (!vehicles) return null
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i)
            if (vehicle && vehicle.id === vehicleId) return vehicle
        }
        return null
    }

    function resolveFocusVehicle() {
        if (selectedIds && selectedIds.length > 0) {
            var selectedVehicle = vehicleById(selectedIds[0])
            if (selectedVehicle) return selectedVehicle
        }
        return activeVehicle
    }

    function resolveRightVehicle() {
        if (selectedIds && selectedIds.length > 0) {
            if (rightPanelVehicleId > 0 && selectedIds.indexOf(rightPanelVehicleId) !== -1) {
                var requestedVehicle = vehicleById(rightPanelVehicleId)
                if (requestedVehicle) return requestedVehicle
            }
            var firstSelectedVehicle = vehicleById(selectedIds[0])
            if (firstSelectedVehicle) return firstSelectedVehicle
        }
        return focusVehicle
    }

    function rightVehicleSelected(vehicleId) {
        return rightVehicle && rightVehicle.id === vehicleId
    }

    function selectedCount() { return selectedIds ? selectedIds.length : 0 }

    function fleetHealthText() {
        if (!fleetHealth || fleetHealth.state === "unknown") return tr("待收集")
        if (fleetHealth.state === "critical") return tr("严重")
        if (fleetHealth.state === "degraded") return tr("降级")
        return tr("健康")
    }

    function fleetHealthColor() {
        if (!fleetHealth || fleetHealth.state === "unknown") return root.muted
        if (fleetHealth.state === "critical") return qgcPal.colorRed
        if (fleetHealth.state === "degraded") return qgcPal.colorOrange
        return root.nominal
    }

    function selectedSummary() {
        if (selectedCount() === 0) {
            return activeVehicle ? tr("\u5730\u56fe\u76ee\u6807\uff1a\u5f53\u524d\u7126\u70b9 UAV-%1").arg(activeVehicle.id) : tr("\u5730\u56fe\u76ee\u6807\uff1a\u672a\u9009\u62e9")
        }
        if (selectedCount() === 1) return tr("\u5730\u56fe\u76ee\u6807\uff1aUAV-%1").arg(selectedIds[0])
        return tr("\u5730\u56fe\u76ee\u6807\uff1a%1 \u67b6 - ID %2").arg(selectedCount()).arg(selectedIds.join(", "))
    }

    function isSelected(vehicleId) { return selectedIds && selectedIds.indexOf(vehicleId) !== -1 }
    function factHasValue(fact) { return fact && !isNaN(Number(fact.rawValue)) }
    function numberText(fact, decimals, suffix) { return factHasValue(fact) ? Number(fact.rawValue).toFixed(decimals) + (suffix ? " " + suffix : "") : "--" }

    function metricFactFor(vehicle, kind) {
        if (!vehicle) return null
        if (kind === "altitude") return vehicle.altitudeRelative
        if (kind === "speed") return (vehicle.fixedWing || vehicle.vtolInFwdFlight) ? vehicle.airSpeed : vehicle.groundSpeed
        if (kind === "climb") return vehicle.climbRate
        if (kind === "heading") return vehicle.heading
        if (kind === "distance") return vehicle.flightDistance
        if (kind === "battery") return vehicle.batteries && vehicle.batteries.count > 0 ? vehicle.batteries.get(0).percentRemaining : null
        if (kind === "gps") return vehicle.gps ? vehicle.gps.count : null
        return null
    }

    function metricFact(kind) { return metricFactFor(focusVehicle, kind) }

    function metricTextFor(vehicle, kind, decimals, suffix) {
        if (kind === "time") return vehicle ? elapsedText(vehicle.flightTime) : "--"
        if (kind === "gps" && vehicle && vehicle.gps) {
            var fixType = Number(vehicle.gps.lock.rawValue)
            if (isNaN(fixType) || fixType < 2) return tr("无有效定位")
        }
        return numberText(metricFactFor(vehicle, kind), decimals, suffix)
    }

    function metricText(kind, decimals, suffix) { return metricTextFor(focusVehicle, kind, decimals, suffix) }

    function showGpsDetails(vehicle) {
        mainWindow.showMessageDialog(tr("GPS / RTK 状态"), gpsStatus.details(vehicle, QGroundControl.gpsRtk))
    }

function elapsedText(fact) {
        if (!factHasValue(fact)) return "--"
        var seconds = Number(fact.rawValue)
        if (seconds < 0) return "--"
        var hours = Math.floor(seconds / 3600)
        var minutes = Math.floor((seconds % 3600) / 60)
        var secs = Math.floor(seconds % 60)
        return (hours < 10 ? "0" : "") + hours + ":" + (minutes < 10 ? "0" : "") + minutes + ":" + (secs < 10 ? "0" : "") + secs
    }

    function batteryPercent(vehicle) {
        if (!vehicle || !vehicle.batteries || vehicle.batteries.count === 0) return "--"
        return numberText(vehicle.batteries.get(0).percentRemaining, 0, "%")
    }

    function temperatureText(vehicle) {
        if (!vehicle) return "--"
        if (vehicle.hygrometer && factHasValue(vehicle.hygrometer.hygroTemp)) {
            return numberText(vehicle.hygrometer.hygroTemp, 1, "°C")
        }
        return numberText(vehicle.temperature ? vehicle.temperature.temperature1 : null, 1, "°C")
    }

    function humidityText(vehicle) {
        return vehicle && vehicle.hygrometer ? numberText(vehicle.hygrometer.hygroHumi, 1, "%") : "--"
    }

    function windSpeedText(vehicle) {
        return vehicle && vehicle.wind ? numberText(vehicle.wind.speed, 1, "m/s") : "--"
    }

    function windDirectionText(vehicle) {
        if (!vehicle || !vehicle.wind || !factHasValue(vehicle.wind.direction)) return "--"
        var degrees = Number(vehicle.wind.direction.rawValue)
        var names = [tr("北"), tr("东北"), tr("东"), tr("东南"), tr("南"), tr("西南"), tr("西"), tr("西北")]
        var index = Math.round(((degrees % 360) + 360) % 360 / 45) % 8
        return names[index] + " " + degrees.toFixed(0) + "°"
    }

    function attitudeTextFor(vehicle, kind) {
        if (!vehicle || !vehicle[kind]) return "--"
        return numberText(vehicle[kind], 0, "°")
    }

    function attitudeText(kind) { return attitudeTextFor(focusVehicle, kind) }

    function vibrationAxisTextFor(vehicle, axis) {
        return vehicle && vehicle.vibration ? root.numberText(vehicle.vibration[axis], 2, "") : "--"
    }

    function vibrationAxisText(axis) { return vibrationAxisTextFor(root.focusVehicle, axis) }

function escFact(vehicle, prefix, motorIndex) {
        if (!vehicle || !vehicle.escStatus) return null
        var suffixes = ["First", "Second", "Third", "Fourth"]
        if (motorIndex < 0 || motorIndex >= suffixes.length) return null
        return vehicle.escStatus[prefix + suffixes[motorIndex]]
    }

    function escNumber(vehicle, prefix, motorIndex, decimals, suffix) {
        return escHasData(vehicle, motorIndex) ? numberText(escFact(vehicle, prefix, motorIndex), decimals, suffix) : "--"
    }

    function escHasData(vehicle, motorIndex) {
        return !!(vehicle && vehicle.escStatus && vehicle.escStatus.received)
    }

    function escMotorOnline(vehicle, motorIndex) {
        if (!escHasData(vehicle, motorIndex)) return false
        if (!vehicle.escStatus.infoReceived) return true
        return (Number(vehicle.escStatus.onlineFlags.rawValue) & (1 << motorIndex)) !== 0
    }

    function escConnectionText(vehicle) {
        if (!vehicle || !vehicle.escStatus || !vehicle.escStatus.infoReceived) return tr("未知")
        var type = Number(vehicle.escStatus.connectionType.rawValue)
        return type === 1 ? tr("串行遥测")
             : type === 4 ? tr("CAN")
             : type === 5 ? tr("DShot 独立遥测")
             : type === 0 ? tr("PWM/PPM")
             : tr("类型 %1").arg(type)
    }

    function escStatusText(vehicle) {
        if (!vehicle) return tr("无数据源")
        if (!vehicle.escStatus || !vehicle.escStatus.received) return tr("未收到遥测")
        return tr("遥测在线")
    }

    function escTipText(motorIndex) {
        var label = tr("电机 M%1").arg(motorIndex + 1)
        if (!focusVehicle) return label + "\n" + tr("等待飞行器接入后显示 ESC 遥测。")
        if (!escHasData(focusVehicle, motorIndex)) return label + "\n" +
                tr("地面站已请求 ESC_STATUS/ESC_INFO，但飞控没有数据可发。DShot 电调请确认遥测线已接入飞控串口并配置 DSHOT_TEL_CFG；CAN 电调请确认 CAN 节点在线。")
        return label + "\n" +
               tr("状态：%1").arg(escMotorOnline(focusVehicle, motorIndex) ? tr("在线") : tr("离线")) + "\n" +
               tr("接口：%1").arg(escConnectionText(focusVehicle)) + "\n" +
               tr("转速：%1").arg(escNumber(focusVehicle, "rpm", motorIndex, 0, "rpm")) + "\n" +
               tr("电流：%1").arg(escNumber(focusVehicle, "current", motorIndex, 1, "A")) + "\n" +
               tr("电压：%1").arg(escNumber(focusVehicle, "voltage", motorIndex, 1, "V")) + "\n" +
               tr("电调温度：%1").arg(escNumber(focusVehicle, "temperature", motorIndex, 1, "°C"))
    }

    function ftcMotorData(vehicle, motorIndex) {
        if (!vehicle || !vehicle.ftcStatus || !vehicle.ftcStatus.motorAvailable || vehicle.ftcStatus.motorStale) return null
        if (motorIndex < 0 || motorIndex >= vehicle.ftcStatus.motorCount) return null
        return vehicle.ftcStatus.motors.get(motorIndex)
    }

    function ftcPercentText(value) {
        return value !== undefined && value >= 0 ? Number(value).toFixed(0) + "%" : tr("N/A")
    }

    function motorTipText(motorIndex) {
        var text = escTipText(motorIndex)
        var motor = ftcMotorData(focusVehicle, motorIndex)
        if (!motor) return text + "\n" + tr("FTC 健康与效能：N/A")
        return text + "\n" +
               tr("FTC 健康：%1（不是剩余寿命）").arg(ftcPercentText(motor.health)) + "\n" +
               tr("FTC 效能：%1").arg(ftcPercentText(motor.effectiveness)) + "\n" +
               tr("故障概率：%1，置信度：%2").arg(ftcPercentText(motor.faultProbability)).arg(ftcPercentText(motor.confidence)) + "\n" +
               tr("不确定度 σ：%1，估计年龄：%2 s").arg(motor.uncertainty >= 0 ? Number(motor.uncertainty).toFixed(3) : "N/A")
                   .arg(motor.estimateAge >= 0 ? Number(motor.estimateAge).toFixed(2) : "N/A") + "\n" +
               tr("状态：%1，分类：%2").arg(motor.dataStateText).arg(motor.faultTypeText)
    }

    function linkStateText(vehicle) {
        if (!vehicle) return tr("\u672a\u8fde\u63a5")
        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.communicationLost) return tr("\u94fe\u8def\u5f02\u5e38")
        return tr("\u5df2\u8fde\u63a5")
    }

    function runGuidedAction(actionId, actionData) {
        if (!guidedController || actionId < 0) return
        if (actionId === guidedController.actionTakeoff) {
            guidedController.defaultTakeoffAltitudeMeters = altitudeCommandMeters
        }
        guidedController.closeAll()
        guidedController.confirmAction(actionId, actionData)
    }

    // 确认框打开后仍可能继续选择车辆，因此执行数据必须使用当时的不可变副本。
    function selectedIdsSnapshot() { return selectedIds ? selectedIds.slice(0) : [] }

    function hasValidFormationSelection() {
        if (!selectedIds || selectedIds.length < 1 || selectedIds.length > 6) return false
        var sorted = selectedIdsSnapshot().sort(function(a, b) { return Number(a) - Number(b) })
        if (Number(sorted[0]) !== 1) return false
        if (sorted.length === 6 && sorted.join(",") !== "1,2,3,4,5,6") return false
        return !guidedController.formationPlanner || guidedController.formationPlanner.minimumSpacingValid()
    }

    function showFloatingToolTip(sourceItem, text, align) {
        if (!sourceItem || !text) return
        var anchor = sourceItem.mapToItem(root, 0, 0)
        floatingToolTip.text = text
        floatingToolTip.align = align ? align : "right"
        floatingToolTip.sourceX = anchor.x
        floatingToolTip.sourceY = anchor.y
        floatingToolTip.sourceWidth = sourceItem.width
        floatingToolTip.sourceHeight = sourceItem.height
    }

    function hideFloatingToolTip() { floatingToolTip.text = "" }

    function numericInput(value, fallback, minValue, maxValue) {
        var parsed = parseFloat(String(value).replace(",", "."))
        if (isNaN(parsed)) parsed = fallback
        return clamp(parsed, minValue, maxValue)
    }

    function controlValueText(kind) {
        if (kind === "altitude") return altitudeCommandMeters.toFixed(1)
        if (kind === "speed") return speedCommandMetersSecond.toFixed(1)
        if (kind === "takeoffSpeed") return takeoffSpeedCommandMetersSecond.toFixed(1)
        return "0.0"
    }

    function setControlValue(kind, value) {
        if (kind === "altitude") altitudeCommandMeters = numericInput(value, altitudeCommandMeters, -100, 100)
        else if (kind === "speed") speedCommandMetersSecond = numericInput(value, speedCommandMetersSecond, 0.1, 40)
        else if (kind === "takeoffSpeed") takeoffSpeedCommandMetersSecond = numericInput(value, takeoffSpeedCommandMetersSecond, 1, 5)
    }

    function adjustControlValue(kind, delta) {
        if (kind === "altitude") altitudeCommandMeters = numericInput(altitudeCommandMeters + delta, altitudeCommandMeters, -100, 100)
        else if (kind === "speed") speedCommandMetersSecond = numericInput(speedCommandMetersSecond + delta, speedCommandMetersSecond, 0.1, 40)
        else if (kind === "takeoffSpeed") takeoffSpeedCommandMetersSecond = numericInput(takeoffSpeedCommandMetersSecond + delta, takeoffSpeedCommandMetersSecond, 1, 5)
    }

    function canSendFocusCommand() {
        return focusVehicle && !(focusVehicle.vehicleLinkManager && focusVehicle.vehicleLinkManager.communicationLost)
    }

    function vehicleCanTakeoff(vehicle) {
        if (!vehicle || !vehicle.initialConnectComplete || !vehicle.guidedModeSupported || !vehicle.takeoffVehicleSupported || vehicle.flying) return false
        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.communicationLost) return false
        var report = vehicle.healthAndArmingCheckReport
        return !report || !report.supported || report.canTakeoff
    }

    function takeoffTargetsAvailable() {
        if (!guidedController) return false
        if (selectedCount() === 0) return guidedController.showTakeoff

        for (var i = 0; i < selectedIds.length; i++) {
            if (vehicleCanTakeoff(vehicleById(selectedIds[i]))) return true
        }
        return false
    }

    function vehicleCanLand(vehicle) {
        if (!vehicle || !vehicle.initialConnectComplete || !vehicle.guidedModeSupported
                || !vehicle.armed || !vehicle.flying || vehicle.fixedWing) return false
        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.communicationLost) return false
        return vehicle.flightMode !== vehicle.landFlightMode
    }

    function landTargetsAvailable() {
        if (!guidedController) return false
        if (selectedCount() === 0) return guidedController.showLand

        for (var i = 0; i < selectedIds.length; i++) {
            if (vehicleCanLand(vehicleById(selectedIds[i]))) return true
        }
        return false
    }

    function vehicleCanRTL(vehicle) {
        if (!vehicle || !vehicle.initialConnectComplete || !vehicle.guidedModeSupported
                || !vehicle.armed || !vehicle.flying) return false
        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.communicationLost) return false
        return vehicle.flightMode !== vehicle.rtlFlightMode && vehicle.flightMode !== vehicle.smartRTLFlightMode
    }

    function rtlTargetsAvailable() {
        if (!guidedController) return false
        if (selectedCount() === 0) return guidedController.showRTL

        for (var i = 0; i < selectedIds.length; i++) {
            if (vehicleCanRTL(vehicleById(selectedIds[i]))) return true
        }
        return false
    }

    function guidedActionData(actionId) {
        if (!guidedController) return undefined
        return actionId === guidedController.actionTakeoff
                || actionId === guidedController.actionLand
                || actionId === guidedController.actionRTL
                || actionId === guidedController.actionSwarm
                ? selectedIdsSnapshot() : undefined
    }

    function applyControlCommand(kind) {
        // 这些是显式人工参数/Guided 操作，不属于 AI 建议链路。
        if (!canSendFocusCommand()) {
            mainWindow.showMessageDialog(tr("指令未发送"), tr("当前没有可用的焦点飞行器，或链路已断开。"))
            return
        }
        if (kind === "altitude") {
            focusVehicle.guidedModeChangeAltitude(altitudeCommandMeters, false)
        } else if (kind === "speed") {
            if (focusVehicle.fixedWing || focusVehicle.vtolInFwdFlight) focusVehicle.guidedModeChangeEquivalentAirspeedMetersSecond(speedCommandMetersSecond)
            else focusVehicle.guidedModeChangeGroundSpeedMetersSecond(speedCommandMetersSecond)
        } else if (kind === "takeoffSpeed") {
            if (!focusVehicle.setGuidedTakeoffSpeed(takeoffSpeedCommandMetersSecond)) {
                mainWindow.showMessageDialog(tr("参数未下发"),
                                             tr("当前飞行器未提供 MPC_TKO_SPEED，或输入值超出飞控允许范围。"))
            }
        }
    }

    function configuredTakeoffSpeedText(vehicle) {
        var refreshTick = now
        if (!vehicle) return "--"
        var value = Number(vehicle.guidedTakeoffSpeed())
        return isNaN(value) ? "--" : value.toFixed(1) + " m/s"
    }

    Rectangle {
        id: fleetPanel
        z: 200
        anchors.left: parent.left
        anchors.leftMargin: root.margin
        anchors.top: parent.top
        anchors.topMargin: root.topInset + root.leftPanelTopExtra
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.margin + root.leftPanelBottomLift
        width: root.leftExpanded ? root.clamp(root.leftPanelWidth, root.leftPanelMinWidth, root.leftPanelMaxWidth) : root.collapsedPanelWidth
        color: root.panelColor
        border.color: root.panelLine
        radius: root.panelRadius
        clip: root.leftExpanded

        Behavior on width {
            enabled: !root.leftPanelResizing
            NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
        }

        DeadMouseArea { anchors.fill: parent }

        MouseArea {
            id: fleetCollapsedMouse
            anchors.fill: parent
            visible: !root.leftExpanded
            hoverEnabled: true
            onClicked: root.leftExpanded = true

            QGCColoredImage {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 16
                width: 32
                height: 32
                source: "/qmlimages/compassInstrumentArrow.svg"
                color: root.accent
            }

            MerivusToolTip {
                visible: fleetCollapsedMouse.containsMouse
                text: tr("\u5c55\u5f00\u673a\u7fa4\u4e0e\u98de\u884c\u63a7\u5236")
                anchors.left: parent.right
                anchors.leftMargin: 8
                anchors.top: parent.top
                anchors.topMargin: 12
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 7
            visible: root.leftExpanded

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: tr("\u98de\u884c\u63a7\u5236")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: root.fontPointSize(18)
                }
                QGCLabel {
                    text: root.vehicles ? tr("%1 \u67b6\u5728\u7ebf\u5217\u8868").arg(root.vehicles.count) : tr("0 \u67b6")
                    color: qgcPal.colorGrey
                    font.pointSize: root.fontPointSize(11)
                }
                QGCButton {
                    id: leftCollapseButton
                    text: "\u2039"
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 26
                    onHoveredChanged: {
                        if (hovered) root.showFloatingToolTip(leftCollapseButton, tr("\u6536\u8d77\u9762\u677f"), "right")
                        else root.hideFloatingToolTip()
                    }
                    onClicked: {
                        root.hideFloatingToolTip()
                        root.leftExpanded = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                radius: 7
                color: root.selectedCount() > 0 ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.18) : qgcPal.windowShade
                border.color: root.selectedCount() > 0 ? root.accent : root.mutedLine
                border.width: root.selectedCount() > 0 ? 2 : 1
                Rectangle {
                    visible: root.selectedCount() > 0
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 5
                    radius: 3
                    color: root.accent
                }
                QGCLabel {
                    anchors.fill: parent
                    anchors.leftMargin: root.selectedCount() > 0 ? 14 : 9
                    anchors.rightMargin: 9
                    verticalAlignment: Text.AlignVCenter
                    text: root.selectedSummary()
                    color: root.selectedCount() > 0 ? root.accent : qgcPal.colorGrey
                    font.pointSize: root.fontPointSize(root.selectedCount() > 0 ? 14 : 12)
                    font.bold: root.selectedCount() > 0
                    elide: Text.ElideRight
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                text: root.focusVehicle ? tr("\u5f53\u524d\u7126\u70b9 UAV-%1").arg(root.focusVehicle.id) : tr("\u5f53\u524d\u65e0\u98de\u884c\u5668\u7126\u70b9")
                color: qgcPal.colorGrey
                font.pointSize: root.fontPointSize(11)
                elide: Text.ElideRight
            }

            QGCLabel {
                Layout.fillWidth: true
                text: root.guidedController && root.guidedController.formationStatus
                        ? root.guidedController.formationStatus
                        : (root.guidedController && root.guidedController.swarmModeEnabled
                           ? tr("编队协议：已启用")
                           : tr("编队协议：未启用"))
                color: root.guidedController && root.guidedController.swarmModeEnabled ? root.nominal : qgcPal.colorOrange
                font.pointSize: root.fontPointSize(11)
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 7
                color: Qt.rgba(root.fleetHealthColor().r, root.fleetHealthColor().g,
                               root.fleetHealthColor().b, 0.12)
                border.color: root.fleetHealthColor()
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    QGCLabel {
                        text: tr("舰队基础健康")
                        color: qgcPal.text
                        font.bold: true
                        font.pointSize: root.fontPointSize(11)
                    }
                    QGCLabel {
                        text: root.fleetHealthText()
                        color: root.fleetHealthColor()
                        font.bold: true
                        font.pointSize: root.fontPointSize(11)
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: root.fleetHealth
                              ? tr("%1/%2 在线 · %3 分 · %4 个运行任务 · %5 条建议")
                                  .arg(root.fleetHealth.onlineVehicleCount || 0)
                                  .arg(root.fleetHealth.vehicleCount || 0)
                                  .arg(root.fleetHealth.score || 0)
                                  .arg(root.missionSummary.runningCount || 0)
                                  .arg(root.faultRecommendations ? root.faultRecommendations.length : 0)
                              : tr("等待车辆状态")
                        color: qgcPal.colorGrey
                        font.pointSize: root.fontPointSize(10)
                        elide: Text.ElideRight
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                text: root.fleetIntentTask
                      ? tr("Fleet OS：意图 %1 · 风险 %2 · 事件 %3 · 待交接 %4")
                          .arg(root.fleetIntentTask.state || tr("未创建"))
                          .arg(root.fleetRiskRadar ? (root.fleetRiskRadar.level || tr("未知")) : tr("未知"))
                          .arg(root.fleetEventBlackBox && root.fleetEventBlackBox.events
                               ? root.fleetEventBlackBox.events.length : 0)
                          .arg(root.fleetHandoffManager ? root.fleetHandoffManager.pendingCount : 0)
                      : tr("Fleet OS：等待意图任务")
                color: root.fleetRiskRadar && root.fleetRiskRadar.level === "critical"
                       ? qgcPal.colorRed : qgcPal.colorGrey
                font.pointSize: root.fontPointSize(10)
                elide: Text.ElideRight
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 5
                columnSpacing: 7
                rowSpacing: 7
                Repeater {
                    model: [
                        { title: tr("\u4efb\u52a1\u89c4\u5212"), icon: "/qmlimages/Plan.svg", enabled: true, type: "plan", action: -1 },
                        { title: tr("\u8d77\u98de"), icon: "/res/takeoff.svg", enabled: root.takeoffTargetsAvailable(), type: "guided", action: root.guidedController ? root.guidedController.actionTakeoff : -1 },
                        { title: tr("\u964d\u843d"), icon: "/res/land.svg", enabled: root.landTargetsAvailable(), type: "guided", action: root.guidedController ? root.guidedController.actionLand : -1 },
                        { title: tr("\u8fd4\u822a"), icon: "/res/rtl.svg", enabled: root.rtlTargetsAvailable(), type: "guided", action: root.guidedController ? root.guidedController.actionRTL : -1 },
                        {
                            title: root.guidedController && (root.guidedController.formationActive || root.guidedController.formationBusy) ? tr("结束编队") : tr("\u7f16\u961f"),
                            icon: "/qmlimages/swarm.svg",
                            enabled: root.guidedController && (root.guidedController.formationActive
                                     || root.guidedController.formationBusy
                                     || (root.guidedController.swarmModeEnabled && root.hasValidFormationSelection())),
                            type: "guided",
                            action: root.guidedController
                                    ? ((root.guidedController.formationActive || root.guidedController.formationBusy)
                                       ? root.guidedController.actionEndSwarm : root.guidedController.actionSwarm)
                                    : -1
                        }
                    ]
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        radius: 6
                        color: commandMouse.pressed ? qgcPal.windowShadeLight : (commandMouse.containsMouse ? root.raisedColor : qgcPal.windowShade)
                        border.color: modelData.enabled && (commandMouse.containsMouse || commandMouse.pressed) ? root.accent : root.mutedLine
                        opacity: modelData.enabled ? 1.0 : 0.45
                        Column {
                            anchors.centerIn: parent
                            spacing: 4
                            QGCColoredImage {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 20
                                height: 20
                                source: modelData.icon
                                color: modelData.enabled ? (commandMouse.containsMouse ? root.accent : qgcPal.text) : root.muted
                            }
                            QGCLabel {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.title
                                color: qgcPal.text
                                font.bold: true
                                font.pointSize: root.fontPointSize(11)
                            }
                        }
                        MouseArea {
                            id: commandMouse
                            anchors.fill: parent
                            enabled: modelData.enabled
                            hoverEnabled: true
                            onClicked: modelData.type === "plan" ? mainWindow.showPlanView()
                                                                  : root.runGuidedAction(modelData.action, root.guidedActionData(modelData.action))
                        }
                        MerivusToolTip {
                            visible: commandMouse.containsMouse
                            text: modelData.type === "plan" ? tr("\u6253\u5f00\u4efb\u52a1\u89c4\u5212") : tr("\u4f7f\u7528\u5f53\u524d\u6846\u9009\u76ee\u6807\u8fdb\u5165\u786e\u8ba4\u6d41\u7a0b")
                            anchors.left: parent.left
                            anchors.bottom: parent.top
                            anchors.bottomMargin: 6
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: root.focusVehicle ? "UAV-" + root.focusVehicle.id : tr("\u65e0\u4eba\u673a\u53c2\u6570")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: root.fontPointSize(14)
                }
                QGCLabel {
                    text: "\u25cb " + root.linkStateText(root.focusVehicle)
                    color: root.focusVehicle && !(root.focusVehicle.vehicleLinkManager && root.focusVehicle.vehicleLinkManager.communicationLost) ? root.nominal : root.muted
                    font.pointSize: root.fontPointSize(11)
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                text: root.focusVehicle ? ((root.focusVehicle.armed ? tr("\u5df2\u89e3\u9501") : tr("\u672a\u89e3\u9501")) + " / " + root.focusVehicle.flightMode) : tr("\u7b49\u5f85\u98de\u884c\u5668\u63a5\u5165")
                color: qgcPal.colorGrey
                font.pointSize: root.fontPointSize(11)
                elide: Text.ElideRight
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 5
                rowSpacing: 5
                Repeater {
                    model: [
                        { label: tr("\u9ad8\u5ea6"), kind: "altitude", adjustable: true, value: root.metricText("altitude", 1, "m"), step: 1.0, unit: "m", limitTop: 100, limitBottom: -100, tip: tr("飞行中为相对高度增量；点击起飞时同时作为默认起飞高度") },
                        { label: tr("\u5730\u901f"), kind: "speed", adjustable: true, value: root.metricText("speed", 1, "m/s"), step: 0.5, unit: "m/s", limitTop: 40, limitBottom: 0.1, tip: tr("目标速度命令：默认 5 m/s") },
                        { label: tr("起飞速度"), kind: "takeoffSpeed", adjustable: true, value: root.configuredTakeoffSpeedText(root.focusVehicle), step: 0.1, unit: "m/s", limitTop: 5, limitBottom: 1, tip: tr("写入 PX4 参数 MPC_TKO_SPEED；MAV_CMD_NAV_TAKEOFF 本身不携带速度") },
                        { label: tr("\u822a\u5411"), kind: "heading", adjustable: false, value: root.metricText("heading", 0, "\u00b0"), tip: tr("机头朝向（0/360 为北向）") },
                        { label: tr("\u7535\u6c60"), kind: "battery", adjustable: false, value: root.metricText("battery", 0, "%"), tip: tr("主电池剩余百分比") },
                        { label: "GPS/RTK", kind: "gps", adjustable: false, value: gpsStatus.summary(root.focusVehicle), tip: tr("点击查看定位类型、精度、速度、高度、GNSS 航向和双接收机信息") }
                    ]
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: modelData.adjustable ? 102 : 58
                        radius: 5
                        color: paramTipMouse.containsMouse ? root.raisedColor : qgcPal.windowShade
                        border.color: modelData.adjustable ? (root.canSendFocusCommand() ? root.accent : root.mutedLine) : (paramTipMouse.containsMouse ? root.accent : root.mutedLine)
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 2
                            QGCLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData.label
                                color: qgcPal.colorGrey
                                font.pointSize: root.fontPointSize(10)
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                visible: modelData.adjustable
                                horizontalAlignment: Text.AlignHCenter
                                text: tr("实时") + " " + modelData.value
                                color: root.accent
                                font.bold: true
                                font.pointSize: root.fontPointSize(12)
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                visible: modelData.adjustable
                                spacing: 3
                                QGCButton {
                                    text: "-"
                                    Layout.preferredWidth: 24
                                    Layout.preferredHeight: 24
                                    onClicked: {
                                        root.adjustControlValue(modelData.kind, -modelData.step)
                                        controlValueField.text = root.controlValueText(modelData.kind)
                                    }
                                }
                                QGCTextField {
                                    id: controlValueField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 24
                                    text: root.controlValueText(modelData.kind)
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pointSize: root.fontPointSize(11)
                                    validator: DoubleValidator { bottom: modelData.adjustable ? modelData.limitBottom : 0; top: modelData.adjustable ? modelData.limitTop : 1; decimals: 1 }
                                    onEditingFinished: root.setControlValue(modelData.kind, text)
                                }
                                QGCButton {
                                    text: "+"
                                    Layout.preferredWidth: 24
                                    Layout.preferredHeight: 24
                                    onClicked: {
                                        root.adjustControlValue(modelData.kind, modelData.step)
                                        controlValueField.text = root.controlValueText(modelData.kind)
                                    }
                                }
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 22
                                visible: modelData.adjustable
                                enabled: root.canSendFocusCommand()
                                text: tr("下发") + " " + modelData.unit
                                onClicked: {
                                    root.setControlValue(modelData.kind, controlValueField.text)
                                    controlValueField.text = root.controlValueText(modelData.kind)
                                    root.applyControlCommand(modelData.kind)
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                visible: !modelData.adjustable
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData.value
                                color: qgcPal.text
                                font.bold: true
                                font.pointSize: root.fontPointSize(12)
                            }
                        }
                        MouseArea {
                            id: paramTipMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: modelData.kind === "gps" ? Qt.LeftButton : Qt.NoButton
                            cursorShape: modelData.kind === "gps" ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: if (modelData.kind === "gps") root.showGpsDetails(root.focusVehicle)
                        }
                        MerivusToolTip {
                            visible: paramTipMouse.containsMouse
                            text: modelData.tip
                            anchors.left: parent.left
                            anchors.bottom: parent.top
                            anchors.bottomMargin: 6
                            maximumWidth: 280
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: tr("\u7535\u673a / ESC")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: root.fontPointSize(12)
                }
                QGCLabel {
                    text: root.escStatusText(root.focusVehicle)
                    color: root.focusVehicle && root.escHasData(root.focusVehicle, 0) ? root.nominal : root.muted
                    font.pointSize: root.fontPointSize(11)
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 4
                columnSpacing: 5
                rowSpacing: 5
                Repeater {
                    model: [
                        { label: "M1", index: 0, direction: "CW" },
                        { label: "M2", index: 1, direction: "CCW" },
                        { label: "M3", index: 2, direction: "CW" },
                        { label: "M4", index: 3, direction: "CCW" }
                    ]
                    Rectangle {
                        property var ftcMotor: root.ftcMotorData(root.focusVehicle, modelData.index)
                        Layout.fillWidth: true
                        Layout.preferredHeight: 72
                        radius: 5
                        color: escMouse.containsMouse ? root.raisedColor : qgcPal.windowShade
                        border.color: ftcMotor ? ftcStatusPalette.colorFor(ftcMotor.severity)
                                               : (root.escMotorOnline(root.focusVehicle, modelData.index) ? root.nominal : (escMouse.containsMouse ? root.accent : root.mutedLine))
                        Column {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 1
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 3
                                QGCLabel { text: modelData.label; color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(11) }
                                QGCLabel { text: root.escMotorOnline(root.focusVehicle, modelData.index) ? "\u25cf" : "\u25cb"; color: root.escMotorOnline(root.focusVehicle, modelData.index) ? root.nominal : root.muted; font.pointSize: root.fontPointSize(9) }
                            }
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 3
                                QGCLabel {
                                    text: root.escNumber(root.focusVehicle, "rpm", modelData.index, 0, "")
                                    color: qgcPal.text
                                    font.bold: true
                                    font.pointSize: root.fontPointSize(12)
                                }
                                QGCLabel {
                                    text: tr("rpm")
                                    color: qgcPal.colorGrey
                                    font.pointSize: root.fontPointSize(9)
                                }
                            }
                            QGCLabel {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.direction
                                color: root.accent
                                font.bold: true
                                font.pointSize: root.fontPointSize(10)
                            }
                            QGCLabel {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: ftcMotor ? tr("H %1 · E %2").arg(root.ftcPercentText(ftcMotor.health)).arg(root.ftcPercentText(ftcMotor.effectiveness))
                                               : tr("H N/A · E N/A")
                                color: ftcMotor ? ftcStatusPalette.colorFor(ftcMotor.severity) : root.muted
                                font.pointSize: root.fontPointSize(9)
                            }
                        }
                        MouseArea {
                            id: escMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                            onContainsMouseChanged: containsMouse
                                ? root.showFloatingToolTip(this, root.motorTipText(modelData.index), "right")
                                : root.hideFloatingToolTip()
                        }
                    }
                }
            }

            FtcStatusPanel {
                Layout.fillWidth: true
                vehicle: root.focusVehicle
                statusPalette: ftcStatusPalette
            }

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    Layout.fillWidth: true
                    text: tr("\u65e0\u4eba\u673a\u5217\u8868")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: root.fontPointSize(13)
                }
                Rectangle {
                    visible: root.selectedCount() > 0
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 26
                    radius: 8
                    color: clearMouse.containsMouse ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.22) : qgcPal.windowShade
                    border.color: clearMouse.containsMouse ? root.accent : root.mutedLine
                    QGCLabel {
                        anchors.centerIn: parent
                        text: tr("\u6e05\u9664")
                        color: clearMouse.containsMouse ? root.accent : qgcPal.text
                        font.bold: true
                        font.pointSize: root.fontPointSize(11)
                    }
                    MouseArea {
                        id: clearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.clearSelectionRequested()
                    }
                }
            }

            QGCFlickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: vehicleColumn.implicitHeight

                ColumnLayout {
                    id: vehicleColumn
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: root.vehicles ? root.vehicles : 0
                        delegate: Rectangle {
                            id: vehicleRow
                            property var vehicle: object
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: 5
                            color: root.isSelected(vehicle.id) ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.18) : qgcPal.windowShade
                            border.color: root.isSelected(vehicle.id) ? root.accent : root.mutedLine
                            border.width: root.isSelected(vehicle.id) ? 2 : 1

                            Rectangle {
                                visible: root.isSelected(vehicle.id)
                                anchors.fill: parent
                                anchors.margins: 2
                                radius: 4
                                color: "transparent"
                                border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.48)
                                border.width: 1
                            }
                            Rectangle {
                                visible: root.isSelected(vehicle.id)
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 4
                                radius: 2
                                color: root.accent
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 6
                                QGCLabel {
                                    Layout.preferredWidth: 62
                                    text: vehicle ? "UAV-" + vehicle.id : "UAV--"
                                    color: root.isSelected(vehicle.id) ? root.accent : qgcPal.text
                                    font.bold: true
                                    font.pointSize: root.fontPointSize(12)
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: vehicle ? ((vehicle.armed ? tr("\u5df2\u89e3\u9501") : tr("\u672a\u89e3\u9501")) + " / " + vehicle.flightMode) : "--"
                                    color: qgcPal.colorGrey
                                    font.pointSize: root.fontPointSize(11)
                                    elide: Text.ElideRight
                                }
                                QGCLabel {
                                    text: root.batteryPercent(vehicle)
                                    color: qgcPal.text
                                    font.pointSize: root.fontPointSize(11)
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                onClicked: {
                                    root.vehicleFocusRequested(vehicle.id)
                                }
                            }
                        }
                    }

                    QGCLabel {
                        width: parent.width
                        visible: !root.vehicles || root.vehicles.count === 0
                        text: tr("\u6682\u65e0\u63a5\u5165\u98de\u884c\u5668")
                        color: qgcPal.colorGrey
                        font.pointSize: root.fontPointSize(12)
                    }
                }
            }
        }

        MouseArea {
            id: leftWidthResizeArea
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeHandleThickness
            visible: root.leftExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeHorCursor
            property real startWidth: 0
            property real startMouseX: 0
            onPressed: {
                mouse.accepted = true
                root.leftPanelResizing = true
                startWidth = root.leftPanelWidth
                startMouseX = mapToItem(root, mouse.x, mouse.y).x
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseX = mapToItem(root, mouse.x, mouse.y).x
                    root.leftPanelWidth = root.clamp(startWidth + currentMouseX - startMouseX, root.leftPanelMinWidth, root.leftPanelMaxWidth)
                }
            }
            onReleased: root.leftPanelResizing = false
            onCanceled: root.leftPanelResizing = false
            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 2
                height: Math.min(120, parent.height * 0.28)
                radius: 1
                color: leftWidthResizeArea.containsMouse || leftWidthResizeArea.pressed ? root.accent : root.mutedLine
            }
        }

        MouseArea {
            id: leftTopResizeArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.resizeHandleThickness
            visible: root.leftExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            property real startTopExtra: 0
            property real startMouseY: 0
            onPressed: {
                mouse.accepted = true
                root.leftPanelResizing = true
                startTopExtra = root.leftPanelTopExtra
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                    var maxTop = Math.max(0, root.height - root.topInset - root.margin - root.leftPanelBottomLift - root.sidePanelMinHeight)
                    root.leftPanelTopExtra = root.clamp(startTopExtra + currentMouseY - startMouseY, 0, maxTop)
                }
            }
            onReleased: root.leftPanelResizing = false
            onCanceled: root.leftPanelResizing = false
            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(92, parent.width * 0.32)
                height: 2
                radius: 1
                color: leftTopResizeArea.containsMouse || leftTopResizeArea.pressed ? root.accent : root.mutedLine
            }
        }

        MouseArea {
            id: leftBottomResizeArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.resizeHandleThickness
            visible: root.leftExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            property real startBottomLift: 0
            property real startMouseY: 0
            onPressed: {
                mouse.accepted = true
                root.leftPanelResizing = true
                startBottomLift = root.leftPanelBottomLift
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                    var maxBottom = Math.max(root.panelBottomLiftMin, root.height - root.topInset - root.leftPanelTopExtra - root.margin - root.sidePanelMinHeight)
                    root.leftPanelBottomLift = root.clamp(startBottomLift - (currentMouseY - startMouseY), root.panelBottomLiftMin, maxBottom)
                }
            }
            onReleased: root.leftPanelResizing = false
            onCanceled: root.leftPanelResizing = false
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(92, parent.width * 0.32)
                height: 2
                radius: 1
                color: leftBottomResizeArea.containsMouse || leftBottomResizeArea.pressed ? root.accent : root.mutedLine
            }
        }
    }

    Rectangle {
        id: rightPanel
        z: 200
        anchors.right: parent.right
        anchors.rightMargin: root.margin
        anchors.top: parent.top
        anchors.topMargin: root.topInset + root.rightPanelTopExtra
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.margin + root.rightPanelBottomLift
        width: root.rightExpanded ? root.clamp(root.rightPanelWidth, root.rightPanelMinWidth, root.rightPanelMaxWidth) : root.collapsedPanelWidth
        color: root.panelColor
        border.color: root.panelLine
        radius: root.panelRadius
        clip: root.rightExpanded

        Behavior on width {
            enabled: !root.rightPanelResizing
            NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
        }

        DeadMouseArea { anchors.fill: parent }

        MouseArea {
            id: environmentCollapsedMouse
            anchors.fill: parent
            visible: !root.rightExpanded
            hoverEnabled: true
            onClicked: root.rightExpanded = true

            QGCColoredImage {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 16
                width: 32
                height: 32
                source: "/qmlimages/compassInstrumentArrow.svg"
                color: root.accent
            }

            MerivusToolTip {
                visible: environmentCollapsedMouse.containsMouse
                text: tr("\u5c55\u5f00\u73af\u5883\u3001\u59ff\u6001\u4e0e\u89c6\u9891")
                anchors.right: parent.left
                anchors.rightMargin: 8
                anchors.top: parent.top
                anchors.topMargin: 12
            }
        }

        QGCFlickable {
            anchors.fill: parent
            anchors.margins: 12
            visible: root.rightExpanded
            clip: true
            contentWidth: width
            contentHeight: rightContent.implicitHeight

            ColumnLayout {
                id: rightContent
                width: parent.width
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    QGCLabel {
                        Layout.fillWidth: true
                        text: tr("环境 / 姿态 / 视频")
                        color: qgcPal.text
                        font.bold: true
                        font.pointSize: root.fontPointSize(18)
                    }
                    RowLayout {
                        visible: root.selectedCount() > 1
                        spacing: 2
                        Repeater {
                            model: root.selectedIds
                            CheckBox {
                                id: rightVehicleOption
                                text: "UAV-" + modelData
                                checked: root.rightVehicleSelected(modelData)
                                font.pointSize: root.fontPointSize(10)
                                font.bold: checked
                                onClicked: root.rightPanelVehicleId = modelData
                                indicator: Rectangle {
                                    implicitWidth: 12
                                    implicitHeight: 12
                                    x: 2
                                    y: rightVehicleOption.height / 2 - height / 2
                                    radius: 3
                                    color: rightVehicleOption.checked ? root.accent : "transparent"
                                    border.color: rightVehicleOption.checked ? root.accent : qgcPal.colorGrey
                                }
                                contentItem: Item {
                                    implicitWidth: rightVehicleOptionLabel.implicitWidth + 20
                                    implicitHeight: 20
                                    QGCLabel {
                                        id: rightVehicleOptionLabel
                                        x: 18
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: rightVehicleOption.text
                                        color: rightVehicleOption.checked ? root.accent : qgcPal.text
                                        font.bold: rightVehicleOption.checked
                                        font.pointSize: root.fontPointSize(10)
                                    }
                                }
                            }
                        }
                    }
                    CheckBox {
                        id: singleRightVehicleCheck
                        visible: root.selectedCount() <= 1
                        checked: root.rightVehicle !== null
                        enabled: root.rightVehicle !== null
                        text: root.rightVehicle ? "UAV-" + root.rightVehicle.id : tr("\u65e0\u6570\u636e")
                        font.pointSize: root.fontPointSize(11)
                        font.bold: true
                        indicator: Rectangle {
                            implicitWidth: 13
                            implicitHeight: 13
                            x: 2
                            y: singleRightVehicleCheck.height / 2 - height / 2
                            radius: 3
                            color: singleRightVehicleCheck.checked ? root.accent : "transparent"
                            border.color: singleRightVehicleCheck.checked ? root.accent : root.mutedLine
                        }
                        contentItem: Item {
                            implicitWidth: singleRightVehicleLabel.implicitWidth + 22
                            implicitHeight: 22
                            QGCLabel {
                                id: singleRightVehicleLabel
                                x: 19
                                anchors.verticalCenter: parent.verticalCenter
                                text: singleRightVehicleCheck.text
                                color: singleRightVehicleCheck.checked ? root.accent : root.muted
                                font.bold: true
                                font.pointSize: root.fontPointSize(11)
                            }
                        }
                    }
                    QGCButton {
                        id: rightCollapseButton
                        text: "›"
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 30
                        onHoveredChanged: {
                            if (hovered) root.showFloatingToolTip(rightCollapseButton, tr("收起面板"), "right")
                            else root.hideFloatingToolTip()
                        }
                        onClicked: {
                            root.hideFloatingToolTip()
                            root.rightExpanded = false
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    radius: 7
                    color: qgcPal.windowShadeDark
                    border.color: root.mutedLine
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 7
                        RowLayout {
                            Layout.fillWidth: true
                            QGCLabel { Layout.fillWidth: true; text: tr("环境参数"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(14) }
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            columns: 3
                            columnSpacing: 7
                            rowSpacing: 7
                            Repeater {
                                model: [
                                    { label: tr("日期"), value: Qt.formatDate(root.now, "yyyy-MM-dd")},
                                    { label: tr("时间"), value: Qt.formatTime(root.now, "HH:mm:ss")},
                                    { label: tr("风向"), value: root.windDirectionText(root.rightVehicle)},
                                    { label: tr("温度"), value: root.temperatureText(root.rightVehicle)},
                                    { label: tr("湿度"), value: root.humidityText(root.rightVehicle)},
                                    { label: tr("风速"), value: root.windSpeedText(root.rightVehicle)}
                                ]
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 5
                                    color: qgcPal.windowShade
                                    border.color: root.mutedLine
                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 1
                                        QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: qgcPal.colorGrey; font.pointSize: root.fontPointSize(10) }
                                        QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.value; color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(13); elide: Text.ElideRight }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    id: sensorBlock
                    property real instrumentSize: Math.min(104, Math.max(82, (rightPanel.width - 72) * 0.28))

                    Layout.fillWidth: true
                    Layout.preferredHeight: 304
                    radius: 8
                    color: qgcPal.windowShadeDark
                    border.color: root.mutedLine
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 20
                            QGCLabel {
                                Layout.fillWidth: true
                                text: tr("姿态与传感器")
                                color: qgcPal.text
                                font.bold: true
                                font.pointSize: root.fontPointSize(14)
                            }
                            QGCLabel {
                                text: tr("R %1 / P %2 / H %3").arg(root.attitudeTextFor(root.rightVehicle, "roll")).arg(root.attitudeTextFor(root.rightVehicle, "pitch")).arg(root.metricTextFor(root.rightVehicle, "heading", 0, "°"))
                                color: qgcPal.colorGrey
                                font.pointSize: root.fontPointSize(10)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 146
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 7
                                color: qgcPal.windowShade
                                border.color: root.mutedLine
                                clip: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 7
                                    spacing: 3
                                    RowLayout {
                                        Layout.fillWidth: true
                                        QGCLabel { Layout.fillWidth: true; text: tr("水平仪"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(13) }
                                    }
                                    QGCAttitudeWidget {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: sensorBlock.instrumentSize
                                        Layout.preferredHeight: sensorBlock.instrumentSize
                                        vehicle: root.rightVehicle
                                        size: sensorBlock.instrumentSize
                                        showHeading: false
                                        showPitch: true
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                    onContainsMouseChanged: containsMouse ? root.showFloatingToolTip(this, tr("水平仪：显示所选无人机遥测中的横滚角与俯仰角。"), "left") : root.hideFloatingToolTip()
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 7
                                color: qgcPal.windowShade
                                border.color: root.mutedLine
                                clip: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 7
                                    spacing: 3
                                    RowLayout {
                                        Layout.fillWidth: true
                                        QGCLabel { Layout.fillWidth: true; text: tr("指南针"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(13) }
                                        QGCLabel { text: root.metricTextFor(root.rightVehicle, "heading", 0, "°"); color: root.accent; font.bold: true; font.pointSize: root.fontPointSize(12) }
                                    }
                                    Rectangle {
                                        id: compassFace
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: sensorBlock.instrumentSize
                                        Layout.preferredHeight: sensorBlock.instrumentSize
                                        radius: width / 2
                                        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.36)
                                        border.color: root.panelLine
                                        border.width: 1

                                        Repeater {
                                            model: [
                                                { label: "N", x: 0.47, y: 0.04, c: root.accent },
                                                { label: "E", x: 0.84, y: 0.45, c: qgcPal.colorGrey },
                                                { label: "S", x: 0.47, y: 0.84, c: qgcPal.colorGrey },
                                                { label: "W", x: 0.08, y: 0.45, c: qgcPal.colorGrey }
                                            ]
                                            QGCLabel {
                                                x: compassFace.width * modelData.x
                                                y: compassFace.height * modelData.y
                                                text: modelData.label
                                                color: modelData.c
                                                font.bold: modelData.label === "N"
                                                font.pointSize: root.fontPointSize(10)
                                            }
                                        }
                                        QGCColoredImage {
                                            anchors.centerIn: parent
                                            width: compassFace.width * 0.46
                                            height: width
                                            source: "/qmlimages/compassInstrumentArrow.svg"
                                            color: root.rightVehicle ? root.accent : root.muted
                                            rotation: root.rightVehicle && root.rightVehicle.heading ? Number(root.rightVehicle.heading.rawValue) : 0
                                            transformOrigin: Item.Center
                                        }
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                    onContainsMouseChanged: containsMouse ? root.showFloatingToolTip(this, tr("指南针：显示所选无人机航向，0°/360° 表示正北。"), "left") : root.hideFloatingToolTip()
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 112
                            radius: 7
                            color: qgcPal.windowShade
                            border.color: root.mutedLine
                            clip: true
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6
                                RowLayout {
                                    Layout.fillWidth: true
                                    QGCLabel { Layout.fillWidth: true; text: tr("IMU / 振动"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(13) }
                                    QGCLabel {
                                        text: root.rightVehicle && root.rightVehicle.vibration ? tr("实时三轴") : tr("等待数据")
                                        color: root.rightVehicle && root.rightVehicle.vibration ? root.nominal : root.muted
                                        font.pointSize: root.fontPointSize(10)
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    spacing: 6
                                    Repeater {
                                        model: [
                                            { label: "X", axis: "xAxis", tip: tr("X 轴振动：横向振动水平，用于发现 IMU 或机架异常。") },
                                            { label: "Y", axis: "yAxis", tip: tr("Y 轴振动：纵向振动水平，用于发现 IMU 或机架异常。") },
                                            { label: "Z", axis: "zAxis", tip: tr("Z 轴振动：垂向振动水平，用于发现桨叶、电机或安装共振。") }
                                        ]
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            radius: 6
                                            color: qgcPal.windowShadeDark
                                            border.color: root.rightVehicle && root.rightVehicle.vibration ? root.accent : root.mutedLine
                                            Column {
                                                anchors.centerIn: parent
                                                spacing: 3
                                                QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: root.accent; font.bold: true; font.pointSize: root.fontPointSize(13) }
                                                QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: root.vibrationAxisTextFor(root.rightVehicle, modelData.axis); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(14) }
                                                Rectangle {
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    width: Math.max(34, parent.width * 0.45)
                                                    height: 3
                                                    radius: 2
                                                    color: root.rightVehicle && root.rightVehicle.vibration ? root.accent : root.mutedLine
                                                }
                                                QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: tr("振动"); color: qgcPal.colorGrey; font.pointSize: root.fontPointSize(9) }
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                acceptedButtons: Qt.NoButton
                                                onContainsMouseChanged: containsMouse ? root.showFloatingToolTip(this, modelData.tip, "left") : root.hideFloatingToolTip()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    QGCLabel { Layout.fillWidth: true; text: tr("视频监控"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(15) }
                    QGCLabel { text: "16:9"; color: root.accent; font.bold: true; font.pointSize: root.fontPointSize(12) }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: rightContent.width * 9 / 16
                    radius: 8
                    color: qgcPal.windowShadeDark
                    border.color: root.accent
                    border.width: 1

                    Item {
                        id: videoViewport
                        anchors.fill: parent
                        anchors.margins: 6
                        clip: true
                    }

                    Rectangle {
                        anchors.fill: videoViewport
                        radius: 6
                        color: "transparent"
                        border.color: root.mutedLine
                        z: 2
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        visible: !QGroundControl.videoManager.hasVideo
                        z: 1
                        QGCColoredImage { anchors.horizontalCenter: parent.horizontalCenter; width: 44; height: 44; source: "/qmlimages/camera_video.svg"; color: root.muted }
                        QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: tr("16:9 视频画面区域"); color: qgcPal.text; font.bold: true; font.pointSize: root.fontPointSize(15) }
                        QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: tr("后续接入 4G 链路视频流 / 吊舱画面"); color: qgcPal.colorGrey; font.pointSize: root.fontPointSize(11) }
                    }

                    QGCLabel {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 8
                        text: QGroundControl.videoManager.recording ? tr("VIDEO · REC") :
                              (QGroundControl.videoManager.decoding ? tr("VIDEO · LIVE") : tr("VIDEO · STANDBY"))
                        color: QGroundControl.videoManager.recording ? qgcPal.colorRed :
                               (QGroundControl.videoManager.decoding ? root.nominal : root.muted)
                        font.pointSize: root.fontPointSize(10)
                        z: 3
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    QGCButton {
                        id: videoRecordButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: QGroundControl.videoManager.recording ? tr("停止录像") : tr("录像")
                        iconSource: "/qmlimages/camera_video.svg"
                        iconLeft: true
                        pointSize: ScreenTools.smallFontPointSize
                        backRadius: 4
                        primary: QGroundControl.videoManager.recording
                        enabled: QGroundControl.videoManager.streaming || QGroundControl.videoManager.recording
                        onHoveredChanged: {
                            if (hovered) root.showFloatingToolTip(videoRecordButton,
                                                                 QGroundControl.videoManager.recording ? tr("停止并保存当前视频录像") : tr("将当前视频流录制到本地"),
                                                                 "right")
                            else root.hideFloatingToolTip()
                        }
                        onClicked: {
                            root.hideFloatingToolTip()
                            if (QGroundControl.videoManager.recording) QGroundControl.videoManager.stopRecording()
                            else QGroundControl.videoManager.startRecording()
                        }
                    }

                    QGCButton {
                        id: videoPhotoButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: tr("拍照")
                        iconSource: "/qmlimages/camera_photo.svg"
                        iconLeft: true
                        pointSize: ScreenTools.smallFontPointSize
                        backRadius: 4
                        enabled: QGroundControl.videoManager.decoding
                        onHoveredChanged: {
                            if (hovered) root.showFloatingToolTip(videoPhotoButton, tr("保存当前视频画面截图"), "right")
                            else root.hideFloatingToolTip()
                        }
                        onClicked: {
                            root.hideFloatingToolTip()
                            QGroundControl.videoManager.grabImage()
                        }
                    }

                    QGCButton {
                        id: videoSettingsButton
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        text: tr("视频设置")
                        iconSource: "/res/gear-black.svg"
                        iconLeft: true
                        pointSize: ScreenTools.smallFontPointSize
                        backRadius: 4
                        onHoveredChanged: {
                            if (hovered) root.showFloatingToolTip(videoSettingsButton, tr("打开应用设置中的视频配置"), "right")
                            else root.hideFloatingToolTip()
                        }
                        onClicked: {
                            root.hideFloatingToolTip()
                            QGroundControl.saveGlobalSetting("Merivus/AppSettingsPage", "qrc:/qml/GeneralSettings.qml")
                            mainWindow.showSettingsTool()
                        }
                    }
                }
            }
        }

        MouseArea {
            id: rightWidthResizeArea
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeHandleThickness
            visible: root.rightExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeHorCursor
            property real startWidth: 0
            property real startMouseX: 0
            onPressed: {
                mouse.accepted = true
                root.rightPanelResizing = true
                startWidth = root.rightPanelWidth
                startMouseX = mapToItem(root, mouse.x, mouse.y).x
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseX = mapToItem(root, mouse.x, mouse.y).x
                    root.rightPanelWidth = root.clamp(startWidth - (currentMouseX - startMouseX), root.rightPanelMinWidth, root.rightPanelMaxWidth)
                }
            }
            onReleased: root.rightPanelResizing = false
            onCanceled: root.rightPanelResizing = false
            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 2
                height: Math.min(120, parent.height * 0.28)
                radius: 1
                color: rightWidthResizeArea.containsMouse || rightWidthResizeArea.pressed ? root.accent : root.mutedLine
            }
        }

        MouseArea {
            id: rightTopResizeArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.resizeHandleThickness
            visible: root.rightExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            property real startTopExtra: 0
            property real startMouseY: 0
            onPressed: {
                mouse.accepted = true
                root.rightPanelResizing = true
                startTopExtra = root.rightPanelTopExtra
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                    var maxTop = Math.max(0, root.height - root.topInset - root.margin - root.rightPanelBottomLift - root.sidePanelMinHeight)
                    root.rightPanelTopExtra = root.clamp(startTopExtra + currentMouseY - startMouseY, 0, maxTop)
                }
            }
            onReleased: root.rightPanelResizing = false
            onCanceled: root.rightPanelResizing = false
            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(92, parent.width * 0.32)
                height: 2
                radius: 1
                color: rightTopResizeArea.containsMouse || rightTopResizeArea.pressed ? root.accent : root.mutedLine
            }
        }

        MouseArea {
            id: rightBottomResizeArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.resizeHandleThickness
            visible: root.rightExpanded
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            property real startBottomLift: 0
            property real startMouseY: 0
            onPressed: {
                mouse.accepted = true
                root.rightPanelResizing = true
                startBottomLift = root.rightPanelBottomLift
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                    var maxBottom = Math.max(root.panelBottomLiftMin, root.height - root.topInset - root.rightPanelTopExtra - root.margin - root.sidePanelMinHeight)
                    root.rightPanelBottomLift = root.clamp(startBottomLift - (currentMouseY - startMouseY), root.panelBottomLiftMin, maxBottom)
                }
            }
            onReleased: root.rightPanelResizing = false
            onCanceled: root.rightPanelResizing = false
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(92, parent.width * 0.32)
                height: 2
                radius: 1
                color: rightBottomResizeArea.containsMouse || rightBottomResizeArea.pressed ? root.accent : root.mutedLine
            }
        }
    }

    MerivusToolTip {
        id: floatingToolTip
        property real sourceX: 0
        property real sourceY: 0
        property real sourceWidth: 0
        property real sourceHeight: 0
        property string align: "right"

        z: 1000000
        x: {
            var gap = 8
            var preferred = align === "left" ? sourceX - width - gap : sourceX + sourceWidth + gap
            var alternate = align === "left" ? sourceX + sourceWidth + gap : sourceX - width - gap
            var fits = preferred >= root.margin && preferred + width <= root.width - root.margin
            return root.clamp(fits ? preferred : alternate,
                              root.margin,
                              Math.max(root.margin, root.width - width - root.margin))
        }
        y: root.clamp(sourceY + (sourceHeight - height) / 2,
                      root.margin,
                      Math.max(root.margin, root.height - height - root.margin))
    }
}

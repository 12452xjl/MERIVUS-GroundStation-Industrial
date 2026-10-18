/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQuick.Dialogs  1.3

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0

QGCPopupDialog {
    title:      qsTr("RC To Param")
    buttons:    StandardButton.Cancel | StandardButton.Ok

    property alias tuningFact: controller.tuningFact

    function _maintenanceWriteError(vehicle) {
        if (!vehicle) return qsTr("没有可用的目标飞行器。")
        if (!vehicle.initialConnectComplete
                || !vehicle.vehicleLinkManager
                || vehicle.vehicleLinkManager.communicationLost) {
            return qsTr("目标飞行器尚未完成连接，或链路已断开。")
        }
        if (vehicle.armed || vehicle.flying) {
            return qsTr("飞行中或已解锁时禁止修改 RC 参数映射。")
        }
        var values = [Number(scale.text), Number(centerValue.text),
                      Number(minValue.text), Number(maxValue.text)]
        for (var i = 0; i < values.length; i++) {
            if (!isFinite(values[i])) return qsTr("参数值必须是有限数字。")
        }
        if (typeof globals !== "undefined" && globals.guidedControllerFlyView) {
            var guided = globals.guidedControllerFlyView
            if (!guided.fleetHealthCenter
                    || typeof guided.fleetHealthCenter.evaluateTaskGate !== "function") {
                return qsTr("舰队健康门禁尚未就绪，参数写入已阻止。")
            }
            var gate = guided.fleetHealthCenter.evaluateTaskGate([Number(vehicle.id)], false)
            if (!gate || gate.allowed !== true) {
                return qsTr("目标飞行器未通过舰队健康门禁。")
            }
        }
        return ""
    }

    onAccepted: {
        var vehicle = QGroundControl.multiVehicleManager.activeVehicle
        var error = _maintenanceWriteError(vehicle)
        if (error.length > 0) {
            mainWindow.showMessageDialog(qsTr("RC 参数映射已阻止"), error)
            return
        }
        vehicle.sendParamMapRC(tuningFact.name, Number(scale.text), Number(centerValue.text),
                               tuningID.currentIndex, Number(minValue.text), Number(maxValue.text))
    }

    RCToParamDialogController {
        id: controller
    }

    ColumnLayout {
        spacing: ScreenTools.defaultDialogControlSpacing

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            text:                   qsTr("Bind an RC Channel to a parameter value. Tuning IDs can be mapped to an RC Channel from Radio Setup page.")
        }

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            text:                   qsTr("Waiting on parameter update from Vehicle.")
            visible:                !controller.ready
        }

        GridLayout {
            id:             mainGrid
            columns:        2
            rowSpacing:     ScreenTools.defaultDialogControlSpacing
            columnSpacing:  ScreenTools.defaultDialogControlSpacing
            enabled:        controller.ready

            QGCLabel { text: qsTr("Parameter") }
            QGCLabel { text: tuningFact.name }

            QGCLabel { text: qsTr("Tuning ID") }
            QGCComboBox {
                id:                 tuningID
                Layout.fillWidth:   true
                currentIndex:       0
                model:              [ 1, 2, 3 ]
            }

            QGCLabel { text: qsTr("Scale") }
            QGCTextField {
                id:     scale
                text:   controller.scale.valueString
            }

            QGCLabel { text: qsTr("Center Value") }
            QGCTextField {
                id:     centerValue
                text:   controller.center.valueString
            }

            QGCLabel { text: qsTr("Min Value") }
            QGCTextField {
                id:     minValue
                text:   controller.min.valueString
            }

            QGCLabel { text: qsTr("Max Value") }
            QGCTextField {
                id:     maxValue
                text:   controller.max.valueString
            }
        }

        QGCLabel {
            Layout.preferredWidth:  mainGrid.width
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            text:                   qsTr("Double check that all values are correct prior to confirming dialog.")
        }
    }
}

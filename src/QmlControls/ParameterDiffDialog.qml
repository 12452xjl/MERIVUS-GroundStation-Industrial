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
    title:      qsTr("Load Parameters")
    buttons:    StandardButton.Cancel | (_previewCurrent && paramController.diffList.count ? StandardButton.Ok : 0)
    acceptButtonEnabled: _previewCurrent && paramController.diffList.count > 0
                         && paramController.writeAllowed()
                         && !paramController.diffOtherVehicle

    property var paramController
    property var _dialogLifetime: ({ alive: true })
    // Copied by the creator; never bind this to a later preview's token.
    property string previewToken: ""
    readonly property bool _previewCurrent: paramController && previewToken.length > 0
                                            && previewToken === paramController.diffToken

    onAccepted: {
        var lifetime = _dialogLifetime
        var originalController = paramController
        var token = previewToken
        var window = mainWindow
        var title = qsTr("Parameter import blocked")
        var replacedText = qsTr("This preview has been replaced or closed. Close this dialog and review the current file again.")
        var failureText = qsTr("The parameter snapshot could not be applied. Reload the file and review the current vehicle values.")
        var reason = ""
        preventClose = true
        if (!lifetime.alive) return
        if (!originalController || !token || token !== originalController.diffToken) {
            reason = replacedText
        } else if (originalController.diffOtherVehicle) {
            reason = qsTr("This parameter file belongs to another vehicle or contains mixed vehicle IDs. Reload a file exported from the selected vehicle.")
        } else {
            var allowed = originalController.writeAllowed()
            if (!lifetime.alive || !originalController) return
            if (token !== originalController.diffToken) {
                reason = replacedText
            } else if (!allowed) {
                reason = originalController.writeBlockReason()
                if (!lifetime.alive || !originalController) return
            } else {
                var accepted = originalController.sendDiff(token)
                if (!lifetime.alive || !originalController) return
                if (accepted) {
                    preventClose = false
                    return
                }
                reason = token === originalController.diffToken ? originalController.diffError || failureText : replacedText
            }
        }
        if (window) window.showMessageDialog(title, reason || failureText)
    }

    Component.onDestruction: {
        _dialogLifetime.alive = false
        if (paramController) paramController.clearDiff(previewToken)
    }

    ColumnLayout {
        spacing: ScreenTools.defaultDialogControlSpacing

        QGCLabel {
            Layout.preferredWidth:  mainGrid.visible ? mainGrid.width : ScreenTools.defaultFontPixelWidth * 40
            wrapMode:               Text.WordWrap
            text:                   !_previewCurrent ?
                                        qsTr("This preview has been replaced or closed. Close this dialog and review the current file again.") : paramController.diffList.count ?
                                        qsTr("The following parameters differ from the current vehicle values. Click 'Ok' to queue the selected changes. Follow the import status on the parameter page and verify the vehicle values.") :
                                        qsTr("There are no differences between the file loaded and the current settings on the Vehicle.")
        }

        QGCLabel {
            Layout.preferredWidth: mainGrid.visible ? mainGrid.width : ScreenTools.defaultFontPixelWidth * 40
            wrapMode: Text.WordWrap
            color: qgcPal.warningText
            visible: _previewCurrent && paramController.diffOtherVehicle
            text: qsTr("Import blocked: the file vehicle ID does not match the selected vehicle (or the file contains mixed/invalid IDs).")
        }

        QGCLabel {
            Layout.preferredWidth: mainGrid.visible ? mainGrid.width : ScreenTools.defaultFontPixelWidth * 40
            wrapMode: Text.WordWrap
            color: qgcPal.warningText
            visible: _previewCurrent && !paramController.writeAllowed() && !paramController.diffOtherVehicle
            text: qsTr("Import is currently blocked: %1").arg(paramController.writeBlockReason())
        }

        GridLayout {
            id:         mainGrid
            rows:       _previewCurrent ? paramController.diffList.count + 1 : 1
            columns:    _previewCurrent && paramController.diffMultipleComponents ? 5 : 4
            flow:       GridLayout.TopToBottom
            visible:    _previewCurrent && paramController.diffList.count > 0
            enabled:    _previewCurrent

            QGCCheckBox {
                checked: true
                onClicked: {
                    var lifetime = _dialogLifetime
                    var originalController = paramController
                    var token = previewToken
                    var selected = checked
                    for (var i=0; lifetime.alive && originalController && token.length > 0 && token === originalController.diffToken && i<originalController.diffList.count; i++) {
                        var row = originalController.diffList.get(i)
                        if (row) row.load = selected
                    }
                }
            }
            Repeater {
                model: _previewCurrent ? paramController.diffList : 0
                QGCCheckBox {
                    checked:    object.load
                    onClicked: {
                        if (_previewCurrent && object) object.load = checked
                    }
                }
            }

            Repeater {
                model: _previewCurrent && paramController.diffMultipleComponents ? 1 : 0
                QGCLabel { text: qsTr("Comp ID") }
            }
            Repeater {
                model: _previewCurrent && paramController.diffMultipleComponents ? paramController.diffList : 0
                QGCLabel { text: object.componentId }
            }

            QGCLabel { text: qsTr("Name") }
            Repeater {
                model: _previewCurrent ? paramController.diffList : 0
                QGCLabel { text: object.name }
            }

            QGCLabel { text: qsTr("File") }
            Repeater {
                model: _previewCurrent ? paramController.diffList : 0
                QGCLabel { text: object.fileValue + " " + object.units }
            }

            QGCLabel { text: qsTr("Vehicle") }
            Repeater {
                model: _previewCurrent ? paramController.diffList : 0
                QGCLabel { text: object.noVehicleValue ? qsTr("N/A") : object.vehicleValue + " " + object.units }
            }
        }
    }
}

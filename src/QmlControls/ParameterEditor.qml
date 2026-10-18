/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Dialogs              1.2
import QtQuick.Layouts              1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    id:         _root

    property Fact   _editorDialogFact: Fact { }
    property int    _rowHeight:         ScreenTools.defaultFontPixelHeight * 2
    property int    _rowWidth:          10 // Dynamic adjusted at runtime
    property bool   _searchFilter:      searchText.text.trim() != "" || controller.showModifiedOnly  ///< true: showing results of search
    property var    _searchResults      ///< List of parameter names from search results
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _showRCToParam:     _activeVehicle.px4Firmware
    property var    _appSettings:       QGroundControl.settingsManager.appSettings
    property var    _controller:        controller
    readonly property var _importState: controller.parameterImportState
    property var _importLifetime: ({ alive: true })

    Component.onDestruction: _importLifetime.alive = false

    function _importStateText(state) {
        switch (state) {
        case "queued": return qsTr("Queued")
        case "sending": return qsTr("Sending")
        case "waiting": return qsTr("Waiting for matching reports")
        case "reports-matched": return qsTr("Matching reports observed (not an acknowledgement)")
        case "cancelled": return qsTr("Remaining sends stopped")
        case "source-retired": return qsTr("Stopped: original source changed")
        case "timed-out": return qsTr("Timed out")
        case "superseded": return qsTr("Stopped: parameters edited separately")
        case "finished-with-unmatched-rows": return qsTr("Finished with unmatched or superseded rows")
        case "failed": return qsTr("Stopped with an error")
        default: return qsTr("No active import")
        }
    }

    property var    _parameterHelpFact: null
    property real   _parameterHelpX: 0
    property real   _parameterHelpY: 0

    function _appendParameterHelpLine(lines, label, value) {
        if (value !== undefined && value !== null && String(value).length > 0) {
            lines.push(label + value)
        }
    }

    function parameterHelpText(fact) {
        if (!fact) return ""

        var lines = [ fact.name ]
        _appendParameterHelpLine(lines, qsTr("\u8bf4\u660e\uff1a"), fact.shortDescription)
        if (fact.longDescription && fact.longDescription !== fact.shortDescription) {
            _appendParameterHelpLine(lines, qsTr("\u8be6\u60c5\uff1a"), fact.longDescription)
        }
        _appendParameterHelpLine(lines, qsTr("\u5f53\u524d\u503c\uff1a"), fact.enumOrValueString + (fact.units ? " " + fact.units : ""))
        if (fact.defaultValueAvailable) {
            _appendParameterHelpLine(lines, qsTr("\u9ed8\u8ba4\u503c\uff1a"), fact.defaultValueString + (fact.units ? " " + fact.units : ""))
        }
        if (!fact.minIsDefaultForType || !fact.maxIsDefaultForType) {
            _appendParameterHelpLine(lines, qsTr("\u8303\u56f4\uff1a"), fact.minString + " - " + fact.maxString + (fact.units ? " " + fact.units : ""))
        }
        if (fact.enumStrings && fact.enumStrings.length > 0) {
            _appendParameterHelpLine(lines, qsTr("\u53ef\u9009\u9879\uff1a"), fact.enumStrings.join(", "))
        }
        if (fact.vehicleRebootRequired || fact.qgcRebootRequired) {
            lines.push(qsTr("\u6ce8\u610f\uff1a\u4fee\u6539\u540e\u9700\u8981\u91cd\u542f %1\u3002").arg(fact.vehicleRebootRequired ? qsTr("\u98de\u63a7") : qsTr("\u5730\u9762\u7ad9")))
        }
        return lines.join("\n")
    }

    function showParameterHelp(sourceItem, fact) {
        if (!sourceItem || !fact) return
        var pos = sourceItem.mapToItem(_root, sourceItem.width + ScreenTools.defaultFontPixelWidth, 0)
        _parameterHelpFact = fact
        _parameterHelpX = Math.min(pos.x, _root.width - parameterHelpCard.width - ScreenTools.defaultFontPixelWidth)
        _parameterHelpY = Math.max(header.height + ScreenTools.defaultFontPixelHeight * 0.5,
                                   Math.min(pos.y, _root.height - parameterHelpCard.height - ScreenTools.defaultFontPixelHeight))
    }

    function hideParameterHelp(fact) {
        if (!fact || _parameterHelpFact === fact) {
            _parameterHelpFact = null
        }
    }

    function _maintenanceVehicleAllowed(vehicle) {
        return vehicle
                && vehicle.initialConnectComplete
                && vehicle.vehicleLinkManager
                && !vehicle.vehicleLinkManager.communicationLost
                && !vehicle.armed
                && !vehicle.flying
    }

    function _showMaintenanceBlocked(reason) {
        mainWindow.showMessageDialog(qsTr("Maintenance write blocked"),
                                     reason || qsTr("Vehicle must be connected, link-ready, disarmed and on the ground."))
    }

    ParameterEditorController {
        id: controller
    }

    ExclusiveGroup { id: sectionGroup }

    //---------------------------------------------
    //-- Header
    Row {
        id:             header
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        ScreenTools.defaultFontPixelWidth

        Timer {
            id:         clearTimer
            interval:   100;
            running:    false;
            repeat:     false
            onTriggered: {
                searchText.text = ""
                controller.searchText = ""
            }
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Search:")
        }

        QGCTextField {
            id:                 searchText
            text:               controller.searchText
            onDisplayTextChanged: controller.searchText = displayText
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCButton {
            text: qsTr("Clear")
            onClicked: {
                if(ScreenTools.isMobile) {
                    Qt.inputMethod.hide();
                }
                clearTimer.start()
            }
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCCheckBox {
            text:                   qsTr("Show modified only")
            anchors.verticalCenter: parent.verticalCenter
            checked:                controller.showModifiedOnly
            onClicked:              controller.showModifiedOnly = checked
            visible:                QGroundControl.multiVehicleManager.activeVehicle.px4Firmware
        }
    } // Row - Header

    QGCFlickable {
        id: importStatusArea
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: !!_importState.runId
        height: visible ? Math.min(importStatusContent.height, Math.max(0, _root.height * 0.35)) : 0
        contentHeight: importStatusContent.height
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick
        clip: true

        Column {
            id: importStatusContent
            width: parent.width
            spacing: ScreenTools.defaultFontPixelHeight * 0.25

            QGCLabel {
                width: parent.width
                wrapMode: Text.Wrap
                text: _importState.runId ? qsTr("Parameter import — %1\nRun: %2\nRows reaching the link writer: %3/%4; pending: %5; matching reports: %6; timed out: %7; superseded: %8.")
                    .arg(_importStateText(_importState.state)).arg(_importState.runId)
                    .arg(_importState.enteredRowCount).arg(_importState.rowCount)
                    .arg(_importState.pendingCount).arg(_importState.matchingReportCount)
                    .arg(_importState.timedOutCount).arg(_importState.supersededCount) : ""
            }
            QGCLabel {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("A matching report does not prove that this import caused it. Stopping cannot undo writes already submitted. Verify vehicle values before use.")
            }
            QGCLabel {
                width: parent.width
                wrapMode: Text.Wrap
                visible: !!_importState.reason
                text: _importState.reason || ""
            }
            QGCButton {
                id: stopImportButton
                width: parent.width
                implicitHeight: Math.max(ScreenTools.implicitButtonHeight, contentItem.implicitHeight + topPadding + bottomPadding)
                focusPolicy: Qt.StrongFocus
                text: qsTr("Stop remaining import sends")
                enabled: _importState.active === true
                QGCPalette { id: importButtonPalette; colorGroupEnabled: stopImportButton.enabled }
                contentItem: QGCLabel {
                    text: stopImportButton.text
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: stopImportButton._showHighlight ? importButtonPalette.buttonHighlightText : importButtonPalette.buttonText
                }
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: importButtonPalette.buttonText
                    border.width: 2
                    visible: stopImportButton.activeFocus
                }
                onActiveFocusChanged: {
                    if (activeFocus) importStatusArea.contentY = Math.max(0, Math.min(y, importStatusArea.contentHeight - importStatusArea.height))
                }
                onClicked: {
                    var lifetime = _importLifetime
                    var originalController = controller
                    var window = mainWindow
                    var runId = _importState.runId
                    if (!_importState.active || !runId) return
                    var unavailableTitle = qsTr("Import state changed")
                    var unavailableText = qsTr("The original import is no longer available. No later import was stopped.")
                    window.showMessageDialog(qsTr("Stop parameter import"),
                        qsTr("Stop remaining sends and retries for import %1? Parameters already sent are not rolled back.").arg(runId),
                        StandardButton.Cancel | StandardButton.Ok,
                        function() {
                            if (!lifetime.alive || !window) return
                            var stopped = originalController && originalController.cancelParameterImport(runId)
                            if (lifetime.alive && window && !stopped) {
                                window.showMessageDialog(unavailableTitle, unavailableText)
                            }
                        })
                }
            }
        }
    }

    QGCButton {
        anchors.top:    header.top
        anchors.bottom: header.bottom
        anchors.right:  parent.right
        text:           qsTr("Tools")
        visible:        !_searchFilter
        onClicked:      toolsMenu.popup()
    }

    QGCMenu {
        id:                 toolsMenu
        QGCMenuItem {
            text:           qsTr("Refresh")
            onTriggered:	controller.refresh()
        }
        QGCMenuItem {
            text:           qsTr("Reset all to firmware's defaults")
            onTriggered: {
                if (!controller.writeAllowed()) {
                    _showMaintenanceBlocked(controller.writeBlockReason())
                    return
                }
                mainWindow.showMessageDialog(qsTr("Reset All"),
                                             qsTr("Select Reset to reset all parameters to their defaults.\n\nNote that this will also completely reset everything, including UAVCAN nodes, all vehicle settings, setup and calibrations."),
                                             StandardButton.Cancel | StandardButton.Reset,
                                             function() {
                                                 if (!controller.resetAllToDefaults()) {
                                                     _showMaintenanceBlocked(controller.writeBlockReason())
                                                 }
                                             })
            }
        }
        QGCMenuItem {
            text:           qsTr("Reset to vehicle's configuration defaults")
            visible:        !_activeVehicle.apmFirmware
            onTriggered: {
                if (!controller.writeAllowed()) {
                    _showMaintenanceBlocked(controller.writeBlockReason())
                    return
                }
                mainWindow.showMessageDialog(qsTr("Reset All"),
                                             qsTr("Select Reset to reset all parameters to the vehicle's configuration defaults."),
                                             StandardButton.Cancel | StandardButton.Reset,
                                             function() {
                                                 if (!controller.resetAllToVehicleConfiguration()) {
                                                     _showMaintenanceBlocked(controller.writeBlockReason())
                                                 }
                                             })
            }
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Load from file...")
            onTriggered: {
                if (!controller.writeAllowed()) {
                    _showMaintenanceBlocked(controller.writeBlockReason())
                    return
                }
                fileDialog.title =          qsTr("Load Parameters")
                fileDialog.selectExisting = true
                fileDialog.openForLoad()
            }
        }
        QGCMenuItem {
            text:           qsTr("Save to file...")
            onTriggered: {
                fileDialog.title =          qsTr("Save Parameters")
                fileDialog.selectExisting = false
                fileDialog.openForSave()
            }
        }
        QGCMenuSeparator { visible: _showRCToParam }
        QGCMenuItem {
            text:           qsTr("Clear all RC to Param")
            onTriggered: {
                if (!_maintenanceVehicleAllowed(_activeVehicle)) {
                    _showMaintenanceBlocked(qsTr("RC-to-parameter maintenance requires a connected, link-ready, disarmed vehicle on the ground."))
                    return
                }
                _activeVehicle.clearAllParamMapRC()
            }
            visible:        _showRCToParam
        }
        QGCMenuSeparator { }
        QGCMenuItem {
            text:           qsTr("Reboot Vehicle")
            onTriggered: {
                var targetVehicle = _activeVehicle
                var targetVehicleId = targetVehicle ? Number(targetVehicle.id) : -1
                if (!_maintenanceVehicleAllowed(targetVehicle)) {
                    _showMaintenanceBlocked(qsTr("Vehicle reboot requires a connected, link-ready, disarmed vehicle on the ground."))
                    return
                }
                mainWindow.showMessageDialog(qsTr("Reboot Vehicle"),
                                             qsTr("Select Ok to reboot vehicle."),
                                             StandardButton.Cancel | StandardButton.Ok,
                                             function() {
                                                 if (!_maintenanceVehicleAllowed(targetVehicle)
                                                         || _activeVehicle !== targetVehicle
                                                         || Number(targetVehicle.id) !== targetVehicleId) {
                                                     _showMaintenanceBlocked(qsTr("Vehicle reboot requires a connected, link-ready, disarmed vehicle on the ground."))
                                                     return
                                                 }
                                                 targetVehicle.rebootVehicle()
                                             })
            }
        }
    }

    /// Group buttons
    QGCFlickable {
        id :                groupScroll
        width:              ScreenTools.defaultFontPixelWidth * 25
        anchors.top:        importStatusArea.bottom
        anchors.bottom:     parent.bottom
        clip:               true
        pixelAligned:       true
        contentHeight:      groupedViewCategoryColumn.height
        flickableDirection: Flickable.VerticalFlick
        visible:            !_searchFilter

        ColumnLayout {
            id:             groupedViewCategoryColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)

            Repeater {
                model: controller.categories

                Column {
                    Layout.fillWidth:   true
                    spacing:            Math.ceil(ScreenTools.defaultFontPixelHeight * 0.25)


                    SectionHeader {
                        id:             categoryHeader
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           object.name
                        checked:        object == controller.currentCategory
                        exclusiveGroup: sectionGroup

                        onCheckedChanged: {
                            if (checked) {
                                controller.currentCategory  = object
                            }
                        }
                    }

                    Repeater {
                        model: categoryHeader.checked ? object.groups : 0

                        QGCButton {
                            width:          ScreenTools.defaultFontPixelWidth * 25
                            text:           object.name
                            height:         _rowHeight
                            checked:        object == controller.currentGroup
                            autoExclusive:  true

                            onClicked: {
                                if (!checked) _rowWidth = 10
                                checked = true
                                controller.currentGroup = object
                            }
                        }
                    }
                }
            }
        }
    }

    /// Parameter list
    QGCListView {
        id:                 editorListView
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       _searchFilter ? parent.left : groupScroll.right
        anchors.right:      parent.right
        anchors.top:        importStatusArea.bottom
        anchors.bottom:     parent.bottom
        orientation:        ListView.Vertical
        model:              controller.parameters
        cacheBuffer:        height > 0 ? height * 2 : 0
        clip:               true

        delegate: Rectangle {
            height: _rowHeight
            width:  _rowWidth
            color:  parameterMouse.containsMouse ? Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.08) : Qt.rgba(0,0,0,0)

            Row {
                id:     factRow
                spacing: Math.ceil(ScreenTools.defaultFontPixelWidth * 0.5)
                anchors.verticalCenter: parent.verticalCenter

                property Fact modelFact: object

                QGCLabel {
                    id:     nameLabel
                    width:  ScreenTools.defaultFontPixelWidth  * 20
                    text:   factRow.modelFact.name
                    clip:   true
                }

                QGCLabel {
                    id:     valueLabel
                    width:  ScreenTools.defaultFontPixelWidth  * 20
                    color:  factRow.modelFact.defaultValueAvailable ? (factRow.modelFact.valueEqualsDefault ? qgcPal.text : qgcPal.warningText) : qgcPal.text
                    text:   {
                        if(factRow.modelFact.enumStrings.length === 0) {
                            return factRow.modelFact.valueString + " " + factRow.modelFact.units
                        }

                        if(factRow.modelFact.bitmaskStrings.length != 0) {
                            return factRow.modelFact.selectedBitmaskStrings.join(',')
                        }

                        return factRow.modelFact.enumStringValue
                    }
                    clip:   true
                }

                QGCLabel {
                    text:   factRow.modelFact.shortDescription
                }

                Component.onCompleted: {
                    if(_rowWidth < factRow.width + ScreenTools.defaultFontPixelWidth) {
                        _rowWidth = factRow.width + ScreenTools.defaultFontPixelWidth
                    }
                }
            }

            Rectangle {
                width:  _rowWidth
                height: 1
                color:  qgcPal.text
                opacity: 0.15
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
            }

            MouseArea {
                id:                 parameterMouse
                anchors.fill:       parent
                acceptedButtons:    Qt.LeftButton
                hoverEnabled:       true
                onEntered:          _root.showParameterHelp(parent, factRow.modelFact)
                onPositionChanged:  _root.showParameterHelp(parent, factRow.modelFact)
                onExited:           _root.hideParameterHelp(factRow.modelFact)
                onClicked: {
                    _editorDialogFact = factRow.modelFact
                    editorDialogComponent.createObject(mainWindow).open()
                }
            }
        }
    }

    Rectangle {
        id: parameterHelpCard
        z: 100000
        x: _parameterHelpX
        y: _parameterHelpY
        width: Math.min(ScreenTools.defaultFontPixelWidth * 52, _root.width * 0.42)
        height: Math.min(helpText.implicitHeight + ScreenTools.defaultFontPixelHeight * 1.4, _root.height * 0.45)
        radius: 7
        visible: _parameterHelpFact !== null
        color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.98)
        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.22)
        border.width: 1

        QGCFlickable {
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            contentWidth: width
            contentHeight: helpText.implicitHeight
            clip: true

            QGCLabel {
                id: helpText
                width: parent.width
                text: _root.parameterHelpText(_parameterHelpFact)
                color: qgcPal.text
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }
    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings.parameterSavePath
        nameFilters:    [ qsTr("Parameter Files (*.%1)").arg(_appSettings.parameterFileExtension) , qsTr("All Files (*)") ]

        onAcceptedForSave: {
            controller.saveToFile(file)
            close()
        }

        onAcceptedForLoad: {
            var lifetime = _importLifetime
            var originalController = controller
            var originalComponent = parameterDiffDialog
            var window = mainWindow
            var selectedFile = file
            var failureTitle = qsTr("Parameter import blocked")
            var failureText = qsTr("The parameter file could not be loaded. Check the file and reload it for the current vehicle.")
            close()
            if (!lifetime.alive || !originalController || !window) return
            var loaded = originalController.buildDiffFromFile(selectedFile)
            if (!lifetime.alive || !originalController || !window) return
            if (loaded) {
                var token = originalController.diffToken
                if (!token || !originalComponent) return
                var dialog = originalComponent.createObject(window, { "paramController": originalController, "previewToken": token })
                if (!lifetime.alive || !originalController || originalController.diffToken !== token) {
                    if (dialog) dialog.destroy()
                    return
                }
                if (dialog) dialog.open()
                else if (window) window.showMessageDialog(failureTitle, failureText)
            } else {
                window.showMessageDialog(failureTitle, originalController.diffError || failureText)
            }
        }
    }

    Component {
        id: editorDialogComponent

        ParameterEditorDialog {
            fact:           _editorDialogFact
            showRCToParam:  _showRCToParam
        }
    }

    Component {
        id: parameterDiffDialog

        ParameterDiffDialog {
            paramController: _controller
        }
    }
}

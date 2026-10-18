import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts   1.11
import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

// Read-only operational memory view. The panel renders historical observations
// and derived estimates; it has no action handler and cannot send a command.
Item {
    id: root

    property var memory: null
    readonly property var summary: memory ? memory.metrics : ({})
    readonly property var points: memory ? memory.mapPoints : []
    readonly property real rowHeight: Math.max(25, ScreenTools.defaultFontPixelHeight * 1.9)

    implicitHeight: Math.max(122, 78 + Math.min(4, points.length) * rowHeight)
    visible: memory !== null

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function metricText(value, suffix) {
        if (value === undefined || value === null || Number(value) < 0) return qsTr("未知")
        return Number(value).toFixed(1) + (suffix || "")
    }

    function categoryLabel(category) {
        if (category === "link_blind_spot") return qsTr("链路盲区")
        if (category === "gnss_instability") return qsTr("GNSS 不稳")
        if (category === "wind") return qsTr("风场")
        if (category === "energy_hotspot") return qsTr("高耗电")
        if (category === "anomaly") return qsTr("异常")
        return qsTr("遥测")
    }

    function categoryColor(category) {
        if (category === "link_blind_spot" || category === "gnss_instability") return qgcPal.colorOrange
        if (category === "anomaly") return qgcPal.colorRed
        if (category === "energy_hotspot") return qgcPal.buttonHighlight
        return qgcPal.colorBlue
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.78)
        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.14)
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("运营记忆地图")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: 9
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("%1 条观测 · %2 个位置点 · 只读").arg(root.summary.observationCount || 0).arg(root.summary.mapPointCount || 0)
                    color: qgcPal.colorGrey
                    horizontalAlignment: Text.AlignRight
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 5
                rowSpacing: 4
                Repeater {
                    model: [
                        { label: qsTr("链路盲区"), value: root.summary.linkBlindSpotCellCount || 0, suffix: qsTr(" 区") },
                        { label: qsTr("GNSS 不稳"), value: root.summary.gnssInstabilityCellCount || 0, suffix: qsTr(" 区") },
                        { label: qsTr("风场样本"), value: root.summary.windSampleCount || 0, suffix: qsTr(" 条") },
                        { label: qsTr("高耗电"), value: root.summary.energyHotspotCellCount || 0, suffix: qsTr(" 区") },
                        { label: qsTr("完成率"), value: root.summary.taskCompletionRate, suffix: qsTr("%") },
                        { label: qsTr("人工干预"), value: root.summary.manualInterventionCount || 0, suffix: qsTr(" 次") },
                        { label: qsTr("响应"), value: root.summary.responseTimeMs, suffix: qsTr(" ms") },
                        { label: qsTr("能耗"), value: root.summary.energyConsumedWh, suffix: qsTr(" Wh") },
                        { label: qsTr("覆盖"), value: root.summary.coveragePercent, suffix: qsTr("%") }
                    ]
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 31
                        radius: 4
                        color: Qt.rgba(qgcPal.windowShadeDark.r, qgcPal.windowShadeDark.g, qgcPal.windowShadeDark.b, 0.72)
                        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.10)
                        Column {
                            anchors.centerIn: parent
                            spacing: 1
                            QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: qgcPal.colorGrey; font.pointSize: 7 }
                            QGCLabel { anchors.horizontalCenter: parent.horizontalCenter; text: root.metricText(modelData.value, modelData.suffix); color: qgcPal.text; font.bold: true; font.pointSize: 9 }
                        }
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(4, root.points.length) * root.rowHeight
                clip: true
                spacing: 3
                model: root.points
                delegate: Rectangle {
                    width: ListView.view.width
                    height: root.rowHeight
                    radius: 4
                    color: Qt.rgba(root.categoryColor(modelData.category).r,
                                   root.categoryColor(modelData.category).g,
                                   root.categoryColor(modelData.category).b, 0.10)
                    border.color: Qt.rgba(root.categoryColor(modelData.category).r,
                                          root.categoryColor(modelData.category).g,
                                          root.categoryColor(modelData.category).b, 0.35)
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 5
                        Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 14; radius: 2; color: root.categoryColor(modelData.category) }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: root.categoryLabel(modelData.category)
                                  + (modelData.vehicleId > 0 ? qsTr(" · UAV-%1").arg(modelData.vehicleId) : "")
                            color: qgcPal.text
                            font.pointSize: 8
                            elide: Text.ElideRight
                        }
                        QGCLabel {
                            text: modelData.isEstimate ? qsTr("估算") : qsTr("观测")
                            color: modelData.isEstimate ? qgcPal.colorOrange : qgcPal.colorGrey
                            font.pointSize: 7
                        }
                    }
                }
            }
        }
    }
}

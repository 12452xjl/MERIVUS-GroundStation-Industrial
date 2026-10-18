import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts   1.11

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

// Read-only Fleet OS catalog summary. It deliberately receives value
// snapshots instead of QObject registries, so this surface cannot register,
// import, enable or remove templates/extensions.
Item {
    id: root

    property var templates: []
    property var extensions: []
    property var autonomyPolicy: ({})
    property bool expanded: false

    readonly property int templateCount: templates && templates.length ? templates.length : 0
    readonly property int extensionCount: extensions && extensions.length ? extensions.length : 0
    readonly property var stages: autonomyPolicy && autonomyPolicy.stages ? autonomyPolicy.stages : []
    readonly property var adapters: autonomyPolicy && autonomyPolicy.platformAdapters
                                  ? autonomyPolicy.platformAdapters : []

    implicitHeight: expanded ? 194 : 40
    visible: templateCount > 0 || extensionCount > 0 || stages.length > 0

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function templateText(item) {
        if (!item) return ""
        var name = item.displayName || item.id || qsTr("未命名模板")
        var version = item.version ? " v" + item.version : ""
        var caps = item.requiredCapabilities && item.requiredCapabilities.length
                   ? qsTr(" · 能力 %1").arg(item.requiredCapabilities.join(", ")) : ""
        return qsTr("模板：%1%2%3").arg(name).arg(version).arg(caps)
    }

    function adapterSummary() {
        if (adapters.length === 0) return qsTr("平台适配器：无声明")
        var unverified = 0
        for (var i = 0; i < adapters.length; i++) {
            var status = String(adapters[i] && adapters[i].signatureStatus || "unknown").toLowerCase()
            if (status !== "verified") unverified++
        }
        return qsTr("平台适配器：%1 · 未验证签名/沙箱 %2 · 不释放命令")
                .arg(adapters.length).arg(unverified)
    }

    function extensionText(item) {
        if (!item) return ""
        var name = item.displayName || item.extensionId || qsTr("未命名扩展")
        var version = item.version ? " v" + item.version : ""
        var type = item.extensionType ? qsTr(" · 类型 %1").arg(item.extensionType) : ""
        var caps = item.capabilities && item.capabilities.length
                   ? qsTr(" · 能力 %1").arg(item.capabilities.join(", ")) : ""
        return qsTr("扩展：%1%2%3%4 · 不加载二进制").arg(name).arg(version).arg(type).arg(caps)
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.72)
        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.14)
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("Fleet OS 目录")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: 9
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("模板 %1 · 扩展 %2 · 仅声明/审阅").arg(root.templateCount).arg(root.extensionCount)
                    color: qgcPal.colorGrey
                    horizontalAlignment: Text.AlignRight
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
                QGCButton {
                    text: root.expanded ? qsTr("收起") : qsTr("详情")
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 25
                    onClicked: root.expanded = !root.expanded
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: root.expanded
                Layout.preferredHeight: root.expanded ? implicitHeight : 0
                spacing: 3

                Repeater {
                    model: Math.min(3, root.templateCount)
                    delegate: QGCLabel {
                        Layout.fillWidth: true
                        text: root.templateText(root.templates[index])
                        color: qgcPal.text
                        font.pointSize: 8
                        elide: Text.ElideRight
                    }
                }

                Repeater {
                    model: Math.min(2, root.extensionCount)
                    delegate: QGCLabel {
                        Layout.fillWidth: true
                        text: root.extensionText(root.extensions[index])
                        color: qgcPal.text
                        font.pointSize: 8
                        elide: Text.ElideRight
                    }
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: root.adapterSummary()
                    color: qgcPal.colorOrange
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("分级自主：%1 个阶段 · 当前结果只读，flightCommandReleased=false")
                          .arg(root.stages.length)
                    color: qgcPal.colorGrey
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
            }
        }
    }
}

import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts   1.11
import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

// Read-only continuity queue. It displays planner suggestions and deliberately
// has no action handler that can mutate a task or send a flight command.
Item {
    id: root

    property var planner: null
    readonly property var plan: planner && planner.lastPlan ? planner.lastPlan : ({})
    readonly property var suggestions: planner && planner.suggestions ? planner.suggestions : []
    readonly property int maxVisibleSuggestions: 6
    readonly property var orderedSuggestions: sortSuggestions(suggestions)
    readonly property real rowHeight: Math.max(60, ScreenTools.defaultFontPixelHeight * 4.4)
    readonly property real panelHeight: Math.max(70, 48
                                                   + Math.min(maxVisibleSuggestions,
                                                              orderedSuggestions.length) * rowHeight
                                                   + (orderedSuggestions.length > maxVisibleSuggestions ? 22 : 0))

    implicitHeight: panelHeight
    visible: planner !== null

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function severityColor(severity) {
        if (severity === "critical") return qgcPal.colorRed
        if (severity === "high") return qgcPal.colorOrange
        return qgcPal.colorGreen
    }

    function severityRank(severity) {
        if (severity === "critical") return 3
        if (severity === "high") return 2
        if (severity === "medium") return 1
        return 0
    }

    function sortSuggestions(source) {
        var result = []
        if (!source || source.length === undefined) return result
        for (var i = 0; i < source.length; i++) {
            if (source[i]) result.push(source[i])
        }
        result.sort(function(left, right) {
            var severityDelta = severityRank(String(right.severity || ""))
                    - severityRank(String(left.severity || ""))
            if (severityDelta !== 0) return severityDelta
            var leftRevision = Number(left.sourceTaskRevision)
            var rightRevision = Number(right.sourceTaskRevision)
            if (isFinite(leftRevision) && isFinite(rightRevision)
                    && leftRevision !== rightRevision) {
                return rightRevision - leftRevision
            }
            var leftId = String(left.suggestionId || "")
            var rightId = String(right.suggestionId || "")
            return leftId === rightId ? 0 : (leftId < rightId ? -1 : 1)
        })
        return result
    }

    function actionLabel(action) {
        if (action === "replace_leader") return qsTr("接替 Leader")
        if (action === "reallocate_task") return qsTr("重分配任务")
        if (action === "split_task") return qsTr("拆分任务")
        if (action === "recover_task") return qsTr("恢复任务")
        if (action === "mid_mission_join") return qsTr("中途加入")
        if (action === "hold_for_operator") return qsTr("等待人工决策")
        return action || qsTr("连续性建议")
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
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("任务连续性")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: 9
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: root.suggestions.length > 0
                          ? qsTr("%1 条建议 · 需人工确认").arg(root.suggestions.length)
                          : qsTr("暂无连续性建议")
                    color: root.suggestions.length > 0 ? qgcPal.colorOrange : qgcPal.colorGrey
                    horizontalAlignment: Text.AlignRight
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
            }

            ListView {
                id: suggestionList
                Layout.fillWidth: true
                Layout.preferredHeight: root.orderedSuggestions.length > 0
                                      ? Math.min(root.maxVisibleSuggestions,
                                                 root.orderedSuggestions.length) * root.rowHeight
                                      : 0
                Layout.minimumHeight: root.orderedSuggestions.length > 0 ? root.rowHeight : 0
                clip: true
                interactive: root.orderedSuggestions.length > root.maxVisibleSuggestions
                spacing: 5
                model: root.orderedSuggestions
                delegate: Rectangle {
                    property var suggestion: modelData
                    width: suggestionList.width
                    height: root.rowHeight
                    radius: 4
                    color: Qt.rgba(root.severityColor(suggestion.severity).r,
                                   root.severityColor(suggestion.severity).g,
                                   root.severityColor(suggestion.severity).b, 0.10)
                    border.color: Qt.rgba(root.severityColor(suggestion.severity).r,
                                          root.severityColor(suggestion.severity).g,
                                          root.severityColor(suggestion.severity).b, 0.42)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 7
                        anchors.rightMargin: 7
                        anchors.topMargin: 4
                        anchors.bottomMargin: 4
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Rectangle {
                                Layout.preferredWidth: 4
                                Layout.preferredHeight: 16
                                radius: 2
                                color: root.severityColor(suggestion.severity)
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: root.actionLabel(suggestion.action)
                                      + (suggestion.taskId
                                         ? qsTr(" · %1").arg(suggestion.taskId) : "")
                                color: qgcPal.text
                                font.pointSize: 8
                                elide: Text.ElideRight
                            }
                            QGCLabel {
                                text: suggestion.severity || qsTr("待复核")
                                color: root.severityColor(suggestion.severity)
                                font.pointSize: 8
                            }
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: (suggestion.message || suggestion.reasonCode || qsTr("需人工复核"))
                                  + (suggestion.sourceTaskRevision !== undefined
                                     ? qsTr(" · rev %1").arg(suggestion.sourceTaskRevision) : "")
                                  + (suggestion.sourceFleetFingerprint
                                     ? qsTr(" · fleet %1").arg(String(suggestion.sourceFleetFingerprint).slice(0, 8)) : "")
                            color: qgcPal.colorGrey
                            font.pointSize: 7
                            elide: Text.ElideRight
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            visible: (suggestion.affectedVehicleIds
                                      && suggestion.affectedVehicleIds.length > 0)
                                  || (suggestion.candidateVehicleIds
                                      && suggestion.candidateVehicleIds.length > 0)
                            text: qsTr("受影响：%1 · 候选：%2")
                                  .arg((suggestion.affectedVehicleIds || []).join(","))
                                  .arg((suggestion.candidateVehicleIds || []).join(","))
                            color: qgcPal.colorGrey
                            font.pointSize: 7
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: root.orderedSuggestions.length > root.maxVisibleSuggestions
                text: qsTr("还有 %1 条建议；可滚动查看，完整应用仍需人工审阅。")
                      .arg(root.orderedSuggestions.length - root.maxVisibleSuggestions)
                color: qgcPal.colorOrange
                font.pointSize: 7
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
            }
        }
    }
}

import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Layouts   1.11
import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

// Read-only digital-twin preview.  It consumes a snapshot and displays
// deterministic review data; no button in this component dispatches a flight
// command or mutates a mission.
Item {
    id: root

    property var twin: null
    readonly property var preview: twin && twin.lastPreview ? twin.lastPreview : ({})
    readonly property var fleetSummary: preview.fleetSummary || ({})
    readonly property var missionSummary: preview.missionSummary || ({})
    readonly property var linkSummary: preview.linkSummary || ({})
    readonly property var energySummary: preview.energySummary || ({})
    readonly property var planningTimeline: preview.planningTimeline || ({})
    readonly property var suggestions: preview.faultSuggestions || []
    readonly property real rowHeight: Math.max(24, ScreenTools.defaultFontPixelHeight * 1.9)
    property var _pageState: ({})
    property var _lifetime: ({alive:true})
    readonly property var pageRows: _pageState.ok === true ? (_pageState.rows || []) : []

    implicitHeight: contentColumn.implicitHeight + 16
    visible: twin !== null && twin.hasSnapshot
    onPreviewChanged: _pageState = ({})
    onTwinChanged: _pageState = ({})
    Component.onDestruction: _lifetime.alive = false

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function severityColor(severity) {
        if (severity === "critical") return qgcPal.colorRed
        if (severity === "major") return qgcPal.colorOrange
        return qgcPal.colorGrey
    }

    function secondsText(value) {
        return typeof value === "number" && isFinite(value) && value >= 0
                ? value.toFixed(2) + qsTr(" 秒") : qsTr("未知")
    }

    function eventText(row) {
        var phase = row.phase === "return" ? qsTr("返程") : qsTr("出程")
        var kind = row.kind === "origin" ? qsTr("首规划点")
                 : row.kind === "dwell" ? qsTr("驻留") : qsTr("航段")
        return qsTr("%1 号机 · %2 · %3 · 点 %4\n相对时间 %5 → %6；本段 %7%8")
                .arg(row.vehicleId).arg(phase).arg(kind).arg(row.pointIndex)
                .arg(secondsText(row.startSeconds)).arg(secondsText(row.endSeconds))
                .arg(secondsText(row.durationSeconds))
                .arg((row.reason ? "\n" + row.reason : "") + "\n" + energyText(row.vehicleId))
    }

    function energyText(vehicleId) {
        var members = planningTimeline.members || []
        for (var index = 0; index < members.length && index < 32; ++index) {
            if (members[index].vehicleId !== vehicleId) continue
            var budget = members[index].batteryEvidence || {}
            var projected = budget.projectedBatteryPercent
            var projectionText = budget.projectionKnown === true && typeof projected === "number"
                    && isFinite(projected) && projected >= 0 && projected <= 100
                    ? projected.toFixed(1) + "%" : qsTr("未知")
            return qsTr("原计划能量预算：可用时长 %1；预计剩余电量 %2（不是实时遥测）")
                    .arg(budget.energyEvidenceKnown === true ? secondsText(budget.availableFlightSeconds) : qsTr("未知"))
                    .arg(projectionText)
        }
        return qsTr("原计划能量预算未知")
    }

    function readPage(offset) {
        var lifetime = _lifetime, owner = twin
        var version = preview.snapshotVersion, hash = preview.previewHash
        if (!lifetime.alive || !owner || owner.busy || planningTimeline.available !== true) return
        var result
        try { result = owner.timelinePage(version, hash, offset) }
        catch (error) { result = {ok:false, error:"timeline-page-read-failed"} }
        if (!lifetime.alive || owner !== twin || version !== preview.snapshotVersion || hash !== preview.previewHash) return
        _pageState = result && result.ok === true ? result
                    : {ok:false, error:result && result.error ? result.error : "timeline-page-unavailable"}
    }

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.72)
        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.14)
        border.width: 1

        ColumnLayout {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("任务数字孪生")
                    color: qgcPal.text
                    font.bold: true
                    font.pointSize: 9
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: preview.deterministic ? qsTr("确定性预演 · 快照 v%1").arg(preview.snapshotVersion || twin.snapshotVersion)
                                                 : qsTr("等待预演")
                    color: preview.deterministic ? qgcPal.colorGreen : qgcPal.colorGrey
                    horizontalAlignment: Text.AlignRight
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
                QGCButton {
                    text: twin && twin.busy ? qsTr("正在捕获…") : qsTr("捕获计划")
                    enabled: twin !== null && !twin.busy
                    onClicked: twin.preview()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                QGCLabel {
                    Layout.fillWidth: true
                    text: twin && twin.capturedPlan ? qsTr("计划成员 %1").arg(planningTimeline.memberCount || 0)
                          : qsTr("机队 %1/%2 在线").arg(fleetSummary.onlineCount || 0).arg(fleetSummary.vehicleCount || 0)
                    color: qgcPal.text
                    font.pointSize: 8
                    elide: Text.ElideRight
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: twin && twin.capturedPlan ? qsTr("规划事件 %1").arg(planningTimeline.eventCount || 0)
                          : qsTr("航点 %1 · 风险 %2").arg(missionSummary.waypointCount || 0).arg(preview.riskCount || 0)
                    color: (preview.riskCount || 0) > 0 ? qgcPal.colorOrange : qgcPal.text
                    font.pointSize: 8
                    horizontalAlignment: Text.AlignHCenter
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: twin && twin.capturedPlan ? qsTr("能量预算按机显示") : qsTr("链路 %1 · 能量 %2")
                          .arg(linkSummary.average >= 0 ? Math.round(linkSummary.average) + "%" : "?")
                          .arg(energySummary.average >= 0 ? Math.round(energySummary.average) + "%" : "?")
                    color: qgcPal.text
                    font.pointSize: 8
                    horizontalAlignment: Text.AlignRight
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: twin && twin.capturedPlan
                      ? (twin.sourceCurrent ? qsTr("当前计划的冻结预演") : qsTr("历史预演：原计划已变化，请重新捕获"))
                        + "\n" + qsTr("相对时间从每机首规划点起算；不同飞机的 0 秒不代表同步。未建模起降、转弯、风和真实飞控动态，不是任务完成时间或飞行许可。")
                      : qsTr("先在任务规划中生成并保存审阅记录，再捕获完整计划。机队概览不是航线预演。")
                color: qgcPal.text
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: !!text
                wrapMode: Text.Wrap
                color: qgcPal.colorOrange
                text: twin && twin.error ? qsTr("捕获失败：%1。请重新审阅并保存计划后再试。").arg(twin.error)
                      : planningTimeline.error ? qsTr("时间线不可用：%1").arg(planningTimeline.error) : ""
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: planningTimeline.available === true
                wrapMode: Text.Wrap
                text: qsTr("计划 %1\n可估累计时间 %2 条；累计时间未知 %3 条。能量取同一计划记录，缺失不补默认值。")
                      .arg(planningTimeline.planId || "")
                      .arg(planningTimeline.knownTimeCount || 0).arg(planningTimeline.unknownTimeCount || 0)
                color: qgcPal.text
            }

            RowLayout {
                Layout.fillWidth: true
                visible: planningTimeline.available === true
                spacing: 8
                QGCButton {
                    text: qsTr("首页")
                    enabled: twin !== null && !twin.busy
                    onClicked: root.readPage(0)
                }
                QGCButton {
                    text: qsTr("上一页")
                    enabled: twin !== null && !twin.busy && root._pageState.ok === true && root._pageState.offset >= 4
                    onClicked: root.readPage(root._pageState.offset - 4)
                }
                QGCButton {
                    text: qsTr("下一页")
                    enabled: twin !== null && !twin.busy && root._pageState.ok === true && root._pageState.hasMore === true
                    onClicked: root.readPage(root._pageState.nextOffset)
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                visible: planningTimeline.available === true
                wrapMode: Text.Wrap
                color: qgcPal.text
                text: root._pageState.ok === true
                      ? qsTr("显示第 %1–%2 条 / 共 %3 条（按成员及原路线顺序，不是跨机同步时间排序）")
                        .arg(root.pageRows.length ? root._pageState.offset + 1 : 0)
                        .arg(root._pageState.offset + root.pageRows.length).arg(root._pageState.totalCount)
                      : root._pageState.error ? qsTr("读取失败：%1；请重新读取首页。").arg(root._pageState.error)
                        : qsTr("点击首页逐段查看，每页最多四条。")
            }

            Repeater {
                model: root.pageRows
                delegate: QGCLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: root.eventText(modelData)
                    color: qgcPal.text
                }
            }

            Repeater {
                model: Math.min(2, root.suggestions.length)
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.rowHeight
                    radius: 4
                    color: Qt.rgba(root.severityColor(root.suggestions[index].severity).r,
                                   root.severityColor(root.suggestions[index].severity).g,
                                   root.severityColor(root.suggestions[index].severity).b, 0.10)
                    border.color: Qt.rgba(root.severityColor(root.suggestions[index].severity).r,
                                          root.severityColor(root.suggestions[index].severity).g,
                                          root.severityColor(root.suggestions[index].severity).b, 0.42)
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 7
                        anchors.rightMargin: 7
                        spacing: 6
                        Rectangle {
                            Layout.preferredWidth: 4
                            Layout.preferredHeight: 15
                            radius: 2
                            color: root.severityColor(root.suggestions[index].severity)
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: (root.suggestions[index].action || qsTr("人工复核"))
                                  + (root.suggestions[index].vehicleIds && root.suggestions[index].vehicleIds.length
                                     ? qsTr(" · %1").arg(root.suggestions[index].vehicleIds.join(",")) : "")
                            color: qgcPal.text
                            font.pointSize: 8
                            elide: Text.ElideRight
                        }
                        QGCLabel {
                            text: qsTr("仅建议")
                            color: root.severityColor(root.suggestions[index].severity)
                            font.pointSize: 8
                        }
                    }
                }
            }
        }
    }
}

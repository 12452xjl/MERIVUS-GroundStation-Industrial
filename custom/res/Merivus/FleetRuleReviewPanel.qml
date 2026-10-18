import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// A session inbox, not an action dispatcher. Historical replay is separate.
ColumnLayout {
    id: root
    property var workspace: null
    signal plannerRequested()
    property var lifetime: ({alive: true, epoch: 0})
    property bool callBusy: false
    property int pageOffset: 0
    property var selected: ({})
    property var detail: ({})
    property string decisionReason: ""
    property string message: ""
    readonly property var status: workspace ? workspace.status || {} : ({})
    readonly property var rows: workspace ? workspace.rows || [] : []
    readonly property var pageRows: rows.slice(pageOffset, pageOffset + 4)
    readonly property bool ready: visible && inbox.visible && workspace && lifetime.alive && !callBusy
    readonly property bool selectionCurrent: currentRow() !== null
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    spacing: Math.max(8, ScreenTools.defaultFontPixelHeight * 0.4)
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function invalidate() {
        var guard = lifetime
        if (!guard.alive) return
        if (guard.epoch >= 9007199254740990) { guard.alive = false; return }
        guard.epoch += 1
        var epoch = guard.epoch
        selected = ({})
        if (!guard.alive || epoch !== guard.epoch) return
        detail = ({})
        if (!guard.alive || epoch !== guard.epoch) return
        decisionReason = ""
    }
    function currentRow() {
        if (!selected.reviewId || !selected.revision) return null
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].reviewId === selected.reviewId && rows[i].revision === selected.revision) return rows[i]
        return null
    }
    function capture(action, row) {
        var value = row || currentRow()
        if (!ready || !value) return null
        return {owner: workspace, epoch: lifetime.epoch, action: action,
            reviewId: value.reviewId, revision: value.revision, reason: decisionReason}
    }
    function current(intent) {
        if (!ready || !intent || intent.owner !== workspace || intent.epoch !== lifetime.epoch) return false
        for (var i = 0; i < rows.length; ++i)
            if (rows[i].reviewId === intent.reviewId && rows[i].revision === intent.revision) return true
        return false
    }
    function choose(intent) {
        if (!current(intent)) return
        var guard = lifetime, epoch = guard.epoch
        selected = {reviewId: intent.reviewId, revision: intent.revision}
        if (!guard.alive || epoch !== guard.epoch || workspace !== intent.owner) return
        decisionReason = ""
        if (!guard.alive || epoch !== guard.epoch || workspace !== intent.owner) return
        readDetail(0)
    }
    function readDetail(offset) {
        if (!ready || !selectionCurrent) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch
        var id = selected.reviewId, revision = selected.revision
        var result = owner.detail(id, revision, offset)
        if (!guard.alive || epoch !== guard.epoch || owner !== workspace
                || id !== selected.reviewId || revision !== selected.revision) return
        detail = result || ({})
        if (!guard.alive || epoch !== guard.epoch) return
        if (detail.ok !== true) message = qsTr("详情已失效或不可读取，请重新选择建议。")
    }
    function decide(intent) {
        if (!current(intent) || intent.reviewId !== selected.reviewId || intent.revision !== selected.revision
                || intent.reason !== decisionReason || intent.reason.trim().length === 0) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch
        callBusy = true
        try {
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace || !visible || !inbox.visible) return
            var outcome = owner.decide(intent.reviewId, intent.revision, intent.action, intent.reason)
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace) return
            if (outcome && outcome.receipt && outcome.receipt.committed === true) {
                invalidate()
                if (!guard.alive || owner !== workspace || guard.epoch !== epoch + 1) return
                message = qsTr("已处理并移出本次会话待审箱；没有执行建议动作。日志接收：%1；证据链接收：%2。接收不等于已保存到磁盘。")
                    .arg(outcome.journalRecorded === true ? qsTr("是") : qsTr("未确认"))
                    .arg(outcome.evidenceRecorded === true ? qsTr("是") : qsTr("未确认"))
            } else message = qsTr("未确认处理成功：%1。请重新选择并核对待审箱。")
                    .arg(outcome && outcome.receipt ? String(outcome.receipt.error || "unknown") : "unknown")
        } catch (error) {
            if (guard.alive && owner === workspace && epoch === guard.epoch)
                message = qsTr("处理回执未知；请核对待审箱，不要假定未处理。")
        } finally { if (guard.alive) callBusy = false }
    }
    function focusVisible(item) {
        if (!lifetime.alive || !inbox.visible || !item) return
        var point = item.mapToItem(scroller.contentItem, 0, 0)
        if (point.y < scroller.contentY) scroller.contentY = Math.max(0, point.y - 8)
        else if (point.y + item.height > scroller.contentY + scroller.height)
            scroller.contentY = Math.max(0, Math.min(scroller.contentHeight - scroller.height,
                point.y + item.height - scroller.height + 8))
    }
    onWorkspaceChanged: invalidate()
    onVisibleChanged: if (!visible) { var guard = lifetime; invalidate(); if (guard.alive) inbox.close() }
    onRowsChanged: {
        var guard = lifetime
        if (pageOffset >= rows.length) pageOffset = Math.max(0, Math.floor((rows.length - 1) / 4) * 4)
        if (guard.alive && !callBusy && selected.reviewId && !selectionCurrent) invalidate()
    }
    onCallBusyChanged: if (!callBusy && lifetime.alive && selected.reviewId && !selectionCurrent) invalidate()
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        id: openButton
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
        focusPolicy: Qt.StrongFocus
        enabled: root.workspace !== null
        text: qsTr("规则建议待审箱（%1）").arg(root.status.pendingCount || 0)
        contentItem: QGCLabel { text: openButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: openButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
        onClicked: inbox.open()
    }
    Controls.Popup {
        id: inbox
        parent: Controls.Overlay.overlay
        modal: true; focus: true; closePolicy: Controls.Popup.CloseOnEscape
        padding: ScreenTools.defaultFontPixelWidth
        width: parent ? Math.max(0, Math.min(parent.width - padding * 2, ScreenTools.defaultFontPixelWidth * 76)) : 0
        height: parent ? Math.max(0, Math.min(parent.height - padding * 2, ScreenTools.defaultFontPixelHeight * 34)) : 0
        x: parent ? (parent.width - width) / 2 : 0
        y: parent ? (parent.height - height) / 2 : 0
        onClosed: root.invalidate()
        contentItem: ColumnLayout {
            spacing: root.spacing
            QGCLabel { Layout.fillWidth: true; text: qsTr("规则建议 · 人工审阅"); font.bold: true; wrapMode: Text.Wrap }
            Flickable {
                id: scroller
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0
                clip: true; contentWidth: width; contentHeight: body.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                Controls.ScrollBar.vertical: Controls.ScrollBar { }
                ColumnLayout {
                    id: body
                    width: Math.max(0, scroller.width - ScreenTools.defaultFontPixelWidth)
                    spacing: root.spacing
                    QGCLabel {
                        Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText
                        text: qsTr("本次会话保留尚未处理的实时建议，重启不恢复。回放不进入待审箱。标记已审阅或拒绝都不是批准，不会改变飞机、任务或草案。")
                    }
                    QGCLabel {
                        Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText
                        text: qsTr("待处理：%1；拒绝接收入箱次数：%2\n最近接收问题：%3")
                            .arg(root.status.pendingCount || 0).arg(String(root.status.overflowCount || "0"))
                            .arg(String(root.status.lastError || qsTr("无")))
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.rows.length === 0; wrapMode: Text.Wrap; text: qsTr("没有待审建议。空列表不代表飞机或任务没有风险。") }
                    Repeater {
                        model: root.pageRows
                        delegate: QGCButton {
                            id: suggestionButton
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                            focusPolicy: Qt.StrongFocus
                            property var pressedIntent: null
                            enabled: root.ready
                            text: String(modelData.ruleId) + " · " + String(modelData.eventType) + "\n" + String(modelData.actionType)
                            contentItem: QGCLabel { text: suggestionButton.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: suggestionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onPressed: pressedIntent = root.capture("inspect", modelData)
                            onCanceled: pressedIntent = null
                            onClicked: { var guard = root.lifetime, intent = pressedIntent; pressedIntent = null; if (guard.alive) root.choose(intent) }
                            onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: root.spacing
                        QGCButton { text: qsTr("上一页"); Layout.minimumHeight: root.rowHeight; enabled: root.ready && root.pageOffset > 0; onClicked: root.pageOffset = Math.max(0, root.pageOffset - 4); onActiveFocusChanged: if (activeFocus) root.focusVisible(this) }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("%1–%2 / %3").arg(root.rows.length ? root.pageOffset + 1 : 0).arg(Math.min(root.pageOffset + 4, root.rows.length)).arg(root.rows.length); horizontalAlignment: Text.AlignHCenter }
                        QGCButton { text: qsTr("下一页"); Layout.minimumHeight: root.rowHeight; enabled: root.ready && root.pageOffset + 4 < root.rows.length; onClicked: root.pageOffset += 4; onActiveFocusChanged: if (activeFocus) root.focusVisible(this) }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.selectionCurrent; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText; text: qsTr("选中建议：%1\n原始规则、事件和动作（分片，仅供核对）：").arg(String(root.selected.reviewId || "")) }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.selectionCurrent; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText
                        text: root.detail.ok === true ? String(root.detail.text || "") : qsTr("尚无详情")
                    }
                    RowLayout {
                        visible: root.selectionCurrent; Layout.fillWidth: true; spacing: root.spacing
                        QGCButton { text: qsTr("详情首页"); Layout.minimumHeight: root.rowHeight; enabled: root.ready; onClicked: root.readDetail(0); onActiveFocusChanged: if (activeFocus) root.focusVisible(this) }
                        QGCButton { text: qsTr("详情后续片段"); Layout.minimumHeight: root.rowHeight; enabled: root.ready && root.detail.ok === true && root.detail.hasMore === true; onClicked: root.readDetail(root.detail.nextOffset); onActiveFocusChanged: if (activeFocus) root.focusVisible(this) }
                    }
                    QGCLabel { visible: root.selectionCurrent; text: qsTr("处理说明（必填，最多 512 字符）") }
                    QGCTextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: root.selectionCurrent
                        maximumLength: 512; enabled: root.ready; text: root.decisionReason
                        onTextChanged: if (root.decisionReason !== text) root.decisionReason = text
                        Accessible.name: qsTr("规则建议处理说明")
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                    }
                    Repeater {
                        model: [{action: "reviewed", label: qsTr("确认已审阅 · 不执行")}, {action: "rejected", label: qsTr("拒绝此建议 · 不执行")}]
                        delegate: QGCButton {
                            id: decisionButton
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: root.selectionCurrent
                            focusPolicy: Qt.StrongFocus
                            property var pressedIntent: null
                            text: modelData.label
                            enabled: root.ready && root.selectionCurrent && root.decisionReason.trim().length > 0
                            contentItem: QGCLabel { text: decisionButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: decisionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onPressed: pressedIntent = root.capture(modelData.action, null)
                            onCanceled: pressedIntent = null
                            onClicked: { var guard = root.lifetime, intent = pressedIntent; pressedIntent = null; if (guard.alive) root.decide(intent) }
                            onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        }
                    }
                    QGCLabel { Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText; text: root.message }
                    QGCButton {
                        id: plannerButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("前往任务规划（不载入建议动作）"); enabled: root.ready
                        contentItem: QGCLabel { text: plannerButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: plannerButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        onClicked: { var guard = root.lifetime; inbox.close(); if (guard.alive) root.plannerRequested() }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                    }
                }
            }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("关闭"); onClicked: inbox.close() }
        }
    }
}

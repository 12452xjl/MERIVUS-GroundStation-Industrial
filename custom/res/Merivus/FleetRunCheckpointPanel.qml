import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Used inside the existing report scroller. No new scroll region or authority.
ColumnLayout {
    id: root
    property var workspace: null
    property var reportConsumer: null
    signal focusRequested(var item)
    property bool expanded: false
    property bool callBusy: false
    property bool enableConsent: false
    property var page: ({})
    property string message: ""
    property var lifetime: ({alive: true, epoch: 0})
    readonly property var status: workspace ? workspace.status || {} : ({})
    readonly property var history: workspace ? workspace.history || {} : ({})
    readonly property string historyId: typeof history.historyId === "string" ? history.historyId : ""
    readonly property bool ready: lifetime.alive && visible && expanded && workspace && !callBusy
    readonly property bool busy: status.busy === true
    readonly property bool pageCurrent: page.ok === true && page.historyId === historyId
        && Array.isArray(page.records) && page.records.length <= 4
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    spacing: Math.max(8, ScreenTools.defaultFontPixelHeight * 0.4)
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function invalidate() {
        var guard = lifetime
        if (!guard.alive) return
        if (guard.epoch >= 9007199254740990) { guard.alive = false; return }
        guard.epoch += 1
        var epoch = guard.epoch
        enableConsent = false
        if (!guard.alive || epoch !== guard.epoch) return
        page = ({})
    }
    function capture(action) {
        if (!ready) return null
        return {owner: workspace, epoch: lifetime.epoch, action: action, enabled: status.enabled === true,
            jobId: status.jobId || "", consent: enableConsent}
    }
    function current(value) {
        return ready && value && value.owner === workspace && value.epoch === lifetime.epoch
            && value.enabled === (status.enabled === true)
    }
    function invoke(value) {
        if (!current(value) || (value.action === "enable" && (!value.consent || !enableConsent))
                || (value.action === "cancel" && value.jobId !== status.jobId)
                || ((value.action === "open" || value.action === "save") && busy)) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch
        callBusy = true
        try {
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace || !visible || !expanded) return
            enableConsent = false
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace || !visible || !expanded) return
            var answer = owner.request(value.action, value.jobId, true)
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace) return
            message = answer && answer.accepted === true
                ? qsTr("请求已受理。请查看原生状态；停用或取消不撤销已写入文件，也不改变飞机。")
                : qsTr("没有取得受理确认，请核对当前状态。不要把未知回执当作文件未写入。")
        } catch (error) {
            if (guard.alive && owner === workspace && epoch === guard.epoch)
                message = qsTr("请求回执未知，请核对原生状态；不自动重试。")
        } finally { if (guard.alive) callBusy = false }
    }
    function readPage(offset) {
        if (!ready || historyId.length !== 36) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch, id = historyId
        var value = owner.page(id, offset)
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || id !== historyId) return
        page = value || ({})
    }
    function showReport(value) {
        if (!ready || !value || value.owner !== workspace || value.epoch !== lifetime.epoch
                || value.historyId !== historyId || typeof reportConsumer !== "function") return
        var owner = workspace, guard = lifetime, epoch = guard.epoch, sink = reportConsumer
        callBusy = true
        try {
            if (!guard.alive || epoch !== guard.epoch || owner !== workspace || !visible || !expanded) return
            var result = owner.report(value.historyId, value.index)
            if (!guard.alive || owner !== workspace || epoch !== guard.epoch || value.historyId !== historyId || sink !== reportConsumer) return
            var shown = result && result.ok === true && sink(result) === true
            if (!guard.alive || owner !== workspace || epoch !== guard.epoch) return
            message = shown ? qsTr("历史记录已送入下方只读报告区。没有恢复任务或批准。")
                : qsTr("未采用历史报告，请核对历史快照与报告区状态后重新选择。")
        } catch (error) {
            if (guard.alive && owner === workspace && epoch === guard.epoch)
                message = qsTr("历史报告查看中断；不会据此恢复任务。")
        } finally { if (guard.alive) callBusy = false }
    }
    function statusText() {
        var value = status
        return qsTr("后台检查点：%1 · %2\n固定路径：%3\n当前代：%4；捕获代：%5；已保存代：%6\n未保存变化：%7；本次捕获来源仍当前：%8\n写入已知：%9；确认已写入：%10；读回复核：%11\n原因：%12 %13")
            .arg(value.enabled === true ? qsTr("已启用") : qsTr("未启用"))
            .arg(String(value.stage || "idle")).arg(String(value.filePath || qsTr("尚未解析")))
            .arg(String(value.currentGeneration || "0")).arg(String(value.capturedGeneration || "0")).arg(String(value.savedGeneration || "0"))
            .arg(value.dirty === true ? qsTr("有") : qsTr("无"))
            .arg(value.action === "save" ? (value.sourceCurrent === true ? qsTr("是") : qsTr("未确认"))
                : qsTr("不适用；不是历史文件当前性证明"))
            .arg(value.fileWrittenKnown === true ? qsTr("是") : qsTr("未确认"))
            .arg(value.fileWritten === true ? qsTr("是") : qsTr("未确认"))
            .arg(value.readbackVerified === true ? qsTr("通过") : qsTr("未确认"))
            .arg(String(value.error || "")).arg(String(value.readbackError || ""))
    }
    onWorkspaceChanged: invalidate()
    onVisibleChanged: if (!visible) invalidate()
    onExpandedChanged: if (!expanded) invalidate()
    onHistoryIdChanged: page = ({})
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        id: toggle
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起 Run 后台检查点") : qsTr("Run 后台检查点与重启后只读恢复")
        enabled: root.workspace !== null
        contentItem: QGCLabel { text: toggle.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: toggle._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
        onClicked: root.expanded = !root.expanded
        onActiveFocusChanged: if (activeFocus) root.focusRequested(this)
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.expanded; spacing: root.spacing
        QGCLabel {
            Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText
            text: qsTr("默认关闭。启用后将本进程已提交的全部原生 Run 观察快照合并写入固定文件，可能覆盖上一会话文件；最多 128 条、整份 32 MiB，超限整份失败。不是完整报文日志或机载执行恢复，也没有断电持久性保证。先打开历史可避免在查看前覆盖旧文件。")
        }
        Controls.CheckBox {
            id: consent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: root.enableConsent; enabled: root.ready && root.status.enabled !== true
            property var pressedContext: null
            text: qsTr("允许本次会话持续替换固定检查点文件（含已有文件）")
            contentItem: QGCLabel { text: consent.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; leftPadding: consent.indicator.width + ScreenTools.defaultFontPixelWidth }
            onPressed: pressedContext = root.capture("consent")
            onCanceled: pressedContext = null
            onClicked: {
                var guard = root.lifetime, value = pressedContext, desired = checked
                pressedContext = null
                if (guard.alive && root.current(value)) root.enableConsent = desired
                if (guard.alive) checked = Qt.binding(function() { return root.enableConsent })
            }
            onActiveFocusChanged: if (activeFocus) root.focusRequested(this)
        }
        Repeater {
            model: [{action:"enable", label:qsTr("确认启用后台检查点")},
                {action:"disable", label:qsTr("停用后续自动保存（不撤回在途写入）")},
                {action:"save", label:qsTr("明确保存／重试当前快照")},
                {action:"open", label:qsTr("打开固定检查点到独立历史槽")},
                {action:"cancel", label:qsTr("请求取消当前文件操作") }]
            delegate: QGCButton {
                id: actionButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                property var pressedIntent: null
                text: modelData.label
                enabled: root.ready && (modelData.action === "enable" ? root.status.enabled !== true && root.enableConsent && !root.busy
                    : modelData.action === "disable" ? root.status.enabled === true
                    : modelData.action === "cancel" ? root.busy
                    : modelData.action === "save" ? root.status.enabled === true && !root.busy : !root.busy)
                contentItem: QGCLabel { text: actionButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: actionButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                onPressed: pressedIntent = root.capture(modelData.action)
                onCanceled: pressedIntent = null
                onClicked: { var guard = root.lifetime, value = pressedIntent; pressedIntent = null; if (guard.alive) root.invoke(value) }
                onActiveFocusChanged: if (activeFocus) root.focusRequested(this)
            }
        }
        QGCLabel { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText; text: root.statusText() }
        QGCLabel {
            Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText
            text: root.historyId ? qsTr("独立历史：%1 条；检查点 %2；源 %3；代 %4\n来源未经认证，当前飞机状态不由该文件证明。")
                .arg(root.history.recordCount).arg(String(root.history.checkpointId)).arg(String(root.history.sourceId)).arg(String(root.history.generation))
                : qsTr("尚未明确打开历史；当前运行记录不会自动当作磁盘恢复结果。")
        }
        QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("读取历史第一页"); enabled: root.ready && root.historyId.length === 36; onClicked: root.readPage(0); onActiveFocusChanged: if (activeFocus) root.focusRequested(this) }
        Repeater {
            model: root.pageCurrent ? root.page.records : []
            delegate: QGCButton {
                id: recordButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                property var pressedIntent: null
                text: qsTr("查看 Run %1\n任务 %2 · 观察修订 %3").arg(String(modelData.runId)).arg(String(modelData.taskId)).arg(String(modelData.observationRevision))
                enabled: root.ready
                contentItem: QGCLabel { text: recordButton.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: recordButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                onPressed: pressedIntent = {owner:root.workspace,epoch:root.lifetime.epoch,historyId:root.historyId,index:modelData.index}
                onCanceled: pressedIntent = null
                onClicked: { var guard = root.lifetime, value = pressedIntent; pressedIntent = null; if (guard.alive) root.showReport(value) }
                onActiveFocusChanged: if (activeFocus) root.focusRequested(this)
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: root.spacing
            QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("上一页"); enabled: root.ready && root.pageCurrent && root.page.offset > 0; onClicked: root.readPage(Math.max(0, root.page.offset - 4)); onActiveFocusChanged: if (activeFocus) root.focusRequested(this) }
            QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("下一页"); enabled: root.ready && root.pageCurrent && root.page.hasMore === true; onClicked: root.readPage(root.page.nextOffset); onActiveFocusChanged: if (activeFocus) root.focusRequested(this) }
        }
        QGCLabel { Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText; text: root.message }
    }
}

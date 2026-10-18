import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ColumnLayout {
    id: root
    property var workspace: null
    property var lifetime: ({alive:true, epoch:0})
    property bool callBusy: false
    property bool overwriteConfirmed: false
    property string filePath: ""
    property string feedback: ""
    property var ownedJob: null
    property string ownedSnapshot: ""
    property var page: ({})
    property string audience: "commander"
    readonly property var status: workspace ? workspace.state || {} : ({})
    readonly property var summary: workspace ? workspace.summary || {} : ({})
    readonly property var lastResult: workspace ? workspace.result || {} : ({})
    readonly property bool available: visible && reportPopup.visible && workspace && lifetime.alive && !callBusy
    readonly property bool busy: status.busy === true
    readonly property bool snapshotCurrent: ownedSnapshot.length > 0 && ownedSnapshot === summary.snapshotId
        && ownedSnapshot === status.snapshotId
    readonly property string sourceStamp: JSON.stringify([status.jobId || "", status.snapshotId || ""])
    readonly property bool pageCurrent: snapshotCurrent && page.ok === true && page.snapshotId === ownedSnapshot
        && page.audience === audience && typeof page.text === "string" && page.text.length <= 2048
    spacing: ScreenTools.defaultFontPixelHeight * 0.4
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function retire() {
        var guard = lifetime
        if (!guard.alive) return
        if (guard.epoch >= 9007199254740990) { guard.alive = false; return }
        guard.epoch += 1
        var epoch = guard.epoch
        overwriteConfirmed = false
        if (!guard.alive || guard.epoch !== epoch) return
        page = ({})
    }
    function capture(action) {
        if (!available || (action !== "cancel" && busy)) return null
        return {owner:workspace, epoch:lifetime.epoch, action:action, path:filePath,
            job:status.jobId || "", snapshot:status.snapshotId || "", source:workspace.sourceToken || "",
            overwrite:overwriteConfirmed, audience:audience}
    }
    function current(intent, includeBusy) {
        return lifetime.alive && intent && intent.owner === workspace && intent.epoch === lifetime.epoch
            && visible && reportPopup.visible && intent.job === (status.jobId || "")
            && intent.snapshot === (status.snapshotId || "") && (includeBusy || !busy)
    }
    function submit(intent) {
        if (!current(intent, intent && intent.action === "cancel") || callBusy) return
        if ((intent.action === "save" || intent.action === "open") && (intent.path !== filePath || !intent.path.length)) return
        if (intent.action === "save" && (!snapshotCurrent || !intent.overwrite || !overwriteConfirmed)) return
        if (intent.action === "capture" && (!intent.source.length || intent.source !== workspace.sourceToken)) return
        var guard = lifetime, owner = intent.owner
        callBusy = true
        try {
            if (!guard.alive || owner !== workspace || !current(intent, intent.action === "cancel")) return
            overwriteConfirmed = false
            if (!guard.alive || owner !== workspace || !current(intent, intent.action === "cancel")) return
            if (intent.action === "cancel") {
                var cancelled = owner.cancel(intent.job)
                if (guard.alive && owner === workspace) feedback = cancelled
                    ? qsTr("已请求取消；等待实际终态，已提交文件不会撤销。") : qsTr("取消未受理，请查看原生状态。")
                return
            }
            if (intent.action === "clear") {
                var cleared = owner.clear(intent.snapshot)
                if (guard.alive && owner === workspace) {
                    if (cleared === true) ownedSnapshot = ""
                    if (!guard.alive || owner !== workspace) return
                    feedback = cleared ? qsTr("已清除本页冻结副本；文件和当前审阅未改变。") : qsTr("副本已变化，清除未受理。")
                }
                return
            }
            var response = intent.action === "capture" ? owner.capture(intent.source, true)
                : owner.requestFile(intent.action, intent.path, intent.job, intent.snapshot, intent.action === "save", true)
            if (!guard.alive || owner !== workspace) return
            if (!response || response.accepted !== true || typeof response.jobId !== "string" || response.jobId.length !== 36) {
                feedback = qsTr("未取得受理回执：%1。请核对路径/来源；不要据此认定文件未写入。").arg(response && response.reason ? response.reason : "unknown")
                return
            }
            ownedJob = {owner:owner, id:response.jobId, action:intent.action}
            if (!guard.alive || owner !== workspace) return
            if (intent.action !== "save") {
                page = ({})
            }
            if (guard.alive && owner === workspace) feedback = qsTr("请求已受理；仅实际完成后采用同一任务的结果。")
        } catch (error) {
            if (guard.alive && owner === workspace) feedback = qsTr("回执未知，请查看原生状态；已发生的写入不回滚。")
        } finally {
            if (guard.alive) {
                callBusy = false
                if (guard.alive) settle()
            }
        }
    }
    function settle() {
        var guard = lifetime
        if (!guard.alive || callBusy || !workspace) return
        var original = ownedJob
        if (!original || original.owner !== workspace || original.id !== status.jobId) return
        var result = lastResult
        if (busy || result.jobId !== original.id || result.action !== original.action) return
        if (result.ok === true && original.action !== "save" && result.reply && result.reply.summary
                && result.reply.summary.snapshotId === summary.snapshotId && summary.snapshotId === status.snapshotId) {
            ownedSnapshot = summary.snapshotId
            if (!guard.alive || original !== ownedJob || original.owner !== workspace) return
        }
        var saved = result.reply || {}
        feedback = original.action === "save" && typeof saved.fileWrittenKnown === "boolean"
            ? qsTr("保存终态：写入已知=%1，已提交=%2，回读相同=%3；错误=%4。非fsync/耐久保证。").arg(saved.fileWrittenKnown).arg(saved.fileWritten).arg(saved.readbackVerified).arg(result.error || "—")
            : result.ok === true ? qsTr("资料已冻结；请选择视角并点击显示首页。")
            : qsTr("未安装新资料：%1；可更正路径/重新捕获。已有旧副本仅作历史。").arg(result.error || "unknown")
    }
    function readPage(offset, intent) {
        if (!available || busy || !snapshotCurrent || !current(intent, false)
                || intent.audience !== audience) return
        var guard = lifetime, owner = workspace, snapshot = ownedSnapshot, selected = audience
        var result = owner.viewPage(snapshot, selected, offset)
        if (!guard.alive || owner !== workspace || snapshot !== ownedSnapshot || selected !== audience) return
        page = result || ({})
    }
    function reveal(item) {
        if (!lifetime.alive || !item || !reportPopup.visible) return
        var top = body.contentY + body.mapFromItem(item, 0, 0).y
        var bottom = top + item.height, target = body.contentY
        if (top < target) target = top
        else if (bottom > target + body.height) target = bottom - body.height
        body.contentY = Math.max(0, Math.min(Math.max(0, body.contentHeight - body.height), target))
    }
    onSourceStampChanged: { var guard = lifetime; retire(); if (guard.alive && !callBusy) settle() }
    onLastResultChanged: if (lifetime.alive && !callBusy) settle()
    onStatusChanged: if (lifetime.alive && !callBusy) settle()
    onWorkspaceChanged: { var guard = lifetime; retire(); if (guard.alive) ownedSnapshot = "" }
    onFilePathChanged: retire()
    onAudienceChanged: { if (lifetime.alive) page = ({}) }
    onVisibleChanged: if (!visible) retire()
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        Layout.fillWidth: true
        Layout.minimumHeight: 44
        text: qsTr("审阅报告：四类视角与文件交付")
        wrapMode: Text.WordWrap
        focusPolicy: Qt.StrongFocus
        onClicked: reportPopup.open()
    }
    Controls.Popup {
        id: reportPopup
        parent: Controls.Overlay.overlay
        x: Math.max(0, (parent.width - width) / 2)
        y: Math.max(0, (parent.height - height) / 2)
        width: Math.min(parent.width - 16, ScreenTools.defaultFontPixelWidth * 90)
        height: Math.min(parent.height - 16, ScreenTools.defaultFontPixelHeight * 38)
        padding: ScreenTools.defaultFontPixelHeight
        modal: true
        focus: true
        closePolicy: Controls.Popup.CloseOnEscape
        onClosed: root.retire()
        background: Rectangle { color: qgcPal.window; border.color: qgcPal.text }
        contentItem: ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight * 0.4
            QGCLabel { Layout.fillWidth: true; text: qsTr("飞前审阅资料 · 不是实际Run结果或事故定论"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true }
            QGCLabel { Layout.fillWidth: true; text: qsTr("完整JSON包含工程原值。客户视角只是显示裁剪，不是脱敏文件或权限控制；保存前确认接收方有权读取完整资料。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.colorOrange }
            QGCFlickable {
                id: body
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: content.implicitHeight
                flickableDirection: Flickable.VerticalFlick
                Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }
                ColumnLayout {
                    id: content
                    width: body.width
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5
                    QGCLabel { Layout.fillWidth: true; text: qsTr("当前操作：%1 / %2，job=%3。历史结果job=%4。").arg(root.status.action || "—").arg(root.status.stage || "idle").arg(root.status.jobId || "—").arg(root.lastResult.jobId || "—"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 44
                        text: qsTr("冻结当前已存档审阅报告")
                        enabled: root.available && !root.busy && root.workspace.sourceToken.length > 0
                        focusPolicy: Qt.StrongFocus
                        property var pressedSource: null
                        onActiveFocusChanged: if (activeFocus) root.reveal(this)
                        onPressed: pressedSource = root.capture("capture")
                        onCanceled: pressedSource = null
                        onClicked: { var guard = root.lifetime, original = pressedSource; pressedSource = null; if (guard.alive) root.submit(original) }
                    }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("本地JSON绝对路径（保存完整报告，重开只替换独立历史副本）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    Controls.TextField {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 44
                        maximumLength: 4096
                        text: root.filePath
                        selectByMouse: true
                        color: qgcPal.text
                        onActiveFocusChanged: if (activeFocus) root.reveal(this)
                        onTextEdited: root.filePath = text
                        background: Rectangle { color: qgcPal.windowShade; border.color: qgcPal.text }
                    }
                    Controls.CheckBox {
                        id: overwrite
                        Layout.fillWidth: true
                        Layout.minimumHeight: 44
                        checked: root.overwriteConfirmed
                        enabled: root.available && !root.busy
                        focusPolicy: Qt.StrongFocus
                        property var pressedSource: null
                        onActiveFocusChanged: if (activeFocus) root.reveal(this)
                        onPressed: pressedSource = root.capture("save")
                        onCanceled: pressedSource = null
                        onClicked: {
                            var guard = root.lifetime, original = pressedSource, chosen = checked
                            pressedSource = null
                            if (!guard.alive) return
                            if (root.current(original, false) && original.path === root.filePath) root.overwriteConfirmed = chosen
                            if (guard.alive) checked = Qt.binding(function() { return root.overwriteConfirmed })
                        }
                        contentItem: Text { text: qsTr("允许写入/覆盖上述路径的完整工程资料"); color: qgcPal.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: overwrite.indicator.width + overwrite.spacing; verticalAlignment: Text.AlignVCenter }
                    }
                    Repeater {
                        model: [{action:"save", label:qsTr("保存冻结完整JSON")}, {action:"open", label:qsTr("重开历史JSON（不导入当前任务）")}, {action:"clear", label:qsTr("清除本页冻结副本")}, {action:"cancel", label:qsTr("取消当前后台请求") }]
                        delegate: QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 44
                            text: modelData.label
                            wrapMode: Text.WordWrap
                            focusPolicy: Qt.StrongFocus
                            enabled: root.available && (modelData.action === "cancel" ? root.busy
                                : !root.busy && (modelData.action === "open" || root.snapshotCurrent)
                                    && (modelData.action !== "save" || root.overwriteConfirmed))
                            property var pressedSource: null
                            onActiveFocusChanged: if (activeFocus) root.reveal(this)
                            onPressed: pressedSource = root.capture(modelData.action)
                            onCanceled: pressedSource = null
                            onClicked: { var guard = root.lifetime, original = pressedSource; pressedSource = null; if (guard.alive) root.submit(original) }
                        }
                    }
                    QGCLabel { Layout.fillWidth: true; text: root.feedback; visible: text.length > 0; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                    QGCLabel { Layout.fillWidth: true; text: root.snapshotCurrent ? qsTr("冻结：%1\n来源：%2\n报告：%3\n计划：%4\n生成时间（文件中仅声称）：%5\n状态：%6；异常%7；未验证%8").arg(root.ownedSnapshot).arg(root.summary.sourceKind).arg(root.summary.reportId).arg(root.summary.planId).arg(root.summary.generatedAt).arg(root.summary.verdict).arg(root.summary.incidentCount).arg(root.summary.unverifiedCount) : qsTr("没有本页已采用的冻结资料；外来/旧任务结果仅供诊断，不自动领用。"); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("显示视角（不会改变保存的完整报告）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    Controls.ComboBox {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 44
                        model: [qsTr("指挥员"), qsTr("客户"), qsTr("工程师"), qsTr("安全员")]
                        focusPolicy: Qt.StrongFocus
                        onActiveFocusChanged: if (activeFocus) root.reveal(this)
                        onActivated: root.audience = ["commander", "customer", "engineer", "safety"][index]
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        QGCButton {
                            Layout.minimumHeight: 44
                            text: qsTr("显示首页"); enabled: root.available && !root.busy && root.snapshotCurrent; focusPolicy: Qt.StrongFocus
                            property var pressedSource: null
                            onActiveFocusChanged: if (activeFocus) root.reveal(this)
                            onPressed: pressedSource = root.capture("page")
                            onCanceled: pressedSource = null
                            onClicked: { var guard = root.lifetime, original = pressedSource; pressedSource = null; if (guard.alive) root.readPage(0, original) }
                        }
                        QGCButton {
                            Layout.minimumHeight: 44
                            text: qsTr("下一片"); enabled: root.available && !root.busy && root.pageCurrent && root.page.hasMore === true; focusPolicy: Qt.StrongFocus
                            property var pressedSource: null
                            property int pressedOffset: 0
                            onActiveFocusChanged: if (activeFocus) root.reveal(this)
                            onPressed: { var guard = root.lifetime, offset = root.page.nextOffset; pressedOffset = offset; if (guard.alive) pressedSource = root.capture("page") }
                            onCanceled: pressedSource = null
                            onClicked: { var guard = root.lifetime, original = pressedSource, offset = pressedOffset; pressedSource = null; if (guard.alive) root.readPage(offset, original) }
                        }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.pageCurrent; text: root.pageCurrent ? qsTr("字符片：%1–%2 / %3（不是日志连续性或全集证明）").arg(root.page.offset).arg(root.page.nextOffset).arg(root.page.totalLength) : ""; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    Controls.TextArea { Layout.fillWidth: true; readOnly: true; selectByMouse: true; text: root.pageCurrent ? root.page.text : ""; textFormat: TextEdit.PlainText; wrapMode: TextEdit.WrapAnywhere; color: qgcPal.text; onActiveFocusChanged: if (activeFocus) root.reveal(this); background: Rectangle { color: qgcPal.windowShade } }
                }
            }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: 44; text: qsTr("关闭（不撤销已提交文件）"); focusPolicy: Qt.StrongFocus; onClicked: reportPopup.close() }
        }
    }
}

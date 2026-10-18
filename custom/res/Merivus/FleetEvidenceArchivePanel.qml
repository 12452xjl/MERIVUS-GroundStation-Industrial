import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Explicit file delivery and isolated historical inspection, never an import
// into the live evidence chain or a source of flight/approval authority.
ColumnLayout {
    id: root
    property var workspace: null
    readonly property bool expanded: archivePopup.visible
    property bool callBusy: false
    property bool overwriteConfirmed: false
    property string filePath: ""
    property string message: ""
    property var pageResult: ({})
    property var selectedDetail: ({})
    property var lifetime: ({alive: true, epoch: 0})
    readonly property var status: workspace ? workspace.status || {} : ({})
    readonly property var history: workspace ? workspace.historicalSummary || {} : ({})
    readonly property string historicalId: typeof history.requestId === "string" ? history.requestId : ""
    readonly property bool ready: visible && expanded && workspace && lifetime.alive && !callBusy
    readonly property bool busy: workspace && workspace.busy === true
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property bool pageCurrent: pageResult.ok === true && pageResult.requestId === historicalId
        && Array.isArray(pageResult.records) && pageResult.records.length <= 4
    readonly property var rows: pageCurrent ? pageResult.records : []
    readonly property bool detailCurrent: selectedDetail.ok === true && selectedDetail.requestId === historicalId
    spacing: ScreenTools.defaultFontPixelHeight * 0.4
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function invalidate() {
        var guard = lifetime
        if (!guard.alive) return
        if (guard.epoch >= 9007199254740990) { guard.alive = false; return }
        guard.epoch += 1
        var epoch = guard.epoch
        overwriteConfirmed = false
        if (!guard.alive || guard.epoch !== epoch) return
        pageResult = ({})
        if (!guard.alive || guard.epoch !== epoch) return
        selectedDetail = ({})
    }
    function capture(action) {
        if (!ready || (action !== "cancel" && busy)) return null
        return {owner: workspace, epoch: lifetime.epoch, action: action, path: filePath,
            count: workspace.count, head: workspace.headHash, consent: overwriteConfirmed,
            requestId: status.requestId || ""}
    }
    function current(intent) {
        return ready && intent && intent.owner === workspace && intent.epoch === lifetime.epoch
    }
    function submit(intent) {
        if (!current(intent) || busy || intent.path !== filePath || filePath.length === 0
                || (intent.action === "save" && (!intent.consent || !overwriteConfirmed))) return
        var owner = intent.owner, guard = lifetime, epoch = guard.epoch
        callBusy = true
        try {
            if (!guard.alive || workspace !== owner || guard.epoch !== epoch || !visible || !expanded) return
            overwriteConfirmed = false
            if (!guard.alive || workspace !== owner || guard.epoch !== epoch || !visible || !expanded) return
            var requestId = owner.request(intent.action, intent.path, intent.count, intent.head, true)
            if (!guard.alive || workspace !== owner || guard.epoch !== epoch) return
            message = typeof requestId === "string" && requestId.length === 36
                ? qsTr("请求已受理；请等待下方原生文件结果。")
                : qsTr("未取得受理回执。请核对路径、证据链是否变化及下方状态；已有写入不因回执缺失而撤销。")
        } catch (error) {
            if (guard.alive && workspace === owner && guard.epoch === epoch)
                message = qsTr("文件请求回执未知；请查看原生状态，不要把它当作未写入。")
        } finally { if (guard.alive) callBusy = false }
    }
    function cancel(intent) {
        if (!current(intent) || !busy || intent.requestId !== status.requestId) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch
        callBusy = true
        try {
            if (!guard.alive || workspace !== owner || guard.epoch !== epoch || !visible || !expanded) return
            var accepted = owner.cancel(intent.requestId)
            if (guard.alive && workspace === owner && guard.epoch === epoch)
                message = accepted ? qsTr("已请求取消。已提交的文件不会撤销；等待实际终态。")
                                   : qsTr("取消未受理；操作可能已经结束，请查看结果。")
        } catch (error) {
            if (guard.alive && workspace === owner && guard.epoch === epoch)
                message = qsTr("取消回执未知；请查看文件操作状态。")
        } finally { if (guard.alive) callBusy = false }
    }
    function readPage(offset) {
        if (!ready || historicalId.length !== 36) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch, id = historicalId
        var result = owner.page(id, offset)
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || id !== historicalId) return
        pageResult = result || ({})
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || id !== historicalId) return
        selectedDetail = ({})
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || id !== historicalId) return
        if (!pageCurrent) message = qsTr("历史页不可用；请重新读取。当前活动证据链不受影响。")
    }
    function readDetail(index, expectedId) {
        if (!ready || expectedId !== historicalId || !pageCurrent) return
        var owner = workspace, guard = lifetime, epoch = guard.epoch
        var result = owner.detail(expectedId, index)
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || expectedId !== historicalId) return
        selectedDetail = result || ({})
        if (!guard.alive || owner !== workspace || epoch !== guard.epoch || expectedId !== historicalId) return
        if (!detailCurrent) message = qsTr("记录不可用，或历史快照已变化；请重新选取。")
    }
    function statusText() {
        if (!status.requestId) return qsTr("尚无文件操作。保存的是点击时冻结的本地链，不是自动持续备份。")
        var lead = qsTr("操作 %1 · %2\n路径：%3\n请求：%4")
            .arg(status.action === "save" ? qsTr("保存") : qsTr("打开历史"))
            .arg(String(status.stage || "")).arg(String(status.filePath || "")).arg(status.requestId)
        if (status.busy === true) return lead + "\n" + qsTr("后台处理中；取消请求不等于未写入。")
        if (status.action === "save") {
            lead += "\n" + (status.fileWrittenKnown !== true ? qsTr("写入结果未知。")
                : status.fileWritten === true ? qsTr("文件已写入。") : qsTr("没有确认文件写入。"))
            lead += " " + (status.readbackVerified === true ? qsTr("本次读回复核通过。") : qsTr("未确认读回复核通过。"))
            lead += "\n" + qsTr("冻结记录：%1；头哈希：%2").arg(status.snapshotCount).arg(String(status.snapshotHeadHash || ""))
        } else lead += "\n" + (status.ok === true ? qsTr("完整历史链已验证并载入独立只读槽。") : qsTr("打开未成功，保留先前历史槽。"))
        if (status.error) lead += "\n" + qsTr("原因：%1").arg(status.error)
        if (status.readbackError && status.readbackError !== status.error)
            lead += "\n" + qsTr("读回原因：%1").arg(status.readbackError)
        return lead + "\n" + qsTr("完整性检查不是身份签名、磁盘持久性或飞行放行证明。")
    }
    function focusVisible(item) {
        var guard = lifetime
        if (!guard.alive || !expanded || !item) return
        var point = item.mapToItem(scroller.contentItem, 0, 0)
        if (!guard.alive) return
        var pad = ScreenTools.defaultFontPixelHeight * 0.5
        if (point.y < scroller.contentY) scroller.contentY = Math.max(0, point.y - pad)
        else if (point.y + item.height > scroller.contentY + scroller.height)
            scroller.contentY = Math.max(0, Math.min(scroller.contentHeight - scroller.height, point.y + item.height - scroller.height + pad))
    }
    onWorkspaceChanged: { var guard = lifetime; invalidate(); if (guard.alive) message = "" }
    onVisibleChanged: if (!visible) { var guard = lifetime; invalidate(); if (guard.alive) archivePopup.close() }
    onExpandedChanged: if (!expanded) invalidate()
    onFilePathChanged: invalidate()
    onHistoricalIdChanged: { var guard = lifetime; pageResult = ({}); if (guard.alive) selectedDetail = ({}) }
    Component.onDestruction: { lifetime.alive = false }

    QGCButton {
        id: openArchive
        Layout.fillWidth: true
        Layout.minimumHeight: root.rowHeight
        focusPolicy: Qt.StrongFocus
        text: qsTr("证据链：保存与查看历史")
        enabled: root.workspace !== null
        contentItem: QGCLabel { text: openArchive.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: openArchive._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
        onClicked: archivePopup.open()
    }
    Controls.Popup {
        id: archivePopup
        parent: Controls.Overlay.overlay
        modal: true; focus: true
        closePolicy: Controls.Popup.CloseOnEscape
        padding: ScreenTools.defaultFontPixelWidth
        width: parent ? Math.max(0, Math.min(parent.width - padding * 2, ScreenTools.defaultFontPixelWidth * 76)) : 0
        height: parent ? Math.max(0, Math.min(parent.height - padding * 2, ScreenTools.defaultFontPixelHeight * 28)) : 0
        x: parent ? Math.max(0, (parent.width - width) / 2) : 0
        y: parent ? Math.max(0, (parent.height - height) / 2) : 0
        background: Rectangle { color: qgcPal.window; border.width: 1; border.color: qgcPal.text }
        onOpened: closeArchive.forceActiveFocus()
        onClosed: if (root.lifetime.alive && root.visible) openArchive.forceActiveFocus()
        contentItem: ColumnLayout {
            QGCButton {
                id: closeArchive
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("关闭（后台操作不会因此撤销）")
                contentItem: QGCLabel { text: closeArchive.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: closeArchive._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                onClicked: archivePopup.close()
            }
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
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                        text: qsTr("当前本地链：%1 条\n头哈希：%2\n打开文件不会替换当前链，也不恢复批准或任务控制权。")
                            .arg(root.workspace ? root.workspace.count : 0).arg(root.workspace ? root.workspace.headHash : "")
                    }
                    QGCLabel { text: qsTr("本地 JSON 文件的完整路径") }
                    QGCTextField {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        enabled: root.ready && !root.busy
                        maximumLength: 4096
                        text: root.filePath
                        onTextChanged: if (root.filePath !== text) root.filePath = text
                        Accessible.name: qsTr("证据链 JSON 文件完整路径")
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                    }
                    Controls.CheckBox {
                        id: overwriteBox
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        focusPolicy: Qt.StrongFocus
                        text: qsTr("保存到此路径，并允许替换同名文件")
                        checked: root.overwriteConfirmed
                        enabled: root.ready && !root.busy && root.filePath.length > 0
                        property var pressedPath: null
                        contentItem: QGCLabel { text: overwriteBox.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; verticalAlignment: Text.AlignVCenter; leftPadding: overwriteBox.indicator.width + ScreenTools.defaultFontPixelWidth }
                        indicator: Rectangle {
                            implicitWidth: ScreenTools.checkBoxIndicatorSize; implicitHeight: implicitWidth
                            x: overwriteBox.leftPadding; y: (overwriteBox.height - height) / 2
                            color: qgcPal.windowShade; border.width: 1; border.color: qgcPal.text
                            QGCColoredImage { anchors.fill: parent; anchors.margins: parent.width * 0.2; source: "/qmlimages/checkbox-check.svg"; color: qgcPal.text; visible: overwriteBox.checked }
                        }
                        onPressed: pressedPath = root.capture("consent")
                        onCanceled: pressedPath = null
                        onClicked: {
                            var guard = root.lifetime, intent = pressedPath, value = checked
                            pressedPath = null
                            if (!guard.alive) return
                            if (root.current(intent) && !root.busy && intent.path === root.filePath) {
                                root.overwriteConfirmed = value
                                if (!guard.alive) return
                            }
                            checked = Qt.binding(function() { return root.overwriteConfirmed })
                        }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        text: qsTr("冻结并保存当前证据链")
                        id: saveEvidenceButton
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        contentItem: QGCLabel { text: saveEvidenceButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: saveEvidenceButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        enabled: root.ready && !root.busy && root.overwriteConfirmed && root.filePath.length > 0
                        property var pressedIntent: null
                        onPressed: pressedIntent = root.capture("save")
                        onClicked: { var guard = root.lifetime, intent = pressedIntent; pressedIntent = null; if (guard.alive) root.submit(intent) }
                        onCanceled: pressedIntent = null
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        text: qsTr("打开为独立历史（不导入当前链）")
                        id: loadEvidenceButton
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        contentItem: QGCLabel { text: loadEvidenceButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: loadEvidenceButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        enabled: root.ready && !root.busy && root.filePath.length > 0
                        property var pressedIntent: null
                        onPressed: pressedIntent = root.capture("load")
                        onClicked: { var guard = root.lifetime, intent = pressedIntent; pressedIntent = null; if (guard.alive) root.submit(intent) }
                        onCanceled: pressedIntent = null
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        visible: root.busy
                        text: qsTr("请求取消文件操作（不撤销已写文件）")
                        id: cancelEvidenceButton
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        contentItem: QGCLabel { text: cancelEvidenceButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: cancelEvidenceButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        enabled: root.ready && root.busy && root.status.cancelRequested !== true
                        property var pressedIntent: null
                        onPressed: pressedIntent = root.capture("cancel")
                        onClicked: { var guard = root.lifetime, intent = pressedIntent; pressedIntent = null; if (guard.alive) root.cancel(intent) }
                        onCanceled: pressedIntent = null
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.message.length > 0
                        text: root.message
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                        color: qgcPal.text
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: root.statusText()
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.historicalId.length > 0
                        text: qsTr("历史只读槽：%1 条\n文件：%2\n头哈希：%3")
                            .arg(root.history.recordCount || 0).arg(String(root.history.filePath || "")).arg(String(root.history.headHash || ""))
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        text: qsTr("读取历史首页（每页 4 条）")
                        id: firstHistoryButton
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        contentItem: QGCLabel { text: firstHistoryButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: firstHistoryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                        enabled: root.ready && root.historicalId.length === 36
                        onClicked: root.readPage(0)
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.pageCurrent
                        text: qsTr("当前偏移 %1，共 %2 条；这里只浏览历史，不恢复权限。")
                            .arg(root.pageResult.offset || 0).arg(root.pageResult.totalCount || 0)
                        wrapMode: Text.Wrap
                    }
                    Repeater {
                        model: root.rows
                        delegate: QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            text: qsTr("记录 %1 · %2 · 查看详情").arg(modelData.index).arg(String(modelData.type).slice(0, 40))
                            id: historyRecordButton
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            contentItem: QGCLabel { text: historyRecordButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: historyRecordButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                            enabled: root.ready && root.pageCurrent
                            property var pressedRecord: null
                            onPressed: pressedRecord = {id: root.historicalId, index: modelData.index, owner: root.workspace, epoch: root.lifetime.epoch}
                            onClicked: {
                                var guard = root.lifetime, chosen = pressedRecord; pressedRecord = null
                                if (guard.alive && chosen && chosen.owner === root.workspace && chosen.epoch === guard.epoch)
                                    root.readDetail(chosen.index, chosen.id)
                            }
                            onCanceled: pressedRecord = null
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            text: qsTr("上一页")
                            id: previousHistoryButton
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            contentItem: QGCLabel { text: previousHistoryButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: previousHistoryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                            enabled: root.ready && root.pageCurrent && root.pageResult.offset >= 4
                            onClicked: root.readPage(Math.max(0, root.pageResult.offset - 4))
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            text: qsTr("下一页")
                            id: nextHistoryButton
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: text
                            contentItem: QGCLabel { text: nextHistoryButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: nextHistoryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onActiveFocusChanged: if (activeFocus) root.focusVisible(this)
                            enabled: root.ready && root.pageCurrent && root.pageResult.hasMore === true
                            onClicked: root.readPage(root.pageResult.nextOffset)
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.detailCurrent
                        text: qsTr("记录 %1\n类型：%2\n时间戳（UTC 毫秒）：%3\n当前哈希：%4\n前项哈希：%5\n%6\n%7")
                            .arg(root.selectedDetail.index).arg(String(root.selectedDetail.type || "")).arg(root.selectedDetail.timestampMs)
                            .arg(String(root.selectedDetail.hash || "")).arg(String(root.selectedDetail.previousHash || ""))
                            .arg(root.selectedDetail.truncated === true ? qsTr("载荷展示已截断；完整内容保留在原文件中。") : qsTr("载荷预览："))
                            .arg(String(root.selectedDetail.payloadPreview || ""))
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                    }
                }
            }
        }
    }
}

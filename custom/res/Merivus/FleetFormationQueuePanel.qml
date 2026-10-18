import QtQuick          2.12
import QtQuick.Layouts  1.11
import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Session-local address queue. Only the existing Guided confirmation path
// may re-review an entry; this view never dispatches or restores authority.
FocusScope {
    id: root
    property var controller: null
    property var selectedIds: []
    property bool expanded: false
    property int pageIndex: 0
    property bool callBusy: false
    property string message: ""
    property real maximumBodyHeight: ScreenTools.defaultFontPixelHeight * 14
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, ticket: null})

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var queueRows: controller ? controller.formationQueueRows : []
    readonly property var queueStatus: controller ? controller.formationQueueStatus : ({})
    readonly property var checked: checkedQueue(queueRows, queueStatus)
    readonly property int pageCount: checked.ok ? Math.max(1, Math.ceil(checked.rows.length / 4)) : 1
    readonly property int displayPage: Math.min(pageIndex, pageCount - 1)
    readonly property var pageRows: checked.ok ? checked.rows.slice(displayPage * 4, displayPage * 4 + 4) : []
    readonly property string selectionStamp: idsStamp(selectedIds)
    readonly property bool sourceReady: !!controller && checked.ok && checked.status.sourceCurrent
    readonly property bool canSave: sourceReady && selectionStamp.length > 0 && !callBusy
        && checked.status.count < checked.status.limit && typeof controller.queueSelectedFormation === "function"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(v) { return !!v && typeof v === "object" && !Array.isArray(v) }
    function exact(v, keys) {
        if (!map(v) || Object.keys(v).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(v, keys[i])) return false
        return true
    }
    function uuid(v) {
        return typeof v === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v)
            && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(v)
    }
    function revision(v) {
        return typeof v === "string" && /^[1-9][0-9]{0,19}$/.test(v)
            && (v.length < 20 || v <= "18446744073709551615")
    }
    function integer(v, lo, hi) { return typeof v === "number" && isFinite(v) && Math.floor(v) === v && v >= lo && v <= hi }
    function idsStamp(ids) {
        if (!Array.isArray(ids) || ids.length < 1 || ids.length > 6) return ""
        var values = [], seen = {}
        for (var i = 0; i < ids.length; ++i) {
            if (!integer(ids[i], 1, 6) || seen[ids[i]]) return ""
            seen[ids[i]] = true; values.push(ids[i])
        }
        if (!seen[1]) return ""
        values.sort(function(a, b) { return a - b })
        return values.join(",")
    }
    function rowKnown(v) {
        return exact(v, ["id", "revision", "vehicleIds", "state", "reason", "createdAt", "expiresAt", "canReview", "canCancel", "canRemove", "taskId"])
            && uuid(v.id) && revision(v.revision) && idsStamp(v.vehicleIds).length > 0
            && ["waiting", "ready-to-review", "awaiting-confirmation", "dispatching", "submitted", "rejected", "cancelled", "expired"].indexOf(v.state) >= 0
            && typeof v.reason === "string" && v.reason.length <= 4096
            && typeof v.createdAt === "string" && v.createdAt.length > 0 && v.createdAt.length <= 64
            && typeof v.expiresAt === "string" && v.expiresAt.length > 0 && v.expiresAt.length <= 64
            && typeof v.canReview === "boolean" && typeof v.canCancel === "boolean" && typeof v.canRemove === "boolean"
            && typeof v.taskId === "string" && v.taskId.length <= 256
    }
    function checkedQueue(rows, status) {
        var bad = {ok: false, rows: [], status: {}}
        if (!exact(status, ["count", "limit", "sourceCurrent", "automaticDispatch", "persistent"])
                || !integer(status.count, 0, 32) || status.limit !== 32 || typeof status.sourceCurrent !== "boolean"
                || status.automaticDispatch !== false || status.persistent !== false
                || !Array.isArray(rows) || rows.length !== status.count) return bad
        var seen = {}
        for (var i = 0; i < rows.length; ++i) {
            if (!rowKnown(rows[i]) || seen[rows[i].id]) return bad
            seen[rows[i].id] = true
        }
        return {ok: true, rows: rows, status: status}
    }
    function stateText(state) {
        var labels = {"waiting": qsTr("等待重新审阅条件"), "ready-to-review": qsTr("可重新审阅"),
            "awaiting-confirmation": qsTr("等待原确认对话框"), "dispatching": qsTr("正在提交派发请求"),
            "submitted": qsTr("已提交派发请求"), "rejected": qsTr("请求被拒或结果未确认"),
            "cancelled": qsTr("待办已取消"), "expired": qsTr("待办已过期")}
        return labels[state] || qsTr("未知状态")
    }
    function reasonText(reason) {
        var labels = {
            "formation_active_or_busy": qsTr("当前编队仍活动或正在处理；仅等待，不会自动结束它。"),
            "queue_predecessor_pending": qsTr("前方待办尚未结束；按 FIFO 等待。"),
            "fresh_review_required": qsTr("需要本次重新审阅与人工确认。"),
            "source_unavailable": qsTr("原生来源不可用；未知不能当空闲。"),
            "native_occupancy_unavailable": qsTr("原生占用／连续性记录当前不可核对；不会当作无占用。"),
            "native_task_occupancy_unavailable": qsTr("实际任务占用来源不可核对；不会当作无任务。"),
            "pending_lifetime_expired": qsTr("本次会话待办已超过 30 分钟，不会自动重排。"),
            "review_confirmation_required": qsTr("等待原确认对话框；关闭对话框不等于取消整条待办。"),
            "dispatch_submission_result_unknown": qsTr("派发提交结果未知；可能已有提交，不会自动重试或释放占用。"),
            "intention_cancelled_not_aircraft_command": qsTr("仅取消了本地待办，没有发送停机／返航指令。"),
            "local_request_submitted": qsTr("已记录本地派发请求提交，不代表机载接受或任务完成。"),
            "local_request_rejected": qsTr("本地请求被拒；仍需核对实际任务和占用，不据此认定飞机空闲。"),
            "queue_revision_exhausted": qsTr("本地修订已耗尽，不能继续操作此队列。"),
            "queue_limit_reached": qsTr("含终态共 32 条已满；请明确移除不再保留的终态记录。"),
            "queue_entry_stale_or_missing": qsTr("待办修订已变化或条目已不在；请重新查看。"),
            "queue_review_invalid_or_expired": qsTr("原审阅已过期或来源变化；需重新审阅，旧确认不能复用。"),
            "formation-dispatch-submitted-not-execution": qsTr("已提交本地派发请求，不表示飞机已经执行或任务成功。"),
            "formation-dispatch-rejected-partial-or-unknown": qsTr("原派发被拒或回执未知，可能已有部分提交；不得据此自动重排。"),
            "formation-dispatch-interrupted-partial-or-unknown": qsTr("派发调用被中断，可能已有部分提交；应核对实际任务和占用，不自动重试。"),
            "formation-task-reservation-denied": qsTr("本次任务未取得全部成员占用；请先处理冲突任务。"),
            "formation-task-reservation-changed": qsTr("本次任务占用或目标已变化；原确认不继续派发。"),
            "formation-resource-busy": qsTr("原编队仍活动或正在处理，本次请求未继续；不会自动结束原会话。")
        }
        var explanation = labels[reason] || ""
        var nativeBlocker = /^vehicle_native_occupancy_held:([1-6])$/.exec(reason)
        var taskBlocker = /^vehicle_reserved_by_task:([1-6]):([0-9a-f-]{36})$/.exec(reason)
        if (nativeBlocker) explanation = qsTr("UAV %1 仍有原生占用或未知待复核，不能作为空闲机。 ").arg(nativeBlocker[1])
        else if (taskBlocker && uuid(taskBlocker[2]))
            explanation = qsTr("UAV %1 被本地任务 %2 保留；此处不结束任务或释放机载控制。 ").arg(taskBlocker[1]).arg(taskBlocker[2])
        if (!reason.length) return qsTr("无附加原因")
        return explanation.length ? explanation + qsTr("\n代码：%1").arg(reason) : reason
    }
    function rowFor(id, rev) {
        if (!checked.ok) return null
        for (var i = 0; i < checked.rows.length; ++i) {
            var row = checked.rows[i]
            if (row.id === id && row.revision === rev) return row
        }
        return null
    }
    function invalidatePresses() {
        var g = lifetime
        if (!g.alive) return
        if (g.epoch >= 9007199254740991) g.exhausted = true
        else ++g.epoch
    }
    function capture(action, row) {
        var g = lifetime
        if (!g.alive || g.exhausted || g.ticket || !visible || !enabled || callBusy) return null
        if (action === "queue") {
            if (!canSave) return null
            return {owner: controller, epoch: g.epoch, action: action, selection: selectionStamp, id: "", revision: ""}
        }
        if (!expanded || !rowKnown(row) || !rowFor(row.id, row.revision)) return null
        return {owner: controller, epoch: g.epoch, action: action, selection: "", id: row.id, revision: row.revision}
    }
    function pressCurrent(press) {
        var g = lifetime
        if (!g.alive || g.exhausted || !press || press.owner !== controller || press.epoch !== g.epoch || !visible || !enabled) return false
        // This is also checked after our own callBusy publication. Do not
        // use canSave here: its display-only !callBusy would self-reject.
        if (press.action === "queue") return sourceReady && selectionStamp.length > 0
            && press.selection === selectionStamp && checked.status.count < checked.status.limit
            && typeof controller.queueSelectedFormation === "function"
        if (!expanded || !checked.ok) return false
        var row = rowFor(press.id, press.revision)
        if (!row) return false
        if (press.action === "review") return sourceReady && row.canReview && typeof controller.reviewQueuedFormation === "function"
        if (press.action === "cancel") return row.canCancel && typeof controller.cancelQueuedFormation === "function"
        if (press.action === "remove") return row.canRemove && typeof controller.removeQueuedFormation === "function"
        return false
    }
    function act(press) {
        var g = lifetime
        if (!g.alive || g.ticket || !pressCurrent(press)) return
        var owner = press.owner, ticket = {}
        g.ticket = ticket
        // Plain ticket stays installed throughout all QML/native callbacks.
        var release = function() {
            if (g.alive && g.ticket === ticket) root.callBusy = false
            if (g.ticket === ticket) g.ticket = null
        }
        try {
            callBusy = true
            if (!g.alive || g.ticket !== ticket || !pressCurrent(press)) return
            var result
            if (press.action === "queue") result = owner.queueSelectedFormation()
            else if (press.action === "review") result = owner.reviewQueuedFormation(press.id, press.revision)
            else if (press.action === "cancel") result = owner.cancelQueuedFormation(press.id, press.revision)
            else result = owner.removeQueuedFormation(press.id, press.revision)
            if (!g.alive || g.ticket !== ticket || owner !== controller) return
            if (!exact(result, ["ok", "reason"]) || typeof result.ok !== "boolean" || typeof result.reason !== "string" || result.reason.length > 4096) {
                message = qsTr("返回结果不完整；不要据此重复派发，请核对当前待办状态。")
                if (g.alive) feedback.forceActiveFocus()
                return
            }
            var success = press.action === "queue" ? qsTr("已保存选中机号到待办；不会自动派发。")
                : press.action === "review" ? qsTr("已请求重新审阅并打开原确认流程；这不表示派发或任务成功。")
                : press.action === "cancel" ? qsTr("已取消本地待办；这不是停止飞机或取消已提交任务。")
                : qsTr("已移除本次会话的终态待办记录。")
            message = result.ok ? success : qsTr("未确认操作：%1\n请核对当前状态后再操作。").arg(result.reason.length ? reasonText(result.reason) : qsTr("原因未知"))
            if (g.alive && !result.ok) feedback.forceActiveFocus()
        } catch (error) {
            if (g.alive && g.ticket === ticket && owner === controller) {
                message = qsTr("操作结果未知；请核对当前待办状态，不要据此重复派发。")
                if (g.alive) feedback.forceActiveFocus()
            }
        } finally { release() }
    }
    function focusVisible(item) {
        var g = lifetime
        if (!g.alive || !item) return
        var parentItem = item.parent
        for (var depth = 0; parentItem && depth < 64; ++depth) {
            if (typeof parentItem.contentY === "number" && typeof parentItem.contentHeight === "number" && parentItem.contentItem) {
                var point = item.mapToItem(parentItem.contentItem, 0, 0)
                if (!g.alive) return
                var bottom = point.y + item.height
                if (point.y < parentItem.contentY) parentItem.contentY = Math.max(0, point.y - pad)
                else if (bottom > parentItem.contentY + parentItem.height)
                    parentItem.contentY = Math.max(0, Math.min(parentItem.contentHeight - parentItem.height, bottom - parentItem.height + pad))
                return
            }
            parentItem = parentItem.parent
        }
    }
    onControllerChanged: invalidatePresses()
    onSelectedIdsChanged: invalidatePresses()
    onVisibleChanged: invalidatePresses()
    onEnabledChanged: invalidatePresses()
    onExpandedChanged: { var g = lifetime; invalidatePresses(); if (g.alive) body.contentY = 0 }
    onPageIndexChanged: { var g = lifetime; invalidatePresses(); if (g.alive) body.contentY = 0 }
    Component.onDestruction: lifetime.alive = false

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: pal.windowShade
        border.color: pal.colorGrey
    }
    ColumnLayout {
        id: form
        width: root.width
        spacing: root.pad * 0.75
        QGCButton {
            id: disclosure
            Layout.fillWidth: true
            Layout.minimumHeight: root.rowHeight
            text: root.expanded ? qsTr("收起编队待办（%1/32）").arg(root.checked.ok ? root.checked.rows.length : "?")
                                : qsTr("展开编队待办（%1/32）").arg(root.checked.ok ? root.checked.rows.length : "?")
            focusPolicy: Qt.StrongFocus
            Accessible.name: text
            onClicked: root.expanded = !root.expanded
            onActiveFocusChanged: if (activeFocus) root.focusVisible(disclosure)
            contentItem: QGCLabel { text: disclosure.text; font.underline: disclosure.activeFocus; color: disclosure._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
        }
        QGCButton {
            id: saveButton
            Layout.fillWidth: true
            Layout.minimumHeight: root.rowHeight
            text: qsTr("保存选中编队到待办")
            enabled: root.canSave
            focusPolicy: Qt.StrongFocus
            Accessible.name: text
            property var pressedSource: null
            onPressed: pressedSource = root.capture("queue", null)
            onCanceled: pressedSource = null
            onClicked: { var g = root.lifetime, value = pressedSource; pressedSource = null; if (g.alive) root.act(value) }
            onActiveFocusChanged: if (activeFocus) root.focusVisible(saveButton)
            contentItem: QGCLabel { text: saveButton.text; font.underline: saveButton.activeFocus; color: saveButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
        }
        // The Overlay's legacy left column is clipped but not scrollable.
        // Bound only this new panel's body, so all four rows and diagnostics
        // remain reachable without changing the older command layout.
        QGCFlickable {
            id: body
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(details.implicitHeight, Math.max(root.rowHeight * 2, root.maximumBodyHeight))
            contentWidth: width
            contentHeight: details.implicitHeight
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            ColumnLayout {
                id: details
                width: body.width
                spacing: root.pad * 0.75
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("仅本次会话 · 30 分钟过期 · 不自动派发。等待不证明健康允许或飞机空闲；提交／失败状态不证明机载执行或撤销。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: !root.checked.ok || !root.sourceReady || root.selectionStamp.length === 0 || (root.checked.ok && root.checked.status.count >= 32)
            text: !root.checked.ok ? qsTr("待办接口尚不可用或数据不完整；不会把未知当空队列。")
                : !root.sourceReady ? qsTr("当前原生来源不可用；不能保存或重新审阅。")
                : root.checked.status.count >= 32 ? qsTr("待办已满（含终态共 32 条）；请明确移除不再保留的终态。")
                : qsTr("请选择协议机号 UAV 1–6 中的不同地址，且必须包含 UAV 1，再保存到待办。")
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.expanded
            spacing: root.pad
            QGCLabel {
                Layout.fillWidth: true
                visible: root.checked.ok && root.checked.rows.length === 0
                text: qsTr("暂无编队待办。这里只保存固定机号；重新审阅仍走原确认对话框和实际占用、健康、权限检查。")
                textFormat: Text.PlainText; wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.pageRows
                delegate: ColumnLayout {
                    id: entry
                    Layout.fillWidth: true
                    readonly property var row: modelData
                    spacing: root.pad * 0.5
                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: pal.colorGrey }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("FIFO #%1 · UAV %2\n%3\n原因：%4\n创建 UTC：%5\n过期 UTC：%6\n待办 %7 · 修订 %8%9")
                            .arg(root.displayPage * 4 + index + 1).arg(entry.row.vehicleIds.join(", "))
                            .arg(root.stateText(entry.row.state)).arg(root.reasonText(entry.row.reason))
                            .arg(entry.row.createdAt).arg(entry.row.expiresAt).arg(entry.row.id).arg(entry.row.revision)
                            .arg(entry.row.taskId.length ? qsTr("\n关联任务：%1（不表示完成）").arg(entry.row.taskId) : "")
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    }
                    QGCButton {
                        id: reviewButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: qsTr("重新审阅并确认")
                        enabled: root.sourceReady && entry.row.canReview && !root.callBusy
                        focusPolicy: Qt.StrongFocus; Accessible.name: text
                        property var pressedSource: null
                        onPressed: pressedSource = root.capture("review", entry.row)
                        onCanceled: pressedSource = null
                        onClicked: { var g = root.lifetime, value = pressedSource; pressedSource = null; if (g.alive) root.act(value) }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(reviewButton)
                        contentItem: QGCLabel { text: reviewButton.text; font.underline: reviewButton.activeFocus; color: reviewButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                    }
                    QGCButton {
                        id: cancelButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: qsTr("取消待办")
                        enabled: entry.row.canCancel && !root.callBusy
                        focusPolicy: Qt.StrongFocus; Accessible.name: text
                        property var pressedSource: null
                        onPressed: pressedSource = root.capture("cancel", entry.row)
                        onCanceled: pressedSource = null
                        onClicked: { var g = root.lifetime, value = pressedSource; pressedSource = null; if (g.alive) root.act(value) }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(cancelButton)
                        contentItem: QGCLabel { text: cancelButton.text; font.underline: cancelButton.activeFocus; color: cancelButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                    }
                    QGCButton {
                        id: removeButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: qsTr("移除终态记录")
                        enabled: entry.row.canRemove && !root.callBusy
                        focusPolicy: Qt.StrongFocus; Accessible.name: text
                        property var pressedSource: null
                        onPressed: pressedSource = root.capture("remove", entry.row)
                        onCanceled: pressedSource = null
                        onClicked: { var g = root.lifetime, value = pressedSource; pressedSource = null; if (g.alive) root.act(value) }
                        onActiveFocusChanged: if (activeFocus) root.focusVisible(removeButton)
                        contentItem: QGCLabel { text: removeButton.text; font.underline: removeButton.activeFocus; color: removeButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
                    }
                }
            }
        }
        QGCLabel {
            id: feedback
            Layout.fillWidth: true
            visible: root.message.length > 0
            text: root.message
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
            activeFocusOnTab: true
            onActiveFocusChanged: if (activeFocus) root.focusVisible(feedback)
        }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: root.expanded
            QGCButton {
                id: previousButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                text: qsTr("上一页"); enabled: root.displayPage > 0 && !root.callBusy
                focusPolicy: Qt.StrongFocus; Accessible.name: text
                onClicked: root.pageIndex = root.displayPage - 1
                onActiveFocusChanged: if (activeFocus) root.focusVisible(previousButton)
                contentItem: QGCLabel { text: previousButton.text; font.underline: previousButton.activeFocus; color: previousButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
            }
            QGCLabel { text: qsTr("%1/%2 页").arg(root.displayPage + 1).arg(root.pageCount); textFormat: Text.PlainText }
            QGCButton {
                id: nextButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                text: qsTr("下一页"); enabled: root.displayPage + 1 < root.pageCount && !root.callBusy
                focusPolicy: Qt.StrongFocus; Accessible.name: text
                onClicked: root.pageIndex = root.displayPage + 1
                onActiveFocusChanged: if (activeFocus) root.focusVisible(nextButton)
                contentItem: QGCLabel { text: nextButton.text; font.underline: nextButton.activeFocus; color: nextButton._showHighlight ? pal.buttonHighlightText : pal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter }
            }
        }
    }
}

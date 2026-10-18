import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// This view retires only specifically reviewed LOCAL restart/history holds.
// Its value-only receipt is neither a remote lease nor a flight permission.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, view: null, opening: null, activeCall: null, owned: null})
    property var ownedReceipt: ({})
    property var reviewOwner: null
    property bool callBusy: false
    property var callTicket: null
    property string addressText: ""
    property string dispositionText: ""
    property string localError: ""
    property var lastResult: ({})
    property double clockMs: Date.now()
    property double reviewCapturedMs: -1
    property double reviewExpiresMs: -1
    property bool clockInvalid: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var recoveryState: controller ? controller.fleetOccupancyRecovery || {} : ({})
    readonly property var workflow: recoveryState.workflow || ({})
    readonly property var persistence: recoveryState.persistence || ({})
    readonly property var identity: recoveryState.operatorIdentity || ({})
    readonly property string sourceToken: boundedToken(recoveryState.sourceToken)
    readonly property var parsedAddresses: parseAddresses(addressText)
    readonly property bool inputValid: parsedAddresses.ok === true && dispositionValid(dispositionText)
    readonly property bool sourceReady: recoveryState.sourceKnown === true && sourceToken.length > 0
        && persistence.bootstrapAccepted === true && persistence.stickyFailure === false
        && checkedIds(recoveryState.availableVehicleIds, 255) !== null
        && checkedIds(persistence.currentUncertainVehicleIds, 255) !== null
    readonly property bool selectedReady: selectedAddressesReady()
    readonly property bool backendBusy: recoveryState.busy !== false
    readonly property string ownToken: receiptToken(ownedReceipt)
    readonly property bool ownReviewCurrent: reviewOwner === controller && ownToken.length > 0
        && ownedReceipt.sourceToken === sourceToken && ownToken === receiptToken(workflow)
    readonly property double displayRemainingMs: !clockInvalid && reviewCapturedMs >= 0 && clockMs >= reviewCapturedMs
        ? Math.max(0, reviewExpiresMs - clockMs) : 0
    readonly property bool canRequest: !!controller && lifetime.alive && !lifetime.exhausted && !!dialog
        && sourceReady && inputValid && selectedReady && identity.authenticated === true
        && !callBusy && !backendBusy && !ownToken.length
        && workflow.state !== "review" && workflow.state !== "approving" && workflow.state !== "preparing"
        && typeof controller.requestFleetOccupancyRecoveryReview === "function"
        && typeof controller.cancelFleetOccupancyRecoveryReview === "function"
    readonly property bool canApprove: ownReviewCurrent && workflow.state === "review" && displayRemainingMs > 0
        && !callBusy && !backendBusy && sourceReady && selectedReady
        && typeof controller.approveFleetOccupancyRecovery === "function"
    readonly property bool canAuthenticate: !!controller && lifetime.alive && !lifetime.exhausted && sourceToken.length > 0
        && !callBusy && !backendBusy && !ownToken.length
        && workflow.state !== "review" && workflow.state !== "approving" && workflow.state !== "preparing"
        && typeof controller.authenticateFleetOperator === "function"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function boundedToken(value) { return typeof value === "string" && value.length > 0 && value.length <= 8192 ? value : "" }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function text(value) { return typeof value === "string" && value.length > 0 ? value.substring(0, 512) : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function checkedIds(value, maximum) {
        if (!Array.isArray(value) || value.length > maximum) return null
        var result = [], previous = 0
        for (var i = 0; i < value.length; ++i) {
            var id = value[i]
            if (typeof id !== "number" || !isFinite(id) || Math.floor(id) !== id || id < 1 || id > 255 || id <= previous) return null
            result.push(id); previous = id
        }
        return result
    }
    function idsText(value) {
        var checked = checkedIds(value, 255)
        return checked === null ? qsTr("未知（不视为空）") : checked.length ? checked.join(", ") : qsTr("无所列地址（不证明远端空闲）")
    }
    function parseAddresses(value) {
        if (typeof value !== "string" || value.length > 512 || !value.trim().length) return {ok: false, ids: []}
        var parts = value.split(","), result = [], seen = {}
        if (parts.length < 1 || parts.length > 32) return {ok: false, ids: []}
        for (var i = 0; i < parts.length; ++i) {
            var part = parts[i].trim(), id = Number(part)
            if (!/^[1-9][0-9]{0,2}$/.test(part) || id > 255 || seen[id]) return {ok: false, ids: []}
            seen[id] = true; result.push(id)
        }
        result.sort(function(a, b) { return a - b })
        return {ok: true, ids: result}
    }
    function utf8Length(value) {
        var bytes = 0
        for (var i = 0; i < value.length; ++i) {
            var code = value.charCodeAt(i)
            if (code < 0x80) bytes += 1
            else if (code < 0x800) bytes += 2
            else if (code >= 0xd800 && code <= 0xdbff) {
                if (++i >= value.length) return -1
                var next = value.charCodeAt(i)
                if (next < 0xdc00 || next > 0xdfff) return -1
                bytes += 4
            } else if (code >= 0xdc00 && code <= 0xdfff) return -1
            else bytes += 3
        }
        return bytes
    }
    function dispositionValid(value) {
        if (typeof value !== "string" || !value.length || value !== value.trim()
                || /[\u0000-\u001f\u007f-\u009f\u200b-\u200f\u202a-\u202e\u2060-\u206f\ufeff]/.test(value)) return false
        var length = utf8Length(value)
        return length > 0 && length <= 256
    }
    function selectedAddressesReady() {
        var available = checkedIds(recoveryState.availableVehicleIds, 255)
        var uncertain = checkedIds(persistence.currentUncertainVehicleIds, 255)
        if (!parsedAddresses.ok || available === null || uncertain === null) return false
        for (var i = 0; i < parsedAddresses.ids.length; ++i)
            if (available.indexOf(parsedAddresses.ids[i]) < 0 || uncertain.indexOf(parsedAddresses.ids[i]) >= 0) return false
        return true
    }
    function receiptToken(value) {
        if (!value || !uuid(value.workflowEpoch) || !uuid(value.reviewId)
                || typeof value.reviewGeneration !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.reviewGeneration)
                || typeof value.bindingHash !== "string" || !/^[0-9a-f]{64}$/.test(value.bindingHash)
                || !value.context || typeof value.context !== "object" || Array.isArray(value.context)
                || !Array.isArray(value.targetIdentities) || value.targetIdentities.length < 1 || value.targetIdentities.length > 32
                || checkedIds(value.targetVehicleIds, 32) === null || value.targetVehicleIds.length < 1
                || !boundedToken(value.sourceToken)) return ""
        return JSON.stringify({workflowEpoch: value.workflowEpoch, reviewId: value.reviewId,
            reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, context: value.context,
            targetIdentities: value.targetIdentities, targetVehicleIds: value.targetVehicleIds, sourceToken: value.sourceToken})
    }
    function cancelExact(owner, receipt) {
        if (!owner || !receipt || !receipt.reviewId) return
        try {
            // Dedicated cancel-only route consumes this original issued receipt,
            // even while a modal call is active or its source token has changed.
            owner.cancelFleetOccupancyRecoveryReview(receipt.reviewId, receipt)
        } catch (error) { /* Missing owner or already committed effect is not rollback. */ }
    }
    function clearChecks() {
        var guard = lifetime, view = dialog
        if (guard.alive && view) view.clearConfirmations()
    }
    function invalidateReview(message) {
        var guard = lifetime, owned = guard.owned
        if (!guard.alive) return
        guard.owned = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        var epoch = guard.epoch
        // Cancel before any QML property notification can destroy the panel.
        if (owned) cancelExact(owned.owner, owned.receipt)
        if (!guard.alive || guard.epoch !== epoch) return
        ownedReceipt = ({})
        if (!guard.alive || guard.epoch !== epoch) return
        reviewOwner = null
        if (!guard.alive || guard.epoch !== epoch) return
        reviewCapturedMs = -1
        if (!guard.alive || guard.epoch !== epoch) return
        reviewExpiresMs = -1
        if (!guard.alive || guard.epoch !== epoch) return
        clearChecks()
        if (guard.alive && guard.epoch === epoch && message && dialog) localError = message
    }
    function pressSnapshot() { return {owner: controller, epoch: lifetime.epoch, source: sourceToken, receipt: ownToken, view: lifetime.view} }
    function pressCurrent(pressed) {
        return pressed && pressed.owner === controller && pressed.epoch === lifetime.epoch && pressed.source === sourceToken
            && pressed.view && pressed.view === lifetime.view && !!dialog && visible
    }
    function finishCall(guard, ticket) {
        if (!guard.alive || guard.activeCall !== ticket) return
        callTicket = null
        if (!guard.alive || guard.activeCall !== ticket) return
        callBusy = false
        // Keep the plain ticket through the last QML notification. A reentrant
        // handler cannot start a second call while this stack is unwinding.
        if (guard.activeCall === ticket) guard.activeCall = null
    }
    function requestReview(pressed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canRequest || !pressCurrent(pressed)) {
            invalidateReview(qsTr("来源、输入或账号在点击期间变化；未申请审阅，请重新核对。")); return
        }
        var owner = controller, source = sourceToken, epoch = guard.epoch, view = guard.view
        var addresses = parsedAddresses.ids.slice(), disposition = dispositionText, started = Date.now(), ticket = {}, result = {}
        // Pure value-only builder remains callable after ownerPanel destruction.
        var extract = function(value) {
            if (!value || typeof value.reviewId !== "string" || !value.reviewId || !value.context || !value.targetIdentities || !value.targetVehicleIds) return null
            try { return JSON.parse(JSON.stringify({workflowEpoch: value.workflowEpoch, reviewId: value.reviewId,
                reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, context: value.context,
                targetIdentities: value.targetIdentities, targetVehicleIds: value.targetVehicleIds, sourceToken: value.sourceToken})) }
            catch (error) { return null }
        }
        var abandon = function(receipt) {
            if (owner && receipt) try { owner.cancelFleetOccupancyRecoveryReview(receipt.reviewId, receipt) } catch (error) {}
        }
        guard.activeCall = ticket
        try {
            callTicket = ticket
            if (!guard.alive || guard.activeCall !== ticket) return
            callBusy = true
            if (!guard.alive || guard.activeCall !== ticket || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            localError = ""
            if (!guard.alive || guard.activeCall !== ticket || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            try { result = owner.requestFleetOccupancyRecoveryReview(addresses, disposition, true, source) || {} }
            catch (error) { result = {reason: "recovery-review-call-unavailable"} }
            var receipt = extract(result)
            if (!guard.alive) { abandon(receipt); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken || !visible || !dialog) { abandon(receipt); return }
            if (result.ok !== true || result.state !== "review" || !receipt || !receiptToken(receipt)
                    || receipt.sourceToken !== source || JSON.stringify(receipt.targetVehicleIds) !== JSON.stringify(addresses)
                    || typeof result.remainingReviewMs !== "number" || !isFinite(result.remainingReviewMs)
                    || result.remainingReviewMs <= 0 || result.remainingReviewMs > 30000) {
                abandon(receipt)
                if (guard.alive && guard.epoch === epoch) localError = qsTr("未建立本次审阅：%1。请检查原生原因后手动重试；没有恢复任何飞行权限。").arg(text(result.reason || result.state))
                return
            }
            guard.owned = {owner: owner, receipt: receipt}
            reviewOwner = owner
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            ownedReceipt = receipt
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewCapturedMs = started
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewExpiresMs = started + result.remainingReviewMs
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockMs = Date.now()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockInvalid = clockMs < started
            if (guard.alive && guard.epoch === epoch && guard.view === view) clearChecks()
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function approveReview(pressed, inspection, localOnly) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canApprove || !pressCurrent(pressed) || pressed.receipt !== ownToken || inspection !== true || localOnly !== true) {
            invalidateReview(qsTr("审阅已变化、到期或确认不完整；没有重复提交，请重新申请。")); return
        }
        var owned = guard.owned, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        if (!owned || owned.owner !== reviewOwner || receiptToken(owned.receipt) !== ownToken) return
        guard.activeCall = ticket
        try {
            callTicket = ticket
            if (!guard.alive || guard.activeCall !== ticket) return
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            try { result = owned.owner.approveFleetOccupancyRecovery(owned.receipt.reviewId, true, true, owned.receipt) || {} }
            catch (error) { result = {reason: "recovery-approval-call-unavailable"} }
            if (!guard.alive) return
            // Preserve a returned historical mutation even if a source change
            // cancelled this view during the native modal. Never turn it false.
            lastResult = JSON.parse(JSON.stringify(result))
            if (!guard.alive) return
            if (guard.epoch === epoch && guard.view === view && result.ok !== true)
                localError = qsTr("审批未核实：%1。状态可能已变化；不自动重试，请查看原生结果。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function authenticate(pressed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canAuthenticate || !pressCurrent(pressed)) { invalidateReview(qsTr("账号来源已变化；请核对后重新认证。")); return }
        var owner = controller, source = sourceToken, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        guard.activeCall = ticket
        try {
            callTicket = ticket
            if (!guard.alive || guard.activeCall !== ticket) return
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            try { result = owner.authenticateFleetOperator() || {} }
            catch (error) { result = {reason: "recovery-operator-authentication-unavailable"} }
            if (!guard.alive) return
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            if (result.ok !== true) localError = qsTr("账号未核实：%1。认证后也不会自动创建审阅或解除限制。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea
        var bottom = point.y + item.height + pad
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (bottom > scroll.contentY + scroll.height) scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), bottom - scroll.height)
    }
    function dismissView() {
        var guard = lifetime, old = dialog
        if (!guard.alive) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (old) try { old.close() } catch (error) {}
    }
    function finishView(view) {
        var guard = lifetime
        if (!guard.alive || dialog !== view) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (guard.alive && !dialog && visible) entryButton.forceActiveFocus()
    }
    function openView() {
        var guard = lifetime
        if (!guard.alive || guard.opening || guard.activeCall || guard.exhausted || !controller || !visible || dialog) return
        var opening = {}, owner = controller, view = null
        guard.opening = opening
        try {
            localError = ""
            if (!guard.alive || guard.opening !== opening) return
            view = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: guard})
            if (!guard.alive) { if (view) view.destroy(); return }
            if (owner !== controller || !visible || dialog || !view) { if (view) view.destroy(); return }
            guard.view = opening
            dialog = view
            if (!guard.alive || guard.view !== opening) { view.destroy(); return }
            view.open()
        } finally { if (guard.opening === opening) guard.opening = null }
    }
    function resultText() {
        if (callBusy || backendBusy) return qsTr("原生核验或系统账号提示进行中；请勿重复提交。耗时计入原30秒期限，界面不延长授权。")
        if (workflow.localHoldsReleased === true || lastResult.localHoldsReleased === true) {
            var base = qsTr("历史事实：某次本地重启/历史待审限制已解除；这不表示现在空闲、远端任务结束或获得起飞许可。审批返回 persistenceConfirmed=false。")
            return base + (recoveryState.currentOccupancyCommitted === true
                ? qsTr("当前只读投影报告本次实际占用身份已匹配提交；这仍不是实机安全、远端空闲或许可证明。")
                : qsTr("尚未核实新的实际占用身份匹配提交；必须等待原生工作线程，不能用旧提交或本页快照替代。"))
        }
        if (workflow.state === "review") return qsTr("等待本页两项人工确认与独立账号审批；原生后端仍会重验全部实际对象、遥测、占用和原期限。")
        return qsTr("尚无本页核实的解除结果。原生状态：%1；原因：%2。未知、取消和失败均不代表空闲。").arg(text(workflow.state)).arg(text(workflow.reason))
    }
    onAddressTextChanged: invalidateReview(qsTr("地址输入已变化；旧审阅及确认已取消。"))
    onDispositionTextChanged: invalidateReview(qsTr("处置编号已变化；旧审阅及确认已取消。"))
    onSourceTokenChanged: invalidateReview(qsTr("实际来源或账号已变化；旧审阅及确认已取消，请重新核对。"))
    onSourceReadyChanged: if (!sourceReady && lifetime.alive && lifetime.owned) invalidateReview(qsTr("原生来源或持久化状态不再可核实；旧审阅已取消。"))
    onSelectedReadyChanged: if (!selectedReady && lifetime.alive && lifetime.owned) invalidateReview(qsTr("所选原地址已失效或出现当前未知；旧审阅已取消。"))
    onControllerChanged: dismissView()
    onVisibleChanged: if (!visible) dismissView()
    onOwnReviewCurrentChanged: if (!ownReviewCurrent && lifetime.alive && lifetime.owned) invalidateReview(qsTr("本页原审阅不再匹配；没有接管其他入口的审阅。"))
    onWorkflowChanged: {
        if (lifetime.alive && lifetime.owned && ["review", "approving"].indexOf(workflow.state) < 0)
            invalidateReview("")
    }
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: {
        var guard = lifetime, owned = guard.owned, old = dialog
        guard.owned = null; guard.alive = false; guard.view = null
        if (owned && owned.owner) try { owned.owner.cancelFleetOccupancyRecoveryReview(owned.receipt.reviewId, owned.receipt) } catch (error) {}
        if (old) try { old.close() } catch (error) {}
    }
    Timer {
        interval: 250; repeat: true; running: !!root.dialog || root.callBusy
        onTriggered: {
            var guard = root.lifetime, now = Date.now()
            if (now < root.clockMs) {
                root.clockInvalid = true
                if (!guard.alive) return
                root.invalidateReview(qsTr("显示时钟发生回退；本页审阅已取消，请重新申请。"))
                if (!guard.alive) return
            }
            root.clockMs = now
            if (!guard.alive) return
            if (guard.owned && root.displayRemainingMs <= 0) root.invalidateReview(qsTr("本页审阅显示期限已到；请重新申请，原生期限仍为最终依据。"))
        }
    }

    ColumnLayout {
        id: entry
        width: root.width; spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("重启 / 历史本地占用 · 专用复核")
            Accessible.name: text
            enabled: !!root.controller && !root.lifetime.exhausted && !root.callBusy
            onClicked: root.openView()
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("仅复核本地重启/历史待审；不清当前未知结果，不发送飞行指令。")
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.dialog && !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.localError; color: qgcPal.warningText }
    }

    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var localViewLife: ({alive: true})
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true
            closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton
            padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 62, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: qgcPal.window; border.color: qgcPal.text; border.width: 1; radius: 3 }
            header: QGCLabel {
                id: heading
                padding: root.pad; text: qsTr("专用本地占用复核 · 不是起飞许可")
                textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
            }
            function clearConfirmations() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive) return
                inspectionConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                localConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                requestButton.pressScope = ({})
                if (!life.alive || !ownLife.alive) return
                approveButton.pressScope = ({})
            }
            function focusError() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive || !errorSummary.visible) return
                errorSummary.forceActiveFocus()
                if (life.alive && ownLife.alive && ownerPanel) ownerPanel.ensureFocusVisible(errorSummary)
            }
            onOpened: cancelButton.forceActiveFocus()
            onAboutToHide: {
                var life = guard
                if (life && life.alive && ownerPanel) ownerPanel.finishView(view)
            }
            onClosed: {
                var life = guard, ownLife = localViewLife, actualView = view
                ownLife.alive = false
                if (life && life.alive && ownerPanel) ownerPanel.finishView(actualView)
                try { actualView.destroy() } catch (error) {}
            }
            Component.onDestruction: {
                var life = guard
                localViewLife.alive = false
                if (life && life.alive && ownerPanel) ownerPanel.finishView(view)
            }
            contentItem: Flickable {
                id: scroll
                clip: true; contentWidth: width; contentHeight: form.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                Controls.ScrollBar.vertical: Controls.ScrollBar {}
                ColumnLayout {
                    id: form
                    width: scroll.width; spacing: root.pad
                    Keys.onPressed: {
                        if (event.key === Qt.Key_Escape) { view.close(); event.accepted = true }
                        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true
                    }
                    QGCButton {
                        id: cancelButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("取消本页审阅 / 关闭（不停止飞机）"); Accessible.name: text
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(cancelButton)
                        contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: cancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: cancelButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: cancelButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: qsTr("仅解除所选地址本次重启与恢复历史的本地待审限制。当前真实未知结果、所有者及任务不会被清除；不证明实机处置、远端任务不存在或可重新起飞。不发送停止、降落、上锁、解锁或启动指令。")
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("可选在线地址：%1\n重启待审：%2\n历史待审：%3\n当前真实未知（不可由本页解除）：%4\n本次所选仍需复核：%5")
                            .arg(root.idsText(root.recoveryState.availableVehicleIds)).arg(root.idsText(root.persistence.restartReviewVehicleIds))
                            .arg(root.idsText(root.persistence.historicalReviewVehicleIds)).arg(root.idsText(root.persistence.currentUncertainVehicleIds))
                            .arg(root.idsText(root.workflow.selectedReviewRequiredVehicleIds))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: qsTr("实际来源已知：%1 · 启动账本已接纳：%2 · 持久化故障：%3\n原因：%4\n这些是只读快照，不是远端空闲或控制权证明。")
                            .arg(root.known(root.recoveryState.sourceKnown)).arg(root.known(root.persistence.bootstrapAccepted))
                            .arg(root.known(root.persistence.stickyFailure)).arg(root.text(root.persistence.errorCode))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("原生复核状态：%1 · 原因：%2\n账本工作线程：%3 · 当前实际占用身份匹配提交：%4\n重启待审列表是协议地址限制，不代表发现了同等数量的飞机或故障。")
                            .arg(root.text(root.workflow.state)).arg(root.text(root.workflow.reason))
                            .arg(root.text(root.persistence.worker ? root.persistence.worker.state : undefined))
                            .arg(root.known(root.recoveryState.currentOccupancyCommitted))
                    }
                    FocusScope {
                        id: errorSummary
                        Layout.fillWidth: true; Layout.preferredHeight: errorLabel.implicitHeight + root.pad
                        visible: !!root.localError; activeFocusOnTab: visible
                        Accessible.role: Accessible.StaticText; Accessible.name: root.localError
                        Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: qgcPal.warningText; border.width: errorSummary.activeFocus ? 3 : 1; radius: 3 }
                        QGCLabel { id: errorLabel; anchors.fill: parent; anchors.margins: root.pad / 2; text: root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: qsTr("当前申请账号：%1 · 角色：%2。显式重新认证可能撤销当前未消费批准；认证成功也不自动申请或批准本复核。")
                            .arg(root.identity.authenticated === true ? root.text(root.identity.actorId) : qsTr("未核实")).arg(root.text(root.identity.role))
                    }
                    QGCButton {
                        id: authenticateButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("显式认证当前申请账号"); Accessible.name: text; enabled: root.canAuthenticate
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: root.authenticate(pressScope)
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authenticateButton)
                    }
                    QGCLabel { id: addressLabel; Layout.fillWidth: true; text: qsTr("所选地址（1–32个，英文逗号分隔，1–255且不重复）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    Controls.TextField {
                        id: addressField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.addressText; maximumLength: 512; selectByMouse: true
                        color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize
                        enabled: !root.callBusy && !root.backendBusy
                        Accessible.name: addressLabel.text
                        onTextEdited: if (view.ownerPanel && view.guard.alive) view.ownerPanel.addressText = text
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(addressField)
                        background: Rectangle { color: qgcPal.windowShade; border.color: addressField.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: addressField.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.addressText.length > 0 && (!root.parsedAddresses.ok || !root.selectedReady)
                        textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: !root.parsedAddresses.ok ? qsTr("地址格式无效、重复或超出范围；请手动修正。") : qsTr("至少一个地址不在已知在线集合或有当前真实未知；不能由本页解除。")
                    }
                    QGCLabel { id: dispositionLabel; Layout.fillWidth: true; text: qsTr("人工处置 / 检查编号（非空，最多256个UTF-8字节）"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    Controls.TextField {
                        id: dispositionField
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.dispositionText; maximumLength: 256; selectByMouse: true
                        color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize
                        enabled: !root.callBusy && !root.backendBusy
                        Accessible.name: dispositionLabel.text
                        onTextEdited: if (view.ownerPanel && view.guard.alive) view.ownerPanel.dispositionText = text
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(dispositionField)
                        background: Rectangle { color: qgcPal.windowShade; border.color: dispositionField.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: dispositionField.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.dispositionText.length > 0 && !root.dispositionValid(root.dispositionText)
                        text: qsTr("编号不能含首尾空白、控制字符、无效字符或超过256个UTF-8字节；原生后端还会重新核验。")
                        textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                    }
                    QGCButton {
                        id: requestButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("1. 申请所选地址专用复核（不解除限制）"); Accessible.name: text; enabled: root.canRequest
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: root.requestReview(pressScope)
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(requestButton)
                        contentItem: QGCLabel { text: requestButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: requestButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本页拥有有效审阅：%1 · 原生成代：%2\n本页保守显示剩余：%3秒（最终以原生单调时钟为准，不续期）\n原审阅地址：%4")
                            .arg(root.known(root.ownReviewCurrent)).arg(root.text(root.ownedReceipt.reviewGeneration))
                            .arg(Math.floor(root.displayRemainingMs / 1000)).arg(root.idsText(root.ownedReceipt.targetVehicleIds))
                    }
                    Controls.CheckBox {
                        id: inspectionConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        checked: false; enabled: root.canApprove
                        text: qsTr("我声明已完成所选实机的现场地面检查；这是人工声明，不是传感器、远端任务或物理处置的认证。")
                        Accessible.name: text
                        contentItem: QGCLabel { text: inspectionConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: inspectionConsent.indicator.width + root.pad; verticalAlignment: Text.AlignVCenter }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(inspectionConsent)
                    }
                    Controls.CheckBox {
                        id: localConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        checked: false; enabled: root.canApprove
                        text: qsTr("我确认只复核本次本地重启/历史待审：不清当前真实未知或所有者，不证明远端空闲、任务完成、控制租约或起飞许可。")
                        Accessible.name: text
                        contentItem: QGCLabel { text: localConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: localConsent.indicator.width + root.pad; verticalAlignment: Text.AlignVCenter }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(localConsent)
                    }
                    QGCButton {
                        id: approveButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("2. 请求独立账号审批 · 仅解除本地待审")
                        Accessible.name: text
                        enabled: root.canApprove && inspectionConsent.checked && localConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: root.approveReview(pressScope, inspectionConsent.checked, localConsent.checked)
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(approveButton)
                        contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: approveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: approveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.colorRed; border.width: approveButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel { Layout.fillWidth: true; text: root.resultText(); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                }
            }
        }
    }
}

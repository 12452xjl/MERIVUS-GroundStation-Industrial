import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Value-only fixed-file declaration review. Native owners and credentials stay
// in the controller; this panel never imports arbitrary JSON or selects a path.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, view: null, opening: null, activeCall: null, owned: null})
    property var ownedReceipt: ({})
    property var reviewOwner: null
    property bool callBusy: false
    property string localError: ""
    property var lastResult: ({})
    property var committedFact: ({})
    property double clockMs: Date.now()
    property double reviewCapturedMs: -1
    property double reviewExpiresMs: -1
    property bool clockInvalid: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var reloadState: controller ? controller.fleetFirmwareCatalogReload || {} : ({})
    readonly property var workflow: reloadState.workflow || ({})
    readonly property var identity: reloadState.operatorIdentity || ({})
    readonly property string sourceToken: token(reloadState.sourceToken)
    readonly property bool sourceReady: reloadState.schemaVersion === 1 && reloadState.sourceKnown === true
        && reloadState.uiWiringPresent === true && reloadState.runtimeValidated === false
        && sourceToken.length > 0 && workflowKnown(workflow)
    readonly property bool backendBusy: reloadState.busy !== false
    readonly property string ownToken: receiptToken(ownedReceipt)
    readonly property bool ownReviewCurrent: reviewOwner === controller && ownToken.length > 0
        && ownedReceipt.scopeToken === sourceToken && ownToken === receiptToken(workflow)
    readonly property bool previewReady: ownReviewCurrent && previewUsable(workflow.preview)
        && workflow.fileObservation && hex(workflow.fileObservation.rawSha256)
        && workflow.preview.inputJsonSha256 === workflow.fileObservation.rawSha256
        && typeof workflow.fixedFilePath === "string" && workflow.fixedFilePath.length > 0
        && workflow.fileObservation.fixedPath === workflow.fixedFilePath
    readonly property double displayRemainingMs: !clockInvalid && reviewCapturedMs >= 0 && clockMs >= reviewCapturedMs
        ? Math.max(0, reviewExpiresMs - clockMs) : 0
    readonly property bool reviewPending: ["preparing", "review", "approving"].indexOf(workflow.state) >= 0
    readonly property bool canAuthenticate: !!controller && lifetime.alive && !lifetime.exhausted && !!dialog
        && sourceToken.length > 0 && !callBusy && !backendBusy && !ownToken.length && !reviewPending
        && typeof controller.authenticateFleetOperator === "function"
    readonly property bool canRequest: !!controller && lifetime.alive && !lifetime.exhausted && !!dialog
        && sourceReady && identity.authenticated === true && ["maintenance", "administrator"].indexOf(identity.role) >= 0
        && !callBusy && !backendBusy && !ownToken.length && !reviewPending
        && typeof controller.requestFleetFirmwareCatalogReload === "function"
        && typeof controller.cancelFleetFirmwareCatalogReload === "function"
    readonly property bool canApprove: sourceReady && previewReady && ownReviewCurrent && workflow.state === "review"
        && integer(workflow.remainingReviewMs, 1, 30000) && displayRemainingMs > 0
        && !callBusy && !backendBusy && typeof controller.approveFleetFirmwareCatalogReload === "function"
    readonly property var changeRows: previewReady ? descriptorChanges(workflow.preview) : []

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function token(value) { return typeof value === "string" && value.length > 0 && value.length <= 32768 ? value : "" }
    function hex(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function receiptToken(value) {
        if (!value || !uuid(value.workflowEpoch) || !uuid(value.reviewId)
                || typeof value.reviewGeneration !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.reviewGeneration)
                || !hex(value.bindingHash) || !token(value.scopeToken)) return ""
        return JSON.stringify({workflowEpoch: value.workflowEpoch, reviewId: value.reviewId,
            reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, scopeToken: value.scopeToken})
    }
    function workflowKnown(value) {
        if (!value || Object.keys(value).length !== 28 || value.schemaVersion !== 1
                || !uuid(value.workflowEpoch) || ["empty", "preparing", "review", "approving", "reloaded", "stale", "expired", "cancelled"].indexOf(value.state) < 0
                || !integer(value.remainingReviewMs, -1, 30000)) return false
        var flags = ["busy", "promptInProgress", "replacementConfirmed", "authenticationConsumed", "reloadCommitted", "authorizationOwnerRetained"]
        for (var i = 0; i < flags.length; ++i) if (typeof value[flags[i]] !== "boolean") return false
        var denied = ["reviewUiIntegrated", "firmwareIdentityAuthenticated", "signatureVerified", "executionPermitted", "flightCommandReleased"]
        for (var j = 0; j < denied.length; ++j) if (value[denied[j]] !== false) return false
        return typeof value.fixedFilePath === "string" && typeof value.reason === "string"
            && value.preview && typeof value.preview === "object" && !Array.isArray(value.preview)
    }
    function descriptorList(value, entries) {
        if (!Array.isArray(value) || value.length > 64) return null
        var output = [], previous = null
        for (var i = 0; i < value.length; ++i) {
            var descriptor = entries && value[i] ? value[i].descriptor : value[i]
            if (!descriptor || typeof descriptor !== "object" || Array.isArray(descriptor)
                    || typeof descriptor.descriptorId !== "string" || !descriptor.descriptorId.length
                    || descriptor.descriptorId.length > 64 || (previous !== null && descriptor.descriptorId <= previous)) return null
            var review = descriptor.engineeringReview
            if (!review || Object.keys(review).length !== 4) return null
            var fields = ["reviewId", "reviewerId", "reviewedAtUtc", "evidenceDigest"]
            for (var f = 0; f < fields.length; ++f) if (typeof review[fields[f]] !== "string" || !review[fields[f]].length) return null
            output.push(descriptor)
            previous = descriptor.descriptorId
        }
        return output
    }
    function checkedIds(value) {
        if (!Array.isArray(value) || value.length > 64) return null
        var output = [], previous = null
        for (var i = 0; i < value.length; ++i) {
            var id = value[i]
            if (typeof id !== "string" || !id.length || id.length > 64 || (previous !== null && id <= previous)) return null
            output.push(id); previous = id
        }
        return output
    }
    function previewUsable(value) {
        if (!value || value.ok !== true || value.wholeReplacement !== true || value.previewOnly !== true || value.mutationCommitted !== false
                || !hex(value.normalizedFingerprint) || !hex(value.inputJsonSha256)
                || typeof value.normalizedJson !== "string" || !value.normalizedJson.length) return false
        var before = descriptorList(value.previousEntries, true), after = descriptorList(value.descriptors, false)
        if (before === null || after === null || !integer(value.previousEntryCount, 0, 64) || !integer(value.descriptorCount, 0, 64)
                || before.length !== value.previousEntryCount || after.length !== value.descriptorCount) return false
        var kinds = ["added", "removed", "changed", "unchanged"], all = [], beforeIds = [], afterIds = []
        for (var b = 0; b < before.length; ++b) beforeIds.push(before[b].descriptorId)
        for (var a = 0; a < after.length; ++a) afterIds.push(after[a].descriptorId)
        for (var k = 0; k < kinds.length; ++k) {
            var kind = kinds[k], ids = checkedIds(value[kind + "DescriptorIds"])
            if (ids === null || !integer(value[kind + "Count"], 0, 64) || ids.length !== value[kind + "Count"]) return false
            for (var n = 0; n < ids.length; ++n) {
                if (all.indexOf(ids[n]) >= 0 || (kind !== "added" && beforeIds.indexOf(ids[n]) < 0)
                        || (kind !== "removed" && afterIds.indexOf(ids[n]) < 0)
                        || (kind === "added" && beforeIds.indexOf(ids[n]) >= 0)
                        || (kind === "removed" && afterIds.indexOf(ids[n]) >= 0)) return false
                all.push(ids[n])
            }
        }
        return value.addedCount + value.changedCount + value.unchangedCount === after.length
            && value.removedCount + value.changedCount + value.unchangedCount === before.length
    }
    function descriptorChanges(value) {
        var before = descriptorList(value.previousEntries, true), after = descriptorList(value.descriptors, false), rows = []
        if (before === null || after === null) return rows
        var kinds = ["added", "removed", "changed", "unchanged"]
        for (var k = 0; k < kinds.length; ++k) {
            var ids = checkedIds(value[kinds[k] + "DescriptorIds"])
            if (ids === null) return []
            for (var n = 0; n < ids.length; ++n) {
                var oldDescriptor = null, newDescriptor = null
                for (var b = 0; b < before.length; ++b) if (before[b].descriptorId === ids[n]) oldDescriptor = before[b]
                for (var a = 0; a < after.length; ++a) if (after[a].descriptorId === ids[n]) newDescriptor = after[a]
                rows.push({kind: kinds[k], id: ids[n], before: oldDescriptor, after: newDescriptor})
            }
        }
        return rows
    }
    function kindText(kind) { return kind === "added" ? qsTr("新增") : kind === "removed" ? qsTr("删除") : kind === "changed" ? qsTr("修改") : qsTr("未变") }
    function reviewText(descriptor) {
        if (!descriptor) return qsTr("本侧无此声明")
        var review = descriptor.engineeringReview || {}
        return qsTr("reviewId：%1\nreviewerId：%2\nreviewedAtUtc：%3\nevidenceDigest：%4")
            .arg(text(review.reviewId)).arg(text(review.reviewerId)).arg(text(review.reviewedAtUtc)).arg(text(review.evidenceDigest))
    }
    function fullJson(value) { try { return JSON.stringify(value, null, 2) } catch (error) { return qsTr("完整数据无法显示；不可据此审批。") } }
    function cancelExact(owner, receipt) {
        if (owner && receipt) try { owner.cancelFleetFirmwareCatalogReload(receipt) } catch (error) { /* No success or rollback is inferred. */ }
    }
    function clearChecks() { var guard = lifetime, actualView = dialog; if (guard.alive && actualView) actualView.clearConfirmations() }
    function invalidateReview(message) {
        var guard = lifetime, owned = guard.owned
        if (!guard.alive) return
        guard.owned = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        var epoch = guard.epoch
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
        callBusy = false
        if (guard.activeCall === ticket) guard.activeCall = null
    }
    function requestReview(pressed, confirmed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canRequest || !pressCurrent(pressed) || confirmed !== true) { invalidateReview(qsTr("来源或确认已变化；请重新核对后申请预览。")); return }
        var owner = controller, source = sourceToken, epoch = guard.epoch, view = guard.view, started = Date.now(), ticket = {}, result = {}
        // This pure value extractor and original-owner cancel closure remain
        // safe to use after a native callback destroys the panel itself.
        var extract = function(value) {
            if (!value || Object.keys(value).length !== 5) return null
            var keys = ["workflowEpoch", "reviewId", "reviewGeneration", "bindingHash", "scopeToken"]
            for (var k = 0; k < keys.length; ++k) if (typeof value[keys[k]] !== "string") return null
            if (!/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value.workflowEpoch)
                    || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value.reviewId)
                    || !/^[1-9][0-9]{0,19}$/.test(value.reviewGeneration) || !/^[0-9a-f]{64}$/.test(value.bindingHash)
                    || !value.scopeToken.length || value.scopeToken.length > 32768) return null
            return {workflowEpoch: value.workflowEpoch, reviewId: value.reviewId, reviewGeneration: value.reviewGeneration, bindingHash: value.bindingHash, scopeToken: value.scopeToken}
        }
        var abandon = function(receipt) { if (owner && receipt) try { owner.cancelFleetFirmwareCatalogReload(receipt) } catch (error) {} }
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            try { result = owner.requestFleetFirmwareCatalogReload(source, true) || {} } catch (error) { result = {reason: "catalog-preview-call-unavailable"} }
            var receipt = extract(result.receipt)
            if (!guard.alive) { abandon(receipt); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || source !== sourceToken || !visible || !dialog) { abandon(receipt); return }
            if (result.ok !== true || result.state !== "review" || workflow.state !== "review" || !receipt || receipt.scopeToken !== source
                    || !receiptToken(receipt) || receiptToken(workflow) !== receiptToken(receipt)
                    || !integer(workflow.remainingReviewMs, 1, 30000) || !previewUsable(workflow.preview)) {
                abandon(receipt)
                if (guard.alive && guard.epoch === epoch) localError = qsTr("未建立本页可审批的完整预览：%1。没有自动重试；请检查固定文件和原生原因。").arg(text(result.reason || result.state))
                return
            }
            guard.owned = {owner: owner, receipt: receipt}
            reviewOwner = owner
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            ownedReceipt = receipt
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewCapturedMs = started
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            reviewExpiresMs = started + workflow.remainingReviewMs
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockMs = Date.now()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockInvalid = clockMs < started
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function approveReview(pressed, declarationsChecked, replacementChecked) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall) return
        if (!canApprove || !pressCurrent(pressed) || pressed.receipt !== ownToken || declarationsChecked !== true || replacementChecked !== true) {
            invalidateReview(qsTr("预览已变化、到期或两项确认不完整；没有重复提交。")); return
        }
        var owned = guard.owned, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        if (!owned || owned.owner !== reviewOwner || receiptToken(owned.receipt) !== ownToken) return
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownReviewCurrent) return
            try { result = owned.owner.approveFleetFirmwareCatalogReload(owned.receipt, true) || {} } catch (error) { result = {reason: "catalog-reload-call-unavailable"} }
            if (!guard.alive) return
            // A new catalog/source token or failed post-call audit is NOT
            // rollback. Keep a returned mutation fact independent of ok and
            // of this view's now-obsolete confirmation/receipt.
            var fact = {ok: result.ok === true, reason: typeof result.reason === "string" ? result.reason : "catalog-result-unverified",
                reviewId: owned.receipt.reviewId, workflowEpoch: owned.receipt.workflowEpoch,
                reloadCommitted: typeof result.reloadCommitted === "boolean" ? result.reloadCommitted : null,
                authenticationConsumed: typeof result.authenticationConsumed === "boolean" ? result.authenticationConsumed : null,
                commitReceipt: ({}), commitReceiptAvailable: false}
            if (result.commitReceipt && result.commitReceipt.mutationCommitted === true) fact.reloadCommitted = true
            try { if (result.commitReceipt && typeof result.commitReceipt === "object" && !Array.isArray(result.commitReceipt) && Object.keys(result.commitReceipt).length > 0) { fact.commitReceipt = JSON.parse(JSON.stringify(result.commitReceipt)); fact.commitReceiptAvailable = true } } catch (error) {}
            if (fact.reloadCommitted === true) committedFact = fact
            if (!guard.alive) return
            lastResult = fact
            if (!guard.alive) return
            if (guard.epoch === epoch && guard.view === view && result.ok !== true)
                localError = qsTr("审批返回未核实：%1。请先看已提交事实和原生回执；不自动重试、不推断回滚。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function authenticate(pressed) {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.activeCall || !canAuthenticate || !pressCurrent(pressed)) return
        var owner = controller, epoch = guard.epoch, view = guard.view, ticket = {}, result = {}
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            try { result = owner.authenticateFleetOperator() || {} } catch (error) { result = {reason: "catalog-operator-authentication-unavailable"} }
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            if (result.ok !== true) localError = qsTr("账号未核实：%1。认证不会自动申请预览或替换目录。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea, bottom = point.y + item.height + pad
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (bottom > scroll.contentY + scroll.height) scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), bottom - scroll.height)
    }
    function finishView(actualView) {
        var guard = lifetime
        if (!guard.alive || dialog !== actualView) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (guard.alive && visible && !dialog) entryButton.forceActiveFocus()
    }
    function dismissView() {
        var guard = lifetime, actualView = dialog
        if (!guard.alive) return
        guard.view = null
        invalidateReview("")
        if (!guard.alive) return
        dialog = null
        if (actualView) try { actualView.close() } catch (error) {}
    }
    function openView() {
        var guard = lifetime
        if (!guard.alive || guard.exhausted || guard.opening || guard.activeCall || !controller || !visible || dialog) return
        var opening = {}, owner = controller, actualView = null
        guard.opening = opening
        try {
            localError = ""
            if (!guard.alive || guard.opening !== opening) return
            actualView = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: guard})
            if (!guard.alive) { if (actualView) actualView.destroy(); return }
            if (owner !== controller || !visible || dialog || !actualView) { if (actualView) actualView.destroy(); return }
            guard.view = opening
            dialog = actualView
            if (!guard.alive || guard.view !== opening) { actualView.destroy(); return }
            actualView.open()
        } finally { if (guard.opening === opening) guard.opening = null }
    }
    function resultText() {
        var busyText = callBusy || backendBusy ? qsTr("原生核验或系统账号提示进行中；不重复提交。耗时计入原30秒审阅期限，不续期。") : ""
        if (committedFact.reloadCommitted === true) return (busyText ? busyText + "\n" : "") + qsTr("历史事实：本页某次本地目录替换已提交（审阅 %1）。后置失败或取消不会回滚。是否仍为当前目录须看原生回执；这不是固件认证、飞行许可或运行验证。").arg(text(committedFact.reviewId))
        if (busyText) return busyText
        if (reviewPending && !ownReviewCurrent) return qsTr("另一个入口拥有当前审阅或调用。本页不接管、不取消它，请在原入口处理。")
        if (ownReviewCurrent) return qsTr("仅本页持有此预览；两项确认后还必须由不同 SID 的安全员、总指挥或管理员独立审批。")
        return qsTr("尚无本页核实的替换事实。原生状态：%1；原因：%2。未知、取消和失败均不表示目录已回滚。").arg(text(workflow.state)).arg(text(workflow.reason))
    }
    onSourceTokenChanged: invalidateReview(qsTr("任务组、账号、策略或目录来源已变化；本页原确认已取消。"))
    onSourceReadyChanged: if (!sourceReady && lifetime.alive && lifetime.owned) invalidateReview(qsTr("来源不可核实；本页原审阅已取消。"))
    onOwnReviewCurrentChanged: if (!ownReviewCurrent && lifetime.alive && lifetime.owned) invalidateReview(qsTr("本页原收据已失效；不接管其他入口。"))
    onWorkflowChanged: if (lifetime.alive && lifetime.owned && ["review", "approving"].indexOf(workflow.state) < 0) invalidateReview("")
    onControllerChanged: dismissView()
    onVisibleChanged: if (!visible) dismissView()
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: {
        var guard = lifetime, owned = guard.owned, actualView = dialog
        guard.owned = null; guard.alive = false; guard.view = null
        if (owned && owned.owner) try { owned.owner.cancelFleetFirmwareCatalogReload(owned.receipt) } catch (error) {}
        if (actualView) try { actualView.close() } catch (error) {}
    }
    Timer {
        interval: 250; repeat: true; running: !!root.dialog || root.callBusy
        onTriggered: {
            var guard = root.lifetime, now = Date.now()
            if (now < root.clockMs) {
                root.clockInvalid = true
                if (!guard.alive) return
                root.invalidateReview(qsTr("显示时钟回退；已取消本页审阅，请手动重新申请。"))
                if (!guard.alive) return
            }
            root.clockMs = now
            if (!guard.alive) return
            if (guard.owned && root.displayRemainingMs <= 0) root.invalidateReview(qsTr("本页显示期限已到；原生30秒期限仍为最终依据。"))
        }
    }

    ColumnLayout {
        id: entry
        width: root.width; spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("固件工程声明目录 · 预览与全量重载")
            Accessible.name: text; enabled: !!root.controller && !root.callBusy && !root.lifetime.exhausted
            onClicked: root.openView()
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("固定本地文件；预览不修改目录，审批只替换工程声明，不刷固件。") }
        QGCLabel { Layout.fillWidth: true; visible: root.committedFact.reloadCommitted === true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.resultText(); color: qgcPal.warningText }
        QGCLabel { Layout.fillWidth: true; visible: !root.dialog && !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.localError; color: qgcPal.warningText }
    }

    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var localViewLife: ({alive: true})
            property bool showBefore: false
            property bool showAfter: false
            property bool showCommit: false
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true; closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton
            padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 66, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: qgcPal.window; border.color: qgcPal.text; border.width: 1; radius: 3 }
            header: QGCLabel { id: heading; padding: root.pad; text: qsTr("本地固件工程声明 · 全量替换，非固件认证"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true }
            function clearConfirmations() {
                var life = guard, ownLife = localViewLife
                if (!life || !life.alive || !ownLife.alive) return
                previewConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                declarationsConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                replacementConsent.checked = false
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
            onOpened: closeButton.forceActiveFocus()
            onAboutToHide: { var life = guard; if (life && life.alive && ownerPanel) ownerPanel.finishView(view) }
            onClosed: {
                var life = guard, actualView = view
                localViewLife.alive = false
                if (life && life.alive && ownerPanel) ownerPanel.finishView(actualView)
                try { actualView.destroy() } catch (error) {}
            }
            Component.onDestruction: { var life = guard; localViewLife.alive = false; if (life && life.alive && ownerPanel) ownerPanel.finishView(view) }
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
                        id: closeButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("取消 / 关闭本页（不回滚已提交的替换）"); Accessible.name: text
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                        contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: closeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: closeButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: qsTr("这会全量替换当前本地工程声明，不是合并。遗漏声明会删除，内容变化会废弃旧任务审阅与批准依据。文件缺项、变化、超时或审批失败均不自动降级或重试。声明中的原 engineeringReview 不会被本次审批人覆盖；它仍不证明固件签名、完整提交身份、硬件构型、起降净空或允许飞行。")
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("固定文件路径（不可在此选择或编辑）：\n%1\n账号：%2\n角色：%3\n原生状态：%4").arg(root.text(root.workflow.fixedFilePath)).arg(root.text(root.identity.actorId)).arg(root.text(root.identity.role)).arg(root.text(root.workflow.state)) }
                    FocusScope {
                        id: errorSummary
                        Layout.fillWidth: true; Layout.preferredHeight: errorLabel.implicitHeight + root.pad
                        visible: !!root.localError; activeFocusOnTab: visible
                        Accessible.role: Accessible.StaticText; Accessible.name: root.localError
                        Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: qgcPal.warningText; border.width: errorSummary.activeFocus ? 3 : 1; radius: 3 }
                        QGCLabel { id: errorLabel; anchors.fill: parent; anchors.margins: root.pad / 2; text: root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    }
                    QGCButton {
                        id: authenticateButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("认证本地操作员（维修员 / 管理员方可申请）"); Accessible.name: text; enabled: root.canAuthenticate
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope; pressScope = ({}); if (life && life.alive && ownLife.alive) root.authenticate(pressed) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authenticateButton)
                        contentItem: QGCLabel { text: authenticateButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: authenticateButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: authenticateButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: authenticateButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    Controls.CheckBox {
                        id: previewConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("仅读取上方固定文件并生成全量替换预览；当前目录此步不修改")
                        checked: false; enabled: root.canRequest; Accessible.name: text
                        contentItem: QGCLabel { text: previewConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (previewConsent.indicator ? previewConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(previewConsent)
                    }
                    QGCButton {
                        id: requestButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("生成固定文件预览（不替换）"); Accessible.name: text; enabled: root.canRequest && previewConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope, checked = previewConsent.checked; pressScope = ({}); if (life && life.alive && ownLife.alive) root.requestReview(pressed, checked) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(requestButton)
                        contentItem: QGCLabel { text: requestButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: requestButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: requestButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: requestButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.resultText(); color: qgcPal.warningText }
                    QGCLabel {
                        Layout.fillWidth: true; visible: root.ownReviewCurrent; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本页审阅：%1\n剩余显示时间：%2 s（原生期限不会延长）\n原目录 revision：%3\n文件原始 SHA-256：%4\n规范内容 SHA-256：%5\n原声明 %6 → 新声明 %7；新增 %8 / 删除 %9 / 修改 %10 / 未变 %11")
                            .arg(root.text(root.ownedReceipt.reviewId)).arg(Math.floor(root.displayRemainingMs / 1000))
                            .arg(root.text((root.workflow.catalogIdentity || {}).revision)).arg(root.text((root.workflow.fileObservation || {}).rawSha256))
                            .arg(root.text((root.workflow.preview || {}).normalizedFingerprint))
                            .arg((root.workflow.preview || {}).previousEntryCount).arg((root.workflow.preview || {}).descriptorCount)
                            .arg((root.workflow.preview || {}).addedCount).arg((root.workflow.preview || {}).removedCount)
                            .arg((root.workflow.preview || {}).changedCount).arg((root.workflow.preview || {}).unchangedCount)
                    }
                    Repeater {
                        model: root.changeRows
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: root.pad / 2
                            readonly property var row: modelData
                            QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.kindText(parent.row.kind) + " · " + parent.row.id }
                            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("原 engineeringReview：\n%1\n候选 engineeringReview：\n%2").arg(root.reviewText(parent.row.before)).arg(root.reviewText(parent.row.after)) }
                        }
                    }
                    QGCButton {
                        id: beforeDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; visible: root.previewReady
                        text: view.showBefore ? qsTr("折叠完整原声明（含原工程审阅）") : qsTr("展开完整原声明（最多64条，不截断）")
                        Accessible.name: text; onClicked: view.showBefore = !view.showBefore
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(beforeDetailsButton)
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showBefore && root.previewReady
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.fullJson(root.workflow.preview.previousEntries); color: qgcPal.text; Accessible.name: qsTr("完整原声明 JSON，只读且不截断"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                    QGCButton {
                        id: afterDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; visible: root.previewReady
                        text: view.showAfter ? qsTr("折叠完整候选声明") : qsTr("展开完整候选档案（最多64条，不截断）")
                        Accessible.name: text; onClicked: view.showAfter = !view.showAfter
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(afterDetailsButton)
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showAfter && root.previewReady
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.workflow.preview.normalizedJson; color: qgcPal.text; Accessible.name: qsTr("完整候选声明档案 JSON，只读且不截断"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                    Controls.CheckBox {
                        id: declarationsConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("已核对固定路径、文件哈希、完整原/新声明、增删改清单及原 engineeringReview")
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: declarationsConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (declarationsConsent.indicator ? declarationsConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(declarationsConsent)
                    }
                    Controls.CheckBox {
                        id: replacementConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("我确认全量替换并使旧审阅失效；随后使用不同账号独立审批，不把本地声明视为固件认证或飞行许可")
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: replacementConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (replacementConsent.indicator ? replacementConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(replacementConsent)
                    }
                    QGCButton {
                        id: approveButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("独立账号审批并全量替换本地声明"); Accessible.name: text
                        enabled: root.canApprove && declarationsConsent.checked && replacementConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, ownLife = view.localViewLife, pressed = pressScope, declarations = declarationsConsent.checked, replacement = replacementConsent.checked; pressScope = ({}); if (life && life.alive && ownLife.alive) root.approveReview(pressed, declarations, replacement) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(approveButton)
                        contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: approveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: approveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.warningText; border.width: approveButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !!root.lastResult.reviewId; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("本页上次审批返回（不是当前目录认证）：\n审阅 %1\nok：%2 · 认证已消费：%3 · 替换已提交：%4\n原因：%5\n已提交事实不因 ok=false、来源变化或关闭而改为未提交。")
                            .arg(root.text(root.lastResult.reviewId)).arg(root.known(root.lastResult.ok)).arg(root.known(root.lastResult.authenticationConsumed)).arg(root.known(root.lastResult.reloadCommitted)).arg(root.text(root.lastResult.reason))
                    }
                    QGCButton {
                        id: commitDetailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        visible: root.committedFact.reloadCommitted === true
                        text: view.showCommit ? qsTr("折叠历史原生提交回执") : qsTr("展开历史原生提交回执（完整只读）")
                        Accessible.name: text; onClicked: view.showCommit = !view.showCommit
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(commitDetailsButton)
                    }
                    Loader {
                        Layout.fillWidth: true; active: view.showCommit && root.committedFact.reloadCommitted === true
                        sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText; text: root.committedFact.commitReceiptAvailable === true ? root.fullJson(root.committedFact.commitReceipt) : qsTr("已返回提交为真，但完整回执无法显示；不推断回滚或仍为当前，请交工程师核查。"); color: qgcPal.text; Accessible.name: qsTr("历史原生提交回执 JSON，只读且不截断"); background: Rectangle { color: qgcPal.windowShade } } }
                        Layout.preferredHeight: item ? item.implicitHeight : 0
                    }
                }
            }
        }
    }
}

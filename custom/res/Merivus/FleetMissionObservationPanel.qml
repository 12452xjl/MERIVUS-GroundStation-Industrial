import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Local observation only. This view neither sends flight commands nor uses
// upload/plan permission as authority to close an older actual Run.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var reviewOwner: null
    property var ownedReview: ({})
    property var lifetime: ({alive: true, epoch: 0, exhausted: false})
    property bool callBusy: false
    property var callTicket: null
    property string localError: ""
    property double clockMs: Date.now()
    property double reviewCapturedMs: -1
    property double reviewExpiresMs: -1
    property bool clockInvalid: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var run: controller ? controller.fleetMissionExecutionObservation || {} : ({})
    readonly property var series: controller ? controller.fleetMissionObservationSeries || {} : ({})
    readonly property string sourceToken: controller ? controller.fleetObservationSourceToken || "" : ""
    readonly property var identity: controller ? controller.fleetObservationOperatorIdentity || {} : ({})
    readonly property var closeState: controller ? controller.fleetObservationClose || {} : ({})
    readonly property bool backendProcessing: closeState.busy === true
        || closeState.state === "authenticating" || closeState.state === "consuming" || closeState.state === "committing"
    readonly property bool currentRun: sourceToken.length > 0 && series.evidenceLost === false && run.prepared === true
        && uuid(run.runId) && uuid(run.startupId) && uuid(run.transactionEpoch)
        && series.previousStartupId === run.startupId && series.previousTransactionEpoch === run.transactionEpoch
    readonly property string runToken: currentRun ? JSON.stringify({sourceToken: sourceToken, runId: run.runId, startupId: run.startupId,
        transactionEpoch: run.transactionEpoch, transactionGeneration: run.transactionGeneration,
        deploymentId: run.deploymentId, executionObservationTaskId: run.executionObservationTaskId,
        binding: run.binding, targetVehicleIds: run.targetVehicleIds}) : ""
    readonly property string ownToken: reviewToken(ownedReview) && ownedReview.observationSourceToken
        ? JSON.stringify({receipt: reviewToken(ownedReview), sourceToken: ownedReview.observationSourceToken}) : ""
    readonly property bool ownReviewCurrent: reviewOwner === controller && ownToken.length > 0
        && ownedReview.observationSourceToken === sourceToken && reviewToken(ownedReview) === reviewToken(closeState)
    readonly property double displayRemainingMs: !clockInvalid && reviewCapturedMs >= 0 && clockMs >= reviewCapturedMs
        ? Math.max(0, reviewExpiresMs - clockMs) : 0
    readonly property bool canReview: controller && lifetime.alive && !lifetime.exhausted
        && currentRun && run.startupRetired === true && run.retired === false
        && !callBusy && !backendProcessing && closeState.reviewActive !== true
        && typeof controller.requestFleetObservationCloseReview === "function"
        && typeof controller.approveFleetObservationClose === "function"
        && typeof controller.cancelFleetObservationCloseReview === "function"
    readonly property bool canApprove: ownReviewCurrent && closeState.state === "awaiting-review"
        && closeState.ok === true && closeState.reviewActive === true && displayRemainingMs > 0
        && !callBusy && !backendProcessing
    readonly property bool canAuthenticate: controller && lifetime.alive && !lifetime.exhausted && sourceToken.length > 0
        && !callBusy && !backendProcessing && closeState.reviewActive !== true
        && typeof controller.authenticateFleetObservationOperator === "function"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function text(value) { return typeof value === "string" && value.length > 0 ? value.substring(0, 512) : qsTr("未知") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function ids(value) {
        if (!value || typeof value === "string" || typeof value.length !== "number" || !isFinite(value.length)
                || Math.floor(value.length) !== value.length || value.length < 0 || value.length > 32) return qsTr("未知")
        var seen = {}
        for (var i = 0; i < value.length; ++i) {
            if (typeof value[i] !== "number" || Math.floor(value[i]) !== value[i] || value[i] < 1 || value[i] > 255 || seen[value[i]]) return qsTr("未知")
            seen[value[i]] = true
        }
        return value.length ? value.join(", ") : qsTr("无本地占用成员")
    }
    function reviewToken(value) {
        return value && uuid(value.reviewId) && uuid(value.workflowEpoch) && typeof value.generation === "string" && value.context
            ? JSON.stringify({reviewId: value.reviewId, workflowEpoch: value.workflowEpoch, generation: value.generation, context: value.context}) : ""
    }
    function advanceEpoch() {
        if (lifetime.epoch >= 9007199254740991) lifetime.exhausted = true
        else lifetime.epoch += 1
    }
    function clearChecks() { if (dialog) dialog.clearConfirmations() }
    function cancelExact(owner, review) {
        if (!owner || !reviewToken(review)) return
        try {
            // Root routes only its issued full receipt to the retained native
            // Close. Selection may already have moved; never query new aliases.
            owner.cancelFleetObservationCloseReview(review.reviewId, review)
        } catch (error) { /* Owner may be gone or commit may already have happened. No rollback claim. */ }
    }
    function cancelOwned() {
        var owner = reviewOwner, review = ownedReview
        ownedReview = ({}); reviewOwner = null; reviewCapturedMs = -1; reviewExpiresMs = -1
        clearChecks()
        cancelExact(owner, review)
    }
    function dismissView() {
        advanceEpoch()
        var old = dialog
        dialog = null
        cancelOwned()
        if (old) old.close()
    }
    function finishView(view) {
        if (dialog !== view) return
        dialog = null; advanceEpoch(); cancelOwned()
        if (visible) entryButton.forceActiveFocus()
    }
    function openView() {
        if (dialog || !controller || !visible || lifetime.exhausted) return
        localError = ""
        var view = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: lifetime})
        if (!view) { localError = qsTr("无法打开监视视图；未请求结束监视。请保留运行状态并联系工程师。"); return }
        dialog = view; view.open()
    }
    function requestReview(pressed) {
        if (!canReview || !lifetime.alive || lifetime.exhausted || !pressed || pressed.owner !== controller || pressed.epoch !== lifetime.epoch || pressed.run !== runToken) {
            localError = qsTr("运行来源在点击期间已变化；请核对实际运行后重新申请。"); clearChecks(); return
        }
        cancelOwned()
        var owner = controller, source = run, selected = sourceToken, guard = lifetime, epoch = guard.epoch, result = {}, startedMs = Date.now()
        // This closure must remain usable even if a native identity/source
        // callback destroys the panel before requestReview returns.
        var abandon = function(receipt) {
            if (!owner || !receipt || typeof receipt.reviewId !== "string" || !receipt.context) return
            try {
                owner.cancelFleetObservationCloseReview(receipt.reviewId, receipt)
            } catch (error) { /* Missing source/finished effect is not rollback. */ }
        }
        var ticket = {}
        callTicket = ticket; callBusy = true; localError = ""
        try { result = owner.requestFleetObservationCloseReview(source.startupId, source.runId, true, selected) || {} }
        catch (error) { result = {reason: "close-review-call-unavailable"} }
        if (guard.alive && callTicket === ticket) { callTicket = null; callBusy = false }
        if (!guard.alive || guard.epoch !== epoch) { abandon(result); return }
        if (owner !== controller || selected !== sourceToken || result.observationSourceToken !== selected
                || !visible || !dialog || source.runId !== run.runId || source.startupId !== run.startupId
                || result.state !== "awaiting-review" || result.reviewActive !== true || !reviewToken(result)
                || result.context.runId !== source.runId || result.context.startupId !== source.startupId
                || typeof result.remainingMs !== "number" || !isFinite(result.remainingMs) || result.remainingMs <= 0 || result.remainingMs > 30000) {
            cancelExact(owner, result)
            localError = qsTr("未建立本次关闭审阅：%1。可能是身份、来源、权限或排空门禁不满足；不自动重试，不恢复旧批准。")
                .arg(text(result.reason || result.state))
            clearChecks(); return
        }
        reviewOwner = owner
        ownedReview = JSON.parse(JSON.stringify({reviewId: result.reviewId, workflowEpoch: result.workflowEpoch,
            generation: result.generation, context: result.context, observationSourceToken: result.observationSourceToken}))
        reviewCapturedMs = startedMs; clockMs = Date.now(); clockInvalid = clockMs < startedMs
        // Subtract request cost conservatively; never rebase on callbacks.
        reviewExpiresMs = startedMs + result.remainingMs
        clearChecks()
    }
    function approveReview(pressed, monitor, release, mayContinue) {
        if (!canApprove || !pressed || pressed.owner !== controller || pressed.epoch !== lifetime.epoch || pressed.review !== ownToken
                || monitor !== true || release !== true || mayContinue !== true) {
            localError = qsTr("审阅已变化、到期或确认不完整。没有重发；请重新核对本地监视状态。"); clearChecks(); return
        }
        var owner = reviewOwner, receipt = ownedReview, reviewId = receipt.reviewId, guard = lifetime, epoch = guard.epoch
        var ticket = {}
        localError = ""; callTicket = ticket; callBusy = true; clearChecks()
        // The native modal may destroy this panel. The plain JS lifetime
        // guard survives it; do not read a deleted QObject on return. Keep
        // callBusy latched for the ENTIRE call, even if current is stale.
        try { owner.approveFleetObservationClose(reviewId, true, true, true, receipt) }
        catch (error) {
            if (guard.alive && guard.epoch === epoch) localError = qsTr("审批调用中断；不能据此判断监视或占用未变化。请查看下面的原生结果，不自动重试。")
        }
        if (guard.alive && callTicket === ticket) { callTicket = null; callBusy = false }
    }
    function authenticateObservation(pressed) {
        if (!canAuthenticate || !pressed || pressed.owner !== controller || pressed.epoch !== lifetime.epoch || pressed.source !== sourceToken) {
            localError = qsTr("监视来源已变化；请核对当前对象后重新认证。"); clearChecks(); return
        }
        var guard = lifetime, epoch = guard.epoch, owner = controller, selected = sourceToken, ticket = {}, result = {}
        callTicket = ticket; callBusy = true; localError = ""; clearChecks()
        try { result = owner.authenticateFleetObservationOperator(selected) || {} }
        catch (error) { result = {ok: false, reason: "observation-authentication-unavailable"} }
        if (!guard.alive) return
        if (callTicket === ticket) { callTicket = null; callBusy = false }
        if (guard.epoch !== epoch || owner !== controller || selected !== sourceToken) return
        if (result.ok !== true) localError = qsTr("本监视账号未核实：%1。请重新核对来源与账号；没有恢复旧批准或发送飞行指令。").arg(text(result.reason))
    }
    function ensureFocusVisible(item) {
        var parent = item ? item.parent : null
        while (parent) {
            if (parent.contentItem && typeof parent.contentY === "number" && typeof parent.contentHeight === "number") {
                var point = item.mapToItem(parent.contentItem, 0, 0)
                var top = point.y - pad, bottom = point.y + item.height + pad
                if (top < parent.contentY) parent.contentY = Math.max(0, top)
                else if (bottom > parent.contentY + parent.height) parent.contentY = Math.min(Math.max(0, parent.contentHeight - parent.height), bottom - parent.height)
                return
            }
            parent = parent.parent
        }
    }
    function resultText() {
        if (callBusy || backendProcessing)
            return qsTr("正在核验账号或处理本地监视审阅；请勿重复提交。系统提示耗时计入原30秒期限，界面快照可能尚未更新。")
        if (closeState.monitoringRetiredKnown === true && closeState.monitoringRetired === true) {
            if (closeState.localReservationsReleaseKnown === true && closeState.localReservationsReleased === true)
                return qsTr("本地监视已结束；本次两个所有者的本地占用表已核为空。这不证明飞机停飞、任务完成或成员适合重新分配。")
            if (closeState.localReservationsReleaseKnown === true && closeState.localReservationsReleased === false)
                return qsTr("本地监视已结束，但已知仍有本地占用。不要据此重新分配成员；不会自动补释放或恢复权限。")
            return qsTr("本地监视已结束；占用释放结果未知。不要把未知视为已释放或已停止飞行。")
        }
        if (closeState.state === "awaiting-review")
            return qsTr("等待明确确认；仅有本地审阅快照，提交前原生后端仍会重新核验账号、实际来源、占用与期限。")
        if (closeState.state === "empty" || !closeState.state) return qsTr("尚未申请关闭审阅。查看本页不会结束监视，也不会发送飞行指令。")
        return qsTr("没有已核实的监视结束结果。状态：%1；原因：%2。取消、失败或过期不证明飞机停飞，也不恢复旧批准。")
            .arg(text(closeState.state)).arg(text(closeState.reason))
    }
    onControllerChanged: { advanceEpoch(); dismissView(); callTicket = null; callBusy = false }
    onSourceTokenChanged: {
        advanceEpoch(); cancelOwned(); clearChecks()
        if (dialog) localError = qsTr("监视对象或选择代次已变化；旧确认已清除。请重新核对并申请审阅。")
    }
    onVisibleChanged: if (!visible) dismissView()
    onOwnTokenChanged: clearChecks()
    onOwnReviewCurrentChanged: if (!ownReviewCurrent) clearChecks()
    onCloseStateChanged: {
        var state = closeState.state || ""
        if (["stale", "failed", "cancelled", "expired"].indexOf(state) >= 0
                || state.indexOf("closed") === 0 || state.indexOf("close-unverified") === 0) clearChecks()
    }
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: { lifetime.alive = false; advanceEpoch(); dismissView() }
    Timer {
        interval: 250; repeat: true; running: !!root.dialog || root.callBusy
        onTriggered: {
            var now = Date.now()
            if (now < root.clockMs) { root.clockInvalid = true; root.clearChecks() }
            root.clockMs = now
            if (root.ownedReview.reviewId && root.displayRemainingMs <= 0) root.clearChecks()
        }
    }

    ColumnLayout {
        id: entry
        width: root.width
        spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("运行监视 · 查看与结束审阅")
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
            Accessible.name: text
            enabled: !!root.controller && !root.lifetime.exhausted
            onClicked: root.openView()
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.currentRun ? (root.run.retired === true ? qsTr("当前本地监视已退休；不代表停飞或任务完成。") : qsTr("有实际运行监视。结束监视不是停止飞机。"))
                : qsTr("没有可操作的当前运行来源；仍可查看本地结果与未知状态。")
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.dialog && !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.localError; color: qgcPal.warningText }
        FleetMissionExecutionReportPanel {
            Layout.fillWidth: true
            controller: root.controller
        }
    }

    Component {
        id: viewComponent
        QGCPopupDialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property bool showBindingDetails: false
            readonly property var backendState: ownerPanel ? ownerPanel.closeState : ({})
            readonly property var context: ownerPanel && ownerPanel.ownReviewCurrent ? ownerPanel.ownedReview.context : ({})
            title: qsTr("运行监视 · 不停止飞机")
            buttons: 0; acceptAllowed: false; rejectAllowed: false
            destroyOnClose: false; closePolicy: Controls.Popup.NoAutoClose
            function clearConfirmations() { stopConsent.checked = false; releaseConsent.checked = false; continueConsent.checked = false; approveButton.pressScope = ({}) }
            function focusError() { if (errorSummary.visible) { errorSummary.forceActiveFocus(); if (ownerPanel) ownerPanel.ensureFocusVisible(errorSummary) } }
            onOpened: closeButton.forceActiveFocus()
            onClosed: { if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view); destroy() }
            ColumnLayout {
                width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 52, view._maxContentWidth - view._contentMargin * 2))
                spacing: root.pad
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) { view.close(); event.accepted = true }
                    else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true
                }
                QGCButton {
                    id: closeButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("关闭视图 / 取消本页审阅（不停止飞机）")
                    contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: closeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: closeButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                    Accessible.name: text
                    onClicked: view.close()
                    onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                    Keys.onReturnPressed: { view.close(); event.accepted = true }
                    Keys.onEnterPressed: { view.close(); event.accepted = true }
                }
                QGCLabel {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: qgcPal.warningText
                    text: qsTr("仅结束地面站对当前Run的本地监视，并尝试释放它和原启动所有者的本地占用。无人机可能继续飞行；不会发送停止、降落、返航或上锁指令，不宣布任务Completed。需要停飞时请使用原飞行安全操作。")
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("实际当前Run：%1\n启动：%2\n本地观察任务：%3\n原成员：%4\n旧启动已退休：%5 · 本地监视已退休：%6\n只有实际Series当前owner可操作；不按飞机ID找替代对象，也不要求当前草稿有效。")
                        .arg(root.text(root.run.runId)).arg(root.text(root.run.startupId)).arg(root.text(root.run.executionObservationTaskId))
                        .arg(root.ids(root.run.targetVehicleIds)).arg(root.known(root.run.startupRetired)).arg(root.known(root.run.retired))
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: root.resultText()
                }
                FocusScope {
                    id: errorSummary
                    Layout.fillWidth: true; Layout.preferredHeight: errorLabel.implicitHeight + root.pad
                    visible: !!root.localError; activeFocusOnTab: visible
                    Accessible.role: Accessible.StaticText
                    Accessible.name: root.localError
                    Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: qgcPal.warningText; border.width: errorSummary.activeFocus ? 3 : 1; radius: 3 }
                    QGCLabel {
                        id: errorLabel
                        anchors.fill: parent; anchors.margins: root.pad / 2
                        textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: root.localError; color: qgcPal.warningText
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: qsTr("本监视账号：%1。重新认证只选择此监视的身份来源，可能撤销当前部署尚未使用的批准；不会结束其他Run或发送飞行指令。")
                        .arg(root.identity.authenticated === true ? qsTr("已认证，提交时仍由后端重验") : qsTr("未核实"))
                }
                QGCButton {
                    id: identityButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("重新认证本监视账号（不启动飞行）")
                    Accessible.name: text
                    enabled: root.canAuthenticate
                    property var pressScope: ({})
                    onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, source: root.sourceToken}
                    onCanceled: pressScope = ({})
                    onClicked: { var scope = pressScope; pressScope = ({}); root.authenticateObservation(scope) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(identityButton)
                    contentItem: QGCLabel { text: identityButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: identityButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: identityButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: identityButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCButton {
                    id: requestButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("生成本次关闭审阅（不结束监视）")
                    contentItem: QGCLabel { text: requestButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: requestButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: requestButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: requestButton.activeFocus ? 3 : 1; radius: 3 }
                    Accessible.name: text
                    enabled: root.canReview
                    property var pressScope: ({})
                    onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, run: root.runToken}
                    onCanceled: pressScope = ({})
                    onClicked: { var scope = pressScope; pressScope = ({}); root.requestReview(scope) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(requestButton)
                }
                QGCLabel {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                    visible: !root.canReview && !root.ownReviewCurrent
                    text: root.backendProcessing || root.callBusy ? qsTr("后端正在处理；禁止重复申请。")
                        : root.closeState.reviewActive === true ? qsTr("另一入口已有当前审阅；本页不能接管或取消它。请回到原入口处理，或等待原期限结束。")
                        : qsTr("需保留当前实际Run、旧启动终态及静默传输栈；账号、独立结束监视权限等由后端核验，不借用上传权限。未满足时请检查原生原因，不自动重试。")
                }
                QGCLabel {
                    Layout.fillWidth: true; visible: root.ownReviewCurrent; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("本页冻结审阅：%1\nRun / 启动：%2 / %3\n部署：%4\n记录 / 版本：%5 / %6\n计划：%7\n原成员：%8\n角色：%9\n剩余约 %10 秒（仅显示，不能续期；最终原生重验）")
                        .arg(root.text(root.ownedReview.reviewId)).arg(root.text(view.context.runId)).arg(root.text(view.context.startupId))
                        .arg(root.text(view.context.deploymentId)).arg(root.text((view.context.binding || {}).recordId))
                        .arg(String((view.context.binding || {}).recordRevision === undefined ? qsTr("未知") : view.context.binding.recordRevision))
                        .arg(root.text((view.context.binding || {}).planId)).arg(root.ids(view.context.targetVehicleIds)).arg(root.text(view.backendState.roleLabel))
                        .arg(Math.ceil(root.displayRemainingMs / 1000))
                }
                Repeater {
                    model: root.ownReviewCurrent ? ["run", "startup"] : []
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        property var reservation: ((view.context.localReservationState || {})[modelData]) || ({})
                        text: qsTr("冻结%1所有者：%2\n实际本地占用：%3\n有效权限已安全撤销：%4（撤销不等于占用消失）")
                            .arg(modelData === "run" ? qsTr("Run") : qsTr("原启动")).arg(root.text(reservation.owner))
                            .arg(root.ids(reservation.occupiedVehicleIds)).arg(root.known(reservation.authorityRevoked))
                    }
                }
                Repeater {
                    // These are the fixed native review rows, not rolling
                    // telemetry. Unknown/lost sources are part of the risk.
                    model: root.ownReviewCurrent && Array.isArray(view.context.targets) && view.context.targets.length <= 32 ? view.context.targets : []
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("冻结成员 ID %1 / 组件 %2\nMODE步骤：%3\n曾开始排队 / 已捕获提交 / 已发出或不确定：%4 / %5 / %6\n连续性未知：%7\n末次来源绑定 / 跟踪：%8 / %9\n原Vehicle / Mission对象仍保留：%10 / %11\n仅审阅时的本地事实，不证明当前遥测新鲜、飞机停止或机载任务结果。")
                            .arg(String(modelData.vehicleId)).arg(String(modelData.componentId)).arg(root.text(modelData.modeStepId))
                            .arg(root.known(modelData.modeQueueBegan)).arg(root.known(modelData.modeQueueCommitted)).arg(root.known(modelData.modeIssuedOrUncertain))
                            .arg(root.known(modelData.continuityUnknown)).arg(root.known(modelData.sourceBindingActiveAtLastObservation)).arg(root.known(modelData.trackingAtLastObservation))
                            .arg(root.known(modelData.vehicleObjectRetained)).arg(root.known(modelData.missionObjectRetained))
                    }
                }
                QGCButton {
                    id: bindingButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.ownReviewCurrent
                    text: view.showBindingDetails ? qsTr("收起完整冻结绑定") : qsTr("展开完整冻结绑定")
                    Accessible.name: text
                    onClicked: view.showBindingDetails = !view.showBindingDetails
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(bindingButton)
                    contentItem: QGCLabel { text: bindingButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: bindingButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: bindingButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: bindingButton.activeFocus ? 3 : 1; radius: 3 }
                }
                QGCLabel {
                    Layout.fillWidth: true; visible: root.ownReviewCurrent && view.showBindingDetails
                    textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("事务epoch / generation：%1 / %2\n原意图任务：%3\n内容hash：%4\n审阅证据hash：%5\n记录库epoch：%6\n输入 / 审阅代次：%7 / %8\n完整绑定由原生保留；同一wire内容不允许跨版本或跨代复用批准。")
                        .arg(root.text(view.context.transactionEpoch)).arg(root.text(view.context.transactionGeneration))
                        .arg(root.text((view.context.binding || {}).sourceTaskId)).arg(root.text((view.context.binding || {}).contentHash))
                        .arg(root.text((view.context.binding || {}).reviewEvidenceHash)).arg(root.text((view.context.binding || {}).storeEpoch))
                        .arg(view.context.binding && view.context.binding.inputGeneration !== undefined ? String(view.context.binding.inputGeneration) : qsTr("未提供（7字段绑定）"))
                        .arg(view.context.binding && view.context.binding.reviewGeneration !== undefined ? String(view.context.binding.reviewGeneration) : qsTr("未提供（7字段绑定）"))
                }
                QGCLabel {
                    Layout.fillWidth: true; visible: root.ownReviewCurrent; wrapMode: Text.WordWrap
                    text: qsTr("下面三项每次新审阅均需重新勾选。确认后使用Windows原生窗口认证另一个合格账号；密码不在本页面输入。两个账号不等于证明两名人员。")
                }
                Controls.CheckBox {
                    id: stopConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.ownReviewCurrent; enabled: root.canApprove; checked: false
                    text: qsTr("我确认结束本次地面站运行监视，将失去该监视的后续观测")
                    Accessible.name: text
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(stopConsent)
                    contentItem: QGCLabel { text: stopConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (stopConsent.indicator ? stopConsent.indicator.width : 0) + root.pad }
                }
                Controls.CheckBox {
                    id: releaseConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.ownReviewCurrent; enabled: root.canApprove; checked: false
                    text: qsTr("我确认仅释放上面本Run及原启动所有者的本地占用，不是远端控制权交接")
                    Accessible.name: text
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(releaseConsent)
                    contentItem: QGCLabel { text: releaseConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (releaseConsent.indicator ? releaseConsent.indicator.width : 0) + root.pad }
                }
                Controls.CheckBox {
                    id: continueConsent
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.ownReviewCurrent; enabled: root.canApprove; checked: false
                    text: qsTr("我明确知道无人机可能继续飞行；本操作不停止飞机、不认证落地或任务完成")
                    Accessible.name: text
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(continueConsent)
                    contentItem: QGCLabel { text: continueConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (continueConsent.indicator ? continueConsent.indicator.width : 0) + root.pad }
                }
                QGCButton {
                    id: approveButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: root.ownReviewCurrent
                    text: qsTr("另一个账号审批后结束本地监视（不停止飞机）")
                    contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: approveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    enabled: root.canApprove && stopConsent.checked && releaseConsent.checked && continueConsent.checked
                    property var pressScope: ({})
                    Accessible.name: text
                    Keys.onReturnPressed: event.accepted = true
                    Keys.onEnterPressed: event.accepted = true
                    onPressed: pressScope = {owner: root.controller, epoch: root.lifetime.epoch, review: root.ownToken}
                    onCanceled: pressScope = ({})
                    onClicked: { var scope = pressScope; pressScope = ({}); root.approveReview(scope, stopConsent.checked, releaseConsent.checked, continueConsent.checked) }
                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(approveButton)
                    background: Rectangle {
                        color: approveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.windowShade
                        border.color: qgcPal.colorRed; border.width: approveButton.activeFocus ? 3 : 1; radius: 3
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("结果所属审阅：%1\n结果所属Run：%2\n本地监视已退休：%3\n本地占用释放是否已知：%4 · 全部释放：%5\nRun / 原启动仍有占用：%6 / %7\n本地效果不等于停飞、远端接管、任务Completed或成员可以重新分配。")
                        .arg(root.text(view.backendState.reviewId)).arg(root.text((view.backendState.context || {}).runId))
                        .arg(view.backendState.monitoringRetiredKnown === true ? root.known(view.backendState.monitoringRetired) : qsTr("未知"))
                        .arg(root.known(view.backendState.localReservationsReleaseKnown)).arg(view.backendState.localReservationsReleaseKnown === true ? root.known(view.backendState.localReservationsReleased) : qsTr("未知"))
                        .arg(root.known(view.backendState.runReservationCurrentlyHeld)).arg(root.known(view.backendState.startupReservationCurrentlyHeld))
                }
            }
        }
    }
}

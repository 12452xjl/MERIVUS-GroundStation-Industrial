import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Local human receipt only. No aircraft control or approval is transferred.
ColumnLayout {
    id: root
    property var manager: null
    property var controller: null
    readonly property bool expanded: handoffPopup.visible
    property bool callBusy: false
    property bool intentExhausted: false
    property var lifetime: ({alive: true, epoch: 0, ticket: null})
    property string recipientText: ""
    property string reasonText: ""
    property string noteText: ""
    property string ownedPreviewToken: ""
    property var sendConsent: null
    property var detailConsent: null
    property string message: ""
    property int filterIndex: 0
    property var pageResult: ({})
    property bool pageStale: true
    property var pageOwner: null
    property string pageActor: ""
    property string pageFilter: ""
    property var selectedDetail: ({})
    property bool detailStale: true
    property var detailOwner: null
    property bool showPreviewVehicles: false
    property bool showDetailVehicles: false
    readonly property string operatorId: manager && typeof manager.operatorId === "string" ? manager.operatorId : ""
    readonly property string identityToken: manager && typeof manager.identityToken === "string" ? manager.identityToken : ""
    readonly property bool handoffAllowed: manager && manager.handoffAllowed === true
    readonly property double publication: manager ? manager.publication : 0
    readonly property var preview: manager && manager.preview ? manager.preview : ({})
    readonly property string previewToken: bounded(preview.token, 128) ? preview.token : ""
    readonly property var previewBrief: briefKnown(preview.brief) ? preview.brief : ({})
    readonly property var run: controller ? controller.fleetMissionExecutionObservation || {} : ({})
    readonly property string sourceToken: controller ? controller.fleetObservationSourceToken || "" : ""
    readonly property bool ready: visible && expanded && manager && lifetime.alive && !intentExhausted && !callBusy && preview.busy !== true
    readonly property bool previewCurrent: ready && handoffAllowed && ownedPreviewToken.length > 0
        && previewToken === ownedPreviewToken && preview.current === true && briefKnown(preview.brief)
        && preview.recipientActorId === recipientText.trim() && preview.reason === reasonText.trim()
        && preview.fromOperator === operatorId && preview.brief.runId === run.runId
    readonly property bool canPrepare: ready && handoffAllowed && sourceToken.length > 0 && run.prepared === true
        && uuid(run.runId) && recipientValid(recipientText.trim()) && recipientText.trim() !== operatorId
        && reasonText.length <= 1024
    readonly property string filter: ["inbox", "outbox", "history", "all"][filterIndex]
    readonly property bool canList: ready && (filter === "all" || operatorId.length > 0)
    readonly property bool pageCurrent: !pageStale && pageOwner === manager && pageActor === operatorId && pageFilter === filter
    readonly property var records: pageKnown(pageResult) ? pageResult.records : []
    readonly property bool detailCurrent: !detailStale && detailOwner === manager && detailKnown(selectedDetail)
    readonly property var detailBrief: briefOf(selectedDetail)
    readonly property bool detailIsRun: detailKnown(selectedDetail) && selectedDetail.metadata.kind === "run-brief"
    readonly property string detailHash: detailIsRun && briefKnown(detailBrief) ? detailBrief.contentHash : ""
    readonly property bool canResolve: ready && handoffAllowed && detailCurrent && selectedDetail.status === "pending"
        && selectedDetail.origin === "local" && (!detailIsRun || briefKnown(detailBrief))
    readonly property bool sendConfirmed: consentCurrent(sendConsent, "send")
    readonly property bool detailConfirmed: consentCurrent(detailConsent, "resolve")
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    spacing: pad / 2
    visible: manager !== null
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function exact(value, keys) {
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function bounded(value, limit) { return typeof value === "string" && value.length <= limit }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }

    // Display checks only; the native shared validator verifies the complete
    // historical value/order contract before either report reaches this view.
    function exitCounter(value, zero) {
        return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value)
            && (zero || value !== "0") && (value.length < 20 || value <= "18446744073709551615")
    }
    function exitLess(a, b) { return a.length < b.length || (a.length === b.length && a < b) }
    function exitReceiptKnown(value) {
        return exact(value, ["sourceEpoch", "sourceSequence", "receivedMonotonicMs"]) && uuid(value.sourceEpoch)
            && exitCounter(value.sourceSequence, false) && exitCounter(value.receivedMonotonicMs, true)
            && (value.receivedMonotonicMs.length < 19 || value.receivedMonotonicMs <= "9223372036854775807")
    }
    function exitValueKnown(value) {
        if (!exact(value, ["customMode", "baseMode", "systemStatus", "priorMissionReceipt", "exitReceipt", "continuityUnknownAtExit"])
                || !integer(value.customMode, 0, 4294967295) || !integer(value.baseMode, 0, 255)
                || !integer(value.systemStatus, 0, 255) || typeof value.continuityUnknownAtExit !== "boolean"
                || !exitReceiptKnown(value.priorMissionReceipt) || !exitReceiptKnown(value.exitReceipt)) return false
        return value.priorMissionReceipt.sourceEpoch === value.exitReceipt.sourceEpoch
            && exitLess(value.priorMissionReceipt.sourceSequence, value.exitReceipt.sourceSequence)
            && !exitLess(value.exitReceipt.receivedMonotonicMs, value.priorMissionReceipt.receivedMonotonicMs)
    }
    function modeExitKnown(value) {
        if (!exact(value, ["schemaVersion", "observed", "count", "candidate", "first", "last",
                "requestCausalityVerified", "remoteOutcomeVerified", "automaticActionsIssued"])
                || value.schemaVersion !== 1 || !exitCounter(value.count, true)
                || typeof value.observed !== "boolean" || value.observed !== (value.count !== "0")
                || value.requestCausalityVerified !== false || value.remoteOutcomeVerified !== false
                || value.automaticActionsIssued !== false) return false
        if (!value.observed) return value.candidate === "not-observed" && value.first === null && value.last === null
        return value.candidate === "mission-mode-exit-observed-not-outcome"
            && exitValueKnown(value.first) && exitValueKnown(value.last)
            && value.first.exitReceipt.sourceEpoch === value.last.exitReceipt.sourceEpoch
            && (value.count === "1" ? JSON.stringify(value.first) === JSON.stringify(value.last)
                : exitLess(value.first.exitReceipt.sourceSequence, value.last.priorMissionReceipt.sourceSequence)
                    && !exitLess(value.last.priorMissionReceipt.receivedMonotonicMs, value.first.exitReceipt.receivedMonotonicMs))
    }
    function exitReceiptText(value) {
        return qsTr("源 %1 / 序号 %2 / 本机单调毫秒 %3")
            .arg(value.sourceEpoch).arg(value.sourceSequence).arg(value.receivedMonotonicMs)
    }
    function exitValueText(label, value) {
        return qsTr("%1：原 HEARTBEAT customMode=%2，baseMode=%3，systemStatus=%4；当时连续性未知=%5\n此前任务模式接收：%6\n退出接收：%7")
            .arg(label).arg(value.customMode).arg(value.baseMode).arg(value.systemStatus)
            .arg(value.continuityUnknownAtExit ? qsTr("是") : qsTr("否"))
            .arg(exitReceiptText(value.priorMissionReceipt)).arg(exitReceiptText(value.exitReceipt))
    }
    function modeExitText(value) {
        if (value === undefined) return qsTr("旧版资料未采集任务模式退出证据；不能当作零次退出。")
        if (!modeExitKnown(value)) return qsTr("任务模式退出证据未知；不据此推断结果。")
        if (!value.observed) return qsTr("本地观察中未见任务模式退出；不证明持续执行、任务完成或停飞。")
        return qsTr("曾观察任务模式退出 %1 次（不是中止原因或任务结果证明）。仅保留首次/末次，不是完整历史。").arg(value.count)
            + "\n" + exitValueText(qsTr("首次退出"), value.first)
            + "\n" + exitValueText(qsTr("末次退出"), value.last)
            + "\n" + qsTr("接收先后不证明请求因果、远端任务结果或自动动作。")
    }
    function uuid(value) { return bounded(value, 36) && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && value !== "00000000-0000-0000-0000-000000000000" }
    function digest(value) { return bounded(value, 64) && /^[0-9a-f]{64}$/.test(value) }
    function decimal(value) { return bounded(value, 20) && /^[1-9][0-9]*$/.test(value) }
    function text(value) { return bounded(value, 2048) && value.length ? value : qsTr("未知 / 未提供") }
    function boolText(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function sequence(value) { return value === null ? qsTr("未观察到") : integer(value, 0, 2047) ? String(value) : qsTr("未知") }
    function recipientValid(value) { return bounded(value, 128) && /^S-1-(?:[0-9]+-)+[0-9]+$/.test(value) }
    function bindingKnown(value) {
        var keys = ["storeEpoch", "sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash"]
        if (value && Object.keys(value).length === 9) keys = keys.concat(["inputGeneration", "reviewGeneration"])
        return exact(value, keys) && uuid(value.storeEpoch) && bounded(value.sourceTaskId, 128) && value.sourceTaskId.length > 0
            && bounded(value.recordId, 128) && value.recordId.length > 0 && integer(value.recordRevision, 1, 9007199254740991)
            && digest(value.planId) && digest(value.contentHash) && digest(value.reviewEvidenceHash)
            && (keys.length === 7 || (integer(value.inputGeneration, 0, 9007199254740991) && integer(value.reviewGeneration, 0, 9007199254740991)))
    }
    function summaryKnown(value, version) {
        var counts = ["targetVehicleCount", "modeQueueBeganVehicleCount", "modeCommittedVehicleCount", "uncommittedVehicleCount",
            "sourceUnknownVehicleCount", "noTerminalCandidateVehicleCount", "terminalCandidateVehicleCount", "indefiniteHoldCandidateVehicleCount", "landedCandidateVehicleCount"]
        if (!exact(value, counts.concat(["observerRetired", "startupTerminalState", "startupTerminalReason", "vehicles"]))
                || !integer(value.targetVehicleCount, 1, 32) || !Array.isArray(value.vehicles) || value.vehicles.length !== value.targetVehicleCount
                || typeof value.observerRetired !== "boolean" || !bounded(value.startupTerminalState, 128) || !bounded(value.startupTerminalReason, 256)) return false
        for (var i = 0; i < counts.length; ++i) if (!integer(value[counts[i]], 0, value.targetVehicleCount)) return false
        var seen = Object.create(null)
        for (i = 0; i < value.vehicles.length; ++i) {
            var row = value.vehicles[i]
            var keys = ["vehicleId", "componentId", "phase", "reason", "modeQueueBegan", "modeQueueCommitted", "sourceCurrent", "continuityUnknown",
                "currentSequence", "reachedSequence", "observedCurrentItemCount", "observedReachedItemCount", "outcomeCandidate", "observationWindow"]
            if (version === 2) keys.push("missionModeExitObservation")
            if (!exact(row, keys)
                    || !integer(row.vehicleId, 1, 255) || seen[row.vehicleId] || !integer(row.componentId, 1, 255)
                    || !bounded(row.phase, 128) || !bounded(row.reason, 256) || !bounded(row.outcomeCandidate, 128) || !bounded(row.observationWindow, 128)
                    || typeof row.modeQueueBegan !== "boolean" || typeof row.modeQueueCommitted !== "boolean"
                    || typeof row.sourceCurrent !== "boolean" || typeof row.continuityUnknown !== "boolean"
                    || !(row.currentSequence === null || integer(row.currentSequence, 0, 2047))
                    || !(row.reachedSequence === null || integer(row.reachedSequence, 0, 2047))
                    || !integer(row.observedCurrentItemCount, 0, 2048) || !integer(row.observedReachedItemCount, 0, 2048)
                    || (version === 2 && (!modeExitKnown(row.missionModeExitObservation)
                        || (!row.modeQueueCommitted && row.missionModeExitObservation.observed)))) return false
            seen[row.vehicleId] = true
        }
        return true
    }
    function briefKnown(value) {
        if (!exact(value, ["schemaVersion", "briefType", "briefId", "capturedAtUtc", "source", "runId", "taskId", "startupId", "deploymentId",
            "transactionEpoch", "transactionGeneration", "binding", "summary", "reportId", "reportContentHash", "claims", "contentHash"])
                || (value.schemaVersion !== 1 && value.schemaVersion !== 2) || value.briefType !== "fleet-run-handoff" || !uuid(value.briefId) || !bounded(value.capturedAtUtc, 32)
                || !exact(value.source, ["lifecycleEpoch", "selectionGeneration", "episodeId"])
                || !uuid(value.source.lifecycleEpoch) || !decimal(value.source.selectionGeneration) || !uuid(value.source.episodeId)
                || !uuid(value.runId) || !uuid(value.taskId) || !uuid(value.startupId) || !uuid(value.deploymentId) || !uuid(value.transactionEpoch)
                || !decimal(value.transactionGeneration) || !bindingKnown(value.binding) || !summaryKnown(value.summary, value.schemaVersion)
                || !uuid(value.reportId) || !digest(value.reportContentHash) || !digest(value.contentHash)) return false
        var claims = value.claims
        return exact(claims, ["snapshotOnly", "historicalOnly", "receiptOnly", "controlOwnershipTransferred", "approvalTransferred", "missionCompleted", "flightCommandReleased", "executable"])
            && claims.snapshotOnly === true && claims.historicalOnly === true && claims.receiptOnly === true
            && claims.controlOwnershipTransferred === false && claims.approvalTransferred === false && claims.missionCompleted === false
            && claims.flightCommandReleased === false && claims.executable === false
    }
    function statusKnown(value) { return ["pending", "accepted", "rejected", "cancelled", "expired"].indexOf(value) >= 0 }
    function statusText(value) {
        if (value === "pending") return qsTr("待确认")
        if (value === "accepted") return qsTr("已确认收到（仅审计）")
        if (value === "rejected") return qsTr("已拒绝")
        if (value === "cancelled") return qsTr("发起人已撤销")
        if (value === "expired") return qsTr("已过期")
        return qsTr("未知")
    }
    function rowKnown(row) {
        return exact(row, ["handoffId", "taskId", "fromOperator", "toOperator", "status", "recordRevision", "createdAt", "expiresAt", "resolvedAt", "resolvedBy",
            "briefAvailable", "briefHash", "runId", "origin"])
            && bounded(row.handoffId, 128) && row.handoffId.length > 0 && bounded(row.taskId, 256)
            && bounded(row.fromOperator, 128) && bounded(row.toOperator, 128) && statusKnown(row.status)
            && integer(row.recordRevision, 1, 9007199254740991) && bounded(row.createdAt, 32) && bounded(row.expiresAt, 32)
            && bounded(row.resolvedAt, 32) && bounded(row.resolvedBy, 128) && typeof row.briefAvailable === "boolean"
            && (row.briefAvailable ? digest(row.briefHash) && uuid(row.runId) : row.briefHash === "" && row.runId === "")
            && (row.origin === "local" || row.origin === "imported")
    }
    function pageKnown(value) {
        if (!exact(value, ["page", "pageCount", "total", "records"]) || !integer(value.total, 0, 100000)
                || !integer(value.pageCount, 0, 25000) || !integer(value.page, 0, 24999)
                || !Array.isArray(value.records) || value.records.length > 4 || value.pageCount !== Math.ceil(value.total / 4)
                || (value.pageCount === 0 ? value.page !== 0 || value.records.length !== 0 : value.page >= value.pageCount)) return false
        for (var i = 0; i < value.records.length; ++i) if (!rowKnown(value.records[i])) return false
        return true
    }
    function detailKnown(value) {
        return exact(value, ["handoffId", "taskId", "fromOperator", "toOperator", "reason", "note", "metadata", "status", "requiresSecondApproval",
            "recordRevision", "createdAt", "expiresAt", "resolvedAt", "resolvedBy", "flightCommandReleased", "origin"])
            && bounded(value.handoffId, 128) && value.handoffId.length > 0 && bounded(value.taskId, 256)
            && bounded(value.fromOperator, 128) && bounded(value.toOperator, 128) && bounded(value.reason, 1024) && bounded(value.note, 1024)
            && value.metadata && typeof value.metadata === "object" && !Array.isArray(value.metadata)
            && statusKnown(value.status) && value.requiresSecondApproval === true && integer(value.recordRevision, 1, 9007199254740991)
            && bounded(value.createdAt, 32) && bounded(value.expiresAt, 32) && bounded(value.resolvedAt, 32) && bounded(value.resolvedBy, 128)
            && value.flightCommandReleased === false && (value.origin === "local" || value.origin === "imported")
    }
    function briefOf(value) { return detailKnown(value) && value.metadata.kind === "run-brief" && briefKnown(value.metadata.brief) ? value.metadata.brief : ({}) }
    function briefText(value) {
        if (!briefKnown(value)) return qsTr("未取得有效冻结运行简报；不会从任务文字或导入值猜测真实运行。")
        var s = value.summary, b = value.binding
        return qsTr("冻结时间：%1\n简报：%2\n真实 Run：%3\n运行观察任务：%4\n原规划任务：%5\n原记录：%6 · 版本 %7\n原部署：%8\n目标 %9 架；本地 MODE 已入队提交 %10 架，未提交 %11 架；来源未知 %12 架。\n监视已退休：%13；启动终态：%14；原因：%15。\n终点候选 %16，持续停留候选 %17，落地候选 %18，未见终点候选 %19。以上不是执行成功率或任务完成证明。\n简报 SHA-256：%20（完整性标识，不是身份签名）")
            .arg(value.capturedAtUtc).arg(value.briefId).arg(value.runId).arg(value.taskId).arg(b.sourceTaskId).arg(b.recordId).arg(b.recordRevision)
            .arg(value.deploymentId).arg(s.targetVehicleCount).arg(s.modeCommittedVehicleCount).arg(s.uncommittedVehicleCount).arg(s.sourceUnknownVehicleCount)
            .arg(boolText(s.observerRetired)).arg(text(s.startupTerminalState)).arg(text(s.startupTerminalReason))
            .arg(s.terminalCandidateVehicleCount).arg(s.indefiniteHoldCandidateVehicleCount).arg(s.landedCandidateVehicleCount).arg(s.noTerminalCandidateVehicleCount)
            .arg(value.contentHash)
    }
    function vehicleText(row) {
        return qsTr("机 %1 / 组件 %2：当时来源绑定 %3，连续性未知 %4；MODE 已提交入队 %5。\n最后 current 序号 %6 / reached 序号 %7（均 0 基，仅观察，不证明完成）；阶段 %8，原因 %9。\n候选原码：%10；观察窗口原码：%11。")
            .arg(row.vehicleId).arg(row.componentId).arg(boolText(row.sourceCurrent)).arg(boolText(row.continuityUnknown)).arg(boolText(row.modeQueueCommitted))
            .arg(sequence(row.currentSequence)).arg(sequence(row.reachedSequence)).arg(text(row.phase)).arg(text(row.reason))
            .arg(text(row.outcomeCandidate)).arg(text(row.observationWindow))
            + "\n" + modeExitText(row.missionModeExitObservation)
    }
    function clearPressed() {
        if (prepareButton) prepareButton.pressedIntent = null
        if (sendButton) sendButton.pressedIntent = null
        if (clearButton) clearButton.pressedIntent = null
        if (acceptButton) acceptButton.pressedIntent = null
        if (rejectButton) rejectButton.pressedIntent = null
        if (cancelButton) cancelButton.pressedIntent = null
    }
    function invalidateConfirmations(dropPreview) {
        if (!integer(lifetime.epoch, 0, 9007199254740990)) intentExhausted = true
        else lifetime.epoch += 1
        sendConsent = null; detailConsent = null; clearPressed()
        if (dropPreview) ownedPreviewToken = ""
    }
    function resetContext() {
        invalidateConfirmations(true); pageResult = ({}); pageStale = true; pageOwner = null
        selectedDetail = ({}); detailStale = true; detailOwner = null
        showPreviewVehicles = false; showDetailVehicles = false
    }
    function inputChanged() { invalidateConfirmations(true) }
    function intent(action) {
        return {action: action, owner: manager, controller: controller, epoch: lifetime.epoch, identityToken: identityToken, actor: operatorId,
            sourceToken: sourceToken, runId: run.runId || "", previewToken: previewToken,
            recipient: recipientText.trim(), reason: reasonText.trim(), note: noteText.trim(), filter: filter,
            handoffId: selectedDetail.handoffId || "", revision: selectedDetail.recordRevision || 0, briefHash: detailHash}
    }
    function intentCurrent(value) {
        return value && lifetime.alive && !intentExhausted && value.owner === manager && value.controller === controller && value.epoch === lifetime.epoch
            && value.identityToken === identityToken && value.actor === operatorId && visible && expanded
    }
    function sourceIntentCurrent(value) {
        return intentCurrent(value) && value.sourceToken === sourceToken && value.runId === run.runId
            && value.recipient === recipientText.trim() && value.reason === reasonText.trim()
    }
    function detailIntentCurrent(value) {
        return intentCurrent(value) && detailCurrent && value.handoffId === selectedDetail.handoffId
            && value.revision === selectedDetail.recordRevision && value.briefHash === detailHash && value.note === noteText.trim()
    }
    function consentCurrent(value, kind) {
        return kind === "send" ? sourceIntentCurrent(value) && value.previewToken === previewToken && ownedPreviewToken === previewToken
            : detailIntentCurrent(value)
    }
    function outcomeKnown(value) {
        return exact(value, ["ok", "error", "committed", "current", "handoffId", "token"])
            && typeof value.ok === "boolean" && bounded(value.error, 512) && (value.committed === null || typeof value.committed === "boolean")
            && typeof value.current === "boolean" && bounded(value.handoffId, 128) && bounded(value.token, 128)
    }
    function actionAllowed(value) {
        if (!ready || lifetime.ticket || !intentCurrent(value)) return false
        if (value.action === "prepare") return canPrepare && sourceIntentCurrent(value)
        if (value.action === "send") return previewCurrent && sendConfirmed && sourceIntentCurrent(value) && value.previewToken === previewToken
        if (value.action === "clear") return value.previewToken.length > 0 && value.previewToken === previewToken
        if (!canResolve || !detailConfirmed || !detailIntentCurrent(value)) return false
        return value.action === "cancel" ? selectedDetail.fromOperator === operatorId
            : (value.action === "accept" || value.action === "reject") && selectedDetail.toOperator === operatorId
    }
    function perform(value) {
        if (!actionAllowed(value)) { sendConsent = null; detailConsent = null; message = qsTr("确认或来源已变化，未发起该操作；请重新核对，不自动重试。"); return }
        var guard = lifetime, owner = manager, ticket = {}, result = null
        guard.ticket = ticket; callBusy = true
        if (!guard.alive) { if (guard.ticket === ticket) guard.ticket = null; return }
        clearPressed(); sendConsent = null; detailConsent = null
        if (value.action === "prepare") ownedPreviewToken = ""
        try {
            if (intentCurrent(value) && guard.ticket === ticket) {
                if (value.action === "prepare") result = owner.prepareRunHandoff(value.sourceToken, value.runId, value.recipient, value.reason)
                else if (value.action === "send") result = owner.sendRunHandoff(value.previewToken, true)
                else if (value.action === "clear") result = owner.clearRunHandoff(value.previewToken)
                else result = owner.resolveRunHandoff(value.handoffId, value.revision, value.briefHash, value.action, value.note, true)
            }
            if (!guard.alive) return
            if (!outcomeKnown(result)) {
                message = qsTr("未取得完整回执，不能断言尚未提交；请刷新本地记录核对。没有自动重试。")
                pageStale = true; detailStale = true; return
            }
            var current = intentCurrent(value) && result.current === true
            if (value.action === "prepare" && current && result.ok === true && previewToken === result.token
                    && preview.current === true && briefKnown(preview.brief) && sourceIntentCurrent(value)
                    && preview.recipientActorId === value.recipient && preview.reason === value.reason && preview.fromOperator === value.actor)
                ownedPreviewToken = result.token
            else if (value.action === "send" || value.action === "clear") ownedPreviewToken = ""
            if (["accept", "reject", "cancel", "send"].indexOf(value.action) >= 0) { pageStale = true; detailStale = true }
            var fact = result.committed === null ? qsTr("提交状态未知，请刷新核对")
                : result.committed ? qsTr("对应本地动作已提交；不代表飞机动作") : qsTr("本次动作未确认提交；不否定此前记录")
            message = qsTr("%1。%2\n交接记录：%3。%4%5")
                .arg(result.ok && current ? qsTr("本次本地操作已完成") : qsTr("本次操作未确认处于当前上下文"))
                .arg(fact).arg(text(result.handoffId))
                .arg(result.error ? qsTr("原因：%1。 ").arg(text(result.error)) : "")
                .arg(value.action === "prepare" && ownedPreviewToken ? qsTr("简报已冻结，尚未发送；请阅读并另行确认。") : qsTr("请查看本地收件箱；没有控制权移交或自动重试。"))
        } catch (error) {
            if (guard.alive) { pageStale = true; detailStale = true; ownedPreviewToken = ""; message = qsTr("操作回执中断，提交状态可能未知；请显式刷新核对已有记录。没有自动重试，也没有回滚飞机动作。") }
        } finally {
            if (guard.alive) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
        if (guard.alive && outcomeKnown(result) && !intentCurrent(value)) {
            message = qsTr("界面或身份已变化；本次回执仅保留为历史事实。已提交本地动作：%1；记录：%2；原因：%3。请刷新核对，不自动重试。")
                .arg(result.committed === null ? qsTr("未知") : boolText(result.committed)).arg(text(result.handoffId)).arg(text(result.error))
        }
    }
    function refreshPage(requestedPage) {
        if (!canList || lifetime.ticket || !integer(requestedPage, 0, 24999)) return
        var guard = lifetime, value = intent("list"), ticket = {}, oldPublication = publication
        guard.ticket = ticket; callBusy = true
        if (!guard.alive) { if (guard.ticket === ticket) guard.ticket = null; return }
        detailConsent = null; selectedDetail = ({}); detailStale = true; showDetailVehicles = false
        try {
            var result = intentCurrent(value) ? value.owner.handoffPage(value.actor, value.filter, requestedPage) : null
            if (!guard.alive) return
            if (!intentCurrent(value) || value.filter !== filter || oldPublication !== publication) { pageStale = true; message = qsTr("列表读取期间上下文或记录变化，请明确刷新。"); return }
            if (!pageKnown(result)) { pageStale = true; message = qsTr("未取得有效记录页：%1。原显示仅供参考。").arg(text(result && result.error)); return }
            pageResult = result; pageOwner = value.owner; pageActor = value.actor; pageFilter = value.filter; pageStale = false
        } catch (error) { if (guard.alive) { pageStale = true; message = qsTr("列表读取中断，请手动刷新；没有自动重试。") } }
        finally { if (guard.alive) callBusy = false; if (guard.ticket === ticket) guard.ticket = null }
    }
    function openDetail(row) {
        if (!ready || lifetime.ticket || !pageCurrent || !rowKnown(row)) return
        var guard = lifetime, value = intent("detail"), ticket = {}, oldPublication = publication
        guard.ticket = ticket; callBusy = true
        if (!guard.alive) { if (guard.ticket === ticket) guard.ticket = null; return }
        detailConsent = null; selectedDetail = ({}); detailStale = true; showDetailVehicles = false
        try {
            var result = intentCurrent(value) ? value.owner.handoff(row.handoffId) : null
            if (!guard.alive) return
            if (!intentCurrent(value) || oldPublication !== publication || !pageCurrent || !detailKnown(result)
                    || result.handoffId !== row.handoffId || result.recordRevision !== row.recordRevision
                    || result.origin !== row.origin || (row.briefAvailable && (!briefKnown(briefOf(result)) || briefOf(result).contentHash !== row.briefHash))) {
                message = qsTr("记录已改变或详情未确认，请刷新列表后重新选择。"); return
            }
            selectedDetail = result; detailOwner = value.owner; detailStale = false; noteText = ""
        } catch (error) { if (guard.alive) message = qsTr("详情读取中断，没有执行接收、拒绝或撤销。") }
        finally { if (guard.alive) callBusy = false; if (guard.ticket === ticket) guard.ticket = null }
    }
    function ensureFocusVisible(item) {
        if (!visible || !expanded || !item) return
        var point = item.mapToItem(body, 0, 0), top = point.y - pad, bottom = point.y + item.height + pad
        var maximum = Math.max(0, scroller.contentHeight - scroller.height)
        if (top < scroller.contentY || item.height + pad * 2 > scroller.height) scroller.contentY = Math.min(maximum, Math.max(0, top))
        else if (bottom > scroller.contentY + scroller.height) scroller.contentY = Math.min(maximum, Math.max(0, bottom - scroller.height))
    }
    function keepFocusVisible() {
        if (previousPage.activeFocus) ensureFocusVisible(previousPage)
        else if (nextPage.activeFocus) ensureFocusVisible(nextPage)
        else if (refreshButton.activeFocus) ensureFocusVisible(refreshButton)
    }
    onManagerChanged: resetContext()
    onControllerChanged: resetContext()
    onIdentityTokenChanged: resetContext()
    onSourceTokenChanged: { ownedPreviewToken = ""; sendConsent = null; clearPressed() }
    onPublicationChanged: { pageStale = true; detailStale = true; detailConsent = null; clearPressed() }
    onPreviewTokenChanged: { sendConsent = null; showPreviewVehicles = false; clearPressed() }
    onFilterChanged: { pageStale = true; detailStale = true; detailConsent = null; selectedDetail = ({}); clearPressed() }
    onVisibleChanged: if (!visible && lifetime.alive) { invalidateConfirmations(true); if (handoffPopup) handoffPopup.close() }
    onExpandedChanged: if (!expanded && lifetime.alive) invalidateConfirmations(true)
    Component.onDestruction: lifetime.alive = false

    QGCButton {
        id: heading
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: qsTr("打开运行简报交接与本地收件箱")
        Accessible.name: text
        contentItem: QGCLabel { text: heading.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: heading._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: heading.activeFocus }
        onClicked: { root.invalidateConfirmations(true); handoffPopup.open() }
    }
    Controls.Popup {
        id: handoffPopup
        parent: Controls.Overlay.overlay
        modal: true; focus: true
        closePolicy: Controls.Popup.CloseOnEscape
        padding: root.pad
        width: parent ? Math.max(0, Math.min(ScreenTools.defaultFontPixelWidth * 76, parent.width - root.pad * 2)) : 0
        height: parent ? Math.max(0, Math.min(parent.height - root.pad * 2,
            root.pad * 3 + Math.max(root.rowHeight, closePopup.implicitHeight) + Math.min(body.implicitHeight, ScreenTools.defaultFontPixelHeight * 18))) : 0
        x: parent ? Math.max(0, (parent.width - width) / 2) : 0
        y: parent ? Math.max(0, (parent.height - height) / 2) : 0
        background: Rectangle { color: qgcPal.window; border.width: 1; border.color: qgcPal.text }
        onOpened: closePopup.forceActiveFocus()
        onClosed: if (root.visible && root.lifetime.alive) heading.forceActiveFocus()
        contentItem: ColumnLayout {
            spacing: root.pad
            QGCButton {
                id: closePopup
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("关闭交接窗口（保留输入，撤销本地确认）"); Accessible.name: text
                contentItem: QGCLabel { text: closePopup.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: closePopup._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: closePopup.activeFocus }
                onClicked: handoffPopup.close()
            }
            Flickable {
                id: scroller
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 0
                visible: root.expanded; clip: true; contentWidth: width; contentHeight: body.implicitHeight
                boundsBehavior: Flickable.StopAtBounds; onContentHeightChanged: root.keepFocusVisible()
                Controls.ScrollBar.vertical: Controls.ScrollBar { }
                ColumnLayout {
                    id: body
                    width: Math.max(0, scroller.width - root.pad); spacing: root.pad
                    QGCLabel { Layout.fillWidth: true; visible: root.intentExhausted; text: qsTr("本界面确认序号已耗尽，所有查询与处理入口已停用；请重新打开地面站实例后核对，不自动重试。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本实例的人际审计交接，不是跨地面站控制权转移。确认收到不改变原任务、监视、机载内容或飞行批准。文件历史需在下方明确保存、加载预览及另行确认追加；没有自动保存、网络发送或账户恢复。") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("当前操作员 SID：%1\n交接权限：%2；当前选定 Run：%3。\n先从运行观察选择真实来源；不再手填任务 ID。").arg(root.text(root.operatorId)).arg(root.boolText(root.handoffAllowed)).arg(root.text(root.run.runId)) }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("接收账户的准确 Windows SID（不是姓名；1–128 字符）"); wrapMode: Text.WordWrap }
                    Controls.TextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 128; selectByMouse: true
                        text: root.recipientText; enabled: !root.callBusy && root.preview.busy !== true; color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize
                        Accessible.name: qsTr("接收账户准确 Windows SID")
                        onTextEdited: { root.recipientText = text; root.inputChanged() }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        background: Rectangle { color: qgcPal.windowShade; border.width: parent.activeFocus ? 2 : 1; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.recipientText.length > 0 && !root.recipientValid(root.recipientText.trim()); text: qsTr("请输入准确 SID，例如 S-1-5-…；这里不验证接收账户是否存在或在线。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("交接原因（可选，最多 1024 字符）"); wrapMode: Text.WordWrap }
                    Controls.TextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 1024; selectByMouse: true
                        text: root.reasonText; enabled: !root.callBusy && root.preview.busy !== true; color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize
                        Accessible.name: qsTr("交接原因")
                        onTextEdited: { root.reasonText = text; root.inputChanged() }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        background: Rectangle { color: qgcPal.windowShade; border.width: parent.activeFocus ? 2 : 1; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text }
                    }
                    QGCButton {
                        id: prepareButton
                        property var pressedIntent: null
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canPrepare; opacity: enabled ? 1 : 0.45
                        text: qsTr("冻结所选真实 Run 的交接简报（尚不发送）"); Accessible.name: text
                        contentItem: QGCLabel { text: prepareButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: prepareButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: prepareButton.activeFocus }
                        onPressed: pressedIntent = root.intent("prepare")
                        onCanceled: pressedIntent = null
                        onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.message.length > 0; text: root.message; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: qgcPal.warningText }
                    ColumnLayout {
                        Layout.fillWidth: true; visible: root.previewToken.length > 0; spacing: root.pad
                        QGCLabel { Layout.fillWidth: true; text: qsTr("待发送简报 · 冻结后不会随运行进度更新"); font.bold: true; wrapMode: Text.WordWrap }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("发起人 %1 → 接收人 %2\n原因：%3\n本页确认来源仍当前：%4。更改输入、身份或运行选择后必须重新冻结。").arg(root.text(root.preview.fromOperator)).arg(root.text(root.preview.recipientActorId)).arg(root.text(root.preview.reason)).arg(root.boolText(root.previewCurrent)) }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.briefText(root.previewBrief) }
                        QGCCheckBox { id: previewVehiclesToggle; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("显示逐机历史观察"); activeFocusOnTab: true; Accessible.name: qsTr("展开当时逐机观察，最多32架"); checked: root.showPreviewVehicles; onClicked: root.showPreviewVehicles = checked; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: previewVehiclesToggle.activeFocus } }
                        Repeater { model: root.expanded && root.showPreviewVehicles && root.briefKnown(root.previewBrief) ? root.previewBrief.summary.vehicles : []; QGCLabel { Layout.fillWidth: true; text: root.vehicleText(modelData); textFormat: Text.PlainText; wrapMode: Text.Wrap } }
                        QGCCheckBox {
                            id: sendConsentBox
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("已阅简报并核对接收人"); activeFocusOnTab: true
                            Accessible.name: qsTr("已阅此精确简报并核对接收SID，仅创建本地待收记录")
                            checked: root.sendConfirmed; enabled: root.previewCurrent
                            onClicked: { root.sendConsent = checked && root.previewCurrent ? root.intent("send") : null; checked = Qt.binding(function() { return root.sendConfirmed }) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: sendConsentBox.activeFocus }
                        }
                        QGCButton {
                            id: sendButton
                            property var pressedIntent: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.previewCurrent && root.sendConfirmed; opacity: enabled ? 1 : 0.45
                            text: qsTr("明确发送本地交接请求（5 分钟内确认）"); Accessible.name: text
                            contentItem: QGCLabel { text: sendButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: sendButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: sendButton.activeFocus }
                            onPressed: pressedIntent = root.intent("send")
                            onCanceled: pressedIntent = null
                            onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                        QGCButton {
                            id: clearButton
                            property var pressedIntent: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.ready; opacity: enabled ? 1 : 0.45
                            text: qsTr("取消此待发送预览（保留文字输入）"); Accessible.name: text
                            contentItem: QGCLabel { text: clearButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: clearButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: clearButton.activeFocus }
                            onPressed: pressedIntent = root.intent("clear")
                            onCanceled: pressedIntent = null
                            onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                    }
                    ColumnLayout {
                        id: historyArea
                        Layout.fillWidth: true; spacing: root.pad
                        property var owner: root.manager
                        property var life: ({alive: true, epoch: 0, exhausted: false, dirty: false, queued: false, draining: false, seenJob: ""})
                        property string filePath: ""
                        property bool overwriteConfirmed: false
                        property var ownedJob: null
                        property var candidate: null
                        property var mergeConfirmation: null
                        property var savedResult: null
                        property var candidatePage: ({})
                        property var candidateDetail: ({})
                        property bool pageCurrent: false
                        property bool detailCurrent: false
                        property bool showCandidateVehicles: false
                        property string cancelledJobId: ""
                        property string feedback: ""
                        readonly property var workspaceState: owner ? owner.historyState || {} : ({})
                        readonly property var previewValue: owner ? owner.historyPreview || {} : ({})
                        readonly property var resultValue: owner ? owner.historyResult || {} : ({})
                        readonly property bool exposed: root.visible && root.expanded
                        readonly property bool stateKnown: stateValid(workspaceState)
                        readonly property bool idle: exposed && life.alive && !life.exhausted && owner && !root.callBusy && stateKnown && workspaceState.busy === false
                            && typeof owner.requestHistory === "function" && typeof owner.historyPreviewPage === "function" && typeof owner.historyPreviewRecord === "function"
                        readonly property bool pathValid: root.bounded(filePath, 4096) && filePath.trim().length > 0 && !/[\u0000-\u001f]/.test(filePath)
                        readonly property bool candidateCurrent: candidate && originCurrent(candidate) && previewValid(previewValue)
                            && samePreview(candidate.value, previewValue) && stateKnown && workspaceState.previewId === candidate.value.previewId
                        readonly property bool canCancel: exposed && life.alive && !root.callBusy && stateKnown && workspaceState.busy === true
                            && ownedJob && ownedJob.owner === owner && ownedJob.jobId === workspaceState.jobId && !ownedJob.settled
                            && cancelledJobId !== ownedJob.jobId

                        function stateValid(v) {
                            return root.exact(v, ["schemaVersion", "busy", "jobId", "action", "stage", "cancelRequested", "previewId", "historicalOnly", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                                && v.schemaVersion === 1 && typeof v.busy === "boolean" && typeof v.cancelRequested === "boolean"
                                && (v.jobId === "" || root.uuid(v.jobId)) && (v.previewId === "" || root.uuid(v.previewId))
                                && ["", "save", "load", "merge", "discard", "cancel"].indexOf(v.action) >= 0
                                && ["idle", "running", "cancelling", "complete", "failed", "cancelled"].indexOf(v.stage) >= 0
                                && v.historicalOnly === true && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false
                        }
                        function previewValid(v) {
                            return root.exact(v, ["schemaVersion", "previewId", "jobId", "path", "contentHash", "recordCount", "runRecordCount", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                                && v.schemaVersion === 1 && root.uuid(v.previewId) && root.uuid(v.jobId)
                                && root.bounded(v.path, 4096) && v.path.length > 0 && root.digest(v.contentHash)
                                && root.integer(v.recordCount, 0, 100000) && root.integer(v.runRecordCount, 0, 256) && v.runRecordCount <= v.recordCount
                                && v.historicalOnly === true && v.authenticated === false && v.permissionsRestored === false
                                && v.executionPermit === false && v.flightCommandReleased === false
                        }
                        function samePreview(a, b) {
                            if (!previewValid(a) || !previewValid(b)) return false
                            var keys = Object.keys(a)
                            for (var i = 0; i < keys.length; ++i) if (a[keys[i]] !== b[keys[i]]) return false
                            return true
                        }
                        function receiptValid(v, action) {
                            return root.exact(v, ["schemaVersion", "action", "accepted", "jobId", "reason", "requestOnly", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                                && v.schemaVersion === 1 && v.action === action && typeof v.accepted === "boolean"
                                && (v.jobId === "" || root.uuid(v.jobId)) && root.bounded(v.reason, 4096)
                                && v.requestOnly === true && v.historicalOnly === true && v.authenticated === false
                                && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false
                        }
                        function saveReplyValid(v, job) {
                            return root.exact(v, ["snapshotId", "contentHash", "path", "fileWriteAttempted", "fileWrittenKnown", "fileWritten", "readbackVerified", "readbackError", "fsyncVerified", "durable", "authenticated"])
                                && v.snapshotId === job.jobId && (v.contentHash === "" || root.digest(v.contentHash)) && root.bounded(v.path, 4096)
                                && typeof v.fileWriteAttempted === "boolean" && typeof v.fileWrittenKnown === "boolean"
                                && typeof v.fileWritten === "boolean" && typeof v.readbackVerified === "boolean" && root.bounded(v.readbackError, 4096)
                                && (!v.readbackVerified || v.fileWrittenKnown && v.fileWritten)
                                && v.fsyncVerified === false && v.durable === false && v.authenticated === false
                        }
                        function resultValid(v, job) {
                            if (!root.exact(v, ["schemaVersion", "jobId", "action", "ok", "error", "reply", "historicalOnly", "authenticated", "permissionsRestored", "executionPermit", "flightCommandReleased"])
                                    || v.schemaVersion !== 1 || v.jobId !== job.jobId || v.action !== job.action
                                    || typeof v.ok !== "boolean" || !root.bounded(v.error, 4096) || !v.reply || typeof v.reply !== "object" || Array.isArray(v.reply)
                                    || v.historicalOnly !== true || v.authenticated !== false || v.permissionsRestored !== false
                                    || v.executionPermit !== false || v.flightCommandReleased !== false) return false
                            if (Object.keys(v.reply).length === 0) return v.ok === false
                            if (job.action === "save") return saveReplyValid(v.reply, job)
                            if (job.action === "load") return v.ok && root.exact(v.reply, ["preview"]) && previewValid(v.reply.preview)
                                && v.reply.preview.jobId === job.jobId && v.reply.preview.path === job.filePath
                            return job.action === "merge" && v.ok && root.exact(v.reply, ["previewId", "addedCount", "duplicateCount", "recordCount"])
                                && v.reply.previewId === job.previewId && root.integer(v.reply.addedCount, 0, 100000)
                                && root.integer(v.reply.duplicateCount, 0, 100000) && root.integer(v.reply.recordCount, 0, 100000)
                                && v.reply.addedCount + v.reply.duplicateCount === job.candidate.value.recordCount
                        }
                        function originCurrent(c) { return life.alive && !life.exhausted && c && c.owner === owner && c.epoch === life.epoch && c.filePath === filePath && exposed }
                        function retire() {
                            var guard = life
                            if (!guard || !guard.alive) return
                            if (guard.epoch >= 9007199254740991) guard.exhausted = true
                            else ++guard.epoch
                            var epoch = guard.epoch
                            mergeConfirmation = null
                            if (!guard.alive || epoch !== guard.epoch) return
                            pageCurrent = false
                            if (!guard.alive || epoch !== guard.epoch) return
                            detailCurrent = false
                        }
                        function scheduleDrain() {
                            var guard = life
                            if (!guard) return
                            guard.dirty = true
                            if (!guard.alive || root.callBusy || guard.queued || guard.draining) return
                            guard.queued = true
                            Qt.callLater(function() {
                                if (!guard.alive) return
                                guard.queued = false
                                historyArea.drain()
                            })
                        }
                        function drain() {
                            var guard = life
                            if (!guard.alive || root.callBusy || guard.draining) { guard.dirty = true; return }
                            guard.draining = true; guard.dirty = false
                            try {
                                if (!stateKnown) return
                                if (workspaceState.jobId && workspaceState.jobId !== guard.seenJob) {
                                    guard.seenJob = workspaceState.jobId
                                    if (!ownedJob || ownedJob.owner !== owner || ownedJob.jobId !== workspaceState.jobId) retire()
                                    if (!guard.alive) return
                                }
                                var job = ownedJob
                                if (!job || job.settled || job.owner !== owner || workspaceState.busy || workspaceState.jobId !== job.jobId
                                        || ["complete", "failed", "cancelled"].indexOf(workspaceState.stage) < 0) return
                                var actual = resultValue
                                if (!resultValid(actual, job)) {
                                    job.settled = true
                                    feedback = qsTr("文件任务已结束但未取得同一原任务的完整回执；提交状态可能未知。请明确重新读取核对，不自动重试。")
                                    return
                                }
                                var copy = JSON.parse(JSON.stringify(actual))
                                if (!guard.alive || job.owner !== owner || ownedJob !== job || resultValue !== actual) return
                                job.settled = true
                                savedResult = {owner: job.owner, job: job, value: copy}
                                if (!guard.alive || ownedJob !== job || job.owner !== owner) return
                                if (job.action === "load" && copy.ok && originCurrent(job) && cancelledJobId !== job.jobId
                                        && samePreview(copy.reply.preview, previewValue)) {
                                    candidate = {owner: job.owner, epoch: job.epoch, filePath: job.filePath, value: copy.reply.preview}
                                    if (!guard.alive || !originCurrent(job) || ownedJob !== job) return
                                    pageCurrent = false
                                    if (!guard.alive || !originCurrent(job) || ownedJob !== job) return
                                    detailCurrent = false
                                    if (!guard.alive || !originCurrent(job) || ownedJob !== job) return
                                    showCandidateVehicles = false
                                }
                                if (!guard.alive || ownedJob !== job || job.owner !== owner) return
                                var outcome = copy.ok ? (job.action === "save" ? qsTr("保存任务已结束；请核对下方写入已知性、实际写入及回读事实，不推断耐久。")
                                    : job.action === "load" ? qsTr("整份文件已形成历史候选，未修改当前记录；请明确浏览并另行确认追加。")
                                    : qsTr("历史追加任务已结束；新增／重复数量见原回执。不会恢复本地待收处理权限或飞机控制。"))
                                    : qsTr("文件任务未确认成功：%1。原本地记录和此前已经发生的提交事实不由此回滚；不自动重试。").arg(root.text(copy.error))
                                if (!originCurrent(job)) outcome += qsTr(" 本页来源或窗口已变化，回执只作历史；需重新明确加载才能取得可操作候选。")
                                feedback = outcome
                            } finally { guard.draining = false; if (guard.alive && guard.dirty) scheduleDrain() }
                        }
                        function capture(action) {
                            if (!idle || life.draining || ["save", "load", "merge", "discard"].indexOf(action) < 0) return null
                            if ((action === "save" || action === "load") && !pathValid) return null
                            if (action === "save" && !overwriteConfirmed) return null
                            if ((action === "merge" || action === "discard") && !candidateCurrent) return null
                            return {owner: owner, epoch: life.epoch, action: action, filePath: filePath, overwrite: overwriteConfirmed,
                                publication: root.publication, candidate: candidate, previewId: action === "merge" || action === "discard" ? candidate.value.previewId : "",
                                expectedJobId: workspaceState.jobId}
                        }
                        function capturedCurrent(c) {
                            return originCurrent(c) && stateKnown && workspaceState.busy === false && c.expectedJobId === workspaceState.jobId
                                && (c.action !== "save" || overwriteConfirmed && c.overwrite === overwriteConfirmed)
                                && ((c.action !== "merge" && c.action !== "discard") || candidateCurrent && c.candidate === candidate && c.previewId === candidate.value.previewId)
                                && (c.action !== "merge" || c.publication === root.publication)
                        }
                        function intentCurrent(c) { return idle && capturedCurrent(c) }
                        function submit(c) {
                            if (life.draining || !intentCurrent(c)) return
                            var guard = life
                            root.callBusy = true
                            try {
                                if (!guard.alive || !capturedCurrent(c)) return
                                mergeConfirmation = null
                                if (!guard.alive || !capturedCurrent(c)) return
                                var intent = {action: c.action, expectedJobId: c.expectedJobId,
                                    filePath: c.action === "save" || c.action === "load" ? c.filePath : "",
                                    previewId: c.previewId, overwriteConfirmed: c.action === "save"}
                                var receipt = c.owner.requestHistory(intent, true)
                                if (!guard.alive) return
                                if (!receiptValid(receipt, c.action) || !receipt.accepted) {
                                    feedback = qsTr("文件请求未确认：%1。未自动重试；不能据此否定已存在的提交。").arg(root.text(receipt && receipt.reason)); return
                                }
                                if (c.action === "discard") {
                                    feedback = receipt.jobId === c.expectedJobId ? qsTr("原候选丢弃请求已接纳；不是删除文件或撤销已追加历史。")
                                        : qsTr("未确认取得原候选的准确丢弃回执；不推断候选或文件已删除。")
                                    return
                                }
                                if (!root.uuid(receipt.jobId) || receipt.jobId === c.expectedJobId || c.owner !== owner) return
                                c.jobId = receipt.jobId; c.settled = false
                                ownedJob = c
                                if (!guard.alive || ownedJob !== c || c.owner !== owner) return
                                cancelledJobId = ""
                                if (!guard.alive || ownedJob !== c || c.owner !== owner) return
                                feedback = qsTr("文件请求已接纳，等待同一原生任务实际结束；此时不是保存、加载或追加已完成。")
                            } catch (error) {
                                if (guard.alive) feedback = qsTr("文件调用中断，提交状态未知；请明确核对，不自动重试。")
                            } finally { if (guard.alive) { root.callBusy = false; if (guard.alive) scheduleDrain() } }
                        }
                        function cancelJob(c) {
                            if (!canCancel || life.draining || c !== ownedJob) return
                            var guard = life
                            root.callBusy = true
                            try {
                                if (!guard.alive || c !== ownedJob || c.owner !== owner || !stateKnown || workspaceState.jobId !== c.jobId || !workspaceState.busy) return
                                cancelledJobId = c.jobId
                                if (!guard.alive || c !== ownedJob || c.owner !== owner) return
                                mergeConfirmation = null
                                if (!guard.alive || c !== ownedJob || c.owner !== owner) return
                                var receipt = c.owner.requestHistory({action: "cancel", expectedJobId: c.jobId, filePath: "", previewId: "", overwriteConfirmed: false}, true)
                                if (!guard.alive) return
                                feedback = receiptValid(receipt, "cancel") && receipt.accepted && receipt.jobId === c.jobId
                                    ? qsTr("取消已请求；等待实际任务结束。已写入文件或已追加历史不会被取消回滚。")
                                    : qsTr("取消未确认，结果可能未知；继续等待原生状态，不自动重试。")
                            } catch (error) { if (guard.alive) feedback = qsTr("取消调用中断；提交状态未知，请等待原任务结束后核对。") }
                            finally { if (guard.alive) { root.callBusy = false; if (guard.alive) scheduleDrain() } }
                        }
                        function captureBrowse(number, row) {
                            if (!idle || life.draining || !candidateCurrent || !root.integer(number, 0, 24999)) return null
                            var actualRow = null
                            if (row) {
                                if (!pageCurrent || !root.rowKnown(row) || !root.pageKnown(candidatePage)) return null
                                var keys = Object.keys(row)
                                for (var i = 0; i < candidatePage.records.length; ++i) {
                                    var original = candidatePage.records[i], same = true
                                    for (var k = 0; k < keys.length; ++k) if (original[keys[k]] !== row[keys[k]]) { same = false; break }
                                    if (same) { actualRow = original; break }
                                }
                                if (!actualRow) return null
                            }
                            return {owner: owner, epoch: life.epoch, filePath: filePath, candidate: candidate,
                                expectedJobId: workspaceState.jobId, page: candidatePage, number: number, row: actualRow}
                        }
                        function browseCurrent(c) {
                            return originCurrent(c) && stateKnown && workspaceState.busy === false && workspaceState.jobId === c.expectedJobId
                                && candidateCurrent && c.candidate === candidate && (!c.row || pageCurrent && c.page === candidatePage && candidatePage.records.indexOf(c.row) >= 0)
                        }
                        function previewPage(c) {
                            if (!idle || life.draining || !browseCurrent(c) || c.row) return
                            var guard = life, original = c.candidate
                            root.callBusy = true
                            try {
                                if (!guard.alive || !browseCurrent(c)) return
                                var result = original.owner.historyPreviewPage(original.value.previewId, c.number)
                                if (!guard.alive || !browseCurrent(c)) return
                                if (!root.pageKnown(result)) { feedback = qsTr("候选页未确认，请核对原预览并明确重读。"); return }
                                for (var i = 0; i < result.records.length; ++i) if (result.records[i].origin !== "imported") return
                                if (result.total !== original.value.recordCount) return
                                candidatePage = result
                                if (!guard.alive || !originCurrent(original) || candidate !== original) return
                                pageCurrent = true
                                if (!guard.alive || !originCurrent(original) || candidate !== original) return
                                detailCurrent = false
                            } catch (error) { if (guard.alive) feedback = qsTr("候选页读取中断，没有追加历史或恢复权限。") }
                            finally { if (guard.alive) { root.callBusy = false; if (guard.alive) scheduleDrain() } }
                        }
                        function previewRecord(c) {
                            if (!idle || life.draining || !browseCurrent(c) || !c.row) return
                            var guard = life, original = c.candidate, row = c.row
                            root.callBusy = true
                            try {
                                if (!guard.alive || !browseCurrent(c)) return
                                var result = original.owner.historyPreviewRecord(original.value.previewId, row.handoffId)
                                if (!guard.alive || !browseCurrent(c)) return
                                if (!root.detailKnown(result) || result.origin !== "imported" || result.handoffId !== row.handoffId || result.recordRevision !== row.recordRevision) {
                                    feedback = qsTr("未取得原候选的完整历史详情；不会将当前本地记录冒充文件候选。"); return
                                }
                                candidateDetail = result
                                if (!guard.alive || !originCurrent(original) || candidate !== original) return
                                showCandidateVehicles = false
                                if (!guard.alive || !originCurrent(original) || candidate !== original) return
                                detailCurrent = true
                            } catch (error) { if (guard.alive) feedback = qsTr("候选详情读取中断，没有追加历史或处理待收请求。") }
                            finally { if (guard.alive) { root.callBusy = false; if (guard.alive) scheduleDrain() } }
                        }
                        function resultText() {
                            if (!savedResult) return qsTr("本页尚无已核对文件回执；共享最新值不能代领为本页成功。")
                            var r = savedResult.value, p = r.reply
                            if (r.action === "save" && saveReplyValid(p, savedResult.job)) return qsTr("原保存任务 %1\n文件：%2\n写入尝试 %3；写入结果已知 %4；文件已写 %5；回读一致 %6\n回读错误：%7\nfsync／断电耐久／认证均未验证。%8")
                                .arg(r.jobId).arg(p.path).arg(root.boolText(p.fileWriteAttempted)).arg(root.boolText(p.fileWrittenKnown)).arg(root.boolText(p.fileWritten)).arg(root.boolText(p.readbackVerified))
                                .arg(root.text(p.readbackError)).arg(p.fileWrittenKnown ? "" : qsTr("写入结果未知，不能把 false 当作未写入。"))
                            if (r.action === "merge" && Object.keys(p).length === 4) return qsTr("原追加任务 %1\n候选 %2；新增 %3；同内容重复 %4；合并后记录总数 %5\n这是历史追加事实，不证明认证、任务完成或控制权移交。")
                                .arg(r.jobId).arg(p.previewId).arg(p.addedCount).arg(p.duplicateCount).arg(p.recordCount)
                            return qsTr("原文件任务 %1 · %2；成功标记 %3；错误 %4。已加载仅是候选，不是已追加或替换当前记录。").arg(r.jobId).arg(r.action).arg(root.boolText(r.ok)).arg(root.text(r.error))
                        }
                        onOwnerChanged: { var guard = life; if (guard) { guard.seenJob = ""; retire(); if (guard.alive) scheduleDrain() } }
                        onFilePathChanged: { var guard = life; retire(); if (guard && guard.alive) overwriteConfirmed = false }
                        onExposedChanged: if (!exposed) retire()
                        onWorkspaceStateChanged: scheduleDrain()
                        onPreviewValueChanged: scheduleDrain()
                        onResultValueChanged: scheduleDrain()
                        Component.onDestruction: life.alive = false
                        Connections {
                            target: root
                            function onCallBusyChanged() { if (!root.callBusy) historyArea.scheduleDrain() }
                            function onIdentityTokenChanged() { historyArea.retire() }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.text; opacity: 0.25 }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("交接文件历史 · 保存 → 预览 → 另行确认追加"); font.bold: true; wrapMode: Text.WordWrap }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("不替换现有本地记录，不恢复账户、批准、待收处理或飞机控制。保存和加载明确读写本地 JSON；追加只处理整份已校验候选。同 ID 同内容去重，异内容整笔拒绝。关闭窗口不自动取消后台任务，重新打开也不自动加载或追加。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("独立本地 JSON 文件绝对路径"); wrapMode: Text.WordWrap }
                        Controls.TextField {
                            id: historyPathField
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 4096; selectByMouse: true
                            text: historyArea.filePath; enabled: historyArea.idle; color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize
                            Accessible.name: qsTr("交接历史本地 JSON 文件路径")
                            onTextEdited: historyArea.filePath = text
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            background: Rectangle { color: qgcPal.windowShade; border.width: parent.activeFocus ? 2 : 1; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text }
                        }
                        QGCLabel { Layout.fillWidth: true; visible: historyArea.filePath.length > 0 && !historyArea.pathValid; text: qsTr("请输入不含控制字符的本地绝对路径；URL、网络路径及实际文件安全检查由原生层拒绝，不自动转换路径。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                        Controls.CheckBox {
                            id: historyOverwriteBox
                            property var pressedPath: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("明确允许保存到此路径（存在时替换文件）")
                            checked: historyArea.overwriteConfirmed; enabled: historyArea.idle; focusPolicy: Qt.StrongFocus; Accessible.name: text
                            contentItem: QGCLabel { text: historyOverwriteBox.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; verticalAlignment: Text.AlignVCenter; leftPadding: historyOverwriteBox.indicator.width + root.pad }
                            indicator: Rectangle {
                                implicitWidth: ScreenTools.checkBoxIndicatorSize; implicitHeight: implicitWidth
                                x: historyOverwriteBox.leftPadding; y: (historyOverwriteBox.height - height) / 2
                                color: qgcPal.windowShade; border.width: 1; border.color: qgcPal.text
                                QGCColoredImage { anchors.fill: parent; anchors.margins: parent.width * 0.2; source: "/qmlimages/checkbox-check.svg"; color: qgcPal.text; fillMode: Image.PreserveAspectFit; visible: historyOverwriteBox.checked }
                            }
                            onPressed: pressedPath = {owner: historyArea.owner, filePath: historyArea.filePath, epoch: historyArea.life.epoch, jobId: historyArea.workspaceState.jobId}
                            onCanceled: pressedPath = null
                            onClicked: {
                                var guard = historyArea.life, c = pressedPath, value = checked
                                pressedPath = null
                                if (!guard.alive) return
                                if (c && c.owner === historyArea.owner && c.filePath === historyArea.filePath && c.epoch === guard.epoch
                                        && historyArea.idle && c.jobId === historyArea.workspaceState.jobId) {
                                    historyArea.overwriteConfirmed = value
                                    if (!guard.alive) return
                                }
                                checked = Qt.binding(function() { return historyArea.overwriteConfirmed })
                            }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                        }
                        Repeater {
                            model: [{action: "save", label: qsTr("明确保存当前交接历史到文件")}, {action: "load", label: qsTr("明确加载文件为候选（不追加）")}]
                            QGCButton {
                                id: historyIoButton
                                property var pressedIntent: null
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                                text: modelData.label; enabled: historyArea.idle && historyArea.pathValid && (modelData.action !== "save" || historyArea.overwriteConfirmed); opacity: enabled ? 1 : 0.45; Accessible.name: text
                                contentItem: QGCLabel { text: historyIoButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: historyIoButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                onPressed: pressedIntent = historyArea.capture(modelData.action)
                                onCanceled: pressedIntent = null
                                onClicked: { var guard = historyArea.life, c = pressedIntent; pressedIntent = null; if (guard.alive) historyArea.submit(c) }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            }
                        }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("文件任务状态：%1；占用 %2；任务 %3").arg(historyArea.stateKnown ? historyArea.workspaceState.stage : qsTr("未知")).arg(root.boolText(historyArea.workspaceState.busy)).arg(root.text(historyArea.workspaceState.jobId)); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                        QGCLabel { Layout.fillWidth: true; visible: historyArea.feedback.length > 0; text: historyArea.feedback; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText; Accessible.name: text }
                        QGCLabel { Layout.fillWidth: true; text: historyArea.resultText(); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                        QGCButton {
                            id: historyCancelButton
                            property var pressedJob: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            text: qsTr("取消本页原文件任务（等待结束）"); enabled: historyArea.canCancel; opacity: enabled ? 1 : 0.45; Accessible.name: text
                            contentItem: QGCLabel { text: historyCancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: historyCancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            onPressed: pressedJob = historyArea.ownedJob
                            onCanceled: pressedJob = null
                            onClicked: { var guard = historyArea.life, c = pressedJob; pressedJob = null; if (guard.alive) historyArea.cancelJob(c) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; visible: historyArea.candidate !== null; spacing: root.pad
                            QGCLabel { Layout.fillWidth: true; text: historyArea.candidate ? qsTr("%1\n候选 %2\n原文件 SHA-256 %3\n完整校验记录 %4 条，其中 Run 简报 %5 条；不是截断前缀。所有记录均仅历史、未认证，不恢复任何权限。\n原文件：%6")
                                .arg(historyArea.candidateCurrent ? qsTr("本页原生文件候选（尚未追加）") : qsTr("旧候选副本，仅供阅读，需重新明确加载"))
                                .arg(historyArea.candidate.value.previewId).arg(historyArea.candidate.value.contentHash).arg(historyArea.candidate.value.recordCount).arg(historyArea.candidate.value.runRecordCount).arg(historyArea.candidate.value.path) : ""; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                            QGCButton {
                                id: historyFirstPage
                                property var pressedBrowse: null
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("明确查看候选第一页（每页4条）"); enabled: historyArea.idle && historyArea.candidateCurrent; opacity: enabled ? 1 : 0.45; Accessible.name: text
                                contentItem: QGCLabel { text: historyFirstPage.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: historyFirstPage._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                onPressed: pressedBrowse = historyArea.captureBrowse(0, null)
                                onCanceled: pressedBrowse = null
                                onClicked: { var guard = historyArea.life, c = pressedBrowse; pressedBrowse = null; if (guard.alive) historyArea.previewPage(c) }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            }
                            QGCLabel { Layout.fillWidth: true; visible: root.pageKnown(historyArea.candidatePage); text: qsTr("候选页 %1 / %2，完整候选共 %3 条；%4。").arg(historyArea.candidatePage.pageCount ? historyArea.candidatePage.page + 1 : 0).arg(historyArea.candidatePage.pageCount).arg(historyArea.candidatePage.total)
                                .arg(historyArea.candidateCurrent && historyArea.pageCurrent ? qsTr("当前候选页，仅历史") : qsTr("旧页副本，仅供阅读；请明确读取首页")); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                            Repeater {
                                model: historyArea.candidatePage.records || []
                                QGCButton {
                                    id: historyCandidateRow
                                    readonly property var row: modelData
                                    property var pressedBrowse: null
                                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: historyArea.idle && historyArea.candidateCurrent && historyArea.pageCurrent; opacity: enabled ? 1 : 0.45
                                    text: qsTr("候选记录 %1 · revision %2\n%3 → %4\n%5 · 仅导入历史").arg(row.handoffId).arg(row.recordRevision).arg(row.fromOperator).arg(row.toOperator).arg(root.statusText(row.status)); Accessible.name: text
                                    contentItem: QGCLabel { text: historyCandidateRow.text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: historyCandidateRow._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                    Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                    onPressed: pressedBrowse = historyArea.captureBrowse(historyArea.candidatePage.page, row)
                                    onCanceled: pressedBrowse = null
                                    onClicked: { var guard = historyArea.life, c = pressedBrowse; pressedBrowse = null; if (guard.alive) historyArea.previewRecord(c) }
                                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true; spacing: root.pad
                                QGCButton { property var pressedBrowse: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("候选上一页"); enabled: historyArea.idle && historyArea.candidateCurrent && historyArea.pageCurrent && historyArea.candidatePage.page > 0; onPressed: pressedBrowse = historyArea.captureBrowse(historyArea.candidatePage.page - 1, null); onCanceled: pressedBrowse = null; onClicked: { var guard = historyArea.life, c = pressedBrowse; pressedBrowse = null; if (guard.alive) historyArea.previewPage(c) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus } }
                                QGCButton { property var pressedBrowse: null; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("候选下一页"); enabled: historyArea.idle && historyArea.candidateCurrent && historyArea.pageCurrent && historyArea.candidatePage.page + 1 < historyArea.candidatePage.pageCount; onPressed: pressedBrowse = historyArea.captureBrowse(historyArea.candidatePage.page + 1, null); onCanceled: pressedBrowse = null; onClicked: { var guard = historyArea.life, c = pressedBrowse; pressedBrowse = null; if (guard.alive) historyArea.previewPage(c) } onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus } }
                            }
                            QGCLabel { Layout.fillWidth: true; visible: root.briefKnown(root.briefOf(historyArea.candidateDetail)); text: qsTr("以下复用简报原文的 Run 身份、冻结时间与逐机观察均为文件声称，未认证；不是本次从本地运行取源，也不证明对应真实 Run 或飞行结果。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText }
                            QGCLabel { Layout.fillWidth: true; visible: root.detailKnown(historyArea.candidateDetail); text: qsTr("%1\n候选详情 %2 · %3\n%4 → %5\n创建 %6；到期 %7；处理时间 %8；处理人 %9\n原因：%10\n备注：%11\n%12").arg(historyArea.detailCurrent && historyArea.candidateCurrent ? qsTr("本页候选详情，仅历史") : qsTr("旧详情副本，仅供阅读"))
                                .arg(root.text(historyArea.candidateDetail.handoffId)).arg(root.statusText(historyArea.candidateDetail.status))
                                .arg(root.text(historyArea.candidateDetail.fromOperator)).arg(root.text(historyArea.candidateDetail.toOperator))
                                .arg(root.text(historyArea.candidateDetail.createdAt)).arg(root.text(historyArea.candidateDetail.expiresAt)).arg(root.text(historyArea.candidateDetail.resolvedAt)).arg(root.text(historyArea.candidateDetail.resolvedBy))
                                .arg(root.text(historyArea.candidateDetail.reason)).arg(root.text(historyArea.candidateDetail.note))
                                .arg(historyArea.candidateDetail.metadata && historyArea.candidateDetail.metadata.kind === "run-brief" ? root.briefText(root.briefOf(historyArea.candidateDetail)) : qsTr("通用历史记录，不含冻结 Run 简报；不从任务文字猜测真实运行。")); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                            QGCCheckBox {
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; activeFocusOnTab: true
                                visible: root.briefKnown(root.briefOf(historyArea.candidateDetail)); text: qsTr("显示候选 Run 简报逐机历史（最多32架）")
                                checked: historyArea.showCandidateVehicles; Accessible.name: text
                                onClicked: { var guard = historyArea.life; historyArea.showCandidateVehicles = checked; if (guard.alive) checked = Qt.binding(function() { return historyArea.showCandidateVehicles }) }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                            }
                            Repeater { model: root.expanded && historyArea.showCandidateVehicles && root.briefKnown(root.briefOf(historyArea.candidateDetail)) ? root.briefOf(historyArea.candidateDetail).summary.vehicles : []; QGCLabel { Layout.fillWidth: true; text: root.vehicleText(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere } }
                            QGCButton {
                                id: historyPrepareMerge
                                property var pressedIntent: null
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("准备仅追加这份历史候选…"); enabled: historyArea.idle && historyArea.candidateCurrent; opacity: enabled ? 1 : 0.45; Accessible.name: text
                                contentItem: QGCLabel { text: historyPrepareMerge.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: historyPrepareMerge._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                onPressed: pressedIntent = historyArea.capture("merge")
                                onCanceled: pressedIntent = null
                                onClicked: { var guard = historyArea.life, c = pressedIntent; pressedIntent = null; if (guard.alive && !guard.draining && historyArea.intentCurrent(c)) historyArea.mergeConfirmation = c }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; visible: historyArea.mergeConfirmation !== null
                                QGCLabel { Layout.fillWidth: true; text: qsTr("再次确认：只追加完整已校验历史，不替换原本地记录。重复同内容跳过，同 ID 异内容整笔拒绝。实际追加数量须等待后台与当前集合 CAS 成功回执，不自动重试。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                                QGCButton {
                                    property var pressedConfirmation: null
                                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("确认追加这一份历史候选"); enabled: historyArea.intentCurrent(historyArea.mergeConfirmation)
                                    Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                    onPressed: pressedConfirmation = historyArea.mergeConfirmation
                                    onCanceled: pressedConfirmation = null
                                    onClicked: { var guard = historyArea.life, c = pressedConfirmation; pressedConfirmation = null; if (guard.alive && c && c === historyArea.mergeConfirmation) historyArea.submit(c) }
                                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                                }
                                QGCButton {
                                    property var pressedConfirmation: null
                                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("放弃这次追加确认")
                                    Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                    onPressed: pressedConfirmation = historyArea.mergeConfirmation
                                    onCanceled: pressedConfirmation = null
                                    onClicked: { var guard = historyArea.life, c = pressedConfirmation; pressedConfirmation = null; if (guard.alive && c && c === historyArea.mergeConfirmation) historyArea.mergeConfirmation = null }
                                    onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                                }
                            }
                            QGCButton {
                                id: historyDiscard
                                property var pressedIntent: null
                                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("丢弃此原生候选（不删除文件或已追加记录）"); enabled: historyArea.idle && historyArea.candidateCurrent; opacity: enabled ? 1 : 0.45; Accessible.name: text
                                contentItem: QGCLabel { text: historyDiscard.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: historyDiscard._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: parent.activeFocus }
                                onPressed: pressedIntent = historyArea.capture("discard")
                                onCanceled: pressedIntent = null
                                onClicked: { var guard = historyArea.life, c = pressedIntent; pressedIntent = null; if (guard.alive) historyArea.submit(c) }
                                onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: qgcPal.text; opacity: 0.25 }
                    QGCLabel { Layout.fillWidth: true; text: qsTr("本地收件箱与历史记录 · 每页 4 条"); font.bold: true; wrapMode: Text.WordWrap }
                    QGCComboBox {
                        id: filterChoice
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        model: [qsTr("我待接收"), qsTr("我待发出"), qsTr("我相关历史（含只读导入）"), qsTr("全部本地记录")]
                        currentIndex: root.filterIndex; enabled: !root.callBusy; Accessible.name: qsTr("本地交接记录筛选")
                        onActivated: { root.filterIndex = index; currentIndex = Qt.binding(function() { return root.filterIndex }) }
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: filterChoice.activeFocus }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: !root.operatorId && root.filter !== "all"; text: qsTr("尚未认证；可选择“全部本地记录”只读查看。接收、拒绝和撤销仍需对应账户权限。"); wrapMode: Text.WordWrap }
                    QGCButton {
                        id: refreshButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.canList; opacity: enabled ? 1 : 0.45
                        text: qsTr("明确刷新本地列表（第一页）"); Accessible.name: text
                        contentItem: QGCLabel { text: refreshButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: refreshButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: refreshButton.activeFocus }
                        onClicked: root.refreshPage(0)
                        onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: !root.pageCurrent; text: qsTr("列表尚未读取或已变旧，请明确刷新。没有轮询；旧详情不允许确认。"); wrapMode: Text.WordWrap; color: qgcPal.warningText }
                    QGCLabel { Layout.fillWidth: true; visible: root.pageKnown(root.pageResult); text: qsTr("共 %1 条；第 %2 / %3 页。%4").arg(root.pageResult.total).arg(root.pageResult.pageCount ? root.pageResult.page + 1 : 0).arg(root.pageResult.pageCount).arg(root.pageCurrent ? qsTr("当前本地快照") : qsTr("旧列表参考")); wrapMode: Text.WordWrap }
                    Repeater {
                        model: root.expanded ? root.records : []
                        QGCButton {
                            id: recordButton
                            readonly property var row: modelData
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: root.ready && root.pageCurrent; opacity: enabled ? 1 : 0.45
                            text: qsTr("查看 %1 · revision %2\n%3 → %4\n%5 · %6 · %7")
                                .arg(row.handoffId).arg(row.recordRevision).arg(row.fromOperator).arg(row.toOperator).arg(root.statusText(row.status))
                                .arg(row.briefAvailable ? qsTr("含冻结 Run 简报") : qsTr("旧审计记录，无 Run 简报")).arg(row.origin === "imported" ? qsTr("导入，仅历史查阅") : qsTr("本实例创建"))
                            Accessible.name: text
                            contentItem: QGCLabel { text: recordButton.text; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: recordButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: recordButton.activeFocus }
                            onClicked: root.openDetail(row)
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: root.pad
                        QGCButton { id: previousPage; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("上一页"); showBorder: true; enabled: root.canList && root.pageCurrent && root.pageResult.page > 0; opacity: enabled ? 1 : 0.45; Accessible.name: text; onClicked: root.refreshPage(root.pageResult.page - 1); onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: previousPage.activeFocus } }
                        QGCButton { id: nextPage; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("下一页"); showBorder: true; enabled: root.canList && root.pageCurrent && root.pageResult.page + 1 < root.pageResult.pageCount; opacity: enabled ? 1 : 0.45; Accessible.name: text; onClicked: root.refreshPage(root.pageResult.page + 1); onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: nextPage.activeFocus } }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; visible: root.detailKnown(root.selectedDetail); spacing: root.pad
                        QGCLabel { Layout.fillWidth: true; text: qsTr("所选记录详情（原样历史快照）"); font.bold: true; wrapMode: Text.WordWrap }
                        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("记录 %1 · revision %2\n%3 → %4\n状态：%5；详情仍当前：%6\n创建 %7；到期 %8；处理时间 %9；处理人 %10\n原因：%11\n已记录备注：%12")
                            .arg(root.text(root.selectedDetail.handoffId)).arg(root.selectedDetail.recordRevision || 0).arg(root.text(root.selectedDetail.fromOperator)).arg(root.text(root.selectedDetail.toOperator))
                            .arg(root.statusText(root.selectedDetail.status)).arg(root.boolText(root.detailCurrent)).arg(root.text(root.selectedDetail.createdAt)).arg(root.text(root.selectedDetail.expiresAt))
                            .arg(root.text(root.selectedDetail.resolvedAt)).arg(root.text(root.selectedDetail.resolvedBy)).arg(root.text(root.selectedDetail.reason)).arg(root.text(root.selectedDetail.note)) }
                        QGCLabel { Layout.fillWidth: true; visible: root.selectedDetail.origin === "imported"; text: qsTr("导入记录只读：不是当前已认证交接，不允许接收、拒绝或撤销，也不恢复任何权限。"); wrapMode: Text.WordWrap; color: qgcPal.warningText }
                        QGCLabel { Layout.fillWidth: true; visible: !root.detailIsRun; text: qsTr("旧审计记录没有冻结 Run 简报；任务文字不证明存在真实运行。若本实例原生允许，仍只能确认这条旧审计记录。"); wrapMode: Text.WordWrap; color: qgcPal.warningText }
                        QGCLabel { Layout.fillWidth: true; visible: root.detailIsRun; text: root.briefText(root.detailBrief); textFormat: Text.PlainText; wrapMode: Text.Wrap }
                        QGCCheckBox { id: detailVehiclesToggle; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: root.briefKnown(root.detailBrief); text: qsTr("显示简报逐机观察"); activeFocusOnTab: true; Accessible.name: text; checked: root.showDetailVehicles; onClicked: root.showDetailVehicles = checked; onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this); Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: detailVehiclesToggle.activeFocus } }
                        Repeater { model: root.expanded && root.showDetailVehicles && root.briefKnown(root.detailBrief) ? root.detailBrief.summary.vehicles : []; QGCLabel { Layout.fillWidth: true; text: root.vehicleText(modelData); textFormat: Text.PlainText; wrapMode: Text.Wrap } }
                        QGCLabel { Layout.fillWidth: true; text: qsTr("本次确认备注（可选，最多 1024 字符）"); wrapMode: Text.WordWrap }
                        Controls.TextField {
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 1024; selectByMouse: true
                            text: root.noteText; enabled: root.canResolve; color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize; Accessible.name: qsTr("交接确认备注")
                            onTextEdited: { root.noteText = text; root.detailConsent = null; root.clearPressed() }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            background: Rectangle { color: qgcPal.windowShade; border.width: parent.activeFocus ? 2 : 1; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text }
                        }
                        QGCCheckBox {
                            id: detailConsentBox
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("已阅本条记录与简报"); activeFocusOnTab: true
                            Accessible.name: qsTr("已阅读此精确记录及简报，下一操作仅为本地审计确认")
                            checked: root.detailConfirmed; enabled: root.canResolve
                            onClicked: { root.detailConsent = checked && root.canResolve ? root.intent("resolve") : null; checked = Qt.binding(function() { return root.detailConfirmed }) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: detailConsentBox.activeFocus }
                        }
                        QGCButton {
                            id: acceptButton
                            property var pressedIntent: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            enabled: root.canResolve && root.detailConfirmed && root.selectedDetail.toOperator === root.operatorId; opacity: enabled ? 1 : 0.45
                            text: root.detailIsRun ? qsTr("确认收到这份简报（不接管飞机）") : qsTr("确认旧审计记录（不证明真实 Run）"); Accessible.name: text
                            contentItem: QGCLabel { text: acceptButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: acceptButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: acceptButton.activeFocus }
                            onPressed: pressedIntent = root.intent("accept")
                            onCanceled: pressedIntent = null
                            onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                        QGCButton {
                            id: rejectButton
                            property var pressedIntent: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            enabled: root.canResolve && root.detailConfirmed && root.selectedDetail.toOperator === root.operatorId; opacity: enabled ? 1 : 0.45
                            text: qsTr("拒绝此本地交接请求"); Accessible.name: text
                            contentItem: QGCLabel { text: rejectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: rejectButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: rejectButton.activeFocus }
                            onPressed: pressedIntent = root.intent("reject")
                            onCanceled: pressedIntent = null
                            onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                        QGCButton {
                            id: cancelButton
                            property var pressedIntent: null
                            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                            enabled: root.canResolve && root.detailConfirmed && root.selectedDetail.fromOperator === root.operatorId; opacity: enabled ? 1 : 0.45
                            text: qsTr("发起人撤销此本地请求"); Accessible.name: text
                            contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: cancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: cancelButton.activeFocus }
                            onPressed: pressedIntent = root.intent("cancel")
                            onCanceled: pressedIntent = null
                            onClicked: { var original = pressedIntent; pressedIntent = null; root.perform(original) }
                            onActiveFocusChanged: if (activeFocus) root.ensureFocusVisible(this)
                        }
                    }
                }
            }
        }
    }
}

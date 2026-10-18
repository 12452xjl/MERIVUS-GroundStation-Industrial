import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Grounded staging and snapshot inspection only. No start/arm/mission-start
// action is exposed. The native backend still owns the final upload gates.
FocusScope {
    id: root
    property var controller: null
    // The one staging surface follows an explicitly selected local review.
    // These values never replace the native upload/identity gates.
    property string sourceKind: "draft"
    property string contextToken: ""
    property bool approvalAllowed: false
    property var refreshVehicleIds: []
    property var archive: ({})
    property var plan: ({})
    property var encoding: ({})
    property bool previewCurrent: false
    property bool inputSettled: false
    property var pendingRequest: ({})
    property var pendingController: null
    property var confirmationDialog: null
    property bool dispatchBusy: false
    property var reconciliationLifetime: ({alive: true})
    property string localError: ""
    property string localNotice: ""
    property double clockMs: Date.now()
    property var inspection: ({})
    property double inspectionCapturedMs: -1
    property string inspectedDeploymentId: ""
    property string inspectedState: ""

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.4)
    readonly property var deployment: controller && controller.missionDeployment ? controller.missionDeployment.current || {} : ({})
    readonly property var authorization: controller && controller.fleetUploadAuthorization ? controller.fleetUploadAuthorization : ({})
    readonly property var identityState: controller && controller.fleetOperatorIdentity ? controller.fleetOperatorIdentity : ({})
    readonly property var identity: identityState.identityReceipt || ({})
    readonly property var uploadApproval: controller && controller.fleetUploadApproval ? controller.fleetUploadApproval : ({})
    readonly property var secondApproval: uploadApproval.approvalReceipt || ({})
    readonly property var uploadLease: uploadApproval.uploadLease || ({})
    readonly property var evidenceRefresh: controller && controller.fleetEvidenceRefresh ? controller.fleetEvidenceRefresh : ({})
    readonly property var refreshAuthorization: controller && controller.fleetEvidenceRefreshAuthorization ? controller.fleetEvidenceRefreshAuthorization : ({})
    readonly property bool sourceKindKnown: sourceKind === "draft" || sourceKind === "remainder" || sourceKind === "reserve-dispatch"
    readonly property string displayedSourceIdentity: JSON.stringify([sourceKind, contextToken,
        archive.sourceTaskId, archive.recordId, archive.recordRevision, archive.planId,
        archive.contentHash, archive.reviewEvidenceHash, archive.storeEpoch])
    readonly property var refreshTargets: reviewRefreshTargets()
    readonly property bool refreshActive: evidenceRefresh.active === true
    readonly property var contentCheck: controller && controller.fleetStartContentCheck ? controller.fleetStartContentCheck : ({})
    readonly property var contentAuthorization: controller && controller.fleetStartContentAuthorization ? controller.fleetStartContentAuthorization : ({})
    readonly property var contentTargets: deploymentReadTargets()
    readonly property bool contentBusy: contentCheck.active === true || contentCheck.reservationHeld === true || contentCheck.transportDraining === true
    readonly property var reconciliation: controller && controller.missionDeployment ? controller.missionDeployment.reconciliation || {} : ({})
    readonly property var reconciliationAuthorization: controller && controller.fleetReconciliationAuthorization ? controller.fleetReconciliationAuthorization : ({})
    readonly property string reconciliationSourceToken: controller && controller.fleetDeploymentLifecycle
        && typeof controller.fleetDeploymentLifecycle.currentToken === "string" ? controller.fleetDeploymentLifecycle.currentToken : ""
    readonly property var reconciliationSnapshot: checkedReconciliation(reconciliation, false)
    readonly property bool reconciliationBusy: reconciliation.active === true || reconciliation.reservationHeld === true || reconciliation.transportDraining === true
    readonly property var repairDraft: controller && controller.fleetMissionRepairDraft ? controller.fleetMissionRepairDraft : ({})
    readonly property string repairDraftToken: typeof repairDraft.draftToken === "string" ? repairDraft.draftToken : ""
    readonly property string repairSourceToken: typeof repairDraft.sourceToken === "string" ? repairDraft.sourceToken : ""
    readonly property bool repairLocalReady: sourceKind === "draft" && visible && controller && repairDraft.schemaVersion === 1 && !!repairDraftToken
        && inputSettled && !dispatchBusy && !pendingRequest.requestId && repairDraft.busy === false
        && !transportBusy && !contentBusy && !reconciliationBusy && !refreshActive
    readonly property bool canInspectRepair: repairLocalReady && typeof controller.inspectFleetMissionRepairDraft === "function"
        && !!reconciliationSnapshot.requestId && ["failed", "cancelled"].indexOf(deployment.state) >= 0
        && ["completed", "stopped", "cancelled"].indexOf(reconciliationSnapshot.state) >= 0
        && reconciliationSnapshot.pendingVehicleCount === 0
    readonly property bool canCreateRepair: repairLocalReady && repairDraft.sourceCurrent === true && !!repairSourceToken
        && repairDraft.destructionPending === false && typeof controller.createFleetMissionRepairDraft === "function"
    readonly property bool canRestoreRepair: repairLocalReady && repairDraft.hasPrevious === true
        && typeof controller.restorePreviousFleetMissionDraft === "function"
    onRepairDraftTokenChanged: {
        if (repairCreateConsent) repairCreateConsent.checked = false
        if (repairReplaceConsent) repairReplaceConsent.checked = false
        if (repairRestoreConsent) repairRestoreConsent.checked = false
    }
    onRepairSourceTokenChanged: {
        if (repairCreateConsent) repairCreateConsent.checked = false
        if (repairReplaceConsent) repairReplaceConsent.checked = false
    }
    readonly property bool reconciliationIdentityKnown: freezeIdentity(identity) !== null
        && identity.actorId === reconciliationAuthorization.actorId && identity.role === reconciliationAuthorization.role
    readonly property bool canReconcile: controller && typeof controller.requestFleetMissionReconciliation === "function"
        && typeof controller.cancelFleetMissionReconciliation === "function" && !!reconciliationSourceToken && inputSettled
        && ["failed", "cancelled"].indexOf(deployment.state) >= 0 && readbackDecimal(deployment.generation, false)
        && contentTargets.length > 0 && reconciliationIdentityKnown && reconciliationAuthorization.allowed === true
        && !reconciliationBusy && !contentBusy && !refreshActive && !transportBusy && !dispatchBusy && !pendingRequest.requestId
    readonly property string reconciliationBlockedReason: !controller || !reconciliationSourceToken ? qsTr("没有当前原生部署来源。")
        : ["failed", "cancelled"].indexOf(deployment.state) < 0 || !contentTargets.length ? qsTr("此入口只核对本次运行失败 / 已取消部署的原冻结整组目标，不采用当前草稿。")
        : !inputSettled ? qsTr("请先结束当前编辑或地图选点。")
        : !reconciliationIdentityKnown ? qsTr("请先认证当前操作员身份。")
        : reconciliationAuthorization.allowed !== true ? qsTr("当前角色没有失败部署只读对账权限。")
        : reconciliationBusy ? qsTr("整组对账或原协议排空仍占用；取消只停止后续读取，不强制释放在途协议。")
        : contentBusy || refreshActive || transportBusy || dispatchBusy || !!pendingRequest.requestId ? qsTr("请先结束其它传输、刷新或上传确认。")
        : qsTr("请明确同意撤销旧审阅 / 审批后，只读核对原整组任务；最终实际对象、地面状态和链路仍由原生后端检查。")
    property double contentObservedMs: Date.now()
    readonly property double contentDisplayRemainingMs: integer(contentCheck.remainingMs, 0, 120000) && clockMs >= contentObservedMs
        ? Math.max(0, contentCheck.remainingMs - (clockMs - contentObservedMs)) : 0
    readonly property bool contentFresh: contentCheck.ok === true && contentCheck.state === "content-matched" && !contentBusy
        && contentCheck.deploymentId === deployment.deploymentId && contentBindingMatches(contentCheck.binding, deployment.binding)
        && contentDisplayRemainingMs > 0
    readonly property bool canReadContent: controller && typeof controller.requestFleetStartContentCheck === "function"
        && typeof controller.cancelFleetStartContentCheck === "function" && inputSettled && contentTargets.length > 0
        && hasAuthenticatedIdentity && contentAuthorization.allowed === true && !contentBusy && !refreshActive && !reconciliationBusy
        && !transportBusy && !dispatchBusy && !pendingRequest.requestId
    readonly property string contentBlockedReason: !controller || !contentTargets.length ? qsTr("需要本次运行的有效部署记录；目标来自已上传部署，不来自当前草稿。")
        : !inputSettled ? qsTr("请先结束草稿编辑或地图选点。")
        : !hasAuthenticatedIdentity ? qsTr("请先认证操作员身份。")
        : contentAuthorization.allowed !== true ? qsTr("当前角色没有机载任务只读核对权限。")
        : contentBusy ? qsTr("任务读取或排空尚未结束；可停止当前读取。")
        : reconciliationBusy || refreshActive || transportBusy || dispatchBusy || !!pendingRequest.requestId ? qsTr("请先结束其它传输、对账、刷新或上传确认。")
        : qsTr("请明确同意撤销旧审阅和审批；原生后端仍核对实际任务、来源和地面状态。")
    readonly property string refreshBlockedReason: !controller ? qsTr("没有当前控制器。")
        : !inputSettled ? qsTr("请先结束草稿编辑或地图选点。")
        : !refreshTargets.length ? qsTr("当前显示来源没有可识别的完整成员范围；请检查该来源的计划，不会改用另一份草稿的飞机。")
        : !hasAuthenticatedIdentity ? qsTr("请先认证操作员身份。")
        : refreshAuthorization.allowed !== true ? qsTr("当前角色没有只读刷新权限。")
        : refreshActive ? qsTr("当前刷新尚未结束；可以停止该请求。")
        : contentBusy ? qsTr("机载任务只读核对或排空尚未结束。")
        : reconciliationBusy ? qsTr("失败部署整组只读对账或排空尚未结束。")
        : transportBusy ? qsTr("原任务传输或排空尚未结束。")
        : dispatchBusy || !!pendingRequest.requestId ? qsTr("请先结束当前操作或取消上传确认。")
        : qsTr("请勾选同意撤销旧审阅与审批；最终链路与地面状态仍由原生后端检查。")
    readonly property bool hasAuthenticatedIdentity: freezeIdentity(identity) !== null
        && identity.actorId === authorization.actorId && identity.role === authorization.role
    readonly property bool hasSecondApproval: uploadApproval.state === "approved" && freezeApproval(secondApproval) !== null
        && archiveBindingMatches(secondApproval.binding) && secondApproval.actorId === identity.actorId
        && secondApproval.applicantSessionId === identity.sessionId && secondApproval.applicantSessionEpoch === identity.sessionEpoch
        && secondApproval.applicantSessionRevision === identity.sessionRevision
        && secondApproval.rolePolicyEpoch === authorization.policyEpoch && secondApproval.rolePolicyRevision === authorization.policyRevision
    readonly property bool hasUploadPermission: freezeAuthorization(authorization) !== null
        && authorization.allowed === true && hasAuthenticatedIdentity && hasSecondApproval
    readonly property bool transportActive: deployment.state === "preparing" || deployment.state === "uploading" || deployment.state === "readback"
    readonly property bool transportBusy: transportActive || deployment.transportDraining === true || deployment.reservationHeld === true
    readonly property var previewTargets: encodedTargets()
    readonly property bool approvalCurrent: controller && sourceKindKnown && !!contextToken && previewCurrent && inputSettled
        && approvalAllowed === true && controller.intentTaskState === "approved"
        && typeof archive.recordId === "string" && archive.recordId.length > 0
        && integer(archive.recordRevision, 1, 9007199254740991)
        && typeof archive.planId === "string" && archive.planId === plan.planId
        && /^[0-9a-f]{64}$/.test(archive.planId)
        && encoding.ok === true && previewTargets.length > 0
        && integer(encoding.totalItemCount, 1, 8192)
    readonly property bool requestCurrent: pendingRequest.requestId !== undefined
        && pendingController === controller && approvalCurrent && !transportBusy
        && hasUploadPermission && authorizationMatches(pendingRequest.actorAuthorization)
        && securityMatches(pendingRequest)
        && bindingMatches(pendingRequest) && clockMs >= pendingRequest.createdAtMs && clockMs <= pendingRequest.expiresAtMs
    readonly property bool canRequest: approvalCurrent && hasUploadPermission && !transportBusy && !refreshActive && !contentBusy && !reconciliationBusy && !dispatchBusy && !pendingRequest.requestId
        && controller && typeof controller.requestFleetMissionUpload === "function"
        && typeof controller.cancelFleetMissionUploadRequest === "function"
    readonly property bool inspectionPresent: inspectionCapturedMs >= 0
    readonly property bool inspectionExpired: !inspectionPresent || clockMs < inspectionCapturedMs
        || clockMs - inspectionCapturedMs > 1000 || inspectedDeploymentId !== deployment.deploymentId
        || inspectedState !== deployment.state || transportBusy || contentBusy || reconciliationBusy
        || (controller && controller.missionDeployment
            && (controller.missionDeployment.startReadiness || {}).snapshotExpired === true)
    readonly property var readinessRows: boundedList((inspection.startReadiness || {}).vehicles, 32)
    readonly property var progressRows: boundedList((inspection.progressObservation || {}).vehicles, 32)
    readonly property var startReview: controller && controller.missionStartReview ? controller.missionStartReview.current || {} : ({})
    readonly property var startReviewArtifact: startReview
    property bool showStartReviewReasons: false

    implicitWidth: ScreenTools.defaultFontPixelWidth * 48
    implicitHeight: content.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function integer(value, minimum, maximum) {
        return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= minimum && value <= maximum
    }
    function boundedList(value, maximum) {
        if (!value || typeof value === "string" || !integer(value.length, 0, 8192)) return []
        var result = []
        for (var i = 0; i < Math.min(value.length, maximum); ++i) result.push(value[i])
        return result
    }
    function text(value, maximum) {
        return (typeof value === "string" && value.length > 0) || typeof value === "number"
            ? String(value).substring(0, maximum || 512) : qsTr("未知")
    }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function resetSourceConsent() {
        resetRepairConsent()
        if (uploadButton) { uploadButton.pressedBinding = ({}); uploadButton.releasedCurrent = false }
        if (uploadApprovalButton) uploadApprovalButton.pressedBinding = ({})
        if (refreshConsent) refreshConsent.checked = false
        if (contentConsent) contentConsent.checked = false
        if (reconciliationConsent) reconciliationConsent.checked = false
        if (pendingRequest.requestId) cancelPending(qsTr("显示的规划来源、记录或上下文已变化；旧上传确认已取消，请重新检查当前来源。"))
    }
    function resetRepairConsent() {
        if (repairCreateConsent) repairCreateConsent.checked = false
        if (repairReplaceConsent) repairReplaceConsent.checked = false
        if (repairRestoreConsent) repairRestoreConsent.checked = false
        if (repairInspectButton) repairInspectButton.pressedIntent = null
        if (repairCreateButton) repairCreateButton.pressedIntent = null
        if (repairCancelButton) repairCancelButton.pressedIntent = null
        if (repairRestoreButton) repairRestoreButton.pressedIntent = null
    }
    function repairIntent(action) {
        return {action: action, owner: controller, draftToken: repairDraftToken, sourceToken: repairSourceToken,
            episodeToken: reconciliationSourceToken, deploymentId: deployment.deploymentId,
            requestId: reconciliationSnapshot.requestId, replacement: repairReplaceConsent.checked,
            confirmed: action === "create" ? repairCreateConsent.checked : action === "restore" ? repairRestoreConsent.checked : true}
    }
    function dispatchRepair(intent) {
        if (!intent || intent.owner !== controller || intent.draftToken !== repairDraftToken
                || intent.episodeToken !== reconciliationSourceToken || !repairLocalReady) return
        var action = intent.action, ready = action === "inspect" ? canInspectRepair
            : action === "create" ? canCreateRepair && repairCreateConsent.checked && intent.confirmed === true
                && (repairDraft.hasPrevious !== true || (intent.replacement && repairReplaceConsent.checked))
            : action === "restore" ? canRestoreRepair && repairRestoreConsent.checked && intent.confirmed === true
            : action === "cancel" && !!repairSourceToken
        if (!ready || intent.sourceToken !== repairSourceToken || intent.deploymentId !== deployment.deploymentId
                || intent.requestId !== reconciliationSnapshot.requestId) {
            fail(qsTr("原规划来源或本地草稿已变化，请重新检查并明确确认。")); return
        }
        var guard = reconciliationLifetime, result = {}
        dispatchBusy = true
        repairCreateConsent.checked = false; repairReplaceConsent.checked = false; repairRestoreConsent.checked = false
        try {
            if (action === "inspect") result = intent.owner.inspectFleetMissionRepairDraft(intent.deploymentId, intent.requestId, intent.draftToken) || {}
            else if (action === "create") result = intent.owner.createFleetMissionRepairDraft(intent.sourceToken, intent.replacement, true) || {}
            else if (action === "restore") result = intent.owner.restorePreviousFleetMissionDraft(intent.draftToken, true) || {}
            else result = {ok: intent.owner.cancelFleetMissionRepairDraftPreview(intent.sourceToken) === true}
        } catch (error) { result = {error: "local_repair_call_interrupted_check_current_draft"} }
        if (!guard.alive) return
        dispatchBusy = false
        if (intent.owner !== controller || intent.episodeToken !== reconciliationSourceToken) {
            fail(qsTr("返回时当前来源已变化；不能确认当前草稿。请查看实际草稿与本地结果，不能把中断当成未发生修改。")); return
        }
        if (action === "cancel" && result.ok === true) {
            localError = ""; localNotice = qsTr("已取消本地来源预览；草稿、部署及机载任务没有因此恢复或改变。"); return
        }
        var returnedCurrent = result.ok === true && result.current === true && result.flightCommandReleased === false
            && result.executable === false && result.draftToken === repairDraftToken
        if (!returnedCurrent || ((action === "create" || action === "restore") && result.committed !== true)) {
            fail(qsTr("本地操作未完整确认：%1。已发布草稿：%2；新候选已装入：%3；备用坐标确认已变化：%4。请核对当前草稿后重试；不会自动上传、回滚或启动。")
                .arg(text(result.error, 160)).arg(known(result.committed)).arg(known(result.candidateInitialized)).arg(known(result.backupChanged)))
            return
        }
        localError = ""
        localNotice = action === "inspect" ? qsTr("已读取原归档的规划来源摘要。尚未创建副本；不同或未知对账结果不是修复成功。")
            : qsTr("已%1；之前的当前草稿保留为可切回备用。仍需重新确认坐标基准、重新生成审阅并审批；没有上传或启动。审计追加：%2。")
                .arg(action === "restore" ? qsTr("切回备用草稿内容") : qsTr("建立可编辑修复副本"))
                .arg(result.auditRecorded === true ? qsTr("已记录") : qsTr("未确认（不撤销已经发生的本地修改）"))
    }
    function readbackMap(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function reconciliationUuid(value) {
        return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value)
            && value !== "00000000-0000-0000-0000-000000000000"
    }
    function reconciliationDigest(value) { return typeof value === "string" && (value === "" || /^[0-9a-f]{64}$/.test(value)) }
    function reconciliationActorMatches(value) {
        if (!readbackMap(value) || Object.keys(value).length !== 9 || value.action !== "mission.reconcile.read"
                || value.allowed !== true || value.requiresSecondApproval !== false
                || value.actorAuthenticated !== false || value.executionPermitted !== false) return false
        var keys = ["actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision", "actorAuthenticated", "executionPermitted"]
        for (var i = 0; i < keys.length; ++i) if (value[keys[i]] !== reconciliationAuthorization[keys[i]]) return false
        return true
    }
    function checkedReconciliation(value, acceptedResult) {
        if (!readbackMap(value) || Object.keys(value).length !== (acceptedResult ? 32 : 30)
                || (acceptedResult && (value.ok !== true || value.requestAccepted !== true))
                || value.schemaVersion !== 1 || !reconciliationUuid(value.requestId) || !reconciliationUuid(value.runEpoch)
                || !readbackDecimal(value.generation, false) || value.deploymentId !== deployment.deploymentId
                || !reconciliationUuid(value.deploymentId) || value.deploymentGeneration !== deployment.generation
                || !readbackDecimal(value.deploymentGeneration, false) || !contentBindingMatches(value.binding, deployment.binding)
                || ["reading", "completed", "stopped", "cancelled"].indexOf(value.state) < 0
                || typeof value.reason !== "string" || value.reason.length > 256
                || typeof value.active !== "boolean" || typeof value.reservationHeld !== "boolean" || typeof value.transportDraining !== "boolean"
                || value.sourceRepresentation !== "independent-diagnostic-raw-int-history"
                || value.historicalOnly !== true || value.snapshotOnly !== true || value.currentOnboardContentsProven !== false
                || value.startupPermitGranted !== false || value.executionVerified !== false
                || value.flightCommandReleased !== false || value.executable !== false
                || !integer(value.totalVehicleCount, 1, 32) || !integer(value.matchedVehicleCount, 0, 32)
                || !integer(value.differentVehicleCount, 0, 32) || !integer(value.unknownVehicleCount, 0, 32)
                || !integer(value.pendingVehicleCount, 0, 32) || !integer(value.expectedTotalItemCount, 1, 8192)
                || !integer(value.observedTotalItemCount, 0, 8192) || !integer(value.remainingMs, 0, 120000)
                || !value.vehicleIds || !value.vehicles || typeof value.vehicleIds === "string" || typeof value.vehicles === "string"
                || value.vehicleIds.length !== value.totalVehicleCount || value.vehicles.length !== value.totalVehicleCount
                || contentTargets.length !== value.totalVehicleCount
                || value.matchedVehicleCount + value.differentVehicleCount + value.unknownVehicleCount + value.pendingVehicleCount !== value.totalVehicleCount) return ({})
        var expectedTotal = 0, observedTotal = 0, matched = 0, different = 0, unknown = 0, pending = 0
        for (var i = 0; i < value.totalVehicleCount; ++i) {
            var row = value.vehicles[i]
            if (value.vehicleIds[i] !== contentTargets[i] || !readbackMap(row) || Object.keys(row).length !== 16
                    || row.vehicleId !== contentTargets[i] || ["queued", "reading", "matched", "different", "unknown"].indexOf(row.state) < 0
                    || typeof row.reason !== "string" || row.reason.length > 256
                    || typeof row.readScopeId !== "string" || (row.readScopeId !== "" && !reconciliationUuid(row.readScopeId))
                    || !readbackDecimal(row.operationId, true) || !integer(row.expectedItemCount, 1, 2048)
                    || row.expectedItemCount !== deployment.vehicles[i].itemCount || !integer(row.observedItemCount, -1, 2048)
                    || !reconciliationDigest(row.expectedContentHash) || row.expectedContentHash.length !== 64
                    || !reconciliationDigest(row.observedContentHash) || !reconciliationDigest(row.rawItemsFingerprint)
                    || typeof row.countObserved !== "boolean" || typeof row.completeReadObserved !== "boolean"
                    || ["canonical-v2", "canonical-heading-tick-v3", "count-only", "none"].indexOf(row.comparisonBasis) < 0
                    || typeof row.observedAtMonotonicMs !== "string"
                    || (row.observedAtMonotonicMs !== "" && !readbackDecimal(row.observedAtMonotonicMs, true))
                    || row.historicalOnly !== true || row.currentOnboardContentsProven !== false) return ({})
            if (row.completeReadObserved !== row.countObserved
                    || (row.completeReadObserved && (row.observedItemCount < 0 || !row.observedAtMonotonicMs
                        || !row.readScopeId || row.operationId === "0" || row.rawItemsFingerprint.length !== 64))
                    || (!row.completeReadObserved && (row.observedItemCount !== -1 || row.observedAtMonotonicMs !== ""
                        || row.observedContentHash !== "" || row.rawItemsFingerprint !== "" || row.comparisonBasis !== "none"))) return ({})
            if (row.state === "matched" && (!row.completeReadObserved || !readbackCanonicalBasis(row.comparisonBasis)
                    || row.expectedContentHash !== row.observedContentHash || row.expectedItemCount !== row.observedItemCount)) return ({})
            if (row.state === "different" && (!row.completeReadObserved || row.comparisonBasis === "none")) return ({})
            expectedTotal += row.expectedItemCount
            if (row.completeReadObserved) observedTotal += row.observedItemCount
            if (row.state === "matched") ++matched
            else if (row.state === "different") ++different
            else if (row.state === "unknown") ++unknown
            else ++pending
        }
        if (expectedTotal !== value.expectedTotalItemCount || observedTotal !== value.observedTotalItemCount
                || matched !== value.matchedVehicleCount || different !== value.differentVehicleCount
                || unknown !== value.unknownVehicleCount || pending !== value.pendingVehicleCount) return ({})
        return value
    }
    function reconciliationSnapshotIdentity(value) {
        return value && value.requestId ? JSON.stringify([reconciliationSourceToken, value.requestId, value.runEpoch,
            value.generation, value.deploymentId, value.deploymentGeneration]) : ""
    }
    function reconciliationStateText(value) {
        var labels = {reading: qsTr("正在顺序读取"), completed: qsTr("本批读取结束（不是整组通过）"),
            stopped: qsTr("已停止后续读取"), cancelled: qsTr("已取消后续读取"), queued: qsTr("尚未读取"),
            matched: qsTr("当次规范化内容匹配（仅历史）"), different: qsTr("当次完整观察与预期不同"), unknown: qsTr("无法确定是否匹配")}
        return labels[value] || qsTr("未知状态")
    }
    function readbackDecimal(value, allowZero) {
        return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value)
            && (allowZero || value !== "0") && (value.length < 20 || value <= "18446744073709551615")
    }
    function readbackWrapper(row, current) {
        var value = row ? row.readbackDiagnostic : null
        if (!readbackMap(value) || Object.keys(value).length !== 16 || value.schemaVersion !== 1
                || typeof value.deploymentId !== "string" || !value.deploymentId.length || value.deploymentId !== current.deploymentId
                || !readbackDecimal(value.generation, false) || value.generation !== current.generation
                || !integer(value.vehicleId, 1, 255) || value.vehicleId !== row.vehicleId
                || !readbackDecimal(value.writeOperationId, false) || value.writeOperationId !== row.writeOperationId
                || !readbackDecimal(value.readOperationId, false) || value.readOperationId !== row.readOperationId
                || !readbackDecimal(value.observedAtMonotonicMs, true) || !integer(value.observedItemCount, 0, 2147483647)
                || value.observationScope !== "ordinary-decoded-mission-items-history"
                || ["captured", "item-limit-exceeded"].indexOf(value.captureState) < 0 || !readbackMap(value.comparison)
                || value.historicalOnly !== true || value.snapshotOnly !== true || value.currentOnboardContentsProven !== false
                || value.startupPermitGranted !== false || value.flightCommandReleased !== false) return null
        if (value.captureState === "item-limit-exceeded")
            return value.observedItemCount > 2048 && Object.keys(value.comparison).length === 0 ? value : null
        return value.observedItemCount <= 2048 ? value : null
    }
    function readbackIdentity(value) {
        return value ? JSON.stringify([value.deploymentId, value.generation, value.vehicleId, value.writeOperationId, value.readOperationId,
            value.observedAtMonotonicMs, value.captureState]) : ""
    }
    function readbackCanonicalBasis(value) {
        return value === "canonical-v2" || value === "canonical-heading-tick-v3"
    }
    function headingValueAbsent(value) { return value === null || typeof value === "undefined" }
    function headingObservationsKnown(value) {
        if (!readbackMap(value) || Object.keys(value).length !== 5
                || !integer(value.totalCount, 0, 2048) || !integer(value.displayedCount, 0, 64)
                || value.displayedCount !== Math.min(value.totalCount, 64)
                || typeof value.complete !== "boolean" || typeof value.truncated !== "boolean"
                || value.truncated !== (value.totalCount > value.displayedCount)
                || !Array.isArray(value.rows) || value.rows.length !== value.displayedCount) return false
        var previous = -1
        for (var i = 0; i < value.rows.length; ++i) {
            var row = value.rows[i]
            if (!readbackMap(row) || Object.keys(row).length !== 9 || !integer(row.sequenceNumber, 0, 2047)
                    || row.sequenceNumber <= previous || ["finite", "system-default", "missing", "invalid"].indexOf(row.actualState) < 0
                    || typeof row.canonicalKnown !== "boolean" || typeof row.semanticMatch !== "boolean"
                    || row.absoluteErrorBoundDegrees !== 0.0006866455078125
                    || ["", "unsupported_wire_parameter", "heading-observation-missing", "heading-observation-system-default",
                        "heading-expected-unavailable", "heading-semantic-different"].indexOf(row.error) < 0) return false
            var required = ["sequenceNumber", "expectedEncoded", "actualRaw", "actualState", "canonical",
                "canonicalKnown", "absoluteErrorBoundDegrees", "error", "semanticMatch"]
            for (var requiredIndex = 0; requiredIndex < required.length; ++requiredIndex)
                if (!Object.prototype.hasOwnProperty.call(row, required[requiredIndex])) return false
            for (var keyIndex = 0; keyIndex < 3; ++keyIndex) {
                var key = ["expectedEncoded", "actualRaw", "canonical"][keyIndex]
                if (!headingValueAbsent(row[key]) && (typeof row[key] !== "number" || !isFinite(row[key]))) return false
            }
            if ((row.actualState === "finite") !== !headingValueAbsent(row.actualRaw)
                    || row.canonicalKnown !== !headingValueAbsent(row.canonical)
                    || (row.canonicalKnown && (row.actualState !== "finite" || row.canonical < 0 || row.canonical >= 360
                        || Math.floor(row.canonical * 256) !== row.canonical * 256))
                    || (row.semanticMatch && (!row.canonicalKnown || headingValueAbsent(row.expectedEncoded)
                        || row.expectedEncoded !== row.canonical || row.error !== ""))) return false
            previous = row.sequenceNumber
        }
        return true
    }
    function readbackComparison(value) {
        var report = value ? value.comparison : null
        if (!readbackMap(report) || (report.schemaVersion !== 1 && report.schemaVersion !== 2)
                || Object.keys(report).length !== (report.schemaVersion === 2 ? 29 : 28)
                || (report.schemaVersion === 2 && !headingObservationsKnown(report.headingObservations))
                || (report.schemaVersion === 1 && (report.headingObservations !== undefined || report.comparisonBasis === "canonical-heading-tick-v3"))
                || report.format !== "merivus-mission-readback-diff"
                || report.sourceRepresentation !== "ordinary-decoded-mission-items-not-raw-int-proof"
                || ["matched", "different", "unknown"].indexOf(report.state) < 0
                || ["canonical-v2", "canonical-heading-tick-v3", "raw-fields-only", "none"].indexOf(report.comparisonBasis) < 0
                || ["input-item-limit", "canonical-match", "canonical-different", "expected-canonical-invalid", "observed-empty", "observed-canonical-invalid"].indexOf(report.reason) < 0
                || !readbackErrorKnown(report.expectedErrorCode) || !readbackErrorKnown(report.observedErrorCode)
                || typeof report.expectedCanonicalValid !== "boolean" || typeof report.observedCanonicalValid !== "boolean"
                || typeof report.expectedContentHash !== "string" || typeof report.observedContentHash !== "string"
                || !integer(report.expectedItemCount, 0, 2147483647) || report.observedItemCount !== value.observedItemCount
                || !integer(report.expectedErrorItemIndex, -1, 2047) || !integer(report.observedErrorItemIndex, -1, 2047)
                || !integer(report.comparedItemCount, 0, 2048) || !integer(report.uncomparedItemCount, 0, 2147483647)
                || report.comparedItemCount + report.uncomparedItemCount !== Math.max(report.expectedItemCount, report.observedItemCount)
                || !integer(report.differenceCount, 0, 34817) || typeof report.differenceCountComplete !== "boolean"
                || !integer(report.displayedDifferenceCount, 0, 64) || typeof report.rowsTruncated !== "boolean"
                || report.displayedDifferenceCount > report.differenceCount || report.rowsTruncated !== (report.differenceCount > report.displayedDifferenceCount)
                || !Array.isArray(report.rows) || report.rows.length !== report.displayedDifferenceCount
                || report.historicalOnly !== true || report.snapshotOnly !== true || report.currentOnboardContentsProven !== false
                || report.startupPermitGranted !== false || report.flightCommandReleased !== false) return null
        if ((report.expectedContentHash !== "" && !/^[0-9a-f]{64}$/.test(report.expectedContentHash))
                || (report.observedContentHash !== "" && !/^[0-9a-f]{64}$/.test(report.observedContentHash))) return null
        if ((report.expectedContentHash.length > 0) !== report.expectedCanonicalValid
                || (report.observedContentHash.length > 0) !== report.observedCanonicalValid) return null
        if (report.differenceCountComplete && (!report.expectedCanonicalValid || !report.observedCanonicalValid || report.uncomparedItemCount !== 0)) return null
        if (report.state === "matched" && (!report.expectedCanonicalValid || !report.observedCanonicalValid
                || !readbackCanonicalBasis(report.comparisonBasis) || !report.expectedContentHash.length
                || report.expectedContentHash !== report.observedContentHash || report.differenceCount !== 0)) return null
        return report
    }
    function readbackErrorKnown(value) {
        return ["", "wire_resource_limit", "invalid_wire_item", "unsupported_wire_command", "invalid_wire_parameter",
            "unsupported_wire_parameter", "invalid_wire_coordinate", "diagnostic-canonical-unavailable"].indexOf(value) >= 0
    }
    function readbackSummary(value, report) {
        if (!value) return qsTr("普通回读诊断：未取得可绑定到本次部署/读写操作的诊断，或诊断格式无效；不表示没有差异。")
        if (value.captureState === "item-limit-exceeded") return qsTr("普通回读诊断：实际回读 %1 项，超过 2048 项比较上限；未比较内容，不能将空明细视为相同。").arg(value.observedItemCount)
        if (!report) return qsTr("普通回读诊断：捕获了历史回读，但比较报告缺失或格式不受支持；结果未知，不根据空行判定一致。")
        var state = report.state === "matched" ? qsTr("当次规范化内容相同（仅历史）")
            : report.state === "different" ? (readbackCanonicalBasis(report.comparisonBasis) ? qsTr("当次规范化内容不同（仅历史）")
                : qsTr("当次观测不同，但未完成双方规范化（仅历史）")) : qsTr("比较结论未知，不能判定相同")
        return qsTr("普通回读诊断：%1\n预期 / 实际项数：%2 / %3 · 已比较 / 未比较：%4 / %5\n发现差异：%6 · 差异计数完整：%7 · 展示：%8 / 最多64 · 明细截断：%9\n原因：%10；依据：%11。")
            .arg(state).arg(report.expectedItemCount).arg(report.observedItemCount).arg(report.comparedItemCount).arg(report.uncomparedItemCount)
            .arg(report.differenceCount).arg(known(report.differenceCountComplete)).arg(report.displayedDifferenceCount).arg(known(report.rowsTruncated))
            .arg(report.reason || qsTr("未给出")).arg(report.comparisonBasis) + headingReadbackText(report)
    }
    function headingReadbackText(report) {
        if (report.schemaVersion !== 2) return ""
        var observation = report.headingObservations
        if (!headingObservationsKnown(observation)) return qsTr("\n朝向原始回读诊断无效；不能推断匹配。")
        var lines = [qsTr("\n朝向语义比较，不是原字节相同或实际姿态到位。候选 %1 项，显示 %2，截断：%3，候选计数完整：%4。")
            .arg(observation.totalCount).arg(observation.displayedCount).arg(known(observation.truncated)).arg(known(observation.complete))]
        for (var i = 0; i < observation.rows.length; ++i) {
            var row = observation.rows[i]
            lines.push(qsTr("序号 %1：上传编码 %2°；实际原值 %3°（%4）；语义代表 %5°；舍入界 %6°；语义匹配 %7；原因 %8。")
                .arg(row.sequenceNumber).arg(headingValueAbsent(row.expectedEncoded) ? qsTr("未知") : String(row.expectedEncoded))
                .arg(headingValueAbsent(row.actualRaw) ? qsTr("未提供有限原值") : String(row.actualRaw)).arg(row.actualState)
                .arg(row.canonicalKnown ? String(row.canonical) : qsTr("未知"))
                .arg(row.absoluteErrorBoundDegrees).arg(known(row.semanticMatch)).arg(row.error || qsTr("无")))
        }
        return "\n" + lines.join("\n")
    }
    function readbackStateText(value) {
        var labels = {present: qsTr("存在"), missing: qsTr("missing：字段缺失"), "null": qsTr("null：显式空值"),
            "typed-null": qsTr("typed-null：类型化空值"), "invalid-number": qsTr("invalid-number：非有限或超出安全显示范围"),
            "unsupported-value": qsTr("unsupported-value：不支持的复杂值"), "item-missing": qsTr("item-missing：整项缺失"),
            "item-invalid": qsTr("item-invalid：任务项无效")}
        return typeof value === "string" && Object.prototype.hasOwnProperty.call(labels, value) ? labels[value] : qsTr("未知状态")
    }
    function readbackValueText(value, state) {
        if (state !== "present") return qsTr("未提供可比较标量（不补零、空字符串或 false）")
        if (value === null || typeof value === "undefined") return qsTr("该项存在但投影未给出标量（不是空字符串，也不表示整项缺失）")
        if (typeof value === "string") return JSON.stringify(value)
        if (typeof value === "boolean" || (typeof value === "number" && isFinite(value))) return String(value)
        return qsTr("未知 / 非标量")
    }
    function readbackScalarKnown(value, field) {
        return value === null || typeof value === "undefined" || typeof value === "boolean"
            || (typeof value === "number" && isFinite(value) && Math.abs(value) <= 9007199254740991)
            || (field === "yawMode" && ["system-default", "absolute-degrees", "not-applicable", "unsupported-command"].indexOf(value) >= 0)
    }
    function readbackRowText(row) {
        var states = ["present", "missing", "null", "typed-null", "invalid-number", "unsupported-value", "item-missing", "item-invalid"]
        var types = ["invalid", "bool", "int", "uint", "int64", "uint64", "double", "string", "map", "list", "other"]
        if (!readbackMap(row) || Object.keys(row).length !== 9 || !integer(row.itemIndex, -1, 2047)
                || ["itemCount", "item", "sequenceNumber", "command", "frame", "autoContinue", "isCurrentItem",
                    "param1", "param2", "param3", "param4", "param5", "param6", "param7", "yawMode", "latitudeE7", "longitudeE7", "param5Integer", "param6Integer"].indexOf(row.field) < 0
                || ["count", "item-presence", "state", "type", "value"].indexOf(row.differenceKind) < 0
                || (row.itemIndex === -1) !== (row.field === "itemCount")
                || (row.differenceKind === "count") !== (row.field === "itemCount")
                || states.indexOf(row.expectedState) < 0 || states.indexOf(row.observedState) < 0
                || types.indexOf(row.expectedType) < 0 || types.indexOf(row.observedType) < 0
                || !readbackScalarKnown(row.expectedValue, row.field) || !readbackScalarKnown(row.observedValue, row.field))
            return qsTr("这条诊断字段格式未知；不推断一致，也不补造值。")
        var location = row.itemIndex === -1 ? qsTr("整表项数（不是任务项序号）")
            : qsTr("比较位置 itemIndex=%1（0基，对应列表第%2项；实际协议序号看 sequenceNumber 字段）").arg(row.itemIndex).arg(row.itemIndex + 1)
        return qsTr("%1 · 字段：%2 · 差异类型：%3\n预期：%4 · 原类型：%5 · 值：%6\n实际：%7 · 原类型：%8 · 值：%9")
            .arg(location).arg(row.field).arg(row.differenceKind)
            .arg(readbackStateText(row.expectedState)).arg(text(row.expectedType)).arg(readbackValueText(row.expectedValue, row.expectedState))
            .arg(readbackStateText(row.observedState)).arg(text(row.observedType)).arg(readbackValueText(row.observedValue, row.observedState))
    }
    function policyNumber(value) {
        // Display only the frozen native numeric value: no null/false -> 0,
        // rounding, guessed default, percentage, or local authorization.
        return typeof value === "number" && isFinite(value) ? String(value) : qsTr("未知")
    }
    function startupPolicySummary(row) {
        row = row || {}
        var policy = row.startupTransitionPolicy || {}
        var schema = policy.schemaVersion
        var heading = policy.headingEnvelope || {}, local = policy.localMotionEnvelope || {}
        var motion = policy.postModeEnvelope || {}, home = policy.homeUpdateEnvelope || {}
        var altitude = policy.altitudeReferenceEnvelope || {}
        var summary = qsTr("ID %1 · 本次冻结的启动观测政策（只读）\n").arg(text(row.vehicleId))
        if (!integer(schema, 1, 4))
            return summary + qsTr("政策缺失或无效；没有默认允许值。请修改计划后重新归档、读取和审阅，不在此界面补值或放行。")
        summary += qsTr("版本 %1 · 最大原始报文间隙 %2 ms · 每阶段最长 %3 ms\n")
            .arg(policyNumber(schema)).arg(policyNumber(policy.maximumRawGapMs)).arg(policyNumber(policy.maximumPhaseDurationMs))
        summary += qsTr("模式切换后：AMSL高度 %1 至 %2 m；航线横向走廊 %3 m；相对高度绝对值上限 %4 m\n水平 / 上升 / 下降速度上限：%5 / %6 / %7 m/s\n")
            .arg(policyNumber(motion.minAltitudeAmslMeters)).arg(policyNumber(motion.maxAltitudeAmslMeters))
            .arg(policyNumber(motion.horizontalCorridorMeters)).arg(policyNumber(motion.maxRelativeAltitudeMagnitudeMeters))
            .arg(policyNumber(motion.maxHorizontalSpeedMetersPerSecond)).arg(policyNumber(motion.maxClimbSpeedMetersPerSecond))
            .arg(policyNumber(motion.maxDescentSpeedMetersPerSecond))
        if (schema === 1) {
            summary += qsTr("局部 XYZ 和航向保持原审阅值；HOME 改变则停止并重新地面审阅。\n")
        } else {
            summary += qsTr("局部观察参考：%1\n相对原局部基线的水平 / 上升 / 下降位移上限：%2 / %3 / %4 m\n局部水平 / 上升 / 下降速度上限：%5 / %6 / %7 m/s\n")
                .arg(text(local.referenceBasis)).arg(policyNumber(local.maxHorizontalDisplacementMeters))
                .arg(policyNumber(local.maxClimbDisplacementMeters)).arg(policyNumber(local.maxDescentDisplacementMeters))
                .arg(policyNumber(local.maxHorizontalSpeedMetersPerSecond)).arg(policyNumber(local.maxClimbSpeedMetersPerSecond))
                .arg(policyNumber(local.maxDescentSpeedMetersPerSecond))
            summary += qsTr("航向偏离 / 相邻变化 / 全过程累计观察转角上限：%1 / %2 / %3 °\n")
                .arg(policyNumber(heading.maxDeviationDegrees)).arg(policyNumber(heading.maxAdjacentChangeDegrees))
                .arg(policyNumber(heading.maxObservedTravelDegrees))
            if (schema === 2 || schema === 4) summary += qsTr("HOME 改变则停止并重新地面审阅。\n")
        }
        if (schema === 3) {
            summary += qsTr("HOME 处理：%1\nHOME 参考：%2\n最多 %3 次完整 HOME 观测更新；普通解锁后、切换模式前最多 %4 次仅相对高度变化\n相对原 HOME 的地理水平 / AMSL高度位移上限：%5 / %6 m\n相对原 HOME 的局部水平 / 垂直位移上限：%7 / %8 m\n切换模式前相对原 GP 的相对高度变化上限：%9 m\n")
                .arg(text(policy.homeChangeHandling)).arg(text(home.referenceBasis))
                .arg(policyNumber(home.maximumHomeUpdates)).arg(policyNumber(home.maximumPreModeRelativeAltitudeChanges))
                .arg(policyNumber(home.maxHorizontalDisplacementMeters)).arg(policyNumber(home.maxAltitudeDisplacementMeters))
                .arg(policyNumber(home.maxLocalHorizontalDisplacementMeters)).arg(policyNumber(home.maxLocalVerticalDisplacementMeters))
                .arg(policyNumber(home.maxPreModeRelativeAltitudeChangeMeters))
            summary += qsTr("以上次数跨步骤累计，不刷新原始参考；0 m 表示严格零变化。HOME 时间戳更新也计次。仅在本次真实排队的普通解锁或模式步骤内核对；不允许切换模式前的局部 XYZ、全球位置、AMSL、速度或航向变化。\n")
        }
        if (schema === 4) {
            summary += qsTr("高度参考声明：%1\n原 ALTITUDE 同包差值相对首次审阅值的变化上限（AMSL-local / AMSL-relative / relative-local）：%2 / %3 / %4 m\n差值对原 origin 海拔 / HOME 海拔 / HOME localZ 的残差上限：%5 / %6 / %7 m\n")
                .arg(text(altitude.referenceBasis)).arg(policyNumber(altitude.maxAmslMinusLocalDriftMeters))
                .arg(policyNumber(altitude.maxAmslMinusRelativeDriftMeters)).arg(policyNumber(altitude.maxRelativeMinusLocalDriftMeters))
                .arg(policyNumber(altitude.maxAmslMinusLocalToOriginResidualMeters)).arg(policyNumber(altitude.maxAmslMinusRelativeToHomeResidualMeters))
                .arg(policyNumber(altitude.maxRelativeMinusLocalToHomeLocalZResidualMeters))
            summary += qsTr("0 m 是明确零容差；未知不当 0。原样本不被最新值替换；实际启动 scope 内已观察越界、坏帧、失源或断档不能由恢复正常自行清除。scope 建立前的审阅等待期没有连续覆盖承诺。\n")
        }
        return summary + qsTr("这些只是观测限制，不证明 HOME / 局部原点有效、不同报文同采样、请求因果、净空或最终启动许可。修改政策必须重新规划、归档、读取、审阅及审批；不能只凭相同航点载荷沿用旧批准。")
    }
    function encodedTargets() {
        var payloads = boundedList(encoding.members, 32)
        if (!encoding.members || payloads.length !== encoding.members.length) return []
        var ids = [], seen = {}, total = 0
        for (var i = 0; i < payloads.length; ++i) {
            var payload = payloads[i] || {}
            if (!integer(payload.vehicleId, 1, 255) || seen[String(payload.vehicleId)]
                    || !integer(payload.itemCount, 1, 2048)) return []
            seen[String(payload.vehicleId)] = true
            ids.push(payload.vehicleId)
            total += payload.itemCount
        }
        return total === encoding.totalItemCount ? ids : []
    }
    function reviewRefreshTargets() {
        if (!sourceKindKnown) return []
        var rows = boundedList(refreshVehicleIds, 32), ids = [], seen = ({})
        if (!refreshVehicleIds || rows.length !== refreshVehicleIds.length) return []
        for (var i = 0; i < rows.length; ++i) {
            var id = rows[i]
            if (!integer(id, 1, 255) || seen[String(id)]) return []
            seen[String(id)] = true; ids.push(id)
        }
        return ids
    }
    function refreshActorMatches(value) {
        if (!value || value.action !== "mission.evidence.refresh") return false
        var keys = ["actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision", "actorAuthenticated", "executionPermitted"]
        for (var i = 0; i < keys.length; ++i) if (value[keys[i]] !== refreshAuthorization[keys[i]]) return false
        return value.allowed === true
    }
    function contentActorMatches(value) {
        if (!value || value.action !== "mission.start.content.read" || value.allowed !== true
                || value.requiresSecondApproval !== false || value.actorAuthenticated !== false || value.executionPermitted !== false) return false
        var keys = ["actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision", "actorAuthenticated", "executionPermitted"]
        for (var i = 0; i < keys.length; ++i) if (value[keys[i]] !== contentAuthorization[keys[i]]) return false
        return true
    }
    function contentBindingMatches(value, expected) {
        if (!value || !expected) return false
        var keys = ["sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash", "storeEpoch"]
        for (var i = 0; i < keys.length; ++i)
            if (value[keys[i]] === undefined || value[keys[i]] !== expected[keys[i]]) return false
        return true
    }
    function deploymentReadTargets() {
        var rows = deployment.vehicles || [], ids = [], seen = {}, total = 0
        if (!deployment.deploymentId || rows.length < 1 || rows.length > 32) return []
        for (var i = 0; i < rows.length; ++i) {
            var row = rows[i] || {}
            if (!integer(row.vehicleId, 1, 255) || seen[String(row.vehicleId)] || !integer(row.itemCount, 1, 2048)) return []
            seen[String(row.vehicleId)] = true; ids.push(row.vehicleId); total += row.itemCount
        }
        return total <= 8192 ? ids : []
    }
    function contentStateLabel(state) {
        var labels = {empty: qsTr("尚未申请"), queued: qsTr("等待读取"), "reading-content": qsTr("正在逐机读取"),
            "awaiting-content": qsTr("等待完整原始任务"), "content-matched": qsTr("历史整表匹配（仍须核有效期）"),
            "matched-stale": qsTr("整表已匹配，但最早读取已过期"),
            failed: qsTr("失败，已停止后续读取"), stale: qsTr("来源或有效期已失效"), canceled: qsTr("已停止后续读取")}
        return labels[state] || qsTr("未知状态：%1").arg(text(state, 64))
    }
    function refreshStateLabel(state) {
        var labels = {empty: qsTr("尚未申请"), queued: qsTr("尚未读取"), refreshing: qsTr("正在只读采集"),
            "awaiting-version-receipt": qsTr("等待新版本观测"), "refreshing-parameters": qsTr("准备读取固定参数"),
            "awaiting-parameter-receipt": qsTr("等待新参数观测"), "observations-renewed": qsTr("本批历史观测采集完成"),
            failed: qsTr("失败，已停止后续发送"), stale: qsTr("上下文失效，已停止"), canceled: qsTr("已取消后续读取")}
        return labels[state] || qsTr("未知状态：%1").arg(text(state, 64))
    }
    function bindingMatches(value) {
        return value && value.sourceKind === sourceKind && value.contextToken === contextToken
            && value.sourceIdentity === displayedSourceIdentity
            && value.recordId === archive.recordId && value.recordRevision === archive.recordRevision && value.planId === archive.planId
    }
    function freezeAuthorization(value) {
        if (!value || typeof value.actorId !== "string" || !value.actorId.length || value.actorId.length > 128
                || typeof value.role !== "string" || !value.role.length || value.role.length > 64 || value.action !== "mission.upload"
                || (value.allowed !== true && value.allowed !== false) || (value.requiresSecondApproval !== true && value.requiresSecondApproval !== false)
                || typeof value.policyEpoch !== "string" || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value.policyEpoch)
                || typeof value.policyRevision !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.policyRevision)
                || value.actorAuthenticated !== false || value.executionPermitted !== false) return null
        return {actorId: value.actorId, role: value.role, action: value.action, allowed: value.allowed,
                requiresSecondApproval: value.requiresSecondApproval, policyEpoch: value.policyEpoch,
                policyRevision: value.policyRevision, actorAuthenticated: false, executionPermitted: false}
    }
    function authorizationMatches(value) {
        if (!value) return false
        var keys = ["actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision", "actorAuthenticated", "executionPermitted"]
        for (var i = 0; i < keys.length; ++i) if (value[keys[i]] !== authorization[keys[i]]) return false
        return true
    }
    function freezeIdentity(value) {
        if (!value || value.schemaVersion !== 1 || value.authenticated !== true
                || value.authenticationSource !== "windows-logon-local-role-groups"
                || typeof value.actorId !== "string" || !/^S-1-[0-9-]{1,120}$/.test(value.actorId)
                || typeof value.role !== "string" || ["observer", "planner", "operator", "safety_officer", "maintenance", "commander", "administrator"].indexOf(value.role) < 0
                || typeof value.sessionRevision !== "string" || !/^[1-9][0-9]{0,19}$/.test(value.sessionRevision)
                || typeof value.displayName !== "string" || value.displayName.length > 256
                || !integer(value.issuedAtMonotonicMs, 0, 9007199254740991)
                || !integer(value.expiresAtMonotonicMs, value.issuedAtMonotonicMs + 1, 9007199254740991)) return null
        for (var i = 0; i < 2; ++i) {
            var field = ["sessionId", "sessionEpoch"][i]
            if (typeof value[field] !== "string" || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value[field])) return null
        }
        return JSON.parse(JSON.stringify(value))
    }
    function freezeApproval(value) {
        if (!value || value.schemaVersion !== 2 || value.action !== "mission.upload" || value.oneUse !== true
                || value.startupPermitGranted !== false || value.flightCommandReleased !== false
                || value.uploadLeaseMaximumMs !== 240000 || value.uploadLeaseRequiresCurrentOperator !== true
                || typeof value.approvalId !== "string" || !value.approvalId.length || value.approvalId.length > 64
                || typeof value.approverId !== "string" || !value.approverId.length || value.approverId === value.actorId
                || !value.binding || typeof value.binding.recordId !== "string" || !integer(value.binding.recordRevision, 1, 9007199254740991)
                || typeof value.bindingHash !== "string" || !/^[0-9a-f]{64}$/.test(value.bindingHash)) return null
        return JSON.parse(JSON.stringify(value))
    }
    function securityMatches(value) {
        if (!value || !identityMatches(value.identityReceipt) || !freezeApproval(value.approvalReceipt)) return false
        var approvalFields = ["schemaVersion", "approvalId", "approvalEpoch", "approvalGeneration", "action", "bindingHash", "actorId",
            "applicantSessionId", "applicantSessionEpoch", "applicantSessionRevision", "approverId", "approverRole",
            "rolePolicyEpoch", "rolePolicyRevision", "approvedAtMonotonicMs", "expiresAtMonotonicMs", "oneUse", "startupPermitGranted", "flightCommandReleased",
            "uploadLeaseMaximumMs", "uploadLeaseRequiresCurrentOperator"]
        for (var a = 0; a < approvalFields.length; ++a)
            if (value.approvalReceipt[approvalFields[a]] !== secondApproval[approvalFields[a]]) return false
        var bindingFields = ["sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash", "storeEpoch", "inputGeneration", "reviewGeneration"]
        for (var b = 0; b < bindingFields.length; ++b)
            if (value.approvalReceipt.binding[bindingFields[b]] !== secondApproval.binding[bindingFields[b]]) return false
        return true
    }
    function identityMatches(value) {
        if (!freezeIdentity(value)) return false
        var identityFields = ["schemaVersion", "sessionId", "sessionEpoch", "sessionRevision", "actorId", "displayName", "role",
            "authenticationSource", "authenticated", "issuedAtMonotonicMs", "expiresAtMonotonicMs"]
        for (var i = 0; i < identityFields.length; ++i)
            if (value[identityFields[i]] !== identity[identityFields[i]]) return false
        return true
    }
    function archiveBindingMatches(value) {
        if (!value) return false
        var fields = ["sourceTaskId", "recordId", "recordRevision", "planId", "contentHash", "reviewEvidenceHash", "storeEpoch"]
        for (var i = 0; i < fields.length; ++i) if (value[fields[i]] !== archive[fields[i]]) return false
        return true
    }
    function bindingSnapshot() {
        return {recordId: archive.recordId, recordRevision: archive.recordRevision, planId: archive.planId,
                sourceKind: sourceKind, contextToken: contextToken, sourceIdentity: displayedSourceIdentity,
                actorAuthorization: freezeAuthorization(authorization), identityReceipt: freezeIdentity(identity), approvalReceipt: freezeApproval(secondApproval)}
    }
    function fail(message) {
        localError = text(message)
        localNotice = ""
        feedback.forceActiveFocus()
        return false
    }
    function cancelReceipt(owner, requestId) {
        return owner && typeof owner.cancelFleetMissionUploadRequest === "function"
            ? owner.cancelFleetMissionUploadRequest(requestId) === true : false
    }
    function freezeRequest(value) {
        if (!value || value.ok !== true || typeof value.requestId !== "string" || !value.requestId.length || value.requestId.length > 256
                || typeof value.recordId !== "string" || !value.recordId.length || value.recordId.length > 64
                || !integer(value.recordRevision, 1, 9007199254740991) || typeof value.planId !== "string" || !/^[0-9a-f]{64}$/.test(value.planId)
                || !integer(value.createdAtMs, 1, 9007199254740991) || !integer(value.expiresAtMs, value.createdAtMs, 9007199254740991)
                || !integer(value.totalItemCount, 1, 8192) || value.replacesOnboardMission !== true
                || value.requiresOperatorConfirmation !== true || value.startsMission !== false || value.armsVehicle !== false
                || value.flightCommandReleased !== false) return null
        var ids = boundedList(value.targetVehicleIds, 32), seen = {}
        if (!value.targetVehicleIds || !ids.length || ids.length !== value.targetVehicleIds.length) return null
        for (var i = 0; i < ids.length; ++i) {
            if (!integer(ids[i], 1, 255) || seen[String(ids[i])]) return null
            seen[String(ids[i])] = true
        }
        var actor = freezeAuthorization(value.actorAuthorization)
        var operator = freezeIdentity(value.identityReceipt), approval = freezeApproval(value.approvalReceipt)
        if (!actor || actor.allowed !== true || !operator || !approval
                || actor.actorId !== operator.actorId || actor.role !== operator.role
                || approval.actorId !== operator.actorId || approval.approverId === operator.actorId
                || approval.applicantSessionId !== operator.sessionId || approval.applicantSessionEpoch !== operator.sessionEpoch
                || approval.applicantSessionRevision !== operator.sessionRevision
                || approval.rolePolicyEpoch !== actor.policyEpoch || approval.rolePolicyRevision !== actor.policyRevision
                || approval.binding.recordId !== value.recordId || approval.binding.recordRevision !== value.recordRevision
                || approval.binding.planId !== value.planId) return null
        return {requestId: value.requestId, recordId: value.recordId, recordRevision: value.recordRevision,
                planId: value.planId, createdAtMs: value.createdAtMs, expiresAtMs: value.expiresAtMs,
                expiresAtUtc: typeof value.expiresAtUtc === "string" ? value.expiresAtUtc.substring(0, 64) : "",
                targetVehicleIds: ids, totalItemCount: value.totalItemCount, actorAuthorization: actor,
                identityReceipt: operator, approvalReceipt: approval}
    }
    function requestUpload(pressedBinding, releasedCurrent) {
        if (!releasedCurrent || !canRequest || !bindingMatches(pressedBinding) || !authorizationMatches(pressedBinding.actorAuthorization) || !securityMatches(pressedBinding))
            return fail(qsTr("按下后版本、操作者或审批已变化，请重新检查计划。"))
        var owner = controller, response = {}, frozen = null
        dispatchBusy = true
        try { response = owner.requestFleetMissionUpload(pressedBinding.recordId) || {} }
        catch (error) { response = {error: qsTr("上传确认服务异常；未获得可用请求。")} }
        dispatchBusy = false
        frozen = freezeRequest(response)
        if (frozen) {
            frozen.sourceKind = pressedBinding.sourceKind
            frozen.contextToken = pressedBinding.contextToken
            frozen.sourceIdentity = pressedBinding.sourceIdentity
        }
        if (!frozen || owner !== controller || !canRequest || !bindingMatches(frozen) || !authorizationMatches(frozen.actorAuthorization) || !securityMatches(frozen)
                || frozen.totalItemCount !== encoding.totalItemCount || frozen.targetVehicleIds.join(",") !== previewTargets.join(",")) {
            if (response.requestId) cancelReceipt(owner, response.requestId)
            return fail(qsTr("未生成当前版本上传确认：%1。请重新审阅，不能沿用旧请求。").arg(text(response.error || qsTr("冻结数据或上下文已变化"))))
        }
        clockMs = Date.now()
        pendingController = owner
        pendingRequest = frozen
        localError = ""
        localNotice = ""
        if (!requestCurrent) { cancelPending(qsTr("上传确认已过期或版本已变化。")); return false }
        var dialog = confirmationComponent.createObject(mainWindow, {ownerPanel: root, requestController: owner, frozenRequest: frozen})
        if (!dialog) { cancelPending(qsTr("无法打开二次确认；请求已取消。")); return false }
        if (!requestCurrent || pendingRequest.requestId !== frozen.requestId || owner !== controller) {
            cancelReceipt(owner, frozen.requestId)
            dialog.destroy()
            if (pendingRequest.requestId === frozen.requestId) cancelPending("")
            return fail(qsTr("打开确认时来源已变化；旧请求已取消，请重新申请。"))
        }
        confirmationDialog = dialog
        dialog.open()
        return true
    }
    function finishDialog(dialog, requestId, owner, consumed) {
        if (!consumed) cancelReceipt(owner, requestId)
        if (pendingRequest.requestId === requestId) { pendingRequest = ({}); pendingController = null }
        if (confirmationDialog === dialog) confirmationDialog = null
        if (visible) {
            if (localError.length > 0) feedback.forceActiveFocus()
            else uploadButton.forceActiveFocus()
        }
    }
    function cancelPending(reason) {
        var request = pendingRequest, owner = pendingController, dialog = confirmationDialog
        pendingRequest = ({}); pendingController = null; confirmationDialog = null
        if (request.requestId) cancelReceipt(owner, request.requestId)
        if (dialog) dialog.close()
        if (reason) fail(reason)
    }
    function submit(dialog, frozen, owner, pressedRequestId, releasedCurrent) {
        clockMs = Date.now()
        if (!releasedCurrent || !requestCurrent || dispatchBusy || owner !== controller
                || confirmationDialog !== dialog || pendingRequest.requestId !== frozen.requestId || pressedRequestId !== frozen.requestId)
            return fail(qsTr("确认已失效；不会上传新版本或替换该请求，请取消后重新申请。"))
        dispatchBusy = true
        dialog.consumed = true
        pendingRequest = ({}); pendingController = null; confirmationDialog = null
        var result = {}
        try { result = owner.confirmFleetMissionUpload(frozen.requestId, true) || {} }
        catch (error) { cancelReceipt(owner, frozen.requestId); result = {error: qsTr("确认调用异常，请核对传输记录；不能假定机载任务未改变。")} }
        dispatchBusy = false
        dialog.close()
        if (result.ok !== true) return fail(qsTr("上传未获确认：%1。查看逐机状态后重新审阅，不自动重试。").arg(text(result.errorCode || result.error)))
        localError = ""
        localNotice = qsTr("已提交上传 / 回读流程；没有解锁或启动。下面显示的匹配仅针对机载任务内容。")
        return true
    }
    function stateLabel(state) {
        var labels = {waiting: qsTr("等待"), preparing: qsTr("准备传输"), uploading: qsTr("上传中"),
            "upload-acknowledged": qsTr("收到上传 ACK"), readback: qsTr("回读核对中"),
            verified: qsTr("内容回读匹配（非执行验证）"), failed: qsTr("失败 / 已停止后续步骤"),
            cancelled: qsTr("已请求停止"), "transport-pending": qsTr("原传输仍在排空")}
        return labels[state] || qsTr("未知状态：%1").arg(text(state, 64))
    }
    function stopUpload(pressedId) {
        if (!controller || dispatchBusy || pressedId !== deployment.deploymentId || (!transportBusy && deployment.state !== "verified"))
            return fail(qsTr("传输身份或状态已变化，请重新查看。"))
        dispatchBusy = true
        var accepted = false
        try { accepted = controller.cancelFleetMissionUpload(pressedId, "operator_stopped_from_deployment_panel") === true }
        catch (error) { accepted = false }
        dispatchBusy = false
        if (!accepted) return fail(qsTr("未接受停止请求；机载任务是否已改变请按逐机记录核对。"))
        localError = ""
        localNotice = qsTr("已请求停止后续步骤；已写入任务不回滚，正在进行的原传输可能仍需排空。")
        return true
    }
    function inspect() {
        if (!controller || dispatchBusy || transportBusy || contentBusy || reconciliationBusy || refreshActive || !deployment.deploymentId) return fail(qsTr("传输占用 / 排空中或没有部署记录，暂不能只读检查。"))
        var owner = controller, id = deployment.deploymentId, state = deployment.state, result = {}
        dispatchBusy = true
        try { result = owner.inspectFleetMissionExecutionState(id) || {} }
        catch (error) { result = {errorCode: qsTr("只读检查调用异常")} }
        dispatchBusy = false
        if (owner !== controller || id !== deployment.deploymentId || state !== deployment.state || result.ok !== true
                || result.snapshotOnly !== true || !result.startReadiness || !result.progressObservation
                || result.startReadiness.deploymentId !== id || result.progressObservation.deploymentId !== id) {
            inspection = ({}); inspectionCapturedMs = -1
            return fail(qsTr("未获得有效只读快照：%1。可能已过期或来源 / 占用变化，请重新检查。").arg(text(result.errorCode || result.error)))
        }
        inspection = result
        inspectionCapturedMs = Date.now(); clockMs = inspectionCapturedMs
        inspectedDeploymentId = id; inspectedState = state
        localError = ""
        localNotice = qsTr("已取得一次性快照；前置条件匹配、缓存匹配或航点序号都不构成启动许可或任务完成证明。")
        return true
    }
    function reasons(values, prefix) {
        var list = boundedList(values, 32), result = []
        for (var i = 0; i < list.length; ++i) result.push(prefix + text(list[i], 256))
        return result
    }
    function observedSequence(value) { return integer(value, 0, 65535) ? String(value) : qsTr("未知 / 无有效序号") }
    function requestStartReview() {
        if (!controller || typeof controller.requestFleetMissionStartReview !== "function" || dispatchBusy
                || transportBusy || contentBusy || reconciliationBusy || refreshActive || !contentFresh || !deployment.deploymentId)
            return fail(qsTr("先完成当前部署整组机载任务核对，并在有效期内生成独立风险审阅；最终来源仍由原生后端复核。"))
        var owner = controller, id = deployment.deploymentId, result = {}
        dispatchBusy = true
        try { result = owner.requestFleetMissionStartReview(id) || {} }
        catch (error) { result = {error: qsTr("独立风险审阅服务异常")} }
        dispatchBusy = false
        if (owner !== controller || id !== deployment.deploymentId || result.ok !== true)
            return fail(qsTr("未生成独立风险审阅：%1。查看来源和权限阻断；不自动重试。").arg(text(result.errorCode || result.error || result.reason)))
        showStartReviewReasons = false
        localError = ""
        localNotice = qsTr("已生成本地独立风险审阅；不是启动授权，此界面不会自动已阅、解锁或开始任务。")
        return true
    }
    onDisplayedSourceIdentityChanged: resetSourceConsent()
    onRequestCurrentChanged: if (!requestCurrent && pendingRequest.requestId) cancelPending(qsTr("上传确认过期，或版本 / 规划来源 / 健康门禁已变化；已取消旧请求。"))
    onControllerChanged: {
        resetRepairConsent()
        if (refreshConsent) refreshConsent.checked = false
        if (contentConsent) contentConsent.checked = false
        if (reconciliationConsent) reconciliationConsent.checked = false
        if (pendingRequest.requestId) cancelPending(qsTr("上传控制器已切换；已取消旧确认。"))
    }
    onAuthorizationChanged: if (pendingRequest.requestId && !authorizationMatches(pendingRequest.actorAuthorization)) cancelPending(qsTr("操作者或角色策略已变化；已取消旧确认。"))
    onIdentityChanged: {
        if (refreshConsent) refreshConsent.checked = false
        if (contentConsent) contentConsent.checked = false
        if (reconciliationConsent) reconciliationConsent.checked = false
        if (pendingRequest.requestId && !securityMatches(pendingRequest)) cancelPending(qsTr("认证身份已变化；已取消旧确认。"))
    }
    onRefreshTargetsChanged: if (refreshConsent) refreshConsent.checked = false
    onRefreshAuthorizationChanged: if (refreshConsent) refreshConsent.checked = false
    onContentAuthorizationChanged: if (contentConsent) contentConsent.checked = false
    onContentTargetsChanged: if (contentConsent) contentConsent.checked = false
    onDeploymentChanged: {
        if (contentConsent) contentConsent.checked = false
        if (reconciliationConsent) reconciliationConsent.checked = false
    }
    onReconciliationAuthorizationChanged: if (reconciliationConsent) reconciliationConsent.checked = false
    onReconciliationSourceTokenChanged: {
        if (reconciliationConsent) reconciliationConsent.checked = false
        resetRepairConsent()
    }
    onContentCheckChanged: { contentObservedMs = Date.now(); clockMs = contentObservedMs }
    onUploadApprovalChanged: if (pendingRequest.requestId && (!hasSecondApproval || !securityMatches(pendingRequest))) cancelPending(qsTr("独立审批已变化或到期；已取消旧确认。"))
    onVisibleChanged: if (!visible) {
        resetRepairConsent()
        if (pendingRequest.requestId) cancelPending("")
    }
    Component.onDestruction: { reconciliationLifetime.alive = false; cancelPending("") }
    Timer { interval: 250; repeat: true; running: root.visible && (!!root.pendingRequest.requestId || root.inspectionPresent || !!root.contentCheck.checkId); onTriggered: root.clockMs = Date.now() }

    ColumnLayout {
        id: content
        width: root.width
        spacing: root.pad
        QGCLabel { Layout.fillWidth: true; text: qsTr("6 · 地面上传与只读检查"); font.bold: true; wrapMode: Text.WordWrap }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.sourceKind === "reserve-dispatch"
                ? qsTr("当前上传来源：已明确采用的独立备用出勤新任务。不会自动切回区域草稿，也不接管原任务。%1")
                    .arg(root.previewCurrent ? qsTr("仍须本次人工计划批准、独立账号审批和覆盖确认。") : qsTr("当前来源已失效；请回备用出勤区重新审阅／采用，或明确退出此来源。"))
                : root.sourceKind === "remainder"
                ? qsTr("当前上传来源：已明确接纳的运行余段计划。即使该记录过期，也不会自动切回区域草稿。%1")
                    .arg(root.previewCurrent ? qsTr("仍须本次人工计划批准、独立账号审批和覆盖确认。") : qsTr("当前来源已失效或条件未完整；请回余段面板重新审阅 / 接纳，或明确退出余段来源。"))
                : root.sourceKind === "draft" ? qsTr("当前上传来源：区域规划草稿的审阅记录。不会使用余段面板尚未接纳的提案。")
                : qsTr("当前规划来源未知，上传保持禁用。")
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: root.sourceKind === "remainder" || root.sourceKind === "reserve-dispatch"
            textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: root.sourceKind === "reserve-dispatch"
                ? qsTr("请在备用出勤区核对完整新出航／返程、执行配置，并展开本记录的逐项协议编码与能量／风险明细；下方再次核对实际目标与编码项总数。这是独立新任务，不是备用自动激活。编码是本地计划值，不是已上传或机载回读；地图／文字只显示规划快照，不证明净空、接管或实际执行。过期记录仅供参考。")
                : qsTr("请在余段区核对本记录的出航 / 返程点和编码项，并在规划地图的只读核对模式查看同一记录。地图只展示二维规划几何，不证明高度净空、避障或机载实际执行；过期记录仅作历史参考。")
        }
        QGCLabel {
            Layout.fillWidth: true
            text: qsTr("危险操作：上传会覆盖目标机载任务。这里只进行地面上传与内容回读，不解锁、不起飞、不启动任务；内容匹配不代表飞行安全。")
            color: qgcPal.warningText
            wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("当前归档记录：%1\n版本：%2\n计划 ID：%3\n目标 ID：%4\n实际编码任务项：%5")
                .arg(root.text(root.archive.recordId)).arg(root.text(root.archive.recordRevision)).arg(root.text(root.archive.planId))
                .arg(root.previewTargets.length ? root.previewTargets.join(", ") : qsTr("未知 / 尚无有效编码预览"))
                .arg(root.integer(root.encoding.totalItemCount, 1, 8192) && root.previewTargets.length ? root.encoding.totalItemCount : qsTr("未知"))
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !root.approvalCurrent; wrapMode: Text.WordWrap
            text: qsTr("上传暂不可申请：需要当前版本的人工计划审批、有效编码与原生来源门禁，并先保存或放弃所有输入。请使用上面的审阅流程。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("本地操作者 SID：%1 · 角色：%2\n上传角色权限：%3 · 独立额外审批要求：%4\n角色策略不是认证：下面的原生系统身份与独立审批是另外两项门禁。此处不能填写身份或切换角色放行。")
                .arg(root.authorization.actorId ? root.text(root.authorization.actorId, 128) : qsTr("未设置，默认禁止上传"))
                .arg(root.text(root.authorization.role, 64)).arg(root.known(root.authorization.allowed)).arg(root.known(root.authorization.requiresSecondApproval))
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !root.hasUploadPermission; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: !root.hasAuthenticatedIdentity
                ? qsTr("缺少有效系统认证。请显式认证，角色须由工程师配置的专用 Windows 本地组分配；未配置、非 Windows 或不支持凭据类型保持禁止。")
                : !root.hasSecondApproval ? qsTr("缺少同一任务的独立账号审批，或审批已过期 / 撤销。请由不同的授权账号审批后，再单独确认覆盖上传；审批不是起飞许可。")
                : qsTr("当前角色没有上传权限。不能用确认框、管理员用户名或任意角色文本绕过。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("系统身份状态：%1 · 原因：%2\n账号：%3 · 来源：%4\n独立审批状态：%5 · 审批账号 SID：%6\n审批 ID：%7 · 快照捕获时剩余毫秒：%8（非实时倒计时）\n两个不同账号不等于已经证明两名不同人员；密码不进入地面站面板或日志。")
                .arg(root.text(root.identityState.state, 64)).arg(root.text(root.identityState.reason, 128))
                .arg(root.text(root.identity.displayName, 256)).arg(root.text(root.identity.authenticationSource, 128))
                .arg(root.text(root.uploadApproval.state, 64)).arg(root.text(root.secondApproval.approverId, 128))
                .arg(root.text(root.secondApproval.approvalId, 64)).arg(root.text(root.uploadApproval.remainingMs))
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("授权期限：申请包含凭据窗口耗时，60 秒内消费一次。消费后，仅本次部署的上传 / 回读租约最多 4 分钟，仍受原操作员会话期限和锁定 / 注销 / 撤权限制；不会自动续期。\n租约部署：%1 · grant：%2\n实际租约长度：%3 ms · 快照剩余：%4 ms（非实时倒计时）\n这不是启动、解锁或跨地面站控制权。")
                .arg(root.text(root.uploadLease.deploymentId, 64)).arg(root.text(root.uploadLease.grantId, 64))
                .arg(root.text(root.uploadLease.durationMs)).arg(root.text(root.uploadLease.remainingMs))
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("使用系统窗口认证操作员（不执行上传）")
            enabled: root.controller && !root.dispatchBusy && !root.transportBusy && !root.pendingRequest.requestId
            onClicked: {
                root.dispatchBusy = true
                var result = {}
                try { result = root.controller.authenticateFleetOperator() || {} }
                catch (error) { result = {reason: qsTr("系统认证调用异常；没有可用认证。")} }
                root.dispatchBusy = false
                if (result.ok !== true) root.fail(qsTr("认证未完成：%1。请检查账号与本地组配置。").arg(root.text(result.reason || result.errorCode)))
                else { root.localError = ""; root.localNotice = qsTr("系统身份已认证；仍需同一任务的独立审批和覆盖确认。") }
            }
        }
        QGCButton {
            id: uploadApprovalButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("为显示的任务申请独立账号审批（不执行上传）")
            enabled: root.approvalCurrent && root.hasAuthenticatedIdentity && root.authorization.allowed === true
                && !root.transportBusy && !root.dispatchBusy && !root.pendingRequest.requestId
            property var pressedBinding: ({})
            onPressed: pressedBinding = root.bindingSnapshot()
            onCanceled: pressedBinding = ({})
            onClicked: {
                if (!root.bindingMatches(pressedBinding) || !root.identityMatches(pressedBinding.identityReceipt)
                        || !root.authorizationMatches(pressedBinding.actorAuthorization))
                    root.fail(qsTr("任务或身份在点击期间变化，请重新申请审批。"))
                else {
                    root.dispatchBusy = true
                    var result = {}
                    var requestedOwner = root.controller, requestedSource = root.displayedSourceIdentity
                    try { result = requestedOwner.requestFleetUploadApproval(pressedBinding.recordId) || {} }
                    catch (error) { result = {reason: qsTr("审批调用异常；没有可用收据。")} }
                    root.dispatchBusy = false
                    if (requestedOwner !== root.controller || requestedSource !== root.displayedSourceIdentity)
                        root.fail(qsTr("独立审批返回时显示来源已变化；请核对原生审批记录，不能将结果用于新来源，也不会自动重试。"))
                    else if (result.ok !== true) root.fail(qsTr("独立审批未完成：%1。不能沿用旧版本或同一账号。").arg(root.text(result.reason || result.errorCode)))
                    else { root.localError = ""; root.localNotice = qsTr("独立审批已记录，60 秒内仅消费一次；消费后的有限租约不会续期。请继续申请覆盖确认。") }
                }
                pressedBinding = ({})
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("撤销此审批（不回滚已写内容）")
            enabled: root.controller && !!root.secondApproval.approvalId && !root.dispatchBusy
            property string pressedId: ""
            onPressed: pressedId = root.secondApproval.approvalId || ""
            onClicked: { if (pressedId) root.controller.revokeFleetUploadApproval(pressedId); pressedId = "" }
            onCanceled: pressedId = ""
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("退出身份并停止后续授权（不回滚）")
            enabled: root.controller && root.hasAuthenticatedIdentity && !root.dispatchBusy
            onClicked: root.controller.logoutFleetOperator()
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("只读证据刷新：目标来自%1，ID %2。只请求原始版本和固定29项参数（原12项、2项总速覆盖、14项起降动作参数、1项高度到点半径）。\n不写参数、不上传、不解锁；会撤销当前审阅与审批，完成后必须重新生成审阅。新回报仅说明本地收到新观测，不证明请求因果、硬件认证、真实起降 / 航点切换轨迹或飞行许可。")
                .arg(root.sourceKind === "reserve-dispatch" ? qsTr("显示的独立备用出勤计划成员")
                    : root.sourceKind === "remainder" ? qsTr("显示的余段计划实际成员") : qsTr("当前区域草稿成员"))
                .arg(root.refreshTargets.length ? root.refreshTargets.join(", ") : qsTr("未选择 / 未知"))
        }
        Controls.CheckBox {
            id: refreshConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            checked: false; focusPolicy: Qt.StrongFocus
            enabled: !root.refreshActive && !root.contentBusy && !root.reconciliationBusy && !root.transportBusy && !root.dispatchBusy
            text: qsTr("同意撤销旧审阅 / 审批，只刷新上述目标的版本及29项参数")
            contentItem: QGCLabel {
                text: refreshConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                leftPadding: (refreshConsent.indicator ? refreshConsent.indicator.width : 0) + root.pad
            }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.refreshBlockedReason
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("刷新版本和固定参数证据（不写入）")
            enabled: root.controller && root.inputSettled && root.refreshTargets.length > 0 && refreshConsent.checked
                && root.hasAuthenticatedIdentity && root.refreshAuthorization.allowed === true
                && !root.refreshActive && !root.contentBusy && !root.reconciliationBusy && !root.transportBusy && !root.dispatchBusy && !root.pendingRequest.requestId
            property var pressedIds: []
            property var pressedIdentity: ({})
            property var pressedActor: ({})
            property var pressedController: null
            property string pressedSourceIdentity: ""
            onPressed: {
                pressedController = root.controller
                pressedSourceIdentity = root.displayedSourceIdentity
                pressedIds = root.refreshTargets.slice(0)
                pressedIdentity = root.freezeIdentity(root.identity)
                pressedActor = JSON.parse(JSON.stringify(root.refreshAuthorization))
            }
            onCanceled: { pressedIds = []; pressedIdentity = ({}); pressedActor = ({}); pressedController = null; pressedSourceIdentity = "" }
            onClicked: {
                if (pressedController !== root.controller || !root.inputSettled || !refreshConsent.checked
                        || pressedSourceIdentity !== root.displayedSourceIdentity
                        || !root.identityMatches(pressedIdentity) || !root.refreshActorMatches(pressedActor)
                        || pressedIds.join(",") !== root.refreshTargets.join(","))
                    root.fail(qsTr("目标或身份已变化，请重新勾选后申请刷新。"))
                else {
                    var requestOwner = pressedController
                    root.dispatchBusy = true
                    var result = {}
                    try { result = requestOwner.requestFleetEvidenceRefresh(pressedIds, true) || {} }
                    catch (error) { result = {reason: qsTr("刷新调用异常；不能假定已有完整新证据。")} }
                    root.dispatchBusy = false
                    if (requestOwner !== root.controller) {
                        if (result.requestId) {
                            try { requestOwner.cancelFleetEvidenceRefresh(result.requestId) }
                            catch (cancelError) { /* Source may already be destroyed; no rollback claim. */ }
                        }
                        root.fail(qsTr("控制器已切换；已尝试停止原请求，请核对原控制器的在途读取。不会恢复旧批准。"))
                    } else if (result.ok !== true) root.fail(qsTr("只读刷新未接受：%1。旧批准不会自动恢复。").arg(root.text(result.reason || result.errorCode)))
                    else { root.localError = ""; root.localNotice = qsTr("已提交有界只读刷新，请查看逐机状态；完成后重新审阅，不自动批准。") }
                }
                refreshConsent.checked = false
                pressedIds = []; pressedIdentity = ({}); pressedActor = ({}); pressedController = null; pressedSourceIdentity = ""
            }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("刷新请求：%1 · 状态：%2\n冻结目标：%3 · 当前机号：%4 · 当前参数：%5\n原因：%6 · 已完成新观测的机数：%7 / %8\n完成不自动恢复计划批准，取消不能撤回已提交的只读请求。")
                .arg(root.text(root.evidenceRefresh.requestId, 64)).arg(root.refreshStateLabel(root.evidenceRefresh.state))
                .arg(root.boundedList(root.evidenceRefresh.vehicleIds, 32).join(", ")).arg(root.text(root.evidenceRefresh.currentVehicleId))
                .arg(root.text(root.evidenceRefresh.currentParameter, 64)).arg(root.text(root.evidenceRefresh.reason, 128))
                .arg(root.text(root.evidenceRefresh.completedVehicleCount)).arg(root.text(root.evidenceRefresh.totalVehicleCount))
        }
        Repeater {
            model: root.boundedList(root.evidenceRefresh.vehicles, 32)
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("证据机号 %1 · 状态 %2 · 原因 %3（仅观测，不是飞行验证）")
                    .arg(root.text(modelData.vehicleId)).arg(root.refreshStateLabel(modelData.state)).arg(root.text(modelData.reason, 128))
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("停止此刷新（不撤回在途请求）")
            enabled: root.refreshActive && root.controller && !root.dispatchBusy
            property string pressedId: ""
            onPressed: pressedId = root.evidenceRefresh.requestId || ""
            onCanceled: pressedId = ""
            onClicked: { if (pressedId) root.controller.cancelFleetEvidenceRefresh(pressedId); pressedId = "" }
        }
        FocusScope {
            id: feedback
            Layout.fillWidth: true
            implicitHeight: feedbackText.implicitHeight + root.pad * 2
            visible: root.localError.length > 0 || root.localNotice.length > 0
            activeFocusOnTab: visible
            Accessible.role: Accessible.StaticText
            Accessible.name: feedbackText.text
            Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: feedback.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: feedback.activeFocus ? 2 : 1 }
            QGCLabel {
                id: feedbackText
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad
                textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: root.localError.length ? qsTr("未完成：") + root.localError : root.localNotice
                color: root.localError.length ? qgcPal.warningText : qgcPal.text
            }
        }
        QGCButton {
            id: uploadButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            text: qsTr("申请上传确认（先不写入）")
            focusPolicy: Qt.StrongFocus; enabled: root.canRequest
            property var pressedBinding: ({})
            property bool releasedCurrent: false
            onPressed: { pressedBinding = root.bindingSnapshot(); releasedCurrent = false }
            onReleased: releasedCurrent = root.bindingMatches(pressedBinding) && root.authorizationMatches(pressedBinding.actorAuthorization) && root.securityMatches(pressedBinding) && root.canRequest
            onCanceled: { pressedBinding = ({}); releasedCurrent = false }
            onClicked: { root.requestUpload(pressedBinding, releasedCurrent); pressedBinding = ({}); releasedCurrent = false }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: root.deployment.deploymentId ? qsTr("部署记录：%1\n传输状态：%2\n原传输排空中：%3\n已经或可能写入：%4 · 回滚：不执行")
                .arg(root.text(root.deployment.deploymentId)).arg(root.stateLabel(root.deployment.state))
                .arg(root.known(root.deployment.transportDraining)).arg(root.known(root.deployment.writeMayHaveCommitted))
                : qsTr("尚无部署记录。申请确认不会写入；只有独立确认后才可能覆盖机载任务。")
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !!root.deployment.reason || root.deployment.transportDraining === true
            textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: (root.deployment.reason ? qsTr("停止 / 失败原因：%1\n").arg(root.text(root.deployment.reason)) : "")
                + qsTr("停止不等于撤销已写数据。排空结束前不要再次上传，或将未知结果当作原任务已保留。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: qsTr("下列诊断只解释当次上传后的普通 decoded MissionItem 回读；不是原始 RAW/INT 报文来源认证，不证明现在机载内容、无线请求因果、起飞许可或飞行放行。未取得或截断明细不等于无差异。")
        }
        Repeater {
            model: root.boundedList(root.deployment.vehicles, 32)
            ColumnLayout {
                id: deploymentTarget
                Layout.fillWidth: true
                readonly property var diagnosticOwner: root.controller
                readonly property var diagnostic: root.readbackWrapper(modelData, root.deployment)
                readonly property var comparison: root.readbackComparison(diagnostic)
                readonly property string diagnosticIdentity: root.readbackIdentity(diagnostic)
                property string expandedIdentity: ""
                readonly property bool detailsOpen: diagnosticIdentity.length > 0 && expandedIdentity === diagnosticIdentity
                onDiagnosticIdentityChanged: expandedIdentity = ""
                onDiagnosticOwnerChanged: expandedIdentity = ""
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: qsTr("ID %1 · %2 · %3 项\n上传 ACK：%4 · 内容回读匹配：%5\n可能已写入：%6")
                        .arg(root.text(modelData.vehicleId)).arg(root.stateLabel(modelData.state)).arg(root.text(modelData.itemCount))
                        .arg(root.known(modelData.uploadAcknowledged)).arg(root.known(modelData.readbackVerified)).arg(root.known(modelData.writeMayHaveCommitted))
                }
                QGCLabel { Layout.fillWidth: true; visible: !!modelData.error; textFormat: Text.PlainText; text: qsTr("本机错误：%1").arg(root.text(modelData.error)); wrapMode: Text.WordWrap; color: qgcPal.warningText }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: root.readbackSummary(deploymentTarget.diagnostic, deploymentTarget.comparison)
                }
                QGCButton {
                    id: readbackDetailsButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    enabled: !!deploymentTarget.diagnostic
                    text: deploymentTarget.detailsOpen ? qsTr("收起该机历史回读诊断") : qsTr("展开该机历史回读诊断（只读）")
                    Accessible.name: text
                    property string pressedIdentity: ""
                    property var pressedOwner: null
                    onPressed: { pressedIdentity = deploymentTarget.diagnosticIdentity; pressedOwner = deploymentTarget.diagnosticOwner }
                    onCanceled: { pressedIdentity = ""; pressedOwner = null }
                    onClicked: {
                        var original = pressedIdentity, owner = pressedOwner
                        pressedIdentity = ""; pressedOwner = null
                        if (original && original === deploymentTarget.diagnosticIdentity && owner === deploymentTarget.diagnosticOwner)
                            deploymentTarget.expandedIdentity = deploymentTarget.detailsOpen ? "" : original
                    }
                    contentItem: QGCLabel { text: readbackDetailsButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: readbackDetailsButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                }
                Loader {
                    // Do not instantiate 32 x 64 field delegates while closed.
                    Layout.fillWidth: true; active: deploymentTarget.detailsOpen
                    Layout.preferredHeight: item ? item.implicitHeight : 0
                    sourceComponent: Component {
                        ColumnLayout {
                            width: parent ? parent.width : 1; spacing: root.pad
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                                text: deploymentTarget.diagnostic ? qsTr("历史诊断归属\n部署：%1 · 世代：%2 · 机号：%3\n原写操作：%4 · 原读操作：%5\n本进程单调观察时间：%6 ms（不是墙钟或新鲜度证明）\n捕获状态：%7")
                                    .arg(deploymentTarget.diagnostic.deploymentId).arg(deploymentTarget.diagnostic.generation).arg(deploymentTarget.diagnostic.vehicleId)
                                    .arg(deploymentTarget.diagnostic.writeOperationId).arg(deploymentTarget.diagnostic.readOperationId)
                                    .arg(deploymentTarget.diagnostic.observedAtMonotonicMs).arg(deploymentTarget.diagnostic.captureState) : qsTr("原诊断身份已变化；不沿用旧展开内容。")
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                                text: deploymentTarget.comparison ? qsTr("预期规范化有效：%1 · 实际规范化有效：%2\n预期规范化错误：%3 · itemIndex：%4\n实际规范化错误：%5 · itemIndex：%6\n预期内容指纹：%7\n实际内容指纹：%8\n错误 itemIndex=-1 表示未知/不指向具体项；非负序号均从0开始。指纹只标识该次规范化内容，不认证来源。")
                                    .arg(root.known(deploymentTarget.comparison.expectedCanonicalValid)).arg(root.known(deploymentTarget.comparison.observedCanonicalValid))
                                    .arg(deploymentTarget.comparison.expectedErrorCode || qsTr("未报告")).arg(deploymentTarget.comparison.expectedErrorItemIndex)
                                    .arg(deploymentTarget.comparison.observedErrorCode || qsTr("未报告")).arg(deploymentTarget.comparison.observedErrorItemIndex)
                                    .arg(deploymentTarget.comparison.expectedContentHash || qsTr("未知/未生成")).arg(deploymentTarget.comparison.observedContentHash || qsTr("未知/未生成"))
                                    : qsTr("没有受支持的比较报告；不把缺失报告或空明细显示为相同。")
                            }
                            QGCLabel {
                                Layout.fillWidth: true; visible: !deploymentTarget.comparison || deploymentTarget.comparison.rows.length === 0
                                textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: qsTr("没有可展示的字段差异行。请以上方比较状态、未比较项数、计数完整性及截断标志为准；空行本身不是一致或许可证明。")
                            }
                            Repeater {
                                model: deploymentTarget.comparison ? deploymentTarget.comparison.rows : []
                                delegate: QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.readbackRowText(modelData); Accessible.name: text }
                            }
                        }
                    }
                }
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("停止后续上传（不回滚）")
            enabled: root.controller && !root.dispatchBusy && root.transportBusy && root.deployment.state !== "cancelled"
            property string pressedId: ""
            onPressed: pressedId = root.deployment.deploymentId || ""
            onCanceled: pressedId = ""
            onClicked: { root.stopUpload(pressedId); pressedId = "" }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.dispatchBusy ? qsTr("正在核对…") : qsTr("只读检查当前来源（不执行）")
            enabled: root.controller && !root.dispatchBusy && !root.transportBusy && !root.contentBusy && !root.reconciliationBusy && !root.refreshActive && !!root.deployment.deploymentId
            onClicked: root.inspect()
        }
        QGCLabel {
            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: !root.inspectionPresent ? qsTr("尚无只读快照。这里不会提供启动或解锁按钮。")
                : root.inspectionExpired ? qsTr("快照已过期 / 上下文已变化。下面仅保留历史参考；重新检查也不会授予启动许可。")
                : qsTr("一次性只读快照，短时展示而非实时放行。所有启动、执行和任务完成认证均未释放。")
        }
        Repeater {
            model: root.readinessRows
            ColumnLayout {
                id: readinessTarget
                Layout.fillWidth: true
                property bool expanded: false
                readonly property var blocked: root.reasons(modelData.blockedReasons, qsTr("阻断："))
                readonly property var unverified: root.reasons(modelData.unverifiedRequirements, qsTr("未验证："))
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: qsTr("ID %1 · 前置条件：%2\n原生来源完整：%3 · 仍有未验证要求（非安全就绪）")
                        .arg(root.text(modelData.vehicleId)).arg(modelData.preconditionsMatch === true ? qsTr("部分相符，非许可") : qsTr("受阻 / 未知"))
                        .arg(root.known(modelData.sourceEvidenceComplete))
                }
                QGCLabel { Layout.fillWidth: true; text: qsTr("阻断 %1 条 · 未验证 %2 条；默认各显示前 3 条。").arg(readinessTarget.blocked.length).arg(readinessTarget.unverified.length); wrapMode: Text.WordWrap }
                Repeater { model: readinessTarget.expanded ? readinessTarget.blocked : readinessTarget.blocked.slice(0, 3); QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; text: modelData; wrapMode: Text.WordWrap } }
                Repeater { model: readinessTarget.expanded ? readinessTarget.unverified : readinessTarget.unverified.slice(0, 3); QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; text: modelData; wrapMode: Text.WordWrap } }
                QGCButton {
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    visible: readinessTarget.blocked.length > 3 || readinessTarget.unverified.length > 3
                    text: readinessTarget.expanded ? qsTr("收起原因") : qsTr("展开全部已捕获原因")
                    onClicked: readinessTarget.expanded = !readinessTarget.expanded
                }
            }
        }
        Repeater {
            model: root.progressRows
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("ID %1 · 只读状态：%2\n当前序号：%3 · 到达序号：%4\n序号未绑定本计划的启动事务；重复回报不代表新增完成事件，不能据此宣布任务成功或安全停车。")
                    .arg(root.text(modelData.vehicleId)).arg(root.text(modelData.phase, 128))
                    .arg(root.observedSequence(modelData.currentSequence)).arg(root.observedSequence(modelData.reachedSequence))
            }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
            text: qsTr("失败 / 取消部署后的整组对账 · 独立只读诊断")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("原部署：%1 · 原部署代次：%2 · 冻结整组：%3\n按顺序重新请求各机完整 RAW Mission INT 内容，与该部署原冻结载荷比较；不采用当前草稿，也不要求旧整组回读成功。最多 32 架、每机 2048 项，期限 120 秒；这是容量边界，不保证大批次及时完成。")
                .arg(root.text(root.deployment.deploymentId, 64)).arg(root.text(root.deployment.generation, 32))
                .arg(root.contentTargets.length ? root.contentTargets.join(", ") : qsTr("无有效原部署整组"))
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("这是独立历史诊断，不是上面的普通 decoded 回读明细，也不是下面的启动前核对凭据。逐机读取不是远端整组原子快照，不证明无线请求因果、硬件身份认证或此刻机载内容。不同 / 未知不自动修复、重新上传或启动；读取协议仍有受限重试。即使全部匹配，也不恢复旧审批或授予启动许可。")
        }
        Controls.CheckBox {
            id: reconciliationConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: false; enabled: root.canReconcile
            text: qsTr("同意撤销旧审阅 / 审批，并只读核对上面失败或取消部署的原整组任务")
            contentItem: QGCLabel {
                text: reconciliationConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                leftPadding: (reconciliationConsent.indicator ? reconciliationConsent.indicator.width : 0) + root.pad
            }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.reconciliationBlockedReason }
        QGCButton {
            id: reconcileButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.reconciliation.active === true ? qsTr("正在顺序核对原整组任务…") : qsTr("重新核对整组机载任务（只读）")
            enabled: root.canReconcile && reconciliationConsent.checked
            property var pressedIntent: null
            onPressed: {
                pressedIntent = root.canReconcile && reconciliationConsent.checked ? {
                    owner: root.controller, token: root.reconciliationSourceToken, deploymentId: root.deployment.deploymentId,
                    deploymentGeneration: root.deployment.generation, ids: root.contentTargets.slice(0),
                    identity: root.freezeIdentity(root.identity), actor: JSON.parse(JSON.stringify(root.reconciliationAuthorization)),
                    binding: JSON.parse(JSON.stringify(root.deployment.binding || {}))
                } : null
            }
            onCanceled: pressedIntent = null
            onClicked: {
                var intent = pressedIntent, guard = root.reconciliationLifetime
                pressedIntent = null
                if (!intent || intent.owner !== root.controller || intent.token !== root.reconciliationSourceToken
                        || !root.canReconcile || !reconciliationConsent.checked || intent.deploymentId !== root.deployment.deploymentId
                        || intent.deploymentGeneration !== root.deployment.generation || intent.ids.join(",") !== root.contentTargets.join(",")
                        || !root.identityMatches(intent.identity) || !root.reconciliationActorMatches(intent.actor)
                        || !root.contentBindingMatches(intent.binding, root.deployment.binding)) {
                    reconciliationConsent.checked = false
                    return root.fail(qsTr("原部署、当前运行来源、身份或权限已变化；请重新确认整组后申请对账。"))
                }
                var result = {}
                root.dispatchBusy = true
                reconciliationConsent.checked = false
                try { result = intent.owner.requestFleetMissionReconciliation(intent.deploymentId, true) || {} }
                catch (error) { result = {reason: "reconciliation_call_interrupted"} }
                if (!guard.alive) return
                root.dispatchBusy = false
                if (intent.owner !== root.controller || intent.token !== root.reconciliationSourceToken
                        || intent.deploymentId !== root.deployment.deploymentId || intent.deploymentGeneration !== root.deployment.generation
                        || intent.ids.join(",") !== root.contentTargets.join(",") || !root.identityMatches(intent.identity)
                        || !root.reconciliationActorMatches(intent.actor) || !root.contentBindingMatches(intent.binding, root.deployment.binding)) {
                    // The controller accepts only the exact current native ID.
                    // A stale ID can never select or cancel a replacement run.
                    if (root.reconciliationUuid(result.requestId)) {
                        try { intent.owner.cancelFleetMissionReconciliation(result.requestId) }
                        catch (cancelError) { /* The original native owner can already be gone. */ }
                    }
                    if (guard.alive) root.fail(qsTr("返回时原来源已变化；已尝试停止原请求，不撤回在途消息、不释放未知占用，也不恢复旧批准。"))
                    return
                }
                var accepted = root.checkedReconciliation(result, true)
                if (accepted.requestId !== root.reconciliation.requestId || accepted.runEpoch !== root.reconciliation.runEpoch
                        || accepted.generation !== root.reconciliation.generation) accepted = ({})
                if (!accepted.requestId) root.fail(qsTr("整组对账未确认接受：%1。检查原生原因与占用后可再次明确申请，不自动重试。").arg(root.text(result.reason, 256)))
                else {
                    root.localError = ""
                    root.localNotice = qsTr("已接受原整组只读对账。逐机结果仅是当次历史诊断；读取结束不等于整组匹配或可以启动。")
                }
            }
            contentItem: QGCLabel { text: reconcileButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: reconcileButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: root.reconciliationSnapshot.requestId
                ? qsTr("对账请求：%1 · %2\n本批整组 %3 架：匹配 %4 / 不同 %5 / 未知 %6 / 待读取或读取中 %7\n原预期 %8 项；仅完整观察累计 %9 项（未观察不按零项计算）。\n占用：%10 · 协议排空：%11 · 原因：%12\n以上是当次观察摘要，不是实时状态、原子快照、完整历史证明或飞行许可。")
                    .arg(root.text(root.reconciliationSnapshot.requestId, 64)).arg(root.reconciliationStateText(root.reconciliationSnapshot.state))
                    .arg(root.reconciliationSnapshot.totalVehicleCount).arg(root.reconciliationSnapshot.matchedVehicleCount)
                    .arg(root.reconciliationSnapshot.differentVehicleCount).arg(root.reconciliationSnapshot.unknownVehicleCount)
                    .arg(root.reconciliationSnapshot.pendingVehicleCount).arg(root.reconciliationSnapshot.expectedTotalItemCount)
                    .arg(root.reconciliationSnapshot.observedTotalItemCount).arg(root.known(root.reconciliationSnapshot.reservationHeld))
                    .arg(root.known(root.reconciliationSnapshot.transportDraining)).arg(root.text(root.reconciliationSnapshot.reason, 256))
                : Object.keys(root.reconciliation).length ? qsTr("对账摘要来源、形状或部署代次无法确认；不能据此判断匹配。若读取仍占用，可停止该原请求。")
                    : qsTr("尚无独立整组对账观察；没有结果不等于没有差异。")
        }
        Repeater {
            model: root.boundedList(root.reconciliationSnapshot.vehicles, 32)
            ColumnLayout {
                id: reconciliationTarget
                Layout.fillWidth: true
                readonly property var diagnosticOwner: root.controller
                readonly property string observationIdentity: root.reconciliationSnapshotIdentity(root.reconciliationSnapshot) + ":" + modelData.vehicleId
                property string expandedIdentity: ""
                readonly property bool detailsOpen: expandedIdentity !== "" && expandedIdentity === observationIdentity
                onObservationIdentityChanged: expandedIdentity = ""
                onDiagnosticOwnerChanged: expandedIdentity = ""
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: qsTr("对账 ID %1 · %2\n预期 %3 项 / %4 · 原因：%5")
                        .arg(modelData.vehicleId).arg(root.reconciliationStateText(modelData.state)).arg(modelData.expectedItemCount)
                        .arg(modelData.completeReadObserved === true ? qsTr("当次完整观察 %1 项").arg(modelData.observedItemCount) : qsTr("尚未取得完整观察（不是零项）"))
                        .arg(root.text(modelData.reason, 256))
                }
                QGCButton {
                    id: reconciliationDetailsButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: reconciliationTarget.detailsOpen ? qsTr("收起 ID %1 对账来源摘要").arg(modelData.vehicleId) : qsTr("展开 ID %1 对账来源摘要（只读）").arg(modelData.vehicleId)
                    property string pressedIdentity: ""
                    property var pressedOwner: null
                    onPressed: { pressedIdentity = reconciliationTarget.observationIdentity; pressedOwner = root.controller }
                    onCanceled: { pressedIdentity = ""; pressedOwner = null }
                    onClicked: {
                        if (pressedOwner === root.controller && pressedIdentity === reconciliationTarget.observationIdentity)
                            reconciliationTarget.expandedIdentity = reconciliationTarget.detailsOpen ? "" : pressedIdentity
                        pressedIdentity = ""; pressedOwner = null
                    }
                    contentItem: QGCLabel { text: reconciliationDetailsButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: reconciliationDetailsButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                }
                Loader {
                    Layout.fillWidth: true
                    active: reconciliationTarget.detailsOpen
                    visible: active
                    sourceComponent: Component {
                        QGCLabel {
                            width: parent ? parent.width : implicitWidth
                            textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                            text: qsTr("来源：独立 RAW Mission INT 历史诊断\n请求运行：%1 · 对账代次：%2 · 原部署代次：%3\n每机读取作用域：%4 · 原生操作 ID：%5\n完整读取：%6 · 计数已观察：%7 · 比较依据：%8\n预期规范化哈希：%9\n观察规范化哈希：%10\n原始条目指纹：%11\n原生单调观察时标：%12（不是墙钟或持续新鲜度）\n此入口提供状态、计数和哈希摘要，不提供逐字段差异；空哈希 / 未知不是没有差异，也不证明源身份或启动许可。")
                                .arg(root.text(root.reconciliationSnapshot.runEpoch, 64)).arg(root.text(root.reconciliationSnapshot.generation, 32))
                                .arg(root.text(root.reconciliationSnapshot.deploymentGeneration, 32)).arg(root.text(modelData.readScopeId, 64))
                                .arg(root.text(modelData.operationId, 32)).arg(root.known(modelData.completeReadObserved))
                                .arg(root.known(modelData.countObserved)).arg(root.text(modelData.comparisonBasis, 64))
                                .arg(root.text(modelData.expectedContentHash, 64)).arg(root.text(modelData.observedContentHash, 64))
                                .arg(root.text(modelData.rawItemsFingerprint, 64)).arg(root.text(modelData.observedAtMonotonicMs, 32))
                            Accessible.name: text
                        }
                    }
                }
            }
        }
        QGCButton {
            id: reconciliationCancelButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("停止本次整组对账（不回滚 / 不强制释放占用）")
            enabled: root.controller && !root.dispatchBusy && typeof root.controller.cancelFleetMissionReconciliation === "function"
                && root.reconciliationUuid(root.reconciliation.requestId) && root.reconciliation.deploymentId === root.deployment.deploymentId
                && root.reconciliation.deploymentGeneration === root.deployment.generation && root.reconciliation.active === true
            property var pressedIntent: null
            onPressed: { pressedIntent = {owner: root.controller, token: root.reconciliationSourceToken, requestId: root.reconciliation.requestId,
                    runEpoch: root.reconciliation.runEpoch, generation: root.reconciliation.generation} }
            onCanceled: pressedIntent = null
            onClicked: {
                var intent = pressedIntent, guard = root.reconciliationLifetime
                pressedIntent = null
                if (!intent || intent.owner !== root.controller || intent.token !== root.reconciliationSourceToken
                        || intent.requestId !== root.reconciliation.requestId || intent.runEpoch !== root.reconciliation.runEpoch
                        || intent.generation !== root.reconciliation.generation) return
                var accepted = false
                root.dispatchBusy = true
                try { accepted = intent.owner.cancelFleetMissionReconciliation(intent.requestId) === true }
                catch (error) { accepted = false }
                if (!guard.alive) return
                root.dispatchBusy = false
                if (intent.owner !== root.controller || intent.token !== root.reconciliationSourceToken
                        || intent.requestId !== root.reconciliation.requestId || intent.runEpoch !== root.reconciliation.runEpoch
                        || intent.generation !== root.reconciliation.generation) {
                    root.fail(qsTr("停止请求返回时来源已变化；不能确认新运行状态，不重试、不释放未知占用。"))
                    return
                }
                if (!accepted) root.fail(qsTr("没有确认停止该原对账请求。请核对当前原生占用；不会强制解锁或重试。"))
                else { root.localError = ""; root.localNotice = qsTr("已接受停止后续对账；已观察结果保留为历史，在途协议按原生规则排空，不回滚机载内容。") }
            }
            contentItem: QGCLabel { text: reconciliationCancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: reconciliationCancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
            text: qsTr("从原规划建立修复副本 · 只编辑本地草稿")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: root.sourceKind === "reserve-dispatch"
                ? qsTr("当前显示独立备用出勤来源：区域草稿恢复／切回不适用，保持禁用。新出勤路线不会反推成区域草稿；请先在备用出勤区明确退出此来源，再处理区域草稿。")
                : root.sourceKind === "remainder"
                ? qsTr("当前显示余段来源：这里的区域草稿恢复 / 切回操作不适用并已禁用。余段路线不会反推成区域草稿；如需返回区域编辑，请先在余段面板明确退出该来源。")
                : qsTr("整组对账终止且协议排空后，可读取原部署归档的区域、成员、端点、保留路线和配置，明确确认后建立可编辑副本。旧版本若没有记录规划来源，则不可恢复；不会从编译航线反推区域。此处不要求飞机在线，也不把历史对账当成新的设备证据。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("不同 / 未知结果需要人工分析；副本不代表已经修好。创建或切回后，都要重新确认坐标基准、重新生成审阅和审批。沿用当前意图的任务 ID，重新审阅形成新归档版本；不会修改原部署 / 原计划，不自动上传，也不发启动指令。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: root.repairDraft.sourceCurrent === true
                ? qsTr("原部署：%1 · 代次：%2\n原归档：%3 · 版本：%4\n原对账：%5 · 匹配 %6 / 不同 %7 / 未知 %8\n原规划修订：%9 · 区域顶点 %10 · 保留成员 %11\n这里只展示来源摘要；完整规划仅保留在控制器私有状态。")
                    .arg(root.text(root.repairDraft.source.deploymentId, 64)).arg(root.text(root.repairDraft.source.deploymentGeneration, 32))
                    .arg(root.text(root.repairDraft.source.recordId, 64)).arg(root.repairDraft.source.recordRevision)
                    .arg(root.text(root.repairDraft.source.reconciliationRequestId, 64)).arg(root.repairDraft.source.matchedVehicleCount)
                    .arg(root.repairDraft.source.differentVehicleCount).arg(root.repairDraft.source.unknownVehicleCount)
                    .arg(root.repairDraft.source.sourceRevision).arg(root.repairDraft.source.vertexCount).arg(root.repairDraft.source.memberCount)
                : qsTr("尚无当前有效的原规划来源预览。请先完成失败 / 取消部署的整组对账并等待排空，再明确读取来源；不会自动读取。")
        }
        QGCButton {
            id: repairInspectButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("读取原规划来源（仅本地，不改草稿）")
            enabled: root.canInspectRepair
            property var pressedIntent: null
            onPressed: pressedIntent = root.repairIntent("inspect")
            onCanceled: pressedIntent = null
            onClicked: { var original = pressedIntent; pressedIntent = null; root.dispatchRepair(original) }
            contentItem: QGCLabel { text: repairInspectButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: repairInspectButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        Controls.CheckBox {
            id: repairCreateConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: false; enabled: root.canCreateRepair
            text: qsTr("确认建立可编辑修复副本，保留当前草稿为备用；撤销旧审阅 / 审批，不上传、不启动")
            contentItem: QGCLabel { text: repairCreateConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (repairCreateConsent.indicator ? repairCreateConsent.indicator.width : 0) + root.pad }
        }
        Controls.CheckBox {
            id: repairReplaceConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.repairDraft.hasPrevious === true
            checked: false; enabled: root.canCreateRepair
            text: qsTr("已有一份更早的备用草稿：确认在新副本成功发布后替换该备用（只保留本次操作前的当前草稿）；旧备用将不能再从此入口切回")
            contentItem: QGCLabel { text: repairReplaceConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (repairReplaceConsent.indicator ? repairReplaceConsent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            id: repairCreateButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("创建并切换到可编辑修复副本")
            enabled: root.canCreateRepair && repairCreateConsent.checked && (root.repairDraft.hasPrevious !== true || repairReplaceConsent.checked)
            property var pressedIntent: null
            onPressed: pressedIntent = root.repairIntent("create")
            onCanceled: pressedIntent = null
            onClicked: { var original = pressedIntent; pressedIntent = null; root.dispatchRepair(original) }
            contentItem: QGCLabel { text: repairCreateButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: repairCreateButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCButton {
            id: repairCancelButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("取消来源预览（保留当前草稿）")
            enabled: root.repairLocalReady && !!root.repairSourceToken
            property var pressedIntent: null
            onPressed: pressedIntent = root.repairIntent("cancel")
            onCanceled: pressedIntent = null
            onClicked: { var original = pressedIntent; pressedIntent = null; root.dispatchRepair(original) }
            contentItem: QGCLabel { text: repairCancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: repairCancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("当前本地选择 %1 · 草稿修订 %2 · %3\n本地备用仅保留在本次应用内存中，不是磁盘备份或跨重启恢复。切回保留草稿内容，但清除坐标基准确认并撤销旧审阅 / 审批；切回前的当前草稿转为新的备用。")
                .arg(root.text(root.repairDraft.draftSelection, 32)).arg(root.text(root.repairDraft.draftRevision, 32))
                .arg(root.repairDraft.hasPrevious === true ? qsTr("有一份可切回备用，修订 %1").arg(root.repairDraft.previousRevision) : qsTr("尚无可切回备用"))
        }
        Controls.CheckBox {
            id: repairRestoreConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: false; enabled: root.canRestoreRepair
            text: qsTr("确认切回备用内容，并重新确认坐标基准和审阅；保留当前草稿为备用，不恢复旧批准")
            contentItem: QGCLabel { text: repairRestoreConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (repairRestoreConsent.indicator ? repairRestoreConsent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            id: repairRestoreButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("切回上一个草稿内容（不上传 / 不启动）")
            enabled: root.canRestoreRepair && repairRestoreConsent.checked
            property var pressedIntent: null
            onPressed: pressedIntent = root.repairIntent("restore")
            onCanceled: pressedIntent = null
            onClicked: { var original = pressedIntent; pressedIntent = null; root.dispatchRepair(original) }
            contentItem: QGCLabel { text: repairRestoreButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: repairRestoreButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            visible: root.repairDraft.busy === true || root.repairDraft.destructionPending === true || !root.repairLocalReady
            text: root.repairDraft.busy === true ? qsTr("正在处理本地草稿切换，请等待明确结果。")
                : root.repairDraft.destructionPending === true ? qsTr("已请求释放不再使用的本地草稿；实际销毁确认前不允许再次创建，仍可明确切回备用。")
                : root.sourceKind !== "draft" ? qsTr("区域修复控件不适用于余段／独立备用出勤／未知来源；不会改写该审阅为区域草稿。")
                : qsTr("请结束当前编辑、确认框或实际传输 / 排空后操作。本地编辑不要求飞机在线；它也不会绕过后续实际审阅与审批。")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true
            text: qsTr("启动前整组任务内容核对 · 只读，不是启动许可")
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("部署：%1 · 冻结目标：%2\n逐机重新请求完整原始 Mission INT 内容，核对本次运行的已归档载荷。最多 32 架 / 每机 2048 项 / 整组 8192 项，总采集期限 120 秒；审阅新鲜度从各机读取开始计时，最早一架先到期，不在完成时续期。容量上限不保证大批次能及时完成。")
                .arg(root.text(root.deployment.deploymentId, 64)).arg(root.contentTargets.length ? root.contentTargets.join(", ") : qsTr("无有效部署目标"))
        }
        Controls.CheckBox {
            id: contentConsent
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: false; enabled: root.canReadContent
            text: qsTr("同意撤销旧审阅 / 审批，并只读核对上面部署的整组机载任务")
            contentItem: QGCLabel {
                text: contentConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                leftPadding: (contentConsent.indicator ? contentConsent.indicator.width : 0) + root.pad
            }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.contentBlockedReason }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.contentCheck.active === true ? qsTr("正在逐机只读核对…") : qsTr("重新读取整组机载任务（不写入 / 不启动）")
            enabled: root.canReadContent && contentConsent.checked
            property var pressedController: null
            property string pressedDeploymentId: ""
            property var pressedIds: []
            property var pressedIdentity: ({})
            property var pressedActor: ({})
            property var pressedBinding: ({})
            onPressed: {
                pressedController = root.controller; pressedDeploymentId = root.deployment.deploymentId || ""
                pressedIds = root.contentTargets.slice(0); pressedIdentity = root.freezeIdentity(root.identity)
                pressedActor = JSON.parse(JSON.stringify(root.contentAuthorization))
                pressedBinding = JSON.parse(JSON.stringify(root.deployment.binding || {}))
            }
            onCanceled: { pressedController = null; pressedDeploymentId = ""; pressedIds = []; pressedIdentity = ({}); pressedActor = ({}); pressedBinding = ({}) }
            onClicked: {
                var owner = pressedController, depId = pressedDeploymentId, ids = pressedIds.slice(0)
                var frozenIdentity = pressedIdentity, actor = pressedActor, binding = pressedBinding
                if (owner !== root.controller || !root.canReadContent || !contentConsent.checked
                        || depId !== root.deployment.deploymentId || ids.join(",") !== root.contentTargets.join(",")
                        || !root.identityMatches(frozenIdentity) || !root.contentActorMatches(actor)
                        || !root.contentBindingMatches(binding, root.deployment.binding))
                    root.fail(qsTr("部署、目标、身份或权限已变化，请重新勾选后申请核对。"))
                else {
                    var result = {}
                    root.dispatchBusy = true
                    try { result = owner.requestFleetStartContentCheck(depId, true) || {} }
                    catch (error) { result = {reason: qsTr("整组任务读取调用异常")} }
                    root.dispatchBusy = false
                    if (owner !== root.controller || depId !== root.deployment.deploymentId
                            || ids.join(",") !== root.contentTargets.join(",") || !root.identityMatches(frozenIdentity)
                            || !root.contentActorMatches(actor) || !root.contentBindingMatches(binding, root.deployment.binding)) {
                        if (result.checkId) {
                            try { owner.cancelFleetStartContentCheck(result.checkId) }
                            catch (cancelError) { /* Old native owner may already be destroyed. */ }
                        }
                        root.fail(qsTr("返回时来源已变化；已尝试停止原匹配请求。不能撤回在途读取，也不会恢复旧批准。"))
                    } else if (result.ok !== true || result.requestAccepted !== true || result.deploymentId !== depId)
                        root.fail(qsTr("整组读取未接受：%1。请核对原生来源与占用，不自动重试。").arg(root.text(result.reason || result.errorCode)))
                    else {
                        root.localError = ""
                        root.localNotice = qsTr("已接受整组只读核对；尚未完成内容匹配。完成后须在原有效期内重新生成风险审阅，不授予启动许可。")
                    }
                }
                contentConsent.checked = false
                pressedController = null; pressedDeploymentId = ""; pressedIds = []; pressedIdentity = ({}); pressedActor = ({}); pressedBinding = ({})
            }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("核对请求：%1 · 状态：%2\n整表匹配：%3 / %4 · 排空占用：%5\n%6 · 显示剩余：%7 秒（原生再次核对才有效）\n原因：%8\n这是逐机顺序读取，不证明远端原子快照、无线请求因果、硬件认证或飞行许可。")
                .arg(root.text(root.contentCheck.checkId, 64)).arg(root.contentStateLabel(root.contentCheck.state))
                .arg(root.text(root.contentCheck.matchedVehicleCount)).arg(root.text(root.contentCheck.totalVehicleCount))
                .arg(root.known(root.contentCheck.transportDraining))
                .arg(root.contentCheck.active === true ? qsTr("采集尚未完成") : root.contentFresh ? qsTr("本地证据显示仍在短有效期内") : qsTr("未完成 / 已过期 / 仅历史参考"))
                .arg(Math.ceil(root.contentDisplayRemainingMs / 1000)).arg(root.text(root.contentCheck.reason, 128))
        }
        Repeater {
            model: root.boundedList(root.contentCheck.vehicles, 32)
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("任务核对 ID %1 · %2 项 · %3 · 原因：%4（不是起飞或完成证明）")
                    .arg(root.text(modelData.vehicleId)).arg(root.text(modelData.itemCount))
                    .arg(root.contentStateLabel(modelData.state)).arg(root.text(modelData.reason, 128))
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("停止当前内容核对 / 撤销本次核对依据（不回滚）")
            enabled: root.controller && !root.dispatchBusy && !!root.contentCheck.checkId
                && (root.contentCheck.active === true || root.contentCheck.state === "content-matched")
            property string pressedId: ""
            property var pressedController: null
            onPressed: { pressedId = root.contentCheck.checkId || ""; pressedController = root.controller }
            onCanceled: { pressedId = ""; pressedController = null }
            onClicked: {
                var owner = pressedController, id = pressedId
                pressedId = ""; pressedController = null
                if (owner && owner === root.controller && id && id === root.contentCheck.checkId) owner.cancelFleetStartContentCheck(id)
            }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("生成独立启动风险审阅（不执行）")
            enabled: root.controller && typeof root.controller.requestFleetMissionStartReview === "function"
                && !root.dispatchBusy && !root.transportBusy && !root.contentBusy && !root.reconciliationBusy && !root.refreshActive && root.contentFresh
            onClicked: root.requestStartReview()
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: root.startReview.reviewId
                ? qsTr("本地风险审阅：%1\n状态：%2 · %3\n仅风险 / 已阅记录，不是启动授权；身份、执行、任务完成和飞行许可均未认证。")
                    .arg(root.text(root.startReview.reviewId)).arg(root.text(root.startReview.state, 64))
                    .arg(root.startReview.snapshotExpired === true ? qsTr("已过期 / 已失效") : qsTr("仍需来源核对"))
                : qsTr("尚无独立启动风险审阅。先明确申请整组机载任务只读核对，再在从最早读取开始计算的有效期内重新审阅；不恢复旧审批，不提供启动、解锁或自动已阅。")
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !!root.startReview.reviewId; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("历史内容回读匹配：%1 · 当前原生缓存匹配：%2\n来源完整：%3 · 部分前置条件匹配：%4\n独立额外审批要求：%5 · 第二审批证据：%6\n匹配或已阅状态都不能替代硬件、定位精度、净空及真实执行验证。")
                .arg(root.known(root.startReviewArtifact.allHistoricalReadbacksMatched)).arg(root.known(root.startReviewArtifact.allCurrentNativeCachesMatch))
                .arg(root.known(root.startReviewArtifact.allSourceEvidenceComplete)).arg(root.known(root.startReviewArtifact.allPreconditionsMatch))
                .arg(root.known(root.startReviewArtifact.requiresAdditionalApproval)).arg(root.known(root.startReviewArtifact.secondApprovalEvidencePresent))
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !!root.startReview.reviewId; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
            text: qsTr("本次新鲜整表读取匹配：%1 · 核对 ID：%2\n审阅不晚于最早原读取到期；顺序读取不构成远端原子快照或执行归属证明。")
                .arg(root.known(root.startReviewArtifact.allFreshOnboardReadsMatched)).arg(root.text(root.startReviewArtifact.freshContentCheckId, 64))
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !!root.startReview.reason; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("审阅提示：%1").arg(root.text(root.startReview.reason))
        }
        Repeater {
            // All native bounded members are displayed; essential budgets
            // are never hidden by the diagnostic-reasons collapse control.
            model: root.boundedList(root.startReviewArtifact.vehicles, 32)
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                text: root.startupPolicySummary(modelData)
            }
        }
        Repeater {
            model: (root.showStartReviewReasons ? root.reasons(root.startReviewArtifact.blockedReasons, qsTr("审阅阻断："))
                                               : root.reasons(root.startReviewArtifact.blockedReasons, qsTr("审阅阻断：")).slice(0, 4))
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; text: modelData; wrapMode: Text.WordWrap }
        }
        Repeater {
            model: (root.showStartReviewReasons ? root.reasons(root.startReviewArtifact.healthReasonCodes, qsTr("健康来源："))
                                               : root.reasons(root.startReviewArtifact.healthReasonCodes, qsTr("健康来源：")).slice(0, 4))
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; text: modelData; wrapMode: Text.WordWrap }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            visible: root.boundedList(root.startReviewArtifact.blockedReasons, 32).length > 4
                || root.boundedList(root.startReviewArtifact.healthReasonCodes, 32).length > 4
            text: root.showStartReviewReasons ? qsTr("收起风险审阅原因") : qsTr("展开全部已捕获风险原因")
            onClicked: root.showStartReviewReasons = !root.showStartReviewReasons
        }
    }

    Component {
        id: confirmationComponent
        QGCPopupDialog {
            id: confirmation
            property var ownerPanel: null
            property var requestController: null
            property var frozenRequest: ({})
            property bool consumed: false
            destroyOnClose: false
            title: qsTr("二次确认 · 覆盖机载任务")
            buttons: 0
            acceptAllowed: false
            rejectAllowed: false
            closePolicy: Controls.Popup.NoAutoClose
            onOpened: cancelButton.forceActiveFocus()
            onClosed: {
                if (ownerPanel) ownerPanel.finishDialog(confirmation, frozenRequest.requestId, requestController, consumed)
                confirmation.destroy()
            }
            ColumnLayout {
                width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 48, confirmation._maxContentWidth - confirmation._contentMargin * 2))
                spacing: root.pad
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) { confirmation.close(); event.accepted = true }
                    else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) event.accepted = true
                }
                QGCLabel {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: qgcPal.warningText
                    text: qsTr("确认后会覆盖下面这些飞机的机载任务。不会解锁、起飞或启动；已写数据不自动回滚。请核对冻结记录，而不是当前新草稿。")
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("冻结规划来源：%1\n本地来源标识：%2（只绑定本次界面上下文，不是飞行许可）")
                        .arg(confirmation.frozenRequest.sourceKind === "reserve-dispatch" ? qsTr("独立备用出勤新任务")
                            : confirmation.frozenRequest.sourceKind === "remainder" ? qsTr("运行余段计划") : qsTr("区域规划草稿"))
                        .arg(root.text(confirmation.frozenRequest.contextToken, 512))
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("请求：%1\n冻结记录：%2\n冻结版本：%3\n冻结计划 ID：%4\n目标 ID：%5\n总编码任务项：%6\n到期（本机时间）：%7\n到期时间戳：%8 ms")
                        .arg(root.text(confirmation.frozenRequest.requestId, 256)).arg(root.text(confirmation.frozenRequest.recordId))
                        .arg(root.text(confirmation.frozenRequest.recordRevision)).arg(root.text(confirmation.frozenRequest.planId))
                        .arg(confirmation.frozenRequest.targetVehicleIds.join(", ")).arg(root.text(confirmation.frozenRequest.totalItemCount))
                        .arg(new Date(confirmation.frozenRequest.expiresAtMs).toLocaleString()).arg(root.text(confirmation.frozenRequest.expiresAtMs))
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("冻结操作者：%1 · 角色：%2\n策略世代：%3\n策略版本：%4\n本地 Windows 认证会话：%5\n独立审批：%6 · 审批账号：%7\n这是原生短期身份/上传审批，不是企业认证、两名人员证明或执行许可。")
                        .arg(root.text(confirmation.frozenRequest.actorAuthorization.actorId, 128))
                        .arg(root.text(confirmation.frozenRequest.actorAuthorization.role, 64))
                        .arg(root.text(confirmation.frozenRequest.actorAuthorization.policyEpoch))
                        .arg(root.text(confirmation.frozenRequest.actorAuthorization.policyRevision))
                        .arg(root.text(confirmation.frozenRequest.identityReceipt.sessionId, 64))
                        .arg(root.text(confirmation.frozenRequest.approvalReceipt.approvalId, 64))
                        .arg(root.text(confirmation.frozenRequest.approvalReceipt.approverId, 128))
                }
                QGCLabel {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                    text: qsTr("按默认焦点取消可安全退出。危险确认不是默认 Enter 动作；仅在请求仍有效时接受显式点击或聚焦后的空格操作。")
                }
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: qsTr("此确认只消费一次当前审批。随后本次部署最多获得 4 分钟上传 / 回读租约，不能超过原操作员会话；锁定、注销、撤权或到期都会停止后续步骤。不续期、不回滚、不启动。")
                }
                QGCButton {
                    id: cancelButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("取消，不上传")
                    Keys.onReturnPressed: { confirmation.close(); event.accepted = true }
                    Keys.onEnterPressed: { confirmation.close(); event.accepted = true }
                    onClicked: confirmation.close()
                }
                QGCButton {
                    id: confirmButton
                    Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                    text: qsTr("确认覆盖机载任务（不启动）")
                    primary: false
                    property string pressedRequestId: ""
                    property bool releasedCurrent: false
                    enabled: confirmation.ownerPanel && confirmation.ownerPanel.requestCurrent && !confirmation.ownerPanel.dispatchBusy
                    Accessible.name: text
                    Keys.onReturnPressed: event.accepted = true
                    Keys.onEnterPressed: event.accepted = true
                    onPressed: { pressedRequestId = confirmation.frozenRequest.requestId; releasedCurrent = false }
                    onReleased: releasedCurrent = confirmation.ownerPanel && confirmation.ownerPanel.requestCurrent && pressedRequestId === confirmation.frozenRequest.requestId
                    onCanceled: { pressedRequestId = ""; releasedCurrent = false }
                    onClicked: if (confirmation.ownerPanel) confirmation.ownerPanel.submit(confirmation, confirmation.frozenRequest, confirmation.requestController, pressedRequestId, releasedCurrent)
                    background: Rectangle {
                        color: confirmButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.windowShade
                        border.color: qgcPal.colorRed
                        border.width: confirmButton.activeFocus ? 3 : 1
                        radius: 3
                    }
                    contentItem: QGCLabel {
                        text: confirmButton.text
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: confirmButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
                    }
                }
            }
        }
    }
}

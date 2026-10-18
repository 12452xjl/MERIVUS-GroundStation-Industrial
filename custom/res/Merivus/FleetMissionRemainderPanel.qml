import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Native-source, operator-selected single-member route proposal. No aircraft
// action, old permission, report-summary or area-polygon reconstruction.
ColumnLayout {
    id: root
    property var controller: null
    property bool inputSettled: false // Excludes this panel's own pendingInput.
    property bool expanded: false
    property bool active: false
    property bool callBusy: false
    property var lifetime: ({alive: true, epoch: 0, reviewEpoch: 0, ticket: null})
    property int selectedVehicleId: 0
    property string ownedCaptureToken: ""
    property string ownedProposalToken: ""
    property string reviewedProposalToken: ""
    property var fields: ({})
    property string profileText: ""
    property bool previewConfirmed: false
    property bool reviewConfirmed: false
    property var adoptionConsent: null
    property string message: ""
    property int waypointPage: 0
    readonly property bool pendingInput: active || callBusy || archivePanel.pendingInput
    readonly property var state: controller ? controller.fleetMissionRemainder || {} : ({})
    readonly property var capture: state.capture || ({})
    readonly property var proposal: state.proposal || ({})
    readonly property var review: state.review && state.review.kind === "single" ? state.review : ({})
    readonly property var planning: controller ? controller.missionPlanningStatus || {} : ({})
    readonly property bool ownPlanning: review.planningPending === true && review.requestId
        && review.requestId === planning.requestId
    // This is a new actual review, not the original Run's authority or the
    // area's draft review. It survives a change of observation selection.
    readonly property var sharedReview: controller ? controller.fleetMissionRemainderReview || {} : ({})
    readonly property var newReview: sharedReview.kind === "single" ? sharedReview : ({})
    readonly property bool foreignGroupContext: capture.kind === "group" || capture.kind === "reserve-dispatch"
        || ((sharedReview.kind === "group" || sharedReview.kind === "reserve-dispatch") && sharedReview.current === true)
    readonly property var newArchive: newReview.archive || ({})
    readonly property int newTargetId: reviewTarget(newReview.plan)
    readonly property string newReviewToken: typeof newReview.token === "string" ? newReview.token : ""
    readonly property string newReviewIdentity: reviewIdentity(newReview)
    readonly property bool editingLocked: newReview.accepted === true
    readonly property bool newReviewCurrent: newReview.current === true && newReviewToken.length > 0
    readonly property bool adoptionConfirmed: adoptionConsent !== null && adoptionConsent.owner === controller
        && adoptionConsent.identity === newReviewIdentity && adoptionConsent.epoch === lifetime.reviewEpoch
    readonly property string approvalState: newReviewCurrent && controller && typeof controller.intentTaskState === "string"
        ? controller.intentTaskState : "unknown"
    readonly property var run: controller ? controller.fleetMissionExecutionObservation || {} : ({})
    readonly property string sourceToken: controller ? controller.fleetObservationSourceToken || "" : ""
    readonly property bool historicalSource: capture.sourceKind === "historical-run-planning-snapshot"
    readonly property bool historicalCaptureRetained: historicalSource && typeof capture.token === "string" && !!capture.token
    readonly property string planningSourceToken: historicalSource ? capture.sourceToken || "" : sourceToken
    readonly property string planningRunId: historicalSource ? capture.runId || "" : run.runId || ""
    readonly property var members: memberChoices(run.vehicles)
    readonly property bool captureCurrent: active && ownedCaptureToken.length > 0 && capture.token === ownedCaptureToken
        && capture.current === true && capture.sourceToken === planningSourceToken && capture.runId === planningRunId
        && capture.sourceVehicleId === selectedVehicleId
    readonly property bool proposalCurrent: captureCurrent && ownedProposalToken.length > 0
        && proposal.token === ownedProposalToken && proposal.captureToken === ownedCaptureToken
    readonly property var waypoints: captureCurrent && capture.waypoints && capture.waypoints.length <= 2048 ? capture.waypoints : []
    readonly property int pageCount: Math.max(1, Math.ceil(waypoints.length / 8))
    readonly property int currentPage: Math.max(0, Math.min(waypointPage, pageCount - 1))
    readonly property bool ready: visible && expanded && controller && lifetime.alive && inputSettled
        && !callBusy && state.busy !== true && planning.active !== true && !foreignGroupContext
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var definitions: [
        {key: "sourceWaypointIndex", label: qsTr("原出航航点起点索引 · 0 基，保留该点完整停留"), min: 0, max: Math.max(0, waypoints.length - 1), integer: true},
        {key: "targetVehicleId", label: qsTr("新提案目标机 ID · 不自动选替补"), min: 1, max: 255, integer: true},
        {key: "startLat", label: qsTr("新起点纬度 (°)"), min: -80, max: 80},
        {key: "startLon", label: qsTr("新起点经度 (°)"), min: -180, max: 180},
        {key: "startAlt", label: qsTr("新起点 AMSL 海拔 (m) · 非离地高"), min: -1000, max: 20000},
        {key: "returnLat", label: qsTr("新返回点纬度 (°)"), min: -80, max: 80},
        {key: "returnLon", label: qsTr("新返回点经度 (°)"), min: -180, max: 180},
        {key: "returnAlt", label: qsTr("新返回点 AMSL 海拔 (m)"), min: -1000, max: 20000},
        {key: "cruiseSpeedMetersPerSecond", label: qsTr("新水平速度 (m/s) · 大于 0"), min: 0, max: 1000, exclusive: true},
        {key: "climbSpeedMetersPerSecond", label: qsTr("新爬升速度 (m/s) · 大于 0"), min: 0, max: 1000, exclusive: true},
        {key: "descentSpeedMetersPerSecond", label: qsTr("新下降速度 (m/s) · 大于 0"), min: 0, max: 1000, exclusive: true}
    ]
    spacing: pad
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }
    signal focusVisibilityRequested(var item)
    signal reviewInvalidationRequested()

    function text(value) { return typeof value === "string" && value.length ? value.substring(0, 512) : qsTr("未知 / 未提供") }
    function number(value) { return typeof value === "number" && isFinite(value) ? String(value) : qsTr("未知") }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function seconds(value) { return typeof value === "number" && isFinite(value) && value >= 0 ? qsTr("%1 s（估算）").arg(value) : qsTr("未知") }
    function memberChoices(values) {
        if (!values || typeof values.length !== "number" || values.length > 32) return []
        var rows = [], seen = ({})
        for (var i = 0; i < values.length; ++i) {
            var row = values[i]
            if (!row || !integer(row.vehicleId, 1, 255) || seen[row.vehicleId]) return []
            seen[row.vehicleId] = true
            rows.push({vehicleId: row.vehicleId, label: qsTr("原成员 ID %1").arg(row.vehicleId)})
        }
        return rows
    }
    function memberIndex() { for (var i = 0; i < members.length; ++i) if (members[i].vehicleId === selectedVehicleId) return i; return -1 }
    function dropConfirmations() { previewConfirmed = false; reviewConfirmed = false; adoptionConsent = null }
    function invalidateLocal() {
        lifetime.epoch += 1; active = false; ownedCaptureToken = ""; ownedProposalToken = ""; reviewedProposalToken = ""
        dropConfirmations(); waypointPage = 0
    }
    function inputChanged(key, value) {
        if (editingLocked || callBusy || lifetime.ticket) return
        lifetime.epoch += 1
        var next = {}; for (var name in fields) next[name] = fields[name]
        next[key] = value; fields = next
        ownedProposalToken = ""; reviewedProposalToken = ""; dropConfirmations()
        reviewInvalidationRequested()
    }
    function profileChanged(value) {
        if (editingLocked || callBusy || lifetime.ticket) return
        lifetime.epoch += 1; profileText = value; ownedProposalToken = ""; reviewedProposalToken = ""; dropConfirmations()
        reviewInvalidationRequested()
    }
    function decimal(value) { return /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(String(value).trim()) ? Number(value) : NaN }
    function fieldError(definition) {
        var value = decimal(fields[definition.key])
        return !isFinite(value) || value < definition.min || value > definition.max || (definition.exclusive && value === 0)
            || (definition.integer && Math.floor(value) !== value) ? qsTr("请输入%1 %2 至 %3 范围内的有限数字%4。")
                .arg(definition.integer ? qsTr("整数") : "").arg(definition.min).arg(definition.max)
                .arg(definition.exclusive ? qsTr("，不含 0") : "") : ""
    }
    function point(prefix) { return {coordinateSystem: "geodetic", altitudeReference: "amsl", latitude: decimal(fields[prefix + "Lat"]), longitude: decimal(fields[prefix + "Lon"]), altitude: decimal(fields[prefix + "Alt"])} }
    function requestValue() {
        for (var i = 0; i < definitions.length; ++i) {
            var error = fieldError(definitions[i])
            if (error) { message = definitions[i].label + "：" + error; return null }
        }
        if (profileText.length > 16384) { message = qsTr("新执行配置 JSON 超过 16384 字符，请缩减后再预览。"); return null }
        var profile = ({})
        try { if (profileText.trim()) profile = JSON.parse(profileText) }
        catch (error) { message = qsTr("新执行配置不是有效 JSON；没有使用原机配置替代。"); return null }
        if (!profile || typeof profile !== "object" || Array.isArray(profile)) { message = qsTr("执行配置必须为完整对象或空对象 {}。"); return null }
        return {sourceWaypointIndex: decimal(fields.sourceWaypointIndex), targetVehicleId: decimal(fields.targetVehicleId),
            start: point("start"), returnTo: point("return"), cruiseSpeedMetersPerSecond: decimal(fields.cruiseSpeedMetersPerSecond),
            climbSpeedMetersPerSecond: decimal(fields.climbSpeedMetersPerSecond), descentSpeedMetersPerSecond: decimal(fields.descentSpeedMetersPerSecond),
            executionProfile: profile, operatorConfirmed: true}
    }
    function perform(action) {
        if (!ready || lifetime.ticket || editingLocked || (action !== "capture" && !captureCurrent)) return
        if (action === "capture" && (memberIndex() < 0 || run.prepared !== true || !sourceToken)) return
        if (action === "preview" && !previewConfirmed) return
        if (action === "review" && (!proposalCurrent || !reviewConfirmed || reviewedProposalToken === ownedProposalToken)) return
        var request = action === "preview" ? requestValue() : null
        if (action === "preview" && !request) return
        var guard = lifetime, epoch = guard.epoch, owner = controller,
            selected = action === "capture" ? sourceToken : planningSourceToken,
            runId = action === "capture" ? run.runId : planningRunId, id = selectedVehicleId,
            capturedToken = ownedCaptureToken, proposalToken = ownedProposalToken
        var ticket = {}; guard.ticket = ticket
        // The human's replacement attempt invalidates the old local consent
        // before any synchronous native/QML notification can be observed.
        if (action === "capture") { ownedCaptureToken = ""; ownedProposalToken = ""; reviewedProposalToken = "" }
        else if (action === "preview") { ownedProposalToken = ""; reviewedProposalToken = "" }
        callBusy = true; dropConfirmations(); message = ""
        var result = null
        try {
            if (guard.alive && guard.epoch === epoch && owner === controller && visible) {
                if (action === "capture") result = owner.captureFleetMissionRemainder(selected, runId, id)
                else if (action === "preview") result = owner.previewFleetMissionRemainder(capturedToken, request)
                else result = owner.reviewFleetMissionRemainder(proposalToken, true)
            }
            if (!guard.alive) return
            var current = guard.epoch === epoch && owner === controller
                && selected === (action === "capture" ? sourceToken : planningSourceToken)
                && runId === (action === "capture" ? run.runId : planningRunId) && id === selectedVehicleId
            adoptResult(action, result, current, selected, runId, id, capturedToken, proposalToken)
        } catch (error) {
            if (guard.alive) message = qsTr("操作回执中断；请检查新审阅归档，不能据此断言未提交。输入保留，没有自动重试。")
        } finally {
            // Keep the local ticket throughout adoption and busy=false's
            // synchronous notification. No stale ready binding depends on
            // a mutation of this plain JS guard.
            if (guard.alive) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function adoptHistoricalCapture(token) {
        if (!visible || !expanded || !controller || !lifetime.alive || callBusy || lifetime.ticket
                || editingLocked || !inputSettled || !token || capture.token !== token
                || capture.current !== true || capture.sourceKind !== "historical-run-planning-snapshot"
                || !integer(capture.sourceVehicleId, 1, 255)) return
        var guard = lifetime
        lifetime.epoch += 1
        selectedVehicleId = capture.sourceVehicleId
        if (!guard.alive) return
        ownedCaptureToken = token; ownedProposalToken = ""; reviewedProposalToken = ""
        if (!guard.alive) return
        active = true; waypointPage = 0
        if (!guard.alive) return
        dropConfirmations()
        if (guard.alive) message = qsTr("已明确选用未认证历史规划资料。输入文字保留：请重新核对原航点索引、新目标、起返点、速度和全新执行配置；不据观察跳点，不恢复原任务或批准。")
    }
    function adoptResult(action, result, current, selected, runId, id, capturedToken, proposalToken) {
        if (!result || typeof result.ok !== "boolean" || typeof result.current !== "boolean"
                || typeof result.committed !== "boolean" || result.flightCommandReleased !== false) {
            message = qsTr("未取得完整回执；请核对已有归档，勿重复点击假定尚未提交。"); return
        }
        if (!current || !visible || result.current !== true) {
            message = result.committed === true ? qsTr("本地提交已发生，但来源或界面已变化；记录 ID：%1。不要当作当前可执行结果。").arg(text(result.recordId))
                                              : qsTr("来源或界面已变化，当前操作未确认；没有自动重试。")
            return
        }
        if (result.error === "remainder_operation_interrupted_check_record_state") {
            message = qsTr("操作中断，提交状态未能完整确认；请检查新审阅归档，勿按失败自动重试。已返回记录 ID：%1。").arg(text(result.recordId)); return
        }
        if (result.ok !== true) { message = qsTr("未完成：%1；已发生本地提交：%2。输入保留，可修正或退出。").arg(text(result.error)).arg(known(result.committed)); return }
        if (action === "review" && result.planningPending === true) {
            message = qsTr("已开始后台计算，输入保留。尚未产生最终审阅或归档；可取消，不会自动上传、启动或接替。")
            return
        }
        if (action === "capture") {
            if (!capture.token || capture.sourceToken !== selected || capture.runId !== runId || capture.sourceVehicleId !== id) {
                message = qsTr("返回的冻结来源不属于原选择，请重新读取。"); return
            }
            ownedCaptureToken = capture.token; ownedProposalToken = ""; reviewedProposalToken = ""; active = true; waypointPage = 0
            message = qsTr("已读取真实 Run 的冻结来源。原输入文本保留，请人工重新核对索引；没有自动选择下一个航点。")
        } else if (action === "preview") {
            if (!proposal.token || proposal.captureToken !== capturedToken) { message = qsTr("预览不属于原冻结来源，请重新生成。"); return }
            ownedProposalToken = proposal.token; reviewedProposalToken = ""
            message = qsTr("已生成几何 / 时间子提案预览，尚未调用实际任务审阅。请核对后另行确认。")
        } else {
            reviewedProposalToken = proposalToken
            message = qsTr("实际审阅已返回；新审阅版本记录 ID：%1。记录存在不表示通过、已上传或已执行。").arg(text(result.recordId))
        }
    }
    function cleanupCaptureToken() { return historicalCaptureRetained ? capture.token : ownedCaptureToken }
    function captureExitIntent() {
        return {owner: controller, epoch: lifetime.epoch, token: cleanupCaptureToken(), reviewToken: newReviewToken}
    }
    function leave(intent) {
        // The new review has its own identity. Never clear it by using an old
        // Run capture token, and never delete an archived record on exit.
        if (!intent || !visible || !expanded || intent.owner !== controller || intent.epoch !== lifetime.epoch
                || intent.token !== cleanupCaptureToken() || intent.reviewToken || newReviewToken) return
        if (callBusy || lifetime.ticket || state.busy === true) {
            message = qsTr("本地操作尚未结束，未确认退出；请待回执返回后重新点击退出。")
            return
        }
        var owner = intent.owner, token = intent.token, guard = lifetime
        invalidateLocal() // Text stays; no automatic recapture/retry.
        if (!guard.alive) return
        if (owner !== controller || guard.epoch !== intent.epoch + 1 || newReviewToken
                || (token && capture.token !== token)) {
            message = qsTr("退出期间来源或新审阅已变化；后台清理未确认，请重新核对后明确退出。")
            return
        }
        var cleared = false
        if (owner && token && !callBusy && !guard.ticket && state.busy !== true) {
            try { var result = owner.clearFleetMissionRemainder(token); cleared = result && result.ok === true }
            catch (error) { cleared = false }
        }
        if (guard.alive) message = cleared || !token
            ? qsTr("已退出余段编辑并清除本页确认；输入文本保留。再次使用需明确重读所选 Run 或重新选用完整历史资料。")
            : qsTr("已退出本页编辑；后台清理未确认。文本保留，下次须重新读取来源，不能沿用旧预览。")
    }
    function reviewTarget(plan) {
        var rows = plan && plan.members
        return rows && rows.length === 1 && rows[0] && integer(rows[0].vehicleId, 1, 255) ? rows[0].vehicleId : 0
    }
    function approvalStateText() {
        if (!newReviewCurrent) return qsTr("过期 / 未确认")
        if (!editingLocked) return qsTr("尚未采用新审阅")
        if (approvalState === "preview") return qsTr("等待申请本地批准")
        if (approvalState === "awaiting-approval") return qsTr("已申请，等待本地批准")
        if (approvalState === "approved") return qsTr("已本地批准，仍未代表上传或起飞")
        return qsTr("当前批准阶段未确认")
    }
    function reviewIdentity(value) {
        if (!value || typeof value.token !== "string" || !value.token) return ""
        var archived = value.archive || {}
        return JSON.stringify({token: value.token, proposalToken: value.proposalToken,
            recordId: archived.recordId, recordRevision: archived.recordRevision, sourceTaskId: archived.sourceTaskId,
            planId: archived.planId, contentHash: archived.contentHash, reviewEvidenceHash: archived.reviewEvidenceHash,
            storeEpoch: archived.storeEpoch, targetId: reviewTarget(value.plan)})
    }
    function reviewActionIntent(action) {
        if (!controller || !newReviewToken || !newReviewIdentity) return null
        return {action: action, owner: controller, epoch: lifetime.reviewEpoch, identity: newReviewIdentity,
            token: newReviewToken, recordId: newArchive.recordId, targetId: newTargetId}
    }
    function confirmAdoption(checked) {
        adoptionConsent = checked && ready && newReviewCurrent && !editingLocked && newTargetId > 0
            ? reviewActionIntent("accept") : null
    }
    function reviewActionCurrent(intent) {
        return intent && lifetime.alive && intent.owner === controller && intent.epoch === lifetime.reviewEpoch
            && intent.identity === newReviewIdentity && intent.token === newReviewToken
    }
    function runReviewAction(action, intent) {
        var clearing = action === "reedit" || action === "exit"
        if (!visible || !expanded || !reviewActionCurrent(intent) || intent.action !== action
                || callBusy || lifetime.ticket || state.busy === true) {
            dropConfirmations(); clearReviewConsent()
            if (lifetime.alive && clearing) message = qsTr("当前操作尚未结束或来源已变化；未确认撤销新审阅。输入保留，请待操作结束后明确退出。")
            return
        }
        if (!clearing && (!ready || !newReviewCurrent)) return
        if (action === "accept" && (editingLocked || !adoptionConfirmed || newTargetId < 1)) return
        if (action === "request" && (!editingLocked || newReview.approvalAllowed !== true || approvalState !== "preview")) return
        if (action === "approve" && (!editingLocked || newReview.approvalAllowed !== true || approvalState !== "awaiting-approval")) return
        if (["accept", "request", "approve", "reedit", "exit"].indexOf(action) < 0) return
        var guard = lifetime, owner = intent.owner, ticket = {}, result = null
        guard.ticket = ticket
        callBusy = true; dropConfirmations(); message = ""
        try {
            if (!guard.alive || !reviewActionCurrent(intent) || !visible) return
            if (action === "accept") result = owner.acceptFleetMissionRemainderReview(intent.token, true)
            else if (action === "request") result = owner.requestFleetMissionRemainderApproval(intent.token)
            else if (action === "approve") result = owner.approveFleetMissionRemainder(intent.token)
            else result = owner.clearFleetMissionRemainderReview(intent.token)
            if (!guard.alive) return
            if (!result || typeof result.ok !== "boolean" || typeof result.current !== "boolean"
                    || typeof result.committed !== "boolean" || result.flightCommandReleased !== false) {
                message = qsTr("未取得完整本地动作回执；请核对当前新审阅与归档记录，不能断言动作未发生。没有自动重试。")
                return
            }
            var sameView = intent.owner === controller && intent.epoch === guard.reviewEpoch && visible
            var sameReview = sameView && intent.identity === newReviewIdentity && intent.token === newReviewToken
            // A clear may deliberately leave the old token for historical
            // display. accepted, not token disappearance, owns the lock.
            var cleared = clearing && result.committed === true && sameView && newReview.accepted !== true
                && (!newReviewToken || newReviewToken === intent.token)
            if (cleared) {
                invalidateLocal()
                message = action === "reedit"
                    ? qsTr("已撤销本次新审阅的本地采用 / 批准上下文。输入文本保留；请重新读取实际来源再编辑。没有删除归档或取消飞机动作。")
                    : qsTr("已退出本次新提案并撤销其本地审阅上下文。归档事实与输入文本保留；没有取消飞机动作。")
                if (!result.current) message += qsTr(" 返回时上下文已变化，不恢复旧授权。")
                return
            }
            if (!sameReview || result.current !== true) {
                message = result.committed === true
                    ? qsTr("本地动作已发生，但新审阅已过期或界面已变化。记录：%1；不可沿用为当前批准，没有自动重试。").arg(text(result.recordId || intent.recordId))
                    : qsTr("本次动作未确认：%1。新审阅可能已变化或正在忙；此前记录 / 动作事实保留，请核对状态，不自动重试。").arg(text(result.error))
                return
            }
            if (result.error === "remainder_operation_interrupted_check_record_state") {
                message = qsTr("本地操作回执中断，不能据此断言此前动作未发生。已返回记录：%1；请核对当前采用 / 批准状态，没有自动重试。").arg(text(result.recordId || intent.recordId)); return
            }
            if (result.ok !== true) {
                message = qsTr("本地操作未完成：%1。已确认发生的本地动作：%2；不是飞行结果。请核对后再操作。")
                    .arg(text(result.error)).arg(known(result.committed)); return
            }
            if (action === "accept") {
                if (newReview.accepted !== true) { message = qsTr("采用回执已返回，但当前投影未确认采用；请核对记录，未自动重试。"); return }
                active = false // End pending input; do not clear the new record.
                message = qsTr("已结束编辑并采用目标 %1 的新审阅记录 %2。采用不等于审阅通过；后续仍须申请 / 批准及独立上传确认。没有发出飞行命令。")
                    .arg(newTargetId).arg(text(newArchive.recordId))
            } else if (action === "request") {
                message = qsTr("已为本次新审阅申请本地批准；请继续核对当前记录和独立批准步骤。没有上传或启动。")
            } else if (action === "approve") {
                message = qsTr("本次新审阅的本地批准动作已返回。请在唯一共享部署区继续核对上传条件；批准不等于已上传或已启动。")
            }
        } catch (error) {
            if (guard.alive) message = qsTr("本地动作回执中断；已发生的采用 / 批准不能据此视为未发生。请核对当前新审阅，输入保留且不自动重试。")
        } finally {
            if (guard.alive) {
                if (newReview.accepted === true) active = false
                callBusy = false
            }
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function progressText(value) {
        var p = value || {}, current = p.currentMissionItem || {}, reached = p.reachedMissionItem || {}
        return qsTr("冻结观察提示：协议 current %1（路径 %2 / 原点索引 %3），reached %4（路径 %5 / 原点索引 %6）。\n来源绑定当时有效：%7；连续性未知：%8。仅最后观察，不证明之前航点全部完成；不自动使用 index + 1。")
            .arg(number(p.currentSequence)).arg(text(current.sourcePath)).arg(number(current.sourcePointIndex))
            .arg(number(p.reachedSequence)).arg(text(reached.sourcePath)).arg(number(reached.sourcePointIndex))
            .arg(known(p.sourceCurrent)).arg(known(p.continuityUnknown))
    }
    function pointText(point, index) {
        point = point || {}
        return qsTr("原出航索引 %1：纬度 %2，经度 %3，AMSL %4 m；完整停留 %5 s。")
            .arg(index).arg(number(point.latitude)).arg(number(point.longitude)).arg(number(point.altitude)).arg(number(point.holdSeconds))
    }
    function summaryText(s) {
        s = s || {}
        return qsTr("原成员 %1 → 新目标 %2；原航点数 %3；人工起点索引 %4（包含该点）；保留 %5 点。\n新出航点数 %6；自身返程点数 %7。进场 %8，路线 %9，完整保留停留 %10，出航合计 %11，返程 %12。\n提供全新执行配置：%13；裁剪策略：%14。\n原 payload：%15\n原路线：%16\n新路线：%17\n仅所选单成员子提案；其他原机、Run 和控制权不变。不是空中接管，不保证障碍净空或剩余工作量，也不恢复旧批准。")
            .arg(number(s.sourceVehicleId)).arg(number(s.targetVehicleId)).arg(number(s.sourceWaypointCount)).arg(number(s.sourceWaypointIndex))
            .arg(number(s.retainedWaypointCount)).arg(number(s.outboundPointCount)).arg(number(s.returnPointCount))
            .arg(seconds(s.estimatedIngressSeconds)).arg(seconds(s.estimatedTaskSeconds)).arg(seconds(s.retainedHoldSeconds))
            .arg(seconds(s.estimatedOutboundSeconds)).arg(seconds(s.estimatedReturnSeconds)).arg(known(s.executionProfileProvided))
            .arg(text(s.selectionPolicy)).arg(text(s.sourcePayloadHash)).arg(text(s.originalRouteHash)).arg(text(s.newRouteHash))
    }
    function reviewReasons(values) {
        if (!values || typeof values.length !== "number" || values.length > 255) return qsTr("未提供有效原因列表")
        var lines = []
        for (var i = 0; i < Math.min(values.length, 12); ++i) {
            var value = values[i]
            lines.push(typeof value === "string" ? text(value) : text(value && (value.detail || value.message || value.reason || value.code)))
        }
        if (values.length > 12) lines.push(qsTr("这里只显示前 12 项；完整结果保留在新审阅版本中。"))
        return lines.length ? lines.join("\n") : qsTr("无已列出的原因（不等于飞行许可）")
    }
    function keepPaginationFocusVisible() {
        remainderRoutePanel.keepPaginationFocusVisible()
        if (!visible || !expanded || !captureCurrent) return
        if (waypointPrevious.activeFocus) focusVisibilityRequested(waypointPrevious)
        else if (waypointNext.activeFocus) focusVisibilityRequested(waypointNext)
    }
    function clearReviewConsent() {
        adoptionConsent = null
        if (acceptReviewButton) acceptReviewButton.pressedIntent = null
        if (requestReviewButton) requestReviewButton.pressedIntent = null
        if (approveReviewButton) approveReviewButton.pressedIntent = null
        if (reeditButton) reeditButton.pressedIntent = null
        if (exitButton) exitButton.pressedReviewIntent = null
    }
    onNewReviewIdentityChanged: clearReviewConsent()
    onNewReviewCurrentChanged: if (!newReviewCurrent) clearReviewConsent()
    onEditingLockedChanged: if (editingLocked) { active = false; dropConfirmations() }
    onControllerChanged: { lifetime.reviewEpoch += 1; invalidateLocal(); clearReviewConsent() }
    onSourceTokenChanged: if (!historicalSource) invalidateLocal()
    onVisibleChanged: if (!visible) { lifetime.reviewEpoch += 1; invalidateLocal(); clearReviewConsent() }
    Component.onDestruction: { lifetime.alive = false; lifetime.epoch += 1; lifetime.reviewEpoch += 1 }

    QGCButton {
        id: heading
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起 · 按运行观察准备余段提案") : qsTr("展开 · 按运行观察准备余段提案")
        contentItem: QGCLabel { text: heading.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: heading._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
        Accessible.name: text
        Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: heading.activeFocus }
        onClicked: {
            if (root.expanded) {
                // Folding is not an instruction to revoke a newly reviewed
                // record. Only the explicit exit/re-edit action does that.
                root.invalidateLocal(); root.clearReviewConsent()
            }
            root.expanded = !root.expanded
        }
        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.historicalSource
            ? qsTr("当前编辑来源为未认证历史资料包 %1；原 Run %2。不是实时飞机来源，也不证明已完成前缀。").arg(root.text(root.capture.sourceArchiveId)).arg(root.text(root.capture.runId))
            : qsTr("用上方生命周期入口选择实际观察组，或在下方明确打开完整历史规划资料包。运行报告摘要不能代替完整来源；不改原区域草稿。当前 Run：%1。").arg(root.text(root.run.runId)) }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("明确读取 / 重读会撤销当前提案的旧审阅与批准；不是只刷新显示。旧归档记录仍保留，但不能沿用旧确认执行新输入。") }
        QGCLabel { Layout.fillWidth: true; visible: !root.inputSettled; text: qsTr("其他草稿输入尚未结束，请先保存或取消；本页退出始终可用。"); wrapMode: Text.WordWrap; color: qgcPal.warningText }
        QGCComboBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            model: root.members; textRole: "label"; currentIndex: root.memberIndex()
            enabled: !root.active && !root.callBusy && !root.editingLocked
            Accessible.name: qsTr("选择原 Run 成员")
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            onActivated: { if (index >= 0 && index < root.members.length) root.selectedVehicleId = root.members[index].vehicleId; root.dropConfirmations(); currentIndex = Qt.binding(function() { return root.memberIndex() }) }
        }
        QGCButton {
            id: captureButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.active ? qsTr("重新读取冻结来源（旧预览失效）") : qsTr("明确读取所选成员冻结来源")
            enabled: root.ready && !root.editingLocked && root.memberIndex() >= 0 && root.run.prepared === true && !!root.sourceToken
            opacity: enabled ? 1 : 0.45
            Accessible.name: text
            contentItem: QGCLabel { text: captureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: captureButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: captureButton.activeFocus }
            onClicked: root.perform("capture")
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCButton {
            id: exitButton
            property var pressedReviewIntent: null
            property var pressedCaptureIntent: null
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: root.newReviewToken ? qsTr("退出当前提案（撤销本地审阅上下文）") : qsTr("退出余段编辑（保留输入文本）")
            Accessible.name: text
            contentItem: QGCLabel { text: exitButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: exitButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: exitButton.activeFocus }
            onPressed: {
                pressedReviewIntent = root.reviewActionIntent("exit")
                pressedCaptureIntent = pressedReviewIntent ? null : root.captureExitIntent()
            }
            onCanceled: { pressedReviewIntent = null; pressedCaptureIntent = null }
            onClicked: {
                var intent = pressedReviewIntent, captureIntent = pressedCaptureIntent
                pressedReviewIntent = null; pressedCaptureIntent = null
                if (intent) root.runReviewAction("exit", intent)
                else if (!root.newReviewToken) root.leave(captureIntent)
                else { root.clearReviewConsent(); root.message = qsTr("新审阅在按下按钮后发生变化，未退出新记录；请重新核对后明确退出。") }
            }
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.historicalCaptureRetained && !root.active
            textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("后台仍保留所选历史编辑来源（收起／隐藏不等于退出）。请先明确点击上方退出，再打开、选用或清除另一份资料；不会自动删除文件或归档。")
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.foreignGroupContext; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("当前共享来源属于多成员联合重规划或独立备用出勤；请在对应编辑区处理或明确退出。此单成员余段区不会采用、批准、取消或清理另一类来源。")
        }
        FleetMissionRemainderArchivePanel {
            id: archivePanel
            Layout.fillWidth: true
            controller: root.controller
            inputSettled: root.inputSettled && !root.callBusy && !root.editingLocked && !root.ownPlanning && !root.foreignGroupContext
            nativeCaptureSelected: root.captureCurrent && !root.historicalSource
            allowLoad: !root.active && !root.editingLocked && !root.historicalCaptureRetained && !root.foreignGroupContext
            allowUse: !root.active && !root.editingLocked && !root.historicalCaptureRetained && !root.foreignGroupContext
            onCaptureAdopted: root.adoptHistoricalCapture(token)
            onFocusVisibilityRequested: root.focusVisibilityRequested(item)
        }
        QGCLabel { Layout.fillWidth: true; visible: root.message.length > 0; text: root.message; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: qgcPal.warningText }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.captureCurrent; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("冻结来源 Run %1 · 原成员 %2 · 原部署 %3 · 原记录 %4；原出航航点 %5。后续进度不会改写本快照。").arg(root.text(root.capture.runId)).arg(root.number(root.capture.sourceVehicleId)).arg(root.text(root.capture.deploymentId)).arg(root.text((root.capture.binding || {}).recordId)).arg(root.number(root.capture.waypointCount)) }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.progressText(root.capture.progress) }
            QGCLabel { Layout.fillWidth: true; text: qsTr("原出航航点 · 第 %1 / %2 页（每页 8 点，0 基索引）").arg(root.currentPage + 1).arg(root.pageCount); wrapMode: Text.WordWrap }
            Repeater {
                model: root.waypoints.slice(root.currentPage * 8, root.currentPage * 8 + 8)
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.pointText(modelData, root.currentPage * 8 + index) }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { id: waypointPrevious; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("上一页"); enabled: root.currentPage > 0; focusPolicy: Qt.StrongFocus; showBorder: true; onClicked: root.waypointPage = root.currentPage - 1; onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
                QGCButton { id: waypointNext; Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("下一页"); enabled: root.currentPage + 1 < root.pageCount; focusPolicy: Qt.StrongFocus; showBorder: true; onClicked: root.waypointPage = root.currentPage + 1; onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
            }
            QGCLabel { Layout.fillWidth: true; text: qsTr("以下均为新提案的明确输入，不自动取 Home、实时机位或原机执行配置。选定点的全部停留会重做；未验证障碍、净空或作业是否已经完成。"); wrapMode: Text.WordWrap }
            Repeater {
                model: root.definitions
                ColumnLayout {
                    Layout.fillWidth: true
                    readonly property var definition: modelData
                    QGCLabel { Layout.fillWidth: true; text: definition.label; wrapMode: Text.WordWrap }
                    Controls.TextField {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 32; selectByMouse: true
                        text: root.fields[definition.key] || ""; enabled: !root.editingLocked && !root.callBusy && root.state.busy !== true
                        color: qgcPal.text; font.pointSize: ScreenTools.defaultFontPointSize; inputMethodHints: Qt.ImhFormattedNumbersOnly
                        Accessible.name: definition.label
                        onTextEdited: root.inputChanged(definition.key, text)
                        onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
                        background: Rectangle { color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: parent.activeFocus ? 2 : 1; radius: 3 }
                    }
                    QGCLabel { Layout.fillWidth: true; visible: text.length > 0; text: (root.fields[definition.key] || "").length ? root.fieldError(definition) : ""; color: qgcPal.warningText; wrapMode: Text.WordWrap }
                }
            }
            QGCLabel { Layout.fillWidth: true; text: qsTr("整份新 executionProfile JSON · 可留空 / {} 仅预览几何；不会复制原机配置。非空时必须提供完整当前意图，实际审阅仍重新取参数证据。"); wrapMode: Text.WordWrap }
            Controls.TextArea {
                Layout.fillWidth: true; Layout.preferredHeight: root.rowHeight * 3
                text: root.profileText; wrapMode: TextEdit.Wrap; selectByMouse: true; color: qgcPal.text
                enabled: !root.editingLocked && !root.callBusy && root.state.busy !== true
                Accessible.name: qsTr("整份新执行配置 JSON，最多 16384 字符")
                onTextChanged: if (activeFocus && text !== root.profileText) root.profileChanged(text)
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
                background: Rectangle { color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text; border.width: parent.activeFocus ? 2 : 1; radius: 3 }
            }
            QGCCheckBox { Layout.fillWidth: true; text: qsTr("确认人工裁剪与新输入"); checked: root.previewConfirmed; enabled: root.ready && !root.editingLocked; onClicked: { root.previewConfirmed = checked; checked = Qt.binding(function() { return root.previewConfirmed }) }; onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
            QGCButton {
                id: previewButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("生成单成员余段预览")
                enabled: root.ready && !root.editingLocked && root.previewConfirmed; focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45
                Accessible.name: text
                contentItem: QGCLabel { text: previewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: previewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: previewButton.activeFocus }
                onClicked: root.perform("preview"); onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.proposalCurrent
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: root.summaryText(root.proposal.summary) }
            QGCCheckBox { Layout.fillWidth: true; text: qsTr("确认生成新的实际审阅版本"); checked: root.reviewConfirmed; enabled: root.ready && !root.editingLocked; onClicked: { root.reviewConfirmed = checked; checked = Qt.binding(function() { return root.reviewConfirmed }) }; onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
            QGCButton {
                id: reviewButton
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("把此精确子提案送入实际审阅")
                enabled: root.ready && !root.editingLocked && root.reviewConfirmed && root.reviewedProposalToken !== root.ownedProposalToken
                focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45; Accessible.name: text
                contentItem: QGCLabel { text: reviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: reviewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: reviewButton.activeFocus }
                onClicked: root.perform("review"); onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.ownPlanning; textFormat: Text.PlainText; wrapMode: Text.Wrap
            text: root.planning.active === true
                ? (root.planning.status === "cancelling" ? qsTr("正在取消后台计算；等待线程退出。输入保留，不接受旧结果。")
                    : qsTr("后台计算进行中；没有最终审阅结果，也没有释放飞行命令。"))
                : qsTr("后台请求已结束或失效，请核对下方结果与原因；输入保留，不会自动重试。")
        }
        QGCButton {
            id: cancelPlanningButton
            property string pressedRequestId: ""
            property var pressedOwner: null
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
            visible: root.ownPlanning && root.planning.active === true
            enabled: visible && root.planning.status !== "cancelling" && !root.callBusy
            text: qsTr("取消本次余段后台计算（保留输入）")
            focusPolicy: Qt.StrongFocus; Accessible.name: text
            contentItem: QGCLabel { text: cancelPlanningButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: cancelPlanningButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: cancelPlanningButton.activeFocus }
            onPressed: { pressedRequestId = root.review.requestId || ""; pressedOwner = root.controller }
            onClicked: {
                if (pressedOwner === root.controller && pressedRequestId && root.ownPlanning
                        && root.review.requestId === pressedRequestId) pressedOwner.cancelMissionPlanning(pressedRequestId)
            }
            onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
        }
        QGCLabel {
            Layout.fillWidth: true; visible: !!root.review.proposalToken && root.review.planningPending !== true; textFormat: Text.PlainText; wrapMode: Text.Wrap
            text: qsTr("最近余段审阅记录（历史结果，不自动更新）：%1 · revision %2。\n记录提交：%3；原审阅回执归档标记（历史）：%4；计划可行：%5；仍需审阅：%6；状态：%7；能量：%8。\n沿用本 Root 的当前任务身份建立新版本，不宣称新 task UUID。没有上传、启动或接替权限。原因：%9")
                .arg(root.text(root.review.recordId)).arg(root.number(root.review.recordRevision)).arg(root.known(root.review.recordCommitted))
                .arg(root.known(root.review.planArchiveRecorded)).arg(root.known(root.review.feasible)).arg(root.known(root.review.needsReview))
                .arg(root.text(root.review.reviewStatus)).arg(root.text(root.review.energyStatus)).arg(root.text(root.review.error))
        }
        QGCLabel { Layout.fillWidth: true; visible: !!root.review.proposalToken; textFormat: Text.PlainText; wrapMode: Text.Wrap; text: qsTr("审阅原因：\n%1\n计划缺项：\n%2").arg(root.reviewReasons(root.review.reasons)).arg(root.reviewReasons(root.review.unmetRequirements)) }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.newReviewToken.length > 0; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; text: qsTr("本次新余段审阅 · 独立采用与批准"); font.bold: true; wrapMode: Text.WordWrap }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap
                text: qsTr("新目标机 ID：%1\n新记录：%2 · 版本 %3\n新计划：%4\n任务身份：%5\n此处不继承原运行记录的批准，也不是原区域草稿。")
                    .arg(root.newTargetId > 0 ? String(root.newTargetId) : qsTr("未知，不能采用"))
                    .arg(root.text(root.newArchive.recordId)).arg(root.number(root.newArchive.recordRevision))
                    .arg(root.text(root.newArchive.planId)).arg(root.text(root.newArchive.sourceTaskId))
            }
            QGCLabel {
                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                color: root.newReviewCurrent ? qgcPal.text : qgcPal.warningText
                text: qsTr("本地采用：%1；身份 / 版本当前有效：%2；允许申请批准：%3。\n实际审阅仍需人工处理：%4。%5")
                    .arg(root.known(root.newReview.accepted)).arg(root.known(root.newReview.current)).arg(root.known(root.newReview.approvalAllowed))
                    .arg(root.known((root.newReview.review || {}).needsReview))
                    .arg(root.newReviewCurrent
                        ? qsTr("当前有效仅表示绑定未过期，不等于审阅通过、上传成功或允许起飞。")
                        : qsTr("此新审阅已过期或未确认，不能继续申请 / 批准。归档事实保留；请明确重新编辑后再审阅。"))
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本地流程阶段：%1").arg(root.approvalStateText()) }
            QGCLabel {
                Layout.fillWidth: true; visible: root.editingLocked; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                text: qsTr("已采用，原输入已锁定。切换旧运行观察不会自动丢弃这份新计划；重新编辑或退出需明确撤销其本地上下文。共享部署区是唯一上传入口，本面板不新增上传或启动按钮。")
            }
            FleetMissionRemainderRoutePanel {
                id: remainderRoutePanel
                Layout.fillWidth: true
                reviewContext: root.newReview
                onFocusVisibilityRequested: root.focusVisibilityRequested(item)
            }
            QGCCheckBox {
                Layout.fillWidth: true; visible: !root.editingLocked
                text: qsTr("已核对新目标与本次记录")
                checked: root.adoptionConfirmed; enabled: root.ready && root.newReviewCurrent && root.newTargetId > 0
                onClicked: { root.confirmAdoption(checked); checked = Qt.binding(function() { return root.adoptionConfirmed }) }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCButton {
                id: acceptReviewButton
                property var pressedIntent: null
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: !root.editingLocked
                text: qsTr("结束编辑并采用本次新审阅")
                enabled: root.ready && root.newReviewCurrent && root.adoptionConfirmed && root.newTargetId > 0
                focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45; Accessible.name: text
                contentItem: QGCLabel { text: acceptReviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: acceptReviewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: acceptReviewButton.activeFocus }
                onPressed: pressedIntent = root.reviewActionIntent("accept")
                onCanceled: pressedIntent = null
                onClicked: { var intent = pressedIntent; pressedIntent = null; root.runReviewAction("accept", intent) }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCButton {
                id: requestReviewButton
                property var pressedIntent: null
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                text: qsTr("为本次新审阅申请批准")
                enabled: root.ready && root.editingLocked && root.newReviewCurrent && root.newReview.approvalAllowed === true && root.approvalState === "preview"
                focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45; Accessible.name: text
                contentItem: QGCLabel { text: requestReviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: requestReviewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: requestReviewButton.activeFocus }
                onPressed: pressedIntent = root.reviewActionIntent("request")
                onCanceled: pressedIntent = null
                onClicked: { var intent = pressedIntent; pressedIntent = null; root.runReviewAction("request", intent) }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCButton {
                id: approveReviewButton
                property var pressedIntent: null
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                text: qsTr("批准本次新审阅（不上传、不起飞）")
                enabled: root.ready && root.editingLocked && root.newReviewCurrent && root.newReview.approvalAllowed === true && root.approvalState === "awaiting-approval"
                focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45; Accessible.name: text
                contentItem: QGCLabel { text: approveReviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: approveReviewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: approveReviewButton.activeFocus }
                onPressed: pressedIntent = root.reviewActionIntent("approve")
                onCanceled: pressedIntent = null
                onClicked: { var intent = pressedIntent; pressedIntent = null; root.runReviewAction("approve", intent) }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCButton {
                id: reeditButton
                property var pressedIntent: null
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                text: qsTr("重新编辑（撤销本次新审阅上下文）")
                enabled: !root.callBusy && root.state.busy !== true
                focusPolicy: Qt.StrongFocus; opacity: enabled ? 1 : 0.45; Accessible.name: text
                contentItem: QGCLabel { text: reeditButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: reeditButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: 2; border.color: qgcPal.buttonHighlight; visible: reeditButton.activeFocus }
                onPressed: pressedIntent = root.reviewActionIntent("reedit")
                onCanceled: pressedIntent = null
                onClicked: { var intent = pressedIntent; pressedIntent = null; root.runReviewAction("reedit", intent) }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("采用和批准只改变本地审阅流程。即使本地动作已提交，版本过期、占用、权限或证据不足仍可能阻止后续步骤；不自动重试，也不删除历史记录。") }
        }
    }
}

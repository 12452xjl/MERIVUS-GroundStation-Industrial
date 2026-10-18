import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// One complete, operator-authored group proposal and one shared current
// review. Never sequential single-member reviews or an onboard reassignment.
ColumnLayout {
    id: root
    property var controller: null
    property bool inputSettled: false // Excludes this panel's own pending.
    property bool expanded: false
    property bool active: false
    property bool callBusy: false
    property var callTicket: null
    property var lifetime: ({alive: true, epoch: 0, ticket: null})
    property string ownedCaptureToken: ""
    property string ownedProposalToken: ""
    property string reviewedProposalToken: ""
    property var editors: ({})
    property string reserveText: "[]"
    property string reservePlansText: "[]"
    property int memberIndex: 0
    property int sourcePage: 0
    property int detailMode: 0
    property int detailPage: 0
    property string message: ""
    property bool captureConsent: false
    property bool previewConsent: false
    property bool reviewConsent: false
    property var adoptionConsent: null
    signal reviewInvalidationRequested()
    signal focusVisibilityRequested(var item)

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var state: controller ? controller.fleetMissionRemainder || {} : ({})
    readonly property var capture: state.capture || ({})
    readonly property var proposal: state.proposal || ({})
    readonly property var reviewStatus: state.review && state.review.kind === "group" ? state.review : ({})
    readonly property var archive: controller ? (controller.fleetMissionRemainderArchive || {}).archive || {} : ({})
    readonly property string archiveToken: archive.token || ""
    readonly property string episodeToken: controller && controller.fleetDeploymentLifecycle
        ? controller.fleetDeploymentLifecycle.currentToken || "" : ""
    readonly property var sharedReview: controller ? controller.fleetMissionRemainderReview || {} : ({})
    readonly property var newReview: sharedReview.kind === "group" ? sharedReview : ({})
    readonly property string reviewToken: newReview.token || ""
    readonly property var planning: controller ? controller.missionPlanningStatus || {} : ({})
    readonly property bool ownPlanning: reviewStatus.planningPending === true && !!reviewStatus.requestId
        && reviewStatus.requestId === planning.requestId
    readonly property bool editingLocked: newReview.accepted === true
    readonly property bool captureCurrent: capture.kind === "group" && capture.current === true && !!ownedCaptureToken
        && capture.token === ownedCaptureToken
    readonly property bool proposalCurrent: captureCurrent && !!ownedProposalToken && proposal.token === ownedProposalToken
        && proposal.captureToken === ownedCaptureToken
    readonly property var members: capture.kind === "group" && capture.members && capture.members.length <= 32 ? capture.members : []
    readonly property int selectedIndex: Math.min(Math.max(0, memberIndex), Math.max(0, Math.max(members.length, viewMembers.length) - 1))
    readonly property var member: members.length ? members[selectedIndex] : ({})
    readonly property var editor: editors[String(member.sourceVehicleId)] || ({})
    readonly property var waypoints: member.waypoints && member.waypoints.length <= 2048 ? member.waypoints : []
    readonly property int sourcePageCount: Math.max(1, Math.ceil(waypoints.length / 8))
    readonly property int currentSourcePage: Math.min(Math.max(0, sourcePage), sourcePageCount - 1)
    readonly property bool pendingInput: active || callBusy || captureConsent || previewConsent || reviewConsent || !!adoptionConsent
    readonly property bool ready: visible && expanded && controller && lifetime.alive && inputSettled
        && !callBusy && !callTicket && state.busy !== true && planning.active !== true
        && lifetime.epoch < 9007199254740991
    readonly property string reviewIdentity: identity(newReview)
    readonly property bool adoptionCurrent: !!adoptionConsent && adoptionConsent.owner === controller
        && adoptionConsent.identity === reviewIdentity && adoptionConsent.epoch === lifetime.epoch
    readonly property var view: newReview.view || ({})
    readonly property bool viewCurrent: view.schemaVersion === 2 && view.planId === (newReview.archive || {}).planId
        && view.planId === (newReview.plan || {}).planId && view.flightCommandReleased === false && view.executable === false
        && view.members && view.members.length <= 32 && view.memberCount === view.members.length
    readonly property var viewMembers: viewCurrent ? view.members : []
    readonly property int viewMemberIndex: Math.min(selectedIndex, Math.max(0, viewMembers.length - 1))
    readonly property var viewMember: viewMembers.length ? viewMembers[viewMemberIndex] : ({})
    readonly property var detailRows: projectedRows()
    readonly property int detailPageCount: Math.max(1, Math.ceil(detailRows.length / 8))
    readonly property int currentDetailPage: Math.min(Math.max(0, detailPage), detailPageCount - 1)
    readonly property var definitions: [
        {key: "sourceWaypointIndex", label: qsTr("原出航起点索引（0基，含该点完整停留）"), min: 0, max: Math.max(0, waypoints.length - 1), integer: true},
        {key: "targetVehicleId", label: qsTr("新目标机ID（全组唯一，不自动选机）"), min: 1, max: 255, integer: true},
        {key: "startLat", label: qsTr("新起点纬度（°）"), min: -80, max: 80},
        {key: "startLon", label: qsTr("新起点经度（°）"), min: -180, max: 180},
        {key: "startAlt", label: qsTr("新起点AMSL海拔（m，非离地高）"), min: -1000, max: 20000},
        {key: "returnLat", label: qsTr("自身新返回点纬度（°）"), min: -80, max: 80},
        {key: "returnLon", label: qsTr("自身新返回点经度（°）"), min: -180, max: 180},
        {key: "returnAlt", label: qsTr("自身新返回点AMSL海拔（m）"), min: -1000, max: 20000},
        {key: "cruiseSpeedMetersPerSecond", label: qsTr("自身新水平速度（m/s，大于0）"), min: 0, max: 1000, positive: true},
        {key: "climbSpeedMetersPerSecond", label: qsTr("自身新爬升速度（m/s，大于0）"), min: 0, max: 1000, positive: true},
        {key: "descentSpeedMetersPerSecond", label: qsTr("自身新下降速度（m/s，大于0）"), min: 0, max: 1000, positive: true}
    ]
    spacing: pad
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function text(value) { return typeof value === "string" && value ? value.substring(0, 1024) : qsTr("未知 / 未提供") }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function decimal(value) { var s = String(value === undefined ? "" : value).trim(); return /^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(s) ? Number(s) : NaN }
    function boolText(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function exactMap(value, keys) {
        if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function finiteNumber(value, min, max) { return typeof value === "number" && isFinite(value) && value >= min && value <= max }
    function reservePoint(value) {
        return exactMap(value, ["coordinateSystem", "altitudeReference", "latitude", "longitude", "altitude"])
            && value.coordinateSystem === "geodetic" && value.altitudeReference === "amsl"
            && finiteNumber(value.latitude, -90, 90) && finiteNumber(value.longitude, -180, 180)
            && finiteNumber(value.altitude, -1000, 20000)
    }
    function reservePlansValue(reserves, requests) {
        var rows = [], targets = {}, declared = {}, byId = {}
        try { if (reservePlansText.length > 32768) throw "budget"; rows = JSON.parse(reservePlansText) }
        catch (error) { message = qsTr("新备用配置须为不超过32768字符的JSON数组；请修正格式，不能自动继承旧机配置。"); return null }
        if (!Array.isArray(rows) || rows.length !== reserves.length || rows.length > 32) {
            message = qsTr("每个新备用ID都须有且仅有一份完整配置；没有备用机时明确填写 []。"); return null
        }
        for (var t = 0; t < requests.length; ++t) targets[requests[t].targetVehicleId] = true
        for (var d = 0; d < reserves.length; ++d) declared[reserves[d]] = true
        for (var r = 0; r < rows.length; ++r) {
            var row = rows[r]
            if (!exactMap(row, ["vehicleId", "start", "returnTo", "reservePlan"]) || !integer(row.vehicleId, 1, 255)
                    || !declared[row.vehicleId] || byId[row.vehicleId] || !reservePoint(row.start) || !reservePoint(row.returnTo)) {
                message = qsTr("新备用配置第 %1 项须含唯一备用ID、自身起点和返点（完整geodetic / amsl数值），不得借用主机或旧机端点。").arg(r + 1); return null
            }
            var policy = row.reservePlan
            if (!exactMap(policy, ["schemaVersion", "replacementVehicleId", "standbyMode", "standbyDurationSeconds",
                    "standbyConsumptionFactor", "cruiseSpeedMetersPerSecond", "climbSpeedMetersPerSecond", "descentSpeedMetersPerSecond"])
                    || policy.schemaVersion !== 1 || !integer(policy.replacementVehicleId, 1, 255)
                    || !targets[policy.replacementVehicleId] || policy.replacementVehicleId === row.vehicleId
                    || ["ground-disarmed", "airborne-hold"].indexOf(policy.standbyMode) < 0
                    || !finiteNumber(policy.standbyDurationSeconds, 0, 86400)
                    || !finiteNumber(policy.standbyConsumptionFactor, 0, 10) || policy.standbyConsumptionFactor <= 0) {
                message = qsTr("备用 %1：请填写完整8字段策略，替补对象须为新执行机，待机方式／时长与正耗能系数须明确。").arg(row.vehicleId); return null
            }
            var rates = ["cruiseSpeedMetersPerSecond", "climbSpeedMetersPerSecond", "descentSpeedMetersPerSecond"]
            for (var s = 0; s < rates.length; ++s) if (!finiteNumber(policy[rates[s]], 0, 1000) || policy[rates[s]] <= 0) {
                message = qsTr("备用 %1：自身三种速度须为大于0且不超过1000的数值（m/s）；能力仍由后续当前审阅判断。").arg(row.vehicleId); return null
            }
            byId[row.vehicleId] = row
        }
        var ordered = []
        for (var i = 0; i < reserves.length; ++i) ordered.push(byId[reserves[i]])
        return ordered // Display validation only; the native planner remains authoritative.
    }
    function identity(value) {
        if (!value || value.kind !== "group" || !value.token) return ""
        var a = value.archive || {}, rows = (value.plan || {}).members || [], ids = []
        if (rows.length > 32) return ""
        for (var i = 0; i < rows.length; ++i) ids.push(rows[i].vehicleId)
        return JSON.stringify({token: value.token, proposalToken: value.proposalToken, recordId: a.recordId,
            recordRevision: a.recordRevision, planId: a.planId, contentHash: a.contentHash,
            reviewEvidenceHash: a.reviewEvidenceHash, storeEpoch: a.storeEpoch, targetVehicleIds: ids})
    }
    function retireConsents() { captureConsent = false; previewConsent = false; reviewConsent = false; adoptionConsent = null }
    function changed() {
        if (editingLocked || callBusy || lifetime.ticket) return
        if (lifetime.epoch < 9007199254740991) lifetime.epoch += 1
        ownedProposalToken = ""; reviewedProposalToken = ""; retireConsents()
        reviewInvalidationRequested()
    }
    function edit(key, value) {
        if (!captureCurrent || editingLocked || callBusy || lifetime.ticket) return
        var values = {}, all = {}, id = String(member.sourceVehicleId)
        for (var name in editor) values[name] = editor[name]
        for (var prior in editors) all[prior] = editors[prior]
        values[key] = value; all[id] = values; editors = all
        changed()
    }
    function select(index) {
        if (callBusy || lifetime.ticket || !integer(index, 0, Math.max(members.length, viewMembers.length) - 1)) return
        memberIndex = index; sourcePage = 0; detailPage = 0; adoptionConsent = null
    }
    function requestValue() {
        if (!members.length || members.length > 32) { message = qsTr("没有完整原成员集合。"); return null }
        var requests = [], seen = ({})
        for (var i = 0; i < members.length; ++i) {
            var original = members[i], values = editors[String(original.sourceVehicleId)] || {}, request = {sourceVehicleId: original.sourceVehicleId, operatorConfirmed: true}
            if (original.available !== true || !original.waypoints || original.waypoints.length < 1) {
                select(i); message = qsTr("原成员 %1 尚未提交或无受支持独立路线；不能静默排除它来生成联合任务。").arg(original.sourceVehicleId); return null
            }
            for (var d = 0; d < definitions.length; ++d) {
                var definition = definitions[d], value = decimal(values[definition.key])
                var maximum = definition.key === "sourceWaypointIndex" ? original.waypoints.length - 1 : definition.max
                if (!isFinite(value) || value < definition.min || value > maximum || (definition.positive && value <= 0)
                        || (definition.integer && Math.floor(value) !== value)) {
                    select(i); message = qsTr("原成员 %1：请修正“%2”，须为范围 %3 至 %4 内的有效数值%5。")
                        .arg(original.sourceVehicleId).arg(definition.label).arg(definition.min).arg(maximum).arg(definition.positive ? qsTr("且大于0") : ""); return null
                }
                request[definition.key] = value
            }
            if (seen[request.targetVehicleId]) { select(i); message = qsTr("新目标ID %1 在组内重复，未自动换机。").arg(request.targetVehicleId); return null }
            seen[request.targetVehicleId] = true
            var profileText = values.profileText || "", profile = ({})
            if (profileText.length > 16384) { select(i); message = qsTr("原成员 %1 的新执行配置超过16384字符。").arg(original.sourceVehicleId); return null }
            try { if (profileText.trim()) profile = JSON.parse(profileText) }
            catch (error) { select(i); message = qsTr("原成员 %1 的新执行配置不是JSON对象。").arg(original.sourceVehicleId); return null }
            if (!profile || typeof profile !== "object" || Array.isArray(profile)) { select(i); message = qsTr("执行配置必须为JSON对象。"); return null }
            request.start = {coordinateSystem: "geodetic", altitudeReference: "amsl", latitude: request.startLat, longitude: request.startLon, altitude: request.startAlt}
            request.returnTo = {coordinateSystem: "geodetic", altitudeReference: "amsl", latitude: request.returnLat, longitude: request.returnLon, altitude: request.returnAlt}
            var endpointKeys = ["startLat", "startLon", "startAlt", "returnLat", "returnLon", "returnAlt"]
            for (var e = 0; e < endpointKeys.length; ++e) delete request[endpointKeys[e]]
            request.executionProfile = profile
            requests.push(request)
        }
        var reserves = []
        try { if (reserveText.length > 1024) throw "budget"; reserves = JSON.parse(reserveText) }
        catch (error) { message = qsTr("新备用ID须为不超过1024字符的JSON数组，例如 [7,8]；没有默认取消原备用要求。"); return null }
        if (!Array.isArray(reserves) || reserves.length > 32) { message = qsTr("新备用ID必须为最多32项数组。"); return null }
        for (var r = 0; r < reserves.length; ++r) {
            if (!integer(reserves[r], 1, 255) || seen[reserves[r]]) { message = qsTr("新备用ID非法、重复或与新执行机冲突。"); return null }
            seen[reserves[r]] = true
        }
        var reservePlans = reservePlansValue(reserves, requests)
        if (reservePlans === null) return null
        return {members: requests, reserveVehicleIds: reserves, reservePlans: reservePlans, operatorConfirmed: true}
    }
    function intent(action) {
        return {action: action, owner: controller, epoch: lifetime.epoch, archiveToken: archiveToken, episodeToken: episodeToken,
            captureToken: capture.kind === "group" ? capture.token || "" : "", proposalToken: ownedProposalToken,
            reviewToken: reviewToken, reviewIdentity: reviewIdentity}
    }
    function current(value) {
        return value && lifetime.alive && value.owner === controller && value.epoch === lifetime.epoch && visible && expanded
            && lifetime.epoch < 9007199254740991 && value.episodeToken === episodeToken
            && (value.action === "exit" || value.action === "cancel" || inputSettled)
            && value.archiveToken === archiveToken
            && value.captureToken === (capture.kind === "group" ? capture.token || "" : "")
    }
    function perform(value) {
        if (!ready || lifetime.ticket || !current(value) || editingLocked || ["capture", "preview", "review"].indexOf(value.action) < 0) return
        if (value.action === "capture" && (!captureConsent || archive.schemaVersion !== 2 || capture.kind === "group"
                || capture.sourceKind === "historical-run-planning-snapshot")) return
        if (value.action !== "capture" && !captureCurrent) return
        if (value.action === "preview" && !previewConsent) return
        if (value.action === "review" && (!reviewConsent || !proposalCurrent || value.proposalToken !== ownedProposalToken
                || reviewedProposalToken === ownedProposalToken)) return
        // Validation may publish feedback, which can synchronously remove
        // this panel. Keep the plain lifetime before entering that path.
        var guard = lifetime
        var request = value.action === "preview" ? requestValue() : null
        if (!guard.alive) return
        if (value.action === "preview" && !request) { errorFocus(); return }
        var ticket = {}, result = null
        guard.ticket = ticket; callTicket = ticket
        if (!guard.alive) return
        callBusy = true
        if (!guard.alive) return
        retireConsents()
        try {
            if (!guard.alive || !current(value) || guard.ticket !== ticket) return
            if (value.action === "capture") result = value.owner.captureFleetMissionGroupRemainderArchive(value.archiveToken, true)
            else if (value.action === "preview") result = value.owner.previewFleetMissionGroupRemainder(value.captureToken, request)
            else result = value.owner.reviewFleetMissionRemainder(value.proposalToken, true)
            if (!guard.alive) return
            var same = value.owner === controller && value.epoch === guard.epoch && archiveToken === value.archiveToken && visible && expanded
            if (!result || typeof result.ok !== "boolean" || typeof result.current !== "boolean" || typeof result.committed !== "boolean"
                    || result.flightCommandReleased !== false) { message = qsTr("回执不完整，不能断言本地动作未发生；请核对记录，不自动重试。"); return }
            if (!same || result.current !== true) { message = qsTr("上下文已变化；本地提交已发生：%1。请核对记录 %2，不沿用旧确认。").arg(boolText(result.committed)).arg(text(result.recordId)); return }
            if (result.ok !== true) { message = qsTr("未完成：%1；本地提交已发生：%2。全部输入保留。").arg(text(result.error)).arg(boolText(result.committed)); return }
            if (value.action === "capture") {
                if (capture.kind !== "group" || capture.current !== true || capture.token !== result.captureToken) { message = qsTr("返回来源不属于当前整组选择。"); return }
                ownedCaptureToken = result.captureToken; ownedProposalToken = ""; reviewedProposalToken = ""; active = true; memberIndex = 0
                message = qsTr("已选择完整历史组；逐成员输入保留，须重新核对，不会由旧任务自动填写目标、索引、端点或profile。")
            } else if (value.action === "preview") {
                if (!proposal.token || proposal.captureToken !== value.captureToken) { message = qsTr("联合预览与原来源不对应。"); return }
                ownedProposalToken = proposal.token; reviewedProposalToken = ""
                message = qsTr("完整联合提案已生成；尚未审阅。请核对全部成员与备用要求后单独提交一次联合审阅。")
            } else {
                reviewedProposalToken = value.proposalToken
                message = result.planningPending === true ? qsTr("已提交一次后台联合规划；尚无最终结果，可取消，不会逐架覆盖审阅或发命令。")
                    : qsTr("联合审阅已返回，记录 %1；记录存在不等于通过、上传或起飞。").arg(text(result.recordId))
            }
        } catch (error) { if (guard.alive) message = qsTr("本地操作回执中断；已发生的保存或审阅不因此回滚。请核对记录，输入保留，不自动重试。") }
        finally {
            if (guard.alive) callBusy = false
            if (guard.alive && callTicket === ticket) callTicket = null
            if (guard.ticket === ticket) guard.ticket = null
        }
        if (guard.alive && value.owner === controller && value.epoch === guard.epoch && value.episodeToken === episodeToken && message) errorFocus()
    }
    function reviewAction(value) {
        var clearing = value && value.action === "exit"
        if (!value || !current(value) || callBusy || lifetime.ticket || state.busy === true) return
        if (!clearing && (!ready || !reviewToken || value.reviewToken !== reviewToken || value.reviewIdentity !== reviewIdentity || newReview.current !== true)) return
        if (value.action === "accept" && (!adoptionCurrent || editingLocked)) return
        if (value.action === "request" && (!editingLocked || newReview.approvalAllowed !== true || controller.intentTaskState !== "preview")) return
        if (value.action === "approve" && (!editingLocked || newReview.approvalAllowed !== true || controller.intentTaskState !== "awaiting-approval")) return
        if (["accept", "request", "approve", "exit"].indexOf(value.action) < 0) return
        if (clearing && (value.reviewToken !== reviewToken || value.reviewIdentity !== reviewIdentity)) return
        var guard = lifetime, ticket = {}, result = null
        guard.ticket = ticket; callTicket = ticket
        if (!guard.alive) return
        callBusy = true
        if (!guard.alive) return
        retireConsents()
        try {
            if (!guard.alive || !current(value) || guard.ticket !== ticket || value.reviewToken !== reviewToken || value.reviewIdentity !== reviewIdentity) return
            if (clearing) result = value.reviewToken ? value.owner.clearFleetMissionRemainderReview(value.reviewToken)
                : value.captureToken ? value.owner.clearFleetMissionRemainder(value.captureToken) : {ok: true, current: true, committed: false, flightCommandReleased: false}
            else if (value.action === "accept") result = value.owner.acceptFleetMissionRemainderReview(value.reviewToken, true)
            else if (value.action === "request") result = value.owner.requestFleetMissionRemainderApproval(value.reviewToken)
            else result = value.owner.approveFleetMissionRemainder(value.reviewToken)
            if (!guard.alive) return
            if (!result || typeof result.ok !== "boolean" || typeof result.current !== "boolean" || typeof result.committed !== "boolean"
                    || result.flightCommandReleased !== false) { message = qsTr("未取得完整动作回执；请核对归档和当前状态，不自动重试。"); return }
            var same = value.owner === controller && value.epoch === guard.epoch && visible && expanded && archiveToken === value.archiveToken
            if (clearing && same && result.ok === true && newReview.accepted !== true
                    && capture.kind !== "group" && (!reviewToken || reviewToken === value.reviewToken)) {
                active = false; ownedCaptureToken = ""; ownedProposalToken = ""; reviewedProposalToken = ""
                message = qsTr("已退出联合本地流程，逐成员文字保留；归档、原Run、飞机动作和占用不变。再次使用须重新明确取源。")
            } else if (!same || !result.current || value.reviewToken !== reviewToken || value.reviewIdentity !== reviewIdentity) {
                message = qsTr("返回时来源已变化；已发生本地动作：%1。请检查记录，不恢复旧确认。").arg(boolText(result.committed))
            } else if (!result.ok) message = qsTr("动作未完成：%1；本地动作已发生：%2。").arg(text(result.error)).arg(boolText(result.committed))
            else { active = newReview.accepted !== true; message = qsTr("整组本地动作已返回：%1。采用／批准不等于上传、启动或接管；后续只走唯一共享部署区。").arg(value.action) }
        } catch (error) { if (guard.alive) message = qsTr("本地动作回执中断；请核对已发生的采用／批准，不能当作回滚。") }
        finally {
            if (guard.alive && newReview.accepted === true) active = false
            if (guard.alive) callBusy = false
            if (guard.alive && callTicket === ticket) callTicket = null
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function errorFocus() {
        var guard = lifetime, epoch = guard.epoch, owner = controller
        if (!guard.alive || !visible || !expanded) return
        feedback.forceActiveFocus()
        if (guard.alive && owner === controller && epoch === guard.epoch && visible && expanded) focusVisibilityRequested(feedback)
    }
    function cancelIntent() {
        if (!ownPlanning || callBusy || lifetime.ticket || !controller) return null
        var value = intent("cancel")
        value.requestId = reviewStatus.requestId
        value.pendingProposalToken = reviewStatus.proposalToken
        return value
    }
    function cancelPlanning(value) {
        if (!current(value) || value.action !== "cancel" || !ownPlanning || callBusy || lifetime.ticket
                || value.requestId !== reviewStatus.requestId || value.pendingProposalToken !== reviewStatus.proposalToken) return
        var guard = lifetime, ticket = {}
        guard.ticket = ticket; callTicket = ticket
        if (!guard.alive) return
        callBusy = true
        if (!guard.alive) return
        try {
            if (guard.alive && current(value) && ownPlanning && guard.ticket === ticket
                    && value.requestId === reviewStatus.requestId && value.pendingProposalToken === reviewStatus.proposalToken)
                value.owner.cancelMissionPlanning(value.requestId)
        } finally {
            if (guard.alive) callBusy = false
            if (guard.alive && callTicket === ticket) callTicket = null
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function projectedRows() {
        if (!viewCurrent || !viewMembers.length) return []
        if (detailMode === 2) return view.encodingAvailable === true && viewMember.encodedItems ? viewMember.encodedItems : []
        if (view.geometryAvailable !== true || !viewMember.routes || viewMember.routes.length !== 2) return []
        var path = detailMode === 0 ? "waypoints" : "returnPath"
        for (var i = 0; i < viewMember.routes.length; ++i) if (viewMember.routes[i].sourcePath === path) return viewMember.routes[i].points || []
        return []
    }
    function localRetire() {
        if (lifetime.epoch < 9007199254740991) lifetime.epoch += 1
        retireConsents(); ownedProposalToken = ""; reviewedProposalToken = ""
    }
    onControllerChanged: { localRetire(); active = false; ownedCaptureToken = "" }
    onVisibleChanged: if (!visible) localRetire()
    onExpandedChanged: if (!expanded) localRetire()
    onArchiveTokenChanged: localRetire()
    onInputSettledChanged: localRetire()
    onEpisodeTokenChanged: localRetire()
    onReviewIdentityChanged: adoptionConsent = null
    onEditingLockedChanged: if (editingLocked) { active = false; retireConsents() }
    Component.onDestruction: { lifetime.alive = false }

    QGCButton {
        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
        text: root.expanded ? qsTr("收起 · 多成员联合余段新任务") : qsTr("展开 · 多成员联合余段新任务")
        onClicked: root.expanded = !root.expanded
    }
    ColumnLayout {
        Layout.fillWidth: true; visible: root.expanded; spacing: root.pad
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("先在上方完整历史资料入口打开或捕获v2整组包。这里必须覆盖全部原成员，每个成员明确填新机、原索引、起返点、速度和新配置；未提交成员不能被静默删除。只生成一个联合新任务并做一次当前审阅，不恢复原任务、控制权或已完成航段。收起保留本地流程，明确退出才释放编辑。")
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("当前资料包 %1 · 原Run %2 · 原成员 %3。只接受v2完整包，不从旧单成员包补组。").arg(root.text(root.archive.archiveId)).arg(root.text(root.archive.runId)).arg(root.archive.members ? root.archive.members.length : 0) }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            checked: root.captureConsent; enabled: root.ready && !root.editingLocked && root.archive.schemaVersion === 2 && root.capture.kind !== "group"
            text: qsTr("确认使用当前完整组资料建立新的联合人工编辑上下文，并撤销旧本地审阅；不恢复原许可")
            onClicked: root.captureConsent = checked
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("明确选择整组资料（不自动填入参数）"); enabled: root.ready && root.captureConsent && !root.editingLocked
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("capture")
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("明确退出联合编辑／撤销本地采用（保留文字和归档）"); enabled: !root.callBusy && root.state.busy !== true
            property var pressedIntent: null
            onPressed: pressedIntent = root.intent("exit")
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.reviewAction(value) }
        }
        QGCLabel { Layout.fillWidth: true; visible: !root.inputSettled; text: qsTr("请先保存、取消或明确退出其它编辑流程；本页退出仍可用。"); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
        RowLayout {
            Layout.fillWidth: true; visible: root.members.length > 0 || root.viewMembers.length > 0
            QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("上一成员"); enabled: root.selectedIndex > 0 && !root.callBusy; onClicked: root.select(root.selectedIndex - 1) }
            QGCLabel { Layout.fillWidth: true; text: qsTr("成员 %1 / %2 · 原ID %3 · 角色 %4").arg(root.selectedIndex + 1).arg(Math.max(root.members.length, root.viewMembers.length)).arg(root.member.sourceVehicleId || root.viewMember.sourceVehicleId || "?").arg(root.member.role || root.viewMember.role || "?"); wrapMode: Text.WordWrap }
            QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("下一成员"); enabled: root.selectedIndex + 1 < Math.max(root.members.length, root.viewMembers.length) && !root.callBusy; onClicked: root.select(root.selectedIndex + 1) }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.captureCurrent; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("原ID %1 · 可取源 %2 · 来源当时有效 %3 · 连续性未知 %4 · 观察current %5 / reached %6。人工索引不是sequence+1，选定航点全部停留会重做。%7").arg(root.member.sourceVehicleId).arg(root.boolText(root.member.available)).arg(root.boolText(root.member.sourceCurrent)).arg(root.boolText(root.member.continuityUnknown)).arg(root.member.currentSequence === null ? "?" : root.member.currentSequence).arg(root.member.reachedSequence === null ? "?" : root.member.reachedSequence).arg(root.member.reason || "") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("原出航路线 · 第 %1 / %2 页（每页8点，AMSL）").arg(root.currentSourcePage + 1).arg(root.sourcePageCount); wrapMode: Text.WordWrap }
            Repeater {
                model: root.waypoints.slice(root.currentSourcePage * 8, root.currentSourcePage * 8 + 8)
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("原索引 %1：%2°, %3°，AMSL %4m；完整停留 %5s").arg(root.currentSourcePage * 8 + index).arg(modelData.latitude).arg(modelData.longitude).arg(modelData.altitude).arg(modelData.holdSeconds === undefined ? 0 : modelData.holdSeconds) }
            }
            RowLayout {
                Layout.fillWidth: true
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("原点上一页"); enabled: root.currentSourcePage > 0; onClicked: root.sourcePage = root.currentSourcePage - 1 }
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("原点下一页"); enabled: root.currentSourcePage + 1 < root.sourcePageCount; onClicked: root.sourcePage = root.currentSourcePage + 1 }
            }
            Repeater {
                model: root.definitions
                ColumnLayout {
                    Layout.fillWidth: true
                    QGCLabel { Layout.fillWidth: true; text: modelData.label; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    QGCTextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: root.editor[modelData.key] || ""; enabled: !root.editingLocked && !root.callBusy; Accessible.name: modelData.label; onTextEdited: root.edit(modelData.key, text); onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
                }
            }
            QGCLabel { Layout.fillWidth: true; text: qsTr("此成员的新执行配置JSON（空对象不继承原配置，最多16384字符）"); wrapMode: Text.WordWrap }
            Controls.TextArea { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight * 3; text: root.editor.profileText || ""; enabled: !root.editingLocked && !root.callBusy; selectByMouse: true; wrapMode: TextEdit.Wrap; color: qgcPal.text; Accessible.name: qsTr("当前成员新执行配置JSON"); onTextChanged: if (activeFocus && text !== (root.editor.profileText || "")) root.edit("profileText", text); onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this) }
            QGCLabel { Layout.fillWidth: true; text: qsTr("明确新备用ID数组（JSON，如 [7,8]；不自动取消原最低备用数量或其它政策）"); wrapMode: Text.WordWrap }
            QGCTextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: root.reserveText; enabled: !root.editingLocked && !root.callBusy; onTextEdited: { root.reserveText = text; root.changed() }; Accessible.name: qsTr("新备用机ID数组") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("完整新备用配置JSON（最多32768字符，每个ID恰一项）：vehicleId、start、returnTo、reservePlan。两端点须为自身完整geodetic / amsl坐标；策略须含schemaVersion=1、replacementVehicleId（新执行机）、standbyMode、standbyDurationSeconds、standbyConsumptionFactor及cruise / climb / descentSpeedMetersPerSecond。显式替换旧备用运行配置，绝不自动重绑旧机号或端点；仍须全新能力／能量审阅。") }
            Controls.TextArea {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight * 3
                text: root.reservePlansText; enabled: !root.editingLocked && !root.callBusy
                selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: qgcPal.text
                Accessible.name: qsTr("完整新备用配置JSON，显式替换旧备用运行配置")
                onTextChanged: if (activeFocus && text !== root.reservePlansText) {
                    var guard = root.lifetime
                    root.reservePlansText = text
                    if (guard.alive) root.changed()
                }
                onActiveFocusChanged: if (activeFocus) root.focusVisibilityRequested(this)
            }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; checked: root.previewConsent; enabled: root.ready && !root.editingLocked; text: qsTr("已核对全部原成员及每个新目标、索引、起返点、速度和配置，并明确确认替换全部备用运行配置，生成完整联合提案"); onClicked: root.previewConsent = checked; contentItem: QGCLabel { text: parent.text; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("生成完整联合新提案（尚不审阅）"); enabled: root.ready && root.captureCurrent && root.previewConsent && !root.editingLocked; property var pressedIntent: null; onPressed: pressedIntent = root.intent("preview"); onCanceled: pressedIntent = null; onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) } }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: root.proposalCurrent; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; text: qsTr("联合预览：%1个执行成员；新目标 %2；明确新备用 %3。未核实净空、性能或完成状态。").arg((root.proposal.summary || {}).memberCount || 0).arg(JSON.stringify((root.proposal.summary || {}).targetVehicleIds || [])).arg(JSON.stringify((root.proposal.summary || {}).reserveVehicleIds || [])); wrapMode: Text.WordWrap }
            Repeater { model: (root.proposal.summary || {}).members || []; QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("原 %1 → 新 %2，角色 %3；原起点索引 %4；保留 %5点；任务时长下限 %6s，返航下限 %7s，自身最低速度 %8m/s。").arg(modelData.sourceVehicleId).arg(modelData.targetVehicleId).arg(modelData.targetRole).arg(modelData.sourceWaypointIndex).arg(modelData.retainedWaypointCount).arg(modelData.missionDurationFloorSeconds).arg(modelData.returnReserveFloorSeconds).arg(modelData.minimumSpeedMps) } }
            Repeater {
                model: (root.proposal.provenance || {}).schemaVersion === 3 ? root.proposal.provenance.reservePlans || [] : []
                QGCLabel {
                    Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("新备用 %1 → 新执行机 %2\n自身起点 %3°, %4°, AMSL %5m；自身返点 %6°, %7°, AMSL %8m\n待机 %9 / %10s，耗能系数 %11；自身巡航 / 爬升 / 下降 %12 / %13 / %14 m/s。仅为显式新计划，未确认实际待命、接替或飞行许可。")
                        .arg(modelData.vehicleId).arg(modelData.reservePlan.replacementVehicleId)
                        .arg(modelData.start.latitude).arg(modelData.start.longitude).arg(modelData.start.altitude)
                        .arg(modelData.returnTo.latitude).arg(modelData.returnTo.longitude).arg(modelData.returnTo.altitude)
                        .arg(modelData.reservePlan.standbyMode).arg(modelData.reservePlan.standbyDurationSeconds)
                        .arg(modelData.reservePlan.standbyConsumptionFactor).arg(modelData.reservePlan.cruiseSpeedMetersPerSecond)
                        .arg(modelData.reservePlan.climbSpeedMetersPerSecond).arg(modelData.reservePlan.descentSpeedMetersPerSecond)
                }
            }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; checked: root.reviewConsent; enabled: root.ready && !root.editingLocked; text: qsTr("确认对上述全部目标和同一联合提案做一次新的当前能力／能量／空域／风险审阅"); onClicked: root.reviewConsent = checked; contentItem: QGCLabel { text: parent.text; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("提交一次完整联合审阅"); enabled: root.ready && root.proposalCurrent && root.reviewConsent && root.reviewedProposalToken !== root.ownedProposalToken && !root.editingLocked; property var pressedIntent: null; onPressed: pressedIntent = root.intent("review"); onCanceled: pressedIntent = null; onClicked: { var value = pressedIntent; pressedIntent = null; root.perform(value) } }
        }
        QGCButton {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: root.ownPlanning
            text: qsTr("取消本次联合后台计算（不取消飞机任务）")
            property var pressedIntent: null
            onPressed: pressedIntent = root.cancelIntent()
            onCanceled: pressedIntent = null
            onClicked: { var value = pressedIntent; pressedIntent = null; root.cancelPlanning(value) }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: !!root.reviewToken; spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("联合审阅记录 %1\n计划 %2\n当前 %3 · 可行 %4 · 仍需复核 %5\n新目标 %6\n原因：%7").arg(root.text((root.newReview.archive || {}).recordId)).arg(root.text((root.newReview.plan || {}).planId)).arg(root.boolText(root.newReview.current)).arg(root.boolText((root.newReview.plan || {}).feasible)).arg(root.boolText((root.newReview.review || {}).needsReview)).arg(JSON.stringify(((root.newReview.plan || {}).members || []).map(function(row) { return row.vehicleId }))).arg(JSON.stringify(root.reviewStatus.reasons || [])) }
            QGCLabel { Layout.fillWidth: true; visible: !root.viewCurrent || root.view.geometryAvailable !== true || root.view.encodingAvailable !== true; text: qsTr("完整组显示状态：几何 %1（%2）；编码 %3（%4）。没有完整编码时不显示成功前缀。").arg(root.boolText(root.view.geometryAvailable)).arg(root.text(root.view.geometryError)).arg(root.boolText(root.view.encodingAvailable)).arg(root.text((root.view.encodingError || {}).errorCode)); wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true; visible: root.viewCurrent
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("新出航点"); onClicked: { root.detailMode = 0; root.detailPage = 0 } }
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("新返程点"); onClicked: { root.detailMode = 1; root.detailPage = 0 } }
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("完整协议项"); onClicked: { root.detailMode = 2; root.detailPage = 0 } }
            }
            QGCLabel { Layout.fillWidth: true; visible: root.viewCurrent; text: qsTr("所选新目标 %1（原 %2）· %3项 · 第%4/%5页。协议索引与航点索引不同；AMSL不是离地高。").arg(root.viewMember.vehicleId || "?").arg(root.viewMember.sourceVehicleId || "?").arg(root.detailRows.length).arg(root.currentDetailPage + 1).arg(root.detailPageCount); wrapMode: Text.WordWrap }
            Repeater { model: root.detailRows.slice(root.currentDetailPage * 8, root.currentDetailPage * 8 + 8); QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: JSON.stringify(modelData) } }
            RowLayout {
                Layout.fillWidth: true; visible: root.viewCurrent
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("明细上一页"); enabled: root.currentDetailPage > 0; onClicked: root.detailPage = root.currentDetailPage - 1 }
                QGCButton { Layout.minimumHeight: root.rowHeight; text: qsTr("明细下一页"); enabled: root.currentDetailPage + 1 < root.detailPageCount; onClicked: root.detailPage = root.currentDetailPage + 1 }
            }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; enabled: root.ready && root.newReview.current === true && !root.editingLocked; checked: root.adoptionCurrent; text: qsTr("已核对完整目标组和此记录，确认结束编辑并采用同一联合新审阅；采用不是审阅通过或执行许可"); onClicked: root.adoptionConsent = checked ? {owner: root.controller, epoch: root.lifetime.epoch, identity: root.reviewIdentity} : null; contentItem: QGCLabel { text: parent.text; wrapMode: Text.WordWrap; leftPadding: (parent.indicator ? parent.indicator.width : 0) + root.pad } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("采用完整联合新审阅"); enabled: root.ready && root.adoptionCurrent && !root.editingLocked; property var pressedIntent: null; onPressed: pressedIntent = root.intent("accept"); onCanceled: pressedIntent = null; onClicked: { var value = pressedIntent; pressedIntent = null; root.reviewAction(value) } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("为完整联合新审阅申请本地批准"); enabled: root.ready && root.editingLocked && root.newReview.approvalAllowed === true && root.controller.intentTaskState === "preview"; property var pressedIntent: null; onPressed: pressedIntent = root.intent("request"); onCanceled: pressedIntent = null; onClicked: { var value = pressedIntent; pressedIntent = null; root.reviewAction(value) } }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("本地批准完整联合新审阅（仍不上传／启动）"); enabled: root.ready && root.editingLocked && root.newReview.approvalAllowed === true && root.controller.intentTaskState === "awaiting-approval"; property var pressedIntent: null; onPressed: pressedIntent = root.intent("approve"); onCanceled: pressedIntent = null; onClicked: { var value = pressedIntent; pressedIntent = null; root.reviewAction(value) } }
        }
        QGCLabel { id: feedback; Layout.fillWidth: true; activeFocusOnTab: true; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText; text: root.message || qsTr("全部输入只在本页保留。未编译／运行验证；不承诺自动自愈、接管或原任务恢复。") }
    }
}

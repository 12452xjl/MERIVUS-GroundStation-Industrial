import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts   1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// A local proposal editor with a separate grounded staging surface, never a
// start/arm/flight-command surface. The supplied
// controller performs version-bound review; the supplied draft facade owns
// canonical data. Text still being edited is intentionally separate from that
// data, invalidates the old preview, and cannot silently fall back to old data.
FocusScope {
    id: root

    property var draft: null
    property var controller: null
    property var selectedVehicleIds: []
    property bool mapPicking: false
    property bool previewCurrent: false
    property var reviewResult: ({})

    signal closeRequested()
    signal mapPickingRequested(bool enabled)
    signal previewRequested()

    readonly property var vertices: draft && draft.vertices ? draft.vertices : []
    readonly property var members: draft && draft.members ? draft.members : []
    readonly property var settings: draft && draft.settings ? draft.settings : ({})
    readonly property real rowHeight: Math.max(36, ScreenTools.defaultFontPixelHeight * 2.4)
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property var review: reviewResult && reviewResult.missionReview ? reviewResult.missionReview : ({})
    readonly property var plan: reviewResult && reviewResult.plan ? reviewResult.plan : (review.plan || {})
    readonly property var archive: reviewResult && reviewResult.planArchive ? reviewResult.planArchive : ({})
    readonly property var planningStatus: controller && controller.missionPlanningStatus
        ? controller.missionPlanningStatus : ({})
    readonly property bool planningBusy: planningStatus.active === true || planningStatus.ready === true
        || planningStatus.status === "completing"
    readonly property var remainderReviewContext: controller && controller.fleetMissionRemainderReview
        ? controller.fleetMissionRemainderReview : ({})
    // Explicit acceptance chooses the one shared staging surface. Expiry
    // disables this selected source; it never falls back to an old draft.
    readonly property bool remainderDeploymentSelected: remainderReviewContext.accepted === true
    readonly property string deploymentSourceKind: remainderDeploymentSelected
        ? (remainderReviewContext.kind === "reserve-dispatch" ? "reserve-dispatch"
            : remainderReviewContext.kind === "single" || remainderReviewContext.kind === "group" ? "remainder" : "unknown") : "draft"
    readonly property string deploymentContextToken: remainderDeploymentSelected
        ? (typeof remainderReviewContext.token === "string" ? remainderReviewContext.token : "")
        : JSON.stringify(["draft", draftSelection, draft ? draft.revision : -1, archive.recordId || "", archive.recordRevision])
    readonly property var deploymentArchive: remainderDeploymentSelected ? remainderReviewContext.archive || ({}) : archive
    readonly property var deploymentPlan: remainderDeploymentSelected ? remainderReviewContext.plan || ({}) : plan
    readonly property var deploymentEncoding: remainderDeploymentSelected ? remainderReviewContext.encoding || ({})
        : review.transportEncoding || reviewResult.transportEncoding || ({})
    readonly property bool deploymentPreviewCurrent: remainderDeploymentSelected
        ? remainderReviewContext.current === true : previewCurrent
    readonly property bool deploymentApprovalAllowed: remainderDeploymentSelected
        ? remainderReviewContext.approvalAllowed === true : !!controller && controller.missionDraftApprovalAllowed === true
    readonly property var deploymentRefreshVehicleIds: memberIdsForRefresh(remainderDeploymentSelected
        ? deploymentPlan.members : members)
    readonly property var energyPlan: review && review.energyPlan ? review.energyPlan : ({})
    readonly property var energyBillRows: boundedEnergyAssignments(energyPlan.assignments)
    property bool energyBillExpanded: false
    property int energyBillPage: 0
    readonly property int energyBillPageCount: Math.max(1, Math.ceil((energyBillRows ? energyBillRows.length : 0) / 8))
    readonly property int energyBillCurrentPage: Math.min(Math.max(0, energyBillPage), energyBillPageCount - 1)
    onEnergyPlanChanged: { energyBillPage = 0; energyBillExpanded = false }
    readonly property var reviewBlockers: blockers()
    readonly property bool reviewApprovalAvailable: !remainderDeploymentSelected && previewCurrent
        && reviewResult.planArchiveRecorded === true && review.needsReview === false
        && controller && controller.missionDraftApprovalAllowed === true
        && !hasPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !planningBusy
    readonly property var activeMember: memberById(activeMemberId)
    readonly property bool activeCoverageMember: activeMember
        && (activeMember.role === "scout" || activeMember.role === "leader")
    readonly property bool activeAuxiliaryMember: activeMember
        && (activeMember.role === "relay" || activeMember.role === "responder")
    readonly property bool activeReserveMember: activeMember && activeMember.role === "reserve"
    readonly property var reserveTargets: reserveTargetChoices()
    readonly property bool hasOtherPendingCoordinates: editingVertex >= 0
        || vertexTexts.lat.length > 0 || vertexTexts.lon.length > 0
        || endpointDirty || auxiliaryInputPending
        || executionProfileEditor.pendingInput || reserveEditing || reserveClearSession !== null
    readonly property bool hasPendingCoordinates: hasOtherPendingCoordinates || remainderPanel.pendingInput || groupRemainderPanel.pendingInput
        || reserveDispatchPanel.pendingInput
        || exclusionsEditor.pendingInput || performancePolicyEditor.pendingInput
    readonly property bool hasPendingSettingText: Object.keys(settingTexts).length > 0
    readonly property bool inputSettled: !hasPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear
    // The child owns its own pending input: excluding it here prevents a
    // self-deadlock while the normal area preview/approval remains blocked.
    readonly property bool remainderInputSettled: !hasOtherPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !draftSwitchBusy && !groupRemainderPanel.pendingInput && !exclusionsEditor.pendingInput
        && !performancePolicyEditor.pendingInput && !reserveDispatchPanel.pendingInput
    readonly property bool groupRemainderInputSettled: !hasOtherPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !draftSwitchBusy && !remainderPanel.pendingInput && !exclusionsEditor.pendingInput
        && !performancePolicyEditor.pendingInput && !reserveDispatchPanel.pendingInput
    readonly property bool reserveDispatchInputSettled: !hasOtherPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !draftSwitchBusy
        && !remainderPanel.pendingInput && !groupRemainderPanel.pendingInput
        && !exclusionsEditor.pendingInput && !performancePolicyEditor.pendingInput
    readonly property bool exclusionsInputSettled: !hasOtherPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !draftSwitchBusy && !planningBusy
        && !remainderPanel.pendingInput && !groupRemainderPanel.pendingInput
        && !performancePolicyEditor.pendingInput && !reserveDispatchPanel.pendingInput
    readonly property bool performancePolicyInputSettled: !hasOtherPendingCoordinates && !hasPendingSettingText && !mapPicking
        && replacementRoster.length === 0 && !confirmClear && !draftSwitchBusy && !planningBusy
        && !remainderPanel.pendingInput && !groupRemainderPanel.pendingInput && !exclusionsEditor.pendingInput
        && !reserveDispatchPanel.pendingInput
    readonly property double draftSelection: controller && controller.fleetMissionRepairDraft
        && typeof controller.fleetMissionRepairDraft.draftSelection === "number" ? controller.fleetMissionRepairDraft.draftSelection : -1
    readonly property bool draftSwitchBusy: controller && controller.fleetMissionRepairDraft
        && controller.fleetMissionRepairDraft.busy === true
    property var draftDisplayRefresh: ({alive: true, queued: null, pending: null})
    onDraftSelectionChanged: { resetReserveConsent(); captureDraftDisplayRefresh() }
    onControllerChanged: { resetReserveConsent(); captureDraftDisplayRefresh() }
    onDraftChanged: { resetReserveConsent(); captureDraftDisplayRefresh() }
    onDraftSwitchBusyChanged: if (!draftSwitchBusy) scheduleDraftDisplayRefresh()
    onInputSettledChanged: if (inputSettled) scheduleDraftDisplayRefresh()
    Component.onCompleted: captureDraftDisplayRefresh()
    Component.onDestruction: {
        draftDisplayRefresh.alive = false
        draftDisplayRefresh.pending = null
    }

    property var settingTexts: ({})
    property var vertexTexts: ({lat: "", lon: ""})
    property int editingVertex: -1
    property int activeMemberId: 0
    property var endpointTexts: ({})
    property bool endpointDirty: false
    property var endpointReserveSession: null
    property bool reserveEditing: false
    property var reserveSession: null
    property var reserveClearSession: null
    property var reserveFields: ({})
    property int reserveTargetId: 0
    property string reserveMode: ""
    property bool reserveConfirmed: false
    property string reserveMessage: ""
    property var auxiliaryTexts: ({lat: "", lon: "", alt: "", heading: "", holdSeconds: ""})
    property bool auxiliaryIsReturn: false
    property bool auxiliaryExplicitHeading: false
    property bool auxiliaryHeadingTouched: false
    property bool auxiliaryExplicitHold: false
    property bool auxiliaryHoldTouched: false
    property int editingAuxiliaryPoint: -1
    property var auxiliarySession: null
    readonly property bool auxiliaryInputPending: auxiliarySession !== null || editingAuxiliaryPoint >= 0
        || auxiliaryExplicitHeading || auxiliaryExplicitHold || auxiliaryTexts.lat.length > 0 || auxiliaryTexts.lon.length > 0
        || auxiliaryTexts.alt.length > 0 || auxiliaryTexts.heading.length > 0 || auxiliaryTexts.holdSeconds.length > 0
    property bool confirmClear: false
    property int outboundPage: 0
    property int returnPage: 0
    property var replacementRoster: []
    property string localError: ""

    readonly property var numericSettings: [
        {key: "altitudeMeters", label: qsTr("覆盖高度 · AMSL 海拔 (m)"), min: -1000, max: 20000},
        {key: "maxAltitudeMeters", label: qsTr("高度上限 · 同一 AMSL 基准 (m)"), min: -1000, max: 20000},
        {key: "laneSpacingMeters", label: qsTr("扫描线间隔 (m)"), min: 0.25, max: 500},
        {key: "speedMetersPerSecond", label: qsTr("规划水平速度 (m/s)"), min: 0.1, max: 300},
        {key: "climbSpeedMetersPerSecond", label: qsTr("规划爬升 / 下降速度 (m/s)"), min: 0.1, max: 20},
        {key: "headingDegrees", label: qsTr("扫描线方位角 (°)"), min: 0, max: 360},
        {key: "timeLimitSeconds", label: qsTr("任务时间预算 · 非航点时刻 (s)"), min: 1, max: 86400},
        {key: "minimumSeparationMeters", label: qsTr("规划最小机间距 (m)"), min: 0.1, max: 1000},
        {key: "returnReserveSeconds", label: qsTr("返航能量时间预留 · 非时序 (s)"), min: 0, max: 86400}
    ]
    readonly property var roles: [
        {label: qsTr("覆盖机 Scout"), value: "scout"},
        {label: qsTr("领航机 Leader"), value: "leader"},
        {label: qsTr("中继机 Relay"), value: "relay"},
        {label: qsTr("响应机 Responder"), value: "responder"},
        {label: qsTr("备用机 Reserve"), value: "reserve"}
    ]
    readonly property var pointFields: [
        {key: "lat", label: qsTr("纬度 (°)"), min: -80, max: 80},
        {key: "lon", label: qsTr("经度 (°)"), min: -180, max: 180},
        {key: "alt", label: qsTr("AMSL 海拔 (m)"), min: -1000, max: 20000}
    ]
    readonly property var endpointFields: [
        {key: "startLat", label: qsTr("出发点纬度 (°)"), min: -80, max: 80},
        {key: "startLon", label: qsTr("出发点经度 (°)"), min: -180, max: 180},
        {key: "startAlt", label: qsTr("出发点 AMSL 海拔 (m)"), min: -1000, max: 20000},
        {key: "returnLat", label: qsTr("返回点纬度 (°)"), min: -80, max: 80},
        {key: "returnLon", label: qsTr("返回点经度 (°)"), min: -180, max: 180},
        {key: "returnAlt", label: qsTr("返回点 AMSL 海拔 (m)"), min: -1000, max: 20000}
    ]
    readonly property var reserveModes: [
        {label: qsTr("地面未解锁待命（仅规划假设）"), value: "ground-disarmed"},
        {label: qsTr("空中悬停待命（仅规划假设）"), value: "airborne-hold"}
    ]
    readonly property var reserveDefinitions: [
        {key: "standbyDurationSeconds", label: qsTr("待命时长 (s) · 必填"), min: 0, max: 86400},
        {key: "standbyConsumptionFactor", label: qsTr("待命耗电系数 · 相对名义飞行耗电，必须大于 0"), min: 0, max: 10, exclusive: true},
        {key: "cruiseSpeedMetersPerSecond", label: qsTr("备用路线水平速度 (m/s) · 必须大于 0"), min: 0, max: 1000, exclusive: true},
        {key: "climbSpeedMetersPerSecond", label: qsTr("备用路线爬升速度 (m/s) · 必须大于 0"), min: 0, max: 1000, exclusive: true},
        {key: "descentSpeedMetersPerSecond", label: qsTr("备用路线下降速度 (m/s) · 必须大于 0"), min: 0, max: 1000, exclusive: true}
    ]

    implicitWidth: ScreenTools.defaultFontPixelWidth * 56
    implicitHeight: ScreenTools.defaultFontPixelHeight * 35

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function captureDraftDisplayRefresh() {
        var state = draftDisplayRefresh
        if (!state || !state.alive) return
        state.pending = controller && draft && draftSelection >= 0
            ? {controller: controller, draft: draft, selection: draftSelection} : null
        scheduleDraftDisplayRefresh()
    }

    function scheduleDraftDisplayRefresh() {
        var state = draftDisplayRefresh
        if (!state || !state.alive || !state.pending || state.queued === state.pending) return
        var pending = state.pending
        state.queued = pending
        // The stable facade's members and the selection projection have
        // independent bindings. Wait until this synchronous publication ends.
        Qt.callLater(function() {
            // An old selection or destroyed panel cannot consume a new request.
            if (!state.alive || state.pending !== pending || state.queued !== pending) return
            state.queued = null
            root.refreshDraftDisplay(state, pending)
        })
    }

    function refreshDraftDisplay(state, pending) {
        if (!state.alive || !pending || state.pending !== pending) return
        if (pending.controller !== controller || pending.draft !== draft || pending.selection !== draftSelection) {
            state.pending = null
            return
        }
        // Keep the request for a busy/input-settled change; never poll or
        // discard uncommitted coordinates, settings or profile input.
        if (draftSwitchBusy || !inputSettled) return
        state.pending = null
        if (!memberById(activeMemberId)) activeMemberId = members.length ? Number(members[0].vehicleId) : 0
        outboundPage = 0; returnPage = 0
        resetEndpointTexts()
        if (executionProfileEditor) executionProfileEditor.cancel()
    }

    function copyObject(value) {
        var copy = {}
        for (var key in value) copy[key] = value[key]
        return copy
    }

    function invalidatePreview() {
        if (draft && typeof draft.invalidatePreview === "function") draft.invalidatePreview()
    }

    function fail(message) {
        localError = String(message || qsTr("草稿操作未完成，请检查输入。"))
        errorSummary.forceActiveFocus()
        return false
    }

    function mutationAccepted(accepted) {
        if (accepted !== true) return fail(draft && draft.lastError ? draft.lastError : qsTr("草稿操作被拒绝。"))
        localError = ""
        return true
    }

    function strictNumber(text) {
        var input = String(text).trim()
        // Empty input, hex notation and Infinity must not become old/default
        // values. Exponents are accepted only when the result is finite.
        if (!/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(input)) return NaN
        var value = Number(input)
        return isFinite(value) ? value : NaN
    }

    function numberError(text, definition) {
        var value = strictNumber(text)
        if (!isFinite(value)) return qsTr("请输入有限的十进制数字。")
        if (value < definition.min || value > definition.max)
            return qsTr("范围：%1 至 %2。实际可用值仍需工程师确认。").arg(definition.min).arg(definition.max)
        return ""
    }

    function fieldText(key) {
        if (settingTexts[key] !== undefined) return settingTexts[key]
        return settings[key] === undefined ? "" : String(settings[key])
    }

    function editSetting(key, text) {
        var next = copyObject(settingTexts)
        next[key] = text
        settingTexts = next
        invalidatePreview()
    }

    function commitSettings() {
        if (!draft) return fail(qsTr("草稿服务不可用。"))
        var next = copyObject(settings)
        for (var i = 0; i < numericSettings.length; i++) {
            var definition = numericSettings[i]
            var error = numberError(fieldText(definition.key), definition)
            if (error.length > 0) return fail(definition.label + "：" + error)
            next[definition.key] = strictNumber(fieldText(definition.key))
        }
        if (next.maxAltitudeMeters < next.altitudeMeters)
            return fail(qsTr("AMSL 高度上限不能低于覆盖高度。"))
        if (!mutationAccepted(draft.configure(next))) return false
        settingTexts = ({})
        return true
    }

    function setBooleanSetting(key, value) {
        if (hasPendingCoordinates) return fail(qsTr("先保存或放弃坐标编辑，再更改该选项。"))
        var next = copyObject(settings)
        next[key] = value === true
        return draft && mutationAccepted(draft.configure(next))
    }

    function setPointText(which, key, text) {
        if (which !== "vertex" && !captureAuxiliarySession()) return
        var next = copyObject(which === "vertex" ? vertexTexts : auxiliaryTexts)
        next[key] = text
        if (which === "vertex") vertexTexts = next
        else auxiliaryTexts = next
        invalidatePreview()
    }

    function parsePoint(texts, includeAltitude) {
        var point = {}
        for (var i = 0; i < (includeAltitude ? 3 : 2); i++) {
            var field = pointFields[i]
            var error = numberError(texts[field.key], field)
            if (error.length > 0) { fail(field.label + "：" + error); return null }
            point[field.key] = strictNumber(texts[field.key])
        }
        return point
    }

    function coordinate(point, canonical, alias) {
        if (!point) return ""
        var value = point[canonical] !== undefined ? point[canonical] : point[alias]
        return typeof value === "number" && isFinite(value) ? String(value) : ""
    }

    function pointLabel(point) {
        var lat = coordinate(point, "latitude", "lat")
        var lon = coordinate(point, "longitude", "lon")
        var alt = coordinate(point, "altitude", "alt")
        return lat + ", " + lon + (alt.length > 0 ? qsTr(" · AMSL %1 m").arg(alt) : "")
    }

    function waypointHeadingLabel(point) {
        if (!point || point.heading === undefined) return qsTr("机头朝向：沿飞控策略（不是 0°）")
        return typeof point.heading === "number" && isFinite(point.heading) && point.heading >= 0 && point.heading < 360
            ? qsTr("机头朝向请求：%1°（原输入，上传审阅会显示量化）").arg(point.heading)
            : qsTr("机头朝向：未知 / 非法，不能作为当前可用输入")
    }

    function waypointHoldLabel(point) {
        if (!point || point.holdSeconds === undefined) return qsTr("驻留：未明确要求（保留原任务缺省，不补明确 0 秒）")
        return typeof point.holdSeconds === "number" && isFinite(point.holdSeconds)
                && point.holdSeconds >= 0 && point.holdSeconds <= 86400
            ? point.holdSeconds === 0 ? qsTr("相对驻留请求：0 秒（明确零）")
                : qsTr("相对驻留请求：%1 秒（非绝对到达时刻）").arg(point.holdSeconds)
            : qsTr("驻留：未知 / 非法，不能作为当前可用输入")
    }

    function saveVertex() {
        var point = parsePoint(vertexTexts, false)
        if (!point || !draft) return false
        var accepted = editingVertex >= 0 ? draft.updateVertex(editingVertex, point) : draft.appendVertex(point)
        if (!mutationAccepted(accepted)) return false
        editingVertex = -1
        vertexTexts = ({lat: "", lon: ""})
        confirmClear = false
        return true
    }

    function beginVertexEdit(index) {
        if (hasPendingCoordinates) return fail(qsTr("先保存或放弃当前坐标输入。"))
        var point = vertices[index]
        if (!point) return false
        editingVertex = index
        vertexTexts = ({lat: coordinate(point, "latitude", "lat"), lon: coordinate(point, "longitude", "lon")})
        mapPickingRequested(false)
        invalidatePreview()
        return true
    }

    function discardVertexEdit() {
        editingVertex = -1
        vertexTexts = ({lat: "", lon: ""})
        localError = ""
    }

    function captureRoster() {
        if (!draft || hasPendingCoordinates) return fail(qsTr("先保存或放弃当前坐标输入。"))
        if (!selectedVehicleIds || selectedVehicleIds.length === 0)
            return fail(qsTr("先在舰队列表选择飞机，再固定规划成员。"))
        if (selectedVehicleIds.length > 32) return fail(qsTr("一次规划最多 32 个成员。"))
        var ids = selectedVehicleIds.slice(0)
        if (members.length > 0) {
            replacementRoster = ids
            invalidatePreview()
            return true
        }
        return applyRoster(ids)
    }

    function applyRoster(ids) {
        if (hasPendingCoordinates) return fail(qsTr("先保存或放弃坐标输入，再确认成员替换。"))
        if (!draft || !mutationAccepted(draft.setVehicleIds(ids))) return false
        replacementRoster = []
        if (!memberById(activeMemberId)) activeMemberId = members.length > 0 ? Number(members[0].vehicleId) : 0
        outboundPage = 0
        returnPage = 0
        resetEndpointTexts()
        return true
    }

    function memberById(id) {
        for (var i = 0; i < members.length; i++) {
            if (Number(members[i].vehicleId) === Number(id)) return members[i]
        }
        return null
    }

    function reserveTargetChoices() {
        var choices = []
        for (var i = 0; i < members.length; ++i) {
            var member = members[i]
            if (member && member.role !== "reserve" && member.vehicleId !== activeMemberId)
                choices.push({vehicleId: member.vehicleId, label: qsTr("ID %1 · %2").arg(member.vehicleId).arg(member.role)})
        }
        return choices
    }
    function reserveChoiceIndex(choices, key, value) {
        for (var i = 0; i < choices.length; ++i) if (choices[i][key] === value) return i
        return -1
    }
    function reserveCaptureSession() {
        return draft && activeReserveMember ? {draft: draft, controller: controller, selection: draftSelection,
            revision: draft.revision, vehicleId: activeMemberId, member: JSON.stringify(activeMember)} : null
    }
    function reserveSessionCurrent(session) {
        return session && draft === session.draft && controller === session.controller
            && draftSelection === session.selection && activeMemberId === session.vehicleId && activeReserveMember
            && draft.revision === session.revision && JSON.stringify(activeMember) === session.member && !draftSwitchBusy
    }
    function resetReserveConsent() { reserveConfirmed = false; reserveClearSession = null }
    function beginReserveEdit() {
        if (!visible || !inputSettled || !activeReserveMember || draftSwitchBusy)
            return fail(qsTr("先保存或取消其他输入，再编辑当前备用机的预算。"))
        var guard = draftDisplayRefresh, session = reserveCaptureSession()
        var accepted = draft.invalidatePreview()
        if (!guard.alive) return false
        if (!accepted || !reserveSessionCurrent(session)) return fail(qsTr("草稿已变化，请重新选择备用机。"))
        var saved = activeMember.reservePlan || {}, values = {}
        for (var i = 0; i < reserveDefinitions.length; ++i) {
            var key = reserveDefinitions[i].key, value = saved[key]
            values[key] = typeof value === "number" && isFinite(value) ? String(value) : ""
        }
        reserveFields = values
        reserveTargetId = typeof saved.replacementVehicleId === "number" ? saved.replacementVehicleId : 0
        reserveMode = typeof saved.standbyMode === "string" ? saved.standbyMode : ""
        reserveSession = session; reserveConfirmed = false; reserveClearSession = null
        reserveEditing = true; reserveMessage = ""; localError = ""
        return true
    }
    function editReserveField(key, text) {
        var values = copyObject(reserveFields); values[key] = text; reserveFields = values
        reserveConfirmed = false; reserveMessage = ""
    }
    function reserveFieldError(definition) {
        var value = strictNumber(reserveFields[definition.key])
        if (!isFinite(value) || value < definition.min || value > definition.max || (definition.exclusive && value === 0))
            return definition.exclusive ? qsTr("请输入大于 0 且不超过 %1 的有限数字；没有默认物理值。").arg(definition.max)
                                       : qsTr("请输入 %1 至 %2 的有限数字。").arg(definition.min).arg(definition.max)
        return ""
    }
    function cancelReserveEdit() {
        reserveSession = null; reserveClearSession = null; reserveConfirmed = false
        reserveEditing = false; reserveFields = ({}); reserveMessage = ""; localError = ""
    }
    function saveReservePlan() {
        var guard = draftDisplayRefresh, session = reserveSession
        if (!visible || !reserveEditing || !reserveSessionCurrent(session))
            return fail(qsTr("备用预算编辑所绑定的草稿已变化；保留了输入，请取消后重新编辑。未自动重试。"))
        if (!reserveConfirmed || reserveChoiceIndex(reserveTargets, "vehicleId", reserveTargetId) < 0
                || reserveChoiceIndex(reserveModes, "value", reserveMode) < 0)
            return fail(qsTr("请选择当前非备用成员、待命方式，并确认预算假设。"))
        var policy = {schemaVersion: 1, replacementVehicleId: reserveTargetId, standbyMode: reserveMode}
        for (var i = 0; i < reserveDefinitions.length; ++i) {
            var definition = reserveDefinitions[i], error = reserveFieldError(definition)
            if (error.length) return fail(definition.label + "：" + error)
            policy[definition.key] = strictNumber(reserveFields[definition.key])
        }
        reserveConfirmed = false
        var accepted = session.draft.setMemberReservePlan(session.vehicleId, policy, session.revision)
        if (!guard.alive) return false
        if (draft !== session.draft || controller !== session.controller || draftSelection !== session.selection
                || activeMemberId !== session.vehicleId || accepted !== true)
            return fail(qsTr("备用预算保存未确认；请检查当前草稿。可能已发生的本地修改不会自动回滚，也不会自动重试。"))
        cancelReserveEdit()
        reserveMessage = qsTr("已保存本地备用预算输入；请补齐自身端点并重新生成审阅。这不是自动接替或起飞许可。")
        return true
    }
    function clearReservePlan() {
        if (!visible || !activeReserveMember || reserveEditing || draftSwitchBusy) return false
        var guard = draftDisplayRefresh, session = reserveClearSession
        if (!session) {
            if (!inputSettled) return fail(qsTr("先保存或放弃其他输入，再清除备用预算。"))
            session = reserveCaptureSession()
            var invalidated = draft.invalidatePreview()
            if (!guard.alive) return false
            if (!invalidated || !reserveSessionCurrent(session)) return fail(qsTr("草稿已变化，请重新核对。"))
            reserveClearSession = session
            reserveMessage = qsTr("再次确认清除后，该备用机失去完整待命 / 接替预算，必须补齐并重新审阅；不删除端点、不改变角色。")
            return true
        }
        reserveClearSession = null
        if (!reserveSessionCurrent(session)) return fail(qsTr("清除所绑定的草稿已变化，请重新检查。"))
        var accepted = session.draft.clearMemberReservePlan(session.vehicleId, session.revision)
        if (!guard.alive) return false
        if (draft !== session.draft || controller !== session.controller || draftSelection !== session.selection
                || activeMemberId !== session.vehicleId || accepted !== true)
            return fail(qsTr("清除未确认；请核对当前草稿，不自动回滚或重试已经发生的本地修改。"))
        reserveMessage = qsTr("已清除备用预算；该成员仍是备用机，现缺少完整能量规划输入。")
        return true
    }

    function selectMember(id) {
        if (reserveEditing || reserveClearSession) return fail(qsTr("先保存或取消备用预算编辑 / 清除确认。"))
        if (executionProfileEditor.pendingInput) return fail(qsTr("先保存或取消当前成员的执行配置编辑／清除确认。"))
        if (endpointDirty || auxiliaryInputPending)
            return fail(qsTr("先保存或放弃该成员的端点 / 航点输入。"))
        activeMemberId = Number(id)
        outboundPage = 0
        returnPage = 0
        resetEndpointTexts()
        reserveMessage = ""
        localError = ""
        return true
    }

    function roleIndex(role) {
        for (var i = 0; i < roles.length; i++) if (roles[i].value === role) return i
        return -1
    }

    function setActiveRole(role) {
        if (reserveEditing || reserveClearSession) return fail(qsTr("先保存或取消备用预算，再改变角色。"))
        if (executionProfileEditor.pendingInput) return fail(qsTr("先保存或取消执行配置／清除确认，再改变角色。"))
        if (endpointDirty || auxiliaryInputPending)
            return fail(qsTr("先保存或放弃该成员的坐标编辑，再修改角色。"))
        return draft && activeMember && mutationAccepted(draft.setRole(activeMemberId, role))
    }

    function resetEndpointTexts() {
        var member = memberById(activeMemberId)
        var start = member ? member.start : null
        var end = member ? member.returnTo : null
        endpointTexts = ({startLat: coordinate(start, "latitude", "lat"), startLon: coordinate(start, "longitude", "lon"),
                          startAlt: coordinate(start, "altitude", "alt"), returnLat: coordinate(end, "latitude", "lat"),
                          returnLon: coordinate(end, "longitude", "lon"), returnAlt: coordinate(end, "altitude", "alt")})
        endpointDirty = false
        endpointReserveSession = null
    }

    function editEndpoint(key, text) {
        if (reserveEditing || reserveClearSession || executionProfileEditor.pendingInput) return
        if (!endpointDirty && activeReserveMember) endpointReserveSession = reserveCaptureSession()
        var next = copyObject(endpointTexts)
        next[key] = text
        endpointTexts = next
        endpointDirty = true
        invalidatePreview()
    }

    function saveEndpoints() {
        var start = parsePoint({lat: endpointTexts.startLat, lon: endpointTexts.startLon, alt: endpointTexts.startAlt}, true)
        if (!start) return false
        var end = parsePoint({lat: endpointTexts.returnLat, lon: endpointTexts.returnLon, alt: endpointTexts.returnAlt}, true)
        if (!end || !draft || (!activeCoverageMember && !activeReserveMember)) return false
        if (reserveEditing || reserveClearSession || executionProfileEditor.pendingInput)
            return fail(qsTr("先保存或取消备用预算 / 执行配置，再保存端点。"))
        var guard = draftDisplayRefresh, session = endpointReserveSession
        if ((activeReserveMember || session) && (!endpointDirty || !reserveSessionCurrent(session)))
            return fail(qsTr("备用端点输入已过期或没有待保存修改；请放弃后重新编辑。"))
        var accepted = activeReserveMember
            ? draft.setReserveEndpoints(activeMemberId, start, end, session.revision)
            : draft.setEndpoints(activeMemberId, start, end)
        if (!guard.alive) return false
        if (session && (draft !== session.draft || controller !== session.controller
                || draftSelection !== session.selection || activeMemberId !== session.vehicleId))
            return fail(qsTr("端点保存后草稿来源已变化，请检查当前草稿；不自动回滚或重试。"))
        if (!mutationAccepted(accepted)) return false
        resetEndpointTexts()
        return true
    }

    function auxiliarySessionCurrent(session, requireRevision) {
        return draftDisplayRefresh.alive && session && auxiliarySession === session
            && draft === session.draft && controller === session.controller && draftSelection === session.selection
            && draft && draft.sourceToken === session.sourceToken && activeMemberId === session.vehicleId
            && auxiliaryIsReturn === session.returnPath && editingAuxiliaryPoint === session.index
            && (!requireRevision || draft.revision === session.revision)
    }

    function captureAuxiliarySession() {
        if (auxiliarySession) return auxiliarySessionCurrent(auxiliarySession, true)
            || fail(qsTr("航点输入对应的草稿或版本已变化；请放弃输入后重新选择航点，不自动应用到新草稿。"))
        if (!draft || !activeAuxiliaryMember || draftSwitchBusy || draft.revisionExhausted
                || typeof draft.sourceToken !== "string" || !draft.sourceToken.length)
            return fail(qsTr("当前草稿或辅助成员不可编辑，请先选择有效成员。"))
        auxiliarySession = {draft: draft, controller: controller, selection: draftSelection, sourceToken: draft.sourceToken,
            revision: draft.revision, vehicleId: activeMemberId, returnPath: auxiliaryIsReturn, index: editingAuxiliaryPoint}
        return auxiliarySessionCurrent(auxiliarySession, true)
    }

    function setAuxiliaryHeadingEnabled(enabled) {
        if (!captureAuxiliarySession()) return false
        auxiliaryExplicitHeading = enabled === true
        auxiliaryHeadingTouched = false
        // The unchecked mode removes this point's explicit request. It does
        // not reinterpret an empty field as north or reuse the scan axis.
        if (!auxiliaryExplicitHeading) {
            var next = copyObject(auxiliaryTexts)
            next.heading = ""
            auxiliaryTexts = next
        }
        invalidatePreview()
        return true
    }

    function auxiliaryHeadingError() {
        if (!auxiliaryExplicitHeading) return ""
        var value = strictNumber(auxiliaryTexts.heading)
        return isFinite(value) && value >= 0 && value < 360 ? ""
            : qsTr("请输入 0°（含）至 360°（不含）的有限十进制数；0° 是显式朝北请求。")
    }

    function setAuxiliaryHoldEnabled(enabled) {
        if (!captureAuxiliarySession()) return false
        auxiliaryExplicitHold = enabled === true
        auxiliaryHoldTouched = false
        if (!auxiliaryExplicitHold) {
            var next = copyObject(auxiliaryTexts)
            next.holdSeconds = ""
            auxiliaryTexts = next
        }
        invalidatePreview()
        return true
    }

    function auxiliaryHoldError() {
        if (!auxiliaryExplicitHold) return ""
        var value = strictNumber(auxiliaryTexts.holdSeconds)
        return isFinite(value) && value >= 0 && value <= 86400 ? ""
            : qsTr("请输入 0 至 86400 秒的有限十进制数；明确 0 秒与未指定不同。")
    }

    function discardAuxiliaryInput() {
        auxiliaryTexts = ({lat: "", lon: "", alt: "", heading: "", holdSeconds: ""})
        auxiliaryExplicitHeading = false
        auxiliaryHeadingTouched = false
        auxiliaryExplicitHold = false
        auxiliaryHoldTouched = false
        editingAuxiliaryPoint = -1
        auxiliarySession = null
        localError = ""
    }

    function captureAuxiliaryRow(index, returnPath) {
        if (!draft || !activeMember || hasPendingCoordinates || hasPendingSettingText || draftSwitchBusy
                || (returnPath !== true && returnPath !== false)) return null
        var route = array(returnPath ? activeMember.returnPath : activeMember.waypoints)
        if (index < 0 || index >= route.length || Math.floor(index) !== index) return null
        return {draft: draft, controller: controller, selection: draftSelection, sourceToken: draft.sourceToken,
            revision: draft.revision, vehicleId: activeMemberId, returnPath: returnPath, index: index}
    }

    function auxiliaryRowCurrent(captured) {
        return draftDisplayRefresh.alive && captured && draft === captured.draft && controller === captured.controller
            && draftSelection === captured.selection && draft && draft.sourceToken === captured.sourceToken
            && draft.revision === captured.revision && activeMemberId === captured.vehicleId
    }

    function beginAuxiliaryPointEdit(captured) {
        if (!auxiliaryRowCurrent(captured)) return false
        if (hasPendingCoordinates || hasPendingSettingText || draftSwitchBusy || !activeAuxiliaryMember)
            return fail(qsTr("先保存或放弃其它输入，再编辑已有航点。"))
        var index = captured.index, returnPath = captured.returnPath
        var route = array(returnPath ? activeMember.returnPath : activeMember.waypoints)
        if (index < 0 || index >= route.length || Math.floor(index) !== index) return false
        var point = route[index]
        auxiliaryIsReturn = returnPath
        editingAuxiliaryPoint = index
        auxiliaryTexts = ({lat: coordinate(point, "latitude", "lat"), lon: coordinate(point, "longitude", "lon"),
            alt: coordinate(point, "altitude", "alt"), heading: point.heading === undefined ? "" : String(point.heading),
            holdSeconds: point.holdSeconds === undefined ? "" : String(point.holdSeconds)})
        auxiliaryExplicitHeading = point.heading !== undefined
        auxiliaryHeadingTouched = false
        auxiliaryExplicitHold = point.holdSeconds !== undefined
        auxiliaryHoldTouched = false
        if (!auxiliaryRowCurrent(captured)) return false
        if (!captureAuxiliarySession()) return false
        var session = auxiliarySession
        invalidatePreview()
        if (!auxiliarySessionCurrent(session, true)) return false
        auxiliaryHeadingToggle.forceActiveFocus()
        return true
    }

    function appendAuxiliaryPoint() {
        if (!captureAuxiliarySession()) return false
        var session = auxiliarySession, guard = draftDisplayRefresh
        var point = parsePoint(auxiliaryTexts, true)
        if (!point || !draft || !activeAuxiliaryMember) return false
        auxiliaryHeadingTouched = true
        auxiliaryHoldTouched = true
        var headingError = auxiliaryHeadingError()
        if (headingError.length) return fail(qsTr("机头朝向：") + headingError)
        if (auxiliaryExplicitHeading) point.heading = strictNumber(auxiliaryTexts.heading)
        var holdError = auxiliaryHoldError()
        if (holdError.length) return fail(qsTr("相对驻留：") + holdError)
        if (auxiliaryExplicitHold) point.holdSeconds = strictNumber(auxiliaryTexts.holdSeconds)
        if (!auxiliarySessionCurrent(session, true))
            return fail(qsTr("航点来源已变化，请放弃输入后重新编辑。"))
        var accepted = session.index >= 0
            ? session.draft.updateMemberWaypoint(session.vehicleId, session.index, point, session.returnPath, session.revision, session.sourceToken)
            : session.draft.appendMemberWaypoint(session.vehicleId, point, session.returnPath, session.revision, session.sourceToken)
        if (!guard.alive) return false
        if (!auxiliarySessionCurrent(session, false))
            return fail(qsTr("航点操作后草稿来源已变化，请核对当前内容；不自动重试或撤回已提交的修改。"))
        if (!mutationAccepted(accepted)) return false
        discardAuxiliaryInput()
        return true
    }

    function removeAuxiliaryPoint(captured) {
        if (!auxiliaryRowCurrent(captured)) return false
        if (!draft || hasPendingCoordinates || hasPendingSettingText || draftSwitchBusy)
            return fail(qsTr("先保存或放弃其它输入，再删除航点。"))
        var original = draft, token = draft.sourceToken, revision = draft.revision
        var memberId = activeMemberId, selection = draftSelection, guard = draftDisplayRefresh
        var accepted = original.removeMemberWaypoint(memberId, captured.index, captured.returnPath, revision, token)
        if (!guard.alive) return false
        if (draft !== original || draft.sourceToken !== token || draftSelection !== selection || activeMemberId !== memberId)
            return fail(qsTr("删除后草稿来源已变化，请检查当前内容；不自动重试。"))
        return mutationAccepted(accepted)
    }

    function makePreview() {
        if (planningBusy) return fail(qsTr("后台规划尚未结束，请等待或取消当前计算；输入会保留。"))
        if (performancePolicyEditor.pendingInput) return fail(qsTr("性能准入政策尚未保存或取消，请先处理该编辑；不能用旧政策生成审阅。"))
        if (executionProfileEditor.pendingInput) return fail(qsTr("执行配置尚在编辑或等待清除确认，请先保存、确认或取消，不能使用旧配置生成审阅。"))
        if (hasPendingCoordinates || replacementRoster.length > 0 || confirmClear)
            return fail(qsTr("还有未保存的坐标或未确认的成员替换，请先保存、添加或放弃。"))
        mapPickingRequested(false)
        if (!commitSettings()) return false
        localError = ""
        previewRequested()
        return true
    }

    function planningStatusText() {
        var state = planningStatus
        var error = typeof state.error === "string" ? state.error : ""
        var expired = /expired|superseded|replaced|invalidated|inputs_changed/.test(error)
        var message = ""
        if (state.status === "cancelling")
            message = qsTr("后台规划正在取消，等待计算线程退出。输入已保留；此时不能重复生成。")
        else if (state.active === true)
            message = qsTr("正在后台计算规划与净空资料。尚无最终审阅结果；可以继续查看输入或取消计算。")
        else if (expired)
            message = qsTr("本次后台规划已过期：输入或来源发生变化。输入仍保留，请重新生成审阅。")
        else if (state.status === "cancelled" || error === "planning_operator_cancelled")
            message = qsTr("后台规划已取消。输入仍保留；如需继续，请重新生成审阅。")
        else if (state.status === "failed" || error.length > 0)
            message = qsTr("后台规划未形成当前可用的审阅绑定。请查看原因、修正输入后重试；已提交的历史记录不会因此回滚。")
        else if (state.ready === true || state.status === "completing")
            message = qsTr("后台计算已结束，正在核对当前来源并整理审阅；尚未批准或执行。")
        else if (state.status === "completed")
            message = qsTr("后台审阅流程已结束。请查看本次结果和阻断项；计算结束不等于允许上传或飞行。")
        if (message && expired && state.active === true)
            message += qsTr("\n原输入已过期，本次结果不能用于当前审批。")
        return message + (message && error.length ? qsTr("\n原因：%1").arg(error) : "")
    }

    function estimate(value) {
        return typeof value === "number" && isFinite(value) && value >= 0
            ? qsTr("约 %1 s（估算）").arg(value.toFixed(1)) : qsTr("未知")
    }

    function boundedEnergyAssignments(value) {
        if (!value || typeof value === "string" || typeof value.length !== "number"
                || !isFinite(value.length) || Math.floor(value.length) !== value.length
                || value.length < 0 || value.length > 255) return null
        var rows = []
        for (var i = 0; i < value.length; ++i) rows.push(value[i])
        return rows
    }
    function energyText(value) { return typeof value === "string" && value.length ? value.substring(0, 500) : qsTr("未提供 / 未知") }
    function energyReason(value) { return value === "" ? qsTr("无附加原因（仍以前述资格标志为准）") : energyText(value) }
    function energyFlag(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function energyFinite(value) { return typeof value === "number" && isFinite(value) && Math.abs(value) <= 9007199254740991 }
    function energyInteger(value, minimum, maximum) { return energyFinite(value) && Math.floor(value) === value && value >= minimum && value <= maximum }
    function energySeconds(value) {
        if (!energyFinite(value) || value < 0) return qsTr("未知")
        return value > 0 && value < 0.1 ? qsTr("< 0.1 s（非零）") : qsTr("%1 s").arg(value.toFixed(1))
    }
    function energyPercent(value) { return energyFinite(value) && value >= 0 && value <= 100 ? qsTr("约 %1%").arg(value.toFixed(1)) : qsTr("未知") }
    function energyStatus(value) {
        if (value === "invalid-input") return qsTr("输入无效（invalid-input）")
        if (value === "estimate-incomplete") return qsTr("估算证据不完整（estimate-incomplete）")
        if (value === "estimate-insufficient") return qsTr("主任务成员估算不足（estimate-insufficient）")
        if (value === "estimate-reserve-shortfall") return qsTr("备用成员估算不足（estimate-reserve-shortfall）")
        if (value === "estimate-feasible") return qsTr("估算可行，非实际续航保证（estimate-feasible）")
        return energyText(value)
    }
    function energyMargin(value, evidenceKnown) {
        // -1 is also a legitimate one-second deficit. Never use the numeric
        // sentinel alone to distinguish unknown evidence from a real budget.
        if (evidenceKnown !== true || !energyFinite(value)) return qsTr("未知（不能解释为零短缺）")
        return value < 0 ? qsTr("短缺 %1").arg(energySeconds(-value)) : qsTr("余量 %1").arg(energySeconds(value))
    }
    function energyGroup(value) {
        if (value === "primary") return energyPlan.boundAssignments === true ? qsTr("原主任务成员 primary") : qsTr("候选主任务成员 primary（未确认原方案绑定）")
        if (value === "reserve") return energyPlan.boundAssignments === true ? qsTr("原备用成员 reserve") : qsTr("候选备用成员 reserve（未确认原方案绑定）")
        return qsTr("身份未知：%1").arg(energyText(value))
    }
    function energyBasis(value) {
        if (value === "autopilot-reported-estimate") return qsTr("飞控报告的剩余时间估计（已扣本地收包年龄，非实测模型）")
        if (value === "direct-remaining") return qsTr("输入的剩余可飞时长")
        if (value === "percent-drain-rate") return qsTr("电量百分比 / 消耗率模型")
        if (value === "native-soc-decline-window") return qsTr("近期原生SOC下降窗口（仅下调已有估算，非实测功率或续航保证）")
        if (value === "full-battery-endurance") return qsTr("满电续航按输入电量折算")
        return qsTr("未知（%1）").arg(energyText(value))
    }
    function energyNativeStateText(value) {
        var label = qsTr("未知状态，不推定可用")
        if (value === "reported-estimate") label = qsTr("已记录唯一推进电池报告，字段是否可用见下方")
        else if (value === "no-observation") label = qsTr("尚无电池观察")
        else if (value === "source-unavailable" || value === "registry-native-source-unavailable") label = qsTr("原生来源不可用")
        else if (value === "registry-native-source-unbound") label = qsTr("尚未绑定实际飞机来源")
        else if (value === "receive-in-progress" || value === "registry-native-read-reentrant") label = qsTr("来源正在处理，本次读取保持未知")
        else if (value === "registry-native-observation-invalid") label = qsTr("原生观察结构不适用")
        else if (value === "slot-limit-exceeded") label = qsTr("电池槽超过支持上限，不能选择部分值")
        else if (value === "unknown-battery-role") label = qsTr("存在用途不明的电池，不能合并估算")
        else if (value === "multiple-flight-batteries") label = qsTr("存在多个推进电池，尚无适用的组合模型")
        else if (value === "no-flight-battery") label = qsTr("没有明确的推进电池报告")
        else if (value === "stale-flight-battery") label = qsTr("推进电池报告已过期，须重新获取并审阅")
        return qsTr("%1（%2）").arg(label).arg(energyText(value))
    }
    function energyNativeSourceText(value) {
        if (!value || typeof value !== "object" || Array.isArray(value) || value.schemaVersion !== 1
                || !Object.prototype.hasOwnProperty.call(value, "aggregationState"))
            return qsTr("原生电池来源摘要未记录或无效。旧版 / 独立输入不因此成为已验证遥测；请结合本行拒绝原因。")
        var age = energyInteger(value.ageMs, 0, 9007199254740991) ? qsTr("%1 ms").arg(value.ageMs) : qsTr("未知")
        return qsTr("电池证据通道：原生 BATTERY_STATUS；聚合状态：%1；审阅采样时来源连续：%2。\n采样时本地收包年龄：%3（上限 3000 ms，不是远端测量年龄）；此处为审阅快照，不实时递增。\n系统 / 组件 / 聚合电池 ID：%4 / %5 / %6；已记录电池槽：%7。\n采样电量已知：%8；原生剩余时间已知：%9；原报告 %10；扣龄后 %11。\n来源 epoch：%12；内容版本：%13。\n只读报告估计，不认证发送者、物理电池拓扑、性能或飞行许可。")
            .arg(energyNativeStateText(value.aggregationState)).arg(energyFlag(value.sourceCurrent)).arg(age)
            .arg(energyInteger(value.vehicleId, 1, 255) ? String(value.vehicleId) : qsTr("未知"))
            .arg(energyInteger(value.componentId, 1, 255) ? String(value.componentId) : qsTr("未知"))
            .arg(energyInteger(value.batteryId, 0, 255) ? String(value.batteryId) : qsTr("未知"))
            .arg(value.rows && energyInteger(value.rows.length, 0, 16) ? String(value.rows.length) : qsTr("未知"))
            .arg(energyFlag(value.batteryPercentKnown)).arg(energyFlag(value.remainingSecondsKnown))
            .arg(value.remainingSecondsKnown === true ? energySeconds(value.reportedRemainingSeconds) : qsTr("未知"))
            .arg(value.remainingSecondsKnown === true ? energySeconds(value.remainingFlightSeconds) : qsTr("未知"))
            .arg(energyText(value.sourceEpoch)).arg(energyText(value.contentRevision))
    }
    function energyDetailSeconds(details, key) {
        if (!details || !Object.prototype.hasOwnProperty.call(details, key)) return qsTr("未提供（不按 0 计）")
        return energySeconds(details[key])
    }
    function energyBudgetDetailsText(value) {
        var d = value && typeof value === "object" && !Array.isArray(value) ? value : ({})
        if (d.basis === "compiled-reserve-route-budget") {
            var standby = d.standbyMode === "ground-disarmed" ? qsTr("地面未解锁待命（工程假设）")
                        : d.standbyMode === "airborne-hold" ? qsTr("空中悬停待命（工程假设）") : qsTr("未知")
            return qsTr("备用路线预算：要求明确输入 %1；完整 %2；原因：%3。\n接替 ID %4；待命方式：%5；时长 %6；耗电系数 %7；等效名义飞行时长 %8。\n自身起点至目标路线进场 %9；完整目标路线 %10；目标停留 %11；自身返程 %12。\n任务预算下限 %13；提供或默认返程预留 %14。\n目标路线哈希：%15。\n这是按备用速度重新估算的完整路线规划，不是断点续飞、能力等价或自动接替；未验证障碍、净空、待命耗电模型及实飞续航。")
                .arg(energyFlag(d.reserveEnergyPlanRequired)).arg(energyFlag(d.reserveEnergyPlanComplete))
                .arg(d.reserveEnergyPlanComplete === true && d.reason === "" ? qsTr("无附加拒绝原因") : energyText(d.reason))
                .arg(energyInteger(d.replacementVehicleId, 1, 255) ? String(d.replacementVehicleId) : qsTr("未知"))
                .arg(standby).arg(energyDetailSeconds(d, "standbyDurationSeconds"))
                .arg(energyFinite(d.standbyConsumptionFactor) && d.standbyConsumptionFactor > 0 ? String(d.standbyConsumptionFactor) : qsTr("未知"))
                .arg(energyDetailSeconds(d, "standbyEquivalentFlightSeconds")).arg(energyDetailSeconds(d, "estimatedIngressSeconds"))
                .arg(energyDetailSeconds(d, "estimatedTaskSeconds")).arg(energyDetailSeconds(d, "targetHoldSeconds"))
                .arg(energyDetailSeconds(d, "estimatedReturnSeconds")).arg(energyDetailSeconds(d, "sharedMissionFloorSeconds"))
                .arg(energyDetailSeconds(d, "suppliedOrDefaultReturnReserveSeconds")).arg(energyText(d.targetRouteHash))
        }
        if (d.basis === "worst-primary-route-reserve-budget") {
            return qsTr("旧版备用预算（仅参考）：曾使用主任务中最长出航与最长返程预算；不代表这架机已有真实替补路线，也不满足新的完整备用输入。\n提供备用路线：%1；包含待命耗电时长：%2。\n路线时序已认证：%3；电池模型已认证：%4。")
                .arg(energyFlag(d.reserveRouteProvided)).arg(energyFlag(d.reserveWaitingTimeIncluded))
                .arg(energyFlag(d.routeTimingCertified)).arg(energyFlag(d.batteryModelCertified))
        }
        if (d.basis !== "compiled-member-route-budget") return qsTr("预算分项未提供或版本未知；不从空字段补造进场、作业或停留时长。")
        return qsTr("路线估算分项：进场 %1；作业 %2；返程 %3。\n出航停留预算 %4；返程停留预算 %5（均非绝对到达时刻）。\n含停留出航估计 %6；含停留返程估计 %7。\n任务时间下限 %8；提供或默认的返程预留 %9。\n任务时长依据：%10；返程时长依据：%11。\n任务 / 返程时长输入无效：%12 / %13。\n路线时序已认证：%14；电池模型已认证：%15；返程预留等于真实路线时长：%16。")
            .arg(energyDetailSeconds(d, "estimatedIngressDurationSeconds")).arg(energyDetailSeconds(d, "estimatedSurveyDurationSeconds"))
            .arg(energyDetailSeconds(d, "estimatedReturnDurationSeconds")).arg(energyDetailSeconds(d, "maximumOutboundHoldSeconds"))
            .arg(energyDetailSeconds(d, "maximumReturnHoldSeconds")).arg(energyDetailSeconds(d, "maximumEstimatedOutboundWithHoldSeconds"))
            .arg(energyDetailSeconds(d, "maximumEstimatedReturnWithHoldSeconds")).arg(energyDetailSeconds(d, "missionTimeLimitBudgetSeconds"))
            .arg(energyDetailSeconds(d, "suppliedOrDefaultReturnReserveSeconds")).arg(energyText(d.missionDurationEvidence)).arg(energyText(d.returnDurationEvidence))
            .arg(energyFlag(d.durationInputInvalid)).arg(energyFlag(d.returnDurationInputInvalid)).arg(energyFlag(d.routeTimingCertified))
            .arg(energyFlag(d.batteryModelCertified)).arg(energyFlag(d.returnReserveIsRouteDuration))
    }
    function ensureEnergyBudgetFocusVisible(item) {
        if (!item || !scroller.visible) return
        var point = item.mapToItem(body, 0, 0)
        var top = point.y - pad, bottom = point.y + item.height + pad
        var maximum = Math.max(0, scroller.contentHeight - scroller.height)
        if (top < scroller.contentY || item.height + pad * 2 > scroller.height)
            scroller.contentY = Math.min(maximum, Math.max(0, top))
        else if (bottom > scroller.contentY + scroller.height)
            scroller.contentY = Math.min(maximum, Math.max(0, bottom - scroller.height))
    }
    function keepEnergyPaginationFocusVisible() {
        if (!draftDisplayRefresh || !draftDisplayRefresh.alive || !visible || !energyBillExpanded) return
        // Page content can change height without changing the button's focus.
        // React to actual layout geometry, not a timer or a stale page callback.
        if (energyPreviousButton && energyPreviousButton.activeFocus) ensureEnergyBudgetFocusVisible(energyPreviousButton)
        else if (energyNextButton && energyNextButton.activeFocus) ensureEnergyBudgetFocusVisible(energyNextButton)
    }

    function array(value) { return value && typeof value.length === "number" ? value : [] }
    function memberIdsForRefresh(value) {
        if (!value || typeof value === "string" || typeof value.length !== "number"
                || !isFinite(value.length) || Math.floor(value.length) !== value.length
                || value.length < 1 || value.length > 32) return []
        var ids = [], seen = ({})
        for (var index = 0; index < value.length; ++index) {
            var id = value[index] ? value[index].vehicleId : undefined
            if (typeof id !== "number" || !isFinite(id) || Math.floor(id) !== id
                    || id < 1 || id > 255 || seen[String(id)]) return []
            seen[String(id)] = true
            ids.push(id)
        }
        return ids
    }

    function blockers() {
        var result = []
        function add(label, values) {
            values = array(values)
            for (var i = 0; i < values.length; i++) {
                var value = values[i]
                var detail = typeof value === "string" ? value
                    : String(value && (value.detail || value.reason || value.message || value.code)
                             || qsTr("详见完整审阅记录"))
                result.push(label + "：" + detail.substring(0, 500))
            }
        }
        add(qsTr("计划"), plan.unmetRequirements)
        var airspace = review.airspaceReport || {}
        add(qsTr("空域"), airspace.violations)
        add(qsTr("间距"), airspace.conflicts)
        add(qsTr("空域原因"), airspace.reasons)
        add(qsTr("能量"), (review.energyPlan || {}).reasons)
        add(qsTr("风险"), (review.riskAssessment || {}).reasons)
        add(qsTr("草稿"), (reviewResult.draftValidation || {}).reasons)
        add(qsTr("草稿"), (reviewResult.draftValidation || {}).errors)
        return result
    }

    Keys.onEscapePressed: {
        mapPickingRequested(false)
        closeRequested()
        event.accepted = true
    }
    onVisibleChanged: if (!visible) { resetReserveConsent(); mapPickingRequested(false) }

    Rectangle {
        anchors.fill: parent
        color: qgcPal.window
        radius: 6
        border.color: qgcPal.text
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.pad
        spacing: root.pad

        RowLayout {
            Layout.fillWidth: true
            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("地图任务规划 · 本地草稿")
                font.bold: true
                wrapMode: Text.WordWrap
            }
            QGCButton {
                text: qsTr("关闭")
                focusPolicy: Qt.StrongFocus
                Layout.minimumHeight: root.rowHeight
                onClicked: { root.mapPickingRequested(false); root.closeRequested() }
            }
        }

        FleetMissionLifecyclePanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        FleetFirmwareCatalogPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        FleetOptionalTelemetryPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }
        FleetLinkLatencyPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }
        FleetCameraStreamCapturePanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        FleetCapabilityReviewPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        FleetArchiveHistoryPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            archive: root.controller ? root.controller.persistentEventArchive : null
        }

        FleetTelemetryHistoryPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            archive: root.controller ? root.controller.telemetryArchive : null
        }

        // Independent of draft validity, map picking and suggestion results.
        FleetOccupancyRecoveryPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        // Always reachable outside the draft/preview/map-picking scroll.
        // The shared controller owns a separate native close-review workflow.
        FleetMissionObservationPanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            controller: root.controller
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: !root.mapPicking
            text: qsTr("编辑和审阅不发送飞行指令。地面上传位于末部，需要独立二次确认并会覆盖机载任务；不解锁或启动。高度均为 AMSL 海拔，不是离地高度，速度和时间显示不代表实际机体能力。")
            wrapMode: Text.WordWrap
            color: qgcPal.colorOrange
        }

        FocusScope {
            id: errorSummary
            Layout.fillWidth: true
            Layout.preferredHeight: errorLabel.implicitHeight + root.pad * 2
            visible: !root.mapPicking && errorLabel.text.length > 0
            activeFocusOnTab: visible
            Accessible.role: Accessible.StaticText
            Accessible.name: errorLabel.text
            Rectangle {
                anchors.fill: parent
                color: qgcPal.windowShade
                border.width: errorSummary.activeFocus ? 2 : 1
                border.color: qgcPal.colorOrange
                radius: 4
            }
            QGCLabel {
                id: errorLabel
                textFormat: Text.PlainText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: root.pad
                text: root.localError.length > 0 ? qsTr("未完成：") + root.localError
                    : (root.draft && root.draft.lastError ? qsTr("草稿提示：") + root.draft.lastError : "")
                wrapMode: Text.WordWrap
            }
        }

        Flickable {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.mapPicking
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight
            onContentHeightChanged: { root.keepEnergyPaginationFocusVisible(); remainderPanel.keepPaginationFocusVisible() }
            boundsBehavior: Flickable.StopAtBounds
            Controls.ScrollBar.vertical: Controls.ScrollBar { }

            ColumnLayout {
                id: body
                width: scroller.width - root.pad
                spacing: root.pad

                FleetMissionSavedDraftPanel {
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.inputSettled && !root.planningBusy
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                FleetUserTaskTemplatesPanel {
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.inputSettled && !root.planningBusy
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                FleetMissionArchiveDraftPanel {
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.inputSettled
                }

                FleetMissionRemainderPanel {
                    id: remainderPanel
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.remainderInputSettled
                    onReviewInvalidationRequested: root.invalidatePreview()
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                FleetMissionGroupRemainderPanel {
                    id: groupRemainderPanel
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.groupRemainderInputSettled
                    onReviewInvalidationRequested: root.invalidatePreview()
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                FleetReserveDispatchPanel {
                    id: reserveDispatchPanel
                    Layout.fillWidth: true
                    controller: root.controller
                    inputSettled: root.reserveDispatchInputSettled
                    onReviewInvalidationRequested: root.invalidatePreview()
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                QGCLabel { Layout.fillWidth: true; text: qsTr("1 · 固定成员和角色"); font.bold: true; wrapMode: Text.WordWrap }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("固定后不随地图选机变化。重新固定会替换成员范围，需要确认；未知性能或不适合角色的飞机仍由审阅阻断。")
                    wrapMode: Text.WordWrap
                }
                QGCButton {
                    Layout.fillWidth: true
                    Layout.minimumHeight: root.rowHeight
                    focusPolicy: Qt.StrongFocus
                    text: qsTr("固定选中成员（%1 架）").arg(root.selectedVehicleIds.length)
                    enabled: root.draft !== null && !root.draft.revisionExhausted
                    onClicked: root.captureRoster()
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.replacementRoster.length > 0
                    QGCLabel {
                        Layout.fillWidth: true
                        textFormat: Text.PlainText
                        text: qsTr("确认将规划成员替换为 ID：%1？此操作不修改正在执行的任务。").arg(root.replacementRoster.join(", "))
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        QGCButton {
                            text: qsTr("确认替换")
                            enabled: !root.hasPendingCoordinates
                            focusPolicy: Qt.StrongFocus
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            onClicked: root.applyRoster(root.replacementRoster.slice(0))
                        }
                        QGCButton {
                            text: qsTr("保留原成员")
                            focusPolicy: Qt.StrongFocus
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            onClicked: root.replacementRoster = []
                        }
                    }
                }
                Repeater {
                    model: root.members
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        focusPolicy: Qt.StrongFocus
                        text: qsTr("ID %1 · %2%3").arg(modelData.vehicleId)
                            .arg(root.roleIndex(modelData.role) >= 0 ? root.roles[root.roleIndex(modelData.role)].label : modelData.role)
                            .arg(Number(modelData.vehicleId) === root.activeMemberId ? qsTr(" · 正在编辑") : "")
                        checked: Number(modelData.vehicleId) === root.activeMemberId
                        onClicked: root.selectMember(modelData.vehicleId)
                    }
                }

                QGCLabel { Layout.fillWidth: true; text: qsTr("2 · 区域边界（%1 / 64 顶点）").arg(root.vertices.length); font.bold: true; wrapMode: Text.WordWrap }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("顺序围成单个多边形，至少 3 点；不支持孔洞、自相交或跨日期变更线。启用地图选点后，仅右键新增顶点；也可以键盘录入经纬度。地图编辑不会发出 GoTo。")
                    wrapMode: Text.WordWrap
                }
                QGCButton {
                    Layout.fillWidth: true
                    Layout.minimumHeight: root.rowHeight
                    focusPolicy: Qt.StrongFocus
                    text: root.mapPicking ? qsTr("停止地图选点") : qsTr("启用地图右键选点")
                    enabled: root.draft !== null
                    onClicked: {
                        if (!root.mapPicking && root.hasPendingCoordinates) root.fail(qsTr("先保存或放弃坐标输入，再启用地图选点。"))
                        else root.mapPickingRequested(!root.mapPicking)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: root.pointFields.slice(0, 2)
                        ColumnLayout {
                            Layout.fillWidth: true
                            readonly property var definition: modelData
                            QGCLabel { text: definition.label }
                            Controls.TextField {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: root.vertexTexts[definition.key]
                                color: qgcPal.text
                                font.pointSize: ScreenTools.defaultFontPointSize
                                selectByMouse: true
                                maximumLength: 32
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Accessible.name: definition.label
                                onTextEdited: root.setPointText("vertex", definition.key, text)
                                background: Rectangle {
                                    color: qgcPal.windowShade
                                    border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                    border.width: parent.activeFocus ? 2 : 1
                                    radius: 3
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: root.vertexTexts[definition.key].length > 0
                                    ? root.numberError(root.vertexTexts[definition.key], definition) : ""
                                visible: text.length > 0
                                color: qgcPal.colorOrange
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        text: root.editingVertex >= 0 ? qsTr("保存顶点 %1").arg(root.editingVertex + 1) : qsTr("添加顶点")
                        focusPolicy: Qt.StrongFocus
                        enabled: root.draft !== null && (root.vertices.length < 64 || root.editingVertex >= 0)
                        onClicked: root.saveVertex()
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        text: qsTr("放弃坐标输入")
                        focusPolicy: Qt.StrongFocus
                        onClicked: root.discardVertexEdit()
                    }
                }
                Repeater {
                    model: root.vertices
                    RowLayout {
                        Layout.fillWidth: true
                        QGCLabel {
                            Layout.fillWidth: true
                            textFormat: Text.PlainText
                            text: qsTr("%1 · %2").arg(index + 1).arg(root.pointLabel(modelData))
                            wrapMode: Text.WrapAnywhere
                        }
                        QGCButton {
                            text: qsTr("编辑")
                            Layout.minimumHeight: root.rowHeight
                            focusPolicy: Qt.StrongFocus
                            onClicked: root.beginVertexEdit(index)
                        }
                        QGCButton {
                            text: qsTr("删除")
                            Layout.minimumHeight: root.rowHeight
                            focusPolicy: Qt.StrongFocus
                            enabled: !root.hasPendingCoordinates
                            onClicked: root.mutationAccepted(root.draft.removeVertex(index))
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    QGCButton {
                        text: qsTr("撤销末点")
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        focusPolicy: Qt.StrongFocus
                        enabled: root.vertices.length > 0 && !root.hasPendingCoordinates
                        onClicked: root.mutationAccepted(root.draft.removeVertex(root.vertices.length - 1))
                    }
                    QGCButton {
                        text: root.confirmClear ? qsTr("确认清空区域") : qsTr("清空区域…")
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        focusPolicy: Qt.StrongFocus
                        enabled: root.vertices.length > 0 && !root.hasPendingCoordinates
                        onClicked: {
                            root.mapPickingRequested(false)
                            if (root.confirmClear) { root.mutationAccepted(root.draft.clearVertices()); root.confirmClear = false }
                            else { root.confirmClear = true; root.invalidatePreview() }
                        }
                    }
                }
                QGCButton {
                    visible: root.confirmClear
                    text: qsTr("取消清空")
                    Layout.minimumHeight: root.rowHeight
                    focusPolicy: Qt.StrongFocus
                    onClicked: root.confirmClear = false
                }

                FleetCoverageExclusionsEditor {
                    id: exclusionsEditor
                    Layout.fillWidth: true
                    draft: root.draft
                    sourceIdentity: root.draft && root.draft.sourceToken ? root.draft.sourceToken : ""
                    inputSettled: root.exclusionsInputSettled
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }

                QGCLabel { Layout.fillWidth: true; text: qsTr("3 · 覆盖参数（必须人工确认）"); font.bold: true; wrapMode: Text.WordWrap }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("默认值仅为编辑示例，不能直接当作机型限制。轨迹为几何提案，不证明避障、转弯半径、地形净空、传感器覆盖或 4D 安全。")
                    wrapMode: Text.WordWrap
                }
                Repeater {
                    model: root.numericSettings
                    ColumnLayout {
                        Layout.fillWidth: true
                        readonly property var definition: modelData
                        QGCLabel { Layout.fillWidth: true; text: definition.label; wrapMode: Text.WordWrap }
                        Controls.TextField {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            text: root.fieldText(definition.key)
                            color: qgcPal.text
                            font.pointSize: ScreenTools.defaultFontPointSize
                            selectByMouse: true
                            maximumLength: 32
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            Accessible.name: definition.label
                            onTextEdited: root.editSetting(definition.key, text)
                            background: Rectangle {
                                color: qgcPal.windowShade
                                border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                border.width: parent.activeFocus ? 2 : 1
                                radius: 3
                            }
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: root.numberError(root.fieldText(definition.key), definition)
                            visible: text.length > 0
                            color: qgcPal.colorOrange
                            wrapMode: Text.WordWrap
                        }
                    }
                }
                FleetPerformancePolicyEditor {
                    id: performancePolicyEditor
                    Layout.fillWidth: true
                    draft: root.draft
                    sourceIdentity: root.draft && root.draft.sourceToken ? root.draft.sourceToken : ""
                    inputSettled: root.performancePolicyInputSettled
                    onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                }
                QGCCheckBox {
                    Layout.fillWidth: true
                    text: qsTr("输入为 WGS84 / AMSL（人工声明）")
                    checked: root.settings.mapDatumAcknowledged === true
                    enabled: root.draft !== null
                    onClicked: {
                        root.setBooleanSetting("mapDatumAcknowledged", checked)
                        checked = Qt.binding(function() { return root.settings.mapDatumAcknowledged === true })
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("这只是操作员声明，不是坐标验证。地图供应商偏移、GNSS 坐标、海拔基准及实机一致性必须由工程师确认；本面板不做 GCJ-02 / BD-09 转换。")
                    wrapMode: Text.WordWrap
                    color: qgcPal.colorOrange
                }
                QGCCheckBox {
                    Layout.fillWidth: true
                    text: qsTr("为每架覆盖机明确端点")
                    checked: root.settings.includeEndpoints === true
                    enabled: root.draft !== null
                    onClicked: {
                        root.setBooleanSetting("includeEndpoints", checked)
                        checked = Qt.binding(function() { return root.settings.includeEndpoints === true })
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.activeMember !== null || root.reserveEditing || root.reserveClearSession !== null
                        || root.endpointReserveSession !== null || root.auxiliaryInputPending
                    QGCLabel { Layout.fillWidth: true; text: qsTr("4 · 编辑 ID %1 的角色和路径").arg(root.activeMemberId); font.bold: true; wrapMode: Text.WordWrap }
                    QGCLabel { text: qsTr("成员角色") }
                    QGCComboBox {
                        Layout.fillWidth: true
                        Layout.minimumHeight: root.rowHeight
                        model: root.roles
                        textRole: "label"
                        currentIndex: root.activeMember ? root.roleIndex(root.activeMember.role) : -1
                        enabled: !root.reserveEditing && !root.reserveClearSession
                        onActivated: {
                            var newRole = root.roles[index].value
                            root.setActiveRole(newRole)
                            currentIndex = Qt.binding(function() { return root.activeMember ? root.roleIndex(root.activeMember.role) : -1 })
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.activeMember && root.activeMember.role === "reserve"
                        text: qsTr("备用成员不自动获得覆盖路线，也不自动起飞接替；可用性仍需单独审阅。")
                        wrapMode: Text.WordWrap
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: root.activeMember && root.activeMember.role !== "reserve" && !!root.activeMember.reservePlan
                        text: qsTr("保留了此成员以前的备用预算输入；当前非备用角色不使用该策略。切回备用后仍需重新审阅。")
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: root.activeReserveMember || root.reserveEditing || root.reserveClearSession !== null
                        spacing: root.pad
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("备用待命 / 接替预算 · 仅本地规划")
                            font.bold: true
                            textFormat: Text.PlainText
                            wrapMode: Text.WordWrap
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("路线假设：备用机自己的起点 → 指定主机的完整已编译路线 → 备用机自己的返回点。不是从故障断点续飞；选择目标不证明机型、载荷或任务能力等价。没有避障 / 净空认证，不会自动生成备用机上传任务或起飞接替。")
                            textFormat: Text.PlainText
                            wrapMode: Text.WordWrap
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            visible: root.activeReserveMember
                            text: root.activeMember && root.activeMember.reservePlan
                                ? qsTr("已保存本地策略：接替 ID %1，待命 %2，时长 %3 秒；必须结合自身端点重新生成审阅。")
                                    .arg(root.activeMember.reservePlan.replacementVehicleId)
                                    .arg(root.activeMember.reservePlan.standbyMode === "ground-disarmed" ? qsTr("地面未解锁")
                                         : root.activeMember.reservePlan.standbyMode === "airborne-hold" ? qsTr("空中悬停") : qsTr("未知"))
                                    .arg(root.activeMember.reservePlan.standbyDurationSeconds)
                                : qsTr("尚无备用策略，不能用最长主机时间代替真实备用预算；请填写策略和自身端点。")
                            textFormat: Text.PlainText
                            wrapMode: Text.WordWrap
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            visible: !root.reserveEditing && !root.reserveClearSession
                            enabled: root.visible && root.inputSettled && !root.draftSwitchBusy
                            text: qsTr("编辑备用预算输入")
                            focusPolicy: Qt.StrongFocus
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                            onClicked: root.beginReserveEdit()
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: root.reserveEditing
                            enabled: root.visible && !root.draftSwitchBusy
                            QGCLabel {
                                Layout.fillWidth: true
                                text: root.reserveSessionCurrent(root.reserveSession)
                                    ? qsTr("绑定 ID %1、本地草稿版本 %2；没有默认系数或速度。版本变化后须取消再编辑。")
                                        .arg(root.reserveSession.vehicleId).arg(root.reserveSession.revision)
                                    : qsTr("原备用编辑来源或版本已变化，不能保存。未提交输入仍保留在下方，请取消后重新选择成员编辑。")
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel { text: qsTr("接替对象 · 当前非备用成员") }
                            QGCComboBox {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                model: root.reserveTargets
                                textRole: "label"
                                currentIndex: root.reserveChoiceIndex(root.reserveTargets, "vehicleId", root.reserveTargetId)
                                Accessible.name: qsTr("选择备用机接替的当前非备用成员")
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                onActivated: {
                                    if (index >= 0 && index < root.reserveTargets.length) root.reserveTargetId = root.reserveTargets[index].vehicleId
                                    root.reserveConfirmed = false
                                    currentIndex = Qt.binding(function() { return root.reserveChoiceIndex(root.reserveTargets, "vehicleId", root.reserveTargetId) })
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                visible: root.reserveTargets.length === 0
                                text: qsTr("没有可选的非备用成员；请取消编辑后调整角色。")
                                wrapMode: Text.WordWrap
                            }
                            QGCLabel { text: qsTr("待命方式 · 必须明确选择") }
                            QGCComboBox {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                model: root.reserveModes
                                textRole: "label"
                                currentIndex: root.reserveChoiceIndex(root.reserveModes, "value", root.reserveMode)
                                Accessible.name: qsTr("备用机待命方式")
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                onActivated: {
                                    if (index >= 0 && index < root.reserveModes.length) root.reserveMode = root.reserveModes[index].value
                                    root.reserveConfirmed = false
                                    currentIndex = Qt.binding(function() { return root.reserveChoiceIndex(root.reserveModes, "value", root.reserveMode) })
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true
                                text: qsTr("耗电系数是工程师声明的“待命耗电 / 名义飞行耗电”比例，不是实测值。地面待命也不能填 0；等效飞行时长 = 待命秒数 × 系数。方式名称不是实际解锁 / 空中状态证明。")
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                            }
                            Repeater {
                                model: root.reserveDefinitions
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    readonly property var definition: modelData
                                    QGCLabel { Layout.fillWidth: true; text: definition.label; wrapMode: Text.WordWrap }
                                    Controls.TextField {
                                        Layout.fillWidth: true
                                        Layout.minimumHeight: root.rowHeight
                                        text: root.reserveFields[definition.key] || ""
                                        color: qgcPal.text
                                        font.pointSize: ScreenTools.defaultFontPointSize
                                        maximumLength: 32
                                        selectByMouse: true
                                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                                        Accessible.name: definition.label
                                        onTextEdited: root.editReserveField(definition.key, text)
                                        onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                        background: Rectangle {
                                            color: qgcPal.windowShade
                                            border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                            border.width: parent.activeFocus ? 2 : 1
                                            radius: 3
                                        }
                                    }
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        text: (root.reserveFields[definition.key] || "").length ? root.reserveFieldError(definition) : ""
                                        visible: text.length > 0
                                        color: qgcPal.warningText
                                        textFormat: Text.PlainText
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                            QGCCheckBox {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: qsTr("我确认这些规划假设")
                                checked: root.reserveConfirmed
                                enabled: root.reserveSessionCurrent(root.reserveSession)
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                                onClicked: {
                                    root.reserveConfirmed = checked
                                    checked = Qt.binding(function() { return root.reserveConfirmed })
                                }
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: qsTr("保存备用预算输入")
                                enabled: root.reserveConfirmed && root.reserveSessionCurrent(root.reserveSession)
                                focusPolicy: Qt.StrongFocus
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                onClicked: root.saveReservePlan()
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: qsTr("取消备用预算编辑")
                                focusPolicy: Qt.StrongFocus
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                onClicked: root.cancelReserveEdit()
                                Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                            }
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            visible: !root.reserveEditing && root.activeMember && !!root.activeMember.reservePlan
                            enabled: root.visible && root.activeReserveMember && !root.draftSwitchBusy
                                && (root.inputSettled || root.reserveClearSession !== null)
                            text: root.reserveClearSession ? qsTr("确认清除备用预算") : qsTr("清除已保存备用预算")
                            focusPolicy: Qt.StrongFocus
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                            onClicked: root.clearReservePlan()
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            visible: root.reserveClearSession !== null
                            text: qsTr("取消清除")
                            focusPolicy: Qt.StrongFocus
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                            onClicked: { root.reserveClearSession = null; root.reserveMessage = "" }
                            Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            visible: root.reserveMessage.length > 0
                            text: root.reserveMessage
                            textFormat: Text.PlainText
                            wrapMode: Text.WordWrap
                        }
                    }
                    FleetExecutionProfileEditor {
                        id: executionProfileEditor
                        Layout.fillWidth: true
                        draft: root.draft
                        sourceIdentity: root.draft && typeof root.draft.sourceToken === "string" ? root.draft.sourceToken : ""
                        onFocusVisibilityRequested: root.ensureEnergyBudgetFocusVisible(item)
                        member: root.activeMember
                        enabled: !root.reserveEditing && !root.reserveClearSession && !root.endpointDirty
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: root.endpointReserveSession !== null || root.activeReserveMember
                            || (root.activeCoverageMember && root.settings.includeEndpoints === true)
                        enabled: !root.reserveEditing && !root.reserveClearSession && !executionProfileEditor.pendingInput
                        QGCLabel {
                            Layout.fillWidth: true
                            text: root.endpointReserveSession && !root.reserveSessionCurrent(root.endpointReserveSession)
                                ? qsTr("备用端点原编辑来源或版本已变化，不能保存。输入仍保留，请放弃后重新编辑当前成员。")
                                : root.activeReserveMember
                                    ? qsTr("备用机自身端点 · 不取 Home、主机端点或实时机位。无论覆盖端点开关状态，都要保存完整起点与返回点；编辑不证明到场、起飞、障碍或净空安全。端点和策略分次保存，最终审阅绑定完整草稿版本。")
                                    : qsTr("端点不自动取 Home 或实时机位。所有覆盖机都必须保存完整端点；区域外的直连段和竖直段仍属于未验证净空。")
                            wrapMode: Text.WordWrap
                        }
                        Repeater {
                            model: root.endpointFields
                            ColumnLayout {
                                Layout.fillWidth: true
                                readonly property var definition: modelData
                                QGCLabel { text: definition.label }
                                Controls.TextField {
                                    Layout.fillWidth: true
                                    Layout.minimumHeight: root.rowHeight
                                    text: root.endpointTexts[definition.key] || ""
                                    color: qgcPal.text
                                    font.pointSize: ScreenTools.defaultFontPointSize
                                    maximumLength: 32
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    Accessible.name: definition.label
                                    onTextEdited: root.editEndpoint(definition.key, text)
                                    background: Rectangle {
                                        color: qgcPal.windowShade
                                        border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                        border.width: parent.activeFocus ? 2 : 1
                                        radius: 3
                                    }
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root.endpointDirty ? root.numberError(root.endpointTexts[definition.key] || "", definition) : ""
                                    visible: text.length > 0
                                    color: qgcPal.colorOrange
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: qsTr("保存完整端点")
                                focusPolicy: Qt.StrongFocus
                                onClicked: root.saveEndpoints()
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.minimumHeight: root.rowHeight
                                text: qsTr("放弃端点编辑")
                                focusPolicy: Qt.StrongFocus
                                onClicked: { root.resetEndpointTexts(); root.localError = "" }
                            }
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        // A backing draft/member may be replaced externally.
                        // Keep stale input and its explicit discard reachable,
                        // even when the new member is no longer auxiliary.
                        visible: root.activeAuxiliaryMember || root.auxiliaryInputPending
                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("辅助角色不能借用覆盖航线。请按顺序分别录入独立出航和返航点，含明确 AMSL 海拔；可编辑已有点而不改变顺序。本面板不推断实际时序。")
                            wrapMode: Text.WordWrap
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            textFormat: Text.PlainText; wrapMode: Text.WordWrap
                            visible: root.editingAuxiliaryPoint >= 0
                            text: qsTr("正在编辑%1第 %2 点；保存才替换原点，放弃不修改原点。")
                                .arg(root.auxiliaryIsReturn ? qsTr("返航") : qsTr("出航")).arg(root.editingAuxiliaryPoint + 1)
                        }
                        QGCLabel { text: qsTr("正在录入的路径") }
                        QGCComboBox {
                            Layout.fillWidth: true
                            Layout.minimumHeight: root.rowHeight
                            model: [qsTr("出航路径"), qsTr("返航路径")]
                            currentIndex: root.auxiliaryIsReturn ? 1 : 0
                            enabled: root.activeAuxiliaryMember && !root.auxiliaryInputPending
                            onActivated: {
                                if (root.auxiliaryInputPending) {
                                    root.fail(qsTr("先保存或放弃正在录入 / 编辑的航点。"))
                                } else root.auxiliaryIsReturn = index === 1
                                currentIndex = Qt.binding(function() { return root.auxiliaryIsReturn ? 1 : 0 })
                            }
                        }
                        Repeater {
                            model: root.pointFields
                            ColumnLayout {
                                Layout.fillWidth: true
                                readonly property var definition: modelData
                                QGCLabel { Layout.fillWidth: true; text: definition.label; wrapMode: Text.WordWrap }
                                Controls.TextField {
                                    Layout.fillWidth: true
                                    Layout.minimumHeight: root.rowHeight
                                    text: root.auxiliaryTexts[definition.key]
                                    color: qgcPal.text
                                    font.pointSize: ScreenTools.defaultFontPointSize
                                    selectByMouse: true
                                    maximumLength: 32
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    Accessible.name: definition.label
                                    enabled: root.activeAuxiliaryMember && (!root.auxiliarySession
                                        || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                                    onTextEdited: root.setPointText("auxiliary", definition.key, text)
                                    onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                    background: Rectangle {
                                        color: qgcPal.windowShade
                                        border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                        border.width: parent.activeFocus ? 2 : 1
                                        radius: 3
                                    }
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: root.auxiliaryTexts[definition.key].length > 0
                                        ? root.numberError(root.auxiliaryTexts[definition.key], definition) : ""
                                    visible: text.length > 0
                                    color: qgcPal.colorOrange
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                        QGCCheckBox {
                            id: auxiliaryHeadingToggle
                            Layout.fillWidth: true
                            Layout.minimumHeight: Math.max(44, root.rowHeight)
                            focusPolicy: Qt.StrongFocus
                            text: qsTr("为这个 NAV 航点指定机头朝向")
                            checked: root.auxiliaryExplicitHeading
                            enabled: root.activeAuxiliaryMember && (!root.auxiliarySession
                                || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                            Accessible.name: text
                            onClicked: {
                                root.setAuxiliaryHeadingEnabled(checked)
                                checked = Qt.binding(function() { return root.auxiliaryExplicitHeading })
                            }
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                        }
                        QGCLabel {
                            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                            text: qsTr("未勾选：不发送显式朝向，沿飞控自身策略；与显式 0° 不同。此处是机头朝向，不是上方“扫描线方位角”；自动生成的覆盖航点仍沿飞控策略。")
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; visible: root.auxiliaryExplicitHeading
                            QGCLabel { Layout.fillWidth: true; text: qsTr("本点机头朝向请求 (°，0 ≤ 值 < 360)"); wrapMode: Text.WordWrap }
                            Controls.TextField {
                                id: auxiliaryHeadingInput
                                Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, root.rowHeight)
                                text: root.auxiliaryTexts.heading; color: qgcPal.text
                                font.pointSize: ScreenTools.defaultFontPointSize
                                selectByMouse: true; maximumLength: 32
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Accessible.name: qsTr("本点机头朝向请求，单位度，包含零不包含三百六十")
                                enabled: root.activeAuxiliaryMember && (!root.auxiliarySession
                                    || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                                onTextEdited: root.setPointText("auxiliary", "heading", text)
                                onEditingFinished: root.auxiliaryHeadingTouched = true
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                background: Rectangle {
                                    color: qgcPal.windowShade
                                    border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                    border.width: parent.activeFocus ? 2 : 1; radius: 3
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: root.auxiliaryHeadingTouched ? root.auxiliaryHeadingError() : ""
                                visible: text.length > 0; color: qgcPal.warningText; Accessible.name: text
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: qsTr("原请求保存在草稿；上传按最接近的 1/256° 网格编码（约 0.004°，靠近 360° 可编码为 0°），上传审阅展示量化结果。编码步长不是实飞精度、转弯或转向完成保证，飞控自身策略仍可能覆盖此请求；起飞 / 降落朝向不在此处设置。")
                            }
                        }
                        QGCCheckBox {
                            id: auxiliaryHoldToggle
                            Layout.fillWidth: true
                            Layout.minimumHeight: Math.max(44, root.rowHeight)
                            focusPolicy: Qt.StrongFocus
                            text: qsTr("为这个 NAV 航点指定相对驻留")
                            checked: root.auxiliaryExplicitHold
                            enabled: root.activeAuxiliaryMember && (!root.auxiliarySession
                                || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                            Accessible.name: text
                            onClicked: {
                                root.setAuxiliaryHoldEnabled(checked)
                                checked = Qt.binding(function() { return root.auxiliaryExplicitHold })
                            }
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                        }
                        QGCLabel {
                            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                            text: qsTr("未勾选：不在本点保存明确驻留要求，保留原任务缺省；与明确 0 秒不同。只声明到达此 NAV 航点后的相对驻留，不是绝对到达时刻、跨机同步或精准悬停承诺。")
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; visible: root.auxiliaryExplicitHold
                            QGCLabel {
                                Layout.fillWidth: true; text: qsTr("本点相对驻留请求 (秒，0 至 86400)")
                                wrapMode: Text.WordWrap
                            }
                            Controls.TextField {
                                id: auxiliaryHoldInput
                                Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, root.rowHeight)
                                text: root.auxiliaryTexts.holdSeconds; color: qgcPal.text
                                font.pointSize: ScreenTools.defaultFontPointSize
                                selectByMouse: true; maximumLength: 32
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                Accessible.name: qsTr("本点相对驻留请求，单位秒，范围零至八万六千四百")
                                enabled: root.activeAuxiliaryMember && (!root.auxiliarySession
                                    || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                                onTextEdited: root.setPointText("auxiliary", "holdSeconds", text)
                                onEditingFinished: root.auxiliaryHoldTouched = true
                                onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(this)
                                background: Rectangle {
                                    color: qgcPal.windowShade
                                    border.color: parent.activeFocus ? qgcPal.buttonHighlight : qgcPal.text
                                    border.width: parent.activeFocus ? 2 : 1; radius: 3
                                }
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: root.auxiliaryHoldTouched ? root.auxiliaryHoldError() : ""
                                visible: text.length > 0; color: qgcPal.warningText; Accessible.name: text
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: qsTr("原秒数保存在草稿；极小正值若经 Float32 或飞控整数微秒计时转换会归零，原生保存会拒绝并保留输入，不夹大、不改成 0。上传编码与实际保持 / 计时条件须在审阅及工程验证中核对。位置保持、何时开始计时和完成条件由飞控及配置决定；不能用驻留输入补造时序、续航或精度证据。起降、HOLD 动作和自动生成覆盖点不在此处设置。")
                            }
                        }
                        QGCLabel {
                            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                            visible: root.auxiliarySession !== null && !root.auxiliarySessionCurrent(root.auxiliarySession, true)
                            text: qsTr("航点编辑来源或版本已变化；输入仍保留，不能保存到新草稿。请放弃输入后重新选择。")
                            color: qgcPal.warningText
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            QGCButton {
                                text: root.editingAuxiliaryPoint >= 0 ? qsTr("保存本点修改")
                                    : root.auxiliaryIsReturn ? qsTr("添加返航点") : qsTr("添加出航点")
                                Layout.fillWidth: true
                                Layout.minimumHeight: Math.max(44, root.rowHeight)
                                focusPolicy: Qt.StrongFocus
                                enabled: root.activeAuxiliaryMember && !root.draftSwitchBusy
                                    && (!root.auxiliarySession || root.auxiliarySessionCurrent(root.auxiliarySession, true))
                                onClicked: root.appendAuxiliaryPoint()
                            }
                            QGCButton {
                                text: qsTr("放弃本点输入")
                                Layout.fillWidth: true
                                Layout.minimumHeight: Math.max(44, root.rowHeight)
                                focusPolicy: Qt.StrongFocus
                                onClicked: root.discardAuxiliaryInput()
                            }
                        }
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: !root.activeAuxiliaryMember && root.activeMember
                            && (root.array(root.activeMember.waypoints).length > 0 || root.array(root.activeMember.returnPath).length > 0)
                        text: qsTr("角色变更后保留的独立路径不用于覆盖或备用提案；可在下面删除，或切回辅助角色继续编辑。")
                        wrapMode: Text.WordWrap
                    }
                    Repeater {
                        model: [false, true]
                        ColumnLayout {
                            id: routeSection
                            Layout.fillWidth: true
                            readonly property bool isReturn: modelData
                            readonly property var points: root.activeMember
                                ? root.array(isReturn ? root.activeMember.returnPath : root.activeMember.waypoints) : []
                            readonly property int pageCount: Math.max(1, Math.ceil(points.length / 12))
                            readonly property int page: Math.min(isReturn ? root.returnPage : root.outboundPage, pageCount - 1)
                            visible: root.activeAuxiliaryMember || points.length > 0
                            QGCLabel {
                                Layout.fillWidth: true
                                text: routeSection.isReturn ? qsTr("独立返航路径（%1 点）").arg(routeSection.points.length)
                                                           : qsTr("独立出航路径（%1 点）").arg(routeSection.points.length)
                                font.bold: true
                                wrapMode: Text.WordWrap
                            }
                            Repeater {
                                model: routeSection.points.slice(routeSection.page * 12, routeSection.page * 12 + 12)
                                RowLayout {
                                    Layout.fillWidth: true
                                    QGCLabel {
                                        Layout.fillWidth: true
                                        textFormat: Text.PlainText
                                        text: qsTr("%1 · %2\n%3\n%4").arg(routeSection.page * 12 + index + 1)
                                            .arg(root.pointLabel(modelData)).arg(root.waypointHeadingLabel(modelData))
                                            .arg(root.waypointHoldLabel(modelData))
                                        wrapMode: Text.WrapAnywhere
                                    }
                                    QGCButton {
                                        text: qsTr("编辑")
                                        Layout.minimumHeight: Math.max(44, root.rowHeight)
                                        focusPolicy: Qt.StrongFocus
                                        visible: root.activeAuxiliaryMember
                                        enabled: !root.hasPendingCoordinates && !root.hasPendingSettingText && !root.draftSwitchBusy
                                        property var pressedSource: null
                                        onPressed: pressedSource = root.captureAuxiliaryRow(routeSection.page * 12 + index, routeSection.isReturn)
                                        onCanceled: pressedSource = null
                                        onClicked: {
                                            var captured = pressedSource
                                            pressedSource = null
                                            root.beginAuxiliaryPointEdit(captured)
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("删除")
                                        Layout.minimumHeight: Math.max(44, root.rowHeight)
                                        focusPolicy: Qt.StrongFocus
                                        enabled: !root.hasPendingCoordinates && !root.hasPendingSettingText && !root.draftSwitchBusy
                                        property var pressedSource: null
                                        onPressed: pressedSource = root.captureAuxiliaryRow(routeSection.page * 12 + index, routeSection.isReturn)
                                        onCanceled: pressedSource = null
                                        onClicked: {
                                            var captured = pressedSource
                                            pressedSource = null
                                            root.removeAuxiliaryPoint(captured)
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                visible: routeSection.points.length > 12
                                QGCButton {
                                    text: qsTr("上一页")
                                    Layout.minimumHeight: root.rowHeight
                                    focusPolicy: Qt.StrongFocus
                                    enabled: routeSection.page > 0
                                    onClicked: {
                                        if (routeSection.isReturn) root.returnPage = routeSection.page - 1
                                        else root.outboundPage = routeSection.page - 1
                                    }
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: qsTr("%1 / %2 页 · 每页 12 点").arg(routeSection.page + 1).arg(routeSection.pageCount)
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.WordWrap
                                }
                                QGCButton {
                                    text: qsTr("下一页")
                                    Layout.minimumHeight: root.rowHeight
                                    focusPolicy: Qt.StrongFocus
                                    enabled: routeSection.page + 1 < routeSection.pageCount
                                    onClicked: {
                                        if (routeSection.isReturn) root.returnPage = routeSection.page + 1
                                        else root.outboundPage = routeSection.page + 1
                                    }
                                }
                            }
                        }
                    }
                }

                QGCLabel { Layout.fillWidth: true; text: qsTr("5 · 区域草稿的只读审阅结果"); font.bold: true; wrapMode: Text.WordWrap }
                QGCLabel {
                    Layout.fillWidth: true; visible: root.remainderDeploymentSelected
                    textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    text: root.deploymentSourceKind === "reserve-dispatch"
                        ? qsTr("当前共享部署区已明确采用独立备用出勤新任务。以下区域草稿不是上传来源，区域批准保持禁用；请在上方备用出勤区核对、批准或明确退出。采用不等于已接管原任务或获得飞行许可。")
                        : qsTr("当前共享部署面板已明确选择余段审阅记录。以下区域草稿结果不是本次上传来源；区域批准按钮保持禁用。余段状态、批准和退出来源请使用上方余段面板。")
                    color: qgcPal.warningText
                }
                QGCLabel {
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                    text: root.previewCurrent ? qsTr("当前版本提案 · %1").arg(root.review.reviewStatus || qsTr("待检查"))
                        : qsTr("尚无当前有效提案，或输入 / 健康 / 版本已变化；旧结果仅供参考。")
                    wrapMode: Text.WordWrap
                    color: root.previewCurrent ? qgcPal.text : qgcPal.colorOrange
                }
                QGCLabel {
                    Layout.fillWidth: true
                    visible: root.previewCurrent && root.controller
                        && root.controller.missionDraftApprovalAllowed !== true
                    text: qsTr("计划、健康、能力或审批门禁未通过，仍需复核。版本仍可显示，不代表条件仍然有效；重新审阅后也不自动释放飞行指令。")
                    color: qgcPal.colorOrange
                    wrapMode: Text.WordWrap
                }
                QGCLabel {
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                    text: qsTr("计划 ID：%1\n归档版本：%2\n归档记录：%3\n计划审阅状态：%4\n飞行授权：未释放；上传须末部独立确认")
                        .arg(root.plan.planId || qsTr("无"))
                        .arg(root.archive.recordRevision === undefined ? qsTr("未归档") : root.archive.recordRevision)
                        .arg(root.archive.recordId || qsTr("无"))
                        .arg(root.controller ? root.controller.intentTaskState : qsTr("未知"))
                    wrapMode: Text.WrapAnywhere
                }
                QGCLabel {
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                    text: qsTr("空域状态：%1 · 完整分析：%2\n能量状态：%3\n几何 / 时间估算不能代替真实净空、性能和时间证据。")
                        .arg((root.review.airspaceReport || {}).reviewStatus || qsTr("未知"))
                        .arg((root.review.airspaceReport || {}).analysisComplete === true ? qsTr("是") : qsTr("否 / 未知"))
                        .arg((root.review.energyPlan || {}).reviewStatus || (root.review.energyPlan || {}).status || qsTr("未知"))
                    wrapMode: Text.WordWrap
                }
                QGCButton {
                    id: energyBillButton
                    Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, root.rowHeight); focusPolicy: Qt.StrongFocus
                    text: root.energyBillExpanded ? qsTr("收起逐机能量账单（只读）") : qsTr("展开逐机能量账单（只读，每页 8 架）")
                    Accessible.name: text
                    onClicked: root.energyBillExpanded = !root.energyBillExpanded
                    onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(energyBillButton)
                    contentItem: QGCLabel { text: energyBillButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: energyBillButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    background: Rectangle { color: energyBillButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: energyBillButton.activeFocus ? 3 : 1; radius: 3 }
                }
                ColumnLayout {
                    Layout.fillWidth: true; visible: root.energyBillExpanded; spacing: root.pad
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        color: root.previewCurrent ? qgcPal.text : qgcPal.warningText
                        text: (root.previewCurrent ? qsTr("当前版本审阅中的预算（仍非放行许可）。") : qsTr("旧预览参考：当前输入、健康或版本已变化；本账单不能用于当前放行。"))
                            + "\n" + qsTr("原方案成员绑定：%1。仅展示后端账单，不重选成员、不自动替补、不上传或启动。时间下限可能使多架预算相同；几何和电池估算不保证实际续航。")
                                .arg(root.energyFlag(root.energyPlan.boundAssignments))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("能量状态：%1；审阅状态：%2。\n全组账单瓶颈：%3。\n已知成员累计短缺：%4；短缺统计完整：%5；续航未知成员：%6。统计不完整时累计值仅为已知下界，0 不等于全组没有短缺；逐机短缺之和不是整组延误。")
                            .arg(root.energyStatus(root.energyPlan.status)).arg(root.energyStatus(root.energyPlan.reviewStatus))
                            .arg(root.energyPlan.bottleneckKnown === true && root.energyInteger(root.energyPlan.bottleneckVehicleId, 1, 255)
                                ? qsTr("ID %1 · %2").arg(root.energyPlan.bottleneckVehicleId).arg(root.energyMargin(root.energyPlan.bottleneckMarginSeconds, true)) : qsTr("未知"))
                            .arg(root.energySeconds(root.energyPlan.totalShortfallSeconds)).arg(root.energyFlag(root.energyPlan.shortfallComplete))
                            .arg(root.energyInteger(root.energyPlan.unknownEnergyVehicleCount, 0, 255) ? String(root.energyPlan.unknownEnergyVehicleCount) : qsTr("未知"))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !root.energyBillRows || root.energyBillRows.length === 0
                        textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: !root.energyBillRows ? qsTr("没有可识别的有界成员账单（上限 255）；旧结果缺字段或形状未知，不代表能量充足。")
                            : qsTr("本次账单为空；不补造成员，也不把空列表视为全组通过。")
                    }
                    Repeater {
                        model: root.energyBillExpanded && root.energyBillRows
                            ? root.energyBillRows.slice(root.energyBillCurrentPage * 8, root.energyBillCurrentPage * 8 + 8) : []
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: root.pad / 2
                            readonly property var bill: modelData && typeof modelData === "object" ? modelData : ({})
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; font.bold: true
                                text: qsTr("ID %1 · %2\n符合本次能量筛选：%3（不是任务或飞行放行）；原因：%4")
                                    .arg(root.energyInteger(bill.vehicleId, 1, 255) ? String(bill.vehicleId) : qsTr("未知"))
                                    .arg(root.energyGroup(bill.group)).arg(root.energyFlag(bill.eligible)).arg(root.energyReason(bill.reason))
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: qsTr("任务预算 %1 ＋ 返程预留 %2；合计需求 %3。\n可用续航：%4；%5。\n输入电量：%6；能量证据已知：%7；依据：%8。\n预测末端电量已知：%9；预测值：%10；预测依据：%11。\n建议后续替补复核：%12（只读建议，不执行替补）。")
                                    .arg(root.energySeconds(bill.missionDurationSeconds)).arg(root.energySeconds(bill.returnReserveSeconds)).arg(root.energySeconds(bill.requiredFlightSeconds))
                                    .arg(bill.energyEvidenceKnown === true ? root.energySeconds(bill.availableFlightSeconds) : qsTr("未知"))
                                    .arg(root.energyMargin(bill.marginSeconds, bill.energyEvidenceKnown)).arg(root.energyPercent(bill.batteryPercent))
                                    .arg(root.energyFlag(bill.energyEvidenceKnown)).arg(root.energyBasis(bill.energyBasis)).arg(root.energyFlag(bill.projectionKnown))
                                    .arg(bill.projectionKnown === true ? root.energyPercent(bill.projectedBatteryPercent) : qsTr("未知（不沿用当前电量代替预测）"))
                                    .arg(root.energyBasis(bill.projectionBasis)).arg(root.energyFlag(bill.replacementRecommended))
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.Wrap
                                text: root.energyNativeSourceText(bill.nativeBatteryEvidence)
                            }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                text: root.energyBudgetDetailsText(bill.budgetDetails)
                            }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: qgcPal.text; opacity: 0.2 }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; visible: root.energyBillRows && root.energyBillRows.length > 0
                        onYChanged: root.keepEnergyPaginationFocusVisible()
                        onHeightChanged: root.keepEnergyPaginationFocusVisible()
                        QGCButton {
                            id: energyPreviousButton
                            QGCPalette { id: energyPreviousPalette; colorGroupEnabled: energyPreviousButton.enabled }
                            Layout.minimumHeight: Math.max(44, root.rowHeight); focusPolicy: Qt.StrongFocus
                            text: qsTr("上一页"); Accessible.name: text; enabled: root.energyBillCurrentPage > 0
                            onClicked: root.energyBillPage = root.energyBillCurrentPage - 1
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(energyPreviousButton)
                            contentItem: QGCLabel { text: energyPreviousButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; opacity: energyPreviousButton.enabled ? 1 : 0.5; color: energyPreviousButton.enabled && energyPreviousButton._showHighlight ? energyPreviousPalette.buttonHighlightText : energyPreviousPalette.buttonText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: energyPreviousButton.enabled && energyPreviousButton._showHighlight ? energyPreviousPalette.buttonHighlight : energyPreviousPalette.button; border.color: energyPreviousPalette.buttonText; border.width: energyPreviousButton.activeFocus ? 3 : 1; radius: 3 }
                        }
                        QGCLabel {
                            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
                            text: qsTr("%1 / %2 页 · 共 %3 架 · 每页最多 8 架").arg(root.energyBillCurrentPage + 1)
                                .arg(root.energyBillPageCount).arg(root.energyBillRows ? root.energyBillRows.length : 0)
                        }
                        QGCButton {
                            id: energyNextButton
                            QGCPalette { id: energyNextPalette; colorGroupEnabled: energyNextButton.enabled }
                            Layout.minimumHeight: Math.max(44, root.rowHeight); focusPolicy: Qt.StrongFocus
                            text: qsTr("下一页"); Accessible.name: text; enabled: root.energyBillCurrentPage + 1 < root.energyBillPageCount
                            onClicked: root.energyBillPage = root.energyBillCurrentPage + 1
                            onActiveFocusChanged: if (activeFocus) root.ensureEnergyBudgetFocusVisible(energyNextButton)
                            contentItem: QGCLabel { text: energyNextButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; opacity: energyNextButton.enabled ? 1 : 0.5; color: energyNextButton.enabled && energyNextButton._showHighlight ? energyNextPalette.buttonHighlightText : energyNextPalette.buttonText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: energyNextButton.enabled && energyNextButton._showHighlight ? energyNextPalette.buttonHighlight : energyNextPalette.button; border.color: energyNextPalette.buttonText; border.width: energyNextButton.activeFocus ? 3 : 1; radius: 3 }
                        }
                    }
                }
                QGCLabel {
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                    visible: root.reviewResult.planArchiveRecorded === false
                    text: qsTr("归档未完成：%1，不能沿用旧审批。").arg(root.archive.error || qsTr("未知原因"))
                    color: qgcPal.colorOrange
                    wrapMode: Text.WordWrap
                }
                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("审阅原因 %1 条，显示前 %2 条。完整原因保留在当前审阅结果中；只有成功归档的计划才有对应版本记录。")
                        .arg(root.reviewBlockers.length).arg(Math.min(24, root.reviewBlockers.length))
                    wrapMode: Text.WordWrap
                }
                Repeater {
                    model: root.reviewBlockers.slice(0, 24)
                    QGCLabel {
                        Layout.fillWidth: true
                        textFormat: Text.PlainText
                        text: "• " + modelData
                        wrapMode: Text.WordWrap
                    }
                }
                Repeater {
                    model: root.array(root.plan.members)
                    QGCLabel {
                        Layout.fillWidth: true
                        textFormat: Text.PlainText
                        text: qsTr("ID %1 · %2\n出航 %3 点 / 返航 %4 点\n进场 %5 · 覆盖 %6 · 返航 %7")
                            .arg(modelData.vehicleId).arg(modelData.role)
                            .arg(root.array(modelData.waypoints).length).arg(root.array(modelData.returnPath).length)
                            .arg(root.estimate(modelData.estimatedIngressDurationSeconds))
                            .arg(root.estimate(modelData.estimatedSurveyDurationSeconds))
                            .arg(root.estimate(modelData.estimatedReturnDurationSeconds))
                        wrapMode: Text.WordWrap
                    }
                }
                FleetMissionDeploymentPanel {
                    Layout.fillWidth: true
                    controller: root.controller
                    sourceKind: root.deploymentSourceKind
                    contextToken: root.deploymentContextToken
                    approvalAllowed: root.deploymentApprovalAllowed
                    refreshVehicleIds: root.deploymentRefreshVehicleIds
                    archive: root.deploymentArchive
                    plan: root.deploymentPlan
                    encoding: root.deploymentEncoding
                    previewCurrent: root.deploymentPreviewCurrent
                    inputSettled: root.inputSettled
                }
            }
        }

        QGCLabel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            textFormat: Text.PlainText
            visible: root.mapPicking
            text: qsTr("右键新增顶点（%1 / 64）。仅编辑草稿，不发飞行指令。停止选点后可键盘录入；触屏请用数值输入。").arg(root.vertices.length)
                + (root.draft && root.draft.lastError ? qsTr("\n有选点错误，请停止选点查看提示。") : "")
            wrapMode: Text.WordWrap
            maximumLineCount: 5
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        QGCButton {
            Layout.fillWidth: true
            Layout.minimumHeight: root.rowHeight
            visible: root.mapPicking
            focusPolicy: Qt.StrongFocus
            text: qsTr("停止选点，返回编辑")
            onClicked: root.mapPickingRequested(false)
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: !root.mapPicking
            text: qsTr("草稿版本 %1%2").arg(root.draft ? root.draft.revision : qsTr("不可用"))
                .arg(root.hasPendingCoordinates || root.hasPendingSettingText ? qsTr(" · 有未提交输入") : "")
            wrapMode: Text.WordWrap
        }
        QGCLabel {
            Layout.fillWidth: true
            visible: text.length > 0
            text: root.planningStatusText()
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: qgcPal.text
            Accessible.name: text
        }
        QGCButton {
            id: cancelPlanningButton
            Layout.fillWidth: true
            Layout.minimumHeight: root.rowHeight
            focusPolicy: Qt.StrongFocus
            visible: root.planningBusy
            enabled: !!root.controller && typeof root.controller.cancelMissionPlanning === "function"
                && typeof root.planningStatus.requestId === "string" && root.planningStatus.requestId.length > 0
                && root.planningStatus.status !== "cancelling"
            text: root.planningStatus.status === "cancelling" ? qsTr("正在取消后台规划…") : qsTr("取消当前后台规划（保留输入）")
            property var pressedSource: null
            onPressed: pressedSource = {controller: root.controller, requestId: root.planningStatus.requestId}
            onCanceled: pressedSource = null
            onClicked: {
                var source = pressedSource
                pressedSource = null
                if (!source || source.controller !== root.controller || source.requestId !== root.planningStatus.requestId) return
                source.controller.cancelMissionPlanning(source.requestId)
            }
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2
                visible: cancelPlanningButton.activeFocus
            }
        }
        QGCButton {
            Layout.fillWidth: true
            Layout.minimumHeight: root.rowHeight
            visible: !root.mapPicking
            focusPolicy: Qt.StrongFocus
            primary: true
            text: root.planningBusy ? qsTr("后台规划处理中（不执行）") : qsTr("生成路线并审阅（不执行）")
            enabled: root.draft !== null && !root.draft.revisionExhausted && !root.planningBusy
            onClicked: root.makePreview()
        }
        RowLayout {
            Layout.fillWidth: true
            visible: !root.mapPicking
            QGCButton {
                Layout.fillWidth: true
                Layout.minimumHeight: root.rowHeight
                focusPolicy: Qt.StrongFocus
                text: qsTr("申请计划审阅")
                enabled: root.reviewApprovalAvailable && root.controller.intentTaskState === "preview"
                onClicked: {
                    if (!root.controller.requestMissionDraftApproval(root.reviewResult.draftRevision, root.archive.recordId))
                        root.fail(qsTr("未申请审阅：版本、健康或输入已经变化，请重新生成提案。"))
                }
            }
            QGCButton {
                Layout.fillWidth: true
                Layout.minimumHeight: root.rowHeight
                focusPolicy: Qt.StrongFocus
                text: qsTr("人工确认计划")
                enabled: root.reviewApprovalAvailable && root.controller.intentTaskState === "awaiting-approval"
                onClicked: {
                    if (!root.controller.approveMissionDraft(root.reviewResult.draftRevision, root.archive.recordId))
                        root.fail(qsTr("未确认计划：此记录已失效或条件不完整，请重新审阅。"))
                }
            }
        }
    }
}

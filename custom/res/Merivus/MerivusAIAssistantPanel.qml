import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.11
import Qt.labs.settings 1.0

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import Merivus                      1.0

Item {
    id: root

    property bool expanded: false
    property bool settingsOpen: false
    readonly property bool agentRequestRunning: aiAgentClient.requestInProgress
    property bool _bubbleHovered: false
    property bool _panelHovered: false
    property var vehicles: QGroundControl.multiVehicleManager.vehicles
    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var pendingIntent: null
    property string pendingSummary: ""
    // AI proposals remain display-only.  The operator may explicitly create
    // a Fleet OS review draft, but this panel never requests approval and
    // never releases a flight command.
    readonly property var guidedController: globals.guidedControllerFlyView
    property var pendingFleetProposal: null
    property var pendingFleetReview: null
    property string pendingFleetReviewSummary: ""
    property var _fleetReviewLifetime: ({alive: true})
    property var _fleetProposalInputVersion: ({})
    property var _fleetReviewTicket: null
    readonly property var fleetPlanningStatus: guidedController && guidedController.missionPlanningStatus
        ? guidedController.missionPlanningStatus : ({})
    readonly property bool fleetPlanningBusy: fleetPlanningStatus.active === true || fleetPlanningStatus.ready === true
        || fleetPlanningStatus.status === "completing"
    onPendingFleetProposalChanged: retireFleetReviewDisplay()
    onPendingIntentChanged: retireFleetReviewDisplay()
    onGuidedControllerChanged: retireFleetReviewDisplay()
    Component.onDestruction: {
        _fleetReviewLifetime.alive = false
        _fleetReviewTicket = null
    }
    property real headerOffset: mainWindow.header && mainWindow.header.visible ? mainWindow.header.height + 6 : 8
    property real availablePanelHeight: parent ? Math.max(320, parent.height - headerOffset - 18) : 820
    property real defaultPanelWidth: parent ? clamp(parent.width * 0.21, 340, 380) : 346
    property real defaultPanelHeight: parent ? clamp(parent.height * 0.90, Math.min(780, availablePanelHeight), availablePanelHeight) : 820
    property real defaultPanelTopMargin: parent ? clamp(headerOffset + 10, headerOffset, Math.max(headerOffset, parent.height - defaultPanelHeight - 8)) : headerOffset + 10
    property real maxPanelWidth: parent ? Math.max(320, Math.min(parent.width * 0.40, 640)) : 640
    property real minPanelWidth: Math.min(320, maxPanelWidth)
    property real minPanelHeight: Math.min(320, availablePanelHeight)
    property real maxPanelHeight: Math.max(minPanelHeight, availablePanelHeight)
    property real panelWidth: clamp(assistantSettings.panelWidth, minPanelWidth, maxPanelWidth)
    property real panelHeight: clamp(assistantSettings.panelHeight, minPanelHeight, maxPanelHeight)
    property real panelTopMargin: clamp(assistantSettings.panelTopMargin, headerOffset,
                                        parent ? Math.max(headerOffset, parent.height - panelHeight - 8) : headerOffset)
    readonly property bool agentCanSend: assistantSettings.agentEnabled && aiSupervisor.healthReady && !agentRequestRunning
    readonly property int _settingsLabelWidth: 68
    readonly property real _settingsFieldHeight: 30
    readonly property string _agentGuide:
        "当前阶段使用本机 Mock Agent HTTP 服务，QML 只负责界面，C++ AiServiceSupervisor 负责本机Agent生命周期，AiAgentClient 负责HTTP请求。不接真实模型，不执行飞行动作。\n\n" +
        "启动方式：发布包使用 applicationDirPath/agent/merivus-agent.exe；开发环境会自动从仓库 agent 目录回退启动，也可显式配置 MERIVUS_AGENT_DEV_PYTHON 和 MERIVUS_AGENT_DEV_ROOT。\n" +
        "默认接口：GET /health、GET /merivus/info、POST /merivus/agent\n" +
        "请求字段：request_id、session_id、message、context、allowed_capabilities\n" +
        "响应字段：request_id、reply、proposal、provider、model、status。proposal 只显示为未执行建议。"

    anchors.fill: parent
    z: QGroundControl.zOrderTopMost + 20

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    AiServiceSupervisor {
        id: aiSupervisor
        enabled: assistantSettings.agentEnabled
        provider: assistantSettings.agentProvider
        ollamaBaseUrl: assistantSettings.ollamaBaseUrl
        ollamaModel: assistantSettings.ollamaModel
        ollamaTimeoutSeconds: assistantSettings.ollamaTimeoutSeconds
        allowMockFallback: assistantSettings.allowMockFallback

        onAgentHealthy: {
            aiAgentClient.checkHealth()
            aiAgentClient.loadInfo()
        }

        onHealthReadyChanged: {
            if (healthReady) {
                aiAgentClient.checkHealth()
                aiAgentClient.loadInfo()
            }
        }

        onLocalTokenChanged: {
            if (token && token.length > 0) {
                aiAgentClient.setLocalToken(token)
            } else {
                aiAgentClient.clearLocalToken()
            }
        }

        onStartFailed: {
            root.appendMessage("assistant", tr("Agent启动失败：%1").arg(message))
        }

        onAgentCrashed: {
            root.appendMessage("assistant", tr("Agent已崩溃，QGC主界面和飞控功能不会被阻塞。"))
        }
    }

    AiAgentClient {
        id: aiAgentClient
        agentEnabled: assistantSettings.agentEnabled
        endpoint: assistantSettings.agentEndpoint

        onResponseReceived: {
            if (reply.length > 0) {
                root.appendMessage("assistant", reply)
            }
            root.handleAgentProposal(proposal, requestId)
        }

        onRequestFailed: {
            root.appendMessage("assistant", tr("Agent请求失败：%1").arg(message))
        }
    }

    Settings {
        id: assistantSettings
        category: "MerivusAIAssistant"

        property real panelWidth: 400
        property real panelHeight: 760
        property real panelTopMargin: 112
        property int layoutVersion: 0
        property bool agentEnabled: false
        property bool developerEnableAiFlightExecution: false
        property string agentEndpoint: "http://127.0.0.1:8765"
        property string agentProvider: "mock"
        property string ollamaBaseUrl: "http://127.0.0.1:11434"
        property string ollamaModel: "qwen3:8b"
        property int ollamaTimeoutSeconds: 60
        property bool allowMockFallback: false
        property int maxMessages: 80
    }

    Component.onCompleted: {
        Qt.callLater(resetPanelLayoutIfNeeded)
    }

    onExpandedChanged: {
        if (expanded && assistantSettings.agentEnabled) {
            aiSupervisor.ensureRunning()
        }
    }

    onAvailablePanelHeightChanged: {
        assistantSettings.panelHeight = clamp(assistantSettings.panelHeight, minPanelHeight, maxPanelHeight)
        assistantSettings.panelTopMargin = clamp(assistantSettings.panelTopMargin, headerOffset,
                                                 parent ? Math.max(headerOffset, parent.height - panelHeight - 8) : headerOffset)
    }

    function tr(text) { return qsTr(text) }

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function currentProviderIndex() {
        return assistantSettings.agentProvider === "ollama" ? 1 : 0
    }

    function applyAgentProviderSettings() {
        aiSupervisor.provider = assistantSettings.agentProvider
        aiSupervisor.ollamaBaseUrl = assistantSettings.ollamaBaseUrl
        aiSupervisor.ollamaModel = assistantSettings.ollamaModel
        aiSupervisor.ollamaTimeoutSeconds = assistantSettings.ollamaTimeoutSeconds
        aiSupervisor.allowMockFallback = assistantSettings.allowMockFallback

        if (!assistantSettings.agentEnabled) {
            return
        }

        if (aiSupervisor.healthReady && !aiSupervisor.ownsProcess) {
            appendMessage("assistant", tr("当前Agent不是由MERIVUS启动，无法自动切换Provider。请手动停止外部Agent后重试，或确认外部Agent已使用目标Provider启动。"))
            aiAgentClient.loadInfo()
            return
        }

        if (aiSupervisor.healthReady || aiSupervisor.processRunning) {
            aiSupervisor.restartAgent()
        } else {
            aiSupervisor.ensureRunning()
        }
    }

    function resetPanelLayoutIfNeeded() {
        if (!parent || parent.height <= 0) return
        if (assistantSettings.layoutVersion < 7) {
            assistantSettings.panelWidth = defaultPanelWidth
            assistantSettings.panelHeight = defaultPanelHeight
            assistantSettings.panelTopMargin = defaultPanelTopMargin
            assistantSettings.layoutVersion = 7
        }
    }

    function appendMessage(role, text) {
        var lifetime = _fleetReviewLifetime
        chatModel.append({ role: role, text: text })
        if (!lifetime.alive) return
        trimHistory()
        Qt.callLater(function() { if (lifetime.alive) chatList.positionViewAtEnd() })
    }

    function trimHistory() {
        var limit = Math.max(12, assistantSettings.maxMessages)
        while (chatModel.count > limit) {
            chatModel.remove(1)
        }
    }

    function clearChatHistory() {
        chatModel.clear()
        chatModel.append({
            role: "assistant",
            text: tr("你好，我可以协助查看飞行器状态、解释参数，并把起飞/降落/返航/暂停转换为待确认命令。")
        })
        pendingIntent = null
        pendingSummary = ""
        pendingFleetProposal = null
        pendingFleetReview = null
        pendingFleetReviewSummary = ""
        Qt.callLater(function() { chatList.positionViewAtEnd() })
    }

    function vehicleById(vehicleId) {
        if (!vehicles) return null
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i)
            if (vehicle && vehicle.id === vehicleId) return vehicle
        }
        return null
    }

    function allVehicleIds() {
        var ids = []
        if (!vehicles) return ids
        for (var i = 0; i < vehicles.count; i++) {
            var vehicle = vehicles.get(i)
            if (vehicle) ids.push(vehicle.id)
        }
        return ids
    }

    function defaultVehicleIds() {
        return activeVehicle ? [ activeVehicle.id ] : []
    }

    function normalizeIds(ids) {
        var result = []
        if (!ids) return result
        for (var i = 0; i < ids.length; i++) {
            var id = parseInt(ids[i])
            if (!isNaN(id) && result.indexOf(id) === -1) result.push(id)
        }
        return result
    }

    function parseVehicleIds(text) {
        if (/全部|所有|全体|all/i.test(text)) return allVehicleIds()

        var ids = []
        var groupMatch = /([0-9,\s，、]+)\s*号/.exec(text)
        if (groupMatch) {
            var parts = groupMatch[1].split(/[,\s，、]+/)
            for (var p = 0; p < parts.length; p++) {
                var id = parseInt(parts[p])
                if (!isNaN(id)) ids.push(id)
            }
        }

        var match
        var numbered = /(?:UAV[-_\s]*|无人机\s*|飞机\s*)?([0-9]+)\s*号/ig
        while ((match = numbered.exec(text)) !== null) {
            var numberedId = parseInt(match[1])
            if (!isNaN(numberedId)) ids.push(numberedId)
        }

        var uavPattern = /UAV[-_\s]*([0-9]+)/ig
        while ((match = uavPattern.exec(text)) !== null) {
            var uavId = parseInt(match[1])
            if (!isNaN(uavId)) ids.push(uavId)
        }

        return normalizeIds(ids.length > 0 ? ids : defaultVehicleIds())
    }

    function parseAltitude(text) {
        var match = /([0-9]+(?:\.[0-9]+)?)\s*(?:米|m|meter|meters)/i.exec(text)
        if (!match) return 10
        var altitude = parseFloat(match[1])
        if (isNaN(altitude)) return 10
        return clamp(altitude, 1, 120)
    }

    function vehicleSummary(vehicle) {
        if (!vehicle) return ""
        var battery = vehicle.batteries && vehicle.batteries.count > 0 ? Number(vehicle.batteries.get(0).percentRemaining.rawValue).toFixed(0) + "%" : "--"
        var altitude = vehicle.altitudeRelative ? Number(vehicle.altitudeRelative.rawValue).toFixed(1) + " m" : "--"
        var mode = vehicle.flightMode ? vehicle.flightMode : "--"
        return tr("UAV-%1：%2，模式 %3，高度 %4，电量 %5")
                .arg(vehicle.id)
                .arg(vehicle.armed ? tr("已解锁") : tr("未解锁"))
                .arg(mode)
                .arg(altitude)
                .arg(battery)
    }

    function describeFleet() {
        if (!vehicles || vehicles.count === 0) return tr("当前没有连接的飞行器。")
        var lines = []
        for (var i = 0; i < vehicles.count; i++) {
            lines.push(vehicleSummary(vehicles.get(i)))
        }
        return lines.join("\n")
    }

    function buildIntent(action, ids, altitude) {
        ids = normalizeIds(ids)
        if (!ids || ids.length === 0) {
            appendMessage("assistant", tr("没有可用飞行器。请先连接飞控，或明确指定 UAV 编号。"))
            return null
        }

        var missing = []
        for (var i = 0; i < ids.length; i++) {
            if (!vehicleById(ids[i])) missing.push(ids[i])
        }
        if (missing.length > 0) {
            appendMessage("assistant", tr("没有找到 UAV-%1。").arg(missing.join(", ")))
            return null
        }

        var title = action === "takeoff" ? tr("起飞")
                  : action === "land" ? tr("降落")
                  : action === "rtl" ? tr("返航")
                  : tr("暂停")
        return { action: action, title: title, vehicleIds: ids, altitude: altitude }
    }

    function prepareIntent(intent) {
        if (!intent) return
        var targetText = intent.vehicleIds.length === 1 ? tr("UAV-%1").arg(intent.vehicleIds[0])
                                                        : tr("%1 架飞行器（%2）").arg(intent.vehicleIds.length).arg(intent.vehicleIds.join(", "))
        pendingIntent = intent
        pendingSummary = intent.action === "takeoff"
                       ? tr("准备让 %1 起飞到 %2 米。").arg(targetText).arg(Number(intent.altitude).toFixed(1))
                       : tr("准备让 %1 执行%2。").arg(targetText).arg(intent.title)
        appendMessage("assistant", pendingSummary + "\n" + tr("建议未执行：当前安全封锁默认不会下发飞行命令。"))
    }

    function _proposalArguments(proposal) {
        if (!proposal || !proposal.arguments || typeof proposal.arguments !== "object") return {}
        return proposal.arguments
    }

    function _proposalVehicleIds(proposal) {
        var args = _proposalArguments(proposal)
        var ids = []
        var candidates = args.vehicle_id !== undefined && args.vehicle_id !== null
                       ? [ args.vehicle_id ] : []
        // The current schema rejects arrays, but accepting a scalar or a
        // comma-separated string here keeps the review adapter forward
        // compatible without trusting arbitrary collection input.
        if (args.selectedVehicleId !== undefined && args.selectedVehicleId !== null) {
            candidates.push(args.selectedVehicleId)
        }
        if (args.vehicle_ids !== undefined && args.vehicle_ids !== null) {
            candidates.push(args.vehicle_ids)
        }
        if (args.selectedVehicleIds !== undefined && args.selectedVehicleIds !== null) {
            candidates.push(args.selectedVehicleIds)
        }
        for (var candidateIndex = 0; candidateIndex < candidates.length; candidateIndex++) {
            var candidate = candidates[candidateIndex]
            var candidateParts = String(candidate).split(/[,，\s]+/)
            for (var partIndex = 0; partIndex < candidateParts.length; partIndex++) {
                var id = Number(candidateParts[partIndex])
                if (isFinite(id) && Math.floor(id) === id && id >= 1 && id <= 255) ids.push(id)
            }
        }
        return normalizeIds(ids)
    }

    function proposalCanCreateFleetReview(proposal) {
        if (!proposal || typeof proposal !== "object") return false
        var command = proposal.command ? String(proposal.command) : ""
        if (command.length === 0) return false
        var validation = String(proposal.validationStatus || "").toLowerCase()
        if (validation !== "valid") return false
        var policyDecision = String(proposal.policyDecision || "").toLowerCase()
        if (policyDecision !== "previewonly" && policyDecision !== "requiresconfirmation") return false
        var reviewableCommands = [
            "mission.create_draft", "mission.analyze", "mission.upload", "mission.start",
            "vehicle.arm", "vehicle.force_arm", "vehicle.takeoff", "vehicle.land",
            "vehicle.rtl", "vehicle.pause", "vehicle.goto"
        ]
        return reviewableCommands.indexOf(command) >= 0
    }

    function retireFleetReviewDisplay() {
        var lifetime = _fleetReviewLifetime
        if (!lifetime || !lifetime.alive) return
        var previous = _fleetReviewTicket
        if (previous) previous.retired = true
        var version = ({})
        _fleetProposalInputVersion = version
        if (!lifetime.alive) return
        if (_fleetProposalInputVersion === version && _fleetReviewTicket === previous) _fleetReviewTicket = null
        if (!lifetime.alive) return
        // Retire the view first. Cancelling the exact old native request may
        // synchronously deliver its completion; it cannot revive this view.
        cancelRetiredFleetReview(previous, previous ? previous.requestId : "")
        if (!lifetime.alive || _fleetProposalInputVersion !== version || _fleetReviewTicket !== null) return
        pendingFleetReview = null
        if (!lifetime.alive || _fleetProposalInputVersion !== version || _fleetReviewTicket !== null) return
        pendingFleetReviewSummary = ""
    }

    function cancelRetiredFleetReview(ticket, requestId) {
        if (!ticket || !ticket.lifetime.alive || typeof requestId !== "string" || !requestId.length) return
        var originalController = ticket.controller
        // A controller may have been destroyed while this panel stayed alive.
        // Never substitute the current controller or its newer request ID.
        try {
            if (!originalController || typeof originalController.cancelMissionPlanning !== "function") return
            var status = originalController.missionPlanningStatus
            if (!ticket.lifetime.alive || !status || status.requestId !== requestId) return
            originalController.cancelMissionPlanning(requestId)
        } catch (e) { /* No live original controller to cancel. */ }
    }

    function fleetProposalFingerprint(proposal) {
        try { return JSON.stringify(proposal) || "" } catch (e) { return "" }
    }

    function fleetReviewContextCurrent(ticket) {
        return ticket && !ticket.retired && ticket.lifetime.alive && _fleetReviewTicket === ticket
            && ticket.controller === guidedController && ticket.proposal === pendingFleetProposal
            && ticket.inputVersion === _fleetProposalInputVersion
            && ticket.proposalFingerprint === fleetProposalFingerprint(pendingFleetProposal)
    }

    function fleetPlanningStatusText() {
        var state = fleetPlanningStatus
        var error = typeof state.error === "string" ? state.error : ""
        var expired = /expired|superseded|replaced|invalidated|inputs_changed/.test(error)
        var text = ""
        if (state.status === "cancelling") text = tr("共享后台规划正在取消，等待计算线程退出；输入保留，暂不能重复生成。")
        else if (state.active === true) text = tr("共享后台规划正在计算，尚无最终审阅结果。可以取消；不会自动批准、上传或执行。")
        else if (expired) text = tr("共享后台规划已过期：来源或输入已变更，请重新生成审阅。")
        else if (state.status === "cancelled" || error === "planning_operator_cancelled") text = tr("共享后台规划已取消，输入保留。")
        else if (state.status === "failed" || error.length > 0) text = tr("共享后台规划未形成当前可用的审阅绑定，请检查原因后重试。")
        else if (state.ready === true || state.status === "completing") text = tr("后台计算已结束，正在核对来源和整理审阅；尚未批准。")
        return text + (text && error.length ? tr("\n原因：%1").arg(error) : "")
    }

    function applyFleetReviewResult(ticket, result) {
        if (!fleetReviewContextCurrent(ticket) || ticket.finished || !result || result.planningPending === true) return false
        // Mark consumed before any QML notification; a synchronous completion
        // and the direct return cannot publish the same result twice.
        ticket.finished = true
        var archive = result.planArchive || {}
        var error = typeof result.planningError === "string" && result.planningError.length
            ? result.planningError : (typeof archive.error === "string" ? archive.error : "")
        var summary = ""
        if (archive.ok === true && result.planArchiveRecorded !== true) {
            summary = tr("审阅记录已经提交，但当前来源绑定已失效；历史记录未回滚，不能沿用旧批准。请重新审阅。")
        } else if (result.planArchiveRecorded === true && result.missionReview) {
            var reviewStatus = String(result.missionReview.reviewStatus || "needs-review")
            var riskLevel = result.risk ? String(result.risk.level || "unknown") : "unknown"
            summary = tr("Fleet OS 审阅草稿：%1；风险 %2；%3。仅预览，未批准、未上传、未下发。")
                .arg(reviewStatus).arg(riskLevel)
                .arg(result.missionReview.needsReview === true ? tr("需要人工复核") : tr("仍需人工确认"))
        } else if (error === "planning_operator_cancelled" || /cancelled/.test(error)) {
            summary = tr("本提案的后台规划已取消；没有形成当前可用的审阅绑定。原提案仍保留。")
        } else if (/expired|superseded|replaced|invalidated|inputs_changed/.test(error)) {
            summary = tr("本提案的后台结果已过期，不能用于当前审阅或审批。请核对提案后重新生成。")
        } else {
            summary = tr("本提案未形成当前可用的审阅绑定。请查看原因并重新生成；不代表批准或执行。")
        }
        if (error.length) summary += tr("\n原因：%1").arg(error)
        pendingFleetReview = result
        if (!ticket.lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        pendingFleetReviewSummary = summary
        if (!ticket.lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        appendMessage("assistant", summary)
        if (!ticket.lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        // Retire the property itself, not just a nested JS flag: QML button
        // bindings must observe completion even on the synchronous path.
        _fleetReviewTicket = null
        return true
    }

    function createFleetReviewForProposal() {
        if (fleetPlanningBusy || (_fleetReviewTicket && !_fleetReviewTicket.finished)) {
            appendMessage("assistant", tr("后台规划尚未结束，请等待或取消当前计算；提案和输入会保留。"))
            return false
        }
        var proposal = pendingFleetProposal
        if (!proposalCanCreateFleetReview(proposal)) {
            appendMessage("assistant", tr("当前建议不能生成 Fleet OS 审阅草稿；请保留为只读展示。"))
            return false
        }
        if (!guidedController || !guidedController.prepareFleetIntent) {
            appendMessage("assistant", tr("Fleet OS 审阅入口尚未就绪，建议仍保持只读展示。"))
            return false
        }

        var lifetime = _fleetReviewLifetime
        var originalController = guidedController
        var inputVersion = _fleetProposalInputVersion
        var proposalFingerprint = fleetProposalFingerprint(proposal)
        if (!proposalFingerprint.length) return false
        var args = _proposalArguments(proposal)
        var ids = _proposalVehicleIds(proposal)
        var reviewParameters = {
            source: "ai-assistant",
            proposalCommand: String(proposal.command),
            selectedVehicleIds: ids,
            vehicleIds: ids,
            reviewOptions: { source: "ai-assistant" }
        }
        if (args.altitude_m !== undefined) reviewParameters.altitude_m = Number(args.altitude_m)
        if (args.latitude !== undefined) reviewParameters.latitude = Number(args.latitude)
        if (args.longitude !== undefined) reviewParameters.longitude = Number(args.longitude)
        if (args.mission_duration_seconds !== undefined) {
            reviewParameters.missionDurationSeconds = Number(args.mission_duration_seconds)
        }
        if (typeof args.templateId === "string"
                && args.templateId.trim().length > 0
                && args.templateId.trim().length <= 64) {
            reviewParameters.templateId = args.templateId.trim()
        }
        if (originalController.fleetRegistry && originalController.fleetRegistry.vehicles) {
            reviewParameters.fleet = originalController.fleetRegistry.vehicles
        }

        if (!lifetime.alive || originalController !== guidedController || inputVersion !== _fleetProposalInputVersion
                || proposal !== pendingFleetProposal || proposalFingerprint !== fleetProposalFingerprint(proposal)) return false
        var ticket = {lifetime:lifetime, controller:originalController, proposal:proposal,
            inputVersion:inputVersion, proposalFingerprint:proposalFingerprint, requestId:"", finished:false, retired:false}
        _fleetReviewTicket = ticket
        if (!lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        pendingFleetReview = null
        if (!lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        pendingFleetReviewSummary = tr("正在准备本提案的审阅。计算中不是通过或拒绝，仍须等待最终结果。")
        if (!lifetime.alive || !fleetReviewContextCurrent(ticket)) return false
        // Completion may be synchronous. A changed proposal/input version,
        // replaced controller or destroyed panel can never consume it.
        var complete = function(result) {
            if (!lifetime.alive) return
            root.applyFleetReviewResult(ticket, result)
        }
        var result = originalController.prepareFleetIntent(String(proposal.command), reviewParameters, complete)
        if (!lifetime.alive) return false
        if (result && result.planningPending === true
                && (ticket.retired || (!ticket.finished && !fleetReviewContextCurrent(ticket)))) {
            // The begin notification can replace the proposal before its ID
            // was returned. Retire that exact late ID, not a newer job.
            ticket.retired = true
            cancelRetiredFleetReview(ticket, result.requestId)
            return false
        }
        if (ticket.finished) return true
        if (!fleetReviewContextCurrent(ticket)) return false
        if (result && result.planningPending === true) {
            ticket.requestId = typeof result.requestId === "string" ? result.requestId : ""
            pendingFleetReviewSummary = tr("本提案正在后台计算，尚无最终审阅结果。可以取消计算；原提案仍保留。")
            return true
        }
        return applyFleetReviewResult(ticket, result || {planArchive:{error:"planning_result_unavailable"}})
    }

    function createFleetReviewForPendingIntent() {
        if (fleetPlanningBusy || (_fleetReviewTicket && !_fleetReviewTicket.finished)) {
            appendMessage("assistant", tr("后台规划尚未结束，请等待或取消当前计算。"))
            return false
        }
        if (!pendingIntent || !pendingIntent.vehicleIds || pendingIntent.vehicleIds.length === 0) {
            appendMessage("assistant", tr("没有可用于审阅的待确认目标。"))
            return false
        }
        var command = "vehicle." + String(pendingIntent.action)
        var proposalArguments = {
            vehicle_id: pendingIntent.vehicleIds[0],
            selectedVehicleIds: pendingIntent.vehicleIds
        }
        if (pendingIntent.action === "takeoff") {
            proposalArguments.altitude_m = Number(pendingIntent.altitude)
        }
        // Treat the local rule-mode intent exactly like a proposal for the
        // review-only adapter. No execution state is inferred from it.
        pendingFleetProposal = {
            command: command,
            arguments: proposalArguments,
            summary: pendingSummary,
            validationStatus: "Valid",
            policyDecision: "PreviewOnly"
        }
        return createFleetReviewForProposal()
    }

    function executePendingIntent() {
        if (!pendingIntent) return
        var intent = pendingIntent
        pendingIntent = null
        pendingSummary = ""

        var targetText = intent.vehicleIds.length === 1 ? tr("UAV-%1").arg(intent.vehicleIds[0])
                                                        : tr("%1 架飞行器（%2）").arg(intent.vehicleIds.length).arg(intent.vehicleIds.join(", "))
        var actionText = intent.action === "takeoff"
                       ? tr("起飞到 %1 米").arg(Number(intent.altitude).toFixed(1))
                       : intent.title
        var flagText = assistantSettings.developerEnableAiFlightExecution
                     ? tr("开发开关 developerEnableAiFlightExecution 已启用，但安全封锁分支不包含真实执行路径。")
                     : tr("开发开关 developerEnableAiFlightExecution 默认关闭。")

        appendMessage("assistant",
                      tr("建议未执行：%1 可执行“%2”。\n%3\n请使用原生 QGC 人工控制入口，并按现场安全流程确认。")
                      .arg(targetText)
                      .arg(actionText)
                      .arg(flagText))
    }

    function cancelPendingIntent() {
        if (!pendingIntent) return
        appendMessage("assistant", tr("已取消：%1").arg(pendingSummary))
        pendingIntent = null
        pendingSummary = ""
    }

    function clearPendingFleetProposal() {
        pendingFleetProposal = null
        pendingFleetReview = null
        pendingFleetReviewSummary = ""
    }

    function routeLocalText(clean) {
        if (/清空|清除|删除.*历史|clear/i.test(clean)) {
            clearChatHistory()
            appendMessage("assistant", tr("已清空当前会话记录。"))
            return true
        }

        if (/智能体|大模型|agent|llm|部署/i.test(clean)) {
            appendMessage("assistant", _agentGuide)
            return true
        }

        if (/帮助|help|怎么用/i.test(clean)) {
            appendMessage("assistant", tr("我可以做基础问答和快捷指令：例如“1号起飞10米”“2号降落”“全部返航”“查看飞行器状态”。涉及飞行动作时，我只生成未执行建议。打开配置后可接入本机 Agent 服务。"))
            return true
        }

        if (/状态|在线|电量|高度|status|list/i.test(clean)) {
            appendMessage("assistant", describeFleet())
            return true
        }

        if (/参数|parameter|说明/i.test(clean)) {
            appendMessage("assistant", tr("参数页面已增加鼠标悬停说明卡片，会直接读取 PX4/APM 参数元数据中的短说明、长说明、单位、范围和默认值。"))
            return true
        }

        var ids = parseVehicleIds(clean)
        if (/起飞|take\s*off|takeoff/i.test(clean)) {
            prepareIntent(buildIntent("takeoff", ids, parseAltitude(clean)))
            return true
        } else if (/降落|着陆|land/i.test(clean)) {
            prepareIntent(buildIntent("land", ids, 0))
            return true
        } else if (/返航|rtl|return/i.test(clean)) {
            prepareIntent(buildIntent("rtl", ids, 0))
            return true
        } else if (/暂停|悬停|pause|hold/i.test(clean)) {
            prepareIntent(buildIntent("pause", ids, 0))
            return true
        }

        return false
    }

    function chatHistoryForAgent() {
        var history = []
        var start = Math.max(0, chatModel.count - 18)
        for (var i = start; i < chatModel.count; i++) {
            var item = chatModel.get(i)
            history.push({ role: item.role, content: item.text })
        }
        return history
    }

    function agentContext() {
        return {
            vehicle_count: vehicles ? vehicles.count : 0,
            active_vehicle_id: activeVehicle ? activeVehicle.id : null,
            connected: activeVehicle !== null,
            armed: activeVehicle ? activeVehicle.armed : false
        }
    }

    function allowedAgentCapabilities() {
        return [
            "vehicle.query_status",
            "vehicle.query_battery",
            "vehicle.query_position",
            "vehicle.query_rtk",
            "log.explain_error",
            "mission.analyze",
            "mission.create_draft",
            "vehicle.takeoff",
            "vehicle.land",
            "vehicle.rtl",
            "vehicle.pause"
        ]
    }

    function localizedValidationStatus(value) {
        var map = {
            "Valid": tr("通过"),
            "InvalidSchema": tr("结构无效"),
            "MissingCommand": tr("缺少命令"),
            "InvalidArguments": tr("参数无效"),
            "UnknownCommand": tr("未知命令")
        }
        return map[value] || value
    }

    function localizedPolicyDecision(value) {
        var map = {
            "AllowReadOnly": tr("允许只读"),
            "AllowUiOnly": tr("允许界面建议"),
            "PreviewOnly": tr("仅预览"),
            "RequiresConfirmation": tr("需要确认"),
            "Deny": tr("拒绝")
        }
        return map[value] || value
    }

    function localizedRiskLevel(value) {
        var map = {
            "Informational": tr("信息"),
            "Low": tr("低"),
            "Medium": tr("中"),
            "High": tr("高"),
            "Critical": tr("严重")
        }
        return map[value] || value
    }

    function localizedPolicyReason(value) {
        var map = {
            "No structured proposal to evaluate.": tr("没有需要评估的结构化建议。"),
            "Schema validation failed.": tr("结构校验失败。"),
            "Read-only proposal allowed for display only.": tr("只读建议仅允许展示，不会执行飞行动作。"),
            "UI-only proposal is allowed for display only; no UI action is executed in this phase.": tr("界面建议仅允许展示，当前阶段不会自动操作界面。"),
            "Mission proposal may be previewed only; no upload or execution is available.": tr("任务建议仅允许预览，当前阶段不会上传或执行任务。"),
            "Command is denied by local policy.": tr("本地策略拒绝该命令。"),
            "Flight-affecting command is preview-only; MERIVUS does not execute AI flight actions.": tr("当前版本禁止 AI 执行飞行动作，该建议仅用于预览。"),
            "Unknown command denied by local policy.": tr("未知命令已被本地策略拒绝。"),
            "ui.open_page requires a short page argument.": tr("打开页面建议需要有效的页面参数。"),
            "vehicle_id must be a positive integer.": tr("vehicle_id 必须是正整数。"),
            "vehicle_id is required and must be a positive integer.": tr("必须提供正整数 vehicle_id。"),
            "latitude and longitude must be valid WGS84 coordinates.": tr("latitude 和 longitude 必须是有效 WGS84 坐标。"),
            "altitude_m must be between 0 and 120.": tr("altitude_m 必须在 0 到 120 米之间。"),
            "takeoff altitude_m must be > 0 and <= 120.": tr("起飞高度 altitude_m 必须大于 0 且不超过 120 米。"),
            "param.write requires name and value for preview, but remains denied.": tr("参数写入建议需要 name 和 value，但本地策略仍会拒绝。"),
            "raw MAVLink arguments are not accepted.": tr("不接受原始 MAVLink 参数。")
        }
        return map[value] || value
    }

    function handleAgentProposal(proposal, requestId) {
        if (!proposal || typeof proposal !== "object") return false

        var command = proposal.command ? String(proposal.command) : tr("未知建议")
        var summary = proposal.summary ? String(proposal.summary) : tr("Agent返回了结构化建议，但未提供摘要。")
        var validation = proposal.validationStatus ? localizedValidationStatus(String(proposal.validationStatus)) : tr("结构无效")
        var decision = proposal.policyDecision ? localizedPolicyDecision(String(proposal.policyDecision)) : tr("拒绝")
        var risk = proposal.localRisk ? localizedRiskLevel(String(proposal.localRisk)) : tr("严重")
        var requiresConfirmation = proposal.requiresConfirmation === true ? tr("是") : tr("否")
        var executable = proposal.executable === true ? tr("是") : tr("否")
        var reason = proposal.reason ? localizedPolicyReason(String(proposal.reason)) : tr("本地策略未提供原因。")
        var argumentSummary = proposal.argumentsSummary ? String(proposal.argumentsSummary) : "{}"
        var source = proposal.source ? String(proposal.source) : "agent"
        var provider = proposal.agentProvider ? String(proposal.agentProvider) : aiAgentClient.provider
        var model = proposal.agentModel ? String(proposal.agentModel) : aiAgentClient.model

        var detail = tr("建议状态：未执行")
        detail += "\n" + tr("建议摘要：%1").arg(summary)
        detail += "\n" + tr("建议命令：%1").arg(command)
        detail += "\n" + tr("参数摘要：%1").arg(argumentSummary)
        detail += "\n" + tr("来源：%1 %2/%3").arg(source).arg(provider).arg(model)
        detail += "\n" + tr("结构校验：%1").arg(validation)
        detail += "\n" + tr("本地风险：%1").arg(risk)
        detail += "\n" + tr("本地策略：%1").arg(decision)
        detail += "\n" + tr("是否需要确认：%1").arg(requiresConfirmation)
        detail += "\n" + tr("是否可执行：%1").arg(executable)
        detail += "\n" + tr("原因：%1").arg(reason)
        detail += "\n" + tr("请求编号：%1").arg(requestId)
        detail += "\n" + tr("该proposal仅用于显示，本阶段不会转换为飞行动作。")

        appendMessage("assistant", detail)
        pendingFleetProposal = proposalCanCreateFleetReview(proposal) ? proposal : null
        pendingFleetReview = null
        pendingFleetReviewSummary = ""
        return true
    }

    function callAgent(clean) {
        if (agentRequestRunning) {
            appendMessage("assistant", tr("已有Agent请求正在处理中，请等待当前请求完成。"))
            return
        }

        if (!aiSupervisor.healthReady) {
            appendMessage("assistant", tr("%1：%2").arg(aiSupervisor.stateText).arg(aiSupervisor.lastError.length > 0 ? aiSupervisor.lastError : tr("本机Agent尚未就绪。")))
            aiSupervisor.ensureRunning()
            return
        }

        if (!aiAgentClient.providerReady) {
            var providerMessage = aiAgentClient.providerError.length > 0 ? aiAgentClient.providerError : tr("当前Provider尚未就绪。请检查本机模型服务或切回Mock。")
            appendMessage("assistant", tr("Provider未就绪：%1").arg(providerMessage))
            aiAgentClient.loadInfo()
            return
        }

        appendMessage("assistant", tr("正在连接本机 Agent：%1").arg(aiAgentClient.endpoint))
        aiAgentClient.sendMessage(clean, agentContext(), allowedAgentCapabilities())
    }

    function handleUserText(text) {
        var clean = text.trim()
        if (clean.length === 0) return
        appendMessage("user", clean)

        if (assistantSettings.agentEnabled) {
            if (/清空|清除|删除.*历史|clear/i.test(clean)) {
                clearChatHistory()
                appendMessage("assistant", tr("已清空当前会话记录。"))
                return
            }
            callAgent(clean)
        } else {
            if (routeLocalText(clean)) return
            appendMessage("assistant", tr("当前使用本地规则模式：支持状态查询、参数说明提示，以及起飞/降落/返航/暂停建议。复杂坐标和航线操作建议继续使用地图交互；需要大模型能力时可在右上角配置中启用外部 Agent。"))
        }
    }

    Rectangle {
        id: assistantPanel
        anchors.top: parent.top
        anchors.topMargin: root.panelTopMargin
        anchors.right: parent.right
        anchors.rightMargin: 8
        width: root.panelWidth
        height: root.panelHeight
        radius: 8
        color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, _panelHovered ? 0.98 : 0.94)
        border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.24)
        border.width: 1
        visible: root.expanded
        clip: true

        property string activeResizeEdge: ""
        property bool resizePressed: resizeLeftHandle.pressed || resizeTopHandle.pressed || resizeBottomHandle.pressed
        property bool resizeLeftActive: activeResizeEdge === "left" || (!resizePressed && resizeLeftHandle.containsMouse)
        property bool resizeTopActive: activeResizeEdge === "top" || (!resizePressed && resizeTopHandle.containsMouse)
        property bool resizeBottomActive: activeResizeEdge === "bottom" || (!resizePressed && resizeBottomHandle.containsMouse)
        property bool inputHovered: false

        Behavior on width { NumberAnimation { duration: resizeLeftHandle.pressed ? 0 : 120 } }
        Behavior on height { NumberAnimation { duration: resizeTopHandle.pressed || resizeBottomHandle.pressed ? 0 : 120 } }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.10) }
                GradientStop { position: 0.22; color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.96) }
                GradientStop { position: 1.0; color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.92) }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 2
            color: qgcPal.buttonHighlight
            opacity: root._panelHovered ? 0.95 : 0.55

            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            onEntered: root._panelHovered = true
            onExited: root._panelHovered = false
        }

        Rectangle {
            id: resizeRail
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: assistantPanel.resizeLeftActive ? 3 : 0
            color: qgcPal.buttonHighlight
            opacity: assistantPanel.resizeLeftActive ? 1 : 0
            z: 4

            Behavior on width { NumberAnimation { duration: 100 } }
            Behavior on opacity { NumberAnimation { duration: 100 } }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: assistantPanel.resizeTopActive ? 3 : 0
            color: qgcPal.buttonHighlight
            opacity: assistantPanel.resizeTopActive ? 1 : 0
            z: 4

            Behavior on height { NumberAnimation { duration: 100 } }
            Behavior on opacity { NumberAnimation { duration: 100 } }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: assistantPanel.resizeBottomActive ? 3 : 0
            color: qgcPal.buttonHighlight
            opacity: assistantPanel.resizeBottomActive ? 1 : 0
            z: 4

            Behavior on height { NumberAnimation { duration: 100 } }
            Behavior on opacity { NumberAnimation { duration: 100 } }
        }

        MouseArea {
            id: resizeLeftHandle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 12
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeHorCursor

            property real startMouseX: 0
            property real startWidth: 0

            onPressed: {
                mouse.accepted = true
                assistantPanel.activeResizeEdge = "left"
                startMouseX = mapToItem(root, mouse.x, mouse.y).x
                startWidth = root.panelWidth
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseX = mapToItem(root, mouse.x, mouse.y).x
                    assistantSettings.panelWidth = root.clamp(startWidth - (currentMouseX - startMouseX), root.minPanelWidth, root.maxPanelWidth)
                }
            }
            onReleased: assistantPanel.activeResizeEdge = ""
            onCanceled: assistantPanel.activeResizeEdge = ""
        }

        MouseArea {
            id: resizeTopHandle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 10
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            z: 5

            property real startMouseY: 0
            property real startHeight: 0
            property real startTop: 0

            onPressed: {
                mouse.accepted = true
                assistantPanel.activeResizeEdge = "top"
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
                startHeight = root.panelHeight
                startTop = root.panelTopMargin
            }
            onPositionChanged: {
                if (!pressed) return
                var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                var delta = currentMouseY - startMouseY
                var bottom = parent ? Math.min(root.height - 8, startTop + startHeight) : startTop + startHeight
                var newTop = root.clamp(startTop + delta, root.headerOffset,
                                        Math.max(root.headerOffset, bottom - root.minPanelHeight))
                var maxHeight = Math.min(root.maxPanelHeight, Math.max(root.minPanelHeight, root.height - newTop - 8))
                var newHeight = root.clamp(bottom - newTop, root.minPanelHeight, maxHeight)
                assistantSettings.panelTopMargin = newTop
                assistantSettings.panelHeight = newHeight
            }
            onReleased: assistantPanel.activeResizeEdge = ""
            onCanceled: assistantPanel.activeResizeEdge = ""
        }

        MouseArea {
            id: resizeBottomHandle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 10
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            propagateComposedEvents: false
            cursorShape: Qt.SizeVerCursor
            z: 5

            property real startMouseY: 0
            property real startHeight: 0

            onPressed: {
                mouse.accepted = true
                assistantPanel.activeResizeEdge = "bottom"
                startMouseY = mapToItem(root, mouse.x, mouse.y).y
                startHeight = root.panelHeight
            }
            onPositionChanged: {
                if (pressed) {
                    var currentMouseY = mapToItem(root, mouse.x, mouse.y).y
                    var delta = currentMouseY - startMouseY
                    var maxHeight = Math.min(root.maxPanelHeight, Math.max(root.minPanelHeight, root.height - root.panelTopMargin - 8))
                    assistantSettings.panelHeight = root.clamp(startHeight + delta, root.minPanelHeight, maxHeight)
                }
            }
            onReleased: assistantPanel.activeResizeEdge = ""
            onCanceled: assistantPanel.activeResizeEdge = ""
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 10
            anchors.topMargin: 14
            anchors.bottomMargin: 14
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 17
                    color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.20)
                    border.color: qgcPal.buttonHighlight
                    border.width: 1

                    QGCColoredImage {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/qml/QGroundControl/FlightDisplay/ai-nine-star.svg"
                        color: qgcPal.buttonHighlight
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    QGCLabel {
                        Layout.fillWidth: true
                        text: tr("Merivus AI")
                        font.bold: true
                        font.pixelSize: 16
                        color: qgcPal.text
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        text: vehicles ? tr("%1 架飞行器在线 · %2")
                                         .arg(vehicles.count)
                                         .arg(assistantSettings.agentEnabled ? aiSupervisor.stateText : tr("本地规则"))
                                       : tr("未连接飞行器 · %1").arg(assistantSettings.agentEnabled ? aiSupervisor.stateText : tr("本地规则"))
                        font.pixelSize: 11
                        color: qgcPal.colorGrey
                    }
                }

                QGCButton {
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 28
                    text: root.settingsOpen ? tr("聊天") : tr("配置")
                    onClicked: root.settingsOpen = !root.settingsOpen
                }

                QGCButton {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 28
                    text: "×"
                    onClicked: root.expanded = false
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root.settingsOpen ? Math.min(540, Math.max(390, root.panelHeight - 174)) : 0
                visible: root.settingsOpen
                radius: 7
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.92)
                border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)
                clip: true

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 12
                    clip: true
                    contentWidth: width
                    contentHeight: settingsColumn.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar { }

                    ColumnLayout {
                        id: settingsColumn
                        width: parent.width
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            QGCCheckBox {
                                id: agentSwitch
                                Layout.fillWidth: true
                                text: tr("启用本机智能体")
                                checked: assistantSettings.agentEnabled
                                onClicked: {
                                    assistantSettings.agentEnabled = checked
                                    if (checked) {
                                        aiSupervisor.ensureRunning()
                                    } else {
                                        aiSupervisor.stopAgent()
                                        aiAgentClient.clearLocalToken()
                                    }
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 24
                                radius: 4
                                color: aiSupervisor.healthReady ? Qt.rgba(qgcPal.colorGreen.r, qgcPal.colorGreen.g, qgcPal.colorGreen.b, 0.18)
                                      : agentRequestRunning || aiSupervisor.state === AiServiceSupervisor.Checking || aiSupervisor.state === AiServiceSupervisor.Starting
                                        ? Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.18)
                                        : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.08)
                                border.color: aiSupervisor.healthReady ? qgcPal.colorGreen
                                             : agentRequestRunning || aiSupervisor.state === AiServiceSupervisor.Checking || aiSupervisor.state === AiServiceSupervisor.Starting
                                               ? qgcPal.colorOrange
                                               : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.18)

                                QGCLabel {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    text: agentRequestRunning ? tr("请求中") : aiSupervisor.stateText
                                    color: aiSupervisor.healthReady ? qgcPal.colorGreen
                                         : agentRequestRunning || aiSupervisor.state === AiServiceSupervisor.Checking || aiSupervisor.state === AiServiceSupervisor.Starting
                                           ? qgcPal.colorOrange
                                           : qgcPal.colorGrey
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: tr("地址：%1").arg(aiAgentClient.endpoint)
                            color: qgcPal.text
                            font.pixelSize: 12
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: aiSupervisor.lastError.length > 0 ? tr("状态：%1").arg(aiSupervisor.lastError)
                                                                    : tr("状态：%1").arg(aiSupervisor.stateText)
                            color: aiSupervisor.state === AiServiceSupervisor.PortConflict ||
                                   aiSupervisor.state === AiServiceSupervisor.Error ||
                                   aiSupervisor.state === AiServiceSupervisor.NotInstalled ||
                                   aiSupervisor.state === AiServiceSupervisor.Crashed ? qgcPal.warningText : qgcPal.text
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: aiSupervisor.workingDirectory.length > 0 ? tr("运行目录：%1").arg(aiSupervisor.workingDirectory) : tr("运行目录：待解析")
                            color: qgcPal.colorGrey
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.10)
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            QGCLabel {
                                Layout.preferredWidth: root._settingsLabelWidth
                                text: tr("Provider")
                                color: qgcPal.text
                                font.pixelSize: 12
                            }

                            QGCComboBox {
                                Layout.preferredWidth: 126
                                Layout.preferredHeight: root._settingsFieldHeight
                                model: [ tr("Mock"), tr("Ollama") ]
                                currentIndex: root.currentProviderIndex()
                                onActivated: {
                                    var nextProvider = currentIndex === 1 ? "ollama" : "mock"
                                    if (assistantSettings.agentProvider !== nextProvider) {
                                        assistantSettings.agentProvider = nextProvider
                                        root.applyAgentProviderSettings()
                                    }
                                }
                            }

                            QGCLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                                text: aiSupervisor.ownsProcess ? tr("MERIVUS托管") : tr("外部Agent")
                                color: aiSupervisor.ownsProcess ? qgcPal.colorGreen : qgcPal.colorOrange
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 8
                            rowSpacing: 7

                            QGCLabel {
                                Layout.preferredWidth: root._settingsLabelWidth
                                text: tr("模型")
                                color: qgcPal.text
                                font.pixelSize: 12
                            }
                            QGCTextField {
                                id: ollamaModelField
                                Layout.fillWidth: true
                                Layout.preferredHeight: root._settingsFieldHeight
                                text: assistantSettings.ollamaModel
                                enabled: assistantSettings.agentProvider === "ollama"
                                font.pixelSize: 12
                                onEditingFinished: {
                                    var value = text.trim()
                                    assistantSettings.ollamaModel = value.length > 0 ? value : "qwen3:8b"
                                    text = assistantSettings.ollamaModel
                                    root.applyAgentProviderSettings()
                                }
                            }

                            QGCLabel {
                                Layout.preferredWidth: root._settingsLabelWidth
                                text: tr("Ollama")
                                color: qgcPal.text
                                font.pixelSize: 12
                            }
                            QGCTextField {
                                id: ollamaBaseUrlField
                                Layout.fillWidth: true
                                Layout.preferredHeight: root._settingsFieldHeight
                                text: assistantSettings.ollamaBaseUrl
                                enabled: assistantSettings.agentProvider === "ollama"
                                font.pixelSize: 12
                                onEditingFinished: {
                                    assistantSettings.ollamaBaseUrl = text.trim().length > 0 ? text.trim() : "http://127.0.0.1:11434"
                                    text = assistantSettings.ollamaBaseUrl
                                    root.applyAgentProviderSettings()
                                }
                            }

                            QGCLabel {
                                Layout.preferredWidth: root._settingsLabelWidth
                                text: tr("超时")
                                color: qgcPal.text
                                font.pixelSize: 12
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                QGCTextField {
                                    Layout.preferredWidth: 66
                                    Layout.preferredHeight: root._settingsFieldHeight
                                    text: String(assistantSettings.ollamaTimeoutSeconds)
                                    enabled: assistantSettings.agentProvider === "ollama"
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: 12
                                    validator: IntValidator { bottom: 1; top: 300 }
                                    onEditingFinished: {
                                        var seconds = parseInt(text)
                                        if (isNaN(seconds)) seconds = 60
                                        assistantSettings.ollamaTimeoutSeconds = root.clamp(seconds, 1, 300)
                                        text = String(assistantSettings.ollamaTimeoutSeconds)
                                        root.applyAgentProviderSettings()
                                    }
                                }

                                QGCCheckBox {
                                    Layout.fillWidth: true
                                    text: tr("允许Mock回退")
                                    checked: assistantSettings.allowMockFallback
                                    onClicked: {
                                        assistantSettings.allowMockFallback = checked
                                        root.applyAgentProviderSettings()
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: tr("当前：%1 / %2").arg(aiAgentClient.provider).arg(aiAgentClient.model)
                            color: qgcPal.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            text: tr("Provider Ready：%1").arg(aiAgentClient.providerReady ? tr("是") : tr("否"))
                            color: aiAgentClient.providerReady ? qgcPal.colorGreen : qgcPal.colorOrange
                            font.pixelSize: 12
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            visible: aiAgentClient.providerError.length > 0
                            text: tr("Provider Error：%1").arg(aiAgentClient.providerError)
                            color: qgcPal.warningText
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }

                        QGCLabel {
                            Layout.fillWidth: true
                            visible: aiAgentClient.availableModelsText.length > 0
                            text: tr("Models：%1").arg(aiAgentClient.availableModelsText)
                            color: qgcPal.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.10)
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            QGCLabel {
                                Layout.fillWidth: true
                                text: tr("消息保留：%1 条").arg(assistantSettings.maxMessages)
                                color: qgcPal.text
                                font.pixelSize: 12
                            }
                            QGCButton {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 30
                                text: "-"
                                onClicked: assistantSettings.maxMessages = Math.max(20, assistantSettings.maxMessages - 20)
                            }
                            QGCButton {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 30
                                text: "+"
                                onClicked: assistantSettings.maxMessages = Math.min(200, assistantSettings.maxMessages + 20)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                text: tr("清空历史")
                                onClicked: root.clearChatHistory()
                            }
                            QGCButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                text: tr("重启Agent")
                                enabled: assistantSettings.agentEnabled
                                onClicked: root.applyAgentProviderSettings()
                            }
                        }

                        QGCButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            text: tr("生成接入说明")
                            onClicked: root.appendMessage("assistant", root._agentGuide)
                        }
                    }
                }
            }

            ListView {
                id: chatList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                model: ListModel {
                    id: chatModel
                    ListElement {
                        role: "assistant"
                        text: "你好，我可以协助查看飞行器状态、解释参数，并把起飞/降落/返航/暂停转换为待确认命令。"
                    }
                }

                delegate: Item {
                    width: chatList.width
                    height: bubble.implicitHeight + 10

                    property bool isUserMessage: model.role === "user"
                    property bool hovered: false

                    Rectangle {
                        id: bubble
                        readonly property real horizontalPadding: 13
                        readonly property real verticalPadding: 10

                        width: Math.min(parent.width * (isUserMessage ? 0.74 : 0.88),
                                        Math.max(126, messageText.implicitWidth + horizontalPadding * 2 + 10))
                        implicitHeight: messageText.implicitHeight + verticalPadding * 2
                        x: isUserMessage ? parent.width - width - 2 : 2
                        y: 3
                        radius: 8
                        color: isUserMessage
                               ? Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, hovered ? 0.98 : 0.88)
                               : Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, hovered ? 0.98 : 0.90)
                        border.color: isUserMessage
                                      ? Qt.rgba(qgcPal.buttonHighlightText.r, qgcPal.buttonHighlightText.g, qgcPal.buttonHighlightText.b, hovered ? 0.35 : 0.18)
                                      : Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, hovered ? 0.34 : 0.16)
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 110 } }
                        Behavior on border.color { ColorAnimation { duration: 110 } }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.leftMargin: 9
                            anchors.rightMargin: 9
                            height: 1
                            color: isUserMessage ? qgcPal.buttonHighlightText : qgcPal.buttonHighlight
                            opacity: hovered ? 0.34 : 0.16

                            Behavior on opacity { NumberAnimation { duration: 110 } }
                        }

                        Rectangle {
                            visible: !isUserMessage
                            width: 3
                            radius: 2
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 6
                            color: qgcPal.buttonHighlight
                            opacity: hovered ? 0.78 : 0.58

                            Behavior on opacity { NumberAnimation { duration: 110 } }
                        }

                        Rectangle {
                            visible: isUserMessage
                            width: 3
                            radius: 2
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 6
                            color: qgcPal.buttonHighlightText
                            opacity: hovered ? 0.58 : 0.38

                            Behavior on opacity { NumberAnimation { duration: 110 } }
                        }

                        QGCLabel {
                            id: messageText
                            anchors.fill: parent
                            anchors.leftMargin: bubble.horizontalPadding + (isUserMessage ? 0 : 6)
                            anchors.rightMargin: bubble.horizontalPadding + (isUserMessage ? 6 : 0)
                            anchors.topMargin: bubble.verticalPadding
                            anchors.bottomMargin: bubble.verticalPadding
                            text: model.text
                            wrapMode: Text.WordWrap
                            lineHeight: 1.18
                            color: isUserMessage ? qgcPal.buttonHighlightText : qgcPal.text
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                            onEntered: hovered = true
                            onExited: hovered = false
                        }
                    }
                }

            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: pendingIntent !== null ? 76 : 0
                visible: pendingIntent !== null
                radius: 7
                color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.12)
                border.color: qgcPal.buttonHighlight

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    QGCLabel {
                        Layout.fillWidth: true
                        text: pendingSummary
                        color: qgcPal.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        QGCButton {
                            Layout.fillWidth: true
                            text: tr("确认建议")
                            onClicked: root.executePendingIntent()
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            text: tr("生成审阅")
                            enabled: !root.fleetPlanningBusy && !(root._fleetReviewTicket && !root._fleetReviewTicket.finished)
                            focusPolicy: Qt.StrongFocus
                            onClicked: root.createFleetReviewForPendingIntent()
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            text: tr("取消")
                            onClicked: root.cancelPendingIntent()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? fleetReviewCard.implicitHeight + 16 : 0
                visible: pendingFleetProposal !== null || pendingFleetReview !== null || root.fleetPlanningBusy
                radius: 7
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.76)
                border.color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.72)

                ColumnLayout {
                    id: fleetReviewCard
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    spacing: 6

                    QGCLabel {
                        Layout.fillWidth: true
                        text: pendingFleetReviewSummary.length > 0 ? pendingFleetReviewSummary
                              : pendingFleetProposal !== null ? tr("该 AI proposal 可转为只读 Fleet OS 审阅草稿，不会执行飞行动作。")
                              : tr("当前共享后台规划不属于此处可见的 AI 提案；此处只显示状态，不接收其他提案的结果。")
                        textFormat: Text.PlainText
                        color: qgcPal.text
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                    QGCLabel {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: root.fleetPlanningStatusText()
                        textFormat: Text.PlainText
                        color: qgcPal.text
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Accessible.name: text
                    }
                    QGCButton {
                        id: aiCancelPlanningButton
                        Layout.fillWidth: true
                        Layout.minimumHeight: 36
                        visible: root.fleetPlanningBusy
                        enabled: !!root.guidedController && typeof root.guidedController.cancelMissionPlanning === "function"
                            && typeof root.fleetPlanningStatus.requestId === "string" && root.fleetPlanningStatus.requestId.length > 0
                            && root.fleetPlanningStatus.status !== "cancelling"
                        focusPolicy: Qt.StrongFocus
                        text: root.fleetPlanningStatus.status === "cancelling" ? tr("正在取消后台规划…") : tr("取消当前后台规划（保留输入）")
                        property var pressedSource: null
                        onPressed: pressedSource = {controller: root.guidedController, requestId: root.fleetPlanningStatus.requestId}
                        onCanceled: pressedSource = null
                        onClicked: {
                            var source = pressedSource
                            pressedSource = null
                            if (!source || source.controller !== root.guidedController || source.requestId !== root.fleetPlanningStatus.requestId) return
                            source.controller.cancelMissionPlanning(source.requestId)
                        }
                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.color: qgcPal.buttonHighlight
                            border.width: 2
                            visible: aiCancelPlanningButton.activeFocus
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        QGCButton {
                            Layout.fillWidth: true
                            focusPolicy: Qt.StrongFocus
                            text: root.fleetPlanningBusy ? tr("后台规划处理中")
                                : pendingFleetReview !== null ? tr("重新生成审阅") : tr("生成舰队审阅草稿")
                            enabled: root.proposalCanCreateFleetReview(root.pendingFleetProposal) && !root.fleetPlanningBusy
                                && !(root._fleetReviewTicket && !root._fleetReviewTicket.finished)
                            onClicked: root.createFleetReviewForProposal()
                        }
                        QGCButton {
                            Layout.fillWidth: true
                            focusPolicy: Qt.StrongFocus
                            text: tr("关闭")
                            onClicked: root.clearPendingFleetProposal()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: 7
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, assistantPanel.inputHovered || inputField.activeFocus ? 0.98 : 0.88)
                border.color: assistantPanel.inputHovered || inputField.activeFocus
                              ? Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.72)
                              : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.20)
                border.width: assistantPanel.inputHovered || inputField.activeFocus ? 2 : 1

                Behavior on color { ColorAnimation { duration: 110 } }
                Behavior on border.color { ColorAnimation { duration: 110 } }
                Behavior on border.width { NumberAnimation { duration: 110 } }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                    onEntered: assistantPanel.inputHovered = true
                    onExited: assistantPanel.inputHovered = false
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    anchors.topMargin: 5
                    anchors.bottomMargin: 5
                    spacing: 8

                    QGCTextField {
                        id: inputField
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: agentRequestRunning ? tr("等待 Agent 响应...")
                                         : assistantSettings.agentEnabled && !aiSupervisor.healthReady ? tr("等待本机智能体就绪...")
                                         : tr("输入指令或问题")
                        enabled: !agentRequestRunning && (!assistantSettings.agentEnabled || aiSupervisor.healthReady)
                        onAccepted: {
                            root.handleUserText(text)
                            text = ""
                        }
                    }
                    QGCButton {
                        Layout.preferredWidth: 62
                        Layout.fillHeight: true
                        text: tr("发送")
                        enabled: !agentRequestRunning && (!assistantSettings.agentEnabled || aiSupervisor.healthReady)
                        onClicked: {
                            root.handleUserText(inputField.text)
                            inputField.text = ""
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: bubbleButton
        anchors.right: parent.right
        anchors.rightMargin: root._bubbleHovered ? 18 : 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root._bubbleHovered ? 18 : 12
        width: root._bubbleHovered ? 56 : 48
        height: width
        radius: width / 2
        visible: !root.expanded
        opacity: root._bubbleHovered ? 1.0 : 0.48
        color: root._bubbleHovered ? qgcPal.buttonHighlight : Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.76)
        border.color: root._bubbleHovered ? qgcPal.buttonHighlightText : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.40)
        border.width: root._bubbleHovered ? 2 : 1

        Behavior on opacity { NumberAnimation { duration: 140 } }
        Behavior on width { NumberAnimation { duration: 140 } }

        QGCColoredImage {
            anchors.centerIn: parent
            width: parent.width * 0.58
            height: width
            source: "qrc:/qml/QGroundControl/FlightDisplay/ai-nine-star.svg"
            color: root._bubbleHovered ? qgcPal.buttonHighlightText : Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.72)
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: root._bubbleHovered = true
            onExited: root._bubbleHovered = false
            onClicked: root.expanded = true
        }

        Rectangle {
            width: 10
            height: 10
            radius: 5
            anchors.right: parent.right
            anchors.top: parent.top
            color: vehicles && vehicles.count > 0 ? qgcPal.colorGreen : qgcPal.colorOrange
            border.color: qgcPal.window
            border.width: 1
        }
    }
}

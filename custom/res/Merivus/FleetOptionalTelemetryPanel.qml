import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Values and explicit Root calls only. No native owner, imported authority,
// automatic capture/apply, or inferred rollback lives in this panel.
FocusScope {
    id: root
    property var controller: null
    property var dialog: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, view: null, opening: null,
        activeCall: null, capture: null, configuration: null, reconciling: false})
    property var captureReceipt: ({})
    property var configurationReceipt: ({})
    property bool callBusy: false
    property bool exhausted: false
    property string vehicleText: "1"
    property string intervalText: "1000000"
    property string localError: ""
    property string resultMessage: ""
    property double clockMs: Date.now()
    property double capturedAtMs: -1
    property double captureDeadlineMs: -1

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var captureState: controller ? controller.fleetOptionalTelemetryCaptureSnapshot || {} : ({})
    readonly property var configurationState: controller ? controller.fleetOptionalTelemetryConfigurationSnapshot || {} : ({})
    readonly property var captureWorkflow: captureState.workflow || ({})
    readonly property var configurationWorkflow: configurationState.workflow || ({})
    readonly property var configurationRun: map(configurationWorkflow.run) ? configurationWorkflow.run : ({})
    readonly property var identity: captureState.operatorIdentity || ({})
    readonly property string stableContext: contextToken()
    readonly property bool sourceReady: !!stableContext && captureKnown(captureWorkflow) && configurationKnown(configurationWorkflow)
    readonly property int targetVehicleId: positiveInput(vehicleText, 255)
    readonly property int targetIntervalUs: positiveInput(intervalText, 2147483647)
    readonly property bool intervalExact: exactReal32(targetIntervalUs)
    readonly property double displayRemainingMs: capturedAtMs >= 0 && clockMs >= capturedAtMs
        ? Math.max(0, captureDeadlineMs - clockMs) : 0
    readonly property bool ownCaptureCurrent: ownedCaptureMatches()
    readonly property bool ownConfigurationCurrent: ownedConfigurationMatches()
    readonly property bool baselineReady: ownCaptureCurrent && captureWorkflow.state === "observations-captured"
        && captureWorkflow.active === false && positiveBaselines(captureWorkflow)
    readonly property bool proposalReady: baselineReady && intervalExact && noIncrease(captureWorkflow, targetIntervalUs)
    readonly property bool nativeActive: captureWorkflow.active === true || configurationWorkflow.active === true
    readonly property bool canAuthenticate: !!dialog && sourceReady && !exhausted && !callBusy && !nativeActive
        && !uuid(captureReceipt.requestId) && !uuid(configurationReceipt.reviewId) && identity.authenticated !== true
        && captureState.busy === false && configurationState.busy === false
        && typeof controller.authenticateFleetOperator === "function"
    readonly property bool canCapture: !!dialog && sourceReady && !exhausted && !callBusy && !nativeActive
        && captureState.busy === false && configurationState.busy === false && targetVehicleId > 0
        && identity.authenticated === true && ["planner", "operator", "safety_officer", "maintenance", "commander", "administrator"].indexOf(identity.role) >= 0
        && typeof controller.requestFleetOptionalTelemetryCapture === "function"
        && typeof controller.cancelFleetOptionalTelemetryCapture === "function"
    readonly property bool canReview: !!dialog && sourceReady && !exhausted && !callBusy && proposalReady
        && configurationWorkflow.active === false && configurationState.busy === false && displayRemainingMs > 0
        && identity.authenticated === true && ["operator", "maintenance", "commander", "administrator"].indexOf(identity.role) >= 0
        && typeof controller.requestFleetOptionalTelemetryConfiguration === "function"
        && typeof controller.cancelFleetOptionalTelemetryConfiguration === "function"
    // projection.busy includes this very review's active flag. It is NOT an
    // approval/cancellation gate; actual Root/native fences remain decisive.
    readonly property bool canApprove: !!dialog && sourceReady && !exhausted && !callBusy
        && ownConfigurationCurrent && configurationWorkflow.active === true && configurationWorkflow.state === "review"
        && integer(configurationWorkflow.remainingMs, 1, 30000) && displayRemainingMs > 0
        && typeof controller.approveFleetOptionalTelemetryConfiguration === "function"

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: entry.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return value && typeof value === "object" && !Array.isArray(value) }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function counter(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,19})$/.test(value) }
    function hash(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知") }
    function yesNo(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function positiveInput(value, maximum) {
        if (typeof value !== "string" || !/^[1-9][0-9]{0,9}$/.test(value)) return 0
        var number = Number(value)
        return integer(number, 1, maximum) ? number : 0
    }
    function exactReal32(value) {
        if (!integer(value, 1, 2147483647)) return false
        var limit = 16777216, step = 1
        while (value > limit) { limit *= 2; step *= 2 }
        return value % step === 0
    }
    function tokenContext(value) {
        if (typeof value !== "string" || !value.length || value.length > 32768) return null
        try {
            var parsed = JSON.parse(value)
            if (!map(parsed) || typeof parsed.scope !== "string" || !parsed.scope.length || parsed.scope.length > 32768
                    || !map(parsed.policy) || Object.keys(parsed.policy).length !== 9) return null
            if (parsed.identity !== null && (!map(parsed.identity) || Object.keys(parsed.identity).length !== 5)) return null
            return parsed
        } catch (error) { return null }
    }
    function contextToken() {
        if (!controller || captureState.schemaVersion !== 1 || configurationState.schemaVersion !== 1
                || captureState.sourceKnown !== true || configurationState.sourceKnown !== true
                || captureState.uiWiringPresent !== true || configurationState.uiWiringPresent !== true
                || captureState.runtimeValidated !== false || configurationState.runtimeValidated !== false) return ""
        var capture = tokenContext(captureState.sourceToken), configuration = tokenContext(configurationState.sourceToken)
        if (!capture || !configuration || capture.scope !== configuration.scope
                || capture.policy.action !== "fleet.telemetry.interval.capture" || configuration.policy.action !== "fleet.telemetry.interval.configure"
                || JSON.stringify(capture.identity) !== JSON.stringify(configuration.identity)) return ""
        // Deliberately exclude our own native generation/ID/state transitions.
        // Full expected sourceToken is still compared immediately before each
        // new request; stable context is NOT a permission or receipt.
        return JSON.stringify({scope: capture.scope, identity: capture.identity,
            capturePolicy: capture.policy, configurationPolicy: configuration.policy})
    }
    function captureKnown(value) {
        if (!map(value) || Object.keys(value).length !== 24 || value.schemaVersion !== 1 || !counter(value.generation)
                || typeof value.state !== "string" || typeof value.reason !== "string" || typeof value.active !== "boolean"
                || !Array.isArray(value.rows) || value.rows.length !== 3 || !Array.isArray(value.messageIds)
                || JSON.stringify(value.messageIds) !== "[62,74,83]" || value.snapshotOnly !== true || value.revalidationRequired !== true) return false
        var denied = ["responseAttributionVerified", "actualTransmitRateVerified", "configurationPermitted",
            "executionPermit", "flightCommandReleased", "permissionsRestored"]
        for (var i = 0; i < denied.length; ++i) if (value[denied[i]] !== false) return false
        for (var r = 0; r < 3; ++r) {
            var row = value.rows[r]
            if (!map(row) || Object.keys(row).length !== 4 || row.messageId !== [62,74,83][r]
                    || typeof row.state !== "string" || typeof row.reason !== "string" || !map(row.query)) return false
        }
        return true
    }
    function configurationKnown(value) {
        if (!map(value) || Object.keys(value).length !== 24 || value.schemaVersion !== 1 || !counter(value.generation)
                || typeof value.state !== "string" || typeof value.reason !== "string" || typeof value.active !== "boolean"
                || !map(value.basisSummary) || !map(value.run) || value.snapshotOnly !== true || value.revalidationRequired !== true
                || (Object.keys(value.basisSummary).length !== 0 && Object.keys(value.basisSummary).length !== 15)
                || (Object.keys(value.run).length !== 0 && Object.keys(value.run).length !== 29)) return false
        var denied = ["authenticated", "responseAttributionVerified", "actualTransmitRateVerified",
            "executionPermit", "flightCommandReleased", "permissionsRestored"]
        for (var i = 0; i < denied.length; ++i) if (value[denied[i]] !== false) return false
        return typeof value.authenticationConsumed === "boolean" && typeof value.configurationRequestsMayRemainInflight === "boolean"
    }
    function configurationRows() {
        if (Object.keys(configurationRun).length !== 29 || !Array.isArray(configurationRun.rows) || configurationRun.rows.length !== 3) return []
        for (var i = 0; i < 3; ++i) {
            var row = configurationRun.rows[i]
            if (!map(row) || Object.keys(row).length !== 9 || row.messageId !== [62,74,83][i]
                    || !map(row.readbackQuery) || !map(row.finalQuery)) return []
        }
        return configurationRun.rows
    }
    function ownedCaptureMatches() {
        var held = captureReceipt
        return map(held) && uuid(held.requestId) && held.owner === controller && held.base === stableContext
            && held.vehicleId === targetVehicleId && captureKnown(captureWorkflow) && captureState.workflowScopeMatches === true
            && captureWorkflow.requestId === held.requestId && captureWorkflow.generation === held.generation
            && captureWorkflow.scopeToken === held.scopeToken && captureWorkflow.vehicleId === held.vehicleId
    }
    function ownedConfigurationMatches() {
        var held = configurationReceipt
        return map(held) && uuid(held.reviewId) && hash(held.bindingHash) && held.owner === controller && held.base === stableContext
            && held.vehicleId === targetVehicleId && held.intervalUs === targetIntervalUs && configurationKnown(configurationWorkflow)
            && configurationState.workflowScopeMatches === true && configurationWorkflow.reviewId === held.reviewId
            && configurationWorkflow.generation === held.generation && configurationWorkflow.bindingHash === held.bindingHash
            && configurationWorkflow.captureId === held.captureId && configurationWorkflow.scopeToken === held.scopeToken
            && configurationWorkflow.vehicleId === held.vehicleId && configurationWorkflow.requestedIntervalUs === held.intervalUs
    }
    function positiveBaselines(value) {
        if (!captureKnown(value)) return false
        for (var i = 0; i < 3; ++i) {
            var query = value.rows[i].query
            if (Object.keys(query).length !== 32 || query.state !== "complete" || query.requestIssued !== true
                    || query.ackObserved !== true || query.ackResult !== 0 || query.reportObserved !== true
                    || !integer(query.intervalUs, 1, 2147483647)) return false
        }
        return true
    }
    function noIncrease(value, interval) {
        if (!positiveBaselines(value) || !exactReal32(interval)) return false
        for (var i = 0; i < 3; ++i) if (interval < value.rows[i].query.intervalUs) return false
        return true
    }
    function pressSnapshot() {
        return {owner: controller, epoch: lifetime.epoch, view: lifetime.view, base: stableContext,
            captureToken: captureState.sourceToken, configurationToken: configurationState.sourceToken,
            vehicleId: targetVehicleId, intervalUs: targetIntervalUs,
            captureId: captureReceipt.requestId || "", reviewId: configurationReceipt.reviewId || "",
            bindingHash: configurationReceipt.bindingHash || ""}
    }
    function pressCurrent(pressed) {
        return pressed && lifetime.alive && !lifetime.exhausted && pressed.owner === controller
            && pressed.epoch === lifetime.epoch && pressed.view && pressed.view === lifetime.view && !!dialog && visible
            && pressed.base === stableContext && pressed.vehicleId === targetVehicleId && pressed.intervalUs === targetIntervalUs
            && pressed.captureId === (captureReceipt.requestId || "") && pressed.reviewId === (configurationReceipt.reviewId || "")
            && pressed.bindingHash === (configurationReceipt.bindingHash || "")
    }
    function clearChecks() { var guard = lifetime, view = dialog; if (guard && guard.alive && view) view.clearConfirmations() }
    function retire(clearCapture, message, preservedCall) {
        var guard = lifetime
        if (!guard || !guard.alive) return
        // Revoke a still-pending Root call even before it has returned an ID.
        // Only capture's exact, not-yet-submitted initial cleanup may spare
        // its own new ticket; nested/explicit retirement never gets that pass.
        if (guard.activeCall && guard.activeCall !== preservedCall && guard.activeCall.cancelState)
            guard.activeCall.cancelState.cancelled = true
        var oldCapture = clearCapture ? guard.capture : null, oldConfiguration = guard.configuration
        // These exact cleanup responsibilities outlive this panel and any
        // notification below. A deleted view cannot strand the detached IDs.
        var cancelDetached = function() {
            if (oldConfiguration && oldConfiguration.owner) try { oldConfiguration.owner.cancelFleetOptionalTelemetryConfiguration(oldConfiguration.reviewId) } catch (error) {}
            if (oldCapture && oldCapture.owner) try { oldCapture.owner.cancelFleetOptionalTelemetryCapture(oldCapture.requestId) } catch (error) {}
        }
        if (clearCapture) guard.capture = null
        guard.configuration = null
        if (guard.epoch >= 9007199254740991) guard.exhausted = true
        else guard.epoch += 1
        var epoch = guard.epoch
        // Drop local authority and confirmations BEFORE any outbound cancel.
        try {
            if (guard.exhausted) exhausted = true
            if (!guard.alive || guard.epoch !== epoch) return
            configurationReceipt = ({})
            if (!guard.alive || guard.epoch !== epoch) return
            if (clearCapture) captureReceipt = ({})
            if (!guard.alive || guard.epoch !== epoch) return
            if (clearCapture) capturedAtMs = -1
            if (!guard.alive || guard.epoch !== epoch) return
            if (clearCapture) captureDeadlineMs = -1
            if (!guard.alive || guard.epoch !== epoch) return
            clearChecks()
        } finally { cancelDetached() }
        if (!guard.alive || guard.epoch !== epoch) return
        if (message && dialog) localError = message
    }
    function reconcile() {
        var guard = lifetime
        if (!guard || !guard.alive || guard.reconciling || guard.activeCall) return
        guard.reconciling = true
        try {
            if ((guard.capture && !ownCaptureCurrent) || (guard.configuration && !ownConfigurationCurrent))
                retire(true, qsTr("原来源或本页回执已变化；只取消本页操作，不接管其他入口。"))
        } finally { if (guard.alive) guard.reconciling = false }
    }
    function sourceChanged() {
        var guard = lifetime
        if (!guard || !guard.alive) return
        // Primary authentication necessarily changes identity. It creates no
        // capture/configuration receipt and Root audits the actual Episode.
        if (guard.activeCall && guard.activeCall.kind === "authenticate" && !guard.capture && !guard.configuration) return
        if (guard.capture || guard.configuration || guard.activeCall)
            retire(true, qsTr("任务组、账号或政策已变化；原确认已撤销，请重新采集。"))
        else clearChecks()
    }
    function finishCall(guard, ticket) {
        if (!guard.alive || guard.activeCall !== ticket) return
        callBusy = false
        if (!guard.alive || guard.activeCall !== ticket) return
        guard.activeCall = null
        reconcile()
    }
    function authenticate(pressed) {
        var guard = lifetime
        if (guard.activeCall || !canAuthenticate || !pressCurrent(pressed)) return
        var owner = controller, epoch = guard.epoch, view = guard.view, ticket = {kind: "authenticate"}, result = {}
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            try { result = owner.authenticateFleetOperator() || {} } catch (error) { result = {reason: "operator-authentication-unavailable"} }
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller) return
            resultMessage = result.ok === true ? qsTr("身份已返回；请核对下方账号和角色后再采集。") : qsTr("认证未完成：%1").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function capture(pressed, consent) {
        var guard = lifetime
        if (guard.activeCall || !canCapture || !pressCurrent(pressed) || consent !== true || pressed.captureToken !== captureState.sourceToken) return
        var owner = controller, expected = pressed.captureToken, base = pressed.base, vehicleId = pressed.vehicleId
        var started = Date.now(), result = {}, ticket = {kind: "capture", cancelState: {cancelled: false}}, view = guard.view
        var originalEpoch = guard.epoch, epoch = originalEpoch + 1
        var abandon = function(value) {
            var id = value && value.requestId
            if (typeof id === "string" && /^[0-9a-f-]{36}$/.test(id)) try { owner.cancelFleetOptionalTelemetryCapture(id) } catch (error) {}
        }
        guard.activeCall = ticket
        try {
            // Keep the original ticket across our own one retirement. An
            // extra cancellation/input ABA cannot lend us a newer epoch.
            retire(true, "", ticket)
            if (!guard.alive || guard.activeCall !== ticket || guard.epoch !== epoch || guard.view !== view
                    || owner !== controller || stableContext !== base || expected !== captureState.sourceToken
                    || pressed.vehicleId !== targetVehicleId || pressed.intervalUs !== targetIntervalUs) return
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || stableContext !== base) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || expected !== captureState.sourceToken) return
            try { result = owner.requestFleetOptionalTelemetryCapture(expected, vehicleId, true, ticket.cancelState) || {} } catch (error) { result = {reason: "capture-call-unavailable"} }
            if (!guard.alive) { abandon(result); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || stableContext !== base || !dialog || !visible) { abandon(result); return }
            var current = captureWorkflow
            if (Object.keys(result).length !== 5 || result.ok !== true || result.requestAccepted !== true || !uuid(result.requestId)
                    || result.flightCommandReleased !== false || !captureKnown(current) || current.requestId !== result.requestId
                    || current.vehicleId !== vehicleId || current.active !== true || captureState.workflowScopeMatches !== true) {
                abandon(result)
                if (guard.alive && guard.epoch === epoch) localError = qsTr("未建立本页采集：%1。检查原生状态后手动重试。").arg(text(result.reason))
                return
            }
            var held = {owner: owner, base: base, requestId: result.requestId, generation: current.generation,
                scopeToken: current.scopeToken, vehicleId: vehicleId}
            guard.capture = held
            capturedAtMs = started
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) { abandon(result); return }
            captureDeadlineMs = started + 30000
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) { abandon(result); return }
            captureReceipt = held
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            clockMs = Date.now()
            if (guard.alive && guard.epoch === epoch) resultMessage = qsTr("只读采集已接纳；没有发送511配置，请等三条报告后另行审阅。")
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function requestReview(pressed, consent) {
        var guard = lifetime
        if (guard.activeCall || !canReview || !pressCurrent(pressed) || consent !== true || pressed.configurationToken !== configurationState.sourceToken) return
        var owner = controller, base = stableContext, expected = pressed.configurationToken, captureId = pressed.captureId
        var interval = pressed.intervalUs, vehicleId = pressed.vehicleId, epoch = guard.epoch, view = guard.view
        var ticket = {kind: "review", cancelState: {cancelled: false}}, result = {}
        var abandon = function(value) {
            var id = value && value.reviewId
            if (typeof id === "string" && /^[0-9a-f-]{36}$/.test(id)) try { owner.cancelFleetOptionalTelemetryConfiguration(id) } catch (error) {}
        }
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || owner !== controller || stableContext !== base) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || expected !== configurationState.sourceToken) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownCaptureCurrent) return
            try { result = owner.requestFleetOptionalTelemetryConfiguration(expected, captureId, interval, true, ticket.cancelState) || {} } catch (error) { result = {reason: "configuration-review-call-unavailable"} }
            if (!guard.alive) { abandon(result); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || stableContext !== base || !dialog || !visible) { abandon(result); return }
            var current = configurationWorkflow
            if (Object.keys(result).length !== 5 || result.ok !== true || result.requestAccepted !== true || !uuid(result.reviewId)
                    || result.flightCommandReleased !== false || !configurationKnown(current) || current.reviewId !== result.reviewId
                    || !hash(current.bindingHash) || current.state !== "review" || current.active !== true || current.captureId !== captureId
                    || current.vehicleId !== vehicleId || current.requestedIntervalUs !== interval || configurationState.workflowScopeMatches !== true) {
                abandon(result)
                if (guard.alive && guard.epoch === epoch) localError = qsTr("审阅未建立：%1。原基线可能已消费或到期，请重新采集；不自动重试。").arg(text(result.reason))
                return
            }
            var held = {owner: owner, base: base, reviewId: result.reviewId, bindingHash: current.bindingHash,
                generation: current.generation, scopeToken: current.scopeToken, captureId: captureId, vehicleId: vehicleId, intervalUs: interval}
            guard.configuration = held
            configurationReceipt = held
            if (guard.alive && guard.epoch === epoch) resultMessage = qsTr("审阅已建立，尚未配置。核对三条原间隔和统一目标后，另行独立审批。")
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function approve(pressed, valuesChecked, riskChecked) {
        var guard = lifetime
        if (guard.activeCall || !canApprove || !pressCurrent(pressed) || valuesChecked !== true || riskChecked !== true) return
        var held = guard.configuration, owner = controller, base = stableContext, epoch = guard.epoch, view = guard.view
        if (!held || held.reviewId !== pressed.reviewId || held.bindingHash !== pressed.bindingHash) return
        var ticket = {kind: "approve"}, result = {}
        var abandon = function() { try { owner.cancelFleetOptionalTelemetryConfiguration(held.reviewId) } catch (error) {} }
        guard.activeCall = ticket
        try {
            callBusy = true
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || stableContext !== base) return
            clearChecks()
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownConfigurationCurrent) return
            localError = ""
            if (!guard.alive || guard.epoch !== epoch || guard.view !== view || !ownConfigurationCurrent) return
            try { result = owner.approveFleetOptionalTelemetryConfiguration(held.reviewId, held.bindingHash, true) || {} } catch (error) { result = {reason: "configuration-approval-call-unavailable"} }
            if (!guard.alive) { abandon(); return }
            if (guard.epoch !== epoch || guard.view !== view || owner !== controller || stableContext !== base || !dialog || !visible) { abandon(); return }
            var accepted = Object.keys(result).length === 5 && result.ok === true && result.requestAccepted === true
                && result.reviewId === held.reviewId && result.flightCommandReleased === false && ownConfigurationCurrent
                && configurationWorkflow.state === "applying" && configurationWorkflow.active === true
                && configurationWorkflow.authenticationConsumed === true
            if (!accepted) {
                abandon()
                if (!guard.alive || guard.epoch !== epoch || guard.view !== view) return
            }
            resultMessage = accepted
                ? qsTr("原生审批返回接纳，后续状态以逐项配置和回读为准；这不是已生效或已验证成功。")
                : qsTr("审批未接纳或后置核验失效：%1。查看已发送事实；不推断回滚，不自动重试。").arg(text(result.reason))
        } finally { if (guard.alive) finishCall(guard, ticket) }
    }
    function ensureFocusVisible(item) {
        if (!dialog || !item) return
        var point = item.mapToItem(dialog.scrollContent, 0, 0), scroll = dialog.scrollArea
        if (point.y - pad < scroll.contentY) scroll.contentY = Math.max(0, point.y - pad)
        else if (point.y + item.height + pad > scroll.contentY + scroll.height)
            scroll.contentY = Math.min(Math.max(0, scroll.contentHeight - scroll.height), point.y + item.height + pad - scroll.height)
    }
    function finishView(view) {
        var guard = lifetime
        if (!guard || !guard.alive || dialog !== view) return
        guard.view = null
        retire(true, "")
        if (!guard.alive) return
        dialog = null
        if (guard.alive && visible) entryButton.forceActiveFocus()
    }
    function dismissView() {
        var guard = lifetime, view = dialog
        if (!guard || !guard.alive) return
        guard.view = null
        retire(true, "")
        if (!guard.alive) return
        dialog = null
        if (view) try { view.close() } catch (error) {}
    }
    function openView() {
        var guard = lifetime
        if (!guard || !guard.alive || guard.exhausted || guard.opening || guard.activeCall || !controller || !visible || dialog) return
        var opening = {}, owner = controller, view = null
        guard.opening = opening
        try {
            localError = ""
            if (!guard.alive || guard.opening !== opening) return
            view = viewComponent.createObject(mainWindow, {ownerPanel: root, guard: guard})
            if (!guard.alive) { if (view) view.destroy(); return }
            if (!view || owner !== controller || !visible || dialog) { if (view) view.destroy(); return }
            guard.view = opening
            dialog = view
            if (!guard.alive || guard.view !== opening) { view.destroy(); return }
            view.open()
        } finally { if (guard.opening === opening) guard.opening = null }
    }
    function intervalLabel(value) { return integer(value, 1, 2147483647) ? String(value) + qsTr(" 微秒（报告值）") : value === 0 ? qsTr("0：报告不可提供") : value === -1 ? qsTr("−1：报告停用") : qsTr("未知／无报告") }
    function messageName(id) { return id === 62 ? "62 · NAV_CONTROLLER_OUTPUT" : id === 74 ? "74 · VFR_HUD" : id === 83 ? "83 · ATTITUDE_TARGET" : qsTr("未知流") }
    function stateText(state) {
        var states = {"empty": qsTr("尚未开始"), "queued": qsTr("等待只读采集"), "capturing": qsTr("正在只读采集"),
            "observations-captured": qsTr("三条报告已采集"), "preparing": qsTr("准备审阅"), "review": qsTr("等待人工独立审批"),
            "approving": qsTr("独立账号核验中"), "applying": qsTr("配置与回读中"), "reported-values-match": qsTr("报告值匹配（非实际频率验证）"),
            "failed": qsTr("失败／停止"), "stale": qsTr("来源失效"), "expired": qsTr("期限已到"), "cancelled": qsTr("已取消后续步骤")}
        return states[state] || text(state)
    }
    function fullJson(value) { try { return JSON.stringify(value, null, 2) } catch (error) { return qsTr("快照无法显示；不可据此操作。") } }

    onStableContextChanged: sourceChanged()
    onCaptureWorkflowChanged: reconcile()
    onConfigurationWorkflowChanged: reconcile()
    onVehicleTextChanged: retire(true, "")
    onIntervalTextChanged: retire(false, "")
    onControllerChanged: dismissView()
    onVisibleChanged: if (!visible) dismissView()
    onLocalErrorChanged: if (localError && dialog) dialog.focusError()
    Component.onDestruction: {
        var guard = lifetime, view = dialog, capture = guard.capture, configuration = guard.configuration
        if (guard.activeCall && guard.activeCall.cancelState) guard.activeCall.cancelState.cancelled = true
        guard.capture = null; guard.configuration = null; guard.alive = false; guard.view = null
        if (configuration && configuration.owner) try { configuration.owner.cancelFleetOptionalTelemetryConfiguration(configuration.reviewId) } catch (error) {}
        if (capture && capture.owner) try { capture.owner.cancelFleetOptionalTelemetryCapture(capture.requestId) } catch (error) {}
        if (view) try { view.close() } catch (error) {}
    }
    Timer {
        interval: 250; repeat: true; running: !!root.dialog || root.callBusy
        onTriggered: {
            var guard = root.lifetime, now = Date.now()
            if (now < root.clockMs) {
                root.retire(true, qsTr("显示时钟回退；已取消本页操作，请手动重新采集。"))
                if (!guard.alive) return
            }
            root.clockMs = now
            if (!guard.alive) return
            if (guard.capture && root.displayRemainingMs <= 0)
                root.retire(true, qsTr("原30秒显示期限已到；不续期或自动重试。请查看历史并重新采集。"))
        }
    }

    ColumnLayout {
        id: entry
        width: root.width; spacing: root.pad / 2
        QGCButton {
            id: entryButton
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            text: qsTr("可选遥测间隔 · 地面采集与独立配置")
            Accessible.name: text; enabled: !!root.controller && !root.callBusy && !root.exhausted
            wrapMode: Text.WordWrap
            onClicked: root.openView()
            contentItem: QGCLabel { text: entryButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: entryButton.enabled ? 1 : 0.5; color: entryButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
            background: Rectangle { color: entryButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: entryButton.activeFocus ? 3 : 1; radius: 3 }
        }
        QGCLabel {
            Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
            text: qsTr("固定三条可选流；只读采集后另行双账号审批，不自动降频或回滚。")
        }
        QGCLabel {
            Layout.fillWidth: true; visible: root.configurationWorkflow.configurationRequestsMayRemainInflight === true
            textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
            text: qsTr("已有请求可能发出；关闭或取消只阻止后续步骤，不表示远端已撤回。")
        }
    }

    Component {
        id: viewComponent
        Controls.Dialog {
            id: view
            property var ownerPanel: null
            property var guard: null
            property var viewLife: ({alive: true})
            property string detailText: ""
            property alias scrollArea: scroll
            property alias scrollContent: form
            parent: Controls.Overlay.overlay
            anchors.centerIn: parent
            modal: true; focus: true; closePolicy: Controls.Popup.NoAutoClose
            standardButtons: Controls.Dialog.NoButton; padding: root.pad
            width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 66, mainWindow.width - root.pad * 4))
            height: Math.max(1, Math.min(form.implicitHeight + heading.implicitHeight + root.pad * 4, mainWindow.height - root.pad * 4))
            background: Rectangle { color: qgcPal.window; border.color: qgcPal.text; border.width: 1; radius: 3 }
            header: QGCLabel { id: heading; padding: root.pad; text: qsTr("可选遥测配置 · 只读采集 → 审阅 → 独立审批"); font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
            function clearConfirmations() {
                var life = guard, ownLife = viewLife
                if (!life || !life.alive || !ownLife.alive) return
                captureConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                reviewConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                valuesConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                riskConsent.checked = false
                if (!life.alive || !ownLife.alive) return
                captureButton.pressScope = ({})
                if (!life.alive || !ownLife.alive) return
                reviewButton.pressScope = ({})
                if (!life.alive || !ownLife.alive) return
                approveButton.pressScope = ({})
            }
            function focusError() {
                if (!guard || !guard.alive || !viewLife.alive || !errorSummary.visible) return
                errorSummary.forceActiveFocus()
                if (guard.alive && viewLife.alive && ownerPanel) ownerPanel.ensureFocusVisible(errorSummary)
            }
            onOpened: closeButton.forceActiveFocus()
            onAboutToHide: if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view)
            onClosed: {
                var actualView = view
                viewLife.alive = false
                if (guard && guard.alive && ownerPanel) ownerPanel.finishView(actualView)
                try { actualView.destroy() } catch (error) {}
            }
            Component.onDestruction: { viewLife.alive = false; if (guard && guard.alive && ownerPanel) ownerPanel.finishView(view) }
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
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("取消本页后续操作 / 关闭（不回滚已发送配置）"); Accessible.name: text
                        onClicked: view.close()
                        Keys.onReturnPressed: { view.close(); event.accepted = true }
                        Keys.onEnterPressed: { view.close(); event.accepted = true }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(closeButton)
                        contentItem: QGCLabel { text: closeButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: closeButton.enabled ? 1 : 0.5; color: closeButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: closeButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: closeButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: qsTr("仅地面、未解锁且原生证据满足时可操作。仅62／74／83三流使用统一正间隔，不启停流、不降频P0。采集、审批和九次配置／回读共用最初30秒；慢链路可能无法完成。取消不撤回已发送字节，不自动重试或回滚。")
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("账号：%1 · 角色：%2\n采集：%3\n配置：%4\n原30秒显示剩余：%5 秒（只会提前截止；原生单调期限为准）")
                            .arg(root.text(root.identity.actorId)).arg(root.text(root.identity.role))
                            .arg(root.stateText(root.captureWorkflow.state)).arg(root.stateText(root.configurationWorkflow.state))
                            .arg(Math.floor(root.displayRemainingMs / 1000))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; visible: !root.sourceReady; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: qsTr("当前任务组／账号来源或固定状态合同不可核实；操作已禁用。不会从显示值或历史恢复权限。")
                    }
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
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("认证本地操作员"); Accessible.name: text; enabled: root.canAuthenticate
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, pressed = pressScope; pressScope = ({}); if (life && life.alive && view.viewLife.alive) root.authenticate(pressed) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(authenticateButton)
                        contentItem: QGCLabel { text: authenticateButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: authenticateButton.enabled ? 1 : 0.5; color: authenticateButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: authenticateButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: authenticateButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("1 · 选择目标并只读采集") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("MAVLink System ID（1–255，不是飞机显示名称）") }
                    QGCTextField {
                        id: vehicleInput
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.vehicleText; numericValuesOnly: true; maximumLength: 3; enabled: !root.callBusy && !root.nativeActive
                        Accessible.name: qsTr("目标MAVLink System ID，1到255")
                        onTextChanged: if (text !== root.vehicleText) root.vehicleText = text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(vehicleInput)
                    }
                    QGCLabel { Layout.fillWidth: true; visible: root.targetVehicleId === 0; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("请输入1–255的十进制整数，不接受符号、空白、小数或前导零。") }
                    Controls.CheckBox {
                        id: captureConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("仅向上方机号读取三条间隔报告，此步不配置"); checked: false; enabled: root.canCapture; Accessible.name: text
                        contentItem: QGCLabel { text: captureConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (captureConsent.indicator ? captureConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(captureConsent)
                    }
                    QGCButton {
                        id: captureButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("开始新的只读采集（62 → 74 → 83）"); Accessible.name: text; enabled: root.canCapture && captureConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, pressed = pressScope, checked = captureConsent.checked; pressScope = ({}); if (life && life.alive && view.viewLife.alive) root.capture(pressed, checked) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(captureButton)
                        contentItem: QGCLabel { text: captureButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: captureButton.enabled ? 1 : 0.5; color: captureButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: captureButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: captureButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: root.ownCaptureCurrent ? qsTr("本页持有此采集回执；报告仍须原生核验，不能证明实际频率或带宽。")
                            : qsTr("下方为共享状态／历史，只读查看。本页不从观察到的ID接管其他入口。")
                    }
                    Repeater {
                        model: root.captureKnown(root.captureWorkflow) ? root.captureWorkflow.rows : []
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: root.pad / 2
                            readonly property var row: modelData
                            QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.messageName(parent.row.messageId) }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                                text: qsTr("阶段：%1 · 间隔：%2\n查询可能发出：%3 · ACK观察到：%4\n原因：%5")
                                    .arg(root.text(parent.row.state)).arg(root.intervalLabel(parent.row.query.intervalUs))
                                    .arg(root.yesNo(parent.row.query.requestIssued)).arg(root.yesNo(parent.row.query.ackObserved)).arg(root.text(parent.row.reason))
                            }
                        }
                    }
                    QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("2 · 审阅统一目标（此步仍不配置）") }
                    QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("三流统一目标间隔（微秒，数值越大频率越低）") }
                    QGCTextField {
                        id: intervalInput
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight
                        text: root.intervalText; numericValuesOnly: true; maximumLength: 10; enabled: !root.callBusy && !root.nativeActive
                        Accessible.name: qsTr("三条可选流统一目标间隔，单位微秒")
                        onTextChanged: if (text !== root.intervalText) root.intervalText = text
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(intervalInput)
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText
                        text: !root.intervalExact ? qsTr("目标必须是1–2147483647且可精确编码为REAL32的整数微秒；0不是停流选项。")
                            : root.baselineReady && !root.proposalReady ? qsTr("目标间隔不得小于任一原正基线；本页不允许增频。")
                            : qsTr("必须本页采集到三条正基线；−1、0和未知报告不能配置。修改间隔会清除原配置审阅，不延长30秒。")
                    }
                    Controls.CheckBox {
                        id: reviewConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("已核对三条原报告与统一目标，仅建立审阅；知道基线一次接管后不能重用")
                        checked: false; enabled: root.canReview; Accessible.name: text
                        contentItem: QGCLabel { text: reviewConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (reviewConsent.indicator ? reviewConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(reviewConsent)
                    }
                    QGCButton {
                        id: reviewButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("建立本页配置审阅（不发送511）"); Accessible.name: text; enabled: root.canReview && reviewConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, pressed = pressScope, checked = reviewConsent.checked; pressScope = ({}); if (life && life.alive && view.viewLife.alive) root.requestReview(pressed, checked) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(reviewButton)
                        contentItem: QGCLabel { text: reviewButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: reviewButton.enabled ? 1 : 0.5; color: reviewButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: reviewButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: reviewButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("3 · 独立审批后逐项配置\n本页审阅：%1\n目标机号：%2 · 目标间隔：%3\n绑定摘要：%4")
                            .arg(root.text(root.configurationReceipt.reviewId)).arg(root.configurationReceipt.vehicleId || qsTr("未知"))
                            .arg(root.intervalLabel(root.configurationReceipt.intervalUs)).arg(root.text(root.configurationReceipt.bindingHash))
                    }
                    Controls.CheckBox {
                        id: valuesConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("已核对本页机号、三条正基线和统一目标，允许独立二审后写入")
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: valuesConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (valuesConsent.indicator ? valuesConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(valuesConsent)
                    }
                    Controls.CheckBox {
                        id: riskConsent
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: qsTr("已知可能部分生效、没有自动重试／回滚；报告值匹配不证明真实发布率或带宽")
                        checked: false; enabled: root.canApprove; Accessible.name: text
                        contentItem: QGCLabel { text: riskConsent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: (riskConsent.indicator ? riskConsent.indicator.width : 0) + root.pad }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(riskConsent)
                    }
                    QGCButton {
                        id: approveButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("不同SID独立审批 → 逐项配置与回读"); Accessible.name: text
                        enabled: root.canApprove && valuesConsent.checked && riskConsent.checked
                        property var pressScope: ({})
                        onPressed: pressScope = root.pressSnapshot()
                        onCanceled: pressScope = ({})
                        onClicked: { var life = view.guard, pressed = pressScope, values = valuesConsent.checked, risk = riskConsent.checked; pressScope = ({}); if (life && life.alive && view.viewLife.alive) root.approve(pressed, values, risk) }
                        Keys.onReturnPressed: event.accepted = true
                        Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(approveButton)
                        contentItem: QGCLabel { text: approveButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: approveButton.enabled ? 1 : 0.5; color: approveButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: approveButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: approveButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText
                        text: qsTr("该次配置原因：%1\n该次已消费独立身份：%2 · 请求可能已发：%3\n该次快照采集时来源检查：%4 · 该次报告值匹配：%5（均非持续核验）\n%6")
                            .arg(root.text(root.configurationWorkflow.reason)).arg(root.yesNo(root.configurationWorkflow.authenticationConsumed))
                            .arg(root.yesNo(root.configurationWorkflow.configurationRequestsMayRemainInflight))
                            .arg(root.yesNo(root.configurationRun.sourceCurrent)).arg(root.yesNo(root.configurationRun.reportedValuesMatch))
                            .arg(root.resultMessage || qsTr("尚无本页操作结果。所有实际频率／带宽／飞行许可均未认证。"))
                    }
                    QGCLabel {
                        Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap
                        text: root.ownConfigurationCurrent ? qsTr("本页持有原审阅；配置状态不是完成验收。")
                            : qsTr("配置状态可能来自其他入口或历史；本页只能查看，不能批准或恢复权限。")
                    }
                    Repeater {
                        model: root.configurationRows()
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: root.pad / 2
                            readonly property var row: modelData
                            QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.messageName(parent.row.messageId) }
                            QGCLabel {
                                Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                                text: qsTr("原间隔：%1\n设置可能发出：%2 · ACK观察到：%3 · ACK结果：%4\n中间回读：%5 · 最终回读：%6 · 最终报告匹配：%7")
                                    .arg(root.intervalLabel(parent.row.originalIntervalUs)).arg(root.yesNo(parent.row.setRequestIssued))
                                    .arg(root.yesNo(parent.row.setAckObserved)).arg(parent.row.setAckResult === null ? qsTr("未知") : String(parent.row.setAckResult))
                                    .arg(root.intervalLabel(parent.row.readbackQuery.intervalUs)).arg(root.intervalLabel(parent.row.finalQuery.intervalUs))
                                    .arg(root.yesNo(parent.row.reportedValueMatches))
                            }
                        }
                    }
                    QGCButton {
                        id: cancelButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("取消 / 清除本页确认（不撤回或回滚已发配置）"); Accessible.name: text
                        enabled: root.uuid(root.captureReceipt.requestId) || root.uuid(root.configurationReceipt.reviewId) || root.callBusy
                        onClicked: root.retire(true, qsTr("本页后续操作已取消；已发送和部分结果仍以共享原生历史为准。"))
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(cancelButton)
                        contentItem: QGCLabel { text: cancelButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: cancelButton.enabled ? 1 : 0.5; color: cancelButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: cancelButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: cancelButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    QGCButton {
                        id: detailsButton
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; wrapMode: Text.WordWrap
                        text: qsTr("读取当前详情快照（只读，不自动刷新）"); Accessible.name: text
                        onClicked: view.detailText = root.fullJson({capture: root.captureWorkflow, configuration: root.configurationWorkflow})
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(detailsButton)
                        contentItem: QGCLabel { text: detailsButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; opacity: detailsButton.enabled ? 1 : 0.5; color: detailsButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                        background: Rectangle { color: detailsButton._showHighlight ? qgcPal.buttonHighlight : qgcPal.button; border.color: qgcPal.buttonText; border.width: detailsButton.activeFocus ? 3 : 1; radius: 3 }
                    }
                    Controls.TextArea {
                        Layout.fillWidth: true; visible: !!view.detailText
                        readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap
                        text: view.detailText; color: qgcPal.text
                        Accessible.name: qsTr("手动取得的完整详情历史快照，不是当前授权")
                        background: Rectangle { color: qgcPal.windowShade }
                        onActiveFocusChanged: if (activeFocus && view.ownerPanel) view.ownerPanel.ensureFocusVisible(this)
                    }
                }
            }
        }
    }
}
